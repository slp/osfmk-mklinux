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
 * Revision 2.4.2.6  92/06/24  18:02:31  jeffreyh
 * 	More XMM Framework Cleanup: Create ksvm object in _proxy_init.
 * 	  Rework release logic.  Remove xmm_pager from *_completed
 * 	  routines.  Move port cleanup to (new) m_export_deallocate
 * 	  routine.  Cleanup use of xmm_reply, and implement
 * 	  proxy_supply_completed.
 * 	[92/06/24            dlb]
 * 
 * 	Changed routines for XMM Framework Cleanup; changes for new
 * 		xmm_reply structure.
 * 	[92/06/18            sjs]
 * 
 * 	Add release logic to proxy_lock_completed.
 * 	[92/06/09            dlb]
 * 
 * 	Remove unused xmm_object_by_memory_object declaration.
 * 	Delete extra arg from M_CHANGE_COMPLETED in proxy_change_completed.
 * 	[92/06/08            dlb]
 * 
 * 	use_old_pageout --> use_routine
 * 	[92/06/04            dlb]
 * 
 * Revision 2.4.2.5  92/05/27  00:53:54  jeffreyh
 * 	Return KERN_FAILURE if the mobj is invalid and release is set in
 * 	_proxy_change_completed - occurs if the passed port is a dead
 * 	 name.
 * 	[92/05/20            sjs]
 * 
 * Revision 2.4.2.4  92/03/28  10:12:45  jeffreyh
 * 	Change data_write to data_write_return, deleted data_return
 * 	 method, added logic to make change_completed work correctly with
 * 	 change_attributes() 
 * 	[92/03/20            sjs]
 * 
 * Revision 2.4.2.3  92/02/21  11:25:50  jsb
 * 	In _proxy_terminate, deallocate xmm_pager and release xmm_kernel.
 * 	[92/02/20  15:46:35  jsb]
 * 
 * 	Reference mobj on port to mobj conversion; release when done.
 * 	[92/02/20  10:54:05  jsb]
 * 
 * 	Changed MACH_PORT use to IP_NULL. Use m_interpose_deallocate.
 * 	[92/02/18  17:13:28  jsb]
 * 
 * 	Changed reply->mobj to reply->kobj.
 * 	[92/02/16  18:22:12  jsb]
 * 
 * 	Explicitly provide name parameter to xmm_decl macro.
 * 	Hide and release mobj in _proxy_terminate.
 * 	[92/02/16  15:51:50  jsb]
 * 
 * 	Renamed xmm_export_notify to xmm_pager_notify.
 * 	[92/02/10  17:27:15  jsb]
 * 
 * 	Changed proxy_init to use xmm object instead of
 * 	<guessed host, memory_object> pair.
 * 	Renamed mobj_port to xmm_pager, and xmm_control to xmm_kernel.
 * 	[92/02/10  17:01:03  jsb]
 * 
 * 	Use new xmm_decl, and new memory_object_name and deallocation protocol.
 * 	[92/02/09  12:51:49  jsb]
 * 
 * Revision 2.4.2.2  92/01/21  21:54:06  jsb
 * 	Added xmm_export_notify stub.
 * 	[92/01/21  18:22:48  jsb]
 * 
 * 	Use ports instead of pointers when communicating with xmm_import.c.
 * 	De-linted. Supports new (dlb) memory object routines.
 * 	Supports arbitrary reply ports to lock_request, etc.
 * 	Converted mach_port_t (and port_t) to ipc_port_t.
 * 	[92/01/20  17:21:43  jsb]
 * 
 * 	Fixes from OSF.
 * 	[92/01/17  14:14:46  jsb]
 * 
 * Revision 2.4.2.1.1.1  92/01/15  12:15:33  jeffreyh
 * 	Deallocate memory object name port on termination. (dlb)
 * 
 * Revision 2.4.2.1  92/01/03  16:38:45  jsb
 * 	Added missing type cast.
 * 	[91/12/27  21:29:32  jsb]
 * 
 * 	Cleaned up debugging printf.
 * 	[91/12/24  14:30:28  jsb]
 * 
 * Revision 2.4  91/11/15  14:10:03  rpd
 * 	Use ipc_port_copy_send in _proxy_init for import_master.
 * 	[91/09/23  09:14:28  jsb]
 * 
 * Revision 2.3  91/07/01  08:26:07  jsb
 * 	Fixed object importation protocol.
 * 	Corrected declaration of _proxy_lock_completed.
 * 	[91/06/29  15:28:46  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:15  jsb
 * 	First checkin.
 * 	[91/06/17  11:06:11  jsb]
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
 *	File:	norma/xmm_export.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer for allowing remote kernels to map a local object.
 */

#include <kern/host.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <xmm/proxy_user.h>
#include <xmm/proxy_server.h>
#include <xmm/xmm_obj.h>
#include <xmm/xmm_methods.h>
#include <xmm/svm_copy.h> /* XXX only for COPY_CALL_PUSH_PAGE_TO_KERNEL */

struct mobj {
	struct xmm_obj	obj;
	ipc_port_t	xmm_pager;
	ipc_port_t	xmm_kernel;
	xmm_sync	sync;
	int		sc_pending;	/* pending synchronize_completed */
};

/*
 * Forward.
 */
xmm_obj_t	convert_xmm_pager_to_mobj(
			ipc_port_t	xmm_pager);

#undef  KOBJ
#define KOBJ    ((struct mobj *) kobj)

#define	m_export_init			m_invalid_init
#define	m_export_terminate		m_invalid_terminate
#define	m_export_copy			m_invalid_copy
#define	m_export_data_request		m_invalid_data_request
#define	m_export_data_unlock		m_invalid_data_unlock
#define	m_export_data_return		m_invalid_data_return
#define	m_export_lock_completed		m_invalid_lock_completed
#define	m_export_supply_completed	m_invalid_supply_completed
#define	m_export_change_completed	m_invalid_change_completed
#define	m_export_synchronize		m_invalid_synchronize
#define	m_export_freeze			m_invalid_freeze
#define	m_export_share			m_invalid_share
#define	m_export_declare_page		m_invalid_declare_page
#define	m_export_declare_pages		m_invalid_declare_pages
#define	m_export_caching		m_invalid_caching
#define	m_export_uncaching		m_invalid_uncaching

xmm_decl(export, "export", sizeof(struct mobj));

kern_return_t
k_export_data_unavailable(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
#ifdef	lint
	(void) K_DATA_UNAVAILABLE(kobj, offset, length);
#endif	/* lint */
	return proxy_data_unavailable(KOBJ->xmm_kernel, offset, length);
}

kern_return_t
k_export_get_attributes(
	xmm_obj_t			kobj,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			*count)
{
#ifdef	lint
	(void) K_GET_ATTRIBUTES(kobj, flavor, attributes, count);
#endif	/* lint */
	return proxy_get_attributes(KOBJ->xmm_kernel,
				    flavor, attributes, count);
}

kern_return_t
k_export_lock_request(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	boolean_t	should_clean,
	boolean_t	should_flush,
	vm_prot_t	lock_value,
	xmm_reply_t	reply)
{
	kern_return_t kr;

#ifdef	lint
	(void) K_LOCK_REQUEST(kobj, offset, length, should_clean, should_flush,
			      lock_value, reply);
#endif	/* lint */
	if (reply == XMM_REPLY_NULL) {
		return proxy_lock_request(KOBJ->xmm_kernel, offset, length,
					  should_clean, should_flush,
					  lock_value, xmm_empty_reply_data);
	}

	return proxy_lock_request(KOBJ->xmm_kernel, offset, length,
				  should_clean, should_flush, lock_value,
				  *reply);
}

kern_return_t
k_export_data_error(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	error_value)
{
#ifdef	lint
	(void) K_DATA_ERROR(kobj, offset, length, error_value);
#endif	/* lint */
	return proxy_data_error(KOBJ->xmm_kernel, offset, length, error_value);
}

kern_return_t
k_export_set_ready(
	xmm_obj_t			kobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
        vm_size_t			cluster_size,
	xmm_reply_t			reply,
	boolean_t			temporary,
	vm_external_map_t		existence_map,
	vm_offset_t			existence_size)
{
	kern_return_t kr;

#ifdef	lint
	(void) K_SET_READY(kobj, may_cache,
			   copy_strategy, cluster_size, reply, temporary,
			   existence_map, existence_size);
#endif	/* lint */
#if 666
	/*
	 * XXX
	 * Should instead switch to out-of-line form
	 *
	 * XXX
	 * Should use sizeof(inline_existence_map_t), after
	 * correcting declaration in mach/mach_types.h
	 */
	if (vm_external_map_size(existence_size) > 512) {
		vm_offset_t new_size;

		new_size = ptoa(512 << 3);
		printf("k_export_set_ready: existence_size: %d -> %d\n",
		       existence_size, new_size);
		existence_size = new_size;
	}
#endif
	if (reply == XMM_REPLY_NULL) {
		return proxy_set_ready(KOBJ->xmm_kernel, KOBJ->xmm_pager,
				       may_cache, copy_strategy,
				       cluster_size, KERN_SUCCESS,
				       xmm_empty_reply_data, temporary, 
				       existence_map,
				       vm_external_map_size(existence_size),
				       existence_size);
	}
	return proxy_set_ready(KOBJ->xmm_kernel, KOBJ->xmm_pager, may_cache,
			       copy_strategy, cluster_size, KERN_SUCCESS,
			       *reply, temporary, existence_map,
			       vm_external_map_size(existence_size),
			       existence_size);
}

kern_return_t
k_export_destroy(
	xmm_obj_t	kobj,
	kern_return_t	reason)
{
#ifdef	lint
	(void) K_DESTROY(kobj, reason);
#endif	/* lint */
	return proxy_destroy(KOBJ->xmm_kernel, reason);
}

kern_return_t
k_export_data_supply(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	vm_prot_t	lock_value,
	boolean_t	precious,
	xmm_reply_t	reply)
{
	kern_return_t kr;

#ifdef	lint
	(void) K_DATA_SUPPLY(kobj, offset, data, length, lock_value, precious,
			     reply);
#endif	/* lint */
	if (reply == XMM_REPLY_NULL) {
		return proxy_data_supply(KOBJ->xmm_kernel, offset,
					 (vm_offset_t) data,
					 length, lock_value, precious,
					 xmm_empty_reply_data);
	}

	return proxy_data_supply(KOBJ->xmm_kernel, offset, (vm_offset_t) data,
				 length, lock_value, precious,
				 *reply);
}

kern_return_t
k_export_create_copy(
	xmm_obj_t	kobj,
	ipc_port_t	new_copy_pager,
	kern_return_t	*result)
{
#ifdef	lint
	(void) K_CREATE_COPY(kobj, new_copy_pager, result);
#endif	/* lint */
	return proxy_create_copy(KOBJ->xmm_kernel, new_copy_pager, result);
}

kern_return_t
k_export_uncaching_permitted(
	xmm_obj_t	kobj)
{
#ifdef	lint
	(void) K_UNCACHING_PERMITTED(kobj);
#endif	/* lint */
	return proxy_uncaching_permitted(KOBJ->xmm_kernel);
}

kern_return_t
k_export_synchronize_completed(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	size)
{

#ifdef	lint
	K_SYNCHRONIZE_COMPLETED(kobj, offset, size);
#endif	/* lint */
	xmm_obj_lock(kobj);
	assert(KOBJ->sc_pending > 0);
	KOBJ->sc_pending--;
	xmm_obj_unlock(kobj);
	return proxy_synchronize_completed(KOBJ->xmm_kernel, offset, size);
}

kern_return_t
k_export_release_all(
	xmm_obj_t	kobj)
{
	kern_return_t kr;

	assert(KOBJ->sc_pending == 0);
	kr = proxy_release_all(KOBJ->xmm_kernel);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	/*
	 * Clean up the MOBJ.
	 */
	xmm_obj_unlink(kobj);
	xmm_obj_lock(kobj);
	xmm_obj_release(kobj); /* consumes lock */
	return kr;
}

xmm_obj_t
convert_xmm_pager_to_mobj(
	ipc_port_t	xmm_pager)
{
	xmm_obj_t mobj = XMM_OBJ_NULL;

	if (IP_VALID(xmm_pager)) {
		ip_lock(xmm_pager);
		if (ip_active(xmm_pager) &&
		    ip_kotype(xmm_pager) == IKOT_XMM_PAGER) {
			mobj = (xmm_obj_t) xmm_pager->ip_kobject;
			xmm_obj_lock(mobj);
			if (MOBJ->xmm_pager) {
				xmm_obj_reference(mobj);
				xmm_obj_unlock(mobj);
			} else {
				xmm_obj_unlock(mobj);
				mobj = XMM_OBJ_NULL;
			}
		}
		ip_unlock(xmm_pager);
	}
	return mobj;
}

#if	MACH_KDB
#include <ddb/db_output.h>

#define printf	kdbprintf

void
m_export_debug(
	xmm_obj_t	mobj,
	int		flags)
{
	xmm_obj_print((db_addr_t)mobj);
	indent += 4;
	iprintf("xmm_pager=0x%x, xmm_kernel=0x%x, sc_pending=%d\n",
		MOBJ->xmm_pager,
		MOBJ->xmm_kernel,
		MOBJ->sc_pending);
	xmm_sync_debug(&MOBJ->sync);
	indent -= 4;
	M_DEBUG(mobj, flags);
}
#endif	/* MACH_KDB */

kern_return_t
_proxy_init(
	ipc_port_t	xmm_object,
	ipc_port_t	xmm_kernel,
	vm_size_t	pagesize,
	boolean_t	internal,
	vm_size_t	size)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	/*
	 * XXX
	 * Check for multiple inits and/or reuse of memory_object.
	 * XXX
	 * Should use proxy_set_ready to return errors.
	 */
	ip_lock(xmm_object);
	if (ip_kotype(xmm_object) != IKOT_XMM_OBJECT) {
		ip_unlock(xmm_object);
		return KERN_INVALID_ARGUMENT;
	}
	mobj = (xmm_obj_t) xmm_object->ip_kobject;
	ip_unlock(xmm_object);

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	xmm_ksvm_create(mobj, &mobj, FALSE);
#else	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	xmm_ksvm_create(mobj, &mobj);
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	xmm_obj_allocate(&export_class, mobj, &mobj);

	/*
	 * It's not worth holding mobj lock since associated KSVM object is not
	 *	yet ready (no operation will come from SVM layer), and import
	 *	layer does not yet know about the MOBJ->xmm_pager port.
	 */
	MOBJ->xmm_pager = ipc_port_alloc_kernel();
	if (MOBJ->xmm_pager == IP_NULL) {
		panic("m_import_init: allocate xmm_pager");
	}
	IP_CLEAR_NMS(MOBJ->xmm_pager);

	xmm_sync_init(&MOBJ->sync);
	MOBJ->xmm_kernel = xmm_kernel;
	MOBJ->sc_pending = 0;
	xmm_obj_reference(mobj);
	ipc_kobject_set(MOBJ->xmm_pager, (ipc_kobject_t) mobj, IKOT_XMM_PAGER);

	kr = M_INIT(mobj, pagesize, internal, size);

	xmm_obj_lock(mobj);
	xmm_obj_release(mobj);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_terminate(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno,
	boolean_t		release)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	assert(release == FALSE);

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}

	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);

	kr = M_TERMINATE(mobj, release);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}


