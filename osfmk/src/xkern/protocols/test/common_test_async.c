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
 * common_test_async.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * common_test_async.c,v
 * Revision 1.5.1.4.1.2  1994/09/07  04:18:37  menze
 * OSF modifications
 *
 * Revision 1.5.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.5.1.3  1994/07/22  20:08:20  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.5.1.2  1994/04/21  16:23:56  menze
 * xMyAllocator became xMyAlloc
 * Added error check
 *
 * Revision 1.5.1.1  1994/04/07  22:53:22  menze
 * Uses new message interface
 *
 */


#ifdef STREAM_TEST
static xkern_return_t	clientStreamDemux( XObj, XObj, Msg );
#else /* ! STREAM_TEST */
static xkern_return_t	clientDemux( XObj, XObj, Msg );
#endif /* STREAM_TEST */

#ifdef SIMPLE_ACKS
Msg_s	savedMsg;
Msg_s	msga[100];
#endif
#ifdef ALL_TOGETHER
long	xk_count;
#endif

#ifndef TEST_STACK_SIZE
#  define TEST_STACK_SIZE 200
#endif

static void
realmServerFuncs( self )
    XObj	self;
{
    self->demux = serverDemux;
}


static void
realmClientFuncs( self )
    XObj	self;
{
#ifdef STREAM_TEST
    self->demux = clientStreamDemux;
#else
    self->demux = clientDemux;
#endif
}


static void
checkHandle( 
	    xmsg_handle_t h,
	    char *str )
{
    switch ( h ) {
      case XMSG_ERR_HANDLE:
      case XMSG_ERR_WOULDBLOCK:
	sprintf(errBuf, "%s returns error handle %d", str, h);
	Kabort(errBuf);
      default:
	;
    }
}


static xkern_return_t
defaultRunTest( self, len, testNumber )
    XObj	self;
    int 	len;
    int		testNumber;
{
#ifndef SIMPLE_ACKS
    static Msg_s  msga[100];
#endif
    int 	m;
    static int	noResponseCount = 0;
    PState	*ps = (PState *)self->state;
    xkern_return_t xkr;
    
    xAssert(ps);
    xkr = msgConstructUltimate(&ps->savedMsg, xMyPath(self),
			      (u_int)len, TEST_STACK_SIZE );
    if (xkr == XK_FAILURE) {
	sprintf(errBuf, "Could not construct a msg of length %d\n", 
		len + TEST_STACK_SIZE);
	Kabort(errBuf);
    }

	for (m=0; m<xk_simul; m++) {msgConstructCopy(&msga[m], &ps->savedMsg);};
	printf("Sending (%d) ...\n", testNumber);
	printf("msg length: %d\n", msgLen(&msga[0]));
	ps->clientRcvd = 0;
#ifdef PROFILE
	startProfile();
#endif
#ifdef TIME
	xGetTime(&starttime);
#endif
#ifdef  ALL_TOGETHER
	for (m = 0; m < xk_trips; m++) {
	    msgConstructCopy(&msga[0], &savedMsg);
	    ps->clientPushResult[0] = xPush(ps->lls, &msga[0]);
	    msgDestroy(&msga[0]);
	}
	xGetTime(&now);
	subtime(&starttime, &now, &total);
	printf("\nlen = %4d, %d xk_trips: %6d.%06d\n",
		msgLen(&savedMsg), xk_trips, total.sec, total.usec);
	do {
	    ps->idle = 1;
	    Delay(DELAY * 1000);
	} while (!ps->idle);
	printf("Packets received = %d\n", xk_count);
	msgDestroy(&savedMsg);
#else   /* ALL_TOGETHER */

#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif /* XK_MEMORY_THROTTLE */
	for (m=0; m<xk_simul; m++) {
	    ps->clientPushResult[m] = xPush(ps->lls, &msga[m]);
#ifdef TCP_PUSH
	    xControl(ps->lls, TCP_PUSH, NULL, 0);
#endif
#ifndef SIMPLE_ACKS
	    checkHandle(ps->clientPushResult[m], "client push");
#endif
	}
	do {
	    ps->idle = 1;
	    Delay(DELAY * 1000);
	} while ( (ps->llpReliable && ps->clientRcvd < xk_trips) ||
		  ! ps->idle );
	if ( ps->clientRcvd < xk_trips ) {
	    printf("Test failed after receiving %d packets\n", ps->clientRcvd);
#ifdef STREAM_TEST
	    ps->receivedLength = 0;
#endif
	} 
	for (m=0; m<xk_simul; m++) { msgDestroy(&msga[m]); };
	if ( ps->clientRcvd == 0 ) {
	    xkr = XK_FAILURE;
	} else {
	    xkr = XK_SUCCESS;
	    noResponseCount = 0;
	}
#endif /* ALL_TOGETHER */
#ifndef SIMPLE_ACKS
    msgDestroy(&ps->savedMsg);
#endif
    return xkr;
}

#ifdef CHECK_MESSAGE_CONTENTS
static long revbcopy0(to,from,len,ignore)
    char *to, *from, *ignore; long len;
{   bcopy(from,to,len); ignore++; return 0; }
#endif


