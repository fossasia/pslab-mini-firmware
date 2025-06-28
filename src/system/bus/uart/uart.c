/**
 * @file uart.c
 * @brief UART (Universal Asynchronous Receiver/Transmitter) module implementation
 *
 * This module provides functions and routines for configuring and handling
 * UART communication. It includes initialization, data transmission, and
 * reception functionalities for serial communication.
 *
 * Implementation Details:
 * - Uses USART3 on STM32H5 (PD8=TX, PD9=RX)
 * - Configured for 115200 baud, 8N1 format
 * - Interrupt-driven with circular buffers (256 bytes each for TX/RX)
 * - Direct RX interrupt handling for optimal performance
 * - DMA-based reception with idle line detection
 * - DMA-based transmission for optimal performance
 * - Supports configurable RX callbacks for protocol implementations
 * - NVIC priority set to 3 for UART interrupts
 *
 * Buffer Management:
 * - RX buffer: Filled by DMA, emptied by UART_read()
 * - TX buffer: Filled by UART_write(), emptied by DMA
 * - Buffer full conditions return errors rather than blocking
 * - Circular buffer implementation with automatic wrap-around
 *
 * @author Alexander Bessman
 * @date 2025-06-27
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stm32h5xx_hal.h"

#include "bus_common.h"
#include "uart.h"


#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256


static UART_HandleTypeDef huart = { 0 };
static DMA_HandleTypeDef hdma_usart3_tx = { 0 };
static DMA_HandleTypeDef hdma_usart3_rx = { 0 };

/* Buffers and their management structures */
static uint8_t rx_buffer_data[UART_RX_BUFFER_SIZE];
static uint8_t tx_buffer_data[UART_TX_BUFFER_SIZE];
static circular_buffer_t rx_buffer;
static circular_buffer_t tx_buffer;

/* Interrupt state */
static bool volatile tx_in_progress = false;
static uint32_t volatile tx_dma_size = 0;

/* RX DMA state */
static uint32_t volatile rx_dma_head = 0;

/* Callback state */
static uart_rx_callback_t rx_callback = NULL;
static uint32_t rx_threshold = 0;


/**
 * @brief Get number of bytes available in TX buffer.
 */
static uint32_t tx_buffer_available(void)
{
    return circular_buffer_available(&tx_buffer);
}

/**
 * @brief Get number of bytes available in RX buffer.
 */
static uint32_t rx_buffer_available(void)
{
    /* Get current DMA write position */
    uint32_t dma_pos = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
    uint32_t dma_head = dma_pos;

    /* Update our cached head position */
    rx_dma_head = dma_head;

    /* Update the circular buffer's head position based on DMA */
    rx_buffer.head = dma_head;

    /* Calculate available bytes using common function */
    return circular_buffer_available(&rx_buffer);
}

/**
 * @brief Start transmission if not already in progress.
 */
static void start_transmission(void)
{
    if (tx_in_progress || circular_buffer_is_empty(&tx_buffer)) {
        return;
    }

    /* Calculate how many contiguous bytes we can send */
    uint32_t available = tx_buffer_available();
    uint32_t contiguous_bytes;

    if (tx_buffer.tail <= tx_buffer.head) {
        contiguous_bytes = tx_buffer.head - tx_buffer.tail;
    } else {
        contiguous_bytes = tx_buffer.size - tx_buffer.tail;
    }

    /* Limit to available bytes and reasonable DMA transfer size */
    if (contiguous_bytes > available) {
        contiguous_bytes = available;
    }
    /* Limit DMA transfer size */
    if (contiguous_bytes > (UART_TX_BUFFER_SIZE / 2)) {
        contiguous_bytes = UART_TX_BUFFER_SIZE / 2;
    }

    if (contiguous_bytes > 0) {
        tx_in_progress = true;
        tx_dma_size = contiguous_bytes;
        HAL_UART_Transmit_DMA(&huart, &tx_buffer.buffer[tx_buffer.tail], contiguous_bytes);
    }
}

/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 */
void UART_init(void)
{
    /* Initialize circular buffers */
    circular_buffer_init(&rx_buffer, rx_buffer_data, UART_RX_BUFFER_SIZE);
    circular_buffer_init(&tx_buffer, tx_buffer_data, UART_TX_BUFFER_SIZE);

    huart.Instance = USART3;
    huart.Init.BaudRate = 115200;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart);

    /* Start DMA reception
     * by restarting transfers in HAL_UART_RxCpltCallback */
    HAL_UART_Receive_DMA(&huart, rx_buffer_data, UART_RX_BUFFER_SIZE);

    /* Enable UART idle line interrupt for packet detection */
    __HAL_UART_ENABLE_IT(&huart, UART_IT_IDLE);

    /* Clear any pending flags */
    __HAL_UART_CLEAR_FLAG(&huart, UART_FLAG_IDLE);
    __HAL_UART_CLEAR_FLAG(&huart, UART_FLAG_ORE);
}

/**
 * @brief Write data to the UART interface.
 *
 * This function queues bytes for transmission using a circular buffer.
 * Data is sent via interrupt-driven transmission.
 *
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return Number of bytes actually written.
 */
