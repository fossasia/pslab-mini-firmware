/************************************************************************************//**
* \file         Source/com.c
* \brief        Bootloader communication interface source file.
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
#if (BOOT_COM_CAN_ENABLE > 0)
#include "can.h"                                 /* can driver module                  */
#endif
#if (BOOT_COM_RS232_ENABLE > 0)
#include "rs232.h"                               /* rs232 driver module                */
#endif
#if (BOOT_COM_MBRTU_ENABLE > 0)
#include "mb.h"                                  /* modbus driver module               */
#endif
#if (BOOT_COM_USB_ENABLE > 0)
#include "usb.h"                                 /* usb driver module                  */
#endif
#if (BOOT_COM_NET_ENABLE > 0)
#include "net.h"                                 /* tcp/ip driver module               */
#endif


#if (BOOT_COM_ENABLE > 0)
/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Holds the communication interface of the currently active interface. */
static tComInterfaceId comActiveInterface = COM_IF_OTHER;


/************************************************************************************//**
** \brief     Initializes the communication module including the hardware needed for
**            the communication.
** \return    none
**
****************************************************************************************/
void ComInit(void)
{
  /* initialize the XCP communication protocol */
  XcpInit();
#if (BOOT_COM_CAN_ENABLE > 0)
  /* initialize the CAN controller */
  CanInit();
  /* set it as active */
  comActiveInterface = COM_IF_CAN;
#endif
#if (BOOT_COM_RS232_ENABLE > 0)
  /* initialize the RS232 interface */
  Rs232Init();
  /* set it as active */
  comActiveInterface = COM_IF_RS232;
#endif
#if (BOOT_COM_MBRTU_ENABLE > 0)
  /* initialize the Modbus RTU interface */
  MbRtuInit();
  /* set it as active */
  comActiveInterface = COM_IF_MBRTU;
#endif
#if (BOOT_COM_USB_ENABLE > 0)
  /* initialize the USB interface */
  UsbInit();
  /* set it as active */
  comActiveInterface = COM_IF_USB;
#endif
#if (BOOT_COM_NET_ENABLE > 0)
  #if (BOOT_COM_NET_DEFERRED_INIT_ENABLE == 0)
  /* initialize the TCP/IP interface */
  NetInit();
  /* set it as active */
  comActiveInterface = COM_IF_NET;
  #endif
#endif
} /*** end of ComInit ***/


/************************************************************************************//**
** \brief     Updates the communication module by checking if new data was received
**            and submitting the request to process newly received data.
** \return    none
**
****************************************************************************************/
void ComTask(void)
{
  blt_int8u xcpPacketLen;
  /* make xcpCtoReqPacket static for runtime efficiency */
  static blt_int8u xcpCtoReqPacket[BOOT_COM_RX_MAX_DATA];

#if (BOOT_COM_CAN_ENABLE > 0)
  if (CanReceivePacket(&xcpCtoReqPacket[0], &xcpPacketLen) == BLT_TRUE)
  {
    /* make this the active interface */
    comActiveInterface = COM_IF_CAN;
    /* process packet */
    XcpPacketReceived(&xcpCtoReqPacket[0], xcpPacketLen);
  }
#endif
#if (BOOT_COM_RS232_ENABLE > 0)
  if (Rs232ReceivePacket(&xcpCtoReqPacket[0], &xcpPacketLen) == BLT_TRUE)
  {
    /* make this the active interface */
    comActiveInterface = COM_IF_RS232;
    /* process packet */
    XcpPacketReceived(&xcpCtoReqPacket[0], xcpPacketLen);
  }
#endif
#if (BOOT_COM_MBRTU_ENABLE > 0)
  if (MbRtuReceivePacket(&xcpCtoReqPacket[0], &xcpPacketLen) == BLT_TRUE)
  {
    /* make this the active interface */
    comActiveInterface = COM_IF_MBRTU;
    /* process packet */
    XcpPacketReceived(&xcpCtoReqPacket[0], xcpPacketLen);
  }
#endif
#if (BOOT_COM_USB_ENABLE > 0)
  if (UsbReceivePacket(&xcpCtoReqPacket[0], &xcpPacketLen) == BLT_TRUE)
  {
    /* make this the active interface */
    comActiveInterface = COM_IF_USB;
    /* process packet */
    XcpPacketReceived(&xcpCtoReqPacket[0], xcpPacketLen);
  }
#endif
#if (BOOT_COM_NET_ENABLE > 0)
  if (NetReceivePacket(&xcpCtoReqPacket[0], &xcpPacketLen) == BLT_TRUE)
  {
    /* make this the active interface */
    comActiveInterface = COM_IF_NET;
    /* process packet */
    XcpPacketReceived(&xcpCtoReqPacket[0], xcpPacketLen);
  }
#endif
} /*** end of ComTask ***/


