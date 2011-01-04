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
 *	File:	net.h
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Definitions for xmm_net.c and various protocols.
 */

/*
 * Eventually will have to decide whether we want rpcs or just ipc.
 * E.g., return values from (simple)routines.
 */

#define	M_INIT_ID		0
#define	M_TERMINATE_ID		1
#define	M_COPY_ID		2
#define	M_DATA_REQUEST_ID	3
#define	M_DATA_UNLOCK_ID	4
#define	M_DATA_WRITE_ID		5
#define	M_LOCK_COMPLETED_ID	6
#define	M_SUPPLY_COMPLETED_ID	7
#define	M_DATA_RETURN_ID	8
#define	M_CHANGE_COMPLETED_ID	9

#define	K_DATA_PROVIDED_ID	10
#define	K_DATA_UNAVAILABLE_ID	11
#define	K_GET_ATTRIBUTES_ID	12
#define	K_LOCK_REQUEST_ID	13
#define	K_DATA_ERROR_ID		14
#define	K_SET_ATTRIBUTES_ID	15
#define	K_DESTROY_ID		16
#define	K_DATA_SUPPLY_ID	17

typedef struct net_msg		*net_msg_t;
typedef struct net_addr		*net_addr_t;
typedef struct net_proto	*net_proto_t;

struct net_addr {
	unsigned long	a[4];
};

#define	NET_ADDR_EQ(na1, na2)		\
	((na1)->a[0] == (na2)->a[0] &&	\
	 (na1)->a[1] == (na2)->a[1] &&	\
	 (na1)->a[2] == (na2)->a[2] &&	\
	 (na1)->a[3] == (na2)->a[3])

#define	NET_ADDR_NULL(na)		\
	((na)->a[0] == 0 &&		\
	 (na)->a[1] == 0 &&		\
	 (na)->a[2] == 0 &&		\
	 (na)->a[3] == 0)

struct net_msg {
	unsigned long	id;
	unsigned long	m_call;		/* boolean */
	unsigned long	arg1;
	unsigned long	arg2;
	unsigned long	arg3;
	unsigned long	arg4;
	unsigned long	arg5;
};

struct net_proto {
	kern_routine_t	server_init;
	kern_routine_t	server_send;
	struct net_addr	server_addr;
	char *		server_name;
};

#if 0
#ifdef	lint
#undef	ntohl
#undef	htonl
static unsigned long __ntohl(x) unsigned long x; { return x; }
static unsigned long __htonl(x) unsigned long x; { return x; }
#define	ntohl(x) __ntohl((unsigned long)(x))
#define	htonl(x) __htonl((unsigned long)(x))
#endif	/* lint */
#endif
