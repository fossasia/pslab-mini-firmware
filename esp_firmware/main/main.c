#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_slave.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"

#define FRAME_LEN 512
#define HEADER_LEN 12
#define MAX_PAYLOAD_LEN (FRAME_LEN - HEADER_LEN)
#define FRAME_MAGIC 0xa5
#define FRAME_TYPE_POLL 0x00
#define FRAME_TYPE_DATA 0x02
#define FRAME_TYPE_SCPI 0x03
#define SLOT_COUNT 12
#define SPI_QUEUED_TRANSFERS 4
#define UDP_BATCH_FRAMES 2
#define UDP_PACKET_LEN (FRAME_LEN * UDP_BATCH_FRAMES)
#define SCPI_QUEUE_DEPTH 8
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static char const *const TAG = "esp_spi_udp_bridge";

typedef struct {
    uint8_t *tx;
    uint8_t *rx;
    spi_slave_transaction_t trans;
    uint32_t sequence;
} bridge_slot_t;

typedef struct {
    uint16_t len;
    uint8_t data[MAX_PAYLOAD_LEN];
} bridge_message_t;

static EventGroupHandle_t wifi_event_group;
static QueueHandle_t free_queue;
static QueueHandle_t full_queue;
static QueueHandle_t scpi_rx_queue;
static QueueHandle_t scpi_tx_queue;
static bridge_slot_t slots[SLOT_COUNT];
static int sta_retry_count;

static uint8_t checksum8(uint8_t const *data, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum = (uint8_t)(sum + data[i]);
    }
    return sum;
}

static uint16_t get_u16_le(uint8_t const *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t get_u32_le(uint8_t const *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

typedef struct {
    uint32_t magic;
    uint32_t type;
    uint32_t length;
    uint32_t checksum;
} frame_error_stats_t;

static void put_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void put_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void prepare_frame(
    uint8_t *frame,
    uint8_t frame_type,
    uint32_t seq,
    uint8_t const *payload,
    uint16_t payload_len
)
{
    memset(frame, 0, FRAME_LEN);
    frame[0] = FRAME_MAGIC;
    frame[1] = frame_type;
    put_u32_le(&frame[2], seq);
    put_u16_le(&frame[6], payload_len);
    frame[8] = checksum8(frame, 8);
    if (payload && payload_len > 0) {
        memcpy(&frame[HEADER_LEN], payload, payload_len);
    }
}

static void prepare_poll_frame(uint8_t *frame, uint32_t seq)
{
    prepare_frame(frame, FRAME_TYPE_POLL, seq, NULL, 0);
}

static uint32_t validate_rx_frame(
    uint8_t const *frame,
    uint8_t *type_out,
    uint32_t *seq_out,
    uint16_t *len_out,
    frame_error_stats_t *stats
)
{
    uint32_t errors = 0;
    if (frame[0] != FRAME_MAGIC) {
        ++errors;
        if (stats) {
            ++stats->magic;
        }
    }
    if (frame[1] != FRAME_TYPE_DATA && frame[1] != FRAME_TYPE_SCPI &&
        frame[1] != FRAME_TYPE_POLL) {
        ++errors;
        if (stats) {
            ++stats->type;
        }
    }
    uint16_t len = get_u16_le(&frame[6]);
    if (len > MAX_PAYLOAD_LEN) {
        ++errors;
        if (stats) {
            ++stats->length;
        }
        len = 0;
    }
    if (frame[8] != checksum8(frame, 8)) {
        ++errors;
        if (stats) {
            ++stats->checksum;
        }
    }

    *type_out = frame[1];
    *seq_out = get_u32_le(&frame[2]);
    *len_out = len;
    return errors;
}

static void set_ready(bool ready)
{
    gpio_set_level(CONFIG_ESP_BRIDGE_PIN_READY, ready ? 1 : 0);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (sta_retry_count < 10) {
            ++sta_retry_count;
            ESP_LOGI(TAG, "retrying station connection");
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "station IP: " IPSTR, IP2STR(&event->ip_info.ip));
        sta_retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void start_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t sta_config = {0};
    strlcpy((char *)sta_config.sta.ssid, CONFIG_ESP_BRIDGE_STA_SSID, sizeof(sta_config.sta.ssid));
    strlcpy((char *)sta_config.sta.password, CONFIG_ESP_BRIDGE_STA_PASSWORD, sizeof(sta_config.sta.password));
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000)
    );
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "station connected");
    } else {
        ESP_LOGW(TAG, "station connection not established yet");
    }
}

