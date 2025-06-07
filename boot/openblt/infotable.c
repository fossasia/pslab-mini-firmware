/************************************************************************************//**
* \file         Source/infotable.v
* \brief        Info table check feature source file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2025  by Feaser    http://www.feaser.com    All rights reserved
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


#if (BOOT_INFO_TABLE_ENABLE > 0)
/****************************************************************************************
* Hook functions
****************************************************************************************/
extern blt_bool InfoTableCheckHook(blt_addr newInfoTable, blt_addr currentInfoTable);


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief   RAM buffer for storing the info table that is extracted from the firmware
 *           file at the start of the firmware update.
 *  \details For remote firmware updates, the extracted info table is downloaded to this
 *           RAM buffer. For firmware updates from locally attached storage, the
 *           extracted info table is directly copied to this RAM buffer.
 */
static blt_int8u   infoTableBuffer[BOOT_INFO_TABLE_LEN];

/** \brief Current write pointer for storing data into the RAM info buffer. */
static blt_int8u * infoTableWritePtr;


/***********************************************************************************//**
** \brief     Initializes the info table module.
** \return    none
**
****************************************************************************************/
void InfoTableInit(void)
{
  /* Initialize module locals. */
  infoTableWritePtr = &infoTableBuffer[0];
  /* Clear the info table in the internal RAM buffer. */
  InfoTableClear(INFO_TABLE_ID_INTERNAL_RAM);
} /*** end of InfoTableInit ***/


