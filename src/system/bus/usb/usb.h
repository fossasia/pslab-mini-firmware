/**
 * @file usb.h
 * @brief USB interface
 *
 * This module exposes a simple, CDC‐based USB API on top of the
 * TinyUSB stack. It allows user applications to:
 *   - Initialize USB hardware and driver
 *   - Periodically poll USB tasks to maintain the USB state machine
 *   - Check for incoming data, read from and write to the USB CDC interface
 *   - Flush any pending transmit data to the host
 */
#ifndef PSLAB_USB_H
#define PSLAB_USB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize USB hardware and TinyUSB driver.
 */
void USB_init(void);

/**
 * @brief Poll TinyUSB device tasks.
 *
 * Must be called periodically from the main loop to handle
 * USB events and maintain the USB state machine.
 */
void USB_task(void);

/**
 * @brief Check if USB RX data is available.
 *
 * @return true if data is ready to be read, false otherwise.
 */
bool USB_rx_ready(void);

/**
 * @brief Read data from USB interface.
 *
 * @param buf Pointer to the buffer to store received data.
 * @param sz  Maximum number of bytes to read.
 * @return Number of bytes actually read.
 */
uint32_t USB_read(uint8_t *buf, uint32_t sz);

/**
 * @brief Write data to USB interface.
 *
 * @param buf Pointer to the data buffer to send.
 * @param sz  Number of bytes to write.
 * @return Number of bytes actually written.
 */
uint32_t USB_write(uint8_t *buf, uint32_t sz);

/**
 * @brief Retrieve the unique device serial as a USB string descriptor.
 *
 * Reads the MCU's unique ID and converts it into an uppercase
 * hexadecimal UTF-16LE string suitable for use as a USB serial
 * number descriptor.
 *
 * @param desc_str1  Buffer to receive the UTF-16LE encoded serial characters.
 * @param max_chars  Maximum number of UTF-16 characters the buffer can hold.
 * @return Number of UTF-16 characters written into desc_str1.
 */
size_t USB_get_serial(uint16_t desc_str1[], size_t max_chars);

/**
 * @brief Flush any pending USB TX data.
 *
 * @return Number of bytes that were flushed.
 */
uint32_t USB_tx_flush(void);

#endif // PSLAB_USB_H
