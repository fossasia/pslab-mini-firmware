/****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/bus/uart.h"
#include "system/bus/usb.h"
#include "system/instrument/dmm.h"
#include "system/led.h"
#include "system/syscalls_config.h"
#include "system/system.h"
#include "util/error.h"
#include "util/logging.h"
#include "util/util.h"

/*****************************************************************************
 * Macros
 ******************************************************************************/

// Callback when at least 5 bytes are available
#define CB_THRESHOLD (sizeof("Hello") - 1)

enum { RX_BUFFER_SIZE = 256 };

enum { CONVERSION_ADC = 2 }; // ADC instance number
/*****************************************************************************
 * Static variables
 ******************************************************************************/

// Buffer for USB RX data
static uint8_t g_usb_rx_buffer_data[RX_BUFFER_SIZE] = { 0 };

static bool g_usb_service_requested = false;

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

int main(void) // NOLINT
{
    SYSTEM_init();
    LOG_INIT("Main application");

    DMM_Handle *dmmh = DMM_init(&(DMM_Config)DMM_CONFIG_DEFAULT);

    // Initialize USB
    CircularBuffer usb_rx_buf;
    circular_buffer_init(&usb_rx_buf, g_usb_rx_buffer_data, RX_BUFFER_SIZE);
    USB_Handle *husb = USB_init(0, &usb_rx_buf);

    USB_set_rx_callback(husb, usb_cb, CB_THRESHOLD);

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
            FIXED_Q1616 voltage = FIXED_ZERO;
            bool new_reading = DMM_read_voltage(dmmh, &voltage);
            (void)new_reading;
            LOG_INFO(
                "DMM Channel 0 Voltage: %d.%04d V (new_reading=%s)",
                FIXED_get_integer_part(voltage),
                (FIXED_get_fractional_part(voltage) * 10000) >> 16,
                new_reading ? "true" : "false"
            );
        }

        if (g_usb_service_requested) {
            g_usb_service_requested = false;
            LED_toggle(LED_GREEN);
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
