#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void status_led_init(void);
void status_led_set(bool on);
void status_led_blink(unsigned int count);
void status_led_command_received(void);
void status_led_capture_started(void);
void status_led_capture_finished(void);

#ifdef __cplusplus
}
#endif

#endif
