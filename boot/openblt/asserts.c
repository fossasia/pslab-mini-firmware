/************************************************************************************//**
* \file         Source/asserts.c
* \brief        Bootloader assertion module source file.
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


#ifndef NDEBUG
/************************************************************************************//**
** \brief     Called when a runtime assertion failed. It stores information about where
**            the assertion occurred and halts the software program.
** \param     file   Name of the source file where the assertion occurred.
** \param     line   Linenumber in the source file where the assertion occurred.
** \return    none
**
****************************************************************************************/
void AssertFailure(blt_char *file, blt_int32u line)
{
  /* hang the software so that it requires a hard reset */
  for (;;)
  {
    /* keep servicing the watchdog so that this one does not cause a reset */
    CopService();
  }
} /*** end of AssertFailure ***/
#endif /* !NDEBUG */


/*********************************** end of assert.c ***********************************/
