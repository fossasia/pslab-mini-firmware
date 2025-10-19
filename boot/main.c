/************************************************************************************//**
* \file         Demo/ARMCM33_STM32H5_Nucleo_H563ZI_GCC/Boot/main.c
* \brief        Bootloader application source file.
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
#include "stm32h5xx.h"                           /* STM32 CPU and HAL header           */
#include "stm32h5xx_ll_pwr.h"                    /* STM32 LL PWR header                */
#include "stm32h5xx_ll_rcc.h"                    /* STM32 LL RCC header                */
#include "stm32h5xx_ll_bus.h"                    /* STM32 LL BUS header                */
#include "stm32h5xx_ll_system.h"                 /* STM32 LL SYSTEM header             */
#include "stm32h5xx_ll_utils.h"                  /* STM32 LL UTILS header              */
#include "stm32h5xx_ll_gpio.h"                   /* STM32 LL GPIO header               */
#include "stm32h5xx_ll_icache.h"                 /* STM32 LL internal cache header     */
#include "stm32h5xx_ll_crs.h"                    /* STM32 LL CRS header                */


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static void Init(void);
static void SystemClock_Config(void);


/************************************************************************************//**
** \brief     This is the entry point for the bootloader application and is called
**            by the reset interrupt vector after the C-startup routines executed.
** \return    Program return code.
**
****************************************************************************************/
int main(void)
{
  /* Initialize the microcontroller */
  Init();
  /* Initialize the bootloader */
  BootInit();
#if (BOOT_COM_DEFERRED_INIT_ENABLE == 1)
  /* The bootloader is configured to NOT initialize the TCP/IP network stack by default
   * to bypass unnecessary delay times before starting the user program. The TCP/IP net-
   * work tack is now only initialized when: (a) no valid user program is detected, or
   * (b) a forced backdoor entry occurred (CpuUserProgramStartHook() returned BLT_FALSE).
   *
   * These demo bootloader and user programs have one extra feature implemented for
   * demonstration purposes. the demo user program can detect firmware update requests
   * from the TCP/IP network in which case it activates the bootloader. But...the
   * TCP/IP network stack will not be initialized in this situation. For this reason
   * the shared parameter module was integrated in both the bootloader and user program.
   * more information about the shared parameter module can be found here:
   *   https://www.feaser.com/en/blog/?p=216
   *
   * The shared parameter at the first index (0) contains a flag. This flag is set to
   * 1, right before the user program activates this bootloader, to explicitly request
   * the bootloader to initialize the TCP/IP network stack. This makes it possible for
   * a firmware update to proceed. The code here reads out this flag and performs the
   * TCP/IP network stack initialization when requested.
   */
  blt_int8u deferredInitRequestFlag = 0;
  SharedParamsReadByIndex(0, &deferredInitRequestFlag);
  if (deferredInitRequestFlag == 1)
  {
    /* Explicitly initialize all communication interface for which the deferred
     * initialization feature was enabled.
     */
    ComDeferredInit();
  }
#endif
  /* Start the infinite program loop */
  while (1)
  {
    /* Run the bootloader task */
    BootTask();
  }

  /* program should never get here */
  return 0;
} /*** end of main ***/


/************************************************************************************//**
** \brief     Initializes the microcontroller.
** \return    none.
**
****************************************************************************************/
static void Init(void)
{
  /* HAL library initialization */
  HAL_Init();
  /* Configure system clock */
  SystemClock_Config();
  /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral. */
  LL_PWR_DisableUCPDDeadBattery();
  /* Enable instruction cache in 1-way (direct mapped cache). */
  LL_ICACHE_SetMode(LL_ICACHE_1WAY);
  LL_ICACHE_Enable();
} /*** end of Init ***/


