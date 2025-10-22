/**
 * @file    spi_ll.h
 * @brief   SPI low-level hardware interface for STM32H563xx
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include "stm32h5xx_hal.h"

/**
 * @brief SPI bus instance enumeration
 */
typedef enum { SPI_BUS_0 = 0, SPI_BUS_1 = 1, SPI_BUS_COUNT = 2 } SPI_Bus;

/**
 * @brief Initialize the SPI peripheral.
 */
void spi_ll_init(void);

/**
 * @brief Transmit data over SPI.
 * @param data Pointer to the data buffer to transmit.
 * @param size Size of the data buffer.
 */
void spi_ll_transmit(uint8_t *data, uint16_t size);

/**
 * @brief Receive data over SPI.
 * @param data Pointer to the buffer to store received data.
 * @param size Size of the data buffer.
 */
void spi_ll_receive(uint8_t *data, uint16_t size);

#endif /* SPI_LL_H */