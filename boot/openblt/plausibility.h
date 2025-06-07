/************************************************************************************//**
* \file         Source/plausibility.h
* \brief        Bootloader plausibility check header file, for checking the configuration
*               at compile time.
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
#ifndef PLAUSIBILITY_H
#define PLAUSIBILITY_H

/****************************************************************************************
*   C P U   D R I V E R   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_CPU_XTAL_SPEED_KHZ
#error "BOOT_CPU_XTAL_SPEED_KHZ is missing in blt_conf.h"
#endif

#if (BOOT_CPU_XTAL_SPEED_KHZ <= 0)
#error "BOOT_CPU_XTAL_SPEED_KHZ must be > 0"
#endif

#ifndef BOOT_CPU_SYSTEM_SPEED_KHZ
#error "BOOT_CPU_SYSTEM_SPEED_KHZ is missing in blt_conf.h"
#endif

#if (BOOT_CPU_SYSTEM_SPEED_KHZ <= 0)
#error "BOOT_CPU_SYSTEM_SPEED_KHZ must be > 0"
#endif

#ifndef BOOT_CPU_BYTE_ORDER_MOTOROLA
#error "BOOT_CPU_BYTE_ORDER_MOTOROLA is missing in blt_conf.h"
#endif

#if (BOOT_CPU_BYTE_ORDER_MOTOROLA < 0) || (BOOT_CPU_BYTE_ORDER_MOTOROLA > 1)
#error "BOOT_CPU_BYTE_ORDER_MOTOROLA must be 0 or 1"
#endif

#ifndef BOOT_CPU_USER_PROGRAM_START_HOOK
#define BOOT_CPU_USER_PROGRAM_START_HOOK   (0)
#endif

#if (BOOT_CPU_USER_PROGRAM_START_HOOK < 0) || (BOOT_CPU_USER_PROGRAM_START_HOOK > 1)
#error "BOOT_CPU_USER_PROGRAM_START_HOOK must be 0 or 1"
#endif


/****************************************************************************************
*   C O M M U N I C A T I O N   I N T E R F A C E   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_COM_CAN_TX_MAX_DATA
#define BOOT_COM_CAN_TX_MAX_DATA       (0)
#endif

#ifndef BOOT_COM_CAN_RX_MAX_DATA
#define BOOT_COM_CAN_RX_MAX_DATA       (0)
#endif

#ifndef BOOT_COM_RS232_TX_MAX_DATA
#define BOOT_COM_RS232_TX_MAX_DATA     (0)
#endif

#ifndef BOOT_COM_RS232_RX_MAX_DATA
#define BOOT_COM_RS232_RX_MAX_DATA     (0)
#endif

#ifndef BOOT_COM_MBRTU_TX_MAX_DATA
#define BOOT_COM_MBRTU_TX_MAX_DATA      (0)
#endif

#ifndef BOOT_COM_MBRTU_RX_MAX_DATA
#define BOOT_COM_MBRTU_RX_MAX_DATA      (0)
#endif

#ifndef BOOT_COM_USB_TX_MAX_DATA
#define BOOT_COM_USB_TX_MAX_DATA       (0)
#endif

#ifndef BOOT_COM_USB_RX_MAX_DATA
#define BOOT_COM_USB_RX_MAX_DATA       (0)
#endif

#ifndef BOOT_COM_NET_TX_MAX_DATA
#define BOOT_COM_NET_TX_MAX_DATA       (0)
#endif

#ifndef BOOT_COM_NET_RX_MAX_DATA
#define BOOT_COM_NET_RX_MAX_DATA       (0)
#endif


#ifndef BOOT_COM_CAN_ENABLE
#define BOOT_COM_CAN_ENABLE             (0)
#endif

#if (BOOT_COM_CAN_ENABLE > 0)
/* Note that the value check was disabled for the following configuration macros, which
 * makes it possible to link them to a function, allowing a more dynamic configuration:
 * - BOOT_COM_CAN_BAUDRATE
 * - BOOT_COM_CAN_TX_MSG_ID
 * - BOOT_COM_CAN_RX_MSG_ID
 */
