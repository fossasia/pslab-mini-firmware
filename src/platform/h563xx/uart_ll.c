/**
 * @file uart_ll.c
 * @brief UART hardware implementation for STM32H563xx
 *
 * This module handles initialization and operation of the UART peripheral of
 * the STM32H5 microcontroller. It configures the hardware and dispatches UART
 * interrupts to the hardware-independent UART implementation.
 *
 * Implementation Details:
 * - Supports multiple UART instances (USART1, USART2, USART3)
 * - Configured for 115200 baud, 8N1 format
 * - DMA-based reception with idle line detection
 * - DMA-based transmission for optimal performance
 * - NVIC priority set to 3 for UART interrupts
 *
 * @author Alexander Bessman
 * @date 2025-06-28
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stm32h5xx_hal.h"

#include "uart_ll.h"

/* UART instance configuration */
typedef struct {
    UART_HandleTypeDef *huart;
    DMA_HandleTypeDef *hdma_tx;
    DMA_HandleTypeDef *hdma_rx;
    uint8_t *rx_buffer_data;
    uint32_t rx_buffer_size;
    bool volatile tx_in_progress;
    uint32_t volatile tx_dma_size;
    UART_LL_tx_complete_callback_t tx_complete_callback;
    UART_LL_rx_complete_callback_t rx_complete_callback;
    UART_LL_idle_callback_t idle_callback;
    bool initialized;
} uart_instance_t;

/* HAL UART handles */
static UART_HandleTypeDef huart1 = {0};
static UART_HandleTypeDef huart2 = {0};
static UART_HandleTypeDef huart3 = {0};

/* DMA handles */
static DMA_HandleTypeDef hdma_usart1_tx = {0};
static DMA_HandleTypeDef hdma_usart1_rx = {0};
static DMA_HandleTypeDef hdma_usart2_tx = {0};
static DMA_HandleTypeDef hdma_usart2_rx = {0};
static DMA_HandleTypeDef hdma_usart3_tx = {0};
static DMA_HandleTypeDef hdma_usart3_rx = {0};

/* Instance array */
static uart_instance_t uart_instances[UART_BUS_COUNT] = {
    [UART_BUS_0] =
        {.huart = &huart1,
         .hdma_tx = &hdma_usart1_tx,
         .hdma_rx = &hdma_usart1_rx},
    [UART_BUS_1] =
        {.huart = &huart2,
         .hdma_tx = &hdma_usart2_tx,
         .hdma_rx = &hdma_usart2_rx},
    [UART_BUS_2] = {
        .huart = &huart3,
        .hdma_tx = &hdma_usart3_tx,
        .hdma_rx = &hdma_usart3_rx
    },
};

/**
 * @brief Get UART instance from HAL handle
 */
static uart_bus_t get_bus_from_handle(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
        return UART_BUS_0;
    if (huart->Instance == USART2)
        return UART_BUS_1;
    if (huart->Instance == USART3)
        return UART_BUS_2;
    return UART_BUS_COUNT; // Invalid
}

/**
 * @brief MSP initialization for UART
 *
 * This function is called by HAL_UART_Init() to configure GPIO pins,
 * DMA channels, and interrupts for the UART interface.
 *
 * @param huart UART handle
 */
