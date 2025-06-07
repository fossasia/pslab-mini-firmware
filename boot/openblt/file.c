/************************************************************************************//**
* \file         Source/file.c
* \brief        Bootloader file system interface source file.
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

/****************************************************************************************
* Include files
****************************************************************************************/
#include "boot.h"                                     /* bootloader generic header     */
#include <string.h>                                   /* for strcpy etc.               */
#include <ctype.h>                                    /* for toupper() etc.            */


#if (BOOT_FILE_SYS_ENABLE > 0)
/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Enumeration for the different internal module states. */
typedef enum
{
  FIRMWARE_UPDATE_STATE_IDLE,                    /**< idle state                       */
  FIRMWARE_UPDATE_STATE_STARTING,                /**< starting state                   */
  FIRMWARE_UPDATE_STATE_ERASING,                 /**< erasing state                    */
  FIRMWARE_UPDATE_STATE_PROGRAMMING,             /**< programming state                */
  FIRMWARE_UPDATE_STATE_EXTRACTING               /**< extracting state                 */
} tFirmwareUpdateState;

/** \brief Structure type with information for the memory erase opeartion. */
typedef struct
{
  blt_addr   start_address;                      /**< erase start address              */
  blt_int32u total_size;                         /**< total number of bytes to erase   */
} tFileEraseInfo;

/** \brief Structure type for grouping FATFS related objects used by this module. */
typedef struct
{
  FATFS fs;                                      /**< file system object for mouting   */
  FIL   file;                                    /**< file object for firmware file    */
} tFatFsObjects;

/****************************************************************************************
* Function prototypes
****************************************************************************************/
#if (BOOT_FILE_LOGGING_ENABLE > 0)
static blt_char      FileLibByteNibbleToChar(blt_int8u nibble);
static blt_char     *FileLibByteToHexString(blt_int8u byte_val, blt_char *destination);
static blt_char     *FileLibLongToIntString(blt_int32u long_val, blt_char *destination);
#endif
static blt_int8u     FileLibHexStringToByte(const blt_char *hexstring);


/****************************************************************************************
* Hook functions
****************************************************************************************/
extern blt_bool        FileIsFirmwareUpdateRequestedHook(void);
extern const blt_char *FileGetFirmwareFilenameHook(void);
extern void            FileFirmwareUpdateStartedHook(void);
extern void            FileFirmwareUpdateCompletedHook(void);
extern void            FileFirmwareUpdateErrorHook(blt_int8u error_code);
extern void            FileFirmwareUpdateLogHook(blt_char *info_string);


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Local variable that holds the internal module state. */
static tFirmwareUpdateState firmwareUpdateState;
/** \brief Local variable for the used FATFS objects in this module. */
static tFatFsObjects        fatFsObjects;
/** \brief Local variable for storing S-record line parsing results. */
static tSrecLineParseObject lineParseObject;
/** \brief Local variable for storing information regarding the memory erase operation.*/
static tFileEraseInfo       eraseInfo;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
/** \brief Local character buffer for storing the string with log information. */
static blt_char             loggingStr[64];
#endif


/***********************************************************************************//**
** \brief     Initializes the file system interface module. The initial firmware
**            update state is set to idle and the file system is mounted as
**            logical disk 0.
** \return    none
**
****************************************************************************************/
void FileInit(void)
{
  FRESULT fresult;

  /* set the initial state */
  firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
  /* mount the file system, using logical disk 0 */
  fresult = f_mount(&fatFsObjects.fs, "0:", 0);
  /* mounting does not access the disk and should succeed unless misconfigured */
  ASSERT_RT(fresult == FR_OK);
} /*** end of FileInit ***/


/***********************************************************************************//**
** \brief     This function checks if a firmware update through the locally attached
**            storage is in progress or not (idle).
** \return    BLT_TRUE when in idle state, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool FileIsIdle(void)
{
  if (firmwareUpdateState == FIRMWARE_UPDATE_STATE_IDLE)
  {
    return BLT_TRUE;
  }
  return BLT_FALSE;
} /*** end of FileIsIdle ***/


