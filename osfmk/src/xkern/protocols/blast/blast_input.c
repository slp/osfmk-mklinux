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
 * blast_input.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * blast_input.c,v
 * Revision 1.48.1.6.1.2  1994/09/07  04:18:26  menze
 * OSF modifications
 *
 * Revision 1.48.1.6  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.48.1.5  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.48.1.4  1994/07/22  20:08:06  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.48.1.3  1994/06/30  01:33:32  menze
 * Timeout messages are built using the allocator of the original
 * fragments, not the allocator attached to the session.
 *
 * Revision 1.48.1.2  1994/04/23  00:13:40  menze
 * Added allocator call-back handlers
 *
 * Revision 1.48.1.1  1994/04/14  21:34:32  menze
 * Uses allocator-based message interface
 *
 * Revision 1.48  1993/12/13  21:16:16  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

#include <machine/machlimits.h>
#include <xkern/include/xkernel.h>
#include <xkern/protocols/blast/blast_internal.h>

#define DONE(mask, num_frags) (BLAST_MASK_EQ(mask, blastFullMask[num_frags]))

static void	receive_timeout( Event, void * );
static void	free_rec_seq( MSG_STATE * );
static void	freeMstate( MSG_STATE * );
static void	insert_frag( BLAST_HDR *, Msg, MSG_STATE * );
static xkern_return_t	reassemble( Msg, MSG_STATE * );
static void	send_nack( MSG_STATE * );
static int	validateOpenEnable( XObj );

#define MIN(x,y) 		((x)<(y) ? (x) : (y))

/* 
 * mstate receiving reference counts:
 *
 * receiving ref counts aren't as complicated as sending reference
 * counts (where events accessing the mstate might block.)  Keep in
 * mind that this will have to be re-examined if blocking is allowed
 * betweeen looking up mstates in the receiving map and the last
 * reference in a code block.
 */

xkern_return_t
blastDemux(self, lls, dg)
    XObj self;
    XObj lls;
    Msg dg;
{
    PState 	*ps;
    BLAST_HDR 	hdr;
    XObj   	blast_s;
    ActiveID 	active_key;
    PassiveID 	passive_key;
    Enable	*e;

#ifdef BLAST_SIM_DROPS
    static int count = 0;
    
    count++;
    xTrace1(blastp, TR_EVENTS, "blast_demux: called(%d)", count);
#else
    xTrace0(blastp, TR_EVENTS, "blast_demux");
#endif /* BLAST_SIM_DROPS */

    xAssert(xIsProtocol(self));
    xAssert(xIsSession(lls));
    
    ps = (PState *) self->state;

    if ( msgPop(dg, blastHdrLoad, &hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTrace0(blastp, TR_SOFT_ERRORS, "msgPop failed!");
	return XK_FAILURE;
    }
    xIfTrace(blastp, TR_MORE_EVENTS) {
	xTrace0(blastp, TR_ALWAYS, "Incoming message ");
	blast_phdr(&hdr);
    }
    /* check for active sessions */
    active_key.lls = lls;
    active_key.prot = hdr.prot_id;

    xIfTrace(blastp, TR_MORE_EVENTS) {
	blastShowActiveKey(&active_key, "blast_demux");
    }
    if ( mapResolve(ps->active_map, &active_key, &blast_s) == XK_FAILURE ) {
	/* nacks must be sent to active session */
	if (hdr.op != BLAST_SEND) {
	    xTrace1(blastp, TR_EVENTS,
		    "blast_demux: spurious msg (op %s) dropped",
		    blastOpStr(hdr.op));
	    return XK_SUCCESS;
	}
	/*
	 * We don't want two threads trying to create the same session
	 */
	semWait(&ps->createSem);
	/*
	 * Check again for active session now that we have the lock
	 */
	if ( mapResolve(ps->active_map, &active_key, &blast_s)
	    						== XK_FAILURE ) {
	    /*
	     * create new session
	     */
	    passive_key = active_key.prot;
	    if ( mapResolve(ps->passive_map, &passive_key, &e)
							== XK_FAILURE ) {
		xTrace0(blastp, TR_EVENTS,
			"blast_demux: no open enable dropping message");
		semSignal(&ps->createSem);
		return XK_SUCCESS;
	    }
	    xTrace0(blastp, TR_EVENTS,
		    "blast_demux found openenable, opening blast sessn");
	    blast_s = blastCreateSessn(self, e->hlpRcv, e->hlpType,
				       &active_key, msgGetPath(dg));
	    if (blast_s == ERR_XOBJ) {
		xTrace0(blastp, TR_ERRORS,
			"blast_demux: can't create session");
		semSignal(&ps->createSem);
		return XK_SUCCESS;
	    }
	    xDuplicate(lls);
	    /* 
	     * xOpenDone will get called right before the message is sent up 
	     */
	}
	semSignal(&ps->createSem);
    }
    if (msgLen(dg) > hdr.len) {
	xTrace2(blastp, TR_MORE_EVENTS,
		"Truncating message from %d to %d bytes",
		msgLen(dg), hdr.len);
	msgTruncate(dg, hdr.len);
    }
#ifdef BLAST_SIM_DROPS
    /* test timeout ability */
    {
	static int 	count = 0;
	static XTime	t;

	if (count == 0) {
	    xGetTime(&t);
	}
	if ((++count % ( t.usec % 100 )) == 0) {
	    xError("blast_demux: dropping random fragment");
	    return XK_SUCCESS;
	}
    }
#endif
    return xPop(blast_s, lls, dg, &hdr);
}


/* 
 * validateOpenEnable -- Checks to see if there is still an openEnable for
 * the session and, if so, calls openDone.
 * This is called right before a message is sent up through
 * a session with no external references.  This has to be done
 * because BLAST sessions
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
    PassiveID	key;
    PState	*ps;

    ps = (PState *)xMyProtl(s)->state;
    key = ss->prot_id;
    if ( mapResolve(ps->passive_map, &key, &e) == XK_FAILURE ) {
	xTrace1(blastp, TR_MAJOR_EVENTS,
		"blastValidateOE -- no OE for hlp %d!", key);
	return 0;
    }
    xOpenDone(e->hlpRcv, s, xMyProtl(s), xMyPath(s));
    return 1;
}
    


static xkern_return_t
blastReceiverPop(
    XObj s,
    Msg dg,
    BLAST_HDR *hdr )
{
    SState	*state;
    MSG_STATE 	*mstate;
    Msg_s 	msg;
    int 	seq;
    xkern_return_t	xkr;
    
    xTrace0(blastp, TR_EVENTS, "BLAST receiverPop");
    xAssert(xIsSession(s));
    
    /*
     * If message is short just send it up
     */
    if (hdr->seq == 0) {
	xTrace0(blastp, TR_EVENTS, "blast_receiverPop: short cut");
	xTrace1(blastp, TR_DETAILED, "hdr_len = %d \n", hdr->len);
	if ( s->rcnt == 1 && ! validateOpenEnable(s) ) {
	    return XK_SUCCESS;
	}
	return xDemux(s, dg);
    }
    state = (SState *)s->state;
    seq = hdr->seq;
    
    /* look for message state */
    if ( mapResolve(state->rec_map, &seq, &mstate ) == XK_FAILURE ) {
	if (hdr->op == BLAST_RETRANSMIT) {
	    /* message must already be finished */
	    return XK_SUCCESS;
	}
	if ( ! (mstate = blastNewMstate(s)) ) {
	    xTraceP0(s, TR_ERRORS, "MState allocation error");
	    return XK_FAILURE;
	}
	mstate->state = state;
	mstate->hdr = *hdr;
	mstate->wait = hdr->num_frag * state->per_packet;
	if ( mapBind(state->rec_map, &seq, mstate, &mstate->binding)
	    	!= XK_SUCCESS ) {
	    freeMstate(mstate);
	    xTraceP0(s, TR_ERRORS, "MState bind error");
	    return XK_FAILURE;
	}	    
	mstate->canChangeBackoff = TRUE;
	evSchedule(mstate->event, receive_timeout, mstate, mstate->wait);
    }
    /* add fragment */
    insert_frag(hdr, dg, mstate);
    xTrace1(blastp, TR_MORE_EVENTS, "Inserted frag, new mask: %s",
	    blastShowMask(mstate->mask));
    if ( mstate->nacks_sent != 0 && hdr->op == BLAST_SEND &&
	 mstate->canChangeBackoff ) {
	/* 
	 * We timed out before an original fragment go to us ... back
	 * off the timeout.  This is a bit heavy-handed ... once we back
	 * off on this session, we never return.  Do this at most once
	 * per message. 
	 */
	mstate->canChangeBackoff = FALSE;
	state->per_packet = MIN(state->per_packet * BACKOFF, MAX_PER_PACKET);
	xTraceP1(s, REXMIT_T, "receive timer backoff to %d",
		 state->per_packet);
    }
    if (DONE(mstate->mask, hdr->num_frag)) {
	xkr = reassemble(&msg, mstate);
	xTrace1(blastp, TR_MORE_EVENTS,
		"blast_pop: incoming msg len (no hdr): %d",
		msgLen(&msg));
	/* get rid of message state */
	free_rec_seq(mstate);
	if ( xkr == XK_SUCCESS ) {
	    if ( s->rcnt > 1 || validateOpenEnable(s) ) {
		xDemux(s, &msg);
	    }
	}
	msgDestroy(&msg);
    } else  if ( BLAST_MASK_IS_BIT_SET(&hdr->mask, hdr->num_frag) ) {
#ifdef BLAST_LAST_FRAG_NACKS
	/* if this is the last fragment send a nack */
	if (mstate->nacks_sent == 0) {
	    xTrace0(blastp, REXMIT_T, "blast_pop: last frag !");
	    mstate->canChangeBackoff = FALSE;
	    evSchedule(mstate->event, receive_timeout, mstate, mstate->wait);
	    send_nack(mstate); 
	}
#endif
    }
    return XK_SUCCESS;
} 


xkern_return_t
blastPop(s, lls, dg, hdr)
    XObj 	s;
    XObj 	lls;
    Msg 	dg;
    void	*hdr; 
{
    xAssert(hdr);
    switch (((BLAST_HDR *)hdr)->op) {
      case BLAST_NACK:
    	return blastSenderPop(s, dg, hdr);

      case BLAST_SEND:
      case BLAST_RETRANSMIT:
	return blastReceiverPop(s, dg, hdr);

      default:
	xTrace0(blastp, TR_ERRORS, "blast_pop: illegal operation ");
	return XK_SUCCESS;
    }
}


/*
 * Free the message and state associated with the incoming message referenced
 * by mstate
 */
static void
free_rec_seq(mstate)
    MSG_STATE *mstate;
{
    EvCancelReturn	evr;

    xTrace1(blastp, TR_EVENTS,
	    "free_rec_msg: begin seq = %d", mstate->hdr.seq);
    
    xTrace1(blastp, TR_MORE_EVENTS, "Cancelling event %x", mstate->event);
    evr = evCancel(mstate->event);
#if	XK_DEBUG
    if ( evr != EVENT_CANCELLED ) {
	/* 
	 * This is an OK thing to happen ... just for debugging
	 */
	xTrace1(blastp, TR_SOFT_ERRORS, "free_rec_seq evCancel returns %d", evr);
    }
#endif
    freeMstate(mstate);
}


static void
freeMstate(mstate)
    MSG_STATE *mstate;
{
    int		i;
    SState	*state;
    XObj	sessn;
    PState	*ps;

    xAssert(mstate->rcnt > 0);
    mstate->rcnt--;
    if ( mstate->rcnt > 0 ) {
	/* 
	 * This shouldn't happen for receiving mstates ...
	 */
	xTrace1(blastp, TR_ERRORS, "receiving mstate ref cnt == %d > 0!",
		mstate->rcnt);
	return;
    }
    state = mstate->state;
    sessn = state->self;
    xAssert(xIsSession(sessn));
    ps = (PState *)xMyProtl(sessn)->state;
    if (mstate->binding) {
	if (mapRemoveBinding(state->rec_map,mstate->binding) == XK_FAILURE) {
	    xTrace0(blastp, TR_ERRORS, "free_rec_seq: can't unbind!");
	    return;
	}
    } 
    xTrace1(blastp, TR_MORE_EVENTS,
	    "free_rec_msg: num_frags = %d", mstate->hdr.num_frag);
    for ( i=1; i <= mstate->hdr.num_frag; i++ ) {
	if ( BLAST_MASK_IS_BIT_SET(&mstate->mask, i) ) {
	    msgDestroy(&mstate->frags[i]);
	}
    }
    if ( stackPush(ps->mstateStack, (void *)mstate) ) {
	xTrace1(blastp, TR_MORE_EVENTS,
		"free_rec_seq: mstate stack is full, freeing %x", mstate);
	evDetach(mstate->event);
	allocFree(mstate);
    }
    /* 
     * Remove reference count for this message
     */
    blastDecIrc(sessn);
}


