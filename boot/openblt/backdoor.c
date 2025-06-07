/************************************************************************************//**
* \file         Source/backdoor.c
* \brief        Bootloader backdoor entry source file.
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
* Macro definitions
****************************************************************************************/
#if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
#ifndef BOOT_BACKDOOR_ENTRY_TIMEOUT_MS
/** \brief Sets the time in milliseconds that the backdoor is open, but allow an
 *         override for this time. To change this value, simply add the macro
 *         BOOT_BACKDOOR_ENTRY_TIMEOUT_MS to blt_conf.h with your desired backdoor 
 *         open time in milliseconds.
 */
#define BOOT_BACKDOOR_ENTRY_TIMEOUT_MS  (500)
#endif
#endif /* BOOT_BACKDOOR_HOOKS_ENABLE == 0 */


/****************************************************************************************
* Hook functions
****************************************************************************************/
#if (BOOT_BACKDOOR_HOOKS_ENABLE > 0)
extern void BackDoorInitHook(void);
extern blt_bool BackDoorEntryHook(void);
#endif


/****************************************************************************************
* Local data declarations
****************************************************************************************/
#if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
/** \brief To determine if the backdoor is open or closed. */
static blt_bool backdoorOpen;
/** \brief To determine how long the backdoor has been open in milliseconds. */
static blt_int32u backdoorOpenTime;
/** \brief In certain scenarios it is desired to be able to extend the default backdoor
 *         entry time at runtime. This variable holds the extension time in milliseconds.
 *         Note that this value must be initialized to zero here and not in function
 *         BackDoorInit(), because BackDoorInit() is one of the last functions called
 *         in BootInit(). This order should not be changed otherwise there is a chance
 *         that the timed backdoor partially or completely times out during BootInit().
 *         Initializing the variable here, allows function BackDoorSetExtension() to be
 *         called before BackDoorInit() was called.
 */
static blt_int32u backdoorExtensionTime = 0;
#endif


/************************************************************************************//**
** \brief     Initializes the backdoor entry option.
** \return    none
**
****************************************************************************************/
void BackDoorInit(void)
{
#if (BOOT_BACKDOOR_HOOKS_ENABLE > 0)
  /* initialize application's backdoor functionality */
  BackDoorInitHook();

  /* attempt to start the user program when no backdoor entry is requested */
  if (BackDoorEntryHook() == BLT_FALSE)
  {
    /* this function does not return if a valid user program is present */
    CpuStartUserProgram();
  }
#if (BOOT_FILE_SYS_ENABLE > 0)
  else
  {
    /* the backdoor is open so we should check if a update from locally  attached storage
     * is requested and, if so, start it.
     */
    FileHandleFirmwareUpdateRequest();
  }
#endif
#else
  /* open the backdoor after a reset */
  backdoorOpen = BLT_TRUE;
  BackDoorRestartTimer();
#endif
  /* perform the first check that open/closes the backdoor */
  BackDoorCheck();
} /*** end of BackDoorInit ***/


/************************************************************************************//**
** \brief     The default backdoor entry feature keeps the bootloader active for a
**            predetermined time after reset, allowing the host application to
**            establish a connection and start a programming sequence. This function
**            controls the opening/closing of the backdoor.
** \return    none
**
****************************************************************************************/
void BackDoorCheck(void)
{
#if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
#if (BOOT_COM_ENABLE > 0)
  /* check if a connection with the host was already established. in this case the
   * backdoor stays open anyway, so no need to check if it needs to be closed.
   */
  if (ComIsConnected() == BLT_TRUE)
  {
    return;
  }
#endif
#if (BOOT_FILE_SYS_ENABLE > 0)
  /* check if the file module is busy, indicating that a firmware update through the
   * locally attached storage is in progress. in this case the backdoor stays open
   * anyway, so no need to check if it needs to be closed.
   */
  if (FileIsIdle() == BLT_FALSE)
  {
    return;
  }
#endif

  /* when the backdoor is still open, check if it's time to close it */
  if (backdoorOpen == BLT_TRUE)
  {
    /* check if the backdoor entry time window elapsed */
    if (TimerGet() >= (BOOT_BACKDOOR_ENTRY_TIMEOUT_MS + backdoorExtensionTime + backdoorOpenTime))
    {
      /* close the backdoor */
      backdoorOpen = BLT_FALSE;
#if (BOOT_FILE_SYS_ENABLE > 0)
      /* during the timed backdoor no remote update request was detected. now do one
       * last check to see if a firmware update from locally attached storage is
       * pending.
       */
      if (FileHandleFirmwareUpdateRequest() == BLT_FALSE)
#endif
      {
        /* no firmware update requests detected, so attempt to start the user program.
         * this function does not return if a valid user program is present.
         */
        CpuStartUserProgram();
      }
    }
  }
#endif
} /*** end of BackDoorCheck ***/


#if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
/************************************************************************************//**
** \brief     Sets the amount of milliseconds that the default backdoor timeout time
**            (BOOT_BACKDOOR_ENTRY_TIMEOUT_MS) is extended.
** \param     extension_ms Extension time in milliseconds.
** \return    none
**
****************************************************************************************/
void BackDoorSetExtension(blt_int32u extension_ms)
{
  /* update the extension time */
  backdoorExtensionTime = extension_ms;
} /*** end of BackDoorSetExtension ***/


/************************************************************************************//**
** \brief     Gets the amount of milliseconds that the default backdoor timeout time
**            (BOOT_BACKDOOR_ENTRY_TIMEOUT_MS) is extended.
** \return    Extension time in milliseconds.
**
****************************************************************************************/
blt_int32u BackDoorGetExtension(void)
{
  /* read out and reutrn the currently configured extension time */
  return backdoorExtensionTime;
} /*** end of BackDoorGetExtension ***/


/************************************************************************************//**
** \brief     Restarts the timed backdoor timer. It uses the current system time as the
**            start time. The backdoor stays open for BOOT_BACKDOOR_ENTRY_TIMEOUT_MS
**            after this start time, possibly extended in case BackDoorSetExtension() was
**            called.
** \return    none
**
****************************************************************************************/
void BackDoorRestartTimer(void)
{
  /* only restart the time if the backdoor is actually still open */
  if (backdoorOpen == BLT_TRUE)
  {
    backdoorOpenTime = TimerGet();
  }
} /*** end of BackDoorRestartTimer ***/
#endif /* BOOT_BACKDOOR_HOOKS_ENABLE == 0 */


/*********************************** end of backdoor.c *********************************/
