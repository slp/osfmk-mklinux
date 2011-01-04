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
 *	File:	dipc/dipc_rpc.c
 *	Author:	Michelle Dominijanni
 *	Date:	1994
 *
 *	The DIPC RPC service.
 */

#include <mach_kdb.h>

#include <kern/assert.h>
#include <kern/misc_protos.h>
#include <mach/boolean.h>
#include <mach/kkt_request.h>
#include <dipc/dipc_types.h>
#include <dipc/dipc_rpc.h>
#include <kern/lock.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <dipc/dipc_funcs.h>


/*
 *	DIPC RPCs may be serviced in interrupt context
 *	or in thread context.  An RPC serviced in
 *	interrupt context is obviously faster than one
 *	serviced in thread context.
 *
 *	A node receiving an RPC distinguishes these
 *	cases based on the channel on which the	RPC arrives.
 */

/*
 *	The dipc_rpc_lock guards access to the
 *	dipc_rpc_list_head and list_tail, which
 *	in turn maintain a queue of incoming DIPC
 *	RPC requests that must be serviced in
 *	thread context.
 */

decl_simple_lock_data(,dipc_rpc_lock_data)

#define		dipc_rpc_lock()		simple_lock(&dipc_rpc_lock_data)
#define		dipc_rpc_unlock()	simple_unlock(&dipc_rpc_lock_data)

rpc_buf_t	dipc_rpc_list_head;
rpc_buf_t	dipc_rpc_list_tail;

dstat_decl(unsigned int c_dipc_rpc_list_length = 0;)
dstat_decl(unsigned int c_dipc_rpc_list_enqueued = 0;)
dstat_decl(unsigned int c_dipc_rpc_list_handled = 0;)
dstat_decl(unsigned int c_dipc_rpc_list_max_length = 0;)

#define	DIPC_RPC_BUF_NULL	(rpc_buf_t)0

void dipc_rpc_deliver(channel_t, rpc_buf_t, boolean_t);
void dipc_rpc_decode(rpc_buf_t);
void dipc_rpc_compute_sizes(void);
char *dipc_rpc_name_lookup(int	procnum);

/*
 *	Number of thread contexts that lie around
 *	waiting to service incoming DIPC RPCs.
 */
#define DIPC_NUM_RPC_THREADS	5
int dipc_num_rpc_threads = DIPC_NUM_RPC_THREADS;

/*
 *	Number of RPC buffers to dedicate for
 *	sending RPCs.
 */
#define DIPC_RPC_SENDERS	5
int dipc_rpc_senders = DIPC_RPC_SENDERS;

/*
 *	Number of RPC buffers to dedicate for
 *	receiving RPCs.
 */
#define DIPC_RPC_RECEIVERS	20
int dipc_rpc_receivers = DIPC_RPC_RECEIVERS;

/*
 *	Number of RPC buffers to dedicate for
 *	sending RPCs from interrupt context.
 */
#define DIPC_RPC_INTR_SENDERS	5
int dipc_rpc_intr_senders = DIPC_RPC_INTR_SENDERS;

/*
 *	Number of RPC buffers to dedicate for
 *	receiving RPCs from interrupt context.
 */
#define DIPC_RPC_INTR_RECEIVERS	5
int dipc_rpc_intr_receivers = DIPC_RPC_INTR_RECEIVERS;

/*
 *	Statistics:  count the number of invocations
 *	of client- and server-side RPCs.
 */
dstat_decl(unsigned int drpc_invocations[DPROC_MAX]);
dstat_decl(unsigned int dproc_invocations[DPROC_MAX]);


