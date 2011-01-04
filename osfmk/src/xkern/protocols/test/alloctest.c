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
 * alloctest.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * alloctest.c,v
 * Revision 1.1.1.2.1.2  1994/09/07  04:18:38  menze
 * OSF modifications
 *
 * Revision 1.1.1.2  1994/09/01  18:59:18  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 * Fixed some rom-option processing bugs
 *
 * Revision 1.1  1994/08/22  21:13:06  menze
 * Initial revision
 *
 */


/* 
 * A simplified version of CHANtest (client)
 */

#include <xkern/include/xkernel.h>

#define TEST_STACK_SIZE	200
#define FAILURE_LIMIT	3
#define PROT_STRING	"alloctest"
#define DELAY 3

XObjInitFunc	alloctest_init;

static IPhost	defaultServer = { 0, 0, 0, 0 }; /* beretta */

static int	xk_trips = 100;
static int	lens[] = {
    1, 8 * 1024,  
    1, 1024, 2 * 1024, 4 * 1024, 8 * 1024, 16 * 1024,
    1, 1024, 2 * 1024, 4 * 1024, 8 * 1024, 16 * 1024
};

/* 
 * Client only
 */
typedef struct {
    char 	mark;
    char	loop;
    XObj	lls;
    int		delay;
    int		isServer;
    IPhost	serverHost;
} PState;

int	tracealloctestp;

static int saveInterval = 15;	/* server only */



#ifdef MACH_KERNEL

extern volatile int x_server;
extern volatile int x_client;

#else

static int	x_server = 1;

#endif /* MACH_KERNEL */


/* 
 * instName client iphostOfServer
 */
