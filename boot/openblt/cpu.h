/************************************************************************************//**
* \file         Source/cpu.h
* \brief        Bootloader cpu module header file.
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
#ifndef CPU_H
#define CPU_H


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void CpuInit(void);
void CpuStartUserProgram(void);
void CpuMemCopy(blt_addr dest, blt_addr src, blt_int16u len);
void CpuMemSet(blt_addr dest, blt_int8u value, blt_int16u len);
void CpuIrqDisable(void);
void CpuIrqEnable(void);


#endif /* CPU_H */
/*********************************** end of cpu.h **************************************/
