/************************************************************************************//**
* \file         Demo/ARMCM33_STM32H5_Nucleo_H563ZI_IAR/Boot/lib/uip/netdev.c
* \brief        uIP network device port source file.
* \ingroup      Boot_ARMCM33_STM32H5_Nucleo_H563ZI_IAR
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2001, Swedish Institute of Computer Science. All rights reserved.
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* 3. Neither the name of the Institute nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* Author: Adam Dunkels <adam@sics.se>
*
* This file is part of the uIP TCP/IP stack.
*
* \endinternal
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <string.h>                              /* String utils (memcpy)              */
#include "uip.h"                                 /* uIP TCP/IP stack                   */
#include "uip_arp.h"                             /* uIP address resolution protocol    */
#include "boot.h"                                /* Bootloader generic header          */
#include "lan8742.h"                             /* LAN8742 PHY driver                 */
#include "stm32h5xx.h"                           /* STM32 CPU and HAL header           */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/* The MAC address for the network interface. */
#define NETDEV_DEFAULT_MACADDR0           (0x08)
#define NETDEV_DEFAULT_MACADDR1           (0x00)
#define NETDEV_DEFAULT_MACADDR2           (0x27)
#define NETDEV_DEFAULT_MACADDR3           (0x69)
#define NETDEV_DEFAULT_MACADDR4           (0x5B)
#define NETDEV_DEFAULT_MACADDR5           (0x45)

/* Timeout time for transmitting a packet. */
#define NETDEV_TX_PACKET_TIMEOUT_MS       (250U)


/****************************************************************************************
* Type definitions
****************************************************************************************/
/* Rx buffer type. The data element address and size must be 32 byte aligned on this
 * STM32. Note that the data element size must be passed to the ETH DMA in the init field
 * (heth.Init.RxBuffLen).
 */
