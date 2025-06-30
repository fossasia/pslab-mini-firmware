/**
 * @file uart_ll.h
 * @brief Low-level UART hardware interface
 *
 * This header provides the low-level hardware interface to the UART bus. It
 * defines the API for initializing the UART peripheral, managing DMA-based
 * transmission and reception, and registering callback functions for UART
 * events (TX complete, RX complete, idle line detection).
 */

#ifndef PSLAB_UART_LL_H
#define PSLAB_UART_LL_H

#include <stdint.h>

/**
 * @brief Initialize the UART peripheral and start DMA-based reception.
 *
 * @param rx_buf Pointer to the reception buffer for DMA
 * @param sz Size of the reception buffer in bytes
 */
void UART_LL_init(uint8_t *rx_buf, uint32_t sz);

/**
 * @brief Get the current DMA position in the RX buffer.
 *
 * @return The number of bytes received so far in the RX buffer
 */
uint32_t UART_LL_get_dma_position(void);

/**
 * @brief Check if a UART TX DMA transfer is currently in progress.
 *
 * @return true if a transmission is ongoing, false otherwise
 */
bool UART_LL_tx_busy(void);

/**
 * @brief Start a UART DMA transmission.
 *
 * @param buffer Pointer to the data to transmit
 * @param size Number of bytes to transmit
 */
void UART_LL_start_dma_tx(uint8_t *buffer, uint32_t size);

/**
 * @brief Callback function type for completed transmissions
 * @param bytes_transferred Number of bytes successfully transmitted
 */
typedef void (*UART_LL_tx_complete_callback_t)(uint32_t bytes_transferred);

/**
 * @brief Callback function type for completed reception
 */
typedef void (*UART_LL_rx_complete_callback_t)(void);

/**
 * @brief Callback function type for UART idle detection
 * @param dma_pos Current DMA position in reception buffer
 */
typedef void (*UART_LL_idle_callback_t)(uint32_t dma_pos);

/**
 * @brief Set the TX complete callback function
 * @param callback Callback function to call when TX is complete
 */
void UART_LL_set_tx_complete_callback(UART_LL_tx_complete_callback_t callback);

/**
 * @brief Set the RX complete callback function
 * @param callback Callback function to call when RX buffer is full
 */
void UART_LL_set_rx_complete_callback(UART_LL_rx_complete_callback_t callback);

/**
 * @brief Set the idle line callback function
 * @param callback Callback function to call when idle line is detected
 */
void UART_LL_set_idle_callback(UART_LL_idle_callback_t callback);

#endif /* PSLAB_UART_LL_H */
