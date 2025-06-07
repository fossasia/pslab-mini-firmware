//*****************************************************************************
// HTTP server.
// Adam Dunkels <adam@dunkels.com>
//
// Copyright (c) 2001, Adam Dunkels.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote
//    products derived from this software without specific prior
//    written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file is part of the uIP TCP/IP stack.
//
//*****************************************************************************

#include "uip.h"
#include "httpd.h"

//*****************************************************************************
//
// Macro for easy access to buffer data
//
//*****************************************************************************
#define BUF_APPDATA ((u8_t *)uip_appdata)

//*****************************************************************************
//
// Definitions of HTTP Server States
//
//*****************************************************************************
#define HTTP_NOGET      0
#define HTTP_FILE       1
#define HTTP_TEXT       2
#define HTTP_FUNC       3
#define HTTP_END        4

//*****************************************************************************
//
// Global for keeping up with web server state.
//
//*****************************************************************************
struct httpd_state *hs;

//*****************************************************************************
//
// Default Web Page - allocated in three segments to allow easy update of a
// counter that is incremented each time the page is sent.
//
//*****************************************************************************
static const char page_not_found[] =
  "HTTP/1.0 404 OK\r\n"
  "Server: UIP/1.0 (http://www.sics.se/~adam/uip/)\r\n"
  "Content-type: text/html\r\n\r\n"
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd\">"
  "<html>"
  "<head>"
  "<title>Page Not Found!</title>"
  "</head>"
  "<body>"
  "Page Not Found!"
  "</body>"
  "</html>";

//*****************************************************************************
//
// Default Web Page - allocated in three segments to allow easy update of a
// counter that is incremented each time the page is sent.
//
//*****************************************************************************
static const char default_page_buf1of3[] =
  "HTTP/1.0 200 OK\r\n"
  "Server: UIP/1.0 (http://www.sics.se/~adam/uip/)\r\n"
  "Content-type: text/html\r\n\r\n"
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd\">"
  "<html>"
  "<head>"
  "<title>Welcome to the uIP web server!</title>"
  "</head>"
  "<body>"
  "<center>"
  "<h1>&micro;IP Web Server</h1>"
  "<p>This web page is served by a small web server running on top of "
  "the <a href=\"http://www.sics.se/~adam/uip/\">&micro;IP embedded TCP/IP "
  "stack</a>."
  "<hr width=\"75%\">"
  "<p>The &micro;IP stack is running on a "
  "<a href=\"http://www.ti.com\">Texas Instruments</a> "
  "Stellaris<small><sup>&reg;</sup></small> Ethernet Evaluation Kit"
  "<hr width=\"75%\">"
  "<p>This page has been sent ";
static char default_page_buf2of3[] =
  "00001";
static const char default_page_buf3of3[] =
  " times since reset!"
  "</center>"
  "</body>"
  "</html>";

//*****************************************************************************
//
// Increment the view count.  This routine will increment the 5-digit ascii
// counter that is sent with the web page.
//
//*****************************************************************************
static void
httpd_inc_page_count(void)
{
  //
  // Increment the 'ones' digit.  If it wraps, then increment the 'tens'
  // digit.
  //
  default_page_buf2of3[4]++;
  if (default_page_buf2of3[4] == 0x3a)
  {
    default_page_buf2of3[4] = 0x30;
    default_page_buf2of3[3]++;
  }

  //
  // If the 'tens' digit wraps, increment the 'hundreds' digit.
  //
  if (default_page_buf2of3[3] == 0x3a)
  {
    default_page_buf2of3[3] = 0x30;
    default_page_buf2of3[2]++;
  }

  //
  // If the 'hundreds' digit wraps, increment the 'thousands' digit.
  //
  if (default_page_buf2of3[2] == 0x3a)
  {
    default_page_buf2of3[2] = 0x30;
    default_page_buf2of3[1]++;
  }

  //
  // If the 'thousands' digit wraps, increment the 'ten-thousands' digit.
  //
  if (default_page_buf2of3[1] == 0x3a)
  {
    default_page_buf2of3[1] = 0x30;
    default_page_buf2of3[0]++;
  }

  //
  // If the 'ten-thousands' digit wrapped, start over.
  //
  if (default_page_buf2of3[0] == 0x3a)
  {
    default_page_buf2of3[0] = 0x30;
  }
}

//*****************************************************************************
//
// Initialize the web server.
//
// Starts to listen for incoming connection requests on TCP port 80.
//
//*****************************************************************************
void
httpd_init(void)
{
  //
  // Listen to port 80.
  //
  uip_listen(HTONS(80));
}

//*****************************************************************************
//
// HTTP Application Callback Function
//
//*****************************************************************************
void
httpd_appcall(void)
{
  switch (uip_conn->lport)
  {
    //
    // This is the web server:
    //
    case HTONS(80):
    {
      //
      // Pick out the application state from the uip_conn structure.
      //
      hs = (struct httpd_state *)&(uip_conn->appstate);

      //
      // We use the uip_ test functions to deduce why we were
      // called. If uip_connected() is non-zero, we were called
      // because a remote host has connected to us. If
      // uip_newdata() is non-zero, we were called because the
      // remote host has sent us new data, and if uip_acked() is
      // non-zero, the remote host has acknowledged the data we
      // previously sent to it. */
      if (uip_connected())
      {
        //
        // Since we have just been connected with the remote host, we
        // reset the state for this connection. The ->count variable
        // contains the amount of data that is yet to be sent to the
        // remote host, and the ->state is set to HTTP_NOGET to signal
        // that we haven't received any HTTP GET request for this
        // connection yet.
        //
        hs->state = HTTP_NOGET;
        hs->count = 0;
        return;
      }
      else if (uip_poll())
      {
        //
        // If we are polled ten times, we abort the connection. This is
        // because we don't want connections lingering indefinately in
        // the system.
        //
        if (hs->count++ >= 10)
        {
          uip_abort();
        }
        return;
      }
      else if (uip_newdata() && hs->state == HTTP_NOGET)
      {
        //
        // This is the first data we receive, and it should contain a
        // GET.
        //
        // Check for GET.
        //
        if (BUF_APPDATA[0] != 'G' ||
            BUF_APPDATA[1] != 'E' ||
            BUF_APPDATA[2] != 'T' ||
            BUF_APPDATA[3] != ' ')
        {
          //
          // If it isn't a GET, we abort the connection.
          //
          uip_abort();
          return;
        }

        //
        // Check to see what we should send.
        //
        if ((BUF_APPDATA[4] == '/') &&
            (BUF_APPDATA[5] == ' '))
        {
          //
          // Send buffer 1
          //
          uip_send(default_page_buf1of3,
                   sizeof(default_page_buf1of3) - 1);
        }
        else
        {
          uip_send(page_not_found,
                   sizeof(page_not_found) - 1);
          hs->count = 3;
        }
      }
      else if (uip_acked())
      {
        hs->count++;
        if (hs->count == 1)
        {
          uip_send(default_page_buf2of3,
                   sizeof(default_page_buf2of3) - 1);
        }
        else if (hs->count == 2)
        {
          uip_send(default_page_buf3of3,
                   sizeof(default_page_buf3of3) - 1);
        }
        else if (hs->count == 3)
        {
          httpd_inc_page_count();
          uip_close();
        }
        else
        {
          uip_close();
        }
      }
      //
      // Finally, return to uIP. Our outgoing packet will soon be on its
      // way...
      //
      return;
    }

    default:
    {
      //
      // Should never happen.
      //
      uip_abort();
      break;
    }
  }
}
