/**
 * @file dso.c
 * @brief DSO-specific SCPI commands implementation
 *
 * This module implements the SCPI commands specific to Digital Storage
 * Oscilloscope functionality, including configuration, triggering, and
 * data acquisition.
 */

#include <stdio.h>
#include <string.h>

#include "lib/scpi/error.h"
#include "lib/scpi/scpi.h"

#include "system/instrument/dso.h"
#include "system/system.h"
#include "util/error.h"
#include "util/si_prefix.h"
#include "util/util.h"

enum {
    TIMEBASE_DEFAULT = 100, // 100 µs / div
    BUFFER_SIZE_DEFAULT = 512,
    HORIZONTAL_DIVISIONS = 10, // Standard oscilloscope divisions
};

// DSO state (internal to this module)
static struct {
    DSO_Handle *dso_handle;
    uint16_t *acquisition_buffer;
    uint32_t acquisition_buffer_size;
    uint32_t timebase_us;
    bool acquisition_complete;
} g_dso_state = {
    .dso_handle = nullptr,
    .acquisition_buffer = nullptr,
    .acquisition_buffer_size = 0,
    .timebase_us = TIMEBASE_DEFAULT,
    .acquisition_complete = false,
};

/**
 * @brief DSO completion callback - called when acquisition is complete
 */
void dso_complete_callback(void) { g_dso_state.acquisition_complete = true; }

/**
 * @brief Reset DSO state to default values
 */
void dso_reset_state(void)
{
    // Stop any ongoing acquisition
    if (g_dso_state.dso_handle) {
        DSO_stop(g_dso_state.dso_handle);
        DSO_deinit(g_dso_state.dso_handle);
        g_dso_state.dso_handle = nullptr;
    }

    // Free acquisition buffer if allocated
    if (g_dso_state.acquisition_buffer) {
        free(g_dso_state.acquisition_buffer);
        g_dso_state.acquisition_buffer = nullptr;
    }

    g_dso_state.acquisition_buffer_size = 0;
    g_dso_state.timebase_us = TIMEBASE_DEFAULT;
    g_dso_state.acquisition_complete = false;
}

/**
 * @brief Helper function to configure sample rate and buffer
 *
 * This function handles the common logic for sample rate calculation,
 * validation, and buffer allocation used by both timebase and acquire
 * points commands.
 *
 * @param context SCPI context for error reporting
 * @param buffer_size Desired buffer size in samples
 * @param mode DSO mode to determine ADC mode for sample rate validation
 * @param[out] config DSO configuration to update
 * @return SCPI_RES_OK on success, SCPI_RES_ERR on failure
 */
