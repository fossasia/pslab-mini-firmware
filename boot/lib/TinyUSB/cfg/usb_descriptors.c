/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "stm32h5xx.h"
#include "stm32h5xx_ll_icache.h"


#define USB_VID   0x1D50  // OpenMoko
#define USB_PID   0x60AC  // OpenBLT Bootloader
#if (CFG_TUD_WINUSB_ENABLED == 1)
#define USB_BCD   0x0201  // Needs to be >= 2.01 for BOS
#else
#define USB_BCD   0x0200
#endif

//--------------------------------------------------------------------+
// Unique ID string
//--------------------------------------------------------------------+
// Length of the unique ID in bytes. On the STM32H5, the unique
// identifier is 96 bits.
#define UID_STR_BYTE_LEN          (96U/8U)
#define UID_STR_WORD_LEN          (UID_STR_BYTE_LEN / sizeof(uint32_t))

// NUL terminated character array for storing the device's unique
// identifier. Callback function tud_descriptor_string_cb() overwrites
// its default value with the actual device's unique identifier.
static char uniqueIdStr[(UID_STR_BYTE_LEN * 2U) + 1U] = "0123456789ABCDEF";

// Initializes the string that holds the ASCII representation of the device's
// unique identifier.
static void UniqueIdStrInit(void)
{
  uint8_t        charIdx = 0U;
  static uint8_t initialized = 0U;
  bool           icacheWasEnabled = false;
  uint32_t       icacheMode = 0U;
  struct
  {
    union
    {
      uint8_t b[UID_STR_BYTE_LEN];
      uint32_t w[UID_STR_WORD_LEN];
    };
  } uidData;

  // Only initialize once.
  if (initialized == 0U)
  {
    // Make sure to disable the instruction cache before accessing UID_BASE. Otherwise
    // you get a hard fault.
    if (LL_ICACHE_IsEnabled() != 0U)
    {
      // Set flag to remember that the instruction cache was enabled.
      icacheWasEnabled = true;
      // Store the instruction cache mode.
      icacheMode = LL_ICACHE_GetMode();
      // Disable the instruction cache.
      LL_ICACHE_Disable();
    }
    // Copy the unique device ID into the local structure.
    uidData.w[0] = *(uint32_t volatile *)(UID_BASE);
    uidData.w[1] = *(uint32_t volatile *)(UID_BASE + 0x04U);
    uidData.w[2] = *(uint32_t volatile *)(UID_BASE + 0x08U);
    // Restore the instruction cache state after accessing UID_BASE.
    if (icacheWasEnabled == true)
    {
      LL_ICACHE_SetMode(icacheMode);
      LL_ICACHE_Enable();
    }
    // Update flag.
    initialized = 1U;
    // Loop through all unique identifier bytes.
    for (uint8_t idx = 0U; idx < UID_STR_BYTE_LEN; idx++)
    {
      // Convert byte value to nibbles.
      uint8_t nibbles[2];
      nibbles[0] = (uidData.b[idx] >> 4U) & 0x0F;
      nibbles[1] = (uidData.b[idx]) & 0x0F;
      // Loop through the nibbles.
      for (uint8_t n = 0U; n < sizeof(nibbles); n++)
      {
        // Convert the nibble to an ASCII character.
        char nibbleChr = (nibbles[n] < 10U) ? nibbles[n] + '0' : nibbles[n] - 10 + 'A';
        // Store the ASCII character in the string.
        uniqueIdStr[charIdx] = nibbleChr;
        // Update indexer for the next character.
        charIdx++;
      }
    }
    // Add NUL termination.
    uniqueIdStr[charIdx] = '\0';
  }
}

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,

  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = 0x0100,

  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,

  .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_VENDOR,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

#define EPNUM_VENDOR_OUT   0x01
#define EPNUM_VENDOR_IN    0x81

uint8_t const desc_fs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 150),

  // Interface number, string index, EP Out & EP In address, EP size
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations

  return desc_fs_configuration;
}

#if (CFG_TUD_WINUSB_ENABLED == 1)
//--------------------------------------------------------------------+
// BOS Descriptor
//--------------------------------------------------------------------+

/* Microsoft OS 2.0 registry property descriptor per MS requirements:
 *   https://msdn.microsoft.com/en-us/library/windows/hardware/hh450799(v=vs.85).aspx
 * device should create DeviceInterfaceGUID. It can be done by driver and
 * in case of real PnP solution device should expose MS "Microsoft OS 2.0
 * registry property descriptor". Such descriptor can insert any record
 * into Windows registry per device/configuration/interface. In our case it
 * will insert "DeviceInterfaceGUID" string property.
 */

#define BOS_TOTAL_LEN             (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

#define MS_OS_20_DESC_LEN         0x9E

#define VENDOR_REQUEST_MICROSOFT  1

// BOS Descriptor is required for MS OS 2.0.
uint8_t const desc_bos[] =
{
  // total length, number of device caps
  TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),

  // Microsoft OS 2.0 descriptor
  TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT)
};

uint8_t const * tud_descriptor_bos_cb(void)
{
  return desc_bos;
}

uint8_t const desc_ms_os_20[] =
{
  // Set header: length, type, windows version, total length
  U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

  // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

  // MS OS 2.0 Registry property descriptor: length, type
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN-0x0A-0x14), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
  U16_TO_U8S_LE(0x0001), U16_TO_U8S_LE(0x0028), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUID\0" in UTF-16
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
  'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00,

  U16_TO_U8S_LE(0x004E), // wPropertyDataLength
  //bPropertyData: "{807999C3-E4E0-40EA-8188-48E852B54F2B}\0"
  '{', 0x00, '8', 0x00, '0', 0x00, '7', 0x00, '9', 0x00, '9', 0x00, '9', 0x00, 'C', 0x00, '3', 0x00, '-', 0x00,
  'E', 0x00, '4', 0x00, 'E', 0x00, '0', 0x00, '-', 0x00, '4', 0x00, '0', 0x00, 'E', 0x00, 'A', 0x00, '-', 0x00,
  '8', 0x00, '1', 0x00, '8', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '8', 0x00, 'E', 0x00, '8', 0x00, '5', 0x00,
  '2', 0x00, 'B', 0x00, '5', 0x00, '4', 0x00, 'F', 0x00, '2', 0x00, 'B', 0x00, '}', 0x00, 0x00, 0x00
};

TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "Incorrect size");

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP) return true;

  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_VENDOR:
      switch (request->bRequest)
      {
        case VENDOR_REQUEST_MICROSOFT:
          if ( request->wIndex == 7 )
          {
            // Get Microsoft OS 2.0 compatible descriptor
            uint16_t total_len;
            memcpy(&total_len, desc_ms_os_20+8, 2);

            return tud_control_xfer(rhport, request, (void*)(uintptr_t) desc_ms_os_20, total_len);
          }else
          {
            return false;
          }

        default:
         break;
      }
    break;

    default: 
      break;
  }

  // stall unknown request
  return false;
}
#endif /* (CFG_TUD_WINUSB_ENABLED == 1) */

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "OpenBLT User",                // 1: Manufacturer
  "WinUSB Bulk Device",          // 2: Product
  uniqueIdStr,                   // 3: Serials, use chip ID
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  // Make sure the unique identifier string is initialized.
  UniqueIdStrInit();

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = (uint8_t) strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

  return _desc_str;
}
