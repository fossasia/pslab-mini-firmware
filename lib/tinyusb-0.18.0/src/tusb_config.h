#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_MCU            OPT_MCU_STM32H5
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE

// Enable only CDC (virtual COM port)
#define CFG_TUD_CDC             1
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_VENDOR          0

#define CFG_TUD_ENDPOINT0_SIZE  64
#define CFG_TUD_CDC_RX_BUFSIZE  64
#define CFG_TUD_CDC_TX_BUFSIZE  64

#endif  // TUSB_CONFIG_H
