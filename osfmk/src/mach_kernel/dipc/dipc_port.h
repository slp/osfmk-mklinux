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
 *	File:	dipc/dipc_port.h
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Definitions for distributed kernel ports.
 */

#ifndef	_DIPC_DIPC_PORT_H_
#define	_DIPC_DIPC_PORT_H_

#include <ipc/ipc_port.h>
#include <dipc/dipc_types.h>
#include <dipc/dipc_counters.h>

/*
 *	The struct dipc_port is an extension of the local
 *	port structure (struct ipc_port).  It contains
 *	all of the data necessary for representing a port
 *	in DIPC.  Some of these fields are required for both
 *	principals and proxies; others are mutually exclusive.
 *
 * State bits:
 *	dipus_proxy
 *		When TRUE, this port represents a remote
 *		port.  When false, this port is local.
 *	dipus_special
 *		This port has been entered in the DIPC
 *		special port table, and thus can be
 *		found via norma_get_special_port.
 *	dipus_forward
 *		This port has become a forwarding proxy for
 *		a migrated receive right.
 *
 * Migration state bits:
 *	dipus_ms_block
 *		The port is migrating or otherwise involved
 *		in a state change.  Regular operations (such
 *		as message enqueuing) should block on the
 *		address of the port until this bit clears.
 *	dipus_ms_convert
 *		A message enqueued on this port must first
 *		be converted to DIPC_FORMAT.
 *
 *	As a receive right migrates, the following states
 *	are possible:
 *				proxy	forward	block	convert	migrate_wait
 *	Original Principal:
 *		Normal 		  0	   0	  0	   0	     0
 *		Right in MSG	  0	   0	  0	   1	     0
 *		Right Migrating	  1	   1	  1	   1	     0
 *		Migration Done	  1	   1	  0	   1	     0
 *		(The principal has become a forwarding proxy.)
 *
 *	Original Proxy:
 *		Forwarding Proxy  1	   1	  0	   1	     0
 *		Regular Proxy	  1	   0	  0	   1	     0
 *		Right Migrating	  0	   0	  1	   0	     1
 *		Migration Done	  0	   0	  0	   0	     0
 *		(The proxy becomes the principal.)
 *
 *	All other states are undefined.
 */

typedef struct dipc_port {
	/*
	 *	State flags.
	 */
	union {
	    struct {
		unsigned int	dipus_proxy:1,	    /* proxy or principal */
				dipus_special:1,    /* port has special id */
				dipus_forward:1,    /* migrated recv right */
				dipus_migrate_wait:1, /* port migrating now */
				dipus_ms_block:1,   /* wait for op to finish */
	    			dipus_ms_convert:1, /* use DIPC_FORMAT msgs */
				dipus_map_alloc:1,  /* calling KKT_MAP_ALLOC */
				dipus_map_wait:1,   /* waiting for map alloc */
	    			dipus_rs_block:1,   /* remote res. shortage */
	    			dipus_resending:1;  /* resend in progress */
	    } dip_us;
	    unsigned int	dipu_flag_bits;
	} dip_u;

	/*
	 * The following is the count (++) of queue_full messages returned
	 * by the receiving node and and queue_unblock (--) messages.
	 */

	long		dip_qf_count;

	/*
	 *	Port's network-wide name.  For send and
	 *	send-once rights, we must also include
	 *	the name of the destination node where we
	 *	think the receive right resides, as the
	 *	node_name in the uid is always and forever
	 *	that of the originating node, which may no
	 *	longer be the residence node.
	 *
	 *	The source node information exists for a
	 *	migrating principal:  it indicates where
	 *	the principal is coming from.
	 */
	uid_t		dip_uid;
	union {
		node_name	dipu2_dest_node;
		node_name	dipu2_source_node;
	} dip_u2;

	/*
	 *	Transit management state.
	 */
	long		dip_transit;
	long		dip_remote_sorights;

	/*
	 *	The following fields are used by the port name
	 *	hash table.  They may only be used under spl.
	 */
	ipc_port_t	dip_hash_next;
	unsigned int	dip_ref_count;

	/*
	 * list of nodes with blocked senders because of QUEUE_FULL on
	 * this port.  (principal)
	 */
	node_map_t	dip_callback_map;

	/*
	 * link field for list of ports to be unblocked by
	 * service thread.  (proxy)
	 */
	ipc_port_t	dip_restart_list;

	/*
	 * dipc_node_wait is a pointer to the node wait structure that this
	 * port is queued on becuase of RESOURCE_SHORTAGE on receiving node.
	 * dipc_node_wait_list is the link field for the list of ports in the
	 * node wait struct.  (proxy)
	 */
	unsigned int	*dip_node_wait;
	ipc_port_t	dip_node_wait_list;
	
#if	DIPC_DO_STATS
	unsigned int	dip_queue_full;	/* queue full hits */
	unsigned int	dip_queue_max;	/* longest queue length to date */
	unsigned int	dip_enqueued;	/* messages deposited on queue */
	unsigned int	dip_thread_hit;	/* messages handed to waiting threads */
#endif	/* DIPC_DO_STATS */

#if	MACH_ASSERT
	long		dip_inited;
	long		dip_spares[3];
#endif	/* MACH_ASSERT */
} *dipc_port_t;

