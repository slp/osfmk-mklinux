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
 * 
 * Revision 2.4.3.7.1.1  92/08/28  16:31:20  jeffreyh
 * 	Bounds check error_value of m_o_data_error to avoid conflicts
 * 	with the kernel return values.
 * 	[92/07/28            sjs]
 *
 * Revision 2.4.3.7  92/06/24  18:03:38  jeffreyh
 * 	Changed routines to meet new interfaces; changes for reply
 * 	structure change.
 * 	[92/06/18            sjs]
 * 
 * 	Add release logic to lock_completed.
 * 	[92/06/09            dlb]
 * 
 * 	Support termintion of uninitialized object.  This means
 * 	that the object was never used, so don't tell the pager.
 * 	Remove k_ prefix from memory_object_create.
 * 	[92/06/08            dlb]
 * 
 * 	Convert use_old_pageout to use_routine.  Add data_initialize case
 * 	to m_user_data_write_return.
 * 	[92/06/04            dlb]
 * 
 * Revision 2.4.3.6  92/05/27  01:01:20  jeffreyh
 * 	Add protection argument check to memory_object_lock_request,
 * 	[92/04/29            dlb]
 * 
 * Revision 2.4.3.5  92/03/28  10:13:41  jeffreyh
 * 	Change data_write to data_write_return and conditionally call
 * 	 memory_object_write or memory_object_data_return.  Added support
 * 	 fo m_o_change_attributes().
 * 	[92/03/20            sjs]
 * 
 * Revision 2.4.3.4  92/03/03  16:24:12  jeffreyh
 * 	Pick up fix from jsb to call K_SET_READY with MAY_CACHE_FALSE. This
 * 	fixes a memory leak.
 * 	[92/02/26  12:25:25  jeffreyh]
 * 
 * Revision 2.4.3.3  92/02/21  11:28:30  jsb
 * 	Copy send right to memory object, so that each kernel can release its
 * 	own send right to memory object.
 * 	[92/02/20  13:57:24  jsb]
 * 
 * 	Reference mobj on port to mobj conversion; release when done.
 * 	[92/02/20  10:54:28  jsb]
 * 
 * 	Removed initialized flag and corresponding code to detect multiple
 * 	initialization, since xmm_kobj_link now handles such detection.
 * 	[92/02/18  08:47:44  jsb]
 * 
 * 	Changed reply->mobj to reply->kobj.
 * 	[92/02/16  18:22:26  jsb]
 * 
 * 	Explicitly provide name parameter to xmm_decl macro.
 * 	Changes for reference counting termination protocol.
 * 	[92/02/16  15:54:47  jsb]
 * 
 * 	Removed is_internal_memory_object_call; xmm internal objects now
 * 	create their own xmm stacks and objects and thus will never be
 * 	seen here. Use new MEMORY_OBJECT_COPY_TEMPORARY strategy instead
 * 	of MEMORY_OBJECT_COPY_NONE for setting internal objects ready.
 * 	Removed ipc_kobject_set of memory_object; this was a hack for when
 * 	xmm_server.c stored a pointer to the svm mobj stack in the
 * 	memory_object kobject. We now use a separate port (the xmm object
 * 	port) for this association, and break that association elsewhere.
 * 	[92/02/11  11:22:23  jsb]
 * 
 * 	Remove vm_object_lookup_by_pager.
 * 	[92/02/10  09:41:36  jsb]
 * 
 * 	Use new xmm_decl, and new memory_object_name and deallocation protocol.
 * 	Let mig do automatic conversion of memory_control port into user obj.
 * 	Cleaned up memory_object_create support.
 * 	[92/02/09  14:01:13  jsb]
 * 
 * Revision 2.4.3.2  92/01/21  21:55:05  jsb
 * 	De-linted. Supports new (dlb) memory object routines.
 * 	Supports arbitrary reply ports to lock_request, etc.
 * 	Converted mach_port_t (and port_t) to ipc_port_t.
 * 	[92/01/20  17:48:19  jsb]
 * 
 * Revision 2.4.3.1  92/01/03  16:39:03  jsb
 * 	Picked up temporary fix to m_user_terminate from dlb.
 * 	[91/12/24  14:31:09  jsb]
 * 
 * Revision 2.4  91/08/28  11:16:30  jsb
 * 	Added definition for xxx_memory_object_lock_request, and temporary
 * 	stubs for data_supply, object_ready, and change_attributes.
 * 	[91/08/16  14:22:37  jsb]
 * 
 * 	Added check for internal memory objects to xmm_user_create.
 * 	[91/08/15  10:14:19  jsb]
 * 
 * Revision 2.3  91/07/01  08:26:46  jsb
 * 	Collect garbage. Support memory_object_create.
 * 	Disassociate kobj from memory_control before calling
 * 	memory_object_terminate to prevent upcalls on terminated kobj.
 * 	[91/06/29  15:51:50  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:48  jsb
 * 	Renamed xmm_vm_object_lookup.
 * 	[91/06/17  13:20:06  jsb]
 * 
 * 	First checkin.
 * 	[91/06/17  11:02:47  jsb]
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
 *	File:	norma/xmm_user.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Interface between memory managers and xmm system.
 */

#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <vm/memory_object.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <kern/lock.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#include <kern/xpr.h>
#include <mach/mach_server.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_user.h>
#include <xmm/xmm_obj.h>
#include <xmm/xmm_methods.h>
#include <xmm/xmm_user_rename.h>

/*
 * Since we ALWAYS have an SVM module above us,
 * we NEVER have more than one memory_control per memory_object.
 * Thus we can combine mobj and kobj.
 */

struct mobj {
	struct xmm_obj	obj;
	ipc_port_t	memory_object;
	ipc_port_t	memory_control;
	int		k_count;		/* # current K_ calls */
	queue_head_t	change_list;
	unsigned
		/* enum */	copy_strategy:3,
		/* boolean_t */	may_cache:1,
		/* boolean_t */	temporary:1,
		/* boolean_t */	invalidate:1,
		/* boolean_t */	initialized:1,
		/* boolean_t */	terminated:1,	/* pending termination */
		/* boolean_t */	released:1;	/* pending release */
};

#undef  KOBJ
#define KOBJ    ((struct mobj *) kobj)

zone_t		xmm_user_request_zone;

typedef struct request {
	queue_chain_t		chain;
	ipc_port_t		reply_to;
	mach_msg_type_name_t	reply_to_type;
	memory_object_flavor_t	flavor;
} *request_t;

#define	m_user_deallocate		m_interpose_deallocate
#define	m_user_freeze			m_invalid_freeze
#define	m_user_share			m_invalid_share
#define	m_user_declare_page		m_invalid_declare_page
#define	m_user_declare_pages		m_invalid_declare_pages
#define	m_user_caching			m_invalid_caching
#define	m_user_uncaching		m_invalid_uncaching

#define	k_user_data_unavailable		k_invalid_data_unavailable
#define	k_user_get_attributes		k_invalid_get_attributes
#define	k_user_lock_request		k_invalid_lock_request
#define	k_user_data_error		k_invalid_data_error
#define	k_user_set_ready		k_invalid_set_ready
#define	k_user_destroy			k_invalid_destroy
#define	k_user_data_supply		k_invalid_data_supply
#define	k_user_create_copy		k_invalid_create_copy
#define	k_user_uncaching_permitted	k_invalid_uncaching_permitted
#define	k_user_release_all		k_invalid_release_all
#define	k_user_synchronize_completed	k_invalid_synchronize_completed

xmm_decl(user, "user", sizeof(struct mobj));

#define	XMM_USER_ENTER(obj, OBJ)				\
MACRO_BEGIN							\
	if (obj == XMM_OBJ_NULL) {				\
		return KERN_INVALID_ARGUMENT;			\
	}							\
	xmm_obj_lock(obj);					\
	if (OBJ->terminated) {					\
		xmm_obj_release(obj);	/* consumes lock */	\
		return KERN_INVALID_ARGUMENT;			\
	}							\
	OBJ->k_count++;						\
MACRO_END

#define	XMM_USER_RETURN(obj, OBJ, error)			\
MACRO_BEGIN							\
	if (--OBJ->k_count == 0 && OBJ->released) {		\
		xmm_obj_unlock(obj);				\
		K_RELEASE_ALL(obj);				\
		xmm_obj_lock(obj);				\
		xmm_obj_release_quick(obj);			\
	}							\
	xmm_obj_release(obj);	/* consumes lock */		\
	return error;						\
MACRO_END

/*
 * Translate from memory_control to kobj. Take a reference.
 */
xmm_obj_t
xmm_kobj_lookup(
	ipc_port_t	memory_control)
{
	register xmm_obj_t kobj;

	if (memory_control == IP_NULL) {
		XPR(XPR_XMM,
		    "xmm_kobj_lookup:  memory_control 0x%x ==> XMM_OBJ_NULL\n",
		    (integer_t) memory_control, 0, 0, 0, 0);
		return XMM_OBJ_NULL;
	}
	ip_lock(memory_control);
	if (ip_kotype(memory_control) == IKOT_PAGING_REQUEST) {
		kobj = (xmm_obj_t) memory_control->ip_kobject;
		xmm_obj_lock(kobj);
		if (KOBJ->memory_control) {
			xmm_obj_reference(kobj);
			xmm_obj_unlock(kobj);
		} else {
			xmm_obj_unlock(kobj);
			kobj = XMM_OBJ_NULL;
		}
	} else {
		kobj = XMM_OBJ_NULL;
	}
	ip_unlock(memory_control);
	XPR(XPR_XMM,
	    "xmm_kobj_lookup:  memory_control 0x%x ==> 0x%x\n",
	    (integer_t) memory_control, (integer_t) kobj, 0, 0, 0);
	return kobj;
}

/*
 * We create our own memory_control port.  This is easier and
 * less confusing than each kernel allocating its own port.
 */
void
xmm_user_create(
	ipc_port_t	memory_object,
	xmm_obj_t	*new_mobj)
{
	ipc_port_t memory_control;
	xmm_obj_t mobj;

	/*
	 * Allocate request port.
	 */
	memory_control = ipc_port_alloc_kernel();
	if (memory_control == IP_NULL) {
		panic("xmm_user_create: memory_control");
	}
	IP_CLEAR_NMS(memory_control);

	/*
	 * Allocate and initialize mobj.
	 */
	xmm_obj_allocate(&user_class, XMM_OBJ_NULL, &mobj);
	MOBJ->memory_object = ipc_port_copy_send(memory_object);
	MOBJ->memory_control = memory_control;
	MOBJ->initialized = FALSE;
	MOBJ->terminated = FALSE;
	MOBJ->temporary = FALSE;
	MOBJ->may_cache = FALSE;
	MOBJ->invalidate = FALSE;
	MOBJ->copy_strategy = MEMORY_OBJECT_COPY_NONE;
	MOBJ->released = FALSE;
	MOBJ->k_count = 0;
	queue_init(&MOBJ->change_list);

	/*
	 * Grab one more send right, for use by xmm_object_terminate.
	 */
	(void) ipc_port_copy_send(memory_object);

	/*
	 * Grab a reference for mobj and associate it with memory_control port.
	 */
	xmm_obj_reference(mobj);
	ipc_kobject_set(memory_control, (ipc_kobject_t) mobj,
			IKOT_PAGING_REQUEST);
	*new_mobj = mobj;
	XPR(XPR_XMM,
	    "xmm_user_create:  memory_object 0x%x ==> new_mobj 0x%x\n",
	    (integer_t) memory_object, (integer_t) mobj, 0, 0, 0);
}

void
xmm_user_init(void)
{
	xmm_user_request_zone = zinit(sizeof(struct request), 512*1024,
				      sizeof(struct request),
				      "xmm.user.request");
}

kern_return_t
m_user_init(
	xmm_obj_t	mobj,
	vm_size_t	pagesize,
	boolean_t	internal,
	vm_size_t	size)
{
	vm_size_t	cluster_size;

#ifdef	lint
	(void) M_INIT(mobj, pagesize, internal, size);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_init:  mobj 0x%x pagesz 0x%x internal %d size 0x%x\n",
	    (integer_t) mobj, (integer_t) pagesize,
	    (integer_t) internal, (integer_t) size, 0);
	xmm_obj_lock(mobj);
	MOBJ->initialized = TRUE;

	assert(MOBJ->memory_object != IP_NULL);
	xmm_obj_unlock(mobj);
	if (internal) {
		/* acquire a naked send right for the default pager */
		ipc_port_t default_pager =
			memory_manager_default_reference(&cluster_size);

		/* consumes the naked send right for default_pager */
		(void) memory_object_create(default_pager,
					    MOBJ->memory_object, size,
					    MOBJ->memory_control,
					    PAGE_SIZE);

		xmm_obj_lock(mobj);
		/* call set_ready, since default pager won't */

		/*
		 *	In earlier versions of the system that used
		 *	paging flow control, the default pager was
		 *	assumed to ALWAYS do modwc.  This assumption
		 *	has been flipped on its head:  in a DIPC
		 *	system, a default pager will NEVER do modwc,
		 *	instead relying on DIPC for flow control.
		 *	Unfortunately, modwc is a paging back-channel
		 *	that can lead to deadlock under load.
		 */
		xmm_obj_unlock(mobj);
		return K_SET_READY(mobj, MAY_CACHE_FALSE,
				   MEMORY_OBJECT_COPY_SYMMETRIC,
				   PAGE_SIZE, XMM_REPLY_NULL,
				   /*TEMPORARY_*/TRUE, (char *) 0, 0);
	} else {
		(void) memory_object_init(MOBJ->memory_object,
					MOBJ->memory_control,
					PAGE_SIZE);
	}
	return KERN_SUCCESS;
}