#ifndef BOOT_COM_CAN_BAUDRATE
#error "BOOT_COM_CAN_BAUDRATE is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_CAN_TX_MSG_ID
#error "BOOT_COM_CAN_TX_MSG_ID is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_CAN_TX_MAX_DATA
#error "BOOT_COM_CAN_TX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_CAN_TX_MAX_DATA <= 0)
#error "BOOT_COM_CAN_TX_MAX_DATA must be > 0"
#endif

#if (BOOT_COM_CAN_TX_MAX_DATA > 8)
#error "BOOT_COM_CAN_TX_MAX_DATA must be <= 8"
#endif

#ifndef BOOT_COM_CAN_RX_MSG_ID
#error "BOOT_COM_CAN_RX_MSG_ID is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_CAN_RX_MAX_DATA
#error "BOOT_COM_CAN_RX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_CAN_RX_MAX_DATA <= 0)
#error "BOOT_COM_CAN_RX_MAX_DATA must be > 0"
#endif

#if (BOOT_COM_CAN_RX_MAX_DATA > 8)
#error "BOOT_COM_CAN_RX_MAX_DATA must be <= 8"
#endif

#ifndef BOOT_COM_CAN_CHANNEL_INDEX
#error "BOOT_COM_CAN_CHANNEL_INDEX is missing in blt_conf.h"
#endif

#if (BOOT_COM_CAN_CHANNEL_INDEX < 0)
#error "BOOT_COM_CAN_CHANNEL_INDEX must be >= 0"
#endif

#endif /* BOOT_COM_CAN_ENABLE > 0 */

#ifndef BOOT_COM_RS232_ENABLE
#define BOOT_COM_RS232_ENABLE           (0)
#endif

#if (BOOT_COM_RS232_ENABLE > 0)
/* Note that the value check was disabled for the following configuration macros, which
 * makes it possible to link them to a function, allowing a more dynamic configuration:
 * - BOOT_COM_RS232_BAUDRATE
 */
#ifndef BOOT_COM_RS232_BAUDRATE
#error "BOOT_COM_RS232_BAUDRATE is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_RS232_TX_MAX_DATA
#error "BOOT_COM_RS232_TX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_RS232_TX_MAX_DATA <= 0)
#error "BOOT_COM_RS232_TX_MAX_DATA must be > 0"
#endif

#if (BOOT_COM_RS232_TX_MAX_DATA > 255)
#error "BOOT_COM_RS232_TX_MAX_DATA must be <= 255"
#endif

#ifndef BOOT_COM_RS232_RX_MAX_DATA
#error "BOOT_COM_RS232_RX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_RS232_RX_MAX_DATA <= 0)
#error "BOOT_COM_RS232_RX_MAX_DATA must be > 0"
#endif

#if (BOOT_COM_RS232_RX_MAX_DATA > 255)
#error "BOOT_COM_RS232_RX_MAX_DATA must be <= 255"
#endif

#ifndef BOOT_COM_RS232_CHANNEL_INDEX
#error "BOOT_COM_RS232_CHANNEL_INDEX is missing in blt_conf.h"
#endif

#if (BOOT_COM_RS232_CHANNEL_INDEX < 0)
#error "BOOT_COM_RS232_CHANNEL_INDEX must be >= 0"
#endif

#ifndef BOOT_COM_RS232_CS_TYPE
#define BOOT_COM_RS232_CS_TYPE   (0)
#endif

#if (BOOT_COM_RS232_CS_TYPE < 0) || (BOOT_COM_RS232_CS_TYPE > 1)
#error "BOOT_COM_RS232_CS_TYPE must be 0 (none) or 1 (byte)"
#endif

#endif /* BOOT_COM_RS232_ENABLE > 0 */

#ifndef BOOT_COM_MBRTU_ENABLE
#define BOOT_COM_MBRTU_ENABLE           (0)
#endif

#if (BOOT_COM_MBRTU_ENABLE > 0)
/* Note that the value check was disabled for the following configuration macros, which
 * makes it possible to link them to a function, allowing a more dynamic configuration:
 * - BOOT_COM_MBRTU_BAUDRATE
 * - BOOT_COM_MBRTU_NODE_ID
 */
