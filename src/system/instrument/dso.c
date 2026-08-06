/**
 * @file dso.c
 * @brief Digital Storage Oscilloscope implementation for PSLab Pico firmware
 *
 * This file ports the PSLab DSO instrument layer to RP2350. The STM32 firmware
 * used ADC_LL and TIM_LL for timer-triggered acquisition; this Pico port keeps
 * the instrument-level DSO responsibility and delegates low-level ADC capture
 * to the ADC frontend interface.
 *
 * @author PSLab Team
 * @date 2025-09-29
 */

#include "system/instrument/dso.h"

#include <stdbool.h>
#include <string.h>

#include "platform/platform.h"
#include "platform/status_led.h"
#include "system/instrument/adc_frontend.h"

enum {
    DSO_DEFAULT_CHANNEL = 0,
    DSO_DEFAULT_SAMPLE_RATE_HZ = 100000,
    DSO_DEFAULT_SAMPLES = 1024,
    DSO_DEFAULT_TRIGGER_LEVEL = 2048,
    DSO_STREAM_TRIGGER_TIMEOUT_US = 10000,
};

static uint16_t capture_buffer[DSO_MAX_SAMPLES];
static DsoCaptureInfo last_capture;
static bool capture_valid;
static bool adc_initialized;
static bool stream_enabled;
static uint32_t stream_sequence;
static uint32_t stream_overruns;

static struct {
    uint32_t channel;
    uint32_t sample_rate_hz;
    uint32_t samples;
    uint32_t trigger_level;
    DsoTriggerMode trigger_mode;
    DsoTriggerSlope trigger_slope;
} state = {
    .channel = DSO_DEFAULT_CHANNEL,
    .sample_rate_hz = DSO_DEFAULT_SAMPLE_RATE_HZ,
    .samples = DSO_DEFAULT_SAMPLES,
    .trigger_level = DSO_DEFAULT_TRIGGER_LEVEL,
    .trigger_mode = DSO_TRIGGER_OFF,
    .trigger_slope = DSO_TRIGGER_RISING,
};

static bool config_is_valid(void)
{
    AdcFrontendCapabilities capabilities;
    return adc_frontend_get_capabilities(
               ADC_FRONTEND_BACKEND_INTERNAL,
               &capabilities
           ) &&
           state.channel <= capabilities.max_channel &&
           state.sample_rate_hz >= capabilities.min_sample_rate_hz &&
           state.sample_rate_hz <= capabilities.max_sample_rate_hz &&
           state.samples >= 1 && state.samples <= DSO_MAX_SAMPLES;
}

static bool apply_config(void)
{
    if (!config_is_valid()) {
        return false;
    }

    AdcFrontendConfig config = {
        .backend = ADC_FRONTEND_BACKEND_INTERNAL,
        .channel = state.channel,
        .sample_rate_hz = state.sample_rate_hz,
    };

    if (adc_initialized) {
        return adc_frontend_configure(&config);
    }

    adc_initialized = adc_frontend_init(&config);
    return adc_initialized;
}

static bool internal_adc_capabilities(AdcFrontendCapabilities *capabilities)
{
    return adc_frontend_get_capabilities(
        ADC_FRONTEND_BACKEND_INTERNAL,
        capabilities
    );
}

static bool set_and_maybe_reconfigure(
    uint32_t *target,
    uint32_t value,
    uint32_t min,
    uint32_t max,
    bool reconfigure
)
{
    if (value < min || value > max) {
        return false;
    }

    uint32_t old_value = *target;
    *target = value;
    capture_valid = false;
    dso_stream_stop();

    if (reconfigure && !apply_config()) {
        *target = old_value;
        return false;
    }

    return true;
}

void dso_reset_state(void)
{
    dso_stream_stop();

    if (adc_initialized) {
        adc_frontend_deinit();
    }

    adc_initialized = false;
    capture_valid = false;
    stream_sequence = 0;
    stream_overruns = 0;
    state.channel = DSO_DEFAULT_CHANNEL;
    state.sample_rate_hz = DSO_DEFAULT_SAMPLE_RATE_HZ;
    state.samples = DSO_DEFAULT_SAMPLES;
    state.trigger_level = DSO_DEFAULT_TRIGGER_LEVEL;
    state.trigger_mode = DSO_TRIGGER_OFF;
    state.trigger_slope = DSO_TRIGGER_RISING;
}

bool dso_set_channel(uint32_t value)
{
    AdcFrontendCapabilities capabilities;
    if (!internal_adc_capabilities(&capabilities)) {
        return false;
    }

    return set_and_maybe_reconfigure(
        &state.channel,
        value,
        0,
        capabilities.max_channel,
        true
    );
}

bool dso_set_sample_rate(uint32_t value)
{
    AdcFrontendCapabilities capabilities;
    if (!internal_adc_capabilities(&capabilities)) {
        return false;
    }

    return set_and_maybe_reconfigure(
        &state.sample_rate_hz,
        value,
        capabilities.min_sample_rate_hz,
        capabilities.max_sample_rate_hz,
        true
    );
}

bool dso_set_samples(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.samples, value, 1, DSO_MAX_SAMPLES, false
    );
}

bool dso_set_trigger_level(uint32_t value)
{
    return set_and_maybe_reconfigure(&state.trigger_level, value, 0, 4095, false);
}

