
#include "protocol.h"
#include "system/led.h"
#include "system/system.h"
#include "util/error.h"
#include "util/logging.h"

int main(void)
{
    SYSTEM_init();
    LOG_INIT("Main application");

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
