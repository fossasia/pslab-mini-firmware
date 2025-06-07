/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/usb.c
* \brief        Bootloader USB communication interface source file.
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

/****************************************************************************************
* Include files
****************************************************************************************/
#include "boot.h"                                     /* bootloader generic header     */
#if (BOOT_COM_USB_ENABLE > 0)
#include "usb.h"                                      /* USB bootloader driver         */
#include "tusb.h"                                     /* TinyUSB stack                 */


/****************************************************************************************
* Configuration macros
****************************************************************************************/
/* For a USB bootloader, the backdoor needs to stay open long enough for the USB device
 * to enumerate on the host PC. Therefore the default backdoor open time needs to be
 * extended. Note that this won't be long enough for a first time USB driver install
 * on the host PC. In this case the bootloader should be started with the backup backdoor
 * that uses, for example, a digital input to force the bootloader to stay active. This
 * can be implemented in CpuUserProgramStartHook(). Feel free to shorten/lengthen this
 * time for finetuning. Note that adding this configuration macro to blt_conf.h overrides
 * the value here.
 */
#ifndef BOOT_COM_USB_BACKDOOR_EXTENSION_MS
#define BOOT_COM_USB_BACKDOOR_EXTENSION_MS   (2000)
#endif


/****************************************************************************************
* Configuration check
****************************************************************************************/
/* The TinyUSB port was modified to support polling mode operation, needed by the 
 * bootloader. Verify that polling mode was actually enabled in the configuration header.
 */
#if (CFG_TUSB_POLLING_ENABLED <= 0)
#error "CFG_TUSB_POLLING_ENABLED must be > 0"
#endif


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static blt_bool  UsbReceiveByte(blt_int8u *data);


/************************************************************************************//**
** \brief     Initializes the USB communication interface.
** \return    none.
**
****************************************************************************************/
void UsbInit(void)
{
  /* initialize the TinyUSB device stack on the configured roothub port */
  tud_init(BOARD_TUD_RHPORT);
  /* extend the time that the backdoor is open in case the default timed backdoor
   * mechanism is used.
   */
#if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
  if (BackDoorGetExtension() < BOOT_COM_USB_BACKDOOR_EXTENSION_MS)
  {
    BackDoorSetExtension(BOOT_COM_USB_BACKDOOR_EXTENSION_MS);
  }
#endif /* BOOT_BACKDOOR_HOOKS_ENABLE == 0 */
} /*** end of UsbInit ***/


/************************************************************************************//**
** \brief     Releases the USB communication interface.
** \return    none.
**
****************************************************************************************/
void UsbFree(void)
{
  /* disconnect the TinyUSB device stack.*/
  tud_disconnect();
} /*** end of UsbFree ***/


/************************************************************************************//**
** \brief     Transmits a packet formatted for the communication interface.
** \param     data Pointer to byte array with data that it to be transmitted.
** \param     len  Number of bytes that are to be transmitted.
** \return    none.
**
****************************************************************************************/
void UsbTransmitPacket(blt_int8u *data, blt_int8u len)
{
  blt_int32u result;

  /* verify validity of the len-paramenter */
  ASSERT_RT(len <= BOOT_COM_USB_TX_MAX_DATA);

  /* first transmit the length of the packet */
  result = tud_vendor_write(&len, 1);
  ASSERT_RT(result == 1);
  /* next transmit the actual packet bytes */
  result = tud_vendor_write(data, len);
  ASSERT_RT(result == len);
  /* make sure the transmission starts, even if the endpoint buffer is not yet full */
  tud_vendor_flush();
} /*** end of UsbTransmitPacket ***/