/***********************************************************************************//**
** \brief     This function checks if a firmware update through the locally attached
**            storage is requested to be started and if so processes this request
**            by transitioning from the IDLE to the STARTING state.
** \return    BLT_TRUE when a firmware update is requested, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool FileHandleFirmwareUpdateRequest(void)
{
#if (BOOT_COM_ENABLE > 0)
  /* make sure that there is no connection with a remote host to prevent two firmware
   * updates happening at the same time
   */
  if (ComIsConnected() == BLT_TRUE)
  {
    return BLT_FALSE;
  }
#endif
  /* a new firmware update request can only be handled if not already busy with one */
  if (firmwareUpdateState != FIRMWARE_UPDATE_STATE_IDLE)
  {
    return BLT_FALSE;
  }
  /* check if a firmware update is requested */
  if (FileIsFirmwareUpdateRequestedHook() == BLT_TRUE)
  {
    /* transition from IDLE to STARTING state, which kicks off the update sequence */
    firmwareUpdateState = FIRMWARE_UPDATE_STATE_STARTING;
    return BLT_TRUE;
  }
  /* still here so no update request pending */
  return BLT_FALSE;
} /*** end of FileHandleFirmwareUpdateRequest ***/


/***********************************************************************************//**
** \brief     File system task function for managing the firmware updates from
**                 locally attached storage.
** \return    none.
**
****************************************************************************************/
void FileTask(void)
{
  blt_int16s  parse_result = 0;
  blt_char   *read_line_ptr;
#if (BOOT_INFO_TABLE_ENABLE > 0)
  static blt_int8u  infoTableBuffer[BOOT_INFO_TABLE_LEN];
  static blt_int16u infoTableByteCnt;
  static blt_bool   infoTableExtracted;
  blt_int16u        infoTableByteIdx;
  blt_int16u        byteIdx;
  blt_addr          byteAddr;
#endif

  /* ------------------------------- idle -------------------------------------------- */
  if (firmwareUpdateState == FIRMWARE_UPDATE_STATE_IDLE)
  {
    /* currently, nothings need to be done while idling */
  }
  /* ------------------------------- starting ---------------------------------------- */
  else if (firmwareUpdateState == FIRMWARE_UPDATE_STATE_STARTING)
  {
    /* reinit the NVM driver because a new firmware update is about the start */
    NvmInit();
#if (BOOT_FILE_STARTED_HOOK_ENABLE > 0)
    /* inform application about update started event via hook function */
    FileFirmwareUpdateStartedHook();
#endif
#if (BOOT_FILE_LOGGING_ENABLE > 0)
    FileFirmwareUpdateLogHook("Firmware update request detected\n\r");
    FileFirmwareUpdateLogHook("Opening firmware file for reading...");
#endif
    /* attempt to obtain a file object for the firmware file */
    if (f_open(&fatFsObjects.file, FileGetFirmwareFilenameHook(), FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
      /* cannot continue with firmware update so go back to idle state */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
      /* can't open file */
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
      FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_OPEN_FIRMWARE_FILE);
#endif
      /* nothing left to do now */
      return;
    }
#if (BOOT_FILE_LOGGING_ENABLE > 0)
    FileFirmwareUpdateLogHook("OK\n\r");
    FileFirmwareUpdateLogHook("Starting the programming sequence\n\r");
#endif
    /* prepare data objects for the erasing state */
    eraseInfo.start_address = 0;
    eraseInfo.total_size = 0;
#if (BOOT_INFO_TABLE_ENABLE > 0)
    /* prepare data object for the extraction state */
    infoTableByteCnt = 0;
    infoTableExtracted = BLT_FALSE;
    /* clear the info table in the internal RAM buffer and reset its write pointer. */
    InfoTableClear(INFO_TABLE_ID_INTERNAL_RAM);
    /* transition from idle to extracting state */
    firmwareUpdateState = FIRMWARE_UPDATE_STATE_EXTRACTING;
    #if (BOOT_FILE_LOGGING_ENABLE > 0)
    FileFirmwareUpdateLogHook("Parsing firmware file to extract info table...");
    #endif
#else
    /* transition from idle to erasing state */
    firmwareUpdateState = FIRMWARE_UPDATE_STATE_ERASING;
    #if (BOOT_FILE_LOGGING_ENABLE > 0)
    FileFirmwareUpdateLogHook("Parsing firmware file to detect erase blocks...");
    #endif
#endif
  }
  #if (BOOT_INFO_TABLE_ENABLE > 0)
  /* ------------------------------- extracting -------------------------------------- */
  else if (firmwareUpdateState == FIRMWARE_UPDATE_STATE_EXTRACTING)
  {
    /* read a line from the file */
    read_line_ptr = f_gets(lineParseObject.line, sizeof(lineParseObject.line), &fatFsObjects.file);
    /* check if an error occurred */
    if (f_error(&fatFsObjects.file) > 0)
    {
      /* cannot continue with firmware update so go back to idle state */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
      FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_READ_FROM_FILE);
#endif
      /* close the file */
      f_close(&fatFsObjects.file);
      return;
    }
    /* parse the S-Record line if the line is not empty */
    if (read_line_ptr != BLT_NULL)
    {
      parse_result = FileSrecParseLine(lineParseObject.line, &lineParseObject.address,
                                       lineParseObject.data);
      /* check parsing result */
      if (parse_result == ERROR_SREC_INVALID_CHECKSUM)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_INVALID_CHECKSUM_IN_FILE);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
    }
    /* only process parsing results if the line contained address/data info */
    if (parse_result > 0)
    {
      /* does the parsed data include info table data? Note that parse_result holds
       * the number of data bytes encountered on the parsed line.
       */
      if (! (((BOOT_INFO_TABLE_ADDR + BOOT_INFO_TABLE_LEN) <= lineParseObject.address) ||
             ((lineParseObject.address + parse_result) <= BOOT_INFO_TABLE_ADDR)) )
      {
        /* loop through all parsed data bytes. */
        for (byteIdx = 0; byteIdx < parse_result; byteIdx++)
        {
          /* does this byte belong to the info table? */
          byteAddr = lineParseObject.address + byteIdx;
          if ( (byteAddr >= BOOT_INFO_TABLE_ADDR) &&
               (byteAddr < (BOOT_INFO_TABLE_ADDR + BOOT_INFO_TABLE_LEN)) )
          {
            /* calculate index into the info table buffer. */
            infoTableByteIdx = byteAddr - BOOT_INFO_TABLE_ADDR;
            /* store the byte in the info table buffer. */
            infoTableBuffer[infoTableByteIdx] = lineParseObject.data[byteIdx];
            infoTableByteCnt++;
            /* done extracting the info table? */
            if (infoTableByteCnt >= BOOT_INFO_TABLE_LEN)
            {
              /* hand the info table over to the info table module's RAM buffer. No need
               * to check the return value because we know the parameters are valid and
               * that the internal RAM buffer is writable.
               */
              (void)InfoTableAddData(INFO_TABLE_ID_INTERNAL_RAM, &infoTableBuffer[0],
                                     BOOT_INFO_TABLE_LEN);
              /* update flag to indicate that info table extraction is complete. */
              infoTableExtracted = BLT_TRUE;
              /* no need to continue the loop. */
              break;
            }
          }
        }
      }
    }
    /* check if the end of the file was reached or the info table extraction completed.*/
    if ( (f_eof(&fatFsObjects.file) > 0) || (infoTableExtracted == BLT_TRUE) )
    {
      /* rewind the file in preparation for the programming state */
      if (f_lseek(&fatFsObjects.file, 0) != FR_OK)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_REWINDING_FILE_READ_POINTER);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
      /* make sure the info table was extracted successfully. */
      if (infoTableExtracted == BLT_FALSE)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_INFO_TABLE_MISSING);
