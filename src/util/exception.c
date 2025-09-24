/**
 * @file exception.c
 * @brief CException configuration implementation
 *
 * This file implements the configurable exception handler mechanism
 * for the CException library integration.
 */

#include <stdint.h>

#include "exception.h"
#include "logging.h"

__attribute__((weak, noreturn)) void EXCEPTION_halt(uint32_t exception_id)
{
    LOG_ERROR("Default EXCEPTION_halt called");
    LOG_ERROR(
        "FATAL: Uncaught exception 0x%08X - system will reset",
        (unsigned int)exception_id
    );
    LOG_task(UINT32_MAX); // Ensure log message is output

    // Default implementation does nothing and hangs indefinitely
    while (1) {
    }
}
