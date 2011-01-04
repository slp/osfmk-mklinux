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
 * chan_client.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * chan_client.c,v
 * Revision 1.49.1.5.1.2  1994/09/07  04:18:44  menze
 * OSF modifications
 *
 * Revision 1.49.1.5  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.49.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.49.1.3  1994/07/22  20:08:09  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.49.1.2  1994/06/30  01:35:38  menze
 * Control messages are built using the allocator of the request
 * message, not the allocator attached to the session.
 *
 * Revision 1.49.1.1  1994/04/14  21:36:51  menze
 * Uses allocator-based message interface
 *
 * Revision 1.49  1993/12/13  22:39:36  menze
 * Modifications from UMass:
 *
 *   [ 93/09/21          nahum ]
 *   Got rid of casts in mapUnbind, mapResolve calls
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/chan/chan_internal.h>
#include <xkern/include/prot/chan.h>

static void		abortCall( XObj );
static xkern_return_t	chanClientClose( XObj );
static int		chanClientControl( XObj, int, char *, int );
static ChanOpenFunc	chanClientOpen;
static XObj		getAnyIdleSessn( XObj, XObj, XObj, ActiveID *, Path );
static int		returnFirstElem( void *, void *, void * );
static int		zombifyClient( void *, void * );

/* 
 * chanOpen
 */
XObj 
chanOpen( self, hlpRcv, hlpType, p, path )
    XObj self, hlpRcv, hlpType;
    Part p;
    Path	path;
{
    XObj   	s, lls;
    ActiveID 	key;
    IPhost	peer;
    Map		chanMap;
    Channel	*chanPtr;
    
    xTrace0(chanp, TR_MAJOR_EVENTS,
	    "CHAN open ..............................");
    if ( (key.prot_id = chanGetProtNum(self, hlpType)) == -1 ) {
	return ERR_XOBJ;
    }
    /*
     * Open lower session
     */
    lls = xOpen(self, self, xGetDown(self, 0), p, path);
    if ( lls == ERR_XOBJ) {
	xTrace0(chanp, TR_SOFT_ERRORS,
		"chan_open: can't open lower session");
	return ERR_XOBJ;
    }
    key.lls = lls;
    /* 
     * See if there is an idle channel for this host
     */
    s = getAnyIdleSessn(self, hlpRcv, hlpType, &key, path);
    if ( s != ERR_XOBJ ) {
	xTrace1(chanp, TR_MAJOR_EVENTS, "chanOpen returning idle session %x",
		s);
	xClose(lls);
	return s;
    }
    if ( xControl(lls, GETPEERHOST, (char *)&peer, sizeof(IPhost))
				< (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS,
		"chan_open: can't do GETPEERHOST on lower session");
	xClose(lls);
	return ERR_XOBJ;
    }
    if ( (chanMap = chanGetChanMap(self, &peer, path)) == 0 ) {
	xClose(lls);
	return ERR_XOBJ;
    }
    /* 
     * Get a new channel number
     */
    if ( mapResolve(chanMap, &key.prot_id, &chanPtr) == XK_FAILURE ) {

	xTrace2(chanp, TR_MAJOR_EVENTS, "first channel to peer %s, hlp %d",
		ipHostStr(&peer), key.prot_id);
	if ( ! (chanPtr = pathAlloc(path, sizeof(Channel))) ) {
	    xClose(lls);
	    return ERR_XOBJ;
	}
	*chanPtr = FIRST_CHAN;
	if ( mapBind(chanMap, &key.prot_id, chanPtr, 0) != XK_SUCCESS ) {
	    pathFree(chanPtr);
	    xClose(lls);
	    return ERR_XOBJ;
	}
    }
    key.chan = (*chanPtr)++;
    xIfTrace(chanp, TR_MAJOR_EVENTS) chanDispKey(&key);
    s = chanClientOpen(self, hlpRcv, hlpType, &key, START_SEQ, TRUE, path);
    if ( s == ERR_XOBJ && *chanPtr == key.chan + 1 ) {
	(*chanPtr)--;
    }
    xClose(lls);
    return s;
}


/* 
 * Creates a new client session based on the given key and sequence number
 */