#endif
        return;
      }
      /* still here so the info table was successfully extracted. continue with checking
       * the info table contents.
       */
      #if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("OK\n\r");
      FileFirmwareUpdateLogHook("Performing info table check...");
      #endif
      /* Request bootloader application to perform the info table check. */
      if (InfoTableCheck() == BLT_FALSE)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_INFO_TABLE_CHECK_NOT_PASSED);
#endif
        return;
      }
      /* info table check passed. okay to continue with the firmware update. */
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("OK\n\r");
      FileFirmwareUpdateLogHook("Parsing firmware file to detect erase blocks...");
#endif
      /* transition from extracting to erasing state */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_ERASING;
    }
  }
  #endif /* BOOT_INFO_TABLE_ENABLE > 0 */
  /* ------------------------------- erasing ----------------------------------------- */
  else if (firmwareUpdateState == FIRMWARE_UPDATE_STATE_ERASING)
  {
    /* read a line from the file */
    read_line_ptr = f_gets(lineParseObject.line, sizeof(lineParseObject.line), &fatFsObjects.file);
    /* check if an error occurred */
    if (f_error(&fatFsObjects.file) > 0)
    {
      /* cannot continue with firmware update so go back to idle state */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
      FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_READ_FROM_FILE);
#endif
      /* close the file */
      f_close(&fatFsObjects.file);
      return;
    }
    /* parse the S-Record line without copying the data values if the line is not empty */
    if (read_line_ptr != BLT_NULL)
    {
      parse_result = FileSrecParseLine(lineParseObject.line, &lineParseObject.address, BLT_NULL);
      /* check parsing result */
      if (parse_result == ERROR_SREC_INVALID_CHECKSUM)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_INVALID_CHECKSUM_IN_FILE);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
    }
    /* only process parsing results if the line contained address/data info */
    if (parse_result > 0)
    {
      /* is this the first address/data info we encountered? */
      if (eraseInfo.total_size == 0)
      {
        /* store the start_address and byte count */
        eraseInfo.start_address = lineParseObject.address;
        eraseInfo.total_size = parse_result;
      }
      else
      {
        /* does this data fit at the end of the previously detected program block? */
        if (lineParseObject.address == (eraseInfo.start_address + eraseInfo.total_size))
        {
          /* update the byte count */
          eraseInfo.total_size += parse_result;
        }
        else
        {
          /* data does not belong to the previously detected block so there must be a
           * gap in the data. first erase the currently detected block and then start
           * tracking a new block.
           */
          #if (BOOT_FILE_LOGGING_ENABLE > 0)
          FileFirmwareUpdateLogHook("OK\n\r");
          FileFirmwareUpdateLogHook("Erasing ");
          /* convert size to string  */
          FileLibLongToIntString(eraseInfo.total_size, loggingStr);
          FileFirmwareUpdateLogHook(loggingStr);
          FileFirmwareUpdateLogHook(" bytes from memory at 0x");
          /* convert address to hex-string  */
          FileLibByteToHexString((blt_int8u)(eraseInfo.start_address >> 24), &loggingStr[0]);
          FileLibByteToHexString((blt_int8u)(eraseInfo.start_address >> 16), &loggingStr[2]);
          FileLibByteToHexString((blt_int8u)(eraseInfo.start_address >> 8), &loggingStr[4]);
          FileLibByteToHexString((blt_int8u)eraseInfo.start_address, &loggingStr[6]);
          FileFirmwareUpdateLogHook(loggingStr);
          FileFirmwareUpdateLogHook("...");
          #endif
          /* still here so we are ready to perform the memory erase operation */
          if (NvmErase(eraseInfo.start_address, eraseInfo.total_size) == BLT_FALSE)
          {
            /* cannot continue with firmware update so go back to idle state */
            firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
            #if (BOOT_FILE_LOGGING_ENABLE > 0)
            FileFirmwareUpdateLogHook("ERROR\n\r");
            #endif
            #if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
            FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_ERASE_MEMORY);
            #endif
            /* close the file */
            f_close(&fatFsObjects.file);
            return;
          }
          #if (BOOT_FILE_LOGGING_ENABLE > 0)
          FileFirmwareUpdateLogHook("OK\n\r");
          FileFirmwareUpdateLogHook("Parsing firmware file to detect erase blocks...");
          #endif

          /* store the start_address and element count */
          eraseInfo.start_address = lineParseObject.address;
          eraseInfo.total_size = parse_result;
        }
      }
    }
    /* check if the end of the file was reached */
    if (f_eof(&fatFsObjects.file) > 0)
    {
      /* rewind the file in preparation for the programming state */
      if (f_lseek(&fatFsObjects.file, 0) != FR_OK)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_REWINDING_FILE_READ_POINTER);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
      /* still here so we are ready to perform the last memory erase operation, if there
       * is still something left to erase.
       */
      if (eraseInfo.total_size > 0)
      {
        #if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("OK\n\r");
        FileFirmwareUpdateLogHook("Erasing ");
        /* convert size to string  */
        FileLibLongToIntString(eraseInfo.total_size, loggingStr);
        FileFirmwareUpdateLogHook(loggingStr);
        FileFirmwareUpdateLogHook(" bytes from memory at 0x");
        /* convert address to hex-string  */
        FileLibByteToHexString((blt_int8u)(eraseInfo.start_address >> 24), &loggingStr[0]);
        FileLibByteToHexString((blt_int8u)(eraseInfo.start_address >> 16), &loggingStr[2]);
        FileLibByteToHexString((blt_int8u)(eraseInfo.start_address >> 8), &loggingStr[4]);
        FileLibByteToHexString((blt_int8u)eraseInfo.start_address, &loggingStr[6]);
        FileFirmwareUpdateLogHook(loggingStr);
        FileFirmwareUpdateLogHook("...");
        #endif
        if (NvmErase(eraseInfo.start_address, eraseInfo.total_size) == BLT_FALSE)
        {
          /* cannot continue with firmware update so go back to idle state */
          firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
          #if (BOOT_FILE_LOGGING_ENABLE > 0)
          FileFirmwareUpdateLogHook("ERROR\n\r");
          #endif
          #if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
          FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_ERASE_MEMORY);
          #endif
          /* close the file */
          f_close(&fatFsObjects.file);
          return;
        }
      }
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("OK\n\r");
#endif
      /* all okay, then go to programming state */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_PROGRAMMING;
    }
  }
  /* ------------------------------- programming ------------------------------------- */
  else if (firmwareUpdateState == FIRMWARE_UPDATE_STATE_PROGRAMMING)
  {
    /* read a line from the file */
    read_line_ptr = f_gets(lineParseObject.line, sizeof(lineParseObject.line), &fatFsObjects.file);
    /* check if an error occurred */
    if (f_error(&fatFsObjects.file) > 0)
    {
      /* cannot continue with firmware update so go back to idle state */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("Reading line from file...ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
      FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_READ_FROM_FILE);
#endif
      /* close the file */
      f_close(&fatFsObjects.file);
      return;
    }
    /* parse the S-Record line if the line is not empty */
    if (read_line_ptr != BLT_NULL)
    {
      parse_result = FileSrecParseLine(lineParseObject.line, &lineParseObject.address, lineParseObject.data);
      /* check parsing result */
      if (parse_result == ERROR_SREC_INVALID_CHECKSUM)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("Invalid checksum found...ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_INVALID_CHECKSUM_IN_FILE);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
    }
    /* only process parsing results if the line contained address/data info */
    if (parse_result > 0)
    {
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("Programming ");
      /* convert size to string  */
      FileLibLongToIntString(parse_result, loggingStr);
      FileFirmwareUpdateLogHook(loggingStr);
      FileFirmwareUpdateLogHook(" bytes to memory at 0x");
      /* convert address to hex-string  */
      FileLibByteToHexString((blt_int8u)(lineParseObject.address >> 24), &loggingStr[0]);
      FileLibByteToHexString((blt_int8u)(lineParseObject.address >> 16), &loggingStr[2]);
      FileLibByteToHexString((blt_int8u)(lineParseObject.address >> 8), &loggingStr[4]);
      FileLibByteToHexString((blt_int8u)lineParseObject.address, &loggingStr[6]);
      FileFirmwareUpdateLogHook(loggingStr);
      FileFirmwareUpdateLogHook("...");
#endif
      /* program the data */
      if (NvmWrite(lineParseObject.address, parse_result, lineParseObject.data) == BLT_FALSE)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_PROGRAM_MEMORY);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("OK\n\r");
#endif
    }
    /* check if the end of the file was reached */
    if (f_eof(&fatFsObjects.file) > 0)
    {
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("Writing program checksum...");
#endif
      /* finish the programming by writing the checksum */
      if (NvmDone() == BLT_FALSE)
      {
        /* cannot continue with firmware update so go back to idle state */
        firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_LOGGING_ENABLE > 0)
        FileFirmwareUpdateLogHook("ERROR\n\r");
#endif
#if (BOOT_FILE_ERROR_HOOK_ENABLE > 0)
        FileFirmwareUpdateErrorHook(FILE_ERROR_CANNOT_WRITE_CHECKSUM);
#endif
        /* close the file */
        f_close(&fatFsObjects.file);
        return;
      }
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("OK\n\r");
      FileFirmwareUpdateLogHook("Closing firmware file\n\r");
#endif
      /* close the file */
      f_close(&fatFsObjects.file);
#if (BOOT_FILE_LOGGING_ENABLE > 0)
      FileFirmwareUpdateLogHook("Firmware update successfully completed\n\r");
#endif
      /* all done so transistion back to idle mode */
      firmwareUpdateState = FIRMWARE_UPDATE_STATE_IDLE;
#if (BOOT_FILE_COMPLETED_HOOK_ENABLE > 0)
      /* inform application about update completed event via hook function */
      FileFirmwareUpdateCompletedHook();
#endif
      /* attempt to start the user program now that programming is done */
      CpuStartUserProgram();
    }
  }
} /*** end of FileTask ***/