void
dipc_rpc_init(void)
{
	kkt_return_t	kktr;
	int		i;

	simple_lock_init(&dipc_rpc_lock_data, ETAP_DIPC_RPC);

	dipc_rpc_list_head = DIPC_RPC_BUF_NULL;
	dipc_rpc_list_tail = DIPC_RPC_BUF_NULL;

	dipc_rpc_compute_sizes();

	for (i = 0; i < dipc_num_rpc_threads; i++)
		(void) kernel_thread(kernel_task, dipc_rpc_server, (char *) 0);

	/* initialize channel for thread-level service */
	kktr = KKT_RPC_INIT(CHAN_DIPC_RPC,
			    dipc_rpc_senders,
			    dipc_rpc_receivers,
			    dipc_rpc_deliver,
			    (kkt_malloc_t)dipc_alloc,
			    (kkt_free_t)dipc_free,
			    (unsigned int) sizeof(struct dipc_rpc_args));
	assert(kktr == KKT_SUCCESS);

	/* initialize channel for interrupt-only service */
	kktr = KKT_RPC_INIT(CHAN_DIPC_RPC_INTR, 
			    dipc_rpc_intr_senders,
			    dipc_rpc_intr_receivers,
			    dipc_rpc_deliver,
			    (kkt_malloc_t)dipc_alloc,
			    (kkt_free_t)dipc_free,
			    (unsigned int) sizeof(struct dipc_rpc_intr_args));
	assert(kktr == KKT_SUCCESS);
}


void
dipc_rpc_deliver(
	channel_t	channel,
	rpc_buf_t	rpc_buf,
	boolean_t	thread_context)
{
	drpc_intr_args_t	args = (drpc_intr_args_t)&rpc_buf->buff;
	dipc_return_t		dr;
	spl_t			s;

	/*
	 * If this RPC is on the DIPC_RPC_INTR channel, then call the
	 * service routine directly.
	 */
	if (channel == CHAN_DIPC_RPC_INTR) {
		switch (args->common.procnum) {
		  case DPROC_WAKEUP_SENDER:
			dstat(dproc_invocations[DPROC_WAKEUP_SENDER]++);
			dr = dproc_wakeup_sender(
					args->args.wakeup_sender_in.waitchan,
					args->args.wakeup_sender_in.status);
			break;

#if	KERNEL_TEST
		  case DPROC_TEST_INTR_RPC:
			dstat(dproc_invocations[DPROC_TEST_INTR_RPC]++);
			dr = dproc_test_intr_rpc(
					args->args.test_intr_rpc_in.arg_in);
			break;
#endif	/* KERNEL_TEST */

		  default:
			printf("dipc_rpc_deliver: bad procnum %d\n",
				args->common.procnum);
			panic("dipc_rpc_deliver");
		}
		args->common.dr = dr;
		KKT_RPC_REPLY(rpc_buf->handle);
		return;
	}

	assert(channel == CHAN_DIPC_RPC);

	/*
	 * This RPC needs thread-level service.  Queue it up and wake up
	 * a service thread.
	 */
	if (thread_context == TRUE) {
		dipc_rpc_decode(rpc_buf);
		return;
	}

	rpc_buf->ulink = (unsigned int *)DIPC_RPC_BUF_NULL;
	s = splkkt();
	dipc_rpc_lock();
	if (dipc_rpc_list_head == DIPC_RPC_BUF_NULL) {
		assert(dipc_rpc_list_tail == DIPC_RPC_BUF_NULL);
		dipc_rpc_list_head = rpc_buf;
		dipc_rpc_list_tail = rpc_buf;
	} else {
		dipc_rpc_list_tail->ulink = (unsigned int *)rpc_buf;
		dipc_rpc_list_tail = rpc_buf;
	}
	dstat(++c_dipc_rpc_list_length);
	dstat(++c_dipc_rpc_list_enqueued);
	dipc_rpc_unlock();
	splx(s);
	thread_wakeup_one((event_t)&dipc_rpc_list_head);
}


/*
 *	Thread context for handling RPC requests.
 *	While some RPCs can be handled in interrupt context,
 *	others require a thread context in which to execute.
 *
 *	Note:  this thread retains its stack because the
 *	pageout path implicitly uses RPCs.
 */
