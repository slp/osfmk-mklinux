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
 * Revision 2.18.3.2  92/03/03  16:27:29  jeffreyh
 * 	Eliminate keep_wired argument from vm_map_copyin().
 * 	[92/02/21  10:16:59  dlb]
 * 	Changes from TRUNK
 * 	[92/02/26  12:36:17  jeffreyh]
 * 
 * Revision 2.18.3.1.2.1  92/02/25  17:00:16  jeffreyh
 * 	[David L. Black 92/02/22  17:07:18  dlb@osf.org]
 * 		Add checks for protection and inheritance arguments.
 * 
 * Revision 2.18.3.1  92/02/21  11:29:08  jsb
 * 	Removed NORMA_VM hackery. Hip hip hooray.
 * 	[92/02/11  17:42:41  jsb]
 * 
 * Revision 2.18  91/12/11  08:44:21  jsb
 * 	Fixed vm_write and vm_copy to check for a null map.
 * 	Fixed vm_write and vm_copy to not check for misalignment.
 * 	Fixed vm_copy to discard the copy if the overwrite fails.
 * 	[91/12/09            rpd]
 * 
 * Revision 2.17  91/12/10  13:27:17  jsb
 * 	Apply temporary NORMA_VM workaround to XMM problem.
 * 	This leaks objects if vm_map() fails.
 * 	[91/12/10  12:55:27  jsb]
 * 
 * Revision 2.16  91/08/28  11:19:07  jsb
 * 	Fixed vm_map to check memory_object with IP_VALID.
 * 	Changed vm_wire to use KERN_INVALID_{HOST,TASK,VALUE}
 * 	instead of a generic KERN_INVALID_ARGUMENT return code.
 * 	[91/07/12            rpd]
 * 
 * Revision 2.15  91/07/31  18:22:40  dbg
 * 	Change vm_pageable to vm_wire.  Require host_priv port to gain
 * 	wiring privileges.
 * 	[91/07/30  17:28:22  dbg]
 * 
 * Revision 2.14  91/05/14  17:51:35  mrt
 * 	Correcting copyright
 * 
 * Revision 2.13  91/03/16  15:07:13  rpd
 * 	Removed temporary extra stats.
 * 	[91/02/10            rpd]
 * 
 * Revision 2.12  91/02/05  18:00:35  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:35:00  mrt]
 * 
 * Revision 2.11  90/08/06  15:08:59  rwd
 * 	Vm_read should check that the map is non null.
 * 	[90/07/26            rwd]
 * 
 * Revision 2.10  90/06/02  15:12:07  rpd
 * 	Moved trap versions of syscalls to kern/ipc_mig.c.
 * 	Removed syscall_vm_allocate_with_pager.
 * 	[90/05/31            rpd]
 * 
 * 	Purged vm_allocate_with_pager.
 * 	[90/04/09            rpd]
 * 	Purged MACH_XP_FPD.  Use vm_map_pageable_user for vm_pageable.
 * 	Converted to new IPC kernel call semantics.
 * 	[90/03/26  23:21:55  rpd]
 * 
 * Revision 2.9  90/05/29  18:39:57  rwd
 * 	New trap versions of exported vm calls from rfr.
 * 	[90/04/20            rwd]
 * 
 * Revision 2.8  90/05/03  15:53:30  dbg
 * 	Set current protection to VM_PROT_DEFAULT in
 * 	vm_allocate_with_pager.
 * 	[90/04/12            dbg]
 * 
 * Revision 2.7  90/03/14  21:11:49  rwd
 * 	Get rfr bug fix.
 * 	[90/03/07            rwd]
 * 
 * Revision 2.6  90/02/22  20:07:02  dbg
 * 	Use new vm_object_copy routines.  Use new vm_map_copy
 * 	technology.  vm_read() no longer requires page alignment.
 * 	Change PAGE_WAKEUP to PAGE_WAKEUP_DONE to reflect the fact
 * 	that it clears the busy flag.
 * 	[90/01/25            dbg]
 * 
 * Revision 2.5  90/01/24  14:08:30  af
 * 	Fixed bug in optimized vm_write: now that we relaxed the restriction
 * 	on the page-alignment of the size arg we must be able to cope with
 * 	e.g. one-and-a-half pages as well.
 * 	Also, by simple measures on my pmax turns out that mapping is a win
 * 	versus copyin even for a single page. IF you can map.
 * 	[90/01/24  11:37:35  af]
 * 
 * Revision 2.4  90/01/22  23:09:42  af
 * 	Go through the map module for machine attributes.
 * 	[90/01/20  17:23:35  af]
 * 
 * 	Added vm_machine_attribute(), which only invokes the
 * 	corresponding pmap operation, for now.  Just a first
 * 	shot at it, lacks proper locking and keeping the info
 * 	around, someplace.
 * 	[89/12/08            af]
 * 
 * Revision 2.3  90/01/19  14:36:22  rwd
 * 	Disable vm_write optimization on mips since it doesn't appear to
 * 	work.
 * 	[90/01/19            rwd]
 * 
 * 	Get version that works on multiprocessor from rfr
 * 	[90/01/10            rwd]
 * 	Get new user copyout code from rfr.
 * 	[90/01/05            rwd]
 * 
 * Revision 2.2  89/09/08  11:29:05  dbg
 * 	Pass keep_wired parameter to vm_map_move.
 * 	[89/07/14            dbg]
 * 
 * 28-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	Changes for MACH_KERNEL:
 *	. Removed non-MACH include files and all conditionals.
 *	. Added vm_pageable, for privileged tasks only.
 *	. vm_read now uses vm_map_move to consolidate map operations.
 *	. If using FAST_PAGER_DATA, vm_write expects data to be in
 *	  current task's address space.
 *
 * Revision 2.12  89/04/18  21:30:56  mwyoung
 * 	All relevant history has been integrated into the documentation below.
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 *	File:	vm/vm_user.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 * 
 *	User-exported virtual memory functions.
 */