/************************************************************************************//**
** \brief     Receives a communication interface packet if one is present.
** \param     data Pointer to byte array where the data is to be stored.
** \param     len Pointer where the length of the packet is to be stored.
** \return    BLT_TRUE if a packet was received, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool UsbReceivePacket(blt_int8u *data, blt_int8u *len)
{
  static blt_int8u xcpCtoReqPacket[BOOT_COM_USB_RX_MAX_DATA+1];  /* one extra for length */
  static blt_int8u xcpCtoRxLength;
  static blt_bool  xcpCtoRxInProgress = BLT_FALSE;

  /* poll for USB interrupt flags to process USB releated event and run the USB device
   * stack task.
    */
  tud_int_handler(BOARD_TUD_RHPORT);
  tud_task();

  /* start of cto packet received? */
  if (xcpCtoRxInProgress == BLT_FALSE)
  {
    /* store the message length when received */
    if (UsbReceiveByte(&xcpCtoReqPacket[0]) == BLT_TRUE)
    {
      if ( (xcpCtoReqPacket[0] > 0) &&
           (xcpCtoReqPacket[0] <= BOOT_COM_USB_RX_MAX_DATA) )
      {
        /* indicate that a cto packet is being received */
        xcpCtoRxInProgress = BLT_TRUE;
        /* reset packet data count */
        xcpCtoRxLength = 0;
      }
    }
  }
  else
  {
    /* store the next packet byte */
    if (UsbReceiveByte(&xcpCtoReqPacket[xcpCtoRxLength+1]) == BLT_TRUE)
    {
      /* increment the packet data count */
      xcpCtoRxLength++;

      /* check to see if the entire packet was received */
      if (xcpCtoRxLength == xcpCtoReqPacket[0])
      {
        /* copy the packet data */
        CpuMemCopy((blt_int32u)data, (blt_int32u)&xcpCtoReqPacket[1], xcpCtoRxLength);
        /* done with cto packet reception */
        xcpCtoRxInProgress = BLT_FALSE;
        /* set the packet length */
        *len = xcpCtoRxLength;
        /* packet reception complete */
        return BLT_TRUE;
      }
    }
  }
  /* packet reception not yet complete */
  return BLT_FALSE;
} /*** end of UsbReceivePacket ***/


/************************************************************************************//**
** \brief     Receives a communication interface byte if one is present.
** \param     data Pointer to byte where the data is to be stored.
** \return    BLT_TRUE if a byte was received, BLT_FALSE otherwise.
**
****************************************************************************************/
static blt_bool UsbReceiveByte(blt_int8u *data)
{
  blt_bool result = BLT_FALSE;
  blt_int32u count;

  /* USB received data available? */
  if (tud_vendor_available())
  {
    /* read the next byte from the internal USB reception buffer */
    count = tud_vendor_read(data, 1);
    /* check read result */
    if (count == 1)
    {
      result = BLT_TRUE;
    }
  }
  /* give the result back to the caller */
  return result;
} /*** end of UsbReceiveByte ***/


/************************************************************************************//**
** \brief     TinyUSB stack callback invoked then the USB device is mounted.
**
****************************************************************************************/
void tud_mount_cb(void)
{
  /* inform application about the connect event */
  UsbConnectHook(BLT_TRUE);
} /*** end of tud_mount_cb ***/


/************************************************************************************//**
** \brief     TinyUSB stack callback invoked then the USB device is unmounted.
**
****************************************************************************************/
void tud_umount_cb(void)
{
  /* inform application about the disconnect event */
  UsbConnectHook(BLT_FALSE);
} /*** end of tud_mount_cb ***/


/************************************************************************************//**
** \brief     TinyUSB stack callback invoked then the USB bus is suspended. Within 7ms
**            the device must draw an average of current less than 2.5 mA from the bus.
** \param     remote_wakeup_en True if the host allows us to perform a remote wakeup.
**
****************************************************************************************/
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;

  /* Inform application that the USB entered low power mode. */
  UsbEnterLowPowerModeHook();
} /*** end of tud_suspend_cb ***/


/************************************************************************************//**
** \brief     TinyUSB stack callback invoked then the USB bus is resumed.
**
****************************************************************************************/
void tud_resume_cb(void)
{
  /* Inform application that the USB left low power mode. */
  UsbLeaveLowPowerModeHook();
} /*** end of tud_resume_cb ***/
#endif /* BOOT_COM_USB_ENABLE > 0 */


/*********************************** end of usb.c **************************************/
