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
 * tcp_input.c
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *    @(#)tcp_input.c 7.13+ (Berkeley) 11/13/87
 *
 * Modified for x-kernel v3.1	12/10/90
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp_input.c,v
 * Revision 1.23.1.4.1.2  1994/09/07  04:18:33  menze
 * OSF modifications
 *
 * Revision 1.23.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.23.1.3  1994/07/22  20:08:15  menze
 * Uses Path-based message interface and UPI
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/tcp/tcp_internal.h>
#include <xkern/protocols/tcp/tcp_fsm.h>
#include <xkern/protocols/tcp/tcp_seq.h>
#include <xkern/protocols/tcp/tcp_timer.h>
#include <xkern/protocols/tcp/tcp_var.h>
#include <xkern/protocols/tcp/tcpip.h>
#include <xkern/protocols/tcp/tcp_debug.h>

#define CANTRCVMORE(so) (sototcpst(so)->closed & 2)

int	tcpprintfs = 0;
int	tcpcksum = 1;
int	tcprexmtthresh = 3;
struct	tcpiphdr tcp_saveti;
extern	tcpnodelack;

static void	tcp_dooptions(struct tcpcb *, char *, int, struct tcphdr *);
static boolean_t	tcp_pulloobchar( char *ptr, long len, void *oobc);
static void 	tcp_pulloutofband(XObj, struct tcphdr *, Msg);
static int      tcp_reass(struct tcpcb *, struct tcphdr *, XObj, Msg, Msg);

/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 *
 * Reassembled message, if one exists, is plaecd in demuxmsg
 */
#define	TCP_REASS(tp, th, so, m, dMsg, flags) { \
	if ((th)->th_seq == (tp)->rcv_nxt && \
	    (tp)->seg_next == (struct reass *)(tp) && \
	    (tp)->t_state == TCPS_ESTABLISHED) { \
		(tp)->rcv_nxt += msgLen(m); \
		flags = (th)->th_flags & TH_FIN; \
		tcpstat->tcps_rcvpack++;\
		tcpstat->tcps_rcvbyte += msgLen(m);\
		msgAssign((dMsg), (m)); \
	} else \
		(flags) = tcp_reass((tp), (th), (so), (m), (dMsg)); \
}


static int
tcp_reass(tp, th, so, m, demuxmsg)
	register struct tcpcb *tp;
	register struct tcphdr *th;
     	XObj so;
	Msg m, demuxmsg;
{
	register struct reass *q, *next;
	struct tcpstat *tcpstat = ((PSTATE *)(xMyProtl(so)->state))->tcpstat;
	int flags = 0;

	xTrace0(tcpp, TR_FUNCTIONAL_TRACE, "tcp_reass function entered");
	/*
	 * Call with th==0 after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (th == 0)
		goto present;

/*	printf("ti_seq = %x, tp->rcv_nxt = %x\n", ti->ti_seq, tp->rcv_nxt); */
	if (tp->seg_next != (struct reass *)tp) {
	  q = tp->seg_next;
/*	  printf("   next seq number = %x\n", q->ti->ti_seq); */
	}

	/*
	 * Find a segment which begins after this one does.
	 */
	for (q = tp->seg_next; q != (struct reass *)tp;
	    q = (struct reass *)q->next)
		if (SEQ_GT(q->th.th_seq, th->th_seq))
			break;

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if (q->prev != (struct reass *)tp) {
		register int i;
		q = q->prev;
		/* conversion to int (in i) handles seq wraparound */
		i = q->th.th_seq + msgLen(&q->m) - th->th_seq;
		if (i > 0) {
			if (i >= msgLen(m)) {
				tcpstat->tcps_rcvduppack++;
				tcpstat->tcps_rcvdupbyte += msgLen(m);
				goto drop;
			}
			msgPopDiscard(m, i);
			th->th_seq += i;
		}
		q = q->next;
	}
	tcpstat->tcps_rcvoopack++;
	tcpstat->tcps_rcvoobyte += msgLen(m);

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while (q != (struct reass *)tp) {
		register int i = (th->th_seq + msgLen(m)) - q->th.th_seq;
		if (i <= 0)
			break;
		if (i < msgLen(&q->m)) {
			q->th.th_seq += i;
			msgPopDiscard(&q->m, i);
			break;
		}
		next = q->next;
		remque(q);
		msgDestroy(&q->m);
		allocFree(q);
		q = next;
	}

	/*
	 * Stick new segment in its place.
	 */
	{
		struct reass *new;
		if ( ! (new = pathAlloc(so->path, sizeof *new)) ) {
		    return 0;
		}
		new->th = *th;
		msgConstructCopy(&new->m, m);
		insque(new, q);
/*		print_reass(tp, "Inserted segment"); */
	}
present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (TCPS_HAVERCVDSYN(tp->t_state) == 0)
		return (0);
	q = tp->seg_next;
	th = &q->th;
	if (q == (struct reass *)tp || th->th_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == TCPS_SYN_RECEIVED && msgLen(&q->m))
		return (0);
	do {
		next = q->next;
		if (! CANTRCVMORE(so)) {
			/*
			 * if msgJoin fails dont dequeue this queue element
			 * return to user the longest demuxmsg possible
			 */
			if (msgJoin(demuxmsg, demuxmsg, &q->m) == XK_FAILURE) {
				break;
			}
		}
		flags = th->th_flags & TH_FIN;
		tp->rcv_nxt += msgLen(&q->m);
		remque(q);
		msgDestroy(&q->m);
		allocFree(q);
		q = next;
		th = &q->th;
	} while (q != (struct reass *)tp && th->th_seq == tp->rcv_nxt);
	return (flags);
drop:
	return (0);
}


xkern_return_t
tcpPop(self, lls, m, h)
    XObj self;
    XObj lls;
    Msg m;
    void *h;
{
    return xDemux(self, m);
}


/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
xkern_return_t
tcp_input(self, transport_s, m)
	XObj self, transport_s;
	Msg m;
{
/*	register struct tcpiphdr *ti; */
	struct inpcb *inp;
/*	Msg m; */
	char *options = 0;
	int option_len = 0;
	int len, tlen, off;
	struct tcpcb *tp;
	register int tiflags;
	XObj so = 0, hlpType = 0;
	struct tcpstate *tcpst;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate;
	int dropsocket = 0;
	int iss = 0;
	Msg_s demuxmsg;
	struct tcphdr 	tHdr;
	IPpseudoHdr 	*pHdr;
	boolean_t invoke_cantrcvmore = FALSE;
	struct tcpstat	*tcpstat = ((PSTATE *)self->state)->tcpstat;

	tcpstat->tcps_rcvtotal++;

	tlen = msgLen(m);
	pHdr = (IPpseudoHdr *)msgGetAttr(m, 0);
	xAssert(pHdr);
	/*
	 * Get IP and TCP header together in stack part of message.
	 * UNIX Note: IP leaves IP header in first mbuf.
	 * x-kernel Note:  IP attaches IP pseudoheader (in network byte
	 * order) as the attribute of the message
	 */
	if ( msgPop(m, tcpHdrLoad, &tHdr, sizeof(tHdr), m) == XK_FAILURE ) {
		xTrace0(tcpp, 3, 
			"tcp_input: msgPop of header failed -- dropping");
		return XK_FAILURE;
	}
	msgSetAttr(m, 0, 0, 0);

	xTrace4(tcpp, 3, "tcp_input seq %d, dlen %d, f( %s ) to port (%d)",
	       tHdr.th_seq, msgLen(m), tcpFlagStr(tHdr.th_flags),
	       tHdr.th_dport);
	msgConstructContig(&demuxmsg, xMyPath(self), 0, 0);

	/*
	 * Check the checksum value (calculated in tcpHdrLoad)
	 */
	if (tcpcksum) {
		if (tHdr.th_sum) {
			xTrace1(tcpp, 3,
				"tcp_input: bad checksum (%x)", tHdr.th_sum);
			tcpstat->tcps_rcvbadsum++;
#if BSD<=43
			tcpstat->tcps_badsum++;
#endif
			goto drop;
		}
	}

	xTrace1(tcpp, 4, "Received msg with sequence number %d",
		tHdr.th_seq);

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.
	 */
	off = tHdr.th_off << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		xTrace2(tcpp, 1, "bad tcp off: src %x off %d", pHdr->src, off);
		tcpstat->tcps_rcvbadoff++;
#if BSD<=43
		tcpstat->tcps_badoff++;
#endif
		goto drop;
	}
	tlen -= off;
	if ((option_len = off - sizeof (struct tcphdr)) > 0) {
		xTrace1(tcpp, 3, "tcp_input: %d bytes of options", option_len);
		/*
		 * Re-check the length, cause option_len increases the size 
		 * of the tcp header
		 */
		if (tlen < 0) {
			xTrace2(tcpp, 2,
				"tcp_input: rcvd short optlen = %d, len = %d",
				option_len, tlen);
			tcpstat->tcps_rcvshort++;
			msgDestroy(&demuxmsg);
			return XK_FAILURE;
		}
		/*
		 * Put the options somewhere reasonable		
		 */
		if ( ! (options = pathAlloc(self->path, option_len)) ) {
		    xTraceP0(self, TR_SOFT_ERRORS, 
			     "tcp_input: options allocation failure");
		    msgDestroy(&demuxmsg);
		    return XK_FAILURE;
		}
		if ( msgPop(m, tcpOptionsLoad, options, option_len, NULL)
		    == XK_FAILURE ) {
		        xTrace0(tcpp, 3, 
				"tcp_input: msgPop of options failed -- dropping");
			allocFree(options);
			msgDestroy(&demuxmsg);
			return XK_FAILURE;   
		}
	}
	tiflags = tHdr.th_flags;

	/*
	 * Locate pcb for segment.
	 */
findpcb:
	{
		ActiveId	activeId;
		PassiveId 	passiveId;
		PSTATE		*pstate;

		pstate = (PSTATE *)self->state;
		activeId.localport = tHdr.th_dport;
		activeId.remoteport = tHdr.th_sport;
		activeId.remoteaddr = pHdr->src;
		xTrace3(tcpp, 4, "Looking for %d->%X.%d", 
			activeId.localport, activeId.remoteaddr,
			activeId.remoteport);
		/* look in the active map */
		if ( mapResolve(pstate->activeMap, &activeId, &so)
		    					== XK_FAILURE ) {
			/*
			 * Look in the passive map
			 */
			Enable	*e;

			passiveId = activeId.localport;
			xTrace1(tcpp, 4, "Looking for passive open on %d",
				passiveId);
			/* look in the map */
			if ( mapResolve(pstate->passiveMap, &passiveId, &e)
			    				    == XK_FAILURE ) {
				xTrace0(tcpp, 4,
					"No passive open object exists");
				so = 0;
				inp = 0;
				tp = 0;
			} else {
				so = e->hlpRcv;
				hlpType = e->hlpType;
				xTrace1(tcpp, 4,
					"Found openenable object: %x",
					so);
				inp = 0;
				tp =  0;
			}
		} else {
			inp = sototcpst(so)->inpcb;
			tp =  sototcpst(so)->tcpcb;
		}
	}


	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */
	if (so == 0) {
		xTrace0(tcpp, 2, "tcp_input:  no so");
		goto dropwithreset;
	}
	if (xIsProtocol(so)) {
		so = sonewconn(self, so, hlpType, &pHdr->src, &pHdr->dst,
			       tHdr.th_sport, tHdr.th_dport, msgGetPath(m));
		if (so == 0)
			goto drop;
		/*
		 * This is ugly, but ....
		 *
		 * Mark socket as temporary until we're
		 * committed to keeping it.  The code at
		 * ``drop'' and ``dropwithreset'' check the
		 * flag dropsocket to see if the temporary
		 * socket created here should be discarded.
		 * We mark the socket as discardable until
		 * we're committed to it below in TCPS_LISTEN.
		 */
		dropsocket++;
		inp = sotoinpcb(so);
		bcopy((char *)&pHdr->dst, (char *)&inp->inp_laddr,
		      sizeof(pHdr->dst));
		inp->inp_lport = tHdr.th_dport;
		tp = intotcpcb(inp);
		tp->t_state = TCPS_LISTEN;
	}
	xIfTrace(tcpp, 1) {
		ostate = tp->t_state;
#if 0
		tcp_saveti = *ti;
#endif
	}
	tcpst = sototcpst(so);
	if (inp == 0) {
		xTrace0(tcpp, 2, "tcp_input:  no inp");
		goto dropwithreset;
	}
	if (tp == 0) {
		xTrace0(tcpp, 2, "tcp_input:  no tp");
		goto dropwithreset;
	}
	if (tp->t_state == TCPS_CLOSED) {
		xTrace0(tcpp, 2, "tcp_input:  TCPS_CLOSED");
		goto drop;
	}

