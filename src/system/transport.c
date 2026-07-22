#include "system/transport.h"

#include <string.h>

#include "platform/esp_spi_bridge.h"

enum {
    PSLAB_PAYLOAD_MAGIC = 0x424c5350u, /* "PSLB" little-endian */
    PSLAB_PAYLOAD_VERSION = 1,
    PSLAB_SUBTYPE_META = 1,
    PSLAB_SUBTYPE_DATA = 2,
    PSLAB_PAYLOAD_HEADER_LEN = 32,
    PSLAB_DATA_BYTES_PER_FRAME =
        ESP_SPI_BRIDGE_PAYLOAD_LEN - PSLAB_PAYLOAD_HEADER_LEN,
    PSLAB_FORMAT_LA_U32_PACKED = 1,
    PSLAB_FORMAT_DSO_U16_LE = 2,
    TRANSPORT_SCPI_BUFFER_SIZE = ESP_SPI_BRIDGE_PAYLOAD_LEN,
};

static TransportMode g_mode = TRANSPORT_MODE_USB;
static uint32_t g_frame_sequence;
static uint8_t g_pending_scpi[TRANSPORT_SCPI_BUFFER_SIZE];
static size_t g_pending_scpi_len;

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

static void prepare_payload_header(
    uint8_t *payload,
    TransportInstrument instrument,
    uint8_t subtype,
    uint32_t capture_sequence,
    uint16_t chunk_index,
    uint16_t chunk_count,
    uint16_t payload_len
)
{
    memset(payload, 0, ESP_SPI_BRIDGE_PAYLOAD_LEN);
    put_u32_le(&payload[0], PSLAB_PAYLOAD_MAGIC);
    payload[4] = PSLAB_PAYLOAD_VERSION;
    payload[5] = (uint8_t)instrument;
    payload[6] = subtype;
    payload[7] = 0;
    put_u32_le(&payload[8], capture_sequence);
    put_u16_le(&payload[12], chunk_index);
    put_u16_le(&payload[14], chunk_count);
    put_u16_le(&payload[16], payload_len);
}

static uint32_t sample_rate_for_meta(
    TransportInstrument instrument,
    TransportCaptureMeta const *meta
)
{
    if (instrument == TRANSPORT_INSTRUMENT_LA && meta->sample_rate_hz == 0) {
        return 0;
    }
    return meta->sample_rate_hz;
}

void transport_init(void)
{
    g_mode = TRANSPORT_MODE_USB;
    g_frame_sequence = 0;
    g_pending_scpi_len = 0;
}

void transport_set_mode(TransportMode mode)
{
    if (mode <= TRANSPORT_MODE_AUTO) {
        g_mode = mode;
        if (mode == TRANSPORT_MODE_WIFI || mode == TRANSPORT_MODE_AUTO) {
            (void)esp_spi_bridge_init();
        }
    }
}

TransportMode transport_get_mode(void) { return g_mode; }

char const *transport_get_mode_name(void)
{
    switch (g_mode) {
    case TRANSPORT_MODE_WIFI:
        return "WIFI";
    case TRANSPORT_MODE_AUTO:
        return "AUTO";
    case TRANSPORT_MODE_USB:
    default:
        return "USB";
    }
}

bool transport_wifi_is_effective(void)
{
    if (g_mode == TRANSPORT_MODE_WIFI) {
        return true;
    }

    return g_mode == TRANSPORT_MODE_AUTO && esp_spi_bridge_is_ready();
}

static void store_scpi_payload(
    EspSpiBridgeFrameType rx_type,
    uint8_t const *payload,
    size_t payload_len
)
{
    if (rx_type != ESP_SPI_BRIDGE_FRAME_SCPI || !payload || payload_len == 0) {
        return;
    }

    if (payload_len > sizeof(g_pending_scpi)) {
        payload_len = sizeof(g_pending_scpi);
    }
    memcpy(g_pending_scpi, payload, payload_len);
    g_pending_scpi_len = payload_len;
}

static bool exchange_and_store_scpi(
    EspSpiBridgeFrameType tx_type,
    uint8_t const *payload,
    size_t payload_len
)
{
    uint8_t rx_payload[ESP_SPI_BRIDGE_PAYLOAD_LEN];
    size_t rx_len = 0;
    EspSpiBridgeFrameType rx_type = ESP_SPI_BRIDGE_FRAME_POLL;
    bool ok = esp_spi_bridge_exchange(
        tx_type,
        g_frame_sequence++,
        payload,
        payload_len,
        &rx_type,
        rx_payload,
        sizeof(rx_payload),
        &rx_len
    );
    if (ok) {
        store_scpi_payload(rx_type, rx_payload, rx_len);
    }
    return ok;
}

