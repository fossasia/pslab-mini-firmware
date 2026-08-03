/**
 * @file pg.c
 * @brief Digital pattern generator SCPI commands implementation
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/pattern_generator_commands.h"

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

scpi_result_t scpi_cmd_pattern_generator_pins(scpi_t *context)
{
    uint32_t pin_base = 0;
    uint32_t pin_count = 0;

    if (!SCPI_ParamUInt32(context, &pin_base, TRUE) ||
        !SCPI_ParamUInt32(context, &pin_count, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    return pg_set_pins(pin_base, pin_count) ? result_ok()
                                            : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_pattern_generator_pins_q(scpi_t *context)
{
    char pins[32];
    snprintf(
        pins,
        sizeof(pins),
        "%lu,%lu",
        (unsigned long)pg_get_pin_base(),
        (unsigned long)pg_get_pin_count()
    );
    SCPI_ResultText(context, pins);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_pattern_generator_rate(scpi_t *context)
{
    uint32_t rate_hz = 0;
    if (!SCPI_ParamUInt32(context, &rate_hz, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    return pg_set_rate(rate_hz) ? result_ok()
                                : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_pattern_generator_rate_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, pg_get_rate());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_pattern_generator_mode(scpi_t *context)
{
    enum { PG_MODE_ONCE, PG_MODE_LOOP };
    scpi_choice_def_t const choices[] = {
        { "ONCE", PG_MODE_ONCE },
        { "LOOP", PG_MODE_LOOP },
        SCPI_CHOICE_LIST_END
    };
    int32_t choice = -1;

    if (!SCPI_ParamChoice(context, choices, &choice, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    if (choice == PG_MODE_LOOP) {
        return pg_set_mode_loop() ? result_ok()
                                  : result_illegal_parameter(context);
    }

    return pg_set_mode_once() ? result_ok() : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_pattern_generator_mode_q(scpi_t *context)
{
    SCPI_ResultText(context, pg_get_mode_loop() ? "LOOP" : "ONCE");
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_pattern_generator_data(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    return pg_upload_data((uint8_t const *)data, len)
               ? result_ok()
               : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_pattern_generator_start(scpi_t *context)
{
    return pg_start() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_pattern_generator_stop(scpi_t *context)
{
    (void)context;
    pg_stop();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_pattern_generator_status_q(scpi_t *context)
{
    char status[64];
    snprintf(
        status,
        sizeof(status),
        "%u,%lu,%lu,%lu",
        pg_is_running() ? 1u : 0u,
        (unsigned long)pg_get_rate(),
        (unsigned long)pg_get_pin_count(),
        (unsigned long)pg_get_pattern_words()
    );
    SCPI_ResultText(context, status);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_pattern_generator_underrun_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, pg_get_underruns());
    return SCPI_RES_OK;
}