/*
 * The object will be terminated in two stages.  The first M_TERMINATE
 * will have release==FALSE and will handle sending m_o_terminate to
 * the pager.  It will also set MOBJ->terminated to block any further
 * calls.  The second M_TERMINATE will have release==TRUE and it will
 * cause the destruction of the xmm stack and ports.
 */
kern_return_t
m_user_terminate(
	xmm_obj_t	mobj,
	boolean_t	release)
{
	kern_return_t kr;

#ifdef	lint
	(void) M_TERMINATE(mobj, release);
#endif	/* lint */

	XPR(XPR_XMM,
	    "m_user_terminate:  mobj 0x%x release %d\n",
	    (integer_t) mobj, (integer_t) release, 0, 0, 0);
	if (release) {
		/* object has already been terminated, just do the release */
		assert(MOBJ->terminated);
		assert(queue_empty(&MOBJ->change_list));
		xmm_object_terminate(MOBJ->memory_object);
		xmm_obj_lock(mobj);
		if (MOBJ->k_count) {
			MOBJ->released = TRUE;
			xmm_obj_unlock(mobj);
			return KERN_FAILURE;
		}
		xmm_obj_release(mobj); /* consumes lock */
		return KERN_SUCCESS;
	}

	ipc_kobject_set(MOBJ->memory_control, IKO_NULL, IKOT_NONE);
	xmm_obj_lock(mobj);
	assert(!MOBJ->terminated);
	MOBJ->terminated = TRUE;

	if (MOBJ->initialized) {
		xmm_obj_unlock(mobj);
		kr = memory_object_terminate(MOBJ->memory_object,
					     MOBJ->memory_control);
	} else {
		/*
		 *	Terminate without preceding init.
		 *	Pager never knew about this, so don't tell it now
		 *	Undo actions of m_user_create.
		 */
		xmm_obj_unlock(mobj);
		ipc_port_release_send(MOBJ->memory_object);
		ipc_port_dealloc_kernel(MOBJ->memory_control);
		kr = KERN_SUCCESS;
	}
	return kr;
}

