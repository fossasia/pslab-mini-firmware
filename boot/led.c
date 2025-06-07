/************************************************************************************//**
* \file         Demo/ARMCM33_STM32H5_Nucleo_H563ZI_GCC/Boot/led.c
* \brief        LED driver source file.
* \ingroup      Boot_ARMCM33_STM32H5_Nucleo_H563ZI_GCC
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
#include "boot.h"                                /* bootloader generic header          */
#include "led.h"                                 /* module header                      */
#include "stm32h5xx.h"                           /* STM32 registers and drivers        */
#include "stm32h5xx_ll_gpio.h"                   /* STM32 LL GPIO header               */


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Holds the desired LED blink interval time. */
static blt_int16u ledBlinkIntervalMs;


/************************************************************************************//**
** \brief     Initializes the LED blink driver.
** \param     interval_ms Specifies the desired LED blink interval time in milliseconds.
** \return    none.
**
****************************************************************************************/
void LedBlinkInit(blt_int16u interval_ms)
{
  /* store the interval time between LED toggles */
  ledBlinkIntervalMs = interval_ms;
} /*** end of LedBlinkInit ***/


/************************************************************************************//**
** \brief     Task function for blinking the LED as a fixed timer interval.
** \return    none.
**
****************************************************************************************/
void LedBlinkTask(void)
{
  static blt_bool ledOn = BLT_FALSE;
  static blt_int32u nextBlinkEvent = 0;

  /* check for blink event */
  if (TimerGet() >= nextBlinkEvent)
  {
    /* toggle the LED state */
    if (ledOn == BLT_FALSE)
    {
      ledOn = BLT_TRUE;
      LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0);
    }
    else
    {
      ledOn = BLT_FALSE;
      LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
    }
    /* schedule the next blink event */
    nextBlinkEvent = TimerGet() + ledBlinkIntervalMs;
  }
} /*** end of LedBlinkTask ***/


/************************************************************************************//**
** \brief     Cleans up the LED blink driver. This is intended to be used upon program
**            exit.
** \return    none.
**
****************************************************************************************/
void LedBlinkExit(void)
{
  /* turn the LED off */
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
} /*** end of LedBlinkExit ***/


/*********************************** end of led.c **************************************/
