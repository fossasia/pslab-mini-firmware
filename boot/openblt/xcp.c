/************************************************************************************//**
* \file         Source/xcp.c
* \brief        XCP 1.0 protocol core source file.
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

/****************************************************************************************
* Include files
****************************************************************************************/
#include "boot.h"                                /* bootloader generic header          */


#if (BOOT_COM_ENABLE > 0)
/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Structure type for grouping XCP internal module information. */
typedef struct
{
  blt_int8u  ctoData[BOOT_COM_RX_MAX_DATA];         /**< cto packet data buffer        */
  blt_int8u  connected;                             /**< connection established        */
  blt_int8u  protection;                            /**< protection state              */
  blt_int8u  s_n_k_resource;                        /**< for seed/key sequence         */
  blt_int8u  ctoPending;                            /**< cto transmission pending flag */
  blt_int16s ctoLen;                                /**< cto current packet length     */
  blt_int32u mta;                                   /**< memory transfer address       */
} tXcpInfo;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
/* transport layer specific functions */
static void      XcpTransmitPacket(blt_int8u *data, blt_int16s len);

/* application specific functions */
static blt_int8u XcpComputeChecksum(blt_int32u address, blt_int32u length,
                                    blt_int32u *checksum);

#if (XCP_SEED_KEY_PROTECTION_EN == 1)
static blt_int8u XcpGetSeed(blt_int8u resource, blt_int8u *seed);
static blt_int8u XcpVerifyKey(blt_int8u resource, blt_int8u *key, blt_int8u len);
#endif

/* general utility functions */
static void       XcpProtectResources(void);
static void       XcpSetCtoError(blt_int8u error);
static blt_int32u XcpGetOrderedLong(blt_int8u const * data);
static void       XcpSetOrderedLong(blt_int32u value, blt_int8u *data);

/* XCP command processors */
static void XcpCmdConnect(blt_int8u *data);
static void XcpCmdDisconnect(blt_int8u *data);
static void XcpCmdGetStatus(blt_int8u *data);
static void XcpCmdSynch(blt_int8u *data);
static void XcpCmdGetId(blt_int8u *data);
static void XcpCmdSetMta(blt_int8u *data);
static void XcpCmdUpload(blt_int8u *data);
static void XcpCmdShortUpload(blt_int8u *data);
static void XcpCmdBuildCheckSum(blt_int8u *data);
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
static void XcpCmdGetSeed(blt_int8u *data);
static void XcpCmdUnlock(blt_int8u *data);
#endif
#if (XCP_RES_CALIBRATION_EN == 1)
static void XcpCmdDownload(blt_int8u *data);
static void XcpCmdDownloadMax(blt_int8u *data);
#endif
#if (XCP_RES_PAGING_EN == 1)
static void XcpCmdSetCalPage(blt_int8u *data);
static void XcpCmdGetCalPage(blt_int8u *data);
#endif
#if (XCP_RES_PROGRAMMING_EN == 1)
static void XcpCmdProgramMax(blt_int8u *data);
static void XcpCmdProgram(blt_int8u *data);
static void XcpCmdProgramStart(blt_int8u *data);
static void XcpCmdProgramClear(blt_int8u *data);
static void XcpCmdProgramReset(blt_int8u *data);
static void XcpCmdProgramPrepare(blt_int8u *data);
#endif
static void XcpCmdUser(blt_int8u *data);

#if (BOOT_INFO_TABLE_ENABLE > 0)
/* XCP USER sub command processors. */
static void XcpCmdUserSubCmdInfoTable(blt_int8u *data);
/* XCP USER info table sub command related command ID processors. */
static void XcpCmdUserSubCmdInfoTableCidGetInfo(blt_int8u *data);
static void XcpCmdUserSubCmdInfoTableCidDownload(blt_int8u *data);
static void XcpCmdUserSubCmdInfoTableCidCheck(blt_int8u *data);
#endif


/****************************************************************************************
* Hook functions
****************************************************************************************/
#if (XCP_PACKET_RECEIVED_HOOK_EN == 1)
extern blt_bool XcpPacketReceivedHook(blt_int8u *data, blt_int8u len);
#endif

#if (XCP_RES_PAGING_EN == 1)
extern blt_int8u XcpCalSetPageHook(blt_int8u segment, blt_int8u page);
extern blt_int8u XcpCalGetPageHook(blt_int8u segment);
#endif

#if (XCP_SEED_KEY_PROTECTION_EN == 1)
extern blt_int8u XcpGetSeedHook(blt_int8u resource, blt_int8u *seed);
extern blt_int8u XcpVerifyKeyHook(blt_int8u resource, blt_int8u *key, blt_int8u len);
#endif


/****************************************************************************************
* External functions
****************************************************************************************/
#if (BOOT_COM_ENABLE == 0)
/* in case no internally supported communication interface is used, a custom
 * communication module can be added. In order to use the XCP protocol in the custom
 * communication module, this hook function needs to be implemented. If the XCP protocol
 * is not needed, then simply remove the xcp.c source from the project.
 */
extern void XcpTransmitPacketHook(blt_int8u *data, blt_int16u len);
#endif


/****************************************************************************************
* Local constants
****************************************************************************************/
/** \brief String buffer with station id. */
static const blt_int8s xcpStationId[] = XCP_STATION_ID_STRING;


/****************************************************************************************
* Local data definitions
****************************************************************************************/
/** \brief Local variable for storing XCP internal module info. */
static tXcpInfo xcpInfo;


/************************************************************************************//**
** \brief     Initializes the XCP driver. Should be called once upon system startup.
** \return    none
**
****************************************************************************************/
void XcpInit(void)
{
  /* reset xcp module info */
  xcpInfo.connected = 0;
  xcpInfo.mta = 0;
  xcpInfo.ctoPending = 0;
  xcpInfo.ctoLen = 0;
  xcpInfo.s_n_k_resource = 0;
  xcpInfo.protection = 0;
} /*** end of XcpInit ***/


