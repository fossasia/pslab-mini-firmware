/**
 * @file mso.c
 * @brief Mixed-signal capture SCPI commands implementation
 *
 * This module exposes the first mixed-signal capture path. The digital
 * channels are captured by the logic analyser engine, the analog channel is
 * captured by the ADC engine, and both use one shared trigger and sample rate.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/mixed_signal_commands.h"

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

scpi_result_t scpi_cmd_configure_mso_digital_pinbase(scpi_t *context)
{
    return configure_uint32(context, mso_commands_set_digital_pin_base);
}

scpi_result_t scpi_cmd_configure_mso_digital_pinbase_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_get_digital_pin_base());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_digital_pincount(scpi_t *context)
{
    return configure_uint32(context, mso_commands_set_digital_pin_count);
}

scpi_result_t scpi_cmd_configure_mso_digital_pincount_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_get_digital_pin_count());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_analog_channel(scpi_t *context)
{
    return configure_uint32(context, mso_commands_set_analog_channel);
}

scpi_result_t scpi_cmd_configure_mso_analog_channel_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_get_analog_channel());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_samples(scpi_t *context)
{
    return configure_uint32(context, mso_commands_set_samples);
}

scpi_result_t scpi_cmd_configure_mso_samples_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_get_samples());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_rate(scpi_t *context)
{
    return configure_uint32(context, mso_commands_set_sample_rate);
}

scpi_result_t scpi_cmd_configure_mso_rate_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_get_sample_rate());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_trigger_pin(scpi_t *context)
{
    return configure_uint32(context, mso_commands_set_trigger_pin);
}

scpi_result_t scpi_cmd_configure_mso_trigger_pin_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_get_trigger_pin());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_trigger_level(scpi_t *context)
{
    scpi_bool_t value = FALSE;
    if (!SCPI_ParamBool(context, &value, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    mso_commands_set_trigger_level(value == TRUE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_trigger_level_q(scpi_t *context)
{
    SCPI_ResultBool(context, mso_commands_get_trigger_level() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_mso_trigger_mode(scpi_t *context)
{
    enum { TRIGGER_MODE_EDGE, TRIGGER_MODE_LEVEL };
    scpi_choice_def_t const trigger_mode_choices[] = {
        { "EDGE", TRIGGER_MODE_EDGE },
        { "LEVEL", TRIGGER_MODE_LEVEL },
        SCPI_CHOICE_LIST_END
    };
    int32_t choice = -1;

    if (!SCPI_ParamChoice(context, trigger_mode_choices, &choice, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    return mso_commands_set_trigger_mode_edge(choice == TRIGGER_MODE_EDGE)
               ? result_ok()
               : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_configure_mso_trigger_mode_q(scpi_t *context)
{
    SCPI_ResultText(
        context,
        mso_commands_get_trigger_mode_edge() ? "EDGE" : "LEVEL"
    );
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_initiate_mso(scpi_t *context)
{
    return mso_commands_initiate() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_fetch_mso_digital_q(scpi_t *context)
{
    uint8_t const *data = NULL;
    size_t len = 0;

    if (!mso_commands_fetch_digital(&data, &len)) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, data, len);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_fetch_mso_analog_q(scpi_t *context)
{
    uint8_t const *data = NULL;
    size_t len = 0;

    if (!mso_commands_fetch_analog(&data, &len)) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, data, len);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_read_mso_digital_q(scpi_t *context)
{
    if (!mso_commands_initiate()) {
        return result_execution_error(context);
    }

    return scpi_cmd_fetch_mso_digital_q(context);
}

scpi_result_t scpi_cmd_read_mso_analog_q(scpi_t *context)
{
    if (!mso_commands_initiate()) {
        return result_execution_error(context);
    }

    return scpi_cmd_fetch_mso_analog_q(context);
}

scpi_result_t scpi_cmd_status_mso_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, mso_commands_status());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_metadata_mso_q(scpi_t *context)
{
    MixedSignalCaptureInfo const *info = mso_commands_get_last_info();
    if (!info) {
        return result_execution_error(context);
    }

    char metadata[160];
    snprintf(
        metadata,
        sizeof(metadata),
        "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%d,%d",
        (unsigned long)info->sample_rate_hz,
        (unsigned long)info->sample_count,
        (unsigned long)info->digital_pin_base,
        (unsigned long)info->digital_pin_count,
        (unsigned long)info->digital_word_count,
        (unsigned long)info->digital_bits_per_word,
        (unsigned long)info->analog_channel,
        (unsigned long)info->analog_gpio,
        (int)info->digital_start_offset_ns,
        (int)info->analog_start_offset_ns
    );
    SCPI_ResultText(context, metadata);
    return SCPI_RES_OK;
}
