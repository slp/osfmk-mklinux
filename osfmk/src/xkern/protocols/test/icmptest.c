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
 * icmptest.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * icmptest.c,v
 * Revision 1.24.1.2.1.2  1994/09/07  04:18:37  menze
 * OSF modifications
 *
 * Revision 1.24.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.24.1.1  1994/07/22  19:57:48  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.24  1993/12/16  01:49:02  menze
 * Fixed type of 'client' function to conform to event interface
 *
 */

/*
 * This protocol is a ping-pong test for the ICMP echo request and reply
 * messages.
 *
 * The same protocol code is used for both the client and the server.  
 * (The server is null since ICMP automatically sends the echoes.)
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/trace.h>
#include <xkern/include/prot/icmp.h>

static IPhost myIpHost;

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static IPhost ServerAddr = { 0, 0, 0, 0 };
static IPhost ClientAddr = { 0, 0, 0, 0 };

static XObj myProtl;

static int count;

#define TRIPS 10
#define TIMES 2
#define DELAY 3

#define NEW_IP_EQUAL(_a, _b) ((_a.a == _b.a) && (_a.b == _b.b) && \
			      (_a.c == _b.c) && (_a.d == _b.d))

xkern_return_t	icmptest_init( XObj );
static void	client( Event, void * );
static int	isClient( void );


static int
testShouldRun( void )
{
    int	testsSpecified = 0;
    int	i;

    for (i=0; i < globalArgc; i++) {
	if ( strncmp(globalArgv[i], "-test", strlen("-test")) == 0) {
	    testsSpecified = 1;
	    if ( strcmp(globalArgv[i], "-testicmp") == 0 ) {
		return 1;
	    }
	}
    }
    /* 
     * If we got here, our test was not specified.  Run only if no other
     * tests were specified.
     */
    return ! testsSpecified;
}


xkern_return_t
icmptest_init( self )
    XObj 	self;
{
    XObj 	ipProtl;
    
    if ( ! testShouldRun() ) {
	xTrace0(prottest, TR_FUNCTIONAL_TRACE, "Parameters indicate icmp test should not run");
	return 0;
    }
    printf("icmpTest_init\n");
    myProtl = self;
    if ((ipProtl = xGetProtlByName("ip")) == ERR_XOBJ) {
	xTrace0(prottest, TR_ALWAYS, "Couldn't get IP protocol");
	return XK_FAILURE;
    }
    if (xControl(ipProtl, GETMYHOST, (char *)&myIpHost, sizeof(IPhost)) < 0) {
	xTrace0(prottest, TR_ALWAYS, "Couldn't get my IP address");
	return XK_FAILURE;
    }
    xTrace1(prottest, TR_GROSS_EVENTS, "icmpTest: My ip addr is <%s>",
	    ipHostStr(&myIpHost));
    if ( isClient() ) {
	Event	ev;

	if ( ! (ev = evAlloc(self->path)) ) {
	    xTraceP0(self, TR_ERRORS, "allocation error");
	    return XK_FAILURE;
	}
	evDetach(evSchedule(ev, client, 0, 0));
    } else {
	xTrace0(prottest, TR_ALWAYS, "icmpTest: I am not the client");
    }
    return 0;
}


static int
isClient()
{
    return NEW_IP_EQUAL(myIpHost, ClientAddr);
}


static void
client( ev, arg )
    Event	ev;
    void	*arg;
{
    XObj s;
    Part_s p;
    int test = 0;
#if XK_DEBUG
    int	total = 0;
#endif /* XK_DEBUG */   
    int lenindex, len, i;
    
    static int lens[] = { 
	1, 1000, 2000, 4000, 8000, 16000
      };
    
    s = NULL;
    printf("I am the client, talking to <%s>\n", ipHostStr(&ServerAddr));
    
    partInit(&p, 1);
    partPush(p, &ServerAddr, sizeof(IPhost));
    
    s = xOpen(myProtl, myProtl, xGetDown(myProtl, 0), &p, xMyPath(myProtl));
    if ( s == ERR_XOBJ ) {
	xTrace0(prottest, TR_ERRORS, "Not sending, could not open session");
	return;
    } else {
#ifdef INF_LOOP
	for (lenindex = 0; ; lenindex++) {
	    if (lenindex >= sizeof(lens)/sizeof(long)) lenindex = 0;
#else
	for (lenindex = 0; lenindex < sizeof(lens)/sizeof(long); lenindex++) {
#endif
	    len = lens[lenindex];
	    for (test = 0; test < TIMES; test++) {
		xTrace2(prottest, TR_MAJOR_EVENTS, "Sending (%d)  len = %d ...\n",
			++total, len);
		for (i=0; i < TRIPS; i++) {
		    if (xControl(s, ICMP_ECHO_REQ, (char *)&len,
				 sizeof(len))) {
			xTrace1(prottest, TR_ERRORS, "\nfailed after %d trips", i);
			break;
		    }
		    xIfTrace(prottest, TR_MAJOR_EVENTS) {
			putchar('.');
			if (! (++count % 50)) {
			    putchar('\n');
			}
		    }
		}
		xTrace1(prottest, 1, "\n%d round trips completed", i);
		Delay(DELAY * 1000);
	    }
	}
#ifdef ABORT
        Kabort("End of test");
#endif
        return;
    }
}