static void init_spi(void)
{
    gpio_config_t ready_config = {
        .pin_bit_mask = 1ULL << CONFIG_ESP_BRIDGE_PIN_READY,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&ready_config));
    set_ready(false);

    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_ESP_BRIDGE_PIN_MOSI,
        .miso_io_num = CONFIG_ESP_BRIDGE_PIN_MISO,
        .sclk_io_num = CONFIG_ESP_BRIDGE_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = FRAME_LEN,
    };

    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = CONFIG_ESP_BRIDGE_PIN_CS,
        .queue_size = SPI_QUEUED_TRANSFERS,
        .flags = 0,
    };

    ESP_ERROR_CHECK(spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
}

static void init_slots(void)
{
    free_queue = xQueueCreate(SLOT_COUNT, sizeof(uint32_t));
    full_queue = xQueueCreate(SLOT_COUNT, sizeof(uint32_t));
    scpi_rx_queue = xQueueCreate(SCPI_QUEUE_DEPTH, sizeof(bridge_message_t));
    scpi_tx_queue = xQueueCreate(SCPI_QUEUE_DEPTH, sizeof(bridge_message_t));
    ESP_ERROR_CHECK((free_queue && full_queue && scpi_rx_queue && scpi_tx_queue) ?
        ESP_OK : ESP_ERR_NO_MEM);

    for (uint32_t i = 0; i < SLOT_COUNT; ++i) {
        slots[i].tx = heap_caps_malloc(FRAME_LEN, MALLOC_CAP_DMA);
        slots[i].rx = heap_caps_malloc(FRAME_LEN, MALLOC_CAP_DMA);
        ESP_ERROR_CHECK((slots[i].tx && slots[i].rx) ? ESP_OK : ESP_ERR_NO_MEM);
        memset(&slots[i].trans, 0, sizeof(slots[i].trans));
        slots[i].trans.length = FRAME_LEN * 8;
        slots[i].trans.tx_buffer = slots[i].tx;
        slots[i].trans.rx_buffer = slots[i].rx;
        slots[i].trans.user = (void *)(uintptr_t)i;
        prepare_poll_frame(slots[i].tx, i);
        memset(slots[i].rx, 0, FRAME_LEN);
        slots[i].sequence = 0;
    }
}

static void queue_slot(uint32_t slot_index, uint32_t poll_seq)
{
    bridge_message_t scpi_msg;
    if (xQueueReceive(scpi_rx_queue, &scpi_msg, 0) == pdTRUE) {
        prepare_frame(
            slots[slot_index].tx,
            FRAME_TYPE_SCPI,
            poll_seq,
            scpi_msg.data,
            scpi_msg.len
        );
    } else {
        prepare_poll_frame(slots[slot_index].tx, poll_seq);
    }
    memset(slots[slot_index].rx, 0, FRAME_LEN);
    ESP_ERROR_CHECK(spi_slave_queue_trans(SPI2_HOST, &slots[slot_index].trans, pdMS_TO_TICKS(1000)));
}

static void refill_spi_queue(uint32_t *queued, uint32_t *poll_seq)
{
    while (*queued < SPI_QUEUED_TRANSFERS) {
        uint32_t free_slot = 0;
        TickType_t wait = (*queued == 0) ? portMAX_DELAY : 0;
        if (xQueueReceive(free_queue, &free_slot, wait) != pdTRUE) {
            break;
        }
        queue_slot(free_slot, (*poll_seq)++);
        ++(*queued);
    }
    set_ready(*queued > 0);
}

static bool udp_endpoint_changed(struct sockaddr_in const *a, struct sockaddr_in const *b)
{
    return a->sin_addr.s_addr != b->sin_addr.s_addr || a->sin_port != b->sin_port;
}

static void log_udp_endpoint(char const *label, struct sockaddr_in const *addr)
{
    char host_ip[16];
    inet_ntoa_r(addr->sin_addr, host_ip, sizeof(host_ip));
    ESP_LOGI(TAG, "%s %s:%u", label, host_ip, ntohs(addr->sin_port));
}

