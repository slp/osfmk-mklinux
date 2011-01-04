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
/* CMU_HIST */
/*
 * Revision 2.15.2.2  92/03/28  10:10:10  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:15:54  jeffreyh]
 * 
 * Revision 2.16  92/02/19  16:06:28  elf
 * 	Added syscall_thread_depress_abort.
 * 	[92/01/20            rwd]
 * 
 * Revision 2.15.2.1  92/01/03  16:36:14  jsb
 * 	Corrected log.
 * 	[91/12/24  14:42:53  jsb]
 * 
 * 
 * Revision 2.15  91/11/14  16:56:39  rpd
 * 	Replaced call to ipc_kmsg_copyout_dest with ipc_kmsg_copyout_to_kernel
 *	in mach_msg_rpc_from_kernel, to preserve rights and memory.
 * 	[91/11/00            jsb]
 *
 * Revision 2.14  91/08/28  11:14:34  jsb
 * 	Fixed syscall_mach_port_insert_right and syscall_task_set_special_port
 * 	to handle non-valid port names correctly.
 * 	[91/08/15            rpd]
 * 
 * 	Changed MACH_RCV_TOO_LARGE and MACH_RCV_INVALID_NOTIFY to work
 * 	like MACH_RCV_HEADER_ERROR, using ipc_kmsg_copyout_dest.
 * 	[91/08/12            rpd]
 * 
 * 	Added seqno argument to ipc_mqueue_receive.
 * 	[91/08/10            rpd]
 * 	Conditionalize mach_msg_rpc_from_kernel on NORMA_VM, not NORMA_TASK.
 * 	[91/08/14  18:34:30  jsb]
 * 
 * Revision 2.13  91/08/03  18:18:53  jsb
 * 	Corrected comment for mach_msg_send_from_kernel.
 * 	[91/07/04  10:08:23  jsb]
 * 
 * Revision 2.12  91/07/30  15:45:43  rvb
 * 	Fixed syscall_vm_map to handle non-valid memory objects correctly.
 * 	[91/07/12            rpd]
 * 
 * Revision 2.11  91/06/17  15:47:05  jsb
 * 	Renamed NORMA conditionals.
 * 	[91/06/17  10:50:07  jsb]
 * 
 * Revision 2.10  91/06/06  17:07:09  jsb
 * 	NORMA_TASK: We still need mach_msg_rpc_from_kernel.
 * 	[91/05/14  09:13:45  jsb]
 * 
 * Revision 2.9  91/05/20  22:22:16  rpd
 * 	Cleaned up the semantics of the syscall forms of kernel RPCs.
 * 	[91/05/20            rpd]
 * 
 * Revision 2.8  91/05/14  16:42:22  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/02/05  17:26:46  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:13:09  mrt]
 * 
 * Revision 2.6  91/01/08  15:15:44  rpd
 * 	Don't need mach_msg_rpc_from_kernel.
 * 	[90/12/26            rpd]
 * 	Updated ipc_mqueue_receive calls.
 * 	[90/11/21            rpd]
 * 	Removed MACH_IPC_GENNOS.
 * 	[90/11/09            rpd]
 * 
 * Revision 2.5  90/11/05  14:30:59  rpd
 * 	Changed ip_reference to ipc_port_reference.
 * 	Changed ip_release to ipc_port_release.
 * 	Removed ipc_object_release_macro.
 * 	Use new ip_reference and ip_release.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.4  90/09/09  14:32:13  rpd
 * 	Added mach_port_allocate_name.
 * 	[90/09/03            rwd]
 * 
 * Revision 2.3  90/06/19  22:58:57  rpd
 * 	Optimized port_name_to_task, port_name_to_thread, etc.
 * 	[90/06/03            rpd]
 * 
 * 	Added mach_port_allocate, mach_port_deallocate, mach_port_insert_right.
 * 	[90/06/02            rpd]
 * 
 * Revision 2.2  90/06/02  14:54:16  rpd
 * 	Moved trap versions of kernel calls here
 * 	from vm/vm_user.c, kern/task.c.
 * 	Converted them to new IPC.
 * 	[90/05/31            rpd]
 * 
 * 	Created for new IPC.
 * 	[90/03/26  23:47:15  rpd]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */

#include <norma_vm.h>
#include <mach_rt.h>
#include <dipc.h>

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/thread_status.h>
#include <mach/mig_errors.h>
#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <mach/mach_syscalls.h>
#include <mach/mach_server.h>
#include <mach/mach_port_server.h>
#include <mach/mach_host_server.h>
#include <kern/ast.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/task.h>
#include <kern/ipc_kobject.h>
#include <kern/misc_protos.h>
#include <vm/vm_map.h>
#include <vm/vm_user.h>
#include <ipc/port.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_thread.h>
#if	DIPC
#include <dipc/dipc_kmsg.h>
#endif	/* DIPC */
#include <device/dev_hdr.h>
#include <device/ds_routines.h>
#include <device/io_req.h>

/*
 * Forward declarations
 */
thread_t port_name_to_thread(
	mach_port_t	name);

task_t port_name_to_task(
	mach_port_t	name);

device_t port_name_to_device(
	mach_port_t	name);

io_done_queue_t port_name_to_io_done_queue(
	mach_port_t	name);

ipc_space_t port_name_to_space(
	mach_port_t	name);

vm_map_t port_name_to_map(
	mach_port_t	name);

thread_act_t port_name_to_act(
	mach_port_t	name);

subsystem_t port_name_to_subsystem(
	mach_port_t	name);

host_t port_name_to_host(
	mach_port_t	name);

vm_object_t port_name_to_vm_object(
	mach_port_t	name);

/* Default (zeroed) template for qos */

static mach_port_qos_t	qos_template;

/*
 *	Routine:	mach_msg_send_from_kernel
 *	Purpose:
 *		Send a message from the kernel.
 *
 *		This is used by the client side of KernelUser interfaces
 *		to implement SimpleRoutines.  Currently, this includes
 *		device_reply and memory_object messages.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Sent the message.
 *		MACH_SEND_INVALID_DATA	Bad destination port.
 */

