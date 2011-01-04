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
 * bidctl.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * bidctl.c,v
 * Revision 1.29.1.4.1.2  1994/09/07  04:18:49  menze
 * OSF modifications
 *
 * Revision 1.29.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.29.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.29.1.2  1994/07/22  19:45:58  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.29.1.1  1994/04/14  00:31:06  menze
 * Uses allocator-based message interface
 *
 * Revision 1.29  1994/01/27  21:45:42  menze
 *   [ 1994/01/13          menze ]
 *   Now uses library routines for rom options
 *
 * Revision 1.28  1993/12/13  21:09:23  menze
 * Modifications from UMass:
 *
 *   [ 93/09/21          nahum ]
 *   Got rid of char cast in mapResolve calls
 *
 */

/* 
 * BIDCTL -- BootId Control Protocol
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/bidctl.h>
#include <xkern/protocols/bidctl/bidctl_i.h>
#include <xkern/include/prot/vnet.h>
#include <xkern/include/romopt.h>

static long		bidctlHdrLoad( void *, char *, long, void * );
static int		bidctlControl( XObj, int, char *, int );
#if XK_DEBUG
static char *		flagStr( int );
#endif /* XK_DEBUG */   
static BidctlState *	getState( XObj, IPhost *, int );
static void		getProtlFuncs( XObj );
#  ifdef ALLOW_TIMEOUT_IN_GETSTATE_BLOCKING
static void		openGiveUp( Event, void * );
#  endif
static void		processRomFile( void );
static xkern_return_t   readBcastOpt( XObj, char **, int, int, void * );
static xkern_return_t   readNoBcastOpt( XObj, char **, int, int, void * );
static xkern_return_t   readClusteridOpt( XObj, char **, int, int, void * );
static void		verifyPeerBid( BidctlState *bs, BootId );

static int	willBcastBoot = 
#ifdef BIDCTL_NO_BOOT_BCAST
  				0
#else
				1
#endif
;

static XObjRomOpt	rOpts[] = {
    { "bcast", 2, readBcastOpt },
    { "nobcast", 2, readNoBcastOpt },
    { "clusterid", 3, readClusteridOpt }
};

#define MIN(x,y) 		((x)>(y) ? (y) : (x))


xkern_return_t
bidctl_init( self )
    XObj	self;
{
    Part_s	p;
    PState	*ps;
    XObj	llp;
    Event	ev;

    xTrace0(bidctlp, TR_GROSS_EVENTS, "bidctl Init");
    if ( ! (ps = pathAlloc(self->path, sizeof(PState))) ) {
	return XK_FAILURE;
    }
    self->state = ps;
    findXObjRomOpts(self, rOpts, sizeof(rOpts)/sizeof(XObjRomOpt), 0);
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xTrace0(bidctlp, TR_ERRORS,
		"bidctl could not get lower protocol -- aborting init");
	return XK_FAILURE;
    }
    ps->bsMap = mapCreate(BIDCTL_BSMAP_SIZE, sizeof(IPhost), self->path);
    if ( ! ps->bsMap ) {
	pathFree(ps);
	return XK_FAILURE;
    }
    ps->myBid = bidctlNewId();
    xTrace1(bidctlp, TR_GROSS_EVENTS, "bidctl using id == %x", ps->myBid);
    getProtlFuncs(self);
    if ( ! (ev = evAlloc(self->path)) ) {
	return XK_FAILURE;
    }
    evSchedule(ev, bidctlTimer, ps, BIDCTL_TIMER_INTERVAL);
    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, 0), &p) == XK_FAILURE ) {
	xTrace0(bidctlp, TR_ERRORS, "openEnable failed in bidctl init");
    }
    if ( willBcastBoot ) {
	bidctlBcastBoot(self);
    }
    return XK_SUCCESS;
}


