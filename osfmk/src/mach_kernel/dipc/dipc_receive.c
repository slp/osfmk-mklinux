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
#include <kernel_test.h>

#include <mach_kdb.h>
#include <kern/assert.h>
#include <ddb/tr.h>

#include <mach/boolean.h>
#include <mach/kkt_request.h>
#include <mach/message.h>
#include <kern/lock.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/ipc_sched.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_pset.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_rpc.h>
#include <dipc/port_table.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <kern/lock.h>
#include <machine/kkt_map.h>

decl_simple_lock_data(,dipc_deliver_lock_data)

#define	dipc_deliver_lock()	simple_lock(&dipc_deliver_lock_data)
#define	dipc_deliver_unlock()	simple_unlock(&dipc_deliver_lock_data)

decl_simple_lock_data(,kkt_recv_sync_lock_data)

#define	kkt_recv_sync_lock()	simple_lock(&kkt_recv_sync_lock_data)
#define	kkt_recv_sync_unlock()	simple_unlock(&kkt_recv_sync_lock_data)

struct ipc_kmsg_queue	dipc_delivery_queue;
dstat_decl(int	c_dipc_delivery_queue_length = 0;)
dstat_decl(int	c_dipc_delivery_queue_enqueued = 0;)
dstat_decl(int	c_dipc_delivery_queue_max_length = 0;)
dstat_decl(int	c_dipc_delivery_queue_handled = 0;)
dstat_decl(int	c_dipc_delivery_fast = 0;)

#define DIPC_NUM_DELIVER_THREADS	3
int dipc_num_deliver_threads = DIPC_NUM_DELIVER_THREADS;

/*
 *	Does the transport make upcalls in thread context
 *	or in interrupt context?  We default to assuming
 *	that the transport upcalls in interrupt context.
 */
boolean_t	dipc_kkt_transport_threads = FALSE;


/*
 *	DIPC incoming message (dim) delivery statistics.
 */
dstat_decl(unsigned int c_dim_total = 0;)
dstat_decl(unsigned int c_dim_resource_shortage = 0;)
dstat_decl(unsigned int c_dim_no_uid = 0;)
dstat_decl(unsigned int c_dim_fast_no_inline = 0;)
dstat_decl(unsigned int c_dim_fast_inline = 0;)
dstat_decl(unsigned int c_dim_fast_data_dropped = 0;)
dstat_decl(unsigned int c_dim_slow_delivery = 0;)
dstat_decl(unsigned int c_dim_alloc_failed = 0;)
dstat_decl(unsigned int c_dim_msg_too_large = 0;)
dstat_decl(unsigned int c_dem_block = 0;)
dstat_decl(unsigned int c_dem_migrated = 0;)
dstat_decl(unsigned int c_dem_dead = 0;)
dstat_decl(unsigned int c_dem_queue_full = 0;)
dstat_decl(unsigned int c_dem_delivered = 0;)

void		dipc_message_deliver_thread(void);
void		dipc_message_deliver(
			ipc_kmsg_t	kmsg);

void		dipc_msg_destroy(
			meta_kmsg_t	mkmsg);

dipc_return_t	dipc_mqueue_deliver(
			ipc_port_t	port,
			ipc_kmsg_t	kmsg,
			boolean_t	*free_handle,
			boolean_t	*kern_port,
			node_name	node,
			boolean_t	thread_context);

void		dipc_recv_inline(
			kkt_return_t	kr,
			handle_t	handle,
			request_block_t	req,
			boolean_t	thread_context);

/*
 * The static node map is a fully allocated kkt node map, used to keep track
 * of nodes who wish to be notified when previously unavailable resources
 * are recovered.  dipc_request_callback_count is the number of nodes in
 * the dipc_static_node_map.
 */
node_map_t	dipc_static_node_map;
int		dipc_request_callback_count = 0;

/*
 * dipc_meta_kmsg_resume_count is the desired number of free meta_kmsgs 
 * to have before unblocking a node.
 */
#define DIPC_META_KMSG_RESUME_COUNT	3
int		dipc_meta_kmsg_resume_count = DIPC_META_KMSG_RESUME_COUNT;

decl_simple_lock_data(,req_callback_lock_data)

#define dipc_request_callback_lock()	simple_lock(&req_callback_lock_data)
#define dipc_request_callback_unlock()	simple_unlock(&req_callback_lock_data)


#if	KERNEL_TEST
#define HANDLE_DIPC_TEST	((handle_t)(-1))

extern void dtest_connect_reply(
				handle_t	handle,
				kern_return_t	ret1,
				kern_return_t	ret2);

#define	KKT_CONNECT_REPLY_MACRO(handle, ret1, ret2)			\
		if ((handle) == HANDLE_DIPC_TEST)			\
			dtest_connect_reply((handle), (ret1), (ret2));	\
		else							\
			KKT_CONNECT_REPLY((handle), (ret1), (ret2));

#define	KKT_HANDLE_FREE_MACRO(handle)					\
	((handle == HANDLE_DIPC_TEST) ? KKT_SUCCESS : 			\
					KKT_HANDLE_FREE(handle))

#define	KKT_HANDLE_INFO_MACRO(kktr, handle, infop)			\
	if ((handle) == HANDLE_DIPC_TEST) {				\
		(infop)->node = NODE_NAME_LOCAL;			\
		kktr = KKT_SUCCESS;					\
	} else								\
		kktr = KKT_HANDLE_INFO((handle), (infop));

#else	/* KERNEL_TEST */

#define	KKT_CONNECT_REPLY_MACRO(handle, ret1, ret2)		\
		KKT_CONNECT_REPLY((handle), (ret1), (ret2))

#define	KKT_HANDLE_FREE_MACRO(handle)	KKT_HANDLE_FREE(handle)

#define	KKT_HANDLE_INFO_MACRO(kktr, handle, infop)			\
		kktr = KKT_HANDLE_INFO((handle), (infop));

#endif	/* KERNEL_TEST */

/* used to initialize trailer */
extern mach_msg_format_0_trailer_t trailer_template;