#include <vm_cpm.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>	/* to get vm_address_t */
#include <mach/memory_object.h>
#include <mach/std_types.h>	/* to get pointer_t */
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/mach_server.h>
#include <mach/mach_syscalls.h>
#include <mach/memory_object_user.h>

#include <kern/host.h>
#include <kern/task.h>
#include <kern/misc_protos.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/memory_object.h>

#if     NORMA_VM
#include <xmm/xmm_server_rename.h>
#endif /* NORMA_VM */

#if	VM_CPM
#include <vm/cpm.h>
#endif	/* VM_CPM */

/*
 *	vm_allocate allocates "zero fill" memory in the specfied
 *	map.
 */
kern_return_t
vm_allocate(
	register vm_map_t	map,
	register vm_offset_t	*addr,
	register vm_size_t	size,
	boolean_t		anywhere)
{
	kern_return_t	result;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);
	if (size == 0) {
		*addr = 0;
		return(KERN_SUCCESS);
	}

	if (anywhere)
		*addr = vm_map_min(map);
	else
		*addr = trunc_page(*addr);
	size = round_page(size);

	result = vm_map_enter(
			map,
			addr,
			size,
			(vm_offset_t)0,
			anywhere,
			VM_OBJECT_NULL,
			(vm_offset_t)0,
			FALSE,
			VM_PROT_DEFAULT,
			VM_PROT_ALL,
			VM_INHERIT_DEFAULT);

	return(result);
}

/*
 *	vm_deallocate deallocates the specified range of addresses in the
 *	specified address map.
 */
kern_return_t
vm_deallocate(
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size)
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (size == (vm_offset_t) 0)
		return(KERN_SUCCESS);

	return(vm_map_remove(map, trunc_page(start),
			     round_page(start+size), VM_MAP_NO_FLAGS));
}

/*
 *	vm_inherit sets the inheritance of the specified range in the
 *	specified map.
 */
kern_return_t
vm_inherit(
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_inherit_t		new_inheritance)
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (new_inheritance > VM_INHERIT_LAST_VALID)
                return(KERN_INVALID_ARGUMENT);

	return(vm_map_inherit(map,
			      trunc_page(start),
			      round_page(start+size),
			      new_inheritance));
}

/*
 *	vm_protect sets the protection of the specified range in the
 *	specified map.
 */

kern_return_t
vm_protect(
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size,
	boolean_t		set_maximum,
	vm_prot_t		new_protection)
{
	if ((map == VM_MAP_NULL) || (new_protection & ~VM_PROT_ALL))
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_protect(map,
			      trunc_page(start),
			      round_page(start+size),
			      new_protection,
			      set_maximum));
}

/*
 * Handle machine-specific attributes for a mapping, such
 * as cachability, migrability, etc.
 */
