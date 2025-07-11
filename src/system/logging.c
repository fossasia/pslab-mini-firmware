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
#include "logging_ll.h"

/**
 * @brief Service platform logging - forward messages to system logging
 */ // NOLINTNEXTLINE: readability-function-cognitive-complexity
void LOG_service_platform(void)
{
    /* Quick check without expensive operations */
    if (!g_LOG_LL_service_request) {
        return;
    }

    enum { SERVICE_MAX_ENTRIES = 8 };
    LOG_LL_Entry entry;
    int processed = 0;

    /* Process up to SERVICE_MAX_ENTRIES platform log entries */
    while (processed < SERVICE_MAX_ENTRIES && LOG_LL_read_entry(&entry)) {
        /* Forward to appropriate system log level */
        switch (entry.level) {
        case LOG_LL_ERROR:
            LOG_ERROR("[LL] %s", entry.message);
            break;

        case LOG_LL_WARN:
            LOG_WARN("[LL] %s", entry.message);
            break;

        case LOG_LL_INFO:
            LOG_INFO("[LL] %s", entry.message);
            break;

        default:
            /* Unknown level, fallthrough to debug */
        case LOG_LL_DEBUG:
            LOG_DEBUG("[LL] %s", entry.message);
            break;
        }
        processed++;
    }

    /* Only clear service request flag if no entries remain */
    if (!LOG_LL_available()) {
        g_LOG_LL_service_request = false;
    }
}