/*
 *	Turn on the fast message delivery path.
 *	For some transports, this happens at interrupt level.
 *	For others, we use the transport's thread context
 *	to deliver the message.
 */
int dipc_interrupt_delivery = TRUE;

/*
 *	Enable reception of an entire (small) kmsg at
 *	dipc_deliver time.  When FALSE, forces use of
 *	meta-kmsgs at all times.
 */
int dipc_interrupt_kmsgs = TRUE;

/*
 * Initialize the locks, queues, and threads used by the receive side
 * of DIPC.  Called from dipc_bootstrap().
 */
void
dipc_receive_init(void)
{
	int i;
	kkt_return_t	kktr;

	simple_lock_init(&kkt_recv_sync_lock_data, ETAP_DIPC_RECV_SYNC);
	simple_lock_init(&dipc_deliver_lock_data, ETAP_DIPC_DELIVER);

	ipc_kmsg_queue_init(&dipc_delivery_queue);

	for (i = 0; i < dipc_num_deliver_threads; i++)
	    (void) kernel_thread(kernel_task, dipc_message_deliver_thread,
				 (char *) 0);

	/* allocate the static node map used for resource shortage callbacks */
	kktr = KKT_MAP_ALLOC(&dipc_static_node_map, TRUE);
	assert(kktr == KKT_SUCCESS);
	simple_lock_init(&req_callback_lock_data, ETAP_DIPC_REQ_CALLBACK);
}


/*
 * deliver routine provided to and used by KKT to deliver data to dipc on.
 * the CHAN_USER_KMSG channel.
 * May be called at interrupt level.
 */
