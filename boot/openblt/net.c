/************************************************************************************//**
* \file         Source/net.c
* \brief        Bootloader TCP/IP network communication interface source file.
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

/****************************************************************************************
* Include files
****************************************************************************************/
#include "boot.h"                                /* bootloader generic header          */
#if (BOOT_COM_NET_ENABLE > 0)
#include "netdev.h"
#include "uip.h"
#include "uip_arp.h"
#endif


#if (BOOT_COM_NET_ENABLE > 0)
/****************************************************************************************
* Configuration macros
****************************************************************************************/
/* Extend the default time that the backdoor is open if firmware updates via TCP/IP
 * are supported. in this case an activation of the bootloader results in an
 * initialization of the ethernet MAC. when connected to the network via a router this
 * can take several seconds. Feel free to shorten/lengthen this time for finetuning,
 * Note that adding this configuration macro to blt_conf.h overrides the value here.
 */
#ifndef BOOT_COM_NET_BACKDOOR_EXTENSION_MS
#define BOOT_COM_NET_BACKDOOR_EXTENSION_MS   (10000)
#endif


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Delta time for the uIP periodic timer. */
#define NET_UIP_PERIODIC_TIMER_MS   (500)
/** \brief Delta time for the uIP ARP timer. */
#define NET_UIP_ARP_TIMER_MS        (10000)
/** \brief Macro for accessing the Ethernet header information in the buffer */
#define NET_UIP_HEADER_BUF          ((struct uip_eth_hdr *)&uip_buf[0])


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static void NetServerTask(void);


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Holds the time out value of the uIP periodic timer. */
static blt_int32u periodicTimerTimeOut;
/** \brief Holds the time out value of the uIP ARP timer. */
static blt_int32u ARPTimerTimeOut;
#if (BOOT_COM_NET_DHCP_ENABLE > 0)
/** \brief Holds the MAC address which is used by the DHCP client. */
static struct uip_eth_addr macAddress;
#endif
/** \brief Boolean flag to determine if the module was initialized or not. */
static blt_bool netInitializedFlag = BLT_FALSE;
#if (BOOT_COM_NET_DEFERRED_INIT_ENABLE == 1)
/** \brief Boolean flag initialized such that the normal initialization via NetInit()
 *         should be deferred. A called to NetDeferredInit() is need to do the actual
 *         initialization of this module.
 */
static blt_bool netInitializationDeferred = BLT_TRUE;
#else
/** \brief Boolean flag initialized such that the normal initialization via NetInit()
 *         proceeds as usual.
 */
static blt_bool netInitializationDeferred = BLT_FALSE;
#endif


/************************************************************************************//**
** \brief     Initializes the TCP/IP network communication interface.
** \return    none.
**
****************************************************************************************/
void NetInit(void)
{
  uip_ipaddr_t ipaddr;

  /* only perform the initialization if there is no request to defer it */
  if (netInitializationDeferred == BLT_FALSE)
  {
    /* initialize the network device */
    netdev_init();
    /* initialize the timer variables */
    periodicTimerTimeOut = TimerGet() + NET_UIP_PERIODIC_TIMER_MS;
    ARPTimerTimeOut = TimerGet() + NET_UIP_ARP_TIMER_MS;
    /* initialize the uIP TCP/IP stack. */
    uip_init();
#if (BOOT_COM_NET_DHCP_ENABLE == 0)
    /* set the IP address */
    uip_ipaddr(ipaddr, BOOT_COM_NET_IPADDR0, BOOT_COM_NET_IPADDR1, BOOT_COM_NET_IPADDR2,
               BOOT_COM_NET_IPADDR3);
    uip_sethostaddr(ipaddr);
    /* set the network mask */
    uip_ipaddr(ipaddr, BOOT_COM_NET_NETMASK0, BOOT_COM_NET_NETMASK1, BOOT_COM_NET_NETMASK2,
               BOOT_COM_NET_NETMASK3);
    uip_setnetmask(ipaddr);
    /* set the gateway address */
    uip_ipaddr(ipaddr, BOOT_COM_NET_GATEWAY0, BOOT_COM_NET_GATEWAY1, BOOT_COM_NET_GATEWAY2,
               BOOT_COM_NET_GATEWAY3);
    uip_setdraddr(ipaddr);
#else
    /* set the IP address */
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_sethostaddr(ipaddr);
    /* set the network mask */
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_setnetmask(ipaddr);
    /* set the gateway address */
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_setdraddr(ipaddr);
#endif
    /* start listening on the configured port for XCP transfers on TCP/IP */
    uip_listen(HTONS(BOOT_COM_NET_PORT));
    /* initialize the MAC and set the MAC address */
    netdev_init_mac();

#if (BOOT_COM_NET_DHCP_ENABLE > 0)
    /* initialize the DHCP client application and send the initial request. */
    netdev_get_mac(&macAddress.addr[0]);
    dhcpc_init(&macAddress.addr[0], 6);
    dhcpc_request();
#endif

    /* extend the time that the backdoor is open in case the default timed backdoor
     * mechanism is used.
     */
  #if (BOOT_BACKDOOR_HOOKS_ENABLE == 0)
    if (BackDoorGetExtension() < BOOT_COM_NET_BACKDOOR_EXTENSION_MS)
    {
      BackDoorSetExtension(BOOT_COM_NET_BACKDOOR_EXTENSION_MS);
    }
  #endif /* BOOT_BACKDOOR_HOOKS_ENABLE == 0 */
    /* set flag to indicate that we are now initialized. */
    netInitializedFlag = BLT_TRUE;
  }
} /*** end of NetInit ***/


