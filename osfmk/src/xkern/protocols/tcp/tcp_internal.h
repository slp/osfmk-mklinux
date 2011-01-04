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
 * tcp_internal.h,v
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
 *	@(#)tcp.h	7.3 (Berkeley) 12/7/87
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp_internal.h,v
 * Revision 1.36.1.4.1.2  1994/09/07  04:18:35  menze
 * OSF modifications
 *
 * Revision 1.36.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.36.1.3  1994/07/22  20:08:16  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.36.1.2  1994/05/10  17:05:25  hkaram
 * Deleted the declaration of tcp_reass
 *
 * Revision 1.36.1.1  1994/04/20  22:08:36  hkaram
 * Uses allocator-based message interface
 *
 * Revision 1.36  1993/12/16  01:41:38  menze
 * fixed '#else' comment
 *
 */

/**************************************************************************

tcp_internal.h

**************************************************************************/
#include <xkern/include/xkernel.h>
#include <xkern/protocols/tcp/sb.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/tcp.h>
#include <xkern/protocols/tcp/tcp_port.h>

typedef u_long 		n_time;
#undef KPROF
#define PR_SLOWHZ	2
#define PR_FASTHZ	5
/* #define KERNEL */
#define BSD 43
#define TCP_COMPAT_42
#ifndef MIN
#define MIN(A,B) ((A) <= (B) ? (A) : (B))
#endif
#ifndef MAX
#define MAX(A,B) ((A) >= (B) ? (A) : (B))
#endif

typedef struct {
    xkSemaphore	s;
    int		waitCount;
} TcpSem;

struct tcpstate {
    u_int	flags;		/* see below */
    struct inpcb *inpcb;
    struct tcpcb *tcpcb;
    TcpSem	waiting;
    TcpSem	lock;
    XObj	hlpType;	/* for passively opened sessions */
    struct sb	*snd;
    int		rcv_space;	/* amount of space in the receiver's buffer */
    int		rcv_hiwat;	/* size of receiver's buffer */
    int		closed;
};

#define TCPST_NBIO		0x0001	/* non-blocking i/o operations */
#define TCPST_OOBINLINE		0x0002	/* inline urgent data */
#define TCPST_PUSH_ALWAYS	0x0004	/* assume TCP-push after every xPush */
#define TCPST_RCV_ACK_ALWAYS	0x0008	/* assume infinite receive buffer */
#define TCPST_MEMWAIT		0x0010	

#define sotoinpcb(S) (((struct tcpstate *)((S)->state))->inpcb)
#define sototcpst(S) ((struct tcpstate *)(S)->state)
#define tcpcbtoso(tp) (tp->t_inpcb->inp_session)
extern int tracetcpp;

/*
 * The arguments to usrreq are:
 *	(*protosw[].pr_usrreq)(up, req, m, nam, opt);
 * where up is a (struct socket *), req is one of these requests,
 * m is a optional mbuf chain containing a message,
 * nam is an optional mbuf chain containing an address,
 * and opt is a pointer to a socketopt structure or nil.
 * The protocol is responsible for disposal of the mbuf chain m,
 * the caller is responsible for any space held by nam and opt.
 * A non-zero return from usrreq gives an
 * UNIX error number which should be passed to higher level software.
 */
#define	PRU_ATTACH		0	/* attach protocol to up */
#define	PRU_DETACH		1	/* detach protocol from up */
#define	PRU_BIND		2	/* bind socket to address */
#define	PRU_LISTEN		3	/* listen for connection */
#define	PRU_CONNECT		4	/* establish connection to peer */
#define	PRU_ACCEPT		5	/* accept connection from peer */
#define	PRU_DISCONNECT		6	/* disconnect from peer */
#define	PRU_SHUTDOWN		7	/* won't send any more data */
#define	PRU_RCVD		8	/* have taken data; more room now */
#define	PRU_SEND		9	/* send this data */
#define	PRU_ABORT		10	/* abort (fast DISCONNECT, DETATCH) */
#define	PRU_CONTROL		11	/* control operations on protocol */
#define	PRU_SENSE		12	/* return status into m */
#define	PRU_RCVOOB		13	/* retrieve out of band data */
#define	PRU_SENDOOB		14	/* send out of band data */
#define	PRU_SOCKADDR		15	/* fetch socket's address */
#define	PRU_PEERADDR		16	/* fetch peer's address */
#define	PRU_CONNECT2		17	/* connect two sockets */
/* begin for protocols internal use */
#define	PRU_FASTTIMO		18	/* 200ms timeout */
#define	PRU_SLOWTIMO		19	/* 500ms timeout */
#define	PRU_PROTORCV		20	/* receive from below */
#define	PRU_PROTOSEND		21	/* send to below */

#if	!defined(KERNEL) || MACH_VMTP
#define PRU_INVOKE		22	/* Invoke a message transaction */
#define PRU_RECVREQ		23	/* Receive a request */
#define PRU_REPLY		24	/* Send a reply */
#define PRU_FORWARD		25	/* Forward a request */
#define PRU_PROBE		26	/* Probe an entity */
#define PRU_GETREPLY		27 	/* Get next reply (multicast) */
#define	PRU_NREQ		27
#else	/* !defined(KERNEL) || MACH_VMTP */
#define PRU_NREQ		21	
#endif	/* !defined(KERNEL) || MACH_VMTP */


extern char *prurequests[];

/*
 * Subset of the inpcb defined in /usr/include/netinet/in_pcb.h
 */
#ifdef	MACH_KERNEL
struct in_addr
	{
		u_long s_addr;
	} ;
#endif  /* MACH_KERNEL */

struct inpcb {
	struct	inpcb *inp_next,*inp_prev;
	struct	inpcb *inp_head;
	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	struct in_addr inp_laddr, inp_raddr;
	u_short	inp_lport, inp_rport;
	XObj	inp_session;
};

#define RCV 0
#define SND 1

#ifndef	MACH_KERNEL
#include <sys/errno.h>
#else
#define	EINVAL		22		/* Invalid argument */
#define	EWOULDBLOCK	35		/* Operation would block */
#define	EOPNOTSUPP	45		/* Operation not supported on socket */
#define ECONNRESET	54
#define ENOBUFS		55
#define	EISCONN		56		/* Socket is already connected */
#define ETIMEDOUT	60
#define ECONNREFUSED	61
#endif  /* MACH_KERNEL */

typedef	u_long	tcp_seq;
/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct tcphdr {
	u_short	th_sport;		/* source port */
	u_short	th_dport;		/* destination port */
	tcp_seq	th_seq;			/* sequence number */
	tcp_seq	th_ack;			/* acknowledgement number */
#if BYTE_ORDER == LITTLE_ENDIAN
	u_int	th_x2:4,		/* (unused) */
		th_off:4;		/* data offset */
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
#if BYTE_ORDER == BIG_ENDIAN
	u_int	th_off:4,		/* data offset */
		th_x2:4;		/* (unused) */
#endif /* BYTE_ORDER == BIG_ENDIAN */
	u_char	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
	u_short	th_win;			/* window */
	u_short	th_sum;			/* checksum */
	u_short	th_urp;			/* urgent pointer */
};

#define	TCPOPT_EOL	0
#define	TCPOPT_NOP	1
#define	TCPOPT_MAXSEG	2

/*
 * Default maximum segment size for TCP.
 * With an IP MSS of 576, this is 536,
 * but 512 is probably more convenient.
 */
#ifdef	lint
#define	TCP_MSS	536
#else
#ifndef IP_MSS
#define	IP_MSS	576
#endif
#define	TCP_MSS	MIN(512, IP_MSS - sizeof (struct tcpiphdr))
#endif

/*
 * User-settable options (used with setsockopt).
 */
#define	TCP_NODELAY	0x01	/* don't delay send to coalesce packets */
#define	TCP_MAXSEG	0x02	/* set maximum segment size */


/*
 * X-Kernel defines
 */

/*====================  #8 10 lines added 070390 cliff ===================*/

typedef struct pstate  {
    IPhost	myHost;
    Map     	activeMap;
    Map      	passiveMap;
    void	*portstate;
    struct tcpstat	*tcpstat;	/* tcp statistics */
    struct inpcb	*tcb;		/* head of queue of active tcpcb's */
}  PSTATE;


typedef TCPport  PassiveId;

typedef struct {
      unsigned short localport;
      unsigned short remoteport;
      IPhost         remoteaddr;
} ActiveId;

typedef struct {
	IPpseudoHdr	*h;
	Msg		m;
} hdrStore_t;

#define FROM_ENABLE	-1
#define TCPRCVWIN	(TCP_BUFFER_SPACE)

#define TCP_STACK_LEN	100

extern char *tcpstates[];

/*
 * in_hacks.c
 */
int	in_pcballoc( XObj );
void	in_pcbdisconnect( struct inpcb * );
void 	in_pcbdetach( struct inpcb * );
boolean_t	in_broadcast( struct in_addr );
u_short	in_cksum( Msg, u_short *, int len );

/*
 * tcp_subr.c
 */
struct tcpiphdr *	tcp_template( struct tcpcb * );
struct tcpcb *	tcp_newtcpcb( struct inpcb *, Allocator );
struct tcpcb *	tcp_drop( struct tcpcb *, int errnum );
struct tcpcb * 	tcp_destroy( struct tcpcb * );
void	tcp_notify( struct inpcb * );
void	tcp_quench( struct inpcb * );
void	tcp_respond( struct tcpcb *, struct tcphdr *, IPpseudoHdr *,
			     tcp_seq ack, tcp_seq seq, int flags,
		             XObj tcpProtl );
 
/*
 * tcp_hdr.c
 */
void 	tcpHdrStore(void *, char *, long, void *);
void 	tcpOptionsStore(void *, char *, long, void *);
long	tcpOptionsLoad(void *, char *, long, void *);
long 	tcpHdrLoad(void *, char *, long, void *);

/*
 * tcp_timer.c
 */
void	tcp_fasttimo( Event, void * );
void	tcp_slowtimo( Event, void * );
void	tcp_canceltimers( struct tcpcb * );
struct tcpcb *	tcp_timers( struct tcpcb *, int );

/*
 * tcp_x.c
 */
void	delete_tcpstate( struct tcpstate * );
void	sorwakeup( XObj );
void	sowwakeup( XObj );
void	soabort( XObj );
void	socantsendmore( XObj );
void	socantrcvmore( XObj );
void	sohasoutofband( XObj, u_int );
void	soisdisconnected( XObj );
void	soisdisconnecting( XObj );
void	soisconnected( XObj );
void	soisconnecting( XObj );
int	soreserve( XObj, int, int );
XObj	sonewconn( XObj, XObj, XObj, IPhost *, IPhost *, int, int, Path );
void	tcpSemWait( TcpSem * );
void	tcpSemSignal( TcpSem * );
void	tcpSemInit( TcpSem *, int );
void	tcpVAll( TcpSem * );

/* 
 * tcp_output.c
 */
int	tcp_output( struct tcpcb * );
void	tcp_setpersist( struct tcpcb * );

/* 
 * tcp_usrreq.c
 */
int	tcp_attach( XObj );
int	tcp_usrreq( XObj, int, Msg, Msg );
struct tcpcb *tcp_disconnect(struct tcpcb * );
struct tcpcb *tcp_usrclosed( struct tcpcb * );

/* 
 * tcp_input.c
 */
void	print_reass( struct tcpcb *, char * );
xkern_return_t	tcp_input( XObj, XObj, Msg );
int	tcp_mss( struct tcpcb * );
xkern_return_t	tcpPop( XObj, XObj, Msg, void * );

/* 
 * tcp_debug.c
 */
void	tcp_trace(int, int, struct tcpcb *, struct tcpiphdr *, int );
char *	tcpFlagStr( int );

#include <xkern/protocols/tcp/insque.h>