/************************************************************************************//**
** \brief     Releases the communication module.
** \return    none
**
****************************************************************************************/
void ComFree(void)
{
#if (BOOT_COM_USB_ENABLE > 0)
  /* disconnect the usb device from the usb host */
  UsbFree();
#endif
} /*** end of ComFree ***/


/************************************************************************************//**
** \brief     Transmits the packet using the xcp transport layer.
** \param     data Pointer to the byte buffer with packet data.
** \param     len  Number of data bytes that need to be transmitted.
** \return    none
**
****************************************************************************************/
void ComTransmitPacket(blt_int8u *data, blt_int16u len)
{
#if (BOOT_COM_CAN_ENABLE > 0)
  /* transmit the packet. note that len is limited to 8 in the plausibility check,
   * so cast is okay.
   */
  if (comActiveInterface == COM_IF_CAN)
  {
    CanTransmitPacket(data, (blt_int8u)len);
  }
#endif
#if (BOOT_COM_RS232_ENABLE > 0)
  /* transmit the packet. note that len is limited to 255 in the plausibility check,
   * so cast is okay.
   */
  if (comActiveInterface == COM_IF_RS232)
  {
    Rs232TransmitPacket(data, (blt_int8u)len);
  }
#endif
#if (BOOT_COM_MBRTU_ENABLE > 0)
  /* transmit the packet. note that len is limited to 255 in the plausibility check,
   * so cast is okay.
   */
  if (comActiveInterface == COM_IF_MBRTU)
  {
    MbRtuTransmitPacket(data, (blt_int8u)len);
  }
#endif
#if (BOOT_COM_USB_ENABLE > 0)
  /* transmit the packet */
  if (comActiveInterface == COM_IF_USB)
  {
    UsbTransmitPacket(data, len);
  }
#endif
#if (BOOT_COM_NET_ENABLE > 0)
  if (comActiveInterface == COM_IF_NET)
  {
    /* transmit the packet */
    NetTransmitPacket(data, len);
  }
#endif

  /* send signal that the packet was transmitted */
  XcpPacketTransmitted();
} /*** end of ComTransmitPacket ***/


/************************************************************************************//**
** \brief     Obtains the maximum number of bytes that can be received on the specified
**            communication interface.
** \return    Maximum number of bytes that can be received.
**
****************************************************************************************/
blt_int16u ComGetActiveInterfaceMaxRxLen(void)
{
  blt_int16u result;

  /* filter on communication interface identifier */
  switch (comActiveInterface)
  {
    case COM_IF_RS232:
      result = BOOT_COM_RS232_RX_MAX_DATA;
      break;

    case COM_IF_MBRTU:
      result = BOOT_COM_MBRTU_RX_MAX_DATA;
      break;

    case COM_IF_CAN:
      result = BOOT_COM_CAN_RX_MAX_DATA;
      break;

    case COM_IF_USB:
      result = BOOT_COM_USB_RX_MAX_DATA;
      break;

    case COM_IF_NET:
      result = BOOT_COM_NET_RX_MAX_DATA;
      break;

    default:
      result = BOOT_COM_RX_MAX_DATA;
      break;
  }

  return result;
} /*** end of ComGetActiveInterfaceMaxRxLen ***/