void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (uartHandle->Instance == USART1) {
        /* USART1 clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPDMA1_CLK_ENABLE();

        /* USART1 GPIO Configuration: PA9=TX, PA10=RX */
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Configure DMA for TX */
        hdma_usart1_tx.Instance = GPDMA1_Channel0;
        hdma_usart1_tx.Init.Request = GPDMA1_REQUEST_USART1_TX;
        hdma_usart1_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
        hdma_usart1_tx.Init.DestInc = DMA_DINC_FIXED;
        hdma_usart1_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        hdma_usart1_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        hdma_usart1_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_usart1_tx.Init.SrcBurstLength = 1;
        hdma_usart1_tx.Init.DestBurstLength = 1;
        hdma_usart1_tx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        hdma_usart1_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart1_tx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&hdma_usart1_tx);
        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart1_tx);

        /* Configure DMA for RX */
        hdma_usart1_rx.Instance = GPDMA1_Channel1;
        hdma_usart1_rx.Init.Request = GPDMA1_REQUEST_USART1_RX;
        hdma_usart1_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.SrcInc = DMA_SINC_FIXED;
        hdma_usart1_rx.Init.DestInc = DMA_DINC_INCREMENTED;
        hdma_usart1_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        hdma_usart1_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        hdma_usart1_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_usart1_rx.Init.SrcBurstLength = 1;
        hdma_usart1_rx.Init.DestBurstLength = 1;
        hdma_usart1_rx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        hdma_usart1_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart1_rx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&hdma_usart1_rx);
        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart1_rx);

        /* UART interrupt init */
        HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);

        /* DMA interrupt init */
        HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
        HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
    } else if (uartHandle->Instance == USART2) {
        /* USART2 clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPDMA1_CLK_ENABLE();

        /* USART2 GPIO Configuration: PA2=TX, PA3=RX */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Configure DMA for TX */
        hdma_usart2_tx.Instance = GPDMA1_Channel2;
        hdma_usart2_tx.Init.Request = GPDMA1_REQUEST_USART2_TX;
        hdma_usart2_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
        hdma_usart2_tx.Init.DestInc = DMA_DINC_FIXED;
        hdma_usart2_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        hdma_usart2_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        hdma_usart2_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_usart2_tx.Init.SrcBurstLength = 1;
        hdma_usart2_tx.Init.DestBurstLength = 1;
        hdma_usart2_tx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        hdma_usart2_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&hdma_usart2_tx);
        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart2_tx);

        /* Configure DMA for RX */
        hdma_usart2_rx.Instance = GPDMA1_Channel3;
        hdma_usart2_rx.Init.Request = GPDMA1_REQUEST_USART2_RX;
        hdma_usart2_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.SrcInc = DMA_SINC_FIXED;
        hdma_usart2_rx.Init.DestInc = DMA_DINC_INCREMENTED;
        hdma_usart2_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        hdma_usart2_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        hdma_usart2_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_usart2_rx.Init.SrcBurstLength = 1;
        hdma_usart2_rx.Init.DestBurstLength = 1;
        hdma_usart2_rx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        hdma_usart2_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart2_rx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&hdma_usart2_rx);
        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart2_rx);

        /* UART interrupt init */
        HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);

        /* DMA interrupt init */
        HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);
        HAL_NVIC_SetPriority(GPDMA1_Channel3_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel3_IRQn);
    } else if (uartHandle->Instance == USART3) {
        /* USART3 clock enable */
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPDMA1_CLK_ENABLE();

        /* UART GPIO Configuration */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* Configure DMA for TX */
        hdma_usart3_tx.Instance = GPDMA1_Channel4;
        hdma_usart3_tx.Init.Request = GPDMA1_REQUEST_USART3_TX;
        hdma_usart3_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart3_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
        hdma_usart3_tx.Init.DestInc = DMA_DINC_FIXED;
        hdma_usart3_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        hdma_usart3_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        hdma_usart3_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_usart3_tx.Init.SrcBurstLength = 1;
        hdma_usart3_tx.Init.DestBurstLength = 1;
        hdma_usart3_tx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        hdma_usart3_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart3_tx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&hdma_usart3_tx);
        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart3_tx);

        /* Configure DMA for RX */
        hdma_usart3_rx.Instance = GPDMA1_Channel5;
        hdma_usart3_rx.Init.Request = GPDMA1_REQUEST_USART3_RX;
        hdma_usart3_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart3_rx.Init.SrcInc = DMA_SINC_FIXED;
        hdma_usart3_rx.Init.DestInc = DMA_DINC_INCREMENTED;
        hdma_usart3_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        hdma_usart3_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        hdma_usart3_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_usart3_rx.Init.SrcBurstLength = 1;
        hdma_usart3_rx.Init.DestBurstLength = 1;
        hdma_usart3_rx.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        hdma_usart3_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart3_rx.Init.Mode = DMA_NORMAL;
        HAL_DMA_Init(&hdma_usart3_rx);
        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart3_rx);

        /* UART interrupt init */
        HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);

        /* DMA interrupt init */
        HAL_NVIC_SetPriority(GPDMA1_Channel4_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel4_IRQn);
        HAL_NVIC_SetPriority(GPDMA1_Channel5_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel5_IRQn);
    }
}

