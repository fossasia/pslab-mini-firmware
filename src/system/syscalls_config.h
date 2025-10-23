#ifndef SYSCALLS_CONFIG_H
#define SYSCALLS_CONFIG_H

#ifndef SYSCALLS_UART_BUS
// 1 maps to USART6, which is UART_BUS_HEADER.
// TODO@bessman: modularize this.
#define SYSCALLS_UART_BUS 1
#endif

#ifndef SYSCALLS_UART_TX_BUFFER_SIZE
#define SYSCALLS_UART_TX_BUFFER_SIZE 1024
#endif

#endif // SYSCALLS_CONFIG_H
