#include "application/protocol.h"

int main(void)
{
    protocol_init();

    while (true) {
        protocol_task();
    }
}
