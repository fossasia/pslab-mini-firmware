/**
 * @file system.c
 * @brief Hardware system initialization routines for the PSLab Mini firmware.
 *
 * This module provides the SYSTEM_init() function, which initializes all core
 * hardware peripherals and must be called immediately after reset, before any
 * other hardware access.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/platform.h"
#include "platform/uart_ll.h"
#include "util/error.h"
#include "util/logging.h"
#include "util/si_prefix.h"
#include "util/util.h"

#include "bus/uart.h"
#include "led.h"
#include "system.h"

// Global variables for logging
static UART_Handle *g_logging_uart_handle = nullptr;
static uint8_t g_log_buf[1024];
static uint8_t g_log_rx_buf[1];
static CircularBuffer g_log_cb;
static CircularBuffer g_log_rx_cb;

void SYSTEM_init(void)
{
    // Initialize logging early to capture any log messages during startup
    LOG_init();
    PLATFORM_init();

    // Set up log output
    circular_buffer_init(&g_log_cb, g_log_buf, sizeof(g_log_buf));
    circular_buffer_init(&g_log_rx_cb, g_log_rx_buf, sizeof(g_log_rx_buf));
    // uint8_t log_bus = 2;
    // g_logging_uart_handle = UART_init(log_bus, &g_log_rx_cb, &g_log_cb);
    extern void syscalls_init(UART_Handle * handle);
    syscalls_init(g_logging_uart_handle);
    // Buffered log messages can now be output with LOG_task
    LOG_task(0xFF);

    LED_init();
}

uint32_t SYSTEM_get_tick(void) { return PLATFORM_get_tick(); }

__attribute__((noreturn)) void SYSTEM_reset(void)
{
    LOG_INFO("Resetting...");

    // Flush any pending log messages
    LOG_task(UINT32_MAX);
    unsigned const bits_per_uart_byte = 10;
    size_t const buffer_size_bits = sizeof(g_log_buf) * bits_per_uart_byte;
    // Timeout to wait for TX buffer to empty (2x time for safety)
    uint32_t timeout =
        ((buffer_size_bits * SI_MILLI_DIV * 2) / UART_DEFAULT_BAUDRATE);

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
