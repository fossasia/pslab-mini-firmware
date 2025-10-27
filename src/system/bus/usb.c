/**
 * @file usb.c
 * @brief Hardware-independent USB interface implementation for TinyUSB
 *
 * This module provides a handle-based CDC USB API on top of the TinyUSB stack,
 * mirroring the UART API functionality to provide a consistent interface
 * for different communication methods. While the current hardware only supports
 * a single USB interface, the handle-based design keeps the API consistent
 * with UART and makes it future-proof for potential multi-interface support.
 *
 * Features:
 * - Handle-based API for consistency with UART driver
 * - Non-blocking read/write operations
 * - Configurable RX callback for protocol implementations
 * - Buffer status inquiry functions
 * - Circular buffers for reliable USB data reception and transmission
 *
 * @author Alexander Bessman
 * @date 2025-07-02
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "platform/usb_ll.h"
#include "util/error.h"
#include "util/util.h"

#include "usb.h"

/* Maximum number of USB interfaces */
#define USB_INTERFACE_COUNT USB_BUS_COUNT

/* Timeout for flushing TX buffer (roughly 100 calls to USB_task) */
enum { USB_TX_FLUSH_TIMEOUT = 100 };

/**
 * @brief USB interface handle structure
 */
struct USB_Handle {
    uint8_t interface_id;
    CircularBuffer *rx_buffer;
    CircularBuffer *tx_buffer;
    USB_RxCallback rx_callback;
    uint32_t rx_threshold;
    uint32_t tx_timeout_counter;
    bool initialized;
};

/* Global array to keep track of active USB handles */
static USB_Handle *g_active_handles[USB_INTERFACE_COUNT] = { nullptr };

/**
 * @brief Get the number of available USB interfaces.
 *
 * @return Number of USB interfaces supported by this platform
 */
size_t USB_get_interface_count(void) { return USB_INTERFACE_COUNT; }

/**
 * @brief Get USB handle from interface ID
 *
 * @param interface_id USB interface ID
 * @return Pointer to handle or nullptr if not found
 */
static USB_Handle *get_handle_from_interface(uint8_t interface_id)
{
    if (interface_id >= USB_INTERFACE_COUNT) {
        return nullptr;
    }
    return g_active_handles[interface_id];
}

/**
 * @brief Move data from TinyUSB CDC RX buffer to our circular buffer
 *
 * @param handle Pointer to USB handle structure
 *
 * @return Number of bytes transferred
 */
static uint32_t transfer_rx(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    uint32_t available = USB_LL_rx_available(handle->interface_id);
    uint32_t transferred = 0;
    uint8_t temp = 0;

    while (available > 0 && !circular_buffer_is_full(handle->rx_buffer)) {
        if (USB_LL_read(handle->interface_id, &temp, 1) == 1) {
            circular_buffer_put(handle->rx_buffer, temp);
            transferred++;
            available--;
        } else {
            break;
        }
    }

    return transferred;
}

/**
 * @brief Move data from TX circular buffer to USB TX endpoint
 *
 * @param handle Pointer to USB handle structure
 *
 * @return Number of bytes transferred
 */
static uint32_t transfer_tx(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    uint32_t to_send = circular_buffer_available(handle->tx_buffer);
    uint32_t transferred = 0;

    while (to_send > 0 && USB_LL_tx_available(handle->interface_id)) {
        uint8_t temp = 0;
        circular_buffer_get(handle->tx_buffer, &temp);
        if (USB_LL_write(handle->interface_id, &temp, 1) == 1) {
            transferred++;
            to_send--;
        } else {
            break;
        }
    }

    return transferred;
}

/**
 * @brief Check if RX callback condition is met and call if needed
 *
 * @param handle Pointer to USB handle structure
 * @return true if condition was met and callback called, false otherwise
 */
static bool check_rx_callback(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    if (!handle->rx_callback) {
        return false;
    }

    if (circular_buffer_available(handle->rx_buffer) < handle->rx_threshold) {
        return false;
    }

    handle->rx_callback(handle, circular_buffer_available(handle->rx_buffer));
    return true;
}

