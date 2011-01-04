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
 */
/*
 * MkLinux
 */
/*
 * common_test.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * common_test.c,v
 * Revision 1.65.1.3.1.2  1994/09/07  04:18:38  menze
 * OSF modifications
 *
 * Revision 1.65.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.65.1.2  1994/07/22  20:08:19  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.65.1.1  1994/04/21  16:22:37  menze
 * tryCall takes a 'self' parameter
 *
 * Revision 1.65  1994/04/21  16:21:14  menze
 *   [ 1994/01/18          menze ]
 *   Extended 'Pfi' kludge to include XSA platform.
 *
 * Revision 1.64  1993/12/16  01:47:42  menze
 * Extended awful 'Pfi' kludge to include IRIX platform.
 * Fixed '#endif' comments
 *
 */

/*
 * This code implements a ping-pong test for another protocol.
 * The same protocol code is used for both the client and the server.  
 *
 * It expects the definitions of following macros which describe
 * the lower protocol:
 *
 *	HOST_TYPE (e.g., ETHhost)
 *	INIT_FUNC (e.g., ethtest_init)
 *	TRACE_VAR (e.g., ethtestp)
 *	PROT_STRING (e.g., "eth")
 *
 * It also needs declarations for Client and Server addresses, e.g.:
 *	HOST_TYPE ServerAddr = { 0xC00C, 0x4558, 0x04d2 };  
 *	HOST_TYPE ClientAddr = { 0xC00C, 0x4558, 0x2694 };  
 *
 * It also needs definitions for the following macros controlling the test:
 *	TRIPS  (number of round trips to make)
 *	TIMES  (number of tests for each length)
 *	DELAY  (number of seconds to delay between each test
 *		(and timeout for declaring failure))
 *
 * Define the macro TIME to do timing calculations.
 *
 * *Finally*, define an array of integers 'lens[]' with the lengths for the
 * tests:
 *
 *	static int lens[] = { 
 *		  1, 200, 400, 600, 800, 1000, 1200
 *	};
 *
 */

/* STREAM_TEST may not work if xk_simul>1 */

#include <xkern/include/xkernel.h>
#ifdef MACH_KERNEL
#include <xkern/include/assert.h>
#endif /* ! MACH_KERNEL */
#ifdef XK_MEMORY_THROTTLE
#include <xkern/include/xk_malloc.h>
#endif /* XK_MEMORY_THROTTLE */

typedef struct {
    XObj	lls;
    int		llpReliable;
    /* 
     * Client only 
     */
    XObj	controlSessn;
    int		sentMsgLength;
    Msg_s	savedMsg;
    char 	mark;
    /* 
     * Async only
     */
    int			clientRcvd;
    xmsg_handle_t	clientPushResult[100];
    xmsg_handle_t	serverPushResult[100];
    int			idle;
    /* 
     * stream only
     */
    int			receivedLength;
} PState;

#ifdef SIMPLE_ACKS
"This option won't work with the new message interface";
#endif
#ifdef ALL_TOGETHER
"This option won't work with the new message interface";
#endif

int	TRACE_VAR ;


xkern_return_t	INIT_FUNC( XObj );

static	void 	client( Event, void * );
static 	void 	server( Event, void * );
static 	int	isServerDefault( XObj );
static 	int	isClientDefault( XObj );
static 	int	(*isServer)( XObj ) = isServerDefault;
static 	int	(*isClient)( XObj ) = isClientDefault;
static 	xkern_return_t	defaultRunTest( XObj, int, int );
static  void	testInit( XObj );
static 	void	realmClientFuncs( XObj );
static 	void	realmServerFuncs( XObj );


#ifndef STR2HOST
#  define STR2HOST str2ipHost
#endif

#ifdef  SIMPLE_ACKS
Msg_s	msgReply;
#endif  /* SIMPLE_ACKS */

static	HOST_TYPE 	myHost;

#if SUPER_CLOCK
int	nets_test;
int	netr_test;
extern void	fc_get();
#endif

/* 
 * Parameters reflecting defaults and command line parameters.  There
 * are OK to be static as they shouldn't change across multiple
 * instantiations. 
 */
static	int	serverParam, clientParam, testsSpecified, myTestSpecified;
static	char	*serverString;
static	int	xk_trips = TRIPS;
static  int	xk_simul = 1; /* number of simultaneous messages circulating */
/* if xk_simul>1, the xk_trips are randomly divided among the messages */

