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
 * tcp_output.c
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
 *	@(#)tcp_output.c	7.12 (Berkeley) 12/7/87
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp_output.c,v
 * Revision 1.13.1.3.1.2  1994/09/07  04:18:34  menze
 * OSF modifications
 *
 * Revision 1.13.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/tcp/tcp_internal.h>
#define	TCPOUTFLAGS
#include <xkern/protocols/tcp/tcp_fsm.h>
#include <xkern/protocols/tcp/tcp_seq.h>
#include <xkern/protocols/tcp/tcp_timer.h>
#include <xkern/protocols/tcp/tcp_var.h>
#include <xkern/protocols/tcp/tcpip.h>
#include <xkern/protocols/tcp/tcp_debug.h>

/*
 * Initial options.
 */
u_char	tcp_initopt[4] = { TCPOPT_MAXSEG, 4, 0x0, 0x0, };


void
tcp_setpersist(tp)
	register struct tcpcb *tp;
{
	register t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;

	if (tp->t_timer[TCPT_REXMT])
		Kabort("tcp_output REXMT");
	/*
	 * Start/restart persistance timer.
	 */
	TCPT_RANGESET(tp->t_timer[TCPT_PERSIST],
	    t * tcp_backoff[tp->t_rxtshift],
	    TCPTV_PERSMIN, TCPTV_PERSMAX);
	if (tp->t_rxtshift < TCP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}


/*
 * Tcp output routine: figure out what should be sent and send it.
 */
int
tcp_output(tp)
	register struct tcpcb *tp;
{
	register XObj so = tp->t_inpcb->inp_session;
	register struct tcpstate *tcpst = sototcpst(so);
	register int len, win;
	struct tcpstat *tcpstat = ((PSTATE *)(xMyProtl(so)->state))->tcpstat;
	int off, flags;
	struct tcphdr	tHdr;
	u_char *opt;
	Msg_s m;
	unsigned optlen = 0;
	int idle, sendalot;

	xTrace0(tcpp, 5, "tcp output");
        if(tcpst == NULL) {
           /*
            * Oops, looks like the socket closed on us.  Well, no
            * need to do anymore output so... -mjk 8/16/90
            */
	    xTrace0(tcpp, 5, "tcpst NULL -- tcp output exiting");
	    return 0;
        }
	/*
	 * Determine length of data that should be transmitted,
	 * and flags that will be used.
	 * If there is some data or critical controls (SYN, RST)
	 * to send, then transmit; otherwise, investigate further.
	 */
	idle = (tp->snd_max == tp->snd_una);
again:
	sendalot = 0;
	off = tp->snd_nxt - tp->snd_una;

	xTrace2(tcpp, TR_MAJOR_EVENTS,
		"tcp_output: tp->snd_wnd=%x, tp->snd_cwnd=%x",
		tp->snd_wnd, tp->snd_cwnd);

	win = MIN(tp->snd_wnd, tp->snd_cwnd);

	/*
	 * If in persist timeout with window of 0, send 1 byte.
	 * Otherwise, if window is small but nonzero
	 * and timer expired, we will send what we can
	 * and go to transmit state.
	 */
	if (tp->t_force) {
		if (win == 0)
			win = 1;
		else {
			tp->t_timer[TCPT_PERSIST] = 0;
			tp->t_rxtshift = 0;
		}
	}

	len = MIN(sblength(tcpst->snd), win) - off;
	xTrace4(tcpp, 5,
		"tcp_output: sbLen: %d  win: %d  off: %d  len == %d",
		sblength(tcpst->snd), win, off, len);
	flags = tcp_outflags[tp->t_state];

	if (len < 0) {
		/*
		 * If FIN has been sent but not acked,
		 * but we haven't been called to retransmit,
		 * len will be -1.  Otherwise, window shrank
		 * after we sent into it.  If window shrank to 0,
		 * cancel pending retransmit and pull snd_nxt
		 * back to (closed) window.  We will enter persist
		 * state below.  If the window didn't close completely,
		 * just wait for an ACK.
		 */
		len = 0;
		if (win == 0) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->snd_nxt = tp->snd_una;
		}
	}
	if (len > tp->t_maxseg) {
		len = tp->t_maxseg;
		sendalot = 1;
	}
	if (SEQ_LT(tp->snd_nxt + len, tp->snd_una + sblength(tcpst->snd)))
		flags &= ~TH_FIN;
	win = tcpst->rcv_space;


	/*
	 * If our state indicates that FIN should be sent
	 * and we have not yet done so, or we're retransmitting the FIN,
	 * then we need to send.
	 */
	if (flags & TH_FIN &&
	    ((tp->t_flags & TF_SENTFIN) == 0 || tp->snd_nxt == tp->snd_una))
		goto send;
	/*
	 * Send if we owe peer an ACK.
	 */
	if (tp->t_flags & TF_ACKNOW)
		goto send;
	if (flags & (TH_SYN|TH_RST))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;

	/*
	 * Sender silly window avoidance.  If connection is idle
	 * and can send all data, a maximum segment,
	 * at least a maximum default-size segment do it,
	 * or are forced, do it; otherwise don't bother.
	 * If peer's buffer is tiny, then send
	 * when window is at least half open.
	 * If retransmitting (possibly after persist timer forced us
	 * to send into a small window), then must resend.
	 */
	if (len) {
		if (len == tp->t_maxseg)
			goto send;
		if ((idle || tp->t_flags & TF_NODELAY) &&
		    len + off >= sblength(tcpst->snd))
			goto send;
		if (tp->t_force)
			goto send;
		if (len >= tp->max_sndwnd / 2)
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))
			goto send;
	}

	/*
	 * Compare available window to amount of window
	 * known to peer (as advertised window less
	 * next expected input).  If the difference is at least two
	 * max size segments or at least 35% of the maximum possible
	 * window, then want to send a window update to peer.
	 */
	if (win > 0) {
		int adv = win - (tp->rcv_adv - tp->rcv_nxt);

		if ((win == tcpst->rcv_hiwat) &&
		    (adv >= 2 * tp->t_maxseg))
		  goto send;
		if (100 * adv / tcpst->rcv_hiwat >= 35)
		  goto send;
	}

	/*
	 * TCP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are:
	 *	idle			not doing retransmits or persists
	 *	persisting		to move a small or zero window
	 *	(re)transmitting	and thereby not persisting
	 *
	 * tp->t_timer[TCPT_PERSIST]
	 *	is set when we are in persist state.
	 * tp->t_force
	 *	is set when we are called to send a persist packet.
	 * tp->t_timer[TCPT_REXMT]
	 *	is set when we are retransmitting
	 * The output side is idle when both timers are zero.
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state.
	 * If nothing happens soon, send when timer expires:
	 * if window is nonzero, transmit what we can,
	 * otherwise force out a byte.
	 */
	if (sblength(tcpst->snd) && tp->t_timer[TCPT_REXMT] == 0 &&
	    tp->t_timer[TCPT_PERSIST] == 0) {
		tp->t_rxtshift = 0;
		tcp_setpersist(tp);
	}

	/*
	 * No reason to send a segment, just return.
	 */
	xTrace0(tcpp, 5, "tcp_output -- no reason to send");
	return (0);

