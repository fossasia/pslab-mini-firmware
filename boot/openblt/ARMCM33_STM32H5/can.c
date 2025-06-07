/************************************************************************************//**
* \file         Source/ARMCM33_STM32H5/can.c
* \brief        Bootloader CAN communication interface source file.
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
#if (BOOT_COM_CAN_ENABLE > 0)
#include "stm32h5xx.h"                           /* STM32 CPU and HAL header           */
#include "stm32h5xx_ll_rcc.h"                    /* STM32 LL RCC header                */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Timeout for transmitting a CAN message in milliseconds. */
#define CAN_MSG_TX_TIMEOUT_MS          (50u)

/* map the configured CAN channel index to the STM32's CAN peripheral */
#if (BOOT_COM_CAN_CHANNEL_INDEX == 0)
/** \brief Set CAN base address to CAN1. */
#define CAN_CHANNEL   FDCAN1
#elif (BOOT_COM_CAN_CHANNEL_INDEX == 1)
/** \brief Set CAN base address to CAN2. */
#define CAN_CHANNEL   FDCAN2
#endif


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Structure type for grouping CAN bus timing related information. */
typedef struct t_can_bus_timing
{
  blt_int8u tseg1;                                    /**< CAN time segment 1          */
  blt_int8u tseg2;                                    /**< CAN time segment 2          */
} tCanBusTiming;


/****************************************************************************************
* Local constant declarations
****************************************************************************************/
/** \brief CAN bittiming table for dynamically calculating the bittiming settings.
 *  \details According to the CAN protocol 1 bit-time can be made up of between 8..25
 *           time quanta (TQ). The total TQ in a bit is SYNC + TSEG1 + TSEG2 with SYNC
 *           always being 1. The sample point is (SYNC + TSEG1) / (SYNC + TSEG1 + SEG2) *
 *           100%. This array contains possible and valid time quanta configurations with
 *           a sample point between 68..78%.
 */
static const tCanBusTiming canTiming[] =
{
  /*  TQ | TSEG1 | TSEG2 | SP  */
  /* ------------------------- */
  {  5, 2 },          /*   8 |   5   |   2   | 75% */
  {  6, 2 },          /*   9 |   6   |   2   | 78% */
  {  6, 3 },          /*  10 |   6   |   3   | 70% */
  {  7, 3 },          /*  11 |   7   |   3   | 73% */
  {  8, 3 },          /*  12 |   8   |   3   | 75% */
  {  9, 3 },          /*  13 |   9   |   3   | 77% */
  {  9, 4 },          /*  14 |   9   |   4   | 71% */
  { 10, 4 },          /*  15 |  10   |   4   | 73% */
  { 11, 4 },          /*  16 |  11   |   4   | 75% */
  { 12, 4 },          /*  17 |  12   |   4   | 76% */
  { 12, 5 },          /*  18 |  12   |   5   | 72% */
  { 13, 5 },          /*  19 |  13   |   5   | 74% */
  { 14, 5 },          /*  20 |  14   |   5   | 75% */
  { 15, 5 },          /*  21 |  15   |   5   | 76% */
  { 15, 6 },          /*  22 |  15   |   6   | 73% */
  { 16, 6 },          /*  23 |  16   |   6   | 74% */
  { 16, 7 },          /*  24 |  16   |   7   | 71% */
  { 16, 8 }           /*  25 |  16   |   8   | 68% */
};


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief CAN handle to be used in API calls. */
static FDCAN_HandleTypeDef canHandle;


/************************************************************************************//**
** \brief     Search algorithm to match the desired baudrate to a possible bus
**            timing configuration.
** \param     baud The desired baudrate in kbps. Valid values are 10..1000.
** \param     prescaler Pointer to where the value for the prescaler will be stored.
** \param     tseg1 Pointer to where the value for TSEG2 will be stored.
** \param     tseg2 Pointer to where the value for TSEG2 will be stored.
** \return    BLT_TRUE if the CAN bustiming register values were found, BLT_FALSE
**            otherwise.
**
****************************************************************************************/
static blt_bool CanGetSpeedConfig(blt_int16u baud, blt_int16u *prescaler,
                                  blt_int8u *tseg1, blt_int8u *tseg2)
{
  blt_int8u  cnt;
  blt_int32u canClockFreqkHz;

  /* set the clock source for the CAN peripheral to the PLL1Q */
  LL_RCC_SetFDCANClockSource(LL_RCC_FDCAN_CLKSOURCE_PLL1Q);
  /* obtain the CAN peripheral clock frequency in kHz. it's the PLLQ clock */
  canClockFreqkHz = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE) / 1000u;
  /* the CAN peripheral clock should not be higher than 48 MHz. so only continue if
   * this is the case. when too high, increase the Q divider in the clock configuration
   * to get to a PLL1Q frequency that is below 48 MHz. A multiple of 8 MHz for the
   * PLL1Q frequency gives the best support for most commonly CAN baudrates.
   */
  if (canClockFreqkHz <= 48000u)
  {
    /* loop through all possible time quanta configurations to find a match */
    for (cnt=0; cnt < sizeof(canTiming)/sizeof(canTiming[0]); cnt++)
    {
      if ((canClockFreqkHz % (baud*(canTiming[cnt].tseg1+canTiming[cnt].tseg2+1))) == 0)
      {
        /* compute the prescaler that goes with this TQ configuration */
        *prescaler = canClockFreqkHz/(baud*(canTiming[cnt].tseg1+canTiming[cnt].tseg2+1));

        /* make sure the prescaler is valid */
        if ((*prescaler > 0) && (*prescaler <= 512))
        {
          /* store the bustiming configuration */
          *tseg1 = canTiming[cnt].tseg1;
          *tseg2 = canTiming[cnt].tseg2;
          /* found a good bus timing configuration */
          return BLT_TRUE;
        }
      }
    }
  }
  /* could not find a good bus timing configuration */
  return BLT_FALSE;
} /*** end of CanGetSpeedConfig ***/


