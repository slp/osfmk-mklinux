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
 * arp.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * arp.c,v
 * Revision 1.75.2.3.1.2  1994/09/07  04:18:23  menze
 * OSF modifications
 *
 * Revision 1.75.2.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.75.2.2  1994/07/22  19:44:24  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.75.2.1  1994/04/14  00:22:44  menze
 * Uses allocator-based message interface
 *
 * Revision 1.75  1993/12/11  00:23:52  menze
 * fixed #endif comments
 *
 * Revision 1.74  1993/11/12  20:47:04  menze
 * SendRarpReply was rewriting arp_tha address from the address of the
 * requesting host (which isn't necessarily the target host.)
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/arp.h>
#include <xkern/protocols/arp/arp_i.h>
#include <xkern/protocols/arp/arp_table.h>

int tracearpp;


#if XK_DEBUG

static char *codenames[] = {
  "impossible",
  "req",
  "rply",
  "rreq",
  "rrply"
};

#endif /* XK_DEBUG */


/* global data of arp protocol */

static ETHhost	ethBcastHost = BCAST_ETH_AD;
static IPhost	ipLocalBcastHost = { 255, 255, 255, 255 };

static int		arpControlProtl( XObj, int, char *, int );
static xkern_return_t 	arpDemux( XObj, XObj, Msg );
static xkern_return_t	arpFuncs( XObj );
static void		arpHdrStore(void *, char *, long, void *);
static long		arpHdrLoad(void *, char *, long, void *);
static xkern_return_t	arpInterfaceInit( XObj, XObj );
static void		newWait( ArpWait *, XObj );
static void 		sendArpReply( XObj, XObj, ArpHdr *, Path );
static void 		sendRarpReply( XObj, XObj, ArpHdr *, Path );

static void
arp_print(
    char *s,
    ArpHdr *m )
{
    xTrace3(arpp, TR_ALWAYS, "%s arp %s (%d) message:", s,
	    (m->arp_op > ARP_MAXOP) ? "unknown" : codenames[m->arp_op],
	    m->arp_op); 
    xTrace2(arpp, TR_ALWAYS, "  source %s @ %s",
	    ethHostStr(&m->arp_sha), ipHostStr(&m->arp_spa));
    xTrace2(arpp, TR_ALWAYS, "  target %s @ %s",
	    ethHostStr(&m->arp_tha), ipHostStr(&m->arp_tpa));
}


static xkern_return_t
arpFuncs(self)
    XObj self;
{
    self->control = arpControlProtl;
    self->demux = arpDemux;
    return XK_SUCCESS;
}


static xkern_return_t
noopDemux( 
    XObj self,
    XObj s,
    Msg msg )
{
    return XK_FAILURE;
}


static int
noopControl( 
    XObj self,
    int op,
    char *buf,
    int len )
{
    return -1;
}


xkern_return_t
arp_init(self)
    XObj self;
{
    PState	*ps;
    XObj	llp;
    
    xTrace0(arpp, 1, "ARP init");
    if ( ! (ps = pathAlloc(xMyPath(self), sizeof(PState))) ) {
	return XK_FAILURE;
    }
    self->state = ps;
    if ( ! (ps->tbl = arpTableInit(xMyPath(self))) ) {
	return XK_FAILURE;
    }
    arpPlatformInit(self);
    /* 
     * We create a rarp protocol for each arp protocol, mainly to be
     * able to find the protocol number of rarp relative to the llp
     */
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xError("ARP -- could not get llp");
	return XK_FAILURE;
    }
    ps->rarp = xCreateProtl(arpFuncs, "rarp", "", self->traceVar, 1, &llp,
			    xMyPath(self));
    if ( ps->rarp == ERR_XOBJ ) {
	xError("ARP could not create RARP protocol");
	return XK_FAILURE;
    }
    ps->rarp->state = self->state;
    arpFuncs(self);
    if ( arpInterfaceInit(self, llp) == XK_FAILURE ) {
 	xError("ARP -- error in initialization");
	self->control = noopControl;
	self->demux = noopDemux;
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xkern_return_t
arpInterfaceInit( self, llp )
    XObj self;
    XObj llp;
{
    ETHhost	localEthHost;
    PState	*ps = (PState *)self->state;
    Part_s	part;

    if ( xControl(llp, GETMYHOST, (char *)&ps->hdr.arp_sha, sizeof(ETHhost))
		< (int)sizeof(ETHhost) ) {
	xError("ARP could not get host from llp");
	return XK_FAILURE;
    }
    localEthHost = ps->hdr.arp_sha;
    /*
     * Openenable lower protocol for arps and rarps
     */
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    xOpenEnable(self, self, llp, &part);
    partPush(part, ANY_HOST, 0);
    xOpenEnable(self, ps->rarp, llp, &part);
    /*
     * Open broadcast session for this interface
     */
    partInit(&part, 1);
    partPush(part, &ethBcastHost, sizeof(ethBcastHost));
    ps->arpSessn = xOpen(self, self, llp, &part, xMyPath(self));
    if (ps->arpSessn == ERR_XOBJ) {
	xError("ARP init failure (arp)");
	return XK_FAILURE;
    }
    /* 
     * Register ourselves with the lower protocol
     */
    xControl(llp, ETH_REGISTER_ARP, (char *)&self, sizeof(XObj));
    /*
     * Create a default header for requests sent on this interface
     */
    ps->hdr.arp_hrd=1;
    ps->hdr.arp_prot=ARP_PROT;
    ps->hdr.arp_hlen=6;
    ps->hdr.arp_plen=4;
    /*
     * Get my IP address for this interface
     */
    xTrace1(arpp, 3, "My eaddr = %s", ethHostStr(&ps->hdr.arp_sha));
    while ( arpRevLookup(self, &ps->hdr.arp_spa, &ps->hdr.arp_sha) != 0 ) {
	sprintf(errBuf, 
	"ARP: Could not get my ip address for interface %s (still trying)",
		llp->name);
	xError(errBuf);
	/*
	 * Most protocols aren't going to be very useful if we can't
	 * find out our own IP address.  Keep trying.
	 */
	Delay( INIT_RARP_DELAY );
    }
    /* 
     * Lock my binding in the table so it doesn't leave the cache.
     */
    arpLock( ps->tbl, &ps->hdr.arp_spa );
    arpTblPurge( ps->tbl, &ps->hdr.arp_spa );
    return XK_SUCCESS;
}


/*
 * sendArpReply -- send an ARP reply to the sender of 'srcHdr'
 * with my ip and physical addresses on the interface 'ifs'
 */
static void
sendArpReply(
    XObj	self,
    XObj	lls,
    ArpHdr	*srcHdr,
    Path	path)
{
    ArpHdr		reply;
    Msg_s		repMsg;
    xkern_return_t	xkr;

    reply = ((PState *)self->state)->hdr;
    reply.arp_tha = srcHdr->arp_sha;
    reply.arp_tpa = srcHdr->arp_spa;
    reply.arp_op = ARP_RPLY;
    if ( msgConstructContig(&repMsg, path, 0, ARP_STACK_LEN ) == XK_FAILURE) {
	xTraceP0(self, TR_SOFT_ERRORS, "couldn't allocate reply msg");
	return;
    }
    xkr = msgPush(&repMsg, arpHdrStore, &reply, ARP_HLEN, NULL);
    xAssert(xkr == XK_SUCCESS);
    xTrace2(arpp, 3, "replying with arp message with op %d (%s)", 
	    reply.arp_op, codenames[reply.arp_op]);
    xPush(lls, &repMsg);
    msgDestroy(&repMsg);
}


/*
 * sendRarpReply -- send a RARP reply to the sender of 'srcHdr'
 * if we know the answer
 */
static void
sendRarpReply(
    XObj	self,
    XObj	lls,
    ArpHdr	*srcHdr,
    Path	path)
{
    ArpHdr		reply;
    Msg_s		repMsg;
    IPhost		ipHost;
    xkern_return_t	xkr;
    
    if ( arpRevLookupTable( self, &ipHost, &srcHdr->arp_tha ) != 0 ) {
	/*
	 * We don't have this value in our table
	 */
	xTrace1(arpp, 3, "Don't know address of %s, can't send RARP reply",
		ethHostStr(&srcHdr->arp_tha));
	return;
    }
    reply = ((PState *)self->state)->hdr;
    reply.arp_op = ARP_RRPLY;
    reply.arp_tha = srcHdr->arp_tha;
    reply.arp_tpa = ipHost;
    if ( msgConstructContig(&repMsg, path, 0, ARP_STACK_LEN) == XK_FAILURE ) {
	xTraceP0(self, TR_SOFT_ERRORS, "couldn't allocate reply msg");
	return;
    }
    xkr = msgPush(&repMsg, arpHdrStore, &reply, ARP_HLEN, NULL);
    xAssert( xkr == XK_SUCCESS );
    xTrace1(arpp, 3, "replying with arp message with op %d", reply.arp_op);
    xPush(lls, &repMsg);
    msgDestroy(&repMsg);
}


static xkern_return_t
arpDemux(
    XObj self,
    XObj s,
    Msg msg )
{
    ArpHdr	hdr;
    PState	*ps = (PState *)self->state;
    
    xAssert(xIsSession(s));
    xAssert(xIsProtocol(self));
    
    if ( msgPop(msg, arpHdrLoad, (void *)&hdr, ARP_HLEN, NULL) == XK_FAILURE ) {
	xTraceP0(self, TR_SOFT_ERRORS, "msgPop fails");
	return XK_FAILURE;
    }
    xIfTrace(arpp, 3) arp_print("received", &hdr);
    switch(hdr.arp_op) {
      case ARP_REQ:
	if ( IP_EQUAL(ps->hdr.arp_spa, hdr.arp_tpa) ) {
	    arpSaveBinding(ps->tbl, &hdr.arp_spa, &hdr.arp_sha);    
	    sendArpReply(self, s, &hdr, msg->path);
	}
	break;
	
      case ARP_RPLY:
	arpSaveBinding(ps->tbl, &hdr.arp_spa, &hdr.arp_sha);
	break;
	
      case ARP_RREQ:
#ifndef ANSWER_RARPS 
	{
	    /* 
	     * Even if we're not configured to answer general RARP
	     * requests, we will still respond to unicast RARP requests
	     * for our own address.
	     */
	    ETHhost	rcvHost;
	    int		cl;

	    if ( ! ETH_ADS_EQUAL(hdr.arp_tha, ps->hdr.arp_sha) ) {
		break;
	    }
	    cl = xControl(s, GETRCVHOST, (char *)&rcvHost, sizeof(rcvHost));
	    if ( cl < (int)sizeof(rcvHost) ) {
		xTraceP0(self, TR_ERRORS, "GETRCVHOST fails");
		break;
	    }
	    if ( ! ETH_ADS_EQUAL(hdr.arp_tha, rcvHost) ) {
		break;
	    }
	    xTraceP0(self, TR_EVENTS, "answering unicast RARP request");
	}
#endif
	sendRarpReply(self, s, &hdr, msg->path);
	break;
	
      case ARP_RRPLY:
	arpSaveBinding(ps->tbl, &hdr.arp_spa, &hdr.arp_sha);
	arpSaveBinding(ps->tbl, &hdr.arp_tpa, &hdr.arp_tha);
	break;
	
      default:
	{/*do nothing*/}
	break;
    }
    return XK_SUCCESS;
}


static void
sendReq( ArpWait *w )
{
    PState	*ps = (PState *)w->self->state;
    Msg_s	msg;

    if ( (msgConstructContig(&msg, xMyPath(w->self), 0, ARP_STACK_LEN)
	  	== XK_FAILURE) ||
	 (msgPush(&msg, arpHdrStore, &w->reqMsg, ARP_HLEN, NULL)
	  	== XK_FAILURE) ) {
	xTraceP0(w->self, TR_SOFT_ERRORS, "couldn't allocate request msg");
	return;
    }
    xIfTrace(arpp, 3) arp_print("sending", &w->reqMsg);
    xAssert(xIsSession(w->lls));
    xPush(w->lls, &msg);
    msgDestroy(&msg);
}


/*
 * arpTimeout -- this is called both for initial requests and
 * as the timeout event
 */ 
static void
arpTimeout(Event ev, void *arg)
{
    ArpWait 		*w = arg;
    PState		*ps = (PState *)w->self->state;

    xTrace1(arpp, 3, "Arp timeout, state = %x", w);
    xAssert(w->event == ev);
    if (*w->status == ARP_RSLVD) {
	xTrace0(arpp, 5, "Request already resolved, timeout exiting");
	evDetach(ev);
	w->event = 0;
	return;
    }
    if (w->reqMsg.arp_op == ARP_REQ && w->tries++ > ARP_RTRY) {
	xTrace0(arpp, 1, "arp timeout: giving up");
	arpSaveBinding( ps->tbl, &w->reqMsg.arp_tpa, 0 );
	evDetach(ev);
	w->event = 0;
	return;
    } else if (w->reqMsg.arp_op == ARP_RREQ && w->tries++ > ARP_RRTRY) {
	xTrace0(arpp, 1, "arp timeout: giving up");
	arpSaveBinding( ps->tbl, 0, &w->reqMsg.arp_tha );
	evDetach(ev);
	w->event = 0;
	return;
    }
    xTrace0(arpp, 3, "arp timeout: trying again");
    evSchedule(ev, arpTimeout, w, ARP_TIME * 1000);
    xAssert(w->event);
    sendReq(w);
}


xkern_return_t
arpSendRequest( ArpWait *w )
{
    if ( ! (w->event = evAlloc(xMyPath(w->self))) ) {
	xTraceP0(w->self, TR_SOFT_ERRORS, "event allocation error");
	return XK_FAILURE;
    }
    evSchedule(w->event, arpTimeout, w, ARP_TIME * 1000);
    sendReq(w);
    return XK_SUCCESS;
}


static int
arpControlProtl(self, op, buf, len)
    XObj self;
    int op;
    char *buf;
    int len;
{
    PState	*ps = (PState *)self->state;
    int 	reply;
    ArpBinding	*b = (ArpBinding *)buf;
    
    xAssert(xIsProtocol(self));
    switch (op) {
	
      case RESOLVE:
	{
	    checkLen(len, sizeof(ArpBinding));
	    if ( ( netMaskNetsEqual(&b->ip, &ps->hdr.arp_spa) && 
	    	 	netMaskIsBroadcast(&b->ip) ) ||
		 IP_EQUAL(b->ip, ipLocalBcastHost) ) {

		xTrace0(arpp, 3, "returning eth broadcast address");
		b->hw = ethBcastHost;
		reply = sizeof(ArpBinding);
	    } else if ( netMaskIsMulticast(&b->ip) ) {
		xTrace2(arpp, 3, "returning eth multicast: %s -> %s",
			ipHostStr(&b->ip), ethHostStr(&b->hw));
		CONVERT_MULTI(&b->ip, &b->hw);
		reply = sizeof(ArpBinding);
	    } else if ( netMaskSubnetsEqual(&b->ip, &ps->hdr.arp_spa) ) {
		reply = (arpLookup(self, &b->ip, &b->hw) == 0) ? 
		  		sizeof(ArpBinding) : -1;
		if ( reply == -1 ) {
		    xTrace1(arpp, TR_SOFT_ERRORS,
			    "ARP lookup for host %s returns error",
			    ipHostStr(&b->ip));
		}
	    } else {
		xTrace1(arpp, TR_SOFT_ERRORS,
			"arp Resolve -- requested address %s is not local",
			ipHostStr(&b->ip));
		reply = -1;
	    }
	}
	break;
	
      case RRESOLVE:
	{
	    checkLen(len, sizeof(ArpBinding));
	    if ( ETH_ADS_EQUAL(b->hw, ethBcastHost) ) {
		b->ip = ipLocalBcastHost;
		reply = sizeof(ArpBinding);
	    } else {
		reply = (arpRevLookup(self, &b->ip, &b->hw) == 0) ? 
	      			sizeof(ArpBinding) : -1;
	    }
	}
	break;
	
      case ARP_INSTALL:
	{
	    checkLen(len, sizeof(ArpBinding));
	    arpSaveBinding(ps->tbl, &b->ip, &b->hw);
	    reply = 0;
	}
	break;
	
      case ARP_GETMYBINDING:
	{
	    checkLen(len, sizeof(ArpBinding));
	    b->ip = ps->hdr.arp_spa;
	    b->hw = ps->hdr.arp_sha;
	    reply = sizeof(ArpBinding);
	}
	break;
	
      case ARP_FOR_EACH:
	{
	    checkLen(len, sizeof(ArpForEach));
	    arpForEach(ps->tbl, (ArpForEach *)buf);
	    reply = 0;
	}
	break;

      default:
	reply = -1;
    }
    xTrace2(arpp, 3, "Arp control %s returns %d", 
	    op == (int)RESOLVE ? "resolve":
	    op == (int)ARP_GETMYBINDING ? "getmybinding" :
	    op == (int)RRESOLVE ? "rresolve" :
	    op == (int)ADDMULTI ? "addmulti" :
	    op == (int)DELMULTI ? "delmulti" :
	    "UNKNOWN", reply);
    return(reply);
}



static void
arpHdrStore(hdr, netHdr, len, arg)
    void *hdr;
    char *netHdr;
    long int len;
    void *arg;
{
    /*
     * Need a temporary header structure to avoid alignment problems
     */
    ArpHdr tmpHdr;
    
    xTrace0(arpp, 5, "Entering arpHdrStore");
    xAssert( len == sizeof(ArpHdr) );
    bcopy( hdr, (char *)&tmpHdr, sizeof(ArpHdr) );
    tmpHdr.arp_hrd = htons(tmpHdr.arp_hrd);
    tmpHdr.arp_prot = htons(tmpHdr.arp_prot);
    tmpHdr.arp_op = htons(tmpHdr.arp_op);
    bcopy( (char *)&tmpHdr, netHdr, sizeof(ArpHdr) );
    xTrace0(arpp, 7, "leaving arpHdrStore");
}


static long
arpHdrLoad(hdr, netHdr, len, arg)
    void *hdr;
    char *netHdr;
    long int len;
    void *arg;
{
    xAssert( len == sizeof(ArpHdr) );

    xTrace0(arpp, 5, "Entering arpHdrLoad");
    bcopy( netHdr, hdr, sizeof(ArpHdr) );
    ((ArpHdr *)hdr)->arp_hrd = ntohs(((ArpHdr *)hdr)->arp_hrd);
    ((ArpHdr *)hdr)->arp_prot = ntohs(((ArpHdr *)hdr)->arp_prot);
    ((ArpHdr *)hdr)->arp_op = ntohs(((ArpHdr *)hdr)->arp_op);
    xTrace0(arpp, 7, "leaving arpHdrLoad");
    return len;
}


static void
newWait(w, self)
    ArpWait 	*w;
    XObj	self;
{
    bzero((char *)w, sizeof(ArpWait));
    semInit(&w->s, 0);
    w->self = self;
}


void
newArpWait(w, self, h, status)
    ArpWait 	*w;
    XObj	self;
    IPhost 	*h;
    ArpStatus 	*status;
{
    PState	*ps = (PState *)self->state;

    newWait(w, self);
    w->reqMsg = ps->hdr;
    w->reqMsg.arp_tpa = *h;
    w->reqMsg.arp_op = ARP_REQ;
    w->status = status;
    xDuplicate(ps->arpSessn);
    w->lls = ps->arpSessn;
}


xkern_return_t
newRarpWait(w, self, h, status)
    ArpWait 	*w;
    XObj	self;
    ETHhost 	*h;
    ArpStatus 	*status;
{
    PState	*ps = (PState *)self->state;
    Part_s	part;

    newWait(w, self);
    w->reqMsg = ps->hdr;
    w->reqMsg.arp_tha = *h;
    w->reqMsg.arp_op = ARP_RREQ;
    w->status = status;
    partInit(&part, 1);
    /* 
     * Open the lower session for sending this request.  If it's for
     * our own address, we'll broadcast.  Otherwise, we shall only send
     * to the remote host itself.
     */
    if ( ETH_ADS_EQUAL(*h, ps->hdr.arp_sha) ) {
	partPush(part, &ethBcastHost, sizeof(ethBcastHost));
    } else {
	partPush(part, h, sizeof(*h));
    }
    w->lls = xOpen(self, ps->rarp, xGetDown(self, 0), &part, xMyPath(self));
    if ( w->lls == ERR_XOBJ ) {
	xTraceP0(self, TR_SOFT_ERRORS, "RARP open failure");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


