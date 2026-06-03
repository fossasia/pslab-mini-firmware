#include "platform/usb_cdc.h"

#include "bsp/board_api.h"
#include "pico/time.h"
#include "tusb.h"

void usb_cdc_init(void)
{
    board_init();
    tusb_init();

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }
}

void usb_cdc_task(void) { tud_task(); }

bool usb_cdc_connected(void) { return tud_cdc_connected(); }

uint32_t usb_cdc_read(uint8_t *buffer, uint32_t buffer_size)
{
    if (!buffer || buffer_size == 0 || !tud_cdc_available()) {
        return 0;
    }

    return tud_cdc_read(buffer, buffer_size);
}

size_t usb_cdc_write(uint8_t const *data, size_t len)
{
    size_t written = 0;
    absolute_time_t deadline = make_timeout_time_ms(1000);

    while (written < len && !time_reached(deadline)) {
        tud_task();

        if (!tud_cdc_connected()) {
            sleep_ms(1);
            continue;
        }

        uint32_t available = tud_cdc_write_available();
        if (available == 0) {
            tud_cdc_write_flush();
            sleep_ms(1);
            continue;
        }

        uint32_t chunk = (uint32_t)(len - written);
        if (chunk > available) {
            chunk = available;
        }

        written += tud_cdc_write(data + written, chunk);
    }

    tud_cdc_write_flush();
    return written;
}

void usb_cdc_flush(void) { tud_cdc_write_flush(); }