/************************************************************************************//**
** \brief     Initializes the CAN controller and synchronizes it to the CAN bus.
** \return    none.
**
****************************************************************************************/
void CanInit(void)
{
  blt_int16u          prescaler = 0;
  blt_int8u           tseg1 = 0, tseg2 = 0;
  blt_int32u          rxMsgId = BOOT_COM_CAN_RX_MSG_ID;
  FDCAN_FilterTypeDef filterConfig;

  /* the current implementation supports CAN1 and 2. throw an assertion error in case
   * a  different CAN channel is configured.
   */
  ASSERT_CT((BOOT_COM_CAN_CHANNEL_INDEX == 0) ||
            (BOOT_COM_CAN_CHANNEL_INDEX == 1));

  /* obtain bittiming configuration information. */
  if (CanGetSpeedConfig(BOOT_COM_CAN_BAUDRATE/1000, &prescaler,
                        &tseg1, &tseg2) == BLT_FALSE)
  {
    /* Incorrect configuration. The specified baudrate is not supported for the given
     * clock configuration. Verify the following settings in blt_conf.h:
     *   - BOOT_COM_CAN_BAUDRATE
     *   - BOOT_CPU_XTAL_SPEED_KHZ
     *   - BOOT_CPU_SYSTEM_SPEED_KHZ
     */
    ASSERT_RT(BLT_FALSE);
  }

  /* set the CAN controller configuration. */
  canHandle.Instance = CAN_CHANNEL;
  canHandle.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  canHandle.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  canHandle.Init.Mode = FDCAN_MODE_NORMAL;
  canHandle.Init.AutoRetransmission = ENABLE;
  canHandle.Init.TransmitPause = DISABLE;
  canHandle.Init.ProtocolException = DISABLE;
  canHandle.Init.NominalPrescaler = prescaler;
  canHandle.Init.NominalSyncJumpWidth = 1;
  canHandle.Init.NominalTimeSeg1 = tseg1;
  canHandle.Init.NominalTimeSeg2 = tseg2;
  /* FD mode is not used by this driver, so the .Init.DataXxx values are don't care. */
  canHandle.Init.DataPrescaler = 1;
  canHandle.Init.DataSyncJumpWidth = 1;
  canHandle.Init.DataTimeSeg1 = 1;
  canHandle.Init.DataTimeSeg2 = 1;
  /* does the message to be received have a standard 11-bit CAN identifier? */
  if ((rxMsgId & 0x80000000) == 0)
  {
    canHandle.Init.StdFiltersNbr = 1;
    canHandle.Init.ExtFiltersNbr = 0;
  }
  else
  {
    canHandle.Init.StdFiltersNbr = 0;
    canHandle.Init.ExtFiltersNbr = 1;
  }
  canHandle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  /* initialize the CAN controller. this only fails if the CAN controller hardware is
   * faulty. no need to evaluate the return value as there is nothing we can do about
   * a faulty CAN controller.
   */
  (void)HAL_FDCAN_Init(&canHandle);

  /* configure the reception filter. note that the implementation of this function
   * always returns HAL_OK as long as the CAN controller is initialized, so no need to
   * evaluate the return value.
   */
  if ((rxMsgId & 0x80000000) == 0)
  {
    filterConfig.IdType = FDCAN_STANDARD_ID;
  }
  else
  {
    filterConfig.IdType = FDCAN_EXTENDED_ID;
    /* negate the ID-type bit */
    rxMsgId &= ~0x80000000;
  }
  filterConfig.FilterIndex = 0;
  filterConfig.FilterType = FDCAN_FILTER_DUAL;
  filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filterConfig.FilterID1 = rxMsgId;
  filterConfig.FilterID2 = rxMsgId;
  (void)HAL_FDCAN_ConfigFilter(&canHandle, &filterConfig);

  /* configure global filter to reject all non-matching frames. */
  HAL_FDCAN_ConfigGlobalFilter(&canHandle, FDCAN_REJECT, FDCAN_REJECT, 
                               FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
  
  /* start the CAN peripheral. no need to evaluate the return value as there is nothing
   * we can do about a faulty CAN controller. */
  (void)HAL_FDCAN_Start(&canHandle);
} /*** end of CanInit ***/


/************************************************************************************//**
** \brief     Transmits a packet formatted for the communication interface.
** \param     data Pointer to byte array with data that it to be transmitted.
** \param     len  Number of bytes that are to be transmitted.
** \return    none.
**
****************************************************************************************/
void CanTransmitPacket(blt_int8u *data, blt_int8u len)
{
  blt_int32u            txMsgId = BOOT_COM_CAN_TX_MSG_ID;
  FDCAN_TxHeaderTypeDef txMsgHeader;
  blt_int32u            timeout;
  HAL_StatusTypeDef     status;
  blt_int32u            txMsgBuffer;

  /* configure the message that should be transmitted. */
  if ((txMsgId & 0x80000000) == 0)
  {
    /* set the 11-bit CAN identifier. */
    txMsgHeader.Identifier = txMsgId;
    txMsgHeader.IdType = FDCAN_STANDARD_ID;
  }
  else
  {
    /* negate the ID-type bit. */
    txMsgId &= ~0x80000000;
    /* set the 29-bit CAN identifier. */
    txMsgHeader.Identifier = txMsgId;
    txMsgHeader.IdType = FDCAN_EXTENDED_ID;
  }
  txMsgHeader.TxFrameType = FDCAN_DATA_FRAME;
  txMsgHeader.DataLength = len;
  txMsgHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  txMsgHeader.BitRateSwitch = FDCAN_BRS_OFF;
  txMsgHeader.FDFormat = FDCAN_CLASSIC_CAN;
  txMsgHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  txMsgHeader.MessageMarker = 0x52;

  /* add the message to the transmit FIFO to request its transmission. */
  status = HAL_FDCAN_AddMessageToTxFifoQ(&canHandle, &txMsgHeader, data);
  /* read out which FIFO buffer was used for the last transmit request. */
  txMsgBuffer = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(&canHandle);

  /* only continue with polling for transmit completion if the message transmit request
   * could be submitted to a valid FIFO buffer.
   */
  if ((status == HAL_OK) && (txMsgBuffer != 0))
  {
    /* determine timeout time for the transmit completion. */
    timeout = TimerGet() + CAN_MSG_TX_TIMEOUT_MS;
    /* poll for completion of the transmit operation. */
    while (HAL_FDCAN_IsTxBufferMessagePending(&canHandle, txMsgBuffer) != 0)
    {
      /* service the watchdog. */
      CopService();
      /* break loop upon timeout. this would indicate a hardware failure or no other
       * nodes connected to the bus.
       */
      if (TimerGet() > timeout)
      {
        break;
      }
    }
  }
} /*** end of CanTransmitPacket ***/


/************************************************************************************//**
** \brief     Receives a communication interface packet if one is present.
** \param     data Pointer to byte array where the data is to be stored.
** \param     len Pointer where the length of the packet is to be stored.
** \return    BLT_TRUE is a packet was received, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool CanReceivePacket(blt_int8u *data, blt_int8u *len)
{
  blt_bool              result = BLT_FALSE;
  blt_int32u            rxMsgId = BOOT_COM_CAN_RX_MSG_ID;
  FDCAN_RxHeaderTypeDef rxMsgHeader;
  HAL_StatusTypeDef     rxStatus = HAL_ERROR;

  /* check if the expected CAN message was received? */
  if (HAL_FDCAN_GetRxFifoFillLevel(&canHandle, FDCAN_RX_FIFO0) > 0)
  {
    /* attempt to read the newly received CAN message from its buffer. */
    rxStatus = HAL_FDCAN_GetRxMessage(&canHandle, FDCAN_RX_FIFO0, &rxMsgHeader, data);
  }

  /* only continue processing the CAN message if something was received. */
  if (rxStatus == HAL_OK)
  {
    /* check if this message has the configured CAN packet identifier. */
    if ((rxMsgId & 0x80000000) == 0)
    {
      /* was an 11-bit CAN message received that matches? */
      if ( (rxMsgHeader.Identifier == rxMsgId) &&
           (rxMsgHeader.IdType == FDCAN_STANDARD_ID) )
      {
        /* set flag that a packet with a matching CAN identifier was received. */
        result = BLT_TRUE;
      }
    }
    else
    {
      /* negate the ID-type bit. */
      rxMsgId &= ~0x80000000;
      /* was an 29-bit CAN message received that matches? */
      if ( (rxMsgHeader.Identifier == rxMsgId) &&
           (rxMsgHeader.IdType == FDCAN_EXTENDED_ID) )
      {
        /* set flag that a packet with a matching CAN identifier was received. */
        result = BLT_TRUE;
      }
    }
    /* store the data length. */
    if (result == BLT_TRUE)
    {
      *len = (blt_int8u)rxMsgHeader.DataLength;
    }
  }

  /* give the result back to the caller. */
  return result;
} /*** end of CanReceivePacket ***/
#endif /* BOOT_COM_CAN_ENABLE > 0 */


/*********************************** end of can.c **************************************/
