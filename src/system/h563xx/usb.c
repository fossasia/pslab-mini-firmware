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

/**
 * @brief Enable USB clock recovery system
 * 
 * Required when using HSI48 clock source.
 */
static void crs_enable(void)
{
    __HAL_RCC_CRS_CLK_ENABLE();
    // USB clock, 48 MHz
    #define USB_CRS_FRQ_TARGET (48000000)
    // USB SOF frequency, 1 kHz
    #define USB_CRS_FRQ_SYNC (1000)
    // CRS trim step, 0.14% per RM0481.
    #define USB_CRS_TRIM_STEP (14)
    // 32 is the default trim value, which neither increases nor decreases
    // the clock frequency. This value will be modified by CRS at runtime.
    #define USB_CRS_TRIM_DEFAULT (32)

    RCC_CRSInitTypeDef CRSInit = {0};
    CRSInit.Prescaler = RCC_CRS_SYNC_DIV1;
    CRSInit.Source = RCC_CRS_SYNC_SOURCE_USB;
    CRSInit.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
    CRSInit.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(
        USB_CRS_FRQ_TARGET,
        USB_CRS_FRQ_SYNC
    );
    CRSInit.ErrorLimitValue = (
        USB_CRS_FRQ_TARGET / USB_CRS_FRQ_SYNC * USB_CRS_TRIM_STEP / 10000 / 2
    );
    CRSInit.HSI48CalibrationValue = USB_CRS_TRIM_DEFAULT;

    HAL_RCCEx_CRSConfig(&CRSInit);
}

void USB_init(void)
{
    HAL_PWREx_EnableVddUSB();

    // Initialize USB clock.
    RCC_PeriphCLKInitTypeDef RCC_PeriphInitStruct = {0};
    RCC_PeriphInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    RCC_PeriphInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphInitStruct) != HAL_OK) {
        // Clock configuration incorrect or hardware failure. Hang the system
        // to prevent damage.
        while (1);
    }

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

    if (__HAL_RCC_GET_USB_SOURCE() == RCC_USBCLKSOURCE_HSI48) {
        crs_enable();
    }

    // TinyUSB owns the pins and ISR from this point.
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
