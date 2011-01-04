/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MkLinux
 */
/*     
 * $RCSfile: tcptest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:40:46 $
 */

/*
 * Ping-pong test of TCP
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/tcp.h>
#include <xkern/include/prot/ip.h>

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC tcptest_init
#define TRACE_VAR tracetcptestp
#define PROT_STRING "tcp"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

#define TRIPS 100
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME


long	serverPort = 2001;
long	clientPort = ANY_PORT;

#define CUSTOM_ASSIGN

static void
clientSetPart( 
	      Part p)
{
    partInit(p, 1);
    partPush(p[0], &ServerAddr, sizeof(IPhost));
    partPush(p[0], &serverPort, sizeof(long));
    /* 
     * If we don't specify the second participant, TCP will select a port
     * for us. 
     */
#if 0
    /* 
     * NOTE -- if you use two participants, make sure the second argument to 
     * the above call to partInit is 2
     */
    partPush(p[1], ANY_HOST, 0);
    partPush(p[1], &clientPort, sizeof(long));
#endif
}

static void
serverSetPart( 
	      Part p)
{
    partInit(p, 1);
    partPush(*p, ANY_HOST, 0);
    partPush(*p, &serverPort, sizeof(long));
}

static int lens[] = { 
  1, 4000, 8000, 16000
};


#define BUFFER_SIZE	4096

#define CUSTOM_OPENDONE
#define CLIENT_OPEN_DONE

static xkern_return_t
customOpenDone(
	       XObj self,
	       XObj s,
	       XObj llp,
	       XObj hlpType,
	       Path path)
{
    u_short space = BUFFER_SIZE;

    xTrace0(prottest, TR_MAJOR_EVENTS, "tcp test program openDone");
    xDuplicate(s);

    if (xControl(s, TCP_SETRCVBUFSIZE, (char*)&space, sizeof(space)) < 0) {
	xError("saveServerSessn: TCP_SETRCVBUFSIZE failed");
    } /* if */
    if (xControl(s, TCP_SETRCVBUFSPACE, (char*)&space, sizeof(space)) < 0){
	xError("saveServerSessn: TCP_SETRCVBUFSPACE failed");
    } /* if */

    return XK_SUCCESS;
}
    

#define CUSTOM_CLIENT_DEMUX

static xkern_return_t
customClientDemux(
		  XObj self,
		  XObj lls,
		  Msg dg,
		  int unused)
{
    u_short space = BUFFER_SIZE;

    xTrace1(prottest, TR_DETAILED, "TCP custom demux resets buffer to size %d",
	    BUFFER_SIZE);
    if (xControl(lls, TCP_SETRCVBUFSPACE, (char*)&space, sizeof(space)) < 0) {
	xError("TCP custom test_demux: TCP_SETRCVBUFSPACE failed");
    } /* if */
    return XK_SUCCESS;
}

#define CUSTOM_SERVER_DEMUX

static xkern_return_t
customServerDemux( 
		  XObj self,
		  XObj lls,
		  Msg dg,
		  int unused)
{
    return customClientDemux( self, lls, dg, unused );
}

#define STREAM_TEST

#include <xkern/protocols/test/common_test.c>

static void
testInit( XObj self )
{
}
