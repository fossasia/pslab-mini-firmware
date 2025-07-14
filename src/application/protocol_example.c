/**
 * @file protocol_example.c
 * @brief Example usage of the SCPI protocol
 *
 * This file demonstrates how to use the SCPI protocol implementation
 * in a main application loop.
 */

#include "protocol.h"
#include <stdio.h>

/**
 * @brief Example main function showing protocol usage
 */
int main(void)
{
    // Initialize the protocol
    if (!protocol_init()) {
        printf("Failed to initialize protocol\n");
        return -1;
    }

    printf("SCPI protocol initialized successfully\n");
    printf("Connect via USB and send SCPI commands:\n");
    printf("  *IDN?          - Get identification\n");
    printf("  *RST           - Reset instrument\n");
    printf("  MEAS:VOLT:DC?  - Measure DC voltage\n");
    printf("  CONF:VOLT:DC   - Configure voltage measurement\n");

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

    // Cleanup (this code will never be reached in this example)
    protocol_deinit();
    return 0;
}
