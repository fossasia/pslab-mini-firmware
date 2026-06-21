#ifndef PSLAB_COMMUNICATION_COMMANDS_H
#define PSLAB_COMMUNICATION_COMMANDS_H

#include "scpi/scpi.h"

#ifdef __cplusplus
extern "C" {
#endif

scpi_result_t scpi_cmd_comm_transport(scpi_t *context);
scpi_result_t scpi_cmd_comm_transport_q(scpi_t *context);
scpi_result_t scpi_cmd_comm_wifi_status_q(scpi_t *context);

#ifdef __cplusplus
}
#endif

#endif
