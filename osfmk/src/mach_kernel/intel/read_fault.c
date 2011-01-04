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
 * Revision 2.2.6.1  92/02/18  19:05:34  jeffreyh
 * 	Include thread.h when compiling with VM_OBJECT_DEBUG
 * 	[91/09/26  04:47:28  bernadat]
 * 
 * Revision 2.2  91/06/06  16:57:48  jsb
 * 	First checkin as intel/read_fault.c.
 * 
 * Revision 2.7  91/05/18  14:30:09  rpd
 * 	Picked up software_reference_bits-related changes from vm_fault.
 * 	[91/04/06            rpd]
 * 	Added VM_FAULT_FICTITIOUS_SHORTAGE.
 * 	[91/03/29            rpd]
 * 
 * Revision 2.6  91/05/14  16:15:46  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/03/16  14:45:13  rpd
 * 	Added resume, continuation arguments to vm_fault_page.
 * 	Added continuation argument to VM_PAGE_WAIT.
 * 	[91/02/05            rpd]
 * 
 * Revision 2.4  91/02/05  17:14:21  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:37:39  mrt]
 * 
 * Revision 2.3  91/01/08  15:11:05  rpd
 * 	Changed VM_WAIT to VM_PAGE_WAIT.
 * 	[90/12/11            rpd]
 * 
 * Revision 2.2  90/05/03  15:37:20  dbg
 * 	Created.
 * 	[90/04/05            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */

#include <kern/thread.h>
#include <vm/vm_fault.h>
#include <mach/kern_return.h>
#include <mach/vm_behavior.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>

#include <intel/intel.h>

#include <kern/macro_help.h>

/*
 *	Expansion of vm_fault for read fault in kernel mode.
 *	Must enter the mapping as writable, since the i386
 *	(and i860 in i386 compatability mode) ignores write
 *	protection in kernel mode.
 *
 *	Note that this routine can be called for pmap's other
 *	than the kernel_pmap, in which case it just enters
 *	a read-only mapping.  (See e.g. kernel_trap().)
 */
kern_return_t
intel_read_fault(
	vm_map_t	map,
	vm_offset_t	vaddr)
{
	vm_map_version_t	version;	/* Map version for
						   verification */
	vm_object_t		object;		/* Top-level object */
	vm_offset_t		offset;		/* Top-level offset */
	vm_prot_t		prot;		/* Protection for mapping */
	vm_behavior_t		behavior;	/* Expected paging behavior */
	vm_offset_t		lo_offset, hi_offset;
	vm_page_t		result_page;	/* Result of vm_fault_page */
	vm_page_t		top_page;	/* Placeholder page */
	boolean_t		wired;		/* Is map region wired? */
	kern_return_t		result;
	register vm_page_t	m;

    RetryFault:

	/*
	 *	Find the backing store object and offset into it
	 *	to begin search.
	 */
	vm_map_lock_read(map);
	result = vm_map_lookup_locked(&map, vaddr, VM_PROT_READ, &version,
				      &object, &offset, &prot, &wired,
				      &behavior, &lo_offset, &hi_offset);
	vm_map_unlock_read(map);

	if (result != KERN_SUCCESS)
	    return (result);

	/*
	 *	Make a reference to this object to prevent its
	 *	disposal while we are playing with it.
	 */
	assert(object->ref_count > 0);
	object->ref_count++;
	vm_object_res_reference(object);
	vm_object_paging_begin(object);

	result = vm_fault_page(object, offset, VM_PROT_READ, FALSE, TRUE,
			       lo_offset, hi_offset, behavior,
			       &prot, &result_page, &top_page,
			       0, map->no_zero_fill, FALSE);

	if (result != VM_FAULT_SUCCESS) {
	    vm_object_deallocate(object);

	    switch (result) {
		case VM_FAULT_RETRY:
		    goto RetryFault;
		case VM_FAULT_INTERRUPTED:
		    return (KERN_SUCCESS);
		case VM_FAULT_MEMORY_SHORTAGE:
		    VM_PAGE_WAIT();
		    goto RetryFault;
		case VM_FAULT_FICTITIOUS_SHORTAGE:
		    vm_page_more_fictitious();
		    goto RetryFault;
		case VM_FAULT_MEMORY_ERROR:
		    return (KERN_MEMORY_ERROR);
	    }
	}

	m = result_page;

	/*
	 *	How to clean up the result of vm_fault_page.  This
	 *	happens whether the mapping is entered or not.
	 */

#define UNLOCK_AND_DEALLOCATE				\
	MACRO_BEGIN					\
	vm_fault_cleanup(m->object, top_page);		\
	vm_object_deallocate(object);			\
	MACRO_END

	/*
	 *	What to do with the resulting page from vm_fault_page
	 *	if it doesn't get entered into the physical map:
	 */

#define RELEASE_PAGE(m)					\
	MACRO_BEGIN					\
	PAGE_WAKEUP_DONE(m);				\
	vm_page_lock_queues();				\
	if (!m->active && !m->inactive)			\
		vm_page_activate(m);			\
	vm_page_unlock_queues();			\
	MACRO_END

	/*
	 *	We must verify that the maps have not changed.
	 */
	vm_object_unlock(m->object);
	while (!vm_map_verify(map, &version)) {
	    vm_object_t		retry_object;
	    vm_offset_t		retry_offset;
	    vm_prot_t		retry_prot;

	    vm_map_lock_read(map);
	    result = vm_map_lookup_locked(&map, vaddr, VM_PROT_READ, &version,
				&retry_object, &retry_offset, &retry_prot,
				&wired, &behavior, &lo_offset, &hi_offset);
	    vm_map_unlock_read(map);

	    if (result != KERN_SUCCESS) {
		vm_object_lock(m->object);
		RELEASE_PAGE(m);
		UNLOCK_AND_DEALLOCATE;
		return (result);
	    }

	    vm_object_unlock(retry_object);

	    if (retry_object != object || retry_offset != offset) {
		vm_object_lock(m->object);
		RELEASE_PAGE(m);
		UNLOCK_AND_DEALLOCATE;
		goto RetryFault;
	    }
	}

	/*
	 *	Put the page in the physical map.
	 */
	PMAP_ENTER(map->pmap, vaddr, m, VM_PROT_READ, wired);

	vm_object_lock(m->object);
	vm_page_lock_queues();
	if (!m->active && !m->inactive)
		vm_page_activate(m);
	m->reference = TRUE;
	vm_page_unlock_queues();

	vm_map_verify_done(map, &version);
	PAGE_WAKEUP_DONE(m);

	UNLOCK_AND_DEALLOCATE;

#undef	UNLOCK_AND_DEALLOCATE
#undef	RELEASE_PAGE

	return (KERN_SUCCESS);
}

