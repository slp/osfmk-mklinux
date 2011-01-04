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
 *	File:	atrium.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer which converts memory_object messages into xmm calls.
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

kern_return_t k_atrium_data_provided();
kern_return_t k_atrium_data_unavailable();
kern_return_t k_atrium_get_attributes();
kern_return_t k_atrium_lock_request();
kern_return_t k_atrium_data_error();
kern_return_t k_atrium_set_attributes();
kern_return_t k_atrium_destroy();
kern_return_t k_atrium_data_supply();

xmm_decl(atrium_mclass,
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

	/* name			*/	"atrium.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(atrium_kclass,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_atrium_data_provided,
	/* k_data_unavailable	*/	k_atrium_data_unavailable,
	/* k_get_attributes	*/	k_atrium_get_attributes,
	/* k_lock_request	*/	k_atrium_lock_request,
	/* k_data_error		*/	k_atrium_data_error,
	/* k_set_attributes	*/	k_atrium_set_attributes,
	/* k_destroy		*/	k_atrium_destroy,
	/* k_data_supply	*/	k_atrium_data_supply,

	/* name			*/	"atrium.k",
	/* size			*/	sizeof(struct kobj)
);

/* XXX should be two different hash tables? */
xmm_hash_t	obj_hash;
#define		obj_lookup(port) \
		((xmm_obj_t) xmm_hash_lookup(obj_hash, (unsigned long) port))

/* XXX stupid name */
obj_init()
{
	obj_hash = xmm_hash_allocate(256);
	Proxy_init();/*XXX*/
}

k_atrium_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	kern_return_t kr;

	kr = memory_object_data_provided(KOBJ->memory_control, offset, data,
					 length, lock_value);
	vm_deallocate_dirty(data);
	return kr;
}

k_atrium_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	kern_return_t kr;

	kr = memory_object_data_unavailable(KOBJ->memory_control, offset,
					    length);
	return kr;
}

k_atrium_get_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t *object_ready;
	boolean_t *may_cache;
	memory_object_copy_strategy_t *copy_strategy;
{
	kern_return_t kr;

	kr = memory_object_get_attributes(KOBJ->memory_control, object_ready,
					  may_cache, copy_strategy);
	return kr;
}

k_atrium_lock_request(kobj, offset, length, should_clean, should_flush,
		      lock_value, mobj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t mobj;
{
	kern_return_t kr;

	/*
	 * XXX There is a general problem with K_LOCK_REQUEST.
	 * XXX So far, it looks like memory managers can only specify
	 * XXX their own memory objects (mobjs) as reply objects.
	 */
	kr = memory_object_lock_request(KOBJ->memory_control, offset, length,
					should_clean, should_flush, lock_value,
					(mobj
					 ? MOBJ->memory_object
					 : MACH_PORT_NULL));
	return kr;
}

k_atrium_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	kern_return_t kr;

	kr = memory_object_data_error(KOBJ->memory_control, offset, length,
				      error_value);
	return kr;
}

k_atrium_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	kern_return_t kr;

	kr = memory_object_set_attributes(KOBJ->memory_control, object_ready,
					  may_cache, copy_strategy);
	return kr;
}

k_atrium_destroy(kobj, reason)
	xmm_obj_t kobj;
	kern_return_t reason;
{
	kern_return_t kr;

	kr = memory_object_destroy(KOBJ->memory_control, reason);
	return kr;
}

k_atrium_data_supply(kobj, offset, data, length, dealloc_data, lock_value,
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
	kern_return_t kr;

	kr = memory_object_data_supply(KOBJ->memory_control, offset, data,
				       length, dealloc_data, lock_value,
				       precious, reply_to);
	return kr;
}

/*
 * Translate messages sent to memory objects into calls on mobjs.
 */

