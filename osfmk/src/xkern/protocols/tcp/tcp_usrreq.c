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
 * tcp_usrreq.c
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
 *	@(#)tcp_usrreq.c	7.6 (Berkeley) 12/7/87
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp_usrreq.c,v
 * Revision 1.8.1.4.1.2  1994/09/07  04:18:34  menze
 * OSF modifications
 *
 * Revision 1.8.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.8.1.3  1994/07/22  20:08:17  menze
 * Uses Path-based message interface and UPI
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/tcp/tcp_internal.h>
#include <xkern/protocols/tcp/tcp_fsm.h>
#include <xkern/protocols/tcp/tcp_seq.h>
#include <xkern/protocols/tcp/tcp_timer.h>
#include <xkern/protocols/tcp/tcp_var.h>
#include <xkern/protocols/tcp/tcpip.h>
#include <xkern/protocols/tcp/tcp_debug.h>

/*
 * TCP protocol interface to socket abstraction.
 */
extern	char *tcpstates[];
int	tcpsenderrors;


static void
copyBytes( void *hdr, char *des, long len, void *arg )
{
    bcopy(hdr, des, len);
}


/*
 * Process a TCP user request for TCP tb.  If this is a send request
 * then m is the mbuf chain of send data.  If this is a timer expiration
 * (called from the software clock routine), then timertype tells which timer.
 */
/*ARGSUSED*/
int
tcp_usrreq(so, req, m, nam)
	XObj so;
	int req;
	Msg m, nam;
{
	register struct inpcb *inp;
	register struct tcpcb *tp = 0;
	register struct tcpstate *tcpst = (struct tcpstate*)so->state;
	/* int s; */
	int error = 0;
	int ostate;
	xkern_return_t	xkr;

	xTrace0(tcpp, 7, "tcp_usrreq");
	/* 
	 * The original code protected this routine with splnet().  If
	 * this thread blocks, there could be a problem.
	 */
 	/* s = splnet(); */
	inp = sotoinpcb(so);
	/*
	 * When a TCP is attached to a socket, then there will be
	 * a (struct inpcb) pointed at by the socket, and this
	 * structure will point at a subsidary (struct tcpcb).
	 */
	if (inp == 0 && req != PRU_ATTACH) {
		/* splx(s); */
		return (EINVAL);		/* XXX */
	}
	if (inp) {
		tp = intotcpcb(inp);
		/* WHAT IF TP IS 0? */
#ifdef KPROF
		tcp_acounts[tp->t_state][req]++;
#endif
		ostate = tp->t_state;
	} else
		ostate = 0;
	switch (req) {

	/*
	 * TCP attaches to socket via PRU_ATTACH, reserving space,
	 * and an internet control block.
	 */
	case PRU_ATTACH:
		if (inp) {
			error = EISCONN;
			break;
		}
		error = tcp_attach(so);
		if (error)
			break;
		tp = sototcpcb(so);
		break;

	/*
	 * PRU_DETACH detaches the TCP protocol from the socket.
	 * If the protocol state is non-embryonic, then can't
	 * do this directly: have to initiate a PRU_DISCONNECT,
	 * which may finish later; embryonic TCB's can just
	 * be discarded here.
	 */
	case PRU_DETACH:
		if (tp->t_state > TCPS_LISTEN)
			tp = tcp_disconnect(tp);
		else
			tp = tcp_destroy(tp);
		break;

	/*
	 * Initiate connection to peer.
	 * Create a template for use in transmissions on this connection.
	 * Enter SYN_SENT state, and mark socket as connecting.
	 * Start keep-alive timer, and seed output sequence space.
	 * Send initial segment on connection.
	 */
	case PRU_CONNECT:
	    {
		struct	tcpstat *tcpstat;

		tcpstat = ((PSTATE *)xMyProtl(so)->state)->tcpstat;
		xTrace0(tcpp, 7, "usrreq: PRU_CONNECT");
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			in_pcbdisconnect(inp);
			error = ENOBUFS;
			break;
		}
		soisconnecting(so);
		tcpstat->tcps_connattempt++;
		tp->t_state = TCPS_SYN_SENT;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;
		tp->iss = tcp_iss; tcp_iss += TCP_ISSINCR/2;
		tcp_sendseqinit(tp);
		error = tcp_output(tp);
		break;
	    }

	/*
	 * Create a TCP connection between two sockets.
	 */
	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	/*
	 * Initiate disconnect from peer.
	 * If connection never passed embryonic stage, just drop;
	 * else if don't need to let data drain, then can just drop anyways,
	 * else have to begin TCP shutdown process: mark socket disconnecting,
	 * drain unread data, state switch to reflect user close, and
	 * send segment (e.g. FIN) to peer.  Socket will be really disconnected
	 * when peer sends FIN and acks ours.
	 *
	 * SHOULD IMPLEMENT LATER PRU_CONNECT VIA REALLOC TCPCB.
	 */
	case PRU_DISCONNECT:
		tp = tcp_disconnect(tp);
		break;

	/*
	 * Accept a connection.  Essentially all the work is
	 * done at higher levels; just return the address
	 * of the peer, storing through addr.
	 */
	case PRU_ACCEPT: {
#if 0
		struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

		nam->m_len = sizeof (struct sockaddr_in);
		sin->sin_family = AF_INET;
		sin->sin_port = inp->inp_fport;
		sin->sin_addr = inp->inp_faddr;
#endif
		break;
		}

	/*
	 * After a receive, possibly send window update to peer.
	 */
	case PRU_RCVD:
		(void) tcp_output(tp);
		break;

	/*
	 * Do a send by putting data in output queue and updating urgent
	 * marker if URG set.  Possibly send more data.
	 */
	case PRU_RCVOOB:
#if 0
		if ((so->so_oobmark == 0 &&
		    (so->so_state & SS_RCVATMARK) == 0) ||
#ifdef SO_OOBINLINE
		    so->so_options & SO_OOBINLINE ||
#endif
		    tp->t_oobflags & TCPOOB_HADDATA) {
			error = EINVAL;
			break;
		}
#endif
		if ((tp->t_oobflags & TCPOOB_HAVEDATA) == 0) {
			error = EWOULDBLOCK;
			break;
		}
		msgDestroy(m);
		xkr = msgConstructContig(m, xMyPath(so), 0, 1);
		if ( xkr == XK_SUCCESS ) {
		    xAssert(msgPush(m, copyBytes, &tp->t_iobc, 1, 0) == XK_SUCCESS);
		    tp->t_oobflags ^= (TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		break;

	case PRU_SENDOOB:
		/*
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section.
		 * Otherwise, snd_up should be one lower.
		 */
		while ( sbappend(tcpst->snd, m, so) != XK_SUCCESS ) {
		    if ( tcpst->flags & TCPST_NBIO ) {
			return -1;
		    }
		    xTraceP0(so, TR_SOFT_ERRORS, "couldn't allocate sb, waiting");
		    tcpst->flags |= TCPST_MEMWAIT;
		    tcpSemWait(&tcpst->waiting);
		    tcpst->flags &= ~TCPST_MEMWAIT;
		    if ( tcpst->closed ) {
			return 0;
		    }
		}
		tp->snd_up = tp->snd_una + sblength(tcpst->snd);
		tp->t_force = 1;
		error = tcp_output(tp);
		tp->t_force = 0;
		break;

	/*
	 * TCP slow timer went off
	 */
	case PRU_SLOWTIMO:
		if ( tcpst->flags & TCPST_MEMWAIT ) {
		    xAssert(tcpst->waiting.waitCount);
		    tcpSemSignal(&tcpst->waiting);
		}
		tp = tcp_timers(tp, (int)nam);
		req |= (int)nam << 8;		/* for debug's sake */
		break;

	default:
		Kabort("tcp_usrreq");
	}
	xIfTrace(tcpp, 3) {
		tcp_trace(TA_USER, ostate, tp, (struct tcpiphdr *)0, req);
	}
	/* splx(s); */
	return (error);
}

int	tcp_sendspace = TCPRCVWIN;
int	tcp_recvspace = TCPRCVWIN;
/*
 * Attach TCP protocol to socket, allocating
 * internet protocol control block, tcp control block,
 * bufer space, and entering LISTEN state if to accept connections.
 */
int
tcp_attach(so)
	XObj so;
{
	register struct tcpcb *tp;
	struct inpcb *inp;
	int error;

	error = soreserve(so, tcp_sendspace, tcp_recvspace);
	if (error)
		return (error);
	error = in_pcballoc(so);
	if (error)
		return (error);
	inp = sotoinpcb(so);
	tp = tcp_newtcpcb(inp, pathGetAlloc(so->path, MD_ALLOC));
	if (tp == 0) {
		in_pcbdetach(inp);
		return (ENOBUFS);
	}
	tp->t_state = TCPS_CLOSED;
	return (0);
}

/*
 * Initiate (or continue) disconnect.
 * If embryonic state, just send reset (once).
 * If in ``let data drain'' option and linger null, just drop.
 * Otherwise (hard), mark socket disconnecting and drop
 * current input data; switch states based on user close, and
 * send segment to peer (with FIN).
 */
struct tcpcb *
tcp_disconnect(tp)
	register struct tcpcb *tp;
{
	XObj so = tp->t_inpcb->inp_session;
	xTrace1(tcpp, 3, "tcp_disconnect: tp %X", tp);
	if (tp->t_state < TCPS_ESTABLISHED)
		tp = tcp_destroy(tp);
	else {
		soisdisconnecting(so);
		tp = tcp_usrclosed(tp);
		if (tp)
			(void) tcp_output(tp);
	}
	return (tp);
}

/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
struct tcpcb *
tcp_usrclosed(tp)
	register struct tcpcb *tp;
{

	switch (tp->t_state) {

	case TCPS_CLOSED:
	case TCPS_LISTEN:
	case TCPS_SYN_SENT:
		tp->t_state = TCPS_CLOSED;
		tp = tcp_destroy(tp);
		break;

	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		tp->t_state = TCPS_FIN_WAIT_1;
		break;

	case TCPS_CLOSE_WAIT:
		tp->t_state = TCPS_LAST_ACK;
		break;
	}
	if (tp && tp->t_state >= TCPS_FIN_WAIT_2)
		soisdisconnected(tp->t_inpcb->inp_session);
	return (tp);
}