/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 */
void UART_LL_init(uart_bus_t bus, uint8_t *rx_buf, uint32_t sz)
{
    if (bus >= UART_BUS_COUNT || uart_instances[bus].initialized) {
        return;
    }

    uart_instance_t *instance = &uart_instances[bus];

    /* Configure UART instance based on bus */
    if (bus == UART_BUS_0) {
        instance->huart->Instance = USART1;
    } else if (bus == UART_BUS_1) {
        instance->huart->Instance = USART2;
    } else if (bus == UART_BUS_2) {
        instance->huart->Instance = USART3;
    }

    instance->huart->Init.BaudRate = 115200;
    instance->huart->Init.WordLength = UART_WORDLENGTH_8B;
    instance->huart->Init.StopBits = UART_STOPBITS_1;
    instance->huart->Init.Parity = UART_PARITY_NONE;
    instance->huart->Init.Mode = UART_MODE_TX_RX;
    instance->huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    instance->huart->Init.OverSampling = UART_OVERSAMPLING_16;
    instance->huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    instance->huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    HAL_UART_Init(instance->huart);

    instance->rx_buffer_data = rx_buf;
    instance->rx_buffer_size = sz;
    instance->tx_in_progress = false;
    instance->tx_dma_size = 0;
    instance->initialized = true;

    /* Start DMA reception */
    HAL_UART_Receive_DMA(
        instance->huart, instance->rx_buffer_data, instance->rx_buffer_size
    );

    /* Enable UART idle line interrupt for packet detection */
    __HAL_UART_ENABLE_IT(instance->huart, UART_IT_IDLE);

    /* Clear any pending flags */
    __HAL_UART_CLEAR_FLAG(instance->huart, UART_FLAG_IDLE);
    __HAL_UART_CLEAR_FLAG(instance->huart, UART_FLAG_ORE);
}

/**
 * @brief Deinitialize the UART peripheral.
 *
 * @param bus UART bus instance to deinitialize
 */
void UART_LL_deinit(uart_bus_t bus)
{
    if (bus >= UART_BUS_COUNT || !uart_instances[bus].initialized) {
        return;
    }

    uart_instance_t *instance = &uart_instances[bus];

    /* Disable interrupts */
    if (bus == UART_BUS_0) {
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    } else if (bus == UART_BUS_1) {
        HAL_NVIC_DisableIRQ(USART2_IRQn);
    } else if (bus == UART_BUS_2) {
        HAL_NVIC_DisableIRQ(USART3_IRQn);
    }

    /* Deinitialize UART */
    HAL_UART_DeInit(instance->huart);

    /* Clear instance data */
    instance->rx_buffer_data = NULL;
    instance->rx_buffer_size = 0;
    instance->tx_in_progress = false;
    instance->tx_dma_size = 0;
    instance->tx_complete_callback = NULL;
    instance->rx_complete_callback = NULL;
    instance->idle_callback = NULL;
    instance->initialized = false;
}

/**
 * @brief Start UART DMA transmission
 *
 * @param bus UART bus instance
 * @param buffer Pointer to data to transmit
 * @param size Number of bytes to transmit
 */
void UART_LL_start_dma_tx(uart_bus_t bus, uint8_t *buffer, uint32_t size)
{
    if (bus >= UART_BUS_COUNT || !uart_instances[bus].initialized) {
        return;
    }

    uart_instance_t *instance = &uart_instances[bus];
    instance->tx_in_progress = true;
    instance->tx_dma_size = size;
    HAL_UART_Transmit_DMA(instance->huart, buffer, size);
}

/**
 * @brief Get current DMA position for RX buffer
 *
 * @param bus UART bus instance
 * @return Current DMA position
 */
uint32_t UART_LL_get_dma_position(uart_bus_t bus)
{
    if (bus >= UART_BUS_COUNT || !uart_instances[bus].initialized) {
        return 0;
    }

    uart_instance_t *instance = &uart_instances[bus];
    return instance->rx_buffer_size - __HAL_DMA_GET_COUNTER(instance->hdma_rx);
}

/**
 * @brief Check if UART TX is busy
 *
 * @param bus UART bus instance
 * @return true if TX is in progress, false otherwise
 */
bool UART_LL_tx_busy(uart_bus_t bus)
{
    if (bus >= UART_BUS_COUNT || !uart_instances[bus].initialized) {
        return false;
    }

    return uart_instances[bus].tx_in_progress;
}

