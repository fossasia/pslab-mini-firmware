/****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "led.h"
#include "system.h"
#include "uart.h"
#include "usb.h"

/*****************************************************************************
 * Macros
 ******************************************************************************/

enum { RX_BUFFER_SIZE = 256, TX_BUFFER_SIZE = 256 };

enum { STLINK_UART = 2 };

/*****************************************************************************
 * Static variables
 ******************************************************************************/

static uint8_t uart_rx_buffer_data[RX_BUFFER_SIZE] = { 0 };
static uint8_t uart_tx_buffer_data[TX_BUFFER_SIZE] = { 0 };
static uint8_t usb_rx_buffer_data[RX_BUFFER_SIZE] = { 0 };

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

void usb_cb(usb_handle_t *husb, uint32_t bytes_available)
{
    (void)husb;
    (void)bytes_available;
    usb_service_requested = true;
}

int main(void) // NOLINT
{
    SYSTEM_init();

    // Initialize UART
    circular_buffer_t uart_rx_buf = { nullptr };
    circular_buffer_t uart_tx_buf = { nullptr };
    circular_buffer_init(&uart_rx_buf, uart_rx_buffer_data, RX_BUFFER_SIZE);
    circular_buffer_init(&uart_tx_buf, uart_tx_buffer_data, TX_BUFFER_SIZE);
    uart_handle_t *huart = UART_init(STLINK_UART, &uart_rx_buf, &uart_tx_buf);

    // Initialize USB
    circular_buffer_t usb_rx_buf = { nullptr };
    circular_buffer_init(&usb_rx_buf, usb_rx_buffer_data, RX_BUFFER_SIZE);
    usb_handle_t *husb = USB_init(0, &usb_rx_buf);

#define CB_THRESHOLD                                                           \
    (sizeof("Hello") - 1) // Callback when at least 5 bytes are available
    UART_set_rx_callback(huart, uart_cb, CB_THRESHOLD);
    USB_set_rx_callback(husb, usb_cb, CB_THRESHOLD);

    /* Basic UART/USB/LED example:
     * - Register callbacks for both UART and USB
     * - Process incoming bytes when callbacks are triggered
     * - If a byte is received, toggle the LED
     * - Try to read five bytes (this may timeout)
     * - If the read bytes equal "Hello", then respond "World"
     * - Otherwise echo back what was received
     */
    while (1) {
        USB_task(husb);

        if (uart_service_requested) {
            uart_service_requested = false;
            LED_toggle();
            uint8_t buf[CB_THRESHOLD + 1] = { 0 };
            uint32_t bytes_read =
                UART_read(huart, (uint8_t *)buf, CB_THRESHOLD);
            buf[bytes_read] = '\0';

            if (strcmp((char *)buf, "Hello") == 0) {
                UART_write(huart, (uint8_t *)"World", CB_THRESHOLD);
            } else {
                UART_write(huart, (uint8_t *)buf, bytes_read);
            }
        }

        if (usb_service_requested) {
            usb_service_requested = false;
            LED_toggle();
            uint8_t buf[CB_THRESHOLD + 1] = { 0 };
            uint32_t bytes_read = USB_read(husb, buf, CB_THRESHOLD);
            buf[bytes_read] = '\0';

            if (strcmp((char *)buf, "Hello") == 0) {
                USB_write(husb, (uint8_t *)"World", CB_THRESHOLD);
            } else {
                USB_write(husb, (uint8_t *)buf, bytes_read);
            }
        }
    }

    __builtin_unreachable();
}
