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
 * blast_output.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * blast_output.c,v
 * Revision 1.40.1.5.1.2  1994/09/07  04:18:27  menze
 * OSF modifications
 *
 * Revision 1.40.1.5  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.40.1.4  1994/08/10  21:03:37  menze
 * Fixed trace statements
 * retransmit() prototype needs BlastMask
 *
 * Revision 1.40.1.3  1994/07/22  20:08:07  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.40.1.2  1994/04/23  00:17:47  menze
 * Fragments are built with the same allocator as their message
 * Added allocator call-backs
 * Return msg handle in push
 * field name spelling change
 *
 * Revision 1.40.1.1  1994/04/14  21:35:43  menze
 * Uses allocator-based message interface
 *
 * Revision 1.40  1993/12/16  01:24:25  menze
 * Removed inappropriate references to xkxkSemaphore internals.
 * Fixed mask references to compile with strict ANSI restrictions
 *
 * Revision 1.39  1993/12/13  22:34:45  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/blast/blast_internal.h>


static void		blastSendTimeout( Event, void * );
static void		freeMstate( MSG_STATE * );
static void		retransmit( BlastMask , MSG_STATE * );
static void		unmapMstate( MSG_STATE * );

/* 
 * mstate sending reference counts:
 *
 * Reference counts represent both:
 *	presence in the senders map (<= 1 ref count)
 *	retransmission requests (>= 0 ref counts)
 *
 * When a sending mstate is removed from the senders map (through
 * either a timeout or user notification), the reference count
 * associated with the map binding is freed.  After this has happened,
 * the mstate will be deallocated once all active retransmission
 * requests complete.
 */

static xmsg_handle_t
sendShortMsg(
    Msg m,
    SState *state )
{
    state->short_hdr.len = msgLen(m);
    xIfTrace(blastp, TR_DETAILED) {
	blast_phdr(&state->short_hdr);
    }
    xTrace1(blastp, TR_DETAILED, "in blast_push2 down_s = %x",
	    xGetDown(state->self, 0));
    if ( msgPush(m, blastHdrStore, &state->short_hdr, BLASTHLEN, 0) == XK_FAILURE ) {
	xTraceP0(state->self, TR_ERRORS, "msgPush fails");
	return XMSG_ERR_HANDLE;
    }
    xIfTrace(blastp,TR_DETAILED) {
	xTrace0(blastp, TR_ALWAYS, "message in header");
	blast_phdr(&state->short_hdr);
    }
    xTrace1(blastp, TR_DETAILED, "in blast_push2 msg len = %d", msgLen(m));
    if (xPush(xGetDown(state->self, 0), m) ==  -1) {
	xTrace0(blastp, TR_ERRORS, "blast_push: can't send message");
	return XMSG_ERR_HANDLE;
    }
    return XMSG_NULL_HANDLE;
}


