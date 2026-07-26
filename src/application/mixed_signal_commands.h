#ifndef MIXED_SIGNAL_COMMANDS_H
#define MIXED_SIGNAL_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "system/instrument/mixed_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

void mso_commands_reset(void);

bool mso_commands_set_digital_pin_base(uint32_t value);
bool mso_commands_set_digital_pin_count(uint32_t value);
bool mso_commands_set_analog_channel(uint32_t value);
bool mso_commands_set_sample_rate(uint32_t value);
bool mso_commands_set_samples(uint32_t value);
bool mso_commands_set_trigger_pin(uint32_t value);
void mso_commands_set_trigger_level(bool value);
bool mso_commands_set_trigger_mode_edge(bool edge_mode);

uint32_t mso_commands_get_digital_pin_base(void);
uint32_t mso_commands_get_digital_pin_count(void);
uint32_t mso_commands_get_analog_channel(void);
uint32_t mso_commands_get_sample_rate(void);
uint32_t mso_commands_get_samples(void);
uint32_t mso_commands_get_trigger_pin(void);
bool mso_commands_get_trigger_level(void);
bool mso_commands_get_trigger_mode_edge(void);
MixedSignalCaptureInfo const *mso_commands_get_last_info(void);

bool mso_commands_initiate(void);
bool mso_commands_fetch_digital(uint8_t const **data, size_t *len);
bool mso_commands_fetch_analog(uint8_t const **data, size_t *len);
uint32_t mso_commands_status(void);

#ifdef __cplusplus
}
#endif

#endif