void
dipc_rpc_server(void)
{
	rpc_buf_t		rpc_buf;
	spl_t			s;

	for (;;) {
		s = splkkt();
		dipc_rpc_lock();
		rpc_buf = dipc_rpc_list_head;
		if (rpc_buf == DIPC_RPC_BUF_NULL) {
			assert(dipc_rpc_list_tail == DIPC_RPC_BUF_NULL);
			assert_wait((event_t)&dipc_rpc_list_head, FALSE);
			dipc_rpc_unlock();
			splx(s);
			thread_block(0);
			continue;
		}
		dstat_max(c_dipc_rpc_list_max_length, c_dipc_rpc_list_length);
		dstat(--c_dipc_rpc_list_length);
		dstat(++c_dipc_rpc_list_handled);
		dipc_rpc_list_head = (rpc_buf_t)rpc_buf->ulink;
		if (dipc_rpc_list_head == DIPC_RPC_BUF_NULL)
			dipc_rpc_list_tail = DIPC_RPC_BUF_NULL;
		dipc_rpc_unlock();
		splx(s);

		dipc_rpc_decode(rpc_buf);
	}
}


/*
 *	Called only in thread context to carry out a requested RPC.
 */
void
dipc_rpc_decode(
	rpc_buf_t	rpc_buf)
{
	drpc_args_t		args;
	dipc_return_t		dr;

	args = (drpc_args_t)&rpc_buf->buff;
	switch(args->common.procnum) {
	    case DPROC_PORT_PROBE:
		dstat(dproc_invocations[DPROC_PORT_PROBE]++);
		dr = dproc_port_probe(
				      &args->args.port_probe_in.uid,
				      &args->args.port_probe_out.new_node);
		break;
		
	    case DPROC_ACQUIRE_TRANSITS:
		dstat(dproc_invocations[DPROC_ACQUIRE_TRANSITS]++);
		dr = dproc_acquire_transits(
					    &args->args.acquire_transits_in.uid,
					    &args->args.acquire_transits_out.transits,
					    &args->args.acquire_transits_out.new_node);
		break;
		
	    case DPROC_YIELD_TRANSITS:
		dstat(dproc_invocations[DPROC_YIELD_TRANSITS]++);
		dr = dproc_yield_transits(
					  &args->args.yield_transits_in.uid,
					  args->args.yield_transits_in.transits,
					  &args->args.yield_transits_out.new_node);
		break;
		
	    case DPROC_SEND_META_KMSG:
		dstat(dproc_invocations[DPROC_SEND_META_KMSG]++);
		dr = dproc_send_meta_kmsg(
					  &args->args.send_meta_kmsg_in.uid,
					  args->args.send_meta_kmsg_in.rnode,
					  args->args.send_meta_kmsg_in.size,
					  args->args.send_meta_kmsg_in.handle);
		break;
		
	    case DPROC_MIGRATE_STATE:
		dstat(dproc_invocations[DPROC_MIGRATE_STATE]++);
		assert(dipc_node_is_valid(
					  args->args.migrate_state_in.uid.origin));
		dr = dproc_migrate_state(
					 &args->args.migrate_state_in.uid,
					 args->args.migrate_state_in.seqno,
					 args->args.migrate_state_in.qlimit,
					 args->args.migrate_state_in.transit,
					 args->args.migrate_state_in.remote_sorights);
		break;
		
	    case DPROC_DN_REGISTER:
		dstat(dproc_invocations[DPROC_DN_REGISTER]++);
		dr = dproc_dn_register(
				       &args->args.dn_register_in.uid,
				       args->args.dn_register_in.rnode,
				       &args->args.dn_register_out.new_node);
		break;
		
	    case DPROC_DN_NOTIFY:
		dstat(dproc_invocations[DPROC_DN_NOTIFY]++);
		dr = dproc_dn_notify(
				     &args->args.dn_notify_in.uid);
		break;
		
	    case DPROC_UNBLOCK_PORT:
		dstat(dproc_invocations[DPROC_UNBLOCK_PORT]++);
		dr = dproc_unblock_port(
					&args->args.unblock_port_in.uid,
					args->args.unblock_port_in.port_dead);
		break;
		
	    case DPROC_REQUEST_CALLBACK:
		dstat(dproc_invocations[DPROC_REQUEST_CALLBACK]++);
		dr = dproc_request_callback(
					    args->args.request_callback_in.sending_node);
		break;
		
	    case DPROC_UNBLOCK_NODE:
		dstat(dproc_invocations[DPROC_UNBLOCK_NODE]++);
		dr = dproc_unblock_node(
					args->args.unblock_node_in.receiving_node);
		break;
		
	    case DPROC_GET_SPECIAL_PORT:
		dstat(dproc_invocations[DPROC_GET_SPECIAL_PORT]++);
		dr = dproc_get_special_port(
					    args->args.get_special_port_in.id,
					    &args->args.get_special_port_out.port_uid,
					    &args->args.get_special_port_out.nms);
		break;
		
#if	KERNEL_TEST
	    case DPROC_TEST_SYNC:
		dstat(dproc_invocations[DPROC_TEST_SYNC]++);
		dr = dproc_test_sync(
				     args->args.test_sync_in.node,
				     args->args.test_sync_in.test,
				     &args->args.test_sync_in.uid,
				     args->args.test_sync_in.opaque);
		break;
		
	    case DPROC_TEST_THREAD_RPC:
		dstat(dproc_invocations[DPROC_TEST_THREAD_RPC]++);
		dr = dproc_test_thread_rpc(
					   args->args.test_thread_rpc_in.arg_in,
					   &args->args.test_thread_rpc_out.arg_out);
		break;
#endif	/* KERNEL_TEST */
		
	    default:
		printf("dipc_rpc_server: bad procnum %d\n",
		       args->common.procnum);
		panic("dipc_rpc_server");
	}
	
	args->common.dr = dr;
	KKT_RPC_REPLY(rpc_buf->handle);
}


