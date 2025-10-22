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
 * @date    2024-10-09
 */

#include "stm32h5xx_hal.h"

#include "util/error.h"

#include "spi_ll.h"

enum { SPI_IRQ_PRIO = 4 }; // NVIC priority for SPI interrupts

/* SPI instance structure */
typedef struct {
    SPI_HandleTypeDef *hspi;
    bool initialized;
    bool tx_in_progress;
    bool rx_in_progress;
    void (*tx_complete_callback)(void);
    void (*rx_complete_callback)(void);
} SPIInstance;

/* HAL SPI handle */
static SPI_HandleTypeDef g_hspi1 = { nullptr };
static SPI_HandleTypeDef g_hspi2 = { nullptr };

/* DMA handles */
static DMA_HandleTypeDef g_hdma_spi1_tx = { nullptr };
static DMA_HandleTypeDef g_hdma_spi1_rx = { nullptr };
static DMA_HandleTypeDef g_hdma_spi2_tx = { nullptr };
static DMA_HandleTypeDef g_hdma_spi2_rx = { nullptr };

/* SPI instance configuration */
static SPIInstance g_spi_instances[2] = {
    [SPI_BUS_0] = {
        .hspi = &g_hspi1,
    },
    [SPI_BUS_1] = {
        .hspi = &g_hspi2,
    },
};

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    if (hspi->Instance == SPI1) {
        /* SPI1 clock enable */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPDMA1_CLK_ENABLE();

        /* SPI1 GPIO Configuration: PA5=SCK, PA6=MISO, PA7=MOSI */
        gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &gpio_init);

        /* Configure DMA for TX */
        g_hdma_spi1_tx.Instance = GPDMA1_Channel6;
        g_hdma_spi1_tx.Init.Request = GPDMA1_REQUEST_SPI1_TX;
        g_hdma_spi1_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        g_hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        g_hdma_spi1_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
        g_hdma_spi1_tx.Init.DestInc = DMA_DINC_FIXED;
        g_hdma_spi1_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        g_hdma_spi1_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        g_hdma_spi1_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        g_hdma_spi1_tx.Init.SrcBurstLength = 1;
        g_hdma_spi1_tx.Init.DestBurstLength = 1;
        g_hdma_spi1_tx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        g_hdma_spi1_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        g_hdma_spi1_tx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&g_hdma_spi1_tx);
        __HAL_LINKDMA(hspi, hdmatx, g_hdma_spi1_tx);

        /* Configure DMA for RX */
        g_hdma_spi1_rx.Instance = GPDMA1_Channel7;
        g_hdma_spi1_rx.Init.Request = GPDMA1_REQUEST_SPI1_RX;
        g_hdma_spi1_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        g_hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        g_hdma_spi1_rx.Init.SrcInc = DMA_SINC_FIXED;
        g_hdma_spi1_rx.Init.DestInc = DMA_DINC_INCREMENTED;
        g_hdma_spi1_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        g_hdma_spi1_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        g_hdma_spi1_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        g_hdma_spi1_rx.Init.SrcBurstLength = 1;
        g_hdma_spi1_rx.Init.DestBurstLength = 1;
        g_hdma_spi1_rx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        g_hdma_spi1_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        g_hdma_spi1_rx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&g_hdma_spi1_rx);
        __HAL_LINKDMA(hspi, hdmarx, g_hdma_spi1_rx);

        /* SPI1 interrupt init */
        HAL_NVIC_SetPriority(SPI1_IRQn, SPI_IRQ_PRIO, 1);
        HAL_NVIC_EnableIRQ(SPI1_IRQn);

        /* DMA interrupt init */
        HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, SPI_IRQ_PRIO, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
        HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, SPI_IRQ_PRIO, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);

    } else if (hspi->Instance == SPI2) {
        /* SPI2 GPIO Configuration: PB13=SCK, PB14=MISO, PB15=MOSI */
        gpio_init.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        /* Configure DMA for TX */
        g_hdma_spi2_tx.Instance = GPDMA1_Channel6;
        g_hdma_spi2_tx.Init.Request = GPDMA1_REQUEST_SPI2_TX;
        g_hdma_spi2_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        g_hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        g_hdma_spi2_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
        g_hdma_spi2_tx.Init.DestInc = DMA_DINC_FIXED;
        g_hdma_spi2_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        g_hdma_spi2_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        g_hdma_spi2_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        g_hdma_spi2_tx.Init.SrcBurstLength = 1;
        g_hdma_spi2_tx.Init.DestBurstLength = 1;
        g_hdma_spi2_tx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        g_hdma_spi2_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        g_hdma_spi2_tx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&g_hdma_spi2_tx);
        __HAL_LINKDMA(hspi, hdmatx, g_hdma_spi2_tx);

        /* Configure DMA for RX */
        g_hdma_spi2_rx.Instance = GPDMA1_Channel7;
        g_hdma_spi2_rx.Init.Request = GPDMA1_REQUEST_SPI2_RX;
        g_hdma_spi2_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        g_hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        g_hdma_spi2_rx.Init.SrcInc = DMA_SINC_FIXED;
        g_hdma_spi2_rx.Init.DestInc = DMA_DINC_INCREMENTED;
        g_hdma_spi2_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        g_hdma_spi2_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        g_hdma_spi2_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        g_hdma_spi2_rx.Init.SrcBurstLength = 1;
        g_hdma_spi2_rx.Init.DestBurstLength = 1;
        g_hdma_spi2_rx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        g_hdma_spi2_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        g_hdma_spi2_rx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&g_hdma_spi2_rx);
        __HAL_LINKDMA(hspi, hdmarx, g_hdma_spi2_rx);

        /* SPI2 interrupt init */
        HAL_NVIC_SetPriority(SPI2_IRQn, SPI_IRQ_PRIO, 1);
        HAL_NVIC_EnableIRQ(SPI2_IRQn);

        /* DMA interrupt init */
        HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, SPI_IRQ_PRIO, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
        HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, SPI_IRQ_PRIO, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);
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
        [SPI_BUS_0] = SPI1,
        [SPI_BUS_1] = SPI2,
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

    /* Configure NVIC for SPI interrupts */
    HAL_NVIC_SetPriority(SPI1_IRQn, SPI_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

    instance->tx_in_progress = false;
    instance->rx_in_progress = false;
    instance->tx_complete_callback = nullptr;
    instance->rx_complete_callback = nullptr;
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

    /* Disable interrupts */
    if (bus == SPI_BUS_0) {
        HAL_NVIC_DisableIRQ(SPI1_IRQn);
    } else if (bus == SPI_BUS_1) {
        HAL_NVIC_DisableIRQ(SPI2_IRQn);
    }

    /* Deinitialize SPI */
    if (HAL_SPI_DeInit(instance->hspi) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    instance->tx_in_progress = false;
    instance->rx_in_progress = false;
    instance->tx_complete_callback = nullptr;
    instance->rx_complete_callback = nullptr;
    instance->initialized = false;
}

/**
 * @brief Transmit data over SPI.
 *
 * @param bus SPI bus instance to use
 * @param data Pointer to the data buffer to transmit
 * @param size Size of the data buffer
 */
void SPI_LL_transmit(SPI_Bus bus, uint8_t const *data, size_t size)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (!instance->initialized) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Start the transmission
    HAL_SPI_Transmit(instance->hspi, (uint8_t *)data, size, HAL_MAX_DELAY);
}
/**
 * @brief Receive data over SPI.
 *
 * @param bus SPI bus instance to use
 * @param data Pointer to the buffer to store received data
 * @param size Size of the data buffer
 */

void SPI_LL_receive(SPI_Bus bus, uint8_t *data, size_t size)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (!instance->initialized) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Start the reception
    HAL_SPI_Receive(instance->hspi, data, size, HAL_MAX_DELAY);
}