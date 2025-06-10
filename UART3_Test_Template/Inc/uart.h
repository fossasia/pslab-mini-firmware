/*
 * uart.h
 *
 *  Created on: Jun 7, 2025
 *      Author: DELL
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "stm32h563xx.h"

void uart3_rx_interrupt_init(void);
void uart3_int_write(char buffer[], uint8_t length);

#define UART3_ISR_RXNE      (0x01<<5)
#define UART3_ISR_TXE       (0x01<<7)
#define UART3_ISR_TC        (0x01<<6)

#endif /* UART_H_ */
