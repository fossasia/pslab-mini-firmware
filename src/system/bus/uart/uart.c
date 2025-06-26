/**
 * @file uart.c
 * @brief UART (Universal Asynchronous Receiver/Transmitter) module implementation.
 *
 * This module provides functions and routines for configuring and handling UART communication.
 * It includes initialization, data transmission, and reception functionalities for serial communication.
 *
 * @author Alexander Bessman
 * @date 2025-06-13
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"

#include "uart.h"


#define UART_TIMEOUT 100000 /** 100 ms */


static UART_HandleTypeDef huart = { 0 };

DMA_HandleTypeDef hdma_usart3_rx = { 0 };
DMA_HandleTypeDef hdma_usart3_tx = { 0 };


/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 *
 * @return 0 on success, non-zero value on failure.
 */
HAL_StatusTypeDef UART_init(void)
{
    huart.Instance = USART3;
    huart.Init.BaudRate = 115200;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;
    
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    return HAL_UART_Init(&huart);
}

/**
 * @brief Initialize the DMA for UART receive.
 *
 * This function configures the DMA channel used for UART reception,
 * setting up the necessary parameters and enabling the clock for the DMA.
 *
 * @return HAL status code indicating success or failure.
 */
HAL_StatusTypeDef hdma_usart3_rx_init(void){

    __HAL_RCC_GPDMA1_CLK_ENABLE();

    hdma_usart3_rx.Instance = GPDMA1_Channel0;
    hdma_usart3_rx.Init.Request = GPDMA1_REQUEST_USART3_RX;
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_usart3_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_usart3_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart3_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart3_rx.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    hdma_usart3_rx.Init.SrcBurstLength = 1;
    hdma_usart3_rx.Init.DestBurstLength = 1;
    hdma_usart3_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT1 |
                                                 DMA_DEST_ALLOCATED_PORT0;
    hdma_usart3_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_usart3_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_usart3_rx.Init.Mode = DMA_NORMAL;

    __HAL_LINKDMA(&huart,hdmarx, hdma_usart3_rx);
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    return HAL_DMA_Init(&hdma_usart3_rx);


}   

/**
 * @brief Initialize the DMA for UART transmit.
 *
 * This function configures the DMA channel used for UART transmission,
 * setting up the necessary parameters and enabling the clock for the DMA.
 *
 * @return HAL status code indicating success or failure.
 */
HAL_StatusTypeDef hdma_usart3_tx_init(void){

    __HAL_RCC_GPDMA1_CLK_ENABLE();

    hdma_usart3_tx.Instance = GPDMA1_Channel1;
    hdma_usart3_tx.Init.Request = GPDMA1_REQUEST_USART3_TX;
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    hdma_usart3_tx.Init.DestInc = DMA_DINC_FIXED;
    hdma_usart3_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart3_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart3_tx.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    hdma_usart3_tx.Init.SrcBurstLength = 1;
    hdma_usart3_tx.Init.DestBurstLength = 1;
    hdma_usart3_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 |
                                                 DMA_DEST_ALLOCATED_PORT1;
    hdma_usart3_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_usart3_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;

    __HAL_LINKDMA(&huart, hdmatx, hdma_usart3_tx);
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

    return HAL_DMA_Init(&hdma_usart3_tx);
}

/**
 * @brief Read data from the UART interface using DMA.
 *
 * This function initiates a DMA transfer to receive data from the UART peripheral
 * into the specified buffer. The transfer size is defined by the `datasize` parameter.
 *
 * @param rxBuffer   Pointer to the buffer where received data will be stored.
 * @param datasize   Number of bytes to read into the buffer.
 *
 * @return HAL status code indicating success or failure of the operation.
 */
HAL_StatusTypeDef usart3_dma_read(uint8_t* rxBuffer, uint16_t datasize)
{
    return HAL_UART_Receive_DMA(&huart, (uint8_t *)rxBuffer, datasize);
}

/**
 * @brief Write data to the UART interface using DMA.
 *
 * This function initiates a DMA transfer to send data from the specified buffer
 * over the UART peripheral. The transfer size is defined by the `datasize` parameter.
 *
 * @param txBuffer   Pointer to the buffer containing data to be transmitted.
 * @param datasize   Number of bytes to write from the buffer.
 *
 * @return HAL status code indicating success or failure of the operation.
 */
HAL_StatusTypeDef usart3_dma_write(uint8_t* txBuffer, uint16_t datasize)
{
    if (txBuffer == NULL || datasize == 0) {
        return HAL_ERROR; // Invalid parameters
    }
    return HAL_UART_Transmit_DMA(&huart, (uint8_t *)txBuffer, datasize);
}


/**
 * @brief Write data to the UART interface.
 *
 * This function transmits a specified number of bytes from the provided buffer
 * over the UART peripheral.
 *
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return 0 on success, non-zero value on failure.
 */
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint16_t const sz)
{
    return HAL_UART_Transmit_IT(&huart, txbuf, sz);
}

/**
 * @brief Read data from the UART interface.
 *
 * This function receives a specified number of bytes into the provided buffer
 * over the UART peripheral.
 *
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return 0 on success, non-zero value on failure.
 */
HAL_StatusTypeDef UART_read(uint8_t *const rxbuf, uint16_t const sz)
{
    return HAL_UART_Receive_IT(&huart, rxbuf, sz);
}

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if the UART receive data register is not empty,
 * indicating that there is unread data available to be received.
 *
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(void)
{
    return __HAL_UART_GET_FLAG(&huart, UART_FLAG_RXNE);
}

/**
 * @brief  Initialize the UART MSP (MCU Support Package).
 *
 * This function configures the hardware resources used for the UART peripheral,
 * including GPIO pins and clocks. It is called by HAL_UART_Init().
 *
 * @param huart UART handle pointer.
 *
 * @return None
 */
void HAL_UART_MspInit(__attribute__((unused))UART_HandleTypeDef *huart)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    GPIO_InitTypeDef uart_gpio_init = {
        .Pin = GPIO_PIN_8 | GPIO_PIN_9,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = GPIO_AF7_USART3,
    };
    HAL_GPIO_Init(GPIOD, &uart_gpio_init);
}

void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart);
}

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(huart.hdmarx);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(huart.hdmatx);
}
