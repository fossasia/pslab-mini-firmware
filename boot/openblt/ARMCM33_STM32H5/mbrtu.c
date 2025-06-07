/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/mbrtu.c
* \brief        Bootloader Modbus RTU communication interface source file.
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
#include "boot.h"                                /* bootloader generic header          */
#if (BOOT_COM_MBRTU_ENABLE > 0)
#include "stm32h5xx.h"                           /* STM32 CPU and HAL header           */
#include "stm32h5xx_ll_usart.h"                  /* STM32 LL USART header              */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Timeout for transmitting a byte in milliseconds. */
#define MBRTU_BYTE_TX_TIMEOUT_MS          (10u)
/* map the configured UART channel index to the STM32's USART peripheral */
#if (BOOT_COM_MBRTU_CHANNEL_INDEX == 0)
/** \brief Set UART base address to USART1. */
#define USART_CHANNEL   USART1
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 1)
/** \brief Set UART base address to USART2. */
#define USART_CHANNEL   USART2
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 2)
/** \brief Set UART base address to USART3. */
#define USART_CHANNEL   USART3
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 3)
/** \brief Set UART base address to UART4. */
#define USART_CHANNEL   UART4
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 4)
/** \brief Set UART base address to UART5. */
#define USART_CHANNEL   UART5
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 5)
/** \brief Set UART base address to USART6. */
#define USART_CHANNEL   USART6
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 6)
/** \brief Set UART base address to UART7. */
#define USART_CHANNEL   UART7
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 7)
/** \brief Set UART base address to UART8. */
#define USART_CHANNEL   UART8
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 8)
/** \brief Set UART base address to UART9. */
#define USART_CHANNEL   UART9
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 9)
/** \brief Set UART base address to USART10. */
#define USART_CHANNEL   USART10
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 10)
/** \brief Set UART base address to USART11. */
#define USART_CHANNEL   USART11
#elif (BOOT_COM_MBRTU_CHANNEL_INDEX == 11)
/** \brief Set UART base address to UART12. */
#define USART_CHANNEL   UART12
#endif


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Stores the number of free running counter ticks that represents the 3.5
 *         character delay time (T3_5) for Modbus RTU.
 */
static blt_int16u mbRtuT3_5Ticks;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static blt_bool   MbRtuReceiveByte(blt_int8u *data);
static void       MbRtuTransmitByte(blt_int8u data, blt_bool end_of_packet);


