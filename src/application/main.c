#include "application/protocol.h"
#include "system/system.h"
#include "util/logging.h"

#include <stdbool.h>

int main(void)
{
    SYSTEM_init();
    LOG_INIT("Main application");

    if (!protocol_init()) {
        LOG_ERROR("Failed to initialize protocol");
        return -1;
    }

    while (true) {
        protocol_task();
        LOG_task(0xF);
    }
}
