/************************************************************************************//**
* \file         Source/usb.h
* \brief        Bootloader USB communication interface header file.
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
#ifndef USB_H
#define USB_H

#if (BOOT_COM_USB_ENABLE > 0)
/****************************************************************************************
* Function prototypes
****************************************************************************************/
void     UsbInit(void);
void     UsbFree(void);
void     UsbTransmitPacket(blt_int8u *data, blt_int8u len);
blt_bool UsbReceivePacket(blt_int8u *data, blt_int8u *len);

/****************************************************************************************
* Hook functions
****************************************************************************************/
extern void UsbEnterLowPowerModeHook(void);
extern void UsbLeaveLowPowerModeHook(void);
extern void UsbConnectHook(blt_bool connect);


#endif /* BOOT_COM_USB_ENABLE > 0 */


#endif /* USB_H */
/*********************************** end of usb.h **************************************/
