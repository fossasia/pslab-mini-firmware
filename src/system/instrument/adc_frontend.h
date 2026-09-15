#ifndef ADC_FRONTEND_H
#define ADC_FRONTEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ADC_FRONTEND_BACKEND_NONE = 0,
    ADC_FRONTEND_BACKEND_INTERNAL = 1,
    ADC_FRONTEND_BACKEND_EXTERNAL_PARALLEL = 2,
} AdcFrontendBackend;

/**
 * @brief Static capability description for an ADC frontend backend.
 *
 * The driver owns any string pointers stored here. Callers receive a copy from
 * adc_frontend_get_capabilities() and must not modify pointed-to storage.
 */
typedef struct {
    AdcFrontendBackend backend;
    char const *name;
    uint32_t max_channel;
    uint32_t min_sample_rate_hz;
    uint32_t max_sample_rate_hz;
    uint32_t max_sample_count;
    uint32_t sample_width_bits;
} AdcFrontendCapabilities;

/**
 * @brief ADC frontend runtime configuration.
 */
typedef struct {
    AdcFrontendBackend backend;
    uint32_t channel;
    uint32_t sample_rate_hz;
} AdcFrontendConfig;

/**
 * @brief Metadata describing a completed or armed frontend capture.
 */
typedef struct {
    AdcFrontendBackend backend;
    uint32_t channel;
    uint32_t gpio;
    uint32_t sample_rate_hz;
    uint32_t sample_count;
    uint32_t sample_width_bits;
} AdcFrontendCaptureInfo;

/**
 * @brief Optional callback called after a capture finishes successfully.
 *
 * The callback pointer and user data are borrowed; they must remain valid until
 * changed by adc_frontend_set_complete_callback().
 */
typedef void (*AdcFrontendCompleteCallback)(
    AdcFrontendCaptureInfo const *info,
    void *user_data
);

/**
 * @brief Driver operations for an ADC frontend backend.
 *
 * The driver object and all pointed-to functions/storage must have static
 * lifetime after registration. Buffer ownership always remains with the caller.
 */
typedef struct {
    AdcFrontendBackend backend;
    char const *name;
    AdcFrontendCapabilities capabilities;
    bool (*init)(AdcFrontendConfig const *config);
    void (*deinit)(void);
    bool (*configure)(AdcFrontendConfig const *config);
    bool (*run)(
        uint16_t *buffer,
        uint32_t sample_count,
        AdcFrontendCaptureInfo *info
    );
    bool (*arm)(
        uint16_t *buffer,
        uint32_t sample_count,
        AdcFrontendCaptureInfo *info
    );
    bool (*start)(void);
    bool (*wait)(void);
    void (*abort)(void);
    bool (*read_once)(uint16_t *sample);
    bool (*is_busy)(void);
    uint32_t (*channel_to_gpio)(uint32_t channel);
} AdcFrontendDriver;

/**
 * @brief Register an ADC frontend driver.
 *
 * @return true when the driver was accepted, false for invalid, duplicate, or
 *         full registry conditions.
 */
bool adc_frontend_register_driver(AdcFrontendDriver const *driver);

/**
 * @brief Select the active backend, deinitializing the previous active backend.
 *
 * @return true if the requested backend is registered.
 */
bool adc_frontend_select_backend(AdcFrontendBackend backend);

/**
 * @brief Get the currently selected backend.
 */
AdcFrontendBackend adc_frontend_get_backend(void);

/**
 * @brief Copy backend capabilities into the caller-provided struct.
 *
 * @return true if the backend exists and capabilities is not NULL.
 */
bool adc_frontend_get_capabilities(
    AdcFrontendBackend backend,
    AdcFrontendCapabilities *capabilities
);

/**
 * @brief Initialize the selected backend using config.
 *
 * @return true on success. The backend from config becomes active.
 */
bool adc_frontend_init(AdcFrontendConfig const *config);

/**
 * @brief Deinitialize the active backend.
 */
void adc_frontend_deinit(void);

/**
 * @brief Configure the requested backend, initializing it if needed.
 *
 * @return true on success. The backend from config becomes active.
 */
bool adc_frontend_configure(AdcFrontendConfig const *config);

/**
 * @brief Run a complete blocking burst capture.
 *
 * The caller owns buffer and it must hold sample_count uint16_t samples.
 * Optional info receives capture metadata.
 *
 * @return true if the capture completed successfully.
 */
bool adc_frontend_run(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcFrontendCaptureInfo *info
);

/**
 * @brief Arm a burst capture without starting it.
 *
 * The caller owns buffer and must keep it valid until wait/abort completes.
 * Optional info receives armed capture metadata.
 *
 * @return true when the backend is armed.
 */
bool adc_frontend_arm(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcFrontendCaptureInfo *info
);

/**
 * @brief Start a previously armed capture.
 *
 * @return true if the backend started.
 */
bool adc_frontend_start(void);

/**
 * @brief Wait for the active capture to complete.
 *
 * @return true if the capture completed successfully.
 */
bool adc_frontend_wait(void);

/**
 * @brief Abort an active or armed capture.
 */
void adc_frontend_abort(void);

/**
 * @brief Read one immediate sample from the active backend.
 *
 * @return true if sample was written.
 */
bool adc_frontend_read_once(uint16_t *sample);

/**
 * @brief Query whether the active backend is busy.
 */
bool adc_frontend_is_busy(void);

/**
 * @brief Convert a backend channel index to a physical GPIO when applicable.
 *
 * @return GPIO number, or 0 if the backend is unknown.
 */
uint32_t adc_frontend_channel_to_gpio(
    AdcFrontendBackend backend,
    uint32_t channel
);

/**
 * @brief Set the optional successful-capture callback.
 */
void adc_frontend_set_complete_callback(
    AdcFrontendCompleteCallback callback,
    void *user_data
);

#ifdef __cplusplus
}
#endif

#endif
