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
 *	File:	xmm_vm.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class providing data from supplied piece of vm.
 */

#include <mach.h>
#include "xmm_obj.h"

struct mobj {
	struct xmm_obj	obj;
	vm_offset_t	data;
	vm_offset_t	length;
};

kern_return_t m_vm_init();
kern_return_t m_vm_terminate();
kern_return_t m_vm_data_request();

xmm_decl(vm_class,
	/* m_init		*/	m_vm_init,
	/* m_terminate		*/	m_vm_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_vm_data_request,
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

	/* name			*/	"vm",
	/* size			*/	sizeof(struct mobj)
);

kern_return_t
xmm_vm_create(data, length, new_mobj)
	vm_offset_t data;
	vm_size_t length;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&vm_class, XMM_OBJ_NULL, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->data = data;
	MOBJ->length = length;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

m_vm_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	xmm_obj_t kobj = mobj;

#if     lint
	memory_object_name++;
#endif  /* lint */
	if (kobj->k_kobj) {
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return;
	}
	if (page_size != vm_page_size) {
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return;
	}
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;
	K_SET_ATTRIBUTES(kobj, TRUE, FALSE, MEMORY_OBJECT_COPY_DELAY);
}

m_vm_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
#if     lint
	mobj++;
	kobj++;
	memory_object_name++;
#endif  /* lint */
}

m_vm_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
#if     lint
	desired_access++;
#endif  /* lint */
	if (offset + length > MOBJ->length) {
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);
	} else {
		K_DATA_PROVIDED(kobj, offset, MOBJ->data + offset, length,
				VM_PROT_WRITE);
	}
}