kern_return_t
_proxy_copy(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);
	kr = M_COPY(mobj);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_data_request(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t		offset,
	vm_size_t		length,
	vm_prot_t		desired_access)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(mobj);
	kr = M_DATA_REQUEST(mobj, offset, length, desired_access);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_data_unlock(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(mobj);
	kr = M_DATA_UNLOCK(mobj, offset, length, desired_access);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_data_return(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_offset_t	data,
	vm_size_t	length,
	boolean_t	dirty,
	boolean_t	kernel_copy)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(mobj);
	kr = M_DATA_RETURN(mobj, offset,
			   (vm_map_copy_t)data, length, dirty, kernel_copy);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_lock_completed(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t		offset,
	vm_size_t		length,
	xmm_reply_data_t	reply_data)
{
	xmm_reply_t reply = &reply_data;
	xmm_obj_t mobj;
	kern_return_t kr;

        mobj = convert_xmm_pager_to_mobj(xmm_pager);
        if (mobj == XMM_OBJ_NULL) {
                return KERN_FAILURE;
        }

        if (xmm_reply_empty(reply)) {
                reply = XMM_REPLY_NULL;
        }

	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(mobj);
	kr = M_LOCK_COMPLETED(mobj, offset, length, reply);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_supply_completed(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t		offset,
	vm_size_t		length,
	kern_return_t		result,
	vm_offset_t		error_offset,
	xmm_reply_data_t	reply_data)
{
	xmm_reply_t reply = &reply_data;
	xmm_obj_t mobj;
	kern_return_t kr;

        mobj = convert_xmm_pager_to_mobj(xmm_pager);
        if (mobj == XMM_OBJ_NULL) {
                return KERN_FAILURE;
        }
        if (xmm_reply_empty(reply)) {
                reply = XMM_REPLY_NULL;
        }
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(mobj);
	kr = M_SUPPLY_COMPLETED(mobj, offset, length, result, 
				  error_offset, reply);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_change_completed(
	ipc_port_t			xmm_pager,
	mach_port_seqno_t	seqno,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	xmm_reply_data_t		reply_data)
{
	xmm_reply_t reply = &reply_data;
	xmm_obj_t mobj;
	kern_return_t kr;

        mobj = convert_xmm_pager_to_mobj(xmm_pager);
        if (mobj == XMM_OBJ_NULL) {
                return KERN_FAILURE;
        }

        if (xmm_reply_empty(reply)) {
                reply = XMM_REPLY_NULL;
        }
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);
	kr = M_CHANGE_COMPLETED(mobj, may_cache, copy_strategy, reply);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_synchronize(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_offset_t	length,
	vm_sync_t	sync_flags)
{
	xmm_obj_t mobj;
	kern_return_t kr;

        mobj = convert_xmm_pager_to_mobj(xmm_pager);
        if (mobj == XMM_OBJ_NULL) {
                return KERN_FAILURE;
        }
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE,
		      offset, length);
	MOBJ->sc_pending++;
	xmm_obj_unlock(mobj);
	kr = M_SYNCHRONIZE(mobj, offset, length, sync_flags);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_freeze(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno,
	char			*existence_map,
	unsigned int		existence_map_size,
	vm_offset_t		existence_size)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);
	kr = M_FREEZE(mobj, (vm_external_map_t)existence_map, existence_size);
	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_share(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);
	kr = M_SHARE(mobj);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_declare_page(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_size_t	size)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, FALSE, offset, size);
	xmm_obj_unlock(mobj);
	kr = M_DECLARE_PAGE(mobj, offset, size);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_declare_pages(
	ipc_port_t		xmm_pager,
	mach_port_seqno_t	seqno,
	char			*existence_map,
	unsigned int		existence_map_size,
	vm_offset_t		existence_size,
	boolean_t		frozen)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);
	kr = M_DECLARE_PAGES(mobj, (vm_external_map_t)existence_map,
						existence_size, frozen);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_caching(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);
	kr = M_CACHING(mobj);

	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

kern_return_t
_proxy_uncaching(
	ipc_port_t	xmm_pager,
	mach_port_seqno_t	seqno)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	mobj = convert_xmm_pager_to_mobj(xmm_pager);
	if (mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_pager, seqno, mobj, &MOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(mobj);

	kr = M_UNCACHING(mobj);
	xmm_obj_lock(mobj);
	xmm_ipc_exit(mobj, seqno, &MOBJ->sync);	/* consumes lock */
	return kr;
}

void
m_export_deallocate(
	xmm_obj_t	mobj)
{
	ipc_port_t port;

	assert(xmm_obj_lock_held(mobj));
	port = MOBJ->xmm_pager;
	MOBJ->xmm_pager = (ipc_port_t)0;
	xmm_obj_unlock(mobj);

	ipc_kobject_set(port, IKO_NULL, IKOT_NONE);
	ipc_port_dealloc_kernel(port);
	ipc_port_release_send(MOBJ->xmm_kernel);
}
