#include "system/instrument/mixed_signal.h"

#include <stddef.h>
#include <string.h>

#include "platform/adc_capture.h"
#include "platform/platform.h"
#include "platform/status_led.h"

enum {
    MIXED_SIGNAL_DEFAULT_DIGITAL_PIN_BASE = 16,
    MIXED_SIGNAL_DEFAULT_DIGITAL_PIN_COUNT = 1,
    MIXED_SIGNAL_DEFAULT_ANALOG_CHANNEL = 0,
    MIXED_SIGNAL_DEFAULT_SAMPLE_RATE_HZ = 500000,
    MIXED_SIGNAL_DEFAULT_SAMPLES = 1024,
    MIXED_SIGNAL_DEFAULT_TRIGGER_PIN = 16,
    MIXED_SIGNAL_MAX_DIGITAL_PIN_COUNT = 8,
    MIXED_SIGNAL_MAX_GPIO_PIN = 29,
    MIXED_SIGNAL_TRIGGER_TIMEOUT_US = 1000000,
};

static LogicAnalyser logic_analyser;
static bool logic_analyser_initialized;
static bool adc_initialized;
static bool capture_valid;

static uint32_t digital_buffer[MIXED_SIGNAL_MAX_SAMPLES];
static uint16_t analog_buffer[MIXED_SIGNAL_MAX_SAMPLES];
static MixedSignalCaptureInfo last_capture;

static struct {
    uint32_t digital_pin_base;
    uint32_t digital_pin_count;
    uint32_t analog_channel;
    uint32_t sample_rate_hz;
    uint32_t samples;
    uint32_t trigger_pin;
    bool trigger_level;
    LogicAnalyserTriggerMode trigger_mode;
} state = {
    .digital_pin_base = MIXED_SIGNAL_DEFAULT_DIGITAL_PIN_BASE,
    .digital_pin_count = MIXED_SIGNAL_DEFAULT_DIGITAL_PIN_COUNT,
    .analog_channel = MIXED_SIGNAL_DEFAULT_ANALOG_CHANNEL,
    .sample_rate_hz = MIXED_SIGNAL_DEFAULT_SAMPLE_RATE_HZ,
    .samples = MIXED_SIGNAL_DEFAULT_SAMPLES,
    .trigger_pin = MIXED_SIGNAL_DEFAULT_TRIGGER_PIN,
    .trigger_level = true,
    .trigger_mode = LOGIC_ANALYSER_TRIGGER_EDGE,
};

static bool digital_pins_are_valid(void)
{
    return state.digital_pin_count >= 1 &&
           state.digital_pin_count <= MIXED_SIGNAL_MAX_DIGITAL_PIN_COUNT &&
           state.digital_pin_base <= MIXED_SIGNAL_MAX_GPIO_PIN &&
           state.digital_pin_base + state.digital_pin_count - 1 <=
               MIXED_SIGNAL_MAX_GPIO_PIN;
}

static bool config_is_valid(void)
{
    return digital_pins_are_valid() &&
           state.analog_channel <= ADC_CAPTURE_MAX_CHANNEL &&
           state.sample_rate_hz >= ADC_CAPTURE_MIN_SAMPLE_RATE_HZ &&
           state.sample_rate_hz <= ADC_CAPTURE_MAX_SAMPLE_RATE_HZ &&
           state.samples >= 1 && state.samples <= MIXED_SIGNAL_MAX_SAMPLES &&
           state.trigger_pin <= MIXED_SIGNAL_MAX_GPIO_PIN;
}

static float logic_analyser_clock_divider(void)
{
    uint32_t sys_clock =
        PLATFORM_get_peripheral_clock_speed(PLATFORM_CLOCK_SYS);
    if (sys_clock == 0 || state.sample_rate_hz == 0) {
        return 0.0f;
    }

    return (float)sys_clock / (float)state.sample_rate_hz;
}

