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
 * Revision 2.4.2.4  92/06/24  18:02:55  jeffreyh
 * 		XMM Framework Cleanup:
 * 	Delete m_kobj, k_mobj fields, and add comment explaining the
 * 	m_mobj and k_kobj fields that remain.  Make reply_to a union
 * 	in xmm_reply structure.  xmm_obj_reference is now a macro here.
 * 	[92/06/24            dlb]
 * 
 * 	Deleted unused kobj_held field.
 * 	[92/06/24            sjs]
 * 
 * 	Change USE_OLD_PAGEOUT booleans to values for data_write,
 * 	data_return, and data_initialize.
 * 	[92/06/04            dlb]
 * 
 * Revision 2.4.2.3  92/03/28  10:13:10  jeffreyh
 * 	Changed data_write to data_write_return and deleted data_return method.
 * 	[92/03/20            sjs]
 * 
 * Revision 2.4.2.2  92/02/21  11:27:30  jsb
 * 	Changed mobj field in xmm_reply to kobj, and added kobj_held field.
 * 	Changed xmm_reply_allocate_send_once macro accordingly.
 * 	[92/02/16  18:23:36  jsb]
 * 
 * 	Added explicit name paramater to xmm_decl, since not all cpps
 * 	recoginize arguments inside quotes. Sigh.
 * 	[92/02/16  14:14:38  jsb]
 * 
 * 	Defined more robust xmm_decl.
 * 	[92/02/09  12:55:40  jsb]
 * 
 * Revision 2.4.2.1  92/01/21  21:54:33  jsb
 * 	Moved IKOT_XMM_* definitions to kern/ipc_kobject.h.
 * 	[92/01/21  18:21:45  jsb]
 * 
 * 	De-linted. Supports new (dlb) memory object routines.
 * 	Supports arbitrary reply ports to lock_request, etc.
 * 	Converted mach_port_t (and port_t) to ipc_port_t.
 * 	Added XMM_USE_MACROS conditional, off by default,
 * 	which toggles between macros and functions for invocation
 * 	routines. (Right now only the functions are provided.)
 * 	[92/01/20  17:27:35  jsb]
 * 
 * Revision 2.4  91/07/01  08:26:25  jsb
 * 	Removed malloc, free definitions.
 * 	Added xmm_decl macro.
 * 	Renamed Xobj_allocate to xmm_obj_allocate.
 * 	Added zone element to xmm_class structure.
 * 	[91/06/29  14:41:25  jsb]
 * 
 * Revision 2.3  91/06/18  20:53:00  jsb
 * 	Removed bogus include.
 * 	[91/06/18  19:06:29  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:28  jsb
 * 	First checkin.
 * 	[91/06/17  11:03:46  jsb]
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
 *	File:	norma/xmm_obj.h
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Common definitions for xmm system.
 */

#ifndef	_XMM_XMM_OBJ_H_
#define	_XMM_XMM_OBJ_H_

#include <mach_kdb.h>

#include <ipc/ipc_port.h>
#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/message.h>
#include <kern/zalloc.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/macro_help.h>
#include <kern/queue.h>
#if	MACH_KDB
#include <ddb/db_sym.h>
#endif	/* MACH_KDB */

typedef struct xmm_class	*xmm_class_t;
typedef struct xmm_obj		*xmm_obj_t;
typedef struct xmm_reply	*xmm_reply_t;

#define	XMM_CLASS_NULL		((xmm_class_t) 0)
#define	XMM_OBJ_NULL		((xmm_obj_t) 0)
#define	XMM_REPLY_NULL		((xmm_reply_t) 0)

#define	MOBJ			((struct mobj *) mobj)
#define	KOBJ			((struct kobj *) kobj)

/*
 *	The xmm_obj structure is the basis of the entire XMM system.
 *
 *	The m_mobj link is used for M_ invocations moving from the
 *	manager towards the kernel.  It points to the next xmm_obj
 *	in the direction of the kernel.
 *
 *	The k_kobj link is used for K_ invocations moving from the
 *	kernel towards the manager.  It points to the next xmm_obj
 *	in the direction of the manager.
 *
 *	These two links always form a doubly linked list.  This means:
 *	(1) if obj->m_mobj is not null, then obj == obj->m_mobj->k_kobj
 *	(2) if obj->k_kobj is not null, then obj == obj->k_kobj->m_mobj
 *
 */

struct xmm_obj {
	int		refcount;
	xmm_class_t	class;
	xmm_obj_t	m_mobj;
	xmm_obj_t	k_kobj;
	decl_mutex_data(, s_lock)
};

#define	xmm_obj_lock_held(xmm_obj)	(mutex_held(&(xmm_obj)->s_lock),1)
#define	xmm_obj_lock_init(xmm_obj)	mutex_init(&(xmm_obj)->s_lock, \
						   ETAP_NORMA_XMMOBJ)
