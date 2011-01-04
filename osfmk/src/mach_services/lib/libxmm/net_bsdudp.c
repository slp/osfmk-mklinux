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
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 *	File:	net_bsdudp.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Berkeley UDP network support for xmm_net.c.
 */

#include <mach.h>
#include <cthreads.h>
#include "xmm_obj.h"

#include "net.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socketvar.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>

extern int errno;

#define	HEADER_MAGIC	0x9fa023b8
static long header_magic;

struct udp_header {
	unsigned long	magic;
	struct net_addr	daddr;
	unsigned long	did;
	struct net_addr	saddr;
	unsigned long	sid;
	struct net_msg	msg;
	unsigned long	length;
};

static int		udp_server_socket;

extern xmm_obj_t	mobj_lookup_by_addr();
extern xmm_obj_t	kobj_lookup_by_addr();

/* shared with tcp */
sock_setbufsize(s, buf)
	int s;
	int buf;
{
	int optval, best;
	char *ova = (char *) &optval;
	int ovs = sizeof(optval);
	
	for (best = 0, optval = 1024; optval < 1024*1024; optval += optval) {
		if (setsockopt(s, SOL_SOCKET, buf, ova, ovs) < 0) {
			char *bufname = (buf == SO_SNDBUF ? "snd" : "rcv");
			if (best) {
#if 0
				printf("%sbufsize set to %dK\n",
				       bufname, best / 1024);
#endif
			} else {
				perror("setsockopt");
				printf("%sbufsize not set\n", bufname);
			}
			return;
		}
		best = optval;
	}
}

static int
udp_setsockoptions(s)
	int s;
{
	/* set send buffer size */
	sock_setbufsize(s, SO_SNDBUF);

	/* set receive buffer size */
	sock_setbufsize(s, SO_RCVBUF);
}

static int
READ(buf, len)
	char *buf;
	int len;
{
	register int rv, startlen = len;
	struct sockaddr_in sin;
	int sin_size;
	
	for (;;) {
		sin_size = sizeof(sin);
		rv = recvfrom(udp_server_socket, buf, len, 0, (struct sockaddr *) &sin, &sin_size);
		if (rv == len) {
			return startlen;
		} else if (rv <= 0) {
			if (errno == EINTR) {
				/* only happens under ux server; a bug? */
				continue;
			}
			return rv;
		}
		buf += rv;
		len -= rv;
	}
}

static int
udp_read_loop()
{
	struct udp_header ih;
	int rv;
	vm_offset_t data;
	vm_size_t length;
	xmm_obj_t mobj;
	xmm_obj_t kobj;

	for (;;) {
		rv = READ((char *) &ih, sizeof(ih));
		if (rv == 0) {
			(void) close(udp_server_socket);
			cthread_exit(0);
		}
		if (rv < 0) {
			perror("udp_read_loop: read.header");
			(void) close(udp_server_socket);
			cthread_exit(0);
		}
		if (ih.magic != header_magic) {
			printf("udp_read_loop: bad magic 0x%lx (vs. 0x%lx)\n",
			       ih.magic, header_magic);
			(void) close(udp_server_socket);
			cthread_exit(0);
		}
		if (ih.msg.m_call) {
			mobj = mobj_lookup_by_addr(&ih.daddr, ntohl(ih.did),
						   TRUE);
			if (mobj == XMM_OBJ_NULL) {
				printf("m_read_loop: bad mobj\n");
				(void) close(udp_server_socket);
				cthread_exit(0);
			}
			kobj = kobj_lookup_by_addr(&ih.saddr, ntohl(ih.sid),
						   FALSE);
			if (kobj == XMM_OBJ_NULL) {
				printf("m_read_loop: bad kobj\n");
				(void) close(udp_server_socket);
				cthread_exit(0);
			}
		} else {
			kobj = kobj_lookup_by_addr(&ih.daddr, ntohl(ih.did),
						   TRUE);
			if (kobj == XMM_OBJ_NULL) {
				printf("k_read_loop: bad kobj\n");
				(void) close(udp_server_socket);
				cthread_exit(0);
			}
			mobj = mobj_lookup_by_addr(&ih.saddr, ntohl(ih.sid),
						   FALSE);
		}
		ih.msg.id = ntohl(ih.msg.id);
		length = ntohl(ih.length);
		if (length > 0) {
			vm_allocate_dirty(&data);
			rv = READ(data, length);
			if (rv != length) {
				perror("udp_read_loop: read.data");
			}
		} else {
			data = (vm_offset_t) 0;
		}
		master_lock();
		net_dispatch(mobj, kobj, &ih.msg, data, length, XMM_OBJ_NULL);
		master_unlock();
	}
}