static xkern_return_t
defaultServerDemux( self, lls, dg )
    XObj self, lls;
    Msg dg;
{
    PState	*ps = (PState *)self->state;
    static int c = 0;
    static Msg_s msga[100]; static int msgi = 0; int i;
#if SUPER_CLOCK
    int startr_test[2], endr_test[2];

    fc_get(startr_test);
#endif

    xIfTrace(prottest, TR_MAJOR_EVENTS) {
	putchar('.');
	if (! (c++ % 50)) {
	    putchar('\n');
	}
    }

#ifdef CHECK_MESSAGE_CONTENTS
    { int len=msgLen(dg), k; char* tbuf=pathAlloc(self->path, len);

      if ( ! tbuf ) { xError("test CMC allocation error"); return XK_FAILURE; }
      msgPop(dg,revbcopy0,tbuf,len,0);
      for (k=0;k<len;k++)
	if ( *(tbuf+k) != (char)(3*k+len))
	{ printf("Message compare failed at byte %d, is %x, sb %x.  msg# %d len %d.\n",
		 k, *(tbuf+k), (char)(3*k+len), c, len); break; }
      if (k==len) printf("Message %d len %d compared ok.\n", c, len);
#ifdef PRINT_MESSAGE_CONTENTS
      printf("msg "); for (k=0;k<len && k<40;k++) printf("%02x",0xff & *(tbuf+k)); printf("\n");
      if (len>=40) { printf("msg ");
        for (k=(len<80)?40:(len-40);k<len;k++) printf("%02x",0xff & *(tbuf+k)); printf("\n");}
#endif
      allocFree(tbuf); }
#endif

#ifdef CUSTOM_SERVER_DEMUX
    customServerDemux(self, lls, dg, 0);
#endif /* CUSTOM_SERVER_DEMUX */
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif /* XK_MEMORY_THROTTLE */
    /* if xk_simul>1, save up a group of messages, then return them */
    /* if things get out of sync, they get really screwed up! */
#ifdef SIMPLE_ACKS
    ps->serverPushResult[0] = xPush(lls, &msgReply);
#else
    if (xk_simul>1) {
      msgConstructCopy(&msga[msgi],dg); msgi++;
      if (msgi==xk_simul) {
	for (i=0; i<xk_simul; i++) {
	    ps->serverPushResult[i] = xPush(lls, &msga[i]);
#ifdef TCP_PUSH
	    xControl(lls, TCP_PUSH, NULL, 0);
#endif
	    checkHandle(ps->serverPushResult[i], "server push");
	    msgDestroy(&msga[i]);
	}
	msgi=0;
      };
    } else {
	ps->serverPushResult[0] = xPush(lls, dg);
#ifdef TCP_PUSH
	xControl(lls, TCP_PUSH, NULL, 0);
#endif
	checkHandle(ps->serverPushResult[0], "server push");
    }
#endif /* SIMPLE_ACKS */
#if SUPER_CLOCK
    fc_get(endr_test);
    netr_test = endr_test[0] - startr_test[0];
#endif
    return XK_SUCCESS;
}


#ifdef STREAM_TEST


static xkern_return_t
clientStreamDemux( self, lls, dg )
    XObj self, lls;
    Msg dg;
{
#ifdef TIME
    XTime 	now, total;
#endif
    Msg_s		msgToPush;
    xmsg_handle_t	h;
    PState		*ps = (PState *)self->state;

    xAssert(ps);
    ps->idle = 0;
    xTrace1(prottest, TR_MORE_EVENTS, "R %d", msgLen(dg));
    ps->receivedLength += msgLen(dg);
    xTrace1(prottest, TR_DETAILED, "total length = %d", ps->receivedLength);
#ifdef CUSTOM_CLIENT_DEMUX
    customClientDemux(self, lls, dg, 0);
#endif /* CUSTOM_CLIENT_DEMUX */
    if (ps->receivedLength == ps->sentMsgLength) {
	/*
	 * Entire response has been received.
	 * Send another message
	 */
	if (++ps->clientRcvd < xk_trips) {
	    xIfTrace(prottest, TR_MAJOR_EVENTS) { 
		putchar(ps->mark);
		if (! (ps->clientRcvd % 50)) {
		    putchar('\n');
		}
	    }
	  if (ps->clientRcvd+xk_simul <= xk_trips) {
	    msgConstructCopy(&msgToPush, &ps->savedMsg);
	    ps->receivedLength = 0;
	    xTrace0(prottest, TR_MORE_EVENTS, "S");
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif /* XK_MEMORY_THROTTLE */
	    h = xPush(lls, &msgToPush);
#ifdef TCP_PUSH
	    xControl(lls, TCP_PUSH, NULL, 0);
#endif
	    checkHandle(h, "client push");
	    msgDestroy(&msgToPush);
	  } else { ps->receivedLength = 0; };
	} else {
#ifdef TIME
	    xGetTime(&now);
	    subtime(&starttime, &now, &total);
	    printf("\nlen = %4d, %d trips: %6d.%06d\n", 
		   ps->receivedLength, xk_trips, total.sec, total.usec);
#else
	    printf("\nlen = %4d, %d trips\n", ps->receivedLength, xk_trips);
#endif
	    ps->receivedLength = 0;
#ifdef PROFILE
	    endProfile();
#endif
	}
    }
    return XK_SUCCESS;
}


