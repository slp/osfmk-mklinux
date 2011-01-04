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
 *	File:	net_bsdtcp.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Berkeley TCP network support for xmm_net.c.
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
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>

extern int errno;

typedef struct tcp_socket	*socket_t;

#define	SOCKET_NULL		((socket_t) 0)

#define	HEADER_MAGIC	0x9f5a023b
static long header_magic;

struct tcp_header {
	unsigned long	magic;
	struct net_addr	daddr;
	unsigned long	did;
	struct net_addr	saddr;
	unsigned long	sid;
	struct net_msg	msg;
	unsigned long	length;
};

struct tcp_socket {
	int		s;	/* socket: -1 if invalid, else valid */
	struct net_addr	addr;
	socket_t	next;
	struct mutex	mutex;
};

/* XXXXXXXXX DEFINITELY should go away! */
struct tcp_read_loop_args {
	int			s;
	struct sockaddr_in	sin;
	int			sin_size;
};

extern xmm_obj_t	mobj_lookup_by_addr();
extern xmm_obj_t	kobj_lookup_by_addr();

static boolean_t	duplex_sockets = FALSE;
static socket_t		socket_list = (socket_t) 0;

static socket_t
tcp_add_sin(sin, s)
	struct sockaddr_in *sin;
{
	socket_t so;
	
	so = (socket_t) malloc(sizeof(*so));
	so->addr.a[0] = htonl((unsigned long) ntohs(sin->sin_port));
	so->addr.a[1] = sin->sin_addr.s_addr;
	so->addr.a[2] = 0;
	so->addr.a[3] = 0;
	so->s = s;
	so->next = socket_list;
	mutex_init(&so->mutex);
	socket_list = so;
	return so;
}

static int
tcp_setsockoptions(s)
	int s;
{
	int optval;
	char *ova = (char *) &optval;
	int ovs = sizeof(optval);
	
	/* turn off delay of small packets */
	optval = 1;
	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, ova, ovs) < 0) {
		perror("tcp_setsockoptions: setsockopt");
	}
	if (optval != 1) {
		printf("tcp_setsockoptions: setsockopt ignored\n");
	}

	/* set send buffer size */
	sock_setbufsize(s, SO_SNDBUF);

	/* set receive buffer size */
	sock_setbufsize(s, SO_RCVBUF);
}

/*
 *  Returns a socket descriptor for sin; create one if necessary.
 */
static socket_t
tcp_lookup(addr)
	net_addr_t addr;
{
	socket_t so;
	int s;
	struct sockaddr_in sin;
	
	for (so = socket_list; so; so = so->next) {
		if (NET_ADDR_EQ(addr, &so->addr)) {
			return so;
		}
	}
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("tcp_lookup: socket");
		return SOCKET_NULL;
	}
	bzero((char *) &sin, sizeof(sin));
	sin.sin_port = htons(ntohl(addr->a[0]));
	sin.sin_addr.s_addr = addr->a[1];
	sin.sin_family = AF_INET;
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		return SOCKET_NULL;
	}
	tcp_setsockoptions(s);
	return tcp_add_sin(&sin, s);
}