/************************************************************************************//**
** \brief     Initializes the Modbus RTU communication interface.
** \attention It is the application's responsibility to initialize a timer peripheral
**            to have an upwards counting free running counter, which runs at 100 kHz.
** \return    none.
**
****************************************************************************************/
void MbRtuInit(void)
{
  blt_int16u startTimeTicks;
  blt_int16u deltaTimeTicks;
  blt_int16u currentTimeTicks;
  blt_int8u  rxDummy;

  LL_USART_InitTypeDef USART_InitStruct = {0};

  /* The current implementation supports USART1 - UART12. throw an assertion error in
   * case a different UART channel is configured.
   */
  ASSERT_CT((BOOT_COM_MBRTU_CHANNEL_INDEX == 0)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 1)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 2)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 3)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 4)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 5)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 6)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 7)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 8)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 9)  ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 10) ||
            (BOOT_COM_MBRTU_CHANNEL_INDEX == 11));

  /* calculate the 3.5 character delay time in free running counter ticks. note that
   * the free running counter runs at 100 kHz, so one tick is 10 us. For baudrates >
   * 19200 bps, it can be fixed to 1750 us.
   */
  if (BOOT_COM_MBRTU_BAUDRATE > 19200)
  {
    /* set T3_5 time to a fixed value of 1750 us. */
    mbRtuT3_5Ticks = 175;
  }
  /* calculate the T3_5 time, because the baudrate is <= 19200 bps. */
  else
  {
    /* T3_5 [us * 10] = 3.5 * Tchar = 3.5 * 11 * 100000 / baudrate = 3850000 / baudrate.
     * make sure to do integer round up though. Make sure to add 1 to adjust for 10us
     * timer resolution inaccuracy.
     */
    mbRtuT3_5Ticks = (blt_int16u)(((3850000UL + (BOOT_COM_MBRTU_BAUDRATE - 1U)) /
                                    BOOT_COM_MBRTU_BAUDRATE) + 1);
  }

  /* disable the UART peripheral */
  LL_USART_Disable(USART_CHANNEL);
  /* configure UART peripheral */
  USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV8;
  USART_InitStruct.BaudRate = BOOT_COM_MBRTU_BAUDRATE;
  #if (BOOT_COM_MBRTU_STOPBITS == 1)
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  #else
  USART_InitStruct.StopBits = LL_USART_STOPBITS_2;
  #endif
  #if (BOOT_COM_MBRTU_PARITY == 0)
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  #elif (BOOT_COM_MBRTU_PARITY == 1)
  USART_InitStruct.Parity = LL_USART_PARITY_ODD;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_9B;
  #else
  USART_InitStruct.Parity = LL_USART_PARITY_EVEN;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_9B;
  #endif
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  /* initialize the UART peripheral */
  LL_USART_Init(USART_CHANNEL, &USART_InitStruct);
  LL_USART_SetTXFIFOThreshold(USART_CHANNEL, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_SetRXFIFOThreshold(USART_CHANNEL, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_DisableFIFO(USART_CHANNEL);
  LL_USART_ConfigAsyncMode(USART_CHANNEL);
  LL_USART_Enable(USART_CHANNEL);
  /* enable the receiver output to be able to receive. */
  MbRtuDriverOutputControlHook(BLT_FALSE);

  /* wait for idle line detection. This is T3_5 time after reception of the last byte. */
  startTimeTicks = MbRtuFreeRunningCounterGet();
  do
  {
    /* service the watchdog. */
    CopService();
    /* get the current value of the free running counter. */
    currentTimeTicks = MbRtuFreeRunningCounterGet();
    /* check if a byte was received while waiting for the idle line. */
    if (MbRtuReceiveByte(&rxDummy) == BLT_TRUE)
    {
      /* restart the idle line detection. */
      startTimeTicks = currentTimeTicks;
    }
    /* calculate the number of ticks that elapsed since the start or since the last
     * byte reception. Note that this calculation works, even if the free running counter
     * overflowed.
     */
    deltaTimeTicks = currentTimeTicks - startTimeTicks;
  }
  while (deltaTimeTicks < mbRtuT3_5Ticks);
} /*** end of MbRtuInit ***/


/************************************************************************************//**
** \brief     Transmits a packet formatted for the communication interface.
** \param     data Pointer to byte array with data that it to be transmitted.
** \param     len  Number of bytes that are to be transmitted.
** \return    none.
**
****************************************************************************************/
void MbRtuTransmitPacket(blt_int8u *data, blt_int8u len)
{
  blt_int16u data_index;
  blt_int16u checksum;
  blt_bool endOfPacket = BLT_FALSE;
  /* Made static to lower stack load and +5 for Modbus RTU packet overhead. */
  static blt_int8u txPacket[BOOT_COM_MBRTU_TX_MAX_DATA + 5];

  /* On Modbus RTU, there must always be a T3_5 time separation between packet trans-
   * missions.
   *
   * This bootloader uses XCP packets embedded in Modbus RTU packets. The XCP
   * communication is always request / response based. That means that this packet is
   * a response packet and it will only be sent, after the reception of a request packet.
   *
   * A response packet is only deemed valid, after the T3_5 idle time. This module
   * implements the T3_5 end-of-packet time event detection. Consequently, it is already
   * guaranteed that there is T3_5 between subsequent packet transmissions. As such, no
   * further T3_5 wait time is needed here.
   */

  /* verify validity of the len-parameter */
  ASSERT_RT(len <= BOOT_COM_MBRTU_TX_MAX_DATA);

  /* construct the Modbus RTU packet. start by adding the slave address. */
  txPacket[0] = BOOT_COM_MBRTU_NODE_ID;
  /* add the user-defined function code for embedding XCP packets. */
  txPacket[1] = BOOT_COM_MBRTU_FCT_CODE_USER_XCP;
  /* add the XCP packet length. */
  txPacket[2] = len;

  /* copy the XCP packet data. */
  CpuMemCopy((blt_int32u)&txPacket[3], (blt_int32u)data, len);
  /* calculate the checksum for the packet, including slave address, function code and
   * extra XCP length.
   */
  checksum = MbRtuCrcCalculate(&txPacket[0], len + 3);
  /* add the checksum at the end of the packet */
  txPacket[len + 3] = (blt_int8u)(checksum & 0xff);
  txPacket[len + 4] = (blt_int8u)(checksum  >> 8);

  /* enable the driver output to be able to send. just make sure to wait a little around
   * the togglng of the DE/NRE pin.
   */
  MbRtuDelay(BOOT_COM_MBRTU_DRIVER_OUTPUT_ENABLE_DELAY_US);
  MbRtuDriverOutputControlHook(BLT_TRUE);
  MbRtuDelay(BOOT_COM_MBRTU_DRIVER_OUTPUT_ENABLE_DELAY_US);

  /* transmit all the packet bytes one-by-one */
  for (data_index = 0; data_index < (len + 5); data_index++)
  {
    /* keep the watchdog happy */
    CopService();
    /* last byte of the packet? */
    if (data_index == ((len + 5) - 1))
    {
      /* update the end of packet flag. */
      endOfPacket = BLT_TRUE;
    }
    /* write byte */
    MbRtuTransmitByte(txPacket[data_index], endOfPacket);
  }
  /* enable the receiver output to be able to receive again. just make sure to wait a
   * little around the togglng of the DE/NRE pin.
   */
  MbRtuDelay(BOOT_COM_MBRTU_DRIVER_OUTPUT_DISABLE_DELAY_US);
  MbRtuDriverOutputControlHook(BLT_FALSE);
  MbRtuDelay(BOOT_COM_MBRTU_DRIVER_OUTPUT_DISABLE_DELAY_US);
} /*** end of MbRtuTransmitPacket ***/


/************************************************************************************//**
** \brief     Receives a communication interface packet if one is present.
** \param     data Pointer to byte array where the data is to be stored.
** \param     len Pointer where the length of the packet is to be stored.
** \return    BLT_TRUE if a packet was received, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool MbRtuReceivePacket(blt_int8u *data, blt_int8u *len)
{
  blt_bool result = BLT_FALSE;
  blt_int8u rxByte;
  blt_int16u currentTimeTicks;
  blt_int16u deltaTimeTicks;
  blt_int16u checksumCalculated;
  blt_int16u checksumReceived;
  /* Made static to lower stack load and +5 for Modbus RTU packet overhead. */
  static blt_int8u rxPacket[BOOT_COM_MBRTU_RX_MAX_DATA + 5];
  static blt_int8u rxLength = 0;
  static blt_bool packetRxInProgress = BLT_FALSE;
  static blt_int16u lastRxByteTimeTicks = 0;

  /* get the current value of the free running counter. */
  currentTimeTicks = MbRtuFreeRunningCounterGet();

  /* check for a newly received byte. */
  if (MbRtuReceiveByte(&rxByte) == BLT_TRUE)
  {
    /* store the time at which the byte was received. */
    lastRxByteTimeTicks = currentTimeTicks;
    /* is this the potential start of a new packet? */
    if (packetRxInProgress == BLT_FALSE)
    {
      /* initialize the reception of a new packet. */
      rxLength = 0;
      packetRxInProgress = BLT_TRUE;
    }
    /* store the newly received byte in the buffer, with buffer overrun protection. */
    if (rxLength < (sizeof(rxPacket)/sizeof(rxPacket[0])))
    {
      rxPacket[rxLength] = rxByte;
      rxLength++;
    }
    /* buffer overrun occurred. received packet was longer than supported so discard
     * the packet to try and sync to the next one.
     */
    else
    {
      /* discard the partially received packet. */
      packetRxInProgress = BLT_FALSE;
    }
  }

  /* only attempt to detect the end of packet, when a reception is in progress. */
  if (packetRxInProgress == BLT_TRUE)
  {
    /* calculate the number of ticks that elapsed since the last byte reception. note
     * that this calculation works, even if the free running counter overflowed.
     */
    deltaTimeTicks = currentTimeTicks - lastRxByteTimeTicks;
    /* packet reception is assumed complete after T3_5 of not receiving new data. */
    if (deltaTimeTicks >= mbRtuT3_5Ticks)
    {
      /* a Modbus RTU packet consists of at least the address field, function code and
       * 16-bit CRC. Validate the packet length based on this info.
       */
      if (rxLength >= 4)
      {
        /* calculate the packet checksum. */
        checksumCalculated = MbRtuCrcCalculate(&rxPacket[0], rxLength - 2);
        /* extract the checksum received with the packet. */
        checksumReceived = rxPacket[rxLength - 2] | (rxPacket[rxLength - 1] << 8);
        /* only continue with packet processing if the checksums match. */
        if (checksumCalculated == checksumReceived)
        {
          /* we are only interested in Modbus RTU packets that are addressed to us and
           * have an XCP packet embedded.
           */
          if ( (rxPacket[0] == BOOT_COM_MBRTU_NODE_ID) &&
               (rxPacket[1] == BOOT_COM_MBRTU_FCT_CODE_USER_XCP) )
          {
            /* An XCP packet embedded in a Modbus RTU packet has an extra XCP packet
             * length value. Use it to double-check that the packet length is valid.
             */
            if (rxPacket[2] == (rxLength - 5))
            {
              /* copy the packet's XCP data. */
              CpuMemCopy((blt_int32u)data, (blt_int32u)&rxPacket[3], rxLength - 5);
              /* set the packet's XCP length. */
              *len = rxLength - 5;
              /* update the result to success to indicate that this XCP packet is ready
               * for processing.
               */
              result = BLT_TRUE;
            }
          }
        }
      }
      /* reset the packet reception in progress flag, to be able to receive the next. */
      packetRxInProgress = BLT_FALSE;
    }
  }

  /* give the result back to the caller. */
  return result;
} /*** end of MbRtuReceivePacket ***/


