#include "application/protocol/bus/i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/gateway/i2c_commands.h"

static uint8_t response_buffer[I2C_GATEWAY_MAX_TRANSFER];
static uint8_t scan_addresses[I2C_GATEWAY_MAX_SCAN_RESULTS];
static char scan_text[I2C_GATEWAY_MAX_SCAN_RESULTS * 3];

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

scpi_result_t scpi_cmd_bus_i2c_open(scpi_t *context)
{
    return i2c_gateway_open() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_bus_i2c_close(scpi_t *context)
{
    (void)context;
    i2c_gateway_close();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_open_q(scpi_t *context)
{
    SCPI_ResultBool(context, i2c_gateway_is_open() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_configure_bus(scpi_t *context)
{
    return configure_uint32(context, i2c_gateway_set_bus);
}

scpi_result_t scpi_cmd_bus_i2c_configure_bus_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, i2c_gateway_get_bus());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_configure_rate(scpi_t *context)
{
    return configure_uint32(context, i2c_gateway_set_rate);
}

scpi_result_t scpi_cmd_bus_i2c_configure_rate_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, i2c_gateway_get_rate());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_configure_address(scpi_t *context)
{
    return configure_uint32(context, i2c_gateway_set_address);
}

scpi_result_t scpi_cmd_bus_i2c_configure_address_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, i2c_gateway_get_address());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_configure_timeout(scpi_t *context)
{
    return configure_uint32(context, i2c_gateway_set_timeout);
}

scpi_result_t scpi_cmd_bus_i2c_configure_timeout_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, i2c_gateway_get_timeout());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_scan_q(scpi_t *context)
{
    if (!i2c_gateway_is_open()) {
        return result_execution_error(context);
    }

    uint32_t count = i2c_gateway_scan(scan_addresses, sizeof(scan_addresses));
    size_t used = 0;
    scan_text[0] = '\0';

    for (uint32_t i = 0; i < count; ++i) {
        int written = snprintf(
            &scan_text[used],
            sizeof(scan_text) - used,
            "%s%02X",
            i == 0 ? "" : ",",
            scan_addresses[i]
        );
        if (written < 0 || (size_t)written >= sizeof(scan_text) - used) {
            return result_execution_error(context);
        }
        used += (size_t)written;
    }

    SCPI_ResultText(context, scan_text);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_write(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!i2c_gateway_is_open() || len > I2C_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t written = i2c_gateway_write((uint8_t const *)data, len);
    if (written < 0) {
        return result_execution_error(context);
    }

    SCPI_ResultUInt32(context, (uint32_t)written);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_read_q(scpi_t *context)
{
    uint32_t len = 0;

    if (!SCPI_ParamUInt32(context, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!i2c_gateway_is_open() || len == 0 ||
        len > I2C_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t bytes_read = i2c_gateway_read(response_buffer, len);
    if (bytes_read < 0) {
        return result_execution_error(context);
    }

    SCPI_ResultArbitraryBlock(context, response_buffer, (size_t)bytes_read);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_i2c_transact_q(scpi_t *context)
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

    if (!i2c_gateway_is_open() || len > I2C_GATEWAY_MAX_TRANSFER ||
        read_len == 0 || read_len > I2C_GATEWAY_MAX_TRANSFER) {
        return result_execution_error(context);
    }

    int32_t bytes_read = i2c_gateway_transact(
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