/**
 * @brief Set the TX complete callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when TX is complete
 */
void UART_LL_set_tx_complete_callback(
    uart_bus_t bus,
    UART_LL_tx_complete_callback_t callback
)
{
    if (bus >= UART_BUS_COUNT) {
        return;
    }
    uart_instances[bus].tx_complete_callback = callback;
}

/**
 * @brief Set the RX complete callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when RX buffer is full
 */
void UART_LL_set_rx_complete_callback(
    uart_bus_t bus,
    UART_LL_rx_complete_callback_t callback
)
{
    if (bus >= UART_BUS_COUNT) {
        return;
    }
    uart_instances[bus].rx_complete_callback = callback;
}

/**
 * @brief Set the idle line callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when idle line is detected
 */
void UART_LL_set_idle_callback(uart_bus_t bus, UART_LL_idle_callback_t callback)
{
    if (bus >= UART_BUS_COUNT) {
        return;
    }
    uart_instances[bus].idle_callback = callback;
}

/**
 * @brief UART TX complete callback
 *
 * Called by HAL when TX DMA transfer is complete.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_bus_t bus = get_bus_from_handle(huart);
    if (bus >= UART_BUS_COUNT) {
        return;
    }

    uart_instance_t *instance = &uart_instances[bus];
    instance->tx_in_progress = false;

    /* Notify the hardware-independent layer */
    if (instance->tx_complete_callback != NULL) {
        instance->tx_complete_callback(bus, instance->tx_dma_size);
    }
}

/**
 * @brief UART RX complete callback
 *
 * Called by HAL when RX DMA buffer is full.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_bus_t bus = get_bus_from_handle(huart);
    if (bus >= UART_BUS_COUNT) {
        return;
    }

    uart_instance_t *instance = &uart_instances[bus];

    /* Restart DMA reception */
    HAL_UART_Receive_DMA(
        huart, instance->rx_buffer_data, instance->rx_buffer_size
    );

    /* Notify the hardware-independent layer */
    if (instance->rx_complete_callback != NULL) {
        instance->rx_complete_callback(bus);
    }
}

/**
 * @brief Common UART interrupt handler
 * @param huart UART handle
 */
static void uart_irq_handler(UART_HandleTypeDef *huart)
{
    uart_bus_t bus = get_bus_from_handle(huart);
    if (bus >= UART_BUS_COUNT) {
        return;
    }

    uart_instance_t *instance = &uart_instances[bus];

    /* Check for idle line detection */
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET) {
        __HAL_UART_CLEAR_IDLEFLAG(huart);

        /* Calculate the number of bytes received */
        uint32_t dma_pos = UART_LL_get_dma_position(bus);

        /* Notify the hardware-independent layer */
        if (instance->idle_callback != NULL) {
            instance->idle_callback(bus, dma_pos);
        }
    }

    /* Handle other UART interrupts */
    HAL_UART_IRQHandler(huart);
}

/**
 * @brief USART1 interrupt handler
 */
void USART1_IRQHandler(void) { uart_irq_handler(&huart1); }

/**
 * @brief USART2 interrupt handler
 */
void USART2_IRQHandler(void) { uart_irq_handler(&huart2); }

/**
 * @brief USART3 interrupt handler
 */
void USART3_IRQHandler(void) { uart_irq_handler(&huart3); }

/**
 * @brief GPDMA1 Channel0 interrupt handler (USART1 TX)
 */
void GPDMA1_Channel0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart1_tx); }

/**
 * @brief GPDMA1 Channel1 interrupt handler (USART1 RX)
 */
void GPDMA1_Channel1_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart1_rx); }

/**
 * @brief GPDMA1 Channel2 interrupt handler (USART2 TX)
 */
void GPDMA1_Channel2_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart2_tx); }

/**
 * @brief GPDMA1 Channel3 interrupt handler (USART2 RX)
 */
void GPDMA1_Channel3_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart2_rx); }

/**
 * @brief GPDMA1 Channel4 interrupt handler (USART3 TX)
 */
void GPDMA1_Channel4_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart3_tx); }

/**
 * @brief GPDMA1 Channel5 interrupt handler (USART3 RX)
 */
void GPDMA1_Channel5_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart3_rx); }
