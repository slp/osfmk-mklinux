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
 * Revision 2.7.8.4  92/09/15  17:36:03  jeffreyh
 * 	05-Aug-92  Dejan Milojicic (dejan)
 * 	Fixed vm_copy_region having an extra parameter.
 * 	[92/08/28  16:31:29  jeffreyh]
 * 
 * Revision 2.7.8.3  92/04/08  15:46:56  jeffreyh
 * 	Removed new mach_vm_region_info, mach_vm_object_info
 * 	mach_vm_object_pages. 
 * 	Restored old mach_mv_object_info and vm_mapped_pages calls.
 * 	Still no more sharing maps.
 * 	[92/04/01            jeffreyh]
 * 
 * Revision 2.7.8.2  92/03/03  16:25:51  jeffreyh
 * 	Eliminate keep_wired argument from vm_map_copyin().
 * 	[92/02/21  10:14:21  dlb]
 * 
 * 	No more sharing maps.
 * 	[92/01/07  11:02:46  dlb]
 * 	Changes from TRUNK
 * 	[92/02/26  12:33:47  jeffreyh]
 * 
 * Revision 2.8  92/01/14  16:47:39  rpd
 * 	Changed host_virtual_physical_table_info and
 * 	mach_vm_object_pages for CountInOut.
 * 	[92/01/14            rpd]
 * 
 * 	Removed <mach_debug/page_info.h>.
 * 	[92/01/08            rpd]
 * 	Fixed mach_vm_region_info for submaps and share maps.
 * 	[92/01/02            rpd]
 * 
 * 	Replaced the old mach_vm_region_info with new mach_vm_region_info,
 * 	mach_vm_object_info, mach_vm_object_pages calls.
 * 	Removed vm_mapped_pages_info.
 * 	[91/12/30            rpd]
 * 
 * Revision 2.7.8.1  92/02/18  19:20:54  jeffreyh
 * 	Include thread.h when compiling with VM_OBJECT_DEBUG
 * 	[91/09/09            bernadat]
 * 
 * Revision 2.7  91/08/28  11:17:57  jsb
 * 	single_use --> use_old_pageout in vm_object.
 * 	[91/08/05  17:44:32  dlb]
 * 
 * Revision 2.6  91/05/14  17:48:16  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/05  17:57:39  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:31:12  mrt]
 * 
 * Revision 2.4  91/01/08  16:44:26  rpd
 * 	Added host_virtual_physical_table_info.
 * 	[91/01/02            rpd]
 * 
 * Revision 2.3  90/10/12  13:05:13  rpd
 * 	Removed copy_on_write field.
 * 	[90/10/08            rpd]
 * 
 * Revision 2.2  90/06/02  15:10:26  rpd
 * 	Moved vm_mapped_pages_info here from vm/vm_map.c.
 * 	[90/05/31            rpd]
 * 
 * 	Created.
 * 	[90/04/20            rpd]
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
/*
 *	File:	vm/vm_debug.c.
 *	Author:	Rich Draves
 *	Date:	March, 1990
 *
 *	Exported kernel calls.  See mach_debug/mach_debug.defs.
 */

#include <kern/thread.h>
#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach/memory_object.h>
#include <mach/vm_prot.h>
#include <mach/vm_inherit.h>
#include <mach/vm_param.h>
#include <mach_debug/vm_info.h>
#include <mach_debug/page_info.h>
#include <mach_debug/hash_info.h>
#include <mach_debug/mach_debug_server.h>	/* WRONG LOCATION */
#include <mach/mach_server.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <kern/task.h>
#include <kern/host.h>
#include <ipc/ipc_port.h>
#include <vm/vm_debug.h>

/*
 *	Routine:	mach_vm_region_info [kernel call]
 *	Purpose:
 *		Retrieve information about a VM region,
 *		including info about the object chain.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		KERN_SUCCESS		Retrieve region/object info.
 *		KERN_INVALID_TASK	The map is null.
 *		KERN_NO_SPACE		There is no entry at/after the address.
 *		KERN_RESOURCE_SHORTAGE	Can't allocate memory.
 */

