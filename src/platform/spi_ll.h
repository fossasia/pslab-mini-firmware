/**
 * @file    spi_ll.h
 * @brief   SPI low-level hardware interface for STM32H563xx
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief SPI bus instance enumeration
 */
typedef enum { SPI_BUS_0 = 0, SPI_BUS_COUNT = 1 } SPI_Bus;

/**
 * @brief Initialize the SPI peripheral.
 */
void SPI_LL_init(SPI_Bus bus);

/**
 * @brief Deinitialize the SPI peripheral.
 * @param bus SPI bus instance to deinitialize.
 */
void SPI_LL_deinit(SPI_Bus bus);

/**
 * @brief Transmit data over SPI.
 * @param txdata Pointer to the data buffer to transmit.
 * @param size Size of the data buffer.
 */
void SPI_LL_transmit(SPI_Bus bus, uint8_t const *txdata, size_t size);

/**
 * @brief Receive data over SPI.
 * @param rxdata Pointer to the buffer to store received data.
 * @param size Size of the data buffer.
 */
void SPI_LL_receive(SPI_Bus bus, uint8_t *rxdata, size_t size);

/**
 * @brief Transmit and receive data over SPI.
 * @param tx_data Pointer to the data buffer to transmit.
 * @param rx_data Pointer to the buffer to store received data.
 * @param size Size of the data buffer.
 */
void SPI_LL_transmit_receive(
    SPI_Bus bus,
    uint8_t const *tx_data,
    uint8_t *rx_data,
    size_t size
);

#endif /* SPI_LL_H */