/**
 * @file usb.h
 * @brief USB interface
 *
 * This module exposes a simple, CDC‐based USB API on top of the
 * TinyUSB stack. It allows user applications to:
 *   - Initialize USB hardware and driver
 *   - Periodically poll USB tasks to maintain the USB state machine
 *   - Check for incoming data, read from and write to the USB CDC interface
 *   - Flush any pending transmit data to or from the host
 */
#ifndef PSLAB_USB_H
#define PSLAB_USB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Callback function type for USB RX data availability.
 *
 * @param bytes_available Number of bytes currently available in RX buffer.
 */
typedef void (*usb_rx_callback_t)(uint32_t bytes_available);

/**
 * @brief Initialize USB hardware and TinyUSB driver
 */
void USB_init(void);

/**
 * @brief Step the TinyUSB state machine
 *
 * Must be called periodically (at least once per 1 ms) to handle USB events.
 */
void USB_task(void);

/**
 * @brief Check if USB RX data is available
 *
 * @return true if data is ready to be read, false otherwise
 */
bool USB_rx_ready(void);

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * @return Number of bytes available to read.
 */
uint32_t USB_rx_available(void);

/**
 * @brief Read data from USB interface
 *
 * @param buf Pointer to the buffer to store received data
 * @param sz  Maximum number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t USB_read(uint8_t *buf, uint32_t sz);

/**
 * @brief Write data to USB interface
 *
 * @param buf Pointer to the data buffer to send
 * @param sz  Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t USB_write(uint8_t const *buf, uint32_t sz);

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void USB_set_rx_callback(usb_rx_callback_t callback, uint32_t threshold);

/**
 * @brief Get TX buffer free space.
 *
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t USB_tx_free_space(void);

/**
 * @brief Check if TX transmission is in progress.
 *
 * @return true if transmission is ongoing, false otherwise.
 */
bool USB_tx_busy(void);

#endif // PSLAB_USB_H
