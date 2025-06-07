/************************************************************************************//**
* \file         Source/net.h
* \brief        Bootloader TCP/IP network communication interface header file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2014  by Feaser    http://www.feaser.com    All rights reserved
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
#ifndef NET_H
#define NET_H

#if (BOOT_COM_NET_ENABLE > 0)
/****************************************************************************************
* Macro definitions
****************************************************************************************/
#ifndef UIP_APPCALL
#define UIP_APPCALL NetApp
#endif /* UIP_APPCALL */


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Define the uip_tcp_appstate_t datatype. This is the state of our tcp/ip
 *         application, and the memory required for this state is allocated together
 *         with each TCP connection. One application state for each TCP connection.
 */
typedef struct net_state
{
  blt_int32u dto_counter;
  blt_int8u  dto_data[BOOT_COM_NET_TX_MAX_DATA + 4]; /* +4 for counter overhead */
  blt_int16u dto_len;
  blt_bool   dto_tx_req;
  blt_bool   dto_tx_pending;
} uip_tcp_appstate_t;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void     NetInit(void);
#if (BOOT_COM_NET_DEFERRED_INIT_ENABLE == 1)
void     NetDeferredInit(void);
#endif
void     NetApp(void);
void     NetTransmitPacket(blt_int8u *data, blt_int8u len);
blt_bool NetReceivePacket(blt_int8u *data, blt_int8u *len);

#else /* BOOT_COM_NET_ENABLE > 0 */

typedef struct net_state
{
  blt_int8u  unused;
} uip_tcp_appstate_t;

#define UIP_APPCALL();

#endif /* BOOT_COM_NET_ENABLE > 0 */

#endif /* NET_H */
/*********************************** end of net.h **************************************/
