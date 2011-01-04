/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		AF_INET protocol family socket handler.
 *
 * Version:	@(#)af_inet.c	(from sock.c) 1.0.17	06/02/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Florian La Roche, <flla@stud.uni-sb.de>
 *		Alan Cox, <A.Cox@swansea.ac.uk>
 *
 * Changes (see also sock.c)
 *
 *		A.N.Kuznetsov	:	Socket death error in accept().
 *		John Richardson :	Fix non blocking error in connect()
 *					so sockets that fail to connect
 *					don't return -EINPROGRESS.
 *		Alan Cox	:	Asynchronous I/O support
 *		Alan Cox	:	Keep correct socket pointer on sock structures
 *					when accept() ed
 *		Alan Cox	:	Semantics of SO_LINGER aren't state moved
 *					to close when you look carefully. With
 *					this fixed and the accept bug fixed 
 *					some RPC stuff seems happier.
 *		Niibe Yutaka	:	4.4BSD style write async I/O
 *		Alan Cox, 
 *		Tony Gale 	:	Fixed reuse semantics.
 *		Alan Cox	:	bind() shouldn't abort existing but dead
 *					sockets. Stops FTP netin:.. I hope.
 *		Alan Cox	:	bind() works correctly for RAW sockets. Note
 *					that FreeBSD at least was broken in this respect
 *					so be careful with compatibility tests...
 *		Alan Cox	:	routing cache support
 *		Alan Cox	:	memzero the socket structure for compactness.
 *		Matt Day	:	nonblock connect error handler
 *		Alan Cox	:	Allow large numbers of pending sockets
 *					(eg for big web sites), but only if
 *					specifically application requested.
 *		Alan Cox	:	New buffering throughout IP. Used dumbly.
 *		Alan Cox	:	New buffering now used smartly.
 *		Alan Cox	:	BSD rather than common sense interpretation of
 *					listen.
 *		Germano Caronni	:	Assorted small races.
 *		Alan Cox	:	sendmsg/recvmsg basic support.
 *		Alan Cox	:	Only sendmsg/recvmsg now supported.
 *		Alan Cox	:	Locked down bind (see security list).
 *		Alan Cox	:	Loosened bind a little.
 *		Mike McLagan	:	ADD/DEL DLCI Ioctls
 *	Willy Konynenberg	:	Transparent proxying support.
 *		David S. Miller	:	New socket lookup architecture for ISS.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/sockios.h>
#include <linux/net.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>

#include <asm/segment.h>
#include <asm/system.h>

#include <linux/inet.h>
#include <linux/netdevice.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/arp.h>
#include <net/rarp.h>
#include <net/route.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/raw.h>
#include <net/icmp.h>
#include <linux/ip_fw.h>
#ifdef CONFIG_IP_MASQUERADE
#include <net/ip_masq.h>
#endif
#ifdef CONFIG_IP_ALIAS
#include <net/ip_alias.h>
#endif
#ifdef CONFIG_BRIDGE
#include <net/br.h>
#endif
#ifdef CONFIG_KERNELD
#include <linux/kerneld.h>
#endif
#ifdef CONFIG_NET_RADIO
#include <linux/wireless.h>
#endif	/* CONFIG_NET_RADIO */

#define min(a,b)	((a)<(b)?(a):(b))

extern struct proto packet_prot;
extern int raw_get_info(char *, char **, off_t, int, int);
extern int snmp_get_info(char *, char **, off_t, int, int);
extern int afinet_get_info(char *, char **, off_t, int, int);
extern int tcp_get_info(char *, char **, off_t, int, int);
extern int udp_get_info(char *, char **, off_t, int, int);

#ifdef CONFIG_DLCI
extern int dlci_ioctl(unsigned int, void*);
#endif

#ifdef CONFIG_DLCI_MODULE
int (*dlci_ioctl_hook)(unsigned int, void *) = NULL;
#endif

int (*rarp_ioctl_hook)(unsigned int,void*) = NULL;

/*
 *	Destroy an AF_INET socket
 */
 
static __inline__ void kill_sk_queues(struct sock *sk)
{
	struct sk_buff *skb;

	while((skb = tcp_dequeue_partial(sk)) != NULL)
		kfree_skb(skb, FREE_WRITE);

	/* Next, the write queue. */
	while((skb = skb_dequeue(&sk->write_queue)) != NULL)
		kfree_skb(skb, FREE_WRITE);

	/* Then, the receive queue. */
	while((skb = skb_dequeue(&sk->receive_queue)) != NULL) {
		/* This will take care of closing sockets that were
		 * listening and didn't accept everything.
		 */
		if (skb->sk != NULL && skb->sk != sk)
			skb->sk->prot->close(skb->sk, 0);
		kfree_skb(skb, FREE_READ);
	}

	/*
	 *	Now we need to clean up the send head. 
	 */
	 
	cli();
	for(skb = sk->send_head; skb != NULL; )
	{
		struct sk_buff *skb2;

		/*
		 * We need to remove skb from the transmit queue,
		 * or maybe the arp queue.
		 */
		if (skb->next  && skb->prev) 
		{
			IS_SKB(skb);
			skb_unlink(skb);
		}
		skb->dev = NULL;
		skb2 = skb->link3;
		kfree_skb(skb, FREE_WRITE);
		skb = skb2;
	}
	sk->send_head = NULL;
	sk->send_tail = NULL;
	sk->send_next = NULL;
	sti();

  	/* Finally, the backlog. */
  	while((skb=skb_dequeue(&sk->back_log)) != NULL) {
		/* skb->sk = NULL; */
		kfree_skb(skb, FREE_READ);
	}
}

static __inline__ void kill_sk_now(struct sock *sk)
{
	/* No longer exists. */
	del_from_prot_sklist(sk);

	/* This is gross, but needed for SOCK_PACKET -DaveM */
	if(sk->prot->unhash)
		sk->prot->unhash(sk);

	if(sk->opt)
		kfree(sk->opt);
	ip_rt_put(sk->ip_route_cache);
	sk_free(sk);
}

static __inline__ void kill_sk_later(struct sock *sk)
{
	/* this should never happen. */
	/* actually it can if an ack has just been sent. */
	/* 
	 * It's more normal than that...
	 * It can happen because a skb is still in the device queues
	 * [PR]
	 */
		  
	NETDEBUG(printk("Socket destroy delayed (r=%d w=%d)\n",
			sk->rmem_alloc, sk->wmem_alloc));

	sk->destroy = 1;
	sk->ack_backlog = 0;
	release_sock(sk);
	reset_timer(sk, TIME_DESTROY, SOCK_DESTROY_TIME);
}

void destroy_sock(struct sock *sk)
{
	lock_sock(sk);			/* just to be safe. */

  	/*
  	 *	Now we can no longer get new packets or once the
  	 *	timers are killed, send them.
  	 */
  	 
  	delete_timer(sk);
	del_timer(&sk->delack_timer);
	del_timer(&sk->retransmit_timer);
	
	kill_sk_queues(sk);

	/*
	 *	Now if it has a half accepted/ closed socket. 
	 */
	 
	if (sk->pair) 
	{
		sk->pair->prot->close(sk->pair, 0);
		sk->pair = NULL;
  	}

	/*
	 * Now if everything is gone we can free the socket
	 * structure, otherwise we need to keep it around until
	 * everything is gone.
	 */

	if (sk->rmem_alloc == 0 && sk->wmem_alloc == 0) 
		kill_sk_now(sk);
	else
		kill_sk_later(sk);
}

/*
 *	The routines beyond this point handle the behaviour of an AF_INET
 *	socket object. Mostly it punts to the subprotocols of IP to do
 *	the work.
 */
 
static int inet_fcntl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk;

	sk = (struct sock *) sock->data;

	switch(cmd) 
	{
		case F_SETOWN:
			/*
			 * This is a little restrictive, but it's the only
			 * way to make sure that you can't send a sigurg to
			 * another process.
			 */
			if (!suser() && current->pgrp != -arg &&
				current->pid != arg) return(-EPERM);
			sk->proc = arg;
			return(0);
		case F_GETOWN:
			return(sk->proc);
		default:
			return(-EINVAL);
	}
}

