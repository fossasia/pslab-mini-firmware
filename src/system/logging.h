#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

/**
 * @file logging.h
 * @brief Logging system using stdio (UART-backed via syscalls)
 *
 * This logging system uses stdio functions (printf family) which are
 * implemented via UART through the syscalls layer. This provides:
 * - Standard C library compatibility
 * - Rich formatting support
 * - Tool integration (GDB, debuggers)
 * - Configurable via syscalls_config.h
 */

// Log levels
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} LOG_Level;

// Default log level (can be overridden via preprocessor)
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Timestamp support (optional - define LOG_WITH_TIMESTAMP to enable)
#ifdef LOG_WITH_TIMESTAMP
#include <time.h>
#define LOG_TIMESTAMP() printf("[%lu] ", (unsigned long)time(NULL))
#else
#define LOG_TIMESTAMP()
#endif

// Core logging macros
#define LOG_ERROR(fmt, ...)                                                    \
    do {                                                                       \
        if (LOG_LEVEL >= LOG_LEVEL_ERROR) {                                    \
            LOG_TIMESTAMP();                                                   \
            printf("[ERROR] " fmt "\n", ##__VA_ARGS__);                        \
            (void)fflush(stdout);                                              \
        }                                                                      \
    } while (0)

#define LOG_WARN(fmt, ...)                                                     \
    do {                                                                       \
        if (LOG_LEVEL >= LOG_LEVEL_WARN) {                                     \
            LOG_TIMESTAMP();                                                   \
            printf("[WARN]  " fmt "\n", ##__VA_ARGS__);                        \
        }                                                                      \
    } while (0)

#define LOG_INFO(fmt, ...)                                                     \
    do {                                                                       \
        if (LOG_LEVEL >= LOG_LEVEL_INFO) {                                     \
            LOG_TIMESTAMP();                                                   \
            printf("[INFO]  " fmt "\n", ##__VA_ARGS__);                        \
        }                                                                      \
    } while (0)

#define LOG_DEBUG(fmt, ...)                                                    \
    do {                                                                       \
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {                                    \
            LOG_TIMESTAMP();                                                   \
            printf("[DEBUG] " fmt "\n", ##__VA_ARGS__);                        \
        }                                                                      \
    } while (0)

// Convenience macros for common patterns
#define LOG_INIT(subsystem) LOG_INFO("Initializing " subsystem)
#define LOG_DEINIT(subsystem) LOG_INFO("Deinitializing " subsystem)
#define LOG_FUNCTION_ENTRY() LOG_DEBUG("Entering %s", __func__)
#define LOG_FUNCTION_EXIT() LOG_DEBUG("Exiting %s", __func__)

/**
 * @brief Platform logging service - forwards platform log messages to system
 * logging
 *
 * The platform layer writes to a ring buffer and sets a service request flag.
 * This function checks the flag and forwards any pending messages to the
 * main logging system.
 *
 * This function should be called periodically by the application main loop
 * or from a timer interrupt.
 */
void LOG_service_platform(void);

#endif // LOGGING_H
