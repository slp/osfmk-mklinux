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
 *	File:	proxy.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class for talking to remote memory manager.
 */

#include <mach.h>
#include "xmm_obj.h"

struct mobj {
	struct xmm_obj	obj;
	mach_port_t	memory_object;
};

struct kobj {
	struct xmm_obj	obj;
	mach_port_t	memory_control;
};

kern_return_t m_proxy_init();
kern_return_t m_proxy_terminate();
kern_return_t m_proxy_copy();
kern_return_t m_proxy_data_request();
kern_return_t m_proxy_data_unlock();
kern_return_t m_proxy_data_write();
kern_return_t m_proxy_lock_completed();
kern_return_t m_proxy_supply_completed();
kern_return_t m_proxy_data_return();

xmm_decl(proxy_mclass,
	/* m_init		*/	m_proxy_init,
	/* m_terminate		*/	m_proxy_terminate,
	/* m_copy		*/	m_proxy_copy,
	/* m_data_request	*/	m_proxy_data_request,
	/* m_data_unlock	*/	m_proxy_data_unlock,
	/* m_data_write		*/	m_proxy_data_write,
	/* m_lock_completed	*/	m_proxy_lock_completed,
	/* m_supply_completed	*/	m_proxy_supply_completed,
	/* m_data_return	*/	m_proxy_data_return,

	/* k_data_provided	*/	k_invalid_data_provided,
	/* k_data_unavailable	*/	k_invalid_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_invalid_data_error,
	/* k_set_attributes	*/	k_invalid_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"proxy.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(proxy_kclass,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_invalid_data_provided,
	/* k_data_unavailable	*/	k_invalid_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_invalid_data_error,
	/* k_set_attributes	*/	k_invalid_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"proxy.k",
	/* size			*/	sizeof(struct kobj)
);

xmm_hash_t	proxy_memory_object_hash;
xmm_hash_t	proxy_memory_control_hash;

/* XXX stupid name */
Proxy_init()
{
	proxy_memory_object_hash = xmm_hash_allocate(256);
	proxy_memory_control_hash = xmm_hash_allocate(256);
}

xmm_obj_t
proxy_kobj_by_memory_control(memory_control)
	mach_port_t memory_control;
{
	return (xmm_obj_t) xmm_hash_lookup(proxy_memory_object_hash,
				       (unsigned long) memory_control);
}

xmm_obj_t
proxy_mobj_by_memory_object(memory_object)
	mach_port_t memory_object;
{
	xmm_obj_t mobj;

	if (! memory_object) {
		return XMM_OBJ_NULL;
	}
	return (xmm_obj_t) xmm_hash_lookup(proxy_memory_control_hash,
				       (unsigned long) memory_object);
}

kern_return_t
xmm_proxy_create(memory_object, new_mobj)
	mach_port_t memory_object;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&proxy_mclass, XMM_OBJ_NULL, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->memory_object = memory_object;
	xmm_hash_enqueue(proxy_memory_control_hash,
			 (void *) mobj,(unsigned long) memory_object);
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

m_proxy_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	kern_return_t kr;
	xmm_obj_t kobj;

	xmm_obj_allocate(&proxy_kclass, XMM_OBJ_NULL, &kobj);
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;

	kr = mach_port_get(&KOBJ->memory_control);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	xmm_hash_enqueue(proxy_memory_object_hash, (void *) kobj,
			 (unsigned long) KOBJ->memory_control);

	/* XXX return value? */
	proxy_init(MOBJ->memory_object,
		   KOBJ->memory_control,
		   memory_object_name,
		   page_size);
	return KERN_SUCCESS;
}

m_proxy_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
	proxy_terminate(MOBJ->memory_object,
			KOBJ->memory_control,
			memory_object_name);
	/* XXX garbage collection */
}

m_proxy_copy(mobj, kobj, offset, length, new_memory_object)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	mach_port_t new_memory_object;
{
	proxy_copy(MOBJ->memory_object,
		   KOBJ->memory_control,
		   offset,
		   length,
		   new_memory_object);
}