#ifndef BOOT_COM_MBRTU_BAUDRATE
#error "BOOT_COM_MBRTU_BAUDRATE is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_MBRTU_STOPBITS
#error "BOOT_COM_MBRTU_STOPBITS is missing in blt_conf.h"
#endif

#if (BOOT_COM_MBRTU_STOPBITS <= 0)
#error "BOOT_COM_MBRTU_STOPBITS must be > 0"
#endif

#if (BOOT_COM_MBRTU_STOPBITS > 2)
#error "BOOT_COM_MBRTU_STOPBITS must be <= 2"
#endif

#ifndef BOOT_COM_MBRTU_PARITY
#error "BOOT_COM_MBRTU_PARITY is missing in blt_conf.h"
#endif

#if (BOOT_COM_MBRTU_PARITY < 0)
#error "BOOT_COM_MBRTU_PARITY must be >= 0"
#endif

#if (BOOT_COM_MBRTU_PARITY > 2)
#error "BOOT_COM_MBRTU_PARITY must be <= 2"
#endif

#ifndef BOOT_COM_MBRTU_TX_MAX_DATA
#error "BOOT_COM_MBRTU_TX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_MBRTU_TX_MAX_DATA <= 0)
#error "BOOT_COM_MBRTU_TX_MAX_DATA must be > 0"
#endif

#if (BOOT_COM_MBRTU_TX_MAX_DATA > 255)
#error "BOOT_COM_MBRTU_TX_MAX_DATA must be <= 255"
#endif

#ifndef BOOT_COM_MBRTU_RX_MAX_DATA
#error "BOOT_COM_MBRTU_RX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_MBRTU_RX_MAX_DATA <= 0)
#error "BOOT_COM_MBRTU_RX_MAX_DATA must be > 0"
#endif

#if (BOOT_COM_MBRTU_RX_MAX_DATA > 255)
#error "BOOT_COM_MBRTU_RX_MAX_DATA must be <= 255"
#endif

#ifndef BOOT_COM_MBRTU_CHANNEL_INDEX
#error "BOOT_COM_MBRTU_CHANNEL_INDEX is missing in blt_conf.h"
#endif

#if (BOOT_COM_MBRTU_CHANNEL_INDEX < 0)
#error "BOOT_COM_MBRTU_CHANNEL_INDEX must be >= 0"
#endif

#ifndef BOOT_COM_MBRTU_NODE_ID
#error "BOOT_COM_MBRTU_NODE_ID is missing in blt_conf.h"
#endif

#endif /* BOOT_COM_MBRTU_ENABLE > 0 */

#ifndef BOOT_COM_USB_ENABLE
#define BOOT_COM_USB_ENABLE             (0)
#endif

#if (BOOT_COM_USB_ENABLE > 0)
#ifndef BOOT_COM_USB_TX_MAX_DATA
#error "BOOT_COM_USB_TX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_USB_TX_MAX_DATA <= 0)
#error "BOOT_COM_USB_TX_MAX_DATA must be > 0"
#endif

#ifndef BOOT_COM_USB_RX_MAX_DATA
#error "BOOT_COM_USB_RX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_USB_RX_MAX_DATA <= 0)
#error "BOOT_COM_USB_RX_MAX_DATA must be > 0"
#endif

#ifdef BOOT_COM_USB_BACKDOOR_EXTENSION_MS
#if (BOOT_COM_USB_BACKDOOR_EXTENSION_MS < 0)
#error "BOOT_COM_USB_BACKDOOR_EXTENSION_MS must be >= 0"
#endif
#endif

#endif /* BOOT_COM_USB_ENABLE > 0 */

#ifndef BOOT_COM_NET_ENABLE
#define BOOT_COM_NET_ENABLE             (0)
#endif

#if (BOOT_COM_NET_ENABLE > 0)
/* Note that the value check was disabled for the following configuration macros, which
 * makes it possible to link them to a function, allowing a more dynamic configuration:
 * - BOOT_COM_NET_IPADDR0
 * - BOOT_COM_NET_IPADDR1
 * - BOOT_COM_NET_IPADDR2
 * - BOOT_COM_NET_IPADDR3
 * - BOOT_COM_NET_NETMASK0
 * - BOOT_COM_NET_NETMASK1
 * - BOOT_COM_NET_NETMASK2
 * - BOOT_COM_NET_NETMASK3
 * - BOOT_COM_NET_GATEWAY0
 * - BOOT_COM_NET_GATEWAY1
 * - BOOT_COM_NET_GATEWAY2
 * - BOOT_COM_NET_GATEWAY3
 * - BOOT_COM_NET_PORT
 */