/************************************************************************************//**
** \brief     Inspects a line from a Motorola S-Record file to determine its type.
** \param     line A line from the S-Record.
** \return    the S-Record line type.
**
****************************************************************************************/
tSrecLineType FileSrecGetLineType(const blt_char *line)
{
  /* check if the line starts with the 'S' character, followed by a digit */
  if ((toupper((blt_int16s)(line[0])) != 'S') || (isdigit((blt_int16s)(line[1])) == 0))
  {
    /* not a valid S-Record line type */
    return LINE_TYPE_UNSUPPORTED;
  }
  /* determine the line type */
  if (line[1] == '1')
  {
    return LINE_TYPE_S1;
  }
  if (line[1] == '2')
  {
    return LINE_TYPE_S2;
  }
  if (line[1] == '3')
  {
    return LINE_TYPE_S3;
  }
  /* still here so not a supported line type found */
  return LINE_TYPE_UNSUPPORTED;
} /*** end of FileSrecGetLineType ***/


/************************************************************************************//**
** \brief     Inspects an S1, S2 or S3 line from a Motorola S-Record file to
**            determine if the checksum at the end is corrrect.
** \param     line An S1, S2 or S3 line from the S-Record.
** \return    BLT_TRUE if the checksum is correct, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool FileSrecVerifyChecksum(const blt_char *line)
{
  blt_int16u bytes_on_line;
  blt_int8u  checksum = 0;

  /* adjust pointer to point to byte count value */
  line += 2;
  /* read out the number of byte values that follow on the line */
  bytes_on_line = FileLibHexStringToByte(line);
  /* byte count is part of checksum */
  checksum += bytes_on_line;
  /* adjust pointer to the first byte of the address */
  line += 2;
  /* add byte values of address and data, but not the final checksum */
  do
  {
    /* add the next byte value to the checksum */
    checksum += FileLibHexStringToByte(line);
    /* update counter */
    bytes_on_line--;
    /* point to next hex string in the line */
    line += 2;
  }
  while (bytes_on_line > 1);
  /* the checksum is calculated by summing up the values of the byte count, address and
   * databytes and then taking the 1-complement of the sum's least signigicant byte */
  checksum = ~checksum;
  /* finally verify the calculated checksum with the one at the end of the line */
  if (checksum != FileLibHexStringToByte(line))
  {
    /* checksum incorrect */
    return BLT_FALSE;
  }
  /* still here so the checksum was correct */
  return BLT_TRUE;
} /*** end of FileSrecVerifyChecksum ***/


