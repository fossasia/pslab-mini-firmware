/**
 ******************************************************************************
 * @file           : main.c
 ******************************************************************************
 */
#include <stdio.h>
#include <stdint.h>

#include "stm32h563xx.h"
#include "uart.h"

#define GPIOF_ENR (0x01<<5)
#define GPIOG_ENR (0x01<<6)

#define LED2_reset  (0x03<<8)
#define LED2_set    (0x01<<8)

#define LED2_toggle (0x01<<4)

#define LED3_reset  (0x03<<8)
#define LED3_set    (0x01<<8)

#define LED3_toggle (0x01<<4)

char key = 0;

// Function declarations
static void usart_rx_callback(void);
extern void usart_tx_callback(void);

char stringBuffer[60];
uint8_t stringLength;

int main(void)
{
    uart3_rx_interrupt_init();

    // Initialize LEDs
    RCC->AHB2ENR |= GPIOF_ENR;
    GPIOF->MODER &= ~(0x01<<9);
    GPIOF->MODER |= (0x01<<8);

    RCC->AHB2ENR |= GPIOG_ENR;
    GPIOG->MODER &= ~(LED3_reset);
    GPIOG->MODER |= (LED3_set);

    // Send initial test string
    stringLength = sprintf(stringBuffer, "UART3 Interrupt Test - Ready!\r\n");
    uart3_int_write(stringBuffer, stringLength);

    while(1){

    }
}

static void usart_rx_callback(void){
    key = USART3->RDR;

    // Toggle LED to indicate received character
    GPIOG->ODR ^= LED3_toggle;

    // Echo back the received character
    stringLength = sprintf(stringBuffer, "Received: %c\r\n", key);
    uart3_int_write(stringBuffer, stringLength);
}

void USART3_IRQHandler(void){

    // Check for receive interrupt
    if (USART3->ISR & UART3_ISR_RXNE){
        usart_rx_callback();
    }

    // Check for transmit interrupt
    if(USART3->ISR & UART3_ISR_TXE){
        usart_tx_callback();
    }
}