send:
	/*
	 * Grab a header mbuf, attaching a copy of data to
	 * be transmitted, and initialize the header from
	 * the template for sends on this connection.
	 */
	if ( sbcollect(tcpst->snd, &m, off, len, 0, so) == XK_FAILURE ) {
	    return ENOBUFS;	
	}
	if (len) {
		if (tp->t_force && len == 1)
			tcpstat->tcps_sndprobe++;
		else if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
			tcpstat->tcps_sndrexmitpack++;
			tcpstat->tcps_sndrexmitbyte += len;
		} else {
			tcpstat->tcps_sndpack++;
			tcpstat->tcps_sndbyte += len;
		}
	} else if (tp->t_flags & TF_ACKNOW)
		tcpstat->tcps_sndacks++;
	else if (flags & (TH_SYN|TH_FIN|TH_RST))
		tcpstat->tcps_sndctrl++;
	else if (SEQ_GT(tp->snd_up, tp->snd_una))
		tcpstat->tcps_sndurg++;
	else
		tcpstat->tcps_sndwinup++;

	if (tp->t_template == 0)
		Kabort("tcp_output");
	tHdr = tp->t_template->ti_t;
	/*
	 * Fill in fields, remembering maximum advertised
	 * window for use in delaying messages about window sizes.
	 * If resending a FIN, be sure not to use a new sequence number.
	 */
	if (flags & TH_FIN && tp->t_flags & TF_SENTFIN && 
	    tp->snd_nxt == tp->snd_max)
		tp->snd_nxt--;
	tHdr.th_seq = tp->snd_nxt;
	xTrace1(tcpp, 4, "Sending seq %d", tHdr.th_seq);
	tHdr.th_ack = tp->rcv_nxt;
	xTrace1(tcpp, 4, "Sending ack %d", tHdr.th_ack);
	/*
	 * Before ESTABLISHED, force sending of initial options
	 * unless TCP set to not do any options.
	 */
	opt = NULL;
	if (flags & TH_SYN && (tp->t_flags & TF_NOOPT) == 0) {
		u_short mss;

		mss = MIN(tcpst->rcv_hiwat / 2, tcp_mss(tp));
		if (mss > IP_MSS - sizeof(struct tcpiphdr)) {
			mss = htons(mss);
			opt = tcp_initopt;
			optlen = sizeof (tcp_initopt);
			bcopy((char *)&mss, (char *)opt+2, sizeof(short));
		}
	}
	tHdr.th_flags = flags;
	/*
	 * Calculate receive window.  Don't shrink window,
	 * but avoid silly window syndrome.
	 */
	if (win < (long)(tcpst->rcv_hiwat / 4) && win < (long)tp->t_maxseg)
		win = 0;
	if (win < (int)(tp->rcv_adv - tp->rcv_nxt))
		win = (int)(tp->rcv_adv - tp->rcv_nxt);
	tHdr.th_win = (u_short)win;
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		tHdr.th_urp = (u_short)(tp->snd_up - tp->snd_nxt);
		tHdr.th_flags |= TH_URG;
	} else
		/*
		 * If no urgent pointer to send, then we pull
		 * the urgent pointer to the left edge of the send window
		 * so that it doesn't drift into the send window on sequence
		 * number wraparound.
		 */
		tp->snd_up = tp->snd_una;		/* drag it along */
	/*
	 * If anything to send and we can send it all, set PUSH.
	 * (This will keep happy those implementations which only
	 * give data to the user when a buffer fills or a PUSH comes in.)
	 */
	if (len && off+len == sblength(tcpst->snd))
		tHdr.th_flags |= TH_PUSH;

	/*
	 * In transmit state, time the transmission and arrange for
	 * the retransmit.  In persist state, just set snd_max.
	 */
	if (tp->t_force == 0 || tp->t_timer[TCPT_PERSIST] == 0) {
		tcp_seq startseq = tp->snd_nxt;

		/*
		 * Advance snd_nxt over sequence space of this segment.
		 */
		if (flags & TH_SYN)
			tp->snd_nxt++;
		if (flags & TH_FIN) {
			tp->snd_nxt++;
			tp->t_flags |= TF_SENTFIN;
		}
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything.
			 */
			if (tp->t_rtt == 0) {
				tp->t_rtt = 1;
				tp->t_rtseq = startseq;
				tcpstat->tcps_segstimed++;
			}
		}

		/*
		 * Set retransmit timer if not currently set,
		 * and not doing an ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
		if (tp->t_timer[TCPT_REXMT] == 0 &&
		    tp->snd_nxt != tp->snd_una) {
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			if (tp->t_timer[TCPT_PERSIST]) {
				tp->t_timer[TCPT_PERSIST] = 0;
				tp->t_rxtshift = 0;
			}
		}
	} else
		if (SEQ_GT(tp->snd_nxt + len, tp->snd_max))
			tp->snd_max = tp->snd_nxt + len;
	/*
	 * Send it out.
	 */
	xTrace2(tcpp, 4, "Sending %d bytes with flags ( %s )",
		msgLen(&m), tcpFlagStr(tHdr.th_flags));
	if (opt) {
	    int padOptLen = (optlen + 3) & ~0x3;
	    if (msgPush(&m, tcpOptionsStore, opt, padOptLen, &optlen)
		     != XK_SUCCESS) {
	        msgDestroy(&m);
	        return ENOBUFS;
	    }
	    tHdr.th_off = (sizeof (struct tcphdr) + padOptLen) >> 2;
	}
	/*
	 *  Push the header
	 */ 
	{
	    hdrStore_t store;

	    store.h = &tp->t_template->ti_p;
	    store.m = &m;
	    if (msgPush(&m, tcpHdrStore, &tHdr, sizeof(struct tcphdr), &store)
		     != XK_SUCCESS) {
	        msgDestroy(&m);
	        return ENOBUFS;
	    }
	}
	if (xPush(xGetDown(tcpcbtoso(tp), 0), &m) != XMSG_NULL_HANDLE) {
	    msgDestroy(&m);
	    return ENOBUFS;
/*
	    if (error) {
		if (error == ENOBUFS)
			tcp_quench(tp->t_inpcb);
		return (error);
	    }
*/
	}
	msgDestroy(&m);
	tcpstat->tcps_sndtotal++;

	/*
	 * Data sent (as far as we can tell).
	 * If this advertises a larger window than any other segment,
	 * then remember the size of the advertised window.
	 * Any pending ACK has now been sent.
	 */
	if (win > 0 && SEQ_GT(tp->rcv_nxt+win, tp->rcv_adv)) {
		tp->rcv_adv = tp->rcv_nxt + win;
		xTrace3(tcpp, 5, "rcv_adv = rcv_nxt (%x) + win (%x) = %x",
			tp->rcv_nxt, win, tp->rcv_adv);
	}
	tp->t_flags &= ~(TF_ACKNOW|TF_DELACK);
	if (sendalot)
		goto again;
	return (0);
}