static int
SEND(addr, data, length)
	net_addr_t addr;
	vm_offset_t data;
	int length;
{
	struct sockaddr_in sin;
	int rv;
	
	bzero((char *)&sin, sizeof(sin));
	sin.sin_port		= addr->a[0];
	sin.sin_addr.s_addr	= addr->a[1];
	sin.sin_family		= AF_INET;
	rv = sendto(udp_server_socket, (char *) data, length, 0, (struct sockaddr *) &sin, sizeof(sin));
	if (rv != length) {
		perror("SEND: sendto");
	}
	return length;		/* XXX a palpable lie */
}

/*
 *  Write data to remote obj.
 */
static
udp_send(daddr, did, saddr, sid, msg, data, length)
	net_addr_t daddr;
	int did;
	net_addr_t saddr;
	int sid;
	net_msg_t msg;
	vm_offset_t data;
	vm_size_t length;
{
	int rv;
	struct udp_header ih;
	
	ih.magic	= header_magic;
	ih.daddr	= *daddr;
	ih.did		= htonl(did);
	ih.saddr	= *saddr;
	ih.sid		= htonl(sid);
	ih.msg		= *msg;
	ih.msg.id	= htonl(msg->id);
	if (data) {
		ih.length = htonl(length);
	} else {
		ih.length = 0;
	}
	rv = SEND(daddr, (char *)&ih, sizeof(ih));
	if (rv != sizeof(ih)) {
		printf("udp_send: SEND.header %d\n", rv);
	}
	if (data) {
		rv = SEND(daddr, data, length);
		if (rv != length) {
			printf("udp_send: SEND.data %d\n", rv);
		}
		vm_deallocate_dirty(data);
	}
	return KERN_SUCCESS;
}

kern_return_t
udp_init(np)
	net_proto_t np;
{
	struct sockaddr_in sin;
	struct hostent *local, *gethostbyname();
	char localname[128];
	int port;
	
	/*
	 *  Create a UDP socket; bind it to a port
	 */
	udp_server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_server_socket < 0) {
		perror("get_server_socket: socket");
		cthread_exit(0);
	}
	udp_setsockoptions(udp_server_socket);
	if (gethostname(localname, sizeof(localname)) < 0) {
		perror("get_server_socket: gethostname");
		cthread_exit(0);
	}
	local = gethostbyname(localname);
	if (local == 0) {
		perror("get_server_socket: gethostbyname");
		cthread_exit(0);
	}
	bzero((char *) &sin, sizeof(sin));
	sin.sin_family = AF_INET;
	for (port = IPPORT_RESERVED;; port++) {
		sin.sin_port = htons(port);
		if (bind(udp_server_socket, (struct sockaddr *)&sin,
			 sizeof(sin)) == 0) {
			break;
		}
		if (errno == EADDRINUSE) {
			continue;
		}
		perror("get_server_socket: bind");
		cthread_exit(0);
	}

	/*
	 * Spawn thread to receive messages on server socket.
	 */
	header_magic = htonl(HEADER_MAGIC);
	cthread_detach(cthread_fork((cthread_fn_t) udp_read_loop, 0));

	/*
	 * Initialize np.
	 */
	np->server_addr.a[0] = sin.sin_port;
	np->server_addr.a[1] = *(int *)local->h_addr;
	np->server_addr.a[2] = 0;
	np->server_addr.a[3] = 0;
	np->server_send = udp_send;
	return KERN_SUCCESS;
}