kern_return_t
mach_vm_region_info(
	vm_map_t		map,
	vm_offset_t		address,
	vm_info_region_t	*regionp,
	vm_info_object_array_t	*objectsp,
	mach_msg_type_number_t	*objectsCntp)
{
	vm_map_copy_t copy;
	vm_offset_t addr;	/* memory for OOL data */
	vm_size_t size;		/* size of the memory */
	unsigned int room;	/* room for this many objects */
	unsigned int used;	/* actually this many objects */
	vm_info_region_t region;
	kern_return_t kr;

	if (map == VM_MAP_NULL)
		return KERN_INVALID_TASK;

	size = 0;		/* no memory allocated yet */

	for (;;) {
		vm_map_t cmap;	/* current map in traversal */
		vm_map_t nmap;	/* next map to look at */
		vm_map_entry_t entry;
		vm_object_t object, cobject, nobject;

		/* nothing is locked */

		vm_map_lock_read(map);
		for (cmap = map;; cmap = nmap) {
			/* cmap is read-locked */

			if (!vm_map_lookup_entry(cmap, address, &entry)) {
				entry = entry->vme_next;
				if (entry == vm_map_to_entry(cmap)) {
					vm_map_unlock_read(cmap);
					if (size != 0)
						kmem_free(ipc_kernel_map,
							  addr, size);
					return KERN_NO_SPACE;
				}
			}

			if (entry->is_sub_map)
				nmap = entry->object.sub_map;
			else
				break;

			/* move down to the lower map */

			vm_map_lock_read(nmap);
			vm_map_unlock_read(cmap);
		}

		/* cmap is read-locked; we have a real entry */

		object = entry->object.vm_object;
		region.vir_start = entry->vme_start;
		region.vir_end = entry->vme_end;
		region.vir_object = (vm_offset_t) object;
		region.vir_offset = entry->offset;
		region.vir_needs_copy = entry->needs_copy;
		region.vir_protection = entry->protection;
		region.vir_max_protection = entry->max_protection;
		region.vir_inheritance = entry->inheritance;
		region.vir_wired_count = entry->wired_count;
		region.vir_user_wired_count = entry->user_wired_count;

		used = 0;
		room = size / sizeof(vm_info_object_t);

		if (object == VM_OBJECT_NULL) {
			vm_map_unlock_read(cmap);
			/* no memory needed */
			break;
		}

		vm_object_lock(object);
		vm_map_unlock_read(cmap);

		for (cobject = object;; cobject = nobject) {
			/* cobject is locked */

			if (used < room) {
				vm_info_object_t *vio =
					&((vm_info_object_t *) addr)[used];

				vio->vio_object =
					(vm_offset_t) cobject;
				vio->vio_size =
					cobject->size;
				vio->vio_ref_count =
					cobject->ref_count;
				vio->vio_resident_page_count =
					cobject->resident_page_count;
				vio->vio_absent_count =
					cobject->absent_count;
				vio->vio_copy =
					(vm_offset_t) cobject->copy;
				vio->vio_shadow =
					(vm_offset_t) cobject->shadow;
				vio->vio_shadow_offset =
					cobject->shadow_offset;
				vio->vio_paging_offset =
					cobject->paging_offset;
				vio->vio_copy_strategy =
					cobject->copy_strategy;
				vio->vio_last_alloc =
					cobject->last_alloc;
				vio->vio_paging_in_progress =
					cobject->paging_in_progress;
				vio->vio_pager_created =
					cobject->pager_created;
				vio->vio_pager_initialized =
					cobject->pager_initialized;
				vio->vio_pager_ready =
					cobject->pager_ready;
				vio->vio_can_persist =
					cobject->can_persist;
				vio->vio_internal =
					cobject->internal;
				vio->vio_temporary =
					cobject->temporary;
				vio->vio_alive =
					cobject->alive;
				vio->vio_lock_in_progress =
					cobject->lock_in_progress;
				vio->vio_lock_restart =
					cobject->lock_restart;
			}

			used++;
			nobject = cobject->shadow;
			if (nobject == VM_OBJECT_NULL) {
				vm_object_unlock(cobject);
				break;
			}

			vm_object_lock(nobject);
			vm_object_unlock(cobject);
		}

		/* nothing locked */

		if (used <= room)
			break;

		/* must allocate more memory */

		if (size != 0)
			kmem_free(ipc_kernel_map, addr, size);
		size = round_page(2 * used * sizeof(vm_info_object_t));

		kr = vm_allocate(ipc_kernel_map, &addr, size, TRUE);
		if (kr != KERN_SUCCESS)
			return KERN_RESOURCE_SHORTAGE;

		kr = vm_map_wire(ipc_kernel_map, addr, addr + size,
				     VM_PROT_READ|VM_PROT_WRITE, FALSE);
		assert(kr == KERN_SUCCESS);
	}

	/* free excess memory; make remaining memory pageable */

	if (used == 0) {
		copy = VM_MAP_COPY_NULL;

		if (size != 0)
			kmem_free(ipc_kernel_map, addr, size);
	} else {
		vm_size_t size_used =
			round_page(used * sizeof(vm_info_object_t));

		kr = vm_map_unwire(ipc_kernel_map, addr, addr + size_used, FALSE);
		assert(kr == KERN_SUCCESS);

		kr = vm_map_copyin(ipc_kernel_map, addr, size_used,
				   TRUE, &copy);
		assert(kr == KERN_SUCCESS);

		if (size != size_used)
			kmem_free(ipc_kernel_map,
				  addr + size_used, size - size_used);
	}

	*regionp = region;
	*objectsp = (vm_info_object_array_t) copy;
	*objectsCntp = used;
	return KERN_SUCCESS;
}

