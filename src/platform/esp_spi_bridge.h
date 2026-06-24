#ifndef ESP_SPI_BRIDGE_H
#define ESP_SPI_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ESP_SPI_BRIDGE_FRAME_LEN = 512,
    ESP_SPI_BRIDGE_HEADER_LEN = 12,
    ESP_SPI_BRIDGE_PAYLOAD_LEN =
        ESP_SPI_BRIDGE_FRAME_LEN - ESP_SPI_BRIDGE_HEADER_LEN,
};

typedef enum {
    ESP_SPI_BRIDGE_FRAME_POLL = 0x00,
    ESP_SPI_BRIDGE_FRAME_DATA = 0x02,
    ESP_SPI_BRIDGE_FRAME_SCPI = 0x03,
} EspSpiBridgeFrameType;

bool esp_spi_bridge_init(void);
bool esp_spi_bridge_is_ready(void);
bool esp_spi_bridge_exchange(
    EspSpiBridgeFrameType tx_type,
    uint32_t sequence,
    uint8_t const *tx_payload,
    size_t tx_payload_len,
    EspSpiBridgeFrameType *rx_type,
    uint8_t *rx_payload,
    size_t rx_payload_size,
    size_t *rx_payload_len
);
bool esp_spi_bridge_send_payload(
    uint32_t sequence,
    uint8_t const *payload,
    size_t payload_len
);

uint32_t esp_spi_bridge_get_sent_frames(void);
uint32_t esp_spi_bridge_get_dropped_frames(void);
uint32_t esp_spi_bridge_get_timeouts(void);

#ifdef __cplusplus
}
#endif

#endif
