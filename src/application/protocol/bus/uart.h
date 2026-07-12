#ifndef PSLAB_APPLICATION_PROTOCOL_BUS_UART_H
#define PSLAB_APPLICATION_PROTOCOL_BUS_UART_H

#include "scpi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

scpi_result_t scpi_cmd_bus_uart_open(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_close(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_open_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_configure_bus(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_configure_bus_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_configure_baud(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_configure_baud_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_configure_timeout(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_configure_timeout_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_write(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_read_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_available_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_clear(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_flush(scpi_t *context);
scpi_result_t scpi_cmd_bus_uart_transact_q(scpi_t *context);

#ifdef __cplusplus
}
#endif

#endif
