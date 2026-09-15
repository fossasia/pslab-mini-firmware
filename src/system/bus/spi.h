/**
 * @file spi.h
 * @brief Hardware-independent SPI bus interface.
 *
 * This layer owns SPI handles and bus lifetime. Application code should use
 * this API rather than calling the platform SPI layer directly.
 */

#ifndef PSLAB_SYSTEM_BUS_SPI_H
#define PSLAB_SYSTEM_BUS_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SPI_Handle SPI_Handle;

typedef enum {
    SPI_MODE_0 = 0,
    SPI_MODE_1 = 1,
    SPI_MODE_2 = 2,
    SPI_MODE_3 = 3,
} SPI_Mode;

typedef enum {
    SPI_BIT_ORDER_MSB_FIRST = 0,
    SPI_BIT_ORDER_LSB_FIRST = 1,
} SPI_BitOrder;

typedef struct {
    uint32_t bus;
    uint32_t sck_gpio;
    uint32_t tx_gpio;
    uint32_t rx_gpio;
    uint32_t cs_gpio;
    uint32_t rate_hz;
    SPI_Mode mode;
    SPI_BitOrder bit_order;
    uint8_t dummy_byte;
    bool cs_active_low;
} SPI_Config;

enum {
    SPI_DEFAULT_BUS = 1,
    SPI_DEFAULT_RATE_HZ = 1000000,
    SPI_DEFAULT_DUMMY_BYTE = 0xff,
};

size_t SPI_get_bus_count(void);
bool SPI_default_config(uint32_t bus, SPI_Config *config);

SPI_Handle *SPI_init(SPI_Config const *config);
void SPI_deinit(SPI_Handle *handle);

bool SPI_is_ready(SPI_Handle const *handle);
uint32_t SPI_get_bus(SPI_Handle const *handle);
uint32_t SPI_get_rate(SPI_Handle const *handle);
SPI_Mode SPI_get_mode(SPI_Handle const *handle);
SPI_BitOrder SPI_get_bit_order(SPI_Handle const *handle);
uint8_t SPI_get_dummy_byte(SPI_Handle const *handle);

/**
 * @brief Full-duplex SPI exchange.
 *
 * Keeps CS asserted for the entire transfer.
 *
 * @return Number of bytes exchanged on success, or a negative value on error.
 */
int32_t SPI_exchange(
    SPI_Handle *handle,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t len
);

/**
 * @brief Write bytes while discarding received bytes.
 *
 * Keeps CS asserted for the entire transfer.
 *
 * @return Number of bytes written on success, or a negative value on error.
 */
int32_t SPI_write(SPI_Handle *handle, uint8_t const *data, size_t len);

/**
 * @brief Read bytes while sending the handle's configured dummy byte.
 *
 * Keeps CS asserted for the entire transfer.
 *
 * @return Number of bytes read on success, or a negative value on error.
 */
int32_t SPI_read(SPI_Handle *handle, uint8_t *data, size_t len);

/**
 * @brief Write a command/prefix and then read a response under one CS window.
 *
 * This is the common register-read pattern for SPI sensors.
 *
 * @return Number of bytes read on success, or a negative value on error.
 */
int32_t SPI_transact(
    SPI_Handle *handle,
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_len
);

#ifdef __cplusplus
}
#endif

#endif