/************************************************************************************//**
** \brief     Obtains the maximum number of bytes that can be transmitted on the
**            specified communication interface.
** \return    Maximum number of bytes that can be received.
**
****************************************************************************************/
blt_int16u ComGetActiveInterfaceMaxTxLen(void)
{
  blt_int16u result;

  /* filter on communication interface identifier */
  switch (comActiveInterface)
  {
    case COM_IF_RS232:
      result = BOOT_COM_RS232_TX_MAX_DATA;
      break;

    case COM_IF_MBRTU:
      result = BOOT_COM_MBRTU_TX_MAX_DATA;
      break;

    case COM_IF_CAN:
      result = BOOT_COM_CAN_TX_MAX_DATA;
      break;

    case COM_IF_USB:
      result = BOOT_COM_USB_TX_MAX_DATA;
      break;

    case COM_IF_NET:
      result = BOOT_COM_NET_TX_MAX_DATA;
      break;

    default:
      result = BOOT_COM_TX_MAX_DATA;
      break;
  }

  return result;
} /*** end of ComGetActiveInterfaceMaxTxLen ***/


/************************************************************************************//**
** \brief     This function obtains the XCP connection state.
** \return    BLT_TRUE when an XCP connection is established, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool ComIsConnected(void)
{
  blt_bool result = BLT_FALSE;

  /* Is there an active XCP connection? This indicates that the communication interface
   * is in the connection state. 
   */  
  if (XcpIsConnected())
  {
    result = BLT_TRUE;
  }
#if (ADDON_GATEWAY_MOD_ENABLE > 0)
  /* Is the gateway active? This indicates an XCP connection with a slave. */
  if (GatewayIsActive())
  {
    result = BLT_TRUE;
  }
#endif
  /* give the result back to the caller. */
  return result;
} /*** end of ComIsConnected ***/


#if (BOOT_COM_DEFERRED_INIT_ENABLE == 1)
/************************************************************************************//**
** \brief     The deferred init feature makes it possible to bypass the initialization of
**            a communication interface until this function is called. This feature can
**            be enabled for a specific communication interface via macro
**            BOOT_COM_XXX_DEFERRED_INIT_ENABLE in blt_conf.h. At this point only the NET
**            communication interface supports this feature, as its initialization can
**            take quite a long time. If there is a valid user program present, then this
**            would cause an unwanted delay after each reset before the user program can
**            be started.
** \attention Note that when this feature is enabled for a communication interface, the
**            communication interface is only enabled when: (a) no valid user program is
**            present or (b) when CpuUserProgramStartHook() returns BLT_FALSE. This means
**            that after a normal reactivation of the bootloader from the user program,
**            the communication interface is not initialized and firmware updates are
**            not possible! In this case it is recommended to somehow pass on the
**            communication initialization request from the user program to the
**            bootloader. When this request detected by the bootloader application, this
**            function should be called. EEPROM or shared RAM can be used to pass on such
**            a request.
** \return    none
**
****************************************************************************************/
void ComDeferredInit(void)
{
#if (BOOT_COM_NET_ENABLE > 0)
  #if (BOOT_COM_NET_DEFERRED_INIT_ENABLE == 1)
  /* perform deferred initialization the TCP/IP interface */
  NetDeferredInit();
  /* set it as active */
  comActiveInterface = COM_IF_NET;
  #endif
#endif
#if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
  /* the default internal timed backdoor mechanism should start its timer after the
   * communication interfaces are initialized. since a deferred initialization was now
   * performed, the backdoor timer should be restarted.
   */
  BackDoorRestartTimer();
#endif
} /*** end of ComDeferredInit ***/
#endif /* BOOT_COM_DEFERRED_INIT_ENABLE == 1 */


#endif /* BOOT_COM_ENABLE > 0 */

/*********************************** end of com.c **************************************/
