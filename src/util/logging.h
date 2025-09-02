/**
 * @file logging.h
 * @brief Unified logging system using circular buffer
 *
 * This logging system provides a unified interface for all firmware layers
 * using a circular buffer for message storage. It supports:
 * - Multiple log levels (ERROR, WARN, INFO, DEBUG)
 * - Printf-style formatting
 * - Configurable buffer size
 *
 * The logging system uses a circular buffer to store log messages, which can
 * be read back by the application using the `LOG_read_entry` function, or
 * written to stdout by the `LOG_task` function.
 *
 * @author PSLab Team
 * @date 2025-07-14
 */

#ifndef PSLAB_LOGGING_H
#define PSLAB_LOGGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "util/util.h" // Include circular buffer definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log levels
 */
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} LOG_Level;

/**
 * @brief Logging configuration
 */
#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 1024 /* Default buffer size - should be power of 2 */
#endif

#ifndef LOG_MAX_MESSAGE_SIZE
#define LOG_MAX_MESSAGE_SIZE 128 /* Maximum size per log message */
#endif

#ifndef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ERROR
#endif

/**
 * @brief Log handle structure (opaque)
 */
typedef struct LOG_Handle LOG_Handle;

/**
 * @brief Log message entry structure
 */
typedef struct {
    LOG_Level level;
    uint16_t length;
    char message[LOG_MAX_MESSAGE_SIZE];
} LOG_Entry;

/**
 * @brief Initialize the logging system
 *
 * This must be called before any logging functions are used.
 * It initializes the internal circular buffer and sets up the logging state.
 *
 * @return Pointer to the log handle, or nullptr on error
 */
LOG_Handle *LOG_init(void);

/**
 * @brief Deinitialize the logging system
 *
 * This should be called when the logging system is no longer needed.
 * It resets the internal state and frees any resources.
 *
 * @param handle Pointer to the log handle to deinitialize
 */
void LOG_deinit(LOG_Handle *handle);

/**
 * @brief Write a log message
 *
 * @param level Log level for this message
 * @param format Printf-style format string
 * @param ... Variable arguments
 *
 * @return Number of bytes written to buffer, or -1 on error
 */
int LOG_write(LOG_Level level, char const *format, ...);

/**
 * @brief Check if log messages are available for reading
 *
 * @return Number of bytes available in the log buffer
 */
size_t LOG_available(void);

/**
 * @brief Read a log entry from the buffer
 *
 * @param entry Pointer to entry structure to fill
 * @return true if entry was read successfully, false if buffer is empty
 */
bool LOG_read_entry(LOG_Entry *entry);

/**
 * @brief Service log messages by outputting them to stdout
 *
 * This function reads log entries from the buffer and outputs them to stdout.
 * It should be called periodically by the application main loop or from a
 * timer interrupt to ensure log messages are displayed in a timely manner.
 *
 * The function processes up to a limited number of entries per call to avoid
 * blocking for too long in interrupt contexts.
 *
 * @param max_entries Maximum number of entries to process in this call
 *
 * @return Number of entries processed
 */
int LOG_task(uint32_t max_entries);

/**
 * @brief Core logging macros with compile-time filtering
 */
#if LOG_COMPILE_TIME_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) LOG_write(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) ((void)0)
#endif

#if LOG_COMPILE_TIME_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) LOG_write(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) ((void)0)
#endif

#if LOG_COMPILE_TIME_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) LOG_write(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_COMPILE_TIME_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) LOG_write(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

/**
 * @brief Convenience macros for common patterns
 */
#define LOG_INIT(subsystem) LOG_INFO("Initializing " subsystem)
#define LOG_DEINIT(subsystem) LOG_INFO("Deinitializing " subsystem)
#define LOG_FUNCTION_ENTRY() LOG_DEBUG("Entering %s", __func__)
#define LOG_FUNCTION_EXIT() LOG_DEBUG("Exiting %s", __func__)

/**
 * @brief Level-specific helper macros for backwards compatibility
 */
#define LOG_LL_ERROR(fmt, ...) LOG_ERROR(fmt, ##__VA_ARGS__)
#define LOG_LL_WARN(fmt, ...) LOG_WARN(fmt, ##__VA_ARGS__)
#define LOG_LL_INFO(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)
#define LOG_LL_DEBUG(fmt, ...) LOG_DEBUG(fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* PSLAB_LOGGING_H */