static XObj
chanClientOpen( self, hlpRcv, hlpType, key, seq, newChan, path )
    XObj	self, hlpRcv, hlpType;
    ActiveID	*key;
    int		seq;
    boolean_t	newChan;
    Path	path;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    XObj	s;

    s = chanCreateSessn(self, hlpRcv, hlpType, key, &ps->cliInfo,
			newChan, path);
    if ( s != ERR_XOBJ ) {
	CHAN_STATE	*ss = (CHAN_STATE *)s->state;
	ss->fsmState = CLNT_FREE;
	ss->hdr.flags = FROM_CLIENT | USER_MSG;
	ss->hdr.seq = seq;
	ss->waitParam = CLIENT_WAIT_DEFAULT;
	ss->maxWait = CLIENT_MAX_WAIT_DEFAULT;
    }
    return s;
}


/* 
 * chanCall
 */
xkern_return_t
chanCall(
    XObj self,
    Msg msg,
    Msg rmsg )
{
    CHAN_STATE   	*state;
    CHAN_HDR 	*hdr;
    XObj		lls;
    xkern_return_t	xkr;
    Msg_s               lmess;
    
    xTrace0(chanp, TR_EVENTS, "CHAN call ..............................");
    
    state = (CHAN_STATE *)self->state;
    xTrace1(chanp, TR_MORE_EVENTS, "chan_call: state = %x", state);
    xTrace1(chanp, TR_MORE_EVENTS,
	    "chan_call: outgoing length (no chan hdr): %d", msgLen(msg));
    
    if ((state->fsmState != CLNT_FREE)) {
	xTrace0(chanp, TR_SOFT_ERRORS, "chan_call: incorrect initial state");
	return XK_FAILURE;
    }
    
    /*
     * Fill in header
     */
    state->hdr.seq++;
    hdr 		= &state->hdr;
    hdr->len 	= msgLen(msg);
    
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(hdr);
    } 
    bzero((char *)&state->info, sizeof(state->info));
    msgConstructCopy(&lmess, msg);
    xkr = msgPush(&lmess, chanHdrStore, hdr, CHANHLEN, NULL);
    if ( xkr == XK_FAILURE ) {
	/* 
	 * We'll assume things will get better and timeout.
	 */
	state->fsmState = CLNT_CALLING;
	xTraceP0(self, TR_SOFT_ERRORS, "msgPush failed");
	msgDestroy(&lmess);
    } else {
    
	lls = xGetDown(self, 0);
	xAssert(xIsSession(lls));
	xTrace1(chanp, TR_MORE_EVENTS, "chan_call: s->push  = %x", lls->push); 
	xTrace1(chanp, TR_MORE_EVENTS, "chan_call: packet len %d", msgLen(&lmess));
	xTrace1(chanp, TR_MORE_EVENTS, "chan_call: length field: %d", hdr->len);
	xTrace2(chanp, TR_MORE_EVENTS, "chan_call: lls %x, packet = %x",
		lls, &lmess); 
	xAssert(rmsg || state->isAsync);
	state->answer = rmsg;
	/*
	 * Send message
	 */
	msgSetAttr(&lmess, 0, (void *)&state->info, sizeof(state->info));
	xkr = (xPush(lls, &lmess) < 0) ? XK_FAILURE : XK_SUCCESS;
	msgDestroy(&lmess);
	if ( xkr == XK_FAILURE ) {
	    int	maxLen;

	    xTrace0(chanp, TR_SOFT_ERRORS, "chan_call: can't send message");
	    /* 
	     * If the request message is just too big, we lose.
	     * Otherwise, assume it's a transitory problem and try
	     * again on the next timeout.
	     */
	    if ( xControl(lls, GETMAXPACKET, (char *)&maxLen, sizeof(int))
			< 0 ) {
		xTraceP0(self, TR_ERRORS, "can't get maxpacket");
		return XK_FAILURE;
	    }
	    if ( maxLen < hdr->len ) {
		xTraceP2(lls, TR_SOFT_ERRORS, "reqLen (%d) > maxLen (%d)",
			 hdr->len, maxLen);
		return XK_FAILURE;
	    }
	}
	state->fsmState = CLNT_WAIT;
    }
    state->wait	= CHAN_CLNT_DELAY(hdr->len, state->waitParam);
    xTrace1(chanp, TR_DETAILED, "chan_call: client_wait = %d", state->wait);

    if ( xkr != XK_SUCCESS || !state->info.reliable ) { 
	/*
	 * Save data (without header) for retransmit
	 */
	msg_flush(state->saved_msg);
	msgConstructCopy(&state->saved_msg.m, msg);
	msg_valid(state->saved_msg);

	xAssert(state->event->state == E_INIT);
        xAssert(state->event->flags.cancelled != 1);
	evSchedule(state->event, chanTimeout, self, state->wait);
    }
    if ( ! state->isAsync ) {
	/*
	 * Wait for reply
	 */
	semWait(&state->reply_sem);
	xTrace1(chanp, TR_MORE_EVENTS,
		"chanCall released from block, returning %d", state->replyValue);
	return state->replyValue;
    }
    return XK_SUCCESS;
}


