#include "system/instrument/adc_frontend.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    ADC_FRONTEND_MAX_DRIVERS = 2,
};

static AdcFrontendDriver const *drivers[ADC_FRONTEND_MAX_DRIVERS];
static AdcFrontendDriver const *active_driver;
static bool initialized;
static AdcFrontendCaptureInfo last_info;
static AdcFrontendCompleteCallback complete_callback;
static void *complete_user_data;

static AdcFrontendDriver const *find_driver(AdcFrontendBackend backend)
{
    for (size_t i = 0; i < ADC_FRONTEND_MAX_DRIVERS; ++i) {
        if (drivers[i] && drivers[i]->backend == backend) {
            return drivers[i];
        }
    }

    return NULL;
}

static bool driver_is_valid(AdcFrontendDriver const *driver)
{
    return driver && driver->backend != ADC_FRONTEND_BACKEND_NONE &&
           driver->name && driver->init && driver->deinit &&
           driver->configure && driver->run && driver->arm &&
           driver->start && driver->wait && driver->abort &&
           driver->read_once && driver->is_busy && driver->channel_to_gpio &&
           driver->capabilities.max_sample_count > 0;
}

static void notify_complete(void)
{
    if (complete_callback) {
        complete_callback(&last_info, complete_user_data);
    }
}

static bool sample_count_is_valid(uint32_t sample_count)
{
    return sample_count > 0 &&
           active_driver &&
           sample_count <= active_driver->capabilities.max_sample_count;
}

bool adc_frontend_register_driver(AdcFrontendDriver const *driver)
{
    if (!driver_is_valid(driver) || find_driver(driver->backend)) {
        return false;
    }

    for (size_t i = 0; i < ADC_FRONTEND_MAX_DRIVERS; ++i) {
        if (!drivers[i]) {
            drivers[i] = driver;
            return true;
        }
    }

    return false;
}

bool adc_frontend_select_backend(AdcFrontendBackend backend)
{
    AdcFrontendDriver const *driver = find_driver(backend);
    if (!driver) {
        return false;
    }

    if (active_driver == driver) {
        return true;
    }

    if (initialized && active_driver) {
        active_driver->deinit();
    }

    active_driver = driver;
    initialized = false;
    memset(&last_info, 0, sizeof(last_info));
    return true;
}

AdcFrontendBackend adc_frontend_get_backend(void)
{
    return active_driver ? active_driver->backend : ADC_FRONTEND_BACKEND_NONE;
}

bool adc_frontend_get_capabilities(
    AdcFrontendBackend backend,
    AdcFrontendCapabilities *capabilities
)
{
    AdcFrontendDriver const *driver = find_driver(backend);
    if (!driver || !capabilities) {
        return false;
    }

    *capabilities = driver->capabilities;
    return true;
}

bool adc_frontend_init(AdcFrontendConfig const *config)
{
    if (!config || !adc_frontend_select_backend(config->backend)) {
        return false;
    }

    initialized = active_driver->init(config);
    return initialized;
}

void adc_frontend_deinit(void)
{
    if (initialized && active_driver) {
        active_driver->deinit();
    }

    initialized = false;
    memset(&last_info, 0, sizeof(last_info));
}

bool adc_frontend_configure(AdcFrontendConfig const *config)
{
    if (!config) {
        return false;
    }

    if (!adc_frontend_select_backend(config->backend)) {
        return false;
    }

    if (!initialized) {
        return adc_frontend_init(config);
    }

    return active_driver->configure(config);
}

bool adc_frontend_run(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcFrontendCaptureInfo *info
)
{
    if (!initialized || !active_driver || !sample_count_is_valid(sample_count)) {
        return false;
    }

    bool complete = active_driver->run(buffer, sample_count, &last_info);
    if (!complete) {
        return false;
    }

    if (info) {
        *info = last_info;
    }
    notify_complete();
    return true;
}

bool adc_frontend_arm(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcFrontendCaptureInfo *info
)
{
    if (!initialized || !active_driver || !sample_count_is_valid(sample_count)) {
        return false;
    }

    bool armed = active_driver->arm(buffer, sample_count, &last_info);
    if (armed && info) {
        *info = last_info;
    }

    return armed;
}

bool adc_frontend_start(void)
{
    return initialized && active_driver && active_driver->start();
}

bool adc_frontend_wait(void)
{
    if (!initialized || !active_driver || !active_driver->wait()) {
        return false;
    }

    notify_complete();
    return true;
}

void adc_frontend_abort(void)
{
    if (initialized && active_driver) {
        active_driver->abort();
    }
}

bool adc_frontend_read_once(uint16_t *sample)
{
    return initialized && active_driver && active_driver->read_once(sample);
}

bool adc_frontend_is_busy(void)
{
    return initialized && active_driver && active_driver->is_busy();
}

uint32_t adc_frontend_channel_to_gpio(
    AdcFrontendBackend backend,
    uint32_t channel
)
{
    AdcFrontendDriver const *driver = find_driver(backend);
    return driver ? driver->channel_to_gpio(channel) : 0;
}

void adc_frontend_set_complete_callback(
    AdcFrontendCompleteCallback callback,
    void *user_data
)
{
    complete_callback = callback;
    complete_user_data = user_data;
}
