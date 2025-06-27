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

#include "uart.h"


#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256


/* Callback function type */
typedef void (*uart_rx_callback_t)(uint32_t bytes_available);

static UART_HandleTypeDef huart = { 0 };
static DMA_HandleTypeDef hdma_usart3_tx = { 0 };
static DMA_HandleTypeDef hdma_usart3_rx = { 0 };

/* Circular buffer structure */
typedef struct {
    uint8_t *buffer;
    uint32_t volatile head;
    uint32_t volatile tail;
    uint32_t size;
} circular_buffer_t;

/* Buffers and their management structures */
static uint8_t rx_buffer_data[UART_RX_BUFFER_SIZE];
static uint8_t tx_buffer_data[UART_TX_BUFFER_SIZE];
static circular_buffer_t rx_buffer = {
    .buffer = rx_buffer_data,
    .size = UART_RX_BUFFER_SIZE
};
static circular_buffer_t tx_buffer = {
    .buffer = tx_buffer_data,
    .size = UART_TX_BUFFER_SIZE
};

/* Interrupt state */
static bool volatile tx_in_progress = false;
static uint32_t volatile tx_dma_size = 0;

/* RX DMA state */
static uint32_t volatile rx_dma_head = 0;

/* Callback state */
static uart_rx_callback_t rx_callback = NULL;
static uint32_t rx_threshold = 0;


/* TX and RX buffer helper functions */

/**
 * @brief Check if TX buffer is empty.
 */
static bool tx_buffer_is_empty(void)
{
    return tx_buffer.head == tx_buffer.tail;
}

/**
 * @brief Check if TX buffer is full.
 */
static bool tx_buffer_is_full(void)
{
    return ((tx_buffer.head + 1) % tx_buffer.size) == tx_buffer.tail;
}

/**
 * @brief Get number of bytes available in TX buffer.
 */
static uint32_t tx_buffer_available(void)
{
    if (tx_buffer.head >= tx_buffer.tail) {
        return tx_buffer.head - tx_buffer.tail;
    } else {
        return tx_buffer.size - tx_buffer.tail + tx_buffer.head;
    }
}

/**
 * @brief Put a byte into TX buffer.
 */
static bool tx_buffer_put(uint8_t const data)
{
    if (tx_buffer_is_full()) {
        /* Buffer full - optionally could overwrite oldest data */
        return false;
    }

    tx_buffer.buffer[tx_buffer.head] = data;
    tx_buffer.head = (tx_buffer.head + 1) % tx_buffer.size;
    return true;
}

/**
 * @brief Get number of bytes available in RX buffer.
 */
static uint32_t rx_buffer_available(void)
{
    /* Get current DMA write position */
    uint32_t dma_head = (
        UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx)
    );

    /* Update our cached head position */
    rx_dma_head = dma_head;

    /* Calculate available bytes */
    if (dma_head >= rx_buffer.tail) {
        return dma_head - rx_buffer.tail;
    } else {
        return UART_RX_BUFFER_SIZE - rx_buffer.tail + dma_head;
    }
}

/**
 * @brief Get a byte from RX buffer.
 */
static bool rx_buffer_get(uint8_t *const data)
{
    /* Check if data is available */
    if (rx_buffer_available() == 0) {
        return false;
    }

    /* Read byte and advance tail */
    *data = rx_buffer_data[rx_buffer.tail];
    rx_buffer.tail = (rx_buffer.tail + 1) % UART_RX_BUFFER_SIZE;
    return true;
}

/**
 * @brief Start transmission if not already in progress.
 */
static void start_transmission(void)
{
    if (tx_in_progress || tx_buffer_is_empty()) {
        return;
    }

    /* Calculate how many contiguous bytes we can send */
    uint32_t available = tx_buffer_available();
    uint32_t contiguous_bytes;

    if (tx_buffer.tail <= tx_buffer.head) {
        /* No wrap-around: send from tail to head */
        contiguous_bytes = tx_buffer.head - tx_buffer.tail;
    } else {
        /* Wrap-around: send from tail to end of buffer */
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

        /* Start DMA transmission */
        HAL_UART_Transmit_DMA(
            &huart, &tx_buffer.buffer[tx_buffer.tail],
            contiguous_bytes
        );
    }
}