void
dipc_deliver(
	channel_t	channel,
	handle_t	handle,
	endpoint_t	*endpoint,
	vm_size_t	size,
	boolean_t	inlinep,
	boolean_t	thread_context)
{
	ipc_port_t		port;
	register ipc_kmsg_t	kmsg = IKM_NULL;
	uid_t			*uidp;
	boolean_t		delivered = FALSE;
	boolean_t		free_handle;
	boolean_t		kern_port;
	boolean_t		use_inline_data = FALSE;
	vm_size_t		delivery_size;
	kkt_return_t		kktr;
	dipc_return_t		cr_code, cr_code2;
	spl_t			s;
	TR_DECL("*dipc_deliver");

	tr5("enter: handle 0x%x inlinep=0x%x size 0x%x thd_ctxt %d",
	    (unsigned int) handle, inlinep, size, thread_context);

	assert(channel == CHAN_USER_KMSG);
				/* I+M+B+T */
	dstat(++c_dim_total);
	delivery_size = size + MACH_MSG_TRAILER_FORMAT_0_SIZE;
	if (dipc_interrupt_kmsgs && inlinep &&
	    (ikm_plus_overhead(delivery_size)<=dipc_fast_kmsg_size)) {
		kmsg = dipc_kmsg_alloc(delivery_size, thread_context);
		tr2("inline: kmsg 0x%x", kmsg);
		if (kmsg != IKM_NULL) {
			mach_msg_format_0_trailer_t	*trailer;

			kktr = KKT_COPYOUT_RECV_BUFFER(handle,
					       (char *) &kmsg->ikm_header);
			assert(kktr == KKT_SUCCESS);

			/*
			 * initialize trailer and grab dipc_sender_kmsg
			 * out of msgh_remote_port
			 */
			trailer = KMSG_DIPC_TRAILER(kmsg);
			*trailer = trailer_template;
			trailer->dipc_sender_kmsg =
				kmsg->ikm_header.msgh_remote_port;
			use_inline_data = TRUE;
		} else {
			dstat(++c_dim_alloc_failed);
		}
	} else {
		dstat(inlinep ? ++c_dim_msg_too_large : 0);
	}


	/*
	 *	If we couldn't grab a fast kmsg, try for a meta-kmsg.
	 *	For the most part, treat it as an ordinary kmsg.
	 */
	if (kmsg == IKM_NULL) {
		kmsg = (ipc_kmsg_t) dipc_meta_kmsg_alloc(FALSE);
		assert(IKM_NULL == (ipc_kmsg_t)MKM_NULL);
		if (kmsg == IKM_NULL) {
			/*
			 * unfortunately, this will cause resource shortage
			 * processing even if the dipc_uid_lookup below would
			 * fail, but that should be an unlikely situation.
			 */
			dstat(++c_dim_resource_shortage);
			KKT_CONNECT_REPLY_MACRO(handle, DIPC_RESOURCE_SHORTAGE,
								DIPC_SUCCESS);
			KKT_HANDLE_FREE_MACRO(handle);
			tr1("exit: resource shortage");
			return;
		}

		kmsg->ikm_size = size;	/* M+B */

		/*
		 * Indicate to the thread context message delivery
		 * routine whether there is inline data available
		 * for this message.
		 */
		if (inlinep)
			kmsg->ikm_header.msgh_size =
				(mach_msg_size_t) delivery_size;
		else
			kmsg->ikm_header.msgh_size = 0;
	}
	assert(kmsg != IKM_NULL);

	/*
	 *      XXX This cast "knows" that the layout of the endpoint
	 *      matches that of a DIPC UID.  Alternatively, we could do
	 *	a DIPC_UID_MAKE, which is a little slower:  we think
	 *	this path is sufficiently critical to justify
	 *	introducing this hack.
	 */
	uidp = (uid_t *) endpoint;

#if	KERNEL_TEST
	if (uidp->origin == NODE_NAME_LOCAL)
		uidp->origin = dipc_node_self();
#endif	/* KERNEL_TEST */
	if (dipc_uid_lookup_fast(uidp, &port) != DIPC_SUCCESS) {
		if (KMSG_IS_META(kmsg))
			dipc_meta_kmsg_free((meta_kmsg_t) kmsg);
		else
			dipc_kmsg_free(kmsg);
		dstat(++c_dim_no_uid);
		KKT_CONNECT_REPLY_MACRO(handle, DIPC_NO_UID, DIPC_SUCCESS);
		KKT_HANDLE_FREE_MACRO(handle);
		tr1("exit: uid lookup failure");
		return;
	}

	/*
	 *	At this point, we've managed to grab a kmsg (or
	 *	meta-kmsg) and locate the requested port.  We've
	 *	also got a handle to use for retrieving any more
	 *	data associated with the message, if any.  Save
	 *	the handle in the kmsg and mark the kmsg
	 *	appropriately.
	 *
	 *	We turn on the REF_CONVERT bit because we acquired
	 *	a DIPC-level reference on the port courtesy of
	 *	successful operation of dipc_uid_lookup.
	 */
	kmsg->ikm_handle = handle;
	kmsg->ikm_header.msgh_remote_port = (mach_port_t) port;
	kmsg->ikm_header.msgh_bits |=
		(MACH_MSGH_BITS_RECEIVING|MACH_MSGH_BITS_HAS_HANDLE|
		 MACH_MSGH_BITS_REF_CONVERT);

	/*
	 * Here we can take a stab at delivering the msg directly
	 * to the port -- either the message queue or a waiting thread.
	 * We can do this if we are successful in acquiring the relevant
	 * locks *without* blocking.  We fall back on the slow delivery
	 * mechanism if we can't get the locks or if the port is in a
	 * funny state.
	 */

	if (dipc_interrupt_delivery && (thread_context == FALSE)) {
		struct handle_info	info;
		dipc_return_t		ret;

		if (thread_context == TRUE)
			ip_lock(port);
		else if (!ip_lock_try(port))
			goto slow_delivery;

		KKT_HANDLE_INFO_MACRO(kktr, handle, &info);
		assert(kktr == KKT_SUCCESS);

		/*
		 *	This check permits the delivery of migrating
		 *	messages, which are not subject to queue limits:
		 *	see long comment in dipc_enueue_message.
		 */
		if (port->ip_ms_block && info.node != port->ip_source_node) {
			ip_unlock(port);
		} else {
			/*
			 * dipc_mqueue_deliver will reset the interrupt
			 * and delivered bits, and will release lock.
			 *
			 * N.B.  We don't need to initialize free_handle
			 * and kern_port before calling dipc_mqueue_deliver.
			 * If the call fails, we don't care about these
			 * values; if the call succeeds, they will be set
			 * for us.
			 */
			ret = dipc_mqueue_deliver(port,
						  kmsg,
						  &free_handle,
						  &kern_port,
						  info.node,
						  thread_context);
			delivered = (ret == DIPC_SUCCESS);
			/*
			 * If we were unabled to deliver the message
			 * for any reason, pull back to thread
			 * delivery.
			 */
		}
	}

    slow_delivery:
	if (delivered) {
		if (!inlinep) {
			cr_code = DIPC_SUCCESS;
			dstat(++c_dim_fast_no_inline);
		} else if (use_inline_data) {
			cr_code = DIPC_DATA_SENT;
			dstat(++c_dim_fast_inline);
		} else {
			cr_code = DIPC_DATA_DROPPED;
			dstat(++c_dim_fast_data_dropped);
		}

		if (kern_port)
			cr_code2 = DIPC_KERNEL_PORT;
		else
			cr_code2 = DIPC_SUCCESS;
		tr4("delivered: cr_code %x cr_code2 %x kmsg 0x%x",
		    cr_code, cr_code2, kmsg);
		KKT_CONNECT_REPLY_MACRO(handle, cr_code, cr_code2);
		if (free_handle)
			KKT_HANDLE_FREE_MACRO(handle);
		dstat(++c_dipc_delivery_fast);
	} else {
		/*
		 *	If we've already got a thread context, it makes
		 *	no sense to enqueue this message to a thread;
		 *	try to do the delivery here.
		 */
		if (thread_context == TRUE) {
			dipc_message_deliver(kmsg);
			goto out;
		}

		s = splkkt();
		dipc_deliver_lock();
		/* 
		 * enqueue kmsg on dipc message delivery queue
		 */
		assert(KMSG_HAS_HANDLE(kmsg));
		tr2("passing kmsg 0x%x to delivery thread", kmsg);
		dstat(++c_dim_slow_delivery);
		dstat(++c_dipc_delivery_queue_length);
		dstat(++c_dipc_delivery_queue_enqueued);
		dstat_max(c_dipc_delivery_queue_max_length,
			  c_dipc_delivery_queue_length);
		ipc_kmsg_enqueue_macro(&dipc_delivery_queue, kmsg);
		dipc_deliver_unlock();
		splx(s);
		thread_wakeup_one((event_t)&dipc_delivery_queue);
	}
    out:
	tr1("exit");
}

/*
 * drop a reference on the dest port (if the kmsg has a pointer to it)
 * and destroy the meta_kmsg or kmsg.
 */
void
dipc_msg_destroy(
	meta_kmsg_t	mkmsg)		/* but it could also be a ipc_kmsg_t */
{
	TR_DECL("dipc_msg_destroy");

	tr_start();
	tr2("mkmsg=0x%x", mkmsg);
	if (mkmsg->mkm_remote_port != IP_NULL)
		ipc_port_release(mkmsg->mkm_remote_port);
	if (KMSG_IS_META(mkmsg))
		dipc_meta_kmsg_free(mkmsg);
	else
		dipc_kmsg_free((ipc_kmsg_t)mkmsg);
	tr_stop();
}


/*
 * Wrapper routine to process a kmsg and enqueue it on the
 * port mqueue if possible.  This routine recognizes if
 * it is called from interrupt or thread context.
 *
 * On success, returns:
 *	free_handle	caller should free handle
 *	kern_port	dest port is owned by kernel
 * On failure, only returns failure status.
 *
 * Input:
 *	port is locked
 * Output:
 *	port is unlocked.
 */
