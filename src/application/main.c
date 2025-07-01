/****************************************************************************
* Includes
*****************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bus.h"
#include "led.h"
#include "system.h"
#include "uart.h"
#include "usb.h"

/*****************************************************************************
* Macros
******************************************************************************/

#define RX_BUFFER_SIZE 256
#define TX_BUFFER_SIZE 256

#define USART3 2

/*****************************************************************************
* Static variables
******************************************************************************/

static uint8_t uart_rx_buffer_data[RX_BUFFER_SIZE] = {0};
static uint8_t uart_tx_buffer_data[TX_BUFFER_SIZE] = {0};

static bool uart_service_requested = false;
static bool usb_service_requested = false;

/*****************************************************************************
* Static prototypes
******************************************************************************/

/******************************************************************************
* Function definitions
******************************************************************************/

void uart_cb(uart_handle_t *huart, uint32_t bytes_available)
{
    (void)huart;
    (void)bytes_available;
    uart_service_requested = true;
}

void usb_cb(uint32_t bytes_available)
{
    (void)bytes_available;
    usb_service_requested = true;
}

int main(void)
{
    SYSTEM_init();
    circular_buffer_t rx_buf = {0};
    circular_buffer_t tx_buf = {0};
    circular_buffer_init(&rx_buf, uart_rx_buffer_data, RX_BUFFER_SIZE);
    circular_buffer_init(&tx_buf, uart_tx_buffer_data, TX_BUFFER_SIZE);

    
    uart_handle_t *huart = UART_init(USART3, &rx_buf, &tx_buf);

    int cb_threshold = 5;  // Callback when at least 5 bytes are available
    UART_set_rx_callback(huart, uart_cb, cb_threshold);
    USB_set_rx_callback(usb_cb, cb_threshold);

    /* Basic UART/USB/LED example:
    * - Register callbacks for both UART and USB
    * - Process incoming bytes when callbacks are triggered
    * - If a byte is received, toggle the LED
    * - Try to read five bytes (this may timeout)
    * - If the read bytes equal "Hello", then respond "World"
    * - Otherwise echo back what was received
    */
    while (1) {
        USB_task();

        if (uart_service_requested) {
            uart_service_requested = false;
            LED_toggle();
            uint8_t buf[6] = {0};
            uint32_t bytes_read = UART_read(huart, (uint8_t *)buf, 5);
            buf[bytes_read] = '\0';

            if (strcmp((char *)buf, "Hello") == 0) {
                UART_write(huart, (uint8_t *)"World", 5);
            } else {
                UART_write(huart, (uint8_t *)buf, bytes_read);
            }
        }

        if (usb_service_requested) {
            usb_service_requested = false;
            LED_toggle();
            uint8_t buf[6] = {0};
            uint32_t bytes_read = USB_read(buf, 5);
            buf[bytes_read] = '\0';

            if (strcmp((char *)buf, "Hello") == 0) {
                USB_write((uint8_t *)"World", 5);
            } else {
                USB_write((uint8_t *)buf, bytes_read);
            }
        }
    }

    __builtin_unreachable();
}