/************************************************************************************//**
** \brief     Parses a line from a Motorola S-Record file and looks for S1, S2 or S3
**            lines with data. Note that if a null pointer is passed as the data
**            parameter, then no data is extracted from the line.
** \param     line    A line from the S-Record.
** \param     address Address found in the S-Record data line.
** \param     data    Byte array where the data bytes from the S-Record data line
**                    are stored.
** \return    The number of data bytes found on the S-record data line, 0 in case
**            the line is not an S1, S2 or S3 line or ERROR_SREC_INVALID_CHECKSUM
**            in case the checksum validation failed.
**
****************************************************************************************/
blt_int16s FileSrecParseLine(const blt_char *line, blt_addr *address, blt_int8u *data)
{
  tSrecLineType lineType;
  blt_int16s    data_byte_count = 0;
  blt_int16u    bytes_on_line;
  blt_int16u    i;

  /* check pointers and not that data can be a null pointer */
  ASSERT_RT((address != BLT_NULL) && (line != BLT_NULL));
  /* figure out what type of line we are dealing with */
  lineType = FileSrecGetLineType(line);
  /* make sure it is one that we can parse */
  if (lineType == LINE_TYPE_UNSUPPORTED)
  {
    /* not a parsing error, but simply no data on this line */
    return 0;
  }
  /* verify the checksum */
  if (FileSrecVerifyChecksum(line) == BLT_FALSE)
  {
    /* error on data line encountered */
    return ERROR_SREC_INVALID_CHECKSUM;
  }
  /* all good so far, now read out the address and databytes for the line */
  switch (lineType)
  {
    /* ---------------------------- S1 line type ------------------------------------- */
    case LINE_TYPE_S1:
      /* adjust pointer to point to byte count value */
      line += 2;
      /* read out the number of byte values that follow on the line */
      bytes_on_line = FileLibHexStringToByte(line);
      /* read out the 16-bit address */
      line += 2;
      *address = FileLibHexStringToByte(line) << 8;
      line += 2;
      *address += FileLibHexStringToByte(line);
      /* adjust pointer to point to the first data byte after the address */
      line += 2;
      /* determine how many data bytes are on the line */
      data_byte_count = bytes_on_line - 3; /* -2 bytes address, -1 byte checksum */
      /* read and store data bytes if requested */
      if (data != BLT_NULL)
      {
        for (i=0; i<data_byte_count; i++)
        {
          data[i] = FileLibHexStringToByte(line);
          line += 2;
        }
      }
      break;

    /* ---------------------------- S2 line type ------------------------------------- */
    case LINE_TYPE_S2:
      /* adjust pointer to point to byte count value */
      line += 2;
      /* read out the number of byte values that follow on the line */
      bytes_on_line = FileLibHexStringToByte(line);
      /* read out the 32-bit address */
      line += 2;
      *address = FileLibHexStringToByte(line) << 16;
      line += 2;
      *address += FileLibHexStringToByte(line) << 8;
      line += 2;
      *address += FileLibHexStringToByte(line);
      /* adjust pointer to point to the first data byte after the address */
      line += 2;
      /* determine how many data bytes are on the line */
      data_byte_count = bytes_on_line - 4; /* -3 bytes address, -1 byte checksum */
      /* read and store data bytes if requested */
      if (data != BLT_NULL)
      {
        for (i=0; i<data_byte_count; i++)
        {
          data[i] = FileLibHexStringToByte(line);
          line += 2;
        }
      }
      break;

    /* ---------------------------- S3 line type ------------------------------------- */
    case LINE_TYPE_S3:
      /* adjust pointer to point to byte count value */
      line += 2;
      /* read out the number of byte values that follow on the line */
      bytes_on_line = FileLibHexStringToByte(line);
      /* read out the 32-bit address */
      line += 2;
      *address = FileLibHexStringToByte(line) << 24;
      line += 2;
      *address += FileLibHexStringToByte(line) << 16;
      line += 2;
      *address += FileLibHexStringToByte(line) << 8;
      line += 2;
      *address += FileLibHexStringToByte(line);
      /* adjust pointer to point to the first data byte after the address */
      line += 2;
      /* determine how many data bytes are on the line */
      data_byte_count = bytes_on_line - 5; /* -4 bytes address, -1 byte checksum */
      /* read and store data bytes if requested */
      if (data != BLT_NULL)
      {
        for (i=0; i<data_byte_count; i++)
        {
          data[i] = FileLibHexStringToByte(line);
          line += 2;
        }
      }
      break;

    default:
      break;
  }

  return data_byte_count;
} /*** end of FileSrecParseLine ***/


