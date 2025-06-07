/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/nvm.c
* \brief        Bootloader non-volatile memory driver source file.
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
#include "flash.h"


/****************************************************************************************
* Hook functions
****************************************************************************************/
#if (BOOT_NVM_HOOKS_ENABLE > 0)
extern void      NvmInitHook(void);
extern blt_int8u NvmWriteHook(blt_addr addr, blt_int32u len, blt_int8u *data);
extern blt_int8u NvmEraseHook(blt_addr addr, blt_int32u len);
extern blt_bool  NvmDoneHook(void);
#endif

#if (BOOT_NVM_CHECKSUM_HOOKS_ENABLE > 0)
extern blt_bool  NvmWriteChecksumHook(void);
extern blt_bool  NvmVerifyChecksumHook(void);
#endif


/************************************************************************************//**
** \brief     Initializes the NVM driver.
** \return    none.
**
****************************************************************************************/
void NvmInit(void)
{
#if (BOOT_NVM_HOOKS_ENABLE > 0)
  /* give the application a chance to initialize a driver for operating on NVM
   * that is not by default supported by this driver.
   */
  NvmInitHook();
#endif

  /* init the internal driver */
  FlashInit();
} /*** end of NvmInit ***/


/************************************************************************************//**
** \brief     Programs the non-volatile memory.
** \param     addr Start address.
** \param     len  Length in bytes.
** \param     data Pointer to the data buffer.
** \return    BLT_TRUE if successful, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool NvmWrite(blt_addr addr, blt_int32u len, blt_int8u *data)
{
#if (BOOT_NVM_HOOKS_ENABLE > 0)
  blt_int8u result = BLT_NVM_NOT_IN_RANGE;
#endif

#if (BOOT_NVM_HOOKS_ENABLE > 0)
  /* give the application a chance to operate on memory that is not by default supported
   * by this driver.
   */
  result = NvmWriteHook(addr, len, data);

  /* process the return code */
  if (result == BLT_NVM_OKAY)
  {
    /* data was within range of the additionally supported memory and succesfully
     * programmed, so we are all done.
     */
    return BLT_TRUE;
  }
  else if (result == BLT_NVM_ERROR)
  {
    /* data was within range of the additionally supported memory and attempted to be
     * programmed, but an error occurred, so we can't continue.
     */
    return BLT_FALSE;
  }
#endif

  /* still here so the internal driver should try and perform the program operation */
  return FlashWrite(addr, len, data);
} /*** end of NvmWrite ***/


/************************************************************************************//**
** \brief     Erases the non-volatile memory.
** \param     addr Start address.
** \param     len  Length in bytes.
** \return    BLT_TRUE if successful, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool NvmErase(blt_addr addr, blt_int32u len)
{
#if (BOOT_NVM_HOOKS_ENABLE > 0)
  blt_int8u result = BLT_NVM_NOT_IN_RANGE;
#endif

#if (BOOT_NVM_HOOKS_ENABLE > 0)
  /* give the application a chance to operate on memory that is not by default supported
   * by this driver.
   */
  result = NvmEraseHook(addr, len);

  /* process the return code */
  if (result == BLT_NVM_OKAY)
  {
    /* address was within range of the additionally supported memory and succesfully
     * erased, so we are all done.
     */
    return BLT_TRUE;
  }
  else if (result == BLT_NVM_ERROR)
  {
    /* address was within range of the additionally supported memory and attempted to be
     * erased, but an error occurred, so we can't continue.
     */
    return BLT_FALSE;
  }
#endif

  /* still here so the internal driver should try and perform the erase operation */
  return FlashErase(addr, len);
} /*** end of NvmErase ***/


/************************************************************************************//**
** \brief     Verifies the checksum, which indicates that a valid user program is
**            present and can be started.
** \return    BLT_TRUE if successful, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool NvmVerifyChecksum(void)
{
#if (BOOT_NVM_CHECKSUM_HOOKS_ENABLE > 0)
  /* check checksum using the application specific method. */
  return NvmVerifyChecksumHook();
#else
  /* check checksum using the interally supported method. */
  return FlashVerifyChecksum();
#endif
} /*** end of NvmVerifyChecksum ***/


/************************************************************************************//**
** \brief     Obtains the base address of the non-volatile memory available to the user
**            program. This is typically that start of the vector table.
** \return    Base address.
**
****************************************************************************************/
blt_addr NvmGetUserProgBaseAddress(void)
{
  return FlashGetUserProgBaseAddress();
} /*** end of NvmGetUserProgBaseAddress ***/


/************************************************************************************//**
** \brief     Once all erase and programming operations are completed, this
**            function is called, so at the end of the programming session and
**            right before a software reset is performed. It is used to calculate
**            a checksum and program this into flash. This checksum is later used
**            to determine if a valid user program is present in flash.
** \return    BLT_TRUE if successful, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool NvmDone(void)
{
#if (BOOT_NVM_HOOKS_ENABLE > 0)
  /* give the application's NVM driver a chance to finish up */
  if (NvmDoneHook() == BLT_FALSE)
  {
    /* error so no need to continue */
    return BLT_FALSE;
  }
#endif

#if (BOOT_NVM_CHECKSUM_HOOKS_ENABLE > 0)
  /* compute and write checksum, using the application specific method. */
  if (NvmWriteChecksumHook() == BLT_FALSE)
  {
    return BLT_FALSE;
  }
#else
  /* compute and write checksum, which is programmed by the internal driver. */
  if (FlashWriteChecksum() == BLT_FALSE)
  {
    return BLT_FALSE;
  }
#endif

  /* finish up internal driver operations */
  return FlashDone();
} /*** end of NvmDone ***/


/*********************************** end of nvm.c **************************************/
