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

enum { CONVERSION_ADC = 2 }; // ADC instance number
/*****************************************************************************
 * Static variables
 ******************************************************************************/

static uint8_t g_usb_rx_buffer_data[RX_BUFFER_SIZE] = { 0 };
// Buffer for USB RX data
static uint32_t g_adc_buffer_data[ADC_BUFFER_SIZE] = { 0 };
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

    ADC_Handle *hadc =
        ADC_init(CONVERSION_ADC, g_adc_buffer_data, ADC_BUFFER_SIZE);

    USB_set_rx_callback(husb, usb_cb, CB_THRESHOLD);
    ADC_set_callback(hadc, adc_cb);

    // Start ADC conversions
    ADC_start(hadc);

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
            if (CONVERSION_ADC == 2) {
                for (uint32_t i = 0; i < ADC_BUFFER_SIZE; i++) {
                    uint16_t adc2_value = (g_adc_buffer_data[i] >> 16) &
                                          0xFFF; // Extract ADC2 data
                    uint16_t adc1_value =
                        g_adc_buffer_data[i] & 0xFFF; // Extract ADC1 data
                    LOG_INFO(
                        "Sample %u: ADC1: %u, ADC2: %u",
                        i + 1,
                        adc1_value,
                        adc2_value
                    );
                }
            } else {
                for (uint32_t i = 0; i < ADC_BUFFER_SIZE; i++) {
                    LOG_INFO("Sample %u: %u", i + 1, g_adc_buffer_data[i]);
                }
            }
            ADC_restart(hadc); // Restart ADC for next conversion
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