/*
 * Size of the buffer for each RPC routine, which is the larger of its
 * in args and out args.
 */
vm_size_t drpc_wakeup_sender_size;
vm_size_t drpc_port_probe_size;
vm_size_t drpc_acquire_transits_size;
vm_size_t drpc_yield_transits_size;
vm_size_t drpc_send_meta_kmsg_size;
vm_size_t drpc_migrate_state_size;
vm_size_t drpc_dn_register_size;
vm_size_t drpc_dn_notify_size;
vm_size_t drpc_unblock_port_size;
vm_size_t drpc_request_callback_size;
vm_size_t drpc_unblock_node_size;
vm_size_t drpc_get_special_port_size;
#if	KERNEL_TEST
vm_size_t drpc_test_sync_size;
vm_size_t drpc_test_intr_rpc_size;
vm_size_t drpc_test_thread_rpc_size;
#endif	/* KERNEL_TEST */

#define max(a, b)	( (a) > (b) ? (a) : (b) )

void
dipc_rpc_compute_sizes(void)
{
	drpc_wakeup_sender_size =
				sizeof(struct wakeup_sender_in_args) +
				sizeof(dipc_return_t);
	drpc_port_probe_size =
				max(sizeof(struct port_probe_in_args),
				    sizeof(struct port_probe_out_args)) +
				sizeof(dipc_return_t);
	drpc_acquire_transits_size =
				max(sizeof(struct acquire_transits_in_args),
				    sizeof(struct acquire_transits_out_args)) +
				sizeof(dipc_return_t);
	drpc_yield_transits_size =
				max(sizeof(struct yield_transits_in_args),
				    sizeof(struct yield_transits_out_args)) +
				sizeof(dipc_return_t);
	drpc_send_meta_kmsg_size =
				sizeof(struct send_meta_kmsg_in_args) +
				sizeof(dipc_return_t);
	drpc_migrate_state_size =
				sizeof(struct migrate_state_in_args) +
				sizeof(dipc_return_t);
	drpc_dn_register_size =
				max(sizeof(struct dn_register_in_args),
				    sizeof(struct dn_register_out_args)) +
				sizeof(dipc_return_t);
	drpc_dn_notify_size =
				sizeof(struct dn_notify_in_args) +
				sizeof(dipc_return_t);
	drpc_unblock_port_size =
				sizeof(struct unblock_port_in_args) +
				sizeof(dipc_return_t);
	drpc_request_callback_size =
				sizeof(struct request_callback_in_args) +
				sizeof(dipc_return_t);
	drpc_unblock_node_size =
				sizeof(struct unblock_node_in_args) +
				sizeof(dipc_return_t);
	drpc_get_special_port_size =
				max(sizeof(struct get_special_port_in_args),
				    sizeof(struct get_special_port_out_args)) +
				sizeof(dipc_return_t);
#if	KERNEL_TEST
	drpc_test_sync_size =
				sizeof(struct test_sync_in_args) +
				sizeof(dipc_return_t);
	drpc_test_intr_rpc_size =
				sizeof(struct test_intr_rpc_in_args) +
				sizeof(dipc_return_t);
	drpc_test_thread_rpc_size =
				max(sizeof(struct test_thread_rpc_in_args),
				    sizeof(struct test_thread_rpc_out_args)) +
				sizeof(dipc_return_t);
#endif	/* KERNEL_TEST */
}