/*
 *	Set socket options on an inet socket.
 */
 
static int inet_setsockopt(struct socket *sock, int level, int optname,
		    char *optval, int optlen)
{
  	struct sock *sk = (struct sock *) sock->data;  
	if (level == SOL_SOCKET)
		return sock_setsockopt(sk,level,optname,optval,optlen);
	if (sk->prot->setsockopt==NULL)
		return(-EOPNOTSUPP);
	else
		return sk->prot->setsockopt(sk,level,optname,optval,optlen);
}

/*
 *	Get a socket option on an AF_INET socket.
 */

static int inet_getsockopt(struct socket *sock, int level, int optname,
		    char *optval, int *optlen)
{
  	struct sock *sk = (struct sock *) sock->data;  	
  	if (level == SOL_SOCKET) 
  		return sock_getsockopt(sk,level,optname,optval,optlen);
  	if(sk->prot->getsockopt==NULL)  	
  		return(-EOPNOTSUPP);
  	else
  		return sk->prot->getsockopt(sk,level,optname,optval,optlen);
}

/*
 *	Automatically bind an unbound socket.
 */

static int inet_autobind(struct sock *sk)
{
	/* We may need to bind the socket. */
	if (sk->num == 0) {
		sk->num = sk->prot->good_socknum();
		if (sk->num == 0) 
			return(-EAGAIN);
		sk->dummy_th.source = ntohs(sk->num);
		sk->prot->rehash(sk);
		add_to_prot_sklist(sk);
	}
	return 0;
}

