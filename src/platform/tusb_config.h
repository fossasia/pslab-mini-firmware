#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUD_ENABLED (1)
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT (0)
#endif

#define CFG_TUD_CDC (1)
#define CFG_TUD_CDC_RX_BUFSIZE (256)
#define CFG_TUD_CDC_TX_BUFSIZE (256)
#define CFG_TUD_CDC_EP_BUFSIZE (64)

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE (64)
#endif

#ifdef __cplusplus
}
#endif

#endif
