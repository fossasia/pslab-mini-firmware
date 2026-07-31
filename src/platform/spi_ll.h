/**
 * @file spi_ll.h
 * @brief Low-level SPI hardware interface.
 *
 * This module is the only layer that should call Pico SDK SPI functions
 * directly. System and application code should build on this interface.
 */

#ifndef PSLAB_SPI_LL_H
#define PSLAB_SPI_LL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SPI_LL_BUS_1 = 1,
    SPI_LL_BUS_COUNT = 2,
} SPI_LL_Bus;

typedef enum {
    SPI_LL_MODE_0 = 0,
    SPI_LL_MODE_1 = 1,
    SPI_LL_MODE_2 = 2,
    SPI_LL_MODE_3 = 3,
} SPI_LL_Mode;

typedef enum {
    SPI_LL_BIT_ORDER_MSB_FIRST = 0,
    SPI_LL_BIT_ORDER_LSB_FIRST = 1,
} SPI_LL_BitOrder;

enum {
    SPI_LL_DEFAULT_RATE_HZ = 1000000,
};

typedef struct {
    uint32_t sck_gpio;
    uint32_t tx_gpio;
    uint32_t rx_gpio;
    uint32_t cs_gpio;
    uint32_t rate_hz;
    SPI_LL_Mode mode;
    SPI_LL_BitOrder bit_order;
    bool cs_active_low;
} SPI_LL_Config;

/**
 * @brief Fill an SPI configuration with board defaults for a bus.
 *
 * Defaults:
 * - SPI1: SCK GPIO10, TX/MOSI GPIO11, RX/MISO GPIO12, CS GPIO13
 * - 1 MHz, mode 0, MSB first, active-low CS
 *
 * SPI0 is intentionally not exposed by this platform interface as
 * it is currently being used by the ESP/WiFi bridge.
 */
bool SPI_LL_default_config(SPI_LL_Bus bus, SPI_LL_Config *config);

bool SPI_LL_init(SPI_LL_Bus bus, SPI_LL_Config const *config);
void SPI_LL_deinit(SPI_LL_Bus bus);
bool SPI_LL_is_initialized(SPI_LL_Bus bus);
uint32_t SPI_LL_get_rate(SPI_LL_Bus bus);
SPI_LL_Mode SPI_LL_get_mode(SPI_LL_Bus bus);
SPI_LL_BitOrder SPI_LL_get_bit_order(SPI_LL_Bus bus);

bool SPI_LL_select(SPI_LL_Bus bus);
void SPI_LL_deselect(SPI_LL_Bus bus);
bool SPI_LL_is_selected(SPI_LL_Bus bus);

int32_t SPI_LL_transfer(
    SPI_LL_Bus bus,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t len
);

#ifdef __cplusplus
}
#endif

#endif