/**
 * @brief Act on USB line state change
 *
 * This function is called by the backend USB stack when the USB CDC line state
 * changes. It detects when DTR (Data Terminal Ready) is de-asserted, which
 * indicates the host has disconnected, and clears the local buffers to prevent
 * stale data.
 *
 * @param itf USB interface number
 * @param dtr Data Terminal Ready state
 * @param rts Request To Send state
 */
static void line_state_callback(USB_Bus itf, bool dtr, bool rts)
{
    (void)rts; // We're only concerned with DTR

    USB_Handle *handle = get_handle_from_interface(itf);
    if (!handle || !handle->initialized) {
        return;
    }

    // When DTR is de-asserted (goes low), the host has disconnected
    if (!dtr) {
        // Reset the circular buffers to clear any pending data
        circular_buffer_reset(handle->rx_buffer);
        circular_buffer_reset(handle->tx_buffer);

        // Reset the TX timeout counter
        handle->tx_timeout_counter = 0;
    }
}

/**
 * @brief Initialize USB hardware and TinyUSB driver
 *
 * Configures the USB hardware and initializes the TinyUSB stack for device
 * operation. Allocates and returns a new USB handle.
 *
 * @param interface USB interface to initialize (0-based index)
 * @param rx_buffer Pointer to pre-allocated RX circular buffer
 * @param tx_buffer Pointer to pre-allocated TX circular buffer
 * @return Pointer to USB handle on success, nullptr on failure
 */
USB_Handle *USB_init(
    size_t interface,
    CircularBuffer *rx_buffer,
    CircularBuffer *tx_buffer
)
{
    if (!rx_buffer || !tx_buffer || interface >= USB_INTERFACE_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    uint8_t interface_id = (uint8_t)interface;

    /* Check if interface is already initialized */
    if (g_active_handles[interface_id] != nullptr) {
        THROW(ERROR_RESOURCE_BUSY);
    }

    /* Allocate handle */
    USB_Handle *handle = malloc(sizeof(USB_Handle));
    if (!handle) {
        THROW(ERROR_OUT_OF_MEMORY);
    }

    /* Initialize handle */
    handle->interface_id = interface_id;
    handle->rx_buffer = rx_buffer;
    handle->tx_buffer = tx_buffer;
    handle->rx_callback = nullptr;
    handle->rx_threshold = 0;
    handle->tx_timeout_counter = 0;
    handle->initialized = true;

    /* Store handle in global array */
    g_active_handles[interface_id] = handle;

    USB_LL_init((USB_Bus)interface_id);
    USB_LL_set_line_state_callback((USB_Bus)interface_id, line_state_callback);

    return handle;
}

/**
 * @brief Deinitialize the USB interface
 *
 * @param handle Pointer to USB handle structure
 */
void USB_deinit(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }

    /* Deinitialize USB hardware if this is interface 0 */
    if (handle->interface_id == 0) {
        USB_LL_deinit((USB_Bus)handle->interface_id);
    }

    /* Clear from global array */
    if (handle->interface_id < USB_INTERFACE_COUNT) {
        g_active_handles[handle->interface_id] = nullptr;
    }

    /* Mark as uninitialized */
    handle->initialized = false;

    /* Free handle */
    free(handle);
}

/**
 * @brief Step the TinyUSB state machine
 *
 * Must be called periodically (at least once per 1 ms) to handle USB events.
 *
 * @param handle Pointer to USB handle structure
 */
