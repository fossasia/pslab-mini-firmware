/**
 * @file usb.c
 * @brief USB interface implementation for STM32H563xx using TinyUSB
 *
 * This module handles initialization and operation of the USB peripheral of
 * the STM32H5 microcontroller. It configures the hardware, provides CDC-based
 * data transfer functions, and dispatches USB interrupts to the TinyUSB stack.
 */

#include <stdbool.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"
#include "tusb.h"

#include "usb.h"

void USB_init(void)
{
    HAL_PWREx_EnableVddUSB();

    // Enable USB-related clocks.
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB_CLK_ENABLE();

    // Configuring GPIO via HAL is technically unnecessary, as TinyUSB handles
    // it under the hood. The following code is to keep HAL's state consistent
    // with the actual system state.
    // PA11 = DM, PA12 = DP
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed= GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_USB;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(USB_DRD_FS_IRQn, 5, 0);

    // TinyUSB takes it from here.
    tusb_init();
}

void USB_task(void)
{
    tud_task();
}

bool USB_rx_ready(void)
{
    return tud_cdc_available();
}

uint32_t USB_read(uint8_t *buf, uint32_t sz)
{
    return tud_cdc_read(buf, sz);
}

uint32_t USB_write(uint8_t *buf, uint32_t sz)
{
    return tud_cdc_write(buf, sz);
}

uint32_t USB_tx_flush(void)
{
    return tud_cdc_write_flush();
}

/**
 * @brief USB FS dual-role device interrupt handler.
 *
 * Dispatches the USB DRD FS interrupt to the TinyUSB device controller
 * driver. Called by the NVIC when USB_DRD_FS_IRQn is triggered.
 */
void USB_DRD_FS_IRQHandler(void)
{
    tud_int_handler(0);
}
