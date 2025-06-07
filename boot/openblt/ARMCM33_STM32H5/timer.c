/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/timer.c
* \brief        Bootloader timer driver source file.
* \ingroup      Target_ARMCM33_STM32H5
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
#include "stm32h5xx.h"                           /* STM32 CPU and HAL header           */
#include "stm32h5xx_ll_bus.h"                    /* STM32 LL BUS header                */
#include "stm32h5xx_ll_rcc.h"                    /* STM32 LL RCC header                */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Frequency of the configured timer peripheral's free running counter in Hz.
 *         This shoud be 100 kHz.
 */
#define TIMER_COUNTER_FREQ_HZ     (100000U)

/** \brief Number of free running counter counts that equal one millisecond. */
#define TIMER_COUNTS_PER_MS       (TIMER_COUNTER_FREQ_HZ / 1000U)


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Local variable for storing the number of milliseconds that have elapsed since
 *         startup.
 */
static blt_int32u millisecond_counter;

/** \brief Buffer for storing the last value of the free running counter. */
static blt_int16u free_running_counter_last;


/************************************************************************************//**
** \brief     Initializes the polling based millisecond timer driver.
** \attention To keep the ROM footprint low, this function aims to only use LL driver
**            inline functions.
** \return    none.
**
****************************************************************************************/
void TimerInit(void)
{
  blt_int8u  tim_multiplier;
  blt_int32u pclk_frequency;
  blt_int32u pclk_tim_frequency;

  /* All STM32H5 derivatives support a TIM1 peripheral. Its free running counter will be
   * used to realize the polling based millisecond time reference in this module.
   * Start by enabling the periperhal.
   */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
  /* The TIM1 peripheral clock is derived from PCLK. Obtain the PCLK frequency. */
  pclk_frequency = __LL_RCC_CALC_PCLK2_FREQ(SystemCoreClock, LL_RCC_GetAPB2Prescaler());
  /* According to the clock tree diagram in the RCC chapter of the reference manual,
   * the TPCLK frequency = PLCK * 1, when the APB1 prescaler is 1, otherwise it is
   * PCLK * 2.
   */
  tim_multiplier = (LL_RCC_GetAPB2Prescaler() == LL_RCC_APB2_DIV_1) ? 1U : 2U;
  /* Obtain the TPCLK frequency. */
  pclk_tim_frequency = pclk_frequency * tim_multiplier;
  /* Configure the free running counter as a 16-bit upwards counter that runs at the
   * desired frequency and enable it.
   */
  TIM1->CR1 = TIM_CR1_CEN;
  TIM1->ARR = 65535U;
  TIM1->PSC = (pclk_tim_frequency / TIMER_COUNTER_FREQ_HZ) - 1U;
  /* Generate an update event to reload the prescaler immediately. */
  TIM1->EGR |= TIM_EGR_UG;
  /* Initialize locals. */
  millisecond_counter = 0;
  free_running_counter_last = (blt_int16u)TIM1->CNT;
} /*** end of TimerInit ***/


/************************************************************************************//**
** \brief     Reset the timer by placing the timer back into it's default reset
**            configuration.
** \return    none.
**
****************************************************************************************/
void TimerReset(void)
{
  /* Bring the TIM1 peripheral back into its reset state and disable its clock. */
  LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_TIM1);
  LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_TIM1);
  LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM1);
  /* Set the SysTick's registers back into the default reset value. Note that this module
   * does not use the SysTick, but HAL_Init() did initialize it.
   */
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
} /* end of TimerReset */


/************************************************************************************//**
** \brief     Updates the millisecond timer.
** \return    none.
**
****************************************************************************************/
void TimerUpdate(void)
{
  blt_int16u free_running_counter_now;
  blt_int16u delta_counts;
  blt_int16u ms_counts;

  /* Get the current value of the free running counter. */
  free_running_counter_now = (blt_int16u)TIM1->CNT;
  /* Calculate the number of counts that passed since the detection of the last
   * millisecond event. Note that this calculation also works, in case the free running
   * counter overflowed, thanks to integer math.
   */
  delta_counts = free_running_counter_now - free_running_counter_last;

  /* Did one or more milliseconds pass since the last event? */
  if (delta_counts >= TIMER_COUNTS_PER_MS)
  {
    /* Calculate how many milliseconds passed. */
    ms_counts = delta_counts / TIMER_COUNTS_PER_MS;
    /* Update the millisecond counter. */
    millisecond_counter += ms_counts;
    /* Store the counter value of the last millisecond event, to detect the next one. */
    free_running_counter_last += (ms_counts * TIMER_COUNTS_PER_MS);
  }
} /*** end of TimerUpdate ***/


/************************************************************************************//**
** \brief     Obtains the counter value of the millisecond timer.
** \return    Current value of the millisecond timer.
**
****************************************************************************************/
blt_int32u TimerGet(void)
{
  /* Updating timer here allows this function to be called in a loop with timeout
   * detection.
   */
  TimerUpdate();
  /* Read and return the amount of milliseconds that passed since initialization. */
  return millisecond_counter;
} /*** end of TimerGet ***/


/************************************************************************************//**
** \brief     Override for the HAL driver's GetTick() functionality. This is needed
**            because the bootloader doesn't use interrupts, but the HAL's tick
**            functionality assumes that it does. This will cause the HAL_Delay()
**            function to not work properly. As a result of this override, the HAL's
**            tick functionality works in polling mode.
** \return    Current value of the millisecond timer.
**
****************************************************************************************/
uint32_t HAL_GetTick(void)
{
  /* Link to the bootloader's 1ms timer. */
  return TimerGet();
} /*** end of HAL_GetTick ***/


/*********************************** end of timer.c ************************************/
