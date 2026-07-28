#ifndef PSLAB_APPLICATION_PROTOCOL_BUS_I2C_H
#define PSLAB_APPLICATION_PROTOCOL_BUS_I2C_H

#include "scpi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

scpi_result_t scpi_cmd_bus_i2c_open(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_close(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_open_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_bus(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_bus_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_rate(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_rate_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_address(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_address_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_timeout(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_configure_timeout_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_scan_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_write(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_read_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_i2c_transact_q(scpi_t *context);

#ifdef __cplusplus
}
#endif

#endif
