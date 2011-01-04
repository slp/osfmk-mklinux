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

#include <mach_kdb.h>
#include <kern/assert.h>
#include <kern/lock.h>

#include <mach/boolean.h>
#include <mach/kkt_request.h>
#include <mach/message.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_rpc.h>
#include <dipc/port_table.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <kern/lock.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <kern/ipc_sched.h>
#include <ddb/tr.h>
#include <machine/kkt_map.h>

#define	TR_ENABLED 1
#if	TR_ENABLED
#include <ddb/tr.h>
#else
#define	TR_DECL(a)
#define	tr1(a)
#define	tr2(a,b)
#define	tr3(a,b,c)
#define	tr4(a,b,c,d)
#define	tr5(a,b,c,d,e)
#endif

/*
 * HOW The OOL Engine Works
 *
 * The principal data structure is the message_progress structure,
 * abbreviated "mp" in most of the code.  Most of the primary fields
 * are self explanatory, but a brief rundown here is useful:
 *
 *	prep_next: used to enqueue an mp when necessary.  This happens
 *		primarily when we need to hand it off to thread
 *		context where the prep_thread will work on it, enqueueing
 *		on the dipc_msg_prep_queue.
 *	kmsg:	Pointer to the kmsg.
 *	lock:	MP lock to protect state and non-structure fields.
 *	state:	Various states it can be in:
 *		XMIT_READY: Ready to go
 *		XMIT_IDLE: Sitting on the idle chain in the mp
 *		XMIT_PREP_REQ: No waiting thread context.
 *		XMIT_PREP_QUEUE: Enqueued on dipc_msg_prep_queue.
 *		XMIT_QUEUEING: A request is actively having data put
 *			into it: to preserve proper ordering, other
 *			requests must wait until this is clear.
 *		XMIT_DONE: Message is completely sent, including trailer.
 *		XMIT_RECV: Receiving if set.
 *	chain_count: number of allocated requests.
 *	idle_count:  number of idle requests in idle_chain array.
 *	idle_chain:  requests that are awaiting services (another request
 *		is getting loaded), or is finished are put into this
 *		"docking" area.
 *	msg_size: total size of message in bytes.  Filled out by caller.
 *	page_err_page: If a VM error is encountered, a structure is
 *		allcoated and information placed therin.  Still untested.
 *	error_count: Total number of VM errors.
 *	copy_list: Pointer to head of copy list (linked list of copy
 *		objects).
 *	prep_queue: Ring buffer of prepped pages.
 *
 *	The prep and pin structures are:
 *	copy_list: current copy object being operated on.
 *	entry:	current entry being operated on.  Zero for KERNEL_BUFFER
 *		copy objects.  In a total sleaze, we use pin.entry on
 *		the receive side to receive the error trailer from the
 *		sender - the requirement is to have some static area to
 *		put the header in.  pin.entry is also used as a simple
 *		switch to indicate a KERNEL_BUFFER.
 *	offset: prep.offset is the current offset into the entry->object
 *		for doing vm_page_lookups.  pin.offset is unused.
 *	prep.size: set to the copy size, but is only used for debugging,
 *		not referenced in the code.
 *	pin.size: set to the copy->size and used to determine if there
 *		are partial pages to send on the last page of the range
 *		being sent.  It is subtracted from each time a page is
 *		pinned.
 *	vaddr: for both prep and pin, used to indicate the current
 *		virtual address being sent.  It is always up to date.
 *	entry_count: Used to signal errors on the remote node.  It is
 *		a simple count of the entries processed from the
 *		beginning of the message (inclusive of all copy objects).
 *		pin.entry count is updated bot not really used.
 *	done: all entries prepped/pinned.
 */

decl_simple_lock_data(,msg_order_lock_data)

#define	kserver_msg_order_lock()	simple_lock(&msg_order_lock_data)
#define	kserver_msg_order_unlock()	simple_unlock(&msg_order_lock_data)

/*
 *	Enable/disable DIPC steal_context synchronization.
 */
boolean_t	dipc_steal_context = FALSE;

/*
 *	Enable/disable DIPC error trailers on OOL transfers.
 */
boolean_t	dipc_generate_error_trailers = TRUE;

/*
 * dipc_sender_kmsg in the trailer will be used for synchronization between
 * the kkt send callback and the RPC that will wake up a blocked sender.
 * Four bits are defined, and their transitions are protected by the
 * kserver_msg_order_lock.
 */
#define CALLBACK_EXPECTED	1	/* cleared by callback */
#define RPC_EXPECTED		2	/* cleared by rpc */
#define SENDER_BLOCKED		4	/* cleared by sending thread */
#define MUST_DESTROY		8	/* sending thread must destroy kmsg */

#define	DIPC_P_NEED_PREP	1
#define	DIPC_P_DONE		2
#define	DIPC_P_INPROGRESS	3
#define	DIPC_P_NEED_FAULT	4


/* forward declarations */

void		dipc_connect_callback(
			kkt_return_t	kktr,
			handle_t	handle,
			request_block_t	req,
			boolean_t	thread_context);

void		dipc_xmit_done(
			kkt_return_t	kktr,
			handle_t	handle,
			request_block_t	req,
			boolean_t	thread_context);

void		dipc_send_err_callback(
			kkt_return_t	kktr,
			handle_t	handle,
			request_block_t	req);

void		dipc_install_error(
			msg_progress_t	mp,
			vm_object_t	object,
			vm_offset_t 	offset);

void		dipc_prep_n_send(
			handle_t	handle,
			msg_progress_t	mp);

dipc_return_t	dipc_pin_page(
			msg_progress_t	mp,
			vm_page_t	*page);

void		dipc_ool_kkt_error(
			kkt_return_t	kktr,
			request_block_t	req,
			ipc_kmsg_t	kmsg,
			msg_progress_t	mp,
			boolean_t	thread_context);

void		dipc_send_error_trailer(
			msg_progress_t	mp,
			request_block_t	req);

void		dipc_receive_error_trailer(
			msg_progress_t	mp,
			request_block_t	req);

void		dipc_process_receive_errors(
			msg_progress_t	mp);

void		dipc_calculate_req(
			msg_progress_t	mp,
			vm_page_t	m,
			kkt_sglist_t	dlist,
			boolean_t	can_clock);

void		dipc_unpin_request(
			request_block_t	req);

vm_page_t	dipc_extract_prep_page(
			prep_pages_queue_t	q);

void		dipc_restart_send(
			ipc_port_t	port);

void		dipc_recv_error_notify(
			kkt_return_t	kktr,
			handle_t	handle,
			request_block_t	req,
			boolean_t	thread_context);

/*
 *	Delayed msg_request free thread.
 */
decl_simple_lock_data(,dipc_msg_req_list_lock)
request_block_t	dipc_msg_req_list = NULL_REQUEST;
dstat_decl(unsigned int c_dmr_list_length = 0;)
dstat_decl(unsigned int c_dmr_list_enqueued = 0;)
dstat_decl(unsigned int c_dmr_list_handled = 0;)
dstat_decl(unsigned int c_dmr_list_max_length = 0;)

/* structures and definitions for blocked senders processing */

ipc_port_t restart_port_list_head;
ipc_port_t restart_port_list_tail;
vm_page_t zero_page;
decl_simple_lock_data(,zero_page_lock)

decl_simple_lock_data(,restart_port_lock_data)

#define restart_port_list_lock()	simple_lock(&restart_port_lock_data)
#define restart_port_list_unlock()	simple_unlock(&restart_port_lock_data)

#define NUM_DIPC_RESTART_THREADS	2
int num_dipc_restart_threads = NUM_DIPC_RESTART_THREADS;

dstat_decl(unsigned int	c_rp_list_length = 0;)
dstat_decl(unsigned int	c_rp_list_enqueued = 0;)
dstat_decl(unsigned int	c_rp_list_handled = 0;)
dstat_decl(unsigned int	c_rp_list_max_length = 0;)


/*
 * CHAINSIZE definition.
 *
 * Default is overridden at boot-time on some architectures.
 */
#define	DIPC_CHAIN_SIZE_DEFAULT		16
unsigned int	dipc_chain_size = DIPC_CHAIN_SIZE_DEFAULT;

/*
 * Queue length declarations for prep pages queue.
 *
 * These values are overridden at boot-time on some architectures.
 */
#define	DIPC_PREP_PAGES_MAX_DEFAULT		256
#define	DIPC_PREP_PAGES_LOW_WATER_DEFAULT	32
unsigned int	dipc_prep_pages_max = DIPC_PREP_PAGES_MAX_DEFAULT;
unsigned int	dipc_prep_pages_low_water = DIPC_PREP_PAGES_LOW_WATER_DEFAULT;

/*
 * Queue declarations for prep thread
 */
msg_progress_t	dipc_msg_prep_queue;
msg_progress_t	dipc_msg_prep_queue_tail;
decl_simple_lock_data(,dipc_msg_prep_queue_lock)
boolean_t	dipc_prep_thread_active = FALSE;
dstat_decl(unsigned int	c_dmp_queue_length = 0;)
dstat_decl(unsigned int	c_dmp_queue_enqueued = 0;)
dstat_decl(unsigned int	c_dmp_queue_handled = 0;)
dstat_decl(unsigned int	c_dmp_queue_max_length = 0;)

struct page_err_header dipc_zero_header;

msg_progress_t	dipc_msg_prep_free_list = MSG_PROGRESS_NULL;
boolean_t	dipc_msg_prep_free_thread_active = TRUE;
dstat_decl(unsigned int c_dmp_free_list_length = 0;)
dstat_decl(unsigned int c_dmp_free_list_enqueued = 0;)
dstat_decl(unsigned int c_dmp_free_list_handled = 0;)
dstat_decl(unsigned int c_dmp_free_list_max_length = 0;)
msg_progress_t	dipc_msg_prep_free_ast_list = MSG_PROGRESS_NULL;
dstat_decl(unsigned int c_dmp_free_ast_list_length = 0;)
dstat_decl(unsigned int c_dmp_free_ast_list_enqueued = 0;)
dstat_decl(unsigned int c_dmp_free_ast_list_handled = 0;)
dstat_decl(unsigned int c_dmp_free_ast_list_max_length = 0;)
decl_simple_lock_data(,dipc_msg_prep_free_lock)

decl_simple_lock_data(,dipc_kmsg_ast_lock)
struct ipc_kmsg_queue   dipc_kmsg_ast_queue;
dstat_decl(unsigned int c_dkaq_length = 0;)
dstat_decl(unsigned int c_dkaq_enqueued = 0;)
dstat_decl(unsigned int c_dkaq_handled = 0;)
dstat_decl(unsigned int c_dkaq_max_length = 0;)

void	dipc_restart_port_thread(void);
void	dipc_prep_thread(void);
void	dipc_msg_prep_free_thread(void);
void	dipc_msg_prep_free(
		msg_progress_t	mp);

typedef struct node_wait {
	struct node_wait	*next;
	struct node_wait	*prev;
	node_name		node;
	ipc_port_t		port_list;
	boolean_t		unblocking;
} *node_wait_t;

dipc_return_t	dipc_remove_blocked_node(node_wait_t nwait,
					 ipc_port_t	port);

struct node_wait	blocked_node_list_head;
#define NODE_WAIT_NULL	((node_wait_t)0)

decl_simple_lock_data(,blocked_node_lock_data)

#define blocked_node_list_lock()	simple_lock(&blocked_node_lock_data)
#define blocked_node_list_unlock()	simple_unlock(&blocked_node_lock_data)

dipc_return_t dipc_kmsg_proxy_enqueue(
	ipc_kmsg_t	kmsg,
	ipc_port_t	port);

dipc_return_t dipc_setup_blocked_node(
	node_name	node,
	ipc_port_t	port,
	node_wait_t	new_nwait);

boolean_t dipc_queue_idle_chain(
	msg_progress_t	mp,
	request_block_t	req,
	boolean_t	wakeup,
	spl_t		ipl,
	boolean_t	thread_context);

#define NODE_WAIT_ZONE_MAX_ELEM		2000
int node_wait_zone_max_elem = NODE_WAIT_ZONE_MAX_ELEM;
#define NODE_WAIT_ZONE_MIN_ELEM		20
int node_wait_zone_min_elem = NODE_WAIT_ZONE_MIN_ELEM;
zone_t	node_wait_zone;

dstat_decl(unsigned int	c_dipc_fast_alloc_pages = 0;)
dstat_decl(unsigned int c_dipc_mqueue_sends = 0;)
dstat_decl(unsigned int c_dipc_mqueue_ool_sends = 0;)


/*
 * Initialize the locks, queues, and threads used by the send side
 * of DIPC.  Called from dipc_bootstrap().
 */
void
dipc_send_init(void)
{
	vm_size_t	node_wait_zone_max;
	vm_size_t	node_wait_zone_min;
	int		i;

	simple_lock_init(&msg_order_lock_data, ETAP_DIPC_MSG_ORDER);
	simple_lock_init(&dipc_msg_prep_queue_lock, ETAP_DIPC_MSG_PREPQ);
	simple_lock_init(&dipc_msg_req_list_lock, ETAP_DIPC_MSG_REQ);
	simple_lock_init(&dipc_msg_prep_free_lock, ETAP_DIPC_MSG_FREE);

	simple_lock_init(&dipc_kmsg_ast_lock, ETAP_DIPC_KMSG_AST);
	ipc_kmsg_queue_init(&dipc_kmsg_ast_queue);

	restart_port_list_head = IP_NULL;
	restart_port_list_tail = IP_NULL;
	simple_lock_init(&restart_port_lock_data, ETAP_DIPC_RESTART_PORT);
	simple_lock_init(&zero_page_lock, ETAP_DIPC_ZERO_PAGE);

	simple_lock_init(&blocked_node_lock_data, ETAP_DIPC_BLOCKED_NODE);
	blocked_node_list_head.next = &blocked_node_list_head;
	blocked_node_list_head.prev = &blocked_node_list_head;
	blocked_node_list_head.node = NODE_NAME_NULL;
	blocked_node_list_head.port_list = IP_NULL;

	/*
	 * the node_wait_zone is expandable and collectable.
	 */
	node_wait_zone_max = (vm_size_t)node_wait_zone_max_elem *
		sizeof(struct node_wait);
	node_wait_zone_min = (vm_size_t)node_wait_zone_min_elem *
		sizeof(struct node_wait);
	node_wait_zone = zinit(sizeof(struct node_wait), node_wait_zone_max,
		sizeof(struct node_wait), "node_wait");
	i = zfill(node_wait_zone, node_wait_zone_min_elem);
	assert(i >= node_wait_zone_min_elem);

	for (i = 0; i < num_dipc_restart_threads; i++)
	    (void) kernel_thread(kernel_task, dipc_restart_port_thread,
				 (char *) 0);

	(void) kernel_thread(kernel_task, dipc_prep_thread, (char *) 0);
	(void) kernel_thread(kernel_task, dipc_msg_prep_free_thread, (char *)0);
}

/*
 * Routine:	dipc_send_handle_alloc
 *
 * Allocate a handle for sending a kmsg
 *
 */
kkt_return_t
dipc_send_handle_alloc(
		       handle_t	*handle)
{

	return (KKT_HANDLE_ALLOC(CHAN_USER_KMSG, handle,
				 dipc_send_err_callback, TRUE, TRUE));
}

/* shorthand for checking for timeouts in dipc_mqueue_send */
#define zero_timeout(option, timeout)	  (( (option) & MACH_SEND_TIMEOUT) && \
							( (timeout) == 0))
#define nonzero_timeout(option, timeout)  (( (option) & MACH_SEND_TIMEOUT) && \
							( (timeout) != 0))


/*
 * The port must be locked on entry, and it will be returned unlocked
 * on success.
 *
 * This function until not return until something (meta_kmsg or kmsg) is
 * enqueued on the receiving port on the remote node, or else there is an
 * error from which recovery is impossible (transport error or remote port
 * error).  If the receiving node cannot enqueue the message and the
 * SEND_ALWAYS option is specified, then the message will be enqueued locally.
 */
