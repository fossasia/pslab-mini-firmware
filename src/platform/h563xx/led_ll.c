/**
 * @file led_ll.c
 * @brief Low-level LED control implementation for PSLab Mini (STM32H563xx)
 *
 * This module provides a low-level interface to control the PSLab's onboard
 * LEDs.
 *
 * Hardware Implementation Details:
 * - Uses STM32H5xx HAL library for GPIO operations
 * - LED1 is connected to GPIOB Pin 0 (PB0)
 * - Configured as push-pull output with no pull-up/pull-down resistors
 * - Low frequency GPIO speed setting for power efficiency
 * - Requires GPIOB clock to be enabled
 *
 * @author PSLab Team
 * @date 2025
 */

#include "stm32h5xx_hal.h"

#include "led_ll.h"

#define LED1_GPIO_GROUP GPIOB
#define LED1_GPIO_PIN GPIO_PIN_0

/**
 * @brief Initialize the LED hardware
 *
 * Configures GPIO Port B Pin 0 (PB0) as an output pin for LED control.
 * Enables the GPIOB clock and sets up the pin as push-pull output with
 * low frequency speed for optimal power consumption.
 */
void LED_LL_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO ports clock enable. */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure the LED GPIO pin. */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief Toggle the LED state
 *
 * Uses the STM32H5xx HAL library function to toggle the GPIO pin state.
 * This will turn the LED on if it was off, or off if it was on.
 *
 * @note LED_LL_init() must be called before using this function.
 */
void LED_LL_toggle(void) { HAL_GPIO_TogglePin(LED1_GPIO_GROUP, LED1_GPIO_PIN); }