mach_msg_return_t
mach_msg_send_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_size_t		send_size)
{
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;

	if (!MACH_PORT_VALID(msg->msgh_remote_port))
		return MACH_SEND_INVALID_DEST;

	mr = ipc_kmsg_get_from_kernel(msg, send_size, &kmsg);
	if (mr != MACH_MSG_SUCCESS)
		panic("mach_msg_send_from_kernel");

	ipc_kmsg_copyin_from_kernel(kmsg);
	ipc_mqueue_send_always(kmsg);

	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	mach_msg_rpc_from_kernel
 *	Purpose:
 *		Send a message from the kernel and receive a reply.
 *		Uses ith_rpc_reply for the reply port.
 *
 *		This is used by the client side of KernelUser interfaces
 *		to implement Routines.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Sent the message.
 *		MACH_RCV_PORT_DIED	The reply port was deallocated.
 */

mach_msg_return_t
mach_msg_rpc_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_size_t		send_size,
	mach_msg_size_t		rcv_size)
{
	ipc_thread_t self = current_thread();
	thread_act_t a_self = self->top_act;
	ipc_port_t reply;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	assert(MACH_PORT_VALID(msg->msgh_remote_port));
	assert(msg->msgh_local_port == MACH_PORT_NULL);

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	mr = ipc_kmsg_get_from_kernel(msg, send_size, &kmsg);
	if (mr != MACH_MSG_SUCCESS)
		panic("mach_msg_rpc_from_kernel");

	rpc_lock(self);

	reply = self->ith_rpc_reply;
	if (reply == IP_NULL) {
		rpc_unlock(self);
		reply = ipc_port_alloc_reply();
		rpc_lock(self);
		if ((reply == IP_NULL) ||
		    (self->ith_rpc_reply != IP_NULL))
			panic("mach_msg_rpc_from_kernel");
		self->ith_rpc_reply = reply;
	}

	/* insert send-once right for the reply port */
	kmsg->ikm_header.msgh_local_port = (mach_port_t)reply;
	kmsg->ikm_header.msgh_bits |=
		MACH_MSGH_BITS(0, MACH_MSG_TYPE_MAKE_SEND_ONCE);

	ipc_port_reference(reply);
	rpc_unlock(self);

	ipc_kmsg_copyin_from_kernel(kmsg);

	ipc_mqueue_send_always(kmsg);

	for (;;) {
		ipc_mqueue_t mqueue;

		ip_lock(reply);
		if ( !ip_active(reply)
		    || (self->top_act && !self->top_act->active)) {
			ip_unlock(reply);
			ipc_port_release(reply);
			return MACH_RCV_PORT_DIED;
		}

		assert(reply->ip_pset == IPS_NULL);
		mqueue = &reply->ip_messages;
		imq_lock(mqueue);
		ip_unlock(reply);

		mr = ipc_mqueue_receive(mqueue, MACH_MSG_OPTION_NONE,
					MACH_MSG_SIZE_MAX,
					MACH_MSG_TIMEOUT_NONE,
					FALSE,
					(void(*)(void))SAFE_INTERNAL_RECEIVE,
					&kmsg, &seqno);
		/* mqueue is unlocked */
		if (mr == MACH_MSG_SUCCESS)
			break;

#if	DIPC
		/* can't tolerate transport errors right now */
		if (mr == MACH_RCV_TRANSPORT_ERROR) {
			printf("mach_msg_rpc_from_kernel: transport error\n");
			printf("    reply port 0x%x kmsg 0x%x\n", reply, kmsg);
			/*
			 * kmsg should be freed but we'll leave it around
			 * since this shouldn't happen anyway.
			 * The assert below will trip because of this error.
			 */
		}

#endif	/* DIPC */
		assert((mr == MACH_RCV_INTERRUPTED) ||
		       (mr == MACH_RCV_PORT_DIED));


		if (self->top_act && self->top_act->handlers) {
			ipc_port_release(reply);
			act_execute_returnhandlers();
			ipc_port_reference(reply);
		}
	}
	ipc_port_release(reply);

	/*
	 * XXXXX  Set manually for now ...
	 *	No, why even bother, since the effort is wasted?
	 *
	{ mach_msg_format_0_trailer_t *trailer = (mach_msg_format_0_trailer_t *)
		((vm_offset_t)&kmsg->ikm_header + kmsg->ikm_header.msgh_size);
	trailer->msgh_trailer_type = MACH_MSG_TRAILER_FORMAT_0;
	trailer->msgh_trailer_size = MACH_MSG_TRAILER_MINIMUM_SIZE;
	}
	 *****/

	if (rcv_size < kmsg->ikm_header.msgh_size) {
		ipc_kmsg_copyout_dest(kmsg, ipc_space_reply);
		ipc_kmsg_put_to_kernel(msg, kmsg, kmsg->ikm_header.msgh_size);
		return MACH_RCV_TOO_LARGE;
	}

	/*
	 *	We want to preserve rights and memory in reply!
	 *	We don't have to put them anywhere; just leave them
	 *	as they are.
	 */

	ipc_kmsg_copyout_to_kernel(kmsg, ipc_space_reply);
	ipc_kmsg_put_to_kernel(msg, kmsg, kmsg->ikm_header.msgh_size);
	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	mach_msg_abort_rpc
 *	Purpose:
 *		Destroy the thread's ith_rpc_reply port.
 *		This will interrupt a mach_msg_rpc_from_kernel
 *		with a MACH_RCV_PORT_DIED return code.
 *	Conditions:
 *		Called with all RPC-related locks held for
 *		thread (see act_lock_thread()).
 */

void
mach_msg_abort_rpc(
	ipc_thread_t	thread)
{
	ipc_port_t reply = IP_NULL;

	if (thread->top_act->ith_self != IP_NULL) {
		reply = thread->ith_rpc_reply;
		thread->ith_rpc_reply = IP_NULL;
	}

	if (reply != IP_NULL)
		ipc_port_dealloc_reply(reply);
}

/*
 *	Routine:	mach_msg
 *	Purpose:
 *		Like mach_msg_overwrite_trap except that message buffers
 *		live in kernel space.  Doesn't handle any options.
 *
 *		This is used by in-kernel server threads to make
 *		kernel calls, to receive request messages, and
 *		to send reply messages.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 */

mach_msg_return_t
mach_msg_overwrite(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		send_size,
	mach_msg_size_t		rcv_size,
	mach_port_t		rcv_name,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify,
	mach_msg_header_t	*rcv_msg,
        mach_msg_size_t		rcv_msg_size)
{
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;
	mach_msg_format_0_trailer_t *trailer;

#ifdef	lint
	notify = (mach_port_t) 0;
	rcv_msg = (mach_msg_header_t *) 0;
	timeout++;
	rcv_msg_size = 0;
#endif	/* lint */

	if (option & MACH_SEND_MSG) {
		mr = ipc_kmsg_get_from_kernel(msg, send_size, &kmsg);
		if (mr != MACH_MSG_SUCCESS)
			panic("mach_msg");

		mr = ipc_kmsg_copyin(kmsg, space, map, MACH_PORT_NULL);
		if (mr != MACH_MSG_SUCCESS) {
			ikm_free(kmsg);
			return mr;
		}

		do
			mr = ipc_mqueue_send(kmsg, MACH_MSG_OPTION_NONE,
					     MACH_MSG_TIMEOUT_NONE);
		while (mr == MACH_SEND_INTERRUPTED);
		assert(mr == MACH_MSG_SUCCESS);
	}

	if (option & MACH_RCV_MSG) {
		ipc_thread_t self = current_thread();

		self->ith_scatter_list = MACH_MSG_BODY_NULL;
		do {
			ipc_object_t object;
			ipc_mqueue_t mqueue;

			mr = ipc_mqueue_copyin(space, rcv_name,
					       &mqueue, &object);
			if (mr != MACH_MSG_SUCCESS)
				return mr;
			/* hold ref for object; mqueue is locked */

			mr = ipc_mqueue_receive(mqueue, MACH_MSG_OPTION_NONE,
						MACH_MSG_SIZE_MAX,
						MACH_MSG_TIMEOUT_NONE,
						FALSE,
					(void(*)(void))SAFE_EXTERNAL_RECEIVE,
						&kmsg, &seqno);
			/* mqueue is unlocked */
			ipc_object_release(object);
#if	DIPC
			if (mr == MACH_RCV_TRANSPORT_ERROR) {
				ipc_port_t port =
				  (ipc_port_t)kmsg->ikm_header.msgh_remote_port;

				/*
				 * release DIPC delivery reference; we have
				 * yet to acquire a reference for the kmsg
				 * itself (which is normally done by
				 * dipc_kmsg_copyout).  We free the useless
				 * kmsg and return the error.  Note that the
				 * caller is not getting a kmsg back; a sequence
				 * number will be lost, but the kernel doesn't
				 * pay attention to sequence numbers anyway.
				 * XXX Is there any reason that we should,
				 * say, be copying out this message to reclaim
				 * transits? XXX
				 */
	    			ipc_port_release(port);
				ikm_free(kmsg);
			}
#endif	/* DIPC */
		} while (mr == MACH_RCV_INTERRUPTED);
		if (mr != MACH_MSG_SUCCESS)
			return mr;

		trailer = (mach_msg_format_0_trailer_t *) 
		    ((vm_offset_t)&kmsg->ikm_header + kmsg->ikm_header.msgh_size);
		if (option & MACH_RCV_TRAILER_MASK) {
			trailer->msgh_seqno = seqno;
			trailer->msgh_trailer_size = REQUESTED_TRAILER_SIZE(option);
		}

		if (rcv_size < (kmsg->ikm_header.msgh_size + trailer->msgh_trailer_size)) {
			ipc_kmsg_copyout_dest(kmsg, space);
			ipc_kmsg_put_to_kernel(msg, kmsg, sizeof *msg);
			return MACH_RCV_TOO_LARGE;
		}

		mr = ipc_kmsg_copyout(kmsg, space, map, MACH_PORT_NULL,
				      MACH_MSG_BODY_NULL);
		if (mr != MACH_MSG_SUCCESS) {
			if ((mr &~ MACH_MSG_MASK) == MACH_RCV_BODY_ERROR
#if	DIPC
				|| mr == MACH_RCV_TRANSPORT_ERROR
#endif	/* DIPC */
			    ) {
				ipc_kmsg_put_to_kernel(msg, kmsg,
						kmsg->ikm_header.msgh_size + trailer->msgh_trailer_size);
			} else {
				ipc_kmsg_copyout_dest(kmsg, space);
				ipc_kmsg_put_to_kernel(msg, kmsg, sizeof *msg);
			}

			return mr;
		}

		ipc_kmsg_put_to_kernel(msg, kmsg, 
		      kmsg->ikm_header.msgh_size + trailer->msgh_trailer_size);
	}

	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	mig_get_reply_port
 *	Purpose:
 *		Called by client side interfaces living in the kernel
 *		to get a reply port.  This port is used for
 *		mach_msg() calls which are kernel calls.
 */

mach_port_t
mig_get_reply_port(void)
{
	ipc_thread_t self = current_thread();

	if (self->ith_mig_reply == MACH_PORT_NULL)
		self->ith_mig_reply = mach_reply_port();

	return self->ith_mig_reply;
}

/*
 *	Routine:	mig_dealloc_reply_port
 *	Purpose:
 *		Called by client side interfaces to get rid of a reply port.
 *		Shouldn't ever be called inside the kernel, because
 *		kernel calls shouldn't prompt Mig to call it.
 */

void
mig_dealloc_reply_port(
	mach_port_t	reply_port)
{
	panic("mig_dealloc_reply_port");
}

/*
 *	Routine:	mig_put_reply_port
 *	Purpose:
 *		Called by client side interfaces after each RPC to 
 *		let the client recycle the reply port if it wishes.
 */
void
mig_put_reply_port(
	mach_port_t	reply_port)
{
}

/*
 * mig_strncpy.c - by Joshua Block
 *
 * mig_strncp -- Bounded string copy.  Does what the library routine strncpy
 * OUGHT to do:  Copies the (null terminated) string in src into dest, a 
 * buffer of length len.  Assures that the copy is still null terminated
 * and doesn't overflow the buffer, truncating the copy if necessary.
 *
 * Parameters:
 * 
 *     dest - Pointer to destination buffer.
 * 
 *     src - Pointer to source string.
 * 
 *     len - Length of destination buffer.
 */
int 
mig_strncpy(
	char	*dest,
	char	*src,
	int	len)
{
    int i;

    if (len <= 0)
	return 0;

    for (i=1; i<len; i++)
	if (! (*dest++ = *src++))
	    return i;

    *dest = '\0';
    return i;
}


#define	fast_send_right_lookup(name, port, abort)			\
MACRO_BEGIN								\
	register ipc_space_t space = current_space();			\
	register ipc_entry_t entry;					\
	register mach_port_index_t index;				\
									\
	is_read_lock(space);						\
	assert(space->is_active);					\
									\
	entry = 0;							\
	index = MACH_PORT_INDEX(name);					\
	if (index < space->is_table_size) {				\
		entry = &space->is_table[index];			\
	}								\
	if (entry == 0 || (entry->ie_bits &				\
		(IE_BITS_GEN_MASK|MACH_PORT_TYPE_SEND)) != 		\
		(MACH_PORT_GEN(name) | MACH_PORT_TYPE_SEND)) {		\
		is_read_unlock(space);					\
		abort;							\
	}								\
	port = (ipc_port_t) entry->ie_object;				\
									\
	assert(port != IP_NULL);					\
									\
	ip_lock(port);							\
	/* can safely unlock space now that port is locked */		\
	is_read_unlock(space);						\
MACRO_END


thread_act_t
port_name_to_act(
	mach_port_t	name)
{
	register ipc_port_t port;
	boolean_t r;
	thread_act_t thr_act;

	r = FALSE;
	thr_act = THR_ACT_NULL;
	while (!r) {
		fast_send_right_lookup(name, port, goto abort);
		/* port is locked */

		r = ref_act_port_locked(port, &thr_act);
		/* port unlocked */
	}

	return thr_act;

    abort: {
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return THR_ACT_NULL;

	thr_act = convert_port_to_act((ipc_port_t)kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return thr_act;
    }
}

subsystem_t
port_name_to_subsystem(
	mach_port_t	name)
{
	register ipc_port_t port;
	register subsystem_t subsys;

	subsys = SUBSYSTEM_NULL;
	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
		(ip_kotype(port) == IKOT_SUBSYSTEM)) {
		subsys = (subsystem_t) port->ip_kobject;
		assert(subsys != SUBSYSTEM_NULL);
	}

	ip_unlock(port);
	return subsys;

abort: {
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr == KERN_SUCCESS) {
		subsys = convert_port_to_subsystem((ipc_port_t)kern_port);
		if (IP_VALID(kern_port))
			ipc_port_release_send(kern_port);
	}
	return subsys;
	}
}

task_t
port_name_to_task(
	mach_port_t	name)
{
	register ipc_port_t port;
	boolean_t r;
	task_t task;

	r = FALSE;
	task = TASK_NULL;
	while (!r) {
		fast_send_right_lookup(name, port, goto abort);
		/* port is locked */

		r = ref_task_port_locked(port, &task);
		/* port unlocked */
	}

	return task;

abort: {
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return TASK_NULL;

	task = convert_port_to_task(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return task;
    }
}

vm_map_t
port_name_to_map(
	mach_port_t	name)
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_TASK)) {
		register vm_map_t map;

		map = ((task_t) port->ip_kobject)->map;
		assert(map != VM_MAP_NULL);

		mutex_lock(&map->s_lock);
		/* can safely unlock port now that map is locked */
		ip_unlock(port);

		map->ref_count++;
		vm_map_res_reference(map);
		mutex_unlock(&map->s_lock);

		return map;
	}

	ip_unlock(port);
	return VM_MAP_NULL;

    abort: {
	vm_map_t map;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return VM_MAP_NULL;

	map = convert_port_to_map(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return map;
    }
}