xmsg_handle_t
chanPush(
    XObj	s,
    Msg		msg)
{
    return chanCall(s, msg, 0) == XK_SUCCESS ?
      XMSG_NULL_HANDLE : XMSG_ERR_HANDLE;
}

/*
 * chanClientPop
 */
static xkern_return_t
chanClientPop(
    XObj self,
    XObj lls,
    Msg msg,
    void *inHdr )
{
    CHAN_STATE 	*state;
    CHAN_HDR	*hdr = (CHAN_HDR *)inHdr;
    u_int 	seq;
    SEQ_STAT 	status;
    
    xTrace0(chanp, TR_EVENTS, "CHAN Client Pop");
    xAssert(hdr);
    state = (CHAN_STATE *)self->state;
    seq = hdr->seq;
    status = chanCheckSeq(state->hdr.seq, seq);
    xTrace4(chanp, TR_MORE_EVENTS,
	    "state: %s  cur_seq = %d, hdr->seq = %d  status: %s",
	    chanStateStr(state->fsmState), state->hdr.seq, hdr->seq,
	    chanStatusStr(status));
    if ( chanCheckMsgLen(hdr->len, msg) ) {
	return XK_SUCCESS;
    }
    xAssert(hdr->chan == state->hdr.chan);
    switch(state->fsmState) {
	
      case CLNT_WAIT:
	if (status != current) {
	    xTrace0(chanp, TR_SOFT_ERRORS,
		    "chan_pop: CLNT_WAIT: non-current sequence number");
	    break;
	}
	if ( hdr->flags & USER_MSG ) {
	    xTrace0(chanp, TR_EVENTS, "chan_pop: CLNT_WAIT: Received reply"); 
	    chanFreeResources(self);
	    /*
	     * Return results
	     */
	    state->fsmState = CLNT_FREE;
	    if ( ! state->isAsync ) {
		state->replyValue = XK_SUCCESS;
		msgAssign(state->answer, msg);
		semSignal(&state->reply_sem);
		xTrace0(chanp, TR_MORE_EVENTS, "chan_pop: CLNT_WAIT: returns OK");
	    }
	    if ( hdr->seq == state->hdr.seq ) {
		if ( hdr->flags & ACK_REQUESTED ) {
		    xTraceP0(self, TR_MORE_EVENTS, "client sending expl ACK");
		    hdr->seq++;
		    chanReply(lls, hdr, FROM_CLIENT, msgGetPath(msg));
		}
		msg_flush(state->saved_msg);
	    }
	    if ( state->isAsync ) {
		xDemux(self, msg);
		/*
		 * For the recursion to work, make sure to return immediately!
		 */
		return XK_SUCCESS;
	    }
	} else {
	    evCancel(state->event);
	    if ( hdr->flags & NEGATIVE_ACK ) {
		chanResend(self, FROM_CLIENT, 1);
		evSchedule(state->event, chanTimeout, self, state->wait);
	    } else {
		/* 
		 * Msg is an ACK of the current request
		 */
		if ( hdr->flags & ACK_REQUESTED ) {
		    /* 
		     * We missed the reply
		     */
		    chanReply(lls, hdr, FROM_CLIENT | NEGATIVE_ACK,
			      msgGetPath(&state->saved_msg.m));
		} else {
		    xTrace0(chanp, TR_MORE_EVENTS,
			    "chan_pop: CLNT_WAIT: received SVC_EXPLICIT_ACK");
		}
	    }
	}
	break;
	
      case CLNT_FREE:
	chanClientIdleRespond(hdr, xGetDown(self, 0), state->hdr.seq, msgGetPath(msg));
	break;
	
     default:
	xTrace1(chanp, TR_ERRORS, "chan_pop: invalid state %d",
		state->fsmState);
	break;
    } 
    xTrace2(chanp, TR_MORE_EVENTS, "end chanClientPop:  state: %s curSeq = %d",
	    chanStateStr(state->fsmState), state->hdr.seq);
    return XK_SUCCESS;
}