void USB_task(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }

    USB_LL_task(handle->interface_id);

    // Nothing to do if not connected
    if (!USB_LL_connected(handle->interface_id)) {
        return;
    }

    // Transfer any available data from the USB FIFO to our circular buffer
    if (USB_LL_rx_available(handle->interface_id) > 0) {
        transfer_rx(handle);
    }

    // Check for RX callbacks after processing USB tasks
    check_rx_callback(handle);

    // Transfer data from our TX buffer to the USB hardware
    if (!circular_buffer_is_empty(handle->tx_buffer)) {
        transfer_tx(handle);
    }

    // Check if there's data in the hardware TX buffer and flush if timeout
    if (USB_LL_tx_available(handle->interface_id) <
        USB_LL_tx_bufsize(handle->interface_id)) {
        handle->tx_timeout_counter++;
        if (handle->tx_timeout_counter >= USB_TX_FLUSH_TIMEOUT) {
            USB_LL_tx_flush(handle->interface_id);
            handle->tx_timeout_counter = 0;
        }
    } else {
        // No data in hardware buffer, reset counter if our buffer is also empty
        if (circular_buffer_is_empty(handle->tx_buffer)) {
            handle->tx_timeout_counter = 0;
        }
    }
}

/**
 * @brief Check if USB RX data is available
 *
 * @param handle Pointer to USB handle structure
 * @return true if data is ready to be read, false otherwise
 */
bool USB_rx_ready(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }
    return (!circular_buffer_is_empty(handle->rx_buffer) ||
            USB_LL_rx_available(handle->interface_id) > 0) != 0;
}

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * @param handle Pointer to USB handle structure
 * @return Number of bytes available to read.
 */
uint32_t USB_rx_available(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    // Make sure we've transferred any available data to our buffer
    if (USB_LL_rx_available(handle->interface_id) > 0) {
        transfer_rx(handle);
    }

    return circular_buffer_available(handle->rx_buffer);
}

/**
 * @brief Read data from USB interface
 *
 * @param handle Pointer to USB handle structure
 * @param buf Pointer to the buffer to store received data
 * @param sz  Maximum number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t USB_read(USB_Handle *handle, uint8_t *buf, uint32_t sz)
{
    if (!handle || !handle->initialized || buf == nullptr || sz == 0) {
        return 0;
    }

    // Make sure we've transferred any available data to our buffer
    if (USB_LL_rx_available(handle->interface_id) > 0) {
        transfer_rx(handle);
    }

    // Read from our circular buffer using the common function
    return circular_buffer_read(handle->rx_buffer, buf, sz);
}

/**
 * @brief Write data to USB interface
 *
 * @param handle Pointer to USB handle structure
 * @param buf Pointer to the data buffer to send
 * @param sz  Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t USB_write(USB_Handle *handle, uint8_t const *buf, uint32_t sz)
{
    if (!handle || !handle->initialized || buf == nullptr || sz == 0) {
        return 0;
    }

    uint32_t const written = circular_buffer_write(handle->tx_buffer, buf, sz);

    // Move as much data as possible to the USB hardware
    if (circular_buffer_is_empty(handle->tx_buffer) &&
        USB_LL_tx_available(handle->interface_id)) {
        transfer_tx(handle);
    }

    return written;
}

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param handle Pointer to USB handle structure
 * @param callback Function to call when threshold is reached (nullptr to
 *                 disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void USB_set_rx_callback(
    USB_Handle *handle,
    USB_RxCallback callback,
    uint32_t threshold
)
{
    if (!handle || !handle->initialized) {
        return;
    }

    handle->rx_callback = callback;
    handle->rx_threshold = threshold;

    // Check if callback should be triggered immediately
    check_rx_callback(handle);
}

/**
 * @brief Get TX buffer free space.
 *
 * @param handle Pointer to USB handle structure
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t USB_tx_free_space(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    // Move as much data as possible to the USB hardware
    if (circular_buffer_is_empty(handle->tx_buffer) &&
        USB_LL_tx_available(handle->interface_id)) {
        transfer_tx(handle);
    }

    return circular_buffer_free_space(handle->tx_buffer);
}

/**
 * @brief Check if TX transmission is in progress.
 *
 * @param handle Pointer to USB handle structure
 * @return true if transmission is ongoing, false otherwise.
 */
bool USB_tx_busy(USB_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }
    // Check if either our TX buffer or the hardware TX buffer has data
    return !circular_buffer_is_empty(handle->tx_buffer) ||
           (USB_LL_tx_available(handle->interface_id) <
            USB_LL_tx_bufsize(handle->interface_id));
}
