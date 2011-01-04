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
 *	Joseph Barrera (jsb) at Carnegie-Mellon University 11-Sep-92
 *	Created from xmm_obj.h. Renamed data_write_return to data_return.
 *	Added freeze, share, declare_page, declare_pages, caching, uncaching,
 *	create_copy, and uncaching_permitted methods. Moved interpose routine
 *	definitions here. 
 *
 *	File:	norma/xmm_methods.h
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Common method-dependent definitions for xmm system.
 */
/* CMU_ENDHIST */

#include <mach_kdb.h>
#include <mach/vm_sync.h>
#include <xmm/xmm_obj.h>

struct xmm_class {
	kern_return_t	(*m_init)(
				xmm_obj_t	mobj,
				vm_size_t	pagesize,
				boolean_t	internal,
				vm_size_t	size);
	kern_return_t	(*m_terminate)(
				xmm_obj_t	mobj,
				boolean_t	release);
	void		(*m_deallocate)(
				xmm_obj_t	mobj);
	kern_return_t	(*m_copy)(
				xmm_obj_t	mobj);
	kern_return_t	(*m_data_request)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				vm_prot_t	desired_access);
	kern_return_t	(*m_data_unlock)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				vm_prot_t	desired_access);
	kern_return_t	(*m_data_return)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_map_copy_t	data,
				vm_size_t	length,
				boolean_t	dirty,
				boolean_t	kernel_copy);
	kern_return_t	(*m_lock_completed)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				xmm_reply_t	reply);
	kern_return_t	(*m_supply_completed)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				kern_return_t	result,
				vm_offset_t	error_offset,
				xmm_reply_t	reply);
	kern_return_t	(*m_change_completed)(
				xmm_obj_t	mobj,
				boolean_t	may_cache,
				memory_object_copy_strategy_t	copy_strategy,
				xmm_reply_t	reply);
	kern_return_t	(*m_synchronize)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_offset_t	length,
				vm_sync_t	flags);
	kern_return_t	(*m_freeze)(
				xmm_obj_t	mobj,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size);
	kern_return_t	(*m_share)(
				xmm_obj_t	mobj);
	kern_return_t	(*m_declare_page)(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	size);
	kern_return_t	(*m_declare_pages)(
				xmm_obj_t	mobj,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size,
				boolean_t	frozen);
	kern_return_t	(*m_caching)(
				xmm_obj_t	mobj);
	kern_return_t	(*m_uncaching)(
				xmm_obj_t	mobj);
#if	MACH_KDB
	void		(*m_debug)(
				xmm_obj_t	mobj,
				int		flags);
#endif	/* MACH_KDB */

	kern_return_t	(*k_data_unavailable)(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length);
	kern_return_t	(*k_get_attributes)(
				xmm_obj_t		kobj,
				memory_object_flavor_t	flavor,
				memory_object_info_t	attributes,
				unsigned		*count);
	kern_return_t	(*k_lock_request)(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length,
				boolean_t	should_clean,
				boolean_t	should_flush,
				vm_prot_t	lock_value,
				xmm_reply_t	reply);
	kern_return_t	(*k_data_error)(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length,
				kern_return_t	error_value);
	kern_return_t	(*k_set_ready)(
				xmm_obj_t	kobj,
				boolean_t	may_cache,
				memory_object_copy_strategy_t	copy_strategy,
        			vm_size_t	cluster_size,
				xmm_reply_t	reply,
				boolean_t	temporary,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size);
	kern_return_t	(*k_destroy)(
				xmm_obj_t	kobj,
				kern_return_t	reason);
	kern_return_t	(*k_data_supply)(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_map_copy_t	data,
				vm_size_t	length,
				vm_prot_t	lock_value,
				boolean_t	precious,
				xmm_reply_t	reply);
	kern_return_t	(*k_create_copy)(
				xmm_obj_t	kobj,
				ipc_port_t	new_copy_pager,
				kern_return_t	*result);
	kern_return_t	(*k_uncaching_permitted)(
				xmm_obj_t	kobj);
	kern_return_t	(*k_release_all)(
				xmm_obj_t	kobj);
	kern_return_t	(*k_synchronize_completed)(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	size);

	char *		c_name;
	int		c_size;
	zone_t		c_zone;
};

