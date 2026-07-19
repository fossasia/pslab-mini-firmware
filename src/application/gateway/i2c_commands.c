#include "application/gateway/i2c_commands.h"

#include "system/bus/i2c.h"

enum {
    I2C_GATEWAY_MIN_RATE_HZ = 1000,
    I2C_GATEWAY_MAX_RATE_HZ = 1000000,
    I2C_GATEWAY_MAX_TIMEOUT_MS = 60000,
};

static I2C_Handle *gateway_i2c;

static struct {
    uint32_t bus;
    uint32_t address;
    uint32_t rate_hz;
    uint32_t timeout_ms;
} state = {
    .bus = I2C_GATEWAY_DEFAULT_BUS,
    .address = I2C_GATEWAY_DEFAULT_ADDRESS,
    .rate_hz = I2C_GATEWAY_DEFAULT_RATE_HZ,
    .timeout_ms = I2C_GATEWAY_DEFAULT_TIMEOUT_MS,
};

static bool address_is_allowed(uint32_t address)
{
    return address >= 0x08 && address < 0x78;
}

bool i2c_gateway_set_bus(uint32_t bus)
{
    if (gateway_i2c || bus >= I2C_get_bus_count()) {
        return false;
    }

    state.bus = bus;
    return true;
}

uint32_t i2c_gateway_get_bus(void) { return state.bus; }

bool i2c_gateway_set_rate(uint32_t rate_hz)
{
    if (gateway_i2c || rate_hz < I2C_GATEWAY_MIN_RATE_HZ ||
        rate_hz > I2C_GATEWAY_MAX_RATE_HZ) {
        return false;
    }

    state.rate_hz = rate_hz;
    return true;
}

uint32_t i2c_gateway_get_rate(void)
{
    return gateway_i2c ? I2C_get_rate(gateway_i2c) : state.rate_hz;
}

bool i2c_gateway_set_address(uint32_t address)
{
    if (!address_is_allowed(address)) {
        return false;
    }

    state.address = address;
    return true;
}

uint32_t i2c_gateway_get_address(void) { return state.address; }

bool i2c_gateway_set_timeout(uint32_t timeout_ms)
{
    if (timeout_ms == 0 || timeout_ms > I2C_GATEWAY_MAX_TIMEOUT_MS) {
        return false;
    }

    state.timeout_ms = timeout_ms;
    return true;
}

uint32_t i2c_gateway_get_timeout(void) { return state.timeout_ms; }

bool i2c_gateway_open(void)
{
    if (gateway_i2c) {
        return true;
    }

    if (state.bus >= I2C_get_bus_count()) {
        return false;
    }

    I2C_Config config;
    if (!I2C_default_config(state.bus, &config)) {
        return false;
    }

    config.rate_hz = state.rate_hz;
    config.timeout_us = state.timeout_ms * 1000u;

    gateway_i2c = I2C_init(&config);
    return gateway_i2c != NULL;
}

void i2c_gateway_close(void)
{
    if (!gateway_i2c) {
        return;
    }

    I2C_Handle *handle = gateway_i2c;
    gateway_i2c = NULL;
    I2C_deinit(handle);
}

bool i2c_gateway_is_open(void) { return gateway_i2c != NULL; }

uint32_t i2c_gateway_scan(uint8_t *addresses, size_t max_count)
{
    if (!gateway_i2c || !addresses || max_count == 0) {
        return 0;
    }

    if (max_count > I2C_GATEWAY_MAX_SCAN_RESULTS) {
        max_count = I2C_GATEWAY_MAX_SCAN_RESULTS;
    }

    return I2C_scan(gateway_i2c, addresses, max_count);
}

int32_t i2c_gateway_write(uint8_t const *data, size_t len)
{
    if (!gateway_i2c || (!data && len > 0) ||
        len > I2C_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    return I2C_write(gateway_i2c, (uint8_t)state.address, data, len, false);
}

int32_t i2c_gateway_read(uint8_t *data, size_t len)
{
    if (!gateway_i2c || !data || len == 0 ||
        len > I2C_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    return I2C_read(gateway_i2c, (uint8_t)state.address, data, len, false);
}

int32_t i2c_gateway_transact(
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_len
)
{
    if (!gateway_i2c || (!tx_data && tx_len > 0) || !rx_data ||
        rx_len == 0 || tx_len > I2C_GATEWAY_MAX_TRANSFER ||
        rx_len > I2C_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    int32_t written = I2C_write(
        gateway_i2c,
        (uint8_t)state.address,
        tx_data,
        tx_len,
        true
    );
    if (written < 0 || (size_t)written != tx_len) {
        return -1;
    }

    return I2C_read(gateway_i2c, (uint8_t)state.address, rx_data, rx_len, false);
}
