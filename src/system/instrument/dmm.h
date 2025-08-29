/**
 * @file dmm.h
 * @brief Digital Multimeter interface for PSLab firmware
 *
 * This header provides a simple digital multimeter implementation using
 * the ADC_LL API in single sample mode. It allows reading voltage values from
 * ADC channels with proper error handling and logging.
 *
 * @author PSLab Team
 * @date 2025-07-18
 */

#ifndef PSLAB_DMM_H
#define PSLAB_DMM_H

#include <stdint.h>

#include "error.h"
#include "fixed_point.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMM handle structure (opaque)
 */
typedef struct DMM_Handle DMM_Handle;

/**
 * @brief DMM channel enumeration
 *
 * This enum provides a system-level abstraction for ADC channels,
 * hiding the low-level ADC_LL_Channel implementation details.
 */
typedef enum {
    DMM_CHANNEL_0 = 0,
    DMM_CHANNEL_1 = 1,
    DMM_CHANNEL_2 = 2,
    DMM_CHANNEL_3 = 3,
    DMM_CHANNEL_4 = 4,
    DMM_CHANNEL_5 = 5,
    DMM_CHANNEL_6 = 6,
    DMM_CHANNEL_7 = 7,
    DMM_CHANNEL_8 = 8,
    DMM_CHANNEL_9 = 9,
    DMM_CHANNEL_10 = 10,
    DMM_CHANNEL_11 = 11,
    DMM_CHANNEL_12 = 12,
    DMM_CHANNEL_13 = 13,
    DMM_CHANNEL_14 = 14,
    DMM_CHANNEL_15 = 15
} DMM_Channel;

/**
 * @brief DMM configuration structure
 */
typedef struct {
    DMM_Channel channel; // ADC channel to use for measurements
    uint32_t oversampling_ratio; // Oversampling ratio (1, 2, 4, 8, 16, 32, 64,
                                 // 128, 256)
} DMM_Config;

/**
 * @brief Default DMM configuration
 */
#define DMM_CONFIG_DEFAULT                                                     \
    {                                                                          \
        .channel = DMM_CHANNEL_0, .oversampling_ratio = 16                     \
    }

/**
 * @brief Initialize the Digital Multimeter
 *
 * This function initializes the DMM subsystem with the given configuration.
 * It sets up the ADC in single sample mode for continuous voltage measurements
 * and starts the first conversion.
 *
 * @param config Pointer to DMM configuration structure
 * @return Pointer to DMM handle on success, NULL on failure
 *
 * @throws ERROR_INVALID_ARGUMENT if config is NULL or contains invalid values
 * @throws ERROR_OUT_OF_MEMORY if memory allocation fails
 * @throws ERROR_HARDWARE_FAULT if ADC initialization fails
 */
DMM_Handle *DMM_init(DMM_Config const *config);

/**
 * @brief Deinitialize the Digital Multimeter
 *
 * This function deinitializes the DMM subsystem, releases hardware resources,
 * and frees the handle memory. The handle becomes invalid after this call.
 *
 * @param handle Pointer to DMM handle to deinitialize
 */
void DMM_deinit(DMM_Handle *handle);

/**
 * @brief Read voltage from the configured ADC channel
 *
 * This function returns the most recent voltage measurement and indicates
 * whether the value is valid. If a valid measurement is available, it
 * starts the next conversion automatically for continuous operation.
 *
 * @param handle Pointer to DMM handle
 * @param voltage_out Pointer to store the measured voltage (in volts,
 * fixed-point)
 * @return true if voltage_out contains a valid measurement, false otherwise
 *
 * @throws ERROR_INVALID_ARGUMENT if handle or voltage_out is NULL
 * @throws ERROR_DEVICE_NOT_READY if DMM is not initialized
 */
bool DMM_read_voltage(DMM_Handle *handle, FIXED_Q1616 *voltage_out);

#ifdef __cplusplus
}
#endif

#endif // PSLAB_DMM_H