#else /* ! STREAM_TEST */



static xkern_return_t
clientDemux( self, lls, dg )
    XObj self, lls;
    Msg dg;
{
    PState	*ps = (PState *)self->state;
#if SUPER_CLOCK
    int s, starts_test[2], ends_test[2];
#endif
#ifdef ALL_TOGETHER
    ps->idle = 0;
    xk_count++;
#else
#ifdef TIME
    XTime now, total;
#endif
    static Msg_s msga[100]; static int msgi = 0; int i;

#ifdef CHECK_MESSAGE_CONTENTS
    { int len=msgLen(dg), k; char* tbuf=pathAlloc(self->path, len);

      if ( ! tbuf ) { xError("test CMC allocation error"); return XK_FAILURE; }
      msgPop(dg,revbcopy0,tbuf,len,0);
      for (k=0;k<len;k++)
	if ( *(tbuf+k) != (char)(3*k+len))
	{ printf("Message compare failed at byte %d, is %x, sb %x.  msg# %d len %d.\n",
		 k, *(tbuf+k), (char)(3*k+len), ps->clientRcvd, len); break; }
      if (k==len) printf("Message %d len %d compared ok.\n", ps->clientRcvd, len);
#ifdef PRINT_MESSAGE_CONTENTS
      printf("msg "); for (k=0;k<len && k<40;k++) printf("%02x",0xff & *(tbuf+k)); printf("\n");
      if (len>=40) { printf("msg ");
        for (k=(len<80)?40:(len-40);k<len;k++) printf("%02x",0xff & *(tbuf+k)); printf("\n");}
#endif
      allocFree(tbuf); }
#endif

/* note that customdemux is not called on final response message */
#ifdef CUSTOM_CLIENT_DEMUX
    customClientDemux(self, lls, dg, msgi);
#endif /* CUSTOM_CLIENT_DEMUX */
    ps->idle = 0;
    if ( ++ps->clientRcvd < xk_trips ) {
	xIfTrace(prottest, TR_MAJOR_EVENTS) {
	    putchar(ps->mark);
	    if (! (ps->clientRcvd % 50)) {
		putchar('\n');
	    }
	}
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif /* XK_MEMORY_THROTTLE */
	if (xk_simul>1) {
	  msgConstructCopy(&msga[msgi],&ps->savedMsg); msgi++;
	  if (msgi==xk_simul) {
	    for (i=0; i<xk_simul; i++) {
		ps->clientPushResult[i] = xPush(lls, &msga[i]);
#ifdef TCP_PUSH
		xControl(lls, TCP_PUSH, NULL, 0);
#endif
		checkHandle(ps->clientPushResult[i], "client push");
		msgDestroy(&msga[i]);
	    }
	    msgi=0;
	  };
	}
	else {
	    /* 
	     * We need to construct a copy of the original message
	     * rather than just loop back the incoming message to
	     * avoid an increasingly fragmented message structure in
	     * the case of loopback.
	     */
#ifdef SIMPLE_ACKS
	    Msg_s	tmpMsg;

	    msgConstructCopy(&tmpMsg, &ps->savedMsg);
	    ps->clientPushResult[0] = xPush(lls, &tmpMsg);
	    msgDestroy(&tmpMsg);
#ifdef TCP_PUSH
            xControl(lls, TCP_PUSH, NULL, 0);
#endif
#else
#if SUPER_CLOCK
          s = sploff();
          fc_get(starts_test);
#endif
          ps->clientPushResult[0] = xPush(ps->lls, dg);
#ifdef TCP_PUSH
	  xControl(lls, TCP_PUSH, NULL, 0);
#endif
          checkHandle(ps->clientPushResult[0], "client push");
#if SUPER_CLOCK
          fc_get(ends_test);
          splon(s);
          nets_test = ends_test[0] - starts_test[0];
#endif
#endif /* SIMPLE_ACKS */
      }
    } else {
#ifdef TIME
	xGetTime(&now);
	subtime(&starttime, &now, &total);
	printf("\nlen = %4d, %d trips: %6d.%06d\n", 
#ifdef SIMPLE_ACKS
                msgLen(&savedMsg), xk_trips, total.sec, total.usec);
        msgDestroy(&savedMsg);
#else
                msgLen(dg), xk_trips, total.sec, total.usec);
#endif
#else /* TIME */
	printf("\nlen = %4d, %d trips\n", msgLen(dg), xk_trips);
#endif /* TIME */
        /* clear out msga when xk_simul>1 */
        if (xk_simul>1) {
            for (i=0; i < msgi; i++) {
                msgDestroy(&msga[i]);
            }
            msgi=0;
        }
#ifdef PROFILE
	endProfile();
#endif
    }
#endif /* ALL_TOGETHER */
    return XK_SUCCESS;
}


#endif /* STREAM_TEST */
