#include "platform/esp_spi_bridge.h"

#include <string.h>

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

enum {
    ESP_SPI_BRIDGE_PIN_SCK = 2,
    ESP_SPI_BRIDGE_PIN_TX = 3,
    ESP_SPI_BRIDGE_PIN_RX = 4,
    ESP_SPI_BRIDGE_PIN_CSN = 5,
    ESP_SPI_BRIDGE_PIN_READY = 6,
    ESP_SPI_BRIDGE_BAUDRATE_HZ = 1000000u,
    ESP_SPI_BRIDGE_READY_TIMEOUT_US = 1000,
    ESP_SPI_BRIDGE_MAGIC = 0xa5,
};

static bool g_initialized;
static int g_tx_dma = -1;
static int g_rx_dma = -1;
static uint8_t g_tx_frame[ESP_SPI_BRIDGE_FRAME_LEN];
static uint8_t g_rx_frame[ESP_SPI_BRIDGE_FRAME_LEN];
static uint32_t g_sent_frames;
static uint32_t g_dropped_frames;
static uint32_t g_timeouts;

static uint8_t checksum8(uint8_t const *data, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum = (uint8_t)(sum + data[i]);
    }
    return sum;
}

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

static uint16_t get_u16_le(uint8_t const *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static bool wait_ready_high(uint32_t timeout_us)
{
    absolute_time_t deadline = make_timeout_time_us(timeout_us);
    while (!gpio_get(ESP_SPI_BRIDGE_PIN_READY)) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) {
            return false;
        }
        tight_loop_contents();
    }
    return true;
}

static void drain_spi_rx_fifo(void)
{
    while (spi_is_readable(spi0)) {
        (void)spi_get_hw(spi0)->dr;
    }
}

static void transfer_frame_dma(void)
{
    drain_spi_rx_fifo();

    dma_channel_config rx_cfg = dma_channel_get_default_config(g_rx_dma);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&rx_cfg, false);
    channel_config_set_write_increment(&rx_cfg, true);
    channel_config_set_dreq(&rx_cfg, spi_get_dreq(spi0, false));

    dma_channel_config tx_cfg = dma_channel_get_default_config(g_tx_dma);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    channel_config_set_dreq(&tx_cfg, spi_get_dreq(spi0, true));

    dma_channel_configure(
        g_rx_dma,
        &rx_cfg,
        g_rx_frame,
        &spi_get_hw(spi0)->dr,
        ESP_SPI_BRIDGE_FRAME_LEN,
        false
    );
    dma_channel_configure(
        g_tx_dma,
        &tx_cfg,
        &spi_get_hw(spi0)->dr,
        g_tx_frame,
        ESP_SPI_BRIDGE_FRAME_LEN,
        false
    );

    gpio_put(ESP_SPI_BRIDGE_PIN_CSN, 0);
    sleep_us(2);
    dma_start_channel_mask((1u << g_rx_dma) | (1u << g_tx_dma));
    dma_channel_wait_for_finish_blocking(g_rx_dma);
    dma_channel_wait_for_finish_blocking(g_tx_dma);
    sleep_us(2);
    gpio_put(ESP_SPI_BRIDGE_PIN_CSN, 1);
}