/***********************************************************************************//**
** \brief     Invokes the hook function to compare the info table contents of the to-be-
**            programmed firmware (in the internal RAM info table) with the info table
**            contents of the currently programmed firmware (in the NVM info table).
**            It allows the bootloader application to implement comparison logic to
**            determine if the firmware update is allowed to proceed. Could for example
**            be used to make sure a firmware update only goes through if the selected
**            firmware file contains  firmware for the correct product type.
** \return    BLT_TRUE if the info table check passed and the firmware update is allowed
**            to proceed. BLT_FALSE if the firmware update is not allowed to proceed.
**
****************************************************************************************/
blt_bool InfoTableCheck(void)
{
  blt_bool result;
  blt_addr infoTableNewAddr;
  blt_addr infoTableCurrentAddr;

  /* Collect info table pointers. */
  infoTableNewAddr     = InfoTableGetPtr(INFO_TABLE_ID_INTERNAL_RAM);
  infoTableCurrentAddr = InfoTableGetPtr(INFO_TABLE_ID_FIRMWARE_NVM);

  /* An info table check can only be performed if a valid one is present. Check the
   * currently programmed firmware's checksum for this reason.
   */
  if (NvmVerifyChecksum() == BLT_FALSE)
  {
    /* No valid firmware present. This usually means that no firmware is yet programmed
     * or the previous firmware update did not run all the way to completion. In either
     * case, the current firmware update is allowed to proceed, without further info
     * table checking.
     */
    result = BLT_TRUE;
  }
  /* Checksum verification of current firmware okay, which means that an info table
   * should be present.
   */
  else
  {
    /* Request bootloader application to perform the info table check. */
    result = InfoTableCheckHook(infoTableNewAddr, infoTableCurrentAddr);
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of InfoTableCheck ***/


/***********************************************************************************//**
** \brief     Obtains the memory pointer of the selected info table.
** \param     infoTableId Specifies the info table to get the pointer to.
** \return    Address of the selected info table.
**
****************************************************************************************/
blt_addr InfoTableGetPtr(tInfoTableId infoTableId)
{
  blt_addr result;

  /* Info table of this module's RAM buffer? This is where the info table of the to-be-
   * programmed firmware will be written to.
   */
  if (infoTableId == INFO_TABLE_ID_INTERNAL_RAM)
  {
    result = (blt_addr)(&infoTableBuffer[0]);
  }
  /* Info table of the currently programmed firmware. Note that it is possible that
   * no firmware is programmed at all.
   */
  else
  {
    result = (blt_addr)BOOT_INFO_TABLE_ADDR;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of InfoTableGetPtr ***/


/***********************************************************************************//**
** \brief     Utility function to clear the contents of the internal RAM buffer and to
**            re-initialize its write pointer to the start.
** \param     infoTableId Specifies the info table to get the pointer to. Must be
**            INFO_TABLE_ID_INTERNAL_RAM, because the INFO_TABLE_ID_FIRMWARE_NVM cannot
**            be changed.
** \return    none.
**
****************************************************************************************/
void InfoTableClear(tInfoTableId infoTableId)
{
  blt_int16u idx;

  /* Verify parameter. */
  ASSERT_RT(infoTableId == INFO_TABLE_ID_INTERNAL_RAM);

  /* Only continue with a valid parameter. */
  if (infoTableId == INFO_TABLE_ID_INTERNAL_RAM)
  {
    /* Clear the contents of the info table RAM buffer. */
    for (idx = 0U; idx < BOOT_INFO_TABLE_LEN; idx++)
    {
      infoTableBuffer[idx] = 0U;
    }
    /* Reset the write pointer. */
    infoTableWritePtr = &infoTableBuffer[0];
  }
} /*** end of InfoTableClear ***/


/***********************************************************************************//**
** \brief     Utility function to determine the current number of bytes stored in the
**            specified buffer. For INFO_TABLE_ID_FIRMWARE_NVM this is always the
**            same. For INFO_TABLE_ID_INTERNAL_RAM is returns the number of bytes that
**            were currenty written to it.
** \param     infoTableId Specifies the info table to get the pointer to. Must be
**            INFO_TABLE_ID_INTERNAL_RAM, because the INFO_TABLE_ID_FIRMWARE_NVM cannot
**            be changed.
** \return    The number of bytes currently present in the info table.
**
****************************************************************************************/
blt_int16u InfoTableCurrentSize(tInfoTableId infoTableId)
{
  blt_int16u result = BOOT_INFO_TABLE_LEN + 1; /* Invalid length (too full). */

  if (infoTableId == INFO_TABLE_ID_INTERNAL_RAM)
  {
    /* Calculate stored number of bytes by making use of the write pointer. First do a
     * sanity check.
     */
    if (infoTableWritePtr >= &infoTableBuffer[0])
    {
      result = (blt_int16u)((blt_addr)infoTableWritePtr-(blt_addr)(&infoTableBuffer[0]));
    }
  }
  else
  {
    /* Info table in NVM has fixed size as it cannot be changed. */
    result = BOOT_INFO_TABLE_LEN;
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of InfoTableCurrentSize ***/


/***********************************************************************************//**
** \brief     Utility function to add data to the internal RAM info table, while also
**            auto incrementing the write pointer.
** \param     infoTableId Specifies the info table to get the pointer to. Must be
**            INFO_TABLE_ID_INTERNAL_RAM, because the INFO_TABLE_ID_FIRMWARE_NVM cannot
**            be changed.
** \param     data Pointer to the byte array with data to add.
** \param     len Number of bytes to add.
** \return    BLT_TRUE if the data could be added to the info table, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool InfoTableAddData(tInfoTableId infoTableId, blt_int8u const * data, 
                          blt_int16u len)
{
  blt_bool result = BLT_FALSE;

  /* Can only add data to the internal RAM info table and with a valid data pointer. */
  if ( (infoTableId == INFO_TABLE_ID_INTERNAL_RAM) && (data != BLT_NULL) && (len > 0) )
  {
    /* Only continue if this data still fits in the info table. */
    if ((InfoTableCurrentSize(INFO_TABLE_ID_INTERNAL_RAM) + len) <= BOOT_INFO_TABLE_LEN)
    {
      /* Store the data in the internal RAM info table. */
      CpuMemCopy((blt_addr)infoTableWritePtr, (blt_addr)((blt_int32u)&data[0]), len);
      /* Update the write pointer accordingly. */
      infoTableWritePtr += len;
      /* Update the result. */
      result = BLT_TRUE;
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of InfoTableAddData ***/
#endif /* BOOT_INFO_TABLE_ENABLE > 0 */


/*********************************** end of infotable.c ********************************/
