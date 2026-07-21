#include "application/communication_commands.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "scpi/error.h"
#include "scpi/types.h"

#include "system/transport.h"

scpi_result_t scpi_cmd_comm_transport(scpi_t *context)
{
    enum {
        COMM_TRANSPORT_USB,
        COMM_TRANSPORT_WIFI,
        COMM_TRANSPORT_AUTO
    };
    scpi_choice_def_t const choices[] = {
        { "USB", COMM_TRANSPORT_USB },
        { "WIFI", COMM_TRANSPORT_WIFI },
        { "WIRELESS", COMM_TRANSPORT_WIFI },
        { "AUTO", COMM_TRANSPORT_AUTO },
        SCPI_CHOICE_LIST_END
    };
    int32_t choice = -1;

    if (!SCPI_ParamChoice(context, choices, &choice, TRUE)) {
        SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        return SCPI_RES_ERR;
    }

    switch (choice) {
    case COMM_TRANSPORT_WIFI:
        transport_set_mode(TRANSPORT_MODE_WIFI);
        break;
    case COMM_TRANSPORT_AUTO:
        transport_set_mode(TRANSPORT_MODE_AUTO);
        break;
    case COMM_TRANSPORT_USB:
    default:
        transport_set_mode(TRANSPORT_MODE_USB);
        break;
    }

    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_comm_transport_q(scpi_t *context)
{
    SCPI_ResultText(context, transport_get_mode_name());
    return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_comm_wifi_status_q(scpi_t *context)
{
    char status[96];
    snprintf(
        status,
        sizeof(status),
        "%u,%lu,%lu,%lu",
        transport_wifi_is_effective() ? 1u : 0u,
        (unsigned long)transport_get_sent_frames(),
        (unsigned long)transport_get_dropped_frames(),
        (unsigned long)transport_get_timeouts()
    );
    SCPI_ResultText(context, status);
    return SCPI_RES_OK;
}
