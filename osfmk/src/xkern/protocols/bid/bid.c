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
 * bid.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * bid.c,v
 * Revision 1.22.1.4.1.2  1994/09/07  04:18:50  menze
 * OSF modifications
 *
 * Revision 1.22.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.22.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.22.1.2  1994/07/22  19:45:18  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.22.1.1  1994/04/14  00:27:05  menze
 * Uses allocator-based message interface
 *
 * Revision 1.22  1993/12/13  21:06:46  menze
 * Modifications from UMass:
 *
 *   [ 93/09/21          nahum ]
 *   Removed char * cast in mapResolve call
 *
 */

/*
 * Protocol initialization, Session creation/destruction,
 * header load/store routines, control (registration/deregistration)
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/bidctl.h>
#include <xkern/protocols/bid/bid_i.h>
#include <xkern/include/prot/bid.h>

static void		activateSessn( XObj );
static xkern_return_t	bidClose( XObj );
static int		bidControlProtl( XObj, int, char *, int );
static int		bidControlSessn( XObj, int, char *, int );
static XObj		bidCreateSessn( XObj, XObj, XObj, ActiveKey *, Path );
static void		bidHdrDump( BidHdr *, char * );
static long		bidHdrLoad( void *, char *, long, void * );
static void		bidHdrStore( void *, char *, long, void * );
static XObj		bidNewSessn( XObj, XObj, XObj, ActiveKey *, Path );
static xkern_return_t	bidOpenDisableAll( XObj, XObj );
static xmsg_handle_t	bidPush( XObj, Msg );
static int		bootidChanged( void *, void *, void * );
static void		getProtlFuncs( XObj );
static xkern_return_t	getSessnFuncs( XObj );


#if XK_DEBUG
int	tracebidp;
#endif /* XK_DEBUG */


xkern_return_t
bid_init(
    XObj	self)
{
    Part_s	p;
    PState	*ps = (PState *)self->state;

    xTrace0(bidp, TR_GROSS_EVENTS, "bid Init");
    if ( ! (ps = pathAlloc(self->path, sizeof(PState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
    self->state = ps;
    if ( ! xIsProtocol(xGetDown(self, BID_XPORT_I)) ) {
	xTrace0(bidp, TR_ERRORS,
		"bid could not get transport protocol -- aborting init");
	return XK_FAILURE;
    }
    if ( ! xIsProtocol(xGetDown(self, BID_CTL_I)) ) {
	xTrace0(bidp, TR_ERRORS,
		"bid could not get control protocol -- aborting init");
	return XK_FAILURE;
    }
    if ( xControl(xGetDown(self, BID_CTL_I), BIDCTL_GET_LOCAL_BID,
		  (char *)&ps->myBid, sizeof(BootId)) < (int)sizeof(BootId) ) {
	xTrace0(bidp, TR_ERRORS, "bid could not get my bid -- aborting init");
	return XK_FAILURE;
    }
#if BID_CHECK_CLUSTERID
    if ( xControl(xGetDown(self, BID_CTL_I), BIDCTL_GET_CLUSTERID,
		  (char *)&ps->myCluster, sizeof(ClusterId)) < (int)sizeof(ClusterId) ) {
	xTrace0(bidp, TR_ERRORS, "bid could not get my clusterid -- aborting init");
	return XK_FAILURE;
    }
#endif /* BID_CHECK_CLUSTERID */
    xTrace1(bidp, TR_GROSS_EVENTS, "bid using id == %x", ps->myBid);
    ps->activeMap = mapCreate(BID_ACTIVE_MAP_SIZE, sizeof(ActiveKey),
			      self->path);
    ps->passiveMap = mapCreate(BID_PASSIVE_MAP_SIZE, sizeof(PassiveKey),
			       self->path);
    if ( ! ps->activeMap || ! ps->passiveMap ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
    semInit(&ps->sessnCreationSem, 1);
    getProtlFuncs(self);
    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, BID_XPORT_I), &p)
							== XK_FAILURE ) {
	xTrace0(bidp, TR_ERRORS, "openEnable failed in bid init");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static XObj
bidOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p,
    Path	path)
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    ActiveKey	key;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bidOpen");
    if ( (key.lls = xOpen(self, self, xGetDown(self, 0), p, path))
		== ERR_XOBJ ) {
	xTrace0(bidp, TR_MAJOR_EVENTS, "bid could not open lls");
	return ERR_XOBJ;
    }
    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTrace0(bidp, TR_ERRORS, "bid open couldn't get hlpNum");
	xClose(key.lls);
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &key, &s) == XK_SUCCESS ) {
	xTrace0(bidp, TR_MAJOR_EVENTS, "bid open found existing sessn");
	xClose(key.lls);
	return s;
    }
    s = bidCreateSessn(self, hlpRcv, hlpType, &key, path);
    if ( s == ERR_XOBJ ) {
	xClose(key.lls);
	return ERR_XOBJ;
    }
    xTrace1(bidp, TR_DETAILED, "bidOpen returning %x", s);
    return s;
}


