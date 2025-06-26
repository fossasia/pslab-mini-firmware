#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UART peripheral.
 *
 * Configures baud rate, data bits, stop bits, parity and enables TX/RX.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_init(void);

/**
 * @brief Transmit data over UART.
 *
 * @param txbuf Pointer to data buffer to transmit.
 * @param sz    Number of bytes to send.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint16_t sz);

/**
 * @brief Receive data from UART.
 *
 * @param rxbuf Pointer to buffer to store received data.
 * @param sz    Number of bytes to read.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read(uint8_t *rxbuf, uint16_t sz);

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if the UART receive data register is not empty,
 * indicating that there is unread data available to be received.
 *
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(void);

/**
 * @brief Initialize the DMA for USART3 RX.
 *
 * Configures the DMA channel used for USART3 reception,
 * setting up the necessary parameters and enabling the clock for the DMA.
 *
 * @return HAL status code indicating success or failure.
 */
HAL_StatusTypeDef hdma_usart3_rx_init(void);

/**
 * @brief Initialize the DMA for USART3 TX.
 *
 * Configures the DMA channel used for USART3 transmission,
 * setting up the necessary parameters and enabling the clock for the DMA.
 *
 * @return HAL status code indicating success or failure.
 */
HAL_StatusTypeDef hdma_usart3_tx_init(void);

/**
 * @brief Read data from USART3 using DMA.
 *
 * This function receives a specified number of bytes into the provided buffer
 * over the USART3 peripheral using DMA.
 *
 * @param rxBuffer Pointer to allocated memory into which to read data.
 * @param datasize Number of bytes to read into the buffer.
 *
 * @return HAL status code indicating success or failure of the operation.
 */
HAL_StatusTypeDef usart3_dma_read(uint8_t* rxBuffer, uint16_t datasize);

/**
 * @brief Write data to USART3 using DMA.
 *
 * This function initiates a DMA transfer to send data from the specified buffer
 * over the USART3 peripheral.
 *
 * @param txBuffer Pointer to the buffer containing data to be transmitted.
 * @param datasize Number of bytes to write from the buffer.
 *
 * @return HAL status code indicating success or failure of the operation.
 */
HAL_StatusTypeDef usart3_dma_write(uint8_t* txBuffer, uint16_t datasize);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
