#include "tusb.h"

// Device
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0xCafe,
  .idProduct          = 0x4001,
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
char const* string_desc_arr[] = {
  (const char[]){0x09, 0x04},   // English
  "TinyUSB",                    // Manufacturer
  "STM32H5 CDC Demo",           // Product
  "123456"                      // Serial
};

uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t const*)&desc_device; }
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) { (void)index; return desc_configuration; }
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  static uint16_t _desc_str[32];
  const char* str = string_desc_arr[index];
  if (!str) return NULL;
  int len = strlen(str);
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*len + 2);
  for (int i = 0; i < len; i++) _desc_str[1+i] = str[i];
  return _desc_str;
}
