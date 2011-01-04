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
 *	File:	dipc/dipc_rpc.h
 *	Author:	Michelle Dominijanni
 *	Date:	1994
 *
 *	Definitions and functions for the DIPC RPC service.
 */


#include <kernel_test.h>

#include <dipc/dipc_counters.h>

/*
 *	--- The DIPC RPC story. ---
 *
 *	While KKT provides a general RPC service, when
 *	implementing DIPC code we prefer to think in terms
 *	of higher-level	procedure calls rather than in
 *	terms of RPC handles, argument marshalling and
 *	unmarshalling, etc.  These RPCs generally take a
 *	small number of arguments and return a success
 *	status plus, perhaps, a return result or two.
 *	In addition to failure modes from the RPC invocation
 *	itself, there are also failure modes from the
 *	underlying transport, such as "node dead".
 *
 *	A DIPC RPC always has the following form:
 *		kkt_return_t				transport error code
 *		drpc_<name>(	node,			target node for RPC
 *				in_arg1 .. in_argN,	input arguments
 *				return code,		RPC result:  success?
 *				out_arg1 .. out_argN)	return values from RPC
 *
 *	The server side of a DIPC RPC always has
 *	the following form:
 *		dipc_return_t				RPC result:  success?
 *		dproc_<name>(	in_arg1 .. in_argN,	input arguments
 *				out_arg1 .. out_argN)	output arguments
 *
 *	The dipc_return_t from dproc_<name> is returned,
 *	call by reference, to the caller of drpc_<name>.
 *
 *	Note that input and output arguments are optional,
 *	but must match in drpc_<name> and dproc_<name>.
 *
 *	Server-side routines may be handled at interrupt
 *	level (CHAN_DIPC_RPC_INTR) or in thread context
 *	(CHAN_DIPC_RPC).  An interrupt-level service
 *	obviously has additional constraints to obey;
 *	most DIPC RPCs are implemented in thread context.
 *
 *	--- Adding a new DIPC RPC. ---
 *
 *	We prefer to avoid adding an interface definition
 *	language or other automated IPC stub generator.
 *	Hence, adding a new RPC requires changing several
 *	places in dipc_rpc.h and dipc_rpc.c.
 *
 *	In dipc_rpc.h:
 *
 *	1.  Define a unique integer identifying the new
 *	service, DPROC_<NAME>.
 *
 *	2.  If necessary, update DPROC_MAX to be at least
 *	one more than the largest procedure call number.
 *
 *	3.  Define prototype definitions for drpc_<name>
 *	and dproc_<name>.  Input arguments should be call
 *	by value and output arguments must all be call
 *	by reference.
 *	
 *	4.  Define argument structures for input (struct
 *	<name>_in_args) and output (struct <name>_out_args)
 *	arguments, if any.  Note that the field definitions
 *	for struct <name>_out_args will NOT be call by
 *	reference; i.e., typically the output arguments in
 *	step 2 will all be pointers to variables while
 *	the output arguments in step 3 will just be the
 *	variables.
 *
 *	5.  If the DIPC RPC service is to be invoked directly
 *	from interrupt context, modify dipc_rpc_intr_args by
 *	adding definitions for the new service's input and output
 *	arguments.
 *
 *	6.  If the DIPC RPC service is to be invoked from thread
 *	context, modify dipc_rpc_args by adding definitions for
 *	the new service's input and output arguments.
 *
 *	That's it for dipc_rpc.h.
 *
 *	In dipc_rpc.c:
 *
 *	1.  If the DIPC RPC service is invoked directly
 *	from interrupt context, modify dipc_rpc_deliver()
 *	by adding a case DPROC_<name> and an invocation of
 *	dproc_<name>(arguments).  Be careful:  when coding
 *	output arguments, each one should be call by
 *	reference.
 *
 *	2.  If the DIPC RPC service is invoked in thread
 *	context, modify dipc_rpc_server() by adding a
 *	case DPROC_<name> and an invocation of dproc_<name>.
 *	Be careful:  when coding output arguments, each one
 *	should be call by reference.
 *
 *	3.  Add a definition for drpc_<name>_size.  This is
 *	the maximum size required for conveying input and
 *	output arguments for this RPC service.
 *
 *	4.  In drpc_compute_sizes(), add code to set the
 *	value of drpc_<name>_size.  This value should be
 *	the maximum of the sizes of <name>_in_args and
 *	<name>_out_args.  Plus sizeof(dipc_return_t).
 *
 *	5.  Write code for the client-side routine, drpc_<name>.
 *	Clone an existing drpc_ call and modify it as appropriate.
 *	Things you must pay attention to:
 *		A.  The KKT_RPC_HANDLE_ALLOC call must
 *		use CHAN_DIPC_RPC (for thread context calls)
 *		or CHAN_DIPC_RPC_INTR (for interrupt context).
 *		B.  The KKT_RPC_HANDLE_ALLOC call must
 *		use drpc_<name>_size.
 *		C.  Set args->common.procnum to DPROC_<name>.
 *		D.  Package up the input arguments, i.e.,
 *		move them from the stack into the args buffer.
 *		E.  For the case when the invocation of KKT_RPC
 *		returns KKT_SUCCESS, extract the dipc_return_t
 *		and any RPC output arguments from the args
 *		buffer, putting them in the places specified
 *		by the caller of drpc_<name>.
 *
 *	That's it for dipc_rpc.c.
 *
 *	In the appropriate file for the new DIPC RPC service,
 *	implement the code for the counterpart server routine,
 *	dproc_<name>(arguments).
 *
 *	That's it for defining a new DIPC RPC service.
 */