/*
 *	Move a socket into listening state.
 */
 
static int inet_listen(struct socket *sock, int backlog)
{
	struct sock *sk = (struct sock *) sock->data;

	if(inet_autobind(sk) != 0)
		return -EAGAIN;

	/* We might as well re use these. */ 
	/*
	 * note that the backlog is "unsigned char", so truncate it
	 * somewhere. We might as well truncate it to what everybody
	 * else does..
	 * Now truncate to 128 not 5. 
	 *
	 * This was wrong, truncate both cases to SOMAXCONN. -DaveM
	 */
	if (((unsigned) backlog == 0) || ((unsigned) backlog > SOMAXCONN))
		backlog = SOMAXCONN;
	sk->max_ack_backlog = backlog;
	if (sk->state != TCP_LISTEN) {
		sk->ack_backlog = 0;
		sk->state = TCP_LISTEN;
		sk->prot->rehash(sk);
		add_to_prot_sklist(sk);
	}
	return(0);
}

/*
 *	Default callbacks for user INET sockets. These just wake up
 *	the user owning the socket.
 */

static void def_callback1(struct sock *sk)
{
	if(!sk->dead)
		wake_up_interruptible(sk->sleep);
}

static void def_callback2(struct sock *sk,int len)
{
	if(!sk->dead)
	{
		wake_up_interruptible(sk->sleep);
		sock_wake_async(sk->socket, 1);
	}
}

static void def_callback3(struct sock *sk)
{
	if(!sk->dead && sk->wmem_alloc*2 <= sk->sndbuf)
	{
		wake_up_interruptible(sk->sleep);
		sock_wake_async(sk->socket, 2);
	}
}

/*
 *	Create an inet socket.
 *
 *	FIXME: Gcc would generate much better code if we set the parameters
 *	up in in-memory structure order. Gcc68K even more so
 */