/************************************************************************************//**
** \brief     Obtains information about the XCP connection state.
** \return    BLT_TRUE is an XCP connection is established, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool XcpIsConnected(void)
{
  if (xcpInfo.connected == 0)
  {
    return BLT_FALSE;
  }
  return BLT_TRUE;
} /*** end of XcpIsConnected ***/


/************************************************************************************//**
** \brief     Informs the core that a pending packet transmission was completed by
**            the transport layer.
** \return    none
**
****************************************************************************************/
void XcpPacketTransmitted(void)
{
  /* reset packet transmission pending flag */
  xcpInfo.ctoPending = 0;
} /*** end of XcpPacketTransmitted ***/


/************************************************************************************//**
** \brief     Informs the core that a new packet was received by the transport layer.
** \param     data Pointer to byte buffer with packet data.
** \param     len Number of bytes in the packet.
** \return    none
**
****************************************************************************************/
void XcpPacketReceived(blt_int8u *data, blt_int8u len)
{

#if (XCP_PACKET_RECEIVED_HOOK_EN == 0)
  /* suppress compiler warning due to unused parameter. */
  (void)len;
#else
  /* give the hook function a chance to process this packet. A return value of BLT_TRUE
   * indicates that the hook function processed the packet and that no further processing
   * is required.
   */
  if (XcpPacketReceivedHook(data, len) == BLT_TRUE)
  {
    /* packet processed by hook function so no need to continue. */
    return;
  }
#endif
  /* was this a connect command? */
  if (data[0] == XCP_CMD_CONNECT)
  {
    /* process the connect command */
    XcpCmdConnect(data);
  }
  /* only continue if connected */
  else if (xcpInfo.connected == 1)
  {
    switch (data[0])
    {
      case XCP_CMD_UPLOAD:
        XcpCmdUpload(data);
        break;
      case XCP_CMD_SHORT_UPLOAD:
        XcpCmdShortUpload(data);
        break;
      case XCP_CMD_SET_MTA:
        XcpCmdSetMta(data);
        break;
      case XCP_CMD_BUILD_CHECKSUM:
        XcpCmdBuildCheckSum(data);
        break;
      case XCP_CMD_GET_ID:
        XcpCmdGetId(data);
        break;
      case XCP_CMD_SYNCH:
        XcpCmdSynch(data);
        break;
      case XCP_CMD_GET_STATUS:
        XcpCmdGetStatus(data);
        break;
      case XCP_CMD_DISCONNECT:
        XcpCmdDisconnect(data);
        break;
#if (XCP_RES_CALIBRATION_EN == 1)
      case XCP_CMD_DOWNLOAD:
        XcpCmdDownload(data);
        break;
      case XCP_CMD_DOWLOAD_MAX:
        XcpCmdDownloadMax(data);
        break;
#endif
#if (XCP_RES_PROGRAMMING_EN == 1)
      case XCP_CMD_PROGRAM_MAX:
        XcpCmdProgramMax(data);
        break;
      case XCP_CMD_PROGRAM:
        XcpCmdProgram(data);
        break;
      case XCP_CMD_PROGRAM_START:
        XcpCmdProgramStart(data);
        break;
      case XCP_CMD_PROGRAM_CLEAR:
        XcpCmdProgramClear(data);
        break;
      case XCP_CMD_PROGRAM_RESET:
        XcpCmdProgramReset(data);
        break;
      case XCP_CMD_PROGRAM_PREPARE:
        XcpCmdProgramPrepare(data);
        break;
#endif
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
      case XCP_CMD_GET_SEED:
        XcpCmdGetSeed(data);
        break;
      case XCP_CMD_UNLOCK:
        XcpCmdUnlock(data);
        break;
#endif
#if (XCP_RES_PAGING_EN == 1)
      case XCP_CMD_SET_CAL_PAGE:
        XcpCmdSetCalPage(data);
        break;
      case XCP_CMD_GET_CAL_PAGE:
        XcpCmdGetCalPage(data);
        break;
#endif
      case XCP_CMD_USER:
        XcpCmdUser(data);
        break;
      default:
        XcpSetCtoError(XCP_ERR_CMD_UNKNOWN);
        break;
    }
  }
  else
  {
    /* return to make sure response packet is not send because we are not connected */
    return;
  }

  /* make sure the previous command was completed */
  if (xcpInfo.ctoPending == 1)
  {
    /* command overrun occurred */
    XcpSetCtoError(XCP_ERR_CMD_BUSY);
  }

  /* send the response if it contains something */
  if (xcpInfo.ctoLen > 0)
  {
    /* set cto packet transmission pending flag */
    xcpInfo.ctoPending = 1;

    /* transmit the cto response packet */
    XcpTransmitPacket(xcpInfo.ctoData, xcpInfo.ctoLen);
  }
} /*** end of XcpPacketReceived ***/


/************************************************************************************//**
** \brief     Transmits the packet using the xcp transport layer.
** \param     data Pointer to the byte buffer with packet data.
** \param     len  Number of data bytes that need to be transmitted.
** \return    none
**
****************************************************************************************/
static void XcpTransmitPacket(blt_int8u *data, blt_int16s len)
{
  /* submit packet to the communication interface for transmission */
#if (BOOT_COM_ENABLE == 0)
  XcpTransmitPacketHook(data, len);
#else
  ComTransmitPacket(data, len);
#endif

} /*** end of XcpTransmitPacket ***/


