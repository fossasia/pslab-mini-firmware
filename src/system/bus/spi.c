/**
 * @file spi.c
 * @brief Hardware-independent SPI bus implementation.
 */

#include "system/bus/spi.h"

#include <stdlib.h>

#include "platform/spi_ll.h"

struct SPI_Handle {
    uint32_t bus;
    SPI_LL_Bus ll_bus;
    uint8_t dummy_byte;
    bool initialized;
};

static SPI_Handle *active_handles[SPI_BUS_COUNT];

static bool valid_bus(uint32_t bus) { return bus < SPI_BUS_COUNT; }

static SPI_LL_Bus to_ll_bus(uint32_t bus)
{
    (void)bus;
    return SPI_LL_BUS_1;
}

static bool valid_mode(SPI_Mode mode) { return mode <= SPI_MODE_3; }

static bool valid_bit_order(SPI_BitOrder bit_order)
{
    return bit_order <= SPI_BIT_ORDER_LSB_FIRST;
}

static SPI_LL_Mode to_ll_mode(SPI_Mode mode) { return (SPI_LL_Mode)mode; }

static SPI_Mode from_ll_mode(SPI_LL_Mode mode) { return (SPI_Mode)mode; }

static SPI_LL_BitOrder to_ll_bit_order(SPI_BitOrder bit_order)
{
    return (SPI_LL_BitOrder)bit_order;
}

static SPI_BitOrder from_ll_bit_order(SPI_LL_BitOrder bit_order)
{
    return (SPI_BitOrder)bit_order;
}

size_t SPI_get_bus_count(void) { return SPI_BUS_COUNT; }

bool SPI_default_config(uint32_t bus, SPI_Config *config)
{
    if (!valid_bus(bus) || !config) {
        return false;
    }

    SPI_LL_Config ll_config;
    if (!SPI_LL_default_config(to_ll_bus(bus), &ll_config)) {
        return false;
    }

    *config = (SPI_Config){
        .bus = bus,
        .sck_gpio = ll_config.sck_gpio,
        .tx_gpio = ll_config.tx_gpio,
        .rx_gpio = ll_config.rx_gpio,
        .cs_gpio = ll_config.cs_gpio,
        .rate_hz = ll_config.rate_hz,
        .mode = from_ll_mode(ll_config.mode),
        .bit_order = from_ll_bit_order(ll_config.bit_order),
        .dummy_byte = SPI_DEFAULT_DUMMY_BYTE,
        .cs_active_low = ll_config.cs_active_low,
    };
    return true;
}

SPI_Handle *SPI_init(SPI_Config const *config)
{
    if (!config || !valid_bus(config->bus) || config->rate_hz == 0 ||
        !valid_mode(config->mode) || !valid_bit_order(config->bit_order) ||
        active_handles[config->bus]) {
        return NULL;
    }

    SPI_Handle *handle = malloc(sizeof(*handle));
    if (!handle) {
        return NULL;
    }

    SPI_LL_Config ll_config = {
        .sck_gpio = config->sck_gpio,
        .tx_gpio = config->tx_gpio,
        .rx_gpio = config->rx_gpio,
        .cs_gpio = config->cs_gpio,
        .rate_hz = config->rate_hz,
        .mode = to_ll_mode(config->mode),
        .bit_order = to_ll_bit_order(config->bit_order),
        .cs_active_low = config->cs_active_low,
    };

    SPI_LL_Bus ll_bus = to_ll_bus(config->bus);
    if (!SPI_LL_init(ll_bus, &ll_config)) {
        free(handle);
        return NULL;
    }

    *handle = (SPI_Handle){
        .bus = config->bus,
        .ll_bus = ll_bus,
        .dummy_byte = config->dummy_byte,
        .initialized = true,
    };
    active_handles[config->bus] = handle;
    return handle;
}

