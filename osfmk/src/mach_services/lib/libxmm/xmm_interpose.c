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
 * MkLinux
 */
/*
 *	File:	norma/xmm_interpose.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Definitions for null instances of xmm functions.
 */

#ifdef	MACH_KERNEL
#include <norma/xmm_obj.h>
#else	/* MACH_KERNEL */
#include "xmm_obj.h"
#endif	/* MACH_KERNEL */

xmm_decl(interpose_class,
	/* m_init		*/	m_interpose_init,
	/* m_terminate		*/	m_interpose_terminate,
	/* m_copy		*/	m_interpose_copy,
	/* m_data_request	*/	m_interpose_data_request,
	/* m_data_unlock	*/	m_interpose_data_unlock,
	/* m_data_write		*/	m_interpose_data_write,
	/* m_lock_completed	*/	m_interpose_lock_completed,
	/* m_supply_completed	*/	m_interpose_supply_completed,
	/* m_data_return	*/	m_interpose_data_return,

	/* k_data_provided	*/	k_interpose_data_provided,
	/* k_data_unavailable	*/	k_interpose_data_unavailable,
	/* k_get_attributes	*/	k_interpose_get_attributes,
	/* k_lock_request	*/	k_interpose_lock_request,
	/* k_data_error		*/	k_interpose_data_error,
	/* k_set_attributes	*/	k_interpose_set_attributes,
	/* k_destroy		*/	k_interpose_destroy,
	/* k_data_supply	*/	k_interpose_data_supply,

	/* name			*/	"interpose",
	/* size			*/	sizeof(struct xmm_obj)
);

kern_return_t
xmm_interpose_create(old_mobj, new_mobj)
	xmm_obj_t old_mobj;
	xmm_obj_t *new_mobj;
{
	return xmm_obj_allocate(&interpose_class, old_mobj, new_mobj);
}

m_interpose_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	xmm_obj_t kobj = mobj;

	/* XXX check for more than one kobj? */
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;
	return M_INIT(mobj, kobj, memory_object_name, page_size);
}

m_interpose_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
	kern_return_t kr;

	kr = M_TERMINATE(mobj, kobj, memory_object_name);
	xmm_obj_deallocate(mobj);
	return kr;
}

m_interpose_copy(mobj, kobj, offset, length, new_mobj)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	xmm_obj_t new_mobj;
{
	return M_COPY(mobj, kobj, offset, length, new_mobj);
}

m_interpose_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	return M_DATA_REQUEST(mobj, kobj, offset, length, desired_access);
}

m_interpose_data_unlock(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	return M_DATA_UNLOCK(mobj, kobj, offset, length, desired_access);
}

m_interpose_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	char *data;
	int length;
{
	return M_DATA_WRITE(mobj, kobj, offset, data, length);
}

m_interpose_lock_completed(mobj, kobj, offset, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	return M_LOCK_COMPLETED(mobj, kobj, offset, length);
}

m_interpose_supply_completed(mobj, kobj, offset, length, result, error_offset)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{
	return M_SUPPLY_COMPLETED(mobj, kobj, offset, length, result,
				  error_offset);
}

m_interpose_data_return(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	return M_DATA_RETURN(mobj, kobj, offset, data, length);
}

k_interpose_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	return K_DATA_PROVIDED(kobj, offset, data, length, lock_value);
}

k_interpose_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	return K_DATA_UNAVAILABLE(kobj, offset, length);
}

k_interpose_get_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t *object_ready;
	boolean_t *may_cache;
	memory_object_copy_strategy_t *copy_strategy;
{
	return K_GET_ATTRIBUTES(kobj, object_ready, may_cache, copy_strategy);
}

k_interpose_lock_request(kobj, offset, length, should_clean, should_flush,
			 lock_value, mobj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t mobj;
{
	return K_LOCK_REQUEST(kobj, offset, length, should_clean, should_flush,
			      lock_value, mobj);
}

k_interpose_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	return K_DATA_ERROR(kobj, offset, length, error_value);
}

k_interpose_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	return K_SET_ATTRIBUTES(kobj, object_ready, may_cache, copy_strategy);
}

k_interpose_destroy(kobj, reason)
	xmm_obj_t kobj;
	kern_return_t reason;
{
	return K_DESTROY(kobj, reason);
}

k_interpose_data_supply(kobj, offset, data, length, dealloc_data, lock_value,
			precious, reply_to)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	boolean_t dealloc_data;
	vm_prot_t lock_value;
	boolean_t precious;
	mach_port_t reply_to;
{
	return K_DATA_SUPPLY(kobj, offset, data, length, dealloc_data,
			     lock_value, precious, reply_to);
}