static bool apply_config(void)
{
    if (!config_is_valid()) {
        return false;
    }

    LogicAnalyserConfig logic_config = {
        .pin_base = state.digital_pin_base,
        .pin_count = state.digital_pin_count,
        .clk_div = logic_analyser_clock_divider(),
    };

    AdcCaptureConfig adc_config = {
        .channel = state.analog_channel,
        .sample_rate_hz = state.sample_rate_hz,
    };

    bool logic_ready = logic_analyser_initialized
                           ? logic_analyser_configure(
                                 &logic_analyser,
                                 &logic_config
                             )
                           : logic_analyser_init(
                                 &logic_analyser,
                                 &logic_config
                             );
    if (!logic_ready) {
        return false;
    }
    logic_analyser_initialized = true;

    bool adc_ready = adc_initialized ? adc_capture_configure(&adc_config)
                                     : adc_capture_init(&adc_config);
    if (!adc_ready) {
        return false;
    }
    adc_initialized = true;

    return true;
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
        (void)apply_config();
        return false;
    }

    return true;
}

void mixed_signal_reset_state(void)
{
    if (logic_analyser_initialized) {
        logic_analyser_deinit(&logic_analyser);
    }
    if (adc_initialized) {
        adc_capture_deinit();
    }

    logic_analyser_initialized = false;
    adc_initialized = false;
    capture_valid = false;
    state.digital_pin_base = MIXED_SIGNAL_DEFAULT_DIGITAL_PIN_BASE;
    state.digital_pin_count = MIXED_SIGNAL_DEFAULT_DIGITAL_PIN_COUNT;
    state.analog_channel = MIXED_SIGNAL_DEFAULT_ANALOG_CHANNEL;
    state.sample_rate_hz = MIXED_SIGNAL_DEFAULT_SAMPLE_RATE_HZ;
    state.samples = MIXED_SIGNAL_DEFAULT_SAMPLES;
    state.trigger_pin = MIXED_SIGNAL_DEFAULT_TRIGGER_PIN;
    state.trigger_level = true;
    state.trigger_mode = LOGIC_ANALYSER_TRIGGER_EDGE;
}

bool mixed_signal_set_digital_pin_base(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.digital_pin_base,
        value,
        0,
        MIXED_SIGNAL_MAX_GPIO_PIN,
        true
    );
}

bool mixed_signal_set_digital_pin_count(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.digital_pin_count,
        value,
        1,
        MIXED_SIGNAL_MAX_DIGITAL_PIN_COUNT,
        true
    );
}

bool mixed_signal_set_analog_channel(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.analog_channel,
        value,
        0,
        ADC_CAPTURE_MAX_CHANNEL,
        true
    );
}

bool mixed_signal_set_sample_rate(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.sample_rate_hz,
        value,
        ADC_CAPTURE_MIN_SAMPLE_RATE_HZ,
        ADC_CAPTURE_MAX_SAMPLE_RATE_HZ,
        true
    );
}

bool mixed_signal_set_samples(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.samples,
        value,
        1,
        MIXED_SIGNAL_MAX_SAMPLES,
        false
    );
}

bool mixed_signal_set_trigger_pin(uint32_t value)
{
    return set_and_maybe_reconfigure(
        &state.trigger_pin,
        value,
        0,
        MIXED_SIGNAL_MAX_GPIO_PIN,
        false
    );
}

void mixed_signal_set_trigger_level(bool value)
{
    state.trigger_level = value;
    capture_valid = false;
}

bool mixed_signal_set_trigger_mode_edge(bool edge_mode)
{
    state.trigger_mode = edge_mode ? LOGIC_ANALYSER_TRIGGER_EDGE
                                   : LOGIC_ANALYSER_TRIGGER_LEVEL;
    capture_valid = false;
    return true;
}

uint32_t mixed_signal_get_digital_pin_base(void)
{
    return state.digital_pin_base;
}

uint32_t mixed_signal_get_digital_pin_count(void)
{
    return state.digital_pin_count;
}

uint32_t mixed_signal_get_analog_channel(void) { return state.analog_channel; }

uint32_t mixed_signal_get_sample_rate(void) { return state.sample_rate_hz; }

uint32_t mixed_signal_get_samples(void) { return state.samples; }

uint32_t mixed_signal_get_trigger_pin(void) { return state.trigger_pin; }

bool mixed_signal_get_trigger_level(void) { return state.trigger_level; }

bool mixed_signal_get_trigger_mode_edge(void)
{
    return state.trigger_mode == LOGIC_ANALYSER_TRIGGER_EDGE;
}