/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_init(void)
{
    HAL_StatusTypeDef status;

    huart.Instance = USART3;
    huart.Init.BaudRate = 115200;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;

    status = HAL_UART_Init(&huart);
    if (status != HAL_OK) {
        return status;
    }

    /* Start DMA reception
     * Note: Although DMA is in normal mode, we implement circular behavior
     * by restarting transfers in HAL_UART_RxCpltCallback */
    HAL_UART_Receive_DMA(&huart, rx_buffer_data, UART_RX_BUFFER_SIZE);

    /* Enable UART idle line interrupt for packet detection */
    __HAL_UART_ENABLE_IT(&huart, UART_IT_IDLE);

    /* Clear any pending flags */
    __HAL_UART_CLEAR_FLAG(&huart, UART_FLAG_IDLE);
    __HAL_UART_CLEAR_FLAG(&huart, UART_FLAG_ORE);

    return HAL_OK;
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
 * @return HAL_OK on success, HAL_ERROR if buffer is full.
 */
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint32_t const sz)
{
    if (txbuf == NULL || sz == 0) {
        return HAL_ERROR;
    }

    /* Queue bytes for transmission */
    for (uint32_t i = 0; i < sz; ++i) {
        if (!tx_buffer_put(txbuf[i])) {
            /* Buffer full - could not queue all bytes */
            return HAL_ERROR;
        }
    }

    /* Start transmission if not already in progress */
    start_transmission();

    return HAL_OK;
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

    uint32_t bytes_read = 0;
    uint8_t data;

    /* Read bytes from receive buffer */
    while (bytes_read < sz && rx_buffer_get(&data)) {
        rxbuf[bytes_read] = data;
        bytes_read++;
    }

    return bytes_read;
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
void HAL_UART_MspInit(__attribute__((unused))UART_HandleTypeDef *const huart)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    GPIO_InitTypeDef uart_gpio_init = {
        .Pin = GPIO_PIN_8 | GPIO_PIN_9,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = GPIO_AF7_USART3,
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
        DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0
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
        DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0
    );
    hdma_usart3_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    /* Use normal mode, manage circular manually */
    hdma_usart3_rx.Init.Mode = DMA_NORMAL;

    HAL_DMA_Init(&hdma_usart3_rx);

    /* Link DMA to UART */
    __HAL_LINKDMA(huart, hdmatx, hdma_usart3_tx);
    __HAL_LINKDMA(huart, hdmarx, hdma_usart3_rx);

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
    if (
        __HAL_UART_GET_FLAG(&huart, UART_FLAG_IDLE)
        && __HAL_UART_GET_IT_SOURCE(&huart, UART_IT_IDLE)
    ) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart);

        /* Check if callback should be triggered */
        if (rx_callback != NULL && rx_threshold > 0) {
            uint32_t available = rx_buffer_available();
            if (available >= rx_threshold) {
                rx_callback(available);
            }
        }
    }

    /* Handle other interrupts through HAL */
    HAL_UART_IRQHandler(&huart);
}

/**
 * @brief TX Complete callback from HAL.
 *
 * This function is called when a DMA transmission is complete.
 * It updates the buffer pointers and starts the next transmission if available.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *const huart_ptr)
{
    if (huart_ptr->Instance == USART3) {
        /* Update buffer tail pointer */
        tx_buffer.tail = (tx_buffer.tail + tx_dma_size) % tx_buffer.size;

        /* Mark transmission as complete */
        tx_in_progress = false;
        tx_dma_size = 0;

        /* Start next transmission if data is available */
        start_transmission();
    }
}

/**
 * @brief RX Complete callback from HAL.
 *
 * This function is called when the RX DMA transfer completes (head reaches
 * end of buffer). It restarts the DMA transfer to continue reception.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *const huart_ptr)
{
    if (huart_ptr->Instance == USART3) {
        /* Reset head position to wraparound buffer */
        rx_dma_head = 0;

        /* Restart DMA reception */
        HAL_UART_Receive_DMA(&huart, rx_buffer_data, UART_RX_BUFFER_SIZE);
    }
}

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * This function sets up a callback that will be called from the interrupt
 * handler when the specified number of bytes becomes available in the RX
 * buffer.
 *
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void UART_set_rx_callback(
    uart_rx_callback_t const callback,
    uint32_t const threshold
) {
    rx_callback = callback;
    rx_threshold = threshold;
}

/**
 * @brief DMA Channel 0 interrupt handler for UART TX.
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

/**
 * @brief DMA Channel 1 interrupt handler for UART RX.
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}