static long
bidHdrLoad(
    void	*hdr,
    char 	*src,
    long	len,
    void	*arg)
{
    xAssert( len == sizeof(BidHdr) );
    bcopy(src, hdr, len);
    ((BidHdr *)hdr)->hlpNum = ntohl(((BidHdr *)hdr)->hlpNum);
    return len;
}


static void
bidHdrStore(
    void	*hdr,
    char 	*dst,
    long	len,
    void	*arg)
{
    BidHdr	h;

    xAssert( len == sizeof(BidHdr) );
    h.hlpNum = htonl(((BidHdr *)hdr)->hlpNum);
    h.srcBid = ((BidHdr *)hdr)->srcBid;
    h.dstBid = ((BidHdr *)hdr)->dstBid;
#if BID_CHECK_CLUSTERID
    h.srcCluster = ((BidHdr *)hdr)->srcCluster;
#endif /* BID_CHECK_CLUSTERID */
    bcopy((char *)&h, dst, len);
}


static XObj
bidCreateSessn(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    ActiveKey	*key,
    Path	path)
{
    PState	*ps = (PState *)self->state;
    XObj	s;

    /* 
     * We might block (but only briefly), so we'll only allow one
     * thread to create a session at a time to avoid map binding
     * conflicts. 
     */
    semWait(&ps->sessnCreationSem);
    /* 
     * Check again now that we have the lock
     */
    if ( mapResolve(ps->activeMap, key, &s) == XK_FAILURE ) {
	s = bidNewSessn(self, hlpRcv, hlpType, key, path);
    }
    semSignal(&ps->sessnCreationSem);
    return s;
}


static XObj
bidNewSessn(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    ActiveKey	*key,
    Path	path)	    
{
    XObj	s, llpCtl;
    PState	*ps = (PState *)self->state;
    SState	*ss;
    Part_s	part;
    IPhost	peer;
    
    if ( xControl(key->lls, GETPEERHOST, (char *)&peer, sizeof(IPhost))
		< (int)sizeof(IPhost) ) {
	return ERR_XOBJ;
    }
    s = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 1, &key->lls, path);
    if ( s == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    llpCtl = xGetDown(self, BID_CTL_I);
    partInit(&part, 1);
    partPush(part, &peer, sizeof(peer));
    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return ERR_XOBJ;
    }
    ss->hdr.hlpNum = key->hlpNum;
    ss->hdr.srcBid = ps->myBid;
#if BID_CHECK_CLUSTERID
    ss->hdr.srcCluster = ps->myCluster;
#endif /* BID_CHECK_CLUSTERID */
    ss->peer = peer;
    s->state = ss;
    if ( mapBind(ps->activeMap, key, s, &s->binding) != XK_SUCCESS ) {
	xDestroy(s);
	return ERR_XOBJ;
    }
    /* 
     * Register this protocol's interest in this host with the control protocol
     */
    if ( xOpenEnable(self, self, llpCtl, &part) == XK_SUCCESS ) {
	BidctlBootMsg	msg;
	
	msg.h = peer;
	msg.id = 0;
	if ( xControl(llpCtl, BIDCTL_GET_PEER_BID, (char *)&msg,
		      sizeof(msg)) == (int)sizeof(msg) ) {
	    ss->hdr.dstBid = msg.id;
	    if ( msg.id != 0 ) {
		activateSessn(s);
	    }
	    return s;
	} else {
	    xTrace1(bidp, TR_ERRORS,
		    "bid CreateSessn: couldn't get peer BID for %s",
		    ipHostStr(&peer));
	}
	partInit(&part, 1);
	partPush(part, &peer, sizeof(peer));
	xOpenDisable(self, self, llpCtl, &part);
    } else {
	xTrace0(bidp, TR_ERRORS, "bidCreateSessn couldn't openEnable BIDCTL");
    }
    xDestroy(s);
    return ERR_XOBJ;
}


