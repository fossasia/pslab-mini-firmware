/************************************************************************************//**
* \file         Source/mb.h
* \brief        Bootloader Modbus communication interface header file.
* \ingroup      Core
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2023  by Feaser    http://www.feaser.com    All rights reserved
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
#ifndef MB_H
#define MB_H

#if (BOOT_COM_MBRTU_ENABLE > 0)

/* XCP on Modbus RTU
 * -----------------
 * The XCP packets are embedded inside a Modbus RTU packet using a user-defined function
 * code. The (configurable) user-defined function code is 109 by default. When embedding
 * the XCP packet, an additional XCP packet length byte is added:
 *
 *  -----------------------------------------------------------------------------------
 * | Node ID | Fcn Code | XCP len |        XCP data                 |       CRC        |
 *  -----------------------------------------------------------------------------------
 *
 * This extra length byte is theoretically not needed, because the T3_5 end-of-packet
 * time event informs you when the packet reception is done. However, a Windows or Linux
 * based OS is typically used on the host side. Being a non real-time OS makes detecting
 * the exact T3_5 timeout tricky. The best approach is to wait way longer than T3_5,
 * otherwise the T3_5 timeout is sometimes already detected between bytes of the same
 * packet. Although this works, it causes unnecessary delays between Modbus RTU packets.
 *
 * By having the extra XCP length embedded, you can determine when the last Modbus RTU
 * packet byte was received, without having to rely on the T3_5 end-of-packet time event.
 *
 */

/****************************************************************************************
* Macro definitions
****************************************************************************************/
#ifndef BOOT_COM_MBRTU_FCT_CODE_USER_XCP
/** \brief This module embeds the bootloader's XCP communication packets inside Modbus
 *         RTU communication packets. As this is a non-standard functionality of Modbus,
 *         a user-defined function code is used to extend the Modbus functionality for
 *         the purpose of embedded XCP packets. User-defined function codes are allowed,
 *         as long as they are in the range 65..72 or 100..110. By default, the user-
 *         defined function code 109 is selected. However, some other Modbus device on
 *         the network might also already use this. For this reason, you can override
 *         the assigned function code by placing this macro in "blt_conf.h" with a
 *         different value assigned to it.
 */
#define BOOT_COM_MBRTU_FCT_CODE_USER_XCP  (109u)
#endif

#ifndef BOOT_COM_MBRTU_DRIVER_OUTPUT_ENABLE_DELAY_US
/** \brief Depending on the hardware, RS485 wire length and capacities, the communication
 *         link might need some time to stabilize after changing the DE/NRE GPIO pin.
 *         With this macro you can configure a delay time before and after the DE/NRE
 *         GPIO pin was toggled to enable transmit mode. Values should be a multiple of
 *         10. Note that you can override the value by placing this macro in "blt_conf.h"
 *         with a different value assigned to it.
 */
#define BOOT_COM_MBRTU_DRIVER_OUTPUT_ENABLE_DELAY_US  (10u)
#endif

#ifndef BOOT_COM_MBRTU_DRIVER_OUTPUT_DISABLE_DELAY_US
/** \brief Depending on the hardware, RS485 wire length and capacities, the communication
 *         link might need some time to stabilize after changing the DE/NRE GPIO pin.
 *         With this macro you can configure a delay time before and after the DE/NRE
 *         GPIO pin was toggled to enable reception mode. Values should be a multiple of
 *         10. Note that you can override the value by placing this macro in "blt_conf.h"
 *         with a different value assigned to it.
 */
#define BOOT_COM_MBRTU_DRIVER_OUTPUT_DISABLE_DELAY_US (10u)
#endif


/****************************************************************************************
* Hook functions
****************************************************************************************/
extern void MbRtuDriverOutputControlHook(blt_bool enable);


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void       MbRtuInit(void);
void       MbRtuTransmitPacket(blt_int8u *data, blt_int8u len);
blt_bool   MbRtuReceivePacket(blt_int8u *data, blt_int8u *len);
blt_int16u MbRtuCrcCalculate(blt_int8u const *data, blt_int16u len);
void       MbRtuDelay(blt_int16u delay_us);
blt_int16u MbRtuFreeRunningCounterGet(void);

#endif /* BOOT_COM_MBRTU_ENABLE > 0 */


#endif /* MB_H */
/*********************************** end of mb.h ***************************************/
