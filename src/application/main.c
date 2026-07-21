#include "application/protocol.h"
#include "system/system.h"
#include "util/logging.h"

int main(void)
{
    SYSTEM_init();
    LOG_INIT("Main application");

    protocol_init();

    while (true) {
        protocol_task();
        LOG_task(0xF);
    }
}
