#include "application/pattern_generator_commands.h"

#include <string.h>

#include "system/pattern_generator.h"

enum {
    PG_DEFAULT_PIN_BASE = 16,
    PG_DEFAULT_PIN_COUNT = 1,
    PG_DEFAULT_RATE_HZ = 1000,
    PG_MAX_PIN_COUNT = 8,
    PG_MAX_PATTERN_WORDS = 16384,
    PG_MAX_RATE_HZ = 75000000,
};

static PatternGenerator pg;
static bool pg_initialized;
static uint32_t pattern_buffer[PG_MAX_PATTERN_WORDS];

static struct {
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t rate_hz;
    uint32_t pattern_words;
    PatternGeneratorMode mode;
} state = {
    .pin_base = PG_DEFAULT_PIN_BASE,
    .pin_count = PG_DEFAULT_PIN_COUNT,
    .rate_hz = PG_DEFAULT_RATE_HZ,
    .pattern_words = 0,
    .mode = PATTERN_GENERATOR_MODE_ONCE,
};

static bool config_is_valid(void)
{
    return state.pin_count >= 1 && state.pin_count <= PG_MAX_PIN_COUNT &&
           state.pin_base + state.pin_count <= 30 && state.rate_hz >= 1 &&
           state.rate_hz <= PG_MAX_RATE_HZ;
}

static bool apply_config(void)
{
    if (!config_is_valid()) {
        return false;
    }

    PatternGeneratorConfig config = {
        .pin_base = state.pin_base,
        .pin_count = state.pin_count,
        .rate_hz = state.rate_hz,
    };

    if (pg_initialized) {
        return pattern_generator_configure(&pg, &config);
    }

    pg_initialized = pattern_generator_init(&pg, &config);
    return pg_initialized;
}

void pg_reset_state(void)
{
    pg_stop();
    if (pg_initialized) {
        pattern_generator_deinit(&pg);
    }

    pg_initialized = false;
    state.pin_base = PG_DEFAULT_PIN_BASE;
    state.pin_count = PG_DEFAULT_PIN_COUNT;
    state.rate_hz = PG_DEFAULT_RATE_HZ;
    state.pattern_words = 0;
    state.mode = PATTERN_GENERATOR_MODE_ONCE;
    memset(pattern_buffer, 0, sizeof(pattern_buffer));
}

void pg_task(void)
{
    if (pg_initialized) {
        pattern_generator_task(&pg);
    }
}

bool pg_set_pins(uint32_t pin_base, uint32_t pin_count)
{
    if (pin_count < 1 || pin_count > PG_MAX_PIN_COUNT ||
        pin_base + pin_count > 30 || pg_is_running()) {
        return false;
    }

    uint32_t old_pin_base = state.pin_base;
    uint32_t old_pin_count = state.pin_count;
    state.pin_base = pin_base;
    state.pin_count = pin_count;

    if (pg_initialized && !apply_config()) {
        state.pin_base = old_pin_base;
        state.pin_count = old_pin_count;
        return false;
    }

    return true;
}

bool pg_set_rate(uint32_t rate_hz)
{
    if (rate_hz < 1 || rate_hz > PG_MAX_RATE_HZ || pg_is_running()) {
        return false;
    }

    uint32_t old_rate_hz = state.rate_hz;
    state.rate_hz = rate_hz;

    if (pg_initialized && !apply_config()) {
        state.rate_hz = old_rate_hz;
        return false;
    }

    return true;
}

bool pg_set_mode_once(void)
{
    if (pg_is_running()) {
        return false;
    }

    state.mode = PATTERN_GENERATOR_MODE_ONCE;
    return true;
}

bool pg_set_mode_loop(void)
{
    if (pg_is_running()) {
        return false;
    }

    state.mode = PATTERN_GENERATOR_MODE_LOOP;
    return true;
}

bool pg_upload_data(uint8_t const *data, size_t len)
{
    if (!data || len == 0 || (len % sizeof(uint32_t)) != 0 || pg_is_running()) {
        return false;
    }

    size_t word_count = len / sizeof(uint32_t);
    if (word_count > PG_MAX_PATTERN_WORDS) {
        return false;
    }

    memcpy(pattern_buffer, data, len);
    state.pattern_words = (uint32_t)word_count;
    return true;
}

bool pg_start(void)
{
    if (state.pattern_words == 0) {
        return false;
    }

    if (!pg_initialized && !apply_config()) {
        return false;
    }

    return pattern_generator_start(
        &pg,
        pattern_buffer,
        state.pattern_words,
        state.mode
    );
}

void pg_stop(void)
{
    if (pg_initialized) {
        pattern_generator_stop(&pg);
    }
}

uint32_t pg_get_pin_base(void) { return state.pin_base; }

uint32_t pg_get_pin_count(void) { return state.pin_count; }

uint32_t pg_get_rate(void) { return state.rate_hz; }

bool pg_get_mode_loop(void) { return state.mode == PATTERN_GENERATOR_MODE_LOOP; }

uint32_t pg_get_pattern_words(void) { return state.pattern_words; }

bool pg_is_running(void)
{
    return pg_initialized && pattern_generator_is_running(&pg);
}