/*
 * The following large MACH_KDB define allows definitions of
 * the class structures w/ or w/o debug methods.  Due to the
 * semantics of cpp, we are stuck with this gross way of doing it.
 */
#if	MACH_KDB
#define xmm_decl_prototypes(class)				\
extern kern_return_t m_ ## class ## _init(			\
		xmm_obj_t	mobj,				\
		vm_size_t	pagesize,			\
		boolean_t	internal,			\
		vm_size_t	size);				\
extern kern_return_t m_ ## class ## _terminate(			\
		xmm_obj_t	mobj,				\
		boolean_t	release);			\
extern void	     m_ ## class ## _deallocate(		\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _copy(			\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _data_request(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		vm_prot_t	desired_access);		\
extern kern_return_t m_ ## class ## _data_unlock(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		vm_prot_t	desired_access);		\
extern kern_return_t m_ ## class ## _data_return(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_map_copy_t	data,				\
		vm_size_t	length,				\
		boolean_t	dirty,				\
		boolean_t	kernel_copy);			\
extern kern_return_t m_ ## class ## _lock_completed(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		xmm_reply_t	reply);				\
extern kern_return_t m_ ## class ## _supply_completed(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		kern_return_t	result,				\
		vm_offset_t	error_offset,			\
		xmm_reply_t	reply);				\
extern kern_return_t m_ ## class ## _change_completed(		\
		xmm_obj_t	mobj,				\
		boolean_t	may_cache,			\
		memory_object_copy_strategy_t	copy_strategy,	\
		xmm_reply_t	reply);				\
extern kern_return_t m_ ## class ## _synchronize(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_offset_t	length,				\
		vm_sync_t	flags);				\
extern kern_return_t m_ ## class ## _freeze(			\
		xmm_obj_t	mobj,				\
		vm_external_map_t existence_map,		\
		vm_offset_t	existence_size);		\
extern kern_return_t m_ ## class ## _share(			\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _declare_page(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	size);				\
extern kern_return_t m_ ## class ## _declare_pages(		\
		xmm_obj_t	mobj,				\
		vm_external_map_t existence_map,		\
		vm_offset_t	existence_size,			\
		boolean_t	frozen);			\
extern kern_return_t m_ ## class ## _caching(			\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _uncaching(			\
		xmm_obj_t	mobj);				\
extern void          m_ ## class ## _debug(			\
		xmm_obj_t	mobj,				\
		int		flags);				\
extern kern_return_t k_ ## class ## _data_unavailable(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length);			\
extern kern_return_t k_ ## class ## _get_attributes(		\
		xmm_obj_t		kobj,			\
		memory_object_flavor_t	flavor,			\
		memory_object_info_t	attributes,		\
		unsigned		*count);		\
extern kern_return_t k_ ## class ## _lock_request(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		boolean_t	should_clean,			\
		boolean_t	should_flush,			\
		vm_prot_t	lock_value,			\
		xmm_reply_t	reply);				\
extern kern_return_t k_ ## class ## _data_error(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		kern_return_t	error_value);			\
extern kern_return_t k_ ## class ## _set_ready(			\
		xmm_obj_t	kobj,				\
		boolean_t	may_cache,			\
		memory_object_copy_strategy_t	copy_strategy,	\
        	vm_size_t	cluster_size,			\
		xmm_reply_t	reply,				\
		boolean_t	temporary,			\
		vm_external_map_t existence_map,		\
		vm_offset_t	existence_size);		\
extern kern_return_t k_ ## class ## _destroy(			\
		xmm_obj_t	kobj,				\
		kern_return_t	reason);			\
extern kern_return_t k_ ## class ## _data_supply(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_map_copy_t	data,				\
		vm_size_t	length,				\
		vm_prot_t	lock_value,			\
		boolean_t	precious,			\
		xmm_reply_t	reply);				\
extern kern_return_t k_ ## class ## _create_copy(		\
		xmm_obj_t	kobj,				\
		ipc_port_t	new_copy_pager,			\
		kern_return_t	*result);			\
extern kern_return_t k_ ## class ## _uncaching_permitted(	\
		xmm_obj_t	kobj);				\
extern kern_return_t k_ ## class ## _release_all(		\
		xmm_obj_t	kobj);				\
extern kern_return_t k_ ## class ## _synchronize_completed(	\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	size);

#define xmm_decl(class, name, size)				\
xmm_decl_prototypes(class)					\
struct xmm_class class ## _class = {				\
	m_ ## class ## _init,					\
	m_ ## class ## _terminate,				\
	m_ ## class ## _deallocate,				\
	m_ ## class ## _copy,					\
	m_ ## class ## _data_request,				\
	m_ ## class ## _data_unlock,				\
	m_ ## class ## _data_return,				\
	m_ ## class ## _lock_completed,				\
	m_ ## class ## _supply_completed,			\
	m_ ## class ## _change_completed,			\
	m_ ## class ## _synchronize,				\
	m_ ## class ## _freeze,					\
	m_ ## class ## _share,					\
	m_ ## class ## _declare_page,				\
	m_ ## class ## _declare_pages,				\
	m_ ## class ## _caching,				\
	m_ ## class ## _uncaching,				\
	m_ ## class ## _debug,					\
								\
	k_ ## class ## _data_unavailable,			\
	k_ ## class ## _get_attributes,				\
	k_ ## class ## _lock_request,				\
	k_ ## class ## _data_error,				\
	k_ ## class ## _set_ready,				\
	k_ ## class ## _destroy,				\
	k_ ## class ## _data_supply,				\
	k_ ## class ## _create_copy,				\
	k_ ## class ## _uncaching_permitted,			\
	k_ ## class ## _release_all,				\
	k_ ## class ## _synchronize_completed,			\
								\
	name,							\
	size,							\
	ZONE_NULL,						\
}

#else /* MACH_KDB */

#define xmm_decl_prototypes(class)				\
extern kern_return_t m_ ## class ## _init(			\
		xmm_obj_t	mobj,				\
		vm_size_t	pagesize,			\
		boolean_t	internal,			\
		vm_size_t	size);				\
extern kern_return_t m_ ## class ## _terminate(			\
		xmm_obj_t	mobj,				\
		boolean_t	release);			\
extern void	     m_ ## class ## _deallocate(		\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _copy(			\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _data_request(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		vm_prot_t	desired_access);		\
extern kern_return_t m_ ## class ## _data_unlock(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		vm_prot_t	desired_access);		\
extern kern_return_t m_ ## class ## _data_return(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_map_copy_t	data,				\
		vm_size_t	length,				\
		boolean_t	dirty,				\
		boolean_t	kernel_copy);			\
extern kern_return_t m_ ## class ## _lock_completed(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		xmm_reply_t	reply);				\
extern kern_return_t m_ ## class ## _supply_completed(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		kern_return_t	result,				\
		vm_offset_t	error_offset,			\
		xmm_reply_t	reply);				\
extern kern_return_t m_ ## class ## _change_completed(		\
		xmm_obj_t	mobj,				\
		boolean_t	may_cache,			\
		memory_object_copy_strategy_t	copy_strategy,	\
		xmm_reply_t	reply);				\
extern kern_return_t m_ ## class ## _synchronize(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_offset_t	length,				\
		vm_sync_t	flags);				\
extern kern_return_t m_ ## class ## _freeze(			\
		xmm_obj_t	mobj,				\
		vm_external_map_t existence_map,		\
		vm_offset_t	existence_size);		\
extern kern_return_t m_ ## class ## _share(			\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _declare_page(		\
		xmm_obj_t	mobj,				\
		vm_offset_t	offset,				\
		vm_size_t	size);				\
extern kern_return_t m_ ## class ## _declare_pages(		\
		xmm_obj_t	mobj,				\
		vm_external_map_t existence_map,		\
		vm_offset_t	existence_size,			\
		boolean_t	frozen);			\
extern kern_return_t m_ ## class ## _caching(			\
		xmm_obj_t	mobj);				\
extern kern_return_t m_ ## class ## _uncaching(			\
		xmm_obj_t	mobj);				\
extern kern_return_t k_ ## class ## _data_unavailable(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length);			\
extern kern_return_t k_ ## class ## _get_attributes(		\
		xmm_obj_t		kobj,			\
		memory_object_flavor_t	flavor,			\
		memory_object_info_t	attributes,		\
		unsigned		*count);		\
extern kern_return_t k_ ## class ## _lock_request(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		boolean_t	should_clean,			\
		boolean_t	should_flush,			\
		vm_prot_t	lock_value,			\
		xmm_reply_t	reply);				\
extern kern_return_t k_ ## class ## _data_error(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	length,				\
		kern_return_t	error_value);			\
extern kern_return_t k_ ## class ## _set_ready(			\
		xmm_obj_t	kobj,				\
		boolean_t	may_cache,			\
		memory_object_copy_strategy_t	copy_strategy,	\
        	vm_size_t	cluster_size,			\
		xmm_reply_t	reply,				\
		boolean_t	temporary,			\
		vm_external_map_t existence_map,		\
		vm_offset_t	existence_size);		\
extern kern_return_t k_ ## class ## _destroy(			\
		xmm_obj_t	kobj,				\
		kern_return_t	reason);			\
extern kern_return_t k_ ## class ## _data_supply(		\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_map_copy_t	data,				\
		vm_size_t	length,				\
		vm_prot_t	lock_value,			\
		boolean_t	precious,			\
		xmm_reply_t	reply);				\
extern kern_return_t k_ ## class ## _create_copy(		\
		xmm_obj_t	kobj,				\
		ipc_port_t	new_copy_pager,			\
		kern_return_t	*result);			\
extern kern_return_t k_ ## class ## _uncaching_permitted(	\
		xmm_obj_t	kobj);				\
extern kern_return_t k_ ## class ## _release_all(		\
		xmm_obj_t	kobj);				\
extern kern_return_t k_ ## class ## _synchronize_completed(	\
		xmm_obj_t	kobj,				\
		vm_offset_t	offset,				\
		vm_size_t	size);

#define xmm_decl(class, name, size)				\
xmm_decl_prototypes(class)					\
struct xmm_class class ## _class = {				\
	m_ ## class ## _init,					\
	m_ ## class ## _terminate,				\
	m_ ## class ## _deallocate,				\
	m_ ## class ## _copy,					\
	m_ ## class ## _data_request,				\
	m_ ## class ## _data_unlock,				\
	m_ ## class ## _data_return,				\
	m_ ## class ## _lock_completed,				\
	m_ ## class ## _supply_completed,			\
	m_ ## class ## _change_completed,			\
	m_ ## class ## _synchronize,				\
	m_ ## class ## _freeze,					\
	m_ ## class ## _share,					\
	m_ ## class ## _declare_page,				\
	m_ ## class ## _declare_pages,				\
	m_ ## class ## _caching,				\
	m_ ## class ## _uncaching,				\
								\
	k_ ## class ## _data_unavailable,			\
	k_ ## class ## _get_attributes,				\
	k_ ## class ## _lock_request,				\
	k_ ## class ## _data_error,				\
	k_ ## class ## _set_ready,				\
	k_ ## class ## _destroy,				\
	k_ ## class ## _data_supply,				\
	k_ ## class ## _create_copy,				\
	k_ ## class ## _uncaching_permitted,			\
	k_ ## class ## _release_all,				\
	k_ ## class ## _synchronize_completed,			\
								\
	name,							\
	size,							\
	ZONE_NULL,						\
}

#endif /* MACH_KDB */

extern kern_return_t	M_INIT(
				xmm_obj_t	mobj,
				vm_size_t	pagesize,
				boolean_t	internal,
				vm_size_t	size);

extern kern_return_t	M_TERMINATE(
				xmm_obj_t	mobj,
				boolean_t	release);

extern kern_return_t	M_COPY(
				xmm_obj_t	mobj);

extern kern_return_t	M_DATA_REQUEST(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				vm_prot_t	desired_access);

extern kern_return_t	M_DATA_UNLOCK(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				vm_prot_t	desired_access);

extern kern_return_t	M_DATA_RETURN(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_map_copy_t	data,
				vm_size_t	length,
				boolean_t	dirty,
				boolean_t	kernel_copy);

extern kern_return_t	M_LOCK_COMPLETED(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				xmm_reply_t	reply);

extern kern_return_t	M_SUPPLY_COMPLETED(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				kern_return_t	result,
				vm_offset_t	error_offset,
				xmm_reply_t	reply);

