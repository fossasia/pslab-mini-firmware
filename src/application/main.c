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

    /* * This example demonstrates the use of UART with DMA for non-blocking
    * communication. It includes the following features:
    * - Transmit a buffer of ten bytes
    * - Receive a buffer of ten bytes
    * - Use DMA for UART transmission and reception
    * - Use HAL_UART_Transmit_DMA and HAL_UART_Receive_DMA for non-blocking communication
    */

    UART_write_DMA(tx_buffer, 10);
    UART_read_DMA(rx_buffer, 10);
    while (1) {
	}

    __builtin_unreachable();
}