#if (BOOT_FILE_LOGGING_ENABLE > 0)
/************************************************************************************//**
** \brief     Helper function to convert a 4-bit value to a character that represents its
**            value in hexadecimal format.
**              Example: FileLibByteNibbleToChar(11)  --> returns 'B'.
** \param     nibble 4-bit value to convert.
** \return    The resulting byte value.
**
****************************************************************************************/
static blt_char FileLibByteNibbleToChar(blt_int8u nibble)
{
  blt_char  c;

  /* convert to ASCII value */
  c = (nibble & 0x0f) + '0';
  if (nibble > 9)
  {
    c += 7;
  }
  else
  {
    c = toupper((blt_int16s)c);
  }
  /* return the character */
  return c;
} /*** end of FileLibByteNibbleToChar ***/


/************************************************************************************//**
** \brief     Helper function to convert a byte value to a string representing the
**            value in hexadecimal format.
**              Example: FileLibByteToHexString(28, strBuffer)  --> returns "1C".
** \param     byte_val    8-bit value to convert.
** \param     destination Pointer to character buffer for storing the results.
** \return    The resulting string.
**
****************************************************************************************/
static blt_char *FileLibByteToHexString(blt_int8u byte_val, blt_char *destination)
{
  /* first the most significant n-bit nibble */
  destination[0] = FileLibByteNibbleToChar(byte_val >> 4);
  /* next the least significant n-bit nibble */
  destination[1] = FileLibByteNibbleToChar(byte_val & 0x0f);
  /* add string termination */
  destination[2] = '\0';
  /* return pointer to resulting string */
  return destination;
} /*** end of FileLibByteToHexString ***/


