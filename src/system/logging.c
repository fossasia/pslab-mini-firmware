/**
 * @file logging.c
 * @brief System logging implementation with platform layer integration
 *
 * This module implements the platform logging service that forwards
 * log messages from the platform layer to the main system logging.
 *
 * @author PSLab Team
 * @date 2025-07-09
 */

#include <stdint.h>

#include "logging.h"
#include "platform_logging.h"

/**
 * @brief Convert platform log level to system log level
 */
static char const *get_level_string(int platform_level)
{
    switch (platform_level) {
    case PLATFORM_LOG_ERROR:
        return "ERROR";
    case PLATFORM_LOG_WARN:
        return "WARN";
    case PLATFORM_LOG_INFO:
        return "INFO";
    case PLATFORM_LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Service platform logging - forward messages to system logging
 */ // NOLINTNEXTLINE: readability-function-cognitive-complexity
void LOG_service_platform(void)
{
    /* Quick check without expensive operations */
    if (!g_platform_log_service_request) {
        return;
    }

    PLATFORM_LogEntry entry;
    bool processed_any = false;

    /* Process all available platform log entries */
    while (platform_log_read_entry(&entry)) {
        processed_any = true;

        /* Forward to appropriate system log level */
        switch (entry.level) {
        case PLATFORM_LOG_ERROR:
            LOG_ERROR(
                "[%s]PLATFORM: %s", get_level_string(entry.level), entry.message
            );
            break;

        case PLATFORM_LOG_WARN:
            LOG_WARN(
                "[%s]PLATFORM: %s", get_level_string(entry.level), entry.message
            );
            break;

        case PLATFORM_LOG_INFO:
            LOG_INFO(
                "[%s]PLATFORM: %s", get_level_string(entry.level), entry.message
            );
            break;

        default:
            /* Unknown level, fallthrough to debug */
        case PLATFORM_LOG_DEBUG:
            LOG_DEBUG(
                "[%s]PLATFORM: %s", get_level_string(entry.level), entry.message
            );
            break;
        }
    }

    /* Clear service request flag only after processing all messages */
    if (processed_any) {
        g_platform_log_service_request = false;
    }
}