/************************************************************************************//**
** \brief     System Clock Configuration. This code was created by CubeMX and configures
**            the system clock to match the configuration in the bootloader's
**            configuration (blt_conf.h), specifically the macros:
**            BOOT_CPU_SYSTEM_SPEED_KHZ and BOOT_CPU_XTAL_SPEED_KHZ.
**            Note that the Lower Layer drivers were selected in CubeMX for the RCC
**            subsystem.
** \return    none.
**
****************************************************************************************/
static void SystemClock_Config(void)
{
  /* Set flash latency. */
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_5)
  {
    ;
  }

  /* Configure the main internal regulator output voltage. */
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
  while (LL_PWR_IsActiveFlag_VOS()== 0)
  {
    ;
  }

  /* Enable the HSE clock with crystal oscillator. */
  LL_RCC_HSE_Enable();
  /* Wait till HSE is ready. */
  while(LL_RCC_HSE_IsReady() != 1)
  {
    ;
  }

  /* Enable the HSI clock. */
  LL_RCC_HSI48_Enable();
  /* Wait till HSI48 is ready. */
  while(LL_RCC_HSI48_IsReady() != 1)
  {
    ;
  }

  /* Configure and enable the PLL. */
  LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSE);
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(5);
  LL_RCC_PLL1_SetN(100);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(25);
  LL_RCC_PLL1_SetR(2);
  LL_RCC_PLL1Q_Enable();
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1_Enable();
  /* Wait till PLL is ready */
  while(LL_RCC_PLL1_IsReady() != 1)
  {
    ;
  }

  /* Set the source for the system clock. */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
  /* Wait till System clock is ready. */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
  {
    ;
  }

  /* Configure peripheral bus prescalers. */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);

  /* Update the system clock speed setting. */
  LL_SetSystemCoreClock(BOOT_CPU_SYSTEM_SPEED_KHZ * 1000u);

  /* Enable and configure the clock recovery system. */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_CRS);
  LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_CRS);
  LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_CRS);
  LL_CRS_SetSyncDivider(LL_CRS_SYNC_DIV_1);
  LL_CRS_SetSyncPolarity(LL_CRS_SYNC_POLARITY_RISING);
  LL_CRS_SetSyncSignalSource(LL_CRS_SYNC_SOURCE_USB);
  LL_CRS_SetReloadCounter(__LL_CRS_CALC_CALCULATE_RELOADVALUE(48000000,1000));
  LL_CRS_SetFreqErrorLimit(34);
  LL_CRS_SetHSI48SmoothTrimming(32);

#if (BOOT_COM_RS232_ENABLE > 0)
  /* UART clock source configuration. */
  LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);
#endif

#if (BOOT_COM_USB_ENABLE > 0)
  /* USB clock source configuration. */
  LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);
#endif
} /*** end of SystemClock_Config ***/


/************************************************************************************//**
** \brief     Initializes the Global MSP. This function is called from HAL_Init()
**            function to perform system level initialization (GPIOs, clock, DMA,
**            interrupt).
** \return    none.
**
****************************************************************************************/
void HAL_MspInit(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO ports clock enable. */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);

#if (BOOT_COM_RS232_ENABLE > 0)
  /* UART clock enable. */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
#endif

#if (BOOT_COM_CAN_ENABLE > 0)
  /* CAN clock enable. */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_FDCAN);
#endif

  /* Configure GPIO pin for the LED. */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12);

  /* Configure GPIO pin for (optional) backdoor entry input. */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#if (BOOT_COM_RS232_ENABLE > 0)
  /* UART TX and RX GPIO pin configuration. */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);
#endif

#if (BOOT_COM_CAN_ENABLE > 0)
  /* CAN TX and RX GPIO pin configuration. */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_9;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif

#if (BOOT_COM_USB_ENABLE > 0)
  /* USB clock enable. */
  LL_PWR_EnableVddUSB();
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USB);
#endif
} /*** end of HAL_MspInit ***/


/************************************************************************************//**
** \brief     DeInitializes the Global MSP. This function is called from HAL_DeInit()
**            function to perform system level de-initialization (GPIOs, clock, DMA,
**            interrupt).
** \return    none.
**
****************************************************************************************/
void HAL_MspDeInit(void)
{
  /* Reset the RCC clock configuration to the default reset state. */
  LL_RCC_DeInit();

  /* Reset GPIO pin for the LED to turn it off. */
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12);

  /* Deinit used GPIOs. */
  LL_GPIO_DeInit(GPIOH);
  LL_GPIO_DeInit(GPIOD);
  LL_GPIO_DeInit(GPIOC);
  LL_GPIO_DeInit(GPIOB);
  LL_GPIO_DeInit(GPIOA);

#if (BOOT_COM_USB_ENABLE > 0)
  /* USB clock disable. */
  LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USB);
  LL_PWR_DisableVDDUSB();
#endif

#if (BOOT_COM_CAN_ENABLE > 0)
  /* CAN clock disable. */
  LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_FDCAN);
#endif
#if (BOOT_COM_RS232_ENABLE > 0)
  /* UART clock disable. */
  LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART3);
#endif

  /* GPIO ports clock disable. */
  LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOH);
  LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
  LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
  LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

  /* Enable the internal pull-up in Dead Battery pins of UCPD peripheral. */
  LL_PWR_EnableUCPDDeadBattery();
} /*** end of HAL_MspDeInit ***/


/************************************************************************************//**
** \brief     This function handles the SysTick interrupt. The HAL driver is initialized
**            before the bootloader disables the global interrupts and reconfigures the
**            SysTick. It is theoretically possible that the SysTick interrupt still
**            fires before the timer driver disables it. Therefore the handler is
**            implemented here. If not, then the default handler from the C startup
**            code is used, which hangs the system.
** \return    none.
**
****************************************************************************************/
void SysTick_Handler(void)
{
  /* Nothing to do here. */
} /*** end of SysTick_Handler ***/


/*********************************** end of main.c *************************************/
