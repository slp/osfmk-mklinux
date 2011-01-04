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
 * bidctl_state.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * bidctl_state.c,v
 * Revision 1.20.1.3.1.2  1994/09/07  04:18:48  menze
 * OSF modifications
 *
 * Revision 1.20.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.20.1.2  1994/07/22  20:08:04  menze
 * Uses Path-based message interface and UPI
 *
 */

/* 
 * Implementation of the transitions between states of the BidctlState
 * FSM and their associated actions.  Includes routines for constructing
 * and sending bidctl control messages to peers and informing other
 * protocols of reboots.
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/bidctl/bidctl_i.h>
#include <xkern/include/netmask.h>	/* for IP_LOCAL_BCAST */

static int	informOfNewId( void *, void *, void * );
static void	randomDelay( int );

#if XK_DEBUG
int	tracebidctlp;
#endif /* XK_DEBUG */


char *
bidctlFsmStr( s )
    FsmState	s;
{
    switch ( s ) {
      case BIDCTL_NORMAL_S: 	return "NORMAL";
      case BIDCTL_QUERY_S:	return "QUERY";
      case BIDCTL_RESET_S:	return "RESET";
      case BIDCTL_OPEN_S:	return "OPEN";
      case BIDCTL_INIT_S:	return "INIT";
      default: 			return "UNKNOWN";
    }
}
  

void
bidctlOutput( bs, sendQuery, sendReply, inHdr )
    BidctlState	*bs;
    BidctlQuery	sendQuery;
    int		sendReply;
    BidctlHdr	*inHdr;
{
    PState		*ps = (PState *)bs->myProtl->state;
    BidctlHdr		h;
    Msg_s		m;
    xkern_return_t	xkr;

    xTrace0(bidctlp, TR_EVENTS, "Bidctl output");
    h.flags = 0;
    h.srcBid = ps->myBid;
    h.dstBid = bs->peerBid;
    if ( sendQuery ) {
	if ( sendQuery == BIDCTL_NEW_QUERY ) {
	    bs->reqTag = bidctlReqTag();
	} else {
	    xAssert(sendQuery == BIDCTL_REPEAT_QUERY);
	}
	h.reqTag = bs->reqTag;
	h.flags |= BIDCTL_QUERY_F;
    } else {
	h.reqTag = 0;
    }
    if ( sendReply ) {
	xAssert(inHdr);
	h.rplTag = inHdr->reqTag;
	h.flags |= BIDCTL_RESPONSE_F;
    } else {
	h.rplTag = 0;
    }
    xIfTrace(bidctlp, TR_DETAILED) {
	bidctlHdrDump(&h, "OUTGOING", &bs->peerHost);
    }
    if ( msgConstructContig(&m, xMyPath(bs->myProtl), 0, BIDCTL_STACK_LEN)
		== XK_FAILURE ) {
	xTraceP0(bs->myProtl, TR_SOFT_ERRORS, "Couldn't allocate output message");
	return;
    }
    xkr = msgPush(&m, bidctlHdrStore, &h, sizeof(h), 0);
    xAssert( xkr == XK_SUCCESS );
    xPush(bs->lls, &m);
    msgDestroy(&m);
}


/* 
 * bidctlBootBroadcast
 */