#ifdef RPCTEST

static xkern_return_t	serverCallDemuxDefault( XObj, XObj, Msg, Msg );
static int		tryCallDefault( XObj, XObj, int, int, XTime *);

static t_calldemux serverCallDemux = serverCallDemuxDefault;
static int (* tryCall)( XObj, XObj, int, int, XTime *) = tryCallDefault;

#else

static xkern_return_t	defaultServerDemux( XObj, XObj, Msg );

static t_demux	serverDemux = defaultServerDemux;

#endif /* RPCTEST */

#ifdef TIME
#  ifndef RPCTEST
static	XTime 	starttime;
#  endif
static 	void	subtime( XTime *t1, XTime *t2, XTime *result);
#endif

#ifndef FAILURE_LIMIT
#  define FAILURE_LIMIT 2
#endif

static xkern_return_t	(*runTest)( XObj, int, int ) = defaultRunTest;


#define DOUBLEQUOTEWRAP(x) #x
#define STRINGIFY(z) DOUBLEQUOTEWRAP(z)

static void
print_compile_options( void )
{   printf("\nCompiled with options:\n");
    printf(
    "HOST_TYPE "  STRINGIFY(HOST_TYPE)
    "; INIT_FUNC " STRINGIFY(INIT_FUNC)
    "; TRACEVAR "  STRINGIFY(TRACEVAR)  "; "
    "PROT_STRING %s\n", PROT_STRING);
    printf("TRIPS = %d  TIMES = %d  DELAY = %d\n", TRIPS, TIMES, DELAY);
    printf("__STDC__ %s  PROFILE %s  TIME %s  MACH_KERNEL %s\n",
	   "on",
#ifdef PROFILE
	   "on",
#else
	   "off",
#endif
#ifdef TIME
	   "on",
#else
	   "off",
#endif
#ifdef MACH_KERNEL
	   "on"
#else
	   "off"
#endif
	   );
    printf("XK_MEMORY_THROTTLE %s  RPCTEST %s  STREAM_TEST %s\n",
#ifdef XK_MEMORY_THROTTLE
	   "on",
#else
	   "off",
#endif
#ifdef RPCTEST
	   "on",
#else
	   "off",
#endif
#ifdef STREAM_TEST
	   "on"
#else
	   "off"
#endif
	   );
    printf("FAILURE_LIMIT %d  CONCURRENCY ", FAILURE_LIMIT);
#ifdef CONCURRENCY
    printf("%d",CONCURRENCY);
#else
    printf("off");
#endif
    printf("  XK_INCOMING_MEMORY_MARK ");
#ifdef XK_INCOMING_MEMORY_MARK
    printf("%d", XK_INCOMING_MEMORY_MARK);
#else
    printf("undefined");
#endif
    printf("\n");
#if (defined(CUSTOM_ASSIGN) || defined(CUSTOM_OPENDONE) || \
     defined(CUSTOM_SERVER_DEMUX) || defined(CUSTOM_CLIENT_DEMUX))
    printf("with");
#ifdef CUSTOM_ASSIGN
	printf(" CUSTOM_ASSIGN");
#endif
#ifdef CUSTOM_OPENDONE
	printf(" CUSTOM_OPENDONE");
#endif
#ifdef CUSTOM_SERVER_DEMUX
	printf(" CUSTOM_SERVER_DEMUX");
#endif
#ifdef CUSTOM_CLIENT_DEMUX
	printf(" CUSTOM_CLIENT_DEMUX");
#endif
    printf("\n");
#endif
#if !(defined(CUSTOM_ASSIGN) && defined(CUSTOM_OPENDONE) && \
      defined(CUSTOM_SERVER_DEMUX) && defined(CUSTOM_CLIENT_DEMUX))
    printf("without");
#ifndef CUSTOM_ASSIGN
	printf(" CUSTOM_ASSIGN");
#endif
#ifndef CUSTOM_OPENDONE
	printf(" CUSTOM_OPENDONE");
#endif
#ifndef CUSTOM_SERVER_DEMUX
	printf(" CUSTOM_SERVER_DEMUX");
#endif
#ifndef CUSTOM_CLIENT_DEMUX
	printf(" CUSTOM_CLIENT_DEMUX");
