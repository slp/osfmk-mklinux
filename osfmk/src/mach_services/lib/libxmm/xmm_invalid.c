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
 *	File:	norma/xmm_invalid.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Definitions for invalid instances of xmm functions.
 */

#ifdef	MACH_KERNEL
#include <norma/xmm_obj.h>
#else	/* MACH_KERNEL */
#include "xmm_obj.h"
#endif	/* MACH_KERNEL */

kern_return_t m_invalid_init();
kern_return_t m_invalid_terminate();
kern_return_t m_invalid_data_request();
kern_return_t m_invalid_data_unlock();
kern_return_t m_invalid_data_write();
kern_return_t m_invalid_lock_completed();
kern_return_t m_invalid_data_return();

kern_return_t k_invalid_data_provided();
kern_return_t k_invalid_data_unavailable();
kern_return_t k_invalid_lock_request();
kern_return_t k_invalid_data_error();
kern_return_t k_invalid_set_attributes();
kern_return_t k_invalid_destroy();
kern_return_t k_invalid_data_supply();

xmm_decl(invalid_mclass,
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

	/* name			*/	"invalid",
	/* size			*/	0
);

xmm_invalid_complain(name, from, to)
	char *name;
	xmm_obj_t from;
	xmm_obj_t to;
{
	printf("m_invalid_%s from xmm_%s to xmm_%s\n",
	       name, from->k_kobj->class->c_name, to->class->c_name);
	return KERN_FAILURE;
}

/* ARGSUSED */
m_invalid_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	printf("m_invalid_init from xmm_%s to xmm_%s\n",
	       k_kobj->class->c_name, mobj->class->c_name);
	return KERN_FAILURE;
}

/* ARGSUSED */
m_invalid_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
	return xmm_invalid_complain("terminate", kobj, mobj);
}

/* ARGSUSED */
m_invalid_copy(mobj, kobj, offset, length, new_mobj)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	xmm_obj_t new_mobj;
{
	return xmm_invalid_complain("copy", kobj, mobj);
}

/* ARGSUSED */
m_invalid_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	return xmm_invalid_complain("data_request", kobj, mobj);
}

/* ARGSUSED */
m_invalid_data_unlock(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	return xmm_invalid_complain("data_unlock", kobj, mobj);
}

/* ARGSUSED */
m_invalid_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	char *data;
	int length;
{
	return xmm_invalid_complain("data_write", kobj, mobj);
}

/* ARGSUSED */
m_invalid_lock_completed(mobj, kobj, offset, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	return xmm_invalid_complain("lock_completed", kobj, mobj);
}

/* ARGSUSED */
m_invalid_supply_completed(mobj, kobj, offset, length, result, error_offset)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{
	return xmm_invalid_complain("supply_completed", kobj, mobj);
}

/* ARGSUSED */
m_invalid_data_return(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	return xmm_invalid_complain("data_return", kobj, mobj);
}

/* ARGSUSED */
k_invalid_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	return xmm_invalid_complain("data_provided", kobj, kobj);
}

/* ARGSUSED */
k_invalid_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	return xmm_invalid_complain("data_unavailable", kobj, kobj);
}

/* ARGSUSED */
k_invalid_get_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t *object_ready;
	boolean_t *may_cache;
	memory_object_copy_strategy_t *copy_strategy;
{
	return xmm_invalid_complain("get_attributes", kobj, kobj);
}

/* ARGSUSED */
k_invalid_lock_request(kobj, offset, length, should_clean, should_flush,
		     lock_value, robj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t robj;
{
	return xmm_invalid_complain("lock_request", kobj, kobj);
}

/* ARGSUSED */
k_invalid_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	return xmm_invalid_complain("data_error", kobj, kobj);
}

/* ARGSUSED */
k_invalid_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	return xmm_invalid_complain("set_attributes", kobj, kobj);
}

/* ARGSUSED */
k_invalid_destroy(kobj, reason)
	xmm_obj_t kobj;
	kern_return_t reason;
{
	return xmm_invalid_complain("destroy", kobj, kobj);
}

/* ARGSUSED */
k_invalid_data_supply(kobj, offset, data, length, dealloc_data, lock_value,
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
	return xmm_invalid_complain("data_supply", kobj, kobj);
}
