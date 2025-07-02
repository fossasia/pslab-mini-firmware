/**
 * @file usb_descriptors.c
 * @brief USB descriptor definitions and TinyUSB callbacks.
 *
 * USB descriptors are data structures used by USB devices to describe their
 * identity, capabilities, and configuration to the host system. They include
 * information such as device class, vendor and product IDs, supported
 * configurations, and human-readable strings.
 *
 * This file defines the USB device, configuration, and string descriptors for
 * the device, and implements TinyUSB callbacks to integrate with the USB
 * stack.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"

#include "usb_ll.h"

#define LANG ((char const[]){0x09, 0x04}) // English
#define MANU ("FOSSASIA")
#define PROD ("Pocket Science Lab")
#define SERI (NULL) // Unique identifier, calculated at runtime from MCU UID.

enum {
    IDX_LANG,
    IDX_MANU,
    IDX_PROD,
    IDX_SERI,
    IDX_TOT
};

// Device
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xcafe,
    .idProduct          = 0x1234,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// Configuration + CDC
uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(0, 4, 0x81, 8, 0x01, 0x82, 64)
};

// String descriptors
char const *string_desc_arr[] = {
    LANG,
    MANU,
    PROD,
    SERI
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&desc_device; }
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) { (void)index; return desc_configuration; }
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    if (index >= IDX_TOT) {return NULL;}

    (void)langid;
    static uint16_t _desc_str[32 + 1];
    #define MAX_DESC_LEN (sizeof(_desc_str) / sizeof(_desc_str[0]) - 1)

    static_assert(sizeof(LANG) == 2, "LANG id must be exactly two bytes");
    static_assert(sizeof(MANU) <= MAX_DESC_LEN, "MANU str too long");
    static_assert(sizeof(PROD) <= MAX_DESC_LEN, "PROD str too long");
    static_assert(USB_UUID_LEN <= MAX_DESC_LEN, "SERI str too long");

    int len;

    switch (index) {
    case IDX_SERI:
        len = USB_LL_get_serial(_desc_str + 1, MAX_DESC_LEN);
        break;
    case IDX_LANG:
        len = 1;
        memcpy(_desc_str + 1, string_desc_arr[index], 2 * len);
        break;
    default:
        char const *str = string_desc_arr[index];
        len = strlen(str);
        // Convert ASCII string into UTF-16
        for (size_t i = 0; i < len; ++i) {
            _desc_str[1 + i] = str[i];
        }
        break;
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);

    return _desc_str;
}
