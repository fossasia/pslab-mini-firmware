/**
 * @file usb_ll.h
 * @brief Low-level USB hardware interface for PSLab
 *
 * This module provides the low-level hardware interface for the PSLab's USB
 * bus. It defines the API for initializing the USB peripheral, managing
 * instance-based operations, and implementing unique device identification
 * (UID) calculation. Each device presents a unique serial number over USB,
 * derived from the MCU's unique ID registers.
 *
 * The USB driver is built on top of the TinyUSB stack and supports multiple
 * interface instances.
 */

#ifndef PSLAB_USB_LL_H
#define PSLAB_USB_LL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define USB_UUID_LEN 12 /* 96 bits */

/**
 * @brief USB bus instance enumeration
 */
typedef enum { USB_BUS_0 = 0, USB_BUS_COUNT = 1 } USB_Bus;

/**
 * @brief USB line state change callback
 *
 * This function is called when the USB line state changes (DTR/RTS).
 *
 * @param interface_id USB bus instance
 * @param dtr Data Terminal Ready state
 * @param rts Request To Send state
 */
typedef void (*USB_LL_LineStateCallback)(USB_Bus const interface_id, bool dtr, bool rts);

/**
 * @brief Initialize the USB hardware and TinyUSB stack
 *
 * Sets up the MCU's USB hardware and initializes the TinyUSB stack for device
 * operation.
 *
 * @param bus USB bus instance to initialize
 */
void USB_LL_init(USB_Bus bus);

/**
 * @brief Deinitialize the USB peripheral
 *
 * @param bus USB bus instance to deinitialize
 */
void USB_LL_deinit(USB_Bus bus);

/**
 * @brief Retrieve the unique device serial as a USB string descriptor
 *
 * Reads the MCU's unique ID and converts it into an uppercase hexadecimal
 * UTF-16LE string suitable for use as a USB serial number descriptor.
 *
 * @param desc_str1 Buffer to receive the UTF-16LE encoded serial characters
 * @param max_chars Maximum number of UTF-16 characters the buffer can hold
 * @return Number of UTF-16 characters written into desc_str1
 */
size_t USB_LL_get_serial(uint16_t desc_str1[], size_t max_chars);

/**
 * @brief Get number of bytes available for reading from USB CDC interface
 *
 * @param interface_id USB CDC interface instance
 * @return Number of bytes available in the RX buffer
 */
uint32_t USB_LL_rx_available(USB_Bus interface_id);

/**
 * @brief Get number of bytes available for writing to USB CDC interface
 *
 * @param interface_id USB CDC interface instance
 * @return Number of bytes that can be written to the TX buffer
 */
uint32_t USB_LL_tx_available(USB_Bus interface_id);

/**
 * @brief Read data from USB CDC interface
 *
 * @param interface_id USB CDC interface instance
 * @param buf Buffer to store the read data
 * @param bufsize Maximum number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t USB_LL_read(USB_Bus interface_id, uint8_t *buf, uint32_t bufsize);

/**
 * @brief Write data to USB CDC interface
 *
 * @param interface_id USB CDC interface instance
 * @param buf Buffer containing data to write
 * @param bufsize Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t USB_LL_write(
    USB_Bus interface_id,
    uint8_t const *buf,
    uint32_t bufsize
);

/**
 * @brief Get the size of the USB CDC transmit buffer
 *
 * @param interface_id USB CDC interface instance
 * @return Size of the TX buffer in bytes
 */
uint32_t USB_LL_tx_bufsize(USB_Bus interface_id);

/**
 * @brief Process USB tasks and handle pending operations
 *
 * This function should be called periodically to handle USB operations
 * and maintain communication with the host.
 *
 * @param interface_id USB CDC interface instance
 */
void USB_LL_task(USB_Bus interface_id);

/**
 * @brief Check if USB CDC interface is connected to host
 *
 * @param interface_id USB CDC interface instance
 * @return true if connected to host, false otherwise
 */
bool USB_LL_connected(USB_Bus interface_id);

/**
 * @brief Flush the USB CDC transmit buffer
 *
 * Forces any buffered transmit data to be sent to the host immediately.
 *
 * @param interface_id USB CDC interface instance
 * @return Number of bytes flushed
 */
uint32_t USB_LL_tx_flush(USB_Bus interface_id);

/**
 * @brief Set the USB line state change callback
 *
 * This function registers a callback to be called when the USB line state
 * changes (DTR/RTS).
 *
 * @param interface_id USB bus instance
 * @param callback Callback function to be called on line state change
 */
void USB_LL_set_line_state_callback(USB_Bus const interface_id, USB_LL_LineStateCallback callback);

#endif /* PSLAB_USB_LL_H */