#define	xmm_obj_lock_try(xmm_obj)	mutex_try(&(xmm_obj)->s_lock)
#define	xmm_obj_lock(xmm_obj)		mutex_lock(&(xmm_obj)->s_lock)
#define	xmm_obj_unlock(xmm_obj)		mutex_unlock(&(xmm_obj)->s_lock)

struct xmm_reply {
	long			*req;
	mach_msg_type_name_t	type;
};

typedef	struct xmm_reply	xmm_reply_data_t;

#define xmm_obj_reference(obj)	(obj)->refcount++
#define xmm_obj_release_quick(obj)	\
MACRO_BEGIN				\
	assert(xmm_obj_lock_held(obj));	\
	assert((obj)->refcount > 1);	\
	(obj)->refcount--;		\
MACRO_END

/*
 * Central repository of magic reply_to_types uses for non-port reply_tos.
 */
#define	XMM_SVM_REPLY		(MACH_MSG_TYPE_LAST + 1)
#define	XMM_SPLIT_REPLY		(MACH_MSG_TYPE_LAST + 2)
#define	XMM_EMPTY_REPLY		(MACH_MSG_TYPE_LAST + 3)
#define	XMM_TEMPORARY_REPLY	(MACH_MSG_TYPE_LAST + 4)
#define	XMM_CHANGE_REPLY	(MACH_MSG_TYPE_LAST + 5)

/*
 *      Macro to test for empty reply (occupies space but doesn't
 *      do anything).  Global declaration of an empty reply that
 *      any code module is free to use.
 */
#define xmm_reply_empty(reply)	((reply)->type == XMM_EMPTY_REPLY)

extern xmm_reply_data_t xmm_empty_reply_data;

#define	xmm_reply_set(reply,tp,val)	\
	(reply)->type = (tp), (reply)->req = (long *) (val)
/*
 * More meaningful parameter constants for memory_object calls.
 */
#define	OBJECT_READY_FALSE	FALSE
#define	MAY_CACHE_FALSE		FALSE
#define	PRECIOUS_FALSE		FALSE

#define	OBJECT_READY_TRUE	TRUE
#define	MAY_CACHE_TRUE		TRUE
#define	PRECIOUS_TRUE		TRUE

/*
 *	The following values are used to overload the length parameter
 *	of a lock_request as it goes from the split object to an svm
 *	object.  This conveys the information that a multi-page
 *	lock_request is starting/stopping, which is necessary to
 *	ensure the XMM object stack does not terminate during the
 *	lock_request call.
 */
#define	LOCK_REQ_START	(1<<(PAGE_SHIFT+1))
#define	LOCK_REQ_STOP	(1<<(PAGE_SHIFT+2))

/*
 * Misc
 */
#define	BOOL(b)		((b) ? "" : "!")

