/**
 * @file dmm.c
 * @brief DMM-specific SCPI commands implementation
 *
 * This module implements the SCPI commands specific to Digital Multimeter
 * functionality, including voltage measurement configuration and reading.
 */

#include <stdio.h>
#include <string.h>

#include "lib/scpi/error.h"
#include "lib/scpi/scpi.h"

#include "system/instrument/dmm.h"
#include "system/system.h"
#include "util/error.h"
#include "util/fixed_point.h"
#include "util/si_prefix.h"
#include "util/util.h"

// DMM state (internal to this module)
static struct {
    DMM_Handle *dmm_handle;
    DMM_Config dmm_config;
    FIXED_Q1616 cached_voltage;
    bool has_cached_voltage;
} g_dmm_state = {
    .dmm_handle = nullptr,
    .dmm_config = DMM_CONFIG_DEFAULT,
    .cached_voltage = 0,
    .has_cached_voltage = false,
};

/**
 * @brief Reset DMM state to default values
 */
void dmm_reset_state(void)
{
    // Reset instrument state
    if (g_dmm_state.dmm_handle) {
        DMM_deinit(g_dmm_state.dmm_handle);
        g_dmm_state.dmm_handle = nullptr;
    }

    g_dmm_state.dmm_config = (DMM_Config)DMM_CONFIG_DEFAULT;
    g_dmm_state.cached_voltage = 0;
    g_dmm_state.has_cached_voltage = false;
}

/**
 * @brief DMM:CONFigure:VOLTage:DC - Configure DC voltage measurement parameters
 */
scpi_result_t scpi_cmd_configure_voltage_dc(scpi_t *context)
{
    Error err = ERROR_NONE;
    DMM_Config config = DMM_CONFIG_DEFAULT;
    DMM_Handle *temp_handle = nullptr;

    // Parse channel parameter if provided
    SCPI_ParamUInt32(context, (uint32_t *)&config.channel, false);

    // Validate configuration by attempting to initialize and deinitialize
    TRY { temp_handle = DMM_init(&config); }
    CATCH(err)
    {
        switch (err) {
        case ERROR_INVALID_ARGUMENT:
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        default:
            LOG_ERROR("DMM configuration error: 0x%08X", err);
            SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
            return SCPI_RES_ERR;
        }
    }

    // Configuration is valid, clean up and store it
    DMM_deinit(temp_handle);
    g_dmm_state.dmm_config = config;

    return SCPI_RES_OK;
}

/**
 * @brief DMM:INITiate:VOLTage:DC - Initialize DMM and start voltage measurement
 */
scpi_result_t scpi_cmd_initiate_voltage_dc(scpi_t *context)
{
    Error err = ERROR_NONE;

    // Clean up any existing DMM handle
    if (g_dmm_state.dmm_handle) {
        DMM_deinit(g_dmm_state.dmm_handle);
        g_dmm_state.dmm_handle = nullptr;
    }

    // Clear cached voltage since we're starting a new measurement
    g_dmm_state.has_cached_voltage = false;
    g_dmm_state.cached_voltage = 0;

    // Initialize DMM with stored configuration (defaults to DMM_CONFIG_DEFAULT
    // if never configured)
    TRY { g_dmm_state.dmm_handle = DMM_init(&g_dmm_state.dmm_config); }
    CATCH(err)
    {
        LOG_ERROR("DMM initialization error: 0x%08X", err);
        SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

/**
 * @brief Helper function to fetch a new voltage reading
 */
static scpi_result_t fetch_new_voltage_dc(scpi_t *context)
{
    Error err = ERROR_NONE;
    FIXED_Q1616 voltage = 0;
    uint32_t timeout = SI_MILLI_DIV; // 1 second timeout
    uint32_t start_time = SYSTEM_get_tick();

    // Read ADC value with timeout
    TRY
    {
        while (!DMM_read_voltage(g_dmm_state.dmm_handle, &voltage)) {
            // Wait for ADC to be ready
            if (SYSTEM_get_tick() - start_time > timeout) {
                LOG_ERROR("DMM read timeout");
                SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
                return SCPI_RES_ERR;
            }
        }
    }
    CATCH(err)
    {
        LOG_ERROR("DMM read error: 0x%08X", err);
        SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
        return SCPI_RES_ERR;
    }

    // Cache the new reading
    g_dmm_state.cached_voltage = voltage;
    g_dmm_state.has_cached_voltage = true;

    // Clean up DMM handle
    DMM_deinit(g_dmm_state.dmm_handle);
    g_dmm_state.dmm_handle = nullptr;
    return SCPI_RES_OK;
}

/**
 * @brief DMM:FETCh:VOLTage:DC? - Fetch the voltage measurement result
 */
scpi_result_t scpi_cmd_fetch_voltage_dc(scpi_t *context)
{
    // If DMM handle exists, read a new value and cache it
    if (g_dmm_state.dmm_handle) {
        if (fetch_new_voltage_dc(context) != SCPI_RES_OK) {
            return SCPI_RES_ERR;
        }
    }

    // If no cached value exists by now, INIT has never run
    if (!g_dmm_state.has_cached_voltage) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    // Convert Q16.16 to millivolts for SCPI output
    int32_t voltage_millivolts =
        FIXED_TO_INT(g_dmm_state.cached_voltage * SI_MILLI_DIV);
    SCPI_ResultUInt32(context, voltage_millivolts);
    return SCPI_RES_OK;
}

/**
 * @brief DMM:READ:VOLTage:DC? - Initiate and fetch voltage measurement
 */
scpi_result_t scpi_cmd_read_voltage_dc(scpi_t *context)
{
    scpi_result_t result = SCPI_RES_OK;

    // Initiate measurement
    result = scpi_cmd_initiate_voltage_dc(context);
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Fetch the result
    return scpi_cmd_fetch_voltage_dc(context);
}

/**
 * @brief DMM:MEASure:VOLTage:DC? - Configure, initiate, and fetch DC voltage
 * measurement
 */
scpi_result_t scpi_cmd_measure_voltage_dc(scpi_t *context)
{
    scpi_result_t result = SCPI_RES_OK;

    // Configure (validate the configuration)
    result = scpi_cmd_configure_voltage_dc(context);
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Read (initiate + fetch)
    return scpi_cmd_read_voltage_dc(context);
}