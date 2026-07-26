#include "application/mixed_signal_commands.h"

void mso_commands_reset(void) { mixed_signal_reset_state(); }

bool mso_commands_set_digital_pin_base(uint32_t value)
{
    return mixed_signal_set_digital_pin_base(value);
}

bool mso_commands_set_digital_pin_count(uint32_t value)
{
    return mixed_signal_set_digital_pin_count(value);
}

bool mso_commands_set_analog_channel(uint32_t value)
{
    return mixed_signal_set_analog_channel(value);
}

bool mso_commands_set_sample_rate(uint32_t value)
{
    return mixed_signal_set_sample_rate(value);
}

bool mso_commands_set_samples(uint32_t value)
{
    return mixed_signal_set_samples(value);
}

bool mso_commands_set_trigger_pin(uint32_t value)
{
    return mixed_signal_set_trigger_pin(value);
}

void mso_commands_set_trigger_level(bool value)
{
    mixed_signal_set_trigger_level(value);
}

bool mso_commands_set_trigger_mode_edge(bool edge_mode)
{
    return mixed_signal_set_trigger_mode_edge(edge_mode);
}

uint32_t mso_commands_get_digital_pin_base(void)
{
    return mixed_signal_get_digital_pin_base();
}

uint32_t mso_commands_get_digital_pin_count(void)
{
    return mixed_signal_get_digital_pin_count();
}

uint32_t mso_commands_get_analog_channel(void)
{
    return mixed_signal_get_analog_channel();
}

uint32_t mso_commands_get_sample_rate(void)
{
    return mixed_signal_get_sample_rate();
}

uint32_t mso_commands_get_samples(void) { return mixed_signal_get_samples(); }

uint32_t mso_commands_get_trigger_pin(void)
{
    return mixed_signal_get_trigger_pin();
}

bool mso_commands_get_trigger_level(void)
{
    return mixed_signal_get_trigger_level();
}

bool mso_commands_get_trigger_mode_edge(void)
{
    return mixed_signal_get_trigger_mode_edge();
}

MixedSignalCaptureInfo const *mso_commands_get_last_info(void)
{
    return mixed_signal_get_last_info();
}

bool mso_commands_initiate(void) { return mixed_signal_initiate(); }

bool mso_commands_fetch_digital(uint8_t const **data, size_t *len)
{
    return mixed_signal_fetch_digital(data, len);
}

bool mso_commands_fetch_analog(uint8_t const **data, size_t *len)
{
    return mixed_signal_fetch_analog(data, len);
}

uint32_t mso_commands_status(void) { return mixed_signal_status(); }
