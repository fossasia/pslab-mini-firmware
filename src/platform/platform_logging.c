/**
 * @file platform_logging.c
 * @brief Platform-layer logging implementation (buffer-based, no stdio)
 *
 * This module implements logging for the platform layer using the common
 * circular buffer implementation. Messages are written to the buffer with a
 * simple printf-like interface. The system layer polls for new messages and
 * forwards them to the main logging system.
 *
 * @author PSLab Team
 * @date 2025-07-09
 */

#include "platform_logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Global log buffer and state variables
 * These are accessed by the system layer as externs
 */
bool volatile g_platform_log_service_request = false;
CircularBuffer g_platform_log_buffer;
uint8_t g_platform_log_buffer_data[PLATFORM_LOG_BUFFER_SIZE];

/**
 * @brief Internal state
 */
static bool g_platform_log_initialized = false;

/**
 * @brief Initialize platform logging
 */
void PLATFORM_log_init(void)
{
    circular_buffer_init(
        &g_platform_log_buffer,
        g_platform_log_buffer_data,
        PLATFORM_LOG_BUFFER_SIZE
    );
    g_platform_log_service_request = false;
    g_platform_log_initialized = true;
}

/**
 * @brief Write a log message to the platform buffer
 */
void PLATFORM_log_write(PLATFORM_LogLevel level, char const *format, ...)
{
    if (!g_platform_log_initialized) {
        return;
    }

    /* Prepare log entry */
    PLATFORM_LogEntry entry;
    entry.level = level;

    /* Format the message */
    va_list args;
    va_start(args, format);
    int formatted_length =
        vsnprintf(entry.message, sizeof(entry.message), format, args);
    va_end(args);

    if (formatted_length < 0) {
        return; /* Formatting error */
    }

    entry.length = (uint16_t)(formatted_length < sizeof(entry.message)
                                  ? formatted_length
                                  : sizeof(entry.message) - 1);
    entry.message[entry.length] = '\0'; /* Ensure null termination */

    /* Calculate total entry size */
    uint32_t entry_size =
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1;

    /* Check if we have enough space */
    if (circular_buffer_free_space(&g_platform_log_buffer) >= entry_size) {
        /* Write entry to buffer */
        circular_buffer_write(
            &g_platform_log_buffer, (uint8_t *)&entry.level, sizeof(entry.level)
        );
        circular_buffer_write(
            &g_platform_log_buffer,
            (uint8_t *)&entry.length,
            sizeof(entry.length)
        );
        circular_buffer_write(
            &g_platform_log_buffer, (uint8_t *)entry.message, entry.length + 1
        );

        /* Signal system layer that service is needed */
        g_platform_log_service_request = true;
    }
    /* If write fails, message is dropped (acceptable for logging) */
}

/**
 * @brief Get the number of bytes available in the log buffer
 */
size_t PLATFORM_log_available(void)
{
    return circular_buffer_available(&g_platform_log_buffer);
}

/**
 * @brief Read a log entry from the buffer
 */
bool platform_log_read_entry(PLATFORM_LogEntry *entry)
{
    if (!entry || circular_buffer_available(&g_platform_log_buffer) <
                      sizeof(entry->level) + sizeof(entry->length)) {
        return false;
    }

    /* Read level and length first */
    if (circular_buffer_read(
            &g_platform_log_buffer,
            (uint8_t *)&entry->level,
            sizeof(entry->level)
        ) != sizeof(entry->level) ||
        circular_buffer_read(
            &g_platform_log_buffer,
            (uint8_t *)&entry->length,
            sizeof(entry->length)
        ) != sizeof(entry->length)) {
        return false;
    }

    /* Validate length */
    if (entry->length >= sizeof(entry->message)) {
        entry->length = sizeof(entry->message) - 1;
    }

    /* Check if we have enough data for the message */
    if (circular_buffer_available(&g_platform_log_buffer) < entry->length + 1) {
        /* Not enough data, this shouldn't happen but handle gracefully */
        return false;
    }

    /* Read the message */
    if (circular_buffer_read(
            &g_platform_log_buffer, (uint8_t *)entry->message, entry->length + 1
        ) != entry->length + 1) {
        return false;
    }

    entry->message[entry->length] = '\0'; /* Ensure null termination */
    return true;
}