mach_msg_return_t
dipc_mqueue_send(
	ipc_kmsg_t		kmsg,
	mach_msg_option_t	option,
	mach_msg_timeout_t	timeout)
{
	ipc_port_t			port;
	handle_t			handle;
	request_block_t			req, chain1;
	boolean_t			steal_context, no_callback, has_waiting;
	boolean_t			more, migrated;
	msg_progress_t			msg_progress;
	ipc_thread_t			self;
	kkt_sglist_t			dlist;
	vm_page_t			*plist;
	mach_msg_format_0_trailer_t	*trailer;
	mach_port_type_t		port_type;
	kkt_return_t			kktr, kktr2;
 	dipc_return_t			dipc_ret[2]={0,0}, dr;
	kern_return_t			result;
	mach_msg_return_t		mr;
	int				req_count, i, mp_inited;
	spl_t				s;
	TR_DECL("dipc_mqueue_send");

	tr_start();
	tr4("enter: kmsg 0x%x option 0x%x timeout 0x%x",kmsg, option, timeout);

	more = !!(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL);

	port = (ipc_port_t)kmsg->ikm_header.msgh_remote_port;
	req = chain1 = NULL_REQUEST;
	steal_context = migrated = FALSE;
	has_waiting = FALSE;
	mp_inited = 0;
	KMSG_DIPC_TRAILER(kmsg)->dipc_sender_kmsg = 0;

restart:
	/* at this point the port should be locked */

	/* the kmsg must be in dipc format */
	assert(KMSG_IS_DIPC_FORMAT(kmsg));
	assert(ip_active(port));
	assert(IP_IS_REMOTE(port));
	tr4("...sending kmsg 0x%x, id %d, to node %d", kmsg,
	    kmsg->ikm_header.msgh_id, port->ip_dest_node);

	trailer = KMSG_DIPC_TRAILER(kmsg);

#if	0
	/*
	 *	This code was originally developed to enforce
	 *	message ordering guarantees for all but a few,
	 *	special-case threads.  Basically, it was meant
	 *	to cure XMM problems, because XMM didn't do a
	 *	good job of maintaining ordering of its own,
	 *	internal messages.
	 *
	 *	Unfortunately, the general use of steal_context
	 *	leads to deadlocks.
	 *
	 *	We retain this code, however, for use by the
	 *	"message flush" facility, which will guarantee
	 *	that all previous messages originating from a
	 *	thread to a given destination port will have entered
	 *	the receiver's fault tolerance domain before the
	 *	"flush message" returns to the sender.
	 */
	if (dipc_steal_context == TRUE &&
	    current_thread()->dipc_steal_context == TRUE) {
		/*
		 * We won't know whether to block until we get an answer
		 * back from SEND_CONNECT that the destination port is in
		 * the kernel.  dipc_sender_kmsg is used for synchronization
		 * with the RPC and the kkt send callback.
		 */
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_SENDER_WAITING;
		has_waiting = TRUE;
		steal_context = TRUE;
		trailer->dipc_sender_kmsg = CALLBACK_EXPECTED | RPC_EXPECTED
							      | SENDER_BLOCKED;
	}
#endif	/* 0 */

	/*
	 * Make a quick check to see if sends are being blocked for this port.
	 * If so, no need to bother trying to connect.  If this message is
	 * being resent to drain the proxy's mqueue after a resource shortage
	 * or queue full, then continue on and try to send it.
	 */
	if ((port->ip_resending || port->ip_rs_block || port->ip_qf_count>0) &&
	    !(option & MACH_SEND_LOCAL_QUEUE)) {
		/* leave port locked */
		goto block_sender;
	}
	assert(!steal_context || !(option & MACH_SEND_LOCAL_QUEUE));

	/*
	 * get the msg_progress pointer out of the trailer -
	 * dutifully put in there by the copyin logic.
	 */
	msg_progress = KMSG_MSG_PROG_SEND(kmsg);

	ip_unlock(port);

	if (!mp_inited++) {
		vm_map_copy_t copy;

		req_count = 1;

		if (msg_progress == MSG_PROGRESS_NULL)
			goto mp_init_done;

		/*
		 * msg_progress->msg_size was filled in by the copyin
		 * code and contains the size of the OOL data.  Add
		 * 1 for the kmsg request.
		 */
		req_count = msg_progress->msg_size / PAGE_SIZE + 1;
		if (req_count >= CHAINSIZE * 2) {
			chain1 = dipc_get_msg_req(msg_progress,
						  REQUEST_SEND);
			chain1->callback = dipc_connect_callback;
			assert(!(msg_progress->flags & MP_F_INIT) ||
			       msg_progress->chain_count == 2);
			msg_progress->chain_count = 2;
			req_count = CHAINSIZE;
		} else {
			if (req_count >= CHAINSIZE)
				req_count = CHAINSIZE;
			assert(!(msg_progress->flags & MP_F_INIT) ||
			       msg_progress->chain_count == 1);
			msg_progress->chain_count = 1;
		}

		if (msg_progress->flags & MP_F_INIT)
			goto mp_init_done;

		assert(msg_progress->state == MP_S_START);

		copy = msg_progress->copy_list->copy;

		/* we handle either ENTRY_LIST or KERNEL_BUFFER copy types */
		if (copy->type == VM_MAP_COPY_ENTRY_LIST) {
		    msg_progress->prep.entry = vm_map_copy_first_entry(copy);
		    msg_progress->prep.offset =
			msg_progress->prep.entry->offset
			+ trunc_page(copy->offset)
			- msg_progress->prep.entry->vme_start;
		} else {
		    assert(copy->type == VM_MAP_COPY_KERNEL_BUFFER);
		    msg_progress->prep.entry = VM_MAP_ENTRY_NULL;
		    msg_progress->prep.offset = 0;
		}

		msg_progress->prep.vaddr = copy->offset;
		msg_progress->prep.copy_list = msg_progress->copy_list;
		msg_progress->prep.size = copy->size;
		msg_progress->pin = msg_progress->prep;

		/*
		 *	Load the prep_queue with prepped pages
		 */
		(void)dipc_prep_pages(msg_progress);

		/*
		 *	Set up the request chain(s)
		 */
		msg_progress->idle_chain[0] = NULL_REQUEST;
		msg_progress->state = MP_S_READY;
		/*
		 *	At the beginning, ASSUME that there is
		 *	a thread context waiting to return to.
		 *	(The problem is that the transfer may
		 *	begin -- or even complete! -- before
		 *	we return inline to this routine, after
		 *	calling KKT_SEND_CONNECT.)  By forcing
		 *	this state, if any of the pages aren't
		 *	prep- and pin-able, the transfer will
		 *	wait for this thread to catch up.
		 *
		 *	Later, when we process the response from
		 *	KKT_SEND_CONNECT, we may be forced to
		 *	change the flag to indicate that there
		 *	isn't a waiting caller any more.
		 */
		msg_progress->flags |= MP_F_INIT | MP_F_THR_AVAIL |
					MP_F_SENDING_THR | MP_F_OOL_IN_PROG;
	}

mp_init_done:
	assert(msg_progress == MSG_PROGRESS_NULL ||
		(((msg_progress->flags & MP_F_THR_AVAIL|MP_F_INIT) ==
			MP_F_THR_AVAIL|MP_F_INIT) &&
			(msg_progress->state == MP_S_READY)));

	assert(req == NULL_REQUEST);
	req = dipc_get_msg_req(msg_progress,
			       REQUEST_SEND);
	req->callback = dipc_connect_callback;
	req->args[0] = (unsigned int)kmsg;

	/* send whole inline msg with connect */
	/* XXX we are currently NOT transmitting the trailer... */
	
	dlist = req->data.list;
	dlist->ios_address = (vm_offset_t)&kmsg->ikm_header;
	dlist->ios_length  = kmsg->ikm_header.msgh_size;	/* M+B */
	req->data_length   = kmsg->ikm_header.msgh_size;	/* M+B */

	plist = (vm_page_t *)req->args[1];
	*plist = (vm_page_t)port; /* remote_port saved here */

	kktr = KKT_HANDLE_ALLOC(CHAN_USER_KMSG, &handle,
				dipc_send_err_callback, TRUE, FALSE);
	if (kktr != KKT_SUCCESS) {
		/* probably means transport is hosed (??) */
		printf("dipc_mqueue_send: KKT_HANDLE_ALLOC failed\n");
		assert(FALSE);
		dipc_free_msg_req(req, TRUE);
		req = NULL_REQUEST;
		if (chain1 != NULL_REQUEST)
			dipc_free_msg_req(chain1, TRUE);
		/* caller must destroy the kmsg */
		ip_lock(port);
		(void) dipc_port_dest_done(port, port_type);
		tr2("exit: KKT_HANDLE_ALLOC failure(%d)", kktr);
		tr_stop();
		return MACH_SEND_TRANSPORT_ERROR;
	}
	kmsg->ikm_handle = handle;
	KMSG_CLEAR_RECEIVING(kmsg);
	KMSG_MARK_HANDLE(kmsg);

	/*
	 * send over the address of the kmsg in the destination port field,
	 * which allows us to skip sending the trailer.  The port has been
	 * temporarily stored in the request structure and will be restored
	 * to its proper place when we get the callback saying that the kmsg
	 * has been sent.
	 * This will give the receiving node a cookie to pass back to
	 * identify this message (for drpc_wakeup_sender).
	 *
	 * N.B.
	 *	There is a difficult race condition here that we deal
	 *	with but may need more thought: The SEND_CONNECT is
	 *	a blocking call, so the remote node can request and
	 *	receive the data described by req before we resume,
	 *	going through dipc_connect_callback() and firing more
	 *	data off.  Above, we unconditionally assumed that
	 *	steal_context will be FALSE and set the msg_progress
	 *	to be PREP_REQ - requires services of the prep thread.
	 *	Worst case, the msg_progress will transition back, but
	 *	there shouldn't be any problems.
	 */
	kmsg->ikm_header.msgh_remote_port = (mach_port_t)kmsg;

	/*
	 *	Must remember the port type because when a
	 *	message contains only inline data, there is
	 *	a race with the SEND_CONNECT callback; when
	 *	the message is sent successfully, it may be
	 *	deallocated before control resumes here.
	 *	Therefore, we can't look at the kmsg in
	 *	many cases.
	 */
	port_type = MACH_MSGH_BITS_REMOTE(kmsg->ikm_header.msgh_bits);

	kktr = KKT_SEND_CONNECT(handle,
				port->ip_dest_node,
				(endpoint_t *)&port->ip_uid,
				req,
				more, 
				(kern_return_t *)dipc_ret);
	if (kktr != KKT_DATA_SENT && kktr != KKT_SUCCESS) {
		/* connection failed; give up */
transport_error:
		printf("dipc_mqueue_send: transport error %x\n", kktr);	/* MMPXXX */
		assert(FALSE);	/* MMPXXX  put in a tr and a counter instead */
		KMSG_DROP_HANDLE(kmsg, handle);
		dipc_free_msg_req(req, TRUE);
		req = NULL_REQUEST;
		if (chain1 != NULL_REQUEST)
			dipc_free_msg_req(chain1, TRUE);
		if (msg_progress != MSG_PROGRESS_NULL)
			msg_progress->state = MP_S_MSG_DONE;
		/* caller must destroy the kmsg */

		/* restore remote_port */
		kmsg->ikm_header.msgh_remote_port = (mach_port_t)port;

		ip_lock(port);
		(void) dipc_port_dest_done(port, port_type);
		tr_stop();
		return MACH_SEND_TRANSPORT_ERROR;
	}

#if 0
	/*
	 * Find out if the remote port is a kernel port.  If not, we don't
	 * need to steal this context.
	 */
	if (steal_context && (dipc_ret[1] != DIPC_KERNEL_PORT)) {
		steal_context = FALSE;
		s = splkkt();
		kserver_msg_order_lock();
		trailer->dipc_sender_kmsg &= ~RPC_EXPECTED;
		kserver_msg_order_unlock();
		splx(s);
	}
#endif

	/*
	 * connection established; if KKT_DATA_SENT, data was
	 * transferred as well.
	 */
	no_callback = FALSE;
	switch (dipc_ret[0]) {
	  case DIPC_DATA_DROPPED:
		/* message enqueued but data dropped.  resend data. */
		assert(kktr == KKT_DATA_SENT);
		kktr = KKT_REQUEST(handle, req);
		if (kktr == KKT_SUCCESS) {
			/*
			 * data queued, now expecting a callback.  Go on
			 * to send ool data (if any).  After calling
			 * dipc_port_enqueued, it is an ERROR to touch
			 * the port again, as it may disappear.
			 */
			ip_lock(port);
			(void) dipc_port_dest_done(port, port_type);
			ip_unlock(port);
#if	DIPC_DO_STATS
			dstat(++c_dipc_mqueue_sends);
			if (more)
				dstat(++c_dipc_mqueue_ool_sends);
#endif	/* DIPC_DO_STATS */
			break;
		} else if (kktr != KKT_DATA_SENT) {
			/* transport error */
			goto transport_error;
		}

		/*
		 * kktr == KKT_DATA_SENT: data was sent immediately, and
		 * there will be no callback.
		 * Fall through to the DIPC_DATA_SENT case below.
		 */
		dipc_ret[0] = DIPC_DATA_SENT;	/* for the assert below */

	  case DIPC_SUCCESS:
	  case DIPC_DATA_SENT:
		/* message was accepted by remote node */
		/*
		 * Careful:  touching the port after this call
		 * is probably a bad idea.  The kmsg itself owns
		 * the only other reference to the port.  In the
		 * KKT_DATA_SENT case, no callback is expected,
		 * so this routine, dipc_mqueue_send, in some sense
		 * still "owns" the kmsg, so we can touch the port
		 * if need be.  Otherwise, the kmsg is in flight
		 * and it could be released (and the port destroyed)
		 * at any time.
		 */
		ip_lock(port);
		(void) dipc_port_dest_done(port, port_type);
		ip_unlock(port);
#if	DIPC_DO_STATS
		dstat(++c_dipc_mqueue_sends);
		if (more)
			dstat(++c_dipc_mqueue_ool_sends);
#endif	/* DIPC_DO_STATS */

		if (kktr == KKT_DATA_SENT) {
			/* done sending inline kmsg */
			assert(dipc_ret[0] == DIPC_DATA_SENT);
			tr2("kmsg 0x%x has DATA_SENT", kmsg);

			/*
			 * restore remote_port:  we don't expect a
			 * callback for the inline message.
			 */
			kmsg->ikm_header.msgh_remote_port = (mach_port_t)port;
			no_callback = TRUE;
			if (more == FALSE) {
				/*
				 * entire message was inline.  Need to check
				 * whether we need to block before freeing
				 * kmsg.
				 */
				KMSG_DROP_HANDLE(kmsg, handle);
				dipc_free_msg_req(req, TRUE);
				req = NULL_REQUEST;
				break;
			}
		} else {
			/* expecting a callback when data is sent */
			/*
			 * After calling dipc_port_dest_done, it is
			 * an ERROR in this case to touch the port
			 * again.  The only other reference to the
			 * port is owned by the kmsg itself, which
			 * is eventually going to be released by
			 * dipc_connect_callback.
			 */
			assert(dipc_ret[0] == DIPC_SUCCESS);
		}
		/* go on to send ool data, if any */
		break;

	  case DIPC_RESOURCE_SHORTAGE:
	  case DIPC_QUEUE_FULL:
	  case DIPC_RETRY_SEND:
		if (kktr != KKT_DATA_SENT) {
			/*
			 * flush the queued data.  The abort code
			 * will invoke all callbacks, synchronously.
			 * It is safe to free the handle when the
			 * abort returns.
			 */
			kktr2 = KKT_HANDLE_ABORT(handle,
						 DIPC_RESOURCE_SHORTAGE);
			assert(kktr2 == KKT_SUCCESS);
		}
		KMSG_DROP_HANDLE(kmsg, handle);
		dipc_free_msg_req(req, TRUE);
		req = NULL_REQUEST;

		/* restore remote_port */
		kmsg->ikm_header.msgh_remote_port = (mach_port_t)port;

		if (dipc_ret[0] == DIPC_QUEUE_FULL ||
		    dipc_ret[0] == DIPC_RETRY_SEND) {
			ip_lock(port);
			if (dipc_ret[0] == DIPC_QUEUE_FULL)
			    port->ip_qf_count++;
			if ((!port->ip_resending ||
			     option & MACH_SEND_LOCAL_QUEUE) &&
			    (port->ip_qf_count <= 0))
			    goto check_condition;
			if (zero_timeout(option, timeout)) {
				ip_unlock(port);
				if (chain1 != NULL_REQUEST)
					dipc_free_msg_req(chain1, TRUE);
				ip_lock(port);
				(void) dipc_port_dest_done(port, port_type);
				tr_stop();
				return MACH_SEND_TIMED_OUT;
			}
		} else {
			node_wait_t	nwait;

			assert(dipc_ret[0] == DIPC_RESOURCE_SHORTAGE);

			if (zero_timeout(option, timeout)) {
				if (chain1 != NULL_REQUEST)
					dipc_free_msg_req(chain1, TRUE);
				ip_lock(port);
				(void) dipc_port_dest_done(port, port_type);
				tr_stop();
				return MACH_SEND_TIMED_OUT;
			}

			/*
			 * preallocate nwait in case it's needed
			 * before locking things.  It will be consumed
			 * by dipc_setup_blocked_node.
			 */
			nwait = (node_wait_t)zalloc(node_wait_zone);

			ip_lock(port);
			if (!port->ip_rs_block) {
				dr = dipc_setup_blocked_node(
							    port->ip_dest_node,
							    port, nwait);
				if (dr == DIPC_RETRY_SEND) {
					/* node is unblocking, just retry */
					goto check_condition;
				}

				assert(dr == DIPC_SUCCESS);
				port->ip_rs_block = TRUE;
			} else {
				/*
				 * this node and port are already in blocked
				 * node list; no need to request another
				 * callback.
				 */
				zfree(node_wait_zone, (vm_offset_t)nwait);
				dipc_ret[0] = DIPC_SUCCESS;
			}
		}

		assert(!zero_timeout(option, timeout));
block_sender:
		/* port is locked */

		self = current_thread();

		/*
		 * don't enqueue or block if this is a resend of a message
		 * that was already locally enqueued.
		 */
		if (!(option & MACH_SEND_LOCAL_QUEUE)) {
			if ((option & MACH_SEND_ALWAYS) && !steal_context) {
				/*
				 * enqueue message locally.  However, we
				 * must also turn off the sender_waiting
				 * bit because the sending thread isn't
				 * going to hang around.
				 */
				kmsg->ikm_header.msgh_bits &=
					~MACH_MSGH_BITS_SENDER_WAITING;
				dipc_kmsg_proxy_enqueue(kmsg, port);
			} else {
				/* put thread on port's blocked senders queue */
				ipc_thread_enqueue(&port->ip_blocked, self);
				self->ith_state = MACH_SEND_IN_PROGRESS;
			}
		}

		if (dipc_ret[0] == DIPC_RESOURCE_SHORTAGE) {
			assert(!(option & MACH_SEND_LOCAL_QUEUE));
			ip_unlock(port);
			kktr = drpc_request_callback(port->ip_dest_node,
							dipc_node_self(), &dr);
			assert(kktr == KKT_SUCCESS);
			ip_lock(port);
			if (dr != DIPC_SUCCESS) {
				/*
				 * port moved.  A separate callback will
				 * restart this port.
				 */
				assert(dr == DIPC_PORT_MIGRATED);
			}
		}

		/*
		 *	Tacky:  we will NEVER return this error code from here,
		 *	so use it to figure out whether we are returning.
		 *	You are expected to understand this.
		 *
		 *	Note that if the message is enqueued on the proxy
		 *	(either MACH_MSG_SUCCESS or MACH_SEND_RESEND_FAILED),
		 *	we do NOT want to call dipc_port_dest_done since
		 *	the extra port reference needs to stay around until
		 *	the message is either successfully sent or destroyed.
		 */
		mr = MACH_SEND_TRANSPORT_ERROR;
		if ((option & MACH_SEND_ALWAYS) && !steal_context)
			mr = MACH_MSG_SUCCESS;
		else if (option & MACH_SEND_LOCAL_QUEUE)
			mr = MACH_SEND_RESEND_FAILED;
		else if (zero_timeout(option, timeout))
			mr = MACH_SEND_TIMED_OUT;

		if (mr == MACH_MSG_SUCCESS) {
			ip_unlock(port);
			if (chain1 != NULL_REQUEST)
			    dipc_free_msg_req(chain1, TRUE);
			tr_stop();
			return mr;
		}

		if (mr != MACH_SEND_TRANSPORT_ERROR) {
			ip_unlock(port);
			if (chain1 != NULL_REQUEST)
				dipc_free_msg_req(chain1, TRUE);
			ip_lock(port);
			if (mr == MACH_SEND_TIMED_OUT)
				(void) dipc_port_dest_done(port, port_type);
			tr_stop();
			return mr;
		}

		if (nonzero_timeout(option, timeout))
			thread_will_wait_with_timeout(self, timeout);
		else
			thread_will_wait(self);

		ip_unlock(port);
		thread_block(0);
		if (nonzero_timeout(option, timeout))
			reset_timeout_check(&self->timer);
		ip_lock(port);

		switch (self->ith_wait_result) {
			case THREAD_AWAKENED:
				assert(self->ith_state == MACH_MSG_SUCCESS);
				break;
			case THREAD_TIMED_OUT:
				assert(nonzero_timeout(option, timeout));
				(void) dipc_port_dest_done(port, port_type);
				tr_stop();
				return MACH_SEND_TIMED_OUT;
			case THREAD_INTERRUPTED:
				(void) dipc_port_dest_done(port, port_type);
				tr_stop();
				return MACH_SEND_INTERRUPTED;
			default:
				panic("dipc_mqueue_send: invalid wait_result");
		}

		goto check_condition;

	  case DIPC_PORT_MIGRATED:
		/*
		 * port wasn't where we thought it was.  figure out
		 * where to try next and start over.
		 */
		migrated = TRUE;
		KMSG_DROP_HANDLE(kmsg, handle);
		dipc_free_msg_req(req, TRUE);
		req = NULL_REQUEST;

		/* restore remote_port */
		kmsg->ikm_header.msgh_remote_port = (mach_port_t)port;

		ip_lock(port);
check_condition:
		/* check port's condition */

		/* wait for migration */
		result = THREAD_AWAKENED;
		while (port->ip_dipc->dip_ms_block) {
			if (zero_timeout(option, timeout)) {
				result = THREAD_TIMED_OUT;
				break;
			}
			migrated = TRUE;
			assert_wait((event_t)port, TRUE);
			if (nonzero_timeout(option, timeout))
				thread_set_timeout((timeout * hz) / 1000);
			ip_unlock(port);
			thread_block(0);
			if (nonzero_timeout(option, timeout))
				reset_timeout_check(&current_thread()->timer);
			ip_lock(port);
			result = current_thread()->wait_result;
			if (result != THREAD_AWAKENED)
				break;
		}
		switch (result) {
			case THREAD_AWAKENED:
				break;
			case THREAD_TIMED_OUT:
				assert(option & MACH_SEND_TIMEOUT);
				ip_unlock(port);
				if (chain1 != NULL_REQUEST)
					dipc_free_msg_req(chain1, TRUE);
				ip_lock(port);
				(void) dipc_port_dest_done(port, port_type);
				tr_stop();
				return MACH_SEND_TIMED_OUT;
			case THREAD_INTERRUPTED:
				ip_unlock(port);
				if (chain1 != NULL_REQUEST)
					dipc_free_msg_req(chain1, TRUE);
				ip_lock(port);
				(void) dipc_port_dest_done(port, port_type);
				tr_stop();
				return MACH_SEND_INTERRUPTED;
			default:
				panic("dipc_mqueue_send: invalid wait_result");
		}
		if (!IP_IS_REMOTE(port)) {
			/*
			 * port migrated to this node.  Go back to
			 * ipc_mqueue_send, which will send locally.
			 */
			ip_unlock(port);
			if (chain1 != NULL_REQUEST)
				dipc_free_msg_req(chain1, TRUE);
			ip_lock(port);
			(void) dipc_port_dest_done(port, port_type);
			tr_stop();
			return MACH_SEND_PORT_MIGRATED;
		}
		if (migrated == TRUE) {
			migrated = FALSE;
			ip_unlock(port);
			dr = dipc_port_update(port);
			assert(dr == DIPC_SUCCESS);
			ip_lock(port);
		}
		if (!ip_active(port)) {
			ip_unlock(port);
			if (chain1 != NULL_REQUEST)
				dipc_free_msg_req(chain1, TRUE);
			ip_lock(port);
			(void) dipc_port_dest_done(port, port_type);
			tr_stop();
			return MACH_SEND_INVALID_DEST;
		}

		goto restart;

	  case DIPC_NO_UID:
	  case DIPC_PORT_DEAD:
		/*
		 * Need to abort the handle as there is data queued to
		 * the remote port.
		 */
		kktr2 = KKT_HANDLE_ABORT(handle, DIPC_PORT_DEAD);
		assert(kktr2 == KKT_SUCCESS);
		KMSG_DROP_HANDLE(kmsg, handle);
		dipc_free_msg_req(req, TRUE);
		req = NULL_REQUEST;

		/* restore remote_port */
		kmsg->ikm_header.msgh_remote_port = (mach_port_t)port;

		if (chain1 != NULL_REQUEST)
			dipc_free_msg_req(chain1, TRUE);

		/* caller must destroy the kmsg */
		ip_lock(port);
		(void) dipc_port_dest_done(port, port_type);
		tr_stop();
		return MACH_SEND_INVALID_DEST;		/* is this right? */

	  default:
		printf("dipc_mqueue_send: DATA_SENT, invalid dr code %d\n",
								dipc_ret[0]);
		panic("dipc_mqueue_send");
		break;
	}

	/*
	 * Connection is established, inline kmsg has been sent.  If
	 * no_callback is TRUE, there will NOT be a callback.
	 */
	if (no_callback && has_waiting) {
		/* no callback expected, turn off that bit. */
		assert(KMSG_HAS_WAITING(kmsg));
		s = splkkt();
		kserver_msg_order_lock();
		assert(trailer->dipc_sender_kmsg & CALLBACK_EXPECTED);
		trailer->dipc_sender_kmsg &= ~CALLBACK_EXPECTED;
		kserver_msg_order_unlock();
		splx(s);
	}

	/*
	 * start up the OOL engine
	 */
	if (msg_progress) {
		/*
		 * If our data was sent inline and there is more to send,
		 * fire it off now to get the chain back in motion.
		 */
		assert(more);

		/*
		 * If we're not stealing the sender's context, clear the
		 * SENDER_BLOCKED bit now.
		 */
		if ((has_waiting == TRUE) && (steal_context == FALSE)) {
			s = splkkt();
			kserver_msg_order_lock();
			tr5("kmsg 0x%x ool  no_call %d steal %d sender_kmsg flags 0x%x",
				kmsg, no_callback, steal_context,
				trailer->dipc_sender_kmsg);
			assert(!(trailer->dipc_sender_kmsg & RPC_EXPECTED));
			trailer->dipc_sender_kmsg &= ~SENDER_BLOCKED;
			kserver_msg_order_unlock();
			splx(s);
			has_waiting = FALSE;
		}
		if (no_callback) {
			req->args[0] = (unsigned int)msg_progress;
			/*
			 *	We don't have to lock the msg_progress
			 *	to make this state check because there
			 *	is NO OTHER TRANSFER IN PROGRESS on this
			 *	mp.  By definition, there is no callback
			 *	outstanding, i.e., we've got no other
			 *	data in the pipe.
			 */
			assert(msg_progress->state != MP_S_OOL_DONE);

			msg_progress->flags |= MP_F_QUEUEING; /* for assert */
			dr = dipc_fill_chain(req, msg_progress,
					     TRUE);
			assert(dr == DIPC_SUCCESS);

			KKT_DO_MAPPINGS();
			req->callback = dipc_xmit_engine;
			msg_progress->flags &= ~MP_F_QUEUEING;
			kktr = KKT_REQUEST(handle, req);

			if (kktr != KKT_SUCCESS)
				goto transport_error;
		}
		dr = dipc_ool_xfer(handle, msg_progress, chain1, steal_context);
		assert(dr == DIPC_SUCCESS || dr == DIPC_TRANSPORT_ERROR);
		assert(!steal_context || msg_progress->state == MP_S_MSG_DONE);
	}

	/*
	 * Block this thread (the sending thread) until the message has
	 * been processed on the receiving node, in effect stealing this
	 * context so that no more messages can be sent until this one is
	 * done.  The remote node will issue an RPC when the message
	 * has been received and processed which will wake this thread up.
	 *
	 * We check has_waiting rather than steal_context
	 * to synchronize with connect_callback to make sure that
	 * someone releases the kmsg (inline-only case).  If steal_context is
	 * FALSE at this point, the RPC_EXPECTED bit will have been cleared
	 * and the thread will not block.
	 *
	 * At this point the kmsg itself may already be freed, so we need
	 * to check the locally saved state of KMSG_HAS_WAITING (has_waiting).
	 */
	if (has_waiting) {
		boolean_t	free_kmsg = FALSE;
		boolean_t	destroy_kmsg = FALSE;

		assert(KMSG_HAS_WAITING(kmsg));
		s = splkkt();
		kserver_msg_order_lock();
		tr5("kmsg 0x%x no_callback %d steal %d sender_kmsg flags 0x%x",
		   kmsg, no_callback, steal_context, trailer->dipc_sender_kmsg);
		while (trailer->dipc_sender_kmsg & RPC_EXPECTED) {
			assert(steal_context == TRUE);
			assert_wait((event_t)kmsg, FALSE);
			kserver_msg_order_unlock();
			splx(s);
			thread_block((void (*)(void)) 0);
			s = splkkt();
			kserver_msg_order_lock();
		}
		trailer->dipc_sender_kmsg &= ~SENDER_BLOCKED;
		if (trailer->dipc_sender_kmsg == 0)
			free_kmsg = TRUE;
		else if (trailer->dipc_sender_kmsg == MUST_DESTROY)
			destroy_kmsg = TRUE;
		assert(destroy_kmsg || free_kmsg ||
			(trailer->dipc_sender_kmsg & CALLBACK_EXPECTED));
		kserver_msg_order_unlock();
		splx(s);
		if (free_kmsg == TRUE) {
			dipc_kmsg_release(kmsg);
		} else if (destroy_kmsg == TRUE) {
			ipc_kmsg_destroy(kmsg);
		}
	} else if (no_callback && (more == FALSE)) {
		/*
		 * inline-only kmsg, data was already sent, and we don't
		 * have to block.  Free the kmsg -- it's all done.
		 */
		assert(msg_progress == MSG_PROGRESS_NULL);
		dipc_kmsg_release(kmsg);
	} else if (more) {
		/*
		 * kmsg w/ool, but not steal_context.  Find out if the
		 * ool portion has been completely sent.  If so, free
		 * the kmsg.  If not, clear the SENDING_THR flag in the mp
		 * so that the kmsg will be freed when the ool engine finishes.
		 */
		assert(msg_progress != MSG_PROGRESS_NULL);

		s = splkkt();
		simple_lock(&msg_progress->lock);
		assert(msg_progress->flags & MP_F_SENDING_THR);
		if (msg_progress->flags & MP_F_OOL_IN_PROG) {
			/* ool data still flowing */
			msg_progress->flags &= ~MP_F_SENDING_THR;
			simple_unlock(&msg_progress->lock);
			splx(s);
			/* DO COW STUFF HERE (?) */
		} else {
			/*
			 * OOL engine is done.
			 *
			 * Dispose of the kmsg.  The msg_progress
			 * itself will be destroyed as a side-effect
			 * of releasing the kmsg.
			 *
			 * XXX Interesting question:  destroying the
			 * kmsg can cause yield_transits behavior,
			 * thus blocking this thread on an RPC.
			 * Is this a problem? XXX
			 */
			assert(msg_progress->state == MP_S_MSG_DONE);
			simple_unlock(&msg_progress->lock);
			splx(s);

			if (msg_progress->flags & MP_F_KKT_ERROR)
				ipc_kmsg_destroy(msg_progress->kmsg);
			else
				dipc_kmsg_release(msg_progress->kmsg);
		}
	}
	/*
	 * else: inline-only kmsg WITH a callback.  dipc_connect_callback
	 * will arrange to free the kmsg.  There is nothing to do here.
	 */

	tr2("exit: steal_context = %d", steal_context);
	tr_stop();

	return MACH_MSG_SUCCESS;
}

/*
 * Allocate a ready-to-go sglist request structure.  There are two
 * elements of dynamic memory inserted here: the sglist for address/length
 * pairs, and an array for holding onto vm_page_t's until we can use them.
 *
 * XXX At this point in time we allocate CHAINSIZE elements of dynamic
 * XXX memory as we don't have a way to track how much needs to be
 * XXX released when we are done with it.
 */
request_block_t
dipc_get_msg_req(
	msg_progress_t	mp,
	unsigned int	req_type)
{
	request_block_t	req;

	req = dipc_get_kkt_req(TRUE);
	req->data.list = (kkt_sglist_t)
		kalloc(CHAINSIZE * sizeof(struct io_scatter));
	req->args[1] = (int)kalloc(CHAINSIZE * sizeof(vm_page_t));

	req->data_length = 0;
	req->callback = 0;
	req->next = 0;

	req->args[0] = (unsigned int)mp;
	req->request_type = req_type | REQUEST_SG;

	return (req);
}

void
dipc_free_msg_req(
	request_block_t	req,
	boolean_t	thread_context)
{
	TR_DECL("dipc_free_msg_req");

	tr_start();
	tr3("enter: req=0x%x thr_context %d", req, thread_context);
	assert(req->request_type & REQUEST_SG);

	if (thread_context) {
		if (req->args[1])
			kfree(req->args[1], CHAINSIZE * sizeof(vm_page_t));
		kfree((int)req->data.list,
					CHAINSIZE * sizeof(struct io_scatter));
		dipc_free_kkt_req(req);
	} else {
		/*
		 * We can't kfree items directly from interrupt level,
		 * hence toss requests off to an AST for deallocation.
		 * Must be at splsched to toy with ASTs; but given a
		 * call from interrupt level, we should already be
		 * at least at splkkt.
		 */
		spl_t	s;

		/* assert(splkkt_held());	XXX not quite ready for this,
		 				even though we know it's true */
		simple_lock(&dipc_msg_req_list_lock);
		req->next = dipc_msg_req_list;
		dipc_msg_req_list = req;
		dstat(++c_dmr_list_length);
		dstat(++c_dmr_list_enqueued);
		simple_unlock(&dipc_msg_req_list_lock);
		s = splsched();
		ast_on(cpu_number(), AST_DIPC);
		splx(s);
	}

	tr_stop();
}


/*
 *
 *
 *	Transfer the ool data to/from the remote node.  This works
 *	with the interrupt half of the ool engine to get the data
 *	to move.
 */
dipc_return_t
dipc_ool_xfer(
	handle_t	handle,
	msg_progress_t	mp,
	request_block_t	chain1,
	boolean_t	steal_context)
{
	ipc_kmsg_t	kmsg;
	vm_page_t	m;
	kkt_return_t	kktr;
	dipc_return_t	dr;
	register	i;
	spl_t		s;
	struct handle_info info;
	TR_DECL("dipc_ool_xfer");

	tr_start();
	tr5("enter: mp 0x%x state 0x%x chain1 0x%x sc %d", mp, mp->state,
	    chain1, steal_context);

	/*
	 *	Mark the msg_progress state as sending until
	 *	both chains are in motion to prevent race conditions
	 *	(notice that until the first chain is sent, there is no
	 *	possibility of anyone else messing with the structure,
	 *	hence no locks).
	 */
	kmsg = mp->kmsg;

	s = splkkt();
	simple_lock(&mp->lock);

	if (chain1) {
		switch (mp->state) {
		  case MP_S_OOL_DONE:
			/*
			 * all the ool regions have already been transferred
			 * using the first chain, started up by callbacks,
			 * before this thread got to run again.  The first
			 * chain will then be used at interrupt level to
			 * send/receive the error trailer; there is nothing
			 * left for us to do here.  Fall through to the next
			 * case to enqueue the now-idle chain1.
			 */

		  case MP_S_ERROR_DONE:
		  case MP_S_MSG_DONE:
			/*
			 * all the ool regions plus the error trailer have
			 * already been transferred using the first chain,
			 * started up by callbacks, before this thread got
			 * to run again.  We don't need this chain after
			 * all... just idle it.
			 */
			for (i = 0; i < MAXCHAINS; i++) {
				if (mp->idle_chain[i] == NULL_REQUEST) {
					mp->idle_chain[i] = chain1;
					mp->idle_count++;
					assert(mp->idle_count<=mp->chain_count);
					break;
				}
			}
			mp->flags |= MP_F_IDLE;
			break;

		  case MP_S_READY:
			mp->flags |= MP_F_QUEUEING;
			simple_unlock(&mp->lock);
			splx(s);

			/*
			 *	Put the second chain into motion.
			 */
			chain1->args[0] = (unsigned int)mp;
			dr = dipc_fill_chain(chain1, mp, TRUE);
			assert(dr == DIPC_SUCCESS);
			KKT_DO_MAPPINGS();
			chain1->callback = dipc_xmit_engine;
			kktr = KKT_REQUEST(handle, chain1);
			s = splkkt();
			simple_lock(&mp->lock);
			/*
			 * It is now safe to release the QUEUEING bit
			 * as our request is on its way.
			 */
			mp->flags &= ~MP_F_QUEUEING;
			if (kktr != KKT_SUCCESS) {
				simple_unlock(&mp->lock);
				splx(s);
				/*
				 * Called with thread_context==FALSE because
				 * we will come back here to free things up.
				 */
				dipc_ool_kkt_error(kktr, chain1, kmsg, mp,
									FALSE);
				splkkt();
				simple_lock(&mp->lock);
			}
			break;

		  default:
			panic("dipc_ool_xfer: invalid state");
		}
	}

	if ((mp->state == MP_S_ERROR_DONE) || (mp->state == MP_S_MSG_DONE)) {
		goto done_early;
	} else if (!steal_context) {
		/*
		 * indicate that there is now NO thread context to come back
		 * to for prepping, faulting, or cleanup.
		 */
		mp->flags &= ~MP_F_THR_AVAIL;
		simple_unlock(&mp->lock);
		splx(s);
		tr1("exit: !steal_context");
		tr_stop();
		return(DIPC_SUCCESS);
	}

	/*
	 * The first chain is now primed and ready to go, the
	 * second chain is also in the queue and ready, and
	 * there are prepped pages in the prep queue.
	 * We sleep and wakeup to pump data as needed, not returning
	 * until the kmsg has been completely sent.
	 */
	while (mp->state == MP_S_READY) {
		simple_unlock(&mp->lock);
		splx(s);
		dipc_prep_n_send(handle, mp);
		s = splkkt();
		simple_lock(&mp->lock);
		if ((mp->flags & MP_F_IDLE) && !(mp->flags & MP_F_PREP_DONE)) {
			continue;
		} else {
			assert_wait((event_t) kmsg, FALSE);
		}
		simple_unlock(&mp->lock);
		splx(s);
		thread_block(0);
		s = splkkt();
		simple_lock(&mp->lock);
	}

	/*
	 * We don't want to continue until all the chains have idled.
	 * the n-1 chain could have woken us up
	 */
done_early:
	while (mp->idle_count < mp->chain_count) {
		assert_wait((event_t) kmsg, FALSE);
		simple_unlock(&mp->lock);
		splx(s);
		thread_block(0);
		s = splkkt();
		simple_lock(&mp->lock);
	}

	assert(mp->state == MP_S_MSG_DONE);
	simple_unlock(&mp->lock);
	splx(s);

	if (mp->error_count != 0) {
		assert(mp->flags & MP_F_RECV);
		assert(FALSE);
		dipc_process_receive_errors(mp);
	}

	/* dispose of transmission resources */
	kktr = KKT_HANDLE_INFO(kmsg->ikm_handle, &info);
	assert(kktr == KKT_SUCCESS);
	KMSG_DROP_HANDLE(kmsg, kmsg->ikm_handle);
	kmsg->ikm_handle = (handle_t)info.node;

	/*
	 *	Free request chains.
	 */
	for (i = 0; i < MAXCHAINS; i++) {
		if (mp->idle_chain[i]) {
			dipc_free_msg_req(mp->idle_chain[i], TRUE);
			mp->chain_count--;
		}
	}
	assert(mp->chain_count == 0);

	/* set up return code */
	if (mp->flags & MP_F_KKT_ERROR)
		dr = DIPC_TRANSPORT_ERROR;
	else
		dr = DIPC_SUCCESS;

	if (!(mp->flags & MP_F_RECV) && steal_context) {
		/*
		 * send side: if an error occurred during transmission,
		 * indicate to the sending thread that it should destroy
		 * the kmsg.
		 */
		assert(steal_context);
		assert(KMSG_HAS_WAITING(kmsg));
		assert(KMSG_DIPC_TRAILER(kmsg)->dipc_sender_kmsg &
								SENDER_BLOCKED);
		if (mp->flags & MP_F_KKT_ERROR)
		    KMSG_DIPC_TRAILER(kmsg)->dipc_sender_kmsg |= MUST_DESTROY;
	}

	tr1("exit: steal_context");
	tr_stop();

	return(dr);
}