#endif
    printf("\n");
#endif
    printf("SAVE_SERVER_SESSN %s  CLIENT_OPENABLE %s\n",
#ifdef SAVE_SERVER_SESSN
	   "on",
#else
	   "off",
#endif
#ifdef CLIENT_OPENABLE
	   "on"
#else
	   "off"
#endif
	   );
    printf("USE_CHECKSUM %s  INF_LOOP %s  XK_TEST_ABORT %s\n",
#ifdef USE_CHECKSUM
	   "on",
#else
	   "off",
#endif
#ifdef INF_LOOP
	   "on",
#else
	   "off",
#endif
#ifdef XK_TEST_ABORT
	   "on"
#else
	   "off"
#endif
	   );
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


static XObjRomOpt	cOpts[] = {
    { "mark", 3, readMark },
};



static void
processOptions( void )
{
    int		i;
    char 	*arg;

#define argPrefix(str) ((! strncmp(arg, str, strlen(str))) && strlen(arg)>strlen(str))
#define argEq(str) (! strcmp(arg, str) )

    for (i=1; i < globalArgc; i++) {
	arg = globalArgv[i];
	if ( argEq("-s") ) {
	    serverParam = 1;
	} else if ( argPrefix("-c") ) {
	    clientParam = 1;
	    serverString = arg + 2;
	} else if ( argEq("-c")) {
	  clientParam = 1;
	  serverString = globalArgv[i+1];
	  i++;
	} else if ( argPrefix("-test")) {
	    testsSpecified = 1;
	    arg += strlen("-test");
	    if ( argEq(PROT_STRING) ) {
		myTestSpecified = 1;
	    }
	} else if ( argPrefix("-trips=") ) {
	    xk_trips = atoi(arg + strlen("-trips="));
	} else if ( argEq ("-trips")) {
	    xk_trips = atoi(globalArgv[i+1]);
	    i++;
	} else if ( argPrefix("-simul=")) {
	    xk_simul = atoi(arg + strlen("-simul="));
	    if (xk_simul>100) {printf("simul clipped to 100\n"); xk_simul=100;};
	    if (xk_simul<1) {printf("simul increased to 1\n"); xk_simul=1;};
	} else if ( argEq("-simul=")) {
	    xk_simul = atoi(globalArgv[i+1]);
	    if (xk_simul>100) {printf("simul clipped to 100\n"); xk_simul=100;};
	    if (xk_simul<1) {printf("simul increased to 1\n"); xk_simul=1;};
	    i++;
	}
    }
    if (serverString && (serverString[0] > '9' || serverString[0] < '0')) {
#if 0
        IPhost haddr;

	if( xk_gethostbyname(serverString, &haddr) == XK_SUCCESS ) {
	  xTrace5(prottest, TR_DETAILED,
		  "server: %s addr %d.%d.%d.%d\n", serverString,
		  haddr.a, haddr.b, haddr.c, haddr.d);
	  strcpy(serverString, ipHostStr(&haddr));
	  xTrace1(prottest, TR_DETAILED, "server: %s addr\n", serverString);
	}
	else
#endif
	  {
	  xTrace1(prottest, TR_SOFT_ERRORS, "Cannot resolve name %s as host address; check dns entries in rom file",
		  serverString);
	}
      }

    if (xk_trips%xk_simul) {
      xk_trips = xk_simul * (1 + xk_trips/xk_simul);
      printf("Rounded trips up to %d, a multiple of simul(%d).\n",
	     xk_trips, xk_simul);
    };
    printf("Running with trips=%d, simul=%d.\n",xk_trips,xk_simul);
    if (xk_simul>1)
     printf(
      "Be sure that server has a simul value <= %d, and is a divisor of %d.\n",
	    xk_simul, xk_trips);
#undef argPrefix
#undef argEq    
}


