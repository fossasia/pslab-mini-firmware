/************************************************************************************//**
* \file         Source/cop.c
* \brief        Bootloader watchdog module source file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2011  by Feaser    http://www.feaser.com    All rights reserved
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


/****************************************************************************************
* Hook functions
****************************************************************************************/
#if (BOOT_COP_HOOKS_ENABLE > 0)
extern void CopInitHook(void);
extern void CopServiceHook(void);
#endif


/************************************************************************************//**
** \brief     Watchdog initialization function.
** \return    none
**
****************************************************************************************/
void CopInit(void)
{
#if (BOOT_COP_HOOKS_ENABLE > 0)
  CopInitHook();
#endif
} /*** end of CopInit ***/


/************************************************************************************//**
** \brief     Watchdog service function to prevent the watchdog from timing out.
** \return    none
**
****************************************************************************************/
void CopService(void)
{
#if (BOOT_COP_HOOKS_ENABLE > 0)
  CopServiceHook();
#endif
} /*** end of CopService ***/


/*********************************** end of cop.c **************************************/