void
dipc_connect_callback(
	kkt_return_t	kktr,
	handle_t	handle,
	request_block_t	req,
	boolean_t	thread_context)
{
	msg_progress_t		mp;
	ipc_kmsg_t		kmsg = (ipc_kmsg_t)req->args[0];
	unsigned int		i;
	mach_msg_format_0_trailer_t	*trailer;
	kkt_return_t		kktr2;
	dipc_return_t		dr;
	struct handle_info	info;
	boolean_t		must_destroy = FALSE;
	boolean_t		sender_gone = FALSE;
	spl_t			s;
	TR_DECL("dipc_connect_callback");

	tr_start();
	tr4("enter:  kktr 0x%x handle 0x%x req 0x%x", kktr, handle, req);

	/* Handle callback error codes. */
	switch (kktr) {
	    case KKT_SUCCESS:
		break;

	    /*
	     *	This callback resulted from KKT_HANDLE_ABORT.  We must
	     *	retrieve the handle error code to distinguish the various
	     *	reasons for aborting.  An abort happens in one of three ways:
	     *
	     *	1.  Transport error.  The handle error code (ironically)
	     *	will be KKT_SUCCESS (which is the same as DIPC_SUCCESS and
	     *	is mandated by the spec to be part of all abort code error
	     *	spaces) because	the transport itself is triggering the abort.
	     *
	     *	2.  The receiver responded to KKT_SEND_CONNECT with
	     *	KKT_CONNECT_REPLY but returned a status	other than success,
	     *	e.g., QUEUE_FULL.  The sender then does a (synchronous)
	     *	abort as part of cleaning up in dipc_mqueue_send.
	     *
	     *	3.  The receiver is destroying a message that it had
	     *	initially accepted.
	     */
	    case KKT_ABORT:
		kktr2 = KKT_HANDLE_INFO(handle, &info);
		assert(kktr2 == KKT_ABORT);
		switch (info.abort_code) {
		    case KKT_SUCCESS:
			/* transport error */
			break;

		    case DIPC_RESOURCE_SHORTAGE:
		    case DIPC_PORT_DEAD:
			/*
			 * There is no need to check steal_context
			 * synchronization here since this callback is
			 * occurring synchronously and there are no races.
			 */
			tr_stop();
			return;

		    case DIPC_MSG_DESTROYED:
			/*
			 *	There are races here w.r.t. dipc_mqueue_send.
			 */
			printf("dipc_conn_call:  rcvr said destroy kmsg 0x%x\n",
			       kmsg);
			/* can't return here; need to check HAS_WAITING */
			break;

		    default:
			panic("dipc_connect_callback:  unrecognized code %d\n",
			      info.abort_code);
			break;
		}
		must_destroy = TRUE;
		break;

	    default:
		/* transport error */
		must_destroy = TRUE;
	}

	/*
	 * Handle steal_context synchronization.
	 */
	if (KMSG_HAS_WAITING(kmsg)) {
		trailer = KMSG_DIPC_TRAILER(kmsg);
		s = splkkt();
		kserver_msg_order_lock();
		tr4("kmsg 0x%x has waiting, sender_kmsg flags 0x%x, destroy=%d",
		    kmsg, trailer->dipc_sender_kmsg, must_destroy);
		assert(trailer->dipc_sender_kmsg & CALLBACK_EXPECTED);
		trailer->dipc_sender_kmsg &= ~CALLBACK_EXPECTED;
		if (trailer->dipc_sender_kmsg == 0) {
			/*
			 * no blocked sender any more; this callback must
			 * release or destroy the kmsg if it is inline only.
			 */
			sender_gone = TRUE;
		} else {
			/*
			 * sender thread is still around; it will clean up.
			 * Set the MUST_DESTROY flag if there was a
			 * callback error to tell the sender to destroy the
			 * kmsg instead of releasing it.
			 */
			assert(trailer->dipc_sender_kmsg & SENDER_BLOCKED);
			if (must_destroy == TRUE)
				trailer->dipc_sender_kmsg |= MUST_DESTROY;
		}
		kserver_msg_order_unlock();
		splx(s);
	} else {
		sender_gone = TRUE;
	}

	/*
	 * get the msg_progress pointer out of the trailer
	 */
	mp = KMSG_MSG_PROG_SEND(kmsg);

	/*
	 * since this is the callback from SEND_CONNECT, the kmsg pointer
	 * will be in the remote_port field.
	 */
	assert(kmsg == (ipc_kmsg_t)kmsg->ikm_header.msgh_remote_port);
	/* restore the remote_port */
	kmsg->ikm_header.msgh_remote_port = *(mach_port_t *)req->args[1];

	if (!(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL)) {
		/*
		 * done: this is the callback from SEND_CONNECT and there
		 * is no ool data
		 */
		assert(mp == MSG_PROGRESS_NULL);
		dipc_free_msg_req(req, thread_context);
		KMSG_DROP_HANDLE(kmsg, handle);

		if (sender_gone == TRUE) {
			ipc_port_t	dest;

			dest = (ipc_port_t)kmsg->ikm_header.msgh_remote_port;

			if (must_destroy == TRUE) {
				dipc_kmsg_destroy_delayed(kmsg);
			} else if (thread_context) {
				dipc_kmsg_release(kmsg);
			} else if (!IP_NMS(dest)) {
				s = splkkt();
				simple_lock(&dipc_kmsg_ast_lock);
				ipc_kmsg_enqueue_macro(&dipc_kmsg_ast_queue,
									kmsg);
				dstat(++c_dkaq_length);
				dstat(++c_dkaq_enqueued);
				assert(kmsg->ikm_next != 0);
				assert(kmsg->ikm_next != IKM_BOGUS);
				assert(kmsg->ikm_prev != 0);
				assert(kmsg->ikm_prev != IKM_BOGUS);
				simple_unlock(&dipc_kmsg_ast_lock);
				(void)splsched();
				ast_on(cpu_number(), AST_DIPC);
				splx(s);
			} else {
				dipc_kmsg_release_delayed(kmsg);
			}
		}
		tr_stop();
		return;
	}

	/* This message has OOL data. */
	assert(mp != MSG_PROGRESS_NULL);

	if (must_destroy == TRUE) {
		/* shut down the OOL engine. */
		dipc_ool_kkt_error(kktr, req, kmsg, mp, thread_context);
		tr_stop();
		return;
	}

	/*
	 * Reset the callback to use the xmit engine.
	 */
	bzero((char *)req->args[1], sizeof(vm_page_t) * CHAINSIZE);
	req->callback = dipc_xmit_engine;
	req->args[0] = (unsigned int)mp;

	s = splkkt();
	simple_lock(&mp->lock);
	if (mp->flags & MP_F_QUEUEING) {
		boolean_t	unlocked;

		unlocked = dipc_queue_idle_chain(mp, req, TRUE, 
						 s, thread_context);
		if (!unlocked) {
			simple_unlock(&mp->lock);
			splx(s);
		}
	} else {
		mp->flags |= MP_F_QUEUEING;
	
		if (mp->state == MP_S_OOL_DONE) {
			/*
			 * We are finished with a small request -
			 * do post processing now.
			 */
			tr2("send zero header req 0x%x\n", req);
			mp->state = MP_S_ERROR_DONE;
			/*
			 *	Release msg_progress lock.
			 */
			simple_unlock(&mp->lock);
			splx(s);
			dipc_send_error_trailer(mp, req);
			tr1("exit");
			tr_stop();
			return;
		}

		/*
		 * dipc_fill_chain can be called with locks held
		 * so long as can_block == FALSE.  req holds the
		 * inline kmsg, so we don't need to unpin it.
		 */
		assert(mp->state == MP_S_READY);
		dr = dipc_fill_chain(req, mp, FALSE);
		if (dr != DIPC_SUCCESS) {
			(void) dipc_queue_idle_chain(mp, req, TRUE, s, FALSE);
			simple_unlock(&mp->lock);
			splx(s);
		} else {
			assert(mp->state == MP_S_READY ||
				mp->state == MP_S_OOL_DONE);
			simple_unlock(&mp->lock);
			splx(s);
			/*
			 * XXX Why release the lock here?  KKT_REQUEST
			 * XXX can be called from interrupt or whereever,
			 * XXX so wouldn't it be better to hold the lock
			 * XXX and prevent callbacks?
			 */
			KKT_DO_MAPPINGS();
			kktr2 = KKT_REQUEST(handle, req);
			s = splkkt();
			simple_lock(&mp->lock);
			mp->flags &= ~MP_F_QUEUEING;
			simple_unlock(&mp->lock);
			splx(s);
			if (kktr2 != KKT_SUCCESS) {
				dipc_ool_kkt_error(kktr2, req, kmsg, mp,
							thread_context);
			}
		}
	}

	tr1("exit");
	tr_stop();
}

/*
 * dipc_ool_kkt_error
 *
 * Description
 *	KKT has returned some sort of error for a message with OOL data.
 *	Shut down the OOL engine and let it clean itself up and destroy
 *	the kmsg.
 */
void
dipc_ool_kkt_error(
	kkt_return_t	kktr,
	request_block_t	req,
	ipc_kmsg_t	kmsg,
	msg_progress_t	mp,
	boolean_t	thread_context)
{
	boolean_t	unlocked;
	spl_t		s;
	TR_DECL("dipc_ool_kkt_error");

	tr_start();
	tr5("enter:  kktr 0x%x req 0x%x kmsg 0x%x mp 0x%x",
		kktr, req, kmsg, mp);

	/*
	 *	We currently don't believe that we are exercising
	 *	this path -- and want to make sure of that.
	 */
	assert(FALSE);

	s = splkkt();
	simple_lock(&mp->lock);
	mp->flags |= MP_F_KKT_ERROR;
	mp->state = MP_S_MSG_DONE;
	unlocked = dipc_queue_idle_chain(mp, req, FALSE, s, thread_context);
	if (!unlocked) {
		simple_unlock(&mp->lock);
		splx(s);
	}

	tr1("exit");
	tr_stop();
	return;
}

/*
 *	dipc_queue_idle_chain
 *
 *	Put an idle request chain into the idle_chain area of
 *	a msg_progress structure.  Set the appropriate state and
 *	enqueue the chain for further processing.  The idle_chain
 *	area is guaranteed to be big enough to hold the total number
 *	of chains.
 *	If no request is present, enqueue the msg_progress field anyway
 *	so we can wake up the prep thread to fill the pipeline, or
 *	wake up the msg_progress freeing thread to clean things up.
 *
 *	If thread_context is TRUE and mp->state is MSG_DONE, then the mp
 *	and kmsg will be freed here.  In this case, the mp->lock will
 *	be unlocked before the mp is freed, and TRUE will be returned
 *	to the caller indicating that the lock was released.  If
 *	thread_context is FALSE, the lock will not be affected.
 *
 *	INPUT
 *		mp->lock is taken.
 */
boolean_t
dipc_queue_idle_chain(
	msg_progress_t	mp,
	request_block_t	req,
	boolean_t	wakeup,
	spl_t		ipl,
	boolean_t	thread_context)
{
	boolean_t	active;
	register int	i;
	spl_t		s;
	TR_DECL("dipc_queue_idle_chain");

	tr_start();
	tr4("enter:  mp 0x%x req 0x%x wakeup 0x%x", mp, req, wakeup);

	/* first idle this request chain (if we have one) */

	if (req) {
		for (i = 0; i < MAXCHAINS; i++) {
			if (mp->idle_chain[i] == NULL_REQUEST) {
				mp->idle_chain[i] = req;
				mp->idle_count++;
				assert(mp->idle_count <= mp->chain_count);
				break;
			}
		}
		mp->flags |= MP_F_IDLE;
	}


	if (mp->state == MP_S_MSG_DONE) {
		boolean_t	unlocked = FALSE;

		if (mp->idle_count == mp->chain_count) {
			ipc_port_t	dest;

			/*
			 * there's nothing left to do on this msg progress and
			 * all request chains are finished.  Just get somebody
			 * to free it.  For the receive side and steal_context
			 * send side, this will be the thread blocked in
			 * dipc_ool_xfer, so wake it up.  If we're in thread
			 * context, just go ahead and free it.  Otherwise,
			 * queue this up for the dipc_msg_prep_free thread.
			 */

			assert(mp->error_count == 0); /* XXX */

			dest = (ipc_port_t)
					mp->kmsg->ikm_header.msgh_remote_port;

			if (mp->flags & MP_F_THR_AVAIL) {
				/* steal_context */
				mp->flags &= ~MP_F_OOL_IN_PROG;
				thread_wakeup((event_t)mp->kmsg);
			} else if (thread_context) {
				simple_unlock(&mp->lock);
				splx(ipl);
				unlocked = TRUE;
				dipc_msg_prep_free(mp);
			} else if (!IP_NMS(dest) &&
			    !(mp->flags & MP_F_KKT_ERROR)) {
				/*
				 * If the kmsg does not need to be destroyed
				 * and the destination port is not using
				 * NMS, then the resources can be freed with
				 * an AST rather than wasting time context
				 * switching to a thread.
				 */

				assert(!(mp->flags & MP_F_RECV));
				tr2("queueing mp 0x%x to be freed by ast", mp);
				s = splkkt();
				simple_lock(&dipc_msg_prep_free_lock);
				mp->prep_next = dipc_msg_prep_free_ast_list;
				dipc_msg_prep_free_ast_list = mp;
				dstat(++c_dmp_free_ast_list_length);
				dstat(++c_dmp_free_ast_list_enqueued);
				simple_unlock(&dipc_msg_prep_free_lock);
				(void)splsched();
				ast_on(cpu_number(), AST_DIPC);
				(void)splx(s);
			} else {
				assert(!(mp->flags & MP_F_RECV));
				tr2("queueing mp 0x%x to be freed", mp);
				/*
				 * At this point we are in interrupt
				 * context so we do not need to spl.
				 */
				simple_lock(&dipc_msg_prep_free_lock);
				mp->prep_next = dipc_msg_prep_free_list;
				dipc_msg_prep_free_list = mp;
				dstat(++c_dmp_free_list_length);
				dstat(++c_dmp_free_list_enqueued);
				active = dipc_msg_prep_free_thread_active;
				simple_unlock(&dipc_msg_prep_free_lock);
				if (active == FALSE)
				    thread_wakeup_one(
					(event_t)&dipc_msg_prep_free_thread);
			}
		}

		/*
		 * (else) there's still an outstanding chain and the mp
		 * will be queued to be freed when the last chain gets here.
		 */
		tr_stop();
		return unlocked;
	}

	if (!wakeup) {
		tr_stop();
		return FALSE;
	}

	if (!(mp->flags & MP_F_THR_AVAIL)) {
		/*
		 * No thread context to go back to: processing is done
		 * by the prep thread.
		 */
		if ( !(mp->flags & MP_F_PREP_QUEUE)) {
			/* Only enqueue one mp structure */
			mp->flags |= MP_F_PREP_QUEUE;
			s = splkkt();
			simple_lock(&dipc_msg_prep_queue_lock);
			if (dipc_msg_prep_queue)
				dipc_msg_prep_queue_tail->prep_next = mp;
			else
				dipc_msg_prep_queue = mp;
			dipc_msg_prep_queue_tail = mp;
			dstat(++c_dmp_queue_length);
			dstat(++c_dmp_queue_enqueued);
			if (!dipc_prep_thread_active) {
				dipc_prep_thread_active = TRUE;
				thread_wakeup_one(
					(event_t)&dipc_prep_thread_active);
			}
			simple_unlock(&dipc_msg_prep_queue_lock);
			splx(s);
		}
	} else {
		thread_wakeup_one((event_t)mp->kmsg);
	}

	tr1("exit:");
	tr_stop();
	return FALSE;
}

/*
 * Routine: dipc_xmit_engine
 *
 *	Interupt context callback routine.  Will put request back into
 *	the pipe and try to keep data flowing.  If there are no pages
 *	available, wakeup the thread portion of this routine to
 *	fill the pipe and fire it up again.
 *
 *	N.B.  In this implementation, we don't wake up a thread
 *	ahead of time to replenish the prepped pages.
 */
void
dipc_xmit_engine(
	kkt_return_t	kktr,
	handle_t	handle,
	request_block_t	req,
	boolean_t	thread_context)
{
	msg_progress_t	mp;
	register	i;
	ipc_kmsg_t	kmsg;
	dipc_return_t	dr;
	kkt_return_t	kr;
	boolean_t	unlocked = FALSE;
	spl_t		s;
	TR_DECL("dipc_xmit_engine");

	tr_start();
	tr2("enter:  req 0x%x", req);

	mp = (msg_progress_t)req->args[0];
#if SJSCHECK
	sjs_check_data(req);
#endif /* SJSCHECK */

	dipc_unpin_request(req);
	if (kktr != KKT_SUCCESS) {
		dipc_ool_kkt_error(kktr, req, mp->kmsg, mp, thread_context);
		tr_stop();
		return;
	}

	s = splkkt();
	simple_lock(&mp->lock);
	if ((mp->flags & MP_F_QUEUEING) && (mp->chain_count > 1)) {
		unlocked = dipc_queue_idle_chain(mp, req,
				(mp->state == MP_S_READY), s, thread_context);
	} else {
		/*
		 * Make sure we are the only one filling a request chain,
		 * otherwise we risk getting pages out of order.  Notice
		 * that this is enforced by a flags bit rather than a
		 * long held lock, as anything can happen while we wait
		 * to fill a chain.
		 */
		mp->flags |= MP_F_QUEUEING;

		assert(!mp->page_err_info);	/* XXX alanl XXX */

		switch (mp->state) {
		  case MP_S_READY:
			/*
			 * dipc_fill_chain can be called with locks held so
			 * long as can_block == FALSE.
			 */
			dr = dipc_fill_chain(req, mp, FALSE);
			if (dr != DIPC_SUCCESS && req->data_length == 0) {
				/*
				 * need prep or fault, and we have nothing
				 * to send now.
				 */
				assert(dr == DIPC_P_NEED_PREP ||
					dr == DIPC_P_NEED_FAULT);
				assert(!mp->page_err_info); /* XXX alanl XXX */
				(void) dipc_queue_idle_chain(mp, req, TRUE, s,
									FALSE);
			} else {
				/*
				 *	If we are handling the last request,
				 *	the state will be OOL_DONE when we
				 *	return from dipc_fill_chain.
				 *	Otherwise, we should remain in
				 *	state READY.
				 */
				assert(mp->state == MP_S_READY ||
				       mp->state == MP_S_OOL_DONE);
				simple_unlock(&mp->lock);
				splx(s);
				KKT_DO_MAPPINGS();
				kr = KKT_REQUEST(handle, req);
				if (kr != KKT_SUCCESS) {
					unlocked = TRUE;
					dipc_ool_kkt_error(kr, req, kmsg, mp,
						thread_context);
					break;
				}
				s = splkkt();
				simple_lock(&mp->lock);
				if (dr == DIPC_P_NEED_PREP)
					(void) dipc_queue_idle_chain(mp,
						NULL_REQUEST, TRUE, s, FALSE);
			}
			mp->flags &= ~MP_F_QUEUEING;
			break;

		  case MP_S_OOL_DONE:
			mp->state = MP_S_ERROR_DONE;
			simple_unlock(&mp->lock);
			splx(s);
			if (mp->flags & MP_F_RECV)
				dipc_receive_error_trailer(mp, req);
			else
				dipc_send_error_trailer(mp, req);
			unlocked = TRUE;
			break;

		  case MP_S_ERROR_DONE:
		  case MP_S_MSG_DONE:
			mp->state = MP_S_MSG_DONE;
			unlocked = dipc_queue_idle_chain(mp, req, FALSE, s,
								thread_context);
			break;

		  default:
			panic("dipc_xmit_engine: bad state");
		}

	}

	if (!unlocked) {
		simple_unlock(&mp->lock);
		splx(s);
	}

	tr1("exit");
	tr_stop();
}

