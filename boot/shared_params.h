/************************************************************************************//**
* \file         Demo/ARMCM33_STM32H5_Nucleo_H563ZI_GCC/Boot/shared_params.h
* \brief        Shared RAM parameters header file.
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
#ifndef SHARED_PARAMS_H
#define SHARED_PARAMS_H

/****************************************************************************************
* Include files
****************************************************************************************/
#include <stdint.h>                                    /* Standard integer types.      */
#include <stdbool.h>                                   /* Standard boolean types.      */


/****************************************************************************************
* Configuration macros
****************************************************************************************/
/** \brief Configuration macro for specifying the size of the data inside the parameter
 *         buffer. This is the length in bytes of the actual parameter data, so
 *         excluding the bufferId and checksum.
 */
#define SHARED_PARAMS_CFG_BUFFER_DATA_LEN        (56u)


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void SharedParamsInit(void);
bool SharedParamsReadByIndex(uint32_t idx, uint8_t * value);
bool SharedParamsWriteByIndex(uint32_t idx, uint8_t value);


#endif /* SHARED_PARAMS_H */
/*********************************** end of shared_params.h ****************************/