bool dso_set_trigger_mode(DsoTriggerMode mode)
{
    if (mode != DSO_TRIGGER_OFF && mode != DSO_TRIGGER_LEVEL &&
        mode != DSO_TRIGGER_EDGE) {
        return false;
    }

    state.trigger_mode = mode;
    capture_valid = false;
    dso_stream_stop();
    return true;
}

bool dso_set_trigger_slope(DsoTriggerSlope slope)
{
    if (slope != DSO_TRIGGER_RISING && slope != DSO_TRIGGER_FALLING) {
        return false;
    }

    state.trigger_slope = slope;
    capture_valid = false;
    dso_stream_stop();
    return true;
}

uint32_t dso_get_channel(void) { return state.channel; }

uint32_t dso_get_gpio(void)
{
    return adc_frontend_channel_to_gpio(
        ADC_FRONTEND_BACKEND_INTERNAL,
        state.channel
    );
}

uint32_t dso_get_sample_rate(void) { return state.sample_rate_hz; }

uint32_t dso_get_samples(void) { return state.samples; }

uint32_t dso_get_trigger_level(void) { return state.trigger_level; }

DsoTriggerMode dso_get_trigger_mode(void) { return state.trigger_mode; }

DsoTriggerSlope dso_get_trigger_slope(void) { return state.trigger_slope; }

static bool sample_matches_level(uint16_t sample)
{
    if (state.trigger_slope == DSO_TRIGGER_FALLING) {
        return sample <= state.trigger_level;
    }

    return sample >= state.trigger_level;
}

static bool trigger_wait_timed_out(bool use_timeout, uint64_t deadline_us)
{
    return use_timeout && PLATFORM_get_time_us() >= deadline_us;
}

static bool wait_for_trigger_timeout(uint32_t timeout_us)
{
    if (state.trigger_mode == DSO_TRIGGER_OFF) {
        return true;
    }

    bool use_timeout = timeout_us > 0;
    uint64_t deadline_us = use_timeout ?
        PLATFORM_get_time_us() + timeout_us :
        0;

    uint16_t sample;
    if (state.trigger_mode == DSO_TRIGGER_LEVEL) {
        do {
            if (trigger_wait_timed_out(use_timeout, deadline_us)) {
                return false;
            }
            if (!adc_frontend_read_once(&sample)) {
                return false;
            }
            PLATFORM_idle();
        } while (!sample_matches_level(sample));
        return true;
    }

    bool armed = false;
    while (true) {
        if (trigger_wait_timed_out(use_timeout, deadline_us)) {
            return false;
        }
        if (!adc_frontend_read_once(&sample)) {
            return false;
        }

        bool matched = sample_matches_level(sample);
        if (!armed) {
            armed = !matched;
        } else if (matched) {
            return true;
        }
        PLATFORM_idle();
    }
}

static bool wait_for_trigger(void)
{
    return wait_for_trigger_timeout(0);
}

bool dso_initiate(void)
{
    dso_stream_stop();

    if (!apply_config()) {
        return false;
    }

    memset(capture_buffer, 0, state.samples * sizeof(capture_buffer[0]));

    status_led_capture_started();
    AdcFrontendCaptureInfo info;
    bool captured = wait_for_trigger() &&
                    adc_frontend_run(capture_buffer, state.samples, &info);
    status_led_capture_finished();

    if (!captured) {
        return false;
    }

    last_capture = (DsoCaptureInfo){
        .channel = info.channel,
        .gpio = info.gpio,
        .sample_rate_hz = info.sample_rate_hz,
        .sample_count = info.sample_count,
    };
    capture_valid = true;
    return true;
}

bool dso_fetch(uint8_t const **data, size_t *len)
{
    if (!capture_valid || !data || !len) {
        return false;
    }

    *data = (uint8_t const *)capture_buffer;
    *len = last_capture.sample_count * sizeof(uint16_t);
    return true;
}

uint32_t dso_status(void)
{
    if (stream_enabled || adc_frontend_is_busy()) {
        return 2;
    }

    return capture_valid ? 1 : 0;
}

bool dso_stream_start(void)
{
    dso_stream_stop();

    if (!apply_config()) {
        ++stream_overruns;
        return false;
    }

    capture_valid = false;
    stream_enabled = true;
    stream_sequence = 0;
    stream_overruns = 0;
    status_led_capture_started();
    return true;
}

void dso_stream_stop(void)
{
    if (stream_enabled) {
        status_led_capture_finished();
    }
    stream_enabled = false;
}

bool dso_stream_is_enabled(void) { return stream_enabled; }

uint32_t dso_stream_get_sequence(void) { return stream_sequence; }

uint32_t dso_stream_get_overruns(void) { return stream_overruns; }

bool dso_stream_next_frame(uint8_t const **data, size_t *len, uint32_t *sequence)
{
    if (!stream_enabled || !data || !len || !sequence) {
        return false;
    }

    AdcFrontendCaptureInfo info;
    bool captured = wait_for_trigger_timeout(DSO_STREAM_TRIGGER_TIMEOUT_US) &&
                    adc_frontend_run(capture_buffer, state.samples, &info);
    if (!captured) {
        ++stream_overruns;
        return false;
    }

    last_capture = (DsoCaptureInfo){
        .channel = info.channel,
        .gpio = info.gpio,
        .sample_rate_hz = info.sample_rate_hz,
        .sample_count = info.sample_count,
    };
    capture_valid = true;
    *data = (uint8_t const *)capture_buffer;
    *len = last_capture.sample_count * sizeof(uint16_t);
    *sequence = stream_sequence++;
    return true;
}