void
bidctlBcastBoot( self )
    XObj	self;
{
    PState	*ps = (PState *)self->state;
    Msg_s	m;
    BidctlHdr	h;
    Part_s	p;
    XObj	lls;
    IPhost	ipBcast;
    xkern_return_t	xkr;

    xTrace0(bidctlp, TR_MAJOR_EVENTS, "Bidctl broadcasting reboot");
    ipBcast = IP_LOCAL_BCAST;
    partInit(&p, 1);
    partPush(p, &ipBcast, sizeof(IPhost));
    lls = xOpen(self, self, xGetDown(self, 0), &p, xMyPath(self));
    if ( lls == ERR_XOBJ ) {
	xError("Bidctl couldn't open broadcast session");
	return;
    }
    h.flags = BIDCTL_BCAST_F;
    h.srcBid = ps->myBid;
    h.dstBid = h.reqTag = h.rplTag = 0;
    xIfTrace(bidctlp, TR_DETAILED) {
	bidctlHdrDump(&h, "OUTGOING", &ipBcast);
    }
    if ( msgConstructContig(&m, xMyPath(self), 0, BIDCTL_STACK_LEN)
		== XK_FAILURE ) {
	xTraceP0(self, TR_SOFT_ERRORS, "couldn't alloc broadcast message");
	return;
    }
    xkr = msgPush(&m, bidctlHdrStore, &h, sizeof(BidctlHdr), 0);
    xAssert( xkr == XK_SUCCESS );
    xPush(lls, &m);
    msgDestroy(&m);
    xClose(lls);
}


static int
informOfNewId( key, val, arg )
    void	*key, *val, *arg;
{
    BidctlState		*bs = (BidctlState *)arg;
    XObj		hlp = *(XObj *)key;
    BidctlBootMsg	rbMsg;
    int			op;

    rbMsg.h = bs->peerHost;
    rbMsg.id = bs->peerBid;
    switch ( bs->fsmState ) {

      case BIDCTL_RESET_S:
	op = BIDCTL_PEER_REBOOTED;
	break;

      case BIDCTL_OPENDONE_S:
	op = BIDCTL_FIRST_CONTACT;
	break;
	
      default:
	xError("bidctl inform of reboot -- bizarre state!!");
	return 0;
    }
    xControl(hlp, op, (char *)&rbMsg, sizeof(rbMsg));
    return MFE_CONTINUE;
}



void
bidctlStartQuery( bs, sendInitialQuery )
    BidctlState	*bs;
    int		sendInitialQuery;
{
    xAssert( bs->fsmState == BIDCTL_NORMAL_S );
    bs->retries = 0;
    bs->fsmState = BIDCTL_QUERY_S;
    if ( sendInitialQuery ) {
	bidctlOutput(bs, BIDCTL_NEW_QUERY, 0, 0);
    }
    evSchedule(bs->ev, bidctlTimeout, bs, BIDCTL_QUERY_TIMEOUT);
}


static void
randomDelay( int d )
{
    XTime	t;
    
    xGetTime(&t);
    d = (d * (t.usec % 100 + 1)) / 100;
    xTrace1(bidctlp, TR_DETAILED, "bidctl delaying %d msec", d);
    Delay(d);
    xTrace0(bidctlp, TR_DETAILED, "bidctl delayed thread wakes");
}

/* 
 * hdr may be null, indicating a request for the state.  This may
 * cause transitions out of initialization states.
 */