kern_return_t
vm_machine_attribute(
	vm_map_t	map,
	vm_address_t	address,
	vm_size_t	size,
	vm_machine_attribute_t	attribute,
	vm_machine_attribute_val_t* value)		/* IN/OUT */
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return vm_map_machine_attribute(map, address, size, attribute, value);
}

kern_return_t
vm_read(
	vm_map_t		map,
	vm_address_t		address,
	vm_size_t		size,
	pointer_t		*data,
	mach_msg_type_number_t	*data_size)
{
	kern_return_t	error;
	vm_map_copy_t	ipc_address;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if ((error = vm_map_copyin(map,
				address,
				size,
				FALSE,	/* src_destroy */
				&ipc_address)) == KERN_SUCCESS) {
		*data = (pointer_t) ipc_address;
		*data_size = size;
	}
	return(error);
}

/*ARGSUSED*/
kern_return_t
vm_write(
	vm_map_t		map,
	vm_address_t		address,
	vm_offset_t		data,
	mach_msg_type_number_t	size)
{
	if (map == VM_MAP_NULL)
		return KERN_INVALID_ARGUMENT;

	return vm_map_copy_overwrite(map, address, (vm_map_copy_t) data,
				     FALSE /* interruptible XXX */);
}

kern_return_t
vm_copy(
	vm_map_t	map,
	vm_address_t	source_address,
	vm_size_t	size,
	vm_address_t	dest_address)
{
	vm_map_copy_t copy;
	kern_return_t kr;

	if (map == VM_MAP_NULL)
		return KERN_INVALID_ARGUMENT;

	kr = vm_map_copyin(map, source_address, size,
			   FALSE, &copy);
	if (kr != KERN_SUCCESS)
		return kr;

	kr = vm_map_copy_overwrite(map, dest_address, copy,
				   FALSE /* interruptible XXX */);
	if (kr != KERN_SUCCESS) {
		vm_map_copy_discard(copy);
		return kr;
	}

	return KERN_SUCCESS;
}

/*
 *	Routine:	vm_map
 */
kern_return_t
vm_map(
	vm_map_t	target_map,
	vm_offset_t	*address,
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	ipc_port_t	port,
	vm_offset_t	offset,
	boolean_t	copy,
	vm_prot_t	cur_protection,
	vm_prot_t	max_protection,
	vm_inherit_t	inheritance)
{
	register
	vm_object_t	object;
	vm_prot_t	prot;
	kern_return_t	result;

	/*
	 * Check arguments for validity
	 */
	if ((target_map == VM_MAP_NULL) ||
		(cur_protection & ~VM_PROT_ALL) ||
		(max_protection & ~VM_PROT_ALL) ||
		(inheritance > VM_INHERIT_LAST_VALID) ||
		size == 0)
		return(KERN_INVALID_ARGUMENT);

	/*
	 * Find the vm object (if any) corresponding to this port.
	 */
	if (!IP_VALID(port)) {
		object = VM_OBJECT_NULL;
		offset = 0;
		copy = FALSE;
	} else if ((object = vm_object_enter(port, size, FALSE, FALSE))
			== VM_OBJECT_NULL)
		return(KERN_INVALID_OBJECT);

	/* wait for object (if any) to be ready */
	if (object != VM_OBJECT_NULL) {
		vm_object_lock(object);
		while (!object->pager_ready) {
			vm_object_wait(object,
					VM_OBJECT_EVENT_PAGER_READY,
					FALSE);
			vm_object_lock(object);
		}
		vm_object_unlock(object);
	}

	*address = trunc_page(*address);
	size = round_page(size);

	/*
	 *	Perform the copy if requested
	 */

	if (copy) {
		vm_object_t	new_object;
		vm_offset_t	new_offset;

		result = vm_object_copy_strategically(object, offset, size,
				&new_object, &new_offset,
				&copy);


		if (result == KERN_MEMORY_RESTART_COPY) {
			boolean_t success;
			boolean_t src_needs_copy;

			/*
			 * XXX
			 * We currently ignore src_needs_copy.
			 * This really is the issue of how to make
			 * MEMORY_OBJECT_COPY_SYMMETRIC safe for
			 * non-kernel users to use. Solution forthcoming.
			 * In the meantime, since we don't allow non-kernel
			 * memory managers to specify symmetric copy,
			 * we won't run into problems here.
			 */
			new_object = object;
			new_offset = offset;
			success = vm_object_copy_quickly(&new_object,
							 new_offset, size,
							 &src_needs_copy,
							 &copy);
			assert(success);
			result = KERN_SUCCESS;
		}
		/*
		 *	Throw away the reference to the
		 *	original object, as it won't be mapped.
		 */

		vm_object_deallocate(object);

		if (result != KERN_SUCCESS)
			return (result);

		object = new_object;
		offset = new_offset;
	}

	if ((result = vm_map_enter(target_map,
				address, size, mask, anywhere,
				object, offset,
				copy,
				cur_protection, max_protection, inheritance
				)) != KERN_SUCCESS)
	vm_object_deallocate(object);
	return(result);
}