static xkern_return_t
readClientOpt( XObj self, char **str, int nFields, int line, void *arg )
{
    PState	*ps = arg;

    if ( nFields > 2 ) {
	if ( str2ipHost(&ps->serverHost, str[2]) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    ps->isServer = 0;
    return XK_SUCCESS;
}


/* 
 * instName server
 */
static xkern_return_t
readServerOpt( XObj self, char **str, int nFields, int line, void *arg )
{
    ((PState *)arg)->isServer = 1;
    return XK_SUCCESS;
}


/* 
 * instName delay int
 */
static xkern_return_t
readDelay( XObj self, char **str, int nFields, int line, void *arg )
{
    int	N;

    if ( (N = atoi(str[2])) < 0 ) {
	return XK_FAILURE;
    } else {
	((PState *)arg)->delay = N;
	return XK_SUCCESS;
    }
}


/* 
 * instName save_interval int
 */
static xkern_return_t
readInterval( XObj self, char **str, int nFields, int line, void *arg )
{
    *(int *)arg = atoi(str[2]);
    return XK_SUCCESS;
}


/* 
 * instName mark char
 */
static xkern_return_t
readMark( XObj self, char **str, int nFields, int line, void *arg )
{
    ((PState *)arg)->mark = *str[2];
    return XK_SUCCESS;
}


/* 
 * instName trips N
 */
static xkern_return_t
readTrips( XObj self, char **str, int nFields, int line, void *arg )
{
    int	N;

    if ( (N = atoi(str[2])) < 0 ) {
	return XK_FAILURE;
    } else {
	xk_trips = N;
	return XK_SUCCESS;
    }
}


/* 
 * instName loop
 */
static xkern_return_t
readLoop( XObj self, char **str, int nFields, int line, void *arg )
{
    ((PState *)arg)->loop = 1;
    return XK_SUCCESS;
}



static XObjRomOpt	cOpts[] = {
    { "server", 0, 0 },
    { "client", 0, 0 },
    { "mark", 3, readMark },
    { "trips", 3, readTrips },
    { "loop", 2, readLoop },
    { "delay", 3, readDelay }
};

static XObjRomOpt	csOpts[] = {
    { "server", 2, readServerOpt },
    { "client", 2, readClientOpt },
    { "mark", 0, 0 },
    { "trips", 0, 0 },
    { "loop", 0, 0 },
    { "delay", 0, 0 }
};




static xkern_return_t
saveServerSessn( XObj self, XObj lls, XObj llp, XObj hlpType, Path path )
{
    xTrace1(prottest, TR_MAJOR_EVENTS,
	    "%s test program duplicates lower server session",
	    PROT_STRING);
    xDuplicate(lls);
    return XK_SUCCESS;
}


static void
killMsg( Event evSelf, void *arg )
{
    static	i = 0;

    i++;
    xIfTrace(prottest, TR_EVENTS) {
	xIfTrace(prottest, TR_MORE_EVENTS) {
	    xTrace0(prottest, TR_MORE_EVENTS, "killMsg runs");
	    xTrace2(prottest, TR_EVENTS, "(msg %d, len %d)",
		    i, msgLen((Msg)arg));
	} else {
	    putchar('K');
	}
    }
    msgDestroy((Msg)arg);
    allocFree(arg);
}


static xkern_return_t
serverCallDemux( XObj self, XObj lls, Msg dg, Msg rmsg )
{
    Msg	newMsg;
    static int c = 0;
    Event ev;

    c++;
    xIfTrace(prottest, TR_MAJOR_EVENTS) {
	putchar('.');
	if (! (c % 50)) {
	    putchar('\n');
	}
    }
    if ( c % saveInterval == 0 ) {
	xIfTrace(prottest, TR_EVENTS) {
	    xIfTrace(prottest, TR_MORE_EVENTS) {
		xTrace0(prottest, TR_MORE_EVENTS,
			"server demux saving reference on message");
		xTrace2(prottest, TR_MORE_EVENTS, "(msg %d, len %d)",
			c, msgLen(dg));
	    } else {
		putchar('S');
	    }
	}
	if ( newMsg = pathAlloc(dg->path, sizeof(Msg_s)) ) {
	    if ( ev = evAlloc(dg->path) ) {
		msgConstructCopy(newMsg, dg);
		evDetach(evSchedule(ev, killMsg, newMsg, 90 * 1000 * 1000));
	    } else {
		xTraceP0(self, TR_ERRORS, "message save allocation failure");
		pathFree(newMsg);
	    }
	} else {
	    xTraceP0(self, TR_ERRORS, "message save allocation failure");
	}
    }
    msgAssign(rmsg, dg);
    return XK_SUCCESS;
}


static int
tryCall( XObj self, XObj sessn, int times, int length, XTime *nT )
{
    xkern_return_t xkr;
    int 	i;
    Msg_s	savedMsg, request, reply;
    XTime 	startTime, endTime;
    int 	c = 0;
    PState 	*ps = (PState *)self->state;
    
    xkr = msgConstructUltimate(&savedMsg, xMyPath(self),
			       (u_int)length, TEST_STACK_SIZE);
    if (xkr == XK_FAILURE) {
	sprintf(errBuf, "Could not construct a msg of length %d\n", 
		length + TEST_STACK_SIZE);
	Kabort(errBuf);
    }
    msgConstructContig(&reply, xMyPath(self), 0, 0);
    msgConstructContig(&request, xMyPath(self), 0, 0);
    xGetTime(&startTime);
    for (i=0; i<times; i++) {
	msgAssign(&request, &savedMsg);
	xkr = xCall(sessn, &request, &reply);
	xIfTrace(prottest, TR_MAJOR_EVENTS) {
	    putchar(ps->mark);
	    if (! (++c % 50)) {
		putchar('\n');
	    }
	}
	if( xkr == XK_FAILURE ) {
	    printf( "RPC call error %d\n" , xkr );
	    goto abort;
	}
	msgTruncate(&reply, 0);
    }
    xGetTime(&endTime);
    xSubTime(nT, endTime, startTime);

abort:
    msgDestroy(&savedMsg);
    msgDestroy(&reply);
    msgDestroy(&request);
    return i;
}


static void
client( Event ev, void *arg )
{
    Part_s	p;
    XObj	self = (XObj)arg;
    XObj	llp;
    PState	*ps = self->state;
    int		testNumber = 0, len, count, lenindex, noResponseCount = 0, i;
    XTime	netTime;
    u_int	maxLen;
    
    xAssert(xIsXObj(self));
    ps->mark = '.';
    ps->loop = 0;
    ps->delay = 0;
    if ( findXObjRomOpts(self, cOpts,
		sizeof(cOpts)/sizeof(XObjRomOpt), ps) == XK_FAILURE ) {
	xError("chantest: romfile config errors, not running");
	return;
    }
    printf("ALLOC timing test\n");
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xError("Test protocol has no lower protocol");
	return;
    }
    printf("I am the client [%c]\n", ps->mark);
    if ( ps->delay ) {
	xTrace1(prottest, TR_ALWAYS, "Delaying %d seconds", ps->delay);
	Delay(ps->delay * 1000 );
    }
    for ( maxLen=0, i=0; i < sizeof(lens)/sizeof(int); i++ ) {
	if ( lens[i] > maxLen ) maxLen = lens[i];
    }
    if ( msgReserveUltimate(xMyPath(self), 1, maxLen, TEST_STACK_SIZE)
		!= XK_SUCCESS ) {
	xError("Test reservation error");
	return;
    }
    partInit(&p, 1);
    partPush(p, &ps->serverHost, sizeof(IPhost));
    ps->lls = xOpen(self, self, llp, &p, xMyPath(self));
    if ( ps->lls == ERR_XOBJ ) {
	printf("%s test: open failed!\n", PROT_STRING);
	xError("End of test");
	return;
    }
    /* 
     * Delay to allow incoming messages to finalize opens (if necessary)
     */
    Delay(1000);
    for (lenindex = 0; lenindex < sizeof(lens)/sizeof(long) || ps->loop;
	 lenindex++) {
	if (lenindex >= sizeof(lens)/sizeof(long)) {
	    lenindex = 0;
	}
	len = lens[lenindex];
	printf("Sending (%d, [%c], %d bytes) ...\n",
	       ++testNumber, ps->mark, len);
	count = tryCall(self, ps->lls, xk_trips, len, &netTime);
	if ( count == xk_trips ) {
	    printf("\n[%c] len = %4d, %d trips: %6d.%06d\n", 
		   ps->mark, len, xk_trips, netTime.sec, netTime.usec);
	}
	if ( count == 0 ) {
	    if ( noResponseCount++ == FAILURE_LIMIT ) {
		printf("Server looks dead.  I'm outta here.\n");
		break;
	    }
	} else {
	    noResponseCount = 0;
	}
	Delay(DELAY * 1000);
    }
    printf("[%c] End of test\n", ps->mark);
    xClose(ps->lls);
    ps->lls = 0;
    msgReserveContig(xMyPath(self), -1, maxLen, TEST_STACK_SIZE);
    xTrace0(prottest, TR_FULL_TRACE, "test client returns");
}


static void
server( Event ev, void *arg )
{
    XObj	self = (XObj)arg;
    XObj	llp;
    Part_s	p;

    xError("CHAN timing test (server)");
    xError("This server saves messages");
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xError("Test protocol has no lower protocol");
	return;
    }
    self->opendone = saveServerSessn;
    self->calldemux = serverCallDemux;
    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if ( xOpenEnable(self, self, llp, &p) == XK_FAILURE ) {
	sprintf(errBuf, "%s server can't openenable lower protocol",
		PROT_STRING);
	xError(errBuf);
    } else {
	sprintf(errBuf, "%s server done with xopenenable", PROT_STRING);
	xError(errBuf);
    }
}


xkern_return_t
alloctest_init( XObj self )
{
    static XObj	firstServer;
    PState	*ps;
    Event	ev;

    if ( ! (ps = pathAlloc(self->path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    if ( ! (ev = evAlloc(self->path)) ) {
	return XK_NO_MEMORY;
    }
    bzero((char *)ps, sizeof(PState));
    ps->isServer = x_server;
    ps->serverHost = defaultServer;
    self->state = ps;
    if ( findXObjRomOpts(self, csOpts, sizeof(csOpts)/sizeof(XObjRomOpt),
			 ps) == XK_FAILURE ) {
	xError("alloctest: romfile config errors, not running");
	return XK_FAILURE;
    }
    if ( ps->isServer ) {
	if ( ! firstServer ) {
	    firstServer = self;
	    xError("Starting server");
	    evDetach(evSchedule(ev, server, self, 0));
	} else {
	    xError("Server already started");
	}
    } else {
	xError("Starting client");
	evDetach(evSchedule(ev, client, self, 0));
 }
    return XK_SUCCESS;
}
