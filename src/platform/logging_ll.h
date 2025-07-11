/**
 * @file logging_ll.h
 * @brief Platform-layer logging interface (buffer-based, no stdio)
 *
 * This module provides logging for the platform layer which:
 * - Cannot access stdio or UART directly
 * - Writes to a statically allocated ring buffer
 * - Signals the system layer via a service request flag
 * - Requires periodic servicing by the system layer
 *
 * The platform layer writes log messages to a ring buffer and sets a
 * service flag. The system layer polls this flag and forwards messages
 * to the main logging system.
 *
 * @author PSLab Team
 * @date 2025-07-09
 */

#ifndef LOGGING_LL_H
#define LOGGING_LL_H

#include "util.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Platform log levels (matches system logging levels)
 */
typedef enum {
    LOG_LL_ERROR = 0,
    LOG_LL_WARN = 1,
    LOG_LL_INFO = 2,
    LOG_LL_DEBUG = 3
} LOG_LL_Level;

/**
 * @brief Platform log buffer configuration
 */
#ifndef LOG_LL_BUFFER_SIZE
#define LOG_LL_BUFFER_SIZE 512 /* Adjust based on memory constraints */
#endif

#ifndef LOG_LL_MAX_MESSAGE_SIZE
#define LOG_LL_MAX_MESSAGE_SIZE 128 /* Max size per log message */
#endif

/**
 * @brief Platform log message structure
 */
typedef struct {
    LOG_LL_Level level;
    uint16_t length;
    char message[LOG_LL_MAX_MESSAGE_SIZE];
} LOG_LL_Entry;

/**
 * @brief Global variables exposed to system layer
 * These are defined in platform_logging.c and accessed as externs by system
 * layer
 */
extern bool volatile g_LOG_LL_service_request;
extern CircularBuffer g_LOG_LL_buffer;
extern uint8_t g_LOG_LL_buffer_data[LOG_LL_BUFFER_SIZE];

/**
 * @brief Initialize platform logging
 */
void LOG_LL_init(void);

/**
 * @brief Write a log message to the platform buffer
 *
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Variable arguments
 *
 * @return Number of bytes written to the buffer, or -1 on error
 */
int LOG_LL_write(LOG_LL_Level level, char const *format, ...);

/**
 * @brief Get the number of bytes available in the log buffer
 *
 * @return Number of bytes available for reading
 */
size_t LOG_LL_available(void);

/**
 * @brief Read a log entry from the buffer
 *
 * @param entry Pointer to entry structure to fill
 *
 * @return true if entry was read successfully, false if buffer is empty
 */
bool LOG_LL_read_entry(LOG_LL_Entry *entry);

/**
 * @brief Convenience macros for platform logging
 */
#define LOG_LL_ERROR(fmt, ...) LOG_LL_write(LOG_LL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_LL_WARN(fmt, ...) LOG_LL_write(LOG_LL_WARN, fmt, ##__VA_ARGS__)

#define LOG_LL_INFO(fmt, ...) LOG_LL_write(LOG_LL_INFO, fmt, ##__VA_ARGS__)

#define LOG_LL_DEBUG(fmt, ...) LOG_LL_write(LOG_LL_DEBUG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_LL_H */