/************************************************************************************//**
** \brief     Called by the BUILD_CHECKSUM command to perform a checksum calculation
**            over the specified memory region.
** \param     address   The start address of the memory region.
** \param     length    Length of the memory region in bytes.
** \param     checksum  Pointer to where the calculated checksum is to be stored.
** \return    Checksum type that was used during the checksum calculation.
**
****************************************************************************************/
static blt_int8u XcpComputeChecksum(blt_int32u address, blt_int32u length,
                                    blt_int32u *checksum)
{
  blt_int8u cs = 0;

  /* this example computes the checksum using the add byte to byte algorithm */
  while (length-- > 0)
  {
    /* add the next byte value */
    cs += *((blt_int8u *)(blt_addr)address);
    /* increment address */
    address++;
    /* service the watchdog */
    CopService();
  }
  /* store the computed checksum value */
  *checksum = cs;
  /* inform the caller of the uses checksum computation algorithm */
  return XCP_CS_ADD11;
} /*** end of XcpComputeChecksum ***/


#if (XCP_SEED_KEY_PROTECTION_EN == 1)
/************************************************************************************//**
** \brief     Provides a seed to the XCP master that will be used for the key
**            generation when the master attempts to unlock the specified resource.
**            Called by the GET_SEED command.
** \param     resource  Resource that the seed if requested for (XCP_RES_XXX).
** \param     seed      Pointer to byte buffer wher the seed will be stored.
** \return    Length of the seed in bytes.
**
****************************************************************************************/
static blt_int8u XcpGetSeed(blt_int8u resource, blt_int8u *seed)
{
  /* pass request on to the application through a hook function */
  return XcpGetSeedHook(resource, seed);
} /*** end of XcpGetSeed ***/


/************************************************************************************//**
** \brief     Called by the UNLOCK command and checks if the key to unlock the
**            specified resource was correct. If so, then the resource protection
**            will be removed.
** \param     resource  resource to unlock (XCP_RES_XXX).
** \param     key       pointer to the byte buffer holding the key.
** \param     len       length of the key in bytes.
** \return    1 if the key was correct, 0 otherwise.
**
****************************************************************************************/
static blt_int8u XcpVerifyKey(blt_int8u resource, blt_int8u *key, blt_int8u len)
{
  /* pass request on to the application through a hook function */
  return XcpVerifyKeyHook(resource, key, len);
} /*** end of XcpVerifyKey ***/
#endif /* XCP_SEED_KEY_PROTECTION_EN == 1 */


/************************************************************************************//**
** \brief     Utility function to protects all the available resources.
** \return    none
**
****************************************************************************************/
static void XcpProtectResources(void)
{
  xcpInfo.protection = 0;

#if (XCP_SEED_KEY_PROTECTION_EN == 1)
#if (XCP_RES_CALIBRATION_EN == 1)
  xcpInfo.protection |= XCP_RES_CALPAG;
#endif

#if (XCP_RES_PAGING_EN == 1)
  xcpInfo.protection |= XCP_RES_CALPAG;
#endif

#if (XCP_RES_PROGRAMMING_EN == 1)
  xcpInfo.protection |= XCP_RES_PGM;
#endif

#if (XCP_RES_DATA_ACQUISITION_EN == 1)
  xcpInfo.protection |= XCP_RES_DAQ;
#endif

#if (XCP_RES_DATA_STIMULATION_EN == 1)
  xcpInfo.protection |= XCP_RES_STIM;
#endif
#endif /* XCP_SEED_KEY_PROTECTION_EN == 1 */
} /*** end of XcpProtectResources ***/


/************************************************************************************//**
** \brief     Prepares the cto packet data for the specified error.
** \param     error XCP error code (XCP_ERR_XXX).
** \return    none
**
****************************************************************************************/
static void XcpSetCtoError(blt_int8u error)
{
  /* prepare the error packet */
  xcpInfo.ctoData[0] = XCP_PID_ERR;
  xcpInfo.ctoData[1] = error;
  xcpInfo.ctoLen = 2;
} /*** end of XcpSetCtoError ***/


/************************************************************************************//**
** \brief     Obtains a 32-bit value from a byte buffer taking into account Intel
**            or Motorola byte ordering.
** \param     data Array to the buffer with the 32-bit value stored as bytes.
** \return    The 32-bit value.
**
****************************************************************************************/
static blt_int32u XcpGetOrderedLong(blt_int8u const * data)
{
  blt_int32u result = 0;

#if (BOOT_CPU_BYTE_ORDER_MOTOROLA	== 0)
  result |= (blt_int32u)data[0];
  result |= (blt_int32u)((blt_int32u)data[1] << 8);
  result |= (blt_int32u)((blt_int32u)data[2] << 16);
  result |= (blt_int32u)((blt_int32u)data[3] << 24);
#else	
  result |= (blt_int32u)data[3];
  result |= (blt_int32u)((blt_int32u)data[2] << 8);
  result |= (blt_int32u)((blt_int32u)data[1] << 16);
  result |= (blt_int32u)((blt_int32u)data[0] << 24);
#endif
  /* Give the result back to the caller. */
  return result;
} /*** end of XcpGetOrderedLong ***/


/************************************************************************************//**
** \brief     Stores a 32-bit value into a byte buffer taking into account Intel
**            or Motorola byte ordering.
** \param     value The 32-bit value to store in the buffer.
** \param     data Array to the buffer for storage.
**
****************************************************************************************/
static void XcpSetOrderedLong(blt_int32u value, blt_int8u *data)
{
#if (BOOT_CPU_BYTE_ORDER_MOTOROLA	== 0)
  data[0] = (blt_int8u) value;
  data[1] = (blt_int8u)(value >>  8);
  data[2] = (blt_int8u)(value >> 16);
  data[3] = (blt_int8u)(value >> 24);
#else
  data[3] = (blt_int8u) value;
  data[2] = (blt_int8u)(value >>  8);
  data[1] = (blt_int8u)(value >> 16);
  data[0] = (blt_int8u)(value >> 24);
#endif
} /*** end of XcpSetOrderedLong ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the CONNECT command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdConnect(blt_int8u *data)
{
  /* suppress compiler warning for unused parameter */
  data = data;

