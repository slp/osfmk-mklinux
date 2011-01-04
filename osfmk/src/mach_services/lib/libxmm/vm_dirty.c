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
 *	File:	vm_dirty.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Routines for vm allocation and deallocation.
 */

#include <mach.h>
#include "vm_dirty.h"

#if 0
static vm_offset_t dirty_page = 0;

vm_allocate_dirty(page)
	vm_offset_t *page;
{
	if (dirty_page) {
		*page = dirty_page;
		dirty_page = 0;
		return KERN_SUCCESS;
	} else {
		return vm_allocate(mach_task_self(), page, vm_page_size, TRUE);
	}
}

vm_deallocate_dirty(page)
	vm_offset_t page;
{
	if (dirty_page == 0) {
		dirty_page = page;
		return KERN_SUCCESS;
	} else {
		return vm_deallocate(mach_task_self(), page, vm_page_size);
	}
}
#else
vm_allocate_dirty(page)
	vm_offset_t *page;
{
	kern_return_t kr;

	kr = vm_allocate(mach_task_self(), page, vm_page_size, TRUE);
	if (kr != KERN_SUCCESS) {
		panic("vm_allocate_dirty: %x/%d", kr, kr);
	}
	return KERN_SUCCESS;
}

vm_deallocate_dirty(page)
	vm_offset_t page;
{
	kern_return_t kr;

	kr = vm_deallocate(mach_task_self(), page, vm_page_size);
	if (kr != KERN_SUCCESS) {
		panic("vm_deallocate_dirty: %x/%d", kr, kr);
	}
	return KERN_SUCCESS;
}
#endif
