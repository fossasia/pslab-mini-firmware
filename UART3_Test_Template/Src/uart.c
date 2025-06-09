/*
 * uart.c
 *
 *  Created on: Jun 7, 2025
 *      Author: DELL
 */

#include "uart.h"
#include "stm32h563xx.h"

#define GPIOD_ENR (0x01<<3)

#define GPIOD_8_reset (0x03<<16)
#define GPIOD_8_set   (0x01<<17)

#define GPIOD_9_reset (0x03<<18)
#define GPIOD_9_set   (0x01<<19)

#define GPIOD_8_AF7_reset  (0x0F<<0)
#define GPIOD_8_AF7_set    (0x07<<0)

#define GPIOD_9_AF7_reset  (0x0F<<4)
#define GPIOD_9_AF7_set    (0x07<<4)

#define SYS_FREQ            32000000
#define APB1_FREQ           SYS_FREQ

#define UART_BAUDRATE       57600

#define UART3_EN          (0x01<<18)

#define UART3_UE            (0x01<<0)
#define UART3_TE            (0x01<<3)
#define UART3_RE            (0x01<<2)

#define UART3_ISR_RXNE      (0x01<<5)
#define UART3_ISR_TXE       (0x01<<7)

#define RXNEIE              (0x01<<5)
#define TXEIE               (0x01<<7)
#define TCIE                (0x01<<6)

// Static variables for interrupt-based transmission
static uint8_t tx_buffer[60];
static uint8_t tx_length = 0;
static uint8_t tx_index = 0;
static uint8_t tx_busy = 0;

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t Periph_clk, uint32_t BaudRate);
static uint32_t compute_uart_bd(uint32_t Periph_clk, uint32_t BaudRate);

void uart3_rx_interrupt_init(void){
    RCC->AHB2ENR |= GPIOD_ENR;

    // Configure PD8 (TX) and PD9 (RX) for alternate function mode
    GPIOD->MODER &= ~(GPIOD_8_reset);
    GPIOD->MODER |=  (0x02 << 16); // AF mode = 10

    GPIOD->MODER &= ~(GPIOD_9_reset);
    GPIOD->MODER |=  (0x02 << 18); // AF mode = 10

    // Set alternate function 7 for UART3
    GPIOD->AFR[1] &= ~(GPIOD_8_AF7_reset);
    GPIOD->AFR[1] |= GPIOD_8_AF7_set;

    GPIOD->AFR[1] &= ~(GPIOD_9_AF7_reset);
    GPIOD->AFR[1] |= GPIOD_9_AF7_set;

    RCC->APB1LENR |= UART3_EN;

    uart_set_baudrate(USART3, APB1_FREQ, UART_BAUDRATE);

    USART3->CR1 &= ~(0x01<<29); // FIFO disable
    USART3->CR1 |= UART3_TE | UART3_RE;

    USART3->CR1 |= RXNEIE; // Enable RX interrupt

    NVIC_EnableIRQ(USART3_IRQn);

    USART3->CR1 |= UART3_UE;
}

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t Periph_clk, uint32_t BaudRate){
    USARTx->BRR = compute_uart_bd(Periph_clk, BaudRate);
}

static uint32_t compute_uart_bd(uint32_t Periph_clk, uint32_t BaudRate){
    return ((Periph_clk + (BaudRate/2U)) / BaudRate);
}

void uart3_int_write(char buffer[], uint8_t length){
    if(tx_busy) return; // Don't start new transmission if busy

    // Copy buffer to static buffer
    for(int i = 0; i < length && i < 60; i++){
        tx_buffer[i] = buffer[i];
    }

    tx_length = length;
    tx_index = 0;
    tx_busy = 1;

    // Send first character and enable TXE interrupt
    if(tx_length > 0){
        USART3->TDR = tx_buffer[tx_index++];
        USART3->CR1 |= TXEIE;
    }
}


void usart_tx_callback(void){
    if(tx_index < tx_length){
        USART3->TDR = tx_buffer[tx_index++];
    } else {
        USART3->CR1 &= ~TXEIE; // Disable TXE interrupt
        tx_busy = 0; // Mark transmission as complete
    }
}