/* insert_frag -- Insert the fragment 'msg' into the message referenced
 * by 'mstate'.  
 */
static void
insert_frag(hdr, msg, mstate)
    BLAST_HDR *hdr;
    Msg msg;
    MSG_STATE *mstate;
{
    int 	loc;
    
    xTrace0(blastp, TR_MORE_EVENTS, "insert_frag: called");
    loc = blast_mask_to_i(hdr->mask);
    if (loc == 0) {
	xTrace0(blastp, TR_ERRORS, "insert_frag: illegal fragment");
	return;
    }
    xTrace1(blastp, TR_MORE_EVENTS, "insert_frag: inserting fragment %d",loc);
    /* prefer later messages */
    xTrace2(blastp, TR_MORE_EVENTS,
	    "insert_frag: hdr->len = %d msg_len = %d", hdr->len,
	    msgLen(msg));
    xAssert(hdr->len == msgLen(msg));
    if ( BLAST_MASK_IS_BIT_SET(&mstate->mask, loc) ) {
	xTrace1(blastp, TR_SOFT_ERRORS,
		"blastInput: duplicate fragment %d", loc);
	xTrace1(blastp, TR_MORE_EVENTS, "Freeing fragment %d", loc);
	msgAssign(&mstate->frags[loc], msg);
    } else {
	msgConstructCopy(&mstate->frags[loc], msg);
    }
    BLAST_MASK_OR(mstate->mask, hdr->mask);
}


/*
 * Return the message formed by the assembly of the fragments in 'mstate'
 * The fragments themselves are freed and the mstate mask is cleared to
 * indicate that there are no more valid fragments in the mstate structure.
 */
static xkern_return_t
reassemble(m, mstate)
    Msg m;
    MSG_STATE *mstate;
{
    int 		i;
    xkern_return_t	xkr = XK_SUCCESS;

    xTrace0(blastp, TR_EVENTS, "reassemble: called");

    msgConstructCopy(m, &mstate->frags[1]);
    msgDestroy(&mstate->frags[1]);
 
    xAssert(BLAST_MASK_EQ(mstate->mask, blastFullMask[mstate->hdr.num_frag]));

    for (i=2; i<=mstate->hdr.num_frag; i++) {
	xTrace1(blastp, TR_MORE_EVENTS,
		"reassemble: frag = %x", &mstate->frags[i]);
	xTrace1(blastp, TR_MORE_EVENTS, "reassemble: frag_len = %d",
		msgLen(&mstate->frags[i]));
	if ( msgJoin(m, m, &mstate->frags[i]) == XK_FAILURE ) {
	    xTraceP0(mstate->state->self, TR_ERRORS, "msgJoin fails");
	    xkr = XK_FAILURE;
	}
	msgDestroy(&mstate->frags[i]);
    }
    xTrace1(blastp, TR_EVENTS, "reassemble: complete msg_len = %d", msgLen(m));
    BLAST_MASK_CLEAR(mstate->mask);
    return xkr;
}


/* receive timeout: sends nack if everything has not arrived in
 * a certain time. If nothing arrives between two nacks then 
 * msg is flushed 
 */
