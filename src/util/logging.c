/**
 * @file logging.c
 * @brief PSLab logging system implementation
 *
 * @author PSLab Team
 * @date 2025-07-14
 */

#include "logging.h"
#include "util.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Internal logging state
 */
struct LOG_Handle {
    CircularBuffer buffer;
    uint8_t buffer_data[LOG_BUFFER_SIZE];
    bool initialized;
};

/**
 * @brief Global logging state
 */
static struct LOG_Handle g_log_handle = { .initialized = false };

/**
 * @brief Level names for formatted output
 */
static char const *const g_LEVEL_NAMES[] = { "ERROR",
                                             "WARN ",
                                             "INFO ",
                                             "DEBUG" };

LOG_Handle *LOG_init(void)
{
    static_assert(
        LOG_BUFFER_SIZE > 0 && (LOG_BUFFER_SIZE & (LOG_BUFFER_SIZE - 1)) == 0,
        "LOG_BUFFER_SIZE must be a power of 2"
    );
    static_assert(
        LOG_MAX_MESSAGE_SIZE > 0 && LOG_MAX_MESSAGE_SIZE < 512,
        "LOG_MAX_MESSAGE_SIZE must be reasonable (1-511)"
    );

    if (g_log_handle.initialized) {
        return &g_log_handle; // Already initialized
    }

    circular_buffer_init(
        &g_log_handle.buffer, g_log_handle.buffer_data, LOG_BUFFER_SIZE
    );
    g_log_handle.initialized = true;

    return &g_log_handle;
}

void LOG_deinit(LOG_Handle *handle)
{
    if (handle != &g_log_handle) {
        return; // Only deinit the global handle
    }

    circular_buffer_reset(&g_log_handle.buffer);
    g_log_handle.initialized = false;
}

int LOG_write(LOG_Level level, char const *format, ...)
{
    if (!g_log_handle.initialized) {
        return -1; /* Not initialized */
    }

    if (level < LOG_LEVEL_ERROR || level > LOG_LEVEL_DEBUG) {
        return -1; /* Invalid log level */
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

    /* Truncate if necessary */
    if (content_len >= (int)sizeof(entry.message)) {
        content_len = sizeof(entry.message) - 1;
    }
    entry.message[content_len] = '\0';
    entry.length = (uint16_t)content_len;

    /* Calculate total entry size for buffer */
    size_t entry_size =
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1;

    /* Check if we have enough space */
    if (circular_buffer_free_space(&g_log_handle.buffer) >= entry_size) {
        /* Write entry to buffer */
        uint32_t written = 0;
        written += circular_buffer_write(
            &g_log_handle.buffer,
            (uint8_t const *)&entry.level,
            sizeof(entry.level)
        );
        written += circular_buffer_write(
            &g_log_handle.buffer,
            (uint8_t const *)&entry.length,
            sizeof(entry.length)
        );
        written += circular_buffer_write(
            &g_log_handle.buffer,
            (uint8_t const *)entry.message,
            entry.length + 1
        );

        return (int)written;
    }

    /* Buffer full - message dropped (acceptable for logging) */
    return -1;
}

size_t LOG_available(void)
{
    if (!g_log_handle.initialized) {
        return 0;
    }
    return circular_buffer_available(&g_log_handle.buffer);
}

bool LOG_read_entry(LOG_Entry *entry)
{
    if (!entry || !g_log_handle.initialized) {
        return false;
    }

    /* Check if we have enough data for header */
    if (circular_buffer_available(&g_log_handle.buffer) <
        sizeof(entry->level) + sizeof(entry->length)) {
        return false;
    }

    /* Read level and length */
    if (circular_buffer_read(
            &g_log_handle.buffer, (uint8_t *)&entry->level, sizeof(entry->level)
        ) != sizeof(entry->level) ||
        circular_buffer_read(
            &g_log_handle.buffer,
            (uint8_t *)&entry->length,
            sizeof(entry->length)
        ) != sizeof(entry->length)) {
        return false;
    }

    /* Check if we have enough data for the message */
    if (circular_buffer_available(&g_log_handle.buffer) < entry->length + 1) {
        return false;
    }

    /* Read the message */
    if (circular_buffer_read(
            &g_log_handle.buffer, (uint8_t *)entry->message, entry->length + 1
        ) != entry->length + 1) {
        return false;
    }

    return true;
}

int LOG_task(uint32_t max_entries)
{
    if (!g_log_handle.initialized) {
        return 0;
    }

    /* Limit the number of entries processed per call to avoid blocking */
    LOG_Entry entry;
    int processed = 0;

    /* Process available log entries */
    while (processed < max_entries && LOG_read_entry(&entry)) {
        /* Format and output the message to stdout */
        printf("[%s] %s\r\n", g_LEVEL_NAMES[entry.level], entry.message);
        processed++;
    }

    /* Flush stdout to ensure messages appear immediately */
    if (processed > 0) {
        (void)fflush(stdout);
    }

    return processed;
}