#ifndef BOOT_COM_NET_TX_MAX_DATA
#error "BOOT_COM_NET_TX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_NET_TX_MAX_DATA <= 0)
#error "BOOT_COM_NET_TX_MAX_DATA must be > 0"
#endif

#ifndef BOOT_COM_NET_RX_MAX_DATA
#error "BOOT_COM_NET_RX_MAX_DATA is missing in blt_conf.h"
#endif

#if (BOOT_COM_NET_RX_MAX_DATA <= 0)
#error "BOOT_COM_NET_RX_MAX_DATA must be > 0"
#endif

#ifndef BOOT_COM_NET_DHCP_ENABLE
#define BOOT_COM_NET_DHCP_ENABLE        (0)
#endif

#if (BOOT_COM_NET_DHCP_ENABLE < 0) || (BOOT_COM_NET_DHCP_ENABLE > 1)
#error "BOOT_COM_NET_DHCP_ENABLE must be 0 or 1"
#endif

#if (BOOT_COM_NET_DHCP_ENABLE == 0)
#ifndef BOOT_COM_NET_IPADDR0
#error "BOOT_COM_NET_IPADDR0 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_IPADDR1
#error "BOOT_COM_NET_IPADDR1 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_IPADDR2
#error "BOOT_COM_NET_IPADDR2 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_IPADDR3
#error "BOOT_COM_NET_IPADDR3 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_NETMASK0
#error "BOOT_COM_NET_NETMASK0 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_NETMASK1
#error "BOOT_COM_NET_NETMASK1 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_NETMASK2
#error "BOOT_COM_NET_NETMASK2 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_NETMASK3
#error "BOOT_COM_NET_NETMASK3 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_GATEWAY0
#error "BOOT_COM_NET_GATEWAY0 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_GATEWAY1
#error "BOOT_COM_NET_GATEWAY1 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_GATEWAY2
#error "BOOT_COM_NET_GATEWAY2 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_GATEWAY3
#error "BOOT_COM_NET_GATEWAY3 is missing in blt_conf.h"
#endif

#ifndef BOOT_COM_NET_PORT
#error "BOOT_COM_NET_PORT is missing in blt_conf.h"
#endif
#endif /* BOOT_COM_NET_DHCP_ENABLE == 0 */

#ifdef BOOT_COM_NET_BACKDOOR_EXTENSION_MS
#if (BOOT_COM_NET_BACKDOOR_EXTENSION_MS < 0)
#error "BOOT_COM_NET_BACKDOOR_EXTENSION_MS must be >= 0"
#endif
#endif

#ifndef BOOT_COM_NET_DEFERRED_INIT_ENABLE
#define BOOT_COM_NET_DEFERRED_INIT_ENABLE   (0)
#endif

#if (BOOT_COM_NET_DEFERRED_INIT_ENABLE < 0) || (BOOT_COM_NET_DEFERRED_INIT_ENABLE > 1)
#error "BOOT_COM_NET_DEFERRED_INIT_ENABLE must be 0 or 1"
#endif

#endif /* BOOT_COM_NET_ENABLE > 0 */

#if (BOOT_COM_NET_DEFERRED_INIT_ENABLE == 1)
#define BOOT_COM_DEFERRED_INIT_ENABLE       (1)
#else
#define BOOT_COM_DEFERRED_INIT_ENABLE       (0)
#endif

#if (BOOT_COM_CAN_ENABLE == 1) || (BOOT_COM_RS232_ENABLE == 1) || (BOOT_COM_MBRTU_ENABLE == 1) || (BOOT_COM_NET_ENABLE == 1) || (BOOT_COM_USB_ENABLE == 1)
#define BOOT_COM_ENABLE   (1)
#else
#define BOOT_COM_ENABLE   (0)
#endif