xkern_return_t
INIT_FUNC( self )
    XObj self;
{
    XObj	llp;
    PState	*ps;
    Path	path = self->path;
    Event	ev;

    xIfTrace(prottest, TR_ERRORS) { print_compile_options(); };
    processOptions();
    if ( testsSpecified && ! myTestSpecified ) {
	xTrace1(prottest, TR_SOFT_ERRORS, "Parameters indicate %s test should not run",
		PROT_STRING);
	return XK_SUCCESS;
    }
    printf("%s timing test\n", PROT_STRING);
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xError("Test protocol has no lower protocol");
	return XK_FAILURE;
    }
    xControl(xGetDown(self, 0), GETMYHOST, (char *)&myHost, sizeof(HOST_TYPE));
    if ( ! (ps = pathAlloc(path, sizeof(PState))) )  {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_NO_MEMORY;
    }
    bzero((char *)ps, sizeof(PState));
    self->state = (void *)ps;
    /* 
     * Call the per-test initialization function which gives the test
     * the opportunity to override the default functions
     */
    testInit(self);
    if ((*isServer)(self)) {
	if ( ! (ev = evAlloc(path)) ) {
	    xTraceP0(self, TR_ERRORS, "allocation error");
	    return XK_NO_MEMORY;
	}
	evDetach( evSchedule(ev, server, self, 0) );
    } else if ( (*isClient)(self)) {
#ifdef CONCURRENCY	
	int	i;

	for ( i=0; i < CONCURRENCY; i++ )
#endif
	  {
	      if ( ! (ev = evAlloc(path)) ){
		  xTraceP0(self, TR_ERRORS, "allocation error");
		  return XK_NO_MEMORY;
	      }
	      evDetach( evSchedule(ev, client, self, 0) );
	  }
    } else {
	printf("%stest: I am neither server nor client\n", PROT_STRING);
    }
    return XK_SUCCESS;
}



static int
isServerDefault( self )
    XObj	self;
{
    if ( serverParam ) {
	return TRUE;
    }
    if ( ! strcmp(self->instName, "server") ) {
	return TRUE;
    }
#if 1
    return ! bcmp((char *)&myHost, (char *)&ServerAddr, sizeof(HOST_TYPE));
#else
    return FALSE;
#endif
}


static int
isClientDefault( self )
    XObj	self;
{
    if ( ! strcmp(self->instName, "client") ) {
	return TRUE;
    }
    if ( clientParam ) {
	STR2HOST(&ServerAddr, serverString);
	ClientAddr = myHost;
	return TRUE;
    }
#if 1
    return ! bcmp((char *)&myHost, (char *)&ClientAddr, sizeof(HOST_TYPE));
#else
    return FALSE;
#endif
}


#ifndef CUSTOM_ASSIGN

static void
clientSetPart( 
	      Part p)
{
    partInit(p, 1);
    partPush(*p, &ServerAddr, sizeof(IPhost));
}

static void
serverSetPart(
	      Part p)
{
    partInit(p, 1);
    partPush(*p, ANY_HOST, 0);
}

#endif /* ! CUSTOM_ASSIGN */


#ifdef SAVE_SERVER_SESSN
static xkern_return_t	saveServerSessn( XObj, XObj, XObj, XObj, Path );

static xkern_return_t
saveServerSessn( self, s, llp, hlpType, path )
    XObj self, s, llp, hlpType;
    Path path;
{
    xTrace1(prottest, TR_MAJOR_EVENTS,
	    "%s test program duplicates lower server session",
	    PROT_STRING);
    xDuplicate(s);
    return XK_SUCCESS;
}
#endif /* SAVE_SERVER_SESSN */


static xkern_return_t
closeDone( 
	  XObj lls)
{
    xTrace2(prottest, TR_MAJOR_EVENTS, "%s test -- closedone (%x) called",
	    PROT_STRING, lls);
#ifdef SAVE_SERVER_SESSION
    if ( (*isServer)() ) {
	xClose(lls);
    }
#endif
    return XK_SUCCESS;
}


static void
server( ev, foo )
    Event	ev;
    void 	*foo;
{
    Part_s p;
    XObj myProtl = (XObj)foo;
    
    printf("I am the  server\n");
    xAssert(xIsProtocol(myProtl));
    realmServerFuncs(myProtl);
#ifdef SAVE_SERVER_SESSN
    myProtl->opendone = saveServerSessn;
#endif 
#ifdef CUSTOM_OPENDONE
    myProtl->opendone = customOpenDone;
#endif 
    myProtl->closedone = closeDone;
    serverSetPart(&p);
#ifdef  SIMPLE_ACKS
    msgConstructAllocate(&msgReply, REPLY_SIZE, &buf);
#endif  /* SIMPLE_ACKS */
    if ( xOpenEnable(myProtl, myProtl, xGetDown(myProtl, 0), &p)
		== XK_FAILURE ) {
	printf("%s test server can't openenable lower protocol\n",
	       PROT_STRING);
    } else {
	printf("%s test server done with xopenenable\n", PROT_STRING);
    }
    return;
}


