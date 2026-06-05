#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SYSTEM_init(void);
uint32_t SYSTEM_get_tick(void);
void SYSTEM_reset(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif
