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
 * chan_server.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * chan_server.c,v
 * Revision 1.47.1.5.1.2  1994/09/07  04:18:43  menze
 * OSF modifications
 *
 * Revision 1.47.1.5  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.47.1.4  1994/08/10  21:27:05  menze
 * Server times out on failed push instead of destroying reply
 *
 * Revision 1.47.1.3  1994/07/22  20:08:10  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.47.1.2  1994/06/30  01:36:06  menze
 * Control messages are built using the allocator of the request
 * message, not the allocator attached to the session.
 *
 * Revision 1.47.1.1  1994/04/14  21:37:49  menze
 * Uses allocator-based message interface
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/chan/chan_internal.h>

static xkern_return_t	chanServerClose( XObj );
static void		closeStub( Event, void * );
static void		doCallDemux( XObj, CHAN_HDR *, Msg );
static int		validateOpenEnable( XObj );
static int		zombifyServer( void *, void * );


/*
 * chanSvcOpen
 */
XObj 
chanSvcOpen( self, hlpRcv, hlpType, key, seq, newSessn, path )
    XObj	self, hlpRcv, hlpType;
    ActiveID	*key;
    int		seq;
    boolean_t	newSessn;
    Path	path;
{
    XObj   	s;
    PSTATE 	*ps = (PSTATE *)self->state;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN svc open ........................");
    xIfTrace(chanp, TR_MAJOR_EVENTS) chanDispKey(key);
    s = chanCreateSessn(self, hlpRcv, hlpType, key, &ps->svrInfo, 
			newSessn, path);
    if ( s != ERR_XOBJ ) {
	CHAN_STATE	*ss = (CHAN_STATE *)s->state;
	ss->fsmState = SVC_IDLE;
	ss->hdr.flags = USER_MSG;
	ss->hdr.seq = seq;
	ss->waitParam = SERVER_WAIT_DEFAULT;
	ss->maxWait = SERVER_MAX_WAIT_DEFAULT;
    }
    return s;
}


xkern_return_t
chanOpenEnable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part 	p;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    PassiveID 	key;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN open enable ......................");
    if ( (key = chanGetProtNum(self, hlpType)) == -1 ) {
	return XK_FAILURE;
    }
    xTrace1(chanp, TR_MAJOR_EVENTS, "chan_openenable: prot_id = %d", key);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, (void *)&key);
}


xkern_return_t
chanOpenDisable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part 	p;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    PassiveID 	key;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN openDisable ......................");
    if ( (key = chanGetProtNum(self, hlpType)) == -1 ) {
	return XK_FAILURE;
    }
    xTrace1(chanp, TR_MAJOR_EVENTS, "chanOpenDisable: prot_id = %d", key);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, (void *)&key);
}


xkern_return_t
chanOpenDisableAll( self, hlpRcv )
    XObj	self, hlpRcv;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN openDisableAll ...................");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}



/*
 * chanServerPop
 */
static xkern_return_t
chanServerPop(
    XObj self,
    XObj lls,
    Msg msg,
    void *inHdr )
{
    CHAN_STATE 	*state;
    CHAN_HDR	*hdr = (CHAN_HDR *)inHdr;
    SEQ_STAT 	status;
    
    xTrace0(chanp, TR_EVENTS, "CHAN pop ...............................");
    
    xAssert(hdr);
    state = (CHAN_STATE *)self->state;
    if ( chanCheckMsgLen(hdr->len, msg) ) {
	/* 
	 * Message was too short -- probably mangled
	 */
	return XK_SUCCESS;
    }
    /* 
     * Check sequence number
     */
    status = chanCheckSeq(state->hdr.seq, hdr->seq);
    if ( status == old ) {
	return XK_SUCCESS;
    }
    xTrace3(chanp, TR_MORE_EVENTS, "state: %s  seq = %d  status: %s",
	    chanStateStr(state->fsmState), state->hdr.seq,
	    chanStatusStr(status));
    xAssert(hdr->chan == state->hdr.chan);
    switch(state->fsmState) {
	
      case SVC_IDLE: 
	
	if ( status == new ) {
	    if ( hdr->flags & USER_MSG ) {
		/*
		 * Fire up the server
		 */
		doCallDemux(self, hdr, msg);
	    } else if ( hdr->flags & ACK_REQUESTED ) {
		chanReply(lls, hdr, NEGATIVE_ACK, msgGetPath(msg));
	    }
	} else {
	    xTrace0(chanp, TR_EVENTS, "chan_pop: SVC_IDLE: ignoring msg");
	}
	break;
	
      case SVC_WAIT:
	if ( status == new ) {
	    /*
	     * We can free resources
	     */
	    xTrace0(chanp, TR_EVENTS, "Server received ACK.  Freeing message");
	    chanFreeResources(self);
	    /* 
	     * Remove ref count for this message
	     */
	    xAssert(self->rcnt > 1);
	    xClose(self);
	    state->fsmState = SVC_IDLE;
	    chanServerPop(self, lls, msg, hdr);
	    break;
	}
	xAssert( status == current );
	if ( hdr->flags & NEGATIVE_ACK ) {
	    /*
	     * Client lost reply message -- retransmit
	     */
	    xTrace0(chanp, TR_EVENTS,
		    "chanPop: client lost reply -- retransmitting");
	    chanResend(self, 0, 1);
	}
	break;

      case SVC_EXECUTE:
	if ( status == new ) {
#ifdef CHAN_ALLOW_USER_ABORT
	    state->fsmState = SVC_IDLE;
	    chanServerPop(self, lls, msg, hdr);
#endif
	    break;
	}
	xAssert(status == current);
	/*
	 * Send ack if requested
	 */
	if ( hdr->flags & ACK_REQUESTED ) {
	    chanReply(xGetDown(self, 0), hdr, 0, msgGetPath(msg));
	}
	break;
	
      default:
	xTrace1(chanp, TR_ERRORS,"chan_pop: invalid state %d",
		state->fsmState);
	break;
    } 
    xTrace2(chanp, TR_MORE_EVENTS, "end chanSvrPop:  state: %s curSeq = %d",
	    chanStateStr(state->fsmState), state->hdr.seq);
    return XK_SUCCESS;
}


/* 
 * validateOpenEnable -- Checks to see if there is still an openEnable for
 * the session and, if so, calls openDone.
 * This is called right before a message is sent up through
 * a session with no external references.  This has to be done
 * because CHAN sessions
 * can survive beyond removal of all external references. 
 *
 * Returns 1 if an openenable exists, 0 if it doesn't.
 */
static int
validateOpenEnable( s )
    XObj	s;
{
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;
    Enable	*e;
    PassiveID	key;
    PSTATE	*ps;

    ps = (PState *)xMyProtl(s)->state;
    key = ss->hdr.prot_id;
    if ( mapResolve(ps->passiveMap, &key, &e) == XK_FAILURE ) {
	xTrace1(chanp, TR_MAJOR_EVENTS,
		"chanValidateOE -- no OE for hlp %d!", key);
	return 0;
    }
    xOpenDone(e->hlpRcv, s, xMyProtl(s), xMyPath(s));
    return 1;
}
    

/*
 * doCallDemux
 */
static void
doCallDemux(self, inHdr, inMsg)
    XObj self;
    CHAN_HDR *inHdr;
    Msg inMsg;
{
    CHAN_STATE	*state;
    CHAN_HDR	*hdr;
    Msg_s	rmsg;
    int		packet_len;
    XObj	lls;
    SeqNum	oldSeq;
    
    state = (CHAN_STATE *)self->state;
    state->fsmState = SVC_EXECUTE;
    hdr = &state->hdr;
    hdr->seq = inHdr->seq;
    lls = xGetDown(self, 0);
    xAssert(xIsSession(lls));
    /*
     * Send ack if requested
     */
    if ( inHdr->flags & ACK_REQUESTED ) {
	chanReply( lls, inHdr, 0, msgGetPath(inMsg) );
    }
    xTrace1(chanp, TR_MORE_EVENTS,
	    "chan_pop: incoming length (no chan hdr): %d", msgLen(inMsg));
    /* 
     * Save current value the sequence number.  We
     * will not send the reply if it has changed by the
     * time the reply is ready.
     */
    oldSeq = state->hdr.seq;
    msgConstructContig(&rmsg, msgGetPath(inMsg), 0, 0);
    if ( self->rcnt <= 1 && ! validateOpenEnable(self) ) {
	xTrace0(chanp, TR_EVENTS, "CHAN -- no server process, no reply");
	msgDestroy(&rmsg);
	return;
    }
    xCallDemux(self, inMsg, &rmsg);
    xTrace0(chanp, TR_EVENTS, "chan_pop: xCallDemux returns");
    xTrace1(chanp, TR_MORE_EVENTS, "chan_pop: (SVC) outgoing length: %d",
	    msgLen(&rmsg));
    if ( oldSeq != state->hdr.seq ) {
	xTrace0(chanp, TR_EVENTS, "CHAN -- reply no longer relevant.");
	msgDestroy(&rmsg);
	return;
    }
    /* 
     * Increase session reference count for this message. 
     */
    xDuplicate(self);
    msgConstructCopy(&state->saved_msg.m, &rmsg);
    msg_valid(state->saved_msg);
    /*
     * Fill in reply header
     */
    hdr->len 	= msgLen(&rmsg); 
    
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(hdr);
    } 
    if ( msgPush(&rmsg, chanHdrStore, hdr, CHANHLEN, NULL) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "msgPush fails");
    } else {
	xTrace1(chanp, TR_MORE_EVENTS, "chan_pop: packet len %d", msgLen(&rmsg)); 
	xTrace1(chanp, TR_MORE_EVENTS, "chan_pop: length field: %d", hdr->len);
	xTrace2(chanp, TR_MORE_EVENTS, "chan_pop: lls %x packet = %x",
		xGetDown(self, 0), &rmsg); 
	/*
	 * Send reply
	 */
	packet_len    = msgLen(&rmsg); 
	bzero((char *)&state->info, sizeof(state->info));
	msgSetAttr(&rmsg, 0, (void *)&state->info, sizeof(state->info));
	if ( xPush(lls, &rmsg) >= 0 ) {
	    state->fsmState = SVC_WAIT;
	} else {
	    xTrace0(chanp, TR_SOFT_ERRORS, "chan_pop: (SVC) can't send message");
	}
    }
    if ( state->fsmState == SVC_WAIT && state->info.reliable ) {
	msg_flush(state->saved_msg);
    } else {
	state->wait = CHAN_SVC_DELAY(packet_len, state->waitParam);
	xTrace1(chanp, TR_DETAILED, "chan_pop: (SVC) server_wait: %d",
		state->wait);
	evSchedule(state->event, chanTimeout, self, state->wait);
    }
    msgDestroy(&rmsg);
}


static xkern_return_t
chanServerClose( s )
    XObj	s;
{
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;
    PState	*ps;

    ps = (PState *)xMyProtl(s)->state;
    xTrace1(chanp, TR_MAJOR_EVENTS, "chan server close of sessn %x", s);

    switch ( ss->fsmState ) {
      case SVC_EXECUTE:
      case SVC_WAIT:
	/* 

	 * This shouldn't happen.
	 */
	xTrace1(chanp, TR_ERRORS, "chanServerClose called in %s!!",
		chanStateStr(ss->fsmState));
	/* 
	 * fall-through
	 */

      case SVC_IDLE:
	chanAddIdleSessn(s, &ps->svrInfo);
	break;

      case DISABLED:
	chanDestroy(s);
	break;

      default:
	xTrace1(chanp, TR_ERRORS, "chanServerClose -- unknown state %s",
		chanStateStr(ss->fsmState));
	break;
    }

    return XK_SUCCESS;
}
    

static void
closeStub( ev, arg )
    Event	ev;
    void	*arg;
{
    xAssert( xIsSession((XObj)arg) );
    xClose( (XObj)arg );
}


/* 
 * Response to a peer reboot notification.  If this session has the
 * appropriate peer, put it in a
 * 'zombie' state where it is not in any map and is only waiting to be
 * shut down.
 *
 * This function must not block.
 */
static int
zombifyServer( key, val )
    void	*key;
    void	*val;
{
    XObj	s = (XObj)val;
    CHAN_STATE	*ss;
    PSTATE	*ps;
    FsmState	oldState;
    xkern_return_t	res;

    xAssert(xIsSession(s));
    ps = (PSTATE *)xMyProtl(s)->state;
    ss = (CHAN_STATE *)s->state;
    xTrace1(chanp, TR_EVENTS,
	    "chan serverDisableSessn -- zombifying session %x", s);
    /* 
     * Remove session from 'key' map.  It will be removed from the
     * 'host' map by mapForEach.
     */
    xAssert(s->binding);
    res = mapRemoveBinding(ps->svrInfo.keyMap, s->binding);
    xAssert( res == XK_SUCCESS );
    s->binding = 0;

    oldState = ss->fsmState;
    ss->fsmState = DISABLED;
    ss->hdr.seq++;	/* Make sure replies don't go out */
    evCancel(ss->event);
    msg_flush(ss->saved_msg);
    if ( oldState == SVC_WAIT ) {
	evDetach(evSchedule(ss->event, closeStub, s, 0));
    }
    return MFE_CONTINUE | MFE_REMOVE;
}



/* 
 * In response to notification that a peer rebooted, destroy idle
 * server sessions and disable active server sessions to that host.
 * This function must not block.
 */
void
chanServerPeerRebooted( ps, peer )
    PSTATE	*ps;
    IPhost	*peer;
{
    xTrace0(chanp, TR_EVENTS, "removing server sessions for rebooted host");

    chanMapChainApply(ps->svrInfo.idleMap, peer, chanMapRemove);
    chanMapChainApply(ps->svrInfo.hostMap, peer, zombifyServer);
}


xkern_return_t
chanGetProcServer(s)
    XObj s;
{
    xAssert(xIsSession(s));
    s->close 	= chanServerClose;
    s->pop 	= chanServerPop; 
    s->control 	= chanControlSessn;
    return XK_SUCCESS;
}