#if (BOOT_FILE_SYS_ENABLE > 0)
  /* reject the connection if the file module is not idle. this means that a firmware
   * update from the locally attached storage is in progress
   */
  if (FileIsIdle() == BLT_FALSE)
  {
    /* command not processed because we are busy */
    XcpSetCtoError(XCP_ERR_CMD_BUSY);
    return;
  }
#endif

  /* enable resource protection */
  XcpProtectResources();

  /* indicate that the connection is established */
  xcpInfo.connected = 1;

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* report available resources */
  xcpInfo.ctoData[1] = 0;
#if (XCP_RES_CALIBRATION_EN == 1)
  xcpInfo.ctoData[1] |= XCP_RES_CALPAG;
#endif

#if (XCP_RES_PAGING_EN == 1)
  xcpInfo.ctoData[1] |= XCP_RES_CALPAG;
#endif

#if (XCP_RES_PROGRAMMING_EN == 1)
  xcpInfo.ctoData[1] |= XCP_RES_PGM;
#endif

#if (XCP_RES_DATA_ACQUISITION_EN == 1)
  xcpInfo.ctoData[1] |= XCP_RES_DAQ;
#endif

#if (XCP_RES_DATA_STIMULATION_EN == 1)
  xcpInfo.ctoData[1] |= XCP_RES_STIM;
#endif

  /* report communication mode info. only byte granularity is supported */
  xcpInfo.ctoData[2] = 0;
  /* configure for motorola or intel byte ordering */
  xcpInfo.ctoData[2] |= XCP_MOTOROLA_FORMAT;

  /* report max cto data length */
  xcpInfo.ctoData[3] = (blt_int8u)XCP_CTO_PACKET_LEN;

  /* report max dto data length */
#if (XCP_MOTOROLA_FORMAT == 0)
  xcpInfo.ctoData[4] = (blt_int8u)XCP_DTO_PACKET_LEN;
  xcpInfo.ctoData[5] = (blt_int8u)(XCP_DTO_PACKET_LEN >> 8);
#else
  xcpInfo.ctoData[4] = (blt_int8u)(XCP_DTO_PACKET_LEN >> 8);
  xcpInfo.ctoData[5] = (blt_int8u)XCP_DTO_PACKET_LEN;
#endif

  /* report msb of protocol layer version number */
  xcpInfo.ctoData[6] = XCP_VERSION_PROTOCOL_LAYER >> 8;

  /* report msb of transport layer version number */
  xcpInfo.ctoData[7] = XCP_VERSION_TRANSPORT_LAYER >> 8;

  /* set packet length */
  xcpInfo.ctoLen = 8;

  /* reinit the NVM driver because a new firmware update is about the start */
  NvmInit();
} /*** end of XcpCmdConnect ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the DISCONNECT command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdDisconnect(blt_int8u *data)
{
  /* suppress compiler warning for unused parameter */
  data = data;

  /* indicate that the xcp connection is disconnected */
  xcpInfo.connected = 0;

  /* enable resource protection */
  XcpProtectResources();

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdDisconnect ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the GET_STATUS command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdGetStatus(blt_int8u *data)
{
  /* suppress compiler warning for unused parameter */
  data = data;

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* report session status */
  xcpInfo.ctoData[1] = 0;

  /* report current resource protection status */
  xcpInfo.ctoData[2] = xcpInfo.protection;

  /* reset reserved and session configuration id values */
  xcpInfo.ctoData[3] = 0;
  xcpInfo.ctoData[4] = 0;
  xcpInfo.ctoData[5] = 0;

  /* set packet length */
  xcpInfo.ctoLen = 6;
} /*** end of XcpCmdGetStatus ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the SYNCH command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdSynch(blt_int8u *data)
{
  /* suppress compiler warning for unused parameter */
  data = data;

  /* synch requires a negative response */
  XcpSetCtoError(XCP_ERR_CMD_SYNCH);
} /*** end of XcpCmdSynch ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the GET_ID command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdGetId(blt_int8u *data)
{
  blt_int32u stationIdLen;
  
  /* suppress compiler warning for unused parameter */
  data = data;

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* point mta to start of station id string */
  xcpInfo.mta = (blt_int32u)&xcpStationId[0];

  /* set station id mode to 0 */
  xcpInfo.ctoData[1] = 0;

  /* reset reserved values */
  xcpInfo.ctoData[2] = 0;
  xcpInfo.ctoData[3] = 0;

  /* store station id length (excl. null termination) for response packet */
  stationIdLen = (sizeof(xcpStationId)/sizeof(xcpStationId[0])) - 1;
  XcpSetOrderedLong(stationIdLen, &xcpInfo.ctoData[4]);

  /* set packet length */
  xcpInfo.ctoLen = 8;
} /*** end of XcpCmdGetId ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the SET_MTA command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdSetMta(blt_int8u *data)
{
  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* update mta. current implementation ignores address extension */
  xcpInfo.mta = XcpGetOrderedLong(&data[4]);

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdSetMta ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the UPLOAD command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUpload(blt_int8u *data)
{
  blt_int16u len;
  blt_int8u *destPtr;

  /* validate length of upload request */
  if (data[1] > (XCP_CTO_PACKET_LEN-1))
  {
    /* requested data length is too long */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    return;
  }

  /* read out the length of the requested upload operation */
  len = data[1];
  /* set the destination pointer */
  destPtr = (blt_int8u *)((blt_addr)(blt_int32u)&xcpInfo.ctoData[1]);

#if (XCP_UPLOAD_EN == 1)
  /* according to the XCP specification memory read/upload functionality is always
   * available. This behavior is unwanted in the case of a bootloader that has the
   * seed/key security feature enabled. The default XCP behavior is deviated in this
   * situation. The deviation is such that all zero values are returned if the seed/key
   * security feature is enabled and the programming resource is not yet unlocked.
   */
  #if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked so memory read access is denied. zero values are returned
     * instead.
     */
    CpuMemSet((blt_addr)destPtr, 0, len);
  }
  else
  {
    /* copy the data from memory to the data packet */
    CpuMemCopy((blt_addr)destPtr,(blt_addr)xcpInfo.mta, len);
  }
  #else
  /* copy the data from memory to the data packet */
  CpuMemCopy((blt_addr)destPtr,(blt_addr)xcpInfo.mta, len);
  #endif /* XCP_SEED_KEY_PROTECTION_EN == 1 */
