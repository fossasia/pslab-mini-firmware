#ifndef PSLAB_APPLICATION_GATEWAY_I2C_COMMANDS_H
#define PSLAB_APPLICATION_GATEWAY_I2C_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    I2C_GATEWAY_DEFAULT_BUS = 0,
    I2C_GATEWAY_DEFAULT_ADDRESS = 0x3c,
    I2C_GATEWAY_DEFAULT_RATE_HZ = 100000,
    I2C_GATEWAY_DEFAULT_TIMEOUT_MS = 100,
    I2C_GATEWAY_MAX_TRANSFER = 512,
    I2C_GATEWAY_MAX_SCAN_RESULTS = 112,
};

bool i2c_gateway_set_bus(uint32_t bus);
uint32_t i2c_gateway_get_bus(void);

bool i2c_gateway_set_rate(uint32_t rate_hz);
uint32_t i2c_gateway_get_rate(void);

bool i2c_gateway_set_address(uint32_t address);
uint32_t i2c_gateway_get_address(void);

bool i2c_gateway_set_timeout(uint32_t timeout_ms);
uint32_t i2c_gateway_get_timeout(void);

bool i2c_gateway_open(void);
void i2c_gateway_close(void);
bool i2c_gateway_is_open(void);

uint32_t i2c_gateway_scan(uint8_t *addresses, size_t max_count);
int32_t i2c_gateway_write(uint8_t const *data, size_t len);
int32_t i2c_gateway_read(uint8_t *data, size_t len);
int32_t i2c_gateway_transact(
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_len
);

#ifdef __cplusplus
}
#endif

#endif