static void
client( ev, foo )
    Event	ev;
    void 	*foo;
{
    Part_s	p[2];
    int 	lenindex;
    XObj	myProtl = (XObj)foo;
    PState	*ps;
    int		testNumber = 0;
    
    xAssert(xIsXObj(myProtl));
    ps = (PState *)myProtl->state;
    printf("I am the client\n");
    realmClientFuncs(myProtl);

    ps->mark = '.';
    if ( findXObjRomOpts(myProtl, cOpts,
		sizeof(cOpts)/sizeof(XObjRomOpt), ps) == XK_FAILURE ) {
	xError("test: romfile config errors, not running");
	return;
    }
#ifdef CLIENT_OPENENABLE
    serverSetPart(p);
    if ( xOpenEnable(myProtl, myProtl, xGetDown(myProtl, 0), p, myProtl->path)
		== XK_FAILURE ) {
	printf("%s test client can't openenable lower protocol\n",
	       PROT_STRING);
	return;
    } else {
	printf("%s test client done with xopenenable\n", PROT_STRING);
    }
#endif
    clientSetPart(p);
    if ( ps->lls == 0 ) {
    	ps->lls = xOpen(myProtl, myProtl, xGetDown(myProtl, 0), p,
			xMyPath(myProtl));
	if ( ps->lls == ERR_XOBJ ) {
	    printf("%s test: open failed!\n", PROT_STRING);
	    Kabort("End of test");
	    return;
	}
    }
#if 0
    /* 
     * Delay to allow incoming messages to finalize opens (if necessary)
     */
    Delay(1000);
#endif
#ifdef CLIENT_OPENDONE
    myProtl->opendone(myProtl, ps->lls, xGetDown(myProtl, 0), myProtl,
		      myProtl->path);
#endif    
#ifdef USE_CONTROL_SESSN
    clientSetPart(p);
    if ( xIsProtocol(xGetDown(myProtl, 1)) ) {
	controlSessn = xOpen(myProtl, myProtl, xGetDown(myProtl, 1), p,
			     myProtl->path);
	if ( ! xIsXObj(controlSessn) ) {
	    xError("test protl could not open control sessn");
	}
    } else {
	xTrace1(prottest, TR_EVENTS,
		"%s test client not opening control session",
		PROT_STRING);
    }
#endif /* USE_CONTROL_SESSN */

#ifdef  SIMPLE_ACKS
    msgConstructAllocate(&msgReply, REPLY_SIZE, &buf);
#endif  /* SIMPLE_ACKS */

#ifdef USE_CHECKSUM
    xControl(ps->lls, UDP_ENABLE_CHECKSUM, NULL, 0);
#endif
#ifdef INF_LOOP
    for (lenindex = 0; ; lenindex++) {
	if (lenindex >= sizeof(lens)/sizeof(long)) {
	    lenindex = 0;
	}
#else
    for (lenindex = 0; lenindex < sizeof(lens)/sizeof(long); lenindex++) {
#endif /* INF_LOOP	 */
	ps->sentMsgLength = lens[lenindex];
	while ( runTest(myProtl, lens[lenindex], ++testNumber) != XK_SUCCESS )
	  ;
    }
    printf("End of test\n");
#if 0
    Just temporarily commented out ...

    xClose(ps->lls);
#endif
    ps->lls = 0;
#ifdef XK_TEST_ABORT
    Kabort("test client aborts at end of test");
#endif
    xTrace0(prottest, TR_FULL_TRACE, "test client returns");
}


#ifdef TIME
static void
subtime(t1, t2, t3)
    XTime *t1, *t2, *t3;
{
    t3->sec = t2->sec - t1->sec;
    t3->usec = t2->usec - t1->usec;
    if (t3->usec < 0) {
	t3->usec += 1000000;
	t3->sec -= 1;
    }
}
#endif


#ifdef RPCTEST

#  include <xkern/protocols/test/common_test_rpc.c>

#else

#  include <xkern/protocols/test/common_test_async.c>

#endif
