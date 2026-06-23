#include "application/dso_commands.h"

#include <stdbool.h>

#include "system/instrument/dso.h"

void dso_commands_reset(void) { dso_reset_state(); }

bool dso_commands_set_channel(uint32_t value) { return dso_set_channel(value); }

bool dso_commands_set_sample_rate(uint32_t value)
{
    return dso_set_sample_rate(value);
}

bool dso_commands_set_samples(uint32_t value) { return dso_set_samples(value); }

bool dso_commands_set_trigger_level(uint32_t value)
{
    return dso_set_trigger_level(value);
}

bool dso_commands_set_trigger_mode_off(void)
{
    return dso_set_trigger_mode(DSO_TRIGGER_OFF);
}

bool dso_commands_set_trigger_mode_level(void)
{
    return dso_set_trigger_mode(DSO_TRIGGER_LEVEL);
}

bool dso_commands_set_trigger_mode_edge(void)
{
    return dso_set_trigger_mode(DSO_TRIGGER_EDGE);
}

bool dso_commands_set_trigger_slope_rising(void)
{
    return dso_set_trigger_slope(DSO_TRIGGER_RISING);
}

bool dso_commands_set_trigger_slope_falling(void)
{
    return dso_set_trigger_slope(DSO_TRIGGER_FALLING);
}

uint32_t dso_commands_get_channel(void) { return dso_get_channel(); }

uint32_t dso_commands_get_gpio(void) { return dso_get_gpio(); }

uint32_t dso_commands_get_sample_rate(void) { return dso_get_sample_rate(); }

uint32_t dso_commands_get_samples(void) { return dso_get_samples(); }

uint32_t dso_commands_get_trigger_level(void) { return dso_get_trigger_level(); }

char const *dso_commands_get_trigger_mode(void)
{
    switch (dso_get_trigger_mode()) {
    case DSO_TRIGGER_LEVEL:
        return "LEVEL";
    case DSO_TRIGGER_EDGE:
        return "EDGE";
    case DSO_TRIGGER_OFF:
    default:
        return "OFF";
    }
}

char const *dso_commands_get_trigger_slope(void)
{
    return dso_get_trigger_slope() == DSO_TRIGGER_FALLING ? "FALL" : "RISE";
}

bool dso_commands_initiate(void) { return dso_initiate(); }

bool dso_commands_fetch(uint8_t const **data, size_t *len)
{
    return dso_fetch(data, len);
}

uint32_t dso_commands_status(void) { return dso_status(); }

bool dso_commands_stream_start(void) { return dso_stream_start(); }

void dso_commands_stream_stop(void) { dso_stream_stop(); }

bool dso_commands_stream_is_enabled(void) { return dso_stream_is_enabled(); }

uint32_t dso_commands_stream_get_sequence(void)
{
    return dso_stream_get_sequence();
}

uint32_t dso_commands_stream_get_overruns(void)
{
    return dso_stream_get_overruns();
}

bool dso_commands_stream_next_frame(
    uint8_t const **data,
    size_t *len,
    uint32_t *sequence
)
{
    return dso_stream_next_frame(data, len, sequence);
}
