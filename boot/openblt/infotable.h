/************************************************************************************//**
* \file         Source/infotable.h
* \brief        Info table check feature header file.
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
#ifndef INFOTABLE_H
#define INFOTABLE_H

#if (BOOT_INFO_TABLE_ENABLE > 0)
/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Enumeration for the info tables. */
typedef enum
{
  INFO_TABLE_ID_INTERNAL_RAM,  /**< Internal RAM buffer for to-be-programmed firmware. */
  INFO_TABLE_ID_FIRMWARE_NVM   /**< Currently programmed firmware in non-volatile mem. */
} tInfoTableId;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void       InfoTableInit(void);
blt_bool   InfoTableCheck(void);
blt_addr   InfoTableGetPtr(tInfoTableId infoTableId);
void       InfoTableClear(tInfoTableId infoTableId);
blt_int16u InfoTableCurrentSize(tInfoTableId infoTableId);
blt_bool   InfoTableAddData(tInfoTableId infoTableId, blt_int8u const * data,
                            blt_int16u len);


#endif /* BOOT_INFO_TABLE_ENABLE > 0 */


#endif /* INFOTABLE_H */
/*********************************** end of infotable.h ********************************/
