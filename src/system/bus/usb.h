/**
 * @file usb.h
 * @brief USB interface
 *
 * This module exposes a handle-based, CDC‐based USB API on top of the
 * TinyUSB stack. It provides a consistent interface with the UART driver
 * and allows for future expansion to support multiple USB interfaces.
 *
 * Features:
 * - Handle-based API for consistency with UART driver
 * - Non-blocking read/write operations
 * - Configurable RX callback for protocol implementations
 * - Buffer status inquiry functions
 * - Circular buffer for reliable USB data reception
 *
 * Basic Usage:
 * @code
 * // Initialize USB with handle and buffers
 * usb_handle_t *usb_handle;
 * circular_buffer_t rx_buffer;
 * uint8_t rx_data[256];
 *
 * circular_buffer_init(&rx_buffer, rx_data, 256);
 * usb_handle = USB_init(0, &rx_buffer);
 * if (usb_handle == NULL) {
 *     // Initialization failed
 *     return;
 * }
 *
 * // Main loop
 * while (1) {
 *     USB_task(usb_handle);
 *
 *     if (USB_rx_ready(usb_handle)) {
 *         uint8_t buffer[32];
 *         uint32_t bytes = USB_read(usb_handle, buffer, sizeof(buffer));
 *         USB_write(usb_handle, buffer, bytes);
 *     }
 * }
 *
 * // Cleanup when done
 * USB_deinit(usb_handle);
 * @endcode
 */
#ifndef PSLAB_USB_H
#define PSLAB_USB_H

#include "bus.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB handle structure
 */
typedef struct usb_handle_t usb_handle_t;

/**
 * @brief Callback function type for USB RX data availability.
 *
 * @param handle Pointer to USB handle structure
 * @param bytes_available Number of bytes currently available in RX buffer.
 */
typedef void (*usb_rx_callback_t)(
    usb_handle_t *handle,
    uint32_t bytes_available
);

/**
 * @brief Get the number of available USB interfaces.
 *
 * @return Number of USB interfaces supported by this platform (currently 1)
 */
size_t USB_get_interface_count(void);

/**
 * @brief Initialize USB hardware and TinyUSB driver
 *
 * Configures the USB hardware and initializes the TinyUSB stack for device
 * operation. Allocates and returns a new USB handle.
 *
 * @param interface USB interface to initialize (0-based index, currently only 0
 * is supported)
 * @param rx_buffer Pointer to pre-allocated RX circular buffer
 * @return Pointer to USB handle on success, NULL on failure (including invalid
 * interface number)
 */
usb_handle_t *USB_init(size_t interface, circular_buffer_t *rx_buffer);

/**
 * @brief Deinitialize the USB interface
 *
 * @param handle Pointer to USB handle structure
 */
void USB_deinit(usb_handle_t *handle);

/**
 * @brief Step the TinyUSB state machine
 *
 * Must be called periodically (at least once per 1 ms) to handle USB events.
 *
 * @param handle Pointer to USB handle structure
 */
void USB_task(usb_handle_t *handle);

/**
 * @brief Check if USB RX data is available
 *
 * @param handle Pointer to USB handle structure
 * @return true if data is ready to be read, false otherwise
 */
bool USB_rx_ready(usb_handle_t *handle);

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * @param handle Pointer to USB handle structure
 * @return Number of bytes available to read.
 */
uint32_t USB_rx_available(usb_handle_t *handle);

/**
 * @brief Read data from USB interface
 *
 * @param handle Pointer to USB handle structure
 * @param buf Pointer to the buffer to store received data
 * @param sz  Maximum number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t USB_read(usb_handle_t *handle, uint8_t *buf, uint32_t sz);

/**
 * @brief Write data to USB interface
 *
 * @param handle Pointer to USB handle structure
 * @param buf Pointer to the data buffer to send
 * @param sz  Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t USB_write(usb_handle_t *handle, uint8_t const *buf, uint32_t sz);

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param handle Pointer to USB handle structure
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void USB_set_rx_callback(
    usb_handle_t *handle,
    usb_rx_callback_t callback,
    uint32_t threshold
);

/**
 * @brief Get TX buffer free space.
 *
 * @param handle Pointer to USB handle structure
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t USB_tx_free_space(usb_handle_t *handle);

/**
 * @brief Check if TX transmission is in progress.
 *
 * @param handle Pointer to USB handle structure
 * @return true if transmission is ongoing, false otherwise.
 */
bool USB_tx_busy(usb_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif // PSLAB_USB_H
