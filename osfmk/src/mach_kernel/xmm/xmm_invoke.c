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
 * Revision 2.1.2.4  92/06/24  18:02:52  jeffreyh
 * 	No more R_ invocations (reply as first arg); all _completed
 * 	routines are now M_ invocations.  No longer pass kobj for
 * 	M_ invocations.  Minor change to xmm_obj_unlink and release
 * 	logic.
 * 	[92/06/24            dlb]
 * 
 * 	Add release logic to M_LOCK_COMPLETED.
 * 	[92/06/09            dlb]
 * 
 * 	use_old_pageout --> use_routine
 * 	[92/06/04            dlb]
 * 
 * Revision 2.1.2.3  92/05/27  00:54:00  jeffreyh
 * 	M_CHANGE_COMPLETED now handles invalid mobjs properly - changed
 * 	macros to make it all work.
 * 	[92/05/20            sjs]
 * 
 * Revision 2.1.2.2  92/03/28  10:13:07  jeffreyh
 * 	Put back in deleted line, some cleanup.
 * 	[92/03/25            sjs]
 * 	Changed data_write to data_write_return and deleted data_return
 * 	 method.  Changed TERMINATE to conditionalize obj_unlink call,
 * 	 and added same logic to CHANGE_COMPLETED to support
 * 	 m_o_change_attributes(). 
 * 	[92/03/20            sjs]
 * 
 * Revision 2.1.2.1  92/02/21  11:27:27  jsb
 * 	Removed reference counting from {M,K}_{REFERENCE,RELEASE} macros.
 * 	Added check for null obj in {M,K}_CALL macros.
 * 	Added call to xmm_obj_unlink to M_TERMINATE.
 * 	[92/02/16  14:17:12  jsb]
 * 
 * 	First checkin.
 * 	[92/02/09  14:17:05  jsb]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
 *	File:	norma/xmm_invoke.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1992
 *
 *	Xmm invocation routines.
 */

#include <kern/misc_protos.h>
#include <mach/boolean.h>
#include <mach/vm_sync.h>
#include <xmm/xmm_obj.h>
#include <xmm/xmm_methods.h>

/*
 * I've considered automatically (i.e., in these macros) grabbing and
 * release obj or class locks, but I don't see a good protocol for
 * doing so.
 */

#define	M_REFERENCE\
	kern_return_t kr;\
	register xmm_obj_t m_mobj;\
\
	check_simple_locks(); \
	m_mobj = mobj->m_mobj;\
	if (m_mobj == XMM_OBJ_NULL) {\
		panic("m_invoke: null obj");\
		return KERN_FAILURE;\
	}\
    	kr = /* function call that follows */

#define	M_RELEASE\
	check_simple_locks(); \
	return kr;

#define	K_REFERENCE\
	kern_return_t kr;\
	register xmm_obj_t k_kobj;\
\
	check_simple_locks(); \
	k_kobj = kobj->k_kobj;\
	if (k_kobj == XMM_OBJ_NULL) {\
		panic("k_invoke: null obj");\
		return KERN_FAILURE;\
	}\
    	kr = /* function call that follows */

#define	K_RELEASE\
	check_simple_locks(); \
	return kr;

#if	__STDC__
#define	M_CALL(method)	(*m_mobj->class->m_ ## method)
#define	K_CALL(method)	(*k_kobj->class->k_ ## method)
#else	/* __STDC__ */
#define	M_CALL(method)	(*m_mobj->class->m_/**/method)
#define	K_CALL(method)	(*k_kobj->class->k_/**/method)
#endif	/* __STDC__ */


kern_return_t
M_INIT(
	xmm_obj_t	mobj,
	vm_size_t	pagesize,
	boolean_t	internal,
	vm_size_t	size)
{
	M_REFERENCE
	M_CALL(init)(m_mobj, pagesize, internal, size);
	M_RELEASE
}

kern_return_t
M_TERMINATE(
	xmm_obj_t	mobj,
	boolean_t	release)
{
	M_REFERENCE
	M_CALL(terminate)(m_mobj, release);
	if (release && kr == KERN_SUCCESS)
		xmm_obj_unlink(mobj);
	M_RELEASE
}

kern_return_t
M_COPY(
	xmm_obj_t	mobj)
{
	M_REFERENCE
	M_CALL(copy)(m_mobj);
	M_RELEASE
}

kern_return_t
M_DATA_REQUEST(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	M_REFERENCE
	M_CALL(data_request)(m_mobj, offset, length, desired_access);
	M_RELEASE
}

kern_return_t
M_DATA_UNLOCK(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	M_REFERENCE
	M_CALL(data_unlock)(m_mobj, offset, length, desired_access);
	M_RELEASE
}

kern_return_t
M_DATA_RETURN(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	boolean_t	dirty,
	boolean_t	kernel_copy)
{
	M_REFERENCE
	M_CALL(data_return)(m_mobj, offset, data, length, dirty, kernel_copy);
	M_RELEASE
}

kern_return_t
M_LOCK_COMPLETED(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	xmm_reply_t	reply)
{
	M_REFERENCE
	M_CALL(lock_completed)(m_mobj, offset, length, reply);
	M_RELEASE
}

kern_return_t
M_SUPPLY_COMPLETED(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	result,
	vm_offset_t	error_offset,
	xmm_reply_t	reply)
{
	M_REFERENCE
	M_CALL(supply_completed)(m_mobj, offset, length, result, error_offset,
				 reply);
	M_RELEASE
}

kern_return_t
M_CHANGE_COMPLETED(
	xmm_obj_t			mobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	xmm_reply_t			reply)
{
	M_REFERENCE
	M_CALL(change_completed)(m_mobj, may_cache, copy_strategy, reply);
	M_RELEASE
}

kern_return_t
M_SYNCHRONIZE(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_offset_t	length,
	vm_sync_t	flags)
{
	M_REFERENCE
	M_CALL(synchronize)(m_mobj, offset, length, flags);
	M_RELEASE
}

kern_return_t
M_FREEZE(
	xmm_obj_t		mobj,
	vm_external_map_t	existence_map,
	vm_offset_t		existence_size)
{
	M_REFERENCE
	M_CALL(freeze)(m_mobj, existence_map, existence_size);
	M_RELEASE
}

kern_return_t
M_SHARE(
	xmm_obj_t	mobj)
{
	M_REFERENCE
	M_CALL(share)(m_mobj);
	M_RELEASE
}

kern_return_t
M_DECLARE_PAGE(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
	M_REFERENCE
	M_CALL(declare_page)(m_mobj, offset, size);
	M_RELEASE
}

kern_return_t
M_DECLARE_PAGES(
	xmm_obj_t		mobj,
	vm_external_map_t	existence_map,
	vm_offset_t		existence_size,
	boolean_t		frozen)
{
	M_REFERENCE
	M_CALL(declare_pages)(m_mobj, existence_map, existence_size, frozen);
	M_RELEASE
}

kern_return_t
M_CACHING(
	xmm_obj_t	mobj)
{
	M_REFERENCE
	M_CALL(caching)(m_mobj);
	M_RELEASE
}

kern_return_t
M_UNCACHING(
	xmm_obj_t	mobj)
{
	M_REFERENCE
	M_CALL(uncaching)(m_mobj);
	M_RELEASE
}

#if	MACH_KDB
void
M_DEBUG(
	xmm_obj_t	mobj,
	int		flags)
{
	register xmm_obj_t m_mobj;

	m_mobj = mobj->m_mobj;
	if (m_mobj == XMM_OBJ_NULL)
		printf("m_invoke: null XMM object");
	else
		M_CALL(debug)(m_mobj, flags);
}
#endif	/* MACH_KDB */

kern_return_t
K_DATA_UNAVAILABLE(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
	K_REFERENCE
	K_CALL(data_unavailable)(k_kobj, offset, length);
	K_RELEASE
}

kern_return_t
K_GET_ATTRIBUTES(
	xmm_obj_t			kobj,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			*count)
{
	K_REFERENCE
	K_CALL(get_attributes)(k_kobj, flavor, attributes, count);
	K_RELEASE
}

kern_return_t
K_LOCK_REQUEST(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	boolean_t	should_clean,
	boolean_t	should_flush,
	vm_prot_t	lock_value,
	xmm_reply_t	reply)
{
	K_REFERENCE
	K_CALL(lock_request)(k_kobj, offset, length, should_clean,
			     should_flush, lock_value, reply);
	K_RELEASE
}

kern_return_t
K_DATA_ERROR(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	error_value)
{
	K_REFERENCE
	K_CALL(data_error)(k_kobj, offset, length, error_value);
	K_RELEASE
}

kern_return_t
K_SET_READY(
	xmm_obj_t			kobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
        vm_size_t			cluster_size,
	xmm_reply_t			reply,
	boolean_t			temporary,
	vm_external_map_t		existence_map,
	vm_offset_t			existence_size)
{
	K_REFERENCE
	K_CALL(set_ready)(k_kobj, may_cache, copy_strategy, cluster_size,
			  reply, temporary, existence_map, existence_size);
	K_RELEASE
}

kern_return_t
K_DESTROY(
	xmm_obj_t	kobj,
	kern_return_t	reason)
{
	K_REFERENCE
	K_CALL(destroy)(k_kobj, reason);
	K_RELEASE
}

kern_return_t
K_DATA_SUPPLY(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	vm_prot_t	lock_value,
	boolean_t	precious,
	xmm_reply_t	reply)
{
	K_REFERENCE
	K_CALL(data_supply)(k_kobj, offset, data, length, lock_value, precious,
			    reply);
	K_RELEASE
}

kern_return_t
K_CREATE_COPY(
	xmm_obj_t	kobj,
	ipc_port_t	new_copy_pager,
	kern_return_t	*result)
{
	K_REFERENCE
	K_CALL(create_copy)(k_kobj, new_copy_pager, result);
	K_RELEASE
}

kern_return_t
K_UNCACHING_PERMITTED(
	xmm_obj_t	kobj)
{
	K_REFERENCE
	K_CALL(uncaching_permitted)(k_kobj);
	K_RELEASE
}

kern_return_t
K_RELEASE_ALL(
	xmm_obj_t	kobj)
{
	K_REFERENCE
	K_CALL(release_all)(k_kobj);
	xmm_obj_unlink(kobj);
	K_RELEASE
}

kern_return_t
K_SYNCHRONIZE_COMPLETED(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
	K_REFERENCE
	K_CALL(synchronize_completed)(k_kobj, offset, size);
	K_RELEASE
}

void
m_interpose_deallocate(
	xmm_obj_t	mobj)
{
	assert(xmm_obj_lock_held(mobj));
	xmm_obj_unlock(mobj);
}

/*ARGSUSED*/
kern_return_t
m_invalid_init(
		xmm_obj_t	mobj,
		vm_size_t	pagesize,
		boolean_t	internal,
		vm_size_t	size)
{
	panic("invalid XMM INIT method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_terminate(
	xmm_obj_t	mobj,
	boolean_t	release)
{
	panic("invalid XMM TERMINATE method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_copy(
	xmm_obj_t	mobj)
{
	panic("invalid XMM COPY method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_data_request(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	panic("invalid XMM DATA_REQUEST method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_data_unlock(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	panic("invalid XMM DATA_UNLOCK method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_data_return(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	boolean_t	dirty,
	boolean_t	kernel_copy)
{
	panic("invalid XMM DATA_RETURN method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_lock_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	xmm_reply_t	reply)
{
	panic("invalid XMM LOCK_COMPLETED method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_supply_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	result,
	vm_offset_t	error_offset,
	xmm_reply_t	reply)
{
	panic("invalid XMM SUPPLY_COMPLETED method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_change_completed(
	xmm_obj_t			mobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	xmm_reply_t			reply)
{
	panic("invalid XMM CHANGE_COMPLETED method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_synchronize(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_offset_t	length,
	vm_sync_t	flags)
{
	panic("invalid XMM SYNCHRONIZE method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_freeze(
	xmm_obj_t		mobj,
	vm_external_map_t	existence_map,
	vm_offset_t		existence_size)
{
	panic("invalid XMM FREEZE method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_share(
	xmm_obj_t	mobj)
{
	panic("invalid XMM SHARE method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_declare_page(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
	panic("invalid XMM DECLARE_PAGE method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_declare_pages(
	xmm_obj_t		mobj,
	vm_external_map_t	existence_map,
	vm_offset_t		existence_size,
	boolean_t		frozen)
{
	panic("invalid XMM DECLARE_PAGES method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_caching(
	xmm_obj_t	mobj)
{
	panic("invalid XMM CACHING method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
m_invalid_uncaching(
	xmm_obj_t	mobj)
{
	panic("invalid XMM UNCACHING method");
	return KERN_FAILURE;
}

#if	MACH_KDB
/*ARGSUSED*/
void
m_invalid_debug(
	xmm_obj_t	mobj,
	int		flags)
{
}
#endif	/* MACH_KDB */

/*ARGSUSED*/
kern_return_t
k_invalid_data_unavailable(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
	panic("invalid XMM DATA_UNAVAILABLE method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_get_attributes(
	xmm_obj_t			kobj,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			*count)
{
	panic("invalid XMM GET_ATTRIBUTES method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_lock_request(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	boolean_t	should_clean,
	boolean_t	should_flush,
	vm_prot_t	lock_value,
	xmm_reply_t	reply)
{
	panic("invalid XMM LOCK_REQUEST method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_data_error(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	error_value)
{
	panic("invalid XMM DATA_ERROR method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_set_ready(
	xmm_obj_t			kobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
        vm_size_t			cluster_size,
	xmm_reply_t			reply,
	boolean_t			temporary,
	vm_external_map_t		existence_map,
	vm_offset_t			existence_size)
{
	panic("invalid XMM SET_READY method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_destroy(
	xmm_obj_t	kobj,
	kern_return_t	reason)
{
	panic("invalid XMM DESTROY method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_data_supply(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	vm_prot_t	lock_value,
	boolean_t	precious,
	xmm_reply_t	reply)
{
	panic("invalid XMM DATA_SUPPLY method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_create_copy(
	xmm_obj_t	kobj,
	ipc_port_t	new_copy_pager,
	kern_return_t	*result)
{
	panic("invalid XMM CREATE_COPY method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_uncaching_permitted(
	xmm_obj_t	kobj)
{
	panic("invalid XMM UNCACHING_PERMITTED method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_release_all(
	xmm_obj_t	kobj)
{
	panic("invalid XMM RELEASE_ALL method");
	return KERN_FAILURE;
}

/*ARGSUSED*/
kern_return_t
k_invalid_synchronize_completed(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
	panic("invalid XMM SYNCHRONIZE_COMPLETED method");
	return KERN_FAILURE;
}