dipc_return_t
dipc_mqueue_deliver(
	ipc_port_t	port,
	ipc_kmsg_t	kmsg,
	boolean_t	*free_handle,		/* OUT */
	boolean_t	*kern_port,		/* OUT */
	node_name	node,
	boolean_t	thread_context)
{
	dipc_return_t		ret;
	boolean_t		need_map_wakeup = FALSE;
	node_map_t		map;
	kkt_return_t		kktr;
	handle_t		ugly_handle;
	mach_msg_return_t	mr;
	TR_DECL("dipc_mqueue_deliver");

	tr_start();
	tr4("enter: port 0x%x kmsg 0x%x node 0x%x", port, kmsg, node);
	if (DIPC_IS_PROXY(port)) {
		/* port migrated away */
		assert(port->ip_forward == TRUE);
		dstat(thread_context == TRUE ? ++c_dem_migrated : 0);
		ret = DIPC_PORT_MIGRATED;
	} else if (!ip_active(port)) {
		/* dead port */
		dstat(thread_context == TRUE ? ++c_dem_dead : 0);
		ret = DIPC_PORT_DEAD;
	} else if (port->ip_msgcount >= port->ip_qlimit && !port->ip_ms_block) {
		/*
		 *	We get here if the queue is full AND the port
		 *	in question ISN'T migrating.  If the port in
		 *	question IS migrating, then we must IGNORE
		 *	the queue limit because the message is part
		 *	of the port migration sequence.
		 */

		dstat(thread_context == TRUE ? ++c_dem_queue_full : 0);
		dstat(port->ip_queue_full++);
		dstat_max(port->ip_queue_max, port->ip_msgcount);
		ret = DIPC_QUEUE_FULL;

		/*
		 *	If we are called from interrupt context,
		 *	bail out now - deal with it from thread
		 *	context.
		 */
		if (thread_context == FALSE) {
			ip_unlock(port);
			tr_stop();
			return ret;
		}

		if (port->ip_receiver == ipc_space_kernel)
			*kern_port = TRUE;
		map = port->ip_callback_map;
		if (map == NODE_MAP_NULL) {
			while (port->ip_map_alloc == TRUE) {
				/* someone else is allocating.  just wait. */
				port->ip_map_wait = TRUE;
				assert_wait((event_t)(((int)
						      &port->ip_callback_map)
						      +2),
					    FALSE);
				ip_unlock(port);
				thread_block(0);
				ip_lock(port);
			}
			if (port->ip_callback_map == NODE_MAP_NULL) {
				port->ip_map_alloc = TRUE;
				ip_unlock(port);
				kktr = KKT_MAP_ALLOC(&map, FALSE);
				ip_lock(port);
				port->ip_map_alloc = FALSE;
				if (kktr == KKT_RESOURCE_SHORTAGE) {
					map = NODE_MAP_NULL;
					ret = DIPC_RESOURCE_SHORTAGE;
				} else {
					assert(kktr == KKT_SUCCESS);
					port->ip_callback_map = map;
				}
			} else {
				map = port->ip_callback_map;
			}
		}
		if (map != NODE_MAP_NULL) {
			kktr = KKT_ADD_NODE(map, node);
			if (kktr == KKT_NODE_PRESENT) {
				ret = DIPC_RETRY_SEND;
			} else if (kktr == KKT_RESOURCE_SHORTAGE) {
				ret = DIPC_RESOURCE_SHORTAGE;
			} else if (kktr != KKT_SUCCESS) {
				printf("port 0x%x map 0x%x ", port, map);
				printf("node 0x%x kktr 0x%x\n", node, kktr);
				panic("dipc_mqueue_deliver");
			}
		}
	} else {
		/* everything's OK; queue up the message */

		/*
		 * If this is a kmsg, then we already have all of the inline
		 * data.  If the COMPLEX_OOL bit is not set, then nothing more
		 * is coming and we can save the sending node here and set
		 * *free_handle TRUE so that dipc_message_deliver will free
		 * it after the CONNECT_REPLY.
		 */
		if (KMSG_IS_META(kmsg)) {
			*free_handle = FALSE;
		} else if (!KMSG_COMPLEX_OOL(kmsg)) {
			/*
			 *	The caller does the actual handle free;
			 *	but we must mark the kmsg as if it had
			 *	no handle at this point so that the
			 *	message is in a consistent state when
			 *	picked up by a (racing) receiver.
			 *
			 *	Note that we must reset the handle field
			 *	to contain the sending node number; this
			 *	conforms with the convention that an
			 *	incoming message always has a handle or
			 *	a node number.
			 */
			*free_handle = TRUE;
			assert(KMSG_HAS_HANDLE(kmsg));
			KMSG_CLEAR_HANDLE(kmsg);
			ugly_handle = kmsg->ikm_handle;
			kmsg->ikm_handle = node;
		} else
			*free_handle = FALSE;

		if (port->ip_receiver == ipc_space_kernel)
			*kern_port = TRUE;
		else
			*kern_port = FALSE;

		/* ipc_mqueue_deliver will unlock the port */
		dstat(thread_context == TRUE ? ++c_dem_delivered : 0);
		dstat(port->ip_enqueued++);
		mr = ipc_mqueue_deliver(port, kmsg, thread_context);
		if (mr != MACH_MSG_SUCCESS) {
			/*
			 *	Ouch.  Fix up everything in sight.
			 */
			*free_handle = FALSE;
			if (!KMSG_IS_META(kmsg) && !KMSG_COMPLEX_OOL(kmsg)) {
				kmsg->ikm_handle = ugly_handle;
				KMSG_MARK_HANDLE(kmsg);
			}
		}
		tr4("exit: delivered %d free_handle=0x%x kern_port=0x%x",
		    mr, *free_handle, *kern_port);
		tr_stop();
		return mr == MACH_MSG_SUCCESS ? DIPC_SUCCESS : DIPC_RETRY_SEND;
	}

	if (ret != DIPC_SUCCESS) {
		if (port->ip_map_wait == TRUE)
			need_map_wakeup = TRUE;
		ip_unlock(port);
		if (need_map_wakeup == TRUE)
			thread_wakeup((event_t)(((int)&port->ip_callback_map)
						+ 2));
	}

	tr_stop();
	return ret;
}


