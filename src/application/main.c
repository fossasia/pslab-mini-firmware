#include "application/protocol.h"
#include "system/system.h"

int main(void)
{
    SYSTEM_init();
    protocol_init();

    while (true) {
        protocol_task();
    }
}