/* DIPC RPC client routines */

kkt_return_t
drpc_wakeup_sender(
	IN	node_name	node,
	IN	unsigned int	waitchan,
	IN	dipc_return_t	status,
	OUT	dipc_return_t	*dr)
{
	handle_t		krpc_handle;
	drpc_intr_args_t	args;
	kkt_return_t		kktr;

	dstat(drpc_invocations[DPROC_WAKEUP_SENDER]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC_INTR, &krpc_handle,
						drpc_wakeup_sender_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_WAKEUP_SENDER;
	args->args.wakeup_sender_in.waitchan = waitchan;
	args->args.wakeup_sender_in.status = status;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_port_probe(
	IN	node_name	node,
	IN	uid_t		*uidp,
	OUT	dipc_return_t	*dr,
	OUT	node_name	*new_node)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_PORT_PROBE]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_port_probe_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_PORT_PROBE;
	args->args.port_probe_in.uid = *uidp;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
		*new_node = args->args.port_probe_out.new_node;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}

/*
 * drpc_alive_notify
 *
 * Called to notify another node that this node is alive and ready
 */
kkt_return_t
drpc_alive_notify(
	IN	node_name	node,
	OUT	dipc_return_t	*dr)
{
	handle_t		krpc_handle;
	drpc_args_t		args;
	kkt_return_t		kktr;

	dstat(drpc_invocations[DPROC_ALIVE_NOTIFY]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC_INTR, &krpc_handle,
						drpc_wakeup_sender_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_ALIVE_NOTIFY;
	args->args.alive_args.node = dipc_node_self();

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}

kkt_return_t
drpc_acquire_transits(
	IN	node_name	node,
	IN	uid_t		*uidp,
	OUT	dipc_return_t	*dr,
	OUT	long		*transits,
	OUT	node_name	*new_node)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_ACQUIRE_TRANSITS]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_acquire_transits_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_ACQUIRE_TRANSITS;
	args->args.acquire_transits_in.uid = *uidp;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
		*transits = args->args.acquire_transits_out.transits;
		*new_node = args->args.acquire_transits_out.new_node;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_yield_transits(
	IN	node_name	node,
	IN	uid_t		*uidp,
	IN	long		transits,
	OUT     dipc_return_t   *dr,
        OUT     node_name       *new_node)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_YIELD_TRANSITS]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_yield_transits_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_YIELD_TRANSITS;
	args->args.yield_transits_in.uid = *uidp;
	args->args.yield_transits_in.transits = transits;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
		*new_node = args->args.yield_transits_out.new_node;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_send_meta_kmsg(
	IN	node_name	node,
	IN	uid_t		*uidp,
	IN	node_name	onode,
	IN	mach_msg_size_t	size,
	IN	handle_t	handle,
	OUT	dipc_return_t	*dr)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_SEND_META_KMSG]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_send_meta_kmsg_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_SEND_META_KMSG;
	args->args.send_meta_kmsg_in.uid = *uidp;
	args->args.send_meta_kmsg_in.rnode = onode;
	args->args.send_meta_kmsg_in.size = size;
	args->args.send_meta_kmsg_in.handle = handle;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_migrate_state(
	IN	node_name		node,
	IN	uid_t			*uidp,
	IN	mach_port_seqno_t	seqno,
	IN	mach_port_msgcount_t	qlimit,
	IN	long			transit,
	IN	long			sorights,
	OUT	dipc_return_t		*dr)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_MIGRATE_STATE]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_migrate_state_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_MIGRATE_STATE;
	args->args.migrate_state_in.uid = *uidp;
	assert(dipc_node_is_valid(args->args.migrate_state_in.uid.origin));
	args->args.migrate_state_in.seqno = seqno;
	args->args.migrate_state_in.qlimit = qlimit;
	args->args.migrate_state_in.transit = transit;
	args->args.migrate_state_in.remote_sorights = sorights;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_dn_register(
	IN	node_name	node,
	IN	uid_t		*uidp,
	IN	node_name	rnode,
	OUT	dipc_return_t	*dr,
	OUT	node_name	*new_node)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_DN_REGISTER]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_dn_register_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_DN_REGISTER;
	args->args.dn_register_in.uid = *uidp;
	args->args.dn_register_in.rnode = rnode;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
		*new_node = args->args.dn_register_out.new_node;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_dn_notify(
	IN	node_name	node,
	IN	uid_t		*uidp)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_DN_NOTIFY]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_dn_notify_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_DN_NOTIFY;
	args->args.dn_notify_in.uid = *uidp;

	kktr = KKT_RPC(node, krpc_handle);
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_unblock_port(
	IN	node_name	node,
	IN	uid_t		*uidp,
	IN	boolean_t	port_dead,
	OUT	dipc_return_t	*dr)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_UNBLOCK_PORT]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_unblock_port_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_UNBLOCK_PORT;
	args->args.unblock_port_in.uid = *uidp;
	args->args.unblock_port_in.port_dead = port_dead;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_request_callback(
	IN	node_name	node,
	IN	node_name	sending_node,
	OUT	dipc_return_t	*dr)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_REQUEST_CALLBACK]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_request_callback_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_REQUEST_CALLBACK;
	args->args.request_callback_in.sending_node = sending_node;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_unblock_node(
	IN	node_name	node,
	IN	node_name	receiving_node,
	OUT	dipc_return_t	*dr)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_UNBLOCK_NODE]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_unblock_node_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_UNBLOCK_NODE;
	args->args.unblock_node_in.receiving_node = receiving_node;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_get_special_port(
	IN	node_name		node,
	IN	unsigned long		id,
	OUT	dipc_return_t		*dr,
	OUT	uid_t			*port_uid,
	OUT	boolean_t		*nms)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_GET_SPECIAL_PORT]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_get_special_port_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_GET_SPECIAL_PORT;
	args->args.get_special_port_in.id = id;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
		*port_uid = args->args.get_special_port_out.port_uid;
		*nms = args->args.get_special_port_out.nms;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


