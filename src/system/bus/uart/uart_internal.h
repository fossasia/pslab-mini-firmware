#ifndef PSLAB_UART_INTERNAL_H
#define PSLAB_UART_INTERNAL_H

/**
 * @brief Initialize the internal UART buffers.
 * Called from UART_init() in h563xx/uart.c
 */
void UART_buffer_init(void);

/**
 * @brief Callback for completed transmissions
 * @param bytes_transferred Number of bytes successfully transmitted
 */
void UART_tx_complete_callback(uint32_t bytes_transferred);

/**
 * @brief Callback for completed reception
 * @param buffer_size Size of the buffer used for reception
 */
void UART_rx_complete_callback(uint32_t buffer_size);

/**
 * @brief Callback for UART idle detection
 * @param dma_pos Current DMA position in reception buffer
 */
void UART_idle_callback(uint32_t dma_pos);

#endif /* PSLAB_UART_INTERNAL_H */