/*
 * NOTE: this routine (and this file) will no longer require mach_host_server.h
 * when vm_wire is changed to use ledgers.
 */
#include <mach/mach_host_server.h>
/*
 *	Specify that the range of the virtual address space
 *	of the target task must not cause page faults for
 *	the indicated accesses.
 *
 *	[ To unwire the pages, specify VM_PROT_NONE. ]
 */
kern_return_t
vm_wire(
	host_t			host,
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_prot_t		access)
{
	kern_return_t		rc;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	if (map == VM_MAP_NULL)
		return KERN_INVALID_TASK;

	if (access & ~VM_PROT_ALL)
		return KERN_INVALID_ARGUMENT;

	if (access != VM_PROT_NONE) {
		rc = vm_map_wire(map, trunc_page(start),
				 round_page(start+size), access, TRUE);
	} else {
		rc = vm_map_unwire(map, trunc_page(start),
				   round_page(start+size), TRUE);
	}
	return rc;
}

/*
 *	vm_msync
 *
 *	Synchronises the memory range specified with its backing store
 *	image by either flushing or cleaning the contents to the appropriate
 *	memory manager engaging in a memory object synchronize dialog with
 *	the manager.  The client doesn't return until the manager issues
 *	m_o_s_completed message.  MIG Magically converts user task parameter
 *	to the task's address map.
 *
 *	interpretation of sync_flags
 *	VM_SYNC_INVALIDATE	- discard pages, only return precious
 *				  pages to manager.
 *
 *	VM_SYNC_INVALIDATE & (VM_SYNC_SYNCHRONOUS | VM_SYNC_ASYNCHRONOUS)
 *				- discard pages, write dirty or precious
 *				  pages back to memory manager.
 *
 *	VM_SYNC_SYNCHRONOUS | VM_SYNC_ASYNCHRONOUS
 *				- write dirty or precious pages back to
 *				  the memory manager.
 *
 *	NOTE
 *	The memory object attributes have not yet been implemented, this
 *	function will have to deal with the invalidate attribute
 *
 *	RETURNS
 *	KERN_INVALID_TASK		Bad task parameter
 *	KERN_INVALID_ARGUMENT		both sync and async were specified.
 *	KERN_SUCCESS			The usual.
 */

kern_return_t
vm_msync(
	vm_map_t	map,
	vm_address_t	address,
	vm_size_t	size,
	vm_sync_t	sync_flags)
{
	msync_req_t		msr;
	msync_req_t		new_msr;
	queue_chain_t		req_q;	/* queue of requests for this msync */
	vm_map_entry_t		entry;
	vm_size_t		amount_left;
	vm_offset_t		offset;
	boolean_t		do_sync_req;
	boolean_t		modifiable;
	

	if ((sync_flags & VM_SYNC_ASYNCHRONOUS) &&
	    (sync_flags & VM_SYNC_SYNCHRONOUS))
		return(KERN_INVALID_ARGUMENT);

	/*
	 * align address and size on page boundaries
	 */
	size = round_page(address + size) - trunc_page(address);
	address = trunc_page(address);

        if (map == VM_MAP_NULL)
                return(KERN_INVALID_TASK);

	if (size == 0)
		return(KERN_SUCCESS);

	queue_init(&req_q);
	amount_left = size;

	while (amount_left > 0) {
		vm_size_t		flush_size;
		vm_object_t		object;

		vm_map_lock(map);
		if (!vm_map_lookup_entry(map, address, &entry)) {
			vm_size_t	skip;

			/*
			 * hole in the address map.
			 */

			/*
			 * Check for empty map.
			 */
			if (entry == vm_map_to_entry(map) &&
			    entry->vme_next == entry) {
				vm_map_unlock(map);
				break;
			}
			/*
			 * Check that we don't wrap and that
			 * we have at least one real map entry.
			 */
			if ((map->hdr.nentries == 0) ||
			    (entry->vme_next->vme_start < address)) {
				vm_map_unlock(map);
				break;
			}
			/*
			 * Move up to the next entry if needed
			 */
			skip = (entry->vme_next->vme_start - address);
			if (skip >= amount_left)
				amount_left = 0;
			else
				amount_left -= skip;
			address = entry->vme_next->vme_start;
			vm_map_unlock(map);
			continue;
		}

		offset = address - entry->vme_start;

		/*
		 * do we have more to flush than is contained in this
		 * entry ?
		 */
		if (amount_left + entry->vme_start + offset > entry->vme_end) {
			flush_size = entry->vme_end -
						 (entry->vme_start + offset);
		} else {
			flush_size = amount_left;
		}
		amount_left -= flush_size;
		address += flush_size;

		object = entry->object.vm_object;

		/*
		 * We can't sync this object if the object has not been
		 * created yet
		 */
		if (object == VM_OBJECT_NULL) {
			vm_map_unlock(map);
			continue;
		}

                vm_object_lock(object);

		/*
		 * We can't sync this object if there isn't a pager.
		 * Don't bother to sync internal objects, since there can't
		 * be any "permanent" storage for these objects anyway.
		 */
		if ((object->pager == IP_NULL) || (object->internal) ||
		    (object->private)) {
			vm_object_unlock(object);
			vm_map_unlock(map);
			continue;
		}
#if     NORMA_VM
		if (object->temporary) {
			vm_object_unlock(object);
			vm_map_unlock(map);
			continue;
		}
#endif  /* NORMA_VM */
		/*
		 * keep reference on the object until syncing is done
		 */
		assert(object->ref_count > 0);
		object->ref_count++;
		vm_object_res_reference(object);
		vm_object_unlock(object);

		offset += entry->offset;
		modifiable = (entry->protection & VM_PROT_WRITE)
				!= VM_PROT_NONE;

		vm_map_unlock(map);

		do_sync_req = memory_object_sync(object,
					offset,
					flush_size,
					sync_flags & VM_SYNC_INVALIDATE,
					(modifiable &&
					(sync_flags & VM_SYNC_SYNCHRONOUS ||
					 sync_flags & VM_SYNC_ASYNCHRONOUS)));

		/*
		 * only send a m_o_s if we returned pages or if the entry
		 * is writable (ie dirty pages may have already been sent back)
		 */
		if (!do_sync_req && !modifiable) {
			vm_object_deallocate(object);
			continue;
		}
		msync_req_alloc(new_msr);

                vm_object_lock(object);
		offset += object->paging_offset;

		new_msr->offset = offset;
		new_msr->length = flush_size;
		new_msr->object = object;
		new_msr->flag = VM_MSYNC_SYNCHRONIZING;
re_iterate:
		queue_iterate(&object->msr_q, msr, msync_req_t, msr_q) {
			/*
			 * need to check for overlapping entry, if found, wait
			 * on overlapping msr to be done, then reiterate
			 */
			msr_lock(msr);
			if (msr->flag == VM_MSYNC_SYNCHRONIZING &&
			    ((offset >= msr->offset && 
			      offset < (msr->offset + msr->length)) ||
			     (msr->offset >= offset &&
			      msr->offset < (offset + flush_size))))
			{
				assert_wait((event_t) msr, TRUE);
				msr_unlock(msr);
				vm_object_unlock(object);
				thread_block((void (*)(void))0);
				if (current_act()->handlers)
					act_execute_returnhandlers();
				vm_object_lock(object);
				goto re_iterate;
			}
			msr_unlock(msr);
		}/* queue_iterate */

		queue_enter(&object->msr_q, new_msr, msync_req_t, msr_q);
		vm_object_unlock(object);

		queue_enter(&req_q, new_msr, msync_req_t, req_q);

		(void) memory_object_synchronize(
				object->pager,
				object->pager_request,
				offset,
				flush_size,
				sync_flags);
	}/* while */

	/*
	 * wait for memory_object_sychronize_completed messages from pager(s)
	 */

	while (!queue_empty(&req_q)) {
		msr = (msync_req_t)queue_first(&req_q);
		msr_lock(msr);
		while(msr->flag != VM_MSYNC_DONE) {
			assert_wait((event_t) msr, TRUE);
			msr_unlock(msr);
			thread_block((void (*)(void))0);
			if (current_act()->handlers)
				act_execute_returnhandlers();
			msr_lock(msr);
		}/* while */
		queue_remove(&req_q, msr, msync_req_t, req_q);
		msr_unlock(msr);
		vm_object_deallocate(msr->object);
		msync_req_free(msr);
	}/* queue_iterate */

	return(KERN_SUCCESS);
}/* vm_msync */


