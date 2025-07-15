/**
 * @file logging.c
 * @brief Unified logging system implementation using circular buffer
 *
 * This module implements a unified logging system that uses a circular buffer
 * for message storage. It provides thread-safe logging with configurable
 * levels and supports printf-style formatting.
 *
 * @author PSLab Team
 * @date 2025-07-14
 */

#include "logging.h"
#include "error.h"
#include "util.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Log message entry structure
 */
typedef struct {
    LOG_Level level;
    uint16_t length;
    char message[LOG_MAX_MESSAGE_SIZE];
} LOG_Entry;

/**
 * @brief Internal logging state
 */
typedef struct {
    CircularBuffer buffer;
    uint8_t buffer_data[LOG_BUFFER_SIZE];
    bool initialized;
} LOG_State;

/**
 * @brief Global logging state
 */
static LOG_State g_log_state = { .initialized = false };

/**
 * @brief Level names for formatted output
 */
static char const *const g_LEVEL_NAMES[] = { "ERROR",
                                             "WARN ",
                                             "INFO ",
                                             "DEBUG" };

/**
 * @brief Initialize the logging system
 */
void LOG_init(void)
{
    static_assert(
        LOG_BUFFER_SIZE > 0 && (LOG_BUFFER_SIZE & (LOG_BUFFER_SIZE - 1)) == 0,
        "LOG_BUFFER_SIZE must be a power of 2"
    );
    static_assert(
        LOG_MAX_MESSAGE_SIZE > 0 && LOG_MAX_MESSAGE_SIZE < 512,
        "LOG_MAX_MESSAGE_SIZE must be reasonable (1-511)"
    );

    circular_buffer_init(
        &g_log_state.buffer, g_log_state.buffer_data, LOG_BUFFER_SIZE
    );
    g_log_state.initialized = true;
}

/**
 * @brief Write a log message
 */
int LOG_write(LOG_Level level, char const *format, ...)
{
    if (!g_log_state.initialized) {
        return -1; /* Not initialized */
    }

    if (!format) {
        return -1; /* Invalid format string */
    }

    /* Prepare log entry */
    LOG_Entry entry;
    entry.level = level;

    va_list args;
    va_start(args, format);
    int content_len =
        vsnprintf(entry.message, sizeof(entry.message), format, args);
    va_end(args);

    if (content_len < 0) {
        return -1; /* Content formatting error */
    }

    if (content_len >= (int)sizeof(entry.message)) {
        content_len = sizeof(entry.message) - 1;
    }
    entry.message[content_len] = '\0';
    entry.length = (uint16_t)content_len;

    /* Calculate total entry size for buffer */
    size_t entry_size =
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1;

    /* Check if we have enough space */
    if (circular_buffer_free_space(&g_log_state.buffer) >= entry_size) {
        /* Write entry to buffer */
        uint32_t written = 0;
        written += circular_buffer_write(
            &g_log_state.buffer,
            (uint8_t const *)&entry.level,
            sizeof(entry.level)
        );
        written += circular_buffer_write(
            &g_log_state.buffer,
            (uint8_t const *)&entry.length,
            sizeof(entry.length)
        );
        written += circular_buffer_write(
            &g_log_state.buffer,
            (uint8_t const *)entry.message,
            entry.length + 1
        );

        return (int)written;
    }

    /* Buffer full - message dropped (acceptable for logging) */
    return -1;
}

/**
 * @brief Check if log messages are available for reading
 */
size_t LOG_available(void)
{
    if (!g_log_state.initialized) {
        return 0;
    }
    return circular_buffer_available(&g_log_state.buffer);
}

/**
 * @brief Read a log entry from the buffer
 *
 * @param entry Pointer to entry structure to fill
 * @return true if entry was read successfully, false if buffer is empty
 */
static bool read_entry(LOG_Entry *entry)
{
    if (!entry || !g_log_state.initialized) {
        return false;
    }

    /* Check if we have enough data for header */
    if (circular_buffer_available(&g_log_state.buffer) <
        sizeof(entry->level) + sizeof(entry->length)) {
        return false;
    }

    /* Read level and length */
    if (circular_buffer_read(
            &g_log_state.buffer, (uint8_t *)&entry->level, sizeof(entry->level)
        ) != sizeof(entry->level) ||
        circular_buffer_read(
            &g_log_state.buffer,
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
    if (circular_buffer_available(&g_log_state.buffer) < entry->length + 1) {
        return false;
    }

    /* Read the message */
    if (circular_buffer_read(
            &g_log_state.buffer, (uint8_t *)entry->message, entry->length + 1
        ) != entry->length + 1) {
        return false;
    }

    /* Ensure null termination */
    entry->message[entry->length] = '\0';
    return true;
}

/**
 * @brief Service log messages by outputting them to stdout
 *
 * This function reads log entries from the buffer and outputs them to stdout.
 * It should be called periodically by the application main loop or from a
 * timer interrupt to ensure log messages are displayed in a timely manner.
 *
 * The function processes up to a limited number of entries per call to avoid
 * blocking for too long in interrupt contexts.
 */
void LOG_task(void)
{
    if (!g_log_state.initialized) {
        return;
    }

    /* Limit the number of entries processed per call to avoid blocking */
    enum { MAX_ENTRIES_PER_CALL = 8 };
    LOG_Entry entry;
    int processed = 0;

    /* Process available log entries */
    while (processed < MAX_ENTRIES_PER_CALL && read_entry(&entry)) {
        /* Format and output the message to stdout */
        printf("[%s] %s\r\n", g_LEVEL_NAMES[entry.level], entry.message);
        processed++;
    }

    /* Flush stdout to ensure messages appear immediately */
    if (processed > 0) {
        (void)fflush(stdout);
    }
}