m_proxy_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	proxy_data_request(MOBJ->memory_object,
			   KOBJ->memory_control,
			   offset,
			   length,
			   desired_access);
}

m_proxy_data_unlock(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	proxy_data_unlock(MOBJ->memory_object,
			  KOBJ->memory_control,
			  offset,
			  length,
			  desired_access);
}

m_proxy_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	int length;
{
	proxy_data_write(MOBJ->memory_object,
			 KOBJ->memory_control,
			 offset,
			 data,
			 length);
}

m_proxy_lock_completed(mobj, kobj, offset, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	proxy_lock_completed(MOBJ->memory_object,
			     KOBJ->memory_control,
			     offset,
			     length);
}

m_proxy_supply_completed(mobj, kobj, offset, length, result, error_offset)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{
	proxy_supply_completed(MOBJ->memory_object,
			       KOBJ->memory_control,
			       offset,
			       length,
			       result,
			       error_offset);
}

m_proxy_data_return(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	proxy_data_return(MOBJ->memory_object,
			  KOBJ->memory_control,
			  offset,
			  data,
			  length);
}

proxy_data_provided(memory_control, offset, data, length, lock_value)
	mach_port_t	memory_control;
	vm_offset_t	offset;
	pointer_t	data;
	unsigned int	length;
	vm_prot_t	lock_value;
{
	xmm_obj_t kobj;

	kobj = proxy_kobj_by_memory_control(memory_control);
	K_DATA_PROVIDED(kobj, offset, data, length, lock_value);
}

proxy_data_unavailable(memory_control, offset, length)
	mach_port_t	memory_control;
	vm_offset_t	offset;
	vm_size_t	length;
{
	xmm_obj_t kobj;

	kobj = proxy_kobj_by_memory_control(memory_control);
	K_DATA_UNAVAILABLE(kobj, offset, length);
}

proxy_get_attributes(memory_control, object_ready, may_cache, copy_strategy)
	mach_port_t	memory_control;
	boolean_t	*object_ready;
	boolean_t	*may_cache;
	memory_object_copy_strategy_t *copy_strategy;
{
	xmm_obj_t kobj;

	kobj = proxy_kobj_by_memory_control(memory_control);
	K_GET_ATTRIBUTES(kobj, object_ready, may_cache, copy_strategy);
}

proxy_lock_request(memory_control, offset, length, should_clean, should_flush,
		   prot, reply_to, reply_to_type)
	mach_port_t	memory_control;
	vm_offset_t	offset;
	vm_size_t	length;
	boolean_t	should_clean;
	boolean_t	should_flush;
	vm_prot_t	prot;
	mach_port_t	reply_to;
	/*mach_msg_type_name_t*/int	reply_to_type;
{
	xmm_obj_t kobj;
	xmm_obj_t robj;

#if     lint
	reply_to_type++;
#endif  /* lint */
	kobj = proxy_kobj_by_memory_control(memory_control);
	robj = proxy_mobj_by_memory_object(reply_to);	/* XXX */
	K_LOCK_REQUEST(kobj, offset, length, should_clean, should_flush,
		       prot, robj);
}

proxy_data_error(memory_control, offset, length, error_value)
	mach_port_t	memory_control;
	vm_offset_t	offset;
	vm_size_t	length;
	kern_return_t	error_value;
{
	xmm_obj_t kobj;

	kobj = proxy_kobj_by_memory_control(memory_control);
	K_DATA_ERROR(kobj, offset, length, error_value);
}

proxy_set_attributes(memory_control, object_ready, may_cache, copy_strategy)
	mach_port_t	memory_control;
	boolean_t	object_ready;
	boolean_t	may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	xmm_obj_t kobj;

	kobj = proxy_kobj_by_memory_control(memory_control);
	K_SET_ATTRIBUTES(kobj, object_ready, may_cache, copy_strategy);
}

proxy_destroy(memory_control, reason)
	mach_port_t	memory_control;
	kern_return_t	reason;
{
	xmm_obj_t kobj;

	kobj = proxy_kobj_by_memory_control(memory_control);
	K_DESTROY(kobj, reason);
}
