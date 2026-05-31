/**
 * @file dso.c
 * @brief Oscilloscope-specific SCPI commands implementation
 *
 * This module implements the SCPI commands specific to oscilloscope
 * functionality, including ADC configuration, triggering, capture, fetch, and
 * streaming control.
 */

#include <stdint.h>
#include <stdio.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/dso_commands.h"
#include "application/logic_analyser_commands.h"

static scpi_result_t result_ok(void) { return SCPI_RES_OK; }

static scpi_result_t result_illegal_parameter(scpi_t *context)
{
    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    return SCPI_RES_ERR;
}

static scpi_result_t result_execution_error(scpi_t *context)
{
    SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
    return SCPI_RES_ERR;
}

static scpi_result_t configure_uint32(
    scpi_t *context,
    bool (*setter)(uint32_t)
)
{
    uint32_t value = 0;
    if (!SCPI_ParamUInt32(context, &value, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    return setter(value) ? result_ok() : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_configure_dso_channel(scpi_t *context)
{
    return configure_uint32(context, dso_commands_set_channel);
}

scpi_result_t scpi_cmd_configure_dso_channel_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, dso_commands_get_channel());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_dso_gpio_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, dso_commands_get_gpio());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_dso_samples(scpi_t *context)
{
    return configure_uint32(context, dso_commands_set_samples);
}

scpi_result_t scpi_cmd_configure_dso_samples_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, dso_commands_get_samples());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_dso_rate(scpi_t *context)
{
    return configure_uint32(context, dso_commands_set_sample_rate);
}

scpi_result_t scpi_cmd_configure_dso_rate_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, dso_commands_get_sample_rate());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_dso_trigger_level(scpi_t *context)
{
    return configure_uint32(context, dso_commands_set_trigger_level);
}

scpi_result_t scpi_cmd_configure_dso_trigger_level_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, dso_commands_get_trigger_level());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_dso_trigger_mode(scpi_t *context)
{
    enum { DSO_MODE_OFF, DSO_MODE_LEVEL, DSO_MODE_EDGE };
    scpi_choice_def_t const choices[] = {
        { "OFF", DSO_MODE_OFF },
        { "LEVEL", DSO_MODE_LEVEL },
        { "EDGE", DSO_MODE_EDGE },
        SCPI_CHOICE_LIST_END
    };
    int32_t choice = -1;

    if (!SCPI_ParamChoice(context, choices, &choice, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    switch (choice) {
    case DSO_MODE_OFF:
        return dso_commands_set_trigger_mode_off()
                   ? result_ok()
                   : result_illegal_parameter(context);
    case DSO_MODE_LEVEL:
        return dso_commands_set_trigger_mode_level()
                   ? result_ok()
                   : result_illegal_parameter(context);
    case DSO_MODE_EDGE:
        return dso_commands_set_trigger_mode_edge()
                   ? result_ok()
                   : result_illegal_parameter(context);
    default:
        return result_illegal_parameter(context);
    }
}

scpi_result_t scpi_cmd_configure_dso_trigger_mode_q(scpi_t *context)
{
    SCPI_ResultText(context, dso_commands_get_trigger_mode());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_dso_trigger_slope(scpi_t *context)
{
    enum { DSO_SLOPE_RISE, DSO_SLOPE_FALL };
    scpi_choice_def_t const choices[] = {
        { "RISE", DSO_SLOPE_RISE },
        { "RISING", DSO_SLOPE_RISE },
        { "FALL", DSO_SLOPE_FALL },
        { "FALLING", DSO_SLOPE_FALL },
        SCPI_CHOICE_LIST_END
    };
    int32_t choice = -1;

    if (!SCPI_ParamChoice(context, choices, &choice, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    return choice == DSO_SLOPE_FALL
               ? (dso_commands_set_trigger_slope_falling()
                      ? result_ok()
                      : result_illegal_parameter(context))
               : (dso_commands_set_trigger_slope_rising()
                      ? result_ok()
                      : result_illegal_parameter(context));
}

scpi_result_t scpi_cmd_configure_dso_trigger_slope_q(scpi_t *context)
{
    SCPI_ResultText(context, dso_commands_get_trigger_slope());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_initiate_dso(scpi_t *context)
{
    return dso_commands_initiate() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_fetch_dso_data_q(scpi_t *context)
{
    uint8_t const *data = NULL;
    size_t len = 0;

    if (!dso_commands_fetch(&data, &len)) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, data, len);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_read_dso_q(scpi_t *context)
{
    if (!dso_commands_initiate()) {
        return result_execution_error(context);
    }

    return scpi_cmd_fetch_dso_data_q(context);
}

scpi_result_t scpi_cmd_status_dso_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, dso_commands_status());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_stream_dso_start(scpi_t *context)
{
    la_stream_stop();
    return dso_commands_stream_start() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_stream_dso_stop(scpi_t *context)
{
    (void)context;
    dso_commands_stream_stop();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_stream_dso_status_q(scpi_t *context)
{
    char status[80];
    snprintf(
        status,
        sizeof(status),
        "%u,%lu,%lu",
        dso_commands_stream_is_enabled() ? 1u : 0u,
        (unsigned long)dso_commands_stream_get_sequence(),
        (unsigned long)dso_commands_stream_get_overruns()
    );
    SCPI_ResultText(context, status);
    return SCPI_RES_OK;
}
