/**
 * @file    spi_ll.c
 * @brief   SPI system level implementation
 *
 *
 * @author  Tejas Garg
 * @date    2024-10-31
 */
#ifndef SPI_H
#define SPI_H

#include <stddef.h>
#include <stdint.h>

#include "util/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI bus handle structure
 */
typedef struct SPI_Handle SPI_Handle;

/**
 * @brief Initialize the SPI peripheral.
 *
 * @param bus SPI bus instance to initialize
 */
SPI_Handle *SPI_init(size_t bus);

/**
 * @brief Deinitialize the SPI peripheral.
 *
 * @param bus SPI bus instance to deinitialize
 */
void SPI_deinit(SPI_Handle *handle);

/**
 * @brief Transmit data over SPI.
 *
 * @param SPI_Handle SPI Handle Pointer
 * @param txbuf Pointer to the data buffer to transmit
 * @param sz Size of the data buffer
 */
void SPI_transmit(SPI_Handle *handle, uint8_t const *txbuf, size_t sz);

/**
 * @brief Receive data over SPI.
 *
 * @param SPI_Handle SPI Handle Pointer
 * @param rxbuf Pointer to the buffer to store received data
 * @param sz Size of the data buffer
 */
void SPI_receive(SPI_Handle *handle, uint8_t *rxbuf, size_t sz);

/**
 * @brief Transmit and receive data over SPI.
 *
 * @param SPI_Handle SPI Handle Pointer
 * @param tx_data Pointer to the data buffer to transmit.
 * @param rx_data Pointer to the buffer to store received data.
 * @param size Size of the data buffer.
 */
void SPI_transmit_receive(
    SPI_Handle *handle,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t size
);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */