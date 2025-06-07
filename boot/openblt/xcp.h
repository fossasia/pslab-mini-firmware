/************************************************************************************//**
* \file         Source/xcp.h
* \brief        XCP 1.0 protocol core header file.
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
#ifndef XCP_H
#define XCP_H

#if (BOOT_COM_ENABLE > 0)
/****************************************************************************************
* Configuration
****************************************************************************************/
/** \brief Maximum length of the transport layer's command transmit object packet. */
#if defined(BOOT_XCP_CUSTOM_RX_MAX_DATA)
#define XCP_CTO_PACKET_LEN             (BOOT_XCP_CUSTOM_RX_MAX_DATA)
#else
#define XCP_CTO_PACKET_LEN             (ComGetActiveInterfaceMaxRxLen())
#endif

/** \brief Maximum length of the transport layer's data transmit object packet. */
#if defined(BOOT_XCP_CUSTOM_TX_MAX_DATA)
#define XCP_DTO_PACKET_LEN             (BOOT_XCP_CUSTOM_TX_MAX_DATA)
#else
#define XCP_DTO_PACKET_LEN             (ComGetActiveInterfaceMaxTxLen())
#endif

/** \brief Name in string format that is used to identify the ECU to the XCP master
 *         using the GET_ID command.
 */
#define XCP_STATION_ID_STRING          "OpenBLT"

#if (BOOT_CPU_BYTE_ORDER_MOTOROLA > 0)
/** \brief XCP byte ordering according to the Motorola (big-endian). */
#define XCP_MOTOROLA_FORMAT            (0x01)
#else
/** \brief XCP byte ordering according to the Intel (little-endian). */
#define XCP_MOTOROLA_FORMAT            (0x00)
#endif

/** \brief Enable (=1) or disable (=0) support for the calibration resource. This is
 *         required when data is written to RAM during the XCP session.
 */
#define XCP_RES_CALIBRATION_EN         (0)

/** \brief Enable (=1) or disable (=0) support for the paging resource. This is
 *         required when switching between application specific calibration pages
 *         should be supported. In this case the application specific external
 *         functions AppCalSetPage and AppCalGetPage must be provided.
 */
#define XCP_RES_PAGING_EN              (0)

/** \brief Enable (=1) or disable (=0) support for the programming resource. This is
 *         required when non-volatile memory will be erased or programmed during an
 *         XCP session.  In this case the following external hardware specific
 *         functions must be provided: NvmWrite, NvmErase and CpuStartUserProgram.
 */
#define XCP_RES_PROGRAMMING_EN         (1)

/** \brief Enable (=1) or disable (=0) support for the data acquisition resource. This
 *         note that this feature is currently not supported by the XCP driver.
 */
#define XCP_RES_DATA_ACQUISITION_EN    (0)

/** \brief Enable (=1) or disable (=0) support for the data stimulation resource. This
 *         note that this feature is currently not supported by the XCP driver.
 */
#define XCP_RES_DATA_STIMULATION_EN    (0)

/** \brief Enable (=1) or disable (=0) support for the seed/key protection feature.
 *         If enabled, the XCP master has to perform a GET_SEED/UNLOCK sequence to
 *         obtain access to a resource. The protection algorithm is implemented in
 *         XcpGetSeed and XcpVerifyKey.
 */
#if (BOOT_XCP_SEED_KEY_ENABLE > 0)
#define XCP_SEED_KEY_PROTECTION_EN     (1)
#else
#define XCP_SEED_KEY_PROTECTION_EN     (0)
#endif

/** \brief Enable (=1) or disable (=0) uploading. By default, XCP always allows memory
 *         read operations using the commands UPLOAD and SHORT_UPLOAD. This is not
 *         always desired for security reasons. If disabled, memory reads via XCP always
 *         return zero values.
 */
#if (BOOT_XCP_UPLOAD_ENABLE > 0)
#define XCP_UPLOAD_EN                  (1)
#else
#define XCP_UPLOAD_EN                  (0)
#endif

/** \brief Enable (=1) or disable the hook function that gets called each time an XCP
 *         packet was received from the host.
 *  \details A master-slave bootloader can be realized by using this hook-function. The
 *           mode parameter in the XCP Connect command can be interpreted as a node ID.
 *           When trying to connect to a slave, a gateway could be activated that passes
 *           the packet on to the slave. When the response packet is received from the
 *           slave, ComTransmitPacket() can be called to pass the response on to the
 *           host. At the end of a firmware update procedure, the XCP Program Reset
 *           command is called, which can be used to deactivate the gateway. If this
 *           hook-function returns BLT_TRUE, the packet is no longer processed by the XCP
 *           module. If it returns BLT_FALSE, then the packet is processed as usual by
 *           the XCP module.
 */
#if (BOOT_XCP_PACKET_RECEIVED_HOOK > 0)
#define XCP_PACKET_RECEIVED_HOOK_EN    (1)
#else
#define XCP_PACKET_RECEIVED_HOOK_EN    (0)
#endif



/****************************************************************************************
* Defines
****************************************************************************************/
/** \brief XCP protocol layer version number (16-bit). */
#define XCP_VERSION_PROTOCOL_LAYER  (0x0100)

/** \brief XCP transport layer version number (16-bit). */
#define XCP_VERSION_TRANSPORT_LAYER (0x0100)

/* XCP packet identifiers */
/** \brief Command response packet identifier. */
#define XCP_PID_RES                 (0xff)
/** \brief Error packet identifier. */
#define XCP_PID_ERR                 (0xfe)

/* XCP error codes */
/** \brief Cmd processor synchronization error code. */
#define XCP_ERR_CMD_SYNCH           (0x00)
/** \brief Command was not executed error code. */
#define XCP_ERR_CMD_BUSY            (0x10)
/** \brief Unknown or unsupported command error code. */
#define XCP_ERR_CMD_UNKNOWN         (0x20)
/** \brief Parameter out of range error code. */
#define XCP_ERR_OUT_OF_RANGE        (0x22)
/** \brief Protected error code. Seed/key required. */
#define XCP_ERR_ACCESS_LOCKED       (0x25)
/** \brief Cal page not valid error code. */
#define XCP_ERR_PAGE_NOT_VALID      (0x26)
/** \brief Sequence error code. */
#define XCP_ERR_SEQUENCE            (0x29)
/** \brief Generic error code. */
#define XCP_ERR_GENERIC             (0x31)

/* XCP command codes */
/** \brief CONNECT command code. */
#define XCP_CMD_CONNECT             (0xff)
/** \brief DISCONNECT command code. */
#define XCP_CMD_DISCONNECT          (0xfe)
/** \brief GET_STATUS command code. */
#define XCP_CMD_GET_STATUS          (0xfd)
/** \brief SYNCH command code. */
#define XCP_CMD_SYNCH               (0xfc)
/** \brief GET_ID command code. */
#define XCP_CMD_GET_ID              (0xfa)
/** \brief GET_SEED command code. */
#define XCP_CMD_GET_SEED            (0xf8)
/** \brief UNLOCK command code. */
#define XCP_CMD_UNLOCK              (0xf7)
/** \brief SET_MTA command code. */
#define XCP_CMD_SET_MTA             (0xf6)
/** \brief UPLOAD command code. */
#define XCP_CMD_UPLOAD              (0xf5)
/** \brief SHORT_UPLOAD command code. */
#define XCP_CMD_SHORT_UPLOAD        (0xf4)
/** \brief BUILD_CHECKSUM command code. */
#define XCP_CMD_BUILD_CHECKSUM      (0xf3)
/** \brief USER command code. */
#define XCP_CMD_USER                (0xf1)
/** \brief DOWNLOAD command code. */
#define XCP_CMD_DOWNLOAD            (0xf0)
/** \brief DOWNLOAD_MAX command code. */
#define XCP_CMD_DOWLOAD_MAX         (0xee)
/** \brief SET_CALPAGE command code. */
#define XCP_CMD_SET_CAL_PAGE        (0xeb)
/** \brief GET_CALPAGE command code. */
#define XCP_CMD_GET_CAL_PAGE        (0xea)
/** \brief PROGRAM_START command code. */
#define XCP_CMD_PROGRAM_START       (0xd2)
/** \brief PROGRAM_CLEAR command code. */
#define XCP_CMD_PROGRAM_CLEAR       (0xd1)
/** \brief PROGRAM command code. */
#define XCP_CMD_PROGRAM             (0xd0)
/** \brief PROGRAM_RESET command code. */
#define XCP_CMD_PROGRAM_RESET       (0xcf)
/** \brief PROGRAM_PREPARE command code. */
#define XCP_CMD_PROGRAM_PREPARE     (0xcc)
/** \brief PROGRAM_MAX command code. */
#define XCP_CMD_PROGRAM_MAX         (0xc9)