kern_return_t
m_user_copy(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) M_COPY(mobj);
#endif	/* lint */
	assert(MOBJ->initialized);
	panic("m_user_copy");
	return KERN_SUCCESS;
}

kern_return_t
m_user_data_request(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
#ifdef	lint
	(void) M_DATA_REQUEST(mobj, offset, length, desired_access);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_data_request:  mobj 0x%x off 0x%x len 0x%x acc 0x%x\n",
	    (integer_t) mobj, (integer_t) offset,
	    (integer_t) length, (integer_t) desired_access, 0);
	assert(MOBJ->initialized);
	return memory_object_data_request(MOBJ->memory_object,
					  MOBJ->memory_control,
					  offset, length, desired_access);
}

kern_return_t
m_user_data_unlock(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
#ifdef	lint
	(void) M_DATA_UNLOCK(mobj, offset, length, desired_access);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_data_unlock:  mobj 0x%x off 0x%x len 0x%x acc 0x%x\n",
	    (integer_t) mobj, (integer_t) offset,
	    (integer_t) length, (integer_t) desired_access, 0);
	assert(MOBJ->initialized);
	return memory_object_data_unlock(MOBJ->memory_object,
					 MOBJ->memory_control,
					 offset, length, desired_access);
}

kern_return_t
m_user_data_return(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	boolean_t	dirty,
	boolean_t	kernel_copy)
{
#ifdef	lint
	(void) M_DATA_RETURN(mobj, offset, data, length, dirty, kernel_copy);
#endif	/* lint */

	XPR(XPR_XMM,
	    "m_user_data_return:  mobj 0x%x off 0x%x data 0x%x...\n",
	    (integer_t) mobj, (integer_t) offset, (integer_t) data, 0, 0);
	XPR(XPR_XMM,
	    "...len 0x%x dirty %d kernel_copy %d\n",
	    (integer_t) length, (integer_t) dirty, (integer_t) kernel_copy,0,0);
#if	MACH_ASSERT
	/*
	 * Verify that pages given to the default pager are non-pageable.
	 */
	if (MOBJ->memory_object->ip_receiver == default_pager_space &&
	    (ip_kotype(MOBJ->memory_object) == IKOT_PAGER_TERMINATING ||
	     ip_kotype(MOBJ->memory_object) == IKOT_PAGER) && 
	    ((vm_object_t)MOBJ->memory_object->ip_kobject)->internal) {
		if (data->type == VM_MAP_COPY_PAGE_LIST) {
			assert(data->cpy_npages == 1);
			assert((!data->cpy_page_list[0]->inactive &&
				!data->cpy_page_list[0]->active) ||
			       data->cpy_page_list[0]->gobbled);
		} else {
			vm_page_t p;
			assert(data->type == VM_MAP_COPY_OBJECT);
			assert(data->cpy_object->resident_page_count == 1);
			queue_iterate(&data->cpy_object->memq,
				      p, vm_page_t, listq) {
				assert(p->wire_count);
			}
		}
	}
#endif	/* MACH_ASSERT */
	xmm_obj_lock(mobj);
	assert(MOBJ->initialized);
	xmm_obj_unlock(mobj);
	return memory_object_data_return(MOBJ->memory_object,
					 MOBJ->memory_control,
					 offset, (vm_offset_t)data,
					 length, dirty, kernel_copy);
}

kern_return_t
m_user_lock_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	xmm_reply_t	reply)
{
	kern_return_t kr;

#ifdef	lint
	(void) M_LOCK_COMPLETED(mobj, offset, length, reply);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_lock_completed:  mobj 0x%x off 0x%x len 0x%x reply 0x%x\n",
	    (integer_t) mobj, (integer_t) offset,
	    (integer_t) length, (integer_t) reply, 0);
	assert(MOBJ->initialized);
	assert(mobj->class == &user_class);

	kr = memory_object_lock_completed((ipc_port_t)reply->req,
					  reply->type,
					  MOBJ->memory_control,
					  offset, length);
	return kr;
}

kern_return_t
m_user_supply_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	result,
	vm_offset_t	error_offset,
	xmm_reply_t	reply)
{
	kern_return_t kr;

#ifdef	lint
	(void) M_SUPPLY_COMPLETED(mobj, offset, length, result, error_offset,
				  reply);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_supply_completed:  mobj 0x%x off 0x%x len 0x%x...\n",
	    (integer_t) mobj, (integer_t) offset, (integer_t) length, 0, 0);
	XPR(XPR_XMM,
	    "...result 0x%x error_offset 0x%x reply 0x%x\n",
	    (integer_t) result, (integer_t) error_offset,(integer_t) reply,0,0);

	assert(MOBJ->initialized);
	assert(mobj->class == &user_class);
	kr = memory_object_supply_completed((ipc_port_t)reply->req,
					    reply->type,
					    MOBJ->memory_control,
					    offset, length, result,
					    error_offset);
	return kr;
}