static int
READ(s, buf, len)
	int s;
	char *buf;
	int len;
{
	register int rv, startlen = len;
	
	for (;;) {
		rv = read(s, buf, len);
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
tcp_read_loop(ci)
	struct tcp_read_loop_args *ci;
{
	struct tcp_header ih;
	int s, rv;
	vm_offset_t data;
	vm_size_t length;
	xmm_obj_t mobj;
	xmm_obj_t kobj;

	s = ci->s;
	free((char *) ci);
	for (;;) {
		rv = READ(s, (char *) &ih, sizeof(ih));
		if (rv == 0) {
			(void) close(s);
			cthread_exit(0);
		}
		if (rv < 0) {
			perror("tcp_read_loop: read.header");
			(void) close(s);
			cthread_exit(0);
		}
		if (ih.magic != header_magic) {
			printf("tcp_read_loop: bad magic 0x%lx (vs. 0x%lx)\n",
			       ih.magic, header_magic);
			(void) close(s);
			cthread_exit(0);
		}
		if (ih.msg.m_call) {
			mobj = mobj_lookup_by_addr(&ih.daddr, ntohl(ih.did),
						   TRUE);
			if (mobj == XMM_OBJ_NULL) {
				printf("m_read_loop: bad mobj\n");
				(void) close(s);
				cthread_exit(0);
			}
			kobj = kobj_lookup_by_addr(&ih.saddr, ntohl(ih.sid),
						   FALSE);
			if (kobj == XMM_OBJ_NULL) {
				printf("m_read_loop: bad kobj\n");
				(void) close(s);
				cthread_exit(0);
			}
		} else {
			kobj = kobj_lookup_by_addr(&ih.daddr, ntohl(ih.did),
						   TRUE);
			if (kobj == XMM_OBJ_NULL) {
				printf("k_read_loop: bad kobj\n");
				(void) close(s);
				cthread_exit(0);
			}
			mobj = mobj_lookup_by_addr(&ih.saddr, ntohl(ih.sid),
						   FALSE);
		}
		ih.msg.id = ntohl(ih.msg.id);
		length = ntohl(ih.length);
		if (length > 0) {
			vm_allocate_dirty(&data);
			rv = READ(s, (char *) data, length);
			if (rv != length) {
				perror("tcp_read_loop: read.data");
			}
		} else {
			data = (vm_offset_t) 0;
		}
		master_lock();
		net_dispatch(mobj, kobj, &ih.msg, data, length, XMM_OBJ_NULL);
		master_unlock();
	}
}

/*
 *  Write data to remote obj.
 */
static
tcp_send(daddr, did, saddr, sid, msg, data, length)
	net_addr_t daddr;
	int did;
	net_addr_t saddr;
	int sid;
	net_msg_t msg;
	vm_offset_t data;
	vm_size_t length;
{
	socket_t so;
	int rv;
	struct tcp_header ih;
	
	so = tcp_lookup(daddr);
	if (so == SOCKET_NULL) {
		printf("tcp_lookup: addr 0x%lx\n", daddr->a[1]);
		return KERN_FAILURE;
	}
	mutex_lock(&so->mutex);
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
	rv = write(so->s, (char *)&ih, sizeof(ih));
	if (rv != sizeof(ih)) {
		printf("tcp_send: write.header %d\n", rv);
	}
	if (data) {
		rv = write(so->s, (char *) data, (int) length);
		if (rv != length) {
			printf("tcp_send: write.data %d\n", rv);
		}
		vm_deallocate_dirty(data);
	}
	mutex_unlock(&so->mutex);
	return KERN_SUCCESS;
}

static int
tcp_accept_loop(s)
	int s;
{
	struct tcp_read_loop_args *ci;
	
	for (;;) {
		ci = (struct tcp_read_loop_args *) malloc(sizeof(*ci));
		bzero((char *) &ci->sin, sizeof(ci->sin));
		ci->sin_size = sizeof(ci->sin);
		ci->s = accept(s, (struct sockaddr *)&ci->sin, &ci->sin_size);
		if (ci->s < 0) {
			if (errno != EINTR) {
				perror("tcp_accept_loop: accept");
			}
			continue;
		}
		tcp_setsockoptions(ci->s);
		if (duplex_sockets) {	/* XXX */
			(void) tcp_add_sin(&ci->sin, ci->s);
		}
		cthread_detach(cthread_fork((cthread_fn_t) tcp_read_loop, (any_t) ci));
	}
}

kern_return_t
tcp_init(np)
	net_proto_t np;
{
	struct sockaddr_in sin;
	struct hostent *local, *gethostbyname();
	char localname[128];
	extern int errno;
	int port, s;
	
	/*
	 * Create server socket.
	 */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("tcp_init: socket");
		return KERN_FAILURE;
	}
	tcp_setsockoptions(s);
	if (gethostname(localname, sizeof(localname)) < 0) {
		perror("tcp_init: gethostname");
		return KERN_FAILURE;
	}
	bzero((char *) &sin, sizeof(sin));
	local = gethostbyname(localname);
	if (local == 0) {
		perror("tcp_init: gethostbyname");
		return KERN_FAILURE;
	}
	sin.sin_addr.s_addr = *(long *)local->h_addr;
	sin.sin_family = AF_INET;
	for (port = IPPORT_RESERVED;; port++) {
		sin.sin_port = htons(port);
		if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
			break;
		}
		if (errno == EADDRINUSE) {
			continue;
		}
		perror("tcp_init: bind");
		return KERN_FAILURE;
	}
	if (listen(s, 5) < 0) {
		perror("tcp_init: listen");
		return KERN_FAILURE;
	}

	/*
	 * Spawn thread to accept connections on server socket.
	 */
	header_magic = htonl(HEADER_MAGIC);
	cthread_detach(cthread_fork((cthread_fn_t) tcp_accept_loop, (any_t) s));

	/*
	 * Initialize np.
	 */
	np->server_addr.a[0] = htonl((unsigned long) ntohs(sin.sin_port));
	np->server_addr.a[1] = sin.sin_addr.s_addr;
	np->server_addr.a[2] = 0;
	np->server_addr.a[3] = 0;
	np->server_send = tcp_send;
	return KERN_SUCCESS;
}
