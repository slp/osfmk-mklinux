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
 * tcp_debug.c
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
 *	@(#)tcp_debug.c	7.2 (Berkeley) 12/7/87
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.1.9.1 $
 * $Date: 1995/02/23 15:37:46 $
 */

#include <xkern/protocols/tcp/tcp_internal.h>
#define TCPSTATES
#include <xkern/protocols/tcp/tcp_fsm.h>
#include <xkern/protocols/tcp/tcp_seq.h>
#define	TCPTIMERS
#include <xkern/protocols/tcp/tcp_timer.h>
#include <xkern/protocols/tcp/tcp_var.h>
#include <xkern/protocols/tcp/tcpip.h>
#define	TANAMES
#include <xkern/protocols/tcp/tcp_debug.h>

int	tcpconsdebug = 1;
int	tracetcpp = 0;
struct	tcp_debug tcp_debug[TCP_NDEBUG];
int	tcp_debx = 0;

/*
 * Tcp debug routines
 */
void
tcp_trace(act, ostate, tp, ti, req)
	int act, ostate;
	struct tcpcb *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int len, flags;
	struct tcp_debug *td = &tcp_debug[tcp_debx++];

	if (tcp_debx == TCP_NDEBUG)
		tcp_debx = 0;
/*	td->td_time = iptime(); */
	td->td_act = act;
	td->td_ostate = ostate;
	td->td_tcb = (caddr_t)tp;
	if (tp)
		td->td_cb = *tp;
	else
		bzero((caddr_t)&td->td_cb, sizeof (*tp));
	if (ti)
		td->td_ti = *ti;
	else
		bzero((caddr_t)&td->td_ti, sizeof (*ti));
	td->td_req = req;
	if (tcpconsdebug == 0)
		return;
	if (tp)
		printf("%x %s:", tp, tcpstates[ostate]);
	else
		printf("???????? ");
	printf("%s ", tanames[act]);
	switch (act) {

	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (ti == 0)
			break;
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs((u_short)len);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			printf("[%x..%x)", seq, seq+len);
		else
			printf("%x", seq);
		printf("@%x, urp=%x", ack, ti->ti_urp);
		flags = ti->ti_flags;
		if (flags) {
#ifndef lint
			char *cp = "<";
#define pf(f) { if (ti->ti_flags&TH_##f) { printf("%s%s", cp, "f"); cp = ","; } }
			pf(SYN); pf(ACK); pf(FIN); pf(RST); pf(PUSH); pf(URG);
#endif
			printf(">");
		}
		break;

	case TA_USER:
		printf("%s", prurequests[req&0xff]);
		if ((req & 0xff) == PRU_SLOWTIMO)
			printf("<%s>", tcptimers[req>>8]);
		break;
	}
	if (tp)
		printf(" -> %s", tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	printf("\n");
	if (tp == 0)
		return;
	if (tracetcpp > 3) {
	  printf("\trcv_(nxt,wnd,up) (%x,%x,%x) snd_(una,nxt,max) (%x,%x,%x)\n",
		 tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up, tp->snd_una, tp->snd_nxt,
		 tp->snd_max);
	  printf("\tsnd_(wl1,wl2,wnd) (%x,%x,%x)\n",
		 tp->snd_wl1, tp->snd_wl2, tp->snd_wnd);
	}
}


char *
tcpFlagStr(f)
    int f;
{
    static char buf[80];

    buf[0] = 0;
    if ( f & TH_FIN ) {
	(void)strcat( buf, "FIN " );
    }
    if ( f & TH_SYN ) {
	(void)strcat( buf, "SYN " );
    }
    if ( f & TH_RST ) {
	(void)strcat( buf, "RST " );
    }
    if ( f & TH_PUSH ) {
	(void)strcat( buf, "PUSH " );
    }
    if ( f & TH_ACK ) {
	(void)strcat( buf, "ACK " );
    }
    if ( f & TH_URG ) {
	(void)strcat( buf, "URG " );
    }
    return buf;
}
