/**
 * @file i2c_ll.c
 * @brief Low-level I2C hardware implementation for RP2040/RP2350.
 */

#include "platform/i2c_ll.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/error.h"

typedef struct {
    i2c_inst_t *instance;
    uint32_t sda_gpio;
    uint32_t scl_gpio;
    uint32_t rate_hz;
    bool initialized;
} I2C_LL_Instance;

static I2C_LL_Instance instances[I2C_LL_BUS_COUNT] = {
    [I2C_LL_BUS_0] = {
        .instance = i2c0,
    },
    [I2C_LL_BUS_1] = {
        .instance = i2c1,
    },
};

static bool valid_bus(I2C_LL_Bus bus) { return bus < I2C_LL_BUS_COUNT; }

static bool valid_address(uint8_t address) { return address < 0x80; }

bool I2C_LL_default_config(I2C_LL_Bus bus, I2C_LL_Config *config)
{
    if (!valid_bus(bus) || !config) {
        return false;
    }

    *config = (I2C_LL_Config){
        .sda_gpio = bus == I2C_LL_BUS_0 ? 0u : 6u,
        .scl_gpio = bus == I2C_LL_BUS_0 ? 1u : 7u,
        .rate_hz = I2C_LL_DEFAULT_RATE_HZ,
        .enable_pullups = true,
    };
    return true;
}

bool I2C_LL_init(I2C_LL_Bus bus, I2C_LL_Config const *config)
{
    if (!valid_bus(bus) || !config || config->rate_hz == 0 ||
        config->sda_gpio > 29 || config->scl_gpio > 29 ||
        config->sda_gpio == config->scl_gpio || instances[bus].initialized) {
        return false;
    }

    I2C_LL_Instance *instance = &instances[bus];
    uint32_t actual_rate = i2c_init(instance->instance, config->rate_hz);

    gpio_set_function(config->sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(config->scl_gpio, GPIO_FUNC_I2C);

    if (config->enable_pullups) {
        gpio_pull_up(config->sda_gpio);
        gpio_pull_up(config->scl_gpio);
    } else {
        gpio_disable_pulls(config->sda_gpio);
        gpio_disable_pulls(config->scl_gpio);
    }

    instance->sda_gpio = config->sda_gpio;
    instance->scl_gpio = config->scl_gpio;
    instance->rate_hz = actual_rate;
    instance->initialized = true;
    return true;
}

void I2C_LL_deinit(I2C_LL_Bus bus)
{
    if (!valid_bus(bus) || !instances[bus].initialized) {
        return;
    }

    I2C_LL_Instance *instance = &instances[bus];
    i2c_deinit(instance->instance);

    gpio_set_function(instance->sda_gpio, GPIO_FUNC_NULL);
    gpio_set_function(instance->scl_gpio, GPIO_FUNC_NULL);
    gpio_disable_pulls(instance->sda_gpio);
    gpio_disable_pulls(instance->scl_gpio);

    instance->sda_gpio = 0;
    instance->scl_gpio = 0;
    instance->rate_hz = 0;
    instance->initialized = false;
}

bool I2C_LL_is_initialized(I2C_LL_Bus bus)
{
    return valid_bus(bus) && instances[bus].initialized;
}

uint32_t I2C_LL_get_rate(I2C_LL_Bus bus)
{
    return I2C_LL_is_initialized(bus) ? instances[bus].rate_hz : 0;
}

int32_t I2C_LL_write(
    I2C_LL_Bus bus,
    uint8_t address,
    uint8_t const *data,
    size_t len,
    bool nostop,
    uint32_t timeout_us
)
{
    if (!I2C_LL_is_initialized(bus) || !valid_address(address) ||
        (!data && len > 0)) {
        return PICO_ERROR_GENERIC;
    }

    return i2c_write_timeout_us(
        instances[bus].instance,
        address,
        data,
        len,
        nostop,
        timeout_us ? timeout_us : I2C_LL_DEFAULT_TIMEOUT_US
    );
}

int32_t I2C_LL_read(
    I2C_LL_Bus bus,
    uint8_t address,
    uint8_t *data,
    size_t len,
    bool nostop,
    uint32_t timeout_us
)
{
    if (!I2C_LL_is_initialized(bus) || !valid_address(address) ||
        !data || len == 0) {
        return PICO_ERROR_GENERIC;
    }

    return i2c_read_timeout_us(
        instances[bus].instance,
        address,
        data,
        len,
        nostop,
        timeout_us ? timeout_us : I2C_LL_DEFAULT_TIMEOUT_US
    );
}

bool I2C_LL_probe_address(
    I2C_LL_Bus bus,
    uint8_t address,
    uint32_t timeout_us
)
{
    uint8_t data = 0;

    if (!I2C_LL_is_initialized(bus) || !valid_address(address)) {
        return false;
    }

    int32_t result = I2C_LL_read(
        bus,
        address,
        &data,
        1,
        false,
        timeout_us
    );
    return result == 1;
}