/*
 * dipc_xmit_done
 *
 * Description
 *	Final callback, received after the error trailer information 
 *	has been sent.
 */
void
dipc_xmit_done(
	kkt_return_t	kktr,
	handle_t	handle,
	request_block_t	req,
	boolean_t	thread_context)
{
	msg_progress_t	mp;
	register	i;
	ipc_kmsg_t	kmsg;
	dipc_return_t	dr;
	boolean_t	unlocked;
	spl_t		s;
	TR_DECL("dipc_xmit_done");

	tr_start();
	tr2("enter:  req 0x%x", req);

	mp = (msg_progress_t)req->args[0];

	if (kktr != KKT_SUCCESS) {
		dipc_ool_kkt_error(kktr, req, mp->kmsg, mp, thread_context);
		tr2("exit, kkt err 0x%x", kktr);
		tr_stop();
		return;
	}

	s = splkkt();
	simple_lock(&mp->lock);
	assert(mp->state == MP_S_ERROR_DONE ||
		(mp->state == MP_S_MSG_DONE && mp->chain_count > 1));
	assert(mp->state == MP_S_ERROR_DONE || mp->state == MP_S_MSG_DONE);

	if (mp->flags & MP_F_RECV) {
		/* get error count and decide if there's more to come */
		mp->error_count =
			*(unsigned int *)(req->data.list->ios_address + 4);

		assert(mp->error_count == 0);	/* XXX alanl XXX */

		/* dipc_ool_xfer will get the error vector, if any */
	}

	mp->state = MP_S_MSG_DONE;

	unlocked = dipc_queue_idle_chain(mp, req, FALSE, s, thread_context);
	if (!unlocked) {
		simple_unlock(&mp->lock);
		splx(s);
	}

	tr1("exit");
	tr_stop();
}

void
dipc_send_err_callback(
	kkt_return_t	kktr,
	handle_t	handle,
	request_block_t	req)
{
	return;
}

dipc_return_t
dproc_wakeup_sender(
	unsigned int	waitchan,
	dipc_return_t	status)
{
	ipc_kmsg_t	kmsg;
	mach_msg_format_0_trailer_t	*trailer;
	spl_t		s;
	TR_DECL("dproc_wakeup_sender");

	tr_start();
	tr2("waitchan=0x%x", waitchan);

	kmsg = (ipc_kmsg_t)waitchan;
	assert(kmsg != IKM_NULL);
	if (!KMSG_HAS_WAITING(kmsg)) {
		if (status != DIPC_SUCCESS)
			panic("dproc_wakeup_sender:  status 0x%x\n", status);
		tr_stop();
		return DIPC_SUCCESS;
	}
	tr2("waking up kmsg 0x%x", kmsg);
	trailer = KMSG_DIPC_TRAILER(kmsg);
	s = splkkt();
	kserver_msg_order_lock();
	assert(trailer->dipc_sender_kmsg & RPC_EXPECTED);
	if (status != DIPC_SUCCESS) {
		/*
		 *	Receiver wound up destroying its kmsg, so
		 *	we must destroy our own copy.  (Note that
		 *	in this case the receiver did NOT reclaim
		 *	transits; we will do so on the send side.)
		 */
		trailer->dipc_sender_kmsg |= MUST_DESTROY;
	}
	trailer->dipc_sender_kmsg &= ~RPC_EXPECTED;
	kserver_msg_order_unlock();
	splx(s);
	thread_wakeup((event_t)kmsg);
	tr_stop();
	return DIPC_SUCCESS;
}

/* functions for blocked senders processing */

/*
 * Enqueue the kmsg on the proxy's msgqueue.  The port is locked and must
 * be kept locked.
 */
dipc_return_t
dipc_kmsg_proxy_enqueue(
	ipc_kmsg_t	kmsg,
	ipc_port_t	port)
{
	ipc_mqueue_t	mqueue;

	assert(IP_IS_REMOTE(port));
	assert(!KMSG_HAS_WAITING(kmsg));
	mqueue = &port->ip_messages;
	imq_lock(mqueue);
	ipc_kmsg_enqueue_macro(&mqueue->imq_messages, kmsg);
	port->ip_msgcount++;
	imq_unlock(mqueue);
	return DIPC_SUCCESS;
}

/*
 * port must be locked
 */
dipc_return_t
dipc_setup_blocked_node(
	node_name	node,
	ipc_port_t	port,
	node_wait_t	new_nwait)
{
	node_wait_t	nwait, headp;
	ipc_port_t	pl;

	headp = &blocked_node_list_head;
	assert(port->ip_node_wait == 0);

	port->ip_node_wait_list = IP_NULL;

	blocked_node_list_lock();

	/* first see if this node is in the blocked node list */
	nwait = headp->next;
	while (nwait != headp) {
		if (nwait->node == node)
			break;
		nwait = nwait->next;
	}

	if (nwait == headp) {
		/* node wasn't in list.  Use the new node_wait struct */
		assert(new_nwait != NODE_WAIT_NULL);
		nwait = new_nwait;

		/* initialize new struct */
		nwait->node = node;
		nwait->port_list = port;
		nwait->unblocking = FALSE;

		/* insert into list */
		nwait->prev = headp->prev;
		nwait->next = headp;
		headp->prev = nwait;
		if (headp->next == headp)
			headp->next = nwait;
		else
			nwait->prev->next = nwait;
	} else {
		/*
		 * node already in list.  Add this port to the end of the
		 * port list.  Free the new nwait allocated by the caller.
		 */
		assert(nwait->port_list != IP_NULL);
		assert(new_nwait != NODE_WAIT_NULL);
		zfree(node_wait_zone, (vm_offset_t)new_nwait);
		if (nwait->unblocking) {
			/* this node is being unblocked.  retry the send */
			blocked_node_list_unlock();
			return DIPC_RETRY_SEND;
		}
		pl = nwait->port_list;
		while (pl->ip_node_wait_list != IP_NULL) {
			assert(pl != port);   /* make sure not already there */
			pl = pl->ip_node_wait_list;
		}
		pl->ip_node_wait_list = port;
	}

	/* take a reference on the port while it is on nwait->port_list */
	ip_reference(port);

	port->ip_node_wait = (unsigned int *)nwait;
	
	blocked_node_list_unlock();

	return DIPC_SUCCESS;
}

/*
 * port is locked
 */
dipc_return_t
dipc_remove_blocked_node(
	node_wait_t	nwait,
	ipc_port_t	port)
{
	ipc_port_t	p, prevp;

	assert(port->ip_rs_block);

	blocked_node_list_lock();

	/* find this port on the list */
	if (port == nwait->port_list) {
		/* port is first on the list */
		if (port->ip_node_wait_list == IP_NULL) {
			/*
			 * this is the only port for this node.  Remove the
			 * node_wait struct from the blocked node list and
			 * free it.
			 */
			nwait->prev->next = nwait->next;
			nwait->next->prev = nwait->prev;
			zfree(node_wait_zone, (vm_offset_t)nwait);
		} else {
			/* remove this port */
			nwait->port_list = port->ip_node_wait_list;
		}
	} else {
		/*
		 * port is not first on list, and therefore is not the
		 * only port on the list.
		 */
		prevp = nwait->port_list;
		p = nwait->port_list->ip_node_wait_list;
		while (p != IP_NULL) {
			if (p == port) {
				/* remove port from list */
				prevp->ip_node_wait_list = p->ip_node_wait_list;
				break;
			}
			prevp = p;
			p = p->ip_node_wait_list;
		}
		assert(p == port);	/* assert that port was on list */
	}

	/*
	 * release the ref (taken by dipc_setup_blocked_node) since this
	 * port is no longer on the list
	 */
	ip_release(port);

	blocked_node_list_unlock();

	return DIPC_SUCCESS;
}

/*
 * Restart sending on this port.  First attempt to send any messages that
 * have accumulated on the proxy's queue, then wake up any blocked senders.
 * A placeholder for the message is left on the message queue until AFTER
 * the message has been successfully sent (i.e. enqueued on the remote node).
 * Port must have ip_resending set.
 *
 * dipc_restart_send will consume a port ref.
 */
void
dipc_restart_send(
	ipc_port_t	port)
{
	mach_msg_return_t	mr;
	ipc_mqueue_t		mqueue = &port->ip_messages;
	ipc_kmsg_queue_t	messages = &mqueue->imq_messages;
	ipc_kmsg_t		kmsg;
	ipc_thread_t		sender;
	ipc_kmsg_t		placeholder;


	placeholder = (ipc_kmsg_t) dipc_meta_kmsg_alloc(TRUE);
	placeholder->ikm_header.msgh_bits |= MACH_MSGH_BITS_PLACEHOLDER;

	ip_lock(port);
	assert(IP_IS_REMOTE(port));
	assert(port->ip_resending);
	if (port->ip_qf_count > 0) {
		port->ip_resending = FALSE;
		ip_release(port);
		ip_unlock(port);
		return;
	}

	while (port->ip_msgcount > 0) {
		if (!ip_active(port)) {
			dipc_meta_kmsg_free((meta_kmsg_t) placeholder);

			/* consume port ref donated to this routine */
			ip_release(port);
			ip_check_unlock(port);
			return;
		}

		imq_lock(mqueue);
		/*
		 *	Replace kmsg on queue with a placeholder.
		 *	This guarantees that no other message
		 *	races ahead, because a new sender will check
		 *	for blocked messages and blocked senders.
		 */
		kmsg = ipc_kmsg_queue_first(messages);
		assert(kmsg != IKM_NULL);
		if (kmsg->ikm_next == kmsg) {
			placeholder->ikm_next = placeholder;
			placeholder->ikm_prev = placeholder;
		} else {
			placeholder->ikm_next = kmsg->ikm_next;
			placeholder->ikm_prev = kmsg->ikm_prev;
			kmsg->ikm_next->ikm_prev = placeholder;
			kmsg->ikm_prev->ikm_next = placeholder;
		}
		messages->ikmq_base = placeholder;
		kmsg->ikm_next = IKM_BOGUS;
		kmsg->ikm_prev = IKM_BOGUS;
		imq_unlock(mqueue);

		mr = dipc_mqueue_send(kmsg, MACH_SEND_LOCAL_QUEUE,
			MACH_MSG_TIMEOUT_NONE);
		/* port was unlocked on SUCCESS by dipc_mqueue_send */

		/*
		 *	Remove placeholder from the queue;
		 *	put back the kmsg if we didn't send it.
		 *	If dipc_mqueue_send was successful, then
		 *	it's the responsibility of the message
		 *	transmission engine and we can't touch
		 *	the kmsg again from this code.
		 */
		if (mr == MACH_MSG_SUCCESS)
		    ip_lock(port);
		imq_lock(mqueue);
		assert(ipc_kmsg_queue_first(messages) == placeholder);
		if (mr == MACH_MSG_SUCCESS) {
			ipc_kmsg_rmqueue_first_macro(messages, placeholder);
			port->ip_msgcount--;
			assert(port->ip_msgcount == 0 ||
			       !ipc_kmsg_queue_empty(messages));
			imq_unlock(mqueue);
			continue;
		} else {
			/*
			 *	We'll handle all the errors below.
			 *	For now, put the kmsg back.
			 */
			if (placeholder->ikm_next == placeholder) {
				kmsg->ikm_next = kmsg;
				kmsg->ikm_prev = kmsg;
			} else {
				kmsg->ikm_next = placeholder->ikm_next;
				kmsg->ikm_prev = placeholder->ikm_prev;
				kmsg->ikm_next->ikm_prev = kmsg;
				kmsg->ikm_prev->ikm_next = kmsg;
			}
			messages->ikmq_base = kmsg;
		}
		imq_unlock(mqueue);

		/*
		 *	Error cases.
		 */
		switch (mr) {
		  case MACH_SEND_TRANSPORT_ERROR:
		  case MACH_SEND_INVALID_DEST:
			/*
			 * Port is unusable.  If it's not already dead,
			 * destroy it now.
			 */

			if (!ip_active(port)) {
				/*
				 * something else beat us to the destroy.
				 * Nothing left to do except consume the
				 * port ref donated to this routine.
				 */
				ip_release(port);
				ip_check_unlock(port);
				break;
			}
			
			/*
			 * reference donated to this routine will be consumed
			 * by ipc_port_destroy.  ipc_port_destroy will clean
			 * up the message queue and wake up blocked senders.
			 */
			ipc_port_destroy(port);		/* consumes lock */
			break;

		  case MACH_SEND_RESEND_FAILED:
			/*
			 * We encountered RESOURCE_SHORTAGE or QUEUE_FULL
			 * again.  Give up now; another callback will start
			 * things up again.
			 */
			assert(port->ip_qf_count > 0 || port->ip_rs_block);
			assert(port->ip_msgcount > 0 &&
			       !ipc_kmsg_queue_empty(messages));
			port->ip_resending = FALSE;
			ip_release(port);	/* consume donated ref */
			ip_unlock(port);
			break;

		  default:
			printf("dipc_restart_send: invalid mr 0x%x\n", mr);
			panic("dipc_restart_send");
			port->ip_resending = FALSE;
			ip_release(port);
			ip_unlock(port);
			break;
		}

		dipc_meta_kmsg_free((meta_kmsg_t) placeholder);
		return;
	}

	port->ip_resending = FALSE;

	/* wake up dipc blocked senders */
	while ((sender = ipc_thread_dequeue(&port->ip_blocked)) != ITH_NULL) {
		sender->ith_state = MACH_MSG_SUCCESS;
		thread_go(sender);
	}

	ip_release(port);	/* consume donated ref */
	ip_unlock(port);
	dipc_meta_kmsg_free((meta_kmsg_t) placeholder);
}

/*
 * Unblock port after previous QUEUE_FULL condition.
 */
dipc_return_t
dproc_unblock_port(
	uid_t		*uidp,
	boolean_t	port_dead)
{
	dipc_return_t	dr;
	ipc_port_t	port;
	TR_DECL("dproc_unblock_port");

	/* look up port */
	dr = dipc_uid_lookup(uidp, &port);
	if (dr != DIPC_SUCCESS) {
		tr2("...failed, err=%d", dr);
		return dr;
	}
	dipc_uid_port_reference(port);

	tr2("unblocking port 0x%x", port);
	ip_lock(port);
	assert(ip_active(port));
	port->ip_qf_count--;
	if ((port->ip_qf_count != 0) || port->ip_resending) {
		/*
		 * If we are already resending, that thread will take
		 * care of this.  If we are not, but the count is not 0,
		 * other events/threads will deal with this.
		 */

		ip_release(port);
	} else {
		assert(port->ip_resending == FALSE);

		if ((port_dead == TRUE) && ip_active(port)) {
			/*
			 * port is dead on principal node and hasn't yet
			 * been destroyed here.  Call ipc_port_destroy which
			 * will mark the proxy dead, clean up the msg queue,
			 * and unblock senders.  The reference taken by
			 * dipc_uid_port_reference will be consumed by
			 * ipc_port_destroy.
			 */
			ipc_port_destroy(port);
			return DIPC_SUCCESS;
		}

		/* sender is blocked (or kmsg enqueued locally) */
		port->ip_resending = TRUE;

		/*
		 * enqueue port for restart service thread, leaving the
		 * extra ref taken by dipc_uid_port_reference.  This reference
		 * will be dropped by dipc_restart_send.
		 */
		restart_port_list_lock();
		port->ip_restart_list = IP_NULL;
		if (restart_port_list_head == IP_NULL) {
			assert(restart_port_list_tail == IP_NULL);
			restart_port_list_head = port;
			restart_port_list_tail = port;
		} else {
			restart_port_list_tail->ip_restart_list = port;
			restart_port_list_tail = port;
		}
		dstat(++c_rp_list_length);
		dstat(++c_rp_list_enqueued);
		restart_port_list_unlock();
		thread_wakeup_one((event_t)&restart_port_list_head);
	}
	ip_unlock(port);

	return DIPC_SUCCESS;
}

dipc_return_t
dproc_unblock_node(
	node_name	receiving_node)
{
	node_wait_t	nwait, headp;
	ipc_port_t	port;

	headp = &blocked_node_list_head;

	blocked_node_list_lock();

	/* find the node_wait struct for this node */
	nwait = headp->next;
	while (nwait != headp) {
		if (nwait->node == receiving_node)
			break;
		nwait = nwait->next;
	}
	if ((nwait == headp) || nwait->unblocking) {
		/*
		 * either this node is not on the list, or it's already
		 * being unblocked.  Nothing to do.
		 */
		blocked_node_list_unlock();
		return DIPC_SUCCESS;
	}

	/*
	 * set the unblocking bit to show that a node unblock is in progress.
	 * Once it's set, the list can be unlocked.
	 */
	nwait->unblocking = TRUE;
	blocked_node_list_unlock();

	/* for each port in the list, enqueue it for restart service */
	while ((port = nwait->port_list) != IP_NULL) {
		ip_lock(port);
		assert(ip_active(port));
		assert(port->ip_rs_block);

		/*
		 * remove port from list.  It has an extra reference obtained
		 * when it was placed on the list.
		 */
		nwait->port_list = port->ip_node_wait_list;
		port->ip_rs_block = FALSE;

		if (port->ip_resending) {
			/*
			 * port could have been restarted by unblock_port (after
			 * QUEUE_FULL).  Let the other resend do the work.
			 */
			ip_release(port);	/* drop extra ref */
			ip_unlock(port);
			continue;
		}

		/* sender is blocked (or kmsg enqueued locally) */
		port->ip_resending = TRUE;

		/*
		 * enqueue port for restart service thread, leaving the
		 * extra ref taken for the nwait port list.  This reference
		 * will be dropped by dipc_restart_send.
		 */
		restart_port_list_lock();
		port->ip_restart_list = IP_NULL;
		if (restart_port_list_head == IP_NULL) {
			assert(restart_port_list_tail == IP_NULL);
			restart_port_list_head = port;
			restart_port_list_tail = port;
		} else {
			restart_port_list_tail->ip_restart_list = port;
			restart_port_list_tail = port;
		}
		dstat(++c_rp_list_length);
		dstat(++c_rp_list_enqueued);
		restart_port_list_unlock();
		thread_wakeup_one((event_t)&restart_port_list_head);
		ip_unlock(port);
	}

	/* remove nwait structure from blocked node list and free it */
	blocked_node_list_lock();
	nwait->unblocking = FALSE;
	nwait->prev->next = nwait->next;
	nwait->next->prev = nwait->prev;
	zfree(node_wait_zone, (vm_offset_t)nwait);
	blocked_node_list_unlock();

	return DIPC_SUCCESS;
}

