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
 * MkLinux
 */

/*
 * KKT machine independent request parameters
 */

#ifndef	_DIPC_KKT_REQUEST_H_
#define	_DIPC_KKT_REQUEST_H_

#include <mach/mach_types.h>
#include <mach/error.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <kern/assert.h>
#include <mach_kdb.h>
#include <mach/kkt_types.h>
#include <machine/kkt.h>
#include <device/io_scatter.h>


/*
 *	KKT exports an SPL level at which callbacks will happen
 *	from KKT to a client.  The client needs to know this
 *	level for synchronizing access to data structures shared
 *	between callbacks and thread context operations.
 *
 *	This spl level is referred to as splkkt, and may be
 *	used in the traditional fashion, e.g.,
 *		s = splkkt();
 *		...
 *		splx(s);
 *
 *	The definition comes from machine/kkt.h.
 */


/*
 * A transport request comes in two pieces: half of it is a
 * type designator, and half of it is the status of the
 * operation (and may be unnecessary, let's see).
 */
typedef struct transport_request {
	short		type;
	short		status;
} tran_req;

/*
 * Valid transport request types
 */
#define	REQUEST_SEND	0x001
#define	REQUEST_RECV	0x002
#define REQUEST_COMMANDS (REQUEST_SEND|REQUEST_RECV)
#define	REQUEST_SG	0x100

typedef io_scatter_t kkt_sglist_t;

/*
 * Primary request block.  Allocated by the user and handed to 
 * KKT_REQUEST.
 */
typedef	struct request_block	*request_block_t;
typedef	unsigned int		request_t;
typedef	kern_return_t		kkt_return_t;
typedef unsigned int		channel_t;


typedef void (*kkt_callback_t)(
			kkt_return_t,
			handle_t,
			request_block_t,
			boolean_t);


struct request_block {
	unsigned int	request_type;
	union {
		vm_offset_t	address;
		kkt_sglist_t	list;
	} data;		
	vm_size_t	data_length;
	kkt_callback_t	callback;
	unsigned int	args[2];
	request_block_t	next;
	transport_t	transport;
};

#define	NULL_REQUEST	(request_block_t)0

/*
 * Error codes
 */
#define	KKTSUB		9
#define	KKTERR(errno)	(err_kern | err_sub(KKTSUB) | errno)

#define KKT_MIN_ERRNO		KKTERR(0)    /* MIN ERRNO */
#define	KKT_SUCCESS		KKTERR(0)    /* operation succeeded */
#define	KKT_DATA_RECEIVED	KKTERR(1)    /* data received immediately,
						...no callback forthcoming */
#define	KKT_DATA_SENT		KKTERR(2)    /* data sent immediately, no
					        ...callback forthcoming */
#define	KKT_ERROR		KKTERR(3)    /* vague:  probably a lower-level
						...transport error */
#define	KKT_ABORT		KKTERR(4)    /* operation was aborted, handle
						...has aborted or is aborting */
#define	KKT_INVALID_ARGUMENT	KKTERR(5)    /* bad argument or range error */
#define	KKT_INVALID_HANDLE	KKTERR(6)    /* illegal handle value */
#define	KKT_NODE_DOWN		KKTERR(7)    /* can't reach node */
#define	KKT_RESOURCE_SHORTAGE	KKTERR(8)    /* requested or required resource
						...can't be allocated */
#define	KKT_NODE_PRESENT	KKTERR(9)    /* node already in map */
#define	KKT_MAP_EMPTY		KKTERR(10)   /* no nodes left in map */
#define	KKT_INVALID_REQUEST	KKTERR(11)   /* illegal operation requested */
#define	KKT_RELEASE		KKTERR(12)   /* resources no longer in use,
						...caller can free at will */
#define KKT_INVALID_CHANNEL	KKTERR(13)   /* illegal channel specifier */
#define	KKT_HANDLE_BUSY		KKTERR(14)   /* connection is active */
#define	KKT_HANDLE_UNUSED	KKTERR(15)   /* handle is unconnected */
#define	KKT_HANDLE_MIGRATED	KKTERR(16)   /* handle already migrated */
#define	KKT_CHANNEL_IN_USE	KKTERR(17)   /* channel already exists */
#define	KKT_HANDLE_TYPE		KKTERR(18)   /* handle is of wrong type,
					        ...possibly aborted */
#define	KKT_INVALID_NODE	KKTERR(19)   /* no such node */
#define KKT_MAX_ERRNO		KKTERR(19)   /* MAX ERRNO */


struct rpc_buf {
	unsigned int		*ulink;		/* user available field */
	handle_t		handle;		/* RPC handle */
	unsigned int		buff[1];	/* variable sized buffer */
};

typedef struct rpc_buf	*rpc_buf_t;