device_t
port_name_to_device(
	mach_port_t	name)
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    ip_kotype(port) == IKOT_DEVICE) {
		register device_t device;

		device = (device_t) port->ip_kobject;
		assert(device != DEVICE_NULL);
		device_reference(device);

		/* can safely unlock port now that device is locked */
		ip_unlock(port);

		return device;
	}

	ip_unlock(port);
	return DEVICE_NULL;

    abort: {
	device_t device;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return DEVICE_NULL;

	device = dev_port_lookup(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return device;
    }
}

io_done_queue_t
port_name_to_io_done_queue(
	mach_port_t	name)
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_IO_DONE_QUEUE)) {
		register io_done_queue_t queue;

		queue = (io_done_queue_t) port->ip_kobject;
		assert(queue != IO_DONE_QUEUE_NULL);
		io_done_queue_reference(queue);

		/* can safely unlock port now that queue is locked */
		ip_unlock(port);

		return queue;
	}

	ip_unlock(port);
	return IO_DONE_QUEUE_NULL;

    abort: {
	io_done_queue_t queue;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return IO_DONE_QUEUE_NULL;

	queue = io_done_queue_port_lookup(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return queue;
    }
}

ipc_space_t
port_name_to_space(
	mach_port_t	name)
{
	register ipc_port_t port;
	boolean_t r;
	ipc_space_t space;

	r = FALSE;
	space = IS_NULL;
	while (!r) {
		fast_send_right_lookup(name, port, goto abort);
		/* port is locked */

		r = ref_space_port_locked(port, &space);
		/* port unlocked */
	}

	return space;

abort: {
	ipc_space_t space;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return IS_NULL;

	space = convert_port_to_space(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return space;
    }
}

host_t
port_name_to_host(
	mach_port_t	name)
{
	ipc_port_t	port;
	host_t		host;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_HOST ||
	     ip_kotype(port) == IKOT_HOST_PRIV)) {
		host = (host_t) port->ip_kobject;
	} else {
		host = HOST_NULL;
	}

	ip_unlock(port);
	return host;

    abort: {
	ipc_port_t	kern_port;
	kern_return_t	kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return HOST_NULL;

	host = convert_port_to_host(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return host;
	}
}

vm_object_t
port_name_to_vm_object(
	mach_port_t	name)
{
	register ipc_port_t port;
	vm_object_t object;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return VM_OBJECT_NULL;

	object = vm_object_lookup(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);


	return object;
}

/*
 *	Things to keep in mind:
 *
 *	The idea here is to duplicate the semantics of the true kernel RPC.
 *	The destination port/object should be checked first, before anything
 *	that the user might notice (like ipc_object_copyin).  Return
 *	MACH_SEND_INTERRUPTED if it isn't correct, so that the user stub
 *	knows to fall back on an RPC.  For other return values, it won't
 *	retry with an RPC.  The retry might get a different (incorrect) rc.
 *	Return values are only set (and should only be set, with copyout)
 *	on successfull calls.
 */

kern_return_t
syscall_vm_map(
	mach_port_t	target_map,
	vm_offset_t	*address,
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	mach_port_t	memory_object,
	vm_offset_t	offset,
	boolean_t	copy,
	vm_prot_t	cur_protection,
	vm_prot_t	max_protection,
	vm_inherit_t	inheritance)
{
	vm_map_t		map;
	ipc_port_t		port;
	vm_offset_t		addr;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(memory_object)) {
		result = ipc_object_copyin(current_space(), memory_object,
					   MACH_MSG_TYPE_COPY_SEND,
					   (ipc_object_t *) &port);
		if (result != KERN_SUCCESS) {
			vm_map_deallocate(map);
			return result;
		}
	} else
		port = (ipc_port_t) memory_object;

	if (copyin((char *)address, (char *)&addr, sizeof(vm_offset_t)))
		result = KERN_INVALID_ADDRESS;
	else
		result = vm_map(map, &addr, size, mask, anywhere,
			port, offset, copy,
			cur_protection, max_protection,	inheritance);

	if (result == KERN_SUCCESS) {
		if (copyout((char*)&addr,(char*)address,sizeof(vm_offset_t))){
			(void) vm_deallocate(map, addr, size);
			result = KERN_INVALID_ADDRESS;
		}
	}
	if (IP_VALID(port))
		ipc_port_release_send(port);
	vm_map_deallocate(map);

	return result;
}

#define TASK_INFO_SMALL 128	/* XXX ought to tune per platform */
kern_return_t
syscall_task_info(
	mach_port_t		task,
	task_flavor_t		flavor,
	task_info_t		task_info_out,	/* ptr to OUT array */
	mach_msg_type_number_t	*task_info_cnt)	/* IN/OUT */
{
	task_t			t;
	mach_msg_type_number_t	cnt;
	kern_return_t		kr;
	/* NB: only one flavor supported here so far;
	 * VM_REGION_INFO_MAX is simply too big for our stack.  */
	natural_t		buff[ TASK_INFO_SMALL ];
	mach_msg_type_number_t	our_cnt = TASK_INFO_SMALL;

	t = port_name_to_task(task);
	if (t == TASK_NULL) {
		return(MACH_SEND_INTERRUPTED);
	}

	if (copyin((char*)task_info_cnt, (char*)&cnt, sizeof(mach_msg_type_number_t)))
		kr = KERN_INVALID_ADDRESS;
	else
		kr = task_info(t, flavor, (task_info_t)buff, &our_cnt);
	if (kr == KERN_SUCCESS && cnt >= our_cnt) {
		if (copyout((char *)&our_cnt, (char *)task_info_cnt,
					sizeof(mach_msg_type_number_t))
		    || copyout((char *)buff, (char *)task_info_out,
				our_cnt * sizeof(mach_msg_type_number_t))) {
			kr = KERN_INVALID_ADDRESS;
		}
	}

	/*
	 * If we can't copyout, or if our buffer was too small because
	 * somebody added a new flavor and didn't fix this routine,
	 * retry and let the kernel figure out what to do.
	 */
	if (kr == KERN_INVALID_ARGUMENT || cnt < our_cnt) {
		kr = MACH_SEND_INTERRUPTED; 
	}
	task_deallocate(t);

	return(kr);
}

kern_return_t
syscall_vm_protect(
	mach_port_t		target_map,
	vm_address_t		address,
	vm_size_t		size,
	boolean_t		set_maximum,
	vm_prot_t		new_protection)
{
	vm_map_t		map;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL) {
		return MACH_SEND_INTERRUPTED;
	}

	result = vm_protect(map, address, size, set_maximum, new_protection);
	vm_map_deallocate(map);

	return result;
}

kern_return_t
syscall_vm_allocate(
	mach_port_t		target_map,
	vm_offset_t		*address,
	vm_size_t		size,
	boolean_t		anywhere)
{
	vm_map_t		map;
	vm_offset_t		addr;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	if (copyin((char *)address, (char *)&addr, sizeof(vm_offset_t)))
		result = KERN_INVALID_ADDRESS;
	else
		result = vm_allocate(map, &addr, size, anywhere);
	if (result == KERN_SUCCESS)
		if (copyout((char*)&addr,(char*)address,sizeof(vm_offset_t))){
			(void) vm_deallocate(map, addr, size);
			result = KERN_INVALID_ADDRESS;
		}

	vm_map_deallocate(map);

	return result;
}

kern_return_t
syscall_vm_deallocate(
	mach_port_t		target_map,
	vm_offset_t		start,
	vm_size_t		size)
{
	vm_map_t		map;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	result = vm_deallocate(map, start, size);
	vm_map_deallocate(map);

	return result;
}

kern_return_t
syscall_vm_msync(
	mach_port_t		target_map,
	vm_address_t		address,
	vm_size_t		size,
	vm_sync_t		sync_flags)
{
	vm_map_t		map;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	result = vm_msync(map, address, size, sync_flags);
	vm_map_deallocate(map);

	return result;
}

kern_return_t
syscall_vm_wire(
	mach_port_t		host_port,
	mach_port_t		target_map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_prot_t		access)
{
	vm_map_t		map;
	host_t			host;
	kern_return_t		result;

	host = port_name_to_host(host_port);
	if (host == HOST_NULL)
		return MACH_SEND_INTERRUPTED;
	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL) {
		return MACH_SEND_INTERRUPTED;
	}

	result = vm_wire(host, map, start, size, access);
	vm_map_deallocate(map);

	return result;
}

