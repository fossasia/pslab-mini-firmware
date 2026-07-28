#ifndef PATTERN_GENERATOR_COMMANDS_H
#define PATTERN_GENERATOR_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pg_reset_state(void);
void pg_task(void);

bool pg_set_pins(uint32_t pin_base, uint32_t pin_count);
bool pg_set_rate(uint32_t rate_hz);
bool pg_set_mode_once(void);
bool pg_set_mode_loop(void);
bool pg_upload_data(uint8_t const *data, size_t len);
bool pg_start(void);
void pg_stop(void);

uint32_t pg_get_pin_base(void);
uint32_t pg_get_pin_count(void);
uint32_t pg_get_rate(void);
bool pg_get_mode_loop(void);
uint32_t pg_get_pattern_words(void);
bool pg_is_running(void);

#ifdef __cplusplus
}
#endif

#endif