/* channel declarations */
#define CHAN_KKT		0	/* KKT internal channel */
#define CHAN_USER_KMSG		1	/* user kmsg channel */
#define CHAN_DIPC_RPC		2	/* dipc rpc, thread delivery */
#define CHAN_DIPC_RPC_INTR	3	/* dipc rpc, interrupt delivery */
#define CHAN_TEST_KMSG		4	/* kkt test channel */
#define CHAN_TEST_RPC		5	/* kkt rpc test channel */
#define CHAN_DIPC_MIGRATION	6	/* dipc migration threads */
#define CHAN_RESERVE_TEST	7	/* Raw RPC/RDMA testing */
#define CHAN_FLIPC		8	/* FLIPC channel */
#define CHAN_DEVICE_KT		9	/* KKT "kt" ethernet device */

/*
 *	KKT properties.  This is configuration information
 *	exported from KKT at run-time.
 */
typedef enum kkt_upcall {
	KKT_THREAD_UPCALLS,		/* all upcalls happen in thread context */
	KKT_INTERRUPT_UPCALLS,		/* all upcalls happen in interrupt context */
	KKT_MIXED_UPCALLS		/* upcalls can happen in EITHER context */
} kkt_upcall_t;

typedef struct kkt_properties {
	/*
	 *	How does KKT invoke client services during upcalls?
	 */
	kkt_upcall_t	kkt_upcall_methods;
	/*
	 *	Total number of *usable* bytes that a client can
	 *	fit into an inline KKT buffer during an RPC.
	 *	When sending data via KKT_SEND_CONNECT, KKT may
	 *	be able to optimize the send by cramming the
	 *	inline data into an RPC buffer (but specifying
	 *	larger amounts of inline data still works).
	 */
	vm_size_t	kkt_buffer_size;
} kkt_properties_t;


/*
 *	KKT interfaces.
 *
 *	Refer to the document, "Kernel to Kernel Transport Interface
 *	for the Mach Kernel", Open Software Foundation Research
 *	Institute, 1994.
 */

typedef void (*kkt_channel_deliver_t)(channel_t, handle_t, endpoint_t *,
				      vm_size_t, boolean_t, boolean_t);
typedef	void (*kkt_rpc_deliver_t)(channel_t, rpc_buf_t, boolean_t);
typedef	void *(*kkt_malloc_t)(channel_t, vm_size_t);
typedef void (*kkt_free_t)(channel_t, vm_offset_t, vm_size_t);
typedef void (*kkt_error_t)(kkt_return_t, handle_t, request_block_t);

void kkt_bootstrap_init(void);

/*
 *	Set up a general communications path, called a
 *	channel, which has a defined set of resources
 *	and operations.  The resources include buffers
 *	for sending and receiving operations.  The
 *	operations include upcalls from KKT to the
 *	client code.
 *
 *	Individual communications endpoints, or handles,
 *	are allocated in the context of a channel.
 *
 *	channel		identifier specified by caller
 *			("channel name")
 *	senders		number of sending buffers
 *	receivers	number of receiving buffers
 *	(*deliver)()	upcall for incoming data
 *	(*malloc)()	upcall when KKT needs memory
 *	(*free)()	upcall to free (*malloc)()'d memory
 */
kkt_return_t	KKT_CHANNEL_INIT(
			channel_t	channel,
			unsigned int	senders,
			unsigned int	receivers,
			kkt_channel_deliver_t deliver,
			kkt_malloc_t	malloc,
			kkt_free_t	free);

/*
 *	Allocate a handle within a channel.  A handle
 *	is a connection endpoint; both receiver and
 *	sender must have a handle in order to communicate.
 *	A handle is not associated with a node at this time.
 *	Requesting a handle may be thought of as requesting
 *	a class of service -- and possibly having to wait
 *	until resources become available.
 *
 *	channel		channel name
 *	uhandle		handle returned from KKT to caller
 *	(*error)()	upcall if transmission error encountered
 *	wait		TRUE=block awaiting handle availability
 *	must_request	TRUE=data flows only on receiver request
 *			(i.e., receiver-pull); FALSE=transport
 *			may optimize (possibly do sender-push)
 */
kkt_return_t	KKT_HANDLE_ALLOC(
			channel_t	channel,
			handle_t	*uhandle,
			kkt_error_t	error,
			boolean_t	wait,
			boolean_t	must_request);

/*
 *	Free up a handle.
 */
kkt_return_t	KKT_HANDLE_FREE(
			handle_t	handle);

/*
 *	Initiate arbitrary-length kernel-to-kernel
 *	data transfer.  The sender blocks until the
 *	receiver responds.  This call causes a (*deliver)()
 *	upcall on the receiver's channel.
 *
 *	handle		KKT-level communications endpoint
 *	node		destination for data
 *	endpoint	client-code's communications endpoint
 *	request		initial data to move
 *	more		hint as to whether more data will follow
 *	*ret		return values from KKT_CONNECT_REPLY
 */
kkt_return_t	KKT_SEND_CONNECT(
			handle_t	handle,
			node_name	node,
			endpoint_t	*endpoint,
			request_block_t request,
			boolean_t	more,
			kern_return_t	*ret);

/*
 *	Respond to KKT_SEND_CONNECT.
 *
 *	handle		KKT handle
 *	status		user-defined success/error indicator
 *	substatus	user-defined success/error indicator
 */
kkt_return_t	KKT_CONNECT_REPLY(
			handle_t	handle,
			kern_return_t	status,
			kern_return_t	substatus);

/*
 *	Enqueue a request block on the handle.  Initiate
 *	transmission if necessary, otherwise, transmit
 *	the data when the opportunity arises.
 *
 *	handle		KKT handle
 *	request		request block describing data to be transferred
 */
kkt_return_t	KKT_REQUEST(
			handle_t	handle,
			request_block_t	request);

/*
 *	Copy the entire RPC buffer contents into the
 *	caller's buffer.  The buffer and size are
 *	inferred from the handle; the caller takes
 *	responsibility for supplying a region large
 *	enough to contain the data to be copied.
 */
kkt_return_t	KKT_COPYOUT_RECV_BUFFER(
			handle_t	handle,
			char		*dest);

/*
 *	Abort ongoing operations on the specified handle.
 *	This need should rarely arise.
 *
 *	handle		KKT handle
 *	reason		status information to be supplied to
 *			any parties currently involved in a
 *			data transfer or otherwise somehow
 *			using this handle
 */
kkt_return_t	KKT_HANDLE_ABORT(
			handle_t	handle,
			kern_return_t	reason);

/*
 *	Obtain status information on a handle.
 *
 *	handle		KKT handle
 *	info		Whatever information KKT can
 *			supply about a handle
 */
kkt_return_t	KKT_HANDLE_INFO(
			handle_t	handle,
			handle_info_t	info);

/*
 *	Move the state associated with old_handle on
 *	node *node* to new_handle.  The new_handle
 *	must have already been created as a valid handle
 *	prior to invoking this call.
 *
 *	node		node from which handle is migrating
 *	old_handle	identifier of handle on node *node*
 *	new_handle	valid handle on current node
 */
kkt_return_t	KKT_HANDLE_MIGRATE(
			node_name	node,
			int		old_handle,
			handle_t	new_handle);

/*
 *	Allocate a map capable of tracking nodes
 *	within a KKT/NORMA domain.  This map need
 *	not be dense.  For instance, an Ethernet-
 *	based cluster may use sparse node names
 *	based on IP addresses.  A node map may be
 *	dynamic, i.e., adding a node to a map may
 *	require dynamic memory allocation.  For
 *	this reason, we define a "fallback" map;
 *	such a map is allocated with sufficient
 *	resources to track every node in the domain.
 *	(A fallback map is necessary when a node is
 *	very low on memory and has no other way to
 *	remember what is going on.)
 *
 *	map		pointer to a map created by KKT
 *	full		TRUE=caller requests creation of
 *			a fallback map
 */
kkt_return_t	KKT_MAP_ALLOC(
			node_map_t	*map,
			boolean_t	full);

/*
 *	Destroy a map.  The map need not be empty.
 *	There is no concept of a reference-counted
 *	map, so destruction takes place immediately.
 */
kkt_return_t	KKT_MAP_FREE(
			node_map_t	map);

/*
 *	Add a node to a map.  This call may block
 *	for memory allocation unless the map is
 *	known to be a fallback map.
 *
 *	map		KKT node map
 *	node		node to add to map (returns
 *			error if node already in map)
 */
kkt_return_t	KKT_ADD_NODE(
			node_map_t	map,
			node_name	node);

/*
 *	Remove the next node from a map.  The caller
 *	does not specify the node to be removed;
 *	rather, KKT returns the node_name of the
 *	next node in the map.
 *
 *	map		KKT node map
 *	node		name of node removed from map
 */
kkt_return_t	KKT_REMOVE_NODE(
			node_map_t	map,
			node_name	*node);

kkt_return_t	KKT_RPC_INIT(
			channel_t	channel,
			unsigned int	senders,
			unsigned int	receivers,
			kkt_rpc_deliver_t deliver,
			kkt_malloc_t	malloc,
			kkt_free_t	free,
			unsigned int	max_send_size);

kkt_return_t	KKT_RPC_HANDLE_ALLOC(
			channel_t	channel,
			handle_t	*uhandle,
			vm_size_t	size);

kkt_return_t	KKT_RPC_HANDLE_BUF(
			handle_t	handle,
			void		**userbuf);

kkt_return_t	KKT_RPC_HANDLE_FREE(
			handle_t	handle);

kkt_return_t	KKT_RPC(
			node_name	node,
			handle_t	handle);

kkt_return_t	KKT_RPC_REPLY(
			handle_t	handle);

kkt_return_t	KKT_PROPERTIES(
			kkt_properties_t *prop);

node_name	KKT_NODE_SELF(void);

boolean_t	KKT_NODE_IS_VALID(node_name node);

#endif	/* _DIPC_KKT_REQUEST_H_ */