void
bidctlSemWait( bs )
    BidctlState	*bs;
{
    xTrace1(bidctlp, TR_DETAILED,
	    "BIDCTL blocking thread on state for peer %s",
	    ipHostStr(&bs->peerHost));
    xAssert(bs->fsmState == BIDCTL_QUERY_S || bs->fsmState == BIDCTL_OPEN_S);
    bs->rcnt++;
    bs->waitSemCount++;
    semWait(&bs->waitSem);
    bs->rcnt--;
    xTrace1(bidctlp, TR_DETAILED,
	    "BIDCTL thread blocked on state for peer %s wakes up",
	    ipHostStr(&bs->peerHost));
}


void
bidctlReleaseWaiters( bs )
    BidctlState	*bs;
{
    xTrace1(bidctlp, TR_EVENTS, "bidctlRelease -- waiters: %d",
	    bs->waitSemCount);
    while ( bs->waitSemCount > 0 ) {
	semSignal(&bs->waitSem);
	bs->waitSemCount--;
    }
}


void
bidctlTimeout( ev, arg )
    Event	ev;
    void	*arg;
{
    BidctlState	*bs = (BidctlState *)arg;

    if ( evIsCancelled(ev) ) {
	return;
    }
    /* 
     * Detach this event
     */
    switch ( bs->fsmState ) {
      case BIDCTL_OPEN_S:
	bs->timeout = MIN( bs->timeout * BIDCTL_OPEN_TIMEOUT_MULT,
			   BIDCTL_OPEN_TIMEOUT_MAX );
	break;
	
      case BIDCTL_QUERY_S:
	bs->timeout = MIN( bs->timeout * BIDCTL_QUERY_TIMEOUT_MULT,
			   BIDCTL_QUERY_TIMEOUT_MAX );
	break;

      default:
	/* 
	 * This event should have been cancelled!
	 */
	xTrace1(bidctlp, TR_ERRORS,
		"openTimeout -- state == %s event no longer needed",
		bidctlFsmStr(bs->fsmState));
	return;
    }
    xTrace1(bidctlp, TR_EVENTS, "bidctlTimeout -- trying again (%d)",
	    bs->retries);
    xTrace1(bidctlp, TR_EVENTS, "bidctlTimeout -- next timeout in %d msec",
	    bs->timeout / 1000);
    evSchedule(bs->ev, bidctlTimeout, bs, bs->timeout);
    bidctlOutput(bs, BIDCTL_REPEAT_QUERY, 0, 0);
}


/*
 * load header from potentially unaligned msg buffer.
 * Result of checksum calculation will be in hdr->sum.
 */
static long
bidctlHdrLoad( hdr, src, len, arg )
    void	*hdr, *arg;
    char 	*src;
    long	len;
{
    xAssert( len == sizeof(BidctlHdr) );
    bcopy(src, hdr, len);
    xTrace1(bidctlp, TR_DETAILED, "original checksum: %x",
	    ((BidctlHdr *)hdr)->sum);
    ((BidctlHdr *)hdr)->sum = ~ ocsum((u_short *)hdr, len/2);
    ((BidctlHdr *)hdr)->flags = ntohs(((BidctlHdr *)hdr)->flags);
    return len;
}


void
bidctlHdrStore( hdr, dst, len, arg )
    void	*hdr, *arg;
    char 	*dst;
    long	len;
{
    BidctlHdr	*h = (BidctlHdr *)hdr; 

    xAssert( len == sizeof(BidctlHdr) );
    h->flags = htons(h->flags);
    h->sum = 0;
    h->sum = ~ocsum((u_short *)h, len/2); 
    xAssert( (~ocsum((u_short *)h, len/2) & 0xffff) == 0 );
    xTrace1(bidctlp, TR_DETAILED, "outgoing checksum: %x", h->sum);
    bcopy((char *)h, dst, len);
}


#ifdef ALLOW_TIMEOUT_IN_GETSTATE_BLOCKING

static void
openGiveUp( ev, arg )
    Event	ev;
    void	*arg;
{
    BidctlState	*bs = (BidctlState *)arg;
    
    semSignal(&bs->waitSem);
}

#endif


