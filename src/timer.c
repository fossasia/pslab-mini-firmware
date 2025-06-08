/************************************************************************************//**
* \file         Demo/ARMCM33_STM32H5_Nucleo_H563ZI_GCC/Prog/timer.c
* \brief        Timer driver source file.
* \ingroup      Prog_ARMCM33_STM32H5_Nucleo_H563ZI_GCC
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2024  by Feaser    http://www.feaser.com    All rights reserved
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* This file is part of OpenBLT. OpenBLT is free software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* OpenBLT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU General Public License for more details.
*
* You have received a copy of the GNU General Public License along with OpenBLT. It
* should be located in ".\Doc\license.html". If not, contact Feaser to obtain a copy.
*
* \endinternal
****************************************************************************************/

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
