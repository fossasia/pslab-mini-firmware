#include "protocol.h"
#include "system/led.h"
#include "system/bus/spi.h"
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

    // Initialize the SPI bus
    SPI_Handle *spi_handle = SPI_init(0);
    if (spi_handle == NULL) {
        LOG_ERROR("Failed to initialize SPI bus");
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

        // SPI communication example
        uint8_t tx_data = 0x9F;
        uint8_t rx_data[5] = {0};
        SPI_transmit_receive(spi_handle, &tx_data, rx_data, sizeof(rx_data));
        LOG_INFO("SPI Received: %02X %02X %02X %02X %02X",
            rx_data[0], rx_data[1], rx_data[2], rx_data[3], rx_data[4]);
    }

    __builtin_unreachable();
}
