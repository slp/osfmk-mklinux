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
 * ip_input.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * ip_input.c,v
 * Revision 1.11.2.1  1994/07/22  19:52:25  menze
 * Uses Path-based message interface and UPI
 * bzero multicomponent keys before use
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/ip/ip_i.h>
#include <xkern/protocols/ip/route.h>

static int		onLocalNet( XObj, IPhost * );
static int		validateOpenEnable( XObj );

/*
 * ipDemux
 */
xkern_return_t
ipDemux(self, transport_s, dg)
    XObj self;
    XObj transport_s;
    Msg dg;
{
    IPheader	hdr;
    XObj        s;
    ActiveId	actKey;
    PState	*pstate = (PState *) self->state;
    int		dataLen;
    char	options[40];
    
    xTrace1(ipp, TR_EVENTS,
	    "IP demux called with datagram of len %d", msgLen(dg));
    if ( ipGetHdr(dg, &hdr, options) ) {
	xTrace0(ipp, TR_SOFT_ERRORS,
		"IP demux : getHdr problems, dropping message\n"); 
	return XK_SUCCESS;
    }
    xTrace3(ipp, TR_MORE_EVENTS,
	    "ipdemux: seq=%d,frag=%d, len=%d", hdr.ident, hdr.frag,
	    msgLen(dg));
    if (GET_HLEN(&hdr) > 5) {
	xTrace0(ipp, TR_SOFT_ERRORS,
		"IP demux: I don't understand options!  Dropping msg");
	return XK_SUCCESS;
    }
    dataLen = hdr.dlen - IPHLEN;
    if ( dataLen < msgLen(dg) ) {
	xTrace1(ipp, TR_MORE_EVENTS,
		"IP demux : truncating right at byte %d", dataLen);
	msgTruncate(dg, dataLen);
    }
    bzero((char *)&actKey, sizeof(ActiveId));
    actKey.protNum = hdr.prot;
    actKey.remote = hdr.source;
    actKey.local = hdr.dest;
    if ( mapResolve(pstate->activeMap, &actKey, &s) == XK_FAILURE ) {
	FwdId	fwdKey;
	IPhost	mask;

	netMaskFind(&mask, &hdr.dest);
	IP_AND(fwdKey, mask, hdr.dest);
	if ( mapResolve(pstate->fwdMap, &fwdKey, &s) == XK_FAILURE ) {
	    xTrace0(ipp, TR_EVENTS, "no active session found");
	    s = ipCreatePassiveSessn(self, transport_s, &actKey, &fwdKey,
				     msgGetPath(dg));
	    if ( s == ERR_XOBJ ) {
		xTrace0(ipp, TR_EVENTS, "...dropping the message");
		return XK_SUCCESS;
	    }
	}
    }
    return xPop(s, transport_s, dg, &hdr);
}


xkern_return_t
ipForwardPop( s, lls, msg, inHdr )
    XObj	s, lls;
    Msg 	msg;
    void	*inHdr;
{
    IPheader		*h = (IPheader *)inHdr;
    xmsg_handle_t	res;

    xTrace0(ipp, TR_EVENTS, "ip forward pop");
    xAssert(h);
    if ( --h->time == 0 ) {
	xTrace0(ipp, TR_EVENTS, "ttl == 0 -- dropping");
	return XK_SUCCESS;
    }
    /* 
     * We need to go through ipSend because the MTU on the outgoing
     * interface might be less than the packet size (and need
     * fragmentation.) 
     */
    res = ipSend(s, xGetDown(s, 0), msg, h);
    return ( res == XMSG_ERR_HANDLE ) ? XK_FAILURE : XK_SUCCESS;
}


static int
onLocalNet( llo, h )
    XObj	llo;
    IPhost	*h;
{
    int	res;

    res = xControl(llo, VNET_HOSTONLOCALNET, (char *)h, sizeof(IPhost));
    if ( res < 0 ) {
	xTrace0(ipp, TR_ERRORS, "ipFwdBcst couldn't do HOSTONLOCALNET on llo");
	return 0;
    }
    return res > 0;
}


/* 
 * Used for ipFwdBcast sessions, sessions which receive network broadcasts
 * in a subnet  environment.  Depending on the incoming interface, the
 * message may  need to be forwarded on other interfaces and locally
 * accepted, or it may be dropped.  See RFC 922.
 *
 * This is not very efficient and is further complicated by the hiding
 * of interfaces in VNET.
 */
