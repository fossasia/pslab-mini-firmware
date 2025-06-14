#include "stm32h5xx_hal.h"

#include "led.h"

#define LED1_GPIO_GROUP  GPIOB
#define LED1_GPIO_PIN  GPIO_PIN_0

void LED_toggle(void)
{
    HAL_GPIO_TogglePin(LED1_GPIO_GROUP, LED1_GPIO_PIN);
}
