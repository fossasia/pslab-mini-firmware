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

uint8_t tx_buffer[10] = {10,20,30,40,50,60,70,80,90,100};
uint8_t rx_buffer[10];

static UART_HandleTypeDef huart = { 0 };

DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

uint32_t rx_counter, tx_counter;

/**
 * @brief Callback function for UART receive complete interrupt.
 *
 * This function is called when a UART receive operation is completed.
 * It increments the receive counter to keep track of the number of received bytes.
 *
 * @param huart Pointer to the UART handle structure.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	rx_counter++;

}

/**
 * @brief Callback function for UART transmit complete interrupt.
 *
 * This function is called when a UART transmit operation is completed.
 * It increments the transmit counter to keep track of the number of transmitted bytes.
 *
 * @param huart Pointer to the UART handle structure.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    tx_counter++;
}

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

    __HAL_RCC_GPDMA1_CLK_ENABLE(); // Enable GPDMA1 clock
    
    // Initialize DMA for UART RX
    hdma_usart3_rx.Instance = GPDMA1_Channel0;
    hdma_usart3_rx.Init.Request = GPDMA1_REQUEST_USART3_RX;
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_usart3_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_usart3_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart3_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart3_rx.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;

    HAL_DMA_Init(&hdma_usart3_rx);
    huart.hdmarx = &hdma_usart3_rx;

    // Initialize DMA for UART TX    
    hdma_usart3_tx.Instance = GPDMA1_Channel1;
    hdma_usart3_tx.Init.Request = GPDMA1_REQUEST_USART3_TX;
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    hdma_usart3_tx.Init.DestInc = DMA_DINC_FIXED;
    hdma_usart3_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart3_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart3_tx.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;

    HAL_DMA_Init(&hdma_usart3_tx);
    huart.hdmatx = &hdma_usart3_tx;

    // Enable UART interrupts
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    // Enable DMA interrupts
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

    return HAL_UART_Init(&huart);

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
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint32_t const sz)
{
    return HAL_UART_Transmit(&huart, txbuf, sz, UART_TIMEOUT);
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
HAL_StatusTypeDef UART_read(uint8_t *const rxbuf, uint32_t const sz)
{
    return HAL_UART_Receive(&huart, rxbuf, sz, UART_TIMEOUT);
}


/**
 * @brief Read data from the UART interface using DMA.
 *
 * This function initiates a DMA transfer to receive a specified number of bytes
 * into the provided buffer over the UART peripheral.
 *
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read_DMA(uint8_t *const rxbuf, uint32_t const sz)
{
    return HAL_UART_Receive_DMA(&huart, rxbuf, sz);
}


/**
 * @brief Write data to the UART interface using DMA.
 *
 * This function initiates a DMA transfer to transmit a specified number of bytes
 * from the provided buffer over the UART peripheral.
 *
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_write_DMA(uint8_t const *txbuf, uint32_t const sz)
{
    return HAL_UART_Transmit_DMA(&huart, txbuf, sz);
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


/**
 * @brief  Initialize the IRQ handlers for the UART peripheral.
 *
 * @param None
 * 
 * @return None
 */
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart);
}

/**
 * @brief  Initialize the IRQ handlers for the UART RX DMA peripheral.
 *
 * @param None
 * 
 * @return None
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(huart.hdmarx);
}

/**
 * @brief  Initialize the IRQ handlers for the UART TX DMA peripheral.
 *
 * @param None
 * 
 * @return None
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(huart.hdmatx);
}