#include "application/gateway/spi_commands.h"

#include "system/bus/spi.h"

enum {
    SPI_GATEWAY_MIN_RATE_HZ = 1000,
    SPI_GATEWAY_MAX_RATE_HZ = 50000000,
};

static SPI_Handle *gateway_spi;

static struct {
    uint32_t bus;
    uint32_t rate_hz;
    uint32_t mode;
    uint32_t dummy_byte;
} state = {
    .bus = SPI_GATEWAY_DEFAULT_BUS,
    .rate_hz = SPI_GATEWAY_DEFAULT_RATE_HZ,
    .mode = SPI_GATEWAY_DEFAULT_MODE,
    .dummy_byte = SPI_GATEWAY_DEFAULT_DUMMY_BYTE,
};

bool spi_gateway_set_bus(uint32_t bus)
{
    if (gateway_spi || bus != SPI_GATEWAY_DEFAULT_BUS) {
        return false;
    }

    state.bus = bus;
    return true;
}

uint32_t spi_gateway_get_bus(void) { return state.bus; }

bool spi_gateway_set_rate(uint32_t rate_hz)
{
    if (gateway_spi || rate_hz < SPI_GATEWAY_MIN_RATE_HZ ||
        rate_hz > SPI_GATEWAY_MAX_RATE_HZ) {
        return false;
    }

    state.rate_hz = rate_hz;
    return true;
}

uint32_t spi_gateway_get_rate(void)
{
    return gateway_spi ? SPI_get_rate(gateway_spi) : state.rate_hz;
}

bool spi_gateway_set_mode(uint32_t mode)
{
    if (gateway_spi || mode > SPI_GATEWAY_DEFAULT_MODE + 3u) {
        return false;
    }

    state.mode = mode;
    return true;
}

uint32_t spi_gateway_get_mode(void)
{
    return gateway_spi ? (uint32_t)SPI_get_mode(gateway_spi) : state.mode;
}

bool spi_gateway_set_dummy_byte(uint32_t dummy_byte)
{
    if (gateway_spi || dummy_byte > 0xff) {
        return false;
    }

    state.dummy_byte = dummy_byte;
    return true;
}

uint32_t spi_gateway_get_dummy_byte(void)
{
    return gateway_spi ? SPI_get_dummy_byte(gateway_spi) : state.dummy_byte;
}

bool spi_gateway_open(void)
{
    if (gateway_spi) {
        return true;
    }

    SPI_Config config;
    if (!SPI_default_config(state.bus, &config)) {
        return false;
    }

    config.rate_hz = state.rate_hz;
    config.mode = (SPI_Mode)state.mode;
    config.dummy_byte = (uint8_t)state.dummy_byte;

    gateway_spi = SPI_init(&config);
    return gateway_spi != NULL;
}

void spi_gateway_close(void)
{
    if (!gateway_spi) {
        return;
    }

    SPI_Handle *handle = gateway_spi;
    gateway_spi = NULL;
    SPI_deinit(handle);
}

bool spi_gateway_is_open(void) { return gateway_spi != NULL; }

int32_t spi_gateway_write(uint8_t const *data, size_t len)
{
    if (!gateway_spi || !data || len == 0 || len > SPI_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    return SPI_write(gateway_spi, data, len);
}

int32_t spi_gateway_read(uint8_t *data, size_t len)
{
    if (!gateway_spi || !data || len == 0 || len > SPI_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    return SPI_read(gateway_spi, data, len);
}

int32_t spi_gateway_exchange(
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t len
)
{
    if (!gateway_spi || !tx_data || !rx_data || len == 0 ||
        len > SPI_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    return SPI_exchange(gateway_spi, tx_data, rx_data, len);
}

int32_t spi_gateway_transact(
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_len
)
{
    if (!gateway_spi || !tx_data || tx_len == 0 || !rx_data ||
        rx_len == 0 || tx_len > SPI_GATEWAY_MAX_TRANSFER ||
        rx_len > SPI_GATEWAY_MAX_TRANSFER) {
        return -1;
    }

    return SPI_transact(gateway_spi, tx_data, tx_len, rx_data, rx_len);
}