/* XCP USER sub command codes */
/** \brief XCP CMD USER sub command code of the info table feature. */
#define XCP_CMD_USER_SUB_INFOTABLE  (0x17)

/* XCP USER info table sub command related command id's  */
/** \brief XCP USER info table GET_INFO command ID. */
#define XCP_CMD_IT_CID_GETINFO      (0x04)
/** \brief XCP USER info table DOWNLOAD sub command ID. */
#define XCP_CMD_IT_CID_DOWNLOAD     (0x06)
/** \brief XCP USER info table CHECK sub command ID. */
#define XCP_CMD_IT_CID_CHECK        (0x08)

/* xcp supported resources */
/** \brief ProGraMming resource. */
#define XCP_RES_PGM            (0x10)
/** \brief data STIMulation resource. */
#define XCP_RES_STIM           (0x08)
/** \brief Data AcQuisition resource. */
#define XCP_RES_DAQ            (0x04)
/** \brief CALibration and PAGing resource. */
#define XCP_RES_CALPAG         (0x01)

/* xcp checksum types */
/** \brief Add byte into byte checksum. */
#define XCP_CS_ADD11           (0x01)
/** \brief Add byte into word checksum. */
#define XCP_CS_ADD12           (0x02)
/** \brief Add byte into dword checksum. */
#define XCP_CS_ADD14           (0x03)
/** \brief Add word into word checksum. */
#define XCP_CS_ADD22           (0x04)
/** \brief Add word into dword checksum. */
#define XCP_CS_ADD24           (0x05)
/** \brief Add dword into dword checksum. */
#define XCP_CS_ADD44           (0x06)
/** \brief Use 16-bit CRC algorithm. */
#define XCP_CS_CRC16           (0x07)
/** \brief Use 16-bit CRC CITT algorithm. */
#define XCP_CS_CRC16CITT       (0x08)
/** \brief Use 32-bit CRC algorithm. */
#define XCP_CS_CRC32           (0x09)
/** \brief Use user defined algorithm. */
#define XCP_CS_USER            (0xff)

/** \brief Maximum number of bytes of a seed for the seed/key security feature. */
#define XCP_SEED_MAX_LEN       (BOOT_XCP_SEED_MAX_LEN)
/** \brief Maximum number of bytes of a key for the seed/key security feature. */
#define XCP_KEY_MAX_LEN        (BOOT_XCP_KEY_MAX_LEN)


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void     XcpInit(void);
blt_bool XcpIsConnected(void);
void     XcpPacketTransmitted(void);
void     XcpPacketReceived(blt_int8u *data, blt_int8u len);


/****************************************************************************************
* Configuration check
****************************************************************************************/
#if (XCP_RES_DATA_ACQUISITION_EN == 1)
#error "XCP.H, Current XCP driver does not support Data AcQuisition resource."
#endif


#if (XCP_RES_DATA_STIMULATION_EN == 1)
#error "XCP.H, Current XCP driver does not support Data StIMulation resource."
#endif


