#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool protocol_init(void);
void protocol_task(void);
void protocol_deinit(void);
bool protocol_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif
