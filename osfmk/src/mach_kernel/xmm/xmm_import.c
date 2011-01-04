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
 * Revision 2.5.2.7  92/06/24  18:02:39  jeffreyh
 * 	Changes routines for new interface, changes for new reply
 * 	 structure.
 * 	[92/06/18            sjs]
 * 
 * 	Add release logic to lock completed processing.
 * 	[92/06/09            dlb]
 * 
 * 	use_old_pageout --> use_routine
 * 	[92/06/04            dlb]
 * 
 * Revision 2.5.2.6  92/03/28  10:12:50  jeffreyh
 * 	Changed data_write to data_write_return and deleted data_return.
 * 	 Fixed terminate and change_completed to support m_o_change_attributes().
 * 	[92/03/20            sjs]
 * 
 * Revision 2.5.2.5  92/02/21  11:25:54  jsb
 * 	Reference mobj on port to mobj conversion; release when done.
 * 	[92/02/20  10:53:45  jsb]
 * 
 * 	Deallocate all resources upon termination.
 * 	Deallocate allocated replies.
 * 	Changed MACH_PORT_NULL uses to IP_NULL.
 * 	[92/02/18  07:56:15  jsb]
 * 
 * 	Explicitly provide name parameter to xmm_decl macro.
 * 	Handle m_import_terminate being called before proxy_set_ready has been.
 * 	This can happen because the vm system may terminate an object after
 * 	it has been initialized but before it is ready.
 * 	[92/02/16  15:26:31  jsb]
 * 
 * 	Don't call proxy_terminate on a null port.
 * 	[92/02/11  18:21:03  jsb]
 * 
 * 	Added ipc_kobject_set to null to break port/mobj association.
 * 	Accordingly, removed dead field and associated logic.
 * 	[92/02/11  13:22:39  jsb]
 * 
 * 	Renamed xmm_import_notify to xmm_kernel_notify.
 * 	[92/02/10  17:26:50  jsb]
 * 
 * 	Use xmm object instead of <guessed host, memory_object> pair in
 * 	proxy_init. Renamed {mobj,kobj}_port to xmm_{pager,kernel}.
 * 	[92/02/10  16:56:20  jsb]
 * 
 * 	Use new xmm_decl, and new memory_object_name and deallocation protocol.
 * 	[92/02/09  12:51:12  jsb]
 * 
 * Revision 2.5.2.4  92/01/21  22:22:24  jsb
 * 	18-Jan-92 David L. Black (dlb) at Open Software Foundation
 * 	Add dead field to mobj and use to synchronize termination
 * 	against other operations.
 * 
 * Revision 2.5.2.3  92/01/21  21:54:12  jsb
 * 	Added xmm_import_notify stub.
 * 	[92/01/21  18:20:54  jsb]
 * 
 * 	Use ports instead of pointers when communicating with xmm_export.c.
 * 	De-linted. Supports new (dlb) memory object routines.
 * 	Supports arbitrary reply ports to lock_request, etc.
 * 	Converted mach_port_t (and port_t) to ipc_port_t.
 * 	[92/01/20  17:22:22  jsb]
 * 
 * 	Fixes from OSF.
 * 	[92/01/17  14:15:01  jsb]
 * 
 * Revision 2.5.2.2.1.1  92/01/15  12:16:26  jeffreyh
 * 	Pass memory_object_name port to proxy terminate. (dlb)
 * 
 * Revision 2.5.2.2  92/01/09  18:46:13  jsb
 * 	Use remote_host_priv() instead of norma_get_special_port().
 * 	[92/01/04  18:33:18  jsb]
 * 
 * Revision 2.5.2.1  92/01/03  16:38:50  jsb
 * 	Corrected log.
 * 	[91/12/24  14:33:29  jsb]
 * 
 * Revision 2.5  91/12/10  13:26:24  jsb
 * 	Added missing third parameter in call to proxy_terminate.
 * 	[91/12/10  12:48:52  jsb]
 * 
 * Revision 2.4  91/11/14  16:52:34  rpd
 * 	Replaced master_device_port_at_node call with calls to
 *	norma_get_special_port and norma_port_location_hint.
 * 	[91/11/00            jsb]
 * 
 * Revision 2.3  91/07/01  08:26:12  jsb
 * 	Fixed object importation protocol. Return valid return values.
 * 	[91/06/29  15:30:10  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:18  jsb
 * 	First checkin.
 * 	[91/06/17  11:03:28  jsb]
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
 *	File:	norma/xmm_import.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer for mapping a remote object.
 */

#include <dipc.h>

#include <kern/misc_protos.h>
#include <xmm/proxy_user.h>
#include <xmm/proxy_server.h>
#include <xmm/xmm_obj.h>
#include <xmm/xmm_methods.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#if	DIPC
#include <dipc/dipc_port.h>
#endif

struct mobj {
	struct xmm_obj	obj;
	ipc_port_t	xmm_object;
	ipc_port_t	xmm_pager;
	ipc_port_t	xmm_kernel;
	unsigned
	/* boolean_t */	needs_terminate:1,
	/* boolean_t */	terminated:1,
 	/* boolean_t */	destroyed:1;
	xmm_sync	sync;
};

/*
 * Forward.
 */
xmm_obj_t	convert_xmm_kernel_to_kobj(
			ipc_port_t	xmm_kernel);

#undef  KOBJ
#define KOBJ    ((struct mobj *) kobj)

#define	m_import_deallocate		m_interpose_deallocate
#define	k_import_data_unavailable	k_invalid_data_unavailable
#define	k_import_get_attributes		k_invalid_get_attributes
#define	k_import_lock_request		k_invalid_lock_request
#define	k_import_data_error		k_invalid_data_error
#define	k_import_set_ready		k_invalid_set_ready
#define	k_import_destroy		k_invalid_destroy
#define	k_import_data_supply		k_invalid_data_supply
#define	k_import_create_copy		k_invalid_create_copy
#define	k_import_uncaching_permitted	k_invalid_uncaching_permitted
#define	k_import_release_all		k_invalid_release_all
#define	k_import_synchronize_completed	k_invalid_synchronize_completed

xmm_decl(import, "import", sizeof(struct mobj));

void
xmm_import_create(
	ipc_port_t	xmm_object,
	xmm_obj_t	*new_mobj)
{
	xmm_obj_t mobj;

	xmm_obj_allocate(&import_class, XMM_OBJ_NULL, &mobj);

	MOBJ->xmm_object = xmm_object;
	MOBJ->xmm_pager = IP_NULL;
	MOBJ->xmm_kernel = IP_NULL;
	MOBJ->needs_terminate = FALSE;
	MOBJ->terminated = FALSE;
	MOBJ->destroyed = FALSE;
	xmm_sync_init(&MOBJ->sync);

	*new_mobj = mobj;
}

kern_return_t
m_import_init(
	xmm_obj_t	mobj,
	vm_size_t	pagesize,
	boolean_t	internal,
	vm_size_t	size)
{
	kern_return_t kr;
	ipc_port_t port;
	
#ifdef	lint
	(void) M_INIT(mobj, pagesize, internal, size);
#endif	/* lint */

	port = ipc_port_alloc_kernel();
	if (port == IP_NULL) {
		panic("m_import_init: allocate xmm_kernel");
	}
	IP_CLEAR_NMS(port);
	ipc_kobject_set(port, (ipc_kobject_t) mobj, IKOT_XMM_KERNEL);
	xmm_obj_lock(mobj);
	MOBJ->xmm_kernel = port;
	xmm_obj_reference(mobj);
	xmm_obj_unlock(mobj);

	kr = proxy_init(MOBJ->xmm_object, MOBJ->xmm_kernel, pagesize,
			internal, size);
	assert(kr == KERN_SUCCESS);
	return KERN_SUCCESS;
}

kern_return_t
m_import_terminate(
	xmm_obj_t	mobj,
	boolean_t	release)
{
	kern_return_t kr;

#ifdef	lint
	(void) M_TERMINATE(mobj, release);
#endif	/* lint */
	assert(MOBJ->xmm_kernel != IP_NULL);

	xmm_obj_lock(mobj);
	if (MOBJ->xmm_pager == IP_NULL) {
		/*
		 * This can happen because the vm system only waits
		 * until the object is initialized before terminating
		 * it; it does not wait until the object is ready.
		 * We must therefore wait for _proxy_set_ready ourselves
		 * before terminating the object. We don't even have the
		 * option of calling proxy_terminate on xmm_object, since
		 * we no longer have send rights for xmm_object.
		 *
		 * We need to retain the xmm_kernel to mobj association
		 * so that we can process _proxy_set_ready. Fortunately,
		 * there is no need to break this association, since the
		 * only call that we should receive is _proxy_set_ready.
		 * Any spurious calls to anything else will be caught
		 * at the xmm_server level.
		 */
		MOBJ->needs_terminate = TRUE;
		xmm_obj_unlock(mobj);
		return KERN_SUCCESS;
	}
	assert(!MOBJ->terminated);
	MOBJ->terminated = TRUE;
	xmm_obj_unlock(mobj);
	kr = proxy_terminate(MOBJ->xmm_pager, release);
	assert(kr == KERN_SUCCESS);

	return KERN_SUCCESS;
}

kern_return_t
m_import_copy(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) M_COPY(mobj);
#endif	/* lint */
	return proxy_copy(MOBJ->xmm_pager);
}

/*
 *	VM system should handle ready synchronization for everything else
 */

kern_return_t
m_import_data_request(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
#ifdef	lint
	(void) M_DATA_REQUEST(mobj, offset, length, desired_access);
#endif	/* lint */
	return proxy_data_request(MOBJ->xmm_pager, offset, length,
				  desired_access);
}

kern_return_t
m_import_data_unlock(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
#ifdef	lint
	(void) M_DATA_UNLOCK(mobj, offset, length, desired_access);
#endif	/* lint */
	return proxy_data_unlock(MOBJ->xmm_pager, offset, length,
				 desired_access);
}

kern_return_t
m_import_data_return(
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

	return proxy_data_return(MOBJ->xmm_pager, offset, (vm_offset_t) data,
				 length, dirty, kernel_copy);
}

kern_return_t
m_import_lock_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	xmm_reply_t	reply)
{
#ifdef	lint
	(void) M_LOCK_COMPLETED(mobj, offset, length, reply);
#endif	/* lint */
	if (reply == XMM_REPLY_NULL) {
		reply = &xmm_empty_reply_data;
	}

	return proxy_lock_completed(MOBJ->xmm_pager, offset, length, *reply);
}

kern_return_t
m_import_supply_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	result,
	vm_offset_t	error_offset,
	xmm_reply_t	reply)
{
#ifdef	lint
	(void) M_SUPPLY_COMPLETED(mobj, offset, length, result, error_offset,
				  reply);
#endif	/* lint */
	if (reply == XMM_REPLY_NULL) {
		reply = &xmm_empty_reply_data;
	}

	return proxy_supply_completed(MOBJ->xmm_pager, offset, length, result,
				      error_offset, *reply);
}

kern_return_t
m_import_change_completed(
	xmm_obj_t			mobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	xmm_reply_t			reply)
{
#ifdef	lint
	(void) M_CHANGE_COMPLETED(mobj, may_cache, copy_strategy, reply);
#endif	/* lint */
	if (reply == XMM_REPLY_NULL) {
		reply = &xmm_empty_reply_data;
	}

	return proxy_change_completed(MOBJ->xmm_pager, may_cache, 
				      copy_strategy, *reply);
}

kern_return_t
m_import_synchronize(
	xmm_obj_t 	mobj,
	vm_offset_t	offset,
	vm_offset_t	length,
	vm_sync_t	sync_flags)
{
#ifdef	lint
	(void) M_SYNCHRONIZE(mobj, may_cache, copy_strategy, reply);
#endif	/* lint */

	return proxy_synchronize(MOBJ->xmm_pager, offset,
				 length, sync_flags);
}

kern_return_t
m_import_freeze(
	xmm_obj_t		mobj,
	vm_external_map_t	existence_map,
	vm_offset_t		existence_size)
{
#ifdef	lint
	(void) M_FREEZE(mobj, existence_map, existence_size);
#endif	/* lint */
	return proxy_freeze(MOBJ->xmm_pager, existence_map,
			vm_external_map_size(existence_size), existence_size);
}

kern_return_t
m_import_share(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) M_SHARE(mobj);
#endif	/* lint */
	return proxy_share(MOBJ->xmm_pager);
}

kern_return_t
m_import_declare_page(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
#ifdef	lint
	(void) M_DECLARE_PAGE(mobj, offset, size);
#endif	/* lint */
	return proxy_declare_page(MOBJ->xmm_pager, offset, size);
}

kern_return_t
m_import_declare_pages(
	xmm_obj_t		mobj,
	vm_external_map_t	existence_map,
	vm_offset_t		existence_size,
	boolean_t		frozen)
{
#ifdef	lint
	(void) M_DECLARE_PAGES(mobj, existence_map, existence_size, frozen);
#endif	/* lint */
	return proxy_declare_pages(MOBJ->xmm_pager, existence_map,
		vm_external_map_size(existence_size), existence_size, frozen);
}

kern_return_t
m_import_caching(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) M_CACHING(mobj);
#endif	/* lint */
	return proxy_caching(MOBJ->xmm_pager);
}

kern_return_t
m_import_uncaching(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) M_UNCACHING(mobj);
#endif	/* lint */ 
	return proxy_uncaching(MOBJ->xmm_pager);
}

#if	MACH_KDB
#include <ddb/db_output.h>