static void
push_fragment(
    XObj s,
    Msg frag,
    BLAST_HDR *h )
{
    Msg_s msgToPush;

    xIfTraceP(s->up, TR_DETAILED) {
	xTraceP0(s->up, TR_ALWAYS, "Outgoing message ");
	blast_phdr(h);
    }
    msgConstructCopy(&msgToPush, frag);
    if ( msgPush(&msgToPush, blastHdrStore, h, BLASTHLEN, 0) == XK_SUCCESS ) {
	if ( xPush(s, &msgToPush) == XMSG_ERR_HANDLE ) {
	    xTraceP0(s->up, TR_ERRORS, "push: can't send fragment");
	}
    } else {
	xTraceP1(s->up, TR_ERRORS, "msgPush fails [path %d]",
		 pathGetId(msgGetPath(&msgToPush)));
    }
    msgDestroy(&msgToPush);
}

    	
xmsg_handle_t
blastPush(s, msg)
    XObj s;
    Msg msg;
{
    SState	*state;
    PState 	*pstate;
    MSG_STATE  	*mstate;
    BLAST_HDR 	*hdr;
    int 	seq;
    int 	num_frags;
    int 	i;
    XObj	lls;
    chan_info_t *info;
    
    xTrace0(blastp, TR_EVENTS, "in blast_push");
    xAssert(xIsSession(s));

    pstate = (PState *) s->myprotl->state;
    state = (SState *) s->state;
    xTrace1(blastp, TR_MORE_EVENTS, "blast_push: state = %d", state);
    xTrace1(blastp, TR_MORE_EVENTS,
	    "blast_push: outgoing msg len (no blast hdr): %d",
	    msgLen(msg));

    if (msgLen(msg) > state->fragmentSize * BLAST_MAX_FRAGS ) {
	xTrace2(blastp, TR_SOFT_ERRORS,
		"blast_push: message length %d > max size %d",
	       msgLen(msg), state->fragmentSize * BLAST_MAX_FRAGS);
	return XMSG_ERR_HANDLE;
    }
    
    /* if message is short, by-pass everthing */
    if ( msgLen(msg) < state->fragmentSize ) {
	return sendShortMsg(msg, state);
    }
    
    xTrace0(blastp, TR_DETAILED, "in blast_push3");

    /* get chan info */
    info = (chan_info_t *) msgGetAttr(msg,0);
    
    /* get new sequence number */
    if ( pstate->max_seq == BLAST_MAX_SEQ ) {
	xTraceP0(s, TR_MAJOR_EVENTS, "sequence number wraps");
	pstate->max_seq = 0;
    }
    seq = ++pstate->max_seq;
    
    /* need new message state */
    if ( ! (mstate = blastNewMstate(s)) ) {
	xTraceP0(s, TR_ERRORS, "MState allocation error");
	return XMSG_ERR_HANDLE;
    }
    mstate->state = state;
    /* bind mstate to sequence number */
    if ( mapBind(pstate->send_map, &seq, mstate, &mstate->binding)
    		!= XK_SUCCESS ) {
	xTraceP0(s, TR_SOFT_ERRORS, "push: can't bind sequence number");
	freeMstate(mstate);
	return XMSG_ERR_HANDLE;
    } 
    /* 
     * Add a reference count for this message
     */
    state->ircnt++;

    /* fill in header */
    hdr = &(mstate->hdr);
    *hdr = state->short_hdr;
    hdr->seq = seq;
    xIfTrace(blastp, TR_DETAILED) {
	xTrace0(blastp, TR_ALWAYS, "blasts_push: static header:");
	blast_phdr(hdr);
    }
    
    num_frags = (msgLen(msg) + state->fragmentSize - 1)/(state->fragmentSize);
    hdr->num_frag = num_frags;
    BLAST_MASK_SET_BIT(&hdr->mask, 1);
    
    /* send off fragments */
    semWait(&pstate->outstanding_messages);
    lls = xGetDown(s, 0);
    for ( i=1; i <= num_frags; i++ ) {
	Msg packet = &mstate->frags[i];
	msgConstructContig(packet, msgGetPath(msg), 0, 0);
	msgChopOff(msg, packet, state->fragmentSize);
	xTrace2(blastp, TR_MORE_EVENTS,
		"fragment: len[%d] = %d", i, msgLen(packet));
	/* fill in dynamic parts of header */
	hdr->len = msgLen(packet);
	xTrace1(blastp, TR_MORE_EVENTS, "blast_push: sending frag %d", i);
	push_fragment(lls, packet, hdr);
	BLAST_MASK_SHIFT_LEFT(hdr->mask, 1);
    }
    /* set timer to free message if no NACK is received */
    mstate->wait = SEND_CONST;
    evSchedule(mstate->event, blastSendTimeout, mstate, mstate->wait);
    if (info) {
	info->transport = s->myprotl;
	info->ticket = seq;
	info->reliable = 0;
	info->expensive = 1;
    }
    return seq;
}


/*
 * Retransmit the message referenced by mstate.  1's in 'mask' indicate
 * fragments received, 0's indicate fragments that need to be retransmitted.
 */
