/**
 * @file system.c
 * @brief Hardware system initialization routines for the PSLab Mini firmware.
 *
 * This module provides the SYSTEM_init() function, which initializes all core
 * hardware peripherals and must be called immediately after reset, before any
 * other hardware access.
 */

#include "platform/platform.h"
#include "platform/uart_ll.h"
#include "util/error.h"
#include "util/logging.h"
#include "util/si_prefix.h"

#include "led.h"
#include "syscalls_config.h"
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

    // Flush any pending log messages
    LOG_task(UINT32_MAX);
    uint32_t timeout =
        ((SYSCALLS_UART_TX_BUFFER_SIZE * SI_MILLI_DIV * 2) /
         UART_DEFAULT_BAUDRATE);

    // 1 ms <= timeout <= 1000 ms
    timeout = timeout < 1 ? 1 : timeout;
    timeout = timeout > SI_MILLI_DIV ? SI_MILLI_DIV : timeout;

    extern bool syscalls_uart_flush(uint32_t timeout);
    syscalls_uart_flush(timeout);
    PLATFORM_reset();
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
    SYSTEM_reset();
}