static int inet_create(struct socket *sock, int protocol)
{
	struct sock *sk;
	struct proto *prot;

	sk = sk_alloc(GFP_KERNEL);
	if (sk == NULL) 
		goto do_oom;
#if 0 /* sk_alloc() does this for us. -DaveM */
	memset(sk,0,sizeof(*sk));	/* Efficient way to set most fields to zero */
#endif
	/*
	 *	Note for tcp that also wiped the dummy_th block for us.
	 */
	if(sock->type == SOCK_STREAM || sock->type == SOCK_SEQPACKET) {
		if (protocol && protocol != IPPROTO_TCP) 
			goto free_and_noproto;
		protocol = IPPROTO_TCP;
		sk->no_check = TCP_NO_CHECK;
		prot = &tcp_prot;
	} else if(sock->type == SOCK_DGRAM) {
		if (protocol && protocol != IPPROTO_UDP) 
			goto free_and_noproto;
		protocol = IPPROTO_UDP;
		sk->no_check = UDP_NO_CHECK;
		prot=&udp_prot;
	} else if(sock->type == SOCK_RAW || sock->type == SOCK_PACKET) {
		if (!suser()) 
			goto free_and_badperm;
		if (!protocol) 
			goto free_and_noproto;
		prot = &raw_prot;
		prot = (sock->type == SOCK_RAW) ? &raw_prot : &packet_prot;
		sk->reuse = 1;
		sk->num = protocol;
	} else {
		goto free_and_badtype;
	}

	sk->socket = sock;
#ifdef CONFIG_TCP_NAGLE_OFF
	sk->nonagle = 1;
#endif  
	sk->type = sock->type;
	sk->protocol = protocol;
	sk->allocation = GFP_KERNEL;
	sk->sndbuf = SK_WMEM_MAX;
	sk->rcvbuf = SK_RMEM_MAX;
	sk->rto = TCP_TIMEOUT_INIT;		/*TCP_WRITE_TIME*/
	sk->cong_window = 1; /* start with only sending one packet at a time. */
	sk->ssthresh = 0x7fffffff;
	sk->priority = 1;
	sk->state = TCP_CLOSE;

	/* this is how many unacked bytes we will accept for this socket.  */
	sk->max_unacked = 2048; /* needs to be at most 2 full packets. */
	sk->delay_acks = 1;
	sk->max_ack_backlog = SOMAXCONN;
	skb_queue_head_init(&sk->write_queue);
	skb_queue_head_init(&sk->receive_queue);
	sk->mtu = 576;
	sk->prot = prot;
	sk->sleep = sock->wait;
	init_timer(&sk->timer);
	init_timer(&sk->delack_timer);
	init_timer(&sk->retransmit_timer);
	sk->timer.data = (unsigned long)sk;
	sk->timer.function = &net_timer;
	skb_queue_head_init(&sk->back_log);
	sock->data =(void *) sk;
	sk->ip_ttl=ip_statistics.IpDefaultTTL;
	if(sk->type==SOCK_RAW && protocol==IPPROTO_RAW)
		sk->ip_hdrincl=1;
	else
		sk->ip_hdrincl=0;
#ifdef CONFIG_IP_MULTICAST
	sk->ip_mc_loop=1;
	sk->ip_mc_ttl=1;
	*sk->ip_mc_name=0;
	sk->ip_mc_list=NULL;
#endif
	/*
	 *	Speed up by setting some standard state for the dummy_th
	 *	if TCP uses it (maybe move to tcp_init later)
	 */
  	
  	sk->dummy_th.ack=1;	
  	sk->dummy_th.doff=sizeof(struct tcphdr)>>2;
  	
	sk->state_change = def_callback1;
	sk->data_ready = def_callback2;
	sk->write_space = def_callback3;
	sk->error_report = def_callback1;

	if (sk->num) {
		/* It assumes that any protocol which allows
		 * the user to assign a number at socket
		 * creation time automatically
		 * shares.
		 */
		sk->dummy_th.source = ntohs(sk->num);

		/* This is gross, but needed for SOCK_PACKET -DaveM */
		if(sk->prot->hash)
			sk->prot->hash(sk);
		add_to_prot_sklist(sk);
	}

	if (sk->prot->init) {
		int err = sk->prot->init(sk);
		if (err != 0) {
			destroy_sock(sk);
			return(err);
		}
	}
	return(0);

free_and_badtype:
	sk_free(sk);
	return -ESOCKTNOSUPPORT;

free_and_badperm:
	sk_free(sk);
	return -EPERM;

free_and_noproto:
	sk_free(sk);
	return -EPROTONOSUPPORT;

do_oom:
	return -ENOBUFS;
}


/*
 *	Duplicate a socket.
 */
 
static int inet_dup(struct socket *newsock, struct socket *oldsock)
{
	return(inet_create(newsock,((struct sock *)(oldsock->data))->protocol));
}

/*
 *	The peer socket should always be NULL (or else). When we call this
 *	function we are destroying the object and from then on nobody
 *	should refer to it.
 */
 
static int inet_release(struct socket *sock, struct socket *peer)
{
	struct sock *sk = (struct sock *) sock->data;

	if (sk) {
		unsigned long timeout;

		sk->state_change(sk);

		/* Start closing the connection.  This may take a while. */

#ifdef CONFIG_IP_MULTICAST
		/* Applications forget to leave groups before exiting */
		ip_mc_drop_socket(sk);
#endif
		/*
		 * If linger is set, we don't return until the close
		 * is complete.  Otherwise we return immediately. The
		 * actually closing is done the same either way.
		 *
		 * If the close is due to the process exiting, we never
		 * linger..
		 */
		timeout = 0;
		if (sk->linger && !(current->flags & PF_EXITING)) {
			if (sk->lingertime)
				timeout = jiffies + HZ*sk->lingertime;
		}

		sock->data = NULL;
		sk->socket = NULL;

		sk->prot->close(sk, timeout);
	}
	return(0);
}


static int inet_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
	struct sockaddr_in *addr=(struct sockaddr_in *)uaddr;
	struct sock *sk=(struct sock *)sock->data;
	unsigned short snum;
	int chk_addr_ret;

	/* If the socket has its own bind function then use it. (RAW AND PACKET) */
	if(sk->prot->bind)
		return sk->prot->bind(sk, uaddr, addr_len);
		
	/* Check these errors (active socket, bad address length, double bind). */
	if ((sk->state != TCP_CLOSE)			||
	    (addr_len < sizeof(struct sockaddr_in))	||
	    (sk->num != 0))
		return -EINVAL;

	snum = ntohs(addr->sin_port);
