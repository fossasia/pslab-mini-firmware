/**
 * @file led_ll.c
 * @brief Low-level LED control implementation for PSLab Mini (STM32H563xx)
 *
 * This module provides a low-level interface to control the PSLab's onboard
 * LEDs.
 *
 * Hardware Implementation Details:
 * - Uses STM32H5xx HAL library for GPIO operations
 * - LED_GREEN (LED1) is connected to GPIOB Pin 12 (PB12)
 * - Configured as push-pull output with no pull-up/pull-down resistors
 * - Low frequency GPIO speed setting for power efficiency
 * - Requires GPIOB clock to be enabled
 *
 * @author PSLab Team
 * @date 2025
 */

#include "stm32h5xx_hal.h"

#include "led_ll.h"

#define LED_GREEN_GPIO_GROUP GPIOB
#define LED_GREEN_GPIO_PIN GPIO_PIN_12

typedef struct {
    GPIO_TypeDef *gpio_group;
    uint16_t gpio_pin;
} LEDConfig;

static LEDConfig const g_LED_CONFIGS[LED_LL_COUNT] = {
    [LED_LL_GREEN] = { LED_GREEN_GPIO_GROUP, LED_GREEN_GPIO_PIN },
};

/**
 * @brief Initialize the LED hardware
 *
 * Configures all LED GPIO pins as outputs and enables the necessary GPIO
 * clocks. Sets up all pins as push-pull outputs with low frequency speed for
 * optimal power consumption.
 */
void LED_LL_init(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    /* Enable GPIO port clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure common GPIO settings */
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Pin = g_LED_CONFIGS[0].gpio_pin;
    HAL_GPIO_Init(g_LED_CONFIGS[0].gpio_group, &gpio_init);
}

void LED_LL_on(LED_LL_ID led_id)
{
    if (led_id < LED_LL_COUNT) {
        HAL_GPIO_WritePin(
            g_LED_CONFIGS[led_id].gpio_group,
            g_LED_CONFIGS[led_id].gpio_pin,
            GPIO_PIN_SET
        );
    }
}

void LED_LL_off(LED_LL_ID led_id)
{
    if (led_id < LED_LL_COUNT) {
        HAL_GPIO_WritePin(
            g_LED_CONFIGS[led_id].gpio_group,
            g_LED_CONFIGS[led_id].gpio_pin,
            GPIO_PIN_RESET
        );
    }
}

void LED_LL_toggle(LED_LL_ID led_id)
{
    if (led_id < LED_LL_COUNT) {
        HAL_GPIO_TogglePin(
            g_LED_CONFIGS[led_id].gpio_group, g_LED_CONFIGS[led_id].gpio_pin
        );
    }
}
