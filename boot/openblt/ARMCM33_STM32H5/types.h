/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/types.h
* \brief        Bootloader types header file.
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
#ifndef TYPES_H
#define TYPES_H

/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Boolean true value. */
#define BLT_TRUE       (1)
/** \brief Boolean false value. */
#define BLT_FALSE      (0)
/** \brief NULL pointer value. */
#define BLT_NULL       ((void *)0)


/****************************************************************************************
* Type definitions
****************************************************************************************/
typedef unsigned char   blt_bool;                     /**<  boolean type               */
typedef char            blt_char;                     /**<  character type             */
typedef unsigned long   blt_addr;                     /**<  memory address type        */
typedef unsigned char   blt_int8u;                    /**<  8-bit unsigned integer     */
typedef signed char     blt_int8s;                    /**<  8-bit   signed integer     */
typedef unsigned short  blt_int16u;                   /**< 16-bit unsigned integer     */
typedef signed short    blt_int16s;                   /**< 16-bit   signed integer     */
typedef unsigned int    blt_int32u;                   /**< 32-bit unsigned integer     */
typedef signed int      blt_int32s;                   /**< 32-bit   signed integer     */
typedef unsigned long long  blt_int64u;               /**< 64-bit unsigned integer     */
typedef signed long long    blt_int64s;               /**< 64-bit   signed integer     */


#endif /* TYPES_H */
/*********************************** end of types.h ************************************/
