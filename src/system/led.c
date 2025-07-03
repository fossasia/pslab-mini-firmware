/**
 * @file led.c
 * @brief LED system implementation - simple wrapper around the led_ll interface
 * 
 * This module implements the LED system interface by wrapping calls to the
 * low-level LED hardware abstraction layer (led_ll). It provides a clean
 * separation between the application layer and hardware-specific implementation.
 */

#include "led_ll.h"
#include "led.h"
#define LED1_GPIO_GROUP GPIOB
#define LED1_GPIO_PIN GPIO_PIN_0

void LED_toggle(void) { HAL_GPIO_TogglePin(LED1_GPIO_GROUP, LED1_GPIO_PIN); }
void LED_init(void)
{
    LED_LL_init();
}

void LED_toggle(void)
{
    LED_LL_toggle();
}
