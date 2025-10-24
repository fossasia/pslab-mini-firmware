/**
 * @file esp.c
 * @brief ESP32 control
 */

#include <stdint.h>

#include "platform/esp_ll.h"

#include "bus/uart.h"
#include "esp.h"

void ESP_init(void) { ESP_LL_init(); }

void ESP_reset(void)
{
    // Pull ESP_EN low for a short period
    uint32_t delay = 0xFF;
    while (delay--) {
        ESP_LL_set(ESP_EN, 0);
    }

    ESP_LL_set(ESP_EN, 1);
}

void ESP_enter_bootloader(void)
{
    // Pull down ESP_BOOT and reset
    ESP_LL_set(ESP_BOOT, 0);
    ESP_reset();
}

void ESP_exit_bootloader(void)
{
    // Release ESP_BOOT and reset
    ESP_LL_set(ESP_BOOT, 1);
    ESP_reset();
}