#ifdef CONFIG_IP_MASQUERADE
	/* The kernel masquerader needs some ports. */		
	if(snum>=PORT_MASQ_BEGIN && snum<=PORT_MASQ_END)
		return -EADDRINUSE;
#endif		 
	if (snum == 0) 
		snum = sk->prot->good_socknum();
        if (snum < PROT_SOCK) {
		if (!suser()) 
		return(-EACCES);
		if (snum == 0)
			return(-EAGAIN);
	}
	
	chk_addr_ret = ip_chk_addr(addr->sin_addr.s_addr);
	if (addr->sin_addr.s_addr != 0 && chk_addr_ret != IS_MYADDR &&
	    chk_addr_ret != IS_MULTICAST && chk_addr_ret != IS_BROADCAST) {
#ifdef CONFIG_IP_TRANSPARENT_PROXY
		/* Superuser may bind to any address to allow transparent proxying. */
		if(!suser())
#endif
			return(-EADDRNOTAVAIL);	/* Source address MUST be ours! */
	}

	/*
	 *      We keep a pair of addresses. rcv_saddr is the one
	 *      used by hash lookups, and saddr is used for transmit.
	 *
	 *      In the BSD API these are the same except where it
	 *      would be illegal to use them (multicast/broadcast) in
	 *      which case the sending device address is used.
	 */
	sk->rcv_saddr = sk->saddr = addr->sin_addr.s_addr;
	if(chk_addr_ret == IS_MULTICAST || chk_addr_ret == IS_BROADCAST)
		sk->saddr = 0;  /* Use device */

	/* Make sure we are allowed to bind here. */
	if(sk->prot->verify_bind(sk, snum))
		return -EADDRINUSE;

	sk->num = snum;
	sk->dummy_th.source = ntohs(sk->num);
	sk->daddr = 0;
	sk->dummy_th.dest = 0;
	sk->prot->rehash(sk);
	add_to_prot_sklist(sk);

	ip_rt_put(sk->ip_route_cache);
	sk->ip_route_cache=NULL;
	return(0);
}

/*
 *	Connect to a remote host. There is regrettably still a little
 *	TCP 'magic' in here.
 */
 
static int inet_connect(struct socket *sock, struct sockaddr * uaddr,
		  int addr_len, int flags)
{
	struct sock *sk=(struct sock *)sock->data;
	int err;
	sock->conn = NULL;

	if (sock->state == SS_CONNECTING && tcp_connected(sk->state)) {
		sock->state = SS_CONNECTED;
		/* Connection completing after a connect/EINPROGRESS/select/connect */
		return 0;	/* Rock and roll */
	}

	if (sock->state == SS_CONNECTING && sk->protocol == IPPROTO_TCP && (flags & O_NONBLOCK)) {
		if(sk->err!=0)
			return sock_error(sk);
		return -EALREADY;	/* Connecting is currently in progress */
  	}

	if (sock->state != SS_CONNECTING) {
		/* We may need to bind the socket. */
		if(inet_autobind(sk) != 0)
			return(-EAGAIN);
		if (sk->prot->connect == NULL) 
			return(-EOPNOTSUPP);
		err = sk->prot->connect(sk, (struct sockaddr_in *)uaddr, addr_len);
		if (err < 0) 
			return(err);
  		sock->state = SS_CONNECTING;
	}
	
	if (sk->state > TCP_FIN_WAIT2 && sock->state==SS_CONNECTING) {
		sock->state=SS_UNCONNECTED;
		return sock_error(sk);
	}

	if (sk->state != TCP_ESTABLISHED &&(flags & O_NONBLOCK)) 
	  	return(-EINPROGRESS);

	cli(); /* avoid the race condition */
	while(sk->state == TCP_SYN_SENT || sk->state == TCP_SYN_RECV) {
		interruptible_sleep_on(sk->sleep);
		if (current->signal & ~current->blocked) {
			sti();
			return(-ERESTARTSYS);
		}
		/* This fixes a nasty in the tcp/ip code. There is a hideous hassle with
		   icmp error packets wanting to close a tcp or udp socket. */
		if(sk->err && sk->protocol == IPPROTO_TCP) {
			sock->state = SS_UNCONNECTED;
			sti();
			return sock_error(sk); /* set by tcp_err() */
		}
	}
	sti();
	sock->state = SS_CONNECTED;

	if (sk->state != TCP_ESTABLISHED && sk->err) {
		sock->state = SS_UNCONNECTED;
		return sock_error(sk);
	}
	return(0);
}