MixedSignalCaptureInfo const *mixed_signal_get_last_info(void)
{
    return capture_valid ? &last_capture : NULL;
}

bool mixed_signal_initiate(void)
{
    if ((!logic_analyser_initialized || !adc_initialized) && !apply_config()) {
        return false;
    }

    uint32_t digital_word_count = logic_analyser_capture_word_count(
        state.digital_pin_count,
        state.samples
    );
    uint32_t digital_bits_per_word =
        logic_analyser_bits_packed_per_word(state.digital_pin_count);
    if (digital_word_count == 0 || digital_word_count > MIXED_SIGNAL_MAX_SAMPLES ||
        digital_bits_per_word == 0) {
        return false;
    }

    memset(digital_buffer, 0, digital_word_count * sizeof(digital_buffer[0]));
    memset(analog_buffer, 0, state.samples * sizeof(analog_buffer[0]));

    AdcCaptureInfo adc_info;
    LogicAnalyserCaptureInfo logic_info;
    if (!adc_capture_arm(analog_buffer, state.samples, &adc_info)) {
        return false;
    }

    bool logic_armed = logic_analyser_capture_arm(
        &logic_analyser,
        state.trigger_pin,
        state.trigger_level,
        state.trigger_mode,
        digital_buffer,
        state.samples,
        &logic_info,
        false
    );
    if (!logic_armed) {
        adc_capture_abort();
        return false;
    }

    status_led_capture_started();
    bool triggered = logic_analyser_wait_for_trigger_timeout(
        state.trigger_pin,
        state.trigger_level,
        state.trigger_mode,
        MIXED_SIGNAL_TRIGGER_TIMEOUT_US
    );
    if (!triggered) {
        adc_capture_abort();
        logic_analyser_capture_abort(&logic_analyser);
        status_led_capture_finished();
        capture_valid = false;
        return false;
    }

    bool adc_started = adc_capture_start();
    bool logic_started = logic_analyser_capture_start_armed(&logic_analyser);
    if (!adc_started || !logic_started) {
        adc_capture_abort();
        logic_analyser_capture_abort(&logic_analyser);
        status_led_capture_finished();
        return false;
    }

    logic_analyser_capture_wait(&logic_analyser);
    bool adc_completed = adc_capture_wait();
    bool logic_completed = logic_analyser_capture_complete(&logic_analyser);
    status_led_capture_finished();

    if (!adc_completed || !logic_completed) {
        capture_valid = false;
        return false;
    }

    last_capture = (MixedSignalCaptureInfo){
        .sample_rate_hz = state.sample_rate_hz,
        .sample_count = state.samples,
        .digital_pin_base = logic_info.pin_base,
        .digital_pin_count = logic_info.pin_count,
        .digital_word_count = logic_info.word_count,
        .digital_bits_per_word = logic_info.bits_per_word,
        .analog_channel = adc_info.channel,
        .analog_gpio = adc_info.gpio,
        .trigger_pin = state.trigger_pin,
        .trigger_level = state.trigger_level,
        .trigger_mode = state.trigger_mode,
        .digital_start_offset_ns = 0,
        .analog_start_offset_ns = 0,
    };
    capture_valid = true;
    return true;
}

bool mixed_signal_fetch_digital(uint8_t const **data, size_t *len)
{
    if (!capture_valid || !data || !len) {
        return false;
    }

    *data = (uint8_t const *)digital_buffer;
    *len = last_capture.digital_word_count * sizeof(digital_buffer[0]);
    return true;
}

bool mixed_signal_fetch_analog(uint8_t const **data, size_t *len)
{
    if (!capture_valid || !data || !len) {
        return false;
    }

    *data = (uint8_t const *)analog_buffer;
    *len = last_capture.sample_count * sizeof(analog_buffer[0]);
    return true;
}

MixedSignalStatus mixed_signal_status(void)
{
    if (logic_analyser_is_busy(&logic_analyser) || adc_capture_is_busy()) {
        return MIXED_SIGNAL_STATUS_BUSY;
    }

    return capture_valid ? MIXED_SIGNAL_STATUS_READY : MIXED_SIGNAL_STATUS_IDLE;
}