/************************************************************************************//**
** \brief     Receives a communication interface byte if one is present.
** \param     data Pointer to byte where the data is to be stored.
** \return    BLT_TRUE if a byte was received, BLT_FALSE otherwise.
**
****************************************************************************************/
static blt_bool MbRtuReceiveByte(blt_int8u *data)
{
  blt_bool result = BLT_FALSE;

  /* check if a new byte was received on the configured channel */
  if (LL_USART_IsActiveFlag_RXNE(USART_CHANNEL) != 0)
  {
    /* update the result */
    result = BLT_TRUE;
    /* check for a frame error. the frame error check is important because it can detect
     * a missing stopbit. on an RS485 bus without bias resistors, the A-B differential
     * voltage is 0. for an RS485 transceiver this is neither a 0 nor a 1 bit, so
     * undefined. most RS485 transceivers feature a reception failsafe function to drive
     * the Rx output (going to the UART Rx) to a defined state of logic 1. in case the
     * used RS485 transceiver doesn't have such a feature, it typically leaves the Rx
     * output in a logic 0 state. this means that after the stop bit of the last packet
     * byte, the UART Rx input sees a logic 0, and assumes it is a start bit. the
     * remaining data bits will always be 0 and, most importantly no stop bit is
     * present, causing a framing error. Long story short: if you don't check for the
     * framing error flag, you might receive an extra byte with value 0, which is not
     * actually transmitted on the RS485 bus. you can catch and ignore this byte by doing
     * a frame error check.
     */
    if (LL_USART_IsActiveFlag_FE(USART_CHANNEL) != 0)
    {
      /* clear the error flag. */
      LL_USART_ClearFlag_FE(USART_CHANNEL);
      /* ignore the byte because of a detected frame error. */
      result = BLT_FALSE;
    }
    #if (BOOT_COM_MBRTU_PARITY > 0)
    /* check for a parity error. */
    if (LL_USART_IsActiveFlag_PE(USART_CHANNEL) != 0)
    {
      /* clear the error flag. */
      LL_USART_ClearFlag_PE(USART_CHANNEL);
      /* ignore the byte because of a detected parity error. */
      result = BLT_FALSE;
    }
    #endif
    /* retrieve and store the newly received byte */
    *data = LL_USART_ReceiveData8(USART_CHANNEL);
  }
  /* give the result back to the caller */
  return result;
} /*** end of MbRtuReceiveByte ***/


