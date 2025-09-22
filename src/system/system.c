/**
 * @file system.c
 * @brief Hardware system initialization routines for the PSLab Mini firmware.
 *
 * This module provides the SYSTEM_init() function, which initializes all core
 * hardware peripherals and must be called immediately after reset, before any
 * other hardware access.
 */

#include "platform/platform.h"
#include "util/error.h"
#include "util/logging.h"

#include "led.h"
#include "system.h"

void SYSTEM_init(void)
{
    PLATFORM_init();
    // Read any logs that were generated during platform initialization
    unsigned const max_log_entries = 32;
    LOG_task(max_log_entries);
    LED_init();
}

__attribute__((noreturn)) void SYSTEM_reset(void)
{
    LOG_INFO("Resetting...");
    PLATFORM_reset();
    __builtin_unreachable();
}

/**
 * @brief Handler for uncaught CException throws
 *
 * This function is called when a THROW occurs outside of any TRY context.
 * It provides a last-resort error handler that logs the exception and
 * resets the system to prevent undefined behavior.
 *
 * @param exception_id The exception ID that was thrown but not caught
 *
 * @note This function does not return; it resets the system
 */
__attribute__((noreturn)) void EXCEPTION_halt(uint32_t exception_id)
{
    LOG_ERROR(
        "FATAL: Uncaught exception 0x%08X - system will reset",
        (unsigned int)exception_id
    );
    LOG_task(0xFF); // Ensure log message is output

    // Force a small delay to allow log message to be transmitted
    // This is a simple busy wait since we're about to reset anyway
    // TODO(bessman): Replace with proper delay function
    // This is about 2 ms on STM32H563xx
    uint32_t volatile delay = 1000000; // NOLINT[readability-magic-numbers]
    while (delay--);

    // Reset the system - this function does not return
    PLATFORM_reset();
}