static void
receive_timeout(ev, arg)
    Event	ev;
    void 	*arg;
{
    MSG_STATE	*mstate = (MSG_STATE *)arg;
#if	XK_DEBUG
    XObj	s = mstate->state->self;
#endif    

    xTraceP0(s, REXMIT_T, "receive_timeout: timeout!");
    xAssert( ! evIsCancelled(ev));
    xAssert(mstate->rcnt > 0);
    xIfTrace(blastp, REXMIT_T) blastShowMstate(mstate, "receive timeout");
    if ( mstate->nacks_sent < NACK_LIMIT) { 
	xTraceP0(s, REXMIT_T, "receive_timeout sending nack");
	send_nack(mstate);
	if ( ! evIsCancelled(ev) ) {
	    xAssert(ev == mstate->event);
	    xAssert(mstate->rcnt > 0);
	    mstate->wait = MIN(BACKOFF * mstate->wait, MAX_WAIT );
	    evSchedule(mstate->event, receive_timeout, mstate, mstate->wait);
	} else {
	    xTraceP0(s, TR_EVENTS, "receive timeout cancelled, not rescheduling");
	}
    } else {
    	xTraceP0(s, REXMIT_T, "receive_timeout aborting");
    	/* flush message */
    	freeMstate(mstate);
    }
}


/* 
 * Be careful of blocking in this function ... mstate may be
 * deallocated after a call to a blocking function (e.g., xPush)
 */
static void
send_nack(mstate)
    MSG_STATE *mstate;
{
    SState 		*state;
    BLAST_HDR		hdr;
    Msg_s 		m;
    xkern_return_t	xkr;
    Path		path = 0;
    int			i;

    xTrace1(blastp, REXMIT_T, "send_nack: called mask = %s",
	    blastShowMask(mstate->mask));
    state = (SState *) mstate->state;
    mstate->nacks_sent++;
    
    hdr = mstate->hdr;
    hdr.op = BLAST_NACK;
    hdr.mask = mstate->mask;
    for ( i=1; i <= BLAST_MAX_FRAGS; i++ ) {
	if ( BLAST_MASK_IS_BIT_SET(&mstate->mask, i) ) {
	    path = msgGetPath(&mstate->frags[i]);
	    xAssert(path);
	    break;
	}
    }
    if ( ! path ) {
	xTraceP0(state->self, TR_ERRORS, "couldn't find allocator for nack");
	return;
    }
    if ( msgConstructContig(&m, path, 0, BLAST_STACK_LEN) == XK_FAILURE ) {
	xTraceP0(state->self, TR_SOFT_ERRORS, "Couldn't alloc Nack message");
	return;
    }
    xkr = msgPush(&m, blastHdrStore, &hdr, sizeof(hdr), 0);
    if ( xkr != XK_SUCCESS ) {
	xTraceP0(state->self, TR_SOFT_ERRORS, "Couldn't msgPush Nack message");
	msgDestroy(&m);
	return;
    }
    xIfTrace(blastp, REXMIT_T + 2) {
	blast_phdr(&hdr);
    }
    if (xPush(xGetDown(state->self, 0), &m) ==  -1) {
	xTrace0(blastp, REXMIT_T, "send_nack: can't send nack");
    }
    msgDestroy(&m);
}



/* 
 * Allocator call-back handlers.  blastAllocCallBack gets called when
 * space is tight, once for each allocator that has overdrawn its
 * reservation.  Right now, we'll drop all partially-reassembled
 * packets associated with that allocator in all sessions.
 */

static int
releaseMsgs( void *key, void *val, void *arg )
{
    MSG_STATE	*mstate = val;
    int		i;
    Allocator	alloc = arg;

    xTraceP0(mstate->state->self, TR_EVENTS, "call-back release");
    for ( i=1; i < BLAST_MAX_FRAGS; i++ ) {
	if ( BLAST_MASK_IS_BIT_SET(&mstate->mask, i) ) {
	    if ( alloc ==
		    pathGetAlloc(msgGetPath(&mstate->frags[i]), MSG_ALLOC) ) {
		xTraceP1(mstate->state->self, TR_EVENTS,
			"call-back releases message %d", mstate->hdr.seq);
		free_rec_seq(mstate);
	    } else {
		xTraceP1(mstate->state->self, TR_EVENTS,
			"call-back allocator mismatch, message %d",
			mstate->hdr.seq);
	    }
	    break;
	}
    }
    return MFE_CONTINUE;
}

int
blastRecvCallBack( void *key, void *val, void *arg )
{
    XObj	sessn = val;

    mapForEach(((SState *)sessn->state)->rec_map, releaseMsgs, arg);
    return MFE_CONTINUE;
}