void
m_import_debug(
	xmm_obj_t	mobj,
	int		flags)
{
	xmm_obj_print((db_addr_t)mobj);
	indent += 4;
	iprintf("xmm_object=0x%x xmm_pager=0x%x xmm_kernel=0x%x\n",
		MOBJ->xmm_object,
		MOBJ->xmm_pager,
		MOBJ->xmm_kernel);
	iprintf("%sneeds_term, %sterminated, %sdestroyed\n",
		BOOL(MOBJ->needs_terminate),
		BOOL(MOBJ->terminated),
		BOOL(MOBJ->destroyed));
	xmm_sync_debug(&MOBJ->sync);
	indent -= 4;
	printf("----------Continued on Node %d",
	       IP_PORT_NODE(MOBJ->xmm_pager));
#if	DIPC
	printf (" DIPC UID origin=0x%x identifier=0x%x",
		MOBJ->xmm_pager->ip_uid.origin,
		MOBJ->xmm_pager->ip_uid.identifier);
#endif	/* DIPC */
	printf("----------\n");
}
#endif	/* MACH_KDB */

xmm_obj_t
convert_xmm_kernel_to_kobj(
	ipc_port_t	xmm_kernel)
{
	xmm_obj_t kobj = XMM_OBJ_NULL;

	if (IP_VALID(xmm_kernel)) {
		ip_lock(xmm_kernel);
		if (ip_active(xmm_kernel) &&
		    ip_kotype(xmm_kernel) == IKOT_XMM_KERNEL) {
			kobj = (xmm_obj_t) xmm_kernel->ip_kobject;
			xmm_obj_lock(kobj);
			if (KOBJ->xmm_kernel) {
				xmm_obj_reference(kobj);
				xmm_obj_unlock(kobj);
			} else {
				xmm_obj_unlock(kobj);
				kobj = XMM_OBJ_NULL;
			}
		}
		ip_unlock(xmm_kernel);
	}
	return kobj;
}

kern_return_t
_proxy_release_all(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno)
{
	kern_return_t kr;
	xmm_obj_t kobj;
	ipc_port_t port;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	/*
	 * Deallocate resources associated with mobj.
	 * MOBJ->xmm_object was deallocated via proxy_init.
	 * MOBJ->xmm_kernel will be deallocated via proxy_terminate.
	 * We must explicitly deallocate MOBJ->xmm_pager
	 * (to make mobj inaccessible) and then release mobj.
	 */
	ipc_kobject_set(xmm_kernel, IKO_NULL, IKOT_NONE);
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, TRUE, 0, 0);
	assert(KOBJ->terminated);
	KOBJ->xmm_kernel = (ipc_port_t)0;
	xmm_obj_unlock(kobj);
	ipc_port_dealloc_kernel(xmm_kernel);

	/*
	 * release reference held by xmm_kernel port
	 */
	xmm_obj_lock(kobj);
	xmm_obj_release(kobj);	/* consumes lock */
#define	UGLY_HACK 1
#ifdef UGLY_HACK	
	if (kobj->k_kobj != XMM_OBJ_NULL)
#endif /* UGLY_HACK */
		kr = K_RELEASE_ALL(kobj);
	/*
	 * release send right (pager reference) on the proxy port.
	 */
	ipc_port_release_send(KOBJ->xmm_pager);
	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return(KERN_SUCCESS);
}