kern_return_t
m_user_change_completed(
	xmm_obj_t			mobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	xmm_reply_t			reply)
{
	request_t r;
	kern_return_t kr;

#ifdef	lint
	(void) M_CHANGE_COMPLETED(mobj, may_cache, copy_strategy, reply);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_change_completed:  mobj 0x%x cache 0x%x strategy 0x%x...\n",
	    (integer_t) mobj, (integer_t) may_cache,
	    (integer_t) copy_strategy, 0, 0);
	XPR(XPR_XMM,
	    "...reply 0x%x...\n", reply, 0, 0, 0, 0);
	assert(MOBJ->initialized);
	assert(mobj->class == &user_class);
	assert(reply->type == XMM_CHANGE_REPLY);

	/*
	 * Retrieve request pointer from reply.
	 */
	r = (request_t)reply->req;
	assert (r != (request_t)0);

	/*
	 * Look for request structure to dequeue it.
	 */
	xmm_obj_lock(mobj);
	assert(!queue_end(&MOBJ->change_list, (queue_entry_t)r));
#if	MACH_ASSERT
	{
		request_t rr;
		queue_iterate(&MOBJ->change_list, rr, request_t, chain) {
			if (r == rr)
				break;
		}
		assert(!queue_end(&MOBJ->change_list, (queue_entry_t)rr));
	}
