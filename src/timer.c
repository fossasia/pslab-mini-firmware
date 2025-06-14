/****************************************************************************************
* Include files
****************************************************************************************/
#include "header.h"                                    /* generic header               */
#include "stm32h5xx_hal.h"                             /* STM32 HAL driver             */

/************************************************************************************//**
** \brief     Initializes the timer.
** \return    none.
**
****************************************************************************************/
void TimerInit(void)
{
  /* Configure the Systick interrupt time for 1 millisecond. */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
  /* Configure the Systick. */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  /* SysTick_IRQn interrupt configuration. */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
} /*** end of TimerInit ***/


/************************************************************************************//**
** \brief     Obtains the counter value of the millisecond timer.
** \return    Current value of the millisecond timer.
**
****************************************************************************************/
unsigned long TimerGet(void)
{
  /* Read and return the tick counter value. */
  return HAL_GetTick();
} /*** end of TimerGet ***/


/************************************************************************************//**
** \brief     Interrupt service routine of the timer.
** \return    none.
**
****************************************************************************************/
void SysTick_Handler(void)
{
  /* Increment the tick counter. */
  HAL_IncTick();
  /* Invoke the system tick handler. */
  HAL_SYSTICK_IRQHandler();
} /*** end of TimerISRHandler ***/


/*********************************** end of timer.c ************************************/