/*
 * Proposed tcp_demux / tcp_pop split
 */

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	tp->t_idle = 0;
	tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;

	/*
	 * Process options if not in LISTEN state,
	 * else do it below (after getting remote address).
	 */
	if (options && tp->t_state != TCPS_LISTEN) {
		tcp_dooptions(tp, options, option_len, &tHdr);
		options = 0;
	}

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.
	 */
	{
		int win;
		win = tcpst->rcv_space;
		if (win < 0)
		  win = 0;
		tp->rcv_wnd = MAX(win, (int)(tp->rcv_adv - tp->rcv_nxt));
		xTrace4(tcpp, 5,
			"New win (%x) = max(std (%x), adv (%x) - nxt (%x))",
			tp->rcv_wnd, win, tp->rcv_adv, tp->rcv_nxt);
	}

	xTrace2(tcpp, 4, "tcp_input: tcpcb %X switch %s", tp,
		tcpstates[tp->t_state]);

	switch (tp->t_state) {

	/*
	 * If the state is LISTEN then ignore segment if it contains an RST.
	 * If the segment contains an ACK then it is bad and send a RST.
	 * If it does not contain a SYN then it is not interesting; drop it.
	 * Don't bother responding if the destination was a broadcast.
	 * Otherwise initialize tp->rcv_nxt, and tp->irs, select an initial
	 * tp->iss, and send a segment:
	 *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
	 * Also initialize tp->snd_nxt to tp->iss+1 and tp->snd_una to tp->iss.
	 * Fill in remote peer address fields if not previously specified.
	 * Enter SYN_RECEIVED state, and process any other fields of this
	 * segment in this state.
	 */
	case TCPS_LISTEN: {
		if (tiflags & TH_RST)
			goto drop;
		if (tiflags & TH_ACK)
			goto dropwithreset;
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (in_broadcast(*(struct in_addr *)&pHdr->dst))
			goto drop;
		xTrace0(tcpp, 3, "tcp_input: LISTEN");
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			tp = tcp_drop(tp, ENOBUFS);
			dropsocket = 0;		/* socket is already gone */
			goto drop;
		}
		if (options) {
			tcp_dooptions(tp, options, option_len, &tHdr);
		}
		if (iss)
			tp->iss = iss;
		else
			tp->iss = tcp_iss;
		tcp_iss += TCP_ISSINCR/2;
		tp->irs = tHdr.th_seq;
		tcp_sendseqinit(tp);
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		tp->t_state = TCPS_SYN_RECEIVED;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;
		dropsocket = 0;		/* committed to socket */
		tcpstat->tcps_accepts++;
		goto trimthenstep6;
	}

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(tHdr.th_ack, tp->iss) ||
		     SEQ_GT(tHdr.th_ack, tp->snd_max))) {
			xTrace0(tcpp, 4,
				"input state SYN_SENT -- dropping with reset");
			xTrace3(tcpp, 4, "   (ack==%d, iss==%d, snd_max==%d)",
				tHdr.th_ack, tp->iss, tp->snd_max);
			goto dropwithreset;
		}
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK)
				tp = tcp_drop(tp, ECONNREFUSED);
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = tHdr.th_ack;
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
		}
		tp->t_timer[TCPT_REXMT] = 0;
		tp->irs = tHdr.th_seq;
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			tcpstat->tcps_connects++;
			soisconnected(so);
			tp->t_state = TCPS_ESTABLISHED;
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			(void) tcp_reass(tp, NULL, so, NULL, &demuxmsg);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtt) {
				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
				TCPT_RANGESET(tp->t_rxtcur, 
				    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
				    TCPTV_MIN, TCPTV_REXMTMAX);
				tp->t_rtt = 0;
			}
		} else
			tp->t_state = TCPS_SYN_RECEIVED;

