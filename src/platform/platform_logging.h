/**
 * @file platform_logging.h
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

#ifndef PLATFORM_LOGGING_H
#define PLATFORM_LOGGING_H

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
    PLATFORM_LOG_ERROR = 0,
    PLATFORM_LOG_WARN = 1,
    PLATFORM_LOG_INFO = 2,
    PLATFORM_LOG_DEBUG = 3
} PLATFORM_LogLevel;

/**
 * @brief Platform log buffer configuration
 */
#ifndef PLATFORM_LOG_BUFFER_SIZE
#define PLATFORM_LOG_BUFFER_SIZE 512 /* Adjust based on memory constraints */
#endif

#ifndef PLATFORM_LOG_MAX_MESSAGE_SIZE
#define PLATFORM_LOG_MAX_MESSAGE_SIZE 128 /* Max size per log message */
#endif

/**
 * @brief Platform log message structure
 */
typedef struct {
    PLATFORM_LogLevel level;
    uint16_t length;
    char message[PLATFORM_LOG_MAX_MESSAGE_SIZE];
} PLATFORM_LogEntry;

/**
 * @brief Global variables exposed to system layer
 * These are defined in platform_logging.c and accessed as externs by system
 * layer
 */
extern bool volatile g_platform_log_service_request;
extern CircularBuffer g_platform_log_buffer;
extern uint8_t g_platform_log_buffer_data[PLATFORM_LOG_BUFFER_SIZE];

/**
 * @brief Initialize platform logging
 */
void PLATFORM_log_init(void);

/**
 * @brief Write a log message to the platform buffer
 *
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void PLATFORM_log_write(PLATFORM_LogLevel level, char const *format, ...);

/**
 * @brief Get the number of bytes available in the log buffer
 *
 * @return Number of bytes available for reading
 */
size_t PLATFORM_log_available(void);

/**
 * @brief Read a log entry from the buffer
 *
 * @param entry Pointer to entry structure to fill
 * @return true if entry was read successfully, false if buffer is empty
 */
bool platform_log_read_entry(PLATFORM_LogEntry *entry);

/**
 * @brief Convenience macros for platform logging
 */
#define PLATFORM_LOG_ERROR(fmt, ...)                                           \
    PLATFORM_log_write(PLATFORM_LOG_ERROR, fmt, ##__VA_ARGS__)

#define PLATFORM_LOG_WARN(fmt, ...)                                            \
    PLATFORM_log_write(PLATFORM_LOG_WARN, fmt, ##__VA_ARGS__)

#define PLATFORM_LOG_INFO(fmt, ...)                                            \
    PLATFORM_log_write(PLATFORM_LOG_INFO, fmt, ##__VA_ARGS__)

#define PLATFORM_LOG_DEBUG(fmt, ...)                                           \
    PLATFORM_log_write(PLATFORM_LOG_DEBUG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_LOGGING_H */