static xkern_return_t
bidClose(
    XObj	self)
{
    XObj	myProtl = xMyProtl(self);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;
    XObj	lls;
    Part_s	part;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bid Close");
    lls = xGetDown(self, 0);

    if ( mapRemoveBinding(ps->activeMap, self->binding) == XK_FAILURE ) {
	xAssert(0);
	return XK_FAILURE;
    }
    partInit(&part, 1);
    partPush(part, &ss->peer, sizeof(ss->peer));
    if ( xOpenDisable(myProtl, myProtl, xGetDown(myProtl, BID_CTL_I), &part)
				== XK_FAILURE ) {
	xTrace0(bidp, TR_ERRORS, "bidClose couldn't openDisable cntrl protl");
    }
    xAssert(xIsSession(lls));
    xClose(lls);
    xDestroy(self);
    return XK_SUCCESS;
}


static xkern_return_t
bidDemux(
    XObj	self,
    XObj	lls,
    Msg		m)
{
    PState	*ps = (PState *)self->state;
    BidHdr	hdr;
    XObj	s;
    Enable	*e;
    ActiveKey	key;

    xTrace0(bidp, TR_EVENTS, "bid demux");
    if ( msgPop(m, bidHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTrace0(bidp, TR_ERRORS, "bid demux -- msg pop failed");
	return XK_SUCCESS;
    }
    xIfTrace(bidp, TR_DETAILED) {
	bidHdrDump(&hdr, "INCOMING");
    }
    key.lls = lls;
    key.hlpNum = hdr.hlpNum;
    if ( mapResolve(ps->activeMap, (char *)&key, &s) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.hlpNum, &e) == XK_FAILURE ) {
	    xTrace1(bidp, TR_SOFT_ERRORS,
		    "bid demux -- no protl for hlpNum %d ", hdr.hlpNum);
	    return XK_FAILURE;
	} else {
	    xTrace1(bidp, TR_DETAILED,
		    "bid demux -- found enable for hlpNum %d ", hdr.hlpNum);
	}
	s = bidCreateSessn(self, e->hlpRcv, e->hlpType, &key, msgGetPath(m));
	if ( s == ERR_XOBJ ) {
	    return XK_FAILURE;
	}
	xDuplicate(lls);
	xOpenDone(e->hlpRcv, s, self, msgGetPath(m));
    } else {
	xAssert(s->rcnt > 0);
    }
    return xPop(s, lls, m, &hdr);
}


static xkern_return_t
bidOpenEnable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    hlpNum = relProtNum(hlpType, self);
    xTrace1(bidp, TR_MAJOR_EVENTS, "bid openEnable binding key %d",
	    hlpNum);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}


static xkern_return_t
bidOpenDisable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bid openDisable");
    hlpNum = relProtNum(hlpType, self);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}


