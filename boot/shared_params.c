/************************************************************************************//**
* \file         Demo/ARMCM33_STM32H5_Nucleo_H563ZI_GCC/Boot/shared_params.c
* \brief        Shared RAM parameters source file.
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
#include <stddef.h>                                    /* Standard definitions (NULL). */
#include "shared_params.h"                             /* Shared parameters header.    */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Constant parameter buffer identifier. This value is always located as the
 *         start of the buffer to validate the the RAM contains valid shared parameters.
 */
#define SHARED_PARAMS_BUFFER_ID                 (0xCE42E7A2u)


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Layout of the shared parameters RAM buffer. */
typedef struct t_shared_params_buffer
{
  /** \brief Fixed buffer identifier to validate that the RAM contains valid shared
   *         parameters.
   */
  uint32_t identifier;
  /**  \brief Array for the actual parameter data. */
  uint8_t data[SHARED_PARAMS_CFG_BUFFER_DATA_LEN];
  /** \brief Checksum value of all the bytes in the buffer, excluding this checksum
   *         value of obvious reasons. The checksum is calculated as the Two's
   *         complement of the sum of the bytes.
   */
  uint16_t checksum;
} tSharedParamsBuffer;


/****************************************************************************************
* Global data declarations
****************************************************************************************/
/** \brief   Declaration of the actual parameter buffer that this module manages.
 *  \warning For the shared RAM parameters to work properly for sharing information
 *           between the bootloader and user program, it is important that this
 *           variable is linked to the exact same RAM address in both the bootloader
 *           and the user program. Additionally, it should be configured such that the
 *           C-startup code does NOT zero its contents during system initialization. This
 *           is the code that runs in the reset event handler, before function main() is
 *           called.
 *           For GCC based embedded toolchains, the solution is to assign this variable
 *           to a custom section, in this case called ".shared". Then in the linker
 *           script, add the following to the SECTIONS:
 *
 *             .shared (NOLOAD) :
 *             {
 *               . = ALIGN(4);
 *               _sshared = .;
 *               __shared_start__ = _sshared;
 *               *(.shared)
 *               *(.shared.*)
 *               KEEP(*(.shared))
 *               . = ALIGN(4);
 *               _eshared = .;
 *               __shared_end__ = _eshared;
 *             } >SHARED
 *
 *           Next, add a new MEMORY entry for SHARED at the start of RAM and reduce
 *           the length of the remaining RAM:
 *
 *             SHARED (xrw)   : ORIGIN = 0x200000C0, LENGTH = 64
 *             RAM (xrw)      : ORIGIN = 0x20000100, LENGTH = 32K - 192 - 64
 *
 *           Note that the previous example is for an STM32F0 microcontroller where
 *           the first 192 (0xC0) bytes in RAM are reserved for the user program
 *           vector table.
 *
 *  \remark  This same approach can be applied with other toolchains such as Keil MDK
 *           and IAR EWARM. Consult the compiler and linker user manuals of your
 *           toolchain to find out how to place a RAM variable at a fixed memory address
 *           and to prevent the C-startup code from zeroing its contents.
 *           Here are a few links to get you started:
 *           * IAR EWARM:
 *               https://www.iar.com/support/tech-notes/compiler/
 *               linker-error-for-absolute-located-variable/
 *           * Keil MDK:
 *               http://www.keil.com/support/docs/3480.htm
 */
static tSharedParamsBuffer sharedParamsBuffer __attribute__ ((section (".shared")));


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static bool     SharedParamsValidateBuffer(void);
static void     SharedParamsWriteChecksum(void);
static bool     SharedParamsVerifyChecksum(void);
static uint16_t SharedParamsCalculateChecksum(void);


/************************************************************************************//**
** \brief     Initializes the shared RAM parameters module.
** \return    none.
**
****************************************************************************************/
void SharedParamsInit(void)
{
  uint32_t byteIdx;

  /* The shared parameter buffer does not get initialized by the C-startup code. Another
   * previously running program could have initialized it, in which case it is ready
   * for use and nothing more needs to be done.
   */
  if (!SharedParamsValidateBuffer())
  {
    /* The shared parameter buffer was not yet initialized by a running program. This
     * typically happens after a cold reset where the RAM contents were lost. In this
     * case we need to explicitly configure and initialize it, since the C-startup code
     * was configured to not do this.
     *
     * The initialization consists of setting the buffer identifier, zeroing the
     * actual parameter data and updating the checksum at the end.
     */
    sharedParamsBuffer.identifier = SHARED_PARAMS_BUFFER_ID;
    for (byteIdx=0; byteIdx < SHARED_PARAMS_CFG_BUFFER_DATA_LEN; byteIdx++)
    {
      sharedParamsBuffer.data[byteIdx] = 0;
    }
    SharedParamsWriteChecksum();
  }
} /*** end of SharedParamsInit ***/