trimthenstep6:
		/*
		 * Advance ti->ti_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
	  	tHdr.th_seq++;
		if (tlen > tp->rcv_wnd) {
			todrop = tlen - tp->rcv_wnd;
			xTrace2(tcpp, 5,
				"tlen (%d) > rcv_wnd (%d) ... truncating",
				tlen, tp->rcv_wnd);
			msgTruncate(m, todrop);
			tlen = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			tcpstat->tcps_rcvpackafterwin++;
			tcpstat->tcps_rcvbyteafterwin += todrop;
		}
		tp->snd_wl1 = tHdr.th_seq - 1;
		tp->rcv_up = tHdr.th_seq;
		goto step6;
	}

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check that at least some bytes of segment are within 
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */
	todrop = tp->rcv_nxt - tHdr.th_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			tHdr.th_seq++;
			if (tHdr.th_urp > 1) 
				tHdr.th_urp--;
			else
				tiflags &= ~TH_URG;
			todrop--;
		}
		if (todrop > tlen ||
		    todrop == tlen && (tiflags & TH_FIN) == 0) {
#ifdef TCP_COMPAT_42
			/*
			 * Don't toss RST in response to 4.2-style keepalive.
			 */
			if (tHdr.th_seq == tp->rcv_nxt - 1 && tiflags & TH_RST)
				goto do_rst;
#endif
			tcpstat->tcps_rcvduppack++;
			tcpstat->tcps_rcvdupbyte += tlen;
			todrop = tlen;
			tiflags &= ~TH_FIN;
			tp->t_flags |= TF_ACKNOW;
		} else {
			tcpstat->tcps_rcvpartduppack++;
			tcpstat->tcps_rcvpartdupbyte += todrop;
		}
		xTrace1(tcpp, 5, "Discarding %d bytes from front of msg",
			todrop);
		(void) msgPopDiscard(m, todrop);
		tHdr.th_seq += todrop;
		tlen -= todrop;
		if (tHdr.th_urp > todrop)
			tHdr.th_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			tHdr.th_urp = 0;
		}
	}

	/*
	 * If new data is received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
#define ISNOTREFERENCED(X) 0
	if (ISNOTREFERENCED(so) &&
	    tp->t_state > TCPS_CLOSE_WAIT && tlen) {
		tp = tcp_destroy(tp);
		tcpstat->tcps_rcvafterclose++;
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (tHdr.th_seq + tlen) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat->tcps_rcvpackafterwin++;
		if (todrop >= tlen) {
			tcpstat->tcps_rcvbyteafterwin += tlen;
			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if (tiflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(tHdr.th_seq, tp->rcv_nxt)) {
				iss = tp->rcv_nxt + TCP_ISSINCR;
				(void) tcp_destroy(tp);
				goto findpcb;
			}
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && tHdr.th_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat->tcps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			tcpstat->tcps_rcvbyteafterwin += todrop;
		xTrace1(tcpp, 5,
			"segment ends after window -- truncating to %d",
			todrop);
		msgTruncate(m, todrop);
		tlen -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

#ifdef TCP_COMPAT_42
do_rst:
#endif
	/*
	 * If the RST bit is set examine the state:
	 *    SYN_RECEIVED STATE:
	 *	If passive open, return to LISTEN state.
	 *	If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	 *	Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT STATES
	 *	Close the tcb.
	 */
	if (tiflags & TH_RST) switch (tp->t_state) {

	case TCPS_SYN_RECEIVED:
		tp = tcp_drop(tp, ECONNREFUSED);
		goto drop;

	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
		tp = tcp_drop(tp, ECONNRESET);
		goto drop;

	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
		tp = tcp_destroy(tp);
		goto drop;
	}

	/*
	 * If a SYN is in the window, then this is an
	 * error and we send an RST and drop the connection.
	 */
	if (tiflags & TH_SYN) {
		tp = tcp_drop(tp, ECONNRESET);
		goto dropwithreset;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0)
		goto drop;
	
	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

	/*
	 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	 * ESTABLISHED state and continue processing, otherwise
	 * send an RST.
	 */
	case TCPS_SYN_RECEIVED:
		if (SEQ_GT(tp->snd_una, tHdr.th_ack) ||
		    SEQ_GT(tHdr.th_ack, tp->snd_max))
			goto dropwithreset;
		tcpstat->tcps_connects++;
		soisconnected(so);
		tp->t_state = TCPS_ESTABLISHED;
		tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
		(void) tcp_reass(tp, NULL, so, NULL, &demuxmsg);
		tp->snd_wl1 = tHdr.th_seq - 1;
		/* fall into ... */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < ti->ti_ack <= tp->snd_max
	 * then advance tp->snd_una to ti->ti_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:

		if (SEQ_LEQ(tHdr.th_ack, tp->snd_una)) {
			if (tlen == 0 && tHdr.th_win == tp->snd_wnd) {
				tcpstat->tcps_rcvdupack++;
				/*
				 * If we have outstanding data (not a
				 * window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.  
                                 *
                                 * We know we're losing at the current
                                 * window size so do congestion avoidance
                                 * (set ssthresh to half the current window
                                 * and pull our congestion window back to
                                 * the new ssthresh).
                                 *
                                 * Dup acks mean that packets have left the
                                 * network (they're now cached at the receiver)
                                 * so bump cwnd by the amount in the receiver
                                 * to keep a constant cwnd packets in the
                                 * network.
                                 */

                                if (tp->t_timer[TCPT_REXMT] == 0 ||
                                    tHdr.th_ack != tp->snd_una)
                                        tp->t_dupacks = 0;
                                else if (++tp->t_dupacks == tcprexmtthresh) {
                                       tcp_seq onxt = tp->snd_nxt;
                                        u_long win =
                                            MIN(tp->snd_wnd,
                                                tp->snd_cwnd);

                                        win /= tp->t_maxseg;
                                        win >>= 1;
                                        if (win < 2)
                                                win = 2;
                                        tp->snd_ssthresh = win * tp->t_maxseg;

                                        tp->t_timer[TCPT_REXMT] = 0;
                                        tp->t_rtt = 0;
                                        tp->snd_nxt = tHdr.th_ack;
                                        tp->snd_cwnd = tp->t_maxseg;
                                        (void) tcp_output(tp);
                                        tp->snd_cwnd = tp->snd_ssthresh +
                                                       tp->t_maxseg *
                                                       tp->t_dupacks;
                                        if (SEQ_GT(onxt, tp->snd_nxt))
                                                tp->snd_nxt = onxt;
                                        goto drop;
                               } else if (tp->t_dupacks > tcprexmtthresh) {
                                        tp->snd_cwnd += tp->t_maxseg;
                                        (void) tcp_output(tp);
                                        goto drop;
                                }
			} else
				tp->t_dupacks = 0;
			break;
		}
		tp->t_dupacks = 0;
		if (SEQ_GT(tHdr.th_ack, tp->snd_max)) {
			tcpstat->tcps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = tHdr.th_ack - tp->snd_una;
		xTrace2(tcpp, 4,
			"Received ACK for byte %d (%d new bytes ACKED)",
			tHdr.th_ack, acked);
		tcpstat->tcps_rcvackpack++;
		tcpstat->tcps_rcvackbyte += acked;

		/*
		 * If transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (tp->t_rtt && SEQ_GT(tHdr.th_ack, tp->t_rtseq)) {
			tcpstat->tcps_rttupdated++;
			if (tp->t_srtt != 0) {
				register short delta;

				/*
				 * srtt is stored as fixed point with 3 bits
				 * after the binary point (i.e., scaled by 8).
				 * The following magic is equivalent
				 * to the smoothing algorithm in rfc793
				 * with an alpha of .875
				 * (srtt = rtt/8 + srtt*7/8 in fixed point).
				 * Adjust t_rtt to origin 0.
				 */
				tp->t_rtt--;
				delta = tp->t_rtt - (tp->t_srtt >> 3);
				if ((tp->t_srtt += delta) <= 0)
					tp->t_srtt = 1;
				/*
				 * We accumulate a smoothed rtt variance
				 * (actually, a smoothed mean difference),
				 * then set the retransmit timer to smoothed
				 * rtt + 2 times the smoothed variance.
				 * rttvar is stored as fixed point
				 * with 2 bits after the binary point
				 * (scaled by 4).  The following is equivalent
				 * to rfc793 smoothing with an alpha of .75
				 * (rttvar = rttvar*3/4 + |delta| / 4).
				 * This replaces rfc793's wired-in beta.
				 */
				if (delta < 0)
					delta = -delta;
				delta -= (tp->t_rttvar >> 2);
				if ((tp->t_rttvar += delta) <= 0)
					tp->t_rttvar = 1;
			} else {
				/* 
				 * No rtt measurement yet - use the
				 * unsmoothed rtt.  Set the variance
				 * to half the rtt (so our first
				 * retransmit happens at 2*rtt)
				 */
				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
			}
			tp->t_rtt = 0;
			tp->t_rxtshift = 0;
			TCPT_RANGESET(tp->t_rxtcur, 
			    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
			    TCPTV_MIN, TCPTV_REXMTMAX);
		}

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (tHdr.th_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			needoutput = 1;
		} else if (tp->t_timer[TCPT_PERSIST] == 0)
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (maxseg per packet).
		 * Otherwise open linearly (maxseg per window,
		 * or maxseg^2 / cwnd per packet).
		 */
		{
		u_long incr = tp->t_maxseg;

		if (tp->snd_cwnd > tp->snd_ssthresh)
			incr = MAX(incr * incr / tp->snd_cwnd, 1);

		xTrace3(tcpp, TR_MAJOR_EVENTS,
			"tcp_input: congestion window=%x (MIN(%x+%x,65535))",
			tp->snd_cwnd + incr, tp->snd_cwnd, incr);
		tp->snd_cwnd = MIN(tp->snd_cwnd + incr, 65535); /* XXX */
		}
		if (acked > sblength(tcpst->snd)) {
			tp->snd_wnd -= sblength(tcpst->snd);
			sbdrop(tcpst->snd, sblength(tcpst->snd), self);
			ourfinisacked = 1;
		} else {
			sbdrop(tcpst->snd, acked, self);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}
		tcpSemSignal(&tcpst->waiting);
