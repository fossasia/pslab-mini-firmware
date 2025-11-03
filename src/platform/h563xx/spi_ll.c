/**
 * @file    spi_ll.c
 * @brief   SPI hardware implementation for STM32H563xx
 *
 * This module handles initialization and operation of the SPI peripheral of
 * the STM32H5 microcontroller. It configures the hardware and dispatches SPI
 * interrupts to the hardware-independent SPI implementation.
 *
 * Implementation Details:
 * - Supports single SPI instance (SPI1)
 * - Configured for 8-bit data size
 * - DMA-based transmission and reception
 *
 * @author  Tejas Garg
 * @date    2025-10-31
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"

#include "util/error.h"

#include "spi_ll.h"

/* SPI instance structure */
typedef struct {
    SPI_HandleTypeDef *hspi;
    bool initialized;
} SPIInstance;

/* HAL SPI handle */
static SPI_HandleTypeDef g_hspi1 = { nullptr };

/* SPI instance configuration */
static SPIInstance g_spi_instances[1] = {
    [SPI_BUS_0] = {
        .hspi = &g_hspi1,
    },
};

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    if (hspi->Instance == SPI3) {
        /* SPI3 clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        /* SPI1 GPIO Configuration: PC10=SCK, PC11=MISO, PC12=MOSI */
        gpio_init.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF6_SPI3;
        HAL_GPIO_Init(GPIOC, &gpio_init);

    } else {
        THROW(ERROR_INVALID_ARGUMENT);
    }
}

/**
 * @brief Initialize the SPI peripheral.
 *
 * @param bus SPI bus instance to initialize
 */
void SPI_LL_init(SPI_Bus bus)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (instance->initialized) {
        return;
    }
    SPI_TypeDef *spi_instance[SPI_BUS_COUNT] = {
        [SPI_BUS_0] = SPI3,
    };

    /* Initialize SPI1 */
    instance->hspi->Instance = spi_instance[bus];
    instance->hspi->Init.Mode = SPI_MODE_MASTER;
    instance->hspi->Init.Direction = SPI_DIRECTION_2LINES;
    instance->hspi->Init.DataSize = SPI_DATASIZE_8BIT;
    instance->hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
    instance->hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
    instance->hspi->Init.NSS = SPI_NSS_SOFT;
    instance->hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    instance->hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
    instance->hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    instance->hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(instance->hspi) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    instance->initialized = true;
}

/**
 * @brief Deinitialize the SPI peripheral.
 *
 * @param bus SPI bus instance to deinitialize
 */
void SPI_LL_deinit(SPI_Bus bus)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (!instance->initialized) {
        return;
    }

    /* Deinitialize SPI */
    if (HAL_SPI_DeInit(instance->hspi) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    instance->initialized = false;
}

/**
 * @brief Transmit data over SPI.
 *
 * @param bus SPI bus instance to use
 * @param data Pointer to the data buffer to transmit
 * @param size Size of the data buffer
 */
void SPI_LL_transmit(SPI_Bus bus, uint8_t const *txdata, size_t size)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (!instance->initialized) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Start the transmission
    HAL_SPI_Transmit(instance->hspi, (uint8_t *)txdata, size, HAL_MAX_DELAY);
}
/**
 * @brief Receive data over SPI.
 *
 * @param bus SPI bus instance to use
 * @param data Pointer to the buffer to store received data
 * @param size Size of the data buffer
 */

void SPI_LL_receive(SPI_Bus bus, uint8_t *rxdata, size_t size)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (!instance->initialized) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Start the reception
    HAL_SPI_Receive(instance->hspi, rxdata, size, HAL_MAX_DELAY);
}

/**
 * @brief Transmit and receive data over SPI.
 *
 * @param bus SPI bus instance to use
 * @param tx_data Pointer to the data buffer to transmit
 * @param rx_data Pointer to the buffer to store received data
 * @param size Size of the data buffer
 */
void SPI_LL_transmit_receive(
    SPI_Bus bus,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t size
)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (!instance->initialized) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Start the transmission and reception
    HAL_SPI_TransmitReceive(
        instance->hspi, (uint8_t *)tx_data, rx_data, size, HAL_MAX_DELAY
    );
}