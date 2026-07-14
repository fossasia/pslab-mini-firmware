/**
 * @file i2c_ll.h
 * @brief Low-level I2C hardware interface for the Raspberry Pi Pico family.
 *
 * This module is the only layer that should call Pico SDK I2C functions
 * directly. System and application code should build on this interface.
 */

#ifndef PSLAB_I2C_LL_H
#define PSLAB_I2C_LL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    I2C_LL_BUS_0 = 0,
    I2C_LL_BUS_1 = 1,
    I2C_LL_BUS_COUNT = 2,
} I2C_LL_Bus;

enum {
    I2C_LL_DEFAULT_RATE_HZ = 100000,
    I2C_LL_DEFAULT_TIMEOUT_US = 100000,
};

typedef struct {
    uint32_t sda_gpio;
    uint32_t scl_gpio;
    uint32_t rate_hz;
    bool enable_pullups;
} I2C_LL_Config;

/**
 * @brief Fill an I2C configuration with board defaults for a bus.
 *
 * Defaults:
 * - I2C0: SDA GPIO0, SCL GPIO1
 * - I2C1: SDA GPIO6, SCL GPIO7
 * - 100 kHz, internal pull-ups enabled
 */
bool I2C_LL_default_config(I2C_LL_Bus bus, I2C_LL_Config *config);

bool I2C_LL_init(I2C_LL_Bus bus, I2C_LL_Config const *config);
void I2C_LL_deinit(I2C_LL_Bus bus);
bool I2C_LL_is_initialized(I2C_LL_Bus bus);
uint32_t I2C_LL_get_rate(I2C_LL_Bus bus);

int32_t I2C_LL_write(
    I2C_LL_Bus bus,
    uint8_t address,
    uint8_t const *data,
    size_t len,
    bool nostop,
    uint32_t timeout_us
);

int32_t I2C_LL_read(
    I2C_LL_Bus bus,
    uint8_t address,
    uint8_t *data,
    size_t len,
    bool nostop,
    uint32_t timeout_us
);

bool I2C_LL_probe_address(
    I2C_LL_Bus bus,
    uint8_t address,
    uint32_t timeout_us
);

#ifdef __cplusplus
}
#endif

#endif
