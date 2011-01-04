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
 * Revision 2.6.2.6  92/06/24  18:02:09  jeffreyh
 * 	XMM framework simplification: delete m_kobj, k_mobj pointers.
 * 	  Delete xmm_kobj_link and rewrite xmm_obj_unlink.
 * 	  Delete reply->kobj_held and xmm_[km]_reply routines.
 * 	  xmm_obj_reference is now a macro in xmm_obj.h
 * 	[92/06/24            dlb]
 * 
 * Revision 2.6.2.5  92/02/21  14:34:51  jsb
 * 	Fixed merge botch.
 * 
 * Revision 2.6.2.4  92/02/21  11:25:25  jsb
 * 	Disassociate and deallocate port in convert_port_to_reply.
 * 	[92/02/18  17:33:30  jsb]
 * 
 * 	Added multiple init detection code to xmm_kobj_link.
 * 	Changed MACH_PORT_NULL uses to IP_NULL.
 * 	[92/02/18  08:45:28  jsb]
 * 
 * 	Changed reply->mobj to reply->kobj.
 * 	Added reference counting to xmm_reply_{allocate,deallocate}.
 * 	[92/02/16  18:25:51  jsb]
 * 
 * 	Do real reference counting (except for xmm_replies).
 * 	[92/02/16  14:11:32  jsb]
 * 
 * 	Moved invocation routines to norma/xmm_invoke.c.
 * 	Changed reply walking routines to use kobj, not mobj.
 * 	Preparation for better reference counting on xmm objs.
 * 	[92/02/09  12:53:36  jsb]
 * 
 * Revision 2.6.2.3  92/02/18  19:17:22  jeffreyh
 * 	Changed reply walking routines to use kobj, not mobj.
 * 	Added xmm_buffer_init to norma_vm_init.
 * 	[92/02/12            jsb]
 * 
 * Revision 2.6.2.2  92/01/21  21:53:31  jsb
 * 	Added xmm_reply_notify and (nonfunctional) xmm_reply_send_once.
 * 	[92/01/21  18:23:51  jsb]
 * 
 * 	De-linted. Added xmm_reply routines. Added functional forms of
 * 	invocation routines. Added xmm_{obj,reply}_print routines.
 * 	[92/01/20  17:13:30  jsb]
 * 
 * Revision 2.6.2.1  92/01/03  16:38:38  jsb
 * 	Corrected log.
 * 	[91/12/24  14:33:43  jsb]
 * 
 * Revision 2.6  91/12/10  13:26:21  jsb
 * 	Added better debugging in xmm_obj_deallocate.
 * 	[91/12/10  11:35:06  jsb]
 * 
 * Revision 2.5  91/11/14  16:52:32  rpd
 *	Added missing argument to bcopy.
 * 	[91/11/00            jsb]
 * 
 * Revision 2.4  91/08/03  18:19:37  jsb
 * 	Renamed xmm_init() to norma_vm_init().
 * 	[91/07/24  23:25:57  jsb]
 * 
 * Revision 2.3  91/07/01  08:25:56  jsb
 * 	Removed non-KERNEL code.
 * 	Replaced Xobj_allocate with xmm_obj_allocate.
 * 	Added xmm_obj_deallocate.
 * 	Use per-class zone for obj allocation.
 * 	[91/06/29  15:21:33  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:10  jsb
 * 	First checkin.
 * 	[91/06/17  10:58:38  jsb]
 * 
 */
/* CMU_ENDHIST */
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
 */
/*
 *	File:	norma/xmm.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Common xmm support routines.
 */

#include <kern/misc_protos.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <xmm/xmm_obj.h>
#include <xmm/xmm_methods.h>
#include <kern/counters.h>
#include <string.h>

xmm_reply_data_t xmm_empty_reply_data = {(long *) 0, XMM_EMPTY_REPLY};

#define	OBJ_SETQ(lhs_obj, rhs_obj)\
((rhs_obj)->refcount++, (int)((lhs_obj) = (rhs_obj)))

/*
 * If caller provides old_mobj, then he donates a reference.
 * Returns a reference for new_mobj to caller.
 */
void
xmm_obj_allocate(
	xmm_class_t	class,
	xmm_obj_t	old_mobj,
	xmm_obj_t	*new_mobj)
{
	xmm_obj_t mobj;

	if (class->c_zone == ZONE_NULL) {
		char *zone_name;

		zone_name = (char *)
		    kalloc((vm_size_t) (strlen(class->c_name) + 5));
		bcopy("xmm.", zone_name, 4);
		bcopy(class->c_name, zone_name + 4, strlen(class->c_name) + 1);
		class->c_zone = zinit(class->c_size, (vm_size_t) 512*1024,
				      class->c_size, zone_name);
	}
	mobj = (xmm_obj_t) zalloc(class->c_zone);
	bzero((char *) mobj, (vm_size_t) class->c_size);
	mobj->class = class;
	mobj->refcount = 1;
	mobj->k_kobj = XMM_OBJ_NULL;
	xmm_obj_lock_init(mobj);
	if (old_mobj) {
		/*
		 *	mobj->m_mobj pointer consumes caller's ref
		 *	on old_mobj.  Back pointer takes out a ref
		 *	on newly created mobj (will have 2 on return).
		 */
		mobj->m_mobj = old_mobj;  
		OBJ_SETQ(old_mobj->k_kobj, mobj);	   
	} else {
		mobj->m_mobj = XMM_OBJ_NULL;
	}
	*new_mobj = mobj;
}

/*
 * XXX
 * REPLACE THIS COMMENT
 *
 * Termination protocol:
 * Each layer terminates layer beneath it before deallocating.
 * Bottom layers disable outside upcalls before blocking.
 * (For example, xmm_user.c does kobject_set(NULL) before
 * calling memory_object_terminate.)
 */
void
xmm_obj_unlink(
	xmm_obj_t	mobj)
{
	xmm_obj_t	m_mobj; 

	xmm_obj_lock(mobj);
	if ((m_mobj = mobj->m_mobj) != XMM_OBJ_NULL) {
		assert(mobj == m_mobj->k_kobj);
		mobj->m_mobj = XMM_OBJ_NULL;
		xmm_obj_release(mobj);	/* consumes lock */

		xmm_obj_lock(m_mobj);
		m_mobj->k_kobj = XMM_OBJ_NULL;
		xmm_obj_release(m_mobj);/* consumes lock */
	} else {
		assert(mobj->refcount > 0);
		xmm_obj_unlock(mobj);
	}
}

/*
 * The lock held on call is consumed on return.
 */
void
xmm_obj_release(
	xmm_obj_t	obj)
{
	assert(xmm_obj_lock_held(obj));
	assert(obj->refcount > 0);
	if (--obj->refcount == 0) {
		assert(obj->m_mobj == XMM_OBJ_NULL);
		assert(obj->k_kobj == XMM_OBJ_NULL);
		obj->class->m_deallocate(obj);	/* consumes lock */
		zfree(obj->class->c_zone, (vm_offset_t) obj);
	} else
		xmm_obj_unlock(obj);
}


zone_t xmm_ipc_zone;

/*
 * Structure to serialize ipc requests in export and import layers.
 */
typedef struct xmm_ipc {
	queue_chain_t	chain;
	mach_port_seqno_t seqno;
	boolean_t	global;
	vm_offset_t	offset;
	vm_size_t	length;
} *xmm_ipc_t;

counter(mach_counter_t	c_xie_seen_wait = 0;)
counter(mach_counter_t	c_xie_seen_blocked = 0;)
counter(mach_counter_t	c_xie_conflict_wait = 0;)
counter(mach_counter_t	c_xie_calls = 0;)
counter(mach_counter_t	c_xix_calls = 0;)

/*
 *	Initialize synchronization state for a
 *	port newly entering XMM.  We assume that
 *	there have been no prior messages exchanged
 *	on this port; therefore, the current sequence
 *	number is zero.
 */
void
xmm_sync_init(
	xmm_sync_t	xsp)
{
	xsp->seqno_arrived = -1;
	xsp->seqno_completed = -1;
	xsp->flags = 0;
	queue_init(&xsp->current_ipc);
}

/*
 * Allocate and queue an ipc request associated to the current service routine.
 *
 * MP notes: Upon return, the mobj lock is held.
 */
void
xmm_ipc_enter(
	ipc_port_t	port,
	mach_port_seqno_t seqno,
	xmm_obj_t	mobj,
	xmm_sync_t	xsp,
	boolean_t	global,
	vm_offset_t	offset,
	vm_size_t	length)
{
	xmm_ipc_t	ipc;
	xmm_ipc_t	wipc;
	unsigned int	conflict;
	int		blocked;

	/*
	 *	Set up an ipc record for this request.  We will
	 *	need it later, while executing the request.
	 */
	assert(!global || (offset == 0 && length == 0));
	counter(++c_xie_calls);
	ipc = (xmm_ipc_t)zalloc(xmm_ipc_zone);
	assert(ipc);
	ipc->seqno = seqno;
	ipc->global = global;
	ipc->offset = offset;
	ipc->length = length;

	xmm_obj_lock(mobj);

	/*
	 *	Guarantee message ordering.  Wait for earlier
	 *	requests to arrive; it's possible for them to get
	 *	held up in IPC (e.g., blocking memory allocation
	 *	or remote message transfer) or to lose an MP race.
	 */
	assert((long) seqno > xsp->seqno_arrived);
	assert((long) seqno > xsp->seqno_completed);
	assert(xsp->seqno_arrived >= xsp->seqno_completed);
	blocked = 0;
	while ((long) seqno > xsp->seqno_arrived + 1) {
		counter(++c_xie_seen_wait);
		xsp->flags |= XS_SEEN_WAIT;
		assert_wait((event_t) xsp, FALSE);
		xmm_obj_unlock(mobj);
		thread_block((void (*)(void)) 0);
		xmm_obj_lock(mobj);
		if (blocked++ == 0) {
			counter(++c_xie_seen_blocked);
		}
	}
	assert((long) seqno == xsp->seqno_arrived + 1);

	/*
	 *	Decide whether this operation conflicts with ones
	 *	already in progress.  Two possibilities:  this
	 *	operation is global, or it overlaps with a current
	 *	request operating on (offset, length).
	 */
	conflict = ipc->global;
	if (!conflict && !queue_empty(&xsp->current_ipc)) {
		queue_iterate(&xsp->current_ipc, wipc, xmm_ipc_t, chain) {
			if (wipc->global ||
			    (wipc->offset <= offset &&
			     wipc->offset + wipc->length > offset) ||
			    (wipc->offset > offset &&
			     wipc->offset < offset + length)) {
				++conflict;
				break;
			}
		}
	}

	/*
	 *	If there's a conflict, block until all prior
	 *	operations complete.  There's an additional
	 *	possible optimization here:  we could recheck
	 *	for conflicts and possibly launch this operation
	 *	earlier than otherwise.
	 */
	if (conflict) {
		while ((long) seqno > xsp->seqno_completed + 1) {
			counter(++c_xie_conflict_wait);
			xsp->flags |= XS_CONFLICT_WAIT;
			assert_wait((event_t) xsp, FALSE);
			xmm_obj_unlock(mobj);
			thread_block((void (*)(void)) 0);
			xmm_obj_lock(mobj);
		}
		assert((long) seqno == xsp->seqno_completed + 1);
	}

	/*
	 *	Conflicts, if any, have been resolved by waiting
	 *	for prior operations to complete.  It's safe to
	 *	launch this operation now.  Record it in the
	 *	current_ipc queue for future conflict detection.
	 */
	assert((long) seqno == xsp->seqno_arrived + 1);
	xsp->seqno_arrived++;
	queue_enter(&xsp->current_ipc, ipc, xmm_ipc_t, chain);
	if (xsp->flags & XS_SEEN_WAIT) {
		xsp->flags &= ~XS_SEEN_WAIT;
		thread_wakeup((event_t) xsp);
	}
}


/*
 * Remove associated ipc request from the mobj list and wake up any
 *	waiting kserver thread.
 *
 * MP notes: Upon call, the mobj is locked; Upon return, the lock has been
 *	consumed.
 */
void
xmm_ipc_exit(
	xmm_obj_t		mobj,
	mach_port_seqno_t	seqno,
	xmm_sync_t		xsp)
{
	xmm_ipc_t		wipc;

	counter(++c_xix_calls);
	assert(xmm_obj_lock_held(mobj));
	assert(!queue_empty(&xsp->current_ipc));
	queue_iterate(&xsp->current_ipc, wipc, xmm_ipc_t, chain) {
		if (seqno != wipc->seqno)
			continue;
		queue_remove(&xsp->current_ipc, wipc, xmm_ipc_t, chain);
		xsp->seqno_completed++;
		assert(xsp->seqno_completed <= xsp->seqno_arrived);
		if (xsp->flags & XS_CONFLICT_WAIT) {
			xsp->flags &= ~XS_CONFLICT_WAIT;
			thread_wakeup((event_t) xsp);
		}
		xmm_obj_release(mobj);	/* consumes lock */
		zfree(xmm_ipc_zone, (vm_offset_t)wipc);
		return;
	}
	panic("xmm_ipc_exit");
}

void
norma_vm_init(void)
{
	xmm_ipc_t ipc;

	xmm_svm_init();
	xmm_user_init();
	xmm_split_init();
	xmm_server_init();

	xmm_ipc_zone = zinit(sizeof(struct xmm_ipc), (vm_size_t)512,
			     sizeof(struct xmm_ipc), "xmm.ipc");
}

#include <mach_kdb.h>
#if	MACH_KDB
#include <ddb/db_output.h>

#define	printf	kdbprintf

/*
 *	Routine:	xmm_obj_print
 *	Purpose:
 *		Pretty-print an xmm obj.
 */

void
xmm_obj_print(
	db_addr_t	addr)
{
	xmm_obj_t obj = (xmm_obj_t)addr;

	printf("xmm obj 0x%x\n", obj);

	indent += 2;

	iprintf("class=0x%x[%s]", obj->class, obj->class->c_name);
	printf(", refcount=%d", obj->refcount);

	printf(", m_mobj=0x%x, k_kobj=0x%x\n", obj->m_mobj, obj->k_kobj);

	indent -=2;
}

/*
 *	Routine:	xmm_reply_print
 *	Purpose:
 *		Pretty-print an xmm reply.
 */

void
xmm_reply_print(
	db_addr_t	addr)
{
	xmm_reply_t reply = (xmm_reply_t)addr;

	printf("xmm reply 0x%x\n", reply);

	indent += 2;

	iprintf("request=0x%x", reply->req);
	printf(", request type=%d[", reply->type);
	switch (reply->type) {
		case MACH_MSG_TYPE_PORT_SEND:
		printf("send");
		break;

		case MACH_MSG_TYPE_PORT_SEND_ONCE:
		printf("send_once");
		break;

		case XMM_SVM_REPLY:
		printf("svm");
		break;

		case XMM_SPLIT_REPLY:
		printf("split");
		break;

		case XMM_EMPTY_REPLY:
		printf("empty");
		break;

		case XMM_TEMPORARY_REPLY:
		printf("temporary");
		break;

		default:
		printf("???");
		break;
	}
	printf("]\n");

	indent -=2;
}

void
xmm_sync_debug(
	xmm_sync_t	xsp)
{
	indent += 2;
	iprintf("Seqnos:  arrived %d, completed %d\n",
		xsp->seqno_arrived, xsp->seqno_completed);
	iprintf("Flags:  0x%x\n", xsp->flags);
	iprintf("current_ipc {next=0x%x, prev=0x%x}\n",
		xsp->current_ipc.next,
		xsp->current_ipc.prev);
	indent -= 2;
}

#endif	/* MACH_KDB */
