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

#include "logging_ll.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Global log buffer and state variables
 * These are accessed by the system layer as externs
 */
bool volatile g_LOG_LL_service_request = false;
CircularBuffer g_LOG_LL_buffer;
uint8_t g_LOG_LL_buffer_data[LOG_LL_BUFFER_SIZE];

/**
 * @brief Internal state
 */
static bool g_initialized = false;

/**
 * @brief Initialize platform logging
 */
void LOG_LL_init(void)
{
    circular_buffer_init(
        &g_LOG_LL_buffer, g_LOG_LL_buffer_data, LOG_LL_BUFFER_SIZE
    );
    g_LOG_LL_service_request = false;
    g_initialized = true;
}

/**
 * @brief Write a log message to the platform buffer
 */
int LOG_LL_write(LOG_LL_Level level, char const *format, ...)
{
    if (!g_initialized) {
        return -1; /* Not initialized */
    }

    /* Prepare log entry */
    LOG_LL_Entry entry;
    entry.level = level;

    /* Format the message */
    va_list args;
    va_start(args, format);
    int formatted_length =
        vsnprintf(entry.message, sizeof(entry.message), format, args);
    // NOLINTNEXTLINE: clang-analyzer-valist.Uninitialized (false positive)
    va_end(args);

    if (formatted_length < 0) {
        return -1; /* Formatting error */
    }

    entry.length = (uint16_t)(formatted_length < sizeof(entry.message)
                                  ? formatted_length
                                  : sizeof(entry.message) - 1);
    entry.message[entry.length] = '\0'; /* Ensure null termination */

    /* Calculate total entry size */
    uint32_t entry_size =
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1;

    /* Check if we have enough space */
    if (circular_buffer_free_space(&g_LOG_LL_buffer) >= entry_size) {
        /* Write entry to buffer */
        circular_buffer_write(
            &g_LOG_LL_buffer, (uint8_t *)&entry.level, sizeof(entry.level)
        );
        circular_buffer_write(
            &g_LOG_LL_buffer, (uint8_t *)&entry.length, sizeof(entry.length)
        );
        circular_buffer_write(
            &g_LOG_LL_buffer, (uint8_t *)entry.message, entry.length + 1
        );

        /* Signal system layer that service is needed */
        g_LOG_LL_service_request = true;
        return entry_size; /* Write OK */
    }
    /* If write fails, message is dropped (acceptable for logging) */
    return -1; /* Not enough space in buffer */
}

/**
 * @brief Get the number of bytes available in the log buffer
 */
size_t LOG_LL_available(void)
{
    return circular_buffer_available(&g_LOG_LL_buffer);
}

/**
 * @brief Read a log entry from the buffer
 */
bool LOG_LL_read_entry(LOG_LL_Entry *entry)
{
    if (!entry || circular_buffer_available(&g_LOG_LL_buffer) <
                      sizeof(entry->level) + sizeof(entry->length)) {
        return false;
    }

    /* Read level and length first */
    if (circular_buffer_read(
            &g_LOG_LL_buffer, (uint8_t *)&entry->level, sizeof(entry->level)
        ) != sizeof(entry->level) ||
        circular_buffer_read(
            &g_LOG_LL_buffer, (uint8_t *)&entry->length, sizeof(entry->length)
        ) != sizeof(entry->length)) {
        return false;
    }

    /* Validate length */
    if (entry->length >= sizeof(entry->message)) {
        entry->length = sizeof(entry->message) - 1;
    }

    /* Check if we have enough data for the message */
    if (circular_buffer_available(&g_LOG_LL_buffer) < entry->length + 1) {
        /* Not enough data, this shouldn't happen but handle gracefully */
        return false;
    }

    /* Read the message */
    if (circular_buffer_read(
            &g_LOG_LL_buffer, (uint8_t *)entry->message, entry->length + 1
        ) != entry->length + 1) {
        return false;
    }

    entry->message[entry->length] = '\0'; /* Ensure null termination */
    return true;
}
