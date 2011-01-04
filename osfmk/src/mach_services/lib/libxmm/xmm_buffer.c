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
 *	File:	xmm_buffer.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class buffering a small amount of data_written data.
 */

#include <mach.h>
#include "xmm_obj.h"

#define	DEBUG	0

typedef struct page	*page_t;
typedef struct buffer	*buffer_t;

struct buffer {
	int		max_page_count;
	int		page_count;
	page_t		page_list;
};

struct page {
	xmm_obj_t	mobj;
	vm_offset_t	offset;
	vm_offset_t	data;
	page_t		next;
};

struct mobj {
	struct xmm_obj	obj;
	buffer_t	buffer;
	boolean_t	ready;
};

#undef	KOBJ
#define	KOBJ	((struct mobj *) kobj)

kern_return_t m_buffer_init();
kern_return_t m_buffer_terminate();
kern_return_t m_buffer_data_request();
kern_return_t m_buffer_data_write();
kern_return_t k_buffer_set_attributes();

xmm_decl(buffer_class,
	/* m_init		*/	m_buffer_init,
	/* m_terminate		*/	m_buffer_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_buffer_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_buffer_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_interpose_data_provided,
	/* k_data_unavailable	*/	k_interpose_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_interpose_data_error,
	/* k_set_attributes	*/	k_buffer_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"buffer",
	/* size			*/	sizeof(struct mobj)
);

kern_return_t
xmm_buffer_allocate(max_page_count, buffer_result)
	int max_page_count;
	void **buffer_result;
{
	buffer_t buffer;

	buffer = (buffer_t) kalloc(sizeof(*buffer));
	if (! buffer) {
		return KERN_FAILURE;
	}
	buffer->max_page_count = max_page_count;
	buffer->page_count = 0;
	buffer->page_list = (page_t) 0;
	*buffer_result = (void *) buffer;
	return KERN_SUCCESS;
}

kern_return_t
xmm_buffer_create(old_mobj, buffer, new_mobj)
	xmm_obj_t old_mobj;
	void *buffer;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;
	
	kr = xmm_obj_allocate(&buffer_class, old_mobj, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->ready = FALSE;
	MOBJ->buffer = (buffer_t) buffer;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

kern_return_t
m_buffer_init(mobj, k_kobj, pager_name, page_size)
	xmm_obj_t	mobj;
	xmm_obj_t	k_kobj;
	mach_port_t	pager_name;
	vm_size_t	page_size;
{
	if (mobj->k_kobj) {
		printf("m_buffer_init: multiple users\n");
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}
	if (page_size != vm_page_size) {
		printf("m_buffer_init: wrong page size\n");
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}
	k_kobj->m_kobj = mobj;
	mobj->k_kobj = k_kobj;
	M_INIT(mobj, mobj, pager_name, page_size);
	return KERN_SUCCESS;
}

kern_return_t
m_buffer_terminate(mobj, kobj, pager_name)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	mach_port_t	pager_name;
{
	page_t page, *pp;

#if     lint
	kobj++;
#endif  /* lint */
	/*
	 * As with all my other terminates, a lot more should happen here.
	 */
#if	DEBUG
	printf("xmm_buffer_terminate\n");
	printf("page_count was %d\n", MOBJ->buffer->page_count);
#endif
	for (pp = &MOBJ->buffer->page_list; page = *pp; ) {
		if (page->mobj == mobj) {
			M_DATA_WRITE(mobj, mobj, page->offset, page->data,
				     vm_page_size);
#if	DEBUG
			printf("dealloc 0x%x data 0x%x\n",
			       page->offset, page->data);
#endif
			*pp = page->next;
			free(page);
			MOBJ->buffer->page_count--;
		} else {
			pp = &page->next;
		}
	}
#if	DEBUG
	printf("page_count now %d\n", MOBJ->buffer->page_count);
#endif
	M_TERMINATE(mobj, mobj, pager_name);
}

kern_return_t
m_buffer_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	vm_offset_t	offset;
	vm_size_t	length;
	vm_prot_t	desired_access;
{
	page_t page, *pp;
	kern_return_t kr;
	vm_offset_t data;
	vm_size_t data_count;

	for (pp = &MOBJ->buffer->page_list; page = *pp; pp = &page->next) {
		if (page->mobj == mobj && page->offset == offset) {
			/*
			 * Move page to front (most recently used)
			 */
			*pp = page->next;
			page->next = MOBJ->buffer->page_list;
			MOBJ->buffer->page_list = page;
			/*
			 * This copy is unfortunate and could be avoided
			 * perhaps if vm_dirty were more clever, by for
			 * example doing invisible reference counting
			 */
			kr = vm_read(mach_task_self(), page->data, length,
				     &data, &data_count);
			if (kr != KERN_SUCCESS) {
				mach_error("xmm_buffer: vm_read", kr);
				K_DATA_ERROR(kobj, offset, length, kr);
				return kr;
			}
			K_DATA_PROVIDED(kobj, offset, data, length,
					VM_PROT_NONE);
			return KERN_SUCCESS;
		}
	}
	M_DATA_REQUEST(mobj, mobj, offset, length, desired_access);
	return KERN_FAILURE;
}

kern_return_t
m_buffer_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	vm_offset_t	offset;
	vm_offset_t	data;
	vm_size_t	length;
{
	page_t page, *pp;

#if     lint
	kobj++;
	length++;
#endif  /* lint */
	for (pp = &MOBJ->buffer->page_list; page = *pp; pp = &page->next) {
		if (page->mobj == mobj && page->offset == offset) {
			vm_deallocate_dirty(page->data);
			*pp = page->next;
			break;
		}
	}
	if (! page) {
		page = (page_t) kalloc(sizeof(*page));
		page->mobj = mobj;
		page->offset = offset;
		MOBJ->buffer->page_count++;
	}
	page->data = data;
	page->next = MOBJ->buffer->page_list;
	MOBJ->buffer->page_list = page;

	if (MOBJ->buffer->page_count > MOBJ->buffer->max_page_count) {
		for (page = MOBJ->buffer->page_list;
		     page->next->next;
		     page = page->next) {
			continue;
		}
		pp = &page->next;
		page = page->next;
		*pp = (page_t) 0;
		MOBJ->buffer->page_count--;
		/*
		 * This is probably synchronous, which is unfortunate,
		 * since this is the only place we write pages down.
		 */
		M_DATA_WRITE(mobj, mobj, page->offset, page->data,
			     vm_page_size);
		free(page);
	}
#if	DEBUG
	printf("** page_count = %d\n", MOBJ->buffer->page_count);
#endif
	return KERN_SUCCESS;
}

k_buffer_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	if (object_ready && ! KOBJ->ready) {
		KOBJ->ready = TRUE;
		K_SET_ATTRIBUTES(kobj, TRUE, may_cache, copy_strategy);
	}
	return KERN_SUCCESS;
}
