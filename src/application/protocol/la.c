/**
 * @file la.c
 * @brief Logic analyser-specific SCPI commands implementation
 *
 * This module implements the SCPI commands specific to logic analyser
 * functionality, including configuration, triggering, capture, fetch, and
 * streaming control.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/dso_commands.h"
#include "application/logic_analyser_commands.h"
#include "platform/test_signal.h"

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

scpi_result_t scpi_cmd_configure_logic_analyser_pinbase(scpi_t *context)
{
    return configure_uint32(context, la_set_pin_base);
}

scpi_result_t scpi_cmd_configure_logic_analyser_pinbase_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, la_get_pin_base());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_pincount(scpi_t *context)
{
    return configure_uint32(context, la_set_pin_count);
}

scpi_result_t scpi_cmd_configure_logic_analyser_pincount_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, la_get_pin_count());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_samples(scpi_t *context)
{
    return configure_uint32(context, la_set_samples);
}

scpi_result_t scpi_cmd_configure_logic_analyser_samples_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, la_get_samples());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_divider(scpi_t *context)
{
    return configure_uint32(context, la_set_divider);
}

scpi_result_t scpi_cmd_configure_logic_analyser_divider_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, la_get_divider());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_trigger_pin(scpi_t *context)
{
    return configure_uint32(context, la_set_trigger_pin);
}

scpi_result_t scpi_cmd_configure_logic_analyser_trigger_pin_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, la_get_trigger_pin());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_trigger_level(scpi_t *context)
{
    scpi_bool_t value = FALSE;
    if (!SCPI_ParamBool(context, &value, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    la_set_trigger_level(value == TRUE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_trigger_level_q(scpi_t *context)
{
    SCPI_ResultBool(context, la_get_trigger_level() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_configure_logic_analyser_trigger_mode(scpi_t *context)
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

    return la_set_trigger_mode_edge(choice == TRIGGER_MODE_EDGE)
               ? result_ok()
               : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_configure_logic_analyser_trigger_mode_q(scpi_t *context)
{
    SCPI_ResultText(context, la_get_trigger_mode_edge() ? "EDGE" : "LEVEL");
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_initiate_logic_analyser(scpi_t *context)
{
    return la_initiate() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_fetch_logic_analyser_data_q(scpi_t *context)
{
    uint8_t const *data = NULL;
    size_t len = 0;

    if (!la_fetch(&data, &len)) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, data, len);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_read_logic_analyser_q(scpi_t *context)
{
    if (!la_initiate()) {
        return result_execution_error(context);
    }

    return scpi_cmd_fetch_logic_analyser_data_q(context);
}

scpi_result_t scpi_cmd_status_logic_analyser_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, la_status());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_stream_logic_analyser_start(scpi_t *context)
{
    dso_commands_stream_stop();
    return la_stream_start() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_stream_logic_analyser_stop(scpi_t *context)
{
    (void)context;
    la_stream_stop();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_stream_logic_analyser_status_q(scpi_t *context)
{
    char status[80];
    snprintf(
        status,
        sizeof(status),
        "%u,%lu,%lu",
        la_stream_is_enabled() ? 1u : 0u,
        (unsigned long)la_stream_get_sequence(),
        (unsigned long)la_stream_get_overruns()
    );
    SCPI_ResultText(context, status);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_square(scpi_t *context)
{
    scpi_bool_t enable = FALSE;
    if (!SCPI_ParamBool(context, &enable, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    if (enable == TRUE) {
        return test_signal_start(
                   TEST_SIGNAL_DEFAULT_PIN,
                   TEST_SIGNAL_DEFAULT_FREQUENCY_HZ
               )
                   ? result_ok()
                   : result_execution_error(context);
    }

    test_signal_stop();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_square_q(scpi_t *context)
{
    SCPI_ResultBool(context, test_signal_is_enabled() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_square_configure(scpi_t *context)
{
    uint32_t pin = 0;
    uint32_t frequency = 0;

    if (!SCPI_ParamUInt32(context, &pin, TRUE) ||
        !SCPI_ParamUInt32(context, &frequency, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    if (pin > 29 || frequency == 0) {
        return result_illegal_parameter(context);
    }

    return test_signal_start(pin, frequency) ? result_ok()
                                             : result_execution_error(context);
}

scpi_result_t scpi_cmd_test_square_pin_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, test_signal_get_pin());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_square_frequency_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, test_signal_get_frequency_hz());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_analog(scpi_t *context)
{
    scpi_bool_t enable = FALSE;
    if (!SCPI_ParamBool(context, &enable, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    if (enable == TRUE) {
        return test_signal_analog_start(
                   TEST_SIGNAL_ANALOG_DEFAULT_PIN,
                   TEST_SIGNAL_ANALOG_DEFAULT_FREQUENCY_HZ,
                   TEST_SIGNAL_ANALOG_DEFAULT_DUTY_PERMILLE
               )
                   ? result_ok()
                   : result_execution_error(context);
    }

    test_signal_analog_stop();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_analog_q(scpi_t *context)
{
    SCPI_ResultBool(context, test_signal_analog_is_enabled() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_analog_configure(scpi_t *context)
{
    uint32_t pin = 0;
    uint32_t frequency = 0;
    uint32_t duty_permille = 0;

    if (!SCPI_ParamUInt32(context, &pin, TRUE) ||
        !SCPI_ParamUInt32(context, &frequency, TRUE) ||
        !SCPI_ParamUInt32(context, &duty_permille, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    if (pin > 29 || frequency == 0 || duty_permille > 1000) {
        return result_illegal_parameter(context);
    }

    return test_signal_analog_start(pin, frequency, duty_permille)
               ? result_ok()
               : result_execution_error(context);
}

scpi_result_t scpi_cmd_test_analog_pin_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, test_signal_analog_get_pin());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_analog_frequency_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, test_signal_analog_get_frequency_hz());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_test_analog_duty_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, test_signal_analog_get_duty_permille());
    return SCPI_RES_OK;
}