void SPI_deinit(SPI_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }

    uint32_t bus = handle->bus;
    SPI_LL_deinit(handle->ll_bus);
    if (valid_bus(bus) && active_handles[bus] == handle) {
        active_handles[bus] = NULL;
    }

    handle->initialized = false;
    free(handle);
}

bool SPI_is_ready(SPI_Handle const *handle)
{
    return handle && handle->initialized &&
           SPI_LL_is_initialized(handle->ll_bus);
}

uint32_t SPI_get_bus(SPI_Handle const *handle)
{
    return SPI_is_ready(handle) ? handle->bus : SPI_BUS_COUNT;
}

uint32_t SPI_get_rate(SPI_Handle const *handle)
{
    return SPI_is_ready(handle) ? SPI_LL_get_rate(handle->ll_bus) : 0;
}

SPI_Mode SPI_get_mode(SPI_Handle const *handle)
{
    return SPI_is_ready(handle) ? from_ll_mode(SPI_LL_get_mode(handle->ll_bus))
                                : SPI_MODE_0;
}

SPI_BitOrder SPI_get_bit_order(SPI_Handle const *handle)
{
    return SPI_is_ready(handle)
               ? from_ll_bit_order(SPI_LL_get_bit_order(handle->ll_bus))
               : SPI_BIT_ORDER_MSB_FIRST;
}

uint8_t SPI_get_dummy_byte(SPI_Handle const *handle)
{
    return SPI_is_ready(handle) ? handle->dummy_byte : SPI_DEFAULT_DUMMY_BYTE;
}

int32_t SPI_exchange(
    SPI_Handle *handle,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t len
)
{
    if (!SPI_is_ready(handle) || !tx_data || !rx_data || len == 0) {
        return -1;
    }

    if (!SPI_LL_select(handle->ll_bus)) {
        return -1;
    }

    int32_t result = SPI_LL_transfer(
        handle->ll_bus,
        tx_data,
        rx_data,
        len,
        handle->dummy_byte
    );
    SPI_LL_deselect(handle->ll_bus);
    return result;
}

int32_t SPI_write(SPI_Handle *handle, uint8_t const *data, size_t len)
{
    if (!SPI_is_ready(handle) || !data || len == 0) {
        return -1;
    }

    if (!SPI_LL_select(handle->ll_bus)) {
        return -1;
    }

    int32_t result = SPI_LL_transfer(
        handle->ll_bus,
        data,
        NULL,
        len,
        handle->dummy_byte
    );
    SPI_LL_deselect(handle->ll_bus);
    return result;
}

int32_t SPI_read(SPI_Handle *handle, uint8_t *data, size_t len)
{
    if (!SPI_is_ready(handle) || !data || len == 0) {
        return -1;
    }

    if (!SPI_LL_select(handle->ll_bus)) {
        return -1;
    }

    int32_t result = SPI_LL_transfer(
        handle->ll_bus,
        NULL,
        data,
        len,
        handle->dummy_byte
    );
    SPI_LL_deselect(handle->ll_bus);
    return result;
}

int32_t SPI_transact(
    SPI_Handle *handle,
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_len
)
{
    if (!SPI_is_ready(handle) || !tx_data || tx_len == 0 || !rx_data ||
        rx_len == 0) {
        return -1;
    }

    if (!SPI_LL_select(handle->ll_bus)) {
        return -1;
    }

    int32_t written = SPI_LL_transfer(
        handle->ll_bus,
        tx_data,
        NULL,
        tx_len,
        handle->dummy_byte
    );
    if (written < 0) {
        SPI_LL_deselect(handle->ll_bus);
        return written;
    }
    if ((size_t)written != tx_len) {
        SPI_LL_deselect(handle->ll_bus);
        return -1;
    }

    int32_t read = SPI_LL_transfer(
        handle->ll_bus,
        NULL,
        rx_data,
        rx_len,
        handle->dummy_byte
    );
    SPI_LL_deselect(handle->ll_bus);
    return read;
}