/*
 * This is pretty much a pared-down version of ipc_mqueue_send that doesn't
 * block waiting for room in the queue.
 * If free_handle is returned set to TRUE, caller must free the handle.
 * Must be called in thread context.
 */
dipc_return_t
dipc_enqueue_message(
	meta_kmsg_t	mkmsg,
	boolean_t	*free_handle,		/* OUT */
	boolean_t	*kern_port)		/* OUT */
{
	ipc_port_t		port;
	dipc_return_t		ret;
	struct handle_info	info;
	kkt_return_t		kktr;
	ipc_kmsg_t		kmsg;
	TR_DECL("dipc_enqueue_message");

	tr_start();
	tr4("enter: mkmsg 0x%x mkmsg->size=0x%x mkmsg->handle=0x%x", 
	   mkmsg, mkmsg->mkm_size, mkmsg->mkm_handle);

	port = mkmsg->mkm_remote_port;
	assert(IP_VALID(port));
	*kern_port = FALSE;
	assert(KMSG_RECEIVING((ipc_kmsg_t) mkmsg));
	assert(KMSG_HAS_HANDLE((ipc_kmsg_t) mkmsg));

	KKT_HANDLE_INFO_MACRO(kktr, mkmsg->mkm_handle, &info);
	assert(kktr == KKT_SUCCESS);

	ip_lock(port);

	/*
	 *	XXX Temporary hack! XXX
	 *
	 *	The message delivery thread blocks when it
	 *	sees a migrating port.  This situation arises
	 *	in one of two ways.
	 *
	 *	1.  Transmission to a receive right that's
	 *	moving off-node.
	 *	2.  Transmission to a proxy that's the recipient
	 *	of an incoming receive right.
	 *
	 *	In both cases, we expect that the kernel will
	 *	clear up the condition and wake us up when the
	 *	migration completes.
	 *
	 *	However, there is a possible deadlock here.
	 *	consider:
	 *		- receive right on A hands out send rights to N..Z
	 *		- receive right on A migrates to B
	 *		- receive right on B starts migrating back to A
	 *		- N..Z send messages to A
	 *		- A's message delivery threads block (port is migrating)
	 *		- A does BEGIN_RECV_MIGRATE back to B
	 *		- B pushes enqueued messages from old principal to A
	 *		- But A has no available message delivery threads!
	 *
	 *	The probable long-term fix is to force the sender to block,
	 *	similarly to queue full processing.  However, this code works
	 *	in the short term.  (Furthermore, turning on the interrupt-level
	 *	delivery optimization breaks the deadlock because the message
	 *	delivery threads are no longer needed.)
	 *
	 *	N.B.  Messages coming from the node sending us the receive
	 *	right shouldn't be blocked -- they are messages being sent
	 *	as part of the migration process.
	 */
	while (port->ip_ms_block && info.node != port->ip_source_node) {
		assert_wait((event_t)port, FALSE);
		ip_unlock(port);
		tr1("blocking");
		dstat(++c_dem_block);
		thread_block(0);
		ip_lock(port);
	}

	ret = dipc_mqueue_deliver(port,
				  (ipc_kmsg_t) mkmsg,
				  free_handle,
				  kern_port,
				  info.node,
				  TRUE);
	if (ret == DIPC_SUCCESS) {
		tr_stop();
		return ret;
	}

	/*
	 * if we reached here, there was an error and the message can't be
	 * enqueued.  Return the reason (which will be sent back via
	 * CONNECT_REPLY), destroy the message, and indicate that the handle
	 * should be freed (can't do it now because it's needed for the
	 * CONNECT_REPLY).
	 *
	 * The caller does the actual handle free; we are lazy and don't
	 * bother to mark the kmsg here to indicate that it (in some sense)
	 * no longer has a handle.
	 */
	dipc_msg_destroy(mkmsg);
	*free_handle = TRUE;
	assert(ret != DIPC_SUCCESS);
	tr2("error exit 0x%x", ret);
	tr_stop();
	return ret;
}


/*
 *	Thread context for user-level message delivery.
 *	This thread is necessary when a message can't be
 *	delivered directly from interrupt level into a
 *	port's message queue or into a waiting thread.
 */
void
dipc_message_deliver_thread(void)
{
	node_name	node;
	kkt_return_t	kktr;
	ipc_kmsg_t	kmsg;
	dipc_return_t	dr;
	spl_t		s;
	TR_DECL("dipc_message_deliver");

	for(;;) {
		tr_start();
		s = splkkt();
		dipc_deliver_lock();
	      deliver_more:
		kmsg = ipc_kmsg_dequeue(&dipc_delivery_queue);
		
		while (kmsg != IKM_NULL) {
			dstat(--c_dipc_delivery_queue_length);
			dstat(++c_dipc_delivery_queue_handled);
			dipc_deliver_unlock();
			splx(s);
			tr2("kmsg 0x%x", kmsg);
			assert(KMSG_HAS_HANDLE(kmsg));
			
			dipc_message_deliver(kmsg);
			
			s = splkkt();
			dipc_deliver_lock();
			kmsg = ipc_kmsg_dequeue(&dipc_delivery_queue);
		}
		
		/*
		 * do node unblocking here if needed and
		 * resources are available
		 */ 
		if ((dipc_request_callback_count > 0) &&
		    (dipc_meta_kmsg_free_count() >
		     dipc_meta_kmsg_resume_count)) {
			dipc_deliver_unlock();
			splx(s);
			dipc_request_callback_lock();
			kktr = KKT_REMOVE_NODE(dipc_static_node_map, &node);
			assert(kktr == KKT_SUCCESS || kktr == KKT_RELEASE);
			dipc_request_callback_count--;
			assert(dipc_request_callback_count >= 0);
			dipc_request_callback_unlock();
			
			kktr = drpc_unblock_node(node, dipc_node_self(), &dr);
			assert(kktr == KKT_SUCCESS);
			assert(dr == DIPC_SUCCESS);
			
			s = splkkt();
			dipc_deliver_lock();
			if (!ipc_kmsg_queue_empty(&dipc_delivery_queue)) {
				goto deliver_more;
			}
		}
		
		assert_wait((event_t)&dipc_delivery_queue, FALSE);
		dipc_deliver_unlock();
		splx(s);
		tr1("exit");
		tr_stop();
		thread_block((void (*)(void)) 0);
	}
}