/************************************************************************************//**
** \brief     Helper function to convert a 32-bit unsigned number to a string that
**            represents its decimal value.
**              Example: FileLibLongToIntString(1234, strBuffer)  --> returns "1234".
** \param     long_val    32-bit value to convert.
** \param     destination Pointer to character buffer for storing the results.
** \return    The resulting string.
**
****************************************************************************************/
static blt_char *FileLibLongToIntString(blt_int32u long_val, blt_char *destination)
{
  blt_int32u long_val_cpy = long_val;

  /* first determine how many digits there will be */
  do
  {
    destination++;
    long_val_cpy /= 10;
  }
  while (long_val_cpy > 0);
  /* add space for the string termination and add it */
  *destination = '\0';
  /* now add the digits from right to left */
  long_val_cpy = long_val;
  do
  {
    /* set write pointer to where the next character should go */
    destination--;
    /* write digit in ASCII format */
    *destination = long_val_cpy % 10 + '0';
    /* move on to the next digit */
    long_val_cpy /= 10;
  }
  while (long_val_cpy > 0);

  return destination;
} /*** end of FileLibLongToIntString ***/
#endif /* (BOOT_FILE_LOGGING_ENABLE > 0) */


/************************************************************************************//**
** \brief     Helper function to convert a sequence of 2 characters that represent
**            a hexadecimal value to the actual byte value.
**              Example: FileLibHexStringToByte("2f")  --> returns 47.
** \param     hexstring String beginning with 2 characters that represent a hexa-
**                      decimal value.
** \return    The resulting byte value.
**
****************************************************************************************/
static blt_int8u FileLibHexStringToByte(const blt_char *hexstring)
{
  blt_int8u result = 0;
  blt_char  c;
  blt_int8u counter;

  /* a hexadecimal character is 2 characters long (i.e 0x4F minus the 0x part) */
  for (counter=0; counter < 2; counter++)
  {
    /* read out the character */
    c = toupper((blt_int16s)(hexstring[counter]));
    /* check that the character is 0..9 or A..F */
    if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A')))
    {
      /* character not valid */
      return 0;
    }
    /* convert character to 4-bit value (check ASCII table for more info) */
    c -= '0';
    if (c > 9)
    {
      c -= 7;
    }
    /* add it to the result */
    result = (result << 4) + c;
  }
  /* return the results */
  return result;
} /*** end of FileLibHexStringToByte ***/

#endif /* BOOT_FILE_SYS_ENABLE > 0 */


/*********************************** end of file.c *************************************/