/* 
 * A message came in for a dormant client channel.  'seq' is the current
 * sequence number for this channel.  The message header may be modified.
 */
void
chanClientIdleRespond( hdr, lls, seq, path )
    CHAN_HDR	*hdr;
    XObj	lls;
    u_int	seq;
    Path	path;
{
    xAssert( xIsXObj(lls) );
    xAssert( ! (hdr->flags & FROM_CLIENT) );
    if ( hdr->seq < seq ) {
	xTrace0(chanp, TR_EVENTS, "idle client channel receives old seq num");
	return;
    }
    if ( hdr->seq > seq ) {
	xTrace0(chanp, TR_ERRORS,
		"chan idle client receives new sequence number!");
	return;
    }
    if ( hdr->flags & ACK_REQUESTED ) {
	xTrace0(chanp, TR_MORE_EVENTS, "Idle client sending explicit ACK");
	hdr->seq++;
	chanReply(lls, hdr, FROM_CLIENT, path);
    }
}


/* 
 * Unblock the current calling thread and have it return with a
 * failure code.  The client session goes into 'idle' state.
 */
static void
abortCall( self )
    XObj	self;
{

    CHAN_STATE	*ss = (CHAN_STATE *)self->state;

    xTrace0(chanp, TR_EVENTS, "chan clientAbort");
    if ( ss->fsmState != CLNT_CALLING && ss->fsmState != CLNT_WAIT ) {
	xTrace1(chanp, TR_SOFT_ERRORS,
		"chan client abort fails, state is %s",
		chanStateStr(ss->fsmState));
	return;
    }
    killticket(self);
    evCancel(ss->event);
    ss->fsmState = CLNT_FREE;
    if ( ! ss->isAsync ) {
	ss->replyValue = XK_FAILURE;
	semSignal(&ss->reply_sem);
    }
}


typedef struct {
    int		seq;
    Channel	chan;
} IdleMapEntry;


static int
returnFirstElem( key, val, arg )
    void	*key, *val, *arg;
{
    IdleMapEntry	*e = (IdleMapEntry *)arg;

    e->chan = *(Channel *)key;
    e->seq = (int)val;
    return 0;
}


/* 
 * Recreates any idle session for the appropriate host, unbinding it from
 * the idle client map and binding it to the active client map.
 * 'key->prot_id' and 'key->lls'
 * should be valid.  'key->chan' will be filled in if an appropriate
 * session is found.
 */