xkern_return_t
ipFwdBcastPop( s, llsIn, msg, inHdr )
    XObj	s, llsIn;
    Msg 	msg;
    void	*inHdr;
{
    IPheader		*h = (IPheader *)inHdr;
    xmsg_handle_t	res;
    route		rt;
    Msg_s		msgCopy;
    void		*ifcId;
    XObj		lls;
    xkern_return_t	xkr;
    PState		*ps = (PState *)xMyProtl(s)->state;

    xTrace0(ipp, TR_EVENTS, "ip forward bcast pop");
    xAssert(h);
    if ( --h->time == 0 ) {
	xTrace0(ipp, TR_EVENTS, "ttl == 0 -- dropping");
	return XK_SUCCESS;
    }
    /* 
     * Did this packet come in on the interface we would use to reach
     * the source?  If not, drop the message
     */
    if ( ! onLocalNet(llsIn, &h->source) ) {
	if ( onLocalNet(xGetDown(xMyProtl(s), 0), &h->source ) ) {
	    xTrace0(ipp, TR_EVENTS,
	          "ipFwdBcast gets packet not on direct connection, dropping");
	    return XK_SUCCESS;
	}
	xkr = rt_get( &ps->rtTbl, &h->source, &rt );
	if ( xkr == XK_FAILURE ) {
	    xTrace0(ipp, TR_SOFT_ERRORS, "ipFwdBcast ... no gateway");
	    return XK_SUCCESS;
	}
	if ( ! onLocalNet(llsIn, &rt.gw) ) {
	    xTrace0(ipp, TR_EVENTS,
		    "ipFwdBcast receives packet from dup gw, dropping");
	    return XK_SUCCESS;
	}
    }
    /* 
     * If this is a session used for local delivery (i.e., if the
     * down[0] is valid), send up for local processing (reassemble first)
     */
    if ( xIsSession(xGetDown(s, 0)) ) {
	msgConstructCopy(&msgCopy, msg);
	ipStdPop(s, llsIn, &msgCopy, h);
	msgDestroy(&msgCopy);
    }
    /* 
     * Send this message back out on all appropriate interfaces except
     * the one on which it was received.
     */
    lls = xGetDown(s, 1);
    xAssert(xIsSession(lls));
    xControl(llsIn, VNET_GETINTERFACEID, (char *)&ifcId, sizeof(void *));
    if ( xControl(lls, VNET_DISABLEINTERFACE, (char *)&ifcId, sizeof(void *))
		< 0 ) {
	xTrace0(ipp, TR_ERRORS,
		"ipFwdBcastPop could not disable lls interface");
	return XK_SUCCESS;
    }
    res = ipSend(s, lls, msg, h);
    xControl(lls, VNET_ENABLEINTERFACE, (char *)&ifcId, sizeof(void *));
    return ( res == XMSG_ERR_HANDLE ) ? XK_FAILURE : XK_SUCCESS;
}


/* 
 * validateOpenEnable -- Checks to see if there is still an openEnable for
 * the session and, if so, calls openDone.
 * This is called right before a message is sent up through
 * a session with no external references.  This has to be done
 * because IP sessions
 * can survive beyond removal of all external references. 
 *
 * Returns 1 if an openenable exists, 0 if it doesn't.
 */
static int
validateOpenEnable( s )
    XObj	s;
{
    SState	*ss = (SState *)s->state;
    Enable	*e;

    e = ipFindEnable(xMyProtl(s), ss->hdr.prot, &ss->hdr.source);
    if ( e == ERR_ENABLE ) {
	xTrace1(ipp, TR_MAJOR_EVENTS, "ipValidateOE -- no OE for hlp %d!",
		ss->hdr.prot);
	return 0;
    }
    xOpenDone(e->hlpRcv, s, xMyProtl(s), xMyPath(s));
    return 1;
}


xkern_return_t
ipMsgComplete(s, lls, dg, inHdr)
    XObj	s, lls;
    Msg 	dg;
    void	*inHdr;
{
    IPheader *h = (IPheader *)inHdr;
    IPpseudoHdr ph;
    
    if ( s->rcnt == 1 && ! validateOpenEnable(s) ) {
	return XK_SUCCESS;
    }
    xAssert(h);
    ph.src = h->source;
    ph.dst = h->dest;
    ph.zero = 0;
    ph.prot = h->prot;
    ph.len = htons( msgLen(dg) );
    msgSetAttr(dg, 0, (void *)&ph, sizeof(IPpseudoHdr));
    xTrace1(ipp, TR_EVENTS, "IP pop, length = %d", msgLen(dg));
    xAssert(xIsSession(s));
    return xDemux(s, dg);
}



xkern_return_t
ipStdPop( s, lls, dg, hdr )
    XObj	s, lls;
    Msg		dg;
    void	*hdr;
{
    s->idle = FALSE;
    if (COMPLETEPACKET(*(IPheader *)hdr)) {
	return ipMsgComplete(s, lls, dg, hdr);
    } else {
	return ipReassemble(s, lls, dg, hdr);
    }
}