/*
 *	In thread context, try to deliver a message to a
 *	target port.  Handle all error cases.
 *
 *	We expect an incoming kmsg or meta-kmsg that has
 *	acquired a DIPC reference.
 */
void
dipc_message_deliver(
	ipc_kmsg_t	kmsg)
{
	handle_t	handle;
	vm_size_t	delivery_size;
	ipc_kmsg_t	kmsg2;
	meta_kmsg_t	mkmsg;
	boolean_t	is_meta;
	boolean_t	kern_port;
	dipc_return_t	dr;
	kern_return_t	cr_code, cr_code2;
	kkt_return_t	kktr;
	boolean_t	data_dropped;
	boolean_t	free_handle;

	data_dropped = FALSE;
	free_handle = FALSE;

	assert(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_REF_CONVERT);
	kmsg->ikm_header.msgh_bits &= ~ MACH_MSGH_BITS_REF_CONVERT;
	(void) dipc_uid_port_reference((ipc_port_t)
				       kmsg->ikm_header.msgh_remote_port);

	if (KMSG_IS_META(kmsg)) {
		delivery_size = (vm_size_t) kmsg->ikm_header.msgh_size;
		if (delivery_size > 0 &&
		    ikm_plus_overhead(delivery_size) <= dipc_fast_kmsg_size) {
								/* I+M+B+T */
			/*
			 * we have inline data but didn't save it yet.
			 * try once again to get a kmsg.
			 */
			kmsg2 = dipc_kmsg_alloc(delivery_size, TRUE);
			if (kmsg2 != IKM_NULL) {
				mach_msg_format_0_trailer_t *trailer;

				mkmsg = (meta_kmsg_t) kmsg;
				kmsg2->ikm_handle = kmsg->ikm_handle;
				kmsg = kmsg2;

				kktr = KKT_COPYOUT_RECV_BUFFER(kmsg->ikm_handle,
						  (char *) &kmsg->ikm_header);
				assert(kktr == KKT_SUCCESS);

				/*
				 * initialize trailer and 
				 * grab dipc_sender_kmsg
				 * out of msgh_remote_port
				 */
				trailer = KMSG_DIPC_TRAILER(kmsg);
				*trailer = trailer_template;
				trailer->dipc_sender_kmsg =
					kmsg->ikm_header.msgh_remote_port;

				kmsg->ikm_header.msgh_remote_port =
					(mach_port_t)mkmsg->mkm_remote_port;
				kmsg->ikm_header.msgh_bits |=
					(MACH_MSGH_BITS_RECEIVING |
					 MACH_MSGH_BITS_HAS_HANDLE);
				dipc_meta_kmsg_free(mkmsg);
			} else {
				/* couldn't get a kmsg: drop data */
				data_dropped = TRUE;
			}
		} else if (delivery_size > 0) {
			/*
			 * there is inline data but it doesn't fit
			 * into the preallocated kmsg size.
			 */
			data_dropped = TRUE;
		}
	}

	is_meta = KMSG_IS_META(kmsg);

	/*
	 * keep a pointer to the handle in case the msg is destroyed
	 * before we return here.
	 */
	assert(KMSG_HAS_HANDLE((ipc_kmsg_t) kmsg));
	handle = kmsg->ikm_handle;

	dr = dipc_enqueue_message((meta_kmsg_t) kmsg, &free_handle, &kern_port);

	if (dr != DIPC_SUCCESS) {
		/* msg was deallocated by dipc_enqueue_message */
		cr_code = dr;
	} else if (!is_meta)
		cr_code = DIPC_DATA_SENT;	/* redundant... */
	else if (data_dropped)
		cr_code = DIPC_DATA_DROPPED;
	else
		cr_code = DIPC_SUCCESS;

	if (kern_port)
		cr_code2 = DIPC_KERNEL_PORT;
	else
		cr_code2 = DIPC_SUCCESS;

	KKT_CONNECT_REPLY_MACRO(handle, cr_code, cr_code2);
	if (free_handle) {
		KKT_HANDLE_FREE_MACRO(handle);
	}
}


#if	DEBUG_TIMING
unsigned int inline_start[2], inline_end[2];
#endif	/* DEBUG_TIMING */

void
dipc_recv_inline(
	kkt_return_t	kr,
	handle_t	handle,
	request_block_t	req,
	boolean_t	thread_context)
{
	ipc_kmsg_t	kmsg;

	kmsg = (ipc_kmsg_t)req->args[0];
	kmsg->ikm_next = (ipc_kmsg_t) kr;
	req->args[1] = KKT_DATA_RECEIVED;
#if	DEBUG_TIMING
	if (inline_start[0])
		fc_get(inline_end);
#endif	/* DEBUG_TIMING */
	thread_wakeup((event_t)kmsg);
}

/*
 * Given a meta_kmsg containing a transport handle, allocate space and
 * get the kmsg contents from the transport.  The meta_kmsg will be
 * freed whether or not the pull succeeds.
 */