static void
retransmit( BlastMask mask, MSG_STATE *mstate )
{
    int 	i;
    SState	*state;
    BLAST_HDR 	*hdr;
    XObj	lls;
    
    xTrace2(blastp, REXMIT_T, "blast retransmit: called seq = %d mask = %s",
	    mstate->hdr.seq, blastShowMask(mask));
    if (mstate == 0) {
	xTrace0(blastp, REXMIT_T, "retransmit: no message state");
	return;
    }
    if ((state = (SState *)mstate->state) == 0) {
	xTrace0(blastp, TR_ERRORS, "retransmit: no state");
	return;
    }
    mstate->nacks_received++;
    hdr = &(mstate->hdr);
    hdr->op = BLAST_RETRANSMIT;
    /* send off fragments */
    lls = xGetDown(mstate->state->self, 0);
    for ( i=1; i<=hdr->num_frag; i++ ) {
	if ( ! BLAST_MASK_IS_BIT_SET(&mask, i) ) {
	    Msg packet = &mstate->frags[i];
	    xTrace2(blastp, REXMIT_T,
		    "retransmit: retransmitting fragment %d, msgptr %x",
		    i, packet);
	    /* fill in dynamic parts of header */
	    BLAST_MASK_CLEAR(hdr->mask);
	    BLAST_MASK_SET_BIT(&hdr->mask, i);
	    hdr->len = msgLen(packet);
	    push_fragment(lls, packet, hdr);
	}
    }
}


/* 
 * Removes the mstate from the senders map, kills the timeout event,
 * and removes the mstate reference count associated with the map
 * binding.
 */
static void
unmapMstate( ms )
    MSG_STATE	*ms;
{
#if	XK_DEBUG
    XObj	sessn;
#endif
    SState	*ss;
    PState	*ps;

    ss = ms->state;
#if	XK_DEBUG    
    sessn = ss->self;
#endif
    xTraceP0(sessn, TR_MORE_EVENTS, "unmapping mstate");
    ps = (PState *)xMyProtl(ss->self)->state;
    evCancel(ms->event);
    if (ms->binding) {
	if ( mapRemoveBinding(ps->send_map, ms->binding) == XK_SUCCESS ) {
	    /* 
	     * Release reference count for the map binding
	     */
	    freeMstate(ms);
	} else {
	    xTraceP0(sessn, TR_ERRORS, "unmapMstate: can't unbind!");
	}
    } else {
	xTraceP0(sessn, TR_ERRORS, "unmapMstate: no binding!");
    }
}


/*
 * Free the message and state associated with the outgoing message referenced
 * by mstate.
 */
static void
freeMstate(mstate)
    MSG_STATE *mstate;
{
    PState	*pstate;
    int		i;
    XObj	sessn;

    xTrace3(blastp, TR_MORE_EVENTS,
	    "blast output freeMstate, seq == %d, mstate == %x, rcnt == %d",
	    mstate->hdr.seq, mstate, mstate->rcnt);
    xAssert(mstate->rcnt > 0);
    mstate->rcnt--;
    if ( mstate->rcnt > 0 ) {
	return;
    }
    /* 
     * if rcnt == 0, unmapMstate should have been called.
     */
    sessn = ((SState *)mstate->state)->self;
    xTraceP0(sessn, TR_MORE_EVENTS, "truly deallocating sending mstate");
    pstate = (PState *)xMyProtl(sessn)->state;
    xTrace1(blastp, TR_MORE_EVENTS, "blast freeMstate: num_frags = %d",
	    mstate->hdr.num_frag);
    for ( i = 1; i <= mstate->hdr.num_frag; i++ ) {
	msgDestroy(&mstate->frags[i]);
    }
    if ( stackPush(pstate->mstateStack, (void *)mstate) ) {
	xTrace1(blastp, TR_MORE_EVENTS,
		"free_send_seq: mstate stack is full, freeing %x", mstate);
	evDetach(mstate->event);
	allocFree(mstate);
    }
    semSignal(&pstate->outstanding_messages);
    /* 
     * Remove ref count corresponding to this message
     */
    blastDecIrc(sessn);
}

/*
 * Are we making progress? 
 */
int
blast_Retransmit(pstate, seq)
    PState *pstate;
    int seq;
{
    MSG_STATE   *mstate;


    xTrace1(blastp, TR_MORE_EVENTS, "blast_Retransmit: begin seq = %d", seq);
    if ( mapResolve(pstate->send_map, &seq, &mstate) == XK_FAILURE ) {
        xTrace0(blastp, TR_SOFT_ERRORS, "blast_Retransmit: no mstate");
        return -1;
    }
    if (mstate->nacks_received) {
	mstate->nacks_received = 0;
	return 0;
    }
    return 1;
}


