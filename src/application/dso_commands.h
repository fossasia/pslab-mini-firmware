#ifndef DSO_COMMANDS_H
#define DSO_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void dso_commands_reset(void);

bool dso_commands_set_channel(uint32_t value);
bool dso_commands_set_sample_rate(uint32_t value);
bool dso_commands_set_samples(uint32_t value);
bool dso_commands_set_trigger_level(uint32_t value);
bool dso_commands_set_trigger_mode_off(void);
bool dso_commands_set_trigger_mode_level(void);
bool dso_commands_set_trigger_mode_edge(void);
bool dso_commands_set_trigger_slope_rising(void);
bool dso_commands_set_trigger_slope_falling(void);

uint32_t dso_commands_get_channel(void);
uint32_t dso_commands_get_gpio(void);
uint32_t dso_commands_get_sample_rate(void);
uint32_t dso_commands_get_samples(void);
uint32_t dso_commands_get_trigger_level(void);
char const *dso_commands_get_trigger_mode(void);
char const *dso_commands_get_trigger_slope(void);

bool dso_commands_initiate(void);
bool dso_commands_fetch(uint8_t const **data, size_t *len);
uint32_t dso_commands_status(void);

bool dso_commands_stream_start(void);
void dso_commands_stream_stop(void);
bool dso_commands_stream_is_enabled(void);
uint32_t dso_commands_stream_get_sequence(void);
uint32_t dso_commands_stream_get_overruns(void);
bool dso_commands_stream_next_frame(
    uint8_t const **data,
    size_t *len,
    uint32_t *sequence
);

#ifdef __cplusplus
}
#endif

#endif