static int inet_socketpair(struct socket *sock1, struct socket *sock2)
{
	 return(-EOPNOTSUPP);
}


/*
 *	Accept a pending connection. The TCP layer now gives BSD semantics.
 */

static int inet_accept(struct socket *sock, struct socket *newsock, int flags)
{
	struct sock *sk1, *sk2;
	int err;

	sk1 = (struct sock *) sock->data;

	/*
	 *	We've been passed an extra socket.
	 *	We need to free it up because the tcp module creates
	 *	its own when it accepts one.
	 */
	 
	if (newsock->data) {
	  	struct sock *sk=(struct sock *)newsock->data;
	  	newsock->data=NULL;
	  	destroy_sock(sk);
	}
  
	if (sk1->prot->accept == NULL) 
		return(-EOPNOTSUPP);

	/*
	 *	Restore the state if we have been interrupted, and then returned. 
	 */
	 
	if (sk1->pair != NULL) {
		sk2 = sk1->pair;
		sk1->pair = NULL;
	} else {
		sk2 = sk1->prot->accept(sk1,flags);
		if (sk2 == NULL)
			return sock_error(sk1);
	}
	newsock->data = (void *)sk2;
	sk2->sleep = newsock->wait;
	sk2->socket = newsock;
	newsock->conn = NULL;
	if (flags & O_NONBLOCK) 
		return(0);

	cli(); /* avoid the race. */
	while(sk2->state == TCP_SYN_RECV) {
		interruptible_sleep_on(sk2->sleep);
		if (current->signal & ~current->blocked) {
			sti();
			sk1->pair = sk2;
			sk2->sleep = NULL;
			sk2->socket=NULL;
			newsock->data = NULL;
			return(-ERESTARTSYS);
		}
	}
	sti();

	if (sk2->state != TCP_ESTABLISHED && sk2->err > 0) {
		err = sock_error(sk2);
		destroy_sock(sk2);
		newsock->data = NULL;
		return err;
	}

	if (sk2->state == TCP_CLOSE) {
		destroy_sock(sk2);
		newsock->data=NULL;
		return -ECONNABORTED;
	}
	newsock->state = SS_CONNECTED;
	return(0);
}


/*
 *	This does both peername and sockname.
 */
 
static int inet_getname(struct socket *sock, struct sockaddr *uaddr,
		 int *uaddr_len, int peer)
{
	struct sockaddr_in *sin=(struct sockaddr_in *)uaddr;
	struct sock *sk;
  
	sin->sin_family = AF_INET;
	sk = (struct sock *) sock->data;
	if (peer) {
		if (!tcp_connected(sk->state)) 
			return(-ENOTCONN);
		sin->sin_port = sk->dummy_th.dest;
		sin->sin_addr.s_addr = sk->daddr;
	} else {
		__u32 addr = sk->rcv_saddr;
		if (!addr) {
			addr = sk->saddr;
			if (!addr)
				addr = ip_my_addr();
		}
		sin->sin_port = sk->dummy_th.source;
		sin->sin_addr.s_addr = addr;
	}
	*uaddr_len = sizeof(*sin);
	return(0);
}



static int inet_recvmsg(struct socket *sock, struct msghdr *ubuf, int size, int noblock, 
		   int flags, int *addr_len )
{
	struct sock *sk = (struct sock *) sock->data;
	
	if (sk->prot->recvmsg == NULL) 
		return(-EOPNOTSUPP);
	if(sk->err)
		return sock_error(sk);

	/* We may need to bind the socket. */
	if(inet_autobind(sk) != 0)
		return(-EAGAIN);

	return(sk->prot->recvmsg(sk, ubuf, size, noblock, flags,addr_len));
}


static int inet_sendmsg(struct socket *sock, struct msghdr *msg, int size, int noblock, 
	   int flags)
{
	struct sock *sk = (struct sock *) sock->data;
	if (sk->shutdown & SEND_SHUTDOWN) {
		send_sig(SIGPIPE, current, 1);
		return(-EPIPE);
	}
	if (sk->prot->sendmsg == NULL) 
		return(-EOPNOTSUPP);
	if(sk->err)
		return sock_error(sk);

	/* We may need to bind the socket. */
	if(inet_autobind(sk) != 0)
		return -EAGAIN;

	return(sk->prot->sendmsg(sk, msg, size, noblock, flags));
			   
}