/*
 * Return an array of virtual pages that are mapped to a task.
 */
kern_return_t
vm_mapped_pages_info(
	task_t			task,
	page_address_array_t	*pages,
	mach_msg_type_number_t  *pages_count)
{
	pmap_t		pmap;
	vm_size_t	size, size_used;
	unsigned int	actual, space;
	page_address_array_t list;
	vm_offset_t	addr;

	if (task == TASK_NULL)
	    return (KERN_INVALID_ARGUMENT);

	pmap = task->map->pmap;
	size = pmap_resident_count(pmap) * sizeof(vm_offset_t);
	size = round_page(size);

	for (;;) {
	    (void) vm_allocate(ipc_kernel_map, &addr, size, TRUE);
	    (void) vm_map_unwire(ipc_kernel_map, addr, addr + size, FALSE);

	    list = (page_address_array_t) addr;
	    space = size / sizeof(vm_offset_t);

	    actual = pmap_list_resident_pages(pmap,
					list,
					space);
	    if (actual <= space)
		break;

	    /*
	     * Free memory if not enough
	     */
	    (void) kmem_free(ipc_kernel_map, addr, size);

	    /*
	     * Try again, doubling the size
	     */
	    size = round_page(actual * sizeof(vm_offset_t));
	}
	if (actual == 0) {
	    *pages = 0;
	    *pages_count = 0;
	    (void) kmem_free(ipc_kernel_map, addr, size);
	}
	else {
	    *pages_count = actual;
	    size_used = round_page(actual * sizeof(vm_offset_t));
	    (void) vm_map_wire(ipc_kernel_map,
				addr, addr + size, 
				VM_PROT_READ|VM_PROT_WRITE, FALSE);
	    (void) vm_map_copyin(
				ipc_kernel_map,
				addr,
				size_used,
				TRUE,
				(vm_map_copy_t *)pages);
	    if (size_used != size) {
		(void) kmem_free(ipc_kernel_map,
				addr + size_used,
				size - size_used);
	    }
	}

	return (KERN_SUCCESS);
}


/*
 *	Routine:	host_virtual_physical_table_info
 *	Purpose:
 *		Return information about the VP table.
 *	Conditions:
 *		Nothing locked.  Obeys CountInOut protocol.
 *	Returns:
 *		KERN_SUCCESS		Returned information.
 *		KERN_INVALID_HOST	The host is null.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
host_virtual_physical_table_info(
	host_t				host,
	hash_info_bucket_array_t	*infop,
	mach_msg_type_number_t 		*countp)
{
	vm_offset_t addr;
	vm_size_t size;
	hash_info_bucket_t *info;
	unsigned int potential, actual;
	kern_return_t kr;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	/* start with in-line data */

	info = *infop;
	potential = *countp;

	for (;;) {
		actual = vm_page_info(info, potential);
		if (actual <= potential)
			break;

		/* allocate more memory */

		if (info != *infop)
			kmem_free(ipc_kernel_map, addr, size);

		size = round_page(actual * sizeof *info);
		kr = kmem_alloc_pageable(ipc_kernel_map, &addr, size);
		if (kr != KERN_SUCCESS)
			return KERN_RESOURCE_SHORTAGE;

		info = (hash_info_bucket_t *) addr;
		potential = size/sizeof *info;
	}

	if (info == *infop) {
		/* data fit in-line; nothing to deallocate */

		*countp = actual;
	} else if (actual == 0) {
		kmem_free(ipc_kernel_map, addr, size);

		*countp = 0;
	} else {
		vm_map_copy_t copy;
		vm_size_t used;

		used = round_page(actual * sizeof *info);

		if (used != size)
			kmem_free(ipc_kernel_map, addr + used, size - used);

		kr = vm_map_copyin(ipc_kernel_map, addr, used,
				   TRUE, &copy);
		assert(kr == KERN_SUCCESS);

		*infop = (hash_info_bucket_t *) copy;
		*countp = actual;
	}

	return KERN_SUCCESS;
}
