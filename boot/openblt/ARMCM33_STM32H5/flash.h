/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/flash.h
* \brief        Bootloader flash driver header file.
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
#ifndef FLASH_H
#define FLASH_H

/****************************************************************************************
* Function prototypes
****************************************************************************************/
void     FlashInit(void);
blt_bool FlashWrite(blt_addr addr, blt_int32u len, blt_int8u *data);
blt_bool FlashErase(blt_addr addr, blt_int32u len);
blt_bool FlashWriteChecksum(void);
blt_bool FlashVerifyChecksum(void);
blt_bool FlashDone(void);
blt_addr FlashGetUserProgBaseAddress(void);


#endif /* FLASH_H */
/*********************************** end of flash.h ************************************/