void
dipc_restart_port_thread(void)
{
	ipc_port_t	port;

	for(;;) {
		restart_port_list_lock();
		while (restart_port_list_head != IP_NULL) {
			port = restart_port_list_head;
			if (port->ip_restart_list == IP_NULL) {
				/* this was the only port on the list */
				assert(restart_port_list_tail == port);
				restart_port_list_head = IP_NULL;
				restart_port_list_tail = IP_NULL;
			} else {
				restart_port_list_head = port->ip_restart_list;
				port->ip_restart_list = IP_NULL;
			}
			dstat_max(c_rp_list_max_length, c_rp_list_length);
			dstat(--c_rp_list_length);
			dstat(++c_rp_list_handled);
			restart_port_list_unlock();
			
			dipc_restart_send(port);
			
			restart_port_list_lock();
		}
		assert_wait((event_t)&restart_port_list_head, FALSE);
		restart_port_list_unlock();
		
		thread_block((void (*)(void))0);
	}
}

/*
 *	dipc_prep_n_send
 *
 *	Given a msg_progress structure, prep pages and send the chains
 *	along.  Don't exit until all chains are on the wire or there
 *	is nothing left to send.
 */
void
dipc_prep_n_send(
	handle_t	handle,
	msg_progress_t	mp)
{
	request_block_t	req;
	register	i;
	dipc_return_t	dr;
	kkt_return_t	kktr;
	spl_t		s;
	boolean_t	partial;
	TR_DECL("dipc_prep_n_send");

	tr_start();
	tr2("enter: mp 0x%x", mp);

	/*
	 * The trick to avoid locking here is to sample the idle count
	 * when we enter, then compare it when we have been through
	 * the idle chains once.  If the mp->idle_count has increased,
	 * we know that the data has gone and been serviced in interrupt
	 * mode.  Only lock and adjust once.
	 */
	while (!(mp->flags & MP_F_PREP_DONE)) {
		if (mp->prep_queue.pslock == 0) {
			(void)dipc_prep_pages(mp);
		}

		s = splkkt();
		simple_lock(&mp->lock);
		if (mp->flags & MP_F_IDLE) {
			for (i = 0; i < MAXCHAINS; i++) {
				/*
				 *	Unfortunately, in the current scheme
				 *	it's possible that the caller has
				 *	already started a chain that has
				 *	transferred the entire message and
				 *	is finishing up right about now.
				 *	We don't learn of this until we drop
				 *	the mp lock, which is also fending off
				 *	interrupts.  So each time through the
				 *	loop, we must check whether the
				 *	message has finished.  We may also
				 *	have caused MP_S_OOL_DONE to be set,
				 *	ourselves, on the previous pass through
				 *	the loop.
				 */
				if (mp->state == MP_S_OOL_DONE) {
					assert(mp->flags & MP_F_PREP_DONE);
					break;
				}
				req = mp->idle_chain[i];
				if (req) {
					mp->flags |= MP_F_QUEUEING;
					assert(mp->state == MP_S_READY);
					simple_unlock(&mp->lock);
					splx(s);

					dr = dipc_fill_chain(req, mp, TRUE);
					assert(dr != DIPC_P_NEED_PREP);
					s = splkkt();
					simple_lock(&mp->lock);
					if (dr != DIPC_SUCCESS) {
						mp->flags &= ~MP_F_QUEUEING;
						break;
					}
					mp->idle_chain[i] = NULL_REQUEST;
					mp->idle_count--;
					assert(req->callback ==
					       dipc_xmit_engine);
					simple_unlock(&mp->lock);
					splx(s);
					KKT_DO_MAPPINGS();
					kktr = KKT_REQUEST(handle, req);
					if (kktr != KKT_SUCCESS) {
						dipc_ool_kkt_error(kktr, req,
							mp->kmsg, mp, TRUE);
						tr2("exit, kkt_error 0x%x",
							kktr);
						tr_stop();
						return;
					}
					s = splkkt();
					simple_lock(&mp->lock);
					mp->flags &= ~MP_F_QUEUEING;
				}
			}
			if (mp->idle_count == 0)
			    mp->flags &= ~MP_F_IDLE;
		} else {
			simple_unlock(&mp->lock);
			splx(s);
			break;
		}
		simple_unlock(&mp->lock);
		splx(s);
	}

	tr1("exit");
	tr_stop();
}

/*
 * Free up various resources without having to context switch to
 * one or more cleanup threads.  This routine may not block.
 */
void
dipc_ast(void)
{
	msg_progress_t	mp;
	request_block_t	req;
	ipc_kmsg_t	kmsg;
	spl_t		s;
	TR_DECL("dipc_ast");

	/*
	 * look for msg_progress structs (ool kmsgs) to deallocate.
	 */
	s = splkkt();
	simple_lock(&dipc_msg_prep_free_lock);
	while ((mp = dipc_msg_prep_free_ast_list) != MSG_PROGRESS_NULL) {
		dipc_msg_prep_free_ast_list = mp->prep_next;
		dstat_max(c_dmp_free_ast_list_max_length,
			  c_dmp_free_ast_list_length);
		dstat(--c_dmp_free_ast_list_length);
		dstat(++c_dmp_free_ast_list_handled);
		simple_unlock(&dipc_msg_prep_free_lock);
		splx(s);

		tr3("freeing mp 0x%x kmsg 0x%x", mp, mp->kmsg);
		assert(!(mp->flags & MP_F_KKT_ERROR));
		assert(
		    !IP_NMS((ipc_port_t)mp->kmsg->ikm_header.msgh_remote_port));
		dipc_msg_prep_free(mp);
		s = splkkt();
		simple_lock(&dipc_msg_prep_free_lock);
	}
	simple_unlock(&dipc_msg_prep_free_lock);

	/*
	 * look for request blocks to release.
	 */
	simple_lock(&dipc_msg_req_list_lock);
	while ((req = dipc_msg_req_list) != NULL_REQUEST) {
		dipc_msg_req_list = req->next;
		dstat_max(c_dmr_list_max_length, c_dmr_list_length);
		dstat(--c_dmr_list_length);
		dstat(++c_dmr_list_handled);
		simple_unlock(&dipc_msg_req_list_lock);
		splx(s);

		tr2("freeing req=0x%x", req);
		dipc_free_msg_req(req, TRUE);
		s = splkkt();
		simple_lock(&dipc_msg_req_list_lock);
	}
	simple_unlock(&dipc_msg_req_list_lock);

	/*
	 * finally, look for inline-only kmsgs to release.
	 */
	simple_lock(&dipc_kmsg_ast_lock);
	kmsg = ipc_kmsg_dequeue(&dipc_kmsg_ast_queue);
	while (kmsg != IKM_NULL) {
		dstat_max(c_dkaq_max_length, c_dkaq_length);
		dstat(--c_dkaq_length);
		dstat(++c_dkaq_handled);
		simple_unlock(&dipc_kmsg_ast_lock);
		splx(s);

		tr2("releasing kmsg=0x%x", kmsg);
		dipc_kmsg_release(kmsg);
		s = splkkt();
		simple_lock(&dipc_kmsg_ast_lock);
		kmsg = ipc_kmsg_dequeue(&dipc_kmsg_ast_queue);
	}
	simple_unlock(&dipc_kmsg_ast_lock);
	splx(s);
}

/*
 *	For send-side only, we have no thread context in
 *	which to prep pages.  Thus, we have no thread context
 *	in which to free the msg_progress or the kmsg when
 *	we are done sending.  Therefore, this thread handles
 *	freeing up msg_progress and kmsg for the send-side.
 */
void
dipc_msg_prep_free_thread(void)
{
	msg_progress_t		mp;
	register int		i;
	ipc_kmsg_t		kmsg;
	spl_t			s;
	TR_DECL("dipc_msg_prep_free_thread");

	for (;;) {
		s = splkkt();
		simple_lock(&dipc_msg_prep_free_lock);
		mp = dipc_msg_prep_free_list;
		if (mp != MSG_PROGRESS_NULL) {
			dipc_msg_prep_free_list = mp->prep_next;
			dstat_max(c_dmp_free_list_max_length,
				  c_dmp_free_list_length);
			dstat(--c_dmp_free_list_length);
			dstat(++c_dmp_free_list_handled);
		} else {
			dipc_msg_prep_free_thread_active = FALSE;
			assert_wait((event_t) &dipc_msg_prep_free_thread, TRUE);
		}
		simple_unlock(&dipc_msg_prep_free_lock);
		splx(s);
		if (mp == MSG_PROGRESS_NULL) {
			thread_block(0);
			continue;
		}
		tr3("mp 0x%x kmsg 0x%x", mp, mp->kmsg);
		dipc_msg_prep_free(mp);
	}
}

/*
 * Free up a msg_progress struct, along with its associated kmsg.
 * May be called from either thread or AST context, but if it's in
 * AST context, the destination port must not use NMS and the kmsg
 * may not be destroyed.
 */
void
dipc_msg_prep_free(
	msg_progress_t	mp)
{
	int		i;
	ipc_kmsg_t	kmsg = mp->kmsg;
	spl_t		s;

	assert(mp->state == MP_S_MSG_DONE);
	assert((mp->flags & MP_F_RECV) == 0);
	assert(mp->error_count == 0);

	KMSG_DROP_HANDLE(kmsg, kmsg->ikm_handle);

	/*
	 *	Free request chains.
	 */
	for (i = 0; i < MAXCHAINS; i++) {
		if (mp->idle_chain[i]) {
			dipc_free_msg_req(mp->idle_chain[i], TRUE);
			mp->chain_count--;
		}
	}
	assert(mp->chain_count == 0);

	s = splkkt();
	simple_lock(&mp->lock);
	if (mp->flags & MP_F_SENDING_THR) {
		/*
		 * sending thread is still around and wants to
		 * look at the kmsg.  It will free the kmsg and mp.
		 */
		mp->flags &= ~MP_F_OOL_IN_PROG;
		simple_unlock(&mp->lock);
		splx(s);
	} else {
		/*
		 *	Dispose of the kmsg.  The msg_progress
		 *	itself will be destroyed as a side-effect
		 *	of releasing the kmsg.
		 *
		 *	XXX Interesting question:  destroying the
		 *	kmsg can cause yield_transits behavior,
		 *	thus blocking this thread on an RPC.
		 *	Is this a problem? XXX
		 */
		simple_unlock(&mp->lock);
		splx(s);
		if (mp->flags & MP_F_KKT_ERROR)
			ipc_kmsg_destroy(kmsg);
		else
			dipc_kmsg_release(kmsg);
	}
}

/*
 * Routine dipc_prep_thread
 *
 * Description
 *	Pull msg_progress fields off of a central queue and
 *	restart the send pipe.  The queue is manipulated by
 *	spin_locks.
 */
void
dipc_prep_thread(void)
{
	msg_progress_t	mp;
	request_block_t	req;
	int		idle_count;
	spl_t		s;
	TR_DECL("dipc_prep_thread");

	for(;;) {
		tr_start();
		tr2("enter: dipc_msg_prep_queue 0x%x", dipc_msg_prep_queue);
		
		s = splkkt();
		simple_lock(&dipc_msg_prep_queue_lock);
		while (dipc_msg_prep_queue) {
			mp = dipc_msg_prep_queue;
			dipc_msg_prep_queue = mp->prep_next;
			dstat_max(c_dmp_queue_max_length, c_dmp_queue_length);
			dstat(--c_dmp_queue_length);
			dstat(++c_dmp_queue_handled);
			simple_unlock(&dipc_msg_prep_queue_lock);
			
			simple_lock(&mp->lock);
			mp->flags &= ~MP_F_PREP_QUEUE;
			assert(mp->state == MP_S_READY ||
			       mp->state == MP_S_OOL_DONE); 
			idle_count = mp->idle_count;
			simple_unlock(&mp->lock);
			splx(s);
			
			if (idle_count)
			    dipc_prep_n_send(mp->kmsg->ikm_handle, mp);
			else {
				if (!(mp->flags & MP_F_PREP_DONE))
				    (void)dipc_prep_pages(mp);
			}
			s = splkkt();
			simple_lock(&dipc_msg_prep_queue_lock);
		}
		
		tr1("exit");
		tr_stop();
		dipc_prep_thread_active = FALSE;
		assert_wait((event_t)&dipc_prep_thread_active, FALSE);
		simple_unlock(&dipc_msg_prep_queue_lock);
		splx(s);
		thread_block((void (*)) 0);
	}
}


/*
 * dipc_send_error_trailer
 *
 * Description
 *	The message has been completely sent, so now we send the
 *	error vector if present, else a dummy indicating no errors.
 *
 * N.B.  This routine used to inherit and free mp->lock, but ther
 *	is no reason for things to be locked here.
 */

void
dipc_send_error_trailer(
	msg_progress_t	mp,
	request_block_t	req)
{
	kkt_return_t	kr;
	kkt_sglist_t	dlist;
	handle_t	handle = mp->kmsg->ikm_handle;
	int		err_count, req_count;
	int 		i;
	struct page_err_page *pe, *ptmp;
	TR_DECL("dipc_send_error_trailer");

	tr_start();
	tr2("enter:  mp 0x%x", mp);

	assert(req);
	req->callback = dipc_xmit_done;
	assert(mp->state == MP_S_ERROR_DONE);
	assert(!(mp->flags & MP_F_RECV));

	if (!mp->page_err_info) {
		tr2("send zero header req 0x%x\n",req);
		req->data.list->ios_address = (vm_offset_t)&dipc_zero_header;
		req->data.list->ios_length =
			req->data_length = sizeof(struct page_err_header);
		if (!dipc_generate_error_trailers) {
			dipc_xmit_done(KKT_SUCCESS, handle, req, FALSE);
			tr1("exit no error trailer");
			tr_stop();
			return;
		}
		kr = KKT_REQUEST(handle, req);
		if (kr != KKT_SUCCESS)
			dipc_ool_kkt_error(kr, req, mp->kmsg, mp, FALSE);
		tr1("exit");
		tr_stop();
		return;
	}

	panic("dipc_send_error_trailer"); /* XXX alanl XXX */

	assert (mp->page_err_info);
	err_count = mp->page_err_info->header.count;
	req_count = err_count / PAGE_ERR_MAX +
					(err_count % PAGE_ERR_MAX ? 1 : 0);
	/*
	 * If we have more error entries that can fit in a single
	 * chain (4 * PAGE_ERR_MAX - a lot!), panic for now.  This
	 * is only because I dont anticipate this ever really
	 * happening and dont feel justified in making it work now.
	 */
	if (req_count > CHAINSIZE)
		panic("dipc_send_error_trailer: too many error pages");
	pe = mp->page_err_info;
	err_count = mp->page_err_info->header.count + 
		req_count * sizeof(struct page_err_header);
	dlist = req->data.list;
	for (i = 0; i < req_count; i++, dlist++) {
		vm_offset_t wsize = err_count < PAGE_SIZE ? err_count : 
			PAGE_SIZE;
		dlist->ios_address = (vm_offset_t)pe;
		dlist->ios_length = wsize;
		req->data_length += wsize;
		err_count -= wsize;
		pe = (struct page_err_page *)pe->header.next;
	}

	kr = KKT_REQUEST(handle, req);
	if (kr != KKT_SUCCESS)
		dipc_ool_kkt_error(kr, req, mp->kmsg, mp, FALSE);

	tr1("exit");
	tr_stop();
}


/*
 * dipc_receive_error_trailer
 *
 * Description
 *	The message has been completely received, so now we get the
 *	error count, and the error vector if present.
 *
 * N.B.  This routine used to inherit and free mp->lock, but ther
 *	is no reason for things to be locked here.
 */

void
dipc_receive_error_trailer(
	msg_progress_t	mp,
	request_block_t	req)
{
	kkt_return_t	kr;
	kkt_sglist_t	dlist;
	handle_t	handle = mp->kmsg->ikm_handle;
	int		err_count, req_count;
	int 		i;
	struct page_err_page *pe, *ptmp;
	TR_DECL("dipc_receive_error_trailer");

	tr_start();
	tr2("enter:  mp 0x%x", mp);

	assert(req);
	req->callback = dipc_xmit_done;
	assert(mp->state == MP_S_ERROR_DONE);
	assert(mp->flags & MP_F_RECV);

	/*
	 * use the now-vacant fields in the pin structure to get the
	 * error count (as this happens from interrupt, we need the
	 * preallocated space and this will work as well as anything).
	 */
	tr2("recv zero header req 0x%x\n",req);
	req->data.list->ios_address = (vm_offset_t)&mp->pin.entry;
	req->data.list->ios_length =
		req->data_length = sizeof(struct page_err_header);
	if (!dipc_generate_error_trailers) {
		*((unsigned int *)req->data.list->ios_address) = 0;
		*((unsigned int *)req->data.list->ios_address + 4) = 0;
		dipc_xmit_done(KKT_SUCCESS, handle, req, FALSE);
		tr1("exit no error trailer");
		tr_stop();
		return;
	}
	kr = KKT_REQUEST(handle, req);
	if (kr != KKT_SUCCESS)
		dipc_ool_kkt_error(kr, req, mp->kmsg, mp, FALSE);

	/*
	 * dipc_xmit_done will find out if there is an error vector and
	 * arrange for it to be received.
	 */

	tr1("exit");
	tr_stop();
}

/*
 * dipc_unpin_request
 *
 * Unpin the pages in a request.
 */
void
dipc_unpin_request(
	request_block_t	req)
{
	register	i;
	kkt_return_t	kr;
	kkt_sglist_t	dlist;
	vm_page_t	*npage;
	vm_offset_t	size;
	int		s;
	TR_DECL("dipc_unpin_request");

	tr_start();
	tr2("enter: req 0x%x", req);

	/*
	 * Release the pages in the list.
	 */
	npage = (vm_page_t *)req->args[1];
	size = req->data_length;
	dlist = req->data.list;
	s = splvm();
	for (i = 0; i < CHAINSIZE && size > 0; i++, npage++, dlist++) {
		size -= dlist->ios_length;
		/*
		 * KERNEL_BUFFER objects set *npage == VM_PAGE_NULL
		 * as they do not get pinned.
		 */
		if (*npage) {
			/* If receiving, mark page dirty. */
			if (req->request_type & REQUEST_RECV)
				pmap_set_modify((*npage)->phys_addr);
			kr = vm_page_unpin(*npage);
			KKT_UNMAP(trunc_page(dlist->ios_address));
			assert(kr == KERN_SUCCESS);
		}
	}
	splx(s);
	tr1("exit");
	tr_stop();
}

