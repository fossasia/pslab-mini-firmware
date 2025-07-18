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
enum { ADC_BUFFER_SIZE = 256 }; // Size of ADC buffer for DMA

/*****************************************************************************
 * Static variables
 ******************************************************************************/

static uint8_t g_usb_rx_buffer_data[RX_BUFFER_SIZE] = { 0 };
// Buffer for USB RX data
static uint16_t g_adc_buffer_data[ADC_BUFFER_SIZE] = { 0 };
// Buffer for ADC data

static bool g_usb_service_requested = false;
bool volatile g_adc_ready = false;

/*****************************************************************************
 * Static prototypes
 ******************************************************************************/

/******************************************************************************
 * Function definitions
 ******************************************************************************/

void usb_cb(USB_Handle *husb, uint32_t bytes_available)
{
    LOG_FUNCTION_ENTRY();
    (void)husb;
    (void)bytes_available;
    g_usb_service_requested = true;
    LOG_FUNCTION_EXIT();
}

static void adc_cb(ADC_Handle *hadc)
{
    LOG_FUNCTION_ENTRY();
    (void)hadc;
    g_adc_ready = true; // Set flag to indicate ADC data is ready
    LOG_FUNCTION_EXIT();
}

int main(void) // NOLINT
{
    Error exc = ERROR_NONE;
    SYSTEM_init();
    LOG_INIT("Main application");
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
        // This will fail
        (void)UART_init(SYSCALLS_UART_BUS, &uart_rx_buf, &uart_tx_buf);
    }
    CATCH(exc)
    {
        LOG_ERROR("Failed to initialize SYSCALLS_UART_BUS");
        LOG_ERROR("%d %s", exc, error_to_string(exc));
    }

    // Initialize USB
    CircularBuffer usb_rx_buf;
    circular_buffer_init(&usb_rx_buf, g_usb_rx_buffer_data, RX_BUFFER_SIZE);
    USB_Handle *husb = USB_init(0, &usb_rx_buf);

    ADC_init(g_adc_buffer_data, ADC_BUFFER_SIZE);

    USB_set_rx_callback(husb, usb_cb, CB_THRESHOLD);
    ADC_set_callback(adc_cb);

    // Start ADC conversions
    ADC_start();

    /* Basic USB/LED example:
     * - Process incoming bytes when USB callback is triggered
     * - If a byte is received, toggle the LED
     * - If the read bytes equal "Hello", then respond "World"
     * - Otherwise echo back what was received
     * - Use printf for debugging output to UART
     */
    while (1) {
        USB_task(husb);

        // Output logs
        LOG_task(8);

        // Log system status periodically (optional)
        static uint32_t log_counter = 0;
        enum { PING_INTERVAL = 1000000 }; // Log every 1 million iterations
        if (log_counter++ % PING_INTERVAL == 0) {
            LOG_INFO("System running, USB active");
        }

        if (g_adc_ready) {
            g_adc_ready = false;
            LED_toggle();
            uint16_t buf[ADC_BUFFER_SIZE] = { 0 };
            uint32_t samples_read = ADC_read(buf, ADC_BUFFER_SIZE);

            if (samples_read > 0) {

                uint16_t num_samples = samples_read;
                for (uint32_t i = 0; i < num_samples; i++) {
                    LOG_INFO("Sample %u: %u", i + 1, buf[i]);
                }
            } else {
                LOG_ERROR("ADC read failed or no data available");
            }
            // ADC_restart(); // Restart ADC for next conversion
        }

        if (g_usb_service_requested) {
            g_usb_service_requested = false;
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
