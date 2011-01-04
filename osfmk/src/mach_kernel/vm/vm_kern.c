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
 * Revision 2.17.6.5  92/09/15  17:36:17  jeffreyh
 * 	Add assertions to guarantee that vm_map_copies have the
 * 	expected copy object page list type.
 * 	[92/06/02            alanl]
 * 
 * Revision 2.17.6.4  92/06/24  18:04:01  jeffreyh
 * 	Removed flush for the iPSC860 after pmap was changed to solve
 * 	problem.
 * 	[92/06/10            jeffreyh]
 * 
 * Revision 2.17.6.3  92/05/29  11:48:42  jeffreyh
 * 	Add a flush for the iPSC860 in kmem_remap_pages
 * 
 * Revision 2.17.6.2  92/03/28  10:16:31  jeffreyh
 * 	19-Mar-92 David L. Black (dlb) at Open Software Foundation
 * 	Change kmem_io_map_copyout to handle multiple page lists.
 * 	Don't call pmap_change_wiring from kmem_io_map_deallocate.
 * 	[92/03/20  14:13:50  jeffreyh]
 * 
 * Revision 2.17.6.1  92/03/03  16:26:10  jeffreyh
 * 	kmem_io_map_deallocate() now calls pmap_remove() -- VM
 * 	code optimization makes this necessary.  Optimized
 * 	out kmem_alloc_pageable() call from kmem_io_map_copyout().
 * 	[92/01/07  16:37:38  dlb]
 * 	Changes from TRUNK
 * 	[92/02/26  12:34:14  jeffreyh]
 * 
 * Revision 2.18  92/01/14  16:47:57  rpd
 * 	Added copyinmap and copyoutmap.
 * 	[91/12/16            rpd]
 * 
 * Revision 2.17  91/08/28  11:18:00  jsb
 * 	Delete kmem_fault_wire, io_wire, io_unwire - Replaced by
 * 	kmem_io_{copyout,deallocate}.
 * 	[91/08/06  17:17:40  dlb]
 * 
 * 	Make kmem_io_map_deallocate return void.
 * 	[91/08/05  17:45:39  dlb]
 * 
 * 	New interfaces for kmem_io_map routines.
 * 	[91/08/02  17:07:40  dlb]
 * 
 * 	New and improved io wiring support based on vm page lists:
 * 	kmem_io_map_{copyout,deallocate}.  io_wire and io_unwire will
 * 	go away when the device logic fully supports this.
 * 	[91/07/31  15:12:02  dlb]
 * 
 * Revision 2.16  91/07/30  15:47:20  rvb
 * 	Fixed io_wire to allocate an object when the entry doesn't have one.
 * 	[91/06/27            rpd]
 * 
 * Revision 2.15  91/05/18  14:40:31  rpd
 * 	Added kmem_alloc_aligned.
 * 	[91/05/02            rpd]
 * 	Added VM_FAULT_FICTITIOUS_SHORTAGE.
 * 	Revised vm_map_find_entry to allow coalescing of entries.
 * 	Fixed deadlock problem in kmem_alloc.
 * 	[91/03/29            rpd]
 * 	Fixed kmem_init to not create a zero-size entry.
 * 	[91/03/25            rpd]
 * 
 * Revision 2.14  91/05/14  17:49:15  mrt
 * 	Correcting copyright
 * 
 * Revision 2.13  91/03/16  15:05:20  rpd
 * 	Fixed kmem_alloc_pages and kmem_remap_pages
 * 	to not hold locks across pmap_enter.
 * 	[91/03/11            rpd]
 * 	Added kmem_realloc.  Changed kmem_alloc, kmem_alloc_wired, and
 * 	kmem_alloc_pageable to return error codes.  Changed kmem_alloc
 * 	to not use the kernel object and to not zero memory.
 * 	Changed kmem_alloc_wired to use the kernel object.
 * 	[91/03/07  16:49:52  rpd]
 * 
 * 	Added resume, continuation arguments to vm_fault_page.
 * 	Added continuation argument to VM_PAGE_WAIT.
 * 	[91/02/05            rpd]
 * 
 * Revision 2.12  91/02/05  17:58:22  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:32:19  mrt]
 * 
 * Revision 2.11  91/01/08  16:44:59  rpd
 * 	Changed VM_WAIT to VM_PAGE_WAIT.
 * 	[90/11/13            rpd]
 * 
 * Revision 2.10  90/10/12  13:05:35  rpd
 * 	Only activate the page returned by vm_fault_page if it isn't
 * 	already on a pageout queue.
 * 	[90/10/09  22:33:09  rpd]
 * 
 * Revision 2.9  90/06/19  23:01:54  rpd
 * 	Picked up vm_submap_object.
 * 	[90/06/08            rpd]
 * 
 * Revision 2.8  90/06/02  15:10:43  rpd
 * 	Purged MACH_XP_FPD.
 * 	[90/03/26  23:12:33  rpd]
 * 
 * Revision 2.7  90/02/22  20:05:39  dbg
 * 	Update to vm_map.h.
 * 	Remove kmem_alloc_wait, kmem_free_wakeup, vm_move.
 * 	Fix copy_user_to_physical_page to test for kernel tasks.
 * 	Simplify v_to_p allocation.
 * 	Change PAGE_WAKEUP to PAGE_WAKEUP_DONE to reflect the
 * 	fact that it clears the busy flag.
 * 	[90/01/25            dbg]
 * 
 * Revision 2.6  90/01/22  23:09:12  af
 * 	Undone VM_PROT_DEFAULT change, moved to vm_prot.h
 * 	[90/01/20  17:28:57  af]
 * 
 * Revision 2.5  90/01/19  14:35:57  rwd
 * 	Get new version from rfr
 * 	[90/01/10            rwd]
 * 
 * Revision 2.4  90/01/11  11:47:44  dbg
 * 	Remove kmem_mb_alloc and mb_map.
 * 	[89/12/11            dbg]
 * 
 * Revision 2.3  89/11/29  14:17:43  af
 * 	Redefine VM_PROT_DEFAULT locally for mips.
 * 	Might migrate in the final place sometimes.
 * 
 * Revision 2.2  89/09/08  11:28:19  dbg
 * 	Add special wiring code for IO memory.
 * 	[89/08/10            dbg]
 * 
 * 	Add keep_wired argument to vm_move.
 * 	[89/07/14            dbg]
 * 
 * 28-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	Changes for MACH_KERNEL:
 *	. Optimize kmem_alloc.  Add kmem_alloc_wired.
 *	. Remove non-MACH include files.
 *	. Change vm_move to call vm_map_move.
 *	. Clean up fast_pager_data option.
 *
 * Revision 2.14  89/04/22  15:35:28  gm0w
 * 	Added code in kmem_mb_alloc to verify that requested allocation
 * 	will fit in the map.
 * 	[89/04/14            gm0w]
 * 
 * Revision 2.13  89/04/18  21:25:45  mwyoung
 * 	Recent history:
 * 	 	Add call to vm_map_simplify to reduce kernel map fragmentation.
 * 	History condensation:
 * 		Added routines for copying user data to physical
 * 		 addresses.  [rfr, mwyoung]
 * 		Added routines for sleep/wakeup forms, interrupt-time
 * 		 allocation. [dbg]
 * 		Created.  [avie, mwyoung, dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
/*
 *	File:	vm/vm_kern.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Kernel memory management.
 */

#include <cpus.h>
#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/thread.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <device/io_req.h>
#include <kern/misc_protos.h>
#include <vm/cpm.h>

#include <string.h>
/*
 *	Variables exported by this module.
 */

vm_map_t	kernel_map;
vm_map_t	kernel_pageable_map;

/*
 * Forward declarations for internal functions.
 */
extern kern_return_t kmem_alloc_pages(
	register vm_object_t	object,
	register vm_offset_t	offset,
	register vm_offset_t	start,
	register vm_offset_t	end,
	vm_prot_t		protection);

extern void kmem_remap_pages(
	register vm_object_t	object,
	register vm_offset_t	offset,
	register vm_offset_t	start,
	register vm_offset_t	end,
	vm_prot_t		protection);

kern_return_t
kmem_alloc_contig(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size,
	vm_offset_t 	mask,
	int 		flags)
{
	vm_object_t		object;
	vm_page_t		m, pages;
	kern_return_t		kr;
	vm_offset_t		addr, i, offset;
	vm_map_entry_t		entry;

	if (map == VM_MAP_NULL || (flags && (flags ^ KMA_KOBJECT))) 
		return KERN_INVALID_ARGUMENT;
	
	if (size == 0) {
		*addrp = 0;
		return KERN_INVALID_ARGUMENT;
	}

	size = round_page(size);
	if ((flags & KMA_KOBJECT) == 0) {
		object = vm_object_allocate(size);
		kr = vm_map_find_space(map, &addr, size, mask, &entry);
	}
	else {
		object = kernel_object;
		kr = vm_map_find_space(map, &addr, size, mask, &entry);
	}

	if ((flags & KMA_KOBJECT) == 0) {
		entry->object.vm_object = object;
		entry->offset = offset = 0;
	} else {
		offset = addr - VM_MIN_KERNEL_ADDRESS;

		if (entry->object.vm_object == VM_OBJECT_NULL) {
			vm_object_reference(object);
			entry->object.vm_object = object;
			entry->offset = offset;
		}
	}

	if (kr != KERN_SUCCESS) {
		if ((flags & KMA_KOBJECT) == 0)
			vm_object_deallocate(object);
		return kr;
	}

	vm_map_unlock(map);

	kr = cpm_allocate(size, &pages, FALSE);

	if (kr != KERN_SUCCESS) {
		vm_map_remove(map, addr, addr + size, 0);
		*addrp = 0;
		return kr;
	}

	vm_object_lock(object);
	for (i = 0; i < size; i += PAGE_SIZE) {
		m = pages;
		pages = NEXT_PAGE(m);
		m->busy = FALSE;
		vm_page_insert(m, object, offset + i);
	}
	vm_object_unlock(object);

	if ((kr = vm_map_wire(map, addr, addr + size, VM_PROT_DEFAULT, FALSE)) 
		!= KERN_SUCCESS) {
		if (object == kernel_object) {
			vm_object_lock(object);
			vm_object_page_remove(object, offset, offset + size);
			vm_object_unlock(object);
		}
		vm_map_remove(map, addr, addr + size, 0);
		return kr;
	}
	if (object == kernel_object)
		vm_map_simplify(map, addr);

	*addrp = addr;
	return KERN_SUCCESS;
}

/*
 * Master entry point for allocating kernel memory.
 * NOTE: this routine is _never_ interrupt safe.
 *
 * map		: map to allocate into
 * addrp	: pointer to start address of new memory
 * size		: size of memory requested
 * flags	: options
 *		  KMA_HERE		*addrp is base address, else "anywhere"
 *		  KMA_NOPAGEWAIT	don't wait for pages if unavailable
 *		  KMA_KOBJECT		use kernel_object
 */

kern_return_t
kernel_memory_allocate(
	register vm_map_t	map,
	register vm_offset_t	*addrp,
	register vm_size_t	size,
	register vm_offset_t	mask,
	int			flags)
{
	vm_object_t object = VM_OBJECT_NULL;
	vm_map_entry_t entry;
	vm_offset_t offset;
	vm_offset_t addr;
	vm_offset_t i;
	kern_return_t kr;

	size = round_page(size);
	if ((flags & KMA_KOBJECT) == 0) {
		/*
		 *	Allocate a new object.  We must do this before locking
		 *	the map, or risk deadlock with the default pager:
		 *		device_read_alloc uses kmem_alloc,
		 *		which tries to allocate an object,
		 *		which uses kmem_alloc_wired to get memory,
		 *		which blocks for pages.
		 *		then the default pager needs to read a block
		 *		to process a memory_object_data_write,
		 *		and device_read_alloc calls kmem_alloc
		 *		and deadlocks on the map lock.
		 */
		object = vm_object_allocate(size);
		kr = vm_map_find_space(map, &addr, size, mask, &entry);
	}
	else {
		object = kernel_object;
		kr = vm_map_find_space(map, &addr, size, mask, &entry);
	}
	if (kr != KERN_SUCCESS) {
		if ((flags & KMA_KOBJECT) == 0)
			vm_object_deallocate(object);
		return kr;
	}

	if ((flags & KMA_KOBJECT) == 0) {
		entry->object.vm_object = object;
		entry->offset = offset = 0;
	} else {
		offset = addr - VM_MIN_KERNEL_ADDRESS;

		if (entry->object.vm_object == VM_OBJECT_NULL) {
			vm_object_reference(object);
			entry->object.vm_object = object;
			entry->offset = offset;
		}
	}

	/*
	 *	Since we have not given out this address yet,
	 *	it is safe to unlock the map.
	 */
	vm_map_unlock(map);

	vm_object_lock(object);
	for (i = 0; i < size; i += PAGE_SIZE) {
		vm_page_t	mem;

		while ((mem = vm_page_alloc(object, offset + i))
			    == VM_PAGE_NULL) {
			if (flags & KMA_NOPAGEWAIT) {
				if (object == kernel_object)
					vm_object_page_remove(object, offset,
						offset + i);
				vm_object_unlock(object);
				vm_map_remove(map, addr, addr + size, 0);
				return KERN_RESOURCE_SHORTAGE;
			}
			vm_object_unlock(object);
			VM_PAGE_WAIT();
			vm_object_lock(object);
		}
		mem->busy = FALSE;
	}
	vm_object_unlock(object);

	if ((kr = vm_map_wire(map, addr, addr + size, VM_PROT_DEFAULT, FALSE)) 
		!= KERN_SUCCESS) {
		if (object == kernel_object) {
			vm_object_lock(object);
			vm_object_page_remove(object, offset, offset + size);
			vm_object_unlock(object);
		}
		vm_map_remove(map, addr, addr + size, 0);
		return (kr);
	}
	if (object == kernel_object)
		vm_map_simplify(map, addr);

	/*
	 *	Return the memory, not zeroed.
	 */
#if	(NCPUS > 1)  &&  i860
	bzero( addr, size );
#endif                                  /* #if (NCPUS > 1)  &&  i860 */
	*addrp = addr;
	return KERN_SUCCESS;
}

/*
 *	kmem_alloc:
 *
 *	Allocate wired-down memory in the kernel's address map
 *	or a submap.  The memory is not zero-filled.
 */

kern_return_t
kmem_alloc(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	return kernel_memory_allocate(map, addrp, size, 0, 0);
}

/*
 *	kmem_realloc:
 *
 *	Reallocate wired-down memory in the kernel's address map
 *	or a submap.  Newly allocated pages are not zeroed.
 *	This can only be used on regions allocated with kmem_alloc.
 *
 *	If successful, the pages in the old region are mapped twice.
 *	The old region is unchanged.  Use kmem_free to get rid of it.
 */
kern_return_t
kmem_realloc(
	vm_map_t	map,
	vm_offset_t	oldaddr,
	vm_size_t	oldsize,
	vm_offset_t	*newaddrp,
	vm_size_t	newsize)
{
	vm_offset_t oldmin, oldmax;
	vm_offset_t newaddr;
	vm_object_t object;
	vm_map_entry_t oldentry, newentry;
	kern_return_t kr;

	oldmin = trunc_page(oldaddr);
	oldmax = round_page(oldaddr + oldsize);
	oldsize = oldmax - oldmin;
	newsize = round_page(newsize);

	/*
	 *	Find space for the new region.
	 */

	kr = vm_map_find_space(map, &newaddr, newsize, (vm_offset_t) 0,
			       &newentry);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	/*
	 *	Find the VM object backing the old region.
	 */

	if (!vm_map_lookup_entry(map, oldmin, &oldentry))
		panic("kmem_realloc");
	object = oldentry->object.vm_object;

	/*
	 *	Increase the size of the object and
	 *	fill in the new region.
	 */

	vm_object_reference(object);
	vm_object_lock(object);
	if (object->size != oldsize)
		panic("kmem_realloc");
	object->size = newsize;
	vm_object_unlock(object);

	newentry->object.vm_object = object;
	newentry->offset = 0;
	assert (newentry->wired_count == 0);
	newentry->wired_count = 1;

	/*
	 *	Since we have not given out this address yet,
	 *	it is safe to unlock the map.  We are trusting
	 *	that nobody will play with either region.
	 */

	vm_map_unlock(map);

	/*
	 *	Remap the pages in the old region and
	 *	allocate more pages for the new region.
	 */

	kmem_remap_pages(object, 0,
			 newaddr, newaddr + oldsize,
			 VM_PROT_DEFAULT);
	kmem_alloc_pages(object, oldsize,
			 newaddr + oldsize, newaddr + newsize,
			 VM_PROT_DEFAULT);

	*newaddrp = newaddr;
	return KERN_SUCCESS;
}

/*
 *	kmem_alloc_wired:
 *
 *	Allocate wired-down memory in the kernel's address map
 *	or a submap.  The memory is not zero-filled.
 *
 *	The memory is allocated in the kernel_object.
 *	It may not be copied with vm_map_copy, and
 *	it may not be reallocated with kmem_realloc.
 */

kern_return_t
kmem_alloc_wired(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	return kernel_memory_allocate(map, addrp, size, 0, KMA_KOBJECT);
}

/*
 *	kmem_alloc_aligned:
 *
 *	Like kmem_alloc_wired, except that the memory is aligned.
 *	The size should be a power-of-2.
 */

kern_return_t
kmem_alloc_aligned(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	if ((size & (size - 1)) != 0)
		panic("kmem_alloc_aligned: size not aligned");
	return kernel_memory_allocate(map, addrp, size, size - 1, KMA_KOBJECT);
}

/*
 *	kmem_alloc_pageable:
 *
 *	Allocate pageable memory in the kernel's address map.
 */

kern_return_t
kmem_alloc_pageable(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	vm_offset_t addr;
	kern_return_t kr;

	addr = vm_map_min(map);
	kr = vm_map_enter(map, &addr, round_page(size),
			  (vm_offset_t) 0, TRUE,
			  VM_OBJECT_NULL, (vm_offset_t) 0, FALSE,
			  VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT);
	if (kr != KERN_SUCCESS)
		return kr;

	*addrp = addr;
	return KERN_SUCCESS;
}

/*
 *	kmem_free:
 *
 *	Release a region of kernel virtual memory allocated
 *	with kmem_alloc, kmem_alloc_wired, or kmem_alloc_pageable,
 *	and return the physical pages associated with that region.
 */

void
kmem_free(
	vm_map_t	map,
	vm_offset_t	addr,
	vm_size_t	size)
{
	kern_return_t kr;

	kr = vm_map_remove(map, trunc_page(addr),
			   round_page(addr + size), VM_MAP_REMOVE_KUNWIRE);
	if (kr != KERN_SUCCESS)
		panic("kmem_free");
}

/*
 *	Allocate new wired pages in an object.
 *	The object is assumed to be mapped into the kernel map or
 *	a submap.
 */

kern_return_t
kmem_alloc_pages(
	register vm_object_t	object,
	register vm_offset_t	offset,
	register vm_offset_t	start,
	register vm_offset_t	end,
	vm_prot_t		protection)
{
	/*
	 *	Mark the pmap region as not pageable.
	 */
	pmap_pageable(kernel_pmap, start, end, FALSE);

	while (start < end) {
	    register vm_page_t	mem;

	    vm_object_lock(object);

	    /*
	     *	Allocate a page
	     */
	    while ((mem = vm_page_alloc(object, offset))
			 == VM_PAGE_NULL) {
		vm_object_unlock(object);
		VM_PAGE_WAIT();
		vm_object_lock(object);
	    }

	    /*
	     *	Wire it down
	     */
	    vm_page_lock_queues();
	    vm_page_wire(mem);
	    vm_page_unlock_queues();
	    vm_object_unlock(object);

	    /*
	     *	Enter it in the kernel pmap
	     */
	    PMAP_ENTER(kernel_pmap, start, mem,
		       protection, TRUE);

	    vm_object_lock(object);
	    PAGE_WAKEUP_DONE(mem);
	    vm_object_unlock(object);

	    start += PAGE_SIZE;
	    offset += PAGE_SIZE;
	}
	return KERN_SUCCESS;
}

/*
 *	Remap wired pages in an object into a new region.
 *	The object is assumed to be mapped into the kernel map or
 *	a submap.
 */
void
kmem_remap_pages(
	register vm_object_t	object,
	register vm_offset_t	offset,
	register vm_offset_t	start,
	register vm_offset_t	end,
	vm_prot_t		protection)
{
	/*
	 *	Mark the pmap region as not pageable.
	 */
	pmap_pageable(kernel_pmap, start, end, FALSE);

	while (start < end) {
	    register vm_page_t	mem;

	    vm_object_lock(object);

	    /*
	     *	Find a page
	     */
	    if ((mem = vm_page_lookup(object, offset)) == VM_PAGE_NULL)
		panic("kmem_remap_pages");

	    /*
	     *	Wire it down (again)
	     */
	    vm_page_lock_queues();
	    vm_page_wire(mem);
	    vm_page_unlock_queues();
	    vm_object_unlock(object);

	    /*
	     *	Enter it in the kernel pmap.  The page isn't busy,
	     *	but this shouldn't be a problem because it is wired.
	     */
	    PMAP_ENTER(kernel_pmap, start, mem,
		       protection, TRUE);

	    start += PAGE_SIZE;
	    offset += PAGE_SIZE;
	}
}

/*
 *	kmem_suballoc:
 *
 *	Allocates a map to manage a subrange
 *	of the kernel virtual address space.
 *
 *	Arguments are as follows:
 *
 *	parent		Map to take range from
 *	addr		Address of start of range (IN/OUT)
 *	size		Size of range to find
 *	pageable	Can region be paged
 *	anywhere	Can region be located anywhere in map
 *	new_map		Pointer to new submap
 */
kern_return_t
kmem_suballoc(
	vm_map_t	parent,
	vm_offset_t	*addr,
	vm_size_t	size,
	boolean_t	pageable,
	boolean_t	anywhere,
	vm_map_t	*new_map)
{
	vm_map_t map;
	kern_return_t kr;

	size = round_page(size);

	/*
	 *	Need reference on submap object because it is internal
	 *	to the vm_system.  vm_object_enter will never be called
	 *	on it (usual source of reference for vm_map_enter).
	 */
	vm_object_reference(vm_submap_object);

	if (anywhere == TRUE)
		*addr = (vm_offset_t)vm_map_min(parent);
	kr = vm_map_enter(parent, addr, size,
			  (vm_offset_t) 0, anywhere,
			  vm_submap_object, (vm_offset_t) 0, FALSE,
			  VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT);
	if (kr != KERN_SUCCESS) {
		vm_object_deallocate(vm_submap_object);
		return (kr);
	}

	pmap_reference(vm_map_pmap(parent));
	map = vm_map_create(vm_map_pmap(parent), *addr, *addr + size, pageable);
	if (map == VM_MAP_NULL)
		panic("kmem_suballoc: vm_map_create failed");	/* "can't happen" */

	kr = vm_map_submap(parent, *addr, *addr + size, map);
	if (kr != KERN_SUCCESS) {
		/*
		 * See comment preceding vm_map_submap().
		 */
		vm_map_remove(parent, *addr, *addr + size, VM_MAP_NO_FLAGS);
		vm_map_deallocate(map);	/* also removes ref to pmap */
		vm_object_deallocate(vm_submap_object);
		return (kr);
	}

	*new_map = map;
	return (KERN_SUCCESS);
}

/*
 *	kmem_init:
 *
 *	Initialize the kernel's virtual memory map, taking
 *	into account all memory allocated up to this time.
 */
void
kmem_init(
	vm_offset_t	start,
	vm_offset_t	end)
{
	kernel_map = vm_map_create(pmap_kernel(),
				   VM_MIN_KERNEL_ADDRESS, end,
				   FALSE);

	/*
	 *	Reserve virtual memory allocated up to this time.
	 */

	if (start != VM_MIN_KERNEL_ADDRESS) {
		vm_offset_t addr = VM_MIN_KERNEL_ADDRESS;
		(void) vm_map_enter(kernel_map,
				    &addr, start - VM_MIN_KERNEL_ADDRESS,
				    (vm_offset_t) 0, TRUE,
				    VM_OBJECT_NULL, (vm_offset_t) 0, FALSE,
				    VM_PROT_DEFAULT, VM_PROT_ALL,
				    VM_INHERIT_DEFAULT);
	}

        /*
         * Account for kernel memory (text, data, bss, vm shenanigans).
         * This may include inaccessible "holes" as determined by what
         * the machine-dependent init code includes in mem_size.
         */
        vm_page_wire_count = (atop(mem_size) - (vm_page_free_count
                                                + vm_page_active_count
                                                + vm_page_inactive_count));
}

/*
 *	kmem_io_map_copyout:
 *
 *	Establish temporary mapping in designated map for the memory
 *	passed in.  Memory format must be a page_list vm_map_copy.
 */

kern_return_t
kmem_io_map_copyout(
	vm_map_t 	map,
	vm_offset_t	*addr,  	/* actual addr of data */
	vm_size_t	*alloc_size,	/* size allocated */
	vm_map_copy_t	copy,
	vm_size_t	min_size,	/* Do at least this much */
	vm_prot_t	prot)		/* Protection of mapping */
{
	vm_offset_t	myaddr, offset;
	vm_size_t	mysize, copy_size;
	kern_return_t	ret;
	register
	vm_page_t	*page_list;
	vm_map_copy_t	new_copy;
	register
	int		i;

	assert(copy->type == VM_MAP_COPY_PAGE_LIST);
	assert(min_size != 0);

	/*
	 *	Figure out the size in vm pages.
	 */
	min_size += copy->offset - trunc_page(copy->offset);
	min_size = round_page(min_size);
	mysize = round_page(copy->offset + copy->size) -
		trunc_page(copy->offset);

	/*
	 *	If total size is larger than one page list and
	 *	we don't have to do more than one page list, then
	 *	only do one page list.  
	 *
	 * XXX	Could be much smarter about this ... like trimming length
	 * XXX	if we need more than one page list but not all of them.
	 */

	copy_size = ptoa(copy->cpy_npages);
	if (mysize > copy_size && copy_size > min_size)
		mysize = copy_size;

	/*
	 *	Allocate some address space in the map (must be kernel
	 *	space).
	 */
	myaddr = vm_map_min(map);
	ret = vm_map_enter(map, &myaddr, mysize,
			  (vm_offset_t) 0, TRUE,
			  VM_OBJECT_NULL, (vm_offset_t) 0, FALSE,
			  prot, prot, VM_INHERIT_DEFAULT);

	if (ret != KERN_SUCCESS)
		return(ret);

	/*
	 *	Tell the pmap module that this will be wired, and
	 *	enter the mappings.
	 */
	pmap_pageable(vm_map_pmap(map), myaddr, myaddr + mysize, TRUE);

	*addr = myaddr + (copy->offset - trunc_page(copy->offset));
	*alloc_size = mysize;

	offset = myaddr;
	page_list = &copy->cpy_page_list[0];
	while (TRUE) {
		for ( i = 0; i < copy->cpy_npages; i++, offset += PAGE_SIZE) {
			PMAP_ENTER(vm_map_pmap(map), offset, *page_list,
				   prot, TRUE);
			page_list++;
		}

		if (offset == (myaddr + mysize))
			break;

		/*
		 *	Onward to the next page_list.  The extend_cont
		 *	leaves the current page list's pages alone; 
		 *	they'll be cleaned up at discard.  Reset this
		 *	copy's continuation to discard the next one.
		 */
		vm_map_copy_invoke_extend_cont(copy, &new_copy, &ret);

		if (ret != KERN_SUCCESS) {
			kmem_io_map_deallocate(map, myaddr, mysize);
			return(ret);
		}
		copy->cpy_cont = vm_map_copy_discard_cont;
		copy->cpy_cont_args = (vm_map_copyin_args_t) new_copy;
		assert(new_copy != VM_MAP_COPY_NULL);
		assert(new_copy->type == VM_MAP_COPY_PAGE_LIST);
		copy = new_copy;
		page_list = &copy->cpy_page_list[0];
	}

	return(ret);
}

/*
 *	kmem_io_map_deallocate:
 *
 *	Get rid of the mapping established by kmem_io_map_copyout.
 *	Assumes that addr and size have been rounded to page boundaries.
 */

void
kmem_io_map_deallocate(
	vm_map_t	map,
	vm_offset_t	addr,
	vm_size_t	size)
{

	register vm_offset_t	va, end;

	end = round_page(addr + size);
	for (va = trunc_page(addr); va < end; va += PAGE_SIZE)
	    pmap_change_wiring(vm_map_pmap(map), va, FALSE);

	/*
	 *	Remove the mappings.  The pmap_remove is needed.
	 */
	
	pmap_remove(vm_map_pmap(map), addr, addr + size);
	vm_map_remove(map, addr, addr + size, VM_MAP_REMOVE_KUNWIRE);
}

/*
 *	kmem_io_page_list_to_sglist:
 *
 *	Allocate and fill in an I/O scatter/gather list from a page_list
 *	vm_map_copy object.
 */

kern_return_t
kmem_io_page_list_to_sglist(
     vm_map_copy_t	copy,
     struct io_sglist	**sgp,		/* scatter/gather list pointer (out) */
     vm_size_t		*alloc_size,	/* size allocated (out)  */
     vm_size_t		min_size)	/* Do at least this much */
{
	kern_return_t	ret;
	register
	vm_page_t	*page_list;
	vm_map_copy_t	new_copy;
	register
	int		i;
	vm_size_t	xfer_length, total_size, off;
        register
        struct io_sg_entry *sge;
	struct io_sglist *sglist;	/* scatter/gather list to fill in */
	int		nentries;

	assert(copy->type == VM_MAP_COPY_PAGE_LIST);
	assert(min_size != 0);

	total_size = copy->size;
	assert(min_size >= total_size);
        xfer_length = 0;
        off = copy->offset - trunc_page(copy->offset);

	/*
	 * If the reequest size is less than the minimum transfer size for
	 * the device, we must expand the scatter/gather list.  This provides
	 * enough data pointers to satisfy the transfer.  A better way to
	 * handle this would be to create a vm_copy_page_list_expand()
	 * function to extend the data area with nulls.  Later!
	 */
	if (copy->size < min_size) {
		/*
		 * Expand the list to cover the minimum device transfer length.
		 */
		nentries = round_page(off + min_size) / PAGE_SIZE;
	} else {
		nentries = vm_map_copy_page_count(copy);
	}
	io_sglist_alloc(sglist, nentries);
	*sgp = sglist;

        sglist->iosg_hdr.length = min_size;
	*alloc_size = sglist->iosg_hdr.length;
        sge = sglist->iosg_list;
	while (TRUE) {
		page_list = &copy->cpy_page_list[0];
		for ( i = 0; i < copy->cpy_npages; i++ ) {
                        assert((sge - sglist->iosg_list) <
			       sglist->iosg_hdr.nentries);
                        xfer_length += PAGE_SIZE;
                        sge->iosge_length = PAGE_SIZE;
                        sge->iosge_phys = (*page_list)->phys_addr;
                        page_list++;
                        sge++;
		}

		if (xfer_length >= total_size)
			break;
		/*
		 *	Onward to the next page_list.  The extend_cont
		 *	leaves the current page list's pages alone; 
		 *	they'll be cleaned up at discard.  Reset this
		 *	copy's continuation to discard the next one.
		 */
		vm_map_copy_invoke_extend_cont(copy, &new_copy, &ret);

		if (ret != KERN_SUCCESS) {
			io_sglist_free(sglist);
			return(ret);
		}
		copy->cpy_cont = vm_map_copy_discard_cont;
		copy->cpy_cont_args = (vm_map_copyin_args_t) new_copy;
		copy = new_copy;
	}

	/*
	 * If the request size is less than the minimum transfer size for
	 * the device, the list must be expanded.  Fill in the remaining
	 * scatter/gather list entries.
	 */
	if (total_size < min_size) {

		/*
		 * Expand the list to cover the minimum device transfer length.
		 * Point each additional scatter/gather entry to the etnry 0's
		 * address and set the length to a page.  Note that the address
		 * in entry 0 is adjusted for an offset later, so if there is
		 * an offset or the length less than a page, we will transfer
		 * garbage data.  Otherwise, the remaining entries just repeat
		 * entry 0's data to pad out to the transfer size.  A security
		 * hole?  Maybe, but the extra data in the page should belong
		 * to the same application, so it's no less secure than the
		 * true data.
		 */
		while (xfer_length < min_size) {
			sge->iosge_length = PAGE_SIZE;
			sge->iosge_phys   = sglist->iosg_list[0].iosge_phys;
			sge++;
			xfer_length += PAGE_SIZE;
		}

		/*
		 * Correct last entry.
		 *
		 * For the last entry, length could be less than the whole page.
		 */
		if (xfer_length > min_size) {
			/*
			 * If the transfer length is greater than the minimum
			 * transfer size, truncate the last entry length down
			 * to the mininum and add any offset.
			 */
			(sge-1)->iosge_length -= (xfer_length - min_size) + off;
			assert((sge-1)->iosge_length <= PAGE_SIZE);
		} else if (off) {
			/*
			 * If there is an offset, set the last entry length to
			 * the offset to handle the shift of the data through
			 * the pages.
			 */
			(sge-1)->iosge_length = off;
		}
	} else {
		/*
		 * Correct last entry.
		 *
		 * For the last entry, length could be less than the whole page.
		 */
		xfer_length -= off;
		sglist->iosg_list[sglist->iosg_hdr.nentries-1].iosge_length
			-= (xfer_length - total_size);
	}

	/*
	 * Correct first entry.
	 *
	 * For the first entry, start could be offset into the page.
	 */
	sglist->iosg_list[0].iosge_phys   += off;
	sglist->iosg_list[0].iosge_length -= off;

	return(KERN_SUCCESS);
}

/*
 *	kmem_io_sglist_object_alloc:
 *
 *	Allocate and fill in an object vm_map_copy_t from an I/O
 *	scatter/gather list.
 */

kern_return_t
kmem_io_sglist_object_alloc(
     struct io_sglist	*sglist,	/* OUT Scatter/gather list to fill */
     vm_map_copy_t	*copy_result)	/* OUT copy object */
{
	register        vm_object_t     object;
	register 	int		i;
        register        vm_size_t       offset, size;

        size = round_page(sglist->iosg_hdr.length);
        object = vm_object_allocate(size);
        if (object == VM_OBJECT_NULL)
                return KERN_RESOURCE_SHORTAGE;

	vm_object_lock(object);

	for (i = 0, offset = 0; offset < size; i++, offset += PAGE_SIZE) {
	    register vm_page_t	mem;

	    /*
	     *	Allocate a page
	     */
	    while ((mem = vm_page_grab()) == VM_PAGE_NULL) {
		VM_PAGE_WAIT();
	    }

	    /*
	     *	Wire it down and insert it into the object
	     */
	    vm_page_lock_queues();
	    vm_page_wire(mem);
	    vm_page_unlock_queues();
	    /*
	     * Unbusy the page
	     */
	    mem->busy = FALSE;

	    vm_page_insert(mem, object, offset);

	    /*
	     * Fill in the scatter/gather entry
	     */
	    sglist->iosg_list[i].iosge_phys = mem->phys_addr;
	    sglist->iosg_list[i].iosge_length = PAGE_SIZE;
        }

	vm_map_copyin_object(object, 0, size, copy_result);
	vm_object_unlock(object);

	return KERN_SUCCESS;
}

/*
 *	kmem_io_object_trunc:
 *
 *	Truncate an object vm_map_copy_t.
 *	Called by the scatter/gather list network code to remove pages from
 *	the tail end of a packet. Also unwires the objects pages.
 */

kern_return_t
kmem_io_object_trunc(copy, new_size)
     vm_map_copy_t	copy;		/* IN/OUT copy object */
     register vm_size_t new_size;	/* IN new object size */
{
	register vm_size_t	offset, old_size;

	assert(copy->type == VM_MAP_COPY_OBJECT);

	old_size = round_page(copy->size);
	copy->size = new_size;
	new_size = round_page(new_size);

        vm_object_lock(copy->cpy_object);
        vm_object_page_remove(copy->cpy_object,
                              (vm_offset_t)new_size, (vm_offset_t)old_size);
        for (offset = 0; offset < new_size; offset += PAGE_SIZE) {
		register vm_page_t	mem;

		if ((mem = vm_page_lookup(copy->cpy_object, offset)) == VM_PAGE_NULL)
		    panic("kmem_io_object_trunc: unable to find object page");

		/*
		 * Make sure these pages are marked dirty
		 */
		mem->dirty = TRUE;
		vm_page_lock_queues();
		vm_page_unwire(mem);
		vm_page_unlock_queues();
	}
        copy->cpy_object->size = new_size;	/*  adjust size of object */
        vm_object_unlock(copy->cpy_object);
        return(KERN_SUCCESS);
}

/*
 *	kmem_io_object_deallocate:
 *
 *	Free an vm_map_copy_t.
 *	Called by the scatter/gather list network code to free a packet.
 */

void
kmem_io_object_deallocate(
     vm_map_copy_t	copy)		/* IN/OUT copy object */
{
	kern_return_t	ret;

	/*
	 * Clear out all the object pages (this will leave an empty object).
	 */
	ret = kmem_io_object_trunc(copy, 0);
	if (ret != KERN_SUCCESS)
		panic("kmem_io_object_deallocate: unable to truncate object");
	/*
	 * ...and discard the copy object.
	 */
	vm_map_copy_discard(copy);
}

/*
 *	Routine:	copyinmap
 *	Purpose:
 *		Like copyin, except that fromaddr is an address
 *		in the specified VM map.  This implementation
 *		is incomplete; it handles the current user map
 *		and the kernel map/submaps.
 */
boolean_t
copyinmap(
	vm_map_t	map,
	vm_offset_t	fromaddr,
	vm_offset_t	toaddr,
	vm_size_t	length)
{
	if (vm_map_pmap(map) == pmap_kernel()) {
		/* assume a correct copy */
		memcpy((void *)toaddr, (void *)fromaddr, length);
		return FALSE;
	}

	if (current_map() == map)
		return copyin((char *)fromaddr, (char *)toaddr, length);

	return TRUE;
}

/*
 *	Routine:	copyoutmap
 *	Purpose:
 *		Like copyout, except that toaddr is an address
 *		in the specified VM map.  This implementation
 *		is incomplete; it handles the current user map
 *		and the kernel map/submaps.
 */
boolean_t
copyoutmap(
	vm_map_t	map,
	vm_offset_t	fromaddr,
	vm_offset_t	toaddr,
	vm_size_t	length)
{
	if (vm_map_pmap(map) == pmap_kernel()) {
		/* assume a correct copy */
		memcpy((void *)toaddr, (void *)fromaddr, length);
		return FALSE;
	}

	if (current_map() == map)
		return copyout((char *)fromaddr, (char *)toaddr, length);

	return TRUE;
}
