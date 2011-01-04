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
 * common_test_rpc.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * common_test_rpc.c,v
 * Revision 1.10.1.5.1.2  1994/09/07  04:18:39  menze
 * OSF modifications
 *
 * Revision 1.10.1.5  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.10.1.4  1994/07/22  20:08:21  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.10.1.3  1994/04/21  16:24:49  menze
 * tryCallDefault takes a 'self' parameter
 * Messages are constructed from the allocator of 'self', rather than lls
 *
 */


static 	xkern_return_t	testCallDemux( XObj, XObj, Msg, Msg );

#ifndef TEST_STACK_SIZE
#  define TEST_STACK_SIZE 200
#endif

static int
tryCallDefault( self, sessn, times, length, nT )
  XObj self, sessn;
  int times;
  int length;
  XTime *nT;
{
    xkern_return_t ret_val;
    int i;
    Msg_s savedMsg, request, reply;
#ifdef  SIMPLE_ACKS
    int lr = msgLen(&msgReply);
#endif
#ifdef TIME
    XTime startTime, endTime, netTime;
#endif
    char *buf;
    int c = 0;
    
    ret_val = msgConstructUltimate(&savedMsg, xMyPath(self),
				   (u_int)length, TEST_STACK_SIZE);
    if (ret_val == XK_FAILURE) {
	sprintf(errBuf, "Could not construct a msg of length %d\n", 
		length + TEST_STACK_SIZE);
	Kabort(errBuf);
    }
    msgConstructContig(&reply, xMyPath(self), 0, 0);
    msgConstructContig(&request, xMyPath(self), 0, 0);
#ifdef TIME
    xGetTime(&startTime);
#endif
    for (i=0; i<times; i++) {
	msgAssign(&request, &savedMsg);
	ret_val = xCall(sessn, &request, &reply);
	xIfTrace(prottest, TR_MAJOR_EVENTS) {
	    putchar('.');
	    if (! (++c % 50)) {
		putchar('\n');
	    }
	}
	if( ret_val == XK_FAILURE ) {
	    printf( "RPC call error %d\n" , ret_val );
	    goto abort;
	}
#ifdef  SIMPLE_ACKS
	if (msgLen(&reply) != lr) {
	    printf("Bizarre reply length.  Expected %d, received %d\n",
		  msgLen(&reply), lr);
	    goto abort;
	}
#else
#if 0
	if (msgLen(&reply) != length) {
	    printf("Bizarre reply length.  Expected %d, received %d\n",
		   length, msgLen(&reply));
	    goto abort;
	}
#endif
#endif  /* SIMPLE_ACKS */
	msgTruncate(&reply, 0);
    }

#ifdef TIME
    xGetTime(&endTime);
    subtime(&startTime, &endTime, &netTime);
    *nT = netTime;
#endif
abort:
    msgDestroy(&savedMsg);
    msgDestroy(&reply);
    msgDestroy(&request);
    return i;
}


#ifdef USE_CONTROL_SESSN
static bool
controlMsgFill(strbuf, len, arg)
    char *strbuf
    long len;
    void *arg;
{
    int testNumber = *(int *)arg;

    sprintf(strbuf, "\nEnd of test %d", testNumber);
    return TRUE;
}
#endif


/* 
 * RPC runTest
 */
static xkern_return_t
defaultRunTest( self, len, testNumber )
    XObj	self;
    int 	len;
    int		testNumber;
{
    int 	test;
    static int	noResponseCount = 0;
    XObj	lls;
#ifdef TIME    
    XTime 	netTime;
#endif
    int		count;
    PState	*ps = self->state;

    xAssert(xIsXObj(self));
    lls = ps->lls;
    xAssert(xIsSession(lls));
    for (test = 0; test < TIMES; test++) {
	printf("Sending (%d) ...\n", testNumber);
	count = 0;
#ifdef PROFILE
	startProfile();
#endif
	if ( (count = tryCall(self, lls, xk_trips, len, &netTime)) == xk_trips ) {
#ifdef TIME
	    printf("\nlen = %4d, %d trips: %6d.%06d\n", 
		   len, xk_trips, netTime.sec, netTime.usec);
#else
	    printf("\nlen = %4d, %d trips\n", len, xk_trips);
#endif
	}
#ifdef USE_CONTROL_SESSN
	if ( xIsXObj(controlSessn) ) {
	    Msg_s	m;
	    char	*strbuf;
	    xkern_return_t xkr;
	    
	    xkr = msgConstructContig(&m, xMyAlloc(controlSessn),
				     (u_int)80, TEST_STACK_SIZE);
	    if (xkr == FAILURE) {
	        sprintf(errBuf, "Could not construct a msg of length %d\n", 
			80 + TEST_STACK_SIZE);
		Kabort(errBuf);
	    }
	    msgForEach(&m, controlMsgFill, &testNumber);

	    xCall(controlSessn, &m, 0);
	    msgDestroy(&m);
	    
	}
#endif /* USE_CONTROL_SESSN */
	if ( count == 0 ) {
	    if ( noResponseCount++ == FAILURE_LIMIT ) {
		printf("Server looks dead.  I'm outta here.\n");
		return XK_FAILURE;
	    }
	} else {
	    noResponseCount = 0;
	}
	Delay(DELAY * 1000);
#ifdef XK_MEMORY_THROTTLE
	{
	  extern int max_thread_pool_used, min_memory_avail,
	             max_xk_threads_inuse;
	  printf("used %d incoming threads; %d regular threads; max %d bytes memory",
		 max_thread_pool_used,
		 max_xk_threads_inuse,
		 min_memory_avail);
	}
#endif /* XK_MEMORY_THROTTLE */
    }
    return XK_SUCCESS;
}


static xkern_return_t
serverCallDemuxDefault(self, lls, dg, rMsg)
    XObj self, lls;
    Msg dg, rMsg;
{
    msgAssign(rMsg, dg);
    return XK_SUCCESS;
}


static xkern_return_t
testCallDemux( self, lls, dg, rMsg )
    XObj self, lls;
    Msg dg, rMsg;
{
    static int c = 0;
    
    xIfTrace(prottest, TR_MAJOR_EVENTS) {
	putchar('.');
	if (! (++c % 50)) {
	    putchar('\n');
	}
    }
#ifdef SIMPLE_ACKS
    msgAssign(rMsg, &msgReply);
#else
    msgAssign(rMsg, dg);
#endif
    return XK_SUCCESS;
}


static void
realmServerFuncs( self )
    XObj	self;
{
    self->calldemux = testCallDemux;
}


static void
realmClientFuncs( self )
    XObj	self;
{
}