/****************************************************************************************
*   F I L E   S Y S T E M   I N T E R F A C E   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_FILE_SYS_ENABLE
#define BOOT_FILE_SYS_ENABLE      (0)
#endif

#if (BOOT_FILE_SYS_ENABLE < 0) || (BOOT_FILE_SYS_ENABLE > 1)
#error "BOOT_FILE_SYS_ENABLE must be 0 or 1"
#endif

#if (BOOT_FILE_SYS_ENABLE > 0)
#ifndef BOOT_FILE_LOGGING_ENABLE
#define BOOT_FILE_LOGGING_ENABLE         (0)
#endif

#if (BOOT_FILE_LOGGING_ENABLE < 0) || (BOOT_FILE_LOGGING_ENABLE > 1)
#error "BOOT_FILE_LOGGING_ENABLE must be 0 or 1"
#endif

#ifndef BOOT_FILE_ERROR_HOOK_ENABLE
#define BOOT_FILE_ERROR_HOOK_ENABLE      (0)
#endif

#if (BOOT_FILE_ERROR_HOOK_ENABLE < 0) || (BOOT_FILE_ERROR_HOOK_ENABLE > 1)
#error "BOOT_FILE_ERROR_HOOK_ENABLE must be 0 or 1"
#endif

#ifndef BOOT_FILE_STARTED_HOOK_ENABLE
#define BOOT_FILE_STARTED_HOOK_ENABLE    (0)
#endif

#if (BOOT_FILE_STARTED_HOOK_ENABLE < 0) || (BOOT_FILE_STARTED_HOOK_ENABLE > 1)
#error "BOOT_FILE_STARTED_HOOK_ENABLE must be 0 or 1"
#endif

#ifndef BOOT_FILE_COMPLETED_HOOK_ENABLE
#define BOOT_FILE_COMPLETED_HOOK_ENABLE  (0)
#endif

#if (BOOT_FILE_COMPLETED_HOOK_ENABLE < 0) || (BOOT_FILE_COMPLETED_HOOK_ENABLE > 1)
#error "BOOT_FILE_COMPLETED_HOOK_ENABLE must be 0 or 1"
#endif
#endif /* BOOT_FILE_SYS_ENABLE > 0 */


/****************************************************************************************
*   B A C K D O O R   E N T R Y   C H E C K
****************************************************************************************/
#ifndef BOOT_BACKDOOR_HOOKS_ENABLE
#define BOOT_BACKDOOR_HOOKS_ENABLE      (0)
#endif

#if (BOOT_BACKDOOR_HOOKS_ENABLE < 0) || (BOOT_BACKDOOR_HOOKS_ENABLE > 1)
#error "BOOT_BACKDOOR_HOOKS_ENABLE must be 0 or 1"
#endif


