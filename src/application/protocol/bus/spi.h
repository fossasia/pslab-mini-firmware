#ifndef PSLAB_APPLICATION_PROTOCOL_BUS_SPI_H
#define PSLAB_APPLICATION_PROTOCOL_BUS_SPI_H

#include "scpi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

scpi_result_t scpi_cmd_bus_spi_open(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_close(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_open_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_bus(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_bus_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_rate(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_rate_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_mode(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_mode_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_dummy(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_configure_dummy_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_write(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_read_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_exchange_q(scpi_t *context);
scpi_result_t scpi_cmd_bus_spi_transact_q(scpi_t *context);

#ifdef __cplusplus
}
#endif

#endif