#else
  /* uploads are disabled, so return zero values for memory read operations */
  CpuMemSet((blt_addr)destPtr, 0, len);
#endif /* XCP_UPLOAD_EN == 1 */

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* post increment the mta */
  xcpInfo.mta += data[1];

  /* set packet length */
  xcpInfo.ctoLen = data[1]+1;
} /*** end of XcpCmdUpload ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the SHORT_UPLOAD command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdShortUpload(blt_int8u *data)
{
  blt_int16u len;
  blt_int8u *destPtr;

  /* validate length of upload request */
  if (data[1] > (XCP_CTO_PACKET_LEN-1))
  {
    /* requested data length is too long */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    return;
  }

  /* update mta. current implementation ignores address extension */
  xcpInfo.mta = XcpGetOrderedLong(&data[4]);
  /* read out the length of the requested upload operation */
  len = data[1];
  /* set the destination pointer */
  destPtr = (blt_int8u *)((blt_addr)(blt_int32u)&xcpInfo.ctoData[1]);

#if (XCP_UPLOAD_EN == 1)
  /* according to the XCP specification memory read/upload functionality is always
   * available. This behavior is unwanted in the case of a bootloader that has the
   * seed/key security feature enabled. The default XCP behavior is deviated in this
   * situation. The deviation is such that all zero values are returned if the seed/key
   * security feature is enabled and the programming resource is not yet unlocked.
   */
  #if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked so memory read access is denied. zero values are returned
     * instead.
     */
    CpuMemSet((blt_addr)destPtr, 0, len);
  }
  else
  {
    /* copy the data from memory to the data packet */
    CpuMemCopy((blt_addr)destPtr,(blt_addr)xcpInfo.mta, len);
  }
  #else
  /* copy the data from memory to the data packet */
  CpuMemCopy((blt_addr)destPtr,(blt_addr)xcpInfo.mta, len);
  #endif /* XCP_SEED_KEY_PROTECTION_EN == 1 */
#else
  /* uploads are disabled, so return zero values for memory read operations */
  CpuMemSet((blt_addr)destPtr, 0, len);
#endif /* XCP_UPLOAD_EN == 1 */

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* post increment the mta */
  xcpInfo.mta += data[1];

  /* set packet length */
  xcpInfo.ctoLen = data[1]+1;
} /*** end of XcpCmdShortUpload ***/


#if (XCP_RES_CALIBRATION_EN == 1)
/************************************************************************************//**
** \brief     XCP command processor function which handles the DOWNLOAD command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdDownload(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if CAL_PAG resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_CALPAG) != 0)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* validate length of download request */
  if (data[1] > (XCP_CTO_PACKET_LEN-2))
  {
    /* requested data length is too long */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    return;
  }

  /* copy the data from the data packet to memory */
  CpuMemCopy((blt_addr)xcpInfo.mta, (blt_addr)((blt_int32u)&data[2]), data[1]);
  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* post increment the mta */
  xcpInfo.mta += data[1];

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdDownload ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the DOWNLOAD_MAX command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdDownloadMax(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if CAL_PAG resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_CALPAG) != 0)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* copy the data from the data packet to memory */
  CpuMemCopy((blt_addr)xcpInfo.mta, (blt_addr)((blt_int32u)&data[1]), \
             XCP_CTO_PACKET_LEN-1);

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* post increment the mta */
  xcpInfo.mta += XCP_CTO_PACKET_LEN-1;

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdDownloadMax ***/
#endif /* XCP_RES_CALIBRATION_EN == 1 */


/************************************************************************************//**
** \brief     XCP command processor function which handles the BUILD_CHECKSUM command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdBuildCheckSum(blt_int8u *data)
{
  blt_int32u checksumLen;
  blt_int32u checksumVal = 0;
  blt_int8u  checksumType;
  
  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* obtain checksum and checksum type */
  checksumLen = XcpGetOrderedLong(&data[4]);
  checksumType = XcpComputeChecksum(xcpInfo.mta, checksumLen, &checksumVal);
  xcpInfo.ctoData[1] = checksumType;
  XcpSetOrderedLong(checksumVal, &xcpInfo.ctoData[4]);

  /* initialize reserved parameters */
  xcpInfo.ctoData[2] = 0;
  xcpInfo.ctoData[3] = 0;

  /* set packet length */
  xcpInfo.ctoLen = 8;
} /*** end of XcpCmdBuildCheckSum ***/


