#include "application/protocol/bus/uart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/gateway/uart_commands.h"

static uint8_t response_buffer[UART_GATEWAY_MAX_TRANSFER];

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

scpi_result_t scpi_cmd_bus_uart_open(scpi_t *context)
{
    return uart_gateway_open() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_bus_uart_close(scpi_t *context)
{
    (void)context;
    uart_gateway_close();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_open_q(scpi_t *context)
{
    SCPI_ResultBool(context, uart_gateway_is_open() ? TRUE : FALSE);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_configure_bus(scpi_t *context)
{
    return configure_uint32(context, uart_gateway_set_bus);
}

scpi_result_t scpi_cmd_bus_uart_configure_bus_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, uart_gateway_get_bus());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_configure_baud(scpi_t *context)
{
    return configure_uint32(context, uart_gateway_set_baud);
}

scpi_result_t scpi_cmd_bus_uart_configure_baud_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, uart_gateway_get_baud());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_configure_timeout(scpi_t *context)
{
    return configure_uint32(context, uart_gateway_set_timeout);
}

scpi_result_t scpi_cmd_bus_uart_configure_timeout_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, uart_gateway_get_timeout());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_write(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!uart_gateway_is_open()) {
        return result_execution_error(context);
    }

    SCPI_ResultUInt32(
        context,
        uart_gateway_write((uint8_t const *)data, len)
    );
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_read_q(scpi_t *context)
{
    uint32_t max_len = UART_GATEWAY_MAX_TRANSFER;

    (void)SCPI_ParamUInt32(context, &max_len, FALSE);

    if (!uart_gateway_is_open()) {
        return result_execution_error(context);
    }

    if (max_len > UART_GATEWAY_MAX_TRANSFER) {
        max_len = UART_GATEWAY_MAX_TRANSFER;
    }

    uint32_t bytes_read = uart_gateway_read(response_buffer, max_len);
    SCPI_ResultArbitraryBlock(context, response_buffer, bytes_read);
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_available_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, uart_gateway_available());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_clear(scpi_t *context)
{
    (void)context;
    uart_gateway_clear();
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_bus_uart_flush(scpi_t *context)
{
    return uart_gateway_flush() ? result_ok() : result_execution_error(context);
}

scpi_result_t scpi_cmd_bus_uart_transact_q(scpi_t *context)
{
    char const *data = NULL;
    size_t len = 0;

    if (!SCPI_ParamArbitraryBlock(context, &data, &len, TRUE)) {
        return result_missing_parameter(context);
    }

    if (!uart_gateway_is_open()) {
        return result_execution_error(context);
    }

    uint32_t bytes_read = uart_gateway_transact(
        (uint8_t const *)data,
        len,
        response_buffer,
        sizeof(response_buffer)
    );
    SCPI_ResultArbitraryBlock(context, response_buffer, bytes_read);
    return SCPI_RES_OK;
}