#if	KERNEL_TEST

kkt_return_t
drpc_test_sync(
	IN	node_name	node,
	IN	unsigned int	test,
	IN	uid_t		*uidp,
	IN	unsigned int	opaque,
	OUT	dipc_return_t	*dr)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_TEST_SYNC]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_test_sync_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_TEST_SYNC;
	args->args.test_sync_in.node = dipc_node_self();
	args->args.test_sync_in.test = test;
	args->args.test_sync_in.uid = *uidp;
	args->args.test_sync_in.opaque = opaque;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_test_intr_rpc(
	IN	node_name	node,
	IN	unsigned int	arg_in,
	OUT	dipc_return_t	*dr)
{
	handle_t		krpc_handle;
	drpc_intr_args_t	args;
	kkt_return_t		kktr;

	dstat(drpc_invocations[DPROC_TEST_INTR_RPC]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC_INTR, &krpc_handle,
						drpc_test_intr_rpc_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_TEST_INTR_RPC;
	args->args.test_intr_rpc_in.arg_in = arg_in;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}


kkt_return_t
drpc_test_thread_rpc(
	IN	node_name	node,
	IN	unsigned int	arg_in,
	OUT	dipc_return_t	*dr,
	OUT	unsigned int	*arg_out)
{
	handle_t	krpc_handle;
	drpc_args_t	args;
	kkt_return_t	kktr;

	dstat(drpc_invocations[DPROC_TEST_THREAD_RPC]++);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_RPC, &krpc_handle,
						drpc_test_thread_rpc_size);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(krpc_handle, (void **)&args);
	assert(kktr == KKT_SUCCESS);
	args->common.procnum = DPROC_TEST_THREAD_RPC;
	args->args.test_thread_rpc_in.arg_in = arg_in;

	kktr = KKT_RPC(node, krpc_handle);
	if (kktr == KKT_SUCCESS) {
		*dr = args->common.dr;
		*arg_out = args->args.test_thread_rpc_out.arg_out;
	}
	KKT_RPC_HANDLE_FREE(krpc_handle);
	return kktr;
}