kern_return_t
syscall_vm_machine_attribute(
	mach_port_t			target_map,
	vm_address_t			address,
	vm_size_t			size,
	vm_machine_attribute_t		attribute,
	vm_machine_attribute_val_t*	value)	/* IN/OUT */
{
	vm_map_t		map;
	kern_return_t		kr;
	vm_machine_attribute_val_t v;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL) {
		return MACH_SEND_INTERRUPTED;
	}

	if (copyin((char*) value, (char*) &v,
		   sizeof (vm_machine_attribute_val_t))) {
		kr = KERN_INVALID_ADDRESS;
	} else {
		kr = vm_machine_attribute(map, address, size, attribute, &v);
	}
	if (kr == KERN_SUCCESS) {
		if (copyout((char *) &v, (char *) value,
			    sizeof(vm_machine_attribute_val_t))) {
			kr = KERN_INVALID_ADDRESS;
		}
	}

	vm_map_deallocate(map);
	return kr;
}

kern_return_t
syscall_task_create(
	mach_port_t		parent_task,
        ledger_port_array_t	ledgers,
        mach_msg_type_number_t	num_ledgers,
	boolean_t		inherit_memory,
	mach_port_t		*child_task)		/* OUT */
{
	task_t		t, c;
	ipc_port_t	port;
	mach_port_t 	name;
	kern_return_t	result;

	t = port_name_to_task(parent_task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_create(t, ledgers, num_ledgers, inherit_memory, &c);
	if (result == KERN_SUCCESS) {
		port = (ipc_port_t) convert_task_to_port(c);
		/* always returns a name, even for non-success return codes */
		(void) ipc_kmsg_copyout_object(current_space(),
					       (ipc_object_t) port,
					       MACH_MSG_TYPE_PORT_SEND, &name);
		if (copyout((char *)&name, (char *)child_task,
						    sizeof(mach_port_t))) {
			(void) task_terminate(c);
			result = KERN_INVALID_ADDRESS;
		} /* XXX else should terminate child if conversion failed */
	}
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_kernel_task_create(
	mach_port_t	parent_task,
	vm_offset_t	map_base,
	vm_size_t	map_size,
	mach_port_t	*child_task)		/* OUT */
{
	task_t		t, c;
	ipc_port_t	port;
	mach_port_t 	name;
	kern_return_t	result;

	t = port_name_to_task(parent_task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = kernel_task_create(t, map_base, map_size, &c);
	if (result == KERN_SUCCESS) {
		port = (ipc_port_t) convert_task_to_port(c);
		/* always returns a name, even for non-success return codes */
		(void) ipc_kmsg_copyout_object(current_space(),
					       (ipc_object_t) port,
					       MACH_MSG_TYPE_PORT_SEND, &name);
		if (copyout((char *)&name, (char *)child_task,
							sizeof(mach_port_t))) {
			(void) task_terminate(c);
			result = KERN_INVALID_ADDRESS;
		} /* XXX else should terminate task if conversion failed */
	}
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_task_terminate(
	mach_port_t	task)
{
	task_t		t;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_terminate(t);
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_task_suspend(
	mach_port_t	task)
{
	task_t		t;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_suspend(t);
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_thread_create_running(
        mach_port_t         	parent_task,
        int                     flavor,
        thread_state_t          new_state,
        natural_t               new_state_count,
        mach_port_t             *child_thread)          /* OUT */
{
	natural_t	t_state[ THREAD_MACHINE_STATE_MAX ];
	task_t		task;
	thread_act_t	thr_act;
	ipc_port_t	port;
	mach_port_t	name;
	kern_return_t	result;

	task = port_name_to_task(parent_task);
	if (task == TASK_NULL) {
		return MACH_SEND_INTERRUPTED;
	}
	if (new_state_count > THREAD_MACHINE_STATE_MAX) {
		task_deallocate(task);
		return(KERN_INVALID_ARGUMENT);
	}
	if (copyin((char *)new_state, (char *)t_state,
					new_state_count*sizeof(natural_t))) {
		task_deallocate(task);
		return KERN_INVALID_ADDRESS;
	}

	result = thread_create_running(task, flavor, t_state, 
				       new_state_count, &thr_act);
	if (result == KERN_SUCCESS) {
		port = (ipc_port_t) convert_act_to_port(thr_act);
		/* always returns a name, even for non-success return codes */
                (void) ipc_kmsg_copyout_object(current_space(),
                                               (ipc_object_t) port,
                                               MACH_MSG_TYPE_PORT_SEND, &name);
                if (copyout((char *)&name, (char *)child_thread,
						    sizeof(mach_port_t))) {
			(void) thread_terminate(thr_act);
			result = KERN_INVALID_ADDRESS;
		}
        }
        task_deallocate(task);

        return result;
}

kern_return_t
syscall_task_set_special_port(
	mach_port_t	task,
	int		which_port,
	mach_port_t	port_name)
{
	task_t		t;
	ipc_port_t	port;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(port_name)) {
		result = ipc_object_copyin(current_space(), port_name,
					   MACH_MSG_TYPE_COPY_SEND,
					   (ipc_object_t *) &port);
		if (result != KERN_SUCCESS) {
			task_deallocate(t);
			return result;
		}
	} else
		port = (ipc_port_t) port_name;

	result = task_set_special_port(t, which_port, port);
	if ((result != KERN_SUCCESS) && IP_VALID(port))
		ipc_port_release_send(port);
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_thread_set_exception_ports(
	mach_port_t		thread,
	exception_mask_t	new_mask,
	mach_port_t	        new_port,
	exception_behavior_t    new_behavior,
	thread_state_flavor_t   new_flavor)
{
	thread_act_t	thr_act;
	ipc_port_t	port;
	kern_return_t	result;

	thr_act = port_name_to_act(thread);
	if (thr_act == THR_ACT_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(new_port)) {
		result = ipc_object_copyin(current_space(), new_port,
					   MACH_MSG_TYPE_COPY_SEND,
					   (ipc_object_t *) &port);
		if (result != KERN_SUCCESS) {
			act_deallocate(thr_act);
			return result;
		}
	} else {
		port = (ipc_port_t) new_port;
	}

	result = thread_set_exception_ports(thr_act, new_mask, port,
						new_behavior, new_flavor);
	if ((result != KERN_SUCCESS) && IP_VALID(port))
		ipc_port_release_send(port);
	act_deallocate(thr_act);

	return(result);
}

kern_return_t
syscall_task_set_exception_ports(
	mach_port_t		task,
	exception_mask_t	new_mask,
	mach_port_t		new_port,
	exception_behavior_t	new_behavior,
	thread_state_flavor_t	new_flavor)
{
	task_t		t;
	ipc_port_t	port;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL) {
		return MACH_SEND_INTERRUPTED;
	}

	if (MACH_PORT_VALID(new_port)) {
		result = ipc_object_copyin(current_space(), new_port,
					   MACH_MSG_TYPE_COPY_SEND,
					   (ipc_object_t *) &port);
		if (result != KERN_SUCCESS) {
			task_deallocate(t);
			return result;
		}
	} else {
		port = (ipc_port_t) new_port;
	}

	result = task_set_exception_ports(t, new_mask, port,
						new_behavior, new_flavor);
	if ((result != KERN_SUCCESS) && IP_VALID(port))
		ipc_port_release_send(port);
	task_deallocate(t);

	return(result);
}

kern_return_t
syscall_mach_port_allocate(
	mach_port_t		task,
	mach_port_right_t	right,
	mach_port_t		*namep)
{
	ipc_space_t		space;
	mach_port_t		name;
	kern_return_t		kr;
	mach_port_qos_t		qos = qos_template;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_allocate_full(space, right, SUBSYSTEM_NULL,
					&qos, &name);
	if (kr == KERN_SUCCESS) {
		if (copyout((char*)&name, (char*)namep, sizeof(mach_port_t))){
			(void) mach_port_destroy(space, name);
			kr = KERN_INVALID_ADDRESS;
		}
	}

	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_allocate_name(
	mach_port_t		task,
	mach_port_right_t	right,
	mach_port_t		name)
{
	ipc_space_t		space;
	kern_return_t		kr;
	mach_port_qos_t		qos = qos_template;

	qos.name = TRUE;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_allocate_full(space, right, SUBSYSTEM_NULL,
					&qos, &name);

	is_release(space);

	return (kr);
}

kern_return_t
syscall_mach_port_deallocate(
	mach_port_t	task,
	mach_port_t	name)
{
	ipc_space_t space;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_deallocate(space, name);
	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_rename(
	mach_port_t	task,
	mach_port_t	old_name,
	mach_port_t	new_name)
{
	ipc_space_t space;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL) {
		return MACH_SEND_INTERRUPTED;
	}

	kr = mach_port_rename(space, old_name, new_name);
	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_destroy(
	mach_port_t	task,
	mach_port_t	name)
{
	ipc_space_t space;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_destroy(space, name);
	is_release(space);

	return kr;
}


kern_return_t
syscall_mach_port_insert_right(
	mach_port_t		task,
	mach_port_t		name,
	mach_port_t		right,
	mach_msg_type_name_t	rightType)
{
	ipc_space_t space;
	ipc_object_t object;
	mach_msg_type_name_t newtype;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	if (!MACH_MSG_TYPE_PORT_ANY(rightType)) {
		is_release(space);
		return KERN_INVALID_VALUE;
	}

	if (MACH_PORT_VALID(right)) {
		kr = ipc_object_copyin(current_space(), right, rightType,
				       &object);
		if (kr != KERN_SUCCESS) {
			is_release(space);
			return kr;
		}
	} else
		object = (ipc_object_t) right;
	newtype = ipc_object_copyin_type(rightType);

	kr = mach_port_insert_right(space, name, (ipc_port_t) object, newtype);
	if ((kr != KERN_SUCCESS) && IO_VALID(object))
		ipc_object_destroy(object, newtype);
	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_move_member(
	mach_port_t	task,
	mach_port_t	member,
	mach_port_t	after)
{
	ipc_space_t	space;
	kern_return_t	error;

	if ((space = port_name_to_space(task)) == IS_NULL)
		return(MACH_SEND_INTERRUPTED);
	
	error = mach_port_move_member(space, member, after);

	is_release(space);
	return(error);
}

kern_return_t
syscall_thread_depress_abort(
	mach_port_t	A)
{
	thread_act_t	thr_act;
	kern_return_t	result;

	thr_act = port_name_to_act(A);
	if (thr_act == THR_ACT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = thread_depress_abort(thr_act);
	act_deallocate(thr_act);

	return result;
}

#define THREAD_INFO_SMALL 128	/* XXX ought to tune per platform */
kern_return_t
syscall_thread_info(
	mach_port_t		thread,
	thread_flavor_t		flavor,
	thread_info_t		thread_info_out,	/* ptr to OUT array */
	mach_msg_type_number_t	*thread_info_cnt)	/* IN/OUT */
{
	thread_act_t		act;
	mach_msg_type_number_t	cnt;
	kern_return_t		kr;
	natural_t		buff[ THREAD_INFO_SMALL ];
	mach_msg_type_number_t	our_cnt = THREAD_INFO_SMALL;

	act = port_name_to_act(thread);
	if (act == THR_ACT_NULL) {
		return(MACH_SEND_INTERRUPTED);
	}

	if (copyin((char*)thread_info_cnt, (char*)&cnt, sizeof(mach_msg_type_number_t)))
		kr = KERN_INVALID_ADDRESS;
	else
		kr = thread_info(act, flavor, (thread_info_t)buff, &our_cnt);
	if (kr == KERN_SUCCESS && cnt >= our_cnt) {
		if (copyout((char *)&our_cnt, (char *)thread_info_cnt,
					sizeof(mach_msg_type_number_t))
		    || copyout((char *)buff, (char *)thread_info_out,
				our_cnt * sizeof(mach_msg_type_number_t))) {
			kr = KERN_INVALID_ADDRESS;
		}
	}

	/*
	 * If we can't copyout, or if our buffer was too small because
	 * somebody added a new flavor and didn't fix this routine,
	 * retry and let the kernel figure out what to do.
	 */
	if (kr == KERN_INVALID_ARGUMENT || cnt < our_cnt) {
		kr = MACH_SEND_INTERRUPTED; 
	}
	act_deallocate(act);

	return(kr);
}

kern_return_t
syscall_thread_create(
	mach_port_t		parent_task,
	mach_port_t		*child_thread)		/* OUT */
{
	task_t		t;
	thread_act_t	c;
	ipc_port_t	port;
	mach_port_t 	name;
	kern_return_t	result;

	t = port_name_to_task(parent_task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = thread_create(t, &c);
	if (result == KERN_SUCCESS) {
		port = (ipc_port_t) convert_act_to_port(c);
		/* always returns a name, even for non-success return codes */
		(void) ipc_kmsg_copyout_object(current_space(),
					       (ipc_object_t) port,
					       MACH_MSG_TYPE_PORT_SEND, &name);
		if (copyout((char *)&name, (char *)child_thread,
						    sizeof(mach_port_t))) {
			(void) thread_terminate(c);
			result = KERN_INVALID_ADDRESS;
		} /* XXX else should terminate child if conversion failed */
	}
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_thread_terminate(
	mach_port_t	thread)
{
	thread_act_t	act;
	kern_return_t	result;

	act = port_name_to_act(thread);
	if (act == THR_ACT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = thread_terminate(act);
	act_deallocate(act);

	return result;
}

kern_return_t
syscall_thread_abort_safely(
	mach_port_t	thread)
{
	thread_act_t	act;
	kern_return_t	result;

	act = port_name_to_act(thread);
	if (act == THR_ACT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = thread_abort_safely(act);
	act_deallocate(act);

	return result;
}

kern_return_t
syscall_thread_suspend(
	mach_port_t	thread)
{
	thread_act_t	act;
	kern_return_t	result;

	act = port_name_to_act(thread);
	if (act == THR_ACT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = thread_suspend(act);
	act_deallocate(act);

	return result;
}

kern_return_t
syscall_thread_set_state(
	mach_port_t		thread,
	int			flavor,
	thread_state_t		state,
	mach_msg_type_number_t	state_count)
{
	thread_act_t	act;
	kern_return_t	result;
	natural_t	th_state[ THREAD_MACHINE_STATE_MAX ];

	act = port_name_to_act(thread);
	if (act == THR_ACT_NULL)
		return MACH_SEND_INTERRUPTED;

	if (state_count > THREAD_MACHINE_STATE_MAX) {
		act_deallocate(act);
		return(KERN_INVALID_ARGUMENT);
	}
	if (copyin((char *) state, (char *) th_state,
		   state_count * sizeof (natural_t))) {
		act_deallocate(act);
		return KERN_INVALID_ADDRESS;
	}

	result = thread_set_state(act, flavor,
				  (thread_state_t) th_state, state_count);
	act_deallocate(act);

	return result;
}

#define HOST_STATS_SMALL 128	/* XXX ought to tune per platform */
kern_return_t
syscall_host_statistics(
	mach_port_t		host_port,
	host_flavor_t		flavor,
	host_info_t		info,
	mach_msg_type_number_t	*count)	/* IN/OUT */
{
	host_t			host;
	natural_t		buff[HOST_STATS_SMALL];
	mach_msg_type_number_t	cnt, our_cnt;
	kern_return_t		kr;

	host = port_name_to_host(host_port);
	if (host == HOST_NULL) {
		return MACH_SEND_INTERRUPTED;
	}

	if (copyin((char *) count, (char *) &cnt,
		   sizeof (mach_msg_type_number_t))) {
		return KERN_INVALID_ADDRESS;
	}

	our_cnt = HOST_STATS_SMALL;
	kr = host_statistics(host, flavor, (host_info_t) buff, &our_cnt);

	if (kr == KERN_SUCCESS && cnt >= our_cnt) {
		if (copyout((char *) &our_cnt, (char *) count,
			    sizeof (mach_msg_type_number_t)) ||
		    copyout((char *) buff, (char *) info,
			    our_cnt * sizeof (natural_t))) {
			kr = KERN_INVALID_ADDRESS;
		}
	}
	/*
	 * If we can't copyout, or if our buffer was too small because
	 * somebody added a new flavor and didn't fix this routine,
	 * retry and let the kernel figure out what to do.
	 */
	if (kr == KERN_INVALID_ARGUMENT || cnt < our_cnt) {
		kr = MACH_SEND_INTERRUPTED; 
	}

	return(kr);
}

char *
mig_user_allocate(
	vm_size_t	size)
{
	return (char *)kalloc(size);
}

void
mig_user_deallocate(
	char		*data,
	vm_size_t	size)
{
	kfree((vm_offset_t)data, size);
}

/*
 * Device I/O syscalls
 */
kern_return_t
syscall_device_read(
	mach_port_t		device,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted,
	io_buf_ptr_t		*data_ptr,
	mach_msg_type_number_t	*data_count_ptr)
{
	device_t		d;
	io_buf_ptr_t		data;
	mach_msg_type_number_t	data_count;
	kern_return_t		result;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	result = ds_device_read_common(d,
				IP_NULL, 0,
				mode, recnum, bytes_wanted,
				(IO_READ | IO_SYNC),
				&data, &data_count);
	if (result == MIG_NO_REPLY)
		panic("syscall_device_read: caller did not do sync operation");

	device_deallocate(d);

	/*
	 * Copy out results on success.
	 */
	if (result == KERN_SUCCESS) {
		if (copyout((char *)&data, (char *)data_ptr, sizeof(*data_ptr)) ||
		    copyout((char *)&data_count, (char *)data_count_ptr, sizeof(*data_count_ptr)))
			result = KERN_INVALID_ADDRESS;
	}

	return result;
}

kern_return_t
syscall_device_read_overwrite(
	mach_port_t		device,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted,
	vm_address_t		buffer,
	mach_msg_type_number_t	*data_count_ptr)
{
	device_t		d;
	io_buf_ptr_t		data;
	mach_msg_type_number_t	data_count;
	kern_return_t		result;
	int			op;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	if (current_act()->kernel_loaded)
		op = IO_READ | IO_OVERWRITE | IO_SYNC | IO_KERNEL_BUF;
	else
		op = IO_READ | IO_OVERWRITE | IO_SYNC;

	data = (io_buf_ptr_t) buffer;

	result = ds_device_read_common(d, IP_NULL, 0,
				mode, recnum, bytes_wanted,
				op, &data, &data_count);

	assert(result != MIG_NO_REPLY);	

	device_deallocate(d);

	/*
	 * Copy out results on success.
	 */
	if (result == KERN_SUCCESS) {
		if (copyout((char *)&data_count, (char *)data_count_ptr, 
			    sizeof(*data_count_ptr)))
			result = KERN_INVALID_ADDRESS;
	}

	return result;
}

kern_return_t
syscall_device_read_request(
	mach_port_t		device,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted)
{
	device_t		d;
	io_buf_ptr_t		data;
	mach_msg_type_number_t	data_count;
	ipc_port_t		kern_reply_port;
	kern_return_t		result;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	result = KERN_SUCCESS;
	if (MACH_PORT_VALID(reply_port)) {
		result = ipc_object_copyin(current_space(), reply_port,
					   MACH_MSG_TYPE_MAKE_SEND_ONCE,
					   (ipc_object_t *) &kern_reply_port);
		if (result != KERN_SUCCESS) {
			device_deallocate(d);
			return result;
		}
	}
	else {
		kern_reply_port = IP_NULL;
	}

	if (result == KERN_SUCCESS) {
		result = ds_device_read_common(d,
				kern_reply_port, MACH_MSG_TYPE_MOVE_SEND_ONCE,
				mode, recnum, bytes_wanted,
				(IO_READ | IO_CALL),
				&data, &data_count);
		if (result == KERN_SUCCESS)
			panic("syscall_device_read_request: unable to handle total success");
		if (result == MIG_NO_REPLY)
			result = KERN_SUCCESS;
	}

	device_deallocate(d);

	if ((result != KERN_SUCCESS) && IP_VALID(kern_reply_port))
		ipc_port_release_sonce(kern_reply_port);

	return result;
}

kern_return_t
syscall_device_read_overwrite_request(
	mach_port_t		device,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted,
	vm_address_t		buffer)
{
	device_t		d;
	io_buf_ptr_t		data;
	mach_msg_type_number_t	data_count;
	ipc_port_t		kern_reply_port;
	kern_return_t		result;
	int			op;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	result = KERN_SUCCESS;
	if (MACH_PORT_VALID(reply_port)) {
		result = ipc_object_copyin(current_space(), reply_port,
					   MACH_MSG_TYPE_MAKE_SEND_ONCE,
					   (ipc_object_t *) &kern_reply_port);
		if (result != KERN_SUCCESS) {
			device_deallocate(d);
			return result;
		}
	}
	else {
		kern_reply_port = IP_NULL;
	}

	if (result == KERN_SUCCESS) {
		if (current_act()->kernel_loaded)
			op = IO_READ | IO_OVERWRITE | IO_CALL | IO_KERNEL_BUF;
		else
			op = IO_READ | IO_OVERWRITE | IO_CALL;

		data = (io_buf_ptr_t) buffer;

		result = ds_device_read_common(d,
				kern_reply_port, MACH_MSG_TYPE_MOVE_SEND_ONCE,
				mode, recnum, bytes_wanted,
				op, &data, &data_count);

		if (result == KERN_SUCCESS)
			panic("syscall_device_read_request: unable to handle total success");
		if (result == MIG_NO_REPLY)
			result = KERN_SUCCESS;
	}

	device_deallocate(d);

	if ((result != KERN_SUCCESS) && IP_VALID(kern_reply_port))
		ipc_port_release_sonce(kern_reply_port);

	return result;
}

kern_return_t
device_read_async(
	mach_port_t		device,
	mach_port_t		io_done_queue,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted)
{
	device_t		d;
	io_done_queue_t		q;
	io_buf_ptr_t		data;
	mach_msg_type_number_t	data_count;
	kern_return_t		result;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return KERN_INVALID_ARGUMENT;

	q = port_name_to_io_done_queue(io_done_queue);
	if (q == IO_DONE_QUEUE_NULL) {
		device_deallocate(d);
		return KERN_INVALID_ARGUMENT;
	}

	result = ds_device_read_common(d, (ipc_port_t) q,
				(mach_msg_type_name_t) reply_port,
				mode, recnum, bytes_wanted, IO_READ | IO_QUEUE,
				&data, &data_count);

	assert (result != KERN_SUCCESS);

	if (result == MIG_NO_REPLY)
		result = KERN_SUCCESS;

	device_deallocate(d);

	return result;
}

kern_return_t
device_read_async_inband(
	mach_port_t		device,
	mach_port_t		io_done_queue,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted)
{

	device_t		d;
	io_done_queue_t		q;
	kern_return_t		result;
	io_buf_ptr_t		data_junk;
	mach_msg_type_number_t	data_count_junk;

#ifdef	lint
	bytes_wanted++;
#endif

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return KERN_INVALID_ARGUMENT;

	q = port_name_to_io_done_queue(io_done_queue);
	if (q == IO_DONE_QUEUE_NULL) {
		device_deallocate(d);
		return KERN_INVALID_ARGUMENT;
	}

	result = ds_device_read_common(d, (ipc_port_t) q,
				(mach_msg_type_name_t) reply_port,
				mode, recnum, bytes_wanted, 
				(IO_READ | IO_QUEUE | IO_INBAND),
				&data_junk, &data_count_junk);

	assert (result != KERN_SUCCESS);

	if (result == MIG_NO_REPLY)
		result = KERN_SUCCESS;

	device_deallocate(d);

	return result;
}

kern_return_t
device_read_overwrite_async(
	mach_port_t		device,
	mach_port_t		io_done_queue,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted,
	vm_address_t		buffer)
{
	device_t		d;
	io_done_queue_t		q;
	io_buf_ptr_t		data;
	mach_msg_type_number_t	data_count;
	kern_return_t		result;
	int			op;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	q = port_name_to_io_done_queue(io_done_queue);
	if (q == IO_DONE_QUEUE_NULL) {
		device_deallocate(d);
		return KERN_INVALID_ARGUMENT;
	}

	if (current_act()->kernel_loaded)
		op = IO_READ | IO_OVERWRITE | IO_QUEUE | IO_KERNEL_BUF;
	else
		op = IO_READ | IO_OVERWRITE | IO_QUEUE;

	data = (io_buf_ptr_t) buffer;

	result = ds_device_read_common(d, (ipc_port_t) q,
				(mach_msg_type_name_t) reply_port,
				mode, recnum, bytes_wanted,
				op, &data, &data_count);

	assert(result != KERN_SUCCESS);

	if (result == MIG_NO_REPLY)
		result = KERN_SUCCESS;

	device_deallocate(d);

	return result;
}

kern_return_t
syscall_device_write(
	mach_port_t		device,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_ptr_t		data,
	mach_msg_type_number_t	data_count,
	io_buf_len_t		*bytes_written_ptr)
{
	device_t		d;
	io_buf_len_t		bytes_written;
	vm_map_copy_t		copy;
	kern_return_t		result;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	if (current_act()->kernel_loaded) {
		result = ds_device_write_common(d,
					 IP_NULL, 0,
					 mode, recnum,
					 data, data_count,
					 (IO_WRITE | IO_SYNC | IO_KERNEL_BUF),
					 &bytes_written);
		assert(result != MIG_NO_REPLY);
	}
	else {
		result = vm_map_copyin_page_list(current_map(),
		        	(vm_offset_t) data, data_count, VM_PROT_READ,
				&copy, FALSE);

		if (result == KERN_SUCCESS) {
			result = ds_device_write_common(d,
					 IP_NULL, 0,
					 mode, recnum,
					 (io_buf_ptr_t)copy, data_count,
					 (IO_WRITE | IO_SYNC),
					 &bytes_written);
			assert(result != MIG_NO_REPLY);
		}

	}
	device_deallocate(d);

	/*
	 * Copy out results on success.
	 */
	if (result == KERN_SUCCESS) {
		if (copyout((char *)&bytes_written, (char *)bytes_written_ptr, sizeof(*bytes_written_ptr)))
			result = KERN_INVALID_ADDRESS;
	}

	return result;
}

kern_return_t
syscall_device_write_request(
	mach_port_t		device,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_ptr_t		data,
	mach_msg_type_number_t	data_count)
{
	device_t		d;
	io_buf_len_t		bytes_written;
	ipc_port_t		kern_reply_port;
	vm_map_copy_t		copy;
	kern_return_t		result;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	result = KERN_SUCCESS;
	if (MACH_PORT_VALID(reply_port)) {
		result = ipc_object_copyin(current_space(), reply_port,
					   MACH_MSG_TYPE_MAKE_SEND_ONCE,
					   (ipc_object_t *) &kern_reply_port);

		if (result != KERN_SUCCESS) {
			device_deallocate(d);
			return result;
		}
	}
	else {
		kern_reply_port = IP_NULL;
	}

	if (current_act()->kernel_loaded) {
		result = ds_device_write_common(d,
				kern_reply_port, MACH_MSG_TYPE_MOVE_SEND_ONCE,
				mode, recnum,
				data, data_count,
				(IO_WRITE | IO_CALL | IO_KERNEL_BUF),
				&bytes_written);

		assert (result != KERN_SUCCESS);

		if (result == MIG_NO_REPLY)
			result = KERN_SUCCESS;
	}
	else {

		result = vm_map_copyin_page_list(current_map(),
		        	(vm_offset_t) data, data_count, VM_PROT_READ,
				&copy, FALSE);

		if (result == KERN_SUCCESS) {
			result = ds_device_write_common(d,
				kern_reply_port, MACH_MSG_TYPE_MOVE_SEND_ONCE,
				mode, recnum,
				(io_buf_ptr_t)copy, data_count,
				(IO_WRITE | IO_CALL),
				&bytes_written);

			assert (result != KERN_SUCCESS);

			if (result == MIG_NO_REPLY)
				result = KERN_SUCCESS;
		}
	}

	device_deallocate(d);

	if ((result != KERN_SUCCESS) && IP_VALID(kern_reply_port))
		ipc_port_release_sonce(kern_reply_port);

	return result;
}

kern_return_t
device_write_async(
	mach_port_t		device,
	mach_port_t		io_done_queue,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_ptr_t		data,
	mach_msg_type_number_t	data_count)
{
	device_t		d;
	io_done_queue_t		q;
	io_buf_len_t		bytes_written;
	vm_map_copy_t		copy;
	kern_return_t		result;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(io_done_queue)) {
		q = port_name_to_io_done_queue(io_done_queue);
		if (q == IO_DONE_QUEUE_NULL) {
			device_deallocate(d);
			return KERN_INVALID_ARGUMENT;
		}
	}
	else	q = IO_DONE_QUEUE_NULL;

	if (current_act()->kernel_loaded) {
		result = ds_device_write_common(d, (ipc_port_t) q,
				(mach_msg_type_name_t) reply_port,
				mode, recnum,
				data, data_count,
				(IO_WRITE | IO_QUEUE | IO_KERNEL_BUF),
				&bytes_written);

		assert (result != KERN_SUCCESS);

		if (result == MIG_NO_REPLY)
			result = KERN_SUCCESS;
	}
	else {

		result = vm_map_copyin_page_list(current_map(),
		        	(vm_offset_t) data, data_count, VM_PROT_READ,
				&copy, FALSE);

		if (result == KERN_SUCCESS) {
			result = ds_device_write_common(d, (ipc_port_t) q,
				(mach_msg_type_name_t) reply_port,
				mode, recnum,
				(io_buf_ptr_t)copy, data_count,
				(IO_WRITE | IO_QUEUE),
				&bytes_written);

			assert (result != KERN_SUCCESS);

			if (result == MIG_NO_REPLY)
				result = KERN_SUCCESS;
		}
	}

	device_deallocate(d);

	return result;
}

kern_return_t
device_write_async_inband(
	mach_port_t		device,
	mach_port_t		io_done_queue,
	mach_port_t		reply_port,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_ptr_t		data,
	mach_msg_type_number_t	data_count)
{
	device_t		d;
	io_done_queue_t		q;
	io_buf_len_t		bytes_written;
	kern_return_t		result;
	io_buf_ptr_inband_t	buf;
	mach_msg_type_number_t	size;

	d = port_name_to_device(device);
	if (d == DEVICE_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(io_done_queue)) {
		q = port_name_to_io_done_queue(io_done_queue);
		if (q == IO_DONE_QUEUE_NULL) {
			device_deallocate(d);
			return KERN_INVALID_ARGUMENT;
		}
	}
	else	q = IO_DONE_QUEUE_NULL;

	if (data_count > sizeof(io_buf_ptr_inband_t))
		size = sizeof(io_buf_ptr_inband_t);
	else
		size = data_count;
	if (copyin((char *) data, (char *) buf, size)) {
		io_done_queue_deallocate(q);	
		device_deallocate(d);
		return KERN_MEMORY_ERROR;
	}

	result = ds_device_write_common(d, (ipc_port_t) q,
					(mach_msg_type_name_t) reply_port,
					mode, recnum, buf, size,
					(IO_WRITE | IO_QUEUE | IO_INBAND),
					&bytes_written);

	if (result == MIG_NO_REPLY)
		result = KERN_SUCCESS;

	device_deallocate(d);

	return result;
}

kern_return_t
io_done_queue_wait(
	mach_port_t		io_done_queue,
	io_done_result_t	*result)
{
	io_done_queue_t		q;
	io_done_result_t	res;
	kern_return_t		ret;

	q = port_name_to_io_done_queue(io_done_queue);
	if (q == IO_DONE_QUEUE_NULL)
		return KERN_INVALID_ARGUMENT;

	ret = ds_io_done_queue_wait(q, &res, result);

	if (ret == KERN_SUCCESS) {
		if (copyout((char *) &res, (char *) result, 
			    sizeof(struct control_info))) {
			ret = KERN_INVALID_ADDRESS;
		}
	}

	io_done_queue_deallocate(q);

	return ret;
}

/*
 * XXX These constants need to be tuned per platform
 */
#define VM_OVERWRITE_SMALL 512

kern_return_t 
syscall_vm_read_overwrite(
	mach_port_t	target_map,
	vm_address_t	address,
	vm_size_t	size,
	vm_address_t	data,
	vm_size_t	*data_size)
{
	struct {
	    long	align;
	    char	buf[VM_OVERWRITE_SMALL];
	} inbuf;
	vm_map_t	map;
	vm_map_t	oldmap;
	kern_return_t	error = KERN_SUCCESS;
	vm_map_copy_t	copy;

	/*
	 * Translate the map and verify
	 */
	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	if (size <= VM_OVERWRITE_SMALL) {
		oldmap = vm_map_switch(map);
		if (copyin((char *)address, (char *)&inbuf, size)) {
			(void) vm_map_switch(oldmap);
			error = KERN_INVALID_ADDRESS;
		}
		else {
			(void) vm_map_switch(oldmap);
			if (copyout((char *)&inbuf, (char *)data, size)) {
				error = KERN_INVALID_ADDRESS;
			}
		}
	}
	else {
		if ((error = vm_map_copyin(map,
					address,
					size,
					FALSE,	/* src_destroy */
					&copy)) == KERN_SUCCESS) {
			if ((error = vm_map_copy_overwrite(
					current_act()->map,
 					data, 
					copy,
					FALSE)) == KERN_SUCCESS) {
			}
			else {
				vm_map_copy_discard(copy);
			}
		}
	}
	vm_map_deallocate(map);

	if (error == KERN_SUCCESS) {
		if (copyout((char *)&size, (char *)data_size, sizeof(*data_size))) {
			error = KERN_INVALID_ADDRESS;
		}
	}

	return(error);
}

kern_return_t 
syscall_vm_write(
	mach_port_t		target_map,
	vm_address_t		address,
	vm_offset_t		data,
	mach_msg_type_number_t	size)
{
	struct {
	    long	align;
	    char	buf[VM_OVERWRITE_SMALL];
	} inbuf;
	vm_map_t	map;
	vm_map_t	oldmap;
	kern_return_t	error = KERN_SUCCESS;
	vm_map_copy_t	copy;

	/*
	 * Translate the map and verify
	 */
	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	if (size <= VM_OVERWRITE_SMALL) {
		if (copyin((char *)data, (char *)&inbuf, size)) {
			error = KERN_INVALID_ADDRESS;
		}
		else {
			oldmap = vm_map_switch(map);
			if (copyout((char *)&inbuf, (char *)address, size)) {
				error = KERN_INVALID_ADDRESS;
			}
			(void) vm_map_switch(oldmap);
		}
	}
	else {
		if ((error = vm_map_copyin(
				 	current_act()->map,
					data,
					size,
					FALSE,	/* src_destroy */
					&copy)) == KERN_SUCCESS) {
			if ((error = vm_map_copy_overwrite(
					map,
 					address, 
					copy,
					FALSE)) == KERN_SUCCESS) {
			}
			else {
				vm_map_copy_discard(copy);
			}
		}
	}
	vm_map_deallocate(map);

	return(error);
}

kern_return_t
syscall_vm_behavior_set(
	mach_port_t	target_map,
	vm_offset_t	address,
	vm_size_t	size,
	vm_behavior_t	new_behavior)
{
	kern_return_t	error = KERN_SUCCESS;
	vm_map_t	map;

	/*
	 * Translate the map and verify 
	 */
	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	error = vm_behavior_set(map, address, size, new_behavior);

	vm_map_deallocate(map);
	return error;
}

kern_return_t
syscall_vm_remap(
	mach_port_t	target_map,
	vm_offset_t	*address,
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	mach_port_t	source_map,
	vm_offset_t	memory_address,
	boolean_t	copy,
	vm_prot_t	*cur_protection,
	vm_prot_t	*max_protection,
	vm_inherit_t	inheritance)
{
	kern_return_t	result;
	ipc_port_t	target_port, source_port;
	vm_offset_t	addr;
	vm_prot_t	cur_prot, max_prot;

	if (!MACH_PORT_VALID(target_map) || !MACH_PORT_VALID(source_map))
		return MACH_SEND_INTERRUPTED;

	result = ipc_object_copyin(current_space(), target_map,
				   MACH_MSG_TYPE_COPY_SEND,
				   (ipc_object_t *)&target_port);
	if (result != KERN_SUCCESS) 
		return result;
	if (!IP_VALID(target_port))
		return MACH_SEND_INTERRUPTED;

	result = ipc_object_copyin(current_space(), source_map,
				   MACH_MSG_TYPE_COPY_SEND,
				   (ipc_object_t *) &source_port);
	if (result != KERN_SUCCESS) {
		ipc_port_release_send(target_port);
		return result;
	}
	if (!IP_VALID(source_port)) {
		ipc_port_release_send(target_port);
		return MACH_SEND_INTERRUPTED;
	}

	if (copyin((char *)address, (char *)&addr, sizeof(addr)))
		result = KERN_INVALID_ADDRESS;
	else
		result = vm_remap(target_port, &addr, size, mask, anywhere,
				source_port, memory_address, copy,
				&cur_prot, &max_prot, inheritance);

	if (result == KERN_SUCCESS &&
		(copyout((char *)&addr, (char *)address, sizeof(addr))
	    || copyout((char *)&cur_prot, (char *)cur_protection,
			sizeof(cur_prot))
	    || copyout((char *)&max_prot, (char *)max_protection,
			sizeof(max_prot))))
		result = KERN_INVALID_ADDRESS;

	ipc_port_release_send(source_port);
	ipc_port_release_send(target_port);
	return result;
}

kern_return_t
syscall_vm_region(
	mach_port_t		target_map,
	vm_offset_t		*address,
	vm_size_t		*length,
	vm_region_flavor_t	flavor,
	vm_region_info_t	information,
	mach_msg_type_number_t	*count,
	mach_port_t		*object_name)
{
	kern_return_t	error;
	vm_map_t	tgt_map;
	vm_offset_t	addr;
	vm_size_t	len;
	int		info[VM_REGION_BASIC_INFO_COUNT];
	mach_msg_type_number_t	cnt;
	mach_port_t	name = MACH_PORT_NULL;

	if ((tgt_map = port_name_to_map(target_map)) == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	if (copyin((char *)address, (char *)&addr, sizeof(addr))) {
		vm_map_deallocate(tgt_map);
		return KERN_INVALID_ADDRESS;
	}

	if (copyin((char *)count, (char *)&cnt, sizeof(cnt))) {
		vm_map_deallocate(tgt_map);
		return KERN_INVALID_ADDRESS;
	}

	if (cnt > sizeof(info)/sizeof(int))
		cnt = sizeof(info)/sizeof(int);

	error = vm_region(tgt_map, &addr, &len, flavor, info, &cnt, 0);

	vm_map_deallocate(tgt_map);

	if (error != KERN_SUCCESS)
		return error;

	if (copyout((char *)&addr, (char *)address, sizeof(addr))
	    || copyout((char *)&len, (char *)length, sizeof(len))
	    || copyout((char *)info, (char *)information, sizeof(int)*cnt)
	    || copyout((char *)&cnt, (char *)count, sizeof(cnt))
	    || copyout((char *)&name, (char *)object_name, sizeof(mach_port_t)))
		return KERN_INVALID_ADDRESS;

	return KERN_SUCCESS;
}

kern_return_t
syscall_mach_port_allocate_subsystem(
	mach_port_t		task,
	mach_port_t		subsystem_name,
	mach_port_t		*namep)
{
	ipc_space_t		space;
	subsystem_t		subsys = SUBSYSTEM_NULL;
	mach_port_t		name;
	kern_return_t		kr;
	mach_port_qos_t		qos = qos_template;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return(MACH_SEND_INTERRUPTED);

	subsys = convert_port_to_subsystem((ipc_port_t)subsystem_name);
	if (subsys == SUBSYSTEM_NULL) {
		is_release(space);
		return(MACH_SEND_INTERRUPTED);
	}

	kr = mach_port_allocate_full(space, MACH_PORT_RIGHT_RECEIVE,
					subsys, &qos, &name);
	if (kr == KERN_SUCCESS) {
	    if (copyout((char*)&name, (char*)namep, sizeof(mach_port_t))){
		    (void) mach_port_destroy(space, name);
		    kr = KERN_INVALID_ADDRESS;
	    }
	}

	subsystem_deallocate(subsys);

	is_release(space);

	return(kr);
}

kern_return_t
syscall_mach_port_allocate_qos(
	mach_port_t		task,
	mach_port_right_t	right,
	mach_port_qos_t		*qosp,
	mach_port_t		*namep)
{
	ipc_space_t		space;
	mach_port_t		name;
	kern_return_t		kr;
	mach_port_qos_t		qos;

	if (copyin ((char *)qosp, (char *)&qos, sizeof (qos)))
		return (KERN_INVALID_ADDRESS);

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_allocate_full(space, right, SUBSYSTEM_NULL,
					&qos, &name);
	if (kr == KERN_SUCCESS) {
		if (copyout((char *)&name, (char *)namep, sizeof(name))) {
			(void) mach_port_destroy (space, name);
			kr = KERN_INVALID_ADDRESS;
		}
	}

	is_release(space);

	return (kr);
}

kern_return_t
syscall_mach_port_allocate_full(
	mach_port_t		task,
	mach_port_right_t	right,
	mach_port_t		subsystem_name,
	mach_port_qos_t		*qosp,
	mach_port_t		*namep)
{
	ipc_space_t		space;
	subsystem_t		subsys = SUBSYSTEM_NULL;
	mach_port_t		name;
	kern_return_t		kr;
	mach_port_qos_t		qos;

	if (copyin ((char *)qosp, (char *)&qos, sizeof (qos)))
		return (KERN_INVALID_ADDRESS);

	if (qos.name)
		if (copyin ((char *)namep, (char *)&name, sizeof (name)))
			return (KERN_INVALID_ADDRESS);

	space = port_name_to_space (task);
	if (space == IS_NULL)
		return (MACH_SEND_INTERRUPTED);

	if (subsystem_name != MACH_PORT_NULL) {
		subsys = port_name_to_subsystem(subsystem_name);
		if (subsys == SUBSYSTEM_NULL) {
			is_release (space);
			return (MACH_SEND_INTERRUPTED);
		}
	}

	kr = mach_port_allocate_full (space, right, subsys, &qos, &name);
	if (kr == KERN_SUCCESS && qos.name == FALSE) {
		if (copyout((char*)&name, (char*)namep, sizeof(name))){
			(void) mach_port_destroy(space, name);
			kr = KERN_INVALID_ADDRESS;
		}
	}

	if (subsystem_name != MACH_PORT_NULL)
		subsystem_deallocate (subsys);

	is_release (space);

	return (kr);
}

kern_return_t
syscall_mach_port_mod_refs(
	mach_port_t		task,
	mach_port_t		name,
	mach_port_right_t	right,
	mach_port_delta_t	delta)
{
	ipc_space_t space;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_mod_refs(space, name, right, delta);
	is_release(space);

	return kr;
}

kern_return_t
syscall_task_set_port_space(
	mach_port_t	task,
	int		table_entries)
{
	task_t		t;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_set_port_space(t, table_entries);
	
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_mach_port_request_notification(
	mach_port_t		task,
	mach_port_t		name,
	mach_msg_id_t		id,
	mach_port_mscount_t	sync,
	mach_port_t		notify_name,
	mach_msg_type_name_t	notify_type,
	mach_port_t		*previousp)	/* OUT */
{
	ipc_space_t space;
	kern_return_t result;
	ipc_port_t notify_port, our_prev_port;
	mach_port_t our_prev_name;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	if (!MACH_MSG_TYPE_PORT_ANY(notify_type)) {
		is_release(space);
		return KERN_INVALID_VALUE;
	}

	if (MACH_PORT_VALID(notify_name)) {
		result = ipc_object_copyin(current_space(),
					   notify_name, notify_type,
					   (ipc_object_t *) &notify_port);
		if (result != KERN_SUCCESS) {
			is_release(space);
			return result;
		}
	} else {
		notify_port = (ipc_port_t) notify_name;
	}

	result = mach_port_request_notification(space, name, id, sync,
					    notify_port, &our_prev_port);

	if (result == KERN_SUCCESS) {
		if (IP_VALID(our_prev_port)) {
			(void) ipc_kmsg_copyout_object(current_space(),
						       (ipc_object_t)
						       our_prev_port,
						       MACH_MSG_TYPE_PORT_SEND_ONCE,
						       &our_prev_name);
		} else {
			our_prev_name = (mach_port_t) our_prev_port;
		}
		if (copyout((char *) &our_prev_name, (char *) previousp,
			    sizeof (mach_port_t))) {
			result = KERN_INVALID_ADDRESS;
		}
	} else if (IP_VALID(notify_port)) {
		ipc_port_release_send(notify_port);
	}

	is_release(space);

	return result;
}

#define M_O_CHANGE_ATTRIBUTES_SMALL 128
kern_return_t
syscall_memory_object_change_attributes(
	mach_port_t		object_name,
	memory_object_flavor_t	flavor,
	memory_object_info_t	attributes,
	mach_msg_type_number_t	count,
	mach_port_t		reply_to_name)
{
	vm_object_t	object;
	kern_return_t	result;
	ipc_port_t	reply_to;
	natural_t	buf[M_O_CHANGE_ATTRIBUTES_SMALL];

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	if (count > M_O_CHANGE_ATTRIBUTES_SMALL) {
		vm_object_deallocate(object);
		return MACH_SEND_INTERRUPTED;
	}
	if (copyin((char *) attributes, (char *) buf,
		   count * sizeof (natural_t))) {
		vm_object_deallocate(object);
		return KERN_MEMORY_ERROR;
	}

	if (MACH_PORT_VALID(reply_to_name)) {
		result = ipc_object_copyin(current_space(),
					   reply_to_name,
					   MACH_MSG_TYPE_MAKE_SEND,
					   (ipc_object_t *) &reply_to);
		if (result != KERN_SUCCESS) {
			vm_object_deallocate(object);
			return result;
		}
	} else {
		reply_to = (ipc_port_t) reply_to_name;
	}

	result = memory_object_change_attributes(object, flavor,
						 (memory_object_info_t) buf,
						 count, reply_to,
						 MACH_MSG_TYPE_MOVE_SEND);

	if (result != KERN_SUCCESS) {
		if (IP_VALID(reply_to))
			ipc_port_release_send(reply_to);
	}
	return result;
}

kern_return_t
syscall_memory_object_data_error(
	mach_port_t	object_name,
	vm_offset_t	offset,
	vm_size_t	size,
	kern_return_t	error_value)
{
	vm_object_t	object;
	kern_return_t	result;

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = memory_object_data_error(object, offset, size, error_value);

	return result;
}

kern_return_t
syscall_memory_object_data_unavailable(
	mach_port_t	object_name,
	vm_offset_t	offset,
	vm_size_t	size)
{
	vm_object_t	object;
	kern_return_t	result;

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = memory_object_data_unavailable(object, offset, size);

	return result;
}

kern_return_t
syscall_memory_object_synchronize_completed(
	mach_port_t	object_name,
	vm_offset_t	offset,
	vm_size_t	size)
{
	vm_object_t	object;
	kern_return_t	result;

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	result = memory_object_synchronize_completed(object, offset, size);

	return result;
}

kern_return_t
syscall_memory_object_lock_request(
	mach_port_t		object_name,
	vm_offset_t		offset,
	vm_size_t		size,
	memory_object_return_t	should_return,
	boolean_t		should_flush,
	vm_prot_t		prot,
	mach_port_t		reply_to_name)
{
	vm_object_t	object;
	kern_return_t	result;
	ipc_port_t	reply_to;

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(reply_to_name)) {
		result = ipc_object_copyin(current_space(),
					   reply_to_name,
					   MACH_MSG_TYPE_MAKE_SEND,
					   (ipc_object_t *) &reply_to);
		if (result != KERN_SUCCESS) {
			vm_object_deallocate(object);
			return result;
		}
	} else {
		reply_to = (ipc_port_t) reply_to_name;
	}

	result = memory_object_lock_request(object, offset, size,
					    should_return, should_flush, prot,
					    reply_to, MACH_MSG_TYPE_MOVE_SEND);

	if (result != KERN_SUCCESS) {
		if (IP_VALID(reply_to))
			ipc_port_release_send(reply_to);
	}
	return result;
}

kern_return_t
syscall_memory_object_discard_reply(
	mach_port_t		object_name,
	vm_offset_t		requested_offset,
	vm_size_t		requested_size,
	vm_offset_t		discard_offset,
	vm_size_t		discard_size,
	memory_object_return_t	should_return,
	mach_port_t		reply_to_name)
{
	vm_object_t	object;
	kern_return_t	result;
	ipc_port_t	reply_to;

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(reply_to_name)) {
		result = ipc_object_copyin(current_space(),
					   reply_to_name,
					   MACH_MSG_TYPE_MAKE_SEND,
					   (ipc_object_t *) &reply_to);
		if (result != KERN_SUCCESS) {
			vm_object_deallocate(object);
			return result;
		}
	} else {
		reply_to = (ipc_port_t) reply_to_name;
	}

	result = memory_object_discard_reply(object,
					     requested_offset, requested_size,
					     discard_offset, discard_size,
					     should_return,
					     reply_to, MACH_MSG_TYPE_MOVE_SEND);

	if (result != KERN_SUCCESS) {
		if (IP_VALID(reply_to))
			ipc_port_release_send(reply_to);
	}
	return result;
}

kern_return_t
syscall_memory_object_data_supply(
	mach_port_t		object_name,
	vm_offset_t		offset,
	vm_offset_t		data,
	mach_msg_type_number_t	data_cnt,
	boolean_t		dataDealloc,
	vm_prot_t		lock_value,
	boolean_t		precious,
	mach_port_t		reply_to_name)
{
	vm_object_t	object;
	kern_return_t	result;
	ipc_port_t	reply_to;
	vm_map_copy_t	copy;

	object = port_name_to_vm_object(object_name);
	if (object == VM_OBJECT_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(reply_to_name)) {
		result = ipc_object_copyin(current_space(),
					   reply_to_name,
					   MACH_MSG_TYPE_MAKE_SEND,
					   (ipc_object_t *) &reply_to);
		if (result != KERN_SUCCESS) {
			vm_object_deallocate(object);
			return result;
		}
	} else {
		reply_to = (ipc_port_t) reply_to_name;
	}

	result = vm_map_copyin_page_list(current_act()->map,
					 data, data_cnt,
					 VM_MAP_COPYIN_OPT_NO_ZERO_FILL|
					 VM_MAP_COPYIN_OPT_STEAL_PAGES,
					 &copy, FALSE);
	if (result != KERN_SUCCESS) {
		if (IP_VALID(reply_to))
			ipc_port_release_send(reply_to);
		vm_object_deallocate(object);
		return result;
	}

	result = memory_object_data_supply(object, offset,
					   (vm_offset_t) copy, data_cnt,
					   lock_value, precious,
					   reply_to, MACH_MSG_TYPE_MOVE_SEND);

	if (result != KERN_SUCCESS) {
		vm_map_copy_discard(copy);
	} else {
		if (dataDealloc) {
			(void) vm_deallocate(current_map(), data, data_cnt);
		}
	}

	return result;
}
