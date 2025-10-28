
#include "protocol.h"
#include "system/bus/uart.h"
#include "system/esp.h"
#include "system/led.h"
#include "system/system.h"
#include "util/error.h"
#include "util/logging.h"
#include "util/util.h"

int main(void)
{
    SYSTEM_init();
    LOG_INIT("Main application");

    ESP_init();
    ESP_enter_bootloader();

    // Enable UART passthrough
    uint8_t rx1_buffer[512];
    CircularBuffer rx1_cb;
    circular_buffer_init(&rx1_cb, rx1_buffer, sizeof(rx1_buffer));
    uint8_t tx1_buffer[512];
    CircularBuffer tx1_cb;
    circular_buffer_init(&tx1_cb, tx1_buffer, sizeof(tx1_buffer));
    uint8_t rx2_buffer[512];
    CircularBuffer rx2_cb;
    circular_buffer_init(&rx2_cb, rx2_buffer, sizeof(rx2_buffer));
    uint8_t tx2_buffer[512];
    CircularBuffer tx2_cb;
    circular_buffer_init(&tx2_cb, tx2_buffer, sizeof(tx2_buffer));
    UART_Handle *uart_handle1 = UART_init(0, &rx1_cb, &tx1_cb);
    UART_Handle *uart_handle2 = UART_init(1, &rx2_cb, &tx2_cb);
    UART_enable_passthrough(uart_handle1, uart_handle2);

    // Initialize the protocol
    if (!protocol_init()) {
        LOG_ERROR("Failed to initialize protocol");
        return -1;
    }

    // Main application loop
    while (1) {
        // Process protocol tasks
        protocol_task();

        LOG_task(0xF);

        static uint32_t last_toggle = 0;
        uint32_t const blink_period = 2000; // 2 second
        if (SYSTEM_get_tick() - last_toggle >= blink_period) {
            LED_toggle(LED_GREEN);
            last_toggle = SYSTEM_get_tick();
        }
    }

    __builtin_unreachable();
}