dipc_return_t
dipc_pull_kmsg(
	meta_kmsg_t	mkmsg,
	ipc_kmsg_t	*kmsgp)
{
	ipc_kmsg_t	kmsg;
	request_block_t	req;
	kkt_return_t	kktr;
	spl_t		s;
	mach_msg_format_0_trailer_t	*trailer;
	TR_DECL("dipc_pull_kmsg");

	tr3("enter: mkmsg 0x%x size 0x%x",
	    (unsigned int)mkmsg, mkmsg->mkm_size);

	assert(KMSG_IS_META(mkmsg));
								/* M+B+T */
	kmsg = ikm_alloc(mkmsg->mkm_size + MACH_MSG_TRAILER_FORMAT_0_SIZE);
	assert(kmsg != IKM_NULL);
	ikm_init(kmsg, mkmsg->mkm_size + MACH_MSG_TRAILER_FORMAT_0_SIZE);
	kmsg->ikm_next = IKM_NULL;
	kmsg->ikm_prev = IKM_NULL;
	KMSG_MARK_RECEIVING(kmsg);
	/*
	 *	We only mark the kmsg as having a handle if we
	 *	leave this routine holding one in the kmsg (as
	 *	opposed to the meta-kmsg).
	 */
	assert(!KMSG_HAS_HANDLE(kmsg));

	assert(mkmsg->mkm_handle != HANDLE_NULL);
#if	KERNEL_TEST
	if (mkmsg->mkm_handle == HANDLE_DIPC_TEST) {
		kmsg->ikm_header.msgh_size = 0;
		kmsg->ikm_handle = HANDLE_DIPC_TEST;
		KMSG_MARK_HANDLE(kmsg);
		goto finish;
	}
#endif	/* KERNEL_TEST */

	req = dipc_get_kkt_req(TRUE);	/* will block waiting for a request */
	req->request_type = REQUEST_RECV;
	req->data.address = (vm_offset_t)(&kmsg->ikm_header);
	req->data_length = mkmsg->mkm_size;		/* M+B */
	req->callback = dipc_recv_inline;
	req->args[0] = (unsigned int)kmsg;
	req->args[1] = KKT_SUCCESS;
	req->next = NULL_REQUEST;
#if	DEBUG_TIMING
	fc_get(inline_start);
#endif	/* DEBUG_TIMING */
	kktr = KKT_REQUEST(mkmsg->mkm_handle, req);

	switch (kktr) {
	  case KKT_DATA_RECEIVED:
		/* the data was ready and waiting.  No callback. */
		break;
	  case KKT_SUCCESS:
		/*
		 * request was queued.  Wait for the callback.
		 * use ikm_next to communicate the return value from the
		 * receive, and args[1] to synchronize.
		 */
		s = splkkt();
		kkt_recv_sync_lock();
		while (req->args[1] != KKT_DATA_RECEIVED) {
			assert_wait((event_t)kmsg, FALSE);
			kkt_recv_sync_unlock();
			splx(s);
			thread_block((void (*)(void)) 0);
			s = splkkt();
			kkt_recv_sync_lock();
		}
		kkt_recv_sync_unlock();
		splx(s);
		if (kmsg->ikm_next == (struct ipc_kmsg *)KKT_SUCCESS)
			break;
		/* otherwise it's a transport error; fall through... */
	  default:
		/* transport error */
		dipc_free_kkt_req(req);
		if (kktr != KKT_INVALID_HANDLE)
			KKT_HANDLE_FREE_MACRO(mkmsg->mkm_handle);
		dipc_meta_kmsg_free(mkmsg);
		kmsg->ikm_header.msgh_remote_port =
			(mach_port_t)mkmsg->mkm_remote_port;
		*kmsgp = kmsg;
		return DIPC_TRANSPORT_ERROR;
	}

	dipc_free_kkt_req(req);

	/*
	 *	We have overwritten the bits in the message header
	 *	with those supplied by the sender.  The following
	 *	bits must be treated carefully:
	 *		HAS_HANDLE	probably set on send-side,
	 *				must be forced to a known
	 *				value on the receive-side
	 *		RECEIVING	must be forced back to TRUE
	 */
	assert(KMSG_IS_DIPC_FORMAT(kmsg));
	assert(!KMSG_IS_META(kmsg));
	assert(!KMSG_PLACEHOLDER(kmsg));
	assert(!(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_REF_CONVERT));
	KMSG_MARK_RECEIVING(kmsg);

	/* if this is the whole message, the handle can be freed here */
	if (!(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL)) {
		struct handle_info	info;

		/*
		 *	For two reasons, we may need to know the
		 *	name of the sending node later.  First,
		 *	the message may contain a migrating receive
		 *	right -- in that case, we need to talk
		 *	back to the sending node.  Second, the
		 *	sending node may have a sender blocked,
		 *	waiting for the message to be received
		 *	and handled -- in that case, we need to
		 *	send the wakeup back to the sending node.
		 *
		 *	Always leave dipc_pull_kmsg with ikm_handle
		 *	set to a valid handle or to the name of the
		 *	node that sent the message.
		 */
		KKT_HANDLE_INFO_MACRO(kktr, mkmsg->mkm_handle, &info);
		assert(kktr == KKT_SUCCESS);
		kmsg->ikm_handle = (handle_t) info.node;
		KKT_HANDLE_FREE_MACRO(mkmsg->mkm_handle);
		KMSG_CLEAR_HANDLE(kmsg);
	} else {
		kmsg->ikm_handle = mkmsg->mkm_handle;
		KMSG_MARK_HANDLE(kmsg);
	}

	/*
	 * initialize trailer and grab dipc_sender_kmsg out of
	 * msgh_remote_port
	 */
	trailer = KMSG_DIPC_TRAILER(kmsg);
	*trailer = trailer_template;
	trailer->dipc_sender_kmsg = kmsg->ikm_header.msgh_remote_port;

#if	KERNEL_TEST
finish:
#endif	/* KERNEL_TEST */
	kmsg->ikm_header.msgh_remote_port = (mach_port_t)mkmsg->mkm_remote_port;
	dipc_meta_kmsg_free(mkmsg);
	*kmsgp = kmsg;
	return DIPC_SUCCESS;
}

/*
 *	Routine dipc_receive_ool
 *
 *	Pull the data described by a msg_progress descriptor from
 *	a remote node.  We prepare the pages to receive data into
 *	and then pull data in CHAINSIZE pages at a time.  The
 *	thread will sleep, waking up only to replenish pages and
 *	when the data transfer is complete.
 *
 *	Takes a msg_progress with a kmsg, returns a msg_progress
 *	with a kmsg and all ool regions.  Caller allocates and
 *	deallocates msg_progress (and kmsg, as the case may be).
 */
