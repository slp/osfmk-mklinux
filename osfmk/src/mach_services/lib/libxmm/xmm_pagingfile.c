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
 *	File:	xmm_pagingfile.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class which pages to supplied open file.
 */

#include <sys/file.h>
#include <mach.h>
#include "xmm_obj.h"
#include "vm_dirty.h"

struct mobj {
	struct xmm_obj	obj;
	int		fd;
	int		max_page;
	int		cur_page;
};

kern_return_t m_pagingfile_init();
kern_return_t m_pagingfile_terminate();
kern_return_t m_pagingfile_data_request();
kern_return_t m_pagingfile_data_write();

xmm_decl(pagingfile_class,
	/* m_init		*/	m_pagingfile_init,
	/* m_terminate		*/	m_pagingfile_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_pagingfile_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_pagingfile_data_write,
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

	/* name			*/	"pagingfile",
	/* size			*/	sizeof(struct mobj)
);

kern_return_t
xmm_pagingfile_create(fd, new_mobj)
	int fd;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&pagingfile_class, XMM_OBJ_NULL, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->fd = fd;
	MOBJ->max_page = -1;
	MOBJ->cur_page = 0;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

m_pagingfile_init(mobj, k_kobj, memory_object_name, page_size)
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

m_pagingfile_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
#if     lint
	memory_object_name++;
#endif  /* lint */
	ftruncate(MOBJ->fd, 0);
	MOBJ->max_page = -1;
	MOBJ->cur_page = 0;
/*	kobj->k_kobj = XMM_OBJ_NULL;*/
}

m_pagingfile_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	int page, rv;
	vm_offset_t data;
	kern_return_t kr;

#if     lint
	desired_access++;
#endif  /* lint */
	page = atop(offset);
	if (page < 0) {
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);
		return;
	}
	if (page > MOBJ->max_page) {
		K_DATA_UNAVAILABLE(kobj, offset, length);
		return;
	}
	/* XXX should keep track of holes */
	kr = vm_allocate_dirty(&data);
	if (kr != KERN_SUCCESS) {
		K_DATA_ERROR(kobj, offset, length, kr);
		return;
	}
	if (page != MOBJ->cur_page) {
		if (lseek(MOBJ->fd, offset, 0) < 0) {
			K_DATA_ERROR(kobj, offset, length,
				     KERN_FAILURE);
			return;
		}
		MOBJ->cur_page = page;
	}
	rv = read(MOBJ->fd, (char *) data, vm_page_size);
	if (rv != vm_page_size) {
		perror("read");
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);
		return;
	}
	MOBJ->cur_page++;
	K_DATA_PROVIDED(kobj, offset, data, length, VM_PROT_NONE);
	vm_deallocate_dirty(data);
}

m_pagingfile_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	int page, rv;

	page = atop(offset);
	if (page < 0) {
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);
		return;
	}
	if (page > MOBJ->max_page) {
		MOBJ->max_page = page;
	}
	/* XXX should keep track of holes */
	if (page != MOBJ->cur_page) {
		if (lseek(MOBJ->fd, offset, 0) < 0) {
			K_DATA_ERROR(kobj, offset, length,
				     KERN_FAILURE);
		}
		MOBJ->cur_page = page;
	}
	rv = write(MOBJ->fd, (char *) data, vm_page_size);
	if (rv != vm_page_size) {
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);/* ??? */
		perror("write");
		printf("*** m_pagingfile_data_write: error\n");
		return;
	}
	MOBJ->cur_page++;
	vm_deallocate_dirty(data);
}