extern kern_return_t	M_CHANGE_COMPLETED(
				xmm_obj_t	mobj,
				boolean_t	may_cache,
				memory_object_copy_strategy_t	copy_strategy,
				xmm_reply_t	reply);

extern kern_return_t   M_SYNCHRONIZE(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_offset_t	length,
				vm_sync_t	flags);

extern kern_return_t	M_FREEZE(
				xmm_obj_t	mobj,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size);

extern kern_return_t	M_SHARE(
				xmm_obj_t	mobj);

extern kern_return_t	M_DECLARE_PAGE(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	size);

extern kern_return_t	M_DECLARE_PAGES(
				xmm_obj_t	mobj,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size,
				boolean_t	frozen);

extern kern_return_t	M_CACHING(
				xmm_obj_t	mobj);

extern kern_return_t	M_UNCACHING(
				xmm_obj_t	mobj);
#if	MACH_KDB
extern void		M_DEBUG(
				xmm_obj_t	mobj,
				int		flags);
#endif	/* MACH_KDB */
extern kern_return_t	K_DATA_UNAVAILABLE(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length);

extern kern_return_t	K_GET_ATTRIBUTES(
				xmm_obj_t		kobj,
				memory_object_flavor_t	flavor,
				memory_object_info_t	attributes,
				unsigned		*count);

extern kern_return_t	K_LOCK_REQUEST(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length,
				boolean_t	should_clean,
				boolean_t	should_flush,
				vm_prot_t	lock_value,
				xmm_reply_t	reply);

extern kern_return_t	K_DATA_ERROR(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length,
				kern_return_t	error_value);

extern kern_return_t	K_SET_READY(
				xmm_obj_t	kobj,
				boolean_t	may_cache,
				memory_object_copy_strategy_t	copy_strategy,
				vm_size_t	cluster_size,
				xmm_reply_t	reply,
				boolean_t	temporary,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size);

extern kern_return_t	K_DESTROY(
				xmm_obj_t	kobj,
				kern_return_t	reason);

extern kern_return_t	K_DATA_SUPPLY(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_map_copy_t	data,
				vm_size_t	length,
				vm_prot_t	lock_value,
				boolean_t	precious,
				xmm_reply_t	reply);

extern kern_return_t	K_CREATE_COPY(
				xmm_obj_t	kobj,
				ipc_port_t	new_copy_pager,
				kern_return_t	*result);

extern kern_return_t	K_UNCACHING_PERMITTED(
				xmm_obj_t	kobj);

extern kern_return_t	K_RELEASE_ALL(
				xmm_obj_t	kobj);

extern kern_return_t	K_DATA_WRITE_COMPLETED(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	size);

extern kern_return_t   K_SYNCHRONIZE_COMPLETED(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	size);

#define	m_interpose_init			M_INIT
#define	m_interpose_terminate			M_TERMINATE
#define	m_interpose_copy			M_COPY
#define	m_interpose_data_request		M_DATA_REQUEST
#define	m_interpose_data_unlock			M_DATA_UNLOCK
#define	m_interpose_data_return			M_DATA_RETURN
#define	m_interpose_lock_completed		M_LOCK_COMPLETED
#define	m_interpose_supply_completed		M_SUPPLY_COMPLETED
#define	m_interpose_change_completed		M_CHANGE_COMPLETED
#define	m_interpose_synchronize			M_SYNCHRONIZE
#define	m_interpose_freeze			M_FREEZE
#define	m_interpose_share			M_SHARE
#define	m_interpose_declare_page		M_DECLARE_PAGE
#define	m_interpose_declare_pages		M_DECLARE_PAGES
#define	m_interpose_caching			M_CACHING
#define	m_interpose_uncaching			M_UNCACHING
#if	MACH_KDB
#define	m_interpose_debug			M_DEBUG
#endif	/* MACH_KDB */
#define	k_interpose_data_unavailable		K_DATA_UNAVAILABLE
#define	k_interpose_get_attributes		K_GET_ATTRIBUTES
#define	k_interpose_lock_request		K_LOCK_REQUEST
#define	k_interpose_data_error			K_DATA_ERROR
#define	k_interpose_set_ready			K_SET_READY
#define	k_interpose_destroy			K_DESTROY
#define	k_interpose_data_supply			K_DATA_SUPPLY
#define	k_interpose_create_copy			K_CREATE_COPY
#define	k_interpose_uncaching_permitted		K_UNCACHING_PERMITTED
#define	k_interpose_release_all			K_RELEASE_ALL
#define	k_interpose_synchronize_completed	K_SYNCHRONIZE_COMPLETED