/************************************************************************************//**
** \brief     Reads a data byte from the shared parameter buffer at the specified index.
** \param     idx Index into the parameter data array. A valid value is between 0 and
**            (SHARED_PARAMS_CFG_BUFFER_DATA_LEN - 1).
** \param     value Pointer to where the read data value is stored.
** \return    True if successful, false otherwise.
**
****************************************************************************************/
bool SharedParamsReadByIndex(uint32_t idx, uint8_t * value)
{
  bool result = false;

  /* Only continue if the buffer and the specified parameters are valid. */
  if ( (SharedParamsValidateBuffer()) &&
       (idx < SHARED_PARAMS_CFG_BUFFER_DATA_LEN)  &&
       (value != NULL) )
  {
    /* Read the value and update the result. */
    *value = sharedParamsBuffer.data[idx];
    result = true;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of SharedParamsReadByIndex ***/


/************************************************************************************//**
** \brief     Writes a data byte to the shared parameter buffer at the specified index.
** \param     idx Index into the parameter data array. A valid value is between 0 and
**            (SHARED_PARAMS_CFG_BUFFER_DATA_LEN - 1).
** \param     value Value to write.
** \return    True if successful, false otherwise.
**
****************************************************************************************/
bool SharedParamsWriteByIndex(uint32_t idx, uint8_t value)
{
  bool result = false;

  /* Only continue if the buffer and the specified parameters are valid. */
  if ( (SharedParamsValidateBuffer()) &&
       (idx < SHARED_PARAMS_CFG_BUFFER_DATA_LEN) )
  {
    /* Write the value. */
    sharedParamsBuffer.data[idx] = value;
    /* Update the checksum since the contents were just changed. */
    SharedParamsWriteChecksum();
    /* Update the result. */
    result = true;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of SharedParamsWriteByIndex ***/


/************************************************************************************//**
** \brief     Validates the shared parameter buffer contents by looking at the table
**            identifier and verifying its checksum.
** \return    True if successful, false otherwise.
**
****************************************************************************************/
static bool SharedParamsValidateBuffer(void)
{
  bool result = false;

  /* Perform validation. */
  if ( (sharedParamsBuffer.identifier == SHARED_PARAMS_BUFFER_ID) &&
       (SharedParamsVerifyChecksum()) )
  {
    /* The shared parameter buffer is valid, so update the result value. */
    result = true;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of SharedParamsValitabeTable ***/


/************************************************************************************//**
** \brief     Calculates and writes the checksum into the buffer.
** \return    none.
**
****************************************************************************************/
static void SharedParamsWriteChecksum(void)
{
  /* Calculate and write the checksum. */
  sharedParamsBuffer.checksum = SharedParamsCalculateChecksum();
} /*** end of SharedParamsWriteChecksum ***/


/************************************************************************************//**
** \brief     Calculates and verifies the checksum that is currently present in the
**            buffer.
** \return    True is the checksum is correct, false otherwise.
**
****************************************************************************************/
static bool SharedParamsVerifyChecksum(void)
{
  bool result = false;

  /* Calculate and verify the checksum. */
  if (SharedParamsCalculateChecksum() == sharedParamsBuffer.checksum)
  {
    /* Checksum is correct, so update the result value. */
    result = true;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of SharedParamsVerifyChecksum ***/


/************************************************************************************//**
** \brief     Calculates and returns the checksum value for the current contents in the
**            buffer. The checksum is calculated by taking the sum of all bytes in the
**            parameter buffer (excluding the checksum at the end) and them taking the
**            two's complement value of it.
** \return    The calculated checksum value.
**
****************************************************************************************/
static uint16_t SharedParamsCalculateChecksum(void)
{
  uint16_t result = 0;
  uint32_t byteIdx;

  /* Add the identifier bytes to the checksum. */
  result += (uint8_t)sharedParamsBuffer.identifier;
  result += (uint8_t)(sharedParamsBuffer.identifier >> 8u);
  result += (uint8_t)(sharedParamsBuffer.identifier >> 16u);
  result += (uint8_t)(sharedParamsBuffer.identifier >> 24u);
  /* Loop through the parameter data array. */
  for (byteIdx=0; byteIdx<SHARED_PARAMS_CFG_BUFFER_DATA_LEN; byteIdx++)
  {
    /* Add parameter data byte to the checksum. */
    result += (uint8_t)sharedParamsBuffer.data[byteIdx];
  }
  /* Determine one's complement. */
  result = ~result;
  /* Determine two's complement. */
  result += 1;
  /* Give the result back to the caller. */
  return result;
} /*** end of SharedParamsCalculateChecksum ***/


/*********************************** end of shared_params.c ****************************/
