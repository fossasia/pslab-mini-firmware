/**
 * @file dso.h
 * @brief Digital Storage Oscilloscope interface for PSLab firmware
 *
 * This header provides a simple digital storage oscilloscope implementation
 * using the ADC_LL API in continuous sampling mode, in either dual-channel or
 * interleaved mode.
 *
 * @author PSLab Team
 * @date 2025-09-29
 */

#ifndef PSLAB_DSO_H
#define PSLAB_DSO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DSO handle structure (opaque)
 */
typedef struct DSO_Handle DSO_Handle;

/**
 * @brief DSO channel enumeration
 *
 * This enum provides a system-level abstraction for ADC channels,
 * hiding the low-level ADC_LL_Channel implementation details.
 *
 * Since the oscilloscope functionality is tightly tied to the analog
 * front-end present on specific pins, only two channels are available.
 */
typedef enum {
    DSO_CHANNEL_0 = 0,
    DSO_CHANNEL_1 = 1,
} DSO_Channel;

typedef enum {
    DSO_MODE_SINGLE_CHANNEL,
    DSO_MODE_DUAL_CHANNEL,
} DSO_Mode;

/**
 * @brief DSO completion callback type
 *
 * This callback is invoked when the DSO data acquisition is complete.
 */
typedef void (*DSO_CompleteCallback)(void);

/**
 * @brief DSO configuration structure
 */
typedef struct {
    DSO_Mode mode; /**< Dual-channel or interleaved mode */
    DSO_Channel channel; /**< Input channel for interleaved mode */
    uint32_t sample_rate; /**< Sample rate in Hz */
    uint16_t *buffer; /**< Pointer to the buffer for storing samples */
    uint32_t buffer_size; /**< Size of the buffer */
    DSO_CompleteCallback
        complete_callback; /**< Callback invoked on completion */
} DSO_Config;

/**
 * @brief Default DSO configuration
 */
#define DSO_CONFIG_DEFAULT                                                     \
    {                                                                          \
        .mode = DSO_MODE_SINGLE_CHANNEL, .channel = DSO_CHANNEL_0,             \
        .sample_rate = 1000000, .buffer = nullptr, .buffer_size = 256,         \
        .complete_callback = nullptr,                                          \
    }

/**
 * @brief Initialize the Oscilloscope
 *
 * This function initializes the DSO subsystem with the given configuration.
 * It validates the configuration parameters and sets up the ADC for the
 * specified mode and channel.
 *
 * @param config Pointer to DSO configuration structure
 * @return Pointer to DSO handle on success, NULL on failure
 *
 * @throws ERROR_INVALID_ARGUMENT if config is NULL or contains invalid values
 * @throws ERROR_OUT_OF_MEMORY if memory allocation fails
 * @throws ERROR_HARDWARE_FAULT if ADC initialization fails
 */
DSO_Handle *DSO_init(DSO_Config const *config);

/**
 * @brief Deinitialize the Oscilloscope
 *
 * This function deinitializes the DSO subsystem, releases hardware resources,
 * and frees the handle memory. The handle becomes invalid after this call;
 * reusing it without reinitialization causes undefined behavior.
 *
 * @param handle Pointer to DSO handle to deinitialize
 */
void DSO_deinit(DSO_Handle *handle);

/**
 * @brief Start the Oscilloscope
 *
 * This function starts the DSO data acquisition process. It starts the timer
 * with the configured sample rate, triggering the ADC to capture data until
 * the buffer is full.
 *
 * @param handle Pointer to DSO handle
 *
 * @throws ERROR_INVALID_ARGUMENT if handle is NULL
 * @throws ERROR_DEVICE_NOT_READY if DSO is not initialized
 */
void DSO_start(DSO_Handle *handle);

/**
 * @brief Stop the Oscilloscope
 *
 * This function stops the DSO data acquisition process without deinitializing
 * it. It stops the timer and the ADC, preventing any further data capture.
 *
 * @param handle Pointer to DSO handle
 *
 * @throws ERROR_INVALID_ARGUMENT if handle is NULL
 * @throws ERROR_DEVICE_NOT_READY if DSO is not initialized
 */
void DSO_stop(DSO_Handle *handle);

/**
 * @brief Get current DSO configuration
 *
 * This function returns the currently applied configuration of the DSO.
 *
 * @param handle Pointer to DSO handle
 * @return Copy of the current DSO configuration
 *
 * @throws ERROR_INVALID_ARGUMENT if handle is NULL
 * @throws ERROR_DEVICE_NOT_READY if DSO is not initialized
 */
DSO_Config DSO_get_config(DSO_Handle *handle);

/**
 * @brief Set DSO configuration
 *
 * This function updates the DSO configuration. The configuration update
 * is not allowed if data acquisition is currently ongoing.
 *
 * @param handle Pointer to DSO handle
 * @param config Pointer to new DSO configuration
 *
 * @throws ERROR_INVALID_ARGUMENT if handle or config is NULL or contains
 * invalid values
 * @throws ERROR_DEVICE_NOT_READY if DSO is not initialized
 * @throws ERROR_RESOURCE_BUSY if data acquisition is currently running
 */
void DSO_set_config(DSO_Handle *handle, DSO_Config const *config);

/**
 * @brief Get maximum sample rate for a given DSO mode
 *
 * This function returns the maximum achievable sample rate for the specified
 * DSO mode, which depends on the underlying ADC capabilities.
 *
 * @param mode DSO mode (single or dual channel)
 * @return Maximum sample rate in Hz for the specified mode
 */
uint32_t DSO_get_max_sample_rate(DSO_Mode mode);

/**
 * @brief Check if DSO acquisition is in progress
 *
 * This function checks if the DSO is currently acquiring data.
 *
 * @param handle Pointer to DSO handle
 * @return true if acquisition is in progress, false otherwise
 *
 * @throws ERROR_INVALID_ARGUMENT if handle is NULL
 * @throws ERROR_DEVICE_NOT_READY if DSO is not initialized
 */
bool DSO_is_acquisition_in_progress(DSO_Handle *handle);

#ifdef __cplusplus
}
#endif

#endif // PSLAB_DSO_H