bool transport_poll_scpi_command(uint8_t *buffer, size_t buffer_size, size_t *len)
{
    if (!buffer || buffer_size == 0 || !len) {
        return false;
    }

    if (!transport_wifi_is_effective()) {
        *len = 0;
        return false;
    }

    if (g_pending_scpi_len == 0 && esp_spi_bridge_init() &&
        esp_spi_bridge_is_ready()) {
        (void)exchange_and_store_scpi(ESP_SPI_BRIDGE_FRAME_POLL, NULL, 0);
    }

    if (g_pending_scpi_len == 0) {
        *len = 0;
        return false;
    }

    size_t copy_len = g_pending_scpi_len;
    if (copy_len > buffer_size) {
        copy_len = buffer_size;
    }
    memcpy(buffer, g_pending_scpi, copy_len);
    g_pending_scpi_len = 0;
    *len = copy_len;
    return true;
}

size_t transport_send_scpi_response(uint8_t const *data, size_t len)
{
    if ((!data && len > 0) || !esp_spi_bridge_init()) {
        return 0;
    }

    size_t sent = 0;
    while (sent < len || (len == 0 && sent == 0)) {
        size_t chunk_len = len - sent;
        if (chunk_len > ESP_SPI_BRIDGE_PAYLOAD_LEN) {
            chunk_len = ESP_SPI_BRIDGE_PAYLOAD_LEN;
        }
        if (!exchange_and_store_scpi(
                ESP_SPI_BRIDGE_FRAME_SCPI,
                data ? &data[sent] : NULL,
                chunk_len
            )) {
            break;
        }
        sent += chunk_len;
        if (len == 0) {
            break;
        }
    }
    return sent;
}

bool transport_send_capture(
    TransportInstrument instrument,
    uint32_t capture_sequence,
    TransportCaptureMeta const *meta,
    uint8_t const *data,
    size_t len
)
{
    if (!transport_wifi_is_effective() || !meta || (!data && len > 0)) {
        return false;
    }

    uint8_t payload[ESP_SPI_BRIDGE_PAYLOAD_LEN];
    prepare_payload_header(
        payload,
        instrument,
        PSLAB_SUBTYPE_META,
        capture_sequence,
        0,
        0,
        24
    );
    put_u32_le(&payload[PSLAB_PAYLOAD_HEADER_LEN + 0], sample_rate_for_meta(instrument, meta));
    put_u32_le(&payload[PSLAB_PAYLOAD_HEADER_LEN + 4], meta->sample_count);
    put_u32_le(&payload[PSLAB_PAYLOAD_HEADER_LEN + 8], meta->channel_count);
    put_u32_le(&payload[PSLAB_PAYLOAD_HEADER_LEN + 12], meta->pin_base_or_channel);
    put_u32_le(&payload[PSLAB_PAYLOAD_HEADER_LEN + 16], meta->trigger_mode);
    put_u32_le(
        &payload[PSLAB_PAYLOAD_HEADER_LEN + 20],
        meta->data_format != 0 ? meta->data_format :
            (instrument == TRANSPORT_INSTRUMENT_DSO ||
             instrument == TRANSPORT_INSTRUMENT_MSO_ANALOG
                 ? PSLAB_FORMAT_DSO_U16_LE
                 : PSLAB_FORMAT_LA_U32_PACKED)
    );

    if (!exchange_and_store_scpi(
            ESP_SPI_BRIDGE_FRAME_DATA,
            payload,
            ESP_SPI_BRIDGE_PAYLOAD_LEN
        )) {
        return false;
    }

    uint16_t chunk_count = (uint16_t)((len + PSLAB_DATA_BYTES_PER_FRAME - 1u) /
                                      PSLAB_DATA_BYTES_PER_FRAME);
    if (chunk_count == 0) {
        chunk_count = 1;
    }

    for (uint16_t chunk = 0; chunk < chunk_count; ++chunk) {
        size_t offset = (size_t)chunk * PSLAB_DATA_BYTES_PER_FRAME;
        size_t remaining = len > offset ? len - offset : 0;
        size_t chunk_len =
            remaining > PSLAB_DATA_BYTES_PER_FRAME ? PSLAB_DATA_BYTES_PER_FRAME
                                                   : remaining;

        prepare_payload_header(
            payload,
            instrument,
            PSLAB_SUBTYPE_DATA,
            capture_sequence,
            chunk,
            chunk_count,
            (uint16_t)chunk_len
        );
        if (chunk_len > 0) {
            memcpy(&payload[PSLAB_PAYLOAD_HEADER_LEN], &data[offset], chunk_len);
        }

        if (!exchange_and_store_scpi(
                ESP_SPI_BRIDGE_FRAME_DATA,
                payload,
                ESP_SPI_BRIDGE_PAYLOAD_LEN
            )) {
            return false;
        }
    }

    return true;
}

uint32_t transport_get_sent_frames(void)
{
    return esp_spi_bridge_get_sent_frames();
}

uint32_t transport_get_dropped_frames(void)
{
    return esp_spi_bridge_get_dropped_frames();
}

uint32_t transport_get_timeouts(void)
{
    return esp_spi_bridge_get_timeouts();
}
