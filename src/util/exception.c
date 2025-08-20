/**
 * @file exception.c
 * @brief CException configuration implementation
 *
 * This file implements the configurable exception handler mechanism
 * for the CException library integration.
 */

#include "exception.h"
#include "logging.h"
#include <stdint.h>

__attribute__((weak, noreturn)) void EXCEPTION_halt(uint32_t exception_id)
{
    (void)exception_id;
    // Default implementation does nothing and hangs indefinitely
    while (1) {
    }
}
