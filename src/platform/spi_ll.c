/**
 * @file spi_ll.c
 * @brief Low-level SPI hardware implementation.
 */

#include "platform/spi_ll.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/error.h"

typedef struct {
    spi_inst_t *instance;
    uint32_t sck_gpio;
    uint32_t tx_gpio;
    uint32_t rx_gpio;
    uint32_t cs_gpio;
    uint32_t rate_hz;
    SPI_LL_Mode mode;
    SPI_LL_BitOrder bit_order;
    bool cs_active_low;
    bool selected;
    bool initialized;
} SPI_LL_Instance;

static SPI_LL_Instance instances[SPI_LL_BUS_COUNT] = {
    [SPI_LL_BUS_1] = {
        .instance = spi1,
    },
};

static bool valid_bus(SPI_LL_Bus bus) { return bus == SPI_LL_BUS_1; }

static bool valid_gpio(uint32_t gpio) { return gpio <= 29; }

static bool valid_mode(SPI_LL_Mode mode) { return mode <= SPI_LL_MODE_3; }

static bool valid_bit_order(SPI_LL_BitOrder bit_order)
{
    return bit_order <= SPI_LL_BIT_ORDER_LSB_FIRST;
}

static bool valid_spi1_rx_gpio(uint32_t gpio)
{
    return gpio == 8 || gpio == 12 || gpio == 24 || gpio == 28;
}

static bool valid_spi1_cs_gpio(uint32_t gpio)
{
    return gpio == 9 || gpio == 13 || gpio == 25 || gpio == 29;
}

static bool valid_spi1_sck_gpio(uint32_t gpio)
{
    return gpio == 10 || gpio == 14 || gpio == 26;
}

static bool valid_spi1_tx_gpio(uint32_t gpio)
{
    return gpio == 11 || gpio == 15 || gpio == 27;
}

static bool pins_match_bus(SPI_LL_Bus bus, SPI_LL_Config const *config)
{
    return bus == SPI_LL_BUS_1 && valid_spi1_sck_gpio(config->sck_gpio) &&
           valid_spi1_tx_gpio(config->tx_gpio) &&
           valid_spi1_rx_gpio(config->rx_gpio) &&
           valid_spi1_cs_gpio(config->cs_gpio);
}

static bool pins_are_distinct(SPI_LL_Config const *config)
{
    return config->sck_gpio != config->tx_gpio &&
           config->sck_gpio != config->rx_gpio &&
           config->sck_gpio != config->cs_gpio &&
           config->tx_gpio != config->rx_gpio &&
           config->tx_gpio != config->cs_gpio &&
           config->rx_gpio != config->cs_gpio;
}

static spi_cpol_t pico_cpol(SPI_LL_Mode mode)
{
    return mode == SPI_LL_MODE_2 || mode == SPI_LL_MODE_3 ? SPI_CPOL_1
                                                          : SPI_CPOL_0;
}

static spi_cpha_t pico_cpha(SPI_LL_Mode mode)
{
    return mode == SPI_LL_MODE_1 || mode == SPI_LL_MODE_3 ? SPI_CPHA_1
                                                          : SPI_CPHA_0;
}

static spi_order_t pico_bit_order(SPI_LL_BitOrder bit_order)
{
    return bit_order == SPI_LL_BIT_ORDER_LSB_FIRST ? SPI_LSB_FIRST
                                                   : SPI_MSB_FIRST;
}

static void set_cs_inactive(SPI_LL_Instance const *instance)
{
    gpio_put(instance->cs_gpio, instance->cs_active_low ? 1 : 0);
}

static void set_cs_active(SPI_LL_Instance const *instance)
{
    gpio_put(instance->cs_gpio, instance->cs_active_low ? 0 : 1);
}

static bool config_is_valid(SPI_LL_Bus bus, SPI_LL_Config const *config)
{
    return config && config->rate_hz > 0 && valid_gpio(config->sck_gpio) &&
           valid_gpio(config->tx_gpio) && valid_gpio(config->rx_gpio) &&
           valid_gpio(config->cs_gpio) && pins_match_bus(bus, config) &&
           pins_are_distinct(config) && valid_mode(config->mode) &&
           valid_bit_order(config->bit_order);
}

bool SPI_LL_default_config(SPI_LL_Bus bus, SPI_LL_Config *config)
{
    if (!valid_bus(bus) || !config) {
        return false;
    }

    *config = (SPI_LL_Config){
        .sck_gpio = 10u,
        .tx_gpio = 11u,
        .rx_gpio = 12u,
        .cs_gpio = 13u,
        .rate_hz = SPI_LL_DEFAULT_RATE_HZ,
        .mode = SPI_LL_MODE_0,
        .bit_order = SPI_LL_BIT_ORDER_MSB_FIRST,
        .cs_active_low = true,
    };
    return true;
}

bool SPI_LL_init(SPI_LL_Bus bus, SPI_LL_Config const *config)
{
    if (!valid_bus(bus) || !config_is_valid(bus, config) ||
        instances[bus].initialized) {
        return false;
    }

    SPI_LL_Instance *instance = &instances[bus];
    uint32_t actual_rate = spi_init(instance->instance, config->rate_hz);
    spi_set_format(
        instance->instance,
        8,
        pico_cpol(config->mode),
        pico_cpha(config->mode),
        pico_bit_order(config->bit_order)
    );

    gpio_set_function(config->rx_gpio, GPIO_FUNC_SPI);
    gpio_set_function(config->sck_gpio, GPIO_FUNC_SPI);
    gpio_set_function(config->tx_gpio, GPIO_FUNC_SPI);

    gpio_init(config->cs_gpio);
    gpio_set_dir(config->cs_gpio, GPIO_OUT);

    instance->sck_gpio = config->sck_gpio;
    instance->tx_gpio = config->tx_gpio;
    instance->rx_gpio = config->rx_gpio;
    instance->cs_gpio = config->cs_gpio;
    instance->rate_hz = actual_rate;
    instance->mode = config->mode;
    instance->bit_order = config->bit_order;
    instance->cs_active_low = config->cs_active_low;
    instance->selected = false;
    instance->initialized = true;

    set_cs_inactive(instance);
    return true;
}

void SPI_LL_deinit(SPI_LL_Bus bus)
{
    if (!valid_bus(bus) || !instances[bus].initialized) {
        return;
    }

    SPI_LL_Instance *instance = &instances[bus];
    set_cs_inactive(instance);
    spi_deinit(instance->instance);

    gpio_set_function(instance->sck_gpio, GPIO_FUNC_NULL);
    gpio_set_function(instance->tx_gpio, GPIO_FUNC_NULL);
    gpio_set_function(instance->rx_gpio, GPIO_FUNC_NULL);
    gpio_set_function(instance->cs_gpio, GPIO_FUNC_NULL);
    gpio_disable_pulls(instance->sck_gpio);
    gpio_disable_pulls(instance->tx_gpio);
    gpio_disable_pulls(instance->rx_gpio);
    gpio_disable_pulls(instance->cs_gpio);

    instance->sck_gpio = 0;
    instance->tx_gpio = 0;
    instance->rx_gpio = 0;
    instance->cs_gpio = 0;
    instance->rate_hz = 0;
    instance->mode = SPI_LL_MODE_0;
    instance->bit_order = SPI_LL_BIT_ORDER_MSB_FIRST;
    instance->cs_active_low = true;
    instance->selected = false;
    instance->initialized = false;
}

bool SPI_LL_is_initialized(SPI_LL_Bus bus)
{
    return valid_bus(bus) && instances[bus].initialized;
}

uint32_t SPI_LL_get_rate(SPI_LL_Bus bus)
{
    return SPI_LL_is_initialized(bus) ? instances[bus].rate_hz : 0;
}

SPI_LL_Mode SPI_LL_get_mode(SPI_LL_Bus bus)
{
    return SPI_LL_is_initialized(bus) ? instances[bus].mode : SPI_LL_MODE_0;
}

SPI_LL_BitOrder SPI_LL_get_bit_order(SPI_LL_Bus bus)
{
    return SPI_LL_is_initialized(bus) ? instances[bus].bit_order
                                      : SPI_LL_BIT_ORDER_MSB_FIRST;
}

bool SPI_LL_select(SPI_LL_Bus bus)
{
    if (!SPI_LL_is_initialized(bus)) {
        return false;
    }

    SPI_LL_Instance *instance = &instances[bus];
    set_cs_active(instance);
    instance->selected = true;
    return true;
}

void SPI_LL_deselect(SPI_LL_Bus bus)
{
    if (!SPI_LL_is_initialized(bus)) {
        return;
    }

    SPI_LL_Instance *instance = &instances[bus];
    set_cs_inactive(instance);
    instance->selected = false;
}

bool SPI_LL_is_selected(SPI_LL_Bus bus)
{
    return SPI_LL_is_initialized(bus) && instances[bus].selected;
}

int32_t SPI_LL_transfer(
    SPI_LL_Bus bus,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t len
)
{
    if (!SPI_LL_is_initialized(bus) || len == 0 ||
        (!tx_data && !rx_data)) {
        return PICO_ERROR_GENERIC;
    }

    if (tx_data && rx_data) {
        return spi_write_read_blocking(
            instances[bus].instance,
            tx_data,
            rx_data,
            len
        );
    }

    if (tx_data) {
        return spi_write_blocking(instances[bus].instance, tx_data, len);
    }

    return spi_read_blocking(instances[bus].instance, 0xff, rx_data, len);
}
