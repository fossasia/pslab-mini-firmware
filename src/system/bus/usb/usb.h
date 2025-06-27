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

#include "tusb.h"

#ifdef STM32H563xx
#define USB_UUID_LEN 12 /* 96 bits = 12 bytes */
#endif // STM32H563xx

/**
 * @brief Initialize USB hardware and TinyUSB driver
 */
void USB_init(void);

/**
 * @brief Step the TinyUSB state machine
 *
 * Must be called periodically (at least once per 1 ms) to handle USB events.
 */
static inline void USB_task(void)
{
    tud_task();
}

/**
 * @brief Check if USB RX data is available
 *
 * @return true if data is ready to be read, false otherwise
 */
static inline bool USB_rx_ready(void)
{
    return tud_cdc_available();
}

/**
 * @brief Read data from USB interface
 *
 * @param buf Pointer to the buffer to store received data
 * @param sz  Maximum number of bytes to read
 * @return Number of bytes actually read
 */
static inline uint32_t USB_read(uint8_t *buf, uint32_t sz)
{
    return tud_cdc_read(buf, sz);
}

/**
 * @brief Write data to USB interface
 *
 * @param buf Pointer to the data buffer to send
 * @param sz  Number of bytes to write
 * @return Number of bytes actually written
 */
static inline uint32_t USB_write(uint8_t *buf, uint32_t sz)
{
    return tud_cdc_write(buf, sz);
}

/**
 * @brief Flush the USB RX buffer
 *
 * This function clears any pending data in the USB RX buffer.
 */
static inline void USB_rx_flush(void)
{
    tud_cdc_read_flush();
}

/**
 * @brief Flush any pending USB TX data
 *
 * @return Number of bytes that were flushed
 */
static inline uint32_t USB_tx_flush(void)
{
    return tud_cdc_write_flush();
}

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
size_t USB_get_serial(uint16_t desc_str1[], size_t max_chars);

#endif // PSLAB_USB_H