/* 
 * dipc_fill_chain
 *
 * Fill a request chain with new data.  This routine will always try to
 * fill the entire chain.
 *
 * INPUT
 *	mp->flags must have MP_F_QUEUEING set (used as a lock).
 *	May be called with mp->lock if can_block == FALSE.
 */
dipc_return_t
dipc_fill_chain(
	request_block_t	req,
	msg_progress_t	mp,
	boolean_t	can_block)
{
	vm_page_t	m;
	register	i;
	dipc_return_t	dr;
	kkt_sglist_t	dlist;
	vm_page_t	*npage;
	vm_offset_t	size;
	spl_t		s;
	TR_DECL("dipc_fill_chain");

	tr_start();
	tr4("enter: req 0x%x mp 0x%x can_block 0x%x",
	    req, mp, can_block);

	assert(mp->flags & MP_F_QUEUEING);
	/*
	 * dipc_fill_chain should not be called unless we know there's
	 * something for it to do.
	 */
	assert(mp->state == MP_S_READY);

	/*
	 * this must be done before returning since dipc_xmit_engine sends
	 * the request if data_length is non-zero.
	 */
	req->data_length = 0;

	/* acquire more pages.  Note that we may run out
	 * of pages at any time so we need to make sure our
	 * callback request has a valid page.
	 */
	s = splkkt();
	simple_lock(&mp->prep_queue.lock);
	(void)splvm();
	dlist = req->data.list;
	npage = (vm_page_t *)req->args[1];
	for (i = 0; i < CHAINSIZE; i++, npage++, dlist++) {
		/*
		 * Check FIRST for the end of pinning.  This case
		 * arises when we are handling KERNEL_BUFFERs.
		 */
		if (mp->state == MP_S_OOL_DONE)
			break;

		/*
		 * Check for the presence of a KERNEL_BUFFER,
		 * indicated by a 0 entry.
		 */
		if (mp->pin.entry == 0) {
			prep_pages_queue_t	prep_queue = &mp->prep_queue;

			assert(mp->pin.copy_list->copy->type == 
						VM_MAP_COPY_KERNEL_BUFFER);
			if (prep_queue_empty(prep_queue)) {
				/*
				 * The prep queue is empty.  Since this
				 * is a KERNEL_BUFFER it doesn't need to be
				 * prepped.  However, the prep fields need
				 * to be advanced to the next copy.  We could
				 * do this here by taking the fill_lock and
				 * checking that pslock is zero, but this
				 * case should be rare and it's not worth
				 * duplicating the code to advance the copy
				 * list, etc.  So just pretend that we tried
				 * to pin and failed, and the prep code will
				 * do the right stuff.
				 */
				dr = DIPC_P_NEED_PREP;
			} else {
				/*
				 * We don't need to get the "page" here
				 * because it's a KERNEL_BUFFER, but we
				 * do need to call dipc_extract_prep_page
				 * to advance the prep_queue extract index.
				 */
				dr = DIPC_SUCCESS;
				m = dipc_extract_prep_page(prep_queue);
				assert(m == (vm_page_t)mp->pin.copy_list->copy);
			}
			m = VM_PAGE_NULL;
		} else {
			dr = dipc_pin_page(mp, &m);
		}

		if (dr != DIPC_SUCCESS) {
			/*
			 * If pinning the page was not successful, return if
			 * we cannot block and brute force it in; this will
			 * wakeup a waiting service thread to deal with it.
			 */
			simple_unlock(&mp->prep_queue.lock);
			splx(s);
			if (can_block) {
				m = dipc_get_pin_page(mp);
				assert(m);
				if (mp->pin.entry == 0) {
					/* this is a kernel buffer */
					m = VM_PAGE_NULL;
				}
				s = splkkt();
				simple_lock(&mp->prep_queue.lock);
				(void)splvm();
			} else {
				tr2("exit: %x\n",dr);
				tr_stop();
				return (dr);
			}
		}
		*npage = m;
		dipc_calculate_req(mp, m, dlist, can_block);
		req->data_length += dlist->ios_length;
	}

	simple_unlock(&mp->prep_queue.lock);
	splx(s);

	if (!(mp->flags & MP_F_PREP_DONE)) {
		int r;

		prep_queue_length((&mp->prep_queue), r);
		if (r <= mp->prep_queue.low_water)
			(void) dipc_queue_idle_chain(mp, NULL_REQUEST, TRUE, s,
									FALSE);
	}

	assert(req->data_length != 0);
	tr1("exit");
	tr_stop();
	return (DIPC_SUCCESS);
}

/*
 * calculate the size of the current request block
 *
 * XXX This should be an inline function
 */
void
dipc_calculate_req(
	msg_progress_t	mp,
	vm_page_t	m,
	kkt_sglist_t	dlist,
	boolean_t	can_block)
{
	register vm_size_t	size;
	unsigned int		offset;
	vm_map_copy_t		copy;
	TR_DECL("dipc_calculate_req");

	tr_start();

	/*
	 *	Case one:  kernel buffer.  Just use existing copy
	 *	address and size, as the whole thing will be
	 *	transferred at once.
	 *
	 *	Case two:  everything else.
	 */
	if (m == VM_PAGE_NULL) {
		copy = mp->pin.copy_list->copy;
		assert(copy->type == VM_MAP_COPY_KERNEL_BUFFER);
		dlist->ios_address = copy->cpy_kdata;
		size = copy->size;
	} else {
		assert(mp->pin.copy_list->copy->type == VM_MAP_COPY_ENTRY_LIST);
		offset = mp->pin.vaddr & PAGE_MASK;
		dlist->ios_address = KKT_FIND_MAPPING(m, offset, can_block);
		size = PAGE_SIZE - offset;
		if (size > mp->pin.size)
			size = mp->pin.size;
	}

	mp->pin.size -= size;
	mp->pin.vaddr += size;
	dlist->ios_length = size;

	/*
	 * update the pin pointers
	 */
	if (mp->pin.size == 0) {
		mp->pin.copy_list = mp->pin.copy_list->next;
		if (mp->pin.copy_list != 0) {
			copy = mp->pin.copy_list->copy;
			if (copy->type == VM_MAP_COPY_ENTRY_LIST) {
				mp->pin.entry = vm_map_copy_first_entry(copy);
				mp->pin.vaddr = copy->offset;
				mp->pin.size = copy->size;
			} else {
				assert(copy->type==VM_MAP_COPY_KERNEL_BUFFER);
				mp->pin.entry = VM_MAP_ENTRY_NULL;
				mp->pin.vaddr = 0;
				mp->pin.size = copy->size;
			}
			mp->pin.entry_count++;
		} else {
			mp->state = MP_S_OOL_DONE;
			tr2("mp 0x%x setting state MP_S_OOL_DONE", mp);
		}
	}

	tr_stop();
}

#include <vm/vm_map.h>
#include <vm/vm_fault.h>

vm_page_t		dipc_prep_one_page(
				vm_object_t	object,
				vm_offset_t 	offset,
				msg_progress_t	mp);

dipc_return_t		dipc_calc_pin_page(
				msg_progress_t	mp,
				vm_object_t	*object,
				vm_offset_t 	*offset);

/*
 * The prep_queue lock is taken at a higher level.
 */
vm_page_t
dipc_extract_prep_page(
	prep_pages_queue_t	q)
{
	vm_page_t m;

	if (prep_queue_empty(q))
		return VM_PAGE_NULL;
	m = q->page[q->ext++];
	if (q->ext >= q->size)
		q->ext = 0;

	return m;
}

/*
 * Routine dipc_prep_pages
 *
 * Description:
 *	Must be called in thread context.  Given an entry list
 *	flavor copy object, prep pages until the _prep_ page
 *	queue is full.  This routine will allocate pages or
 *	fault pages in as necessary.  If the copy object is the KERNEL_BUFFER
 *	flavor, insert the pointer to the copy object in the prep array.
 *	Returns 0 if the prep array is full.
 *	FOOXXX lock is taken on entry.
 */
boolean_t
dipc_prep_pages(
	msg_progress_t	mp)
{
	prep_pages_queue_t	prep;
	vm_map_copy_t	copy;
	vm_map_entry_t	entry;
	vm_offset_t	va;
	vm_offset_t	offset;
	vm_object_t	object;
	kern_return_t	kr;
	int		s;
	boolean_t	set;
	vm_size_t	vm_copy_size;
	boolean_t	restart_flag;
	TR_DECL("dipc_prep_pages");

	tr_start();
	tr2("enter:  mp 0x%x", mp);

	if (mp->prep_queue.pslock) {
		tr_stop();
		return 1;
	}
	/*
	 * set the prep pslock under lock to serialize
	 * access to filling the queue.  Releasing
	 * the pslock is simply resetting it.
	 */
	set = FALSE;
	s = splkkt();
	if (fill_lock_lock(&mp->prep_queue)) {
		if (mp->prep_queue.pslock == 0) {
			mp->prep_queue.pslock = 1;
			set = TRUE;
		}
		fill_lock_unlock(&mp->prep_queue);
	}
	splx(s);
	if (!set) {
		tr_stop();
		return 1;
	}
	assert(!(mp->flags & MP_F_PREP_DONE) && mp->pin.size);
    restart:
	copy = mp->prep.copy_list->copy;
	assert(copy);

	entry = mp->prep.entry;
	offset = mp->prep.offset;
	prep = &mp->prep_queue;
	va = trunc_page(mp->prep.vaddr);

	if (copy->type == VM_MAP_COPY_KERNEL_BUFFER) {
		assert(entry == VM_MAP_ENTRY_NULL);
		if (prep_queue_full(prep)) {
			/* Release serialization lock */
			mp->prep_queue.pslock = 0;
			tr1("exit");
			tr_stop();
			return prep_queue_full(prep);
		}

		prep_insert_page(prep, (vm_page_t)copy);
		goto next_copy;			/* skip entry list stuff */
	}

	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);

	object = entry->object.vm_object;

	/*
	 * Iterate over pages.
	 */
	while (va < copy->offset + copy->size && !prep_queue_full(prep)) {
		vm_page_t m;

		assert(entry != vm_map_copy_to_entry(copy));
		assert(va < entry->vme_end);

		assert(object != VM_OBJECT_NULL);

		m = dipc_prep_one_page(object, offset, mp);

		/*
		 * Locking here is unecessary: we have
		 * guaranteed we are the only thread in here,
		 * and the ins pointer is the only thing updated.
		 */
		prep_insert_page(prep, m);

		va += PAGE_SIZE;
		offset += PAGE_SIZE;

		if (va >= entry->vme_end || (va >= copy->offset + copy->size)) {
			/*
			 * advance to next extry.  If there aren't any more
			 * because we've finished this copy, offset won't
			 * be changed and the while loop will terminate.
			 */
			entry = entry->vme_next;
			if (entry && entry != vm_map_copy_to_entry(copy)) {
				offset = entry->offset;
				object = entry->object.vm_object;
			}
			mp->prep.entry_count++;
		}
	}

	mp->prep.vaddr = va;
	mp->prep.offset = offset;
	mp->prep.entry = entry;
next_copy:
	restart_flag = FALSE;
	/*
	 * check for null entry first in case we got here with a KERNEL_BUFFER
	 * copy_t instead of an ENTRY_LIST.
	 */
	if (!entry || entry == vm_map_copy_to_entry(copy)) {
		/*
		 * finished one copy object, start the next one.
		 */
		mp->prep.copy_list = mp->prep.copy_list->next;
		if (mp->prep.copy_list != 0) {
			copy = mp->prep.copy_list->copy;
			if (copy->type == VM_MAP_COPY_ENTRY_LIST) {
				mp->prep.entry = vm_map_copy_first_entry(copy);
				mp->prep.offset = mp->prep.entry->offset
					+ trunc_page(copy->offset)
					- mp->prep.entry->vme_start;
			} else {
				assert(copy->type == VM_MAP_COPY_KERNEL_BUFFER);
				mp->prep.entry = VM_MAP_ENTRY_NULL;
				mp->prep.offset = 0;
			}
			mp->prep.entry_count++;
			mp->prep.size = copy->size;
			mp->prep.vaddr = copy->offset;
			if (!prep_queue_full(prep))
				restart_flag = TRUE;
		} else
			mp->flags |= MP_F_PREP_DONE;
	}
	if (restart_flag)
		goto restart;

	/* Release serialization lock */
	mp->prep_queue.pslock = 0;
	tr1("exit");
	tr_stop();
	return prep_queue_full(prep);
}

dipc_return_t
dipc_calc_pin_page(
	msg_progress_t	mp,
	vm_object_t	*object,
	vm_offset_t 	*offset)
{
	vm_map_copy_t	copy;
	vm_map_entry_t	entry;
	vm_offset_t	va;

	copy = mp->pin.copy_list->copy;
	assert(copy);
	entry = mp->pin.entry;
	assert(entry);
	assert(entry == vm_map_copy_first_entry(copy));
	va = trunc_page(mp->pin.vaddr);

	while (va >= entry->vme_end) {
		entry = entry->vme_next;
		assert(entry != vm_map_copy_to_entry(copy));
	}

	*object = entry->object.vm_object;
	assert(*object != VM_OBJECT_NULL);
	*offset = entry->offset + (va - entry->vme_start);
	return DIPC_SUCCESS;
}

vm_page_t
dipc_prep_one_page(
	vm_object_t	object,
	vm_offset_t 	offset,
	msg_progress_t	mp)
{
	vm_page_t m;
	vm_page_t top_page;
	vm_map_entry_t entry;
	kern_return_t kr;
	TR_DECL("dipc_prep_one_page");

	tr_start();
	tr4("enter: obj 0x%x off 0x%x mp 0x%x", object, offset, mp);

	/*
	 *	Try to find the page of data.
	 */
start:
	top_page = VM_PAGE_NULL;
	vm_object_lock(object);
	if (((m = vm_page_lookup(object, offset)) !=
	     VM_PAGE_NULL) && !m->busy && !m->fictitious &&
	    !m->absent && !m->error && !m->restart) {
		/*
		 *	Found the page - we win.
		 */
		top_page = VM_PAGE_NULL;
	}
	else {
		vm_prot_t result_prot;

		/*
		 * If the page is NULL then this is a new page so just
		 * grab one and use it, when receiving.
		 * If overwrite optimization is being applied to this
		 * copy, then the page must be faulted in rather than
		 * vm_page_alloc'ed.
		 */
		if ((m == VM_PAGE_NULL) && (mp->flags & MP_F_RECV) &&
		    !(mp->prep.copy_list->flags & DCL_OVERWRITE)) {
			top_page = VM_PAGE_NULL;
			m = vm_page_alloc(object, offset);
			tr2("fast alloc, page 0x%x", m);
			if (m == VM_PAGE_NULL) {
				vm_object_unlock(object);
                                VM_PAGE_WAIT();
				goto start;
			}
			/*
			 *	The page must be placed on the active
			 *	queue so that it will be correctly
			 *	handled later, when the page is
			 *	unprepped/unpinned.
			 */
			assert(m->inactive == FALSE);
			assert(m->gobbled == FALSE);
			assert(m->wire_count == 0);
			vm_page_lock_queues();
			vm_page_activate(m);
			vm_page_unlock_queues();
			dstat(++c_dipc_fast_alloc_pages);
		} else {
			boolean_t	no_zero_fill = FALSE;

			tr1("slow fault");

retry:
			result_prot = VM_PROT_READ;
		
			/*
			 * We call vm_fault_page w/ no_zero_fill == TRUE
			 * when receiving.
			 * This affects pages which have not previously
			 * existed and we are going to overwrite them
			 * anyways when we pull the data off the mesh.
			 */
			if ((mp->flags & MP_F_RECV) &&
			    !(mp->prep.copy_list->flags & DCL_OVERWRITE)) {
				no_zero_fill = TRUE;
				result_prot |= VM_PROT_WRITE;
			}

			/*
			 * XXX Add in entry->offset?
			 */
			vm_object_paging_begin(object);
			kr = vm_fault_page(object,
					   offset,
					   result_prot,
					   FALSE,	/* must_be_resident */
					   FALSE,	/* interruptible */
					   offset,	/* lo_offset */
					   offset + PAGE_SIZE,
					   		/* hi_offset XXX ? */
					   VM_BEHAVIOR_SEQUENTIAL,
					   &result_prot,
					   &m,		
					   &top_page,
					   0,		/* error code */
					   no_zero_fill,
					   FALSE);	/* data_supply */
			if (kr == VM_FAULT_MEMORY_SHORTAGE) {
				VM_PAGE_WAIT();
				vm_object_lock(object);
				goto retry;
			}
			if (kr == VM_FAULT_FICTITIOUS_SHORTAGE) {
				vm_page_more_fictitious();
				vm_object_lock(object);
				goto retry;
			}
			if (kr == VM_FAULT_MEMORY_ERROR) {
				printf("MEMORY_ERROR encountered\n");
				dipc_install_error(mp, object, offset);
				m = zero_page;
			} else {
				if (kr != VM_FAULT_SUCCESS) {
					vm_object_lock(object);
					goto retry;
				}
				vm_object_paging_end(m->object);
			}
		}
	}
	assert(m);
	if (m->busy)
		m->busy = FALSE;
	kr = vm_page_prep(m);
	assert(kr == KERN_SUCCESS);
	if (top_page != VM_PAGE_NULL) {
		vm_object_unlock(m->object);
		vm_object_lock(object);
		VM_PAGE_FREE(top_page);
		vm_object_paging_end(object);
	}
	vm_object_unlock(object);
	tr2("exit, page 0x%x", m);
	tr_stop();
	return m;
}

/*
 * Routine dipc_install_error
 *
 * Description
 *	While trying to prep a page, we have encountered a  memory error;
 *	Use (or create) a global zero_page to transmit, and update the
 *	error information that will be transmitted as a trailer to the
 *	message.
 *
 * INPUT
 *	Nothing locked.
 */
void
dipc_install_error(
	msg_progress_t	mp,
	vm_object_t	object,
	vm_offset_t 	offset)
{
	struct page_err_page *pe, *ptmp;
	int		cnt;
	vm_page_t	m;
	page_error	*err_entry;
	TR_DECL("dipc_install_error");

	tr_start();
	tr4("enter:  mp 0x%x object %x0x offset 0x%x", 
	    mp, object, offset);

	panic("dipc_install_error");	/* XXX alanl XXX */

	simple_lock(&zero_page_lock);
	/*
	 * A memory error means we will transmit a page
	 * of zeros in its place.  Allocate a page that
	 * can be used whenever we need it.
	 */
	if (zero_page == VM_PAGE_NULL) {
		/*
		 *	Maybe this should be a kmem_alloc?  XXX
		 */
		while ((m = vm_page_grab()) == VM_PAGE_NULL) {
			VM_PAGE_WAIT();
		}
		assert(m->active == FALSE);
		assert(m->inactive == FALSE);
		vm_page_wire(m);
		simple_lock(&zero_page_lock);
		if (zero_page == VM_PAGE_NULL) {
			zero_page = m;
			simple_unlock(&zero_page_lock);
		} else {
			simple_unlock(&zero_page_lock);
			vm_page_release(m);
		}
	}
	/*
	 * If we don't have an err page, get one.
	 */
	pe = mp->page_err_info;
	if (!pe) {
		pe = (struct page_err_page *)
			kalloc(sizeof(struct page_err_page));
		bzero((char *)pe, sizeof(struct page_err_page));
		mp->page_err_info = pe;
	}
	/*
	 * Update the header and find the appropriate err
	 * page to make an entry into.
	 */
	cnt = pe->header.count++;
	while (cnt > PAGE_ERR_MAX) {
		pe = (struct page_err_page *)pe->header.next;
		cnt -= PAGE_ERR_MAX;
	}
	/*
	 * Check to see if we need to add a new page
	 */
	if (cnt == PAGE_ERR_MAX) {
		assert(pe->header.next == (page_err_header_t *)0);
		pe = (struct page_err_page *)
			kalloc(sizeof(struct page_err_page));
		bzero((char *)ptmp, sizeof(struct page_err_page));
		pe->header.next = (page_err_header_t *)ptmp;
		pe = ptmp;
		cnt = 0;
	}
	err_entry = &pe->error_vec[cnt];
	err_entry->entry = mp->prep.entry_count;
	err_entry->offset = offset;
	err_entry->error = m->page_error;
	tr1("exit");
	tr_stop();
}

