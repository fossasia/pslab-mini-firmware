/**
 * @file i2c.c
 * @brief Hardware-independent I2C bus implementation.
 */

#include "system/bus/i2c.h"

#include <stdlib.h>

#include "platform/i2c_ll.h"

struct I2C_Handle {
    I2C_LL_Bus bus;
    uint32_t timeout_us;
    bool initialized;
};

static I2C_Handle *active_handles[I2C_LL_BUS_COUNT];

static bool valid_bus(uint32_t bus) { return bus < I2C_LL_BUS_COUNT; }

static bool valid_7bit_address(uint8_t address) { return address < 0x80; }

static bool reserved_7bit_address(uint8_t address)
{
    return address <= 0x07 || address >= 0x78;
}

size_t I2C_get_bus_count(void) { return I2C_LL_BUS_COUNT; }

bool I2C_default_config(uint32_t bus, I2C_Config *config)
{
    if (!valid_bus(bus) || !config) {
        return false;
    }

    I2C_LL_Config ll_config;
    if (!I2C_LL_default_config((I2C_LL_Bus)bus, &ll_config)) {
        return false;
    }

    *config = (I2C_Config){
        .bus = bus,
        .sda_gpio = ll_config.sda_gpio,
        .scl_gpio = ll_config.scl_gpio,
        .rate_hz = ll_config.rate_hz,
        .timeout_us = I2C_DEFAULT_TIMEOUT_US,
        .enable_pullups = ll_config.enable_pullups,
    };
    return true;
}

I2C_Handle *I2C_init(I2C_Config const *config)
{
    if (!config || !valid_bus(config->bus) || config->rate_hz == 0 ||
        config->timeout_us == 0 || config->sda_gpio == config->scl_gpio ||
        active_handles[config->bus]) {
        return NULL;
    }

    I2C_Handle *handle = malloc(sizeof(*handle));
    if (!handle) {
        return NULL;
    }

    I2C_LL_Config ll_config = {
        .sda_gpio = config->sda_gpio,
        .scl_gpio = config->scl_gpio,
        .rate_hz = config->rate_hz,
        .enable_pullups = config->enable_pullups,
    };

    if (!I2C_LL_init((I2C_LL_Bus)config->bus, &ll_config)) {
        free(handle);
        return NULL;
    }

    *handle = (I2C_Handle){
        .bus = (I2C_LL_Bus)config->bus,
        .timeout_us = config->timeout_us,
        .initialized = true,
    };
    active_handles[config->bus] = handle;
    return handle;
}

void I2C_deinit(I2C_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }

    I2C_LL_Bus bus = handle->bus;
    I2C_LL_deinit(bus);
    if (valid_bus(bus) && active_handles[bus] == handle) {
        active_handles[bus] = NULL;
    }

    handle->initialized = false;
    free(handle);
}

bool I2C_is_ready(I2C_Handle const *handle)
{
    return handle && handle->initialized &&
           I2C_LL_is_initialized(handle->bus);
}

uint32_t I2C_get_bus(I2C_Handle const *handle)
{
    return I2C_is_ready(handle) ? (uint32_t)handle->bus : I2C_LL_BUS_COUNT;
}

uint32_t I2C_get_rate(I2C_Handle const *handle)
{
    return I2C_is_ready(handle) ? I2C_LL_get_rate(handle->bus) : 0;
}

uint32_t I2C_get_timeout(I2C_Handle const *handle)
{
    return I2C_is_ready(handle) ? handle->timeout_us : 0;
}

int32_t I2C_write(
    I2C_Handle *handle,
    uint8_t address,
    uint8_t const *data,
    size_t len,
    bool nostop
)
{
    if (!I2C_is_ready(handle) || !valid_7bit_address(address) ||
        (!data && len > 0)) {
        return -1;
    }

    return I2C_LL_write(
        handle->bus,
        address,
        data,
        len,
        nostop,
        handle->timeout_us
    );
}

int32_t I2C_read(
    I2C_Handle *handle,
    uint8_t address,
    uint8_t *data,
    size_t len,
    bool nostop
)
{
    if (!I2C_is_ready(handle) || !valid_7bit_address(address) ||
        !data || len == 0) {
        return -1;
    }

    return I2C_LL_read(
        handle->bus,
        address,
        data,
        len,
        nostop,
        handle->timeout_us
    );
}

bool I2C_probe(I2C_Handle *handle, uint8_t address)
{
    if (!I2C_is_ready(handle) || !valid_7bit_address(address) ||
        reserved_7bit_address(address)) {
        return false;
    }

    return I2C_LL_probe_address(handle->bus, address, handle->timeout_us);
}

uint32_t I2C_scan(I2C_Handle *handle, uint8_t *addresses, size_t max_count)
{
    if (!I2C_is_ready(handle) || !addresses || max_count == 0) {
        return 0;
    }

    uint32_t count = 0;
    for (uint8_t address = 0x08; address < 0x78; ++address) {
        if (I2C_probe(handle, address)) {
            addresses[count++] = address;
            if (count >= max_count) {
                break;
            }
        }
    }

    return count;
}
