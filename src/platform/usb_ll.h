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
typedef enum { USB_BUS_0 = 0, USB_BUS_COUNT = 1 } usb_bus_t;

/**
 * @brief Initialize the USB hardware and TinyUSB stack
 *
 * Sets up the MCU's USB hardware and initializes the TinyUSB stack for device
 * operation.
 *
 * @param bus USB bus instance to initialize
 */
void USB_LL_init(usb_bus_t bus);

/**
 * @brief Deinitialize the USB peripheral
 *
 * @param bus USB bus instance to deinitialize
 */
void USB_LL_deinit(usb_bus_t bus);

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

#endif /* PSLAB_USB_LL_H */