static int send_udp_with_retry(
    int sock,
    uint8_t const *packet,
    size_t packet_len,
    struct sockaddr_in const *dest_addr,
    uint32_t *retry_count,
    int *last_errno
)
{
    for (int attempt = 0; attempt < 4; ++attempt) {
        int sent = sendto(sock, packet, packet_len, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
        if (sent == (int)packet_len) {
            return sent;
        }

        *last_errno = errno;
        if (errno != ENOMEM && errno != EAGAIN && errno != ENOBUFS) {
            return sent;
        }

        ++(*retry_count);
        if (attempt < 2) {
            taskYIELD();
        } else {
            vTaskDelay(1);
        }
    }

    return -1;
}

static bool line_is_query(uint8_t const *line, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (line[i] == '?') {
            return true;
        }
    }
    return false;
}

static bool queue_scpi_line(uint8_t const *line, size_t len)
{
    if (!line || len == 0) {
        return false;
    }

    bridge_message_t scpi_msg = {
        .len = (uint16_t)len,
    };
    if (scpi_msg.len > MAX_PAYLOAD_LEN) {
        scpi_msg.len = MAX_PAYLOAD_LEN;
    }
    memcpy(scpi_msg.data, line, scpi_msg.len);
    if (scpi_msg.len < MAX_PAYLOAD_LEN &&
        scpi_msg.data[scpi_msg.len - 1] != '\n') {
        scpi_msg.data[scpi_msg.len++] = '\n';
    }
    return xQueueSend(scpi_rx_queue, &scpi_msg, pdMS_TO_TICKS(250)) == pdTRUE;
}

static void flush_scpi_tx_queue(void)
{
    bridge_message_t discarded;
    while (xQueueReceive(scpi_tx_queue, &discarded, 0) == pdTRUE) {
    }
}

static void spi_task(void *arg)
{
    (void)arg;
    uint32_t poll_seq = 0;
    uint32_t queued = 0;
    uint32_t spi_frames = 0;
    uint32_t spi_errors = 0;
    uint32_t queue_drops = 0;
    uint32_t scpi_frames = 0;
    uint32_t last_seq = 0;
    frame_error_stats_t error_stats = {0};
    int64_t report_start = esp_timer_get_time();

    for (uint32_t i = 0; i < SPI_QUEUED_TRANSFERS; ++i) {
        queue_slot(i, poll_seq++);
        ++queued;
    }
    for (uint32_t i = SPI_QUEUED_TRANSFERS; i < SLOT_COUNT; ++i) {
        xQueueSend(free_queue, &i, 0);
    }
    set_ready(true);

    while (true) {
        refill_spi_queue(&queued, &poll_seq);

        spi_slave_transaction_t *completed = NULL;
        esp_err_t err = spi_slave_get_trans_result(SPI2_HOST, &completed, pdMS_TO_TICKS(1000));
        if (err != ESP_OK) {
            ++spi_errors;
            continue;
        }

        if (queued > 0) {
            --queued;
        }

        uint32_t slot_index = (uint32_t)(uintptr_t)completed->user;
        uint8_t frame_type = FRAME_TYPE_POLL;
        uint32_t seq = 0;
        uint16_t frame_len = 0;
        uint32_t frame_errors = validate_rx_frame(
            slots[slot_index].rx,
            &frame_type,
            &seq,
            &frame_len,
            &error_stats
        );
        spi_errors += frame_errors;
        last_seq = seq;
        ++spi_frames;

        if (frame_errors == 0 && frame_type == FRAME_TYPE_DATA) {
            slots[slot_index].sequence = seq;
            if (xQueueSend(full_queue, &slot_index, 0) != pdTRUE) {
                ++queue_drops;
                xQueueSend(free_queue, &slot_index, 0);
            }
        } else if (frame_errors == 0 && frame_type == FRAME_TYPE_SCPI) {
            bridge_message_t scpi_msg = {
                .len = frame_len,
            };
            if (frame_len > 0) {
                memcpy(scpi_msg.data, &slots[slot_index].rx[HEADER_LEN], frame_len);
            }
            if (xQueueSend(scpi_tx_queue, &scpi_msg, 0) != pdTRUE) {
                ++queue_drops;
            } else {
                ++scpi_frames;
            }
            xQueueSend(free_queue, &slot_index, 0);
        } else {
            xQueueSend(free_queue, &slot_index, 0);
        }

        refill_spi_queue(&queued, &poll_seq);

        int64_t now = esp_timer_get_time();
        if (now - report_start >= (int64_t)CONFIG_ESP_BRIDGE_REPORT_INTERVAL_MS * 1000) {
            printf("spi_frames=%" PRIu32 ",queued=%" PRIu32 ",spi_errors=%" PRIu32 ",magic=%" PRIu32 ",type=%" PRIu32 ",len=%" PRIu32 ",chk=%" PRIu32 ",queue_drops=%" PRIu32 ",scpi_frames=%" PRIu32 ",last_seq=%" PRIu32 "\n",
                   spi_frames,
                   queued,
                   spi_errors,
                   error_stats.magic,
                   error_stats.type,
                   error_stats.length,
                   error_stats.checksum,
                   queue_drops,
                   scpi_frames,
                   last_seq);
            spi_frames = 0;
            spi_errors = 0;
            queue_drops = 0;
            scpi_frames = 0;
            error_stats = (frame_error_stats_t){0};
            report_start = now;
        }
    }
}

