/************************************************************************************//**
* \file         Source/file.h
* \brief        Bootloader file system interface header file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2013  by Feaser    http://www.feaser.com    All rights reserved
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
#ifndef FILE_H
#define FILE_H

#if (BOOT_FILE_SYS_ENABLE > 0)
/****************************************************************************************
* Include files
****************************************************************************************/
#include "ff.h"                                  /* FATFS file system library          */


/****************************************************************************************
* Defines
****************************************************************************************/
/** \brief Error code for not being able to open the firmware file. */
#define FILE_ERROR_CANNOT_OPEN_FIRMWARE_FILE       (1)
/** \brief Error code for not being able to read from the firmware file. */
#define FILE_ERROR_CANNOT_READ_FROM_FILE           (2)
/** \brief Error code because in incorrect checksum was found in the firmware file. */
#define FILE_ERROR_INVALID_CHECKSUM_IN_FILE        (3)
/** \brief Error code because the file pointers read pointer could not be rewinded. */
#define FILE_ERROR_REWINDING_FILE_READ_POINTER     (4)
/** \brief Error code because an error occurred during the memory erase operation. */
#define FILE_ERROR_CANNOT_ERASE_MEMORY             (5)
/** \brief Error code because an error occurred during the memory write operation. */
#define FILE_ERROR_CANNOT_PROGRAM_MEMORY           (6)
/** \brief Error code because the program's checksum could not be written to memory. */
#define FILE_ERROR_CANNOT_WRITE_CHECKSUM           (7)
#if (BOOT_INFO_TABLE_ENABLE > 0)
/** \brief Error code because no info table or only a partial one was detected. */
#define FILE_ERROR_INFO_TABLE_MISSING              (8)
/** \brief Error code because the info table check did not pass. */
#define FILE_ERROR_INFO_TABLE_CHECK_NOT_PASSED     (9)
#endif /* (BOOT_INFO_TABLE_ENABLE > 0) */
/** \brief Maximum number of characters that can be on a line in the firmware file. */
#define MAX_CHARS_PER_LINE                  (256)
/** \brief Maximum number of data bytes that can be on a line in the firmware file
 *         (S-record).
 */
#define MAX_DATA_BYTES_PER_LINE             (MAX_CHARS_PER_LINE/2)
/** \brief Return code in case an invalid checksum was detected on an S-record line. */
#define ERROR_SREC_INVALID_CHECKSUM         (-1)


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Enumeration for the different S-record line types. */
typedef enum
{
  LINE_TYPE_S1,                                  /**< 16-bit address line              */
  LINE_TYPE_S2,                                  /**< 24-bit address line              */
  LINE_TYPE_S3,                                  /**< 32-bit address line              */
  LINE_TYPE_UNSUPPORTED                          /**< unsupported line                 */
} tSrecLineType;

/** \brief Structure type for grouping the parsing results of an S-record line. */
typedef struct
{
  blt_char  line[MAX_CHARS_PER_LINE];            /**< string buffer for the line chars */
  blt_int8u data[MAX_DATA_BYTES_PER_LINE];       /**< array for S1, S2 or S3 data bytes*/
  blt_addr  address;                             /**< address on S1, S2 or S3 line     */
} tSrecLineParseObject;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void          FileInit(void);
void          FileTask(void);
blt_bool      FileIsIdle(void);
blt_bool      FileHandleFirmwareUpdateRequest(void);
/* functions for reading data from a Motorola S-record file. */
tSrecLineType FileSrecGetLineType(const blt_char *line);
blt_bool      FileSrecVerifyChecksum(const blt_char *line);
blt_int16s    FileSrecParseLine(const blt_char *line, blt_addr *address, blt_int8u *data);

#endif /* BOOT_FILE_SYS_ENABLE > 0 */


#endif /* FILE_H */
/*********************************** end of file.h *************************************/