/*
 *	The xmm_sync structure enforces ordering guarantees
 *	as messages flow between export and import layers.
 *	The problem is that, while IPC guarantees to deliver
 *	messages to ports in the same order that a sending
 *	thread generated them, there are no ordering guarantees
 *	after the message is received.  It's up to XMM to maintain
 *	its own ordering.
 *
 *	seqno_arrived tracks requests as they arrive
 *	from IPC.  If a sequence number shows up that
 *	is greater than seqno_arrived+1, then the thread(s)
 *	that received messages seqno_arrived+1..seqno-1
 *	have blocked somewhere in IPC.  The request
 *	bearing seqno must be deferred until the earlier
 *	requests arrive.
 *
 *	seqno_completed tracks requests satisfied by XMM.
 *	This information is used in conjunction with
 *	the "global" information supplied to xmm_ipc_enter,
 *	and the current_ipc list, to determine when we can
 *	launch the current request.  Sometimes it is necessary
 *	to block, waiting for previous requests to complete.
 *	Note that, once launched, operations can still complete
 *	out of order, so seqno_completed does not necessarily
 *	match the sequence number of each request as it completes.
 */
typedef struct xmm_sync {
	long			seqno_arrived;
	long			seqno_completed;
	unsigned int		flags;
#define	XS_SEEN_WAIT		0x1
#define	XS_CONFLICT_WAIT	0x2
	queue_head_t		current_ipc;
} xmm_sync, *xmm_sync_t;

extern void		xmm_sync_init(
				xmm_sync_t	xsp);

#if	MACH_KDB
extern void		xmm_sync_debug(
				xmm_sync_t	xsp);
#endif	/* MACH_KDB */

extern void		xmm_obj_allocate(
				xmm_class_t	class,
				xmm_obj_t	old_mobj,
				xmm_obj_t	*new_mobj);

extern void		xmm_obj_unlink(
				xmm_obj_t	mobj);

extern void		xmm_obj_release(
				xmm_obj_t	obj);

extern void		xmm_ipc_enter(
				ipc_port_t	port,
				mach_port_seqno_t seqno,
				xmm_obj_t	mobj,
				xmm_sync_t	xsp,
				boolean_t	global,
				vm_offset_t	offset,
				vm_size_t	length);

extern void		xmm_ipc_exit(
				xmm_obj_t	mobj,
				mach_port_seqno_t seqno,
				xmm_sync_t	xsp);

extern void		norma_vm_init(void);

extern void		xmm_user_init(void);

extern void		xmm_split_init(void);

extern void		xmm_svm_init(void);

extern void		xmm_memory_object_init(
				vm_object_t	object);

extern void		xmm_server_caching_threadcall(void);

extern boolean_t	xmm_server_caching_timeout(void);

extern void		xmm_server_init(void);

#if	MACH_KDB
extern void		xmm_obj_print(
				db_addr_t	addr);

extern void		xmm_reply_print(
				db_addr_t	addr);

extern void		xmm_object_stack_print(
				xmm_obj_t 	mobj);
#endif	/* MACH_KDB */

extern void		xmm_user_create(
				ipc_port_t	memory_object,
				xmm_obj_t	*new_mobj);

extern void		xmm_svm_create(
				xmm_obj_t	old_mobj,
				ipc_port_t	memory_object,
				xmm_obj_t	*new_mobj);

extern void		xmm_ksvm_create(
				xmm_obj_t	mobj,
				xmm_obj_t	*new_kobj);

extern void		xmm_import_create(
				ipc_port_t	xmm_object,
				xmm_obj_t	*new_mobj);

extern void		xmm_split_create(
				xmm_obj_t	old_mobj,
				xmm_obj_t	*new_mobj);

extern void		xmm_import_create(
				ipc_port_t	xmm_object,
				xmm_obj_t	*new_mobj);

extern xmm_obj_t	xmm_kobj_lookup(
				ipc_port_t	memory_control);

extern void		norma_lazy_init(void);

extern kern_return_t	xmm_object_reference(
				ipc_port_t	memory_object);

extern void		xmm_object_release(
				ipc_port_t	memory_object);

extern kern_return_t	xmm_memory_manager_export(
				xmm_obj_t	mobj,
				ipc_port_t	memory_object);

extern ipc_port_t	xmm_object_by_memory_object(
				ipc_port_t	memory_object);

extern void		xmm_object_terminate(
				ipc_port_t	memory_object);

#endif	/* _XMM_XMM_OBJ_H_ */