/* 
 * Kill the timeout event and free the outgoing message state
 */
int
blast_freeSendSeq(pstate, seq)
    PState *pstate;
    int seq;
{
    MSG_STATE	*mstate;
    
    xTrace1(blastp, TR_MORE_EVENTS, "free_send_msg: begin seq = %d", seq);
    if ( mapResolve(pstate->send_map, &seq, &mstate) == XK_FAILURE ) {
	xTrace0(blastp, TR_SOFT_ERRORS, "free_send_seq: nothing to free");
	return -1;
    } 
    unmapMstate(mstate);
    return 0;
}


/*
 * send_timout garbage collects the storage associated with 
 * a given sequence number. Since blast uses nacks it has 
 * to have some way of getting rid of storage 
 */
static void
blastSendTimeout(ev, arg)
    Event	ev;
    void 	*arg;
{
    MSG_STATE	*mstate = (MSG_STATE *)arg;

    xTrace0(blastp, REXMIT_T, "blast: send_timeout: timeout!");
    unmapMstate(mstate);
}

    
/* 
 * blastSenderPop -- process a retransmission request from out peer
 */
xkern_return_t
blastSenderPop(s, dg, hdr)
    XObj s;
    Msg dg;
    BLAST_HDR *hdr;
{
    SState	*state;
    PState	*pstate;
    MSG_STATE 	*mstate;
    
    xTrace0(blastp, TR_EVENTS, "BLAST_pop");
    xAssert(xIsSession(s));
    
    state = (SState *)s->state;
    pstate = (PState *)s->myprotl->state;
    /*
     * look for message state
     */
    if ( mapResolve(pstate->send_map, &hdr->seq, &mstate) == XK_FAILURE ) {
	xTraceP0(s, TR_EVENTS, "senderPop: unmatched nack");
	return XK_SUCCESS;
    }
    xAssert(mstate->rcnt > 0);
    mstate->rcnt++;
    /* 
     * Cancel and restart timeout event
     */
    if ( mstate->wait ) {
	xTraceP1(s, TR_MORE_EVENTS,
		"rescheduling send timeout for mstate %x", mstate);
	evSchedule(mstate->event, blastSendTimeout, mstate, mstate->wait);
	retransmit(hdr->mask, mstate);
    } else {
	/* 
	 * If the wait field hasn't been set yet, not all fragments
	 * have been originally transmitted, and we don't know which
	 * fragments in the mstate are valid.
	 */
	xTraceP0(s, TR_SOFT_ERRORS, "retransmission request ignored");
    }
    freeMstate(mstate);	/* decrease ref count */
    return XK_SUCCESS;
} 




static int
releaseSendMsgs( void *key, void *val, void *arg )
{
    MSG_STATE	*ms = val;
    Allocator	alloc = (Allocator)arg;

    xTraceP0(ms->state->self, TR_EVENTS, "call-back releaseSend");
    /* 
     * If the wait field hasn't been set, we may still be sending
     * and we shouldn't remove the state. 
     */
    if ( ms->wait ) {
	if ( alloc == pathGetAlloc(msgGetPath(&ms->frags[1]), MSG_ALLOC) ) {
	    xTraceP1(ms->state->self, TR_EVENTS,
		     "call-back releases message %d", ms->hdr.seq);
	    /* 
	     * in-line version of unmapMstate ... direct map
	     * operations here would not be advised.
	     */
	    evCancel(ms->event);
	    freeMstate(ms);
	    return MFE_CONTINUE | MFE_REMOVE;
	} else {
	    xTraceP1(ms->state->self, TR_EVENTS,
		     "call-back allocator mismatch, message %d",
		     ms->hdr.seq);
	}
    }
    return MFE_CONTINUE;
}


void
blastSendCallBack( XObj self, Allocator a )
{
    mapForEach(((PState *)self->state)->send_map, releaseSendMsgs, a);
}