/*
 * dipc_get_pin_page
 *
 * Description
 *	Blocking routine that will return a pinned page or VM_PAGE_NULL
 *	if there are no more pages.
 *
 * INPUT
 *	Nothing locked
 */
vm_page_t
dipc_get_pin_page(
	msg_progress_t	mp)
{
	dipc_return_t	dr;
	kern_return_t	kr;
	vm_page_t	m;
	spl_t		s;
	TR_DECL("dipc_get_pin_page");

	tr_start();
	tr2("enter:  mp 0x%x", mp);

	do {
		s = splkkt();
		simple_lock(&mp->prep_queue.lock);
		(void)splvm();
		if (mp->pin.entry == VM_MAP_ENTRY_NULL) {
			prep_pages_queue_t	prep_queue = &mp->prep_queue;

			/* A kernel buffer is the next thing to be pinned. */
			assert(mp->pin.copy_list->copy->type ==
					VM_MAP_COPY_KERNEL_BUFFER);
			if (prep_queue_empty(prep_queue)) {
				/*
				 * First time through the loop: the prep queue
				 * is empty.  Arrange for dipc_prep_pages
				 * to be called below.
				 */
				dr = DIPC_P_NEED_PREP;
			} else {
				/*
				 * The kernel buffer has been "prepped" so
				 * extract it from the prep queue and we're
				 * done.
				 */
				m = dipc_extract_prep_page(prep_queue);
				assert(m == (vm_page_t)mp->pin.copy_list->copy);
				dr = DIPC_SUCCESS;
			}
		} else {
			dr = dipc_pin_page(mp, &m);
		}

		simple_unlock(&mp->prep_queue.lock);
		splx(s);

		if (dr != DIPC_SUCCESS) {
			switch (dr) {
			    case DIPC_P_NEED_PREP:
				(void)dipc_prep_pages(mp);
				break;
			    case DIPC_P_NEED_FAULT:
				while (kr != KERN_SUCCESS) {
					vm_object_t	obj;
					vm_offset_t	off;

					/*
					 * Our page got swapped out;
					 * unprep and bring it back in.
					 */
					
					kr = vm_page_unprep(m);
					assert(kr == KERN_SUCCESS);

					dr = dipc_calc_pin_page(mp, &obj,
						&off);
					assert(dr == DIPC_SUCCESS);

					m = dipc_prep_one_page(obj, off, mp);

					s = splvm();
					kr = vm_page_pin(m);
					splx(s);
				}
				/*
				 * remove the page from the prep queue
				 */
				(void) dipc_extract_prep_page(&mp->prep_queue);
				dr = DIPC_SUCCESS;
				break;
			}
		}
	} while (dr != DIPC_SUCCESS);

	tr1("exit");
	tr_stop();
	return m;
}

/*
 *	dipc_pin_page
 *
 * Description
 *	Remove a page from the prep queue and pin it, which results in
 *	one of the following return codes:
 *	    DIPC_P_NEED_PREP:	No page available.  Caller must invoke
 *				dipc_prep_pages and try again.
 *	    DIPC_P_NEED_FAULT:	The prep queue remains unaltered, as the
 *				first element must be faulted back in.
 *	    DIPC_SUCCESS:	*page is valid, life is good.
 *
 * INPUT
 *	called at splvm().
 *	mp->prep_queue.lock is taken
 */
dipc_return_t
dipc_pin_page(
	msg_progress_t	mp,
	vm_page_t	*page)
{
	kern_return_t	kr;
	vm_page_t	m;
	dipc_return_t	dr = DIPC_SUCCESS;
	TR_DECL("dipc_pin_page");

	tr_start();
	tr2("enter:  mp 0x%x", mp);

	/*
	 * dipc_pin_page must only be called when we know there is a
	 * page to be pinned.
	 */
	assert(mp->state == MP_S_READY);

	m = prep_next_page(((prep_pages_queue_t)&mp->prep_queue));
	if (m == VM_PAGE_NULL) {
		assert(!(mp->flags & MP_F_PREP_DONE));
		tr1("exit: DIPC_P_NEED_PREP");
		tr_stop();
		return (DIPC_P_NEED_PREP);
	}

	assert(m != VM_PAGE_NULL);

	kr = vm_page_pin(m);

	*page = m;

	if (kr != KERN_SUCCESS) {
		tr2("exit: DIPC_NEED_FAULT page 0x%x", m);
		tr_stop();
		return (DIPC_P_NEED_FAULT);
	}

	/*
	 * things are stable; remove the page from the queue
	 */
	m = dipc_extract_prep_page(&mp->prep_queue);
	assert(m == *page);
	tr2("exit: page 0x%x", m);
	tr_stop();
	return (dr);
}

/*
 *	dipc_pin_page_vector
 *
 * Description
 *	Remove N pages from the prep queue and pin it, which results in
 *	one of the following return codes:
 *	    DIPC_P_NEED_PREP:	No page available.  Caller must invoke
 *				dipc_prep_pages and try again.
 *	    DIPC_P_DONE:	There are no more pages in the prep queue.
 *				pin.done is now set.
 *	    DIPC_P_NEED_FAULT:	The prep queue remains unaltered, as the
 *				first element must be faulted back in.
 *	    DIPC_SUCCESS:	*page is valid, life is good.
 *
 * INPUT
 *	mp->prep_queue.lock is taken.
 */
#if 0
dipc_return_t
dipc_pin_page_vector(
	msg_progress_t	mp,
	vm_page_t	*page_vec,
	int		*page_cnt)
{
	kern_return_t	kr;
	vm_page_t	m, *p;
	int		ret, i, pcnt, scnt;
	TR_DECL("dipc_pin_page_vector");

	tr_start();
	tr3("enter:  mp 0x%x page_vec 0x%x", mp, page_vec);

	pcnt = 0;
	p = page_vec;
	for (i = 0; i < CHAINSIZE; i++, p++) {
		prep_next_page_v(((prep_pages_queue_t)&mp->prep_queue), i, m);
		*p++ = m;
		if (m == VM_PAGE_NULL) {
			break;
		}
		pcnt++;
	}

	if (mp->flags & MP_F_PREP_DONE)
		mp->flags |= MP_F_PIN_DONE;

	if (pcnt == 0) {
		if (mp->flags & MP_F_PIN_DONE) {
			tr_stop();
			return (DIPC_P_DONE);
		} else {
			tr_stop();
			return (DIPC_P_NEED_PREP);
		}
	}

	kr = vm_page_pin_vector(page_vec, pcnt, &scnt);
	*page_cnt = scnt;

	/*
	 * things are stable; remove the pages from the queue
	 */
	for (i = scnt; i; i--)
		(void)dipc_extract_prep_page(&mp->prep_queue);

	if (kr != KERN_SUCCESS) {
		tr1("exit: DIPC_NEED_FAULT");
		tr_stop();
		return (DIPC_P_NEED_FAULT);
	}

	tr2("exit: page 0x%x", m);
	tr_stop();
	return (DIPC_SUCCESS);
}
#endif /* 0 */

void
dipc_recv_error_notify(
	kkt_return_t	kktr,
	handle_t	handle,
	request_block_t	req,
	boolean_t	thread_context)
{
	unsigned int	*wait;

	wait = *(unsigned int **)req->args[1];
	*wait = FALSE;

	if (kktr != KKT_SUCCESS) {
		msg_progress_t	mp;

		mp = (msg_progress_t)req->args[0];
		assert(mp != MSG_PROGRESS_NULL);
		mp->flags |= MP_F_KKT_ERROR;
	}
	thread_wakeup_one((event_t)wait);
}

/*
 * dipc_process_receive_errors
 *
 * Description
 *	The sender has alerted us that there are some number of
 *	error pages.  Pull them from the remote node and process
 *	them on this one; pass through the copy object again to
 *	find the error pages
 *
 * INPUT
 *	Nothing locked.  Called in thread context.  Our handle &
 *	request resources are all intact and must be release here.
 *
 * N.B.
 *	In this implementation, this is the only spot where we lose
 *	symmetry of send and receive requests.  If there are more than
 *	PAGE_ERR_MAX page errors, then we request the whole thing in
 *	a single kalloc; the send size will send them one page at at time.
 */
void
dipc_process_receive_errors(
	msg_progress_t	mp)
{
	vm_page_t	m;
	vm_page_t	top_page = VM_PAGE_NULL;
	vm_offset_t 	offset;
	vm_map_copy_t	copy;
	vm_map_entry_t entry;
	vm_object_t	object;
	dipc_copy_list_t	copy_list;
	int		i, err_count;
	int		entry_count, req_count;
	kkt_return_t	kr;
	page_error	*pe;
	unsigned int	wait;
	request_block_t	req;
	kkt_sglist_t	dlist;

	panic("dipc_process_receive_errors"); /* XXX alanl XXX */
	err_count = mp->error_count;
	req_count = err_count / PAGE_ERR_MAX * sizeof(struct page_err_header)
		+ err_count * sizeof(struct page_error);

	for (i = 0; i < MAXCHAINS; i++) {
		req = mp->idle_chain[i];
		if (req != NULL_REQUEST)
			break;
	}
	assert(req != NULL_REQUEST);

	/*
	 * Bounds check our size - this is currently a send size
	 * limitation that may have to be removed in the future
	 * (but it is a *pile* of page errors!
	 */
	assert(err_count < PAGE_ERR_MAX * CHAINSIZE);
	mp->page_err_info = 
		(struct page_err_page *)kalloc(req_count);
	assert(mp->page_err_info);
	/*
	 * KKT_REQUEST the information from the remote node
	 */
	mp->page_err_info->header.count = err_count;
	
	dlist = req->data.list;
	dlist->ios_address = (vm_offset_t)mp->page_err_info->error_vec;
	dlist->ios_length = req_count;
	req->callback = dipc_recv_error_notify;
	*(unsigned int *)req->args[1] = (unsigned int)&wait;
	wait = TRUE;
	mp->flags |= MP_F_QUEUEING;
	assert_wait((event_t)&wait, FALSE);

	kr = KKT_REQUEST(mp->kmsg->ikm_handle, req);
	if (kr != KKT_SUCCESS) {
		clear_wait(current_thread(), 0, FALSE);
		mp->flags |= MP_F_KKT_ERROR;
		return;
	}

	/*
	 * The notify routine will reset the MP_F_QUEUEING bit and
	 * wake us up; we need to be synchronous at this point.
	 */
	while (wait)
		thread_block(0);

	if (mp->flags & MP_F_KKT_ERROR)
		return;

	pe = mp->page_err_info->error_vec;
	copy_list = mp->copy_list;
	entry_count = 0;

    err_top:
	entry = vm_map_copy_first_entry(copy_list->copy);
	while (entry != vm_map_copy_to_entry(copy)) {
		vm_prot_t result_prot;
		vm_offset_t offset;

		/*
		 * Loop over all of the error pages in an entry,
		 * faulting them in to mark them in error.  The fault
		 * path only does limited error checking as we recently
		 * obtained these pages and know they are good.
		 */
		while (entry_count == pe->entry && err_count) {
			offset = pe->offset;
			/*
			 * Found the error entry.  Get the page and mark
			 * it in error.
			 */
			object = entry->object.vm_object;
			assert(object);

			vm_object_lock(object);
			vm_object_paging_begin(object);

			result_prot = VM_PROT_READ;
			/*
			 * XXX Add in entry->offset?
			 */
			kr = vm_fault_page(object,
					   offset,
					   VM_PROT_READ,
					   FALSE,	/* must_be_resident */
					   FALSE,	/* interruptible */
					   entry->offset,	/* lo_offset */
					   entry->offset + 
					   (entry->vme_end -
					    entry->vme_start),	/* hi_offset */
					   entry->behavior,
					   &result_prot,
					   &m,
					   &top_page,
					   0,		/* error code */
					   TRUE,	/* no_zero_fill */
					   FALSE);	/* data_supply */

			if (kr == VM_FAULT_MEMORY_SHORTAGE) {
				VM_PAGE_WAIT();
				vm_object_lock(object);
				goto err_top;
			} else
				if (kr != VM_FAULT_SUCCESS) {
					vm_object_lock(object);
					goto err_top;
				}
			m->error = TRUE;
			m->page_error = pe->error;
			m->busy = FALSE;
			vm_object_paging_end(m->object);
			if (top_page != VM_PAGE_NULL) {
				vm_object_unlock(m->object);
				vm_object_lock(object);
				VM_PAGE_FREE(top_page);
				vm_object_paging_end(object);
			}
			vm_object_unlock(object);
			pe++;
			err_count--;
		}
		if (err_count == 0)
			break;
	}
	entry = entry->vme_next;
	if (entry && entry != vm_map_copy_to_entry(copy))
		goto err_top;
	else
		if ((copy_list = copy_list->next))
			goto err_top;
}

#if	MACH_KDB
#include <ddb/db_output.h>

#define	BOOLPRINT(x)	(x ? "" : "!")

void		db_dipc_msg_progress(msg_progress_t	mp);
char *		db_mp_state_string(unsigned short	state);

char *
db_mp_state_string(
	unsigned short	state)
{
	switch (state) {
		case MP_S_START:
			return "START";
		case MP_S_READY:
			return "READY";
		case MP_S_OOL_DONE:
			return "OOL_DONE";
		case MP_S_ERROR_DONE:
			return "ERROR_DONE";
		case MP_S_MSG_DONE:
			return "MSG_DONE";
		default:
			return "UNKNOWN";
	}
}


void
db_dipc_msg_progress(
	msg_progress_t	mp)
{
	register i;
	extern int indent;

	db_printf("message_progress: 0x%x, prep_next=0x%x kmsg=0x%x\n",
		  mp, mp->prep_next, mp->kmsg);
	indent += 3;
	db_printf("state=%s, flags={%sinit, %sthr_avail, %sprep_queue,",
		  db_mp_state_string(mp->state),
		  BOOLPRINT(mp->flags & MP_F_INIT),
		  BOOLPRINT(mp->flags & MP_F_THR_AVAIL),
		  BOOLPRINT(mp->flags & MP_F_PREP_QUEUE));
	db_printf(" %sprep_done, %squeuing,",
		  BOOLPRINT(mp->flags & MP_F_PREP_DONE),
		  BOOLPRINT(mp->flags & MP_F_QUEUEING));
	db_printf("\t%sidle, %srecv, %skkt_err, %sending_thr, %sool_in_prog}\n",
		  BOOLPRINT(mp->flags & MP_F_IDLE),
		  BOOLPRINT(mp->flags & MP_F_RECV),
		  BOOLPRINT(mp->flags & MP_F_KKT_ERROR),
		  BOOLPRINT(mp->flags & MP_F_SENDING_THR),
		  BOOLPRINT(mp->flags & MP_F_OOL_IN_PROG));
	db_printf("chain_count=%d, idle_count=%d, idle_chains=",
		  mp->chain_count, mp->idle_count);
	for (i = 0; i < MAXCHAINS; i++)
		db_printf("0x%x ", mp->idle_chain[i]);
	db_printf("\n");
	db_printf("msg_size=0x%x, page_err_info=0x%x, error_count=%d\n",
		 mp->msg_size, mp->page_err_info, mp->error_count);
	db_printf("copy_list=0x%x\n", mp->copy_list);
	db_printf("prep: copy_list=0x%x, entry=0x%x, offset=0x%x,",
		  mp->prep.copy_list, mp->prep.entry, mp->prep.offset);
	db_printf(" size=0x%x, vaddr=0x%x\n",
		  mp->prep.size, mp->prep.vaddr);
	db_printf("pin: copy_list=0x%x, entry=0x%x, offset=0x%x,",
		  mp->pin.copy_list, mp->pin.entry, mp->pin.offset);
	db_printf(" size=0x%x, vaddr=0x%x\n",
		  mp->pin.size, mp->pin.vaddr);
	db_printf("prep_queue: ins=0x%x, ext=0x%x, size=0x%x\n",
		  mp->prep_queue.ins, mp->prep_queue.ext, mp->prep_queue.size);
	db_printf("     0x%x, 0x%x, 0x%x\n", mp->prep_queue.page[0],
		  mp->prep_queue.page[1], mp->prep_queue.page[2]);
	indent -= 3;
}

#endif	/*MACH_KDB*/


#if SJSCHECK
/*
 * This routine checks the data sent from the OOL test, verifying the
 * signature in each page.
 */
sjs_check_data(
	request_block_t	req)
{
	vm_page_t	m;
	register	i;
	kkt_return_t	kr;
	dipc_return_t	dr;
	kkt_sglist_t	dlist;
	unsigned int	*sr;
	vm_page_t	*npage;
	int		size;
	static boolean_t	seen = FALSE, apply_delta = FALSE;
	static int	should_be = 0;
	static msg_progress_t	mp = (msg_progress_t)0;
	

	size = req->data_length;
	dlist = req->data.list;
	sr = (unsigned int *)dlist->ios_address;
	if (*sr == 0xdeadd0d0) {
		mp = (msg_progress_t)req->args[0];
		seen = TRUE;
		should_be = 1;
		if (dlist->ios_length == PAGE_SIZE) {
			apply_delta = TRUE;
			should_be++;
		}
		size -= dlist->ios_length;
		dlist++;
	}
	if (!seen || mp != (msg_progress_t)req->args[0])
		return;
	for (i = 0; i < CHAINSIZE && size > 0; i++, npage++, dlist++) {
		if (apply_delta)
			sr = (unsigned int *)(dlist->ios_address + 
					      PAGE_SIZE - 0x100);
		else
			sr = (unsigned int *)(dlist->ios_address);
		if (*sr != should_be) {
			show_kkt_request(req);
			printf("sr=0x%x %sseen %sapply_delta should_be=0x%x\n",
			       sr, BOOLPRINT(seen), BOOLPRINT(apply_delta),
			       should_be);
			printf("size=0x%x dlist=0x%x\n", size, dlist);
			assert(0);
		}
		should_be++;
		size -= dlist->ios_length;
	}
}
#endif /* SJSCHECK */