#if (XCP_SEED_KEY_PROTECTION_EN == 1)
/************************************************************************************//**
** \brief     XCP command processor function which handles the GET_SEED command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdGetSeed(blt_int8u *data)
{
  blt_int8u resourceOK;
  /* made seed buffer static to lower stack load */
  static blt_int8u seedBuffer[XCP_SEED_MAX_LEN];
  static blt_int8u seedRemainderLen = 0;
  static blt_int8u *seedCurrentPtr;
  static blt_bool sequenceInProgress = BLT_FALSE;
  blt_int8u seedCurrentLen;

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* validate requested resource in case the mode flag equals 0 */
  if (data[1] == 0)
  {
    /* init resource check variable as if an illegal resource is requested */
    resourceOK = 0;

    /* check if calibration/paging resource is requested for seed/key and make
     * sure this is the only requested resource
     */
    if (((data[2] & XCP_RES_CALPAG) > 0) && ((data[2] & ~XCP_RES_CALPAG) == 0))
    {
      resourceOK = 1;
    }

    /* check if programming resource is requested for seed/key and make
     * sure this is the only requested resource
     */
    if (((data[2] & XCP_RES_PGM) > 0) && ((data[2] & ~XCP_RES_PGM) == 0))
    {
      resourceOK = 1;
    }

    /* check if data acquisition resource is requested for seed/key and make
     * sure this is the only requested resource
     */
    if (((data[2] & XCP_RES_DAQ) > 0) && ((data[2] & ~XCP_RES_DAQ) == 0))
    {
      resourceOK = 1;
    }

    /* check if data stimulation resource is requested for seed/key and make
     * sure this is the only requested resource
     */
    if (((data[2] & XCP_RES_STIM) > 0) && ((data[2] & ~XCP_RES_STIM) == 0))
    {
      resourceOK = 1;
    }

    /* now process the resource validation */
    if (resourceOK == 0)
    {
      XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
      return;
    }

    /* check if the resource is already unlocked */
    if ((xcpInfo.protection & data[2]) == 0)
    {
      /* set the seed length to 0 to indicate that the resource is already unlocked */
      xcpInfo.ctoData[1] = 0;
      /* set packet length */
      xcpInfo.ctoLen = 2;
      /* no need to continue processing */
      return;
    }

    /* store resource for which the seed/key sequence is started */
    xcpInfo.s_n_k_resource = data[2];
  }

  /* process the mode flag. 0 is first part of the seed, 1 is remainder of the seed */
  if (data[1] == 0)
  {
    /* set flag that a seed reading sequence is now in progress */
    sequenceInProgress = BLT_TRUE;
    /* obtain the seed and store it in the buffer */
    seedRemainderLen = XcpGetSeed(xcpInfo.s_n_k_resource, seedBuffer);
    /* protect against buffer overrun */
    ASSERT_RT(seedRemainderLen <= XCP_SEED_MAX_LEN);
    /* set seed pointer */
    seedCurrentPtr = &seedBuffer[0];
  }
  /* seed remainder is requested */
  else
  {
    /* this is only allowed if a sequence is in progress */
    if (sequenceInProgress == BLT_FALSE)
    {
      /* invalid sequence */
      XcpSetCtoError(XCP_ERR_SEQUENCE);
      /* reset seed/key resource variable for possible next unlock */
      xcpInfo.s_n_k_resource = 0;
      return;
    }
  }
  /* determine number of seed bytes that fit in the first response */
  seedCurrentLen = seedRemainderLen;
  if (seedCurrentLen > (XCP_DTO_PACKET_LEN-2))
  {
    seedCurrentLen = XCP_DTO_PACKET_LEN-2;
  }
  /* store the first part of the seed in the response */
  CpuMemCopy((blt_addr)(&xcpInfo.ctoData[2]), (blt_addr)seedCurrentPtr, seedCurrentLen);
  xcpInfo.ctoData[1] = seedRemainderLen;
  /* update control variables */
  seedRemainderLen -= seedCurrentLen;
  seedCurrentPtr += seedCurrentLen;
  /* reset sequence flag at the end of the sequence */
  if (seedRemainderLen == 0)
  {
    sequenceInProgress = BLT_FALSE;
  }
  /* set packet length */
  xcpInfo.ctoLen = seedCurrentLen + 2;
} /*** end of XcpCmdGetSeed ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the UNLOCK command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUnlock(blt_int8u *data)
{
  /* made key buffer static to lower stack load */
  static blt_int8u keyBuffer[XCP_KEY_MAX_LEN];
  static blt_int8u keyPreviousRemainder = 0;
  static blt_int8u keyTotalLen = 0;
  static blt_int8u *keyCurrentPtr;
  static blt_int8u keyReceivedLen = 0;
  blt_int8u keyCurrentLen;

  /* verify that the key will actually fit in the buffer */
  if (data[1] > XCP_KEY_MAX_LEN)
  {
    /* reset previous remainder for the next loop iteration */
    keyPreviousRemainder = 0;
    /* key is too long */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    /* reset seed/key resource variable for possible next unlock */
    xcpInfo.s_n_k_resource = 0;
    return;
  }

  /* is this the start of a key reception? the first unlock message contains the total
   * length of the key and subsequent messages the remainder length. if the received
   * length is >= than the previously received remainder, it must be the reception
   * start of a new key.
   */
  if (data[1] >= keyPreviousRemainder)
  {
    /* store the total length of the key */
    keyTotalLen = data[1];
    /* initialize pointer to key reception buffer */
    keyCurrentPtr = &keyBuffer[0];
    /* reset number of received key bytes */
    keyReceivedLen = 0;

  }
  /* store length / remainder for checking during the next iteration */
  keyPreviousRemainder = data[1];
  /* determine how many key bytes were received */
  keyCurrentLen = data[1];
  if (keyCurrentLen > (XCP_CTO_PACKET_LEN-2))
  {
    keyCurrentLen = XCP_CTO_PACKET_LEN-2;
  }
  /* store the received key bytes to the buffer */
  CpuMemCopy((blt_addr)keyCurrentPtr, (blt_addr)(&data[2]), keyCurrentLen);
  /* update control variables */
  keyCurrentPtr += keyCurrentLen;
  keyReceivedLen += keyCurrentLen;
  /* check if the entire key was received */
  if (keyReceivedLen >= keyTotalLen)
  {
    /* reset previous remainder for the next loop iteration */
    keyPreviousRemainder = 0;
    /* verify the key */
    if (XcpVerifyKey(xcpInfo.s_n_k_resource, keyBuffer, keyTotalLen) == 0)
    {
      /* invalid key so inform the master and do a disconnect */
      XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
      /* indicate that the xcp connection is disconnected */
      xcpInfo.connected = 0;
      /* reset seed/key resource variable for possible next unlock */
      xcpInfo.s_n_k_resource = 0;
      /* enable resource protection */
      XcpProtectResources();
      return;
    }
    /* key correct so unlock the resource */
    xcpInfo.protection &= ~xcpInfo.s_n_k_resource;
    /* reset seed/key resource variable for possible next unlock */
    xcpInfo.s_n_k_resource = 0;
  }

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;
  /* report the current resource protection */
  xcpInfo.ctoData[1] = xcpInfo.protection;
  /* set packet length */
  xcpInfo.ctoLen = 2;
} /*** end of XcpCmdUnlock ***/
#endif /* XCP_SEED_KEY_PROTECTION_EN == 1 */