void
bidctlTransition( bs, hdr )
    BidctlState	*bs;
    BidctlHdr	*hdr;
{
    int		sendMsg = 0, sendReply = 0;
    BidctlQuery	sendQuery = BIDCTL_NO_QUERY;

    xIfTrace(bidctlp, TR_MORE_EVENTS) {
	bidctlDispState(bs);
    }
    if ( hdr && (hdr->flags & BIDCTL_BCAST_F) ) {
	/* 
	 * Delay in order to avoid overwhelming the newly rebooted
	 * machine 
	 */
	randomDelay(BIDCTL_BCAST_QUERY_DELAY);
	if ( hdr->srcBid == bs->peerBid ) {
	    /* 

	     */
	    xTrace0(bidctlp, TR_DETAILED,
		    "peer bcast bootid already confirmed");
	    return;
	}
    }
    if ( hdr == 0 && bs->fsmState != BIDCTL_INIT_S ) {
	return;
    }
    switch ( bs->fsmState ) {

      case BIDCTL_INIT_S:
	sendQuery = BIDCTL_NEW_QUERY;
	bs->fsmState = BIDCTL_OPEN_S;
	break;

      case BIDCTL_NORMAL_S:
	{
	    xAssert(hdr);
	    if ( hdr->dstBid != ((PState *)bs->myProtl->state)->myBid ) {
		if ( ! (hdr->flags & BIDCTL_QUERY_F) ) {
		    sendMsg = 1;
		}
	    }
	    if ( hdr->srcBid != bs->peerBid ) {
		bidctlStartQuery(bs, 0);
		sendQuery = BIDCTL_NEW_QUERY;
	    }
	    break;
	}

      case BIDCTL_OPEN_S:
	xAssert(hdr);
	if ( hdr->flags & BIDCTL_BCAST_F ) {
	    sendQuery = BIDCTL_REPEAT_QUERY;
	    /* 
	     * Maybe we should restart the timeout event?
	     */
	    break;
	}
	if ( ! (hdr->flags & BIDCTL_RESPONSE_F) ) {
	    xTrace0(bidctlp, TR_MORE_EVENTS, "ignoring non-RESPONSE");
	    break;
	}
	if ( hdr->rplTag != bs->reqTag ) {
	    xTrace0(bidctlp, TR_MORE_EVENTS, "dropping RESPONSE Bid mismatch");
	    break;
	}
	/* 
	 * This is an answer to our request during open.
	 */
	xTrace0(bidctlp, TR_MORE_EVENTS, "response to request received");
	bs->peerBid = hdr->srcBid;
	bs->fsmState = BIDCTL_OPENDONE_S;
	evCancel(bs->ev);
	bidctlReleaseWaiters(bs);
	mapForEach(bs->hlpMap, informOfNewId, bs);
	bs->fsmState = BIDCTL_NORMAL_S;
	
	break;
	    
      case BIDCTL_QUERY_S:
	xAssert(hdr);
	if ( hdr->flags & BIDCTL_BCAST_F ) {
	    xTrace0(bidctlp, TR_MORE_EVENTS, "BCAST -- sending new query");
	    sendQuery = BIDCTL_NEW_QUERY;
	    bs->retries = 0;
	} else if ( hdr->flags & BIDCTL_RESPONSE_F ) {
	    if ( hdr->rplTag != bs->reqTag ) {
		xTrace0(bidctlp, TR_MORE_EVENTS,
			"dropping RESPONSE Bid mismatch");
		break;
	    }
	    evCancel(bs->ev);
	    if ( hdr->srcBid == bs->peerBid ) {
		xTrace0(bidctlp, TR_MORE_EVENTS,
			"Response matches old Bid, returning to NORMAL state");
	    } else {
		xTrace0(bidctlp, TR_MORE_EVENTS,
			"RESPONSE is new Bid, going to RESET state");
		bs->peerBid = hdr->srcBid;
		bs->fsmState = BIDCTL_RESET_S;
		xTrace1(bidctlp, TR_MAJOR_EVENTS, "Host %s rebooted",
			ipHostStr(&bs->peerHost));
		mapForEach(bs->hlpMap, informOfNewId, bs);
	    }
	    bs->fsmState = BIDCTL_NORMAL_S;
	    bidctlReleaseWaiters(bs);
	} else {
	    xTrace0(bidctlp, TR_MORE_EVENTS, "dropping non-response");
	}
	break;
	    
      case BIDCTL_OPENDONE_S:
      case BIDCTL_RESET_S:
	xTrace0(bidctlp, TR_MORE_EVENTS, "dropping message");
	break;
    }
    if ( hdr ) {
	sendReply = hdr->flags & BIDCTL_QUERY_F;
    }
    if ( sendMsg || sendQuery || sendReply ) {
	if ( ! sendQuery && (bs->fsmState == BIDCTL_QUERY_S ||
			     bs->fsmState == BIDCTL_OPEN_S) ) {
	    /* 
	     * Since we're sending something anyway, we'll just let
	     * a repeat of the query tag-along instead of waiting for
	     * the timeout.
	     */
	    sendQuery = BIDCTL_REPEAT_QUERY;
	}
	bidctlOutput(bs, sendQuery, sendReply, hdr);
    }
    xIfTrace(bidctlp, TR_MORE_EVENTS) {
	bidctlDispState(bs);
    }
}


