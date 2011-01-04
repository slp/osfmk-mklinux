/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		The User Datagram Protocol (UDP).
 *
 * Version:	@(#)udp.c	1.0.13	06/02/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Arnt Gulbrandsen, <agulbra@nvg.unit.no>
 *		Alan Cox, <Alan.Cox@linux.org>
 *
 * Fixes:
 *		Alan Cox	:	verify_area() calls
 *		Alan Cox	: 	stopped close while in use off icmp
 *					messages. Not a fix but a botch that
 *					for udp at least is 'valid'.
 *		Alan Cox	:	Fixed icmp handling properly
 *		Alan Cox	: 	Correct error for oversized datagrams
 *		Alan Cox	:	Tidied select() semantics. 
 *		Alan Cox	:	udp_err() fixed properly, also now 
 *					select and read wake correctly on errors
 *		Alan Cox	:	udp_send verify_area moved to avoid mem leak
 *		Alan Cox	:	UDP can count its memory
 *		Alan Cox	:	send to an unknown connection causes
 *					an ECONNREFUSED off the icmp, but
 *					does NOT close.
 *		Alan Cox	:	Switched to new sk_buff handlers. No more backlog!
 *		Alan Cox	:	Using generic datagram code. Even smaller and the PEEK
 *					bug no longer crashes it.
 *		Fred Van Kempen	: 	Net2e support for sk->broadcast.
 *		Alan Cox	:	Uses skb_free_datagram
 *		Alan Cox	:	Added get/set sockopt support.
 *		Alan Cox	:	Broadcasting without option set returns EACCES.
 *		Alan Cox	:	No wakeup calls. Instead we now use the callbacks.
 *		Alan Cox	:	Use ip_tos and ip_ttl
 *		Alan Cox	:	SNMP Mibs
 *		Alan Cox	:	MSG_DONTROUTE, and 0.0.0.0 support.
 *		Matt Dillon	:	UDP length checks.
 *		Alan Cox	:	Smarter af_inet used properly.
 *		Alan Cox	:	Use new kernel side addressing.
 *		Alan Cox	:	Incorrect return on truncated datagram receive.
 *	Arnt Gulbrandsen 	:	New udp_send and stuff
 *		Alan Cox	:	Cache last socket
 *		Alan Cox	:	Route cache
 *		Jon Peatfield	:	Minor efficiency fix to sendto().
 *		Mike Shaver	:	RFC1122 checks.
 *		Alan Cox	:	Nonblocking error fix.
 *	Willy Konynenberg	:	Transparent proxying support.
 *		David S. Miller	:	New socket lookup architecture for ISS.
 *					Last socket cache retained as it
 *					does have a high hit rate.
 *              Elliot Poger    :       Added support for SO_BINDTODEVICE.
 *	Willy Konynenberg	:	Transparent proxy adapted to new
 *					socket hash code.
 *		Philip Gladstone:	Added missing ip_rt_put
 *
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
 
/* RFC1122 Status:
   4.1.3.1 (Ports):
     SHOULD send ICMP_PORT_UNREACHABLE in response to datagrams to 
       an un-listened port. (OK)
   4.1.3.2 (IP Options)
     MUST pass IP options from IP -> application (OK)
     MUST allow application to specify IP options (OK)
   4.1.3.3 (ICMP Messages)
     MUST pass ICMP error messages to application (OK)
   4.1.3.4 (UDP Checksums)
     MUST provide facility for checksumming (OK)
     MAY allow application to control checksumming (OK)
     MUST default to checksumming on (OK)
     MUST discard silently datagrams with bad csums (OK)
   4.1.3.5 (UDP Multihoming)
     MUST allow application to specify source address (OK)
     SHOULD be able to communicate the chosen src addr up to application
       when application doesn't choose (NOT YET - doesn't seem to be in the BSD API)
       [Does opening a SOCK_PACKET and snooping your output count 8)]
   4.1.3.6 (Invalid Addresses)
     MUST discard invalid source addresses (NOT YET -- will be implemented
       in IP, so UDP will eventually be OK.  Right now it's a violation.)
     MUST only send datagrams with one of our addresses (NOT YET - ought to be OK )
   950728 -- MS
*/

#include <asm/system.h>
#include <asm/segment.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/termios.h>
#include <linux/mm.h>
#include <linux/config.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <net/snmp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/tcp.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/udp.h>
#include <net/icmp.h>
#include <net/route.h>
#include <net/checksum.h>

/*
 *	Snmp MIB for the UDP layer
 */

struct udp_mib		udp_statistics;

struct sock *udp_hash[UDP_HTABLE_SIZE];

static int udp_v4_verify_bind(struct sock *sk, unsigned short snum)
{
	struct sock *sk2;
	int retval = 0, sk_reuse = sk->reuse;

	SOCKHASH_LOCK();
	for(sk2 = udp_hash[snum & (UDP_HTABLE_SIZE - 1)]; sk2 != NULL; sk2 = sk2->next) {
		if((sk2->num == snum) && (sk2 != sk)) {
			int sk2_reuse = sk2->reuse;

			/* Two sockets can be bound to the same port if they're
			 * bound to different interfaces... */
			if (sk->bound_device != sk2->bound_device)
				continue;

			if(!sk2->rcv_saddr || !sk->rcv_saddr) {
				if((!sk2_reuse) || (!sk_reuse)) {
					retval = 1;
					break;
				}
			} else if(sk2->rcv_saddr == sk->rcv_saddr) {
				if((!sk_reuse) || (!sk2_reuse)) {
					retval = 1;
					break;
				}
			}
		}
	}
	SOCKHASH_UNLOCK();
	return retval;
}

static inline int udp_lport_inuse(u16 num)
{
	struct sock *sk = udp_hash[num & (UDP_HTABLE_SIZE - 1)];

	for(; sk != NULL; sk = sk->next) {
		if(sk->num == num)
			return 1;
	}
	return 0;
}

/* Shared by v4/v6 tcp. */
unsigned short udp_good_socknum(void)
{
	int result;
	static int start = 0;
	int i, best, best_size_so_far;

	SOCKHASH_LOCK();

	/* Select initial not-so-random "best" */
	best = PROT_SOCK + 1 + (start & 1023);
	best_size_so_far = 32767;	/* "big" num */
	result = best;
	for (i = 0; i < UDP_HTABLE_SIZE; i++, result++) {
		struct sock *sk;
		int size;

		sk = udp_hash[result & (UDP_HTABLE_SIZE - 1)];

		/* No clashes - take it */
		if (!sk)
			goto out;

		/* Is this one better than our best so far? */
		size = 0;
		do {
			if(++size >= best_size_so_far)
				goto next;
		} while((sk = sk->next) != NULL);
		best_size_so_far = size;
		best = result;
next:
	}

	while (udp_lport_inuse(best))
		best += UDP_HTABLE_SIZE;
	result = best;
out:
	start = result;
	SOCKHASH_UNLOCK();
	return result;
}

static void udp_v4_hash(struct sock *sk)
{
	struct sock **skp;
	int num = sk->num;

	num &= (UDP_HTABLE_SIZE - 1);
	skp = &udp_hash[num];

	SOCKHASH_LOCK();
	sk->next = *skp;
	*skp = sk;
	sk->hashent = num;
	SOCKHASH_UNLOCK();
}

static void udp_v4_unhash(struct sock *sk)
{
	struct sock **skp;
	int num = sk->num;

	num &= (UDP_HTABLE_SIZE - 1);
	skp = &udp_hash[num];

	SOCKHASH_LOCK();
	while(*skp != NULL) {
		if(*skp == sk) {
			*skp = sk->next;
			break;
		}
		skp = &((*skp)->next);
	}
	SOCKHASH_UNLOCK();
}

static void udp_v4_rehash(struct sock *sk)
{
	struct sock **skp;
	int num = sk->num;
	int oldnum = sk->hashent;

	num &= (UDP_HTABLE_SIZE - 1);
	skp = &udp_hash[oldnum];

	SOCKHASH_LOCK();
	while(*skp != NULL) {
		if(*skp == sk) {
			*skp = sk->next;
			break;
		}
		skp = &((*skp)->next);
	}
	sk->next = udp_hash[num];
	udp_hash[num] = sk;
	sk->hashent = num;
	SOCKHASH_UNLOCK();
}

/* UDP is nearly always wildcards out the wazoo, it makes no sense to try
 * harder than this. -DaveM
 */
__inline__ struct sock *udp_v4_lookup(u32 saddr, u16 sport, u32 daddr, u16 dport,
				      struct device *dev)
{
	struct sock *sk, *result = NULL;
	unsigned short hnum = ntohs(dport);
	int badness = -1;

	for(sk = udp_hash[hnum & (UDP_HTABLE_SIZE - 1)]; sk != NULL; sk = sk->next) {
		if((sk->num == hnum) && !(sk->dead && (sk->state == TCP_CLOSE))) {
			int score = 0;
			if(sk->rcv_saddr) {
				if(sk->rcv_saddr != daddr)
					continue;
				score++;
			}
			if(sk->daddr) {
				if(sk->daddr != saddr)
					continue;
				score++;
			}
			if(sk->dummy_th.dest) {
				if(sk->dummy_th.dest != sport)
					continue;
				score++;
			}
			/* If this socket is bound to a particular interface,
			 * did the packet come in on it? */
			if (sk->bound_device) {
				if (dev == sk->bound_device)
					score++;
				else
					continue;  /* mismatch--not this sock */
			}
			if(score == 4) {
				result = sk;
				break;
			} else if(score > badness) {
				result = sk;
				badness = score;
			}
		}
	}
	return result;
}

#ifdef CONFIG_IP_TRANSPARENT_PROXY
struct sock *udp_v4_proxy_lookup(u32 saddr, u16 sport, u32 daddr, u16 dport, u32 paddr, u16 rport,
				 struct device *dev)
{
	struct sock *hh[3], *sk, *result = NULL;
	int i;
	int badness = -1;
	unsigned short hnum = ntohs(dport);
	unsigned short hpnum = ntohs(rport);

	SOCKHASH_LOCK();
	hh[0] = udp_hash[hnum & (UDP_HTABLE_SIZE - 1)];
	hh[1] = udp_hash[hpnum & (UDP_HTABLE_SIZE - 1)];
	for (i = 0; i < 2; i++) {
		for(sk = hh[i]; sk != NULL; sk = sk->next) {
			if(sk->num == hnum || sk->num == hpnum) {
				int score = 0;
				if(sk->dead && (sk->state == TCP_CLOSE))
					continue;
				if(sk->rcv_saddr) {
					if((sk->num != hpnum || sk->rcv_saddr != paddr) &&
					   (sk->num != hnum || sk->rcv_saddr != daddr))
						continue;
					score++;
				}
				if(sk->daddr) {
					if(sk->daddr != saddr)
						continue;
					score++;
				}
				if(sk->dummy_th.dest) {
					if(sk->dummy_th.dest != sport)
						continue;
					score++;
				}
				/* If this socket is bound to a particular interface,
				 * did the packet come in on it? */
				if(sk->bound_device) {
					if (sk->bound_device != dev)
						continue;
					score++;
				}
				if(score == 4 && sk->num == hnum) {
					result = sk;
					break;
				} else if(score > badness && (sk->num == hpnum || sk->rcv_saddr)) {
					result = sk;
					badness = score;
				}
			}
		}
	}
	SOCKHASH_UNLOCK();
	return result;
}
#endif

static inline struct sock *udp_v4_mcast_next(struct sock *sk,
					     unsigned short num,
					     unsigned long raddr,
					     unsigned short rnum,
					     unsigned long laddr,
					     struct device *dev)
{
	struct sock *s = sk;
	unsigned short hnum = ntohs(num);
	for(; s; s = s->next) {
		if ((s->num != hnum)					||
		    (s->dead && (s->state == TCP_CLOSE))		||
		    (s->daddr && s->daddr!=raddr)			||
		    (s->dummy_th.dest != rnum && s->dummy_th.dest != 0) ||
		    ((s->bound_device) && (s->bound_device!=dev))       ||
		    (s->rcv_saddr  && s->rcv_saddr != laddr))
			continue;
		break;
  	}
  	return s;
}

#define min(a,b)	((a)<(b)?(a):(b))


/*
 * This routine is called by the ICMP module when it gets some
 * sort of error condition.  If err < 0 then the socket should
 * be closed and the error returned to the user.  If err > 0
 * it's just the icmp type << 8 | icmp code.  
 * Header points to the ip header of the error packet. We move
 * on past this. Then (as it used to claim before adjustment)
 * header points to the first 8 bytes of the udp header.  We need
 * to find the appropriate port.
 */

void udp_err(int type, int code, unsigned char *header, __u32 daddr,
	__u32 saddr, struct inet_protocol *protocol, int len)
{
	struct udphdr *uh;
	struct sock *sk;

	/*
	 *	Find the 8 bytes of post IP header ICMP included for us
	 */  
	 
	if(len<sizeof(struct udphdr))
		return;
	
	uh = (struct udphdr *)header;  
   
	sk = udp_v4_lookup(daddr, uh->dest, saddr, uh->source, NULL);
	if (sk == NULL) 
	  	return;	/* No socket for error */
  	
	if (type == ICMP_SOURCE_QUENCH) 
	{	/* Slow down! */
		if (sk->cong_window > 1) 
			sk->cong_window = sk->cong_window/2;
		return;
	}

	if (type == ICMP_PARAMETERPROB)
	{
		sk->err = EPROTO;
		sk->error_report(sk);
		return;
	}
			
	/*
	 *	Various people wanted BSD UDP semantics. Well they've come 
	 *	back out because they slow down response to stuff like dead
	 *	or unreachable name servers and they screw term users something
	 *	chronic. Oh and it violates RFC1122. So basically fix your 
	 *	client code people.
	 */
	 
	/* RFC1122: OK.  Passes ICMP errors back to application, as per */
	/* 4.1.3.3. */
	/* After the comment above, that should be no surprise. */

	if(code<=NR_ICMP_UNREACH && icmp_err_convert[code].fatal)
	{
		/*
		 *	4.x BSD compatibility item. Break RFC1122 to
		 *	get BSD socket semantics.
		 */
		if(sk->bsdism && sk->state!=TCP_ESTABLISHED)
			return;
		sk->err = icmp_err_convert[code].errno;
		sk->error_report(sk);
	}
}


static unsigned short udp_check(struct udphdr *uh, int len, unsigned long saddr, unsigned long daddr, unsigned long base)
{
	return(csum_tcpudp_magic(saddr, daddr, len, IPPROTO_UDP, base));
}

struct udpfakehdr 
{
	struct udphdr uh;
	__u32 daddr;
	__u32 other;
	const char *from;
	__u32 wcheck;
};

/*
 *	Copy and checksum a UDP packet from user space into a buffer. We still have to do the planning to
 *	get ip_build_xmit to spot direct transfer to network card and provide an additional callback mode
 *	for direct user->board I/O transfers. That one will be fun.
 */
 
static void udp_getfrag(const void *p, __u32 saddr, char * to, unsigned int offset, unsigned int fraglen) 
{
	struct udpfakehdr *ufh = (struct udpfakehdr *)p;
	const char *src;
	char *dst;
	unsigned int len;

	if (offset) 
	{
		len = fraglen;
	 	src = ufh->from+(offset-sizeof(struct udphdr));
	 	dst = to;
	}
	else 
	{
		len = fraglen-sizeof(struct udphdr);
 		src = ufh->from;
		dst = to+sizeof(struct udphdr);
	}
	ufh->wcheck = csum_partial_copy_fromuser(src, dst, len, ufh->wcheck);
	if (offset == 0) 
	{
 		ufh->wcheck = csum_partial((char *)ufh, sizeof(struct udphdr),
 				   ufh->wcheck);
		ufh->uh.check = csum_tcpudp_magic(saddr, ufh->daddr, 
					  ntohs(ufh->uh.len),
					  IPPROTO_UDP, ufh->wcheck);
		if (ufh->uh.check == 0)
			ufh->uh.check = -1;
		memcpy(to, ufh, sizeof(struct udphdr));
	}
}

/*
 *	Unchecksummed UDP is sufficiently critical to stuff like ATM video conferencing
 *	that we use two routines for this for speed. Probably we ought to have a CONFIG_FAST_NET
 *	set for >10Mb/second boards to activate this sort of coding. Timing needed to verify if
 *	this is a valid decision.
 */
 
static void udp_getfrag_nosum(const void *p, __u32 saddr, char * to, unsigned int offset, unsigned int fraglen) 
{
	struct udpfakehdr *ufh = (struct udpfakehdr *)p;
	const char *src;
	char *dst;
	unsigned int len;

	if (offset) 
	{
		len = fraglen;
	 	src = ufh->from+(offset-sizeof(struct udphdr));
	 	dst = to;
	}
	else 
	{
		len = fraglen-sizeof(struct udphdr);
 		src = ufh->from;
		dst = to+sizeof(struct udphdr);
	}
	memcpy_fromfs(dst,src,len);
	if (offset == 0) 
		memcpy(to, ufh, sizeof(struct udphdr));
}


/*
 *	Send UDP frames.
 */

static int udp_send(struct sock *sk, struct sockaddr_in *sin,
		      const unsigned char *from, int len, int rt,
		    __u32 saddr, int noblock) 
{
	int ulen = len + sizeof(struct udphdr);
	int a;
	struct udpfakehdr ufh;
	
	if(ulen>65535-sizeof(struct iphdr))
		return -EMSGSIZE;

	ufh.uh.source = sk->dummy_th.source;
	ufh.uh.dest = sin->sin_port;
	ufh.uh.len = htons(ulen);
	ufh.uh.check = 0;
	ufh.daddr = sin->sin_addr.s_addr;
	ufh.other = (htons(ulen) << 16) + IPPROTO_UDP*256;
	ufh.from = from;
	ufh.wcheck = 0;

#ifdef CONFIG_IP_TRANSPARENT_PROXY
	if (rt&MSG_PROXY)
	{
		/*
		 * We map the first 8 bytes of a second sockaddr_in
		 * into the last 8 (unused) bytes of a sockaddr_in.
		 * This _is_ ugly, but it's the only way to do it
		 * easily,  without adding system calls.
		 */
		struct sockaddr_in *sinfrom =
			(struct sockaddr_in *) sin->sin_zero;

		if (!suser())
			return(-EPERM);
		if (sinfrom->sin_family && sinfrom->sin_family != AF_INET)
			return(-EINVAL);
		if (sinfrom->sin_port == 0)
			return(-EINVAL);
		saddr = sinfrom->sin_addr.s_addr;
		ufh.uh.source = sinfrom->sin_port;
	}
#endif

	/* RFC1122: OK.  Provides the checksumming facility (MUST) as per */
	/* 4.1.3.4. It's configurable by the application via setsockopt() */
	/* (MAY) and it defaults to on (MUST).  Almost makes up for the */
	/* violation above. -- MS */

	if(sk->no_check)
		a = ip_build_xmit(sk, udp_getfrag_nosum, &ufh, ulen, 
			sin->sin_addr.s_addr, saddr, sk->opt, rt, IPPROTO_UDP, noblock);
	else
		a = ip_build_xmit(sk, udp_getfrag, &ufh, ulen, 
			sin->sin_addr.s_addr, saddr, sk->opt, rt, IPPROTO_UDP, noblock);
	if(a<0)
		return a;
	udp_statistics.UdpOutDatagrams++;
	return len;
}


static int udp_sendto(struct sock *sk, const unsigned char *from, int len, int noblock,
	   unsigned flags, struct sockaddr_in *usin, int addr_len)
{
	struct sockaddr_in sin;
	int tmp;
	__u32 saddr=0;

	/* 
	 *	Check the flags. We support no flags for UDP sending
	 */

#ifdef CONFIG_IP_TRANSPARENT_PROXY
	if (flags&~(MSG_DONTROUTE|MSG_PROXY))
#else
	if (flags&~MSG_DONTROUTE) 
#endif
	  	return(-EINVAL);
	/*
	 *	Get and verify the address. 
	 */
	 
	if (usin) 
	{
		if (addr_len < sizeof(sin)) 
			return(-EINVAL);
		if (usin->sin_family && usin->sin_family != AF_INET) 
			return(-EINVAL);
		if (usin->sin_port == 0) 
			return(-EINVAL);
	} 
	else 
	{
#ifdef CONFIG_IP_TRANSPARENT_PROXY
		/* We need to provide a sockaddr_in when using MSG_PROXY. */
		if (flags&MSG_PROXY)
			return(-EINVAL);
#endif
		if (sk->state != TCP_ESTABLISHED) 
			return(-EINVAL);
		sin.sin_family = AF_INET;
		sin.sin_port = sk->dummy_th.dest;
		sin.sin_addr.s_addr = sk->daddr;
		usin = &sin;
  	}
  
  	/*
  	 *	BSD socket semantics. You must set SO_BROADCAST to permit
  	 *	broadcasting of data.
  	 */
  	 
	/* RFC1122: OK.  Allows the application to select the specific */
	/* source address for an outgoing packet (MUST) as per 4.1.3.5. */
	/* Optional addition: a mechanism for telling the application what */
	/* address was used. (4.1.3.5, MAY) -- MS */

	/* RFC1122: MUST ensure that all outgoing packets have one */
	/* of this host's addresses as a source addr.(4.1.3.6) - bind in  */
	/* af_inet.c checks these. It does need work to allow BSD style */
	/* bind to multicast as is done by xntpd		*/

  	if(usin->sin_addr.s_addr==INADDR_ANY)
  		usin->sin_addr.s_addr=ip_my_addr();
  		
  	if(!sk->broadcast && ip_chk_addr(usin->sin_addr.s_addr)==IS_BROADCAST)
	    	return -EACCES;			/* Must turn broadcast on first */

	lock_sock(sk);

	/* Send the packet. */
	tmp = udp_send(sk, usin, from, len, flags, saddr, noblock);

	/* The datagram has been sent off.  Release the socket. */
	release_sock(sk);
	return(tmp);
}

/*
 *	Temporary
 */
 
static int udp_sendmsg(struct sock *sk, struct msghdr *msg, int len, int noblock, 
	int flags)
{
	if(msg->msg_iovlen==1)
		return udp_sendto(sk,msg->msg_iov[0].iov_base,len, noblock, flags, msg->msg_name, msg->msg_namelen);
	else
	{
		/*
		 *	For awkward cases we linearise the buffer first. In theory this is only frames
		 *	whose iovec's don't split on 4 byte boundaries, and soon encrypted stuff (to keep
		 *	skip happy). We are a bit more general about it.
		 */
		 
		unsigned char *buf;
		int fs;
		int err;
		if(len>65515)
			return -EMSGSIZE;
		buf=kmalloc(len, GFP_KERNEL);
		if(buf==NULL)
			return -ENOBUFS;
		memcpy_fromiovec(buf, msg->msg_iov, len);
		fs=get_fs();
		set_fs(get_ds());
		err=udp_sendto(sk,buf,len, noblock, flags, msg->msg_name, msg->msg_namelen);
		set_fs(fs);
		kfree_s(buf,len);
		return err;
	}
}

/*
 *	IOCTL requests applicable to the UDP protocol
 */
 
int udp_ioctl(struct sock *sk, int cmd, unsigned long arg)
{
	int err;
	switch(cmd) 
	{
		case TIOCOUTQ:
		{
			unsigned long amount;

			if (sk->state == TCP_LISTEN) return(-EINVAL);
			amount = sock_wspace(sk);
			err=verify_area(VERIFY_WRITE,(void *)arg,
					sizeof(unsigned long));
			if(err)
				return(err);
			put_fs_long(amount,(unsigned long *)arg);
			return(0);
		}

		case TIOCINQ:
		{
			struct sk_buff *skb;
			unsigned long amount;

			if (sk->state == TCP_LISTEN) return(-EINVAL);
			amount = 0;
			skb = skb_peek(&sk->receive_queue);
			if (skb != NULL) {
				/*
				 * We will only return the amount
				 * of this packet since that is all
				 * that will be read.
				 */
				amount = skb->len-sizeof(struct udphdr);
			}
			err=verify_area(VERIFY_WRITE,(void *)arg,
						sizeof(unsigned long));
			if(err)
				return(err);
			put_fs_long(amount,(unsigned long *)arg);
			return(0);
		}

		default:
			return(-EINVAL);
	}
	return(0);
}


/*
 * 	This should be easy, if there is something there we\
 * 	return it, otherwise we block.
 */

int udp_recvmsg(struct sock *sk, struct msghdr *msg, int len,
	     int noblock, int flags,int *addr_len)
{
  	int copied = 0;
  	int truesize;
  	struct sk_buff *skb;
  	int er;
  	struct sockaddr_in *sin=(struct sockaddr_in *)msg->msg_name;

	/*
	 *	Check any passed addresses
	 */
	 
  	if (addr_len) 
  		*addr_len=sizeof(*sin);
  
	/*
	 *	From here the generic datagram does a lot of the work. Come
	 *	the finished NET3, it will do _ALL_ the work!
	 */
	 	
	skb=skb_recv_datagram(sk,flags,noblock,&er);
	if(skb==NULL)
  		return er;
  
  	truesize = skb->len - sizeof(struct udphdr);
  	copied = min(len, truesize);

  	/*
  	 *	FIXME : should use udp header size info value 
  	 */
  	 
	skb_copy_datagram_iovec(skb,sizeof(struct udphdr),msg->msg_iov,copied);
	sk->stamp=skb->stamp;

	/* Copy the address. */
	if (sin) 
	{
		sin->sin_family = AF_INET;
		sin->sin_port = skb->h.uh->source;
		sin->sin_addr.s_addr = skb->daddr;
#ifdef CONFIG_IP_TRANSPARENT_PROXY
		if (flags&MSG_PROXY)
		{
			/*
			 * We map the first 8 bytes of a second sockaddr_in
			 * into the last 8 (unused) bytes of a sockaddr_in.
			 * This _is_ ugly, but it's the only way to do it
			 * easily,  without adding system calls.
			 */
			struct sockaddr_in *sinto =
				(struct sockaddr_in *) sin->sin_zero;

			sinto->sin_family = AF_INET;
			sinto->sin_port = skb->h.uh->dest;
			sinto->sin_addr.s_addr = skb->saddr;
		}
#endif
  	}
  
  	skb_free_datagram(sk, skb);
  	return(copied);
}

int udp_connect(struct sock *sk, struct sockaddr_in *usin, int addr_len)
{
	struct rtable *rt;
	if (addr_len < sizeof(*usin)) 
	  	return(-EINVAL);

	if (usin->sin_family && usin->sin_family != AF_INET) 
	  	return(-EAFNOSUPPORT);
	if (usin->sin_addr.s_addr==INADDR_ANY)
		usin->sin_addr.s_addr=ip_my_addr();

	if(!sk->broadcast && ip_chk_addr(usin->sin_addr.s_addr)==IS_BROADCAST)
		return -EACCES;			/* Must turn broadcast on first */
  	
  	rt=ip_rt_route((__u32)usin->sin_addr.s_addr, sk->localroute, sk->bound_device);
  	if (rt==NULL)
  		return -ENETUNREACH;
  	if(!sk->saddr)
	  	sk->saddr = rt->rt_src;		/* Update source address */
	if(!sk->rcv_saddr)
		sk->rcv_saddr = rt->rt_src;
	sk->daddr = usin->sin_addr.s_addr;
	sk->dummy_th.dest = usin->sin_port;
	sk->state = TCP_ESTABLISHED;
	if (sk->ip_route_cache)
	        ip_rt_put(sk->ip_route_cache);
	sk->ip_route_cache = rt;
	return(0);
}


static void udp_close(struct sock *sk, unsigned long timeout)
{
	lock_sock(sk);
	sk->state = TCP_CLOSE;
	sk->dead = 1;
	release_sock(sk);
	udp_v4_unhash(sk);
	destroy_sock(sk);
}

static inline void udp_queue_rcv_skb(struct sock * sk, struct sk_buff *skb)
{
	/*
	 *	Charge it to the socket, dropping if the queue is full.
	 */

	/* I assume this includes the IP options, as per RFC1122 (4.1.3.2). */
	/* If not, please let me know. -- MS */

	if (__sock_queue_rcv_skb(sk,skb)<0) {
		udp_statistics.UdpInErrors++;
		ip_statistics.IpInDiscards++;
		ip_statistics.IpInDelivers--;
		skb->sk = NULL;
		kfree_skb(skb, FREE_WRITE);
		return;
	}
	udp_statistics.UdpInDatagrams++;
}


static inline void udp_deliver(struct sock *sk, struct sk_buff *skb)
{
	skb->sk = sk;

	if (sk->users) {
		__skb_queue_tail(&sk->back_log, skb);
		return;
	}
	udp_queue_rcv_skb(sk, skb);
}

#ifdef CONFIG_IP_TRANSPARENT_PROXY
/*
 *	Check whether a received UDP packet might be for one of our
 *	sockets.
 */

int udp_chkaddr(struct sk_buff *skb)
{
	struct iphdr *iph = skb->h.iph;
	struct udphdr *uh = (struct udphdr *)(skb->h.raw + iph->ihl*4);
	struct sock *sk;

	sk = udp_v4_lookup(iph->saddr, uh->source, iph->daddr, uh->dest,
			   skb->dev);
	if (!sk)
		return 0;
	/* 0 means accept all LOCAL addresses here, not all the world... */
	if (sk->rcv_saddr == 0)
		return 0;
	return 1;
}
#endif

#ifdef CONFIG_IP_MULTICAST
/*
 *	Multicasts and broadcasts go to each listener.
 */
static int udp_v4_mcast_deliver(struct sk_buff *skb, struct udphdr *uh,
				 u32 saddr, u32 daddr)
{
	struct sock *sk;
	int given = 0;

	SOCKHASH_LOCK();
	sk = udp_hash[ntohs(uh->dest) & (UDP_HTABLE_SIZE - 1)];
	sk = udp_v4_mcast_next(sk, uh->dest, saddr, uh->source, daddr, skb->dev);
	if(sk) {
		struct sock *sknext = NULL;

		do {
			struct sk_buff *skb1 = skb;

			sknext = udp_v4_mcast_next(sk->next, uh->dest, saddr,
						   uh->source, daddr, skb->dev);
			if(sknext)
				skb1 = skb_clone(skb, GFP_ATOMIC);

			if(skb1)
				udp_deliver(sk, skb1);
			sk = sknext;
		} while(sknext);
		given = 1;
	}
	SOCKHASH_UNLOCK();
	if(!given)
		kfree_skb(skb, FREE_READ);
	return 0;
}
#endif

/*
 *	All we need to do is get the socket, and then do a checksum. 
 */
 
int udp_rcv(struct sk_buff *skb, struct device *dev, struct options *opt,
	__u32 daddr, unsigned short len,
	__u32 saddr, int redo, struct inet_protocol *protocol)
{
  	struct sock *sk;
  	struct udphdr *uh;
	unsigned short ulen;
	int addr_type;

	/*
	 * If we're doing a "redo" (the socket was busy last time
	 * around), we can just queue the packet now..
	 */
	if (redo) {
		udp_queue_rcv_skb(skb->sk, skb);
		return 0;
	}

	/*
	 * First time through the loop.. Do all the setup stuff
	 * (including finding out the socket we go to etc)
	 */

	addr_type = IS_MYADDR;
	if(!dev || dev->pa_addr!=daddr)
		addr_type=ip_chk_addr(daddr);
		
	/*
	 *	Get the header.
	 */
	 
  	uh = (struct udphdr *) skb->h.uh;
  	
  	ip_statistics.IpInDelivers++;

	/*
	 *	Validate the packet and the UDP length.
	 */
	 
	ulen = ntohs(uh->len);
	
	if (ulen > len || len < sizeof(*uh) || ulen < sizeof(*uh)) 
	{
		NETDEBUG(printk("UDP: short packet: %d/%d\n", ulen, len));
		udp_statistics.UdpInErrors++;
		kfree_skb(skb, FREE_WRITE);
		return(0);
	}

	/* RFC1122 warning: According to 4.1.3.6, we MUST discard any */
	/* datagram which has an invalid source address, either here or */
	/* in IP. */
	/* Right now, IP isn't doing it, and neither is UDP. It's on the */
	/* FIXME list for IP, though, so I wouldn't worry about it. */
	/* (That's the Right Place to do it, IMHO.) -- MS */

	if (uh->check && (
		( (skb->ip_summed == CHECKSUM_HW) && udp_check(uh, len, saddr, daddr, skb->csum ) ) ||
		( (skb->ip_summed == CHECKSUM_NONE) && udp_check(uh, len, saddr, daddr,csum_partial((char*)uh, len, 0)))
			  /* skip if CHECKSUM_UNNECESSARY */
		         )
	   )
	{
		/* <mea@utu.fi> wants to know, who sent it, to
		   go and stomp on the garbage sender... */

	  /* RFC1122: OK.  Discards the bad packet silently (as far as */
	  /* the network is concerned, anyway) as per 4.1.3.4 (MUST). */

		NETDEBUG(printk("UDP: bad checksum. From %08lX:%d to %08lX:%d ulen %d\n",
		       ntohl(saddr),ntohs(uh->source),
		       ntohl(daddr),ntohs(uh->dest),
		       ulen));
		udp_statistics.UdpInErrors++;
		kfree_skb(skb, FREE_WRITE);
		return(0);
	}

	/*
	 *	These are supposed to be switched. 
	 */
	 
	skb->daddr = saddr;
	skb->saddr = daddr;

	len=ulen;

	skb->dev = dev;
	skb_trim(skb,len);

#ifdef CONFIG_IP_MULTICAST
	if (addr_type==IS_BROADCAST || addr_type==IS_MULTICAST)
		return udp_v4_mcast_deliver(skb, uh, saddr, daddr);
#endif
#ifdef CONFIG_IP_TRANSPARENT_PROXY
	if(skb->redirport)
		sk = udp_v4_proxy_lookup(saddr, uh->source, daddr, uh->dest,
					 dev->pa_addr, skb->redirport, dev);
	else
#endif
	sk = udp_v4_lookup(saddr, uh->source, daddr, uh->dest, dev);
	
	if (sk == NULL) 
  	{
  		udp_statistics.UdpNoPorts++;
		if (addr_type != IS_BROADCAST && addr_type != IS_MULTICAST) 
		{
			icmp_send(skb, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, 0, dev);
		}
		/*
		 * Hmm.  We got an UDP broadcast to a port to which we
		 * don't wanna listen.  Ignore it.
		 */
		skb->sk = NULL;
		kfree_skb(skb, FREE_WRITE);
		return(0);
  	}
	udp_deliver(sk, skb);
	return 0;
}

struct proto udp_prot = {
	(struct sock *)&udp_prot,	/* sklist_next */
	(struct sock *)&udp_prot,	/* sklist_prev */
	udp_close,			/* close */
	ip_build_header,		/* build_header */
	udp_connect,			/* connect */
	NULL,				/* accept */
	ip_queue_xmit,			/* queue_xmit */
	NULL,				/* retransmit */
	NULL,				/* write_wakeup */
	NULL,				/* read_wakeup */
	udp_rcv,			/* rcv */
	datagram_select,		/* select */
	udp_ioctl,			/* ioctl */
	NULL,				/* init */
	NULL,				/* shutdown */
	ip_setsockopt,			/* setsockopt */
	ip_getsockopt,			/* getsockopt */
	udp_sendmsg,			/* sendmsg */
	udp_recvmsg,			/* recvmsg */
	NULL,				/* bind */
	udp_v4_hash,			/* hash */
	udp_v4_unhash,			/* unhash */
	udp_v4_rehash,			/* rehash */
	udp_good_socknum,		/* good_socknum */
	udp_v4_verify_bind,		/* verify_bind */
	128,				/* max_header */
	0,				/* retransmits */
	"UDP",				/* name */
	0,				/* inuse */
	0				/* highestinuse */
};
