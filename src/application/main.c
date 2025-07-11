/****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adc.h"
#include "error.h"
#include "led.h"
#include "logging.h"
#include "syscalls_config.h"
#include "system.h"
#include "uart.h"
#include "usb.h"
#include "util.h"

/*****************************************************************************
 * Macros
 ******************************************************************************/

// Callback when at least 5 bytes are available
#define CB_THRESHOLD (sizeof("Hello") - 1)
enum { RX_BUFFER_SIZE = 256 };

/*****************************************************************************
 * Static variables
 ******************************************************************************/

static uint8_t usb_rx_buffer_data[RX_BUFFER_SIZE] = { 0 };
static bool usb_service_requested = false;

/*****************************************************************************
 * Static prototypes
 ******************************************************************************/

/******************************************************************************
 * Function definitions
 ******************************************************************************/

void usb_cb(USB_Handle *husb, uint32_t bytes_available)
{
    (void)husb;
    (void)bytes_available;
    usb_service_requested = true;
}

int main(void) // NOLINT
{
    Error e;
    SYSTEM_init();
    // Try to initialize SYSCALLS_UART_BUS directly, demonstrating CException
    // handling
    TRY
    {
        uint32_t uart_buf_sz = 8;
        uint8_t uart_rx_buffer_data[uart_buf_sz];
        uint8_t uart_tx_buffer_data[uart_buf_sz];
        CircularBuffer uart_rx_buf;
        CircularBuffer uart_tx_buf;
        circular_buffer_init(&uart_rx_buf, uart_rx_buffer_data, uart_buf_sz);
        circular_buffer_init(&uart_tx_buf, uart_tx_buffer_data, uart_buf_sz);
        UART_Handle *huart =
            UART_init(SYSCALLS_UART_BUS, &uart_rx_buf, &uart_tx_buf);
        if (huart == nullptr) {
            LOG_ERROR("Failed to initialize SYSCALLS_UART_BUS");
            THROW(ERROR_RESOURCE_UNAVAILABLE);
        }
    }
    CATCH(e)
    {
        LOG_DEBUG("Caught exception during UART initialization:");
        LOG_DEBUG("%d %s", e, error_to_string(e));
    }

    // Initialize USB
    CircularBuffer usb_rx_buf;
    circular_buffer_init(&usb_rx_buf, usb_rx_buffer_data, RX_BUFFER_SIZE);
    USB_Handle *husb = USB_init(0, &usb_rx_buf);

    USB_set_rx_callback(husb, usb_cb, CB_THRESHOLD);

    // Initialize ADC
    ADC_init();

    /* Basic USB/LED example:
     * - Process incoming bytes when USB callback is triggered
     * - If a byte is received, toggle the LED
     * - If the read bytes equal "Hello", then respond "World"
     * - Otherwise echo back what was received
     * - Use printf for debugging output to UART
     */
    while (1) {
        USB_task(husb);

        // Read low-level logs
        LOG_service_platform();

        // Log system status periodically (optional)
        static uint32_t log_counter = 0;
        if (++log_counter % 1000000 == 0) {
            LOG_INFO("System running, USB active");
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
