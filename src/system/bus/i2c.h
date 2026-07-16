/**
 * @file i2c.h
 * @brief Hardware-independent I2C bus interface.
 *
 * This layer owns I2C handles and bus lifetime. Application code should use
 * this API rather than calling the platform I2C layer directly.
 */

#ifndef PSLAB_SYSTEM_BUS_I2C_H
#define PSLAB_SYSTEM_BUS_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct I2C_Handle I2C_Handle;

typedef struct {
    uint32_t bus;
    uint32_t sda_gpio;
    uint32_t scl_gpio;
    uint32_t rate_hz;
    uint32_t timeout_us;
    bool enable_pullups;
} I2C_Config;

enum {
    I2C_DEFAULT_BUS = 0,
    I2C_DEFAULT_RATE_HZ = 100000,
    I2C_DEFAULT_TIMEOUT_US = 100000,
};

size_t I2C_get_bus_count(void);
bool I2C_default_config(uint32_t bus, I2C_Config *config);

I2C_Handle *I2C_init(I2C_Config const *config);
void I2C_deinit(I2C_Handle *handle);

bool I2C_is_ready(I2C_Handle const *handle);
uint32_t I2C_get_bus(I2C_Handle const *handle);
uint32_t I2C_get_rate(I2C_Handle const *handle);
uint32_t I2C_get_timeout(I2C_Handle const *handle);

int32_t I2C_write(
    I2C_Handle *handle,
    uint8_t address,
    uint8_t const *data,
    size_t len,
    bool nostop
);

int32_t I2C_read(
    I2C_Handle *handle,
    uint8_t address,
    uint8_t *data,
    size_t len,
    bool nostop
);

bool I2C_probe(I2C_Handle *handle, uint8_t address);
uint32_t I2C_scan(I2C_Handle *handle, uint8_t *addresses, size_t max_count);

#ifdef __cplusplus
}
#endif

#endif
