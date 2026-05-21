#include "application/logic_analyser_commands.h"

#include <string.h>

#include "hardware/pio.h"
#include "platform/logic_analyser.h"
#include "system/status_led.h"

enum {
    LA_DEFAULT_PIN_BASE = 16,
    LA_DEFAULT_PIN_COUNT = 2,
    LA_DEFAULT_SAMPLES = 96,
    LA_DEFAULT_DIVIDER = 1,
    LA_MAX_PIN_COUNT = 8,
    LA_MAX_SAMPLES = 4096,
    LA_MAX_DIVIDER = 16777215,
};

static LogicAnalyser la;
static bool la_initialized;
static uint32_t capture_buffer[LA_MAX_SAMPLES];
static LogicAnalyserCaptureInfo last_capture;
static bool capture_valid;

static struct {
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t samples;
    uint32_t divider;
    uint32_t trigger_pin;
    bool trigger_level;
    LogicAnalyserTriggerMode trigger_mode;
} state = {
    .pin_base = LA_DEFAULT_PIN_BASE,
    .pin_count = LA_DEFAULT_PIN_COUNT,
    .samples = LA_DEFAULT_SAMPLES,
    .divider = LA_DEFAULT_DIVIDER,
    .trigger_pin = LA_DEFAULT_PIN_BASE,
    .trigger_level = true,
    .trigger_mode = LOGIC_ANALYSER_TRIGGER_EDGE,
};

static bool config_is_valid(void)
{
    return state.pin_count >= 1 && state.pin_count <= LA_MAX_PIN_COUNT &&
           state.pin_base + state.pin_count <= 30 && state.samples >= 1 &&
           state.samples <= LA_MAX_SAMPLES && state.divider >= 1 &&
           state.trigger_pin <= 29;
}

static bool apply_config(void)
{
    if (!config_is_valid()) {
        return false;
    }

    LogicAnalyserConfig config = {
        .pio = pio0,
        .sm = 0,
        .pin_base = state.pin_base,
        .pin_count = state.pin_count,
        .clk_div = (float)state.divider,
    };

    if (la_initialized) {
        return logic_analyser_configure(&la, &config);
    }

    la_initialized = logic_analyser_init(&la, &config);
    return la_initialized;
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

    if (reconfigure && !apply_config()) {
        *target = old_value;
        return false;
    }

    return true;
}

void la_reset_state(void)
{
    if (la_initialized) {
        logic_analyser_deinit(&la);
    }

    la_initialized = false;
    capture_valid = false;
    state.pin_base = LA_DEFAULT_PIN_BASE;
    state.pin_count = LA_DEFAULT_PIN_COUNT;
    state.samples = LA_DEFAULT_SAMPLES;
    state.divider = LA_DEFAULT_DIVIDER;
    state.trigger_pin = LA_DEFAULT_PIN_BASE;
    state.trigger_level = true;
    state.trigger_mode = LOGIC_ANALYSER_TRIGGER_EDGE;
}

bool la_set_pin_base(uint32_t value)
{
    bool ok = set_and_maybe_reconfigure(&state.pin_base, value, 0, 29, true);
    if (ok && state.trigger_pin < state.pin_base) {
        state.trigger_pin = state.pin_base;
    }
    return ok;
}

bool la_set_pin_count(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.pin_count, value, 1, LA_MAX_PIN_COUNT, true
    );
}

bool la_set_samples(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.samples, value, 1, LA_MAX_SAMPLES, false
    );
}

bool la_set_divider(uint32_t value)
{
    return set_and_maybe_reconfigure(&state.divider, value, 1, LA_MAX_DIVIDER, true);
}

bool la_set_trigger_pin(uint32_t value)
{
    return set_and_maybe_reconfigure(&state.trigger_pin, value, 0, 29, false);
}

void la_set_trigger_level(bool value)
{
    state.trigger_level = value;
    capture_valid = false;
}

bool la_set_trigger_mode_edge(bool edge_mode)
{
    state.trigger_mode = edge_mode ? LOGIC_ANALYSER_TRIGGER_EDGE
                                   : LOGIC_ANALYSER_TRIGGER_LEVEL;
    capture_valid = false;
    return true;
}

uint32_t la_get_pin_base(void) { return state.pin_base; }

uint32_t la_get_pin_count(void) { return state.pin_count; }

uint32_t la_get_samples(void) { return state.samples; }

uint32_t la_get_divider(void) { return state.divider; }

uint32_t la_get_trigger_pin(void) { return state.trigger_pin; }

bool la_get_trigger_level(void) { return state.trigger_level; }

bool la_get_trigger_mode_edge(void)
{
    return state.trigger_mode == LOGIC_ANALYSER_TRIGGER_EDGE;
}

bool la_initiate(void)
{
    if (!la_initialized && !apply_config()) {
        return false;
    }

    uint32_t word_count = logic_analyser_capture_word_count(
        state.pin_count, state.samples
    );
    if (word_count == 0 ||
        word_count > sizeof(capture_buffer) / sizeof(capture_buffer[0])) {
        return false;
    }

    memset(capture_buffer, 0, word_count * sizeof(capture_buffer[0]));

    status_led_capture_started();
    bool captured = logic_analyser_capture(
            &la,
            state.trigger_pin,
            state.trigger_level,
            state.trigger_mode,
            capture_buffer,
            state.samples,
            &last_capture
        );
    status_led_capture_finished();

    if (!captured) {
        return false;
    }

    capture_valid = true;
    return true;
}

bool la_fetch(uint8_t const **data, size_t *len)
{
    if (!capture_valid || !data || !len) {
        return false;
    }

    *data = (uint8_t const *)capture_buffer;
    *len = last_capture.word_count * sizeof(uint32_t);
    return true;
}

uint32_t la_status(void)
{
    if (la_initialized && logic_analyser_is_busy(&la)) {
        return 2;
    }

    return capture_valid ? 1 : 0;
}