#ifndef XCP_CTO_PACKET_LEN
#error  "XCP.H, Configuration macro XCP_CTO_PACKET_LEN is missing."
#endif


#ifndef XCP_DTO_PACKET_LEN
#error  "XCP.H, Configuration macro XCP_DTO_PACKET_LEN is missing."
#endif


#ifndef XCP_STATION_ID_STRING
#error  "XCP.H, Configuration macro XCP_STATION_ID_STRING is missing."
#endif


#ifndef XCP_RES_CALIBRATION_EN
#error  "XCP.H, Configuration macro XCP_RES_CALIBRATION_EN is missing."
#endif

#if     (XCP_RES_CALIBRATION_EN < 0) || (XCP_RES_CALIBRATION_EN > 1)
#error  "XCP.H, XCP_RES_CALIBRATION_EN must be 0 or 1."
#endif


#ifndef XCP_RES_PAGING_EN
#error  "XCP.H, Configuration macro XCP_RES_PAGING_EN is missing."
#endif

#if     (XCP_RES_PAGING_EN < 0) || (XCP_RES_PAGING_EN > 1)
#error  "XCP.H, XCP_RES_PAGING_EN must be 0 or 1."
#endif


#ifndef XCP_RES_PROGRAMMING_EN
#error  "XCP.H, Configuration macro XCP_RES_PAGING_EN is missing."
#endif

#if     (XCP_RES_PROGRAMMING_EN < 0) || (XCP_RES_PROGRAMMING_EN > 1)
#error  "XCP.H, XCP_RES_PROGRAMMING_EN must be 0 or 1."
#endif


#ifndef XCP_RES_DATA_ACQUISITION_EN
#error  "XCP.H, Configuration macro XCP_RES_DATA_ACQUISITION_EN is missing."
#endif

#if     (XCP_RES_DATA_ACQUISITION_EN < 0) || (XCP_RES_DATA_ACQUISITION_EN > 1)
#error  "XCP.H, XCP_RES_DATA_ACQUISITION_EN must be 0 or 1."
#endif


#ifndef XCP_RES_DATA_STIMULATION_EN
#error  "XCP.H, Configuration macro XCP_RES_DATA_STIMULATION_EN is missing."
#endif

#if     (XCP_RES_DATA_STIMULATION_EN < 0) || (XCP_RES_DATA_STIMULATION_EN > 1)
#error  "XCP.H, XCP_RES_DATA_STIMULATION_EN must be 0 or 1."
#endif


#ifndef XCP_SEED_KEY_PROTECTION_EN
#error  "XCP.H, Configuration macro XCP_SEED_KEY_PROTECTION_EN is missing."
#endif

#if     (XCP_SEED_KEY_PROTECTION_EN < 0) || (XCP_SEED_KEY_PROTECTION_EN > 1)
#error  "XCP.H, XCP_SEED_KEY_PROTECTION_EN must be 0 or 1."
#endif

#ifndef XCP_UPLOAD_EN
#error  "XCP.H, Configuration macro XCP_UPLOAD_EN is missing."
#endif

#if     (XCP_UPLOAD_EN < 0) || (XCP_UPLOAD_EN > 1)
#error  "XCP.H, XCP_UPLOAD_EN must be 0 or 1."
#endif

#ifndef XCP_PACKET_RECEIVED_HOOK_EN
#error  "XCP.H, Configuration macro XCP_PACKET_RECEIVED_HOOK_EN is missing."
#endif

#if     (XCP_PACKET_RECEIVED_HOOK_EN < 0) || (XCP_PACKET_RECEIVED_HOOK_EN > 1)
#error  "XCP.H, XCP_PACKET_RECEIVED_HOOK_EN must be 0 or 1."
#endif

#endif /* BOOT_COM_ENABLE > 0 */
#endif /* XCP_H */
/******************************** end of xcp.h *~~~~~***********************************/