#define	DIP_NULL		((dipc_port_t) 0)

#define	dip_proxy		dip_u.dip_us.dipus_proxy
#define	dip_special		dip_u.dip_us.dipus_special
#define	dip_forward		dip_u.dip_us.dipus_forward
#define	dip_migrate_wait	dip_u.dip_us.dipus_migrate_wait
#define	dip_ms_block		dip_u.dip_us.dipus_ms_block
#define	dip_ms_convert		dip_u.dip_us.dipus_ms_convert
#define	dip_map_alloc		dip_u.dip_us.dipus_map_alloc
#define	dip_map_wait		dip_u.dip_us.dipus_map_wait
#define	dip_rs_block		dip_u.dip_us.dipus_rs_block
#define	dip_resending		dip_u.dip_us.dipus_resending
#define	dip_flag_bits		dip_u.dipu_flag_bits

#define	dip_dest_node		dip_u2.dipu2_dest_node
#define	dip_source_node		dip_u2.dipu2_source_node

#define	ip_flag_bits		ip_dipc->dip_flag_bits
#define	ip_proxy		ip_dipc->dip_proxy
#define	ip_special		ip_dipc->dip_special
#define	ip_forward		ip_dipc->dip_forward
#define	ip_migrate_wait		ip_dipc->dip_migrate_wait
#define	ip_ms_block		ip_dipc->dip_ms_block
#define	ip_ms_convert		ip_dipc->dip_ms_convert
#define	ip_map_alloc		ip_dipc->dip_map_alloc
#define	ip_map_wait		ip_dipc->dip_map_wait
#define	ip_rs_block		ip_dipc->dip_rs_block
#define	ip_qf_count		ip_dipc->dip_qf_count
#define	ip_resending		ip_dipc->dip_resending
#define	ip_uid			ip_dipc->dip_uid
#define	ip_dest_node		ip_dipc->dip_dest_node
#define	ip_source_node		ip_dipc->dip_source_node
#define	ip_transit		ip_dipc->dip_transit
#define	ip_remote_sorights	ip_dipc->dip_remote_sorights
#define	ip_hash_next		ip_dipc->dip_hash_next
#define	ip_ref_count		ip_dipc->dip_ref_count
#define	ip_callback_map		ip_dipc->dip_callback_map
#define	ip_restart_list		ip_dipc->dip_restart_list
#define	ip_node_wait_list	ip_dipc->dip_node_wait_list
#define	ip_node_wait		ip_dipc->dip_node_wait