static void udp_task(void *arg)
{
    (void)arg;
    start_wifi();

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed errno=%d", errno);
        vTaskDelete(NULL);
    }

    int broadcast_enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    int send_buffer_size = 16 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, sizeof(send_buffer_size));

    struct sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_ESP_BRIDGE_LOCAL_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGW(TAG, "bind failed errno=%d", errno);
    }

    struct sockaddr_in wave_dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_ESP_BRIDGE_DEST_PORT),
    };
    inet_aton(CONFIG_ESP_BRIDGE_DEST_IP, &wave_dest_addr.sin_addr);

    bool using_wave_unicast = false;
    uint32_t udp_packets = 0;
    uint32_t udp_frames = 0;
    uint32_t udp_errors = 0;
    uint32_t udp_retries = 0;
    uint32_t last_seq = 0;
    uint64_t bytes_sent = 0;
    int last_errno = 0;
    uint8_t *udp_packet = heap_caps_malloc(UDP_PACKET_LEN, MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(udp_packet ? ESP_OK : ESP_ERR_NO_MEM);
    int64_t report_start = esp_timer_get_time();

    ESP_LOGI(TAG, "UDP initial destination %s:%d", CONFIG_ESP_BRIDGE_DEST_IP, CONFIG_ESP_BRIDGE_DEST_PORT);

    while (true) {
        uint8_t control_buffer[MAX_PAYLOAD_LEN];
        struct sockaddr_in control_addr = {0};
        socklen_t control_addr_len = sizeof(control_addr);
        int control_len = recvfrom(
            sock,
            control_buffer,
            sizeof(control_buffer),
            MSG_DONTWAIT,
            (struct sockaddr *)&control_addr,
            &control_addr_len
        );
        if (control_len > 0) {
            if (control_len == 18 &&
                memcmp(control_buffer, "PSLAB_UDP_REGISTER", 18) == 0) {
                bool changed = !using_wave_unicast ||
                               udp_endpoint_changed(&wave_dest_addr, &control_addr);
                wave_dest_addr = control_addr;
                using_wave_unicast = true;
                if (changed) {
                    log_udp_endpoint("registered waveform host", &wave_dest_addr);
                }
            }
        }

        uint32_t batch_slots[UDP_BATCH_FRAMES] = {0};
        uint32_t batch_count = 0;
        if (xQueueReceive(full_queue, &batch_slots[batch_count], pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }
        ++batch_count;

        while (batch_count < UDP_BATCH_FRAMES) {
            if (xQueueReceive(full_queue, &batch_slots[batch_count], pdMS_TO_TICKS(1)) != pdTRUE) {
                break;
            }
            ++batch_count;
        }

        for (uint32_t i = 0; i < batch_count; ++i) {
            memcpy(&udp_packet[i * FRAME_LEN], slots[batch_slots[i]].rx, FRAME_LEN);
            last_seq = slots[batch_slots[i]].sequence;
        }

        int sent = send_udp_with_retry(
            sock,
            udp_packet,
            batch_count * FRAME_LEN,
            &wave_dest_addr,
            &udp_retries,
            &last_errno
        );
        if (sent == (int)(batch_count * FRAME_LEN)) {
            ++udp_packets;
            udp_frames += batch_count;
            bytes_sent += (uint64_t)sent;
        } else {
            ++udp_errors;
            last_errno = errno;
        }

        for (uint32_t i = 0; i < batch_count; ++i) {
            xQueueSend(free_queue, &batch_slots[i], portMAX_DELAY);
        }

        int64_t now = esp_timer_get_time();
        if (now - report_start >= (int64_t)CONFIG_ESP_BRIDGE_REPORT_INTERVAL_MS * 1000) {
            double seconds = (now - report_start) / 1000000.0;
            double kbyte_s = seconds > 0.0 ? (bytes_sent / seconds / 1000.0) : 0.0;
            double mbit_s = seconds > 0.0 ? (bytes_sent * 8.0 / seconds / 1000000.0) : 0.0;
            printf("udp_packets=%" PRIu32 ",udp_frames=%" PRIu32 ",bytes=%" PRIu64 ",kbyte_s=%.3f,mbit_s=%.3f,udp_errors=%" PRIu32 ",udp_retries=%" PRIu32 ",last_errno=%d,mode=%s,last_seq=%" PRIu32 "\n",
                   udp_packets,
                   udp_frames,
                   bytes_sent,
                   kbyte_s,
                   mbit_s,
                   udp_errors,
                   udp_retries,
                   last_errno,
                   using_wave_unicast ? "unicast" : "broadcast",
                   last_seq);
            udp_packets = 0;
            udp_frames = 0;
            udp_errors = 0;
            udp_retries = 0;
            bytes_sent = 0;
            last_errno = 0;
            report_start = now;
        }
    }
}

static void tcp_scpi_task(void *arg)
{
    (void)arg;

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "TCP SCPI socket failed errno=%d", errno);
        vTaskDelete(NULL);
    }

    int reuse = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in listen_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_ESP_BRIDGE_LOCAL_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        ESP_LOGE(TAG, "TCP SCPI bind failed errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    if (listen(listen_sock, 1) < 0) {
        ESP_LOGE(TAG, "TCP SCPI listen failed errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "TCP SCPI listening on port %d", CONFIG_ESP_BRIDGE_LOCAL_PORT);

    uint8_t line[MAX_PAYLOAD_LEN];
    while (true) {
        struct sockaddr_in client_addr = {0};
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client < 0) {
            ESP_LOGW(TAG, "TCP SCPI accept failed errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        log_udp_endpoint("registered TCP SCPI host", &client_addr);
        size_t line_len = 0;
        while (true) {
            uint8_t buffer[128];
            int received = recv(client, buffer, sizeof(buffer), 0);
            if (received <= 0) {
                break;
            }

            for (int i = 0; i < received; ++i) {
                if (line_len < sizeof(line)) {
                    line[line_len++] = buffer[i];
                }
                if (buffer[i] != '\n' && buffer[i] != '\r') {
                    continue;
                }

                bool query = line_is_query(line, line_len);
                if (query) {
                    flush_scpi_tx_queue();
                }

                if (!queue_scpi_line(line, line_len)) {
                    static char const err[] = "-200,\"SCPI bridge queue full\"\n";
                    send(client, err, strlen(err), 0);
                    line_len = 0;
                    continue;
                }

                if (query) {
                    bridge_message_t response;
                    if (xQueueReceive(
                            scpi_tx_queue,
                            &response,
                            pdMS_TO_TICKS(2000)
                        ) == pdTRUE) {
                        send(client, response.data, response.len, 0);
                    } else {
                        flush_scpi_tx_queue();
                        static char const err[] = "-200,\"SCPI bridge timeout\"\n";
                        send(client, err, strlen(err), 0);
                    }
                }

                line_len = 0;
            }
        }

        close(client);
        ESP_LOGI(TAG, "TCP SCPI client disconnected");
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP SPI -> UDP bridge");
    ESP_LOGI(TAG, "SCLK=%d MOSI_IN=%d MISO_OUT=%d CS=%d READY_OUT=%d",
             CONFIG_ESP_BRIDGE_PIN_SCLK,
             CONFIG_ESP_BRIDGE_PIN_MOSI,
             CONFIG_ESP_BRIDGE_PIN_MISO,
             CONFIG_ESP_BRIDGE_PIN_CS,
             CONFIG_ESP_BRIDGE_PIN_READY);

    init_spi();
    init_slots();

    xTaskCreate(spi_task, "spi_task", 4096, NULL, 6, NULL);
    xTaskCreate(udp_task, "udp_task", 4096, NULL, 5, NULL);
    xTaskCreate(tcp_scpi_task, "tcp_scpi_task", 4096, NULL, 4, NULL);
}