/************************************************************************************//**
** \brief     Transmits a communication interface byte.
** \param     data Value of byte that is to be transmitted.
** \param     end_of_packet BLT_TRUE if this is the last byte of the packet, BLT_FALSE
**            otherwise.
** \return    none.
**
****************************************************************************************/
static void MbRtuTransmitByte(blt_int8u data, blt_bool end_of_packet)
{
  blt_int32u timeout;

  /* write byte to transmit holding register */
  LL_USART_TransmitData8(USART_CHANNEL, data);
  /* set timeout time to wait for transmit completion. */
  timeout = TimerGet() + MBRTU_BYTE_TX_TIMEOUT_MS;

  /* not the last byte of the packet? */
  if (end_of_packet == BLT_FALSE)
  {
    /* wait for tx holding register to be empty */
    while (LL_USART_IsActiveFlag_TXE(USART_CHANNEL) == 0)
    {
      /* keep the watchdog happy */
      CopService();
      /* break loop upon timeout. this would indicate a hardware failure. */
      if (TimerGet() > timeout)
      {
        break;
      }
    }
  }
  /* this is the last byte of a packet. */
  else
  {
    /* wait for tx complete event. this is needed for the last byte, otherwise the
     * transceiver's transmit output gets disabled with MbRtuDriverOutputControlHook()
     * before the byte reception completes.
     */
    while (LL_USART_IsActiveFlag_TC(USART_CHANNEL) == 0)
    {
      /* keep the watchdog happy */
      CopService();
      /* break loop upon timeout. this would indicate a hardware failure. */
      if (TimerGet() > timeout)
      {
        break;
      }
    }
  }
} /*** end of MbRtuTransmitByte ***/


/************************************************************************************//**
** \brief     Obtains the counter value of the 100 kHz free running counter. Note that
**            each count represent 10 us. The Modbus RTU communication module uses this
**            free running counter for Modbus RTU packet timing related purposes. The
**            already available 1 ms timer does not have the needed resolution for this
**            purpose.
** \return    Current value of the free running counter.
**
****************************************************************************************/
blt_int16u MbRtuFreeRunningCounterGet(void)
{
  blt_int16u result;

  /* This port's timer module is based on a 100 kHz free running counter, which is
   * already configured and running. Therefore we just need to read out its free running
   * counter value.
   */
  result = (blt_int16u)TIM1->CNT;

  /* give the result back to the caller. */
  return result;
} /*** end of MbRtuFreeRunningCounterGet ***/
#endif /* BOOT_COM_MBRTU_ENABLE > 0 */

/*********************************** end of mbrtu.c ************************************/