#if (XCP_RES_PAGING_EN == 1)
/************************************************************************************//**
** \brief     XCP command processor function which handles the SET_CAL_PAGE command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdSetCalPage(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if CAL_PAG resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_CALPAG) == XCP_RES_CALPAG)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* select the page. note that the mode parameter is ignored */
  if (XcpCalSetPageHook(data[2], data[3]) == 0)
  {
    /* calibration page could not be selected */
    XcpSetCtoError(XCP_ERR_PAGE_NOT_VALID);
    return;
  }

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdSetCalPage ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the GET_CAL_PAGE command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdGetCalPage(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if CAL_PAG resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_CALPAG) == XCP_RES_CALPAG)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* initialize reserved parameters */
  xcpInfo.ctoData[1] = 0;
  xcpInfo.ctoData[2] = 0;

  /* store the calibration page */
  xcpInfo.ctoData[3] = XcpCalGetPageHook(data[2]);

  /* set packet length */
  xcpInfo.ctoLen = 4;
} /*** end of XcpCmdGetCalPage ***/
#endif /* XCP_RES_PAGING_EN == 1 */


#if (XCP_RES_PROGRAMMING_EN == 1)
/************************************************************************************//**
** \brief     XCP command processor function which handles the PROGRAM_START command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdProgramStart(blt_int8u *data)
{
  /* suppress compiler warning for unused parameter */
  data = data;

#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* initialize reserved parameter */
  xcpInfo.ctoData[1] = 0;

  /* no special communication mode supported during programming */
  xcpInfo.ctoData[2] = 0;

  /* cto packet length stays the same during programming */
  xcpInfo.ctoData[3] = (blt_int8u)XCP_CTO_PACKET_LEN;

  /* no block size, st-min time, or queue size supported */
  xcpInfo.ctoData[4] = 0;
  xcpInfo.ctoData[5] = 0;
  xcpInfo.ctoData[6] = 0;

  /* set packet length */
  xcpInfo.ctoLen = 7;
} /*** end of XcpCmdProgramStart ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the PROGRAM_MAX command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdProgramMax(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* program the data */
  if (NvmWrite((blt_addr)xcpInfo.mta, XCP_CTO_PACKET_LEN-1, &data[1]) == BLT_FALSE)
  {
    /* error occurred during programming */
    XcpSetCtoError(XCP_ERR_GENERIC);
    return;
  }

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* post increment the mta */
  xcpInfo.mta += XCP_CTO_PACKET_LEN-1;

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdProgramMax ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the PROGRAM command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdProgram(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* validate length of download request */
  if (data[1] > (XCP_CTO_PACKET_LEN-2))
  {
    /* requested data length is too long */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    return;
  }

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* set packet length */
  xcpInfo.ctoLen = 1;

  /* end of programming sequence (datasize is 0)? */
  if (data[1] == 0)
  {
    /* call erase/programming cleanup routine */
    if (NvmDone() == BLT_FALSE)
    {
      /* error occurred while finishing up programming */
      XcpSetCtoError(XCP_ERR_GENERIC);
    }
    return;
  }
  /* program the data */
  if (NvmWrite((blt_addr)xcpInfo.mta, data[1], &data[2]) == BLT_FALSE)
  {
    /* error occurred during programming */
    XcpSetCtoError(XCP_ERR_GENERIC);
    return;
  }

  /* post increment the mta */
  xcpInfo.mta += data[1];
} /*** end of XcpCmdProgram ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the PROGRAM_CLEAR command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdProgramClear(blt_int8u *data)
{
  blt_int32u eraseLen;
  blt_addr   eraseAddr;
  
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* erase the memory */
  eraseAddr = xcpInfo.mta;
  eraseLen = XcpGetOrderedLong(&data[4]);
  if (NvmErase(eraseAddr, eraseLen) == BLT_FALSE)
  {
    /* error occurred during erasure */
    XcpSetCtoError(XCP_ERR_GENERIC);
    return;
  }

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdProgramClear ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the PROGRAM_RESET command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdProgramReset(blt_int8u *data)
{
  /* suppress compiler warning for unused parameter */
  data = data;

#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* reset the ecu after programming is done. so basically, just start the newly programmed
   * firmware. it is okay if the code does not return here. 
   */
  CpuStartUserProgram();

  /* set packet id to command response packet */
  xcpInfo.ctoData[0] = XCP_PID_RES;

  /* set packet length */
  xcpInfo.ctoLen = 1;
} /*** end of XcpCmdProgramReset ***/


/************************************************************************************//**
** \brief     XCP command processor function which handles the PROGRAM_PREPARE command as
**            defined by the protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdProgramPrepare(blt_int8u *data)
{
#if (XCP_SEED_KEY_PROTECTION_EN == 1)
  /* check if PGM resource is unlocked */
  if ((xcpInfo.protection & XCP_RES_PGM) == XCP_RES_PGM)
  {
    /* resource is locked. use seed/key sequence to unlock */
    XcpSetCtoError(XCP_ERR_ACCESS_LOCKED);
    return;
  }
#endif

  /* programming with kernel currently not needed and therefore not supported */
  XcpSetCtoError(XCP_ERR_GENERIC);
  return;
} /*** end of XcpCmdProgramPrepare ***/
#endif /* XCP_RES_PROGRAMMING_EN == 1 */


