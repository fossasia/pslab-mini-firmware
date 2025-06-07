/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * This file is part of the uIP TCP/IP stack
 *
 * @(#)$Id: dhcpc.c,v 1.2 2006/06/11 21:46:37 adam Exp $
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "uip.h"
#include "dhcpc.h"
#include "uip_timer.h"
#include "pt.h"

#define STATE_INITIAL         0
#define STATE_SENDING         1
#define STATE_OFFER_RECEIVED  2
#define STATE_CONFIG_RECEIVED 3

static struct dhcpc_state s;

struct dhcp_msg
{
  u8_t op, htype, hlen, hops;
  u8_t xid[4];
  u16_t secs, flags;
  u8_t ciaddr[4];
  u8_t yiaddr[4];
  u8_t siaddr[4];
  u8_t giaddr[4];
  u8_t chaddr[16];
#ifndef UIP_CONF_DHCP_LIGHT
  u8_t sname[64];
  u8_t file[128];
#endif
  u8_t options[312];
};

#define BOOTP_BROADCAST 0x8000

#define DHCP_REQUEST        1
#define DHCP_REPLY          2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6
#define DHCP_MSG_LEN      236

#define DHCPC_SERVER_PORT  67
#define DHCPC_CLIENT_PORT  68

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_END         255

static const u8_t xid[4] = {0xad, 0xde, 0x12, 0x23};
static const u8_t magic_cookie[4] = {99, 130, 83, 99};

static void dhcpc_configured(const struct dhcpc_state *s);
static void dhcpc_unconfigured(void);

/*---------------------------------------------------------------------------*/
static u8_t *
add_msg_type(u8_t *optptr, u8_t type)
{
  *optptr++ = DHCP_OPTION_MSG_TYPE;
  *optptr++ = 1;
  *optptr++ = type;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_server_id(u8_t *optptr)
{
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, s.serverid, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_req_ipaddr(u8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_IPADDR;
  *optptr++ = 4;
  memcpy(optptr, s.ipaddr, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_req_options(u8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_LIST;
  *optptr++ = 3;
  *optptr++ = DHCP_OPTION_SUBNET_MASK;
  *optptr++ = DHCP_OPTION_ROUTER;
  *optptr++ = DHCP_OPTION_DNS_SERVER;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_end(u8_t *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static void
create_msg(register struct dhcp_msg *m)
{
  m->op = DHCP_REQUEST;
  m->htype = DHCP_HTYPE_ETHERNET;
  m->hlen = s.mac_len;
  m->hops = 0;
  memcpy(m->xid, xid, sizeof(m->xid));
  m->secs = 0;
  m->flags = HTONS(BOOTP_BROADCAST); /*  Broadcast bit. */
  /*  uip_ipaddr_copy(m->ciaddr, uip_hostaddr);*/
  memcpy(m->ciaddr, uip_hostaddr, sizeof(m->ciaddr));
  memset(m->yiaddr, 0, sizeof(m->yiaddr));
  memset(m->siaddr, 0, sizeof(m->siaddr));
  memset(m->giaddr, 0, sizeof(m->giaddr));
  memcpy(m->chaddr, s.mac_addr, s.mac_len);
  memset(&m->chaddr[s.mac_len], 0, sizeof(m->chaddr) - s.mac_len);
#ifndef UIP_CONF_DHCP_LIGHT
  memset(m->sname, 0, sizeof(m->sname));
  memset(m->file, 0, sizeof(m->file));
#endif

  memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}
/*---------------------------------------------------------------------------*/
static void
send_discover(void)
{
  u8_t *end;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

  create_msg(m);

  end = add_msg_type(&m->options[4], DHCPDISCOVER);
  end = add_req_options(end);
  end = add_end(end);

  uip_send(uip_appdata, end - (u8_t *)uip_appdata);
}
/*---------------------------------------------------------------------------*/
static void
send_request(void)
{
  u8_t *end;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

  create_msg(m);

  end = add_msg_type(&m->options[4], DHCPREQUEST);
  end = add_server_id(end);
  end = add_req_ipaddr(end);
  end = add_end(end);

  uip_send(uip_appdata, end - (u8_t *)uip_appdata);
}
/*---------------------------------------------------------------------------*/
static u8_t
parse_options(u8_t *optptr, int len)
{
  u8_t *end = optptr + len;
  u8_t type = 0;

  while (optptr < end)
  {
    switch (*optptr)
    {
      case DHCP_OPTION_SUBNET_MASK:
        memcpy(s.netmask, optptr + 2, 4);
        break;
      case DHCP_OPTION_ROUTER:
        memcpy(s.default_router, optptr + 2, 4);
        break;
      case DHCP_OPTION_DNS_SERVER:
        memcpy(s.dnsaddr, optptr + 2, 4);
        break;
      case DHCP_OPTION_MSG_TYPE:
        type = *(optptr + 2);
        break;
      case DHCP_OPTION_SERVER_ID:
        memcpy(s.serverid, optptr + 2, 4);
        break;
      case DHCP_OPTION_LEASE_TIME:
        memcpy(s.lease_time, optptr + 2, 4);
        break;
      case DHCP_OPTION_END:
        return type;
    }

    optptr += optptr[1] + 2;
  }
  return type;
}
/*---------------------------------------------------------------------------*/
static u8_t
parse_msg(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

  if (m->op == DHCP_REPLY &&
      memcmp(m->xid, xid, sizeof(xid)) == 0 &&
      memcmp(m->chaddr, s.mac_addr, s.mac_len) == 0)
  {
    memcpy(s.ipaddr, m->yiaddr, 4);
    return parse_options(&m->options[4], uip_datalen());
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_dhcp(void))
{
  unsigned long lease_renew_time;

  PT_BEGIN(&s.pt);

  /* try_again:*/
  s.state = STATE_SENDING;
  s.ticks = CLOCK_SECOND;

  do
  {
    send_discover();
    timer_set(&s.timer, s.ticks);
    PT_YIELD(&s.pt);
    PT_WAIT_UNTIL(&s.pt, uip_newdata() || timer_expired(&s.timer));

    if (uip_newdata() && parse_msg() == DHCPOFFER)
    {
      s.state = STATE_OFFER_RECEIVED;
      break;
    }

    if (s.ticks < CLOCK_SECOND * 60)
    {
      s.ticks *= 2;
    }
  }
  while (s.state != STATE_OFFER_RECEIVED);

  s.ticks = CLOCK_SECOND;

  do
  {
    send_request();
    timer_set(&s.timer, s.ticks);
    PT_YIELD(&s.pt);
    PT_WAIT_UNTIL(&s.pt, uip_newdata() || timer_expired(&s.timer));

    if (uip_newdata() && parse_msg() == DHCPACK)
    {
      s.state = STATE_CONFIG_RECEIVED;
      break;
    }

    if (s.ticks <= CLOCK_SECOND * 10)
    {
      s.ticks += CLOCK_SECOND;
    }
    else
    {
      PT_RESTART(&s.pt);
    }
  }
  while (s.state != STATE_CONFIG_RECEIVED);

  /* inform the application that a DHCP configuration was received such that it can
   * handle further processing of it.
   */
  dhcpc_configured(&s);

  /* keep renewing the lease each time 50% of the lease time expired. */
  while (1)
  {
    /* calculate 50% lease time in seconds. */
    lease_renew_time  = ((unsigned long)(ntohs(s.lease_time[0])) << 16u);
    lease_renew_time |= (ntohs(s.lease_time[1]));
    lease_renew_time /= 2;

    /* the internal timer (s.timer) is of type clock_time_t, so make sure the clock ticks
     * equivalent to 50% of the lease renewal time would actually fit. note that
     * clock_time_t is type defined as an int.
     */
    if ((lease_renew_time * CLOCK_SECOND) <= INT_MAX)
    {
      s.ticks = lease_renew_time * CLOCK_SECOND;
    }
    else
    {
      s.ticks = INT_MAX;
    }

    /* now wait for the timer to expire. */
    timer_set(&s.timer, s.ticks);
    PT_YIELD(&s.pt);
    PT_WAIT_UNTIL(&s.pt, timer_expired(&s.timer));
    /* since a lease is already available, there is no need to request a new offer via
     * the discover message. it suffices to sent the request message and wait for the
     * acknowledge.
     */
    s.state = STATE_OFFER_RECEIVED;
    s.ticks = CLOCK_SECOND;

    /* keep in mind that DHCP request messages must come for source address 0.0.0.0, so
     * reinitialize the DHCP request messages and inform the application that the
     * DHCP configuration is now temporarily reset until the lease was successfully
     * renewed.
     */
    dhcpc_request();
    dhcpc_unconfigured();

    do
    {
      send_request();
      timer_set(&s.timer, s.ticks);
      PT_YIELD(&s.pt);
      PT_WAIT_UNTIL(&s.pt, uip_newdata() || timer_expired(&s.timer));

      if (uip_newdata() && parse_msg() == DHCPACK)
      {
        /* renewal successful. */
        s.state = STATE_CONFIG_RECEIVED;
        break;
      }

      if (s.ticks <= CLOCK_SECOND * 10)
      {
        s.ticks += CLOCK_SECOND;
      }
      else
      {
        /* the lease could not be renewed, so restart this entire thread from the
         * begin to attempt to re-bind.
         */
        PT_RESTART(&s.pt);
      }
    }
    while (s.state != STATE_CONFIG_RECEIVED);

    /* inform the application that a DHCP configuration was received such that it can
     * handle further processing of it.
     */
    dhcpc_configured(&s);
  }

  /* code won't get here but this statement is still required at the end of a thread
   * function.
   */
  PT_END(&s.pt);
}
/*---------------------------------------------------------------------------*/
void
dhcpc_init(const void *mac_addr, int mac_len)
{
  uip_ipaddr_t addr;

  s.mac_addr = mac_addr;
  s.mac_len  = mac_len;

  s.state = STATE_INITIAL;
  uip_ipaddr(addr, 255,255,255,255);
  s.conn = uip_udp_new(&addr, HTONS(DHCPC_SERVER_PORT));
  if (s.conn != NULL)
  {
    uip_udp_bind(s.conn, HTONS(DHCPC_CLIENT_PORT));
  }
  PT_INIT(&s.pt);
}
/*---------------------------------------------------------------------------*/
void
dhcpc_appcall(void)
{
  handle_dhcp();
}
/*---------------------------------------------------------------------------*/
void
dhcpc_request(void)
{
  uip_ipaddr_t ipaddr;

  uip_ipaddr(ipaddr, 0,0,0,0);
  uip_sethostaddr(ipaddr);
}
/*---------------------------------------------------------------------------*/
void
dhcpc_configured(const struct dhcpc_state *s)
{
  /* set the IP address received from the DHCP server. */
  uip_sethostaddr(&s->ipaddr);
  /* set the network mask received from the DHCP server. */
  uip_setnetmask(&s->netmask);
  /* set the gateway address received from the DHCP server. */
  uip_setdraddr(&s->default_router);
}
/*---------------------------------------------------------------------------*/
void
dhcpc_unconfigured(void)
{
  /* nothing needs to be done here. */
}
/*---------------------------------------------------------------------------*/