#endif	/* MACH_ASSERT */
	queue_remove(&MOBJ->change_list, r, request_t, chain);
	xmm_obj_unlock(mobj);

	if (IP_VALID(r->reply_to)) {
			kr = memory_object_change_completed(
					r->reply_to, r->reply_to_type,
					(MOBJ->terminated ? IP_NULL :
					 MOBJ->memory_control), r->flavor);
	}

	zfree(xmm_user_request_zone, (vm_offset_t)r);
	return kr;
}

kern_return_t
m_user_synchronize(
	xmm_obj_t 	mobj,
	vm_offset_t	offset,
	vm_offset_t	length,
	vm_sync_t	sync_flags)
{
	kern_return_t kr;

#ifdef	lint
	(void) M_SYNCHRONIZE(mobj,  offset, length, sync_flags);
#endif	/* lint */
	XPR(XPR_XMM,
	    "m_user_synchronize:  mobj 0x%x off 0x%x len 0x%x flags 0x%x\n",
	    (integer_t) mobj, (integer_t) offset, (integer_t) length,
	    (integer_t) sync_flags, 0);
	assert(MOBJ->initialized);
	assert(mobj->class == &user_class);
	kr = memory_object_synchronize(MOBJ->memory_object,
				       MOBJ->memory_control,
				       offset, length, sync_flags);
	return kr;
}

#if	MACH_KDB
#include <ddb/db_output.h>
#include <ddb/db_print.h>

/*ARGSUSED*/
void
m_user_debug(
	xmm_obj_t	mobj,
	int		flags)
{
	db_addr_t task;
	int task_id;

	xmm_obj_print((db_addr_t)mobj);
	indent += 2;
	iprintf("mem_object=0x%x, mem_control=0x%x\n",
		MOBJ->memory_object,
		MOBJ->memory_control);
	iprintf("%sinitialized, %sinvalidate\n",
		BOOL(MOBJ->initialized),
		BOOL(MOBJ->invalidate));
	iprintf("copy_strategy=%d, %smay_cache, %stemporary\n",
		MOBJ->copy_strategy,
		BOOL(MOBJ->may_cache),
		BOOL(MOBJ->temporary));
	iprintf("k_count=%d, %sterminated, %sreleased\n",
		MOBJ->k_count,
		BOOL(MOBJ->terminated),
		BOOL(MOBJ->released));
	iprintf("change_list {next=0x%x, prev=0x%x}\n",
		MOBJ->change_list.next,
		MOBJ->change_list.prev);
	indent -= 2;

	if (!ip_active(MOBJ->memory_object))
		task = (db_addr_t)0;
	else if (MOBJ->memory_object->ip_receiver_name != MACH_PORT_NULL)
		task = db_task_from_space(MOBJ->memory_object->ip_receiver,
					  &task_id);
	else if (MOBJ->memory_object->ip_destination != MACH_PORT_NULL)
		task = db_task_from_space(MOBJ->memory_object->ip_destination->
					  ip_receiver, &task_id);
	else
		task = (db_addr_t)0;
	if (IP_IS_REMOTE(MOBJ->memory_object)) {
		printf("---------- Pager port 0x%x is on node %d ----------\n",
		       MOBJ->memory_object, IP_PORT_NODE(MOBJ->memory_object));
	} else if (task)
		printf("---------- Pager is in task%d at 0x%x ----------\n",
		       task_id, task);
	else
		printf("---------- Pager is in an unknown task ----------\n");
}
#endif	/* MACH_KDB */