/*
 *	task_wire
 *
 *	Set or clear the map's wiring_required flag.  This flag, if set,
 *	will cause all future virtual memory allocation to allocate
 *	user wired memory.  Unwiring pages wired down as a result of
 *	this routine is done with the vm_wire interface.
 */
kern_return_t
task_wire(
	vm_map_t	map,
	boolean_t	must_wire)
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (must_wire)
		map->wiring_required = TRUE;
	else
		map->wiring_required = FALSE;

	return(KERN_SUCCESS);
}

/*
 *	vm_behavior_set sets the paging behavior attribute for the 
 *	specified range in the specified map. This routine will fail
 *	with KERN_INVALID_ADDRESS if any address in [start,start+size)
 *	is not a valid allocated or reserved memory region.
 */
kern_return_t 
vm_behavior_set(
	vm_map_t		map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_behavior_t		new_behavior)
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_behavior_set(map, trunc_page(start), 
				   round_page(start+size), new_behavior));
}

#if	VM_CPM
/*
 *	Control whether the kernel will permit use of
 *	vm_allocate_cpm at all.
 */
unsigned int	vm_allocate_cpm_enabled = 1;

/*
 *	Ordinarily, the right to allocate CPM is restricted
 *	to privileged applications (those that can gain access
 *	to the host port).  Set this variable to zero if you
 *	want to let any application allocate CPM.
 */
unsigned int	vm_allocate_cpm_privileged = 0;

/*
 *	Allocate memory in the specified map, with the caveat that
 *	the memory is physically contiguous.  This call may fail
 *	if the system can't find sufficient contiguous memory.
 *	This call may cause or lead to heart-stopping amounts of
 *	paging activity.
 *
 *	Memory obtained from this call should be freed in the
 *	normal way, viz., via vm_deallocate.
 */
