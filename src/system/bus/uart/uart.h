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
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint32_t sz);

/**
 * @brief Receive data from UART.
 *
 * @param rxbuf Pointer to buffer to store received data.
 * @param sz    Number of bytes to read.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read(uint8_t *rxbuf, uint32_t sz);

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
 * @brief Check if the UART transmit buffer is ready to send data.
 *
 * This function returns true if the UART transmit data register is empty,
 * indicating that it is ready to accept new data for transmission.
 *
 * @return true if the transmit buffer is ready, false otherwise.
 */
bool UART_tx_ready(void);

/**
 * @brief Initialize the DMA for UART RX.
 *
 * This function configures the DMA channel for receiving data over UART.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef hdma_usart3_rx_init(void);

/**
 * @brief Initialize the DMA for UART TX.
 *
 * This function configures the DMA channel for transmitting data over UART.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef hdma_usart3_tx_init(void);


/**
 * @brief Receive data from UART using DMA.
 *
 * This function initiates a DMA transfer to receive a specified number of bytes
 * into the provided buffer over the UART peripheral.
 *
 * @param rxbuf Pointer to allocated memory into which to read data.
 * @param sz    Number of bytes to read into the buffer.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read_DMA(uint8_t *rxbuf, uint32_t sz);

/**
 * @brief Transmit data over UART using DMA.
 *
 * This function initiates a DMA transfer to transmit a specified number of bytes
 * from the provided buffer over the UART peripheral.
 *
 * @param txbuf Pointer to the data buffer to be transmitted.
 * @param sz    Number of bytes to write from the buffer.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_write_DMA(uint8_t const *txbuf, uint32_t sz);


#ifdef __cplusplus
}
#endif

#endif /* UART_H */