#if (BOOT_COM_NET_DEFERRED_INIT_ENABLE == 1)
/************************************************************************************//**
** \brief     Performs a deferred initialization of the TCP/IP network communication
**            interface.
** \return    none.
**
****************************************************************************************/
void NetDeferredInit(void)
{
  /* reset the request to defer the initializaton */
  netInitializationDeferred = BLT_FALSE;
  /* perform the initialization if not yet initialized */
  if (netInitializedFlag == BLT_FALSE)
  {
    NetInit();
  }
} /*** end of NetDeferredInit ***/
#endif


/************************************************************************************//**
** \brief     Transmits a packet formatted for the communication interface.
** \param     data Pointer to byte array with data that it to be transmitted.
** \param     len  Number of bytes that are to be transmitted.
** \return    none.
**
****************************************************************************************/
void NetTransmitPacket(blt_int8u *data, blt_int8u len)
{
  uip_tcp_appstate_t *s;
  blt_int16u cnt;

  /* no need to send the packet if this module is not initialized */
  if (netInitializedFlag == BLT_TRUE)
  {
    /* get pointer to application state */
    s = &(uip_conn->appstate);

    /* add the dto counter first */
    *(blt_int32u *)&(s->dto_data[0]) = s->dto_counter;
    /* copy the actual XCP response */
    for (cnt=0; cnt<len; cnt++)
    {
      s->dto_data[cnt+4] = data[cnt];
    }
    /* set the length of the TCP/IP packet */
    s->dto_len = len + 4;
    /* set the flag to request the transmission of this packet. */
    s->dto_tx_req = BLT_TRUE;
    /* update dto counter for the next transmission */
    s->dto_counter++;
  }
} /*** end of NetTransmitPacket ***/


/************************************************************************************//**
** \brief     Receives a communication interface packet if one is present.
** \param     data Pointer to byte array where the data is to be stored.
** \param     len Pointer where the length of the packet is to be stored.
** \return    BLT_TRUE if a packet was received, BLT_FALSE otherwise.
**
****************************************************************************************/
blt_bool NetReceivePacket(blt_int8u *data, blt_int8u *len)
{
  /* no need to check for newly received packets if this module is not initialized */
  if (netInitializedFlag == BLT_TRUE)
  {
    /* run the TCP/IP server task function, which will handle the reception and
     * transmission of XCP packets
     */
    NetServerTask();
  }
  /* packet reception and transmission is completely handled by the NetServerTask so
   * always return BLT_FALSE here.
   */
  return BLT_FALSE;
} /*** end of NetReceivePacket ***/


/************************************************************************************//**
** \brief     The uIP network application that implements XCP on TCP/IP. Note that this
**            application make use of the fact that XCP is request/response based. So
**            no new request will come in when a response is pending for transmission,
**            if so, the transmission of the pending response is aborted.
** \return    none.
**
****************************************************************************************/
void NetApp(void)
{
  uip_tcp_appstate_t *s;
  blt_int8u *newDataPtr;

  /* get pointer to application state */
  s = &(uip_conn->appstate);

  if (uip_connected())
  {
    /* init the dto counter and reset the pending dto data length and transmit related
     * flags.
     */
    s->dto_counter = 1;
    s->dto_len = 0;
    s->dto_tx_req = BLT_FALSE;
    s->dto_tx_pending = BLT_FALSE;
    return;
  }

  if (uip_acked())
  {
    /* dto sent so reset the pending flag. */
    s->dto_tx_pending = BLT_FALSE;
  }

  if (uip_rexmit())
  {
    /* is a dto transmission pending that should now be retransmitted? */
    /* retransmit the currently pending dto response */
    if (s->dto_tx_pending == BLT_TRUE)
    {
      /* resend the last pending dto response */
      uip_send(s->dto_data, s->dto_len);
    }
  }

  if (uip_poll())
  {
    /* check if there is a packet waiting to be transmitted. this is done via polling
     * because then it is possible to asynchronously send data. otherwise data is
     * only really send after a newly received packet was received.
     */
    if (s->dto_tx_req == BLT_TRUE)
    {
      /* reset the transmit request flag. */
      s->dto_tx_req = BLT_FALSE;
      if (s->dto_len > 0)
      {
        /* set the transmit pending flag. */
        s->dto_tx_pending = BLT_TRUE;
        /* submit the data for transmission. */
        uip_send(s->dto_data, s->dto_len);
      }
    }
  }

  if (uip_newdata())
  {
    /* only process the data if its length is not longer than expected. otherwise just
     * ignore it. XCP is request/response. this means that a new requests should
     * only be processed when the response the the previous request was sent. new
     * requests before the last response was sent can therefore also be ignored.
     */
    if ( ((uip_datalen() - 4) <= BOOT_COM_NET_RX_MAX_DATA) &&
         (s->dto_tx_pending == BLT_FALSE) )
    {
      /* the first 4 bytes contain a counter value in which we are not really interested */
      newDataPtr = uip_appdata;
      XcpPacketReceived(&newDataPtr[4], (blt_int8u)(uip_datalen() - 4));
    }
  }
} /*** end of NetApp ***/


/************************************************************************************//**
** \brief     Runs the TCP/IP server task.
** \return    none.
**
****************************************************************************************/
static void NetServerTask(void)
{
  blt_int32u connection;
  blt_int32u packetLen;

  /* check for an RX packet and read it. */
  packetLen = netdev_read();
  if (packetLen > 0)
  {
    /* set uip_len for uIP stack usage */
    uip_len = (blt_int16u)packetLen;

    /* process incoming IP packets here. */
    if (NET_UIP_HEADER_BUF->type == htons(UIP_ETHTYPE_IP))
    {
      uip_arp_ipin();
      uip_input();
      /* if the above function invocation resulted in data that
       * should be sent out on the network, the global variable
       * uip_len is set to a value > 0.
       */
      if (uip_len > 0)
      {
        uip_arp_out();
        netdev_send();
        uip_len = 0;
      }
    }
    /* process incoming ARP packets here. */
    else if (NET_UIP_HEADER_BUF->type == htons(UIP_ETHTYPE_ARP))
    {
      uip_arp_arpin();

      /* if the above function invocation resulted in data that
       * should be sent out on the network, the global variable
       * uip_len is set to a value > 0.
       */
      if (uip_len > 0)
      {
        netdev_send();
        uip_len = 0;
      }
    }
  }

  /* process TCP/IP Periodic Timer here. */
  if (TimerGet() >= periodicTimerTimeOut)
  {
    periodicTimerTimeOut += NET_UIP_PERIODIC_TIMER_MS;
    for (connection = 0; connection < UIP_CONNS; connection++)
    {
      uip_periodic(connection);
      /* If the above function invocation resulted in data that
       * should be sent out on the network, the global variable
       * uip_len is set to a value > 0.
       */
      if (uip_len > 0)
      {
        uip_arp_out();
        netdev_send();
        uip_len = 0;
      }
    }

#if UIP_UDP
    for (connection = 0; connection < UIP_UDP_CONNS; connection++)
    {
      uip_udp_periodic(connection);
      /* If the above function invocation resulted in data that
       * should be sent out on the network, the global variable
       * uip_len is set to a value > 0.
       */
      if(uip_len > 0)
      {
        uip_arp_out();
        netdev_send();
        uip_len = 0;
      }
    }
#endif
  }

  /* process ARP Timer here. */
  if (TimerGet() >= ARPTimerTimeOut)
  {
    ARPTimerTimeOut += NET_UIP_ARP_TIMER_MS;
    uip_arp_timer();
  }

  /* perform polling operations here. */
  for (connection = 0; connection < UIP_CONNS; connection++)
  {
    uip_poll_conn(&uip_conns[connection]);
    /* If the above function invocation resulted in data that
     * should be sent out on the network, the global variable
     * uip_len is set to a value > 0.
     */
    if (uip_len > 0)
    {
      uip_arp_out();
      netdev_send();
      uip_len = 0;
    }
  }

} /*** end of NetServerTask ***/

#endif /* BOOT_COM_NET_ENABLE > 0 */


/*********************************** end of net.c **************************************/
