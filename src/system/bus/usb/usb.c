/**
 * @file usb.c
 * @brief USB interface implementation for TinyUSB
 *
 * This module provides a CDC-based USB API on top of the TinyUSB stack,
 * mirroring the UART API functionality to provide a consistent interface
 * for different communication methods.
 *
 * Features:
 * - Non-blocking read/write operations
 * - Configurable RX callback for protocol implementations
 * - Buffer status inquiry functions
 * - Circular buffer for reliable USB data reception
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"

#include "bus_common.h"
#include "usb.h"
#include "usb_internal.h"

/* Circular buffer size - must be a power of 2 */
#define USB_RX_BUFFER_SIZE 256

/* Timeout for flushing TX buffer (roughly 100 calls to USB_task) */
#define USB_TX_FLUSH_TIMEOUT 100

/* Buffers and their management structures */
static uint8_t rx_buffer_data[USB_RX_BUFFER_SIZE];
static circular_buffer_t rx_buffer;

/* Callback state */
static usb_rx_callback_t rx_callback = NULL;
static uint32_t rx_threshold = 0;

/* TX flush timeout tracking */
static uint32_t tx_timeout_counter = 0;

/**
 * @brief Move data from TinyUSB CDC RX buffer to our circular buffer
 */
static uint32_t transfer_from_usb_to_buffer(void)
{
    uint32_t available = tud_cdc_available();
    uint32_t transferred = 0;
    uint8_t temp;

    while (available > 0 && !circular_buffer_is_full(&rx_buffer)) {
        if (tud_cdc_read(&temp, 1) == 1) {
            circular_buffer_put(&rx_buffer, temp);
            transferred++;
            available--;
        } else {
            break;
        }
    }

    return transferred;
}

/* Periodic check for callbacks */
static bool check_rx_callback(void)
{
    if (rx_callback && circular_buffer_available(&rx_buffer) >= rx_threshold) {
        rx_callback(circular_buffer_available(&rx_buffer));
        return true;
    }
    return false;
}

/**
 * @brief Initialize the internal USB buffers.
 * Called from USB_init() in h563xx/usb.c
 */
void USB_buffer_init(void)
{
    circular_buffer_init(&rx_buffer, rx_buffer_data, USB_RX_BUFFER_SIZE);
}

void USB_task(void)
{
    tud_task();

    // Transfer any available data from the USB FIFO to our circular buffer
    if (tud_cdc_available() > 0) {
        transfer_from_usb_to_buffer();
    }

    // Check for RX callbacks after processing USB tasks
    check_rx_callback();

    // Check if there's data in the TX buffer by comparing available space with total size
    if (tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE) {
        tx_timeout_counter++;
        if (tx_timeout_counter >= USB_TX_FLUSH_TIMEOUT) {
            tud_cdc_write_flush();
            tx_timeout_counter = 0;
        }
    } else {
        // No data in buffer, reset counter
        tx_timeout_counter = 0;
    }
}

bool USB_rx_ready(void)
{
    return !circular_buffer_is_empty(&rx_buffer) || tud_cdc_available() > 0;
}

uint32_t USB_rx_available(void)
{
    // Make sure we've transferred any available data to our buffer
    if (tud_cdc_available() > 0) {
        transfer_from_usb_to_buffer();
    }

    return circular_buffer_available(&rx_buffer);
}

uint32_t USB_read(uint8_t *buf, uint32_t sz)
{
    if (buf == NULL || sz == 0) {
        return 0;
    }

    // Make sure we've transferred any available data to our buffer
    if (tud_cdc_available() > 0) {
        transfer_from_usb_to_buffer();
    }

    // Read from our circular buffer using the common function
    return circular_buffer_read(&rx_buffer, buf, sz);
}

uint32_t USB_write(uint8_t const *buf, uint32_t sz)
{
    if (buf == NULL || sz == 0) {
        return 0;
    }

    // Reset the timeout counter when new data is written
    tx_timeout_counter = 0;

    return tud_cdc_write(buf, sz);
}

void USB_set_rx_callback(usb_rx_callback_t callback, uint32_t threshold)
{
    rx_callback = callback;
    rx_threshold = threshold;

    // Check if callback should be triggered immediately
    check_rx_callback();
}

uint32_t USB_tx_free_space(void)
{
    return tud_cdc_write_available();
}

bool USB_tx_busy(void)
{
    // TinyUSB doesn't have a direct way to check if TX is busy,
    // but we can check if the buffer is non-empty
    return tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE;
}