/****************************************************************************************
*   F L A S H   D R I V E R   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_FLASH_CRYPTO_HOOKS_ENABLE
#define BOOT_FLASH_CRYPTO_HOOKS_ENABLE  (0)
#endif

#if (BOOT_FLASH_CRYPTO_HOOKS_ENABLE < 0) || (BOOT_FLASH_CRYPTO_HOOKS_ENABLE > 1)
#error "BOOT_FLASH_CRYPTO_HOOKS_ENABLE must be 0 or 1"
#endif


/****************************************************************************************
*   N V M   D R I V E R   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_NVM_HOOKS_ENABLE
#define BOOT_NVM_HOOKS_ENABLE           (0)
#endif

#if (BOOT_NVM_HOOKS_ENABLE < 0) || (BOOT_NVM_HOOKS_ENABLE > 1)
#error "BOOT_NVM_HOOKS_ENABLE must be 0 or 1"
#endif

#ifndef BOOT_NVM_SIZE_KB
#error "BOOT_NVM_SIZE_KB is missing in blt_conf.h"
#endif

#if (BOOT_NVM_SIZE_KB <= 0)
#error "BOOT_NVM_SIZE_KB must be > 0"
#endif

#ifndef BOOT_NVM_CHECKSUM_HOOKS_ENABLE
#define BOOT_NVM_CHECKSUM_HOOKS_ENABLE  (0)
#endif

#if (BOOT_NVM_CHECKSUM_HOOKS_ENABLE < 0) || (BOOT_NVM_CHECKSUM_HOOKS_ENABLE > 1)
#error "BOOT_NVM_CHECKSUM_HOOKS_ENABLE must be 0 or 1"
#endif


/****************************************************************************************
*   W A T C H D O G   D R I V E R   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_COP_HOOKS_ENABLE
#define BOOT_COP_HOOKS_ENABLE           (0)
#endif

#if (BOOT_COP_HOOKS_ENABLE < 0) || (BOOT_COP_HOOKS_ENABLE > 1)
#error "BOOT_COP_HOOKS_ENABLE must be 0 or 1"
#endif


/****************************************************************************************
*   I N F O   T A B L E   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_INFO_TABLE_ENABLE
#define BOOT_INFO_TABLE_ENABLE          (0)
#endif

#ifndef BOOT_INFO_TABLE_LEN
#define BOOT_INFO_TABLE_LEN             (0)
#endif

#ifndef BOOT_INFO_TABLE_ADDR
#define BOOT_INFO_TABLE_ADDR            (0)
#endif

#if (BOOT_INFO_TABLE_ENABLE < 0) || (BOOT_INFO_TABLE_ENABLE > 1)
#error "BOOT_INFO_TABLE_ENABLE must be 0 or 1"
#endif

#if (BOOT_INFO_TABLE_ENABLE > 0)
#if (BOOT_INFO_TABLE_LEN <= 0)
#error "BOOT_INFO_TABLE_LEN must be > 0"
#endif

#if (BOOT_INFO_TABLE_LEN > 0xFFFF)
#error "BOOT_INFO_TABLE_LEN must be <= 0xFFFF"
#endif

#if (BOOT_INFO_TABLE_ADDR < 0)
#error "BOOT_INFO_TABLE_ADDR must be >= 0"
#endif

#if (BOOT_INFO_TABLE_ADDR > (0xFFFFFFFF - BOOT_INFO_TABLE_LEN))
#error "BOOT_INFO_TABLE_ADDR must be <= (0xFFFFFFFF - BOOT_INFO_TABLE_LEN)"
#endif
#endif /* BOOT_INFO_TABLE_ENABLE > 0 */


/****************************************************************************************
*   X C P   M O D U L E   C O N F I G U R A T I O N   C H E C K
****************************************************************************************/
#ifndef BOOT_XCP_SEED_KEY_ENABLE
#define BOOT_XCP_SEED_KEY_ENABLE        (0)
#endif

#if (BOOT_XCP_SEED_KEY_ENABLE < 0) || (BOOT_XCP_SEED_KEY_ENABLE > 1)
#error "BOOT_XCP_SEED_KEY_ENABLE must be 0 or 1"
#endif

#ifndef BOOT_XCP_UPLOAD_ENABLE
#define BOOT_XCP_UPLOAD_ENABLE          (1)
#endif

#if (BOOT_XCP_UPLOAD_ENABLE < 0) || (BOOT_XCP_UPLOAD_ENABLE > 1)
#error "BOOT_XCP_UPLOAD_ENABLE must be 0 or 1"
#endif

#ifndef BOOT_XCP_PACKET_RECEIVED_HOOK
#define BOOT_XCP_PACKET_RECEIVED_HOOK   (0)
#endif

#if (BOOT_XCP_PACKET_RECEIVED_HOOK < 0) || (BOOT_XCP_PACKET_RECEIVED_HOOK > 1)
#error "BOOT_XCP_PACKET_RECEIVED_HOOK must be 0 or 1"
#endif

#ifndef BOOT_XCP_SEED_MAX_LEN
#define BOOT_XCP_SEED_MAX_LEN           (64)
#endif

#if (BOOT_XCP_SEED_MAX_LEN <= 0)
#error "BOOT_XCP_SEED_MAX_LEN must be > 0"
#endif

#ifndef BOOT_XCP_KEY_MAX_LEN
#define BOOT_XCP_KEY_MAX_LEN            (64)
#endif

#if (BOOT_XCP_KEY_MAX_LEN <= 0)
#error "BOOT_XCP_KEY_MAX_LEN must be > 0"
#endif

#endif /* PLAUSIBILITY_H */
/*********************************** end of plausibility.h *****************************/