uint32_t UART_write(uint8_t const *txbuf, uint32_t const sz)
{
    if (txbuf == NULL || sz == 0) {
        return 0;
    }

    /* Queue bytes for transmission */
    uint32_t bytes_written = circular_buffer_write(&tx_buffer, txbuf, sz);

    /* Start transmission if not already in progress */
    start_transmission();

    return bytes_written;
}

/**
 * @brief Read data from the UART interface.
 *
 * This function reads bytes from the receive circular buffer.
 * Returns the number of bytes actually read.
 *
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return Number of bytes actually read.
 */
uint32_t UART_read(uint8_t *const rxbuf, uint32_t const sz)
{
    if (rxbuf == NULL || sz == 0) {
        return 0;
    }

    uint32_t available = rx_buffer_available();
    uint32_t to_read = sz > available ? available : sz;

    /* Read from circular buffer using common function */
    return circular_buffer_read(&rx_buffer, rxbuf, to_read);
}

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if there is data available in the receive buffer,
 * indicating that there is unread data available to be received.
 *
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(void)
{
    return rx_buffer_available() > 0;
}

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * This function returns the number of bytes currently available
 * to read from the receive buffer.
 *
 * @return Number of bytes available to read.
 */
uint32_t UART_rx_available(void)
{
    return rx_buffer_available();
}

/**
 * @brief Get TX buffer free space.
 *
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t UART_tx_free_space(void)
{
    uint32_t used = tx_buffer_available();
    /* -1 because buffer full leaves one slot empty */
    return tx_buffer.size - used - 1;
}

/**
 * @brief Check if TX transmission is in progress.
 *
 * @return true if transmission is ongoing, false otherwise.
 */
bool UART_tx_busy(void)
{
    return tx_in_progress;
}

/**
 * @brief  Initialize the UART MSP (MCU Support Package).
 *
 * This function configures the hardware resources used for the UART
 * peripheral, including GPIO pins and clocks. It is called by
 * HAL_UART_Init().
 *
 * @param huart UART handle pointer.
 *
 * @return None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *const huart_ptr)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    GPIO_InitTypeDef uart_gpio_init = {
        .Pin = GPIO_PIN_8 | GPIO_PIN_9,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF7_USART3
    };
    HAL_GPIO_Init(GPIOD, &uart_gpio_init);

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
        DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1
    );
    hdma_usart3_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;

    HAL_DMA_Init(&hdma_usart3_tx);

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
        DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1
    );
    hdma_usart3_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    /* Use normal mode, manage circular manually */
    hdma_usart3_rx.Init.Mode = DMA_NORMAL;

    HAL_DMA_Init(&hdma_usart3_rx);

    /* Link DMA to UART */
    __HAL_LINKDMA(huart_ptr, hdmatx, hdma_usart3_tx);
    __HAL_LINKDMA(huart_ptr, hdmarx, hdma_usart3_rx);

    /* Configure NVIC for UART and DMA interrupts */
    HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 3, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 3, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
}

/**
 * @brief UART interrupt handler.
 *
 * This function handles RX interrupts directly for better performance,
 * while delegating TX and other interrupts to the HAL handler.
 */
void USART3_IRQHandler(void)
{
    /* Handle idle line interrupt for packet detection */
    if (__HAL_UART_GET_FLAG(&huart, UART_FLAG_IDLE)
        && __HAL_UART_GET_IT_SOURCE(&huart, UART_IT_IDLE)
    ) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart);

        /* Check for RX callback */
        if (rx_callback && rx_buffer_available() >= rx_threshold) {
            rx_callback(rx_buffer_available());
        }
    }

    /* Handle other interrupts through HAL */
    HAL_UART_IRQHandler(&huart);
}

/**
 * @brief TX Complete callback from HAL.
 *
 * This function is called when a DMA transmission is complete.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *const huart_ptr)
{
    if (huart_ptr->Instance == USART3) {
        /* Update tail position to reflect transmitted data */
        tx_buffer.tail = (tx_buffer.tail + tx_dma_size) % tx_buffer.size;
        tx_in_progress = false;

        /* Check if more data needs to be transmitted */
        start_transmission();
    }
}

/**
 * @brief RX Complete callback from HAL.
 *
 * This function is called when DMA RX is complete (i.e., buffer full).
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *const huart_ptr)
{
    if (huart_ptr->Instance == USART3) {
        /* Restart DMA reception */
        HAL_UART_Receive_DMA(&huart, rx_buffer_data, UART_RX_BUFFER_SIZE);

        /* Check for RX callback */
        if (rx_callback && rx_buffer_available() >= rx_threshold) {
            rx_callback(rx_buffer_available());
        }
    }
}

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void UART_set_rx_callback(
    uart_rx_callback_t callback,
    uint32_t const threshold
) {
    rx_callback = callback;
    rx_threshold = threshold;

    /* Check if callback should be triggered immediately */
    if (rx_callback && rx_buffer_available() >= rx_threshold) {
        rx_callback(rx_buffer_available());
    }
}

/**
 * @brief DMA TX interrupt handler.
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

/**
 * @brief DMA RX interrupt handler.
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}
