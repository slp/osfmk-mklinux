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
 * tcp_subr.c
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
 *	@(#)tcp_subr.c	7.13 (Berkeley) 12/7/87
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp_subr.c,v
 * Revision 1.17.1.4.1.2  1994/09/07  04:18:33  menze
 * OSF modifications
 *
 * Revision 1.17.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.17.1.3  1994/07/22  20:08:17  menze
 * Uses Path-based message interface and UPI
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/icmp.h>
#include <xkern/protocols/tcp/tcp_internal.h>
#include <xkern/protocols/tcp/tcp_fsm.h>
#include <xkern/protocols/tcp/tcp_seq.h>
#include <xkern/protocols/tcp/tcp_timer.h>
#include <xkern/protocols/tcp/tcp_var.h>
#include <xkern/protocols/tcp/tcpip.h>

int	tcp_ttl = TCP_TTL;

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
struct tcpiphdr *
tcp_template(tp)
	struct tcpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register struct tcpiphdr *n;
	XObj	lls, tcpSessn;
	IPpseudoHdr	pHdr;

	tcpSessn = tcpcbtoso(tp);
	xAssert(xIsSession(tcpSessn));
	if ((n = tp->t_template) == 0) {
		if ( ! (n = pathAlloc(tcpSessn->path, sizeof *n)) ) {
			return 0;
		}
	}
	lls = xGetDown(tcpSessn,0);
	xAssert(xIsSession(lls));
	if ( xControl(lls, IP_GETPSEUDOHDR, (char *)&pHdr, sizeof(pHdr)) < 0 ) {
	    xTraceP0(tcpSessn, TR_ERRORS,
		     "tcp_template could not get pseudo-hdr from lls");
	    pHdr.prot = 0;
	}
	n->ti_next = n->ti_prev = 0;
	n->ti_x1 = 0;
	n->ti_pr = pHdr.prot;
	n->ti_len = sizeof (struct tcpiphdr) - sizeof (struct ipovly);
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_raddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_rport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	/*
	 * IP pseudo-header
	 */
	n->ti_p.src = *(IPhost *)&n->ti_src;
	n->ti_p.dst = *(IPhost *)&n->ti_dst;
	n->ti_p.zero = 0;
	n->ti_p.prot = n->ti_pr;
	return (n);
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If flags==0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */
void
tcp_respond(tp, th, pHdr, ack, seq, flags, tcp)
	struct tcpcb *tp;
    	register struct tcphdr *th;
    	register IPpseudoHdr *pHdr;
	tcp_seq ack, seq;
	int flags;
	XObj tcp;
{
	int win = 0, tlen;
	Msg_s m;
	XObj s = NULL;
	struct tcphdr tHdr;
	hdrStore_t	store;
	xkern_return_t	ret;

	if (tp) {
		s = tp->t_inpcb->inp_session;
		win = sototcpst(s)->rcv_space;
		s = xGetDown(s, 0);
	}
#ifdef TCP_COMPAT_42
	tlen = flags == 0;
#else
	tlen = 0;
#endif
	tHdr = *th;
	if (flags == 0) {
		flags = TH_ACK;
	} else {
	    	u_short tmp;

		tmp = tHdr.th_sport;
		tHdr.th_sport = tHdr.th_dport;
		tHdr.th_dport = tmp;
	}
	tHdr.th_seq = seq;
	tHdr.th_ack = ack;
	tHdr.th_x2 = 0;
	tHdr.th_off = sizeof (struct tcphdr) >> 2;
	tHdr.th_flags = flags;
	tHdr.th_win = win;
	tHdr.th_urp = 0;
	if ( msgConstructContig(&m, s ? xMyPath(s) : xMyPath(tcp),
				0, TCP_STACK_LEN) == XK_FAILURE ) {
	    xTraceP0(tcp, TR_SOFT_ERRORS, "couldn't allocate respose message");
	    return;
	}
	
	store.m = &m;
	store.h = pHdr;
	/*
	 * msgPush must succeed because we allocated enough stack
	 * space above
	 */
	ret = msgPush(&m, tcpHdrStore, &tHdr, sizeof(tHdr), &store);
	xAssert(ret == XK_SUCCESS);
	if (s) {
	    xPush(s, &m);
	} else {
	    Part_s p[2];
	    
	    partInit(p, 2);
	    partPush(p[0], &pHdr->dst, sizeof(IPhost));
	    partPush(p[1], &pHdr->src, sizeof(IPhost));
	    s = xOpen(tcp, tcp, xGetDown(tcp, 0), p, xMyPath(tcp));
	    if ( xIsSession(s) ) {
		xPush(s, &m);
		xClose(s);
	    } else {
		xError("tcp_respond could not open lower session!");
	    }
	}
	msgDestroy(&m);
}


/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.  Assumes that the ip control block and associated
 * tcp session already exist.
 */
struct tcpcb *
tcp_newtcpcb(inp, alloc)
    	struct inpcb *inp;
    	Allocator alloc;
{
	register struct tcpcb *tp;

	if ( ! (tp = (struct tcpcb *)allocGet(alloc, sizeof *tp)) ) {
	    return 0;
	}
	bzero((char*)tp, sizeof *tp);
	tp->seg_next = tp->seg_prev = (struct reass *)tp;
	tp->t_maxseg = TCP_MSS;
	tp->t_flags = 0;		/* sends options! */
	tp->t_inpcb = inp;
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = TCPTV_SRTTDFLT << 2;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = sbspace(sototcpst(inp->inp_session)->snd);
	tp->snd_ssthresh = 65535;		/* XXX */
	inp->inp_ppcb = (caddr_t)tp;
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, errnum)
	register struct tcpcb *tp;
	int errnum;
{
	XObj so = tp->t_inpcb->inp_session;
	PSTATE	*ps = (PSTATE *)xMyProtl(so)->state;

	xTrace2(tcpp, 2, "tcp_drop: tcpcb %X s %X", tp, so);
	if (TCPS_HAVERCVDSYN(tp->t_state)) {
	  tp->t_state = TCPS_CLOSED;
	  (void) tcp_output(tp);
	  ps->tcpstat->tcps_drops++;
	} else {
	  ps->tcpstat->tcps_conndrops++;
	}
	socantrcvmore(so);
#if 0
	return (tcp_destroy(tp));
#else
	return 0;
#endif
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
struct tcpcb *
tcp_destroy(tp)
	register struct tcpcb *tp;
{
	register struct reass *this, *next;
	struct inpcb *inp;
	XObj so;
	PSTATE *pstate;

	xTrace1(tcpp, 1, "tcp_destroy: tcpcb %X", tp);
	inp = tp->t_inpcb;
	xTrace1(tcpp, 3, "tcp_destroy: inpcb %X", inp);
	so = inp->inp_session;
	pstate = (PSTATE *)so->myprotl->state;
	this = tp->seg_next;
	while (this != (struct reass *)tp) {
		next = this->next;
		msgDestroy(&this->m);
		remque(this);
		allocFree(this);
		this = next;
	}
	if (tp->t_template) {
	    tcpReleasePort(pstate->portstate, tp->t_template->ti_sport);
	    allocFree(tp->t_template);
	}
	allocFree((char *)tp);
	inp->inp_ppcb = 0;
	/*
	 * This used to be a soisdisconnected, but really needs to delete
	 * all of the state.  
	 */
	delete_tcpstate(sototcpst(so));
	so->state = 0;
	mapRemoveBinding(pstate->activeMap, so->binding);
	{
		int i;

		for (i=0; i < so->numdown; i++) {
			xClose(xGetDown(so, i));
		}
	}
	xDestroy(so);

	in_pcbdetach(inp);
	pstate->tcpstat->tcps_closed++;
	return ((struct tcpcb *)0);
}


/*
 * Notify a tcp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
void
tcp_notify(inp)
	register struct inpcb *inp;
{

	sorwakeup(inp->inp_session);
	sowwakeup(inp->inp_session);
}


#if BSD<43
/* XXX fake routine */
tcp_abort(inp)
	struct inpcb *inp;
{
	return;
}
#endif

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
void
tcp_quench(inp)
	struct inpcb *inp;
{
	struct tcpcb *tp = intotcpcb(inp);

	if (tp)
		tp->snd_cwnd = tp->t_maxseg;
}