kern_return_t
memory_object_data_unavailable(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
	kern_return_t kr;

	XPR(XPR_XMM,
	    "memory_object_data_unavailable:  kobj 0x%x off 0x%x len 0x%x\n",
	    (integer_t) kobj, (integer_t) offset, (integer_t) length, 0, 0);
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	kr = K_DATA_UNAVAILABLE(kobj, offset, length);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_get_attributes(
	xmm_obj_t			kobj,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			*count)
{
	kern_return_t kr;

	XPR(XPR_XMM,
	    "memory_object_get_attributes:  kobj 0x%x flavor 0x%x\n",
	    (integer_t) kobj, (integer_t) flavor, 0, 0, 0);
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	kr = K_GET_ATTRIBUTES(kobj, flavor, attributes, count);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_lock_request(
	xmm_obj_t		kobj,
	vm_offset_t		offset,
	vm_size_t		length,
	int			should_return,
	boolean_t		should_flush,
	vm_prot_t		prot,
	ipc_port_t		reply_to,
	mach_msg_type_name_t	reply_to_type)
{
	xmm_reply_data_t reply_data;
	xmm_reply_t reply = XMM_REPLY_NULL;
	kern_return_t kr;

	XPR(XPR_XMM,
	    "memory_object_lock_request:  kobj 0x%x off 0x%x len 0x%x...\n",
	    (integer_t) kobj, (integer_t) offset, (integer_t) length, 0, 0);
	XPR(XPR_XMM,
	    "...should_ret %d should_flsh %d prot 0x%x reply_to rt_type 0x%x\n",
	    (integer_t) should_return, (integer_t) should_flush,
	    (integer_t) prot, (integer_t) reply_to, (integer_t) reply_to_type);
	if ((prot & ~VM_PROT_ALL) != 0 && prot != VM_PROT_NO_CHANGE) {
		return KERN_INVALID_ARGUMENT;
	}
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	length = round_page(length);
	if (reply_to != IP_NULL) {
		xmm_reply_set(&reply_data, reply_to_type, reply_to);
		reply = &reply_data;
	}
	kr = K_LOCK_REQUEST(kobj, offset, length, should_return, should_flush,
			    prot, reply);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_data_error(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	error_value)
{
	kern_return_t kr;

	XPR(XPR_XMM,
	    "memory_object_data_error:  kobj 0x%x off 0x%x len 0x%x err 0x%x\n",
	    (integer_t) kobj, (integer_t) offset, (integer_t) length,
	    (integer_t) error_value, 0);
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	if (error_value <= KERN_RETURN_MAX) {
		error_value = KERN_MEMORY_ERROR;
	}
	kr = K_DATA_ERROR(kobj, offset, length, error_value);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_change_attributes(
	xmm_obj_t			kobj,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			count,
	ipc_port_t			reply_to,
	mach_msg_type_name_t		reply_to_type)
{
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
	vm_size_t cluster_size;
	boolean_t invalidate;
	boolean_t temporary;
	kern_return_t kr;
	xmm_reply_data_t reply_data;
	xmm_reply_t reply;
	request_t r;

	XPR(XPR_XMM,
	    "memory_object_change_attributes:  kobj 0x%x flavor 0x%x\n",
	    (integer_t) kobj, (integer_t) flavor, 0, 0, 0);
	if (reply_to != IP_NULL) {
		r = (request_t) zalloc(xmm_user_request_zone);
		r->reply_to = reply_to;
		r->reply_to_type = reply_to_type;
		r->flavor = flavor;
		xmm_reply_set(&reply_data, XMM_CHANGE_REPLY, r);
		reply = &reply_data;
	} else
		reply = XMM_REPLY_NULL;

	XMM_USER_ENTER(kobj, KOBJ);

	switch (flavor) {
	case MEMORY_OBJECT_BEHAVIOR_INFO:
		if (count < MEMORY_OBJECT_BEHAVE_INFO_COUNT)
			kr = KERN_INVALID_ARGUMENT;
		else {
			memory_object_behave_info_t attr =
				(memory_object_behave_info_t)attributes;

			may_cache = KOBJ->may_cache;
			copy_strategy = attr->copy_strategy;
			cluster_size = page_size;
			temporary = attr->temporary;
			invalidate = attr->invalidate;
			kr = KERN_SUCCESS;
		}
		break;

	case MEMORY_OBJECT_PERFORMANCE_INFO:
		if (count < MEMORY_OBJECT_PERF_INFO_COUNT)
			kr = KERN_INVALID_ARGUMENT;
		else {
			memory_object_perf_info_t attr =
				(memory_object_perf_info_t)attributes;

			may_cache = attr->may_cache;
			copy_strategy = KOBJ->copy_strategy;
			assert(attr->cluster_size == PAGE_SIZE);
			cluster_size = attr->cluster_size;
			temporary = KOBJ->temporary;
			invalidate = KOBJ->invalidate;
			kr = KERN_SUCCESS;
		}
		break;

	case OLD_MEMORY_OBJECT_ATTRIBUTE_INFO:
		if (count < OLD_MEMORY_OBJECT_ATTR_INFO_COUNT)
			kr = KERN_INVALID_ARGUMENT;
		else {
			old_memory_object_attr_info_t attr =
				(old_memory_object_attr_info_t)attributes;

			may_cache = attr->may_cache;
			copy_strategy = attr->copy_strategy;
			cluster_size = page_size;
			temporary = KOBJ->temporary;
			invalidate = KOBJ->invalidate;
			kr = KERN_SUCCESS;
		}
		break;

	case MEMORY_OBJECT_ATTRIBUTE_INFO:
		if (count < MEMORY_OBJECT_ATTR_INFO_COUNT)
			kr = KERN_INVALID_ARGUMENT;
		else {
			memory_object_attr_info_t attr =
				(memory_object_attr_info_t)attributes;

			may_cache = attr->may_cache_object;
			copy_strategy = attr->copy_strategy;
			cluster_size = page_size;		/* XXX */
			temporary = KOBJ->temporary;
			invalidate = KOBJ->invalidate;
			kr = KERN_SUCCESS;
		}
		break;

	case MEMORY_OBJECT_NORMA_INFO:
		if (count < MEMORY_OBJECT_NORMA_INFO_COUNT)
			kr = KERN_INVALID_ARGUMENT;
		else {
			memory_object_norma_info_t attr =
				(memory_object_norma_info_t)attributes;

			may_cache = attr->may_cache;
			copy_strategy = attr->copy_strategy;
			assert(attr->cluster_size == PAGE_SIZE);
			cluster_size = attr->cluster_size;
			temporary = KOBJ->temporary;
			invalidate = KOBJ->invalidate;
			kr = KERN_SUCCESS;
		}
		break;

	default:
		kr = KERN_INVALID_ARGUMENT;
		break;
	}

	assert(cluster_size == PAGE_SIZE);
	if (copy_strategy == MEMORY_OBJECT_COPY_CALL ||
	    copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
		/*
		 * XXX
		 * Yet to be implemented.
		 */
		kr = KERN_INVALID_ARGUMENT;
	}

	if (kr != KERN_SUCCESS) {
		XMM_USER_RETURN(kobj, KOBJ, kr);
		zfree(xmm_user_request_zone, (vm_offset_t)r);
		return kr;
	}

	KOBJ->copy_strategy = copy_strategy;
	KOBJ->may_cache = may_cache;
	KOBJ->temporary = temporary;
	KOBJ->invalidate = invalidate;
	if (reply_to != IP_NULL)
		queue_enter(&KOBJ->change_list, r, request_t, chain);
	xmm_obj_unlock(kobj);

	if (copy_strategy == MEMORY_OBJECT_COPY_TEMPORARY) {
		copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		temporary = TRUE;
	} else {
		temporary = FALSE;
	}

	kr = K_SET_READY(kobj, may_cache, copy_strategy, cluster_size,
			 reply, temporary, (char *) 0, 0);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_destroy(
	xmm_obj_t	kobj,
	kern_return_t	reason)
{
	kern_return_t kr;

	XPR(XPR_XMM,
	    "memory_object_destroy:  kobj 0x%x reason 0x%x\n",
	    (integer_t) kobj, (integer_t) reason, 0, 0, 0);
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	kr = K_DESTROY(kobj, reason);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_data_supply(
	xmm_obj_t		kobj,
	vm_offset_t		offset,
	vm_offset_t		data,
	unsigned int		length,
	vm_prot_t		lock_value,
	boolean_t		precious,
	ipc_port_t		reply_to,
	mach_msg_type_name_t	reply_to_type)
{
	xmm_reply_data_t reply_data;
	xmm_reply_t reply = XMM_REPLY_NULL;
	kern_return_t kr;
	
	XPR(XPR_XMM,
	    "memory_object_data_supply:  kobj 0x%x off 0x%x data 0x%x...\n",
	    (integer_t) kobj, (integer_t) offset, (integer_t) data, 0, 0);
	XPR(XPR_XMM,
	    "...len 0x%x lock 0x%x precious %d reply 0x%x rt_type 0x%x\n",
	    (integer_t) length, (integer_t) lock_value, (integer_t) precious,
	    (integer_t) reply_to, (integer_t) reply_to_type);
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	if (reply_to != IP_NULL) {
		xmm_reply_set(&reply_data, reply_to_type, reply_to);
		reply = &reply_data;
	}
	kr = K_DATA_SUPPLY(kobj, offset, (vm_map_copy_t)data,
			   length, lock_value, precious, reply);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}

kern_return_t
memory_object_synchronize_completed(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
	kern_return_t kr;
	
	XPR(XPR_XMM,
	    "memory_object_synchronize_completed: kobj 0x%x off 0x%x sz 0x%x\n",
	    (integer_t) kobj, (integer_t) offset, (integer_t) size, 0, 0);
	XMM_USER_ENTER(kobj, KOBJ);
	xmm_obj_unlock(kobj);
	kr = K_SYNCHRONIZE_COMPLETED(kobj, offset, size);
	xmm_obj_lock(kobj);
	XMM_USER_RETURN(kobj, KOBJ, kr);
}
