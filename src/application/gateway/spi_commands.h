#ifndef PSLAB_APPLICATION_GATEWAY_SPI_COMMANDS_H
#define PSLAB_APPLICATION_GATEWAY_SPI_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SPI_GATEWAY_DEFAULT_BUS = 1,
    SPI_GATEWAY_DEFAULT_RATE_HZ = 1000000,
    SPI_GATEWAY_DEFAULT_MODE = 0,
    SPI_GATEWAY_DEFAULT_DUMMY_BYTE = 0xff,
    SPI_GATEWAY_MAX_TRANSFER = 512,
};

bool spi_gateway_set_bus(uint32_t bus);
uint32_t spi_gateway_get_bus(void);

bool spi_gateway_set_rate(uint32_t rate_hz);
uint32_t spi_gateway_get_rate(void);

bool spi_gateway_set_mode(uint32_t mode);
uint32_t spi_gateway_get_mode(void);

bool spi_gateway_set_dummy_byte(uint32_t dummy_byte);
uint32_t spi_gateway_get_dummy_byte(void);

bool spi_gateway_open(void);
void spi_gateway_close(void);
bool spi_gateway_is_open(void);

int32_t spi_gateway_write(uint8_t const *data, size_t len);
int32_t spi_gateway_read(uint8_t *data, size_t len);
int32_t spi_gateway_exchange(
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t len
);
int32_t spi_gateway_transact(
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_len
);

#ifdef __cplusplus
}
#endif

#endif