static XObj
getAnyIdleSessn( self, hlpRcv, hlpType, key, path )
    XObj	self, hlpRcv, hlpType;
    ActiveID	*key;
    Path	path;
{
    PSTATE		*ps = (PSTATE *)self->state;
    Map			map;
    IdleMapEntry	e;
    XObj		s;
    int			seqCheck;

    map = chanGetMap(ps->cliInfo.idleMap, key->lls, key->prot_id);
    if ( ! map ) {
	xTrace0(chanp, TR_EVENTS, "chanGetAnyIdleSessn -- no map");
	return ERR_XOBJ;
    }
    e.seq = -1;
    mapForEach(map, returnFirstElem, &e);
    if ( e.seq == -1 ) {
	return ERR_XOBJ;
    } 
    /* 
     * Transfer from idle to active map
     */
    xAssert( mapResolve(map, &e.chan, &seqCheck) == XK_SUCCESS && 
	     seqCheck == e.seq );
    key->chan = e.chan;
    s = chanClientOpen(self, hlpRcv, hlpType, key, e.seq, FALSE, path);
    if ( s == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    mapUnbind(map, &e.chan);
    return s;
}


static xkern_return_t
chanClientClose( s )
    XObj	s;
{
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;
    PSTATE	*ps = (PSTATE *)xMyProtl(s)->state;

    xTrace1(chanp, TR_MAJOR_EVENTS, "chan client close of sessn %x", s);
    switch ( ss->fsmState ) {
      case CLNT_WAIT:
	/* 
	 * This shouldn't happen
	 */
	xTrace0(chanp, TR_ERRORS,
		"chanClientClose called in CLNT_WAIT state!!!");
	abortCall(s);
	break;

      case CLNT_FREE:
	xTrace0(chanp, TR_MAJOR_EVENTS, "chanClientClose marking sessn idle");
	return chanAddIdleSessn(s, &ps->cliInfo);

      case DISABLED:
	xTrace0(chanp, TR_MAJOR_EVENTS, "chanClientClose destroying sessn");
	chanDestroy(s);
	break;

      default:
	xTrace1(chanp, TR_ERRORS,
		"chanClientClose called with unhandled state type %s",
		chanStateStr(ss->fsmState));
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static int
chanClientControl( self, op, buf, len )
    XObj	self;
    int		op, len;
    char 	*buf;
{
    switch ( op ) {
	
      case CHAN_ABORT_CALL:
#ifdef CHAN_ALLOW_USER_ABORT
	abortCall(self);
#endif
	return 0;

      default:
	return chanControlSessn( self, op, buf, len );
    }
}


/* 
 * Used as the 'call' function for disabled sessions
 */
static xkern_return_t
errorCall(
    XObj	s,
    Msg		m,
    Msg		rm )
{
    return XK_FAILURE;
}


/* 
 * Disable the session 'val'.  It is removed from the map, it's caller
 * is released with failure, and any subsequent calls will fail.
 * This function must not block.
 */
static int
zombifyClient( key, val )
    void	*key;
    void	*val;
{
    XObj	s = (XObj)val;
    PState	*ps;
    CHAN_STATE	*ss;
    xkern_return_t	res;
    
    xAssert(xIsSession(s));
    ss = (CHAN_STATE *)s->state;
    ps = (PState *)xMyProtl(s)->state;
    xTrace1(chanp, TR_EVENTS,
	    "chanZombifyClient -- disabling session %x", s);
    if ( ss->fsmState == CLNT_CALLING || ss->fsmState == CLNT_WAIT ) {
	xTrace0(chanp, TR_EVENTS, "chanZombifyClient -- releasing caller");
	abortCall(s);
    } else {
	xAssert( ss->fsmState == CLNT_FREE );
    }
    ss->fsmState = DISABLED;
    /* 
     * Remove session from 'key' map.  It will be removed from the
     * 'host' map by mapForEach.
     */
    xAssert(s->binding);
    res = mapRemoveBinding(ps->cliInfo.keyMap, s->binding);
    xAssert( res == XK_SUCCESS );
    s->binding = 0;
    s->call = errorCall;
    return MFE_CONTINUE | MFE_REMOVE;
}


/* 
 * In response to notification that a peer rebooted, destroy idle
 * client sessions and disable active client sessions to that host.
 * This function must not block.
 */
void
chanClientPeerRebooted( ps, peer )
    PSTATE	*ps;
    IPhost	*peer;
{
    xTrace0(chanp, TR_EVENTS, "removing client sessions for rebooted host");
    chanMapChainApply(ps->cliInfo.idleMap, peer, chanMapRemove);
    chanMapChainApply(ps->cliInfo.hostMap, peer, zombifyClient);
}


/* 
 * We have received a first-contact notification for this channel's
 * peer.  Immediately send a retransmission, if appropriate.
 * This function is called directly from the notification handler and
 * thus must not block.
 *
 * This is a mapForEach function
 */
static int
immediateTimeout( 
    void	*key,
    void	*val )
{
    XObj	sessn = val;
    SState	*ss = sessn->state;

    if ( ss->fsmState == CLNT_CALLING || ss->fsmState == CLNT_WAIT ) {
	xTraceP0(sessn, TR_EVENTS,
		 "first contact, short-circuiting timeout");
	/* 
	 * Since this wasn't a full-duration timeout, don't enter
	 * exponential backoff quite yet ...
	 */
	ss->wait /= 2;
	evSchedule(ss->event, chanTimeout, sessn, 0);
    }
    return MFE_CONTINUE;
}


/* 
 * When informed that initial Boot-ID contact with a peer has been
 * established, resend requests for that peer immediately.
 */
void
chanClientPeerContact( 
    PSTATE	*ps,
    IPhost	*peer )
{
    xTrace0(chanp, TR_EVENTS, "informing client sessions of initial contact");
    chanMapChainApply(ps->cliInfo.hostMap, peer, immediateTimeout);
}    


xkern_return_t
chanGetProcClient(s)
    XObj s;
{
    xAssert(xIsSession(s));
    s->close 	= chanClientClose;
    s->call 	= chanCall;
    s->pop 	= chanClientPop; 
    s->control 	= chanClientControl;
    return XK_SUCCESS;
}


