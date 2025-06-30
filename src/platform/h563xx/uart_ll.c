/**
 * @file uart_ll.c
 * @brief UART hardware implementation for STM32H563xx
 *
 * This module handles initialization and operation of the UART peripheral of
 * the STM32H5 microcontroller. It configures the hardware and dispatches UART
 * interrupts to the hardware-independent UART implementation.
 *
 * Implementation Details:
 * - Uses USART3 on STM32H5 (PD8=TX, PD9=RX)
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

/* HAL UART handle */
static UART_HandleTypeDef huart = { 0 };
static DMA_HandleTypeDef hdma_usart3_tx = { 0 };
static DMA_HandleTypeDef hdma_usart3_rx = { 0 };

/* Buffer for DMA reception */
static uint8_t *rx_buffer_data;
static uint32_t rx_buffer_size;

/* Interrupt state */
static volatile bool tx_in_progress = false;
static volatile uint32_t tx_dma_size = 0;

/* Callback function pointers */
static UART_LL_tx_complete_callback_t tx_complete_callback = NULL;
static UART_LL_rx_complete_callback_t rx_complete_callback = NULL;
static UART_LL_idle_callback_t idle_callback = NULL;

/**
 * @brief MSP initialization for UART
 *
 * This function is called by HAL_UART_Init() to configure GPIO pins,
 * DMA channels, and interrupts for the UART interface.
 *
 * @param huart UART handle
 */
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    if (uartHandle->Instance == USART3) {
        /* USART3 clock enable */
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPDMA1_CLK_ENABLE();

        /* UART GPIO Configuration */
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* UART DMA Init */
        /* Configure DMA for TX */
        hdma_usart3_tx.Instance = GPDMA1_Channel0;
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
        hdma_usart3_tx.Init.TransferAllocatedPort = (
            DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0
        );
        hdma_usart3_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        hdma_usart3_tx.Init.Mode = DMA_NORMAL;

        HAL_DMA_Init(&hdma_usart3_tx);
        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart3_tx);

        /* Configure DMA for RX */
        hdma_usart3_rx.Instance = GPDMA1_Channel1;
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
        hdma_usart3_rx.Init.TransferAllocatedPort = (
            DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0
        );
        hdma_usart3_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        /* Use normal mode, manage circular manually */
        hdma_usart3_rx.Init.Mode = DMA_NORMAL;

        HAL_DMA_Init(&hdma_usart3_rx);
        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart3_rx);

        /* UART interrupt Init */
        HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);

        /* DMA interrupt init */
        HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

        HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
    }
}

/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 */
void UART_LL_init(uint8_t *rx_buf, uint32_t sz)
{
    huart.Instance = USART3;
    huart.Init.BaudRate = 115200;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;
    huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    HAL_UART_Init(&huart);

    rx_buffer_data = rx_buf;
    rx_buffer_size = sz;

    /* Start DMA reception */
    HAL_UART_Receive_DMA(&huart, rx_buffer_data, rx_buffer_size);

    /* Enable UART idle line interrupt for packet detection */
    __HAL_UART_ENABLE_IT(&huart, UART_IT_IDLE);

    /* Clear any pending flags */
    __HAL_UART_CLEAR_FLAG(&huart, UART_FLAG_IDLE);
    __HAL_UART_CLEAR_FLAG(&huart, UART_FLAG_ORE);
}

/**
 * @brief Start UART DMA transmission
 *
 * @param buffer Pointer to data to transmit
 * @param size Number of bytes to transmit
 */
void UART_LL_start_dma_tx(uint8_t *buffer, uint32_t size)
{
    tx_in_progress = true;
    tx_dma_size = size;
    HAL_UART_Transmit_DMA(&huart, buffer, size);
}

/**
 * @brief Get current DMA position for RX buffer
 *
 * @return Current DMA position
 */
uint32_t UART_LL_get_dma_position(void)
{
    return rx_buffer_size - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
}

/**
 * @brief Check if UART TX is busy
 *
 * @return true if TX is in progress, false otherwise
 */
bool UART_LL_tx_busy(void)
{
    return tx_in_progress;
}

/**
 * @brief Set up the RX buffer for DMA
 *
 * This function is called from the hardware-independent UART module to
 * provide the buffer for DMA operations.
 *
 * @param buffer Pointer to the RX buffer
 * @param size Size of the RX buffer
 */
void UART_LL_set_dma_buffer(uint8_t *buffer, uint32_t size)
{
    rx_buffer_data = buffer;
    rx_buffer_size = size;
}

/**
 * @brief Set the TX complete callback function
 * @param callback Callback function to call when TX is complete
 */
void UART_LL_set_tx_complete_callback(UART_LL_tx_complete_callback_t callback)
{
    tx_complete_callback = callback;
}

/**
 * @brief Set the RX complete callback function
 * @param callback Callback function to call when RX buffer is full
 */
void UART_LL_set_rx_complete_callback(UART_LL_rx_complete_callback_t callback)
{
    rx_complete_callback = callback;
}

/**
 * @brief Set the idle line callback function
 * @param callback Callback function to call when idle line is detected
 */
void UART_LL_set_idle_callback(UART_LL_idle_callback_t callback)
{
    idle_callback = callback;
}

/**
 * @brief UART TX complete callback
 *
 * Called by HAL when TX DMA transfer is complete.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        tx_in_progress = false;
        /* Notify the hardware-independent layer */
        if (tx_complete_callback != NULL) {
            tx_complete_callback(tx_dma_size);
        }
    }
}

/**
 * @brief UART RX complete callback
 *
 * Called by HAL when RX DMA buffer is full.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        /* Restart DMA reception */
        HAL_UART_Receive_DMA(huart, rx_buffer_data, rx_buffer_size);

        /* Notify the hardware-independent layer */
        if (rx_complete_callback != NULL) {
            rx_complete_callback();
        }
    }
}

/**
 * @brief USART3 interrupt handler
 *
 * This function handles USART3 interrupts including idle line detection
 * and error handling.
 */
void USART3_IRQHandler(void)
{
    /* Check for idle line detection */
    if(__HAL_UART_GET_FLAG(&huart, UART_FLAG_IDLE) != RESET) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart);

        /* Calculate the number of bytes received */
        uint32_t dma_pos = UART_LL_get_dma_position();

        /* Notify the hardware-independent layer */
        if (idle_callback != NULL) {
            idle_callback(dma_pos);
        }
    }

    /* Handle other UART interrupts */
    HAL_UART_IRQHandler(&huart);
}

/**
 * @brief GPDMA1 Channel0 interrupt handler (TX channel)
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

/**
 * @brief GPDMA1 Channel1 interrupt handler (RX channel)
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}