#if 0
		if ((so->so_snd.sb_flags & SB_WAIT) || so->so_snd.sb_sel)
			sowwakeup(so);
#endif
		tp->snd_una = tHdr.th_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more
				 * data, then closing user can proceed.
				 * Starting the timer is contrary to the
				 * specification, but if we don't get a FIN
				 * we'll hang forever.
				 */
				if (CANTRCVMORE(so)) {
					soisdisconnected(so);
					tp->t_timer[TCPT_2MSL] = TCPTV_MAXIDLE;
				}
				tp->t_state = TCPS_FIN_WAIT_2;
			}
			break;

	 	/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */
		case TCPS_CLOSING:
			if (ourfinisacked) {
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
				soisdisconnected(so);
			}
			break;

		/*
		 * In LAST_ACK, we may still be waiting for data to drain
		 * and/or to be acked, as well as for the ack of our FIN.
		 * If our FIN is now acknowledged, delete the TCB,
		 * enter the closed state and return.
		 */
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp = tcp_destroy(tp);
				goto drop;
			}
			break;

		/*
		 * In TIME_WAIT state the only thing that should arrive
		 * is a retransmission of the remote FIN.  Acknowledge
		 * it and restart the finack timer.
		 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			goto dropafterack;
		}
	}

step6:
	xTrace1(tcpp, 4, "tcp_input:  step6 on %X", tp);
	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 */
	if ((tiflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, tHdr.th_seq) || tp->snd_wl1 == tHdr.th_seq &&
	    (SEQ_LT(tp->snd_wl2, tHdr.th_ack) ||
	     tp->snd_wl2 == tHdr.th_ack && tHdr.th_win > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (tlen == 0 &&
		    tp->snd_wl2 == tHdr.th_ack && tHdr.th_win > tp->snd_wnd)
			tcpstat->tcps_rcvwinupd++;
		tp->snd_wnd = tHdr.th_win;
		tp->snd_wl1 = tHdr.th_seq;
		tp->snd_wl2 = tHdr.th_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}

	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) && tHdr.th_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data.
		 */
		if (tHdr.th_urp > SB_MAX) {
			tHdr.th_urp = 0;			/* XXX */
			tiflags &= ~TH_URG;		/* XXX */
			goto dodata;			/* XXX */
		}
		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since
		 * a FIN has been received from the remote side. 
		 * In these states we ignore the URG.
		 *
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section
		 * as the original spec states.
		 */
		if (SEQ_GT(tHdr.th_seq+tHdr.th_urp, tp->rcv_up)) {
		    tp->rcv_up = tHdr.th_seq + tHdr.th_urp;
		    sohasoutofband(so,
				   (tcpst->rcv_hiwat - tcpst->rcv_space) +
				   (tp->rcv_up - tp->rcv_nxt) - 1);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		/*
		 * Remove out of band data so doesn't get presented to user.
		 * This can happen independent of advancing the URG pointer,
		 * but if two URG's are pending at once, some out-of-band
		 * data may creep in... ick.
		 */
		if (tHdr.th_urp <= tlen &&
		    (tcpst->flags & TCPST_OOBINLINE) == 0)
		  tcp_pulloutofband(so, &tHdr, m);
	} else
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
dodata:							/* XXX */
	xTrace1(tcpp, 4, "tcp_input:  do data on %X", tp);
	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((tlen || (tiflags&TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		xTrace1(tcpp, 5,
			"Calling macro reassemble with msg len %d",
			msgLen(m));
		TCP_REASS(tp, &tHdr, so, m, &demuxmsg, tiflags);
		xTrace1(tcpp, 5,
			"after reassembly demux msg has length %d",
			msgLen(m));
		/*
		 * Pass received message to user:
		 */
		{
		    int mlen = msgLen(&demuxmsg);
		    if (mlen != 0) {
			if ( ! (tcpst->flags & TCPST_RCV_ACK_ALWAYS) ) {
			    tcpst->rcv_space -= mlen;
			}
			xPop(so, transport_s, &demuxmsg, 0);
		    } /* if */
		    msgDestroy(&demuxmsg);
		}

		if (tcpnodelack == 0)
			tp->t_flags |= TF_DELACK;
		else
			tp->t_flags |= TF_ACKNOW;
		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 */
		len = tcpst->rcv_hiwat - (tp->rcv_adv - tp->rcv_nxt);
		if (len > tp->max_rcvd)
			tp->max_rcvd = len;
	} else {
		tiflags &= ~TH_FIN;
	}

	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.
	 */
	if (tiflags & TH_FIN) {
		xTrace1(tcpp, 4, "tcp_input:  got fin on %X", tp);

		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		    invoke_cantrcvmore = TRUE;
		    tp->t_flags |= TF_ACKNOW;
		    tp->rcv_nxt++;
		}
		switch (tp->t_state) {

	 	/*
		 * In SYN_RECEIVED and ESTABLISHED STATES
		 * enter the CLOSE_WAIT state.
		 */
		case TCPS_SYN_RECEIVED:
		case TCPS_ESTABLISHED:
			tp->t_state = TCPS_CLOSE_WAIT;
			break;

	 	/*
		 * If still in FIN_WAIT_1 STATE FIN has not been acked so
		 * enter the CLOSING state.
		 */
		case TCPS_FIN_WAIT_1:
			tp->t_state = TCPS_CLOSING;
			break;

	 	/*
		 * In FIN_WAIT_2 state enter the TIME_WAIT state,
		 * starting the time-wait timer, turning off the other 
		 * standard timers.
		 */
		case TCPS_FIN_WAIT_2:
			tp->t_state = TCPS_TIME_WAIT;
			tcp_canceltimers(tp);
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			soisdisconnected(so);
			break;

		/*
		 * In TIME_WAIT state restart the 2 MSL time_wait timer.
		 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			break;
		}
	}
	xIfTrace(tcpp, 2)
		tcp_trace(TA_INPUT, ostate, tp, &tcp_saveti, 0);

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW)) {
		(void) tcp_output(tp);
	}

	if (invoke_cantrcvmore) {
	    socantrcvmore(so);
	} /* if */
	return XK_SUCCESS;

dropafterack:
	xTrace1(tcpp, 4, "tcp_input:  drop after ack %X", tp);
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	msgDestroy(&demuxmsg);
	return XK_SUCCESS;

dropwithreset:
	xTrace1(tcpp, 4, "tcp_input:  drop with reset on %X", tp);
	if (options) {
		allocFree(options);
		options = 0;
	}
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 * Don't bother to respond if destination was broadcast.
	 */
	if ((tiflags & TH_RST) || in_broadcast(*(struct in_addr *)&pHdr->dst))
		goto drop;
	{
		IPpseudoHdr	tmpPhdr;
		
		tmpPhdr.src = pHdr->dst;
		tmpPhdr.dst = pHdr->src;
		tmpPhdr.zero = 0;
		tmpPhdr.prot = pHdr->prot;
		if (tiflags & TH_ACK)
		  	tcp_respond(tp, &tHdr, &tmpPhdr, tHdr.th_ack,
				    (tcp_seq)0, TH_RST, self);
		else {
			if (tiflags & TH_SYN)
			  	tlen++;
			tcp_respond(tp, &tHdr, &tmpPhdr, tHdr.th_seq + tlen,
				    (tcp_seq)0, TH_RST|TH_ACK, self);
		}
	}
	/* destroy temporarily created socket */
	if (dropsocket)
	  	soabort(so);
	msgDestroy(&demuxmsg);
	return XK_SUCCESS;

drop:
	xTrace1(tcpp, 4, "tcp_input:  drop on %X", tp);
	if (options) {
		allocFree(options);
		options = 0;
	}
	/*
	 * Drop space held by incoming segment and return.
	 */
	xIfTrace(tcpp, 2)
		tcp_trace(TA_DROP, ostate, tp, &tcp_saveti, 0);
	/* destroy temporarily created socket */
	if (dropsocket)
		soabort(so);
	msgDestroy(&demuxmsg);
	return XK_SUCCESS;
}


static void
tcp_dooptions(tp, options, option_len, tHdr)
	struct tcpcb *tp;
	char         *options;
	int	      option_len;
	struct tcphdr *tHdr;
{
	register u_char *cp;
	int opt, optlen, cnt;

	cp = (u_char *)options;
	cnt = option_len;
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			break;

		case TCPOPT_MAXSEG:
			if (optlen != 4)
				continue;
			if (!(tHdr->th_flags & TH_SYN))
				continue;
			tp->t_maxseg = *(u_short *)(cp + 2);
			tp->t_maxseg = ntohs((u_short)tp->t_maxseg);
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			break;
		}
	}
	allocFree(options);
}