extern void		m_interpose_deallocate(
				xmm_obj_t	mobj);

extern kern_return_t	m_invalid_init(
				xmm_obj_t	mobj,
				vm_size_t	pagesize,
				boolean_t	internal,
				vm_size_t	size);

extern kern_return_t	m_invalid_terminate(
				xmm_obj_t	mobj,
				boolean_t	release);

extern kern_return_t	m_invalid_copy(
				xmm_obj_t	mobj);

extern kern_return_t	m_invalid_data_request(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				vm_prot_t	desired_access);

extern kern_return_t	m_invalid_data_unlock(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				vm_prot_t	desired_access);

extern kern_return_t	m_invalid_data_return(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_map_copy_t	data,
				vm_size_t	length,
				boolean_t	dirty,
				boolean_t	kernel_copy);

extern kern_return_t	m_invalid_lock_completed(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				xmm_reply_t	reply);

extern kern_return_t	m_invalid_supply_completed(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	length,
				kern_return_t	result,
				vm_offset_t	error_offset,
				xmm_reply_t	reply);

extern kern_return_t	m_invalid_change_completed(
				xmm_obj_t			mobj,
				boolean_t			may_cache,
				memory_object_copy_strategy_t	copy_strategy,
				xmm_reply_t			reply);

extern kern_return_t	m_invalid_synchronize(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_offset_t	length,
				vm_sync_t	flags);

extern kern_return_t	m_invalid_freeze(
				xmm_obj_t	mobj,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size);

extern kern_return_t	m_invalid_share(
				xmm_obj_t	mobj);

extern kern_return_t	m_invalid_declare_page(
				xmm_obj_t	mobj,
				vm_offset_t	offset,
				vm_size_t	size);

extern kern_return_t	m_invalid_declare_pages(
				xmm_obj_t	mobj,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size,
				boolean_t	frozen);

extern kern_return_t	m_invalid_caching(
				xmm_obj_t	mobj);

extern kern_return_t	m_invalid_uncaching(
				xmm_obj_t	mobj);

#if	MACH_KDB
extern void		m_invalid_debug(
				xmm_obj_t	mobj,
				int		flags);
#endif	/* MACH_KDB */

extern kern_return_t	k_invalid_data_unavailable(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length);

extern kern_return_t	k_invalid_get_attributes(
				xmm_obj_t		kobj,
				memory_object_flavor_t	flavor,
				memory_object_info_t	attributes,
				unsigned		*count);

extern kern_return_t	k_invalid_lock_request(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length,
				boolean_t	should_clean,
				boolean_t	should_flush,
				vm_prot_t	lock_value,
				xmm_reply_t	reply);

extern kern_return_t	k_invalid_data_error(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	length,
				kern_return_t	error_value);

extern kern_return_t	k_invalid_set_ready(
				xmm_obj_t	kobj,
				boolean_t	may_cache,
				memory_object_copy_strategy_t	copy_strategy,
        			vm_size_t	cluster_size,
				xmm_reply_t	reply,
				boolean_t	temporary,
				vm_external_map_t	existence_map,
				vm_offset_t	existence_size);

extern kern_return_t	k_invalid_destroy(
				xmm_obj_t	kobj,
				kern_return_t	reason);

extern kern_return_t	k_invalid_data_supply(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_map_copy_t	data,
				vm_size_t	length,
				vm_prot_t	lock_value,
				boolean_t	precious,
				xmm_reply_t	reply);

extern kern_return_t	k_invalid_create_copy(
				xmm_obj_t	kobj,
				ipc_port_t	new_copy_pager,
				kern_return_t	*result);

extern kern_return_t	k_invalid_uncaching_permitted(
				xmm_obj_t	kobj);

extern kern_return_t	k_invalid_release_all(
				xmm_obj_t	kobj);

extern kern_return_t	k_invalid_synchronize_completed(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_size_t	size);