kernel_obj_lookup(memory_object, memory_control, mobj, kobj)
	mach_port_t memory_object;
	mach_port_t memory_control;
	xmm_obj_t *mobj;
	xmm_obj_t *kobj;
{
	*mobj = obj_lookup(memory_object);
	if (*mobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	*kobj = obj_lookup(memory_control);
	if (*kobj == XMM_OBJ_NULL) {
		return KERN_FAILURE;
	}
	return KERN_SUCCESS;
}

kern_return_t
xmm_memory_object_create(old_mobj, memory_object)
	xmm_obj_t old_mobj;
	mach_port_t *memory_object;
{
	kern_return_t kr;
	xmm_obj_t mobj;

	kr = xmm_obj_allocate(&atrium_mclass, old_mobj, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	kr = mach_port_get(&MOBJ->memory_object);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	xmm_hash_enqueue(obj_hash, (void *) mobj,
		     (unsigned long) MOBJ->memory_object);
	*memory_object = MOBJ->memory_object;
	return KERN_SUCCESS;
}

memory_object_init(memory_object, memory_control, memory_object_name,
		   page_size)
	mach_port_t memory_object;
	mach_port_t memory_control;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	mobj = obj_lookup(memory_object);
	if (mobj == XMM_OBJ_NULL) {
		printf("atrium: bad memory object\n");
		return KERN_FAILURE;
	}
	kobj = obj_lookup(memory_control);
	if (kobj) {
		printf("atrium: multiple inits with same memory_control\n");
		return KERN_FAILURE;
	}
	kr = xmm_obj_allocate(&atrium_kclass, XMM_OBJ_NULL, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	KOBJ->memory_control = memory_control;
	xmm_hash_enqueue(obj_hash, (void *) kobj,
			 (unsigned long) memory_control);

	M_INIT(mobj, kobj, memory_object_name, page_size);

	return KERN_SUCCESS;
}

memory_object_terminate(memory_object, memory_control, memory_object_name)
	mach_port_t memory_object;
	mach_port_t memory_control;
	mach_port_t memory_object_name;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_TERMINATE(mobj, kobj, memory_object_name);
	/* XXX garbage collection */
	return KERN_SUCCESS;
}

/* ARGSUSED */
memory_object_copy(memory_object, memory_control, offset, length,
		   new_memory_object)
	memory_object_t memory_object;
	memory_object_control_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	memory_object_t new_memory_object;
{
#if 1
	return KERN_FAILURE;
#else
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	xmm_obj_t nobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_COPY(mobj, kobj, offset, length, &nobj);
	/* XXX ? */
#endif
}

memory_object_data_request(memory_object, memory_control, offset, length,
			   desired_access)
	mach_port_t memory_object, memory_control;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	M_DATA_REQUEST(mobj, kobj, offset, length, desired_access);
	return KERN_SUCCESS;
}

memory_object_data_unlock(memory_object, memory_control, offset, length,
			  desired_access)
	mach_port_t memory_object, memory_control;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_DATA_UNLOCK(mobj, kobj, offset, length, desired_access);
	return KERN_SUCCESS;
}

memory_object_data_write(memory_object, memory_control, offset, data, length)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	char *data;
	int length;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_DATA_WRITE(mobj, kobj, offset, data, length);
	return KERN_SUCCESS;
}

memory_object_lock_completed(memory_object, memory_control, offset, length)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_LOCK_COMPLETED(mobj, kobj, offset, length);
	return KERN_SUCCESS;
}

memory_object_supply_completed(memory_object, memory_control, offset, length,
			       result, error_offset)
	memory_object_t memory_object;
	memory_object_control_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_SUPPLY_COMPLETED(mobj, kobj, offset, length, result, error_offset);
	return KERN_SUCCESS;
}

memory_object_data_return(memory_object, memory_control, offset, data, length)
	memory_object_t memory_object;
	memory_object_control_t memory_control;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, memory_control, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_DATA_RETURN(mobj, kobj, offset, data, length);
	return KERN_SUCCESS;
}

memory_object_change_completed(memory_object, may_cache, copy_strategy)
	memory_object_t memory_object;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = kernel_obj_lookup(memory_object, MACH_PORT_NULL, &mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	M_CHANGE_COMPLETED(mobj, kobj, may_cache, copy_strategy);
	return KERN_SUCCESS;
}