kern_return_t
_proxy_data_unavailable(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_size_t	length)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(kobj);
	kr = K_DATA_UNAVAILABLE(kobj, offset, length);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_get_attributes(
	ipc_port_t			xmm_kernel,
	mach_port_seqno_t	seqno,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			*count)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(kobj);
	kr = K_GET_ATTRIBUTES(kobj, flavor, attributes, count);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_lock_request(
	ipc_port_t		xmm_kernel,
	mach_port_seqno_t	seqno,
	vm_offset_t		offset,
	vm_size_t		length,
	boolean_t		should_clean,
	boolean_t		should_flush,
	vm_prot_t		prot,
	xmm_reply_data_t	reply_data)
{
	kern_return_t kr;
	xmm_reply_t reply = &reply_data;
	xmm_obj_t kobj;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		/*
		 * XXX	What about reply message???
		 * XXX	Printf here for now.
		 * XXX  Same thing goes for other calls that ask for replies.
		 */
		printf("Rejecting proxy_lock_request on dead object\n");
		return KERN_FAILURE;
	}
	if (xmm_reply_empty(reply)) {
		reply = XMM_REPLY_NULL;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(kobj);
	kr = K_LOCK_REQUEST(kobj, offset, length, should_clean, should_flush,
			    prot, reply);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_data_error(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	error_value)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(kobj);
	kr = K_DATA_ERROR(kobj, offset, length, error_value);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_set_ready(
	ipc_port_t			xmm_kernel,
	mach_port_seqno_t	seqno,
	ipc_port_t			xmm_pager,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
        vm_size_t			cluster_size,
	kern_return_t			error_value,
	xmm_reply_data_t		reply_data,
	boolean_t			temporary,
	char				*existence_map,
	unsigned int			existence_map_size,
	vm_offset_t			existence_size)
{
	xmm_obj_t kobj;
	xmm_reply_t reply = &reply_data;
	kern_return_t kr;

	if (error_value) {
		/* destroy? or should export have done that? */
		printf("proxy_set_ready loses\n");
	}
	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}

	/*
	 * save the pager port if this is the first send right made,
	 * otherwise release the right.  The original send right will
	 * be sent back and released via proxy_terminate.
	 */
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, TRUE, 0, 0);
	if (KOBJ->xmm_pager == IP_NULL)
		KOBJ->xmm_pager = xmm_pager;
	else {
		xmm_obj_unlock(kobj);
		ipc_port_release_send(KOBJ->xmm_pager);
		xmm_obj_lock(kobj);
	}

	if (KOBJ->needs_terminate) {
		/*
		 * Now that we have xmm_pager, we can process the
		 * pending m_import_terminate.  Note that the
		 * change completed is expected to come after
		 * the terminate.
		 */
		xmm_obj_unlock(kobj);
		kr = m_import_terminate(kobj, FALSE);

		if (! xmm_reply_empty(reply))
			m_import_change_completed(kobj, may_cache, 
						  copy_strategy, reply);
		xmm_obj_lock(kobj);
		xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
		/*
		 * m_import_terminate returns KERN_ABORTED to
		 * prevent the mobj stack from going away before
		 * k_release_all comes down.  Here, we just want to
		 * return success, else Mig will dump all the stuff
		 * we just received.
		 */
		return  KERN_SUCCESS;
	}
	xmm_obj_unlock(kobj);
	if (xmm_reply_empty(reply)) {
		reply = XMM_REPLY_NULL;
	}
	kr = K_SET_READY(kobj, may_cache,
			 copy_strategy, cluster_size, reply, temporary,
			 (vm_external_map_t)existence_map, existence_size);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_destroy(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno,
	kern_return_t	reason)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, TRUE, 0, 0);
	KOBJ->destroyed = TRUE;
	xmm_obj_unlock(kobj);
	kr = K_DESTROY(kobj, reason);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_data_supply(
	ipc_port_t		xmm_kernel,
	mach_port_seqno_t	seqno,
	vm_offset_t		offset,
	vm_offset_t		data,
	unsigned int		length,
	vm_prot_t		lock_value,
	boolean_t		precious,
	xmm_reply_data_t	reply_data)
{
	xmm_obj_t kobj;
	xmm_reply_t reply = &reply_data;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	if (xmm_reply_empty(reply)) {
		reply = XMM_REPLY_NULL;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, FALSE,
		      offset, length);
	xmm_obj_unlock(kobj);
	kr = K_DATA_SUPPLY(kobj, offset, (vm_map_copy_t)data,
			   length, lock_value, precious,  reply);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_create_copy(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno,
	ipc_port_t	new_copy_pager,
	kern_return_t	*result)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(kobj);
	kr = K_CREATE_COPY(kobj, new_copy_pager, result);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_uncaching_permitted(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, TRUE, 0, 0);
	xmm_obj_unlock(kobj);
	kr = K_UNCACHING_PERMITTED(kobj);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}

kern_return_t
_proxy_synchronize_completed(
	ipc_port_t	xmm_kernel,
	mach_port_seqno_t	seqno,
	vm_offset_t	offset,
	vm_size_t	size)
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kobj = convert_xmm_kernel_to_kobj(xmm_kernel);
	if (kobj == XMM_OBJ_NULL) {
		printf("proxy_synchronize_completed:ERROR: kernel 0x%x\n", xmm_kernel);
		return KERN_FAILURE;
	}
	xmm_ipc_enter(xmm_kernel, seqno, kobj, &KOBJ->sync, FALSE,
		      offset, size);
	xmm_obj_unlock(kobj);
	kr = K_SYNCHRONIZE_COMPLETED(kobj, offset, size);

	xmm_obj_lock(kobj);
	xmm_ipc_exit(kobj, seqno, &KOBJ->sync); /* consumes lock */
	return kr;
}