dipc_return_t
dipc_receive_ool(
		 msg_progress_t		mp)
{
	int		req_count, i;
	ipc_kmsg_t	kmsg;
	request_block_t	req, sreq, chain1;
	kkt_return_t	kktr;
	dipc_return_t	dr;
	vm_page_t	m;
	vm_map_copy_t	copy;
	struct handle_info info;
	TR_DECL("dipc_receive_ool");

	tr2("enter: mp 0x%x", (unsigned int)mp);

	req_count = mp->msg_size / PAGE_SIZE;

	kmsg = mp->kmsg;

	mp->prep.copy_list = mp->copy_list;
	copy = mp->prep.copy_list->copy;
	mp->prep.vaddr = copy->offset;
	mp->prep.size = copy->size;
	if (copy->type == VM_MAP_COPY_ENTRY_LIST) {
		mp->prep.entry = vm_map_copy_first_entry(copy);
		mp->prep.offset = mp->prep.entry->offset +
			trunc_page(copy->offset) - mp->prep.entry->vme_start;
	} else {
		mp->prep.entry = VM_MAP_ENTRY_NULL;
		mp->prep.offset = 0;
	}
	mp->pin = mp->prep;

	mp->flags |= MP_F_RECV;
	mp->state = MP_S_READY;

	/*
	 *	Set up the request chain(s)
	 */
	if (req_count >= CHAINSIZE * 2) {
		chain1 = dipc_get_msg_req(mp,
					  REQUEST_RECV);
		chain1->callback = dipc_xmit_engine;
		mp->chain_count = 2;
		req_count = CHAINSIZE;
	} else {
		if (req_count >= CHAINSIZE)
			req_count = CHAINSIZE;
		mp->chain_count = 1;
		chain1 = NULL_REQUEST;
	}

	req = dipc_get_msg_req(mp,
			       REQUEST_RECV);
	req->callback = dipc_xmit_engine;

	/*
	 *	Load the prep_queue with prepped pages
	 */
	(void)dipc_prep_pages(mp);

	mp->flags |= MP_F_QUEUEING | MP_F_THR_AVAIL;
	dr = dipc_fill_chain(req, mp, TRUE);
	assert(dr == DIPC_SUCCESS);
	KKT_DO_MAPPINGS();
	assert(mp->idle_count == 0);
	mp->flags &= ~MP_F_QUEUEING;

	assert(KMSG_HAS_HANDLE(kmsg));
	kktr = KKT_REQUEST(kmsg->ikm_handle, req);
	assert(kktr == KKT_SUCCESS);

	dr = dipc_ool_xfer(kmsg->ikm_handle, mp, chain1, TRUE);
	assert(dr == DIPC_SUCCESS || dr == DIPC_TRANSPORT_ERROR);

	assert(mp->state == MP_S_MSG_DONE);
	assert(mp->chain_count == 0);

	return (dr);
}

dipc_return_t
dproc_request_callback(
	node_name	sending_node)
{
	kkt_return_t kktr;

	dipc_request_callback_lock();
	kktr = KKT_ADD_NODE(dipc_static_node_map, sending_node);
	assert(kktr == KKT_SUCCESS || kktr == KKT_NODE_PRESENT);
	if (kktr == KKT_SUCCESS) {
		dipc_request_callback_count++;
	}
	dipc_request_callback_unlock();
	return DIPC_SUCCESS;
}

/*
 * get a node from the ip_callback_map.  If it is remote, unblock the
 * port on that node.  If it's local, return DIPC_WAKEUP_LOCAL to let
 * the caller unblock a local sender.
 * Port must be locked by the caller.
 * Called from ipc_mqueue_receive when a kmsg has been removed from the
 * queue, and also from dipc_wakeup_blocked_senders (below) to restart
 * sends after a receive right migration.
 * If the rpc returns DIPC_NO_MSGS, then the proxy no longer has anything
 * to send and another node should be awakened.
 */
dipc_return_t
dipc_callback_sender(
	ipc_port_t	port)
{
	kkt_return_t	kktr;
	node_name	node;
	dipc_return_t	dr;

	while (port->ip_callback_map != NODE_MAP_NULL) {
		kktr = KKT_REMOVE_NODE(port->ip_callback_map, &node);
		assert(kktr != KKT_MAP_EMPTY);
		assert(kktr != KKT_INVALID_ARGUMENT);
		if (kktr == KKT_SUCCESS || kktr == KKT_RELEASE) {
			if (kktr == KKT_RELEASE) {
				kktr = KKT_MAP_FREE(port->ip_callback_map);
				assert(kktr == KKT_SUCCESS);
				port->ip_callback_map = NODE_MAP_NULL;
			}
			if (node == dipc_node_self()) {
				/* wake up local blocked sender */
				return DIPC_WAKEUP_LOCAL;
			} else {
				/* wake up a remote blocked sender */
				boolean_t port_dead = !ip_active(port);

				ip_unlock(port);
				assert(dipc_node_is_valid(node));
				kktr = drpc_unblock_port(node, &port->ip_uid,
							port_dead, &dr);
				ip_lock(port);
				assert(kktr == KKT_SUCCESS);
				if (dr == DIPC_SUCCESS);
					return DIPC_SUCCESS;

				/*
				 * that node doesn't have anything to send.
				 * try another.
				 */
				assert(dr == DIPC_NO_MSGS);
			}
		}
	}
	return DIPC_SUCCESS;
}

/*
 * Wake up blocked senders on all remote nodes for this port.  Called to
 * restart sends after a receive right migration.
 */
dipc_return_t
dipc_wakeup_blocked_senders(
	ipc_port_t	port)
{
	dipc_return_t	dr;

	ip_lock(port);
	while (port->ip_callback_map != NODE_MAP_NULL) {
		dr = dipc_callback_sender(port);
		assert(dr == DIPC_SUCCESS);
	}
	ip_unlock(port);
	return DIPC_SUCCESS;
}