/************************************************************************************//**
** \brief     XCP command processor function which handles the USER command as
**            defined by the protocol. Note that this is a command that allows user
**            specific extensions to the XCP protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUser(blt_int8u *data)
{
  blt_int8u subCommand;

  /* Read out the sub command code. */
  subCommand = data[1];

  /* Dispatch sub command handling. */
  switch (subCommand)
  {
#if (BOOT_INFO_TABLE_ENABLE > 0)
  case XCP_CMD_USER_SUB_INFOTABLE:
    XcpCmdUserSubCmdInfoTable(data);
    break;
#endif

  default:
    XcpSetCtoError(XCP_ERR_CMD_UNKNOWN);
    break;
  }
} /*** end of XcpCmdUser ***/


#if (BOOT_INFO_TABLE_ENABLE > 0)
/************************************************************************************//**
** \brief     XCP sub command processor function which handles the info table related
**            sub command of the USER command. Note that this subcommand is a user
**            specific extensions to the XCP protocol.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUserSubCmdInfoTable(blt_int8u *data)
{
  blt_int8u commandId;

  /* Read out the command ID. */
  commandId = data[2];

  /* Dispatch info table command ID handling. */
  switch (commandId)
  {
  case XCP_CMD_IT_CID_GETINFO:
    XcpCmdUserSubCmdInfoTableCidGetInfo(data);
    break;

  case XCP_CMD_IT_CID_DOWNLOAD:
    XcpCmdUserSubCmdInfoTableCidDownload(data);
    break;

  case XCP_CMD_IT_CID_CHECK:
    XcpCmdUserSubCmdInfoTableCidCheck(data);
    break;

  default:
    XcpSetCtoError(XCP_ERR_CMD_UNKNOWN);
    break;
  }
} /*** end of XcpCmdUserSubCmdInfoTable ***/


/************************************************************************************//**
** \brief     XCP user sub command processor function which handles the info table
**            GET_INFO command ID.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUserSubCmdInfoTableCidGetInfo(blt_int8u *data)
{
  blt_int16u tableLen;
  blt_addr   tableAddr;

  /* Suppress compiler warning for unused parameter. */
  data = data;

  /* Set table length and base address information. */
  tableLen = InfoTableCurrentSize(INFO_TABLE_ID_FIRMWARE_NVM);
  tableAddr = InfoTableGetPtr(INFO_TABLE_ID_FIRMWARE_NVM);
  /* Clear the info table in the internal RAM buffer and reset its write pointer. */
  InfoTableClear(INFO_TABLE_ID_INTERNAL_RAM);

  /* Set packet id to command response packet. */
  xcpInfo.ctoData[0] = XCP_PID_RES;
  /* Set the command ID that this is a response for. */
  xcpInfo.ctoData[1] = XCP_CMD_IT_CID_GETINFO;
  /* Store the table length. */
  CpuMemCopy((blt_addr)(&xcpInfo.ctoData[2]), (blt_addr)&tableLen, sizeof(tableLen));
  /* Store the table base address. */
  CpuMemCopy((blt_addr)(&xcpInfo.ctoData[4]), (blt_addr)&tableAddr, sizeof(tableAddr));
  /* Set packet length. */
  xcpInfo.ctoLen = 8;
} /*** end of XcpCmdUserSubCmdInfoTableCidGetInfo ***/


/************************************************************************************//**
** \brief     XCP user sub command processor function which handles the info table
**            DOWNLOAD command ID.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUserSubCmdInfoTableCidDownload(blt_int8u *data)
{
  /* Validate length of download request */
  if (data[3] > (XCP_CTO_PACKET_LEN-4))
  {
    /* Specified data length is too long. */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    return;
  }

  /* Attempt to store the newly received data in the info table. */
  if (InfoTableAddData(INFO_TABLE_ID_INTERNAL_RAM, &data[4], data[3]) == BLT_FALSE)
  {
    /* Data does not fit in the info table RAM buffer. */
    XcpSetCtoError(XCP_ERR_OUT_OF_RANGE);
    return;
  }

  /* Set packet id to command response packet. */
  xcpInfo.ctoData[0] = XCP_PID_RES;
  /* Set the command ID that this is a response for. */
  xcpInfo.ctoData[1] = XCP_CMD_IT_CID_DOWNLOAD;
  /* Set packet length. */
  xcpInfo.ctoLen = 2;
} /*** end of XcpCmdUserSubCmdInfoTableCidDownload ***/


/************************************************************************************//**
** \brief     XCP user sub command processor function which handles the info table
**            CHECK command ID.
** \param     data Pointer to a byte buffer with the packet data.
** \return    none
**
****************************************************************************************/
static void XcpCmdUserSubCmdInfoTableCidCheck(blt_int8u *data)
{
  /* Set packet id to command response packet. */
  xcpInfo.ctoData[0] = XCP_PID_RES;
  /* Set the command ID that this is a response for. */
  xcpInfo.ctoData[1] = XCP_CMD_IT_CID_CHECK;
  /* Set packet length. */
  xcpInfo.ctoLen = 3;

  /* Verify that the contents of the internal RAM info table has the same size as the
   * one in non-volatile memory.
   */
  if (InfoTableCurrentSize(INFO_TABLE_ID_INTERNAL_RAM) !=
      InfoTableCurrentSize(INFO_TABLE_ID_FIRMWARE_NVM))
  {
    /* Content of the internal RAM buffer were not yet fully downloaded. */
    XcpSetCtoError(XCP_ERR_SEQUENCE);
    return;
  }

  /* Request bootloader application to perform the info table check. */
  if (InfoTableCheck() == BLT_TRUE)
  {
    /* Info table check passed. Firmware update is allow to proceed. */
    xcpInfo.ctoData[2] = 1;
  }
  else
  {
    /* Info table check failed. Firmware update should be aborted. */
    xcpInfo.ctoData[2] = 0;
  }
} /*** end of XcpCmdUserSubCmdInfoTableCidCheck ***/
#endif /* BOOT_INFO_TABLE_ENABLE > 0 */


#endif /* BOOT_COM_ENABLE > 0 */

/******************************** end of xcp.c *****************************************/
