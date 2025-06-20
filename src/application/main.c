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

/*****************************************************************************
* Static prototypes
******************************************************************************/

/******************************************************************************
* Function definitions
******************************************************************************/

int main(void)
{
    SYSTEM_init();

    /* Basic UART/LED example:
    * - Poll UART for incoming bytes
    * - If a byte is received, toggle the LED
    * - Try to read five bytes (this may timeout)
    * - If the read bytes equal "Hello", then respond "World"
    */
    while (1) {
        if (UART_rx_ready()) {
            LED_toggle();
            char buf[6];
            UART_read((uint8_t *)buf, 5);
            buf[5] = '\0';

            if (strcmp(buf, "Hello") == 0){
                UART_write((uint8_t *)"World", 5);
            }
        }
    }

    __builtin_unreachable();
}
