/****************************************************************************
* Includes
*****************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "system.h"

/*****************************************************************************
* Macros
******************************************************************************/

/*****************************************************************************
* Static variables
******************************************************************************/

static bool uart_service_requested = false;

/*****************************************************************************
* Static prototypes
******************************************************************************/

/******************************************************************************
* Function definitions
******************************************************************************/

void uart_cb(uint32_t bytes_available)
{
    (void)bytes_available;
    uart_service_requested = true;
}

int main(void)
{
    SYSTEM_init();
    int cb_threshold = 5;  // Callback when at least 5 bytes are available
    UART_set_rx_callback(uart_cb, cb_threshold);

    /* Basic UART/LED example:
    * - Poll UART for incoming bytes
    * - Wait for at least 5 bytes to arrive
    * - If the read bytes equal "Hello", then respond "World"
    * - Otherwise echo back what was received
    */
    while (1) {
        if (uart_service_requested) {
            uart_service_requested = false;
            LED_toggle();
            char buf[6] = {0};
            uint32_t bytes_read = UART_read((uint8_t *)buf, 5);
            buf[bytes_read] = '\0';  // Ensure null termination

            if (strcmp(buf, "Hello") == 0){
                UART_write((uint8_t *)"World", 5);
            } else {
                UART_write((uint8_t *)buf, bytes_read);
            }
        }
    }

    __builtin_unreachable();
}
