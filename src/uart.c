/****************************************************************************************
* Include files
****************************************************************************************/
#include "header.h"                                    /* generic header               */
#include "stm32h5xx_hal.h"                             /* STM32 HAL driver             */
#include "uart.h"                                   /* UART driver                  */


/****************************************************************************************
 * Variables
 ****************************************************************************************/
UART_HandleTypeDef huart3;  /* UART handle for USART3 */


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{

}


 /************************************************************************************//**
  ** \brief     This function initializes the USART3 peripheral.
  **            It configures the GPIO pins for USART3 TX and RX, sets the baud rate,
  **            word length, stop bits, parity, mode, hardware flow control, and
  **            oversampling for the USART3 peripheral.
  ** \return    none.
  **
  ****************************************************************************************/

  void usart3_init(void)
  {	//ENABLE GPIO pins required for USART3

      /* Enable the GPIO clock for USART3 pins */
      __HAL_RCC_GPIOD_CLK_ENABLE();

      /* Configure PD8 and PD9 as alternate function for USART3 */
      /* USART3 TX on PD8 and RX on PD9 */
      GPIO_InitTypeDef 	GPIO_InitStruct ={0};
      __HAL_RCC_GPIOD_CLK_ENABLE();
      GPIO_InitStruct.Pin   = GPIO_PIN_8;
      GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull  =  GPIO_NOPULL;
      GPIO_InitStruct.Speed =  GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
      HAL_GPIO_Init (GPIOD, &GPIO_InitStruct);

      GPIO_InitStruct.Pin = GPIO_PIN_9;
      HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

      //ENABLE the UART module clock
      __HAL_RCC_USART3_CLK_ENABLE();

      // Initialize the USART3 peripheral
      huart3.Instance = USART3;

      huart3.Init.BaudRate = 115200; // Set the baud rate to 115200
      huart3.Init.WordLength = UART_WORDLENGTH_8B;
      huart3.Init.StopBits = UART_STOPBITS_1;
      huart3.Init.Parity = UART_PARITY_NONE;
      huart3.Init.Mode = UART_MODE_TX_RX;
      huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
      huart3.Init.OverSampling = UART_OVERSAMPLING_16;

      HAL_UART_Init(&huart3);

  } /*** end of usart3_init ***/
  