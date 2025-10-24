/**
 * @file esp_ll.c
 * @brief ESP32 Low-Level Operations
 *
 * This file provides functions to control the ESP32 interface pins.
 */

#include "stm32h5xx_hal.h"

#include "esp_ll.h"

void ESP_LL_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init = { 0 };

    // Configure ESP_BOOT (PB4) and ESP_EN (PB5) as output
    gpio_init.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // Configure ESP_DATA_READY (PA15) as output
    gpio_init.Pin = GPIO_PIN_15;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

void ESP_LL_set(ESP_Pin pin, bool state)
{
    GPIO_TypeDef *port = nullptr;
    uint16_t pin_mask = 0;

    switch (pin) {
    case ESP_BOOT:
        port = GPIOB;
        pin_mask = GPIO_PIN_4;
        break;
    case ESP_EN:
        port = GPIOB;
        pin_mask = GPIO_PIN_5;
        break;
    case ESP_DATA_READY:
        port = GPIOA;
        pin_mask = GPIO_PIN_15;
        break;
    default:
        return; // Invalid pin
    }

    HAL_GPIO_WritePin(port, pin_mask, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
