#include "application/protocol/bus/spi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/gateway/spi_commands.h"

static uint8_t response_buffer[SPI_GATEWAY_MAX_TRANSFER];

static scpi_result_t result_ok(void) { return SCPI_RES_OK; }

static scpi_result_t result_missing_parameter(scpi_t *context)
{
    SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
    return SCPI_RES_ERR;
}

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
        return result_missing_parameter(context);
    }

    return setter(value) ? result_ok() : result_illegal_parameter(context);
}

scpi_result_t scpi_cmd_bus_spi_open(scpi_t *context)
{
    return spi_gateway_open() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_bus_spi_close(scpi_t *context)
{
    (void)context;
    spi_gateway_close();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_open_q(scpi_t *context)
{
    SCPI_ResultBool(context, spi_gateway_is_open() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_configure_bus(scpi_t *context)
{
    return configure_uint32(context, spi_gateway_set_bus);
}

scpi_result_t scpi_cmd_bus_spi_configure_bus_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, spi_gateway_get_bus());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_configure_rate(scpi_t *context)
{
    return configure_uint32(context, spi_gateway_set_rate);
}

scpi_result_t scpi_cmd_bus_spi_configure_rate_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, spi_gateway_get_rate());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_configure_mode(scpi_t *context)
{
    return configure_uint32(context, spi_gateway_set_mode);
}

scpi_result_t scpi_cmd_bus_spi_configure_mode_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, spi_gateway_get_mode());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_configure_dummy(scpi_t *context)
{
    return configure_uint32(context, spi_gateway_set_dummy_byte);
}

scpi_result_t scpi_cmd_bus_spi_configure_dummy_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, spi_gateway_get_dummy_byte());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_write(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!spi_gateway_is_open() || len > SPI_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t written = spi_gateway_write((uint8_t const *)data, len);
    if (written < 0) {
        return result_execution_error(context);
    }

    SCPI_ResultUInt32(context, (uint32_t)written);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_read_q(scpi_t *context)
{
    uint32_t len = 0;

    if (!SCPI_ParamUInt32(context, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!spi_gateway_is_open() || len == 0 ||
        len > SPI_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t bytes_read = spi_gateway_read(response_buffer, len);
    if (bytes_read < 0) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, response_buffer, (size_t)bytes_read);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_exchange_q(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!spi_gateway_is_open() || len == 0 ||
        len > SPI_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t bytes_read = spi_gateway_exchange(
        (uint8_t const *)data,
        response_buffer,
        len
    );
    if (bytes_read < 0) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, response_buffer, (size_t)bytes_read);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_spi_transact_q(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;
    uint32_t read_len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!SCPI_ParamUInt32(context, &read_len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!spi_gateway_is_open() || len == 0 ||
        len > SPI_GATEWAY_MAX_TRANSFER || read_len == 0 ||
        read_len > SPI_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t bytes_read = spi_gateway_transact(
        (uint8_t const *)data,
        len,
        response_buffer,
        read_len
    );
    if (bytes_read < 0) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, response_buffer, (size_t)bytes_read);
    return SCPI_RES_OK;
}
