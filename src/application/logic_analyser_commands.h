#ifndef LOGIC_ANALYSER_COMMANDS_H
#define LOGIC_ANALYSER_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void la_reset_state(void);

bool la_set_pin_base(uint32_t value);
bool la_set_pin_count(uint32_t value);
bool la_set_samples(uint32_t value);
bool la_set_divider(uint32_t value);
bool la_set_trigger_pin(uint32_t value);
void la_set_trigger_level(bool value);
bool la_set_trigger_mode_edge(bool edge_mode);

uint32_t la_get_pin_base(void);
uint32_t la_get_pin_count(void);
uint32_t la_get_samples(void);
uint32_t la_get_divider(void);
uint32_t la_get_trigger_pin(void);
bool la_get_trigger_level(void);
bool la_get_trigger_mode_edge(void);

bool la_initiate(void);
bool la_fetch(uint8_t const **data, size_t *len);
uint32_t la_status(void);

bool la_stream_start(void);
void la_stream_stop(void);
bool la_stream_is_enabled(void);
uint32_t la_stream_get_sequence(void);
uint32_t la_stream_get_overruns(void);
bool la_stream_next_frame(uint8_t const **data, size_t *len, uint32_t *sequence);

#ifdef __cplusplus
}
#endif

#endif