static scpi_result_t configure_sample_rate_and_buffer(
    scpi_t *context,
    uint32_t buffer_size,
    DSO_Mode mode,
    DSO_Config *config
)
{
    // Calculate sample rate using current timebase
    // Total acquisition time = timebase_us * 10 divisions
    // sample_rate = buffer_size * 1,000,000 / (timebase_us * 10)
    uint32_t sample_rate = (buffer_size * SI_MICRO_DIV) /
                           (g_dso_state.timebase_us * HORIZONTAL_DIVISIONS);

    // Validate calculated sample rate (must be > 0)
    if (sample_rate == 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    // Check if sample rate is achievable with current DSO mode
    // Use the mode parameter (the new mode being set)
    uint32_t max_sample_rate = DSO_get_max_sample_rate(mode);

    if (sample_rate > max_sample_rate) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    // Allocate buffer if needed
    uint16_t *new_buffer = nullptr;

    if (!g_dso_state.acquisition_buffer ||
        g_dso_state.acquisition_buffer_size != buffer_size) {

        new_buffer = realloc(
            g_dso_state.acquisition_buffer, buffer_size * sizeof(uint16_t)
        );
        if (!new_buffer) {
            SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
            return SCPI_RES_ERR;
        }
    } else {
        new_buffer = g_dso_state.acquisition_buffer;
    }

    // Update configuration
    config->sample_rate = sample_rate;
    config->buffer = new_buffer;
    config->buffer_size = buffer_size;

    return SCPI_RES_OK;
}

/**
 * @brief Helper function to apply DSO configuration changes
 *
 * This function handles the common logic of either initializing a new DSO
 * handle or updating an existing one with new configuration parameters.
 *
 * @param context SCPI context for error reporting
 * @param new_config New configuration to apply
 * @return SCPI_RES_OK on success, SCPI_RES_ERR on failure
 */
static scpi_result_t apply_dso_config(scpi_t *context, DSO_Config *new_config)
{
    new_config->complete_callback = dso_complete_callback;
    Error err = ERROR_NONE;

    TRY
    {
        if (!g_dso_state.dso_handle) {
            // DSO is not initialized, initialize it
            g_dso_state.dso_handle = DSO_init(new_config);
        } else {
            // Update existing configuration
            DSO_set_config(g_dso_state.dso_handle, new_config);
        }
    }
    CATCH(err)
    {
        // If the new buffer is not the same as the acquisition buffer, free
        // it here. Otherwise, keep the existing buffer.
        if (new_config->buffer != g_dso_state.acquisition_buffer) {
            free(new_config->buffer);
        }

        switch (err) {
        case ERROR_INVALID_ARGUMENT:
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        case ERROR_RESOURCE_BUSY:
            SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
            return SCPI_RES_ERR;
        default:
            LOG_ERROR("DSO configuration error: 0x%08X", err);
            SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
            return SCPI_RES_ERR;
        }
    }

    // Update state - the buffer pointer may have changed due to realloc
    g_dso_state.acquisition_buffer = new_config->buffer;
    g_dso_state.acquisition_buffer_size = new_config->buffer_size;
    g_dso_state.acquisition_complete = false;

    return SCPI_RES_OK;
}

/**
 * @brief CONFigure:OSCilloscope:CHANnel - Set DSO channel
 *
 * Syntax: CONFigure:OSCilloscope:CHANnel {CH1|CH2|CH1CH2}
 */
scpi_result_t scpi_cmd_configure_oscilloscope_channel(scpi_t *context)
{
    enum { CHANNEL_CH1, CHANNEL_CH2, CHANNEL_CH1CH2 };
    scpi_choice_def_t const channel_choices[] = { { "CH1", CHANNEL_CH1 },
                                                  { "CH2", CHANNEL_CH2 },
                                                  { "CH1CH2", CHANNEL_CH1CH2 },
                                                  SCPI_CHOICE_LIST_END };

    int32_t channel_choice = -1;

    // Parse required parameter using SCPI_ParamChoice
    if (!SCPI_ParamChoice(context, channel_choices, &channel_choice, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    if (g_dso_state.dso_handle &&
        DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
        // Can't change configuration while acquisition is in progress
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    // Get current configuration or use default
    DSO_Config config = g_dso_state.dso_handle
                            ? DSO_get_config(g_dso_state.dso_handle)
                            : (DSO_Config)DSO_CONFIG_DEFAULT;

    // Update channel and mode based on choice
    switch (channel_choice) {
    case CHANNEL_CH1:
        config.channel = DSO_CHANNEL_0;
        config.mode = DSO_MODE_SINGLE_CHANNEL;
        break;
    case CHANNEL_CH2:
        config.channel = DSO_CHANNEL_1;
        config.mode = DSO_MODE_SINGLE_CHANNEL;
        break;
    case CHANNEL_CH1CH2:
        config.channel = DSO_CHANNEL_0; // Primary channel
        config.mode = DSO_MODE_DUAL_CHANNEL;
        break;
    default:
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    // Use current buffer size, or default if not set
    uint32_t buffer_size = g_dso_state.acquisition_buffer_size > 0
                               ? g_dso_state.acquisition_buffer_size
                               : BUFFER_SIZE_DEFAULT;

    // Configure sample rate and buffer using helper function
    scpi_result_t result = configure_sample_rate_and_buffer(
        context, buffer_size, config.mode, &config
    );
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Apply configuration
    return apply_dso_config(context, &config);
}

/**
 * @brief CONFigure:OSCilloscope:CHANnel? - Query current DSO channel
 * configuration
 *
 * Returns the currently configured channel as a string:
 * "CH1" - Single channel mode, channel 1
 * "CH2" - Single channel mode, channel 2
 * "CH1CH2" - Dual channel mode
 */
scpi_result_t scpi_cmd_configure_oscilloscope_channel_q(scpi_t *context)
{
    DSO_Config config = g_dso_state.dso_handle
                            ? DSO_get_config(g_dso_state.dso_handle)
                            : (DSO_Config)DSO_CONFIG_DEFAULT;

    if (config.mode == DSO_MODE_DUAL_CHANNEL) {
        SCPI_ResultText(context, "CH1CH2");
    } else if (config.channel == DSO_CHANNEL_0) {
        SCPI_ResultText(context, "CH1");
    } else if (config.channel == DSO_CHANNEL_1) {
        SCPI_ResultText(context, "CH2");
    } else {
        SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

/**
 * @brief CONFigure:OSCilloscope:TIMEbase - Set DSO timebase (µs / div)
 *
 * Syntax: CONFigure:OSCilloscope:TIMEbase <timebase_us>
 *
 * Sets the timebase and updates sample rate based on the current buffer size.
 * The sample rate is calculated as: buffer_size * 1,000,000 / timebase_us
 */
scpi_result_t scpi_cmd_configure_oscilloscope_timebase(scpi_t *context)
{
    uint32_t timebase_us = TIMEBASE_DEFAULT;

    // Parse required parameter
    if (!SCPI_ParamUInt32(context, &timebase_us, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    // Validate timebase parameter (must be > 0)
    if (timebase_us == 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    if (g_dso_state.dso_handle &&
        DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
        // Can't change configuration while acquisition is in progress
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    // Get current configuration or use default
    DSO_Config config = g_dso_state.dso_handle
                            ? DSO_get_config(g_dso_state.dso_handle)
                            : (DSO_Config)DSO_CONFIG_DEFAULT;

    // Use current buffer size, or default if not set
    uint32_t buffer_size = g_dso_state.acquisition_buffer_size > 0
                               ? g_dso_state.acquisition_buffer_size
                               : BUFFER_SIZE_DEFAULT;

    // Configure sample rate and buffer using helper function
    scpi_result_t result = configure_sample_rate_and_buffer(
        context, buffer_size, config.mode, &config
    );
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Store timebase in state
    g_dso_state.timebase_us = timebase_us;

    // Apply configuration
    return apply_dso_config(context, &config);
}

/**
 * @brief CONFigure:OSCilloscope:TIMEbase? - Query current DSO timebase
 *
 * Returns the currently configured timebase in microseconds per division.
 */
scpi_result_t scpi_cmd_configure_oscilloscope_timebase_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, g_dso_state.timebase_us);
    return SCPI_RES_OK;
}

/**
 * @brief CONFigure:OSCilloscope:ACQuire:POINts - Set DSO buffer size
 *
 * Syntax: CONFigure:OSCilloscope:ACQuire:POINts <n>
 *
 * Sets the buffer size and updates sample rate based on the current timebase.
 * The sample rate is calculated as: buffer_size * 1,000,000 / timebase_us
 */
scpi_result_t scpi_cmd_configure_oscilloscope_acquire_points(scpi_t *context)
{
    uint32_t buffer_size = BUFFER_SIZE_DEFAULT;

    // Parse required parameter
    if (!SCPI_ParamUInt32(context, &buffer_size, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    // Validate buffer size parameter (must be > 0)
    if (buffer_size == 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    if (g_dso_state.dso_handle &&
        DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
        // Can't change configuration while acquisition is in progress
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    // Get current configuration or use default
    DSO_Config config = g_dso_state.dso_handle
                            ? DSO_get_config(g_dso_state.dso_handle)
                            : (DSO_Config)DSO_CONFIG_DEFAULT;

    // Configure sample rate and buffer using helper function
    scpi_result_t result = configure_sample_rate_and_buffer(
        context, buffer_size, config.mode, &config
    );
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Apply configuration
    return apply_dso_config(context, &config);
}

/**
 * @brief CONFigure:OSCilloscope:ACQuire:POINts? - Query current DSO buffer size
 *
 * Returns the currently configured acquisition buffer size in points/samples.
 */
scpi_result_t scpi_cmd_configure_oscilloscope_acquire_points_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, g_dso_state.acquisition_buffer_size);
    return SCPI_RES_OK;
}

/**
 * @brief CONFigure:OSCilloscope:ACQuire:SRATe? - Query current DSO sample rate
 *
 * Returns the currently configured sample rate in samples per second.
 * The sample rate is calculated based on the timebase and buffer size.
 */
scpi_result_t scpi_cmd_configure_oscilloscope_acquire_srate_q(scpi_t *context)
{
    DSO_Config config = g_dso_state.dso_handle
                            ? DSO_get_config(g_dso_state.dso_handle)
                            : (DSO_Config)DSO_CONFIG_DEFAULT;
    SCPI_ResultUInt32(context, config.sample_rate);
    return SCPI_RES_OK;
}

/**
 * @brief INITiate:OSCilloscope - Start DSO data acquisition
 */
scpi_result_t scpi_cmd_initiate_oscilloscope(scpi_t *context)
{
    Error err = ERROR_NONE;

    // If DSO is not configured, initialize with default configuration
    if (!g_dso_state.dso_handle) {
        DSO_Config config = (DSO_Config)DSO_CONFIG_DEFAULT;

        // Use current buffer size, or default if not set
        uint32_t buffer_size = g_dso_state.acquisition_buffer_size > 0
                                   ? g_dso_state.acquisition_buffer_size
                                   : BUFFER_SIZE_DEFAULT;

        // Configure sample rate and buffer using helper function
        scpi_result_t result = configure_sample_rate_and_buffer(
            context, buffer_size, config.mode, &config
        );
        if (result != SCPI_RES_OK) {
            return result;
        }

        // Apply default configuration
        result = apply_dso_config(context, &config);
        if (result != SCPI_RES_OK) {
            return result;
        }
    }

    // Clear acquisition flags
    g_dso_state.acquisition_complete = false;

    // Start acquisition
    TRY { DSO_start(g_dso_state.dso_handle); }
    CATCH(err)
    {
        LOG_ERROR("DSO start error: 0x%08X", err);
        SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

/**
 * @brief ABORt:OSCilloscope - Abort ongoing oscilloscope acquisition
 */
scpi_result_t scpi_cmd_abort_oscilloscope(scpi_t *context)
{
    (void)context;

    if (g_dso_state.dso_handle) {
        DSO_stop(g_dso_state.dso_handle);
        DSO_deinit(g_dso_state.dso_handle);
        g_dso_state.dso_handle = nullptr;
    }

    g_dso_state.acquisition_complete = false;

    return SCPI_RES_OK;
}

/**
 * @brief FETCh:OSCilloscope:DATa? - Fetch the oscilloscope data
 */
scpi_result_t scpi_cmd_fetch_oscilloscope_data_q(scpi_t *context)
{
    // Check if DSO is configured
    if (!g_dso_state.dso_handle || !g_dso_state.acquisition_buffer) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    // If acquisition is still in progress, wait for completion
    if (g_dso_state.dso_handle &&
        DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
        uint32_t timeout = SI_MILLI_DIV; // 1 second timeout
        uint32_t start_time = SYSTEM_get_tick();

        while (DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
            if (SYSTEM_get_tick() - start_time > timeout) {
                LOG_ERROR("DSO acquisition timeout - stopping acquisition");
                DSO_stop(g_dso_state.dso_handle);
                SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
                return SCPI_RES_ERR;
            }
        }
    }

    // Check if acquisition completed successfully
    if (!g_dso_state.acquisition_complete) {
        DSO_stop(g_dso_state.dso_handle);
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    DSO_stop(g_dso_state.dso_handle);

    // Output acquisition data as SCPI arbitrary block
    size_t data_size = g_dso_state.acquisition_buffer_size * sizeof(uint16_t);
    SCPI_ResultArbitraryBlock(
        context, (char *)g_dso_state.acquisition_buffer, data_size
    );

    return SCPI_RES_OK;
}

/**
 * @brief READ:OSCilloscope? - Initiate and fetch oscilloscope data
 */
scpi_result_t scpi_cmd_read_oscilloscope_q(scpi_t *context)
{
    scpi_result_t result = SCPI_RES_OK;

    // Abort any ongoing acquisition
    if (g_dso_state.dso_handle &&
        DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
        result = scpi_cmd_abort_oscilloscope(context);
        if (result != SCPI_RES_OK) {
            return result;
        }
    }

    // Initiate acquisition
    result = scpi_cmd_initiate_oscilloscope(context);
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Fetch the result
    return scpi_cmd_fetch_oscilloscope_data_q(context);
}

/**
 * @brief MEASure:OSCilloscope? - Configure, initiate, and fetch oscilloscope
 * data
 *
 * This is a convenience command that combines configuration and measurement.
 * Parameters: channel, mode, sample_rate, buffer_size
 */
scpi_result_t scpi_cmd_measure_oscilloscope_q(scpi_t *context)
{
    scpi_result_t result = SCPI_RES_OK;

    // Configure (validate the configuration)
    result = scpi_cmd_configure_oscilloscope_channel(context);
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Read (initiate + fetch)
    return scpi_cmd_read_oscilloscope_q(context);
}

/**
 * @brief STATus:OSCilloscope:ACQuisition? - Query acquisition status
 *
 * Returns:
 * 0 - No acquisition started
 * 1 - Acquisition in progress
 * 2 - Acquisition complete
 */
scpi_result_t scpi_cmd_status_oscilloscope_acquisition_q(scpi_t *context)
{
    uint32_t status = 0;

    if (g_dso_state.acquisition_buffer) {
        if (g_dso_state.dso_handle &&
            DSO_is_acquisition_in_progress(g_dso_state.dso_handle)) {
            status = 1;
        } else if (g_dso_state.acquisition_complete) {
            status = 2;
        }
    }

    SCPI_ResultUInt32(context, status);
    return SCPI_RES_OK;
}