/*
 *	IN and OUT are defined for documenting
 *	argument-passing conventions and have
 *	no meaning to the compiler.
 */
#define	IN
#define	OUT


/*
 *	WAKEUP_SENDER:  Wakeup blocked remote sender.
 *	Optionally (status == DIPC_MSG_DESTROYED) cause
 *	the sender to destroy the message.
 */
#define DPROC_WAKEUP_SENDER		1

extern kkt_return_t	drpc_wakeup_sender(
			IN	node_name		node,
			IN	unsigned int		waitchan,
			IN	dipc_return_t		status,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_wakeup_sender(
			IN	unsigned int		waitchan,
			IN	dipc_return_t		status);

struct wakeup_sender_in_args {
	unsigned int	waitchan;
	dipc_return_t	status;
};


/*
 *	PORT_PROBE:  Query whether principal is alive.
 *	Returns either alive, dead or a new node to ask.
 */

#define	DPROC_PORT_PROBE		10

extern kkt_return_t	drpc_port_probe(
			IN	node_name		node,
			IN	uid_t			*uidp,
			OUT	dipc_return_t		*dr,
			OUT	node_name		*new_node);

extern dipc_return_t	dproc_port_probe(
			IN	uid_t			*uidp,
			OUT	node_name		*new_node);

struct port_probe_in_args {
	uid_t		uid;
};

struct port_probe_out_args {
	node_name	new_node;
};


/*
 *	Ask principal for more transits.
 *	Returns more transits or new node
 *	where principal has moved.
 */

#define	DPROC_ACQUIRE_TRANSITS		11

extern kkt_return_t	drpc_acquire_transits(
			IN	node_name		node,
			IN	uid_t			*uidp,
			OUT	dipc_return_t		*dr,
			OUT	long	     		*transits,
			OUT	node_name		*new_node);

extern dipc_return_t	dproc_acquire_transits(
			IN	uid_t			*uidp,
			OUT	long			*transits,
			OUT	node_name		*new_node);

struct acquire_transits_in_args {
	uid_t		uid;
};

struct acquire_transits_out_args {
	long		transits;
	node_name	new_node;
};


/*
 *	Return unused transits to principal.
 *	Only required for ports that have
 *	no more senders detection enabled.
 */

#define	DPROC_YIELD_TRANSITS		12

extern kkt_return_t	drpc_yield_transits(
			IN	node_name		node,
			IN	uid_t			*uidp,
			IN	long			transits,
                        OUT     dipc_return_t           *dr,
                        OUT     node_name               *new_node);

extern dipc_return_t	dproc_yield_transits(
			IN	uid_t			*uidp,
			IN	long			transits,
	                OUT     node_name               *new_node);

struct yield_transits_in_args {
	uid_t		uid;
	long		transits;
};

struct yield_transits_out_args {
	node_name	new_node;
};


/*
 *	Move an existing meta kmsg from one
 *	node to another as part of a port
 *	migration sequence.
 */

#define	DPROC_SEND_META_KMSG		20

extern kkt_return_t	drpc_send_meta_kmsg(
			IN	node_name		node,
			IN	uid_t			*uidp,
			IN	node_name		onode,
			IN	mach_msg_size_t		size,
			IN	handle_t		handle,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_send_meta_kmsg(
			IN	uid_t		*uidp,
			IN	node_name	rnode,
			IN	vm_size_t	size,
			IN	handle_t	handle);

struct send_meta_kmsg_in_args {
	uid_t		uid;
	node_name	rnode;
	vm_size_t	size;
	handle_t	handle;
};


/*
 *	Terminate port migration sequence.
 *	Send final port state to port's new node.
 */

#define	DPROC_MIGRATE_STATE		22

extern kkt_return_t	drpc_migrate_state(
			IN	node_name		mnode,
			IN	uid_t			*uidp,
			IN	mach_port_seqno_t	seqno,
			IN	mach_port_msgcount_t	qlimit,
			IN	long			transit,
			IN	long			sorights,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_migrate_state(
			IN	uid_t			*uidp,
			IN	mach_port_seqno_t	seqno,
			IN	mach_port_msgcount_t	qlimit,
			IN	long			transits,
			IN	long			remote_sorights);

struct migrate_state_in_args {
	uid_t			uid;
	mach_port_seqno_t	seqno;
	mach_port_msgcount_t	qlimit;
	long			transit;
	long			remote_sorights;
};


/*
 *	DN_REGISTER:  register for a deadname notification.
 */

#define	DPROC_DN_REGISTER		30

extern kkt_return_t	drpc_dn_register(
			IN	node_name		node,
			IN	uid_t			*uidp,
			IN	node_name		rnode,
			OUT	dipc_return_t		*dr,
			OUT	node_name		*new_node);

extern dipc_return_t	dproc_dn_register(
			IN	uid_t			*uidp,
			IN	node_name		rnode,
			OUT	node_name		*new_node);

struct dn_register_in_args {
	uid_t		uid;
	node_name	rnode;
};

struct dn_register_out_args {
	node_name	new_node;
};


/*
 *	DN_NOTIFY:  propagate a deadname notification.
 */

#define	DPROC_DN_NOTIFY			31

extern kkt_return_t	drpc_dn_notify(
			IN	node_name		node,
			IN	uid_t			*uidp);

extern dipc_return_t	dproc_dn_notify(
			IN	uid_t			*uidp);

struct dn_notify_in_args {
	uid_t		uid;
};


/*
 *	UNBLOCK_PORT:  Wakeup blocked senders on port.
 */
#define DPROC_UNBLOCK_PORT		40

extern kkt_return_t	drpc_unblock_port(
			IN	node_name		node,
			IN	uid_t			*uidp,
			IN	boolean_t		port_dead,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_unblock_port(
			IN	uid_t			*uidp,
			IN	boolean_t		port_dead);

struct unblock_port_in_args {
	uid_t		uid;
	boolean_t	port_dead;
};


/*
 *	REQUEST_CALLBACK:  ask for a callback when resource shortage has eased.
 */
#define DPROC_REQUEST_CALLBACK		41

extern kkt_return_t	drpc_request_callback(
			IN	node_name		node,
			IN	node_name		sending_node,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_request_callback(
			IN	node_name		sending_node);

struct request_callback_in_args {
	node_name	sending_node;
};


/*
 *	UNBLOCK_NODE:  Wakeup blocked senders waiting for receiving_node.
 */
#define DPROC_UNBLOCK_NODE		42

extern kkt_return_t	drpc_unblock_node(
			IN	node_name		node,
			IN	node_name		receiving_node,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_unblock_node(
			IN	node_name		receiving_node);

struct unblock_node_in_args {
	node_name	receiving_node;
};


/*
 *	GET_SPECIAL_PORT:  Query whether remote node has
 *	a port registered with the specified special port ID.
 *	On success, returns the remote special port's UID
 *	and whether it has NMS detection enabled.
 */

#define	DPROC_GET_SPECIAL_PORT		50

extern kkt_return_t	drpc_get_special_port(
			IN	node_name		node,
			IN	unsigned long		id,
			OUT	dipc_return_t		*dr,
			OUT	uid_t			*port_uid,
			OUT	boolean_t		*nms);

extern dipc_return_t	dproc_get_special_port(
			IN	unsigned long		id,
			OUT	uid_t			*port_uid,
			OUT	boolean_t		*nms);

struct get_special_port_in_args {
	unsigned long	id;
};

struct get_special_port_out_args {
	uid_t		port_uid;
	boolean_t	nms;
};

/*
 *	DRPC_ALIVE_NOTIFY: notify another node that we are alive
 *	and working.  Typically called at boot time.
 */

#define DPROC_ALIVE_NOTIFY	51

kkt_return_t
drpc_alive_notify(
	IN	node_name	node,
	OUT	dipc_return_t	*dr);

struct alive_notify_args {
	node_name	node;
};

#if	KERNEL_TEST

/*
 * 	TEST_SYNC:  mechanism for synchronizing
 *	internode unit tests.
 */

#define DPROC_TEST_SYNC			90

extern kkt_return_t	drpc_test_sync(
			IN	node_name		node,
			IN	unsigned int		test,
			IN      uid_t			*uidp,
			IN	unsigned int		opaque,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_test_sync(
			IN	node_name		node,
			IN	unsigned int		test,
			IN      uid_t			*uidp,
			IN	unsigned int		opaque);

struct test_sync_in_args {
	node_name	node;
	unsigned int	test;
	uid_t		uid;
	unsigned int	opaque;
};


/*
 *	TEST_INTR_RPC:  Test interrupt-level rpc service.
 */
#define DPROC_TEST_INTR_RPC		91

extern kkt_return_t	drpc_test_intr_rpc(
			IN	node_name		node,
			IN	unsigned int		arg_in,
			OUT	dipc_return_t		*dr);

extern dipc_return_t	dproc_test_intr_rpc(
			IN	unsigned int		arg_in);

struct test_intr_rpc_in_args {
	unsigned int	arg_in;
};


/*
 *	TEST_THREAD_RPC:  Test thread-level rpc service.
 */

#define	DPROC_TEST_THREAD_RPC		92

extern kkt_return_t	drpc_test_thread_rpc(
			IN	node_name		node,
			IN	unsigned int		arg_in,
			OUT	dipc_return_t		*dr,
			OUT	unsigned int		*arg_out);

extern dipc_return_t	dproc_test_thread_rpc(
			IN	unsigned int		arg_in,
			OUT	unsigned int		*arg_out);

struct test_thread_rpc_in_args {
	unsigned int	arg_in;
};

struct test_thread_rpc_out_args {
	unsigned int	arg_out;
};

#endif	/* KERNEL_TEST */

/*
 *	Bound on the highest allocated remote
 *	procedure call number.  This number
 *	may be used for allocating arrays.
 */
#define	DPROC_MAX			100

/* arguments for thread-level service */
typedef struct dipc_rpc_args {
	union {
		int		procnum;
		dipc_return_t	dr;
	} common;
	union {
		struct port_probe_in_args		port_probe_in;
		struct port_probe_out_args		port_probe_out;
		struct acquire_transits_in_args		acquire_transits_in;
		struct acquire_transits_out_args	acquire_transits_out;
		struct yield_transits_in_args		yield_transits_in;
		struct yield_transits_out_args		yield_transits_out;
		struct send_meta_kmsg_in_args		send_meta_kmsg_in;
		struct migrate_state_in_args		migrate_state_in;
		struct dn_register_in_args		dn_register_in;
		struct dn_register_out_args		dn_register_out;
		struct dn_notify_in_args		dn_notify_in;
		struct unblock_port_in_args		unblock_port_in;
		struct request_callback_in_args		request_callback_in;
		struct unblock_node_in_args		unblock_node_in;
		struct get_special_port_in_args		get_special_port_in;
		struct get_special_port_out_args	get_special_port_out;
		struct alive_notify_args		alive_args;
#if	KERNEL_TEST
		struct test_sync_in_args		test_sync_in;
		struct test_thread_rpc_in_args		test_thread_rpc_in;
		struct test_thread_rpc_out_args		test_thread_rpc_out;
#endif	/* KERNEL_TEST */
	} args;
} *drpc_args_t;

typedef struct dipc_rpc_intr_args {
	union {
		int		procnum;
		dipc_return_t	dr;
	} common;
	union {
		struct wakeup_sender_in_args		wakeup_sender_in;
#if	KERNEL_TEST
		struct test_intr_rpc_in_args		test_intr_rpc_in;
#endif	/* KERNEL_TEST */
	} args;
} *drpc_intr_args_t;


#if	DIPC_DO_STATS
extern void	dipc_rpc_stats(void);
#endif	/* DIPC_DO_STATS */