#if	DIPC_DO_STATS
#define	ip_queue_full		ip_dipc->dip_queue_full
#define	ip_queue_max		ip_dipc->dip_queue_max
#define	ip_enqueued		ip_dipc->dip_enqueued
#define	ip_thread_hit		ip_dipc->dip_thread_hit
#endif	/* DIPC_DO_STATS */

#if	MACH_ASSERT
#define	ip_loaned_transits	ip_dipc->dip_loaned_transits
#endif	/* MACH_ASSERT */

#if	MACH_ASSERT
#define	DIP_DEAD		((dipc_port_t) 0xdeadbeef)
#endif	/* MACH_ASSERT */


#define DIPC_FAKE_DNREQUEST		(ipc_port_t)0x33333333

/*
 *	Determine whether a port is part of DIPC.  Note that
 *	once a port has entered DIPC, we don't expect it to
 *	ever leave again until it is destroyed.  Also note
 *	that this check is only as good as the lock it is
 *	held under.
 */
#if	MACH_ASSERT
#define	DIPC_INIT_POINTER(port)	((port)->ip_dipc = DIP_DEAD)
#define	DIPC_IS_DIPC_PORT(port)	(((port)->ip_dipc == DIP_NULL ||	\
				 (port)->ip_dipc == DIP_DEAD) ? FALSE : TRUE)
#else	/* MACH_ASSERT */
#define	DIPC_INIT_POINTER(port)	((port)->ip_dipc = DIP_NULL)
#define	DIPC_IS_DIPC_PORT(port)	((port)->ip_dipc == DIP_NULL ? FALSE : TRUE)
#endif	/* MACH_ASSERT */


/*
 *	Determine whether a port is local or remote.  However, no
 *	guarantee is made that the port remains in place after
 *	the call completes.
 *
 *	This call works because if the port isn't in DIPC then it
 *	must be local; if the port *is* in DIPC, then we can be
 *	sure that the ip_dipc structure will stay in place because
 *	the caller holds a reference on the port.
 *
 *	The caller need not hold a lock because the check is an
 *	atomic read of a single memory location.  However, if the
 *	caller doesn't hold a lock, there is no guarantee preventing
 *	the port from migrating while the check is in progress.
 */
#define	DIPC_IS_PROXY(port)	(DIPC_IS_DIPC_PORT(port) && (port)->ip_proxy)


#define DIPC_ORIGIN_NODE(port)	(DIPC_IS_DIPC_PORT(port) ?		\
				 (port)->ip_dipc->dip_uid.origin :	\
				 dipc_node_self())

/*
 *	Careful when using dipc_port_init:  as long as
 *	assign_uid==TRUE, dipc_port_init protects against
 *	all races when entering a port into DIPC.  However,
 *	when assign_uid==FALSE, the caller must guarantee
 *	that no other context can somehow obtain the port
 *	in it's partially-initialized state and then extract
 *	what may be a bogus UID from the port.
 */
extern dipc_return_t	dipc_port_init(
				ipc_port_t		port,
				boolean_t		assign_uid);

extern dipc_return_t	dipc_port_free(
				ipc_port_t		port);

extern dipc_return_t	dipc_port_copyin(
				ipc_port_t		port,
				mach_msg_type_name_t	type,
				uid_t			*uidp);

extern dipc_return_t	dipc_port_copyout(
				uid_t			*uidp,
				mach_port_type_t	type,
				node_name		rnode,
				boolean_t		destination,
				boolean_t		nms,
				ipc_port_t		*return_port);

extern dipc_return_t	dipc_port_dest_done(
				ipc_port_t		port,
				mach_port_type_t	type);


extern dipc_return_t	dipc_port_update(
				ipc_port_t		port);

#if	MACH_KDB
extern void		dipc_port_print(
				ipc_port_t		port);
#endif	/* MACH_KDB */

#endif	/* _DIPC_DIPC_PORT_H_ */