bool esp_spi_bridge_init(void)
{
    if (g_initialized) {
        return true;
    }

    gpio_init(ESP_SPI_BRIDGE_PIN_READY);
    gpio_set_dir(ESP_SPI_BRIDGE_PIN_READY, GPIO_IN);
    gpio_pull_down(ESP_SPI_BRIDGE_PIN_READY);

    gpio_init(ESP_SPI_BRIDGE_PIN_CSN);
    gpio_set_dir(ESP_SPI_BRIDGE_PIN_CSN, GPIO_OUT);
    gpio_put(ESP_SPI_BRIDGE_PIN_CSN, 1);

    spi_init(spi0, ESP_SPI_BRIDGE_BAUDRATE_HZ);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(ESP_SPI_BRIDGE_PIN_RX, GPIO_FUNC_SPI);
    gpio_set_function(ESP_SPI_BRIDGE_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(ESP_SPI_BRIDGE_PIN_TX, GPIO_FUNC_SPI);

    g_tx_dma = dma_claim_unused_channel(false);
    g_rx_dma = dma_claim_unused_channel(false);
    if (g_tx_dma < 0 || g_rx_dma < 0) {
        return false;
    }

    g_initialized = true;
    return true;
}

bool esp_spi_bridge_is_ready(void)
{
    return g_initialized && gpio_get(ESP_SPI_BRIDGE_PIN_READY);
}

bool esp_spi_bridge_exchange(
    EspSpiBridgeFrameType tx_type,
    uint32_t sequence,
    uint8_t const *tx_payload,
    size_t tx_payload_len,
    EspSpiBridgeFrameType *rx_type,
    uint8_t *rx_payload,
    size_t rx_payload_size,
    size_t *rx_payload_len
)
{
    if (!g_initialized && !esp_spi_bridge_init()) {
        ++g_dropped_frames;
        return false;
    }

    if ((!tx_payload && tx_payload_len > 0) ||
        tx_payload_len > ESP_SPI_BRIDGE_PAYLOAD_LEN) {
        ++g_dropped_frames;
        return false;
    }

    if (!wait_ready_high(ESP_SPI_BRIDGE_READY_TIMEOUT_US)) {
        ++g_timeouts;
        ++g_dropped_frames;
        return false;
    }

    memset(g_tx_frame, 0, sizeof(g_tx_frame));
    memset(g_rx_frame, 0, sizeof(g_rx_frame));
    g_tx_frame[0] = ESP_SPI_BRIDGE_MAGIC;
    g_tx_frame[1] = (uint8_t)tx_type;
    put_u32_le(&g_tx_frame[2], sequence);
    put_u16_le(&g_tx_frame[6], (uint16_t)tx_payload_len);
    g_tx_frame[8] = checksum8(g_tx_frame, 8);
    if (tx_payload_len > 0) {
        memcpy(&g_tx_frame[ESP_SPI_BRIDGE_HEADER_LEN], tx_payload, tx_payload_len);
    }

    transfer_frame_dma();
    ++g_sent_frames;

    if (rx_payload_len) {
        *rx_payload_len = 0;
    }
    if (rx_type) {
        *rx_type = ESP_SPI_BRIDGE_FRAME_POLL;
    }

    if (g_rx_frame[0] != ESP_SPI_BRIDGE_MAGIC ||
        g_rx_frame[8] != checksum8(g_rx_frame, 8)) {
        return true;
    }

    uint16_t payload_len = get_u16_le(&g_rx_frame[6]);
    if (payload_len > ESP_SPI_BRIDGE_PAYLOAD_LEN) {
        return true;
    }

    if (rx_type) {
        *rx_type = (EspSpiBridgeFrameType)g_rx_frame[1];
    }
    if (rx_payload && rx_payload_len) {
        size_t copy_len = payload_len;
        if (copy_len > rx_payload_size) {
            copy_len = rx_payload_size;
        }
        if (copy_len > 0) {
            memcpy(rx_payload, &g_rx_frame[ESP_SPI_BRIDGE_HEADER_LEN], copy_len);
        }
        *rx_payload_len = copy_len;
    }

    return true;
}

bool esp_spi_bridge_send_payload(
    uint32_t sequence,
    uint8_t const *payload,
    size_t payload_len
)
{
    return esp_spi_bridge_exchange(
        ESP_SPI_BRIDGE_FRAME_DATA,
        sequence,
        payload,
        payload_len,
        NULL,
        NULL,
        0,
        NULL
    );
}

uint32_t esp_spi_bridge_get_sent_frames(void) { return g_sent_frames; }

uint32_t esp_spi_bridge_get_dropped_frames(void) { return g_dropped_frames; }

uint32_t esp_spi_bridge_get_timeouts(void) { return g_timeouts; }