static BidctlState *
bsAllocate( Path p )
{
    BidctlState	*bs;

    if ( ! (bs = pathAlloc(p, sizeof(BidctlState))) ) {
	return 0;
    }
    bzero((char *)bs, sizeof(BidctlState));
    if ( ! (bs->ev = evAlloc(p)) ) {
	pathFree(bs);
	return 0;
    }
    if ( ! (bs->hlpMap = mapCreate(BIDCTL_HLPMAP_SIZE, sizeof(XObj), p)) ) {
	evDetach(bs->ev);
	pathFree(bs);
	return 0;
    }
    return bs;
}


static BidctlState *
getState( self, peer, create )
    XObj	self;
    IPhost	*peer;
    int		create;
{
    PState		*ps = (PState *)self->state;
    BidctlState		*bs;
    XObj		lls;
    Part_s		part;
    xkern_return_t 	xkr;

    /* 
     * Find the appropriate bidctl state
     */
    xTrace1(bidctlp, TR_MORE_EVENTS,
	    "getBidState -- finding state for host %s", ipHostStr(peer));
    if ( mapResolve(ps->bsMap, (char *)peer, &bs) == XK_FAILURE ) {
	if ( ! create ) {
	    xTrace0(bidctlp, TR_MORE_EVENTS,
		    "getBidState -- declining to create new state");
	    return 0;
	}
	xTrace0(bidctlp, TR_MORE_EVENTS, "getBidState -- creating new state");
	partInit(&part, 1);
	partPush(part, peer, sizeof(peer));
	lls = xOpen(self, self, xGetDown(self, 0), &part, xMyPath(self));
	if ( lls == ERR_XOBJ ) {
	    xTrace0(bidctlp, TR_ERRORS,
		    "getBidState -- could not open lls");
	    return 0;
	}
	if ( ! (bs = bsAllocate(self->path)) ) {
	    xTraceP0(self, TR_ERRORS, "Allocation error");
	    xClose(lls);
	    return 0;
	}
	bs->myProtl = self;
	bs->peerHost = *peer;
	bs->fsmState = BIDCTL_INIT_S;
	bs->lls = lls;
	semInit(&bs->waitSem, 0);
	xkr = mapBind(ps->bsMap, peer, bs, &bs->bind);
	switch( xkr ) {
	  case XK_SUCCESS:
	    /* 
	     * Send an initial query for the remote boot id -- it actually gets
	     * started in the transition out of this state (i.e., not here)
	     */
	    xTrace0(bidctlp, TR_MORE_EVENTS, "getBidState -- starting query");
	    bs->timeout = BIDCTL_OPEN_TIMEOUT;
	    evSchedule(bs->ev, bidctlTimeout, bs, bs->timeout);
	    break;

	  case XK_ALREADY_BOUND:
	    /* 
	     * We can block in the xOpen above, during which some
	     * other thread may have created the state
	     */
	    xTraceP0(self, TR_SOFT_ERRORS,
		    "mapBind failed ... state already exists");
	    bidctlDestroy(bs);
	    xkr = mapResolve(ps->bsMap, (char *)peer, &bs);
	    xAssert(xkr == XK_SUCCESS);
	    break;

	  default:
	    xTraceP0(self, TR_SOFT_ERRORS, "mapBind failed");
	    bidctlDestroy(bs);
	    bs = 0;
	    break;
	}
    } else {
	xTraceP0(self, TR_MORE_EVENTS, "getBidState -- state already exists");
    }
    xTraceP2(self, TR_MORE_EVENTS, "getBidState returns %x (rcnt %d)",
	     bs, bs ? bs->rcnt : 0);
    return bs;
}


xkern_return_t
bidctlDestroy( bs )
    BidctlState	*bs;
{
    PState	*ps = (PState *)bs->myProtl->state;

    xTrace0(bidctlp, TR_DETAILED, "bidctlDestroy");
    evCancel(bs->ev);
    evDetach(bs->ev);
    xAssert( bs->rcnt == 0 );
    if ( bs->bind ) {
	if ( mapRemoveBinding(ps->bsMap, bs->bind) == XK_FAILURE ) {
	    xTrace0(bidctlp, TR_ERRORS, "bidctlDestroy -- mapUnbind fails!");
	    return XK_FAILURE;
	}
    }
    xAssert(xIsXObj(bs->lls));
    xClose(bs->lls);
    mapClose(bs->hlpMap);
    allocFree(bs);
    return XK_SUCCESS;
}


