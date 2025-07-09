/**
 * @file platform_logging_example.c
 * @brief Example usage of the platform logging system
 *
 * This example demonstrates how to use the two-layer logging system:
 * 1. Platform layer logs to a buffer
 * 2. System layer services the buffer and forwards to stdio
 *
 * @author PSLab Team
 * @date 2025-07-09
 */

/* Platform layer example (in any platform source file) */
#ifdef PLATFORM_LAYER_EXAMPLE

#include "platform_logging.h"

void some_platform_function(void)
{
    /* Initialize platform logging once at startup */
    PLATFORM_log_init();

    /* Use platform logging throughout platform layer */
    PLATFORM_LOG_INFO("Platform initialization started");

    int status = some_hardware_init();
    if (status != 0) {
        PLATFORM_LOG_ERROR("Hardware init failed with status %d", status);
        return;
    }

    PLATFORM_LOG_DEBUG("Register value: 0x%X", read_register(0x1000));
    PLATFORM_LOG_INFO("Platform initialization completed successfully");
}

#endif /* PLATFORM_LAYER_EXAMPLE */

/* System layer example (in main application) */
#ifdef SYSTEM_LAYER_EXAMPLE

#include "logging.h"

int main(void)
{
    /* System initialization */
    LOG_INFO("System startup");

    /* Main application loop */
    while (1) {
        /* Your application logic here */

        /* Service platform logging - call this periodically */
        LOG_service_platform();

        /* Other periodic tasks */

        /* Small delay to prevent busy waiting */
        HAL_Delay(10);  /* Or your platform's delay function */
    }
}

/* Alternative: Service from timer interrupt */
void SysTick_Handler(void)
{
    /* Call platform logging service from timer interrupt */
    LOG_service_platform();

    /* Other timer-based tasks */
}

#endif /* SYSTEM_LAYER_EXAMPLE */

/**
 * Example output:
 *
 * [INFO]  System startup
 * [INFO][PLATFORM] Platform initialization started
 * [ERROR][PLATFORM] Hardware init failed with status -1
 * [DEBUG][PLATFORM] Register value: 0xDEADBEEF
 * [INFO][PLATFORM] Platform initialization completed successfully
 */