static xkern_return_t
bidOpenDisableAll(
    XObj	self,
    XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bid openDisableAll");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

static xkern_return_t
bidPop(
    XObj	self,
    XObj	lls,
    Msg		m,
    void	*inHdr)
{
    SState	*ss = (SState *)self->state;
    BidHdr	*hdr = (BidHdr *)inHdr;
    
    xAssert(hdr);
#if BID_CHECK_CLUSTERID
    if ( hdr->srcCluster != ss->hdr.srcCluster ) {
	xTrace2(bidp, TR_MAJOR_EVENTS,
		"bidPop: mismatch of my clusterid (%x (hdr) vs. %x (state))",
		hdr->srcCluster, ss->hdr.srcCluster);
	return XK_FAILURE;
    }
#endif /* BID_CHECK_CLUSTERID */
    if ( hdr->dstBid != ss->hdr.srcBid ) {
	Msg_s	nullMsg;

	xTrace2(bidp, TR_MAJOR_EVENTS,
		"bidPop: mismatch of my bid (%x (hdr) vs. %x (state))",
		hdr->dstBid, ss->hdr.srcBid);
	/* 
	 * Send a null message to my peer so it can ask its control
	 * protocol to straighten it out.
	 */
	if ( msgConstructContig(&nullMsg, msgGetPath(m), 0, BID_STACK_LEN)
	    	== XK_SUCCESS ) {
	    bidPush(self, &nullMsg);
	    msgDestroy(&nullMsg);
	} else {
	    xTraceP0(self, TR_SOFT_ERRORS, "Couldn't allocate message");
	}
	return XK_FAILURE;
    }
    if ( hdr->srcBid != ss->hdr.dstBid ) {
	XObj		llpCtl = xGetDown(xMyProtl(self), BID_CTL_I);
	BidctlBootMsg	msg;

	xTrace2(bidp, TR_MAJOR_EVENTS,
		"bidPop: mismatch of peer bid (%x (hdr) vs. %x (state))",
		hdr->srcBid, ss->hdr.srcBid);
	msg.h = ss->peer;
	msg.id = hdr->srcBid;
	xControl(llpCtl, BIDCTL_GET_PEER_BID, (char *)&msg, sizeof(msg));
	return XK_FAILURE;
    }
    return xDemux(self, m);
}


static int
bidControlProtl(
    XObj	self,
    int		op,
    char 	*buf,
    int		len)
{
    PState	*ps = (PState *)self->state;

    switch ( op ) {

	case BIDCTL_FIRST_CONTACT:
	case BIDCTL_PEER_REBOOTED:
	    mapForEach(ps->activeMap, bootidChanged, (BidctlBootMsg *)buf);
	    return 0;

	case GETMAXPACKET:
	case GETOPTPACKET:
	    if (xControl(xGetDown(self, 0), op, buf, len) < (int)sizeof(int)) {
		return -1;
	    }
	    *(int *)buf -= sizeof(BidHdr);
	    return (sizeof(int));

	default:
	    return xControl(xGetDown(self, 0), op, buf, len);
    }
}


static int
bootidChanged(
    void	*key,
    void	*val,
    void	*arg)
{
    XObj		s = (XObj)val;
    BidctlBootMsg	*m = (BidctlBootMsg *)arg;
    SState		*ss;

    xAssert(xIsSession(s));
    xAssert(arg);
    ss = (SState *)s->state;
    if ( ! IP_EQUAL(ss->peer, m->h) ) {
	xTrace3(bidp, TR_EVENTS,
		"bid sessn %x (host %s) ignoring reboot message for %s",
		s, ipHostStr(&ss->peer), ipHostStr(&m->h));
	return 1;
    }
    activateSessn(s);
    xTrace3(bidp, TR_MAJOR_EVENTS, "bid session %x resetting BID of %s to %x",
	    s, ipHostStr(&m->h), m->id);
    ss->hdr.dstBid = m->id;
    return MFE_CONTINUE;
}


static int
bidControlSessn(
    XObj	self,
    int		op,
    char 	*buf,
    int		len)
{
    switch ( op ) {

	case GETMAXPACKET:
	case GETOPTPACKET:
	    if (xControl(xGetDown(self, 0), op, buf, len) < (int)sizeof(int)) {
		return -1;
	    }
	    *(int *)buf -= sizeof(BidHdr);
	    return (sizeof(int));

	default:
	    return xControl(xGetDown(self, 0), op, buf, len);
    }
}


static xmsg_handle_t
bidPush(
    XObj	self,
    Msg		m)
{
    SState	*ss = (SState *)self->state;

    xTrace0(bidp, TR_EVENTS, "bidPush");
    xIfTrace(bidp, TR_DETAILED) {
	bidHdrDump(&ss->hdr, "OUTGOING");
    }
    if ( msgPush(m, bidHdrStore, &ss->hdr, sizeof(BidHdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "msg push fails");
	return XMSG_ERR_HANDLE;
    }
    return xPush(xGetDown(self, 0), m);
}


static void
bidHdrDump(
    BidHdr	*h,
    char	*str)
{
    xTrace1(bidp, TR_ALWAYS, "Bid %s header:", str);
    xTrace3(bidp, TR_ALWAYS,
	    "srcBid: %.8x  dstBid: %.8x  hlpNum: %d",
	    h->srcBid, h->dstBid, h->hlpNum);
}


static void
getProtlFuncs(
    XObj	p)
{
    p->open = bidOpen;
    p->control = bidControlProtl;
    p->demux = bidDemux;
    p->openenable = bidOpenEnable;
    p->opendisable = bidOpenDisable;
    p->opendisableall = bidOpenDisableAll;
}


static void
activateSessn(
    XObj	s)
{
    s->push = bidPush;
    s->pop = bidPop;
}


static xkern_return_t
getSessnFuncs(
    XObj	s)
{
    s->control = bidControlSessn;
    s->close = bidClose;
    return XK_SUCCESS;
}


