/**
 * @file linear_buffer.c
 * @brief Linear buffer implementation
 *
 * This module provides a linear buffer implementation for storing and
 * processing data.
 *
 * @author Tejas Garg
 * @date 2025-07-18
 */

#include "linear_buffer.h"
#include "error.h"

void linear_buffer_init(
    LinearBuffer *linear_buffer,
    uint32_t *buffer,
    uint32_t size
)
{
    if (!linear_buffer || !buffer) {
        THROW(ERROR_INVALID_ARGUMENT);
        return; // Invalid parameters
    }

    if (size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return; // Size must be greater than zero
    }

    linear_buffer->buffer = buffer;
    linear_buffer->size = size;
}

uint32_t linear_buffer_read(
    LinearBuffer *linear_buffer,
    uint32_t *data,
    uint32_t size
)
{
    if (!linear_buffer || !data || size == 0) {
        return 0;
    }

    uint32_t bytes_read = 0;
    while (bytes_read < size) {
        *data = linear_buffer->buffer[bytes_read];
        bytes_read++;
    }

    return bytes_read;
}