static int inet_shutdown(struct socket *sock, int how)
{
	struct sock *sk=(struct sock*)sock->data;

	/*
	 * This should really check to make sure
	 * the socket is a TCP socket. (WHY AC...)
	 */
	how++; /* maps 0->1 has the advantage of making bit 1 rcvs and
		       1->2 bit 2 snds.
		       2->3 */
	if ((how & ~SHUTDOWN_MASK) || how==0)	/* MAXINT->0 */
		return(-EINVAL);
	if (sock->state == SS_CONNECTING && sk->state == TCP_ESTABLISHED)
		sock->state = SS_CONNECTED;
	if (!sk || !tcp_connected(sk->state)) 
		return(-ENOTCONN);
	sk->shutdown |= how;
	if (sk->prot->shutdown)
		sk->prot->shutdown(sk, how);
	return(0);
}


static int inet_select(struct socket *sock, int sel_type, select_table *wait )
{
	struct sock *sk=(struct sock *) sock->data;
	if (sk->prot->select == NULL) 
		return(0);

	return(sk->prot->select(sk, sel_type, wait));
}

/*
 *	ioctl() calls you can issue on an INET socket. Most of these are
 *	device configuration and stuff and very rarely used. Some ioctls
 *	pass on to the socket itself.
 *
 *	NOTE: I like the idea of a module for the config stuff. ie ifconfig
 *	loads the devconfigure module does its configuring and unloads it.
 *	There's a good 20K of config code hanging around the kernel.
 */

static int inet_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk=(struct sock *)sock->data;
	int err;
	int pid;

	switch(cmd) 
	{
		case FIOSETOWN:
		case SIOCSPGRP:
			err=verify_area(VERIFY_READ,(int *)arg,sizeof(long));
			if(err)
				return err;
			pid = get_user((int *) arg);
			/* see inet_fcntl */
			if (current->pid != pid && current->pgrp != -pid && !suser())
				return -EPERM;
			sk->proc = pid;
			return(0);
		case FIOGETOWN:
		case SIOCGPGRP:
			err=verify_area(VERIFY_WRITE,(void *) arg, sizeof(long));
			if(err)
				return err;
			put_fs_long(sk->proc,(int *)arg);
			return(0);			
		case SIOCGSTAMP:
			if(sk->stamp.tv_sec==0)
				return -ENOENT;
			err=verify_area(VERIFY_WRITE,(void *)arg,sizeof(struct timeval));
			if(err)
				return err;
			memcpy_tofs((void *)arg,&sk->stamp,sizeof(struct timeval));
			return 0;
		case SIOCADDRT:
		case SIOCDELRT:
			return(ip_rt_ioctl(cmd,(void *) arg));
		case SIOCDARP:
		case SIOCGARP:
		case SIOCSARP:
		case OLD_SIOCDARP:
		case OLD_SIOCGARP:
		case OLD_SIOCSARP:
			return(arp_ioctl(cmd,(void *) arg));
		case SIOCDRARP:
		case SIOCGRARP:
		case SIOCSRARP:
#ifdef CONFIG_KERNELD
			if (rarp_ioctl_hook == NULL)
				request_module("rarp");
#endif
			if (rarp_ioctl_hook != NULL)
				return(rarp_ioctl_hook(cmd,(void *) arg));
		case SIOCGIFCONF:
		case SIOCGIFFLAGS:
		case SIOCSIFFLAGS:
		case SIOCGIFADDR:
		case SIOCSIFADDR:
		case SIOCADDMULTI:
		case SIOCDELMULTI:
		case SIOCGIFDSTADDR:
		case SIOCSIFDSTADDR:
		case SIOCGIFBRDADDR:
		case SIOCSIFBRDADDR:
		case SIOCGIFNETMASK:
		case SIOCSIFNETMASK:
		case SIOCGIFMETRIC:
		case SIOCSIFMETRIC:
		case SIOCGIFMEM:
		case SIOCSIFMEM:
		case SIOCGIFMTU:
		case SIOCSIFMTU:
		case SIOCSIFLINK:
		case SIOCGIFHWADDR:
		case SIOCSIFHWADDR:
		case SIOCSIFMAP:
		case SIOCGIFMAP:
		case SIOCSIFSLAVE:
		case SIOCGIFSLAVE:
			return(dev_ioctl(cmd,(void *) arg));

		case SIOCGIFBR:
		case SIOCSIFBR:
#ifdef CONFIG_BRIDGE		
			return(br_ioctl(cmd,(void *) arg));
#else
			return -ENOPKG;
#endif						
			
		case SIOCADDDLCI:
		case SIOCDELDLCI:
#ifdef CONFIG_DLCI
			return(dlci_ioctl(cmd, (void *) arg));
#endif

#ifdef CONFIG_DLCI_MODULE

#ifdef CONFIG_KERNELD
			if (dlci_ioctl_hook == NULL)
				request_module("dlci");
#endif

			if (dlci_ioctl_hook)
				return((*dlci_ioctl_hook)(cmd, (void *) arg));
#endif
			return -ENOPKG;

		default:
			if ((cmd >= SIOCDEVPRIVATE) &&
			   (cmd <= (SIOCDEVPRIVATE + 15)))
				return(dev_ioctl(cmd,(void *) arg));

#ifdef CONFIG_NET_RADIO
			if((cmd >= SIOCIWFIRST) &&
			   (cmd <= SIOCIWLAST))
				return(dev_ioctl(cmd,(void *) arg));
#endif	/* CONFIG_NET_RADIO */

			if (sk->prot->ioctl==NULL) 
				return(-EINVAL);
			return(sk->prot->ioctl(sk, cmd, arg));
	}
	/*NOTREACHED*/
	return(0);
}

