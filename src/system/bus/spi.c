/**
 * @file    spi_ll.c
 * @brief   SPI system level implementation
 *
 *
 * @author  Tejas Garg
 * @date    2024-10-31
 */

#include "platform/spi_ll.h"
#include "util/error.h"

#include "spi.h"

struct SPI_Handle {
    SPI_Bus bus_id;
    bool initialized;
};

static SPI_Handle *g_spi_instances[SPI_BUS_COUNT] = { nullptr };

/**
 * @brief Initialize the SPI peripheral.
 *
 * @param bus SPI bus instance to initialize
 */

SPI_Handle *SPI_init(size_t bus)
{
    if (bus >= SPI_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    SPIInstance *instance = &g_spi_instances[bus];
    if (instance->initialized) {
        return;
    }
    /* Allocate handle */
    SPI_Handle *handle = malloc(sizeof(SPI_Handle));
    if (!handle) {
        THROW(ERROR_OUT_OF_MEMORY);
    }

    /* Initialize SPI */
    SPI_LL_init(bus);
    instance->initialized = true;

    return handle;
}

/**
 * @brief Deinitialize the SPI peripheral.
 *
 * @param bus SPI bus instance to deinitialize
 */

void SPI_deinit(SPI_Handle *handle)
{
    if (handle == NULL) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->initialized) {
        return;
    }

    /* Deinitialize SPI */
    SPI_LL_deinit(handle->bus_id);
    handle->initialized = false;

    /* Free handle */
    free(handle);
}

/**
 * @brief Transmit data over SPI.
 *
 * @param SPI_Handle SPI Handle Pointer
 * @param txbuf Pointer to the data buffer to transmit
 * @param sz Size of the data buffer
 */
void SPI_transmit(SPI_Handle *handle, const uint8_t *txbuf, size_t sz)
{
    if (handle == NULL || txbuf == NULL || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
    }

    SPI_LL_transmit(handle->bus_id, txbuf, sz);
}

/**
 * @brief Receive data over SPI.
 *
 * @param SPI_Handle SPI Handle Pointer
 * @param rxbuf Pointer to the buffer to store received data
 * @param sz Size of the data buffer
 */
void SPI_receive(SPI_Handle *handle, uint8_t *rxbuf, size_t sz)
{
    if (handle == NULL || rxbuf == NULL || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
    }

    SPI_LL_receive(handle->bus_id, rxbuf, sz);
}

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
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t size
)
{
    if (handle == NULL || tx_data == NULL || rx_data == NULL || size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
    }

    SPI_LL_transmit_receive(handle->bus_id, tx_data, rx_data, size);
}