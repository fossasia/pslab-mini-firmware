
#include <stdio.h>

#include "system/led.h"
#include "system/system.h"
#include "util/error.h"
#include "util/logging.h"
#include "protocol.h"

int main(void)
{
    SYSTEM_init();
    LOG_INIT("Main application");

    // Initialize the protocol
    if (!protocol_init()) {
        LOG_ERROR("Failed to initialize protocol");
        return -1;
    }

    // Main application loop
    while (1) {
        // Process protocol tasks
        protocol_task();

        // Add other application tasks here
        // For example: sensor readings, LED updates, etc.

        // Small delay to prevent 100% CPU usage
        // In a real implementation, this would be handled by an RTOS
        // or proper sleep/wait functions
    }

    __builtin_unreachable();
}
