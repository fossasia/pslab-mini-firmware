/************************************************************************************//**
* \file         Source/nvm.h
* \brief        Bootloader non-volatile memory driver header file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2016  by Feaser    http://www.feaser.com    All rights reserved
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
#ifndef NVM_H
#define NVM_H

/****************************************************************************************
* Function prototypes
****************************************************************************************/
void     NvmInit(void);
blt_bool NvmWrite(blt_addr addr, blt_int32u len, blt_int8u *data);
blt_bool NvmErase(blt_addr addr, blt_int32u len);
blt_bool NvmVerifyChecksum(void);
blt_addr NvmGetUserProgBaseAddress(void);
blt_bool NvmDone(void);


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/* return codes for hook function NvmWrite/Erase */
/** \brief Return code for success. */
#define BLT_NVM_ERROR                   (0x00)
/** \brief Return code for error. */
#define BLT_NVM_OKAY                    (0x01)
/** \brief Return code for not in range. */
#define BLT_NVM_NOT_IN_RANGE            (0x02)


#endif /* NVM_H */
/*********************************** end of nvm.h **************************************/