kern_return_t
vm_allocate_cpm(
	host_t			host,
	register vm_map_t	map,
	register vm_offset_t	*addr,
	register vm_size_t	size,
	boolean_t		anywhere)
{
	vm_object_t		cpm_obj;
	pmap_t			pmap;
	vm_page_t		m, pages;
	kern_return_t		kr;
	vm_offset_t		va, start, end, offset;
#if	MACH_ASSERT
	extern vm_offset_t	avail_start, avail_end;
	vm_offset_t		prev_addr;
#endif	/* MACH_ASSERT */

	if (!vm_allocate_cpm_enabled)
		return KERN_FAILURE;

	if (vm_allocate_cpm_privileged && host == HOST_NULL)
		return KERN_INVALID_HOST;

	if (map == VM_MAP_NULL)
		return KERN_INVALID_ARGUMENT;
	
	if (size == 0) {
		*addr = 0;
		return KERN_SUCCESS;
	}

	if (anywhere)
		*addr = vm_map_min(map);
	else
		*addr = trunc_page(*addr);
	size = round_page(size);

	if ((kr = cpm_allocate(size, &pages, TRUE)) != KERN_SUCCESS)
		return kr;

	cpm_obj = vm_object_allocate(size);
	assert(cpm_obj != VM_OBJECT_NULL);
	assert(cpm_obj->internal);
	assert(cpm_obj->size == size);
	assert(cpm_obj->can_persist == FALSE);
	assert(cpm_obj->pager_created == FALSE);
	assert(cpm_obj->pageout == FALSE);
	assert(cpm_obj->shadow == VM_OBJECT_NULL);

	/*
	 *	Insert pages into object.
	 */

	vm_object_lock(cpm_obj);
	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		m = pages;
		pages = NEXT_PAGE(m);

		assert(!m->gobbled);
		assert(!m->wanted);
		assert(!m->pageout);
		assert(!m->tabled);
		assert(m->busy);
		assert(m->phys_addr>=avail_start && m->phys_addr<=avail_end);

		m->busy = FALSE;
		vm_page_insert(m, cpm_obj, offset);
	}
	assert(cpm_obj->resident_page_count == size / PAGE_SIZE);
	vm_object_unlock(cpm_obj);

	/*
	 *	Hang onto a reference on the object in case a
	 *	multi-threaded application for some reason decides
	 *	to deallocate the portion of the address space into
	 *	which we will insert this object.
	 *
	 *	Unfortunately, we must insert the object now before
	 *	we can talk to the pmap module about which addresses
	 *	must be wired down.  Hence, the race with a multi-
	 *	threaded app.
	 */
	vm_object_reference(cpm_obj);

	/*
	 *	Insert object into map.
	 */

	kr = vm_map_enter(
			  map,
			  addr,
			  size,
			  (vm_offset_t)0,
			  anywhere,
			  cpm_obj,
			  (vm_offset_t)0,
			  FALSE,
			  VM_PROT_ALL,
			  VM_PROT_ALL,
			  VM_INHERIT_DEFAULT);

	if (kr != KERN_SUCCESS) {
		/*
		 *	A CPM object doesn't have can_persist set,
		 *	so all we have to do is deallocate it to
		 *	free up these pages.
		 */
		assert(cpm_obj->pager_created == FALSE);
		assert(cpm_obj->can_persist == FALSE);
		assert(cpm_obj->pageout == FALSE);
		assert(cpm_obj->shadow == VM_OBJECT_NULL);
		vm_object_deallocate(cpm_obj); /* kill acquired ref */
		vm_object_deallocate(cpm_obj); /* kill creation ref */
	}

	/*
	 *	Inform the physical mapping system that the
	 *	range of addresses may not fault, so that
	 *	page tables and such can be locked down as well.
	 */
	start = *addr;
	end = start + size;
	pmap = vm_map_pmap(map);
	pmap_pageable(pmap, start, end, FALSE);

	/*
	 *	Enter each page into the pmap, to avoid faults.
	 *	Note that this loop could be coded more efficiently,
	 *	if the need arose, rather than looking up each page
	 *	again.
	 */
	for (offset = 0, va = start; offset < size;
	     va += PAGE_SIZE, offset += PAGE_SIZE) {
		vm_object_lock(cpm_obj);
		m = vm_page_lookup(cpm_obj, offset);
		vm_object_unlock(cpm_obj);
		assert(m != VM_PAGE_NULL);
		PMAP_ENTER(pmap, va, m, VM_PROT_ALL, TRUE);
	}

#if	MACH_ASSERT
	/*
	 *	Verify ordering in address space.
	 */
	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		vm_object_lock(cpm_obj);
		m = vm_page_lookup(cpm_obj, offset);
		vm_object_unlock(cpm_obj);
		if (m == VM_PAGE_NULL)
			panic("vm_allocate_cpm:  obj 0x%x off 0x%x no page",
			      cpm_obj, offset);
		assert(m->tabled);
		assert(!m->busy);
		assert(!m->wanted);
		assert(!m->fictitious);
		assert(!m->private);
		assert(!m->absent);
		assert(!m->error);
		assert(!m->cleaning);
		assert(!m->precious);
		assert(!m->clustered);
		if (offset != 0) {
			if (m->phys_addr != prev_addr + PAGE_SIZE) {
				printf("start 0x%x end 0x%x va 0x%x\n",
				       start, end, va);
				printf("obj 0x%x off 0x%x\n", cpm_obj, offset);
				printf("m 0x%x prev_address 0x%x\n", m,
				       prev_addr);
				panic("vm_allocate_cpm:  pages not contig!");
			}
		}
		prev_addr = m->phys_addr;
	}
#endif	/* MACH_ASSERT */

	vm_object_deallocate(cpm_obj); /* kill extra ref */

	return kr;
}


#else	/* VM_CPM */

/*
 *	Interface is defined in all cases, but unless the kernel
 *	is built explicitly for this option, the interface does
 *	nothing.
 */

kern_return_t
vm_allocate_cpm(
	host_t			host,
	register vm_map_t	map,
	register vm_offset_t	*addr,
	register vm_size_t	size,
	boolean_t		anywhere)
{
	return KERN_FAILURE;
}
#endif	/* VM_CPM */