#endif	/* KERNEL_TEST */

#if	MACH_KDB
#include <ddb/db_output.h>
#define	printf	kdbprintf

typedef struct rpc_call_name {
	unsigned int	call_number;
	char		*call_name;
} rcn;

rcn	rpc_names[] = {
	{ DPROC_WAKEUP_SENDER,		"wakeup_sender" },
	{ DPROC_PORT_PROBE,		"port_probe" },
	{ DPROC_ACQUIRE_TRANSITS,	"acquire_transits" },
	{ DPROC_YIELD_TRANSITS,		"yield_transits" },
	{ DPROC_SEND_META_KMSG,		"send_meta_kmsg" },
	{ DPROC_MIGRATE_STATE,		"migrate_state" },
	{ DPROC_DN_REGISTER,		"dn_register" },
	{ DPROC_DN_NOTIFY,		"dn_notify" },
	{ DPROC_UNBLOCK_PORT,		"unblock_port" },
	{ DPROC_REQUEST_CALLBACK,	"request_callback" },
	{ DPROC_UNBLOCK_NODE,		"unblock_node" },
	{ DPROC_GET_SPECIAL_PORT,	"get_special_port" },
	{ DPROC_ALIVE_NOTIFY,		"alive_notify" },
#if	KERNEL_TEST
	{ DPROC_TEST_SYNC,		"test_sync" },
	{ DPROC_TEST_INTR_RPC,		"test_intr_rpc" },
	{ DPROC_TEST_THREAD_RPC,	"test_thread_rpc" },
#endif	/* KERNEL_TEST */
};

rcn	*rpc_names_end = rpc_names + (sizeof(rpc_names) / sizeof(rcn));

char *
dipc_rpc_name_lookup(
	int	procnum)
{
	rcn	*r;

	for (r = rpc_names; r < rpc_names_end; ++r)
		if (r->call_number == procnum)
			return r->call_name;
	return "unknown";
}


#if	DIPC_DO_STATS
void
dipc_rpc_stats()
{
	unsigned int	i;
	extern int	indent;

	iprintf("RPC statistics:\n");
	indent += 2;

	iprintf("Call Number  Name	                   DRPCs     DPROCs\n");
	for (i = 0; i < DPROC_MAX; ++i)
		if (drpc_invocations[i] != 0 || dproc_invocations[i] != 0)
			iprintf("%11d  %-20s  %9d  %9d\n",
				i, dipc_rpc_name_lookup(i),
				drpc_invocations[i], dproc_invocations[i]);

	iprintf("max thread args %d bytes, intr args %d bytes\n",
		sizeof(struct dipc_rpc_args),
		sizeof(struct dipc_rpc_intr_args));

	indent -= 2;
}
#endif	/* DIPC_DO_STATS */
#endif	/* MACH_KDB */
