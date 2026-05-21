#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void usb_cdc_init(void);
void usb_cdc_task(void);
bool usb_cdc_connected(void);
uint32_t usb_cdc_read(uint8_t *buffer, uint32_t buffer_size);
size_t usb_cdc_write(uint8_t const *data, size_t len);
void usb_cdc_flush(void);

#ifdef __cplusplus
}
#endif

#endif
