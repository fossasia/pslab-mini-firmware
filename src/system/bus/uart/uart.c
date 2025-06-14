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
