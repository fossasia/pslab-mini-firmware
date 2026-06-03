#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"

#define USB_VID 0xCafe
#define USB_PID 0x4010
#define USB_BCD 0x0200

tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL,
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),
    TUD_CDC_DESCRIPTOR(
        ITF_NUM_CDC,
        4,
        EPNUM_CDC_NOTIF,
        8,
        EPNUM_CDC_OUT,
        EPNUM_CDC_IN,
        CFG_TUD_CDC_EP_BUFSIZE
    ),
};

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC,
};

static char const *const string_desc_arr[] = {
    (char const[]){0x09, 0x04},
    "PSLab Pico",
    "PSLab Pico",
    NULL,
    "SCPI CDC",
};

static uint16_t desc_str[33];

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    size_t char_count;
    if (index == STRID_LANGID) {
        memcpy(&desc_str[1], string_desc_arr[STRID_LANGID], 2);
        char_count = 1;
    } else if (index == STRID_SERIAL) {
        char_count = board_usb_get_serial(desc_str + 1, 32);
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL;
        }

        char const *str = string_desc_arr[index];
        char_count = strlen(str);
        if (char_count > 32) {
            char_count = 32;
        }

        for (size_t i = 0; i < char_count; ++i) {
            desc_str[1 + i] = str[i];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * char_count + 2));
    return desc_str;
}