static boolean_t
tcp_pulloobchar( ptr, len, oobc )
    char	*ptr;
    void	*oobc;
    long	len;
{
    xAssert(len >= 1);
    *(char *)oobc = *ptr;
    return FALSE;
} /* tcp_pulloobchar */


/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
static void
tcp_pulloutofband(so, th, m)
	XObj so;
	struct tcphdr *th;
	Msg m;
{
	Msg_s firstPart;
	struct tcpcb *tp = sototcpcb(so);
	int cnt = th->th_urp - 1;
	
	tp->t_oobflags |= TCPOOB_HAVEDATA;

	msgConstructContig(&firstPart, xMyPath(so), 0, 0);
	msgChopOff(m, &firstPart, cnt);
	msgForEach(m, tcp_pulloobchar, (void *)&tp->t_iobc);
	(void) msgPopDiscard(m, 1);
	msgJoin(m, &firstPart, m);
	msgDestroy(&firstPart);
}

/*
 *  Determine a reasonable value for maxseg size.
 *  If the route is known, use one that can be handled
 *  on the given interface without forcing IP to fragment.
 *  If bigger than an mbuf cluster (MCLBYTES), round down to nearest size
 *  to utilize large mbufs.
 *  If interface pointer is unavailable, or the destination isn't local,
 *  use a conservative size (512 or the default IP max size, but no more
 *  than the mtu of the interface through which we route),
 *  as we can't discover anything about intervening gateways or networks.
 *  We also initialize the congestion/slow start window to be a single
 *  segment if the destination isn't local; this information should
 *  probably all be saved with the routing entry at the transport level.
 *
 *  This is ugly, and doesn't belong at this level, but has to happen somehow.
 */
int
tcp_mss(tp)
	register struct tcpcb *tp;
{
    	XObj tcpSessn;
	int mss;

	tcpSessn = tcpcbtoso(tp);
	if (xControl(xGetDown(tcpSessn, 0), GETOPTPACKET,
		     (char *)&mss, sizeof(int)) < sizeof(int)) {
	    xTrace0(tcpp, 3, "tcp_mss: GETOPTPACKET control of lls failed");
	    mss = 512;
	}
	xTrace1(tcpp, 3, "tcp_mss: GETOPTPACKET control of lls returned %d",
		mss);
	mss -= sizeof(struct tcphdr);
	tp->snd_cwnd = mss;
	return (mss);
}


void
print_reass(tp, m)
    struct tcpcb *tp;
    char *m;
{
	struct reass *s;
	printf("Printing reass\n");
	printf("%s %x:", m, tp->rcv_nxt);
	for (s = tp->seg_next; s != (struct reass *)tp; s = s->next) {
		printf("[%x-%x) ", s->th.th_seq, msgLen(&s->m));
	}
	printf("\n");
}