static xkern_return_t
bidctlDemux( 
    XObj	self,
    XObj	lls,
    Msg		m )
{
    PState	*ps = (PState *)self->state;
    BidctlHdr	hdr;
    BidctlState	*bs;
    IPhost	peer;

    xTrace0(bidctlp, TR_EVENTS, "bidctl demux");
    if ( msgPop(m, bidctlHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTrace0(bidctlp, TR_ERRORS, "bidctl demux -- msg pop failed");
	return XK_SUCCESS;
    }
    if ( xControl(lls, GETPEERHOST, (char *)&peer, sizeof(IPhost)) < 0 ) {
	return XK_FAILURE;
    }
    xIfTrace(bidctlp, TR_DETAILED) {
	bidctlHdrDump(&hdr, "INCOMING", &peer);
    }
    if ( hdr.sum ) {
	xTrace1(bidctlp, TR_ERRORS, "bidctl demux -- checksum (%x) non-zero",
		hdr.sum);
	return XK_FAILURE;
    }
    xIfTrace(bidctlp, TR_MAJOR_EVENTS) {
	if ( hdr.flags & BIDCTL_BCAST_F ) {
	    xTrace1(bidctlp, TR_ALWAYS,
		    "boot broadcast received from peer %s", ipHostStr(&peer));
	}
    }
    if ( mapResolve(ps->bsMap, (char *)&peer, &bs) == XK_FAILURE ) {
	/* 
	 * Active session does not exist.  
	 */
	bs = getState(self, &peer, ! (hdr.flags & BIDCTL_BCAST_F));
	if ( bs == 0 ) {
	    return XK_FAILURE;
	}
    }
    bs->timer = 0;
    bidctlTransition(bs, &hdr);
    return XK_SUCCESS;
}


static int
getHostFromPart( 
    Part	p,
    IPhost	*h )
{
    void	*ptr;

    if ( partLen(p) < 1 ) {
	return 1;
    }
    if ( (ptr = partPop(*p)) == 0 ) {
	return 1;
    }
    *h = *(IPhost *)ptr;
    return 0;
}


static xkern_return_t
bidctlOpenEnable( 
    XObj   	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p )
{
    IPhost	peer;
    BidctlState	*bs;
    
    if ( getHostFromPart(p, &peer) ) {
	xTrace0(bidctlp, TR_SOFT_ERRORS, "bidctlOpenEnable participant error");
	return XK_FAILURE;
    }
    xTrace1(bidctlp, TR_EVENTS, "bidctl openEnable, peer %s",
	    ipHostStr(&peer));
    if ( xControl(xGetDown(self, 0), VNET_ISMYADDR, (char *)&peer,
		  sizeof(IPhost)) > 0 ) {
	xTrace0(bidctlp, TR_EVENTS, "bidctlOpenEnable -- local host");
	return XK_SUCCESS;
    }
    if ( (bs = getState(self, &peer, 1)) == 0 ) {
	return XK_FAILURE;
    }
    bidctlTransition(bs, 0);
    if ( defaultOpenEnable(bs->hlpMap, hlpRcv, hlpRcv, &hlpRcv)
		!= XK_SUCCESS ) {
	return XK_FAILURE;
    }
    bs->rcnt++;
    xTraceP1(self, TR_MORE_EVENTS, "openEnable -- state rcnt == %d", bs->rcnt);
    return XK_SUCCESS;
}


static xkern_return_t
bidctlOpenDisable(
    XObj   	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p )
{
    IPhost	peer;
    BidctlState	*bs;
    
    if ( getHostFromPart(p, &peer) ) {
	xTrace0(bidctlp, TR_SOFT_ERRORS,
		"bidctlOpenDisable participant error");
	return XK_FAILURE;
    }
    xTrace1(bidctlp, TR_EVENTS, "bidctl openDisable, peer %s",
	    ipHostStr(&peer));
    if ( xControl(xGetDown(self, 0), VNET_ISMYADDR, (char *)&peer,
		  sizeof(IPhost)) > 0 ) {
	xTrace0(bidctlp, TR_EVENTS, "bidctlOpenDisable -- local host");
	return XK_SUCCESS;
    }
    if ( (bs = getState(self, &peer, 0)) == 0 ) {
	return XK_FAILURE;
    }
    if ( defaultOpenDisable(bs->hlpMap, hlpRcv, hlpRcv, &hlpRcv)
		!= XK_SUCCESS ) {
	return XK_FAILURE;
    }
    bs->rcnt--;
    xTraceP1(self, TR_MORE_EVENTS, "openDisable -- state rcnt == %d", bs->rcnt);
    return XK_SUCCESS;
}


static int
bidctlControl( self, op, buf, len )
    XObj	self;
    int		op, len;
    char	*buf;
{
    PState	*ps = (PState *)self->state;

    switch ( op ) {

      case BIDCTL_GET_LOCAL_BID:
	checkLen(len, sizeof(BootId));
	*(BootId *)buf = ps->myBid;
	return sizeof(BootId);

      case BIDCTL_GET_CLUSTERID:
	checkLen(len, sizeof(ClusterId));
	*(ClusterId *)buf = ps->myCluster;
	return sizeof(ClusterId);
	
      case BIDCTL_GET_PEER_BID:
      case BIDCTL_GET_PEER_BID_BLOCKING:
	{
	    BidctlBootMsg	*m = (BidctlBootMsg *)buf;
	    BidctlState		*bs;

	    checkLen(len, sizeof(BidctlBootMsg));
	    xTrace2(bidctlp, TR_EVENTS, "BIDCTL_GET_PEER_BID%s peer %s",
		    (op == BIDCTL_GET_PEER_BID) ? "" : "_BLOCKING",
		    ipHostStr(&m->h));
	    if ( xControl(xGetDown(self, 0), VNET_ISMYADDR, (char *)&m->h,
			  sizeof(IPhost)) > 0 ) {
		m->id = ps->myBid;
		return sizeof(BidctlBootMsg);
	    }
	    if ( (bs = getState(self, &m->h, 1)) == 0 ) {
		return 0;
	    }
	    /* 
	     * Transition to take us from INIT to OPEN state if necessary
	     */
	    bidctlTransition(bs, 0);
	    if ( op == BIDCTL_GET_PEER_BID ) {
		if ( m->id != 0 ) {
		    /* 
		     * The caller has offered a suggested BootId
		     */
		    if ( m->id != bs->peerBid &&
			bs->fsmState == BIDCTL_NORMAL_S ) {
			/* 
			 * The caller has reason to believe that the input
			 * bootid is correct.  We'll check it out.
			 */
			bidctlStartQuery(bs, 1);
		    }
		}
	    } else {
		/* 
		 * BLOCKING operation
		 */
		if ( m->id == 0 ) {
		    /* 
		     * Caller will simply wait on any BID
		     */
		    if ( bs->peerBid == 0 ) {
			bidctlSemWait(bs);
		    }
		} else {
		    verifyPeerBid(bs, m->id);
		}
	    }
	    m->id = bs->peerBid;
	    return sizeof(BidctlBootMsg);
	}

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}


/* 
 * Verify -- caller wants verification that the
 * suggested BID is still reasonable or
 * confirmation (via peer handshake *after* the request)
 * that it is incorrect.  This condition holds when we return.
 */
static void
verifyPeerBid( bs, sugBid )
    BidctlState	*bs;
    BootId	sugBid;
{
    if ( sugBid == bs->peerBid ) {
	return;
    }
    xTrace2(bidctlp, TR_EVENTS,
	    "BIDCTL verify -- suggested bid %x != state bid %x",
	    sugBid, bs->peerBid);
    switch ( bs->fsmState ) {
      case BIDCTL_QUERY_S:
      case BIDCTL_OPEN_S:
	/* 
	 * We are already waiting on someone else's query. 
	 */
	bidctlSemWait(bs);
	if ( bs->fsmState == BIDCTL_QUERY_S ) {
	    /* 
	     * We're now waiting on (another) someone else's query, but
	     * we know that it took place after the request.
	     */
	    bidctlSemWait(bs);
	    break;
	} 
	xAssert( bs->fsmState == BIDCTL_NORMAL_S );
	if ( sugBid == bs->peerBid ) {
	    break;
	}
	/* 
	 * Fallthrough -- send our own request
	 */
	
      case BIDCTL_NORMAL_S:
	/* 
	 * The caller has reason to believe that
	 * the input bootid is correct.  We'll check
	 * it out.
	 */
	xTrace0(bidctlp, TR_EVENTS, "starting user-initiated query sequence");
	xTrace2(bidctlp, TR_EVENTS, "\t(cur == %x, supplied == %x)",
		bs->peerBid, sugBid);
	bidctlStartQuery(bs, 1);
	bidctlSemWait(bs);
	break;
	
      default:
	xAssert(0);
    }
}


#if XK_DEBUG

static char *
flagStr( f )
    int	f;
{
    static char	s[80];

    if ( f == 0 ) {
	return "NONE ";
    }
    s[0] = 0;
    if ( f & BIDCTL_QUERY_F ) {
	(void)strcat(s, "QUERY ");
    }
    if ( f & BIDCTL_RESPONSE_F ) {
	(void)strcat(s, "RESPONSE ");
    }
    if ( f & BIDCTL_BCAST_F ) {
	(void)strcat(s, "BCAST ");
    }
    if ( s[0] == 0 ) {
	return "UNRECOGNIZED";
    }
    return s;
}

#endif /* XK_DEBUG */   


void
bidctlHdrDump( h, str, peer )
    BidctlHdr	*h;
    char	*str;
    IPhost	*peer;
{
    xTrace2(bidctlp, TR_ALWAYS, "Bidctl %s header (peer %s):",
	    str, ipHostStr(peer));
    xTrace4(bidctlp, TR_ALWAYS,
	   "flags: %s  sum: %.4x  srcBid: %.8x  dstBid: %.8x",
	    flagStr(h->flags), h->sum, h->srcBid, h->dstBid);
    xTrace2(bidctlp, TR_ALWAYS, "reqTag: %.8x  rplTag: %.8x",
	    h->reqTag, h->rplTag);
}


static void
getProtlFuncs( p )
    XObj	p;
{
    p->demux = bidctlDemux;
    p->openenable = bidctlOpenEnable;
    p->opendisable = bidctlOpenDisable;
    p->control = bidctlControl;
}

void
bidctlDispState( bs )
    BidctlState	*bs;
{
    xTrace1(bidctlp, TR_ALWAYS, "bidctl state for peer %s",
	    ipHostStr(&bs->peerHost));
    xTrace3(bidctlp, TR_ALWAYS, "%s, rcnt %d, peerbid %x",
	    bidctlFsmStr(bs->fsmState), bs->rcnt, 
	    bs->peerBid);
    xTrace2(bidctlp, TR_ALWAYS, "timer %d, retries %d",
	    bs->timer, bs->retries);
}



static xkern_return_t
readBcastOpt( self, str, nFields, line, arg )
    XObj	self;
    char	**str;
    int		nFields, line;
    void	*arg;
{
    xTrace0(bidctlp, TR_EVENTS, "rom file broadcast specified");
    willBcastBoot = 1;
    return XK_SUCCESS;
}

static xkern_return_t
readNoBcastOpt( self, str, nFields, line, arg )
    XObj	self;
    char	**str;
    int		nFields, line;
    void	*arg;
{
    xTrace0(bidctlp, TR_EVENTS, "rom file no broadcast specified");
    willBcastBoot = 0;
    return XK_SUCCESS;
}

static xkern_return_t
readClusteridOpt( self, str, nFields, line, arg )
    XObj	self;
    char	**str;
    int		nFields, line;
    void	*arg;
{
    PState	*ps = (PState *)self->state;
    
    xTrace1(bidctlp, TR_EVENTS, "rom file clusterid (%s) specified", str[2]);

    ps->myCluster = atoi(str[2]);
    return XK_SUCCESS;
}

