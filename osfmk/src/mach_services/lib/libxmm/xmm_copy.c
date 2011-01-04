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
 *	File:	xmm_copy.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class for write access backed by read-only data.
 */

#include <mach.h>
#include "xmm_obj.h"

struct mobj {
	struct xmm_obj	obj;
	xmm_obj_t	sobj;
	int		unready;
	int		num_pages;
	boolean_t	*shadowed;	/* which pages are in shadow obj */
};

#undef	KOBJ
#define	KOBJ	((struct mobj *) kobj)

kern_return_t m_copy_init();
kern_return_t m_copy_terminate();
kern_return_t m_copy_data_request();
kern_return_t m_copy_data_write();
kern_return_t k_copy_data_provided();
kern_return_t k_copy_data_unavailable();
kern_return_t k_copy_data_error();
kern_return_t k_copy_set_attributes();

xmm_decl(copy_mclass,
	/* m_init		*/	m_copy_init,
	/* m_terminate		*/	m_copy_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_copy_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_copy_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_copy_data_provided,
	/* k_data_unavailable	*/	k_copy_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_copy_data_error,
	/* k_set_attributes	*/	k_copy_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"copy.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(copy_sclass,
	/* m_init		*/	m_copy_init,
	/* m_terminate		*/	m_copy_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_copy_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_copy_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_copy_data_provided,
	/* k_data_unavailable	*/	k_copy_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_copy_data_error,
	/* k_set_attributes	*/	k_copy_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"copy.k",
	/* size			*/	sizeof(struct xmm_obj)
);

kern_return_t
m_copy_create(copied, sobj, size, new_mobj)
	xmm_obj_t copied;
	xmm_obj_t sobj;
	vm_size_t size;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&copy_mclass, copied, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	kr = xmm_obj_allocate(&copy_sclass, sobj, &sobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->sobj = sobj;
	MOBJ->unready = 2;	/* one for shadow, one for copied */
	MOBJ->num_pages = atop(round_page(size));
	MOBJ->shadowed = (boolean_t *) calloc(MOBJ->num_pages,
					      sizeof(boolean_t));
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

kern_return_t
m_copy_init(mobj, k_kobj, pager_name, page_size)
	xmm_obj_t	mobj;
	xmm_obj_t	k_kobj;
	mach_port_t	pager_name;
	vm_size_t	page_size;
{
	if (mobj->k_kobj) {
		printf("m_copy_init: multiple users\n");
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}
	if (page_size != vm_page_size) {
		printf("m_copy_init: wrong page size\n");
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}
	k_kobj->m_kobj = mobj;
	mobj->k_kobj = k_kobj;
	M_INIT(mobj, mobj, pager_name, page_size);
	M_INIT(MOBJ->sobj, mobj, pager_name, page_size);
	return KERN_SUCCESS;
}

kern_return_t
m_copy_terminate(mobj, kobj, pager_name)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	mach_port_t	pager_name;
{
#if     lint
	kobj++;
#endif  /* lint */
	/*
	 * As with all my other terminates, a lot more should happen here.
	 */
	M_TERMINATE(MOBJ->sobj, mobj, pager_name);
	M_TERMINATE(mobj, mobj, pager_name);
	return KERN_SUCCESS;
}

kern_return_t
m_copy_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	vm_offset_t	offset;
	vm_size_t	length;
	vm_prot_t	desired_access;
{
	int page = atop(offset);

#if     lint
	kobj++;
	desired_access++;
#endif  /* lint */
	if (page >= MOBJ->num_pages) {
		printf("Grrr\n");
		return KERN_FAILURE;
	}
	if (MOBJ->shadowed[page]) {
		M_DATA_REQUEST(MOBJ->sobj, mobj, offset, length,
			       VM_PROT_ALL);
	} else {
		M_DATA_REQUEST(mobj, mobj, offset, length, VM_PROT_ALL);
	}
	return KERN_SUCCESS;
}

kern_return_t
m_copy_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	vm_offset_t	offset;
	vm_offset_t	data;
	vm_size_t	length;
{
	int page = atop(offset);

#if     lint
	kobj++;
#endif  /* lint */
	if (page >= MOBJ->num_pages) {
		printf("Grrr\n");
		return KERN_FAILURE;
	}
	MOBJ->shadowed[page] = TRUE;
	M_DATA_WRITE(MOBJ->sobj, mobj, offset, data, length);
	return KERN_SUCCESS;
}

k_copy_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
#if     lint
	lock_value++;
#endif  /* lint */
	K_DATA_PROVIDED(kobj, offset, data, length, VM_PROT_NONE);
}

k_copy_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	K_DATA_UNAVAILABLE(kobj, offset, length);
}

k_copy_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	K_DATA_ERROR(kobj, offset, length, error_value);
}

k_copy_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	if (object_ready && --KOBJ->unready == 0) {
		K_SET_ATTRIBUTES(kobj, TRUE, may_cache, copy_strategy);
	}
	return KERN_SUCCESS;
}