static struct proto_ops inet_proto_ops = {
	AF_INET,

	inet_create,
	inet_dup,
	inet_release,
	inet_bind,
	inet_connect,
	inet_socketpair,
	inet_accept,
	inet_getname, 
	inet_select,
	inet_ioctl,
	inet_listen,
	inet_shutdown,
	inet_setsockopt,
	inet_getsockopt,
	inet_fcntl,
	inet_sendmsg,
	inet_recvmsg
};

extern unsigned long seq_offset;

/*
 *	Called by socket.c on kernel startup.  
 */
 
#ifdef	CONFIG_OSFMACH3
/*
 * Work-around some invalid code "optimizations": 
 * the proc_dir_entries below are stored in a read-only data section...
 */
static int __proc_net_register(struct proc_dir_entry * dp)
{
	return proc_net_register(dp);
}
#define proc_net_register(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
{ \
	static struct proc_dir_entry xxx; \
	xxx = *(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10); \
	__proc_net_register(&xxx); \
}
#endif	/* CONFIG_OSFMACH3 */

void inet_proto_init(struct net_proto *pro)
{
	struct inet_protocol *p;

	printk("Swansea University Computer Society TCP/IP for NET3.034\n");

	/*
	 *	Tell SOCKET that we are alive... 
	 */
   
  	(void) sock_register(inet_proto_ops.family, &inet_proto_ops);

  	seq_offset = CURRENT_TIME*250;

	/*
	 *	Add all the protocols. 
	 */
	 
	printk("IP Protocols: ");
	for(p = inet_protocol_base; p != NULL;) 
	{
		struct inet_protocol *tmp = (struct inet_protocol *) p->next;
		inet_add_protocol(p);
		printk("%s%s",p->name,tmp?", ":"\n");
		p = tmp;
	}

	/*
	 *	Set the ARP module up
	 */
	arp_init();
  	/*
  	 *	Set the IP module up
  	 */
	ip_init();
	/*
	 *	Set the ICMP layer up
	 */
	icmp_init(&inet_proto_ops);
	/*
	 *	Set the firewalling up
	 */
#if defined(CONFIG_IP_ACCT)||defined(CONFIG_IP_FIREWALL)|| \
    defined(CONFIG_IP_MASQUERADE)
	ip_fw_init();
#endif
	/*
	 *	Initialise the multicast router
	 */
#if defined(CONFIG_IP_MROUTE)
	ip_mr_init();
#endif

	/*
	 *  Initialise AF_INET alias type (register net_alias_type)
	 */

#if defined(CONFIG_IP_ALIAS)
	ip_alias_init();
#endif

#ifdef CONFIG_INET_RARP
	rarp_ioctl_hook = rarp_ioctl;
#endif
	/*
	 *	Create all the /proc entries.
	 */

#ifdef CONFIG_PROC_FS

#ifdef CONFIG_INET_RARP
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_RARP, 4, "rarp",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		rarp_get_info
	});
#endif		/* RARP */

	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_RAW, 3, "raw",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		raw_get_info
	});
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_SNMP, 4, "snmp",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		snmp_get_info
	});
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_SOCKSTAT, 8, "sockstat",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		afinet_get_info
	});
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_TCP, 3, "tcp",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		tcp_get_info
	});
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_UDP, 3, "udp",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		udp_get_info
	});
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_ROUTE, 5, "route",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		rt_get_info
	});
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_RTCACHE, 8, "rt_cache",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		rt_cache_get_info
	});
#endif		/* CONFIG_PROC_FS */
}
