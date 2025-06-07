/************************************************************************************//**
* \file         Source/com.h
* \brief        Bootloader communication interface header file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2011  by Feaser    http://www.feaser.com    All rights reserved
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
#ifndef COM_H
#define COM_H

#if (BOOT_COM_ENABLE > 0)
/****************************************************************************************
* Include files
****************************************************************************************/
#include "xcp.h"                                      /* xcp communication layer       */
#if (BOOT_COM_MBRTU_ENABLE > 0)
#include "mb.h"                                       /* modbus driver module          */
#endif /* BOOT_COM_MBRTU_ENABLE > 0 */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Defines the maximum number of bytes for transport layer reception
 *         depending on the activates interface(s).
 */
#define BOOT_COM_RX_MAX_DATA            (1)
/* update in case CAN interface uses more */
#if (BOOT_COM_CAN_RX_MAX_DATA > BOOT_COM_RX_MAX_DATA)
#undef BOOT_COM_RX_MAX_DATA
#define BOOT_COM_RX_MAX_DATA            (BOOT_COM_CAN_RX_MAX_DATA)
#endif
/* update in case RS232 interface uses more */
#if (BOOT_COM_RS232_RX_MAX_DATA > BOOT_COM_RX_MAX_DATA)
#undef BOOT_COM_RX_MAX_DATA
#define BOOT_COM_RX_MAX_DATA            (BOOT_COM_RS232_RX_MAX_DATA)
#endif
/* update in case Modbus RTU interface uses more */
#if (BOOT_COM_MBRTU_RX_MAX_DATA > BOOT_COM_RX_MAX_DATA)
#undef BOOT_COM_RX_MAX_DATA
#define BOOT_COM_RX_MAX_DATA            (BOOT_COM_MBRTU_RX_MAX_DATA)
#endif
/* update in case USB interface uses more */
#if (BOOT_COM_USB_RX_MAX_DATA > BOOT_COM_RX_MAX_DATA)
#undef BOOT_COM_RX_MAX_DATA
#define BOOT_COM_RX_MAX_DATA            (BOOT_COM_USB_RX_MAX_DATA)
#endif
/* update in case NET interface uses more */
#if (BOOT_COM_NET_RX_MAX_DATA > BOOT_COM_RX_MAX_DATA)
#undef BOOT_COM_RX_MAX_DATA
#define BOOT_COM_RX_MAX_DATA            (BOOT_COM_NET_RX_MAX_DATA)
#endif

/** \brief Defines the maximum number of bytes for transport layer transmission
 *         depending on the activates interface(s).
 */
#define BOOT_COM_TX_MAX_DATA            (1)
/* update in case CAN interface uses more */
#if (BOOT_COM_CAN_TX_MAX_DATA > BOOT_COM_TX_MAX_DATA)
#undef BOOT_COM_TX_MAX_DATA
#define BOOT_COM_TX_MAX_DATA            (BOOT_COM_CAN_TX_MAX_DATA)
#endif
/* update in case RS232 interface uses more */
#if (BOOT_COM_RS232_TX_MAX_DATA > BOOT_COM_TX_MAX_DATA)
#undef BOOT_COM_TX_MAX_DATA
#define BOOT_COM_TX_MAX_DATA            (BOOT_COM_RS232_TX_MAX_DATA)
#endif
/* update in case Modbus RTU interface uses more */
#if (BOOT_COM_MBRTU_TX_MAX_DATA > BOOT_COM_TX_MAX_DATA)
#undef BOOT_COM_TX_MAX_DATA
#define BOOT_COM_TX_MAX_DATA            (BOOT_COM_MBRTU_TX_MAX_DATA)
#endif
/* update in case USB interface uses more */
#if (BOOT_COM_USB_TX_MAX_DATA > BOOT_COM_TX_MAX_DATA)
#undef BOOT_COM_TX_MAX_DATA
#define BOOT_COM_TX_MAX_DATA            (BOOT_COM_USB_TX_MAX_DATA)
#endif
/* update in case NET interface uses more */
#if (BOOT_COM_NET_TX_MAX_DATA > BOOT_COM_TX_MAX_DATA)
#undef BOOT_COM_TX_MAX_DATA
#define BOOT_COM_TX_MAX_DATA            (BOOT_COM_NET_TX_MAX_DATA)
#endif


/****************************************************************************************
* Plausibility
****************************************************************************************/
#if (BOOT_COM_TX_MAX_DATA < 1)
#undef BOOT_COM_TX_MAX_DATA
#define BOOT_COM_TX_MAX_DATA   (8)
#endif

#if (BOOT_COM_TX_MAX_DATA > 256)
#error  "COM.H, BOOT_COM_TX_MAX_DATA cannot be larger than 256."
#endif

#if (BOOT_COM_RX_MAX_DATA < 1)
#undef BOOT_COM_RX_MAX_DATA
#define BOOT_COM_RX_MAX_DATA   (8)
#endif

#if (BOOT_COM_RX_MAX_DATA > 65536)
#error  "COM.H, BOOT_COM_RX_MAX_DATA cannot be larger than 65536."
#endif


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Enumeration for the different communication interfaces. */
typedef enum
{
  COM_IF_RS232,                                  /**< RS232 interface                  */
  COM_IF_MBRTU,                                  /**< Modbus RTU interface             */
  COM_IF_CAN,                                    /**< CAN interface                    */
  COM_IF_USB,                                    /**< USB interface                    */
  COM_IF_NET,                                    /**< NET interface                    */
  COM_IF_OTHER                                   /**< Other interface                  */
} tComInterfaceId;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void       ComInit(void);
#if (BOOT_COM_DEFERRED_INIT_ENABLE == 1)
void       ComDeferredInit(void);
#endif
void       ComTask(void);
void       ComFree(void);
blt_int16u ComGetActiveInterfaceMaxRxLen(void);
blt_int16u ComGetActiveInterfaceMaxTxLen(void);
void       ComTransmitPacket(blt_int8u *data, blt_int16u len);
blt_bool   ComIsConnected(void);

#endif /* BOOT_COM_ENABLE > 0 */

#endif /* COM_H */
/*********************************** end of com.h **************************************/
