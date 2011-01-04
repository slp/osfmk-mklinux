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
 * $RCSfile: udptest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:40:54 $
 */


/*
 * Ping-pong test of UDP
 */
#include <xkern/include/xkernel.h>
#include <xkern/include/part.h>
#include <xkern/include/prot/udp.h>

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC udptest_init
#define TRACE_VAR traceudptestp
#define PROT_STRING "udp"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

static long	serverPort = 2001;

#define TRIPS 1000
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME


/* 
 * It may be convenient to send between different UDP ports on the
 * same host, so we'll have custom participant assignment
 */
#define CUSTOM_ASSIGN

static void
clientSetPart( 
	      Part p)
{
    partInit(p, 1);
    partPush(p[0], &ServerAddr, sizeof(IPhost));
    partPush(p[0], &serverPort, sizeof(long));
    /* 
     * If we don't specify the second participant, UDP will select a port
     * for us. 
     */
#if 0
    {
	long	clientPort = ANY_PORT;
	/* 
	 * NOTE -- if you use two participants, make sure the second argument to 
	 * the above call to partInit is 2
	 */
	partPush(p[1], ANY_HOST, 0);
	partPush(p[1], &clientPort, sizeof(long));
    }
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
  1, 1024, 2048, 4096, 8192, 16384
};

#define SAVE_SERVER_SESSN
#define INF_LOOP

#include <xkern/protocols/test/common_test.c>


static void
testInit( XObj self )
{
}
