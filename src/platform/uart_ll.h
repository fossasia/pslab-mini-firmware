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

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief UART bus instance enumeration
 */
typedef enum {
    UART_BUS_0 = 0,
    UART_BUS_1 = 1,
    UART_BUS_2 = 2,
    UART_BUS_COUNT = 3
} UART_Bus;

/**
 * @brief Initialize the UART peripheral and start DMA-based reception.
 *
 * @param bus UART bus instance to initialize
 * @param rx_buf Pointer to the reception buffer for DMA
 * @param sz Size of the reception buffer in bytes
 */
void UART_LL_init(UART_Bus bus, uint8_t *rx_buf, uint32_t sz);

/**
 * @brief Deinitialize the UART peripheral.
 *
 * @param bus UART bus instance to deinitialize
 */
void UART_LL_deinit(UART_Bus bus);

/**
 * @brief Get the current DMA position in the RX buffer.
 *
 * @param bus UART bus instance
 * @return The number of bytes received so far in the RX buffer
 */
uint32_t UART_LL_get_dma_position(UART_Bus bus);

/**
 * @brief Check if a UART TX DMA transfer is currently in progress.
 *
 * @param bus UART bus instance
 * @return true if a transmission is ongoing, false otherwise
 */
bool UART_LL_tx_busy(UART_Bus bus);

/**
 * @brief Start a UART DMA transmission.
 *
 * @param bus UART bus instance
 * @param buffer Pointer to the data to transmit
 * @param size Number of bytes to transmit
 */
void UART_LL_start_dma_tx(UART_Bus bus, uint8_t *buffer, uint32_t size);

/**
 * @brief Callback function type for completed transmissions
 * @param bus UART bus instance
 * @param bytes_transferred Number of bytes successfully transmitted
 */
typedef void (*UART_LL_TxCompleteCallback)(
    UART_Bus bus,
    uint32_t bytes_transferred
);

/**
 * @brief Callback function type for completed reception
 * @param bus UART bus instance
 */
typedef void (*UART_LL_RxCompleteCallback)(UART_Bus bus);

/**
 * @brief Callback function type for UART idle detection
 * @param bus UART bus instance
 * @param dma_pos Current DMA position in reception buffer
 */
typedef void (*UART_LL_IdleCallback)(UART_Bus bus, uint32_t dma_pos);

/**
 * @brief Set the TX complete callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when TX is complete
 */
void UART_LL_set_tx_complete_callback(
    UART_Bus bus,
    UART_LL_TxCompleteCallback callback
);

/**
 * @brief Set the RX complete callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when RX buffer is full
 */
void UART_LL_set_rx_complete_callback(
    UART_Bus bus,
    UART_LL_RxCompleteCallback callback
);

/**
 * @brief Set the idle line callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when idle line is detected
 */
void UART_LL_set_idle_callback(UART_Bus bus, UART_LL_IdleCallback callback);

#endif /* PSLAB_UART_LL_H */