typedef struct t_rx_buf
{
  /* Pointer to the next Rx buffer in single linked chain. */
  struct t_rx_buf * next;
  /* Number of data bytes in this chain link. */
  uint16_t length;
  /* Total number of data bytes in the entire chain. */
  uint16_t total_length;
  /* Data byte buffer for the bytes in this chain link. */
  uint8_t data[(UIP_CONF_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} tRxBuf;

/* Tx buffer type. The data element address and size must be 32 byte aligned on this
 * STM32.
 */
typedef struct t_tx_buf
{
  /* Data byte buffer. */
  uint8_t data[(UIP_CONF_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} tTxBuf;


/****************************************************************************************
* Global data declarations
****************************************************************************************/
/* Ethernet handle used by the HAL drivers. */
ETH_HandleTypeDef  heth;

/* Ethernet Rx DMA Descriptors. */
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT];

/* Ethernet Tx DMA Descriptors. */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];


/****************************************************************************************
* Function prototypes
****************************************************************************************/
/* Ethernet PHY I/O routines. */
static int32_t   ETH_PHY_IO_Init    (void);

static int32_t   ETH_PHY_IO_DeInit  (void);

static int32_t   ETH_PHY_IO_ReadReg (uint32_t   devAddr,
                                     uint32_t   regAddr,
                                     uint32_t * regValPtr);

static int32_t   ETH_PHY_IO_WriteReg(uint32_t   devAddr,
                                     uint32_t   regAddr,
                                     uint32_t   regVal);

static int32_t   ETH_PHY_IO_GetTick (void);

/* Rx buffer management routines. */
static uint8_t * rxbuf_alloc        (void);

static void      rxbuf_free         (uint8_t  * rx_buf_data);

/* Tx buffer management routines. */
static uint8_t * txbuf_alloc        (void);

static void      txbuf_free         (uint8_t  * tx_buf_data);


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/* MAC adress buffer. */
static struct uip_eth_addr macAddress;

/* LAN8742 PHY component object handle. */
static lan8742_Object_t    lan8742Object;

/* LAN8742 PHY context with I/O function pointers. */
static lan8742_IOCtx_t     lan8742IOCtx =
{
  ETH_PHY_IO_Init,
  ETH_PHY_IO_DeInit,
  ETH_PHY_IO_WriteReg,
  ETH_PHY_IO_ReadReg,
  ETH_PHY_IO_GetTick
};

/* Rx buffer pool. Must be greater than ETH_RX_DESC_CNT yet smaller than 32,
 * otherwise there are not enough rx buffer allocation status flags.
 */
static tRxBuf              rxBufs[ETH_RX_DESC_CNT * 2U];

/* Rx buffer allocation status flags. */
static uint32_t            rxBufAllocFlags = 0U;

/* Tx buffer pool. Ideally greater than 1 yet smaller than 32, otherwise there are not
 * enough tx buffer allocation status flags.
 */
static tTxBuf              txBufs[2U];

/* Rx buffer allocation status flags. */
static uint32_t            txBufAllocFlags = 0U;


/****************************************************************************************
*            N E T W O R K   D E V I C E   R O U T I N E S
****************************************************************************************/
/************************************************************************************//**
** \brief     Initializes the network device.
**
****************************************************************************************/
void netdev_init(void)
{
  int32_t              phyLinkState;
  uint32_t             duplex;
  uint32_t             speed;
  ETH_MACConfigTypeDef macConf = {0};

  /* Store the default MAC address. */
  macAddress.addr[0] = NETDEV_DEFAULT_MACADDR0;
  macAddress.addr[1] = NETDEV_DEFAULT_MACADDR1;
  macAddress.addr[2] = NETDEV_DEFAULT_MACADDR2;
  macAddress.addr[3] = NETDEV_DEFAULT_MACADDR3;
  macAddress.addr[4] = NETDEV_DEFAULT_MACADDR4;
  macAddress.addr[5] = NETDEV_DEFAULT_MACADDR5;

  /* Initialize Ethernet. */
  heth.Instance = ETH;
  heth.Init.MACAddr = &macAddress.addr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = (UIP_CONF_BUFFER_SIZE + 31) & ~31;
  (void)HAL_ETH_Init(&heth);

  /* Link the PHY I/O functions to the component object and initialize the PHY. */
  LAN8742_RegisterBusIO(&lan8742Object, &lan8742IOCtx);
  LAN8742_Init(&lan8742Object);

  /* Obtain the current link state from the PHY. */
  phyLinkState = LAN8742_GetLinkState(&lan8742Object);

  /* Filter on the state to retrieve the speed and half/full-duplex properties. */
  switch (phyLinkState)
  {
    case LAN8742_STATUS_100MBITS_FULLDUPLEX:
      duplex = ETH_FULLDUPLEX_MODE;
      speed = ETH_SPEED_100M;
      break;
    case LAN8742_STATUS_100MBITS_HALFDUPLEX:
      duplex = ETH_HALFDUPLEX_MODE;
      speed = ETH_SPEED_100M;
      break;
    case LAN8742_STATUS_10MBITS_FULLDUPLEX:
      duplex = ETH_FULLDUPLEX_MODE;
      speed = ETH_SPEED_10M;
      break;
    case LAN8742_STATUS_10MBITS_HALFDUPLEX:
      duplex = ETH_HALFDUPLEX_MODE;
      speed = ETH_SPEED_10M;
      break;
    default:
      duplex = ETH_FULLDUPLEX_MODE;
      speed = ETH_SPEED_100M;
      break;
  }

  /* Get and update the current MAC configuration. */
  HAL_ETH_GetMACConfig(&heth, &macConf);
  macConf.DuplexMode = duplex;
  macConf.Speed = speed;
  HAL_ETH_SetMACConfig(&heth, &macConf);

  /* Kick of the Ethernet packet communication in polling mode. */
  HAL_ETH_Start(&heth);
} /*** end of netdev_init ***/


/************************************************************************************//**
** \brief     Initializes the MAC address of the network device.
**
****************************************************************************************/
void netdev_init_mac(void)
{
  /* Configure the MAC address */
  uip_setethaddr(macAddress);
} /*** end of netdev_init_mac ***/


/************************************************************************************//**
** \brief     Obtains the MAC address of the network device.
** \param     mac_addr MAC address storage buffer as a byte array.
**
****************************************************************************************/
void netdev_get_mac(unsigned char * mac_addr)
{
  mac_addr[0] = macAddress.addr[0];
  mac_addr[1] = macAddress.addr[1];
  mac_addr[2] = macAddress.addr[2];
  mac_addr[3] = macAddress.addr[3];
  mac_addr[4] = macAddress.addr[4];
  mac_addr[5] = macAddress.addr[5];
} /*** end of netdev_get_mac ***/


/************************************************************************************//**
** \brief     Read newly received data from the network device.
** \details   Copy the newly received data to the uip_buf byte buffer. Keep in mind its
**            maximum size (UIP_CONF_BUFFER_SIZE).
** \return    Number of newly received bytes that were copied to uip_buf or 0 if no new
**            data was received.
**
****************************************************************************************/
unsigned int netdev_read(void)
{
  unsigned int   result = 0;
  tRxBuf       * rxBuf  = NULL;

  /* Check for newly receive data. Note this function triggers HAL_ETH_RxLinkCallback()
   * one or more times to build a chain link of tRxBufs. If new data was available,
   * the pointer to the first chain in the link is stored in rxBuf.
   */
  if (HAL_ETH_ReadData(&heth, (void **)&rxBuf) == HAL_OK)
  {
    /* Only continue with a valid buffer pointer, meaning that a chain link was built. */
    if (rxBuf != NULL)
    {
      /* Iterate over all the links in the chain. */
      for (tRxBuf * p = rxBuf; p != NULL; p = p->next)
      {
        /* Only copy the data for later processing if it actually fits in the uip_buf
         * and if the length and total length elements are valid.
         */
        if ((p->length > 0U) && (p->total_length > 0U) &&
            (p->length <= p->total_length) && (p->total_length <= UIP_CONF_BUFFER_SIZE))
        {
          /* Append the data from this chain link to the uIP data buffer. */
          memcpy(&uip_buf[result], p->data, p->length);
          /* Update the result. */
          result += p->length;
        }
        /* Release the allocated Rx buffer now that we're done with it. Keep in mind
         * that it's freed by specifying the address of its data pointer.
         */
        rxbuf_free(p->data);
      }
    }
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of netdev_read ***/


/************************************************************************************//**
** \brief     Send data on the network device.
** \details   The data to set in available in the uip_buf byte buffer and the number of
**            bytes to send is stored in uip_len.
**
****************************************************************************************/
void netdev_send(void)
{
  ETH_TxPacketConfigTypeDef   txPacketConfig = {0};
  ETH_BufferTypeDef           txBuffer       = {0};
  uint8_t                   * txData;

  /* Allocate a new tx data buffer from the pool. */
  txData = txbuf_alloc();

  /* Only continue if the allocation succeeded. */
  if (txData != NULL)
  {
    /* Copy the packet data to the tx data buffer. */
    memcpy(txData, uip_buf, uip_len);

    /* Prepare the transmit buffer. This implementation assumes that a single ETH
     * transmit descriptor can handle the packet with a size up to UIP_CONF_BUFFER_SIZE.
     * Note that the STM32 Ethernet peripheral can handle up to 32k bytes per ETH
     * transmit descriptor.
     */
    txBuffer.buffer = txData;
    txBuffer.len    = uip_len;
    txBuffer.next   = NULL;

    /* Prepare transmit packet configuration. */
    txPacketConfig.Attributes   = ETH_TX_PACKETS_FEATURES_CSUM |
                                  ETH_TX_PACKETS_FEATURES_CRCPAD;
    txPacketConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    txPacketConfig.CRCPadCtrl   = ETH_CRC_PAD_INSERT;
    txPacketConfig.Length       = uip_len;
    txPacketConfig.TxBuffer     = &txBuffer;
    txPacketConfig.pData        = txBuffer.buffer;

    /* Submit request to transmit the Ethernet Packet in polling mode. */
    HAL_ETH_Transmit(&heth, &txPacketConfig, NETDEV_TX_PACKET_TIMEOUT_MS);

    /* Release the previously allocated tx data buffer back to the pool. Keep in mind
     * that it's freed by specifying the address of its data pointer.
     */
    txbuf_free(txData);
  }
} /*** end of netdev_send ***/


/****************************************************************************************
*            R X   B U F F E R   M A N A G E M E N T   R O U T I N E S
****************************************************************************************/
/************************************************************************************//**
** \brief     Allocate a new Rx buffer from the pool.
** \return    Pointer to the allocated Rx buffer's data element if successful, NULL
**            otherwise.
**
****************************************************************************************/
static uint8_t * rxbuf_alloc(void)
{
  uint8_t *     result      = NULL;
  const uint8_t totalRxBufs = sizeof(rxBufs)/sizeof(rxBufs[0]);

  /* Find the next free buffer. */
  for (uint8_t rxBufIdx = 0; rxBufIdx < totalRxBufs; rxBufIdx++)
  {
    /* Is the buffer at this index available? */
    if ((rxBufAllocFlags & (1 << rxBufIdx)) == 0U)
    {
      /* Flag this one as allocated. */
      rxBufAllocFlags |= (1 << rxBufIdx);
      /* Update the result. */
      result = rxBufs[rxBufIdx].data;
      /* Stop the loop. */
      break;
    }
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of rxbuf_alloc ***/


/************************************************************************************//**
** \brief     Releases a previously allocated Rx buffer and give it back to the pool.
** \param     rx_buf_data Pointer to the allocated Rx buffer's data element.
**
****************************************************************************************/
static void rxbuf_free(uint8_t * rx_buf_data)
{
  const uint8_t totalRxBufs = sizeof(rxBufs)/sizeof(rxBufs[0]);

  /* Find the index of the specified buffer in the pool. */
  for (uint8_t rxBufIdx = 0; rxBufIdx < totalRxBufs; rxBufIdx++)
  {
    /* Is this the one that should be released? */
    if (rxBufs[rxBufIdx].data == rx_buf_data)
    {
      /* Flag this one as free. */
      rxBufAllocFlags &= ~(1 << rxBufIdx);
      /* Stop the loop. */
      break;
    }
  }
} /*** end of rxbuf_free ***/


/****************************************************************************************
*            T X   B U F F E R   M A N A G E M E N T   R O U T I N E S
****************************************************************************************/
/************************************************************************************//**
** \brief     Allocate a new Tx buffer from the pool.
** \return    Pointer to the allocated Tx buffer's data element if successful, NULL
**            otherwise.
**
****************************************************************************************/
static uint8_t * txbuf_alloc(void)
{
  uint8_t *     result      = NULL;
  const uint8_t totalTxBufs = sizeof(txBufs)/sizeof(txBufs[0]);

  /* Find the next free buffer. */
  for (uint8_t txBufIdx = 0; txBufIdx < totalTxBufs; txBufIdx++)
  {
    /* Is the buffer at this index available? */
    if ((txBufAllocFlags & (1 << txBufIdx)) == 0U)
    {
      /* Flag this one as allocated. */
      txBufAllocFlags |= (1 << txBufIdx);
      /* Update the result. */
      result = txBufs[txBufIdx].data;
      /* Stop the loop. */
      break;
    }
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of txbuf_alloc ***/


/************************************************************************************//**
** \brief     Releases a previously allocated Tx buffer and give it back to the pool.
** \param     tx_buf_data Pointer to the allocated Tx buffer's data element.
**
****************************************************************************************/
static void txbuf_free(uint8_t * tx_buf_data)
{
  const uint8_t totalTxBufs = sizeof(txBufs)/sizeof(txBufs[0]);

  /* Find the index of the specified buffer in the pool. */
  for (uint8_t txBufIdx = 0; txBufIdx < totalTxBufs; txBufIdx++)
  {
    /* Is this the one that should be released? */
    if (txBufs[txBufIdx].data == tx_buf_data)
    {
      /* Flag this one as free. */
      txBufAllocFlags &= ~(1 << txBufIdx);
      /* Stop the loop. */
      break;
    }
  }
} /*** end of txbuf_free ***/


/****************************************************************************************
*            H A L   C A L L B A C K   R O U T I N E S
****************************************************************************************/
/************************************************************************************//**
** \brief     Callback to allocate a packet reception buffer.
** \param     buff Pointer for storing the allocation results.
**
****************************************************************************************/
void HAL_ETH_RxAllocateCallback(uint8_t ** buff)
{
  /* Allocate a new Rx buffer from the pool. */
  *buff = rxbuf_alloc();
} /*** end of HAL_ETH_RxAllocateCallback ***/


/************************************************************************************//**
** \brief     Callback to process newly received data.
** \param     pStart pointer to packet start
** \param     pEnd pointer to packet end
** \param     buff pointer to received data
** \param     length received data length
**
****************************************************************************************/
void HAL_ETH_RxLinkCallback(void     ** pStart,
                            void     ** pEnd,
                            uint8_t  *  buff,
                            uint16_t    length)
{
  tRxBuf ** ppStart = (tRxBuf **)pStart;
  tRxBuf ** ppEnd   = (tRxBuf **)pEnd;
  tRxBuf *  p;

  /* Get the tRxBuf pointer from the buff address. */
  p = (tRxBuf *)(buff - offsetof(tRxBuf, data));
  p->next = NULL;
  p->total_length = 0;
  p->length = length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd = p;

  /* Update the total length of all the buffers of the chain. Each link in the chain
   * should have its total_length set to its own length, plus the length of all the
   * following links in the chain.
   */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->total_length += length;
  }
} /*** end of HAL_ETH_RxLinkCallback ***/


/****************************************************************************************
*            H A L   M S P   R O U T I N E S
****************************************************************************************/
/************************************************************************************//**
** \brief     ETH MSP Initialization.
** \details   This function configures the hardware resources used in this example.
** \param     heth ETH handle pointer.
**
****************************************************************************************/
void HAL_ETH_MspInit(ETH_HandleTypeDef* heth)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(heth->Instance==ETH)
  {
    /* Peripheral clock enable */
    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_ETHTX_CLK_ENABLE();
    __HAL_RCC_ETHRX_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB15     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  }
} /*** end of HAL_ETH_MspInit ***/


/************************************************************************************//**
** \brief     ETH MSP De-Initialization.
** \details   This function freeze the hardware resources used in this example.
** \param     heth ETH handle pointer.
**
****************************************************************************************/
void HAL_ETH_MspDeInit(ETH_HandleTypeDef* heth)
{
  if(heth->Instance==ETH)
  {
    /* Peripheral clock disable */
    __HAL_RCC_ETH_CLK_DISABLE();
    __HAL_RCC_ETHTX_CLK_DISABLE();
    __HAL_RCC_ETHRX_CLK_DISABLE();
    /* Reset Ethernet GPIO pin configuration. */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_15);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11|GPIO_PIN_13);
  }
} /*** end of HAL_ETH_MspDeInit ***/


/****************************************************************************************
*            E T H E R N E T   P H Y   I / O   R O U T I N E S
****************************************************************************************/
/************************************************************************************//**
** \brief     Initializes the MDIO interface GPIO and clocks.
** \details   This function assumes that the MDIO GPIO configuration is already done.
** \return    0 if successful, -1 otherwise.
**
****************************************************************************************/
static int32_t ETH_PHY_IO_Init(void)
{
  /* Configure the MDIO Clock. */
  HAL_ETH_SetMDIOClockRange(&heth);
  return 0;
} /*** end of ETH_PHY_IO_Init ***/


/************************************************************************************//**
** \brief     De-Initializes the MDIO interface.
** \return    0 if successful, -1 otherwise.
**
****************************************************************************************/
static int32_t ETH_PHY_IO_DeInit(void)
{
  /* Nothing to do for this specific PHY. */
  return 0;
} /*** end of ETH_PHY_IO_DeInit ***/


/************************************************************************************//**
** \brief     Read a PHY register through the MDIO interface.
** \param     devAddr PHY port address
** \param     regAddr PHY register address
** \param     regValPtr Pointer to hold the register value.
** \return    0 if successful, -1 otherwise.
**
****************************************************************************************/
static int32_t ETH_PHY_IO_ReadReg(uint32_t   devAddr,
                                  uint32_t   regAddr,
                                  uint32_t * regValPtr)
{
  int32_t result = 0;

  if (HAL_ETH_ReadPHYRegister(&heth, devAddr, regAddr, regValPtr) != HAL_OK)
  {
    result = -1;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of ETH_PHY_IO_ReadReg ***/


/************************************************************************************//**
** \brief     Write a value to a PHY register through the MDIO interface.
** \param     devAddr PHY port address
** \param     regAddr PHY register address
** \param     regVal Value to be written.
** \return    0 if successful, -1 otherwise.
**
****************************************************************************************/
static int32_t ETH_PHY_IO_WriteReg(uint32_t devAddr,
                                   uint32_t regAddr,
                                   uint32_t regVal)
{
  int32_t result = 0;

  if (HAL_ETH_WritePHYRegister(&heth, devAddr, regAddr, regVal) != HAL_OK)
  {
    result = -1;
  }
  /* Give the result back to the caller. */
  return result;
} /*** end of ETH_PHY_IO_WriteReg ***/


/************************************************************************************//**
** \brief     Get the time in milliseconds used for internal PHY driver process.
** \return    Current system time in milliseconds.
**
****************************************************************************************/
static int32_t ETH_PHY_IO_GetTick(void)
{
  return TimerGet();
} /*** end of ETH_PHY_IO_GetTick ***/


/*********************************** end of netdev.c ***********************************/
