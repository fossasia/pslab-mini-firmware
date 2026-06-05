#include "system/system.h"

#include <stdint.h>

#include "hardware/watchdog.h"
#include "pico/time.h"

#include "platform/status_led.h"
#include "platform/test_signal.h"
#include "platform/usb_cdc.h"
#include "system/bus/uart.h"
#include "util/logging.h"
#include "util/util.h"

enum {
    LOG_UART_BUS = 0,
    LOG_BUFFER_SIZE_BYTES = 1024,
    LOG_RX_BUFFER_SIZE_BYTES = 2,
};

static UART_Handle *logging_uart_handle;
static uint8_t log_tx_data[LOG_BUFFER_SIZE_BYTES];
static uint8_t log_rx_data[LOG_RX_BUFFER_SIZE_BYTES];
static CircularBuffer log_tx_buffer;
static CircularBuffer log_rx_buffer;

extern void syscalls_init(UART_Handle *handle);
extern bool syscalls_uart_flush(uint32_t timeout);

void SYSTEM_init(void)
{
    LOG_init();

    circular_buffer_init(&log_tx_buffer, log_tx_data, sizeof(log_tx_data));
    circular_buffer_init(&log_rx_buffer, log_rx_data, sizeof(log_rx_data));
    logging_uart_handle =
        UART_init(LOG_UART_BUS, &log_rx_buffer, &log_tx_buffer);
    syscalls_init(logging_uart_handle);
    LOG_task(0xFF);

    status_led_init();
    test_signal_init();
    usb_cdc_init();
}

uint32_t SYSTEM_get_tick(void)
{
    return to_ms_since_boot(get_absolute_time());
}

void SYSTEM_reset(void)
{
    LOG_INFO("Resetting...");
    LOG_task(UINT32_MAX);
    syscalls_uart_flush(1000);
    watchdog_reboot(0, 0, 0);
    while (true) {
        tight_loop_contents();
    }
}

void EXCEPTION_halt(uint32_t exception_id)
{
    LOG_ERROR(
        "FATAL: Uncaught exception 0x%08lX",
        (unsigned long)exception_id
    );
    SYSTEM_reset();
}
