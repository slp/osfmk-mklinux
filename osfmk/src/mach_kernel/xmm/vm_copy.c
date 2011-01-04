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
 * 05-Aug-92  Joseph Barrera (jsb) at Carnegie-Mellon University
 * Many many changes. Replaced vm_object_find, which could create an
 * arbitrarily large number of map entries at the destination, with
 * norma_get_shadow technology. Make better use of pagemaps. Straightened
 * out some of the internal/temporary/COPY_SYMMETRIC confusion.
 * Removed yield_transit cheating code (which was disabled anyway).
 *
 *
 * Revision 1.2  1992/12/07  21:31:08  robert
 * 	integrate any changes below for 14.0 (branch from 13.16 base)
 * 	[1992/12/06  20:58:17  robert]
 *
 * Revision 1.1.2.2  1992/12/06  21:55:20  robert
 * 	integrate any changes below for 14.0 (branch from 13.16 base)
 * 	[1992/12/06  20:58:17  robert]
 *
 * Revision 1.1.1.2  1992/12/06  20:58:17  robert
 * 	integrate any changes below for 14.0 (branch from 13.16 base)
 *
 * Revision 2.1.2.6  92/05/26  10:48:09  jsb
 * 	Fixed kalloc, copy object, and port leaks in vm_map_vector usage.
 * 	Added preliminary support for norma_task_clone and VM_INHERIT_SHARE.
 * 	Added norma_export_internal, which is based on the norma_copy_create
 * 	that was in vm/vm_object.c, but which now uses the exported and
 * 	importcount logic to reuse, share, and garbage collect temporary
 * 	objects correctly. Added memory_object_release_temporary, an upcall
 * 	which helps implement exported/importcount logic. Added new private
 * 	form of vm_map which currently understands temporary flag (to correctly
 * 	import the temporary object at the mapping end) and which will be
 * 	extended to handle lazy creation of exported shadow chains.
 * 
 * Revision 2.1.2.5  92/04/26  15:24:55  jsb
 * 	Merged in norma branch changes as of NORMA_MK9.
 * 
 * Revision 2.1.2.4  92/04/23  14:22:25  jsb
 * 	Use out-of-line form of vm_map_vector instead of old inline form.
 * 
 * Revision 2.1.2.3  92/04/22  14:50:56  jsb
 * 	Added support for batching form of vm_map. Added code to fake batching
 * 	of no-senders messages (to measure potential speedup).
 * 	Added code to walk down shadow chains until object containing page
 * 	is found. This allows remote caching of temporary objects to be
 * 	effective. It also allows remote copy-on-reference of object to
 * 	be done by remotely mapping the pager for the temporary object
 * 	instead of mapping a fake pager (implemented by xmm_copy.c) which
 * 	paged from the shadow chain starting at the temporary object.
 * 	The current implementation (in which this pager is mapped remotely
 * 	with no indication that it should be shadowed) will break if the
 * 	object mapped does not itself contain the page, since the resulting
 * 	data_unavailable will result in a zero-fill instead of a descent
 * 	down the shadow chain. The fix will require a modified vm_map.
 * 	(You are not expected to understand this.)
 * 
 * Revision 2.1.2.2  92/03/20  16:17:26  jsb
 * 	Improved debugging printfs.
 * 
 * Revision 2.1.2.1  92/03/13  08:29:48  jsb
 * 	Removed obsolete offset and size arguments to norma_copy_create.
 * 	Instead, pass offset to r_vm_map call.
 * 	[92/03/13            jsb]
 * 
 * Revision 2.1.1.1  92/03/04  17:55:24  jsb
 * 	First checkin.
 * 
 *	Joseph S. Barrera III 1992
 *	NORMA task address space copy support.
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1992 Carnegie Mellon University
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

#include <mach_pagemap.h>

#include <mach/machine/vm_types.h>
#include <mach/vm_param.h>
#include <mach/task_info.h>
#include <mach/task_special_ports.h>
#include <mach/mach_server.h>
#include <ipc/ipc_space.h>
#include <kern/mach_param.h>
#include <kern/task.h>
#include <kern/host.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <kern/processor.h>
#include <kern/ipc_tt.h>
#include <kern/misc_protos.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_user.h>
#include <kern/norma_protos.h>
#include <xmm/xmm_types.h>
#include <xmm/xmm_server_rename.h>
#include <xmm/xmm_internal.h>
#include <xmm/xmm_internal_server.h>
#include <ddb/tr.h>
#include <stdarg.h>

int vm_copy_debug = 0;
int vm_copy_debug_object = 0;
#define	dprintf	vm_copy_dprintf

extern vm_map_t kernel_map;

typedef struct vm_map_data	*vm_map_data_t;

typedef struct vm_map_vect	*vm_map_vect_t;

struct vm_map_data {
	memory_object_t	*vmd_memory_object;
	vm_object_t	*vmd_object;
	memory_object_t	*vmd_shadow;
	vm_map_vect_t	vmd_vmv;
	unsigned long	vmd_count;
	unsigned long	vmd_size;
};


struct vm_map_vect {
	vm_offset_t	vmv_start;
	vm_offset_t	vmv_end;
	vm_offset_t	vmv_offset;
	vm_size_t	vmv_size;
	vm_offset_t	vmv_shadow_soffset;
	vm_size_t	vmv_shadow_size;
	int		vmv___where;
	unsigned int
	/* boolean_t */	vmv_is_shared:1,
	/* boolean_t */	vmv_needs_copy:1,
	/* vm_prot_t */	vmv_protection:3,
	/* vm_prot_t */	vmv_max_protection:3,
	/* vm_inh._t */	vmv_inheritance:2,
	/* boolean_t */	vmv_temporary:1,
	/* boolean_t */	vmv_shadow_shadowed:1;
};

int vmd_start_size = 128;

/*
 * Forward.
 */
void		vmd_allocate(
			vm_map_data_t	vmd,
			int		vmd_size);

void		vmd_grow(
			vm_map_data_t	vmd);

void		vmd_free(
			vm_map_data_t	vmd);

void		vmd_add(
			vm_map_data_t	vmd,
			vm_object_t	object,
			vm_map_entry_t	entry,
			vm_offset_t	offset,
			boolean_t	dst_needs_copy,
			boolean_t	temporary,
			int		__where);

kern_return_t	task_copy_entry(
			vm_map_entry_t	entry,
			vm_object_t	object,
			vm_offset_t	offset,
			boolean_t	dst_needs_copy,
			vm_map_data_t	vmd,
			int		__where);

void		norma_fork_share(
			vm_map_t	old_map,
			vm_map_entry_t	old_entry,
			vm_map_data_t	vmd);

void		norma_fork_teleport(
			vm_map_t	old_map,
			vm_map_entry_t	old_entry,
			vm_map_data_t	vmd);

boolean_t	norma_fork_copy_quickly(
			vm_map_t	old_map,
			vm_map_entry_t	old_entry,
			vm_map_data_t	vmd);

boolean_t	norma_fork_copy(
			vm_map_t	old_map,
			vm_map_entry_t	*old_entry_p,
			vm_map_data_t	vmd);

vm_object_t	vm_object_chain_enter(
			ipc_port_t	host,
			ipc_port_t	pager0,
			vm_size_t	pager0_size,
			ipc_port_t	pager1,
			vm_size_t	pager1_size,
			vm_offset_t	pager1_soffset,
			boolean_t	pager1_shadowed);

kern_return_t	vm_map_vector_common(
			host_t		host,
			vm_map_t	map,
			vm_map_vector_t	vmv_copy,
			unsigned	vmv_count,
			memory_object_t	*memory_object,
			unsigned	count,
			memory_object_t	*shadow,
			unsigned	count2,
			boolean_t	remap,
			vm_offset_t	*address,
			vm_size_t	size,
			vm_offset_t	mask,
			boolean_t	anywhere,
			ipc_port_t	parent_host);

void		vm_copy_dprintf(
			const char *fmt, ...);

unsigned long	vmd_uid(
			memory_object_t	mo);

void		vmd_print(
			vm_map_data_t	vmd);

void		vm_copy_print_object(
			vm_object_t	object);

void		vm_copy_print_entry(
			vm_map_entry_t	entry);

void		show_vmv(vm_map_vect_t vmv);


void
vmd_allocate(
	vm_map_data_t	vmd,
	int		vmd_size)
{
	vmd->vmd_count = 0;
	vmd->vmd_size = vmd_size;
	vmd->vmd_vmv = (vm_map_vect_t)
			kalloc(vmd->vmd_size * sizeof(vmd->vmd_vmv[0]));
	vmd->vmd_memory_object = (memory_object_t *)
	    kalloc((vm_size_t) (vmd->vmd_size * sizeof(memory_object_t)));
	vmd->vmd_object = (vm_object_t *)
	    kalloc((vm_size_t) (vmd->vmd_size * sizeof(vm_object_t)));
	vmd->vmd_shadow = (memory_object_t *)
	    kalloc((vm_size_t) (vmd->vmd_size * sizeof(memory_object_t)));
}

#define	vmd_grow_if_needed(vmd) if (vmd->vmd_count == vmd->vmd_size) vmd_grow(vmd)

void
vmd_grow(
	vm_map_data_t	vmd)
{
	struct vm_map_data old_vmd0;
	vm_map_data_t old_vmd = &old_vmd0;
	kern_return_t kr;

	*old_vmd = *vmd;
	vmd_allocate(vmd, (int) (2 * old_vmd->vmd_size));
	vmd->vmd_count = old_vmd->vmd_count;
	bcopy((char *) old_vmd->vmd_vmv, (char *) vmd->vmd_vmv,
	      old_vmd->vmd_count * sizeof(old_vmd->vmd_vmv[0]));
	bcopy((char *) old_vmd->vmd_memory_object,
	      (char *) vmd->vmd_memory_object,
	      old_vmd->vmd_count * sizeof(old_vmd->vmd_memory_object[0]));
	bcopy((char *) old_vmd->vmd_object, (char *) vmd->vmd_object,
	      old_vmd->vmd_count * sizeof(old_vmd->vmd_object[0]));
	bcopy((char *) old_vmd->vmd_shadow, (char *) vmd->vmd_shadow,
	      old_vmd->vmd_count * sizeof(old_vmd->vmd_shadow[0]));
	kfree((vm_offset_t) old_vmd->vmd_vmv,
	      (vm_size_t) (old_vmd->vmd_size * sizeof(old_vmd->vmd_vmv[0])));
	kfree((vm_offset_t) old_vmd->vmd_memory_object,
	      (vm_size_t) (old_vmd->vmd_size * sizeof(memory_object_t)));
	kfree((vm_offset_t) old_vmd->vmd_object,
	      (vm_size_t) (old_vmd->vmd_size * sizeof(vm_object_t)));
	kfree((vm_offset_t) old_vmd->vmd_shadow,
	      (vm_size_t) (old_vmd->vmd_size * sizeof(memory_object_t)));
}

void
vmd_free(
	vm_map_data_t	vmd)
{
	kfree((vm_offset_t) vmd->vmd_vmv,
	      (vm_size_t) (vmd->vmd_size * sizeof(vmd->vmd_vmv[0])));
	kfree((vm_offset_t) vmd->vmd_memory_object,
	      (vm_size_t) (vmd->vmd_size * sizeof(memory_object_t)));
	kfree((vm_offset_t) vmd->vmd_object,
	      (vm_size_t) (vmd->vmd_size * sizeof(vm_object_t)));
	kfree((vm_offset_t) vmd->vmd_shadow,
	      (vm_size_t) (vmd->vmd_size * sizeof(memory_object_t)));
}

/*
 *	Routine:	vmd_add
 *	Purpose:
 *		Copy object information from object and map
 *		entry into a vector to be given to a remote
 *		node.
 *	In/out conditions:
 *		The object is locked on entry and exit.
 */
void
vmd_add(
	vm_map_data_t	vmd,
	vm_object_t	object,
	vm_map_entry_t	entry,
	vm_offset_t	offset,
	boolean_t	dst_needs_copy,
	boolean_t	temporary,
	int		__where)
{
	vm_map_vect_t vmv = &vmd->vmd_vmv[vmd->vmd_count];

	assert(vmd->vmd_count < vmd->vmd_size);

	vmd->vmd_object[vmd->vmd_count] = object;

	if (object == VM_OBJECT_NULL) {
		vmd->vmd_memory_object[vmd->vmd_count] = MACH_PORT_NULL;
		vmd->vmd_shadow[vmd->vmd_count] = MACH_PORT_NULL;
		vmv->vmv_shadow_soffset = 0;
		vmv->vmv_shadow_shadowed = FALSE;
	} else if (object->shadow == VM_OBJECT_NULL) {
		vmd->vmd_memory_object[vmd->vmd_count] =
		    (mach_port_t) ipc_port_copy_send(object->pager);
		vmv->vmv_size = object->size;
		vmd->vmd_shadow[vmd->vmd_count] = MACH_PORT_NULL;
		vmv->vmv_shadow_soffset = 0;
		vmv->vmv_shadow_shadowed = FALSE;
	} else {
		vmd->vmd_memory_object[vmd->vmd_count] =
		    (mach_port_t) ipc_port_copy_send(object->pager);
		vmv->vmv_size = object->size;
		vmd->vmd_shadow[vmd->vmd_count] =
		    (mach_port_t) ipc_port_copy_send(object->shadow->pager);
		vmv->vmv_shadow_size = object->shadow->size;
		vmv->vmv_shadow_soffset = object->shadow_offset;
		vmv->vmv_shadow_shadowed =
		    (object->shadow->shadow != VM_OBJECT_NULL);
	}

	vmv->vmv_start = entry->vme_start;
	vmv->vmv_end = entry->vme_end;
	vmv->vmv_offset = offset;
	vmv->vmv_is_shared = entry->is_shared;
	vmv->vmv_needs_copy = dst_needs_copy;
	vmv->vmv_protection = entry->protection;
	vmv->vmv_max_protection = entry->max_protection;
	vmv->vmv_inheritance = entry->inheritance;
	vmv->vmv_temporary = temporary;
	vmv->vmv___where = __where;

	vmd->vmd_count++;
}

/*
 * Caller donates object reference, which is kept in vmd info and released
 * after vm_map_vector call returns. Object must be locked.
 * Object must not be null.
 */
int collapse_hack = 0;
kern_return_t
task_copy_entry(
	vm_map_entry_t	entry,
	vm_object_t	object,
	vm_offset_t	offset,
	boolean_t	dst_needs_copy,
	vm_map_data_t	vmd,
	int		__where)
{
	vm_object_t shadow;

	assert(page_aligned(entry->vme_start));
	assert(page_aligned(entry->vme_end));
	assert(entry->vme_end > entry->vme_start);
	assert(object != VM_OBJECT_NULL);

	if (collapse_hack) {
		vm_object_collapse(object);
	}

	if (object->internal && ! object->pager_initialized) {
		vm_object_pager_create(object);
	}

	if (object->shadow != VM_OBJECT_NULL) {
		shadow = object->shadow;
		vm_object_lock(shadow);
		if (shadow->internal && ! shadow->pager_initialized) {
			vm_object_unlock(object);
			vm_object_pager_create(shadow);
			vm_object_unlock(shadow);
			vm_object_lock(object);
		} else
			vm_object_unlock(shadow);
	}

	vmd_add(vmd, object, entry, offset, dst_needs_copy,
		(boolean_t) object->temporary, __where);

	return KERN_SUCCESS;
}

void
norma_fork_share(
	vm_map_t	old_map,
	vm_map_entry_t	old_entry,
	vm_map_data_t	vmd)
{
	vm_object_t object;

	/*
	 *	New sharing code.  New map entry
	 *	references original object.  Internal
	 *	objects use asymmetric copy algorithm for
	 *	future copies.  First make sure we have
	 *	the right object.  If we need a shadow,
	 *	or someone else already has one, then
	 *	make a new shadow and share it.
	 */
	
	object = old_entry->object.vm_object;
	if (object == VM_OBJECT_NULL) {
		object = vm_object_allocate((vm_size_t)(old_entry->vme_end -
							old_entry->vme_start));
		old_entry->offset = 0;
		old_entry->object.vm_object = object;
		assert(!old_entry->needs_copy);
	} else if (object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC) {
		
		/*
		 *	We are already using an asymmetric
		 *	copy, and therefore we already have
		 *	the right object.
		 */
		
		/* COPY_CALL, copy might have been deallocated since */
		assert((!old_entry->needs_copy) ||
		       (object->copy_strategy == MEMORY_OBJECT_COPY_CALL &&
			object->ref_count == 1));
	} else if (old_entry->needs_copy ||	/* case 1 */
		   object->shadowed ||		/* case 2 */
		   (! old_entry->is_shared &&	/* case 3 */
		    object->size >
		    (vm_size_t)(old_entry->vme_end - old_entry->vme_start))) {
		
		/*
		 *	We need to create a shadow.
		 *	There are three cases here.
		 *	In the first case, we need to
		 *	complete a deferred symmetrical
		 *	copy that we participated in.
		 *	In the second and third cases,
		 *	we need to create the shadow so
		 *	that changes that we make to the
		 *	object do not interfere with
		 *	any symmetrical copies which
		 *	have occured (case 2) or which
		 *	might occur (case 3).
		 */
		
		assert(! (object->shadowed && old_entry->is_shared));
		vm_object_shadow(&old_entry->object.vm_object,
				 &old_entry->offset,
				 (vm_size_t)
				 (old_entry->vme_end - old_entry->vme_start));

		/*
		 *	If we're making a shadow for other than
		 *	copy on write reasons, then we have
		 *	to remove write permission.
		 */

		if (!old_entry->needs_copy &&
		    (old_entry->protection & VM_PROT_WRITE)) {
			pmap_protect(vm_map_pmap(old_map),
				     old_entry->vme_start,
				     old_entry->vme_end,
				     old_entry->protection &
				     ~VM_PROT_WRITE);
		}
		old_entry->needs_copy = FALSE;
		object = old_entry->object.vm_object;
	}
	
	/*
	 *	If object was using a symmetric copy strategy,
	 *	change its copy strategy to the default
	 *	asymmetric copy strategy, which is copy_delay
	 *	in the non-norma case and copy_call in the
	 *	norma case. Bump the reference count for the
	 *	new entry.
	 */

	vm_object_lock(object);
	
	if (object->internal) {
		/*
		 * If there is no pager, or if a pager is currently
		 * being created, then wait until it has been created.
		 */
		if (! object->pager_initialized) {
			vm_object_pager_create(object);
		}
		assert(object->pager_initialized);
	}

	if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
		assert(object->temporary);
		object->copy_strategy = MEMORY_OBJECT_COPY_CALL;

		/*
		 *	Notify xmm about change in copy strategy,
		 *	so that it will give correct copy strategy
		 *	to other kernels that map this object.
		 */
		vm_object_unlock(object);
		memory_object_share(object->pager, object->pager_request);
		vm_object_lock(object);
	}

#if	MACH_PAGEMAP
	/*
	 * Since this object will be shared, any existence
	 * info will become fatally stale.
	 */
	if (object->existence_map != VM_EXTERNAL_NULL) {
		vm_external_destroy(object->existence_map, object->size);
	}
#endif	/* MACH_PAGEMAP */

	object->ref_count++;
	vm_object_res_reference(object);
	old_entry->is_shared = TRUE;	/* XXX Is this correct? */

	task_copy_entry(old_entry, old_entry->object.vm_object,
			old_entry->offset, FALSE, vmd, __LINE__);
	vm_object_unlock(object);
}

#define	XXX_ALLOW_TELEPORT_DISABLE	1
#if	XXX_ALLOW_TELEPORT_DISABLE
boolean_t	norma_fork_teleport_disable = FALSE;
#endif	/* XXX_ALLOW_TELEPORT_DISABLE */

/*
 * Migrate memory to destination, copying when possible.
 */
void
norma_fork_teleport(
	vm_map_t	old_map,
	vm_map_entry_t	old_entry,
	vm_map_data_t	vmd)
{
	vm_object_t object;

	/*
	 *	Norma_fork_teleport uses copy instead of sharing
	 *	when cloning, taking advantage of the fact that we are killing
	 *	the parent and no one else will remain to share the object.
	 *	Copying is better when object is copy-symmetric, since the
	 *	migration cost is the same and subsequent copying costs are
	 *	reduced (since we can continue to use copy-symmetric, instead
	 *	of copy-call which has higher startup costs, requiring an rpc
	 *	to the node holding the xmm obj).	 
	 */

#if	XXX_ALLOW_TELEPORT_DISABLE
	if (norma_fork_teleport_disable) {
		norma_fork_share(old_map, old_entry, vmd);
		return;
	}
#endif	/* XXX_ALLOW_TELEPORT_DISABLE */

	object = old_entry->object.vm_object;
	if (object == VM_OBJECT_NULL ||
	    object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {

		/*
		 *	The object is null, or has a symmetric copy strategy,
		 *	so copy it.
		 */

		if (! norma_fork_copy_quickly(old_map, old_entry, vmd)) {
			norma_fork_copy(old_map, &old_entry, vmd);
		}
	} else {

		/*
		 *	We may not copy the object,
		 *	so share it.
		 */

		norma_fork_share(old_map, old_entry, vmd);
	}
}

boolean_t
norma_fork_copy_quickly(
	vm_map_t	old_map,
	vm_map_entry_t	old_entry,
	vm_map_data_t	vmd)
{
	boolean_t src_needs_copy;
	boolean_t dst_needs_copy;
	vm_object_t object;
	vm_offset_t offset;
	
	if (old_entry->wired_count != 0) {
		return FALSE;
	}
		
	object = old_entry->object.vm_object;
	if (object == VM_OBJECT_NULL) {
		vmd_add(vmd, VM_OBJECT_NULL, old_entry, 0, FALSE, FALSE,
			__LINE__);
		return TRUE;
	}

	offset = old_entry->offset;

	/*
	 * Create the pager now so we can distribute the pagemap
	 */
	vm_object_lock(object);
	if (object->internal && ! object->pager_initialized) {
		vm_object_pager_create(object);
	}
	vm_object_unlock(object);

	if (! vm_object_copy_quickly(&object, offset, object->size,
				     &src_needs_copy, &dst_needs_copy)) {
		return FALSE;
	}

	/*
	 *	Handle copy-on-write obligations
	 */
	
	if (src_needs_copy && ! old_entry->needs_copy) {
		vm_object_pmap_protect(old_entry->object.vm_object,
				       old_entry->offset,
				       (old_entry->vme_end -
					old_entry->vme_start),
				       (old_entry->is_shared ?
					PMAP_NULL :
					old_map->pmap),
				       old_entry->vme_start,
				       old_entry->protection &
				       ~VM_PROT_WRITE);

		old_entry->needs_copy = TRUE;
	}
	if (src_needs_copy && 
	    old_entry->object.vm_object->internal &&
	    !old_entry->object.vm_object->pagemap_exported)
		vm_object_export_pagemap_try(
					     old_entry->object.vm_object,
					     old_entry->object.vm_object->size
#if 0
					     , (old_entry->vme_end -
						old_entry->vme_start)
#endif
					     );
	vm_object_lock(object);
	task_copy_entry(old_entry, object, offset,
			dst_needs_copy, vmd, __LINE__);
	vm_object_unlock(object);

	return TRUE;
}

boolean_t
norma_fork_copy(
	vm_map_t	old_map,
	vm_map_entry_t	*old_entry_p,
	vm_map_data_t	vmd)
{
	vm_map_entry_t old_entry = *old_entry_p;
	vm_size_t entry_size = old_entry->vme_end - old_entry->vme_start;
	vm_offset_t start = old_entry->vme_start;
	vm_map_copy_t copy;
	vm_map_entry_t next_entry, new_entry;
	vm_object_t object;

	vm_map_unlock(old_map);
	if (vm_map_copyin(old_map, start, entry_size, FALSE, &copy)
	    != KERN_SUCCESS) {
		vm_map_lock(old_map);
		if (!vm_map_lookup_entry(old_map, start, &next_entry)) {
			next_entry = next_entry->vme_next;
		}
		*old_entry_p = next_entry;

		/*
		 *	For some error returns, want to
		 *	skip to the next element.
		 */
		
		return FALSE;
	}
	
	/*
	 *	XXX
	 *	If we started with one entry, we ought to end up
	 *	with just one copy... right?
	 */

	assert(vm_map_copy_first_entry(copy) ==
	       vm_map_copy_last_entry(copy));

	/*
	 *	Pick up the traversal at the end of
	 *	the copied region.
	 */
	
	vm_map_lock(old_map);
	start += entry_size;
	if (! vm_map_lookup_entry(old_map, start, &next_entry)) {
		next_entry = next_entry->vme_next;
	} else if (start > next_entry->vme_start) {
		_vm_map_clip_start(&old_map->hdr, next_entry, start);
	}
	*old_entry_p = next_entry;

	new_entry = vm_map_copy_first_entry(copy);
	object = new_entry->object.vm_object;
	assert(object != VM_OBJECT_NULL);
	vm_object_lock(object);
	object->ref_count++;
	vm_object_res_reference(object);
	task_copy_entry(old_entry, object, new_entry->offset,
			new_entry->needs_copy, vmd, __LINE__);
	vm_object_unlock(object);
	vm_map_copy_discard(copy);

	return TRUE;
}

void
task_copy_vm(
	ipc_port_t	host,
	vm_map_t	old_map,
	boolean_t	clone,
	boolean_t	kill_parent,
	ipc_port_t	to)
{
	kern_return_t		kr;
	vm_map_entry_t		entry;
	struct vm_map_data	vmd0;
	register vm_map_data_t	vmd = &vmd0;
	vm_map_copy_t		vector_copy;
	boolean_t		teleport = (clone && kill_parent);
	memory_object_t		*memory_object_buf, *shadow_buf;
	vm_size_t		buffer_size;
	int i;
	
	/* XXX I think some code here depends on the following assertion */
	assert(host != realhost.host_priv_self);

	vmd_allocate(vmd, vmd_start_size);

	vm_map_lock(old_map);

	entry = vm_map_first_entry(old_map);
	while (entry != vm_map_to_entry(old_map)) {
		vmd_grow_if_needed(vmd); /* before grabbing simple locks */
		if (entry->is_sub_map) {
			panic("norma_fork: encountered a submap");
		}
#if 666
		vm_copy_print_entry(entry);
#endif
		if (teleport) {
			norma_fork_teleport(old_map, entry, vmd);
		} else if (clone || entry->inheritance == VM_INHERIT_SHARE) {
			norma_fork_share(old_map, entry, vmd);
		} else if (entry->inheritance == VM_INHERIT_COPY) {
			if (! norma_fork_copy_quickly(old_map, entry, vmd)) {
				norma_fork_copy(old_map, &entry, vmd);
				continue;
			}
		} else {
			assert(entry->inheritance == VM_INHERIT_NONE);
		}
		entry = entry->vme_next;
	}

	vm_map_unlock(old_map);

	vmd_print(vmd);

	/*
	 * allocate a KERNEL_BUFFER copy_t to point to the vector.
	 * vm_map_vector is an rpc, so we don't need to copy the data.
	 */

	vector_copy = (vm_map_copy_t)kalloc(sizeof(struct vm_map_copy));
	assert(vector_copy != VM_MAP_COPY_NULL);
	vector_copy->type = VM_MAP_COPY_KERNEL_BUFFER;
	vector_copy->size = vmd->vmd_count * sizeof(vmd->vmd_vmv[0]);
	vector_copy->offset = 0;
	vector_copy->cpy_kdata = (vm_offset_t)vmd->vmd_vmv;
	vector_copy->cpy_kalloc_size = sizeof(struct vm_map_copy);

	buffer_size = vmd->vmd_count * sizeof(memory_object_t);
	assert(sizeof(&vmd->vmd_shadow[0]) ==
	       sizeof(&vmd->vmd_memory_object[0]));

	memory_object_buf = (memory_object_t *) kalloc(buffer_size);
	shadow_buf = (memory_object_t *) kalloc(buffer_size);

	(void) bcopy((char *)vmd->vmd_memory_object,
		     (char *) memory_object_buf, buffer_size);
	(void) bcopy((char *) vmd->vmd_shadow,
		     (char *) shadow_buf, buffer_size);

	kr = r_vm_map_vector(host, to, (vm_map_vector_t)vector_copy,
			     vmd->vmd_count *
			     (sizeof(vmd->vmd_vmv[0]) / sizeof(long)),
			     memory_object_buf, vmd->vmd_count,
			     shadow_buf, vmd->vmd_count,
			     realhost.host_priv_self);

	for (i = 0; i < vmd->vmd_count; i++) {
		vm_object_deallocate(vmd->vmd_object[i]);
	}
	vmd_free(vmd);
}

/*
 *	The following unfortunate macros allow
 *	vm_object_chain_enter to be structured
 *	recursively without actually using recursion.
 *	Avoiding recursion is important because
 *	kernel stacks are not large and because
 *	there is no intrinsic limit on the length
 *	of a shadow chain, particularly since the
 *	shadow chain collapsing code is not hard
 *	to fake out.
 */

int XXnoise = 0;
vm_offset_t XXsoffset = 0;

#define	BEGIN()\
	struct context {\
		vm_object_t	object0;\
		ipc_port_t	pager1;\
		vm_offset_t	pager1_soffset;\
		struct context	*old_context;\
	} *context = (struct context *) 0, *old_context;\
	XXsoffset = pager1_soffset;\
	if(XXnoise && XXsoffset) printf("XXsoffset=0x%x\n",XXsoffset);\
_call:

#define	CALL_SELF()\
	old_context = context;\
	context = (struct context *) kalloc(sizeof(*context));\
	assert(context);\
	context->object0 = object0;\
	context->pager1 = pager1;\
	context->pager1_soffset = pager1_soffset;\
	context->old_context = old_context;\
	pager0 = pager1;\
	pager0_size = pager1_size;\
	pager1 = pager2;\
	pager1_size = pager2_size;\
	pager1_soffset = pager2_soffset;\
	pager1_shadowed = pager2_shadowed;\
	goto _call;\
_return:\
	object1 = object0;\
	object0 = context->object0;\
	pager1 = context->pager1;\
	pager1_soffset = context->pager1_soffset;\
	old_context = context->old_context;\
	kfree((vm_offset_t) context, sizeof(*context));\
	context = old_context;

#define	RETURN(value)\
	object0 = (value);\
	if (context) {\
		goto _return;\
	} else {\
		return object0;\
	}

vm_object_t
vm_object_chain_enter(
	ipc_port_t	host,
	ipc_port_t	pager0,
	vm_size_t	pager0_size,
	ipc_port_t	pager1,
	vm_size_t	pager1_size,
	vm_offset_t	pager1_soffset,
	boolean_t	pager1_shadowed)
{
	vm_object_t	object0;
	vm_object_t	object1;
	ipc_port_t	pager2;
	vm_size_t	pager2_size;
	vm_offset_t	pager2_soffset;
	boolean_t	pager2_shadowed;
	vm_object_t	rewalk_object = VM_OBJECT_NULL;

	BEGIN();

	/*
	 * Find or create an object for this memory object.
	 * If we don't have to connect its shadow, then we're done.
	 */
	object0 = vm_object_enter(pager0, pager0_size,
				  (pager1 != IP_NULL &&
				   !IP_IS_REMOTE(pager0)), FALSE);
	if (object0 == VM_OBJECT_NULL) {
		RETURN(VM_OBJECT_NULL);
	}

	/*
	 * We wait for the object to be ready so that we can
	 * examine its copy strategy.
	 */
	vm_object_lock(object0);
	if (!object0->internal)
		while (! object0->pager_ready) {
			vm_object_wait(object0,
				       VM_OBJECT_EVENT_PAGER_READY, FALSE);
			vm_object_lock(object0);
		}
	else if (!object0->pager_ready) {
		object0->pager_ready = TRUE;
		vm_object_wakeup(object0, VM_OBJECT_EVENT_PAGER_READY);
	}
	vm_object_unlock(object0);

	if (pager1 == IP_NULL) {
		RETURN(object0);
	}
	if (object0->shadow) {
		/*
		 * Note that in this case, it may be that
		 * object0->shadow->pager != pager1,
		 * for example if object0 shadowed a
		 * copy_call object, and a new copy was
		 * pushed. There may be collapsing examples
		 * as well. In general, we assume that if
		 * object0 has a shadow, then it is a
		 * correct shadow.
		 *
		 * XXX
		 * There is however a race between installing
		 * a shadow chain and having it be expanded by
		 * create_copy. We can fix this by, when
		 * we come to a shadow which is a permanent
		 * copy-call object that we don't yet have
		 * entered, entering the object and then
		 * backing up to what we think is its shadow
		 * and walking down the chain once more.
		 * If the chain expanded before the entering,
		 * we'll find the new shadows this time.
		 * If the chain expands after the entering,
		 * we'll be told about it (and will remember).
		 * Only tricky thing here is adding a bit
		 * of state so that we can differentiate
		 * between the first and second passes; just
		 * requires adding a parameter to our hacked
		 * up nonrecursive recursive function, yuck.
		 * (We cannot infer from the fact that the object
		 * is entered that we are in the second pass.)
		 * We probably also have to make create_copy
		 * understand why we don't have an old copy,
		 * or actually since we'll have to point the
		 * shadow we have at it, we'll have to explain
		 * why we have the wrong copy.
		 */
		RETURN(object0);
	}

	/*
	 * This object does not have a local shadow, even though it should.
	 * If the shadow has a shadow, get it now.
	 */
	if (pager1_shadowed) {
		kern_return_t kr;
		kr = r_xmm_get_shadow(host, pager1, &pager2, &pager2_size,
				      &pager2_soffset, &pager2_shadowed);
		if (kr != KERN_SUCCESS) {
			panic("vm_object_chain_enter: get_shadow: 0x%x\n", kr);
		}
	} else {
		pager2 = IP_NULL;
		pager2_size = 0;
		pager2_soffset = 0;
		pager2_shadowed = FALSE;
	}

	/* object1 = */ CALL_SELF();

	/*
	 * XXX
	 * This is where we should grab the lock again.
	 */
	if (object0->shadow) {
		assert(object0->shadow->pager == pager1);
		vm_object_deallocate(object1);
	} else if (object1->copy_strategy != MEMORY_OBJECT_COPY_CALL) {
		vm_object_lock(object0);
		object0->shadow = object1;
		object0->shadow_offset = pager1_soffset;
		vm_object_unlock(object0);
	} else if (rewalk_object == VM_OBJECT_NULL) {
		/*
		 * While we were entering object1, new copy-call shadows
		 * may have been inserted between object0 and object1.
		 * Store our reference to object1 in rewalk_object,
		 * and rewalk from object0 down to object1, picking
		 * up any new copy-call shadows.
		 */
/*		panic("rewalk FALSE, copy 0x%x and shadow 0x%x\n",
		      object0, object1);*/
		rewalk_object = object1;
		goto rewalk_entry;
	} else {
#if 666
		/*
		 * XXX
		 * Need to do this for real!
		 */
rewalk_entry:
		vm_object_lock(object0);
		vm_object_lock(object1);
		rewalk_object->ref_count++;
		VM_OBJ_RES_INCR(rewalk_object);
#endif
		/*
		 * We have completed a rewalk. Release the reference
		 * to the rewalk object (since we should have a new
		 * reference to the same object in object1), and
		 * connect the rewalk object with its copy-call object.
		 */
/*		panic("rewalk=0x%x, copy=0x%x, shadow=0x%x\n",
		      rewalk_object, object0, object1);*/
		assert(rewalk_object == object1);
		assert(rewalk_object->ref_count >= 2);
		rewalk_object->ref_count--;
		VM_OBJ_RES_DECR(rewalk_object);
		rewalk_object = VM_OBJECT_NULL;
		object0->shadow = object1;
		object0->shadow_offset = pager1_soffset;
		object1->copy = object0;
		vm_object_unlock(object1);
		vm_object_unlock(object0);
	}
	RETURN(object0);
}

#undef	BEGIN
#undef	CALL_SELF
#undef	RETURN

kern_return_t
vm_map_vector_common(
	host_t		host,
	vm_map_t	map,
	vm_map_vector_t	vmv_copy,
	unsigned	vmv_count,
	memory_object_t	*memory_object,
	unsigned	count,
	memory_object_t	*shadow,
	unsigned	count2,
	boolean_t	remap,
	vm_offset_t	*address,
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	ipc_port_t	parent_host)
{
	unsigned i, j;
	kern_return_t kr, result;
	vm_map_vect_t vmv, vmv_start;
	vm_size_t vmv_size;
	vm_size_t vsize;
	vm_offset_t end;
	vm_object_t *object;
	vm_map_entry_t insp_entry;
	vm_map_copy_t new_copy;
	TR_DECL("vm_map_vector_common");

	tr5("enter: host 0x%x map 0x%x vmv_copy 0x%x vmv_count 0x%x",
	    host, map, vmv_copy, vmv_count);
	tr5("enter: *object 0x%x count 0x%x *shadow 0x%x count 0x%x",
	    memory_object, count, shadow, count2);
	tr5("enter: %sremap *address 0x%x size 0x%x mask 0x%x",
	    (remap?" ":"!"), address, size, mask);
	tr3("enter: %sanywhere parent_host 0x%x",
	    (anywhere?" ":"!"), parent_host);
	if (parent_host == MACH_PORT_NULL || !IP_VALID(parent_host)) {
		printf("vm_map_vector_common: invalid parent_host port\n");
		tr1("exit: KERN_INVALID_ARGUMENT");
		return KERN_INVALID_ARGUMENT;
	}

	if (host == HOST_NULL || map == VM_MAP_NULL ||
	    count == 0 || count != count2 ||
	    vmv_count * sizeof (int) != count * sizeof (struct vm_map_vect)) {
		result = KERN_INVALID_ARGUMENT;
		tr1("arg failure");
		goto end;
	}

	new_copy = vm_map_copy_copy((vm_map_copy_t)vmv_copy);
	vmv_size = new_copy->size;
	result = vm_map_copyout(kernel_map,
				(vm_offset_t *)&vmv_start, new_copy);
	if (result != KERN_SUCCESS) {
		vm_map_copy_discard(new_copy);
		tr1("vm_map_copyout failure");
		goto end;
	}

	object = (vm_object_t *)kalloc(sizeof(vm_object_t) * count);
	assert(object != (vm_object_t *)0);

	/*
	 * Firstly validate entries and allocates needed vm_objects
	 * without map locked.
	 */
	end = 0;
	for (i = 0, vmv = vmv_start; i < count; i++, vmv++) {
		/*
		 *	Verify that entries are ordered by memory addresses
		 *	and that remap entries are contiguous.
		 */
		if (vmv->vmv_start < end ||
		    vmv->vmv_start >= vmv->vmv_end ||
		    (remap && end != 0 && vmv->vmv_start != end)) {
			result = KERN_INVALID_ADDRESS;
			tr1("misordered entries");
			for (j = 0; j < i; j++)
				if (object[j] != VM_OBJECT_NULL)
					vm_object_deallocate(object[j]);
			goto cleanup;
		}
		end = vmv->vmv_end;

		/*
		 *	Verify that entry start address and size
		 *	are page rouded.
		 */
		vsize = vmv->vmv_end - vmv->vmv_start;
		if (vmv->vmv_start != trunc_page(vmv->vmv_start) ||
		    vsize != round_page(vsize)) {
			tr1("misrounded addresses");
			result = KERN_INVALID_ADDRESS;
			for (j = 0; j < i; j++)
				if (object[j] != VM_OBJECT_NULL)
					vm_object_deallocate(object[j]);
			goto cleanup;
		}

		/*
		 *	Verify inheritance value and protection bits.
		 */
		switch (vmv->vmv_inheritance) {
		case VM_INHERIT_NONE:
		case VM_INHERIT_COPY:
		case VM_INHERIT_SHARE:
			if ((vmv->vmv_max_protection & ~VM_PROT_ALL) == 0 &&
			    (vmv->vmv_protection & ~VM_PROT_ALL) == 0) 
				break;
			/*FALL THRU*/
		default:
			tr1("bogus inheritance");
			show_vmv(vmv);
			result = KERN_INVALID_ARGUMENT;
			for (j = 0; j < i; j++)
				if (object[j] != VM_OBJECT_NULL)
					vm_object_deallocate(object[j]);
			goto cleanup;
		}

		if (! IP_VALID((ipc_port_t) memory_object[i])) {
			object[i] = VM_OBJECT_NULL;
		} else if ((object[i] =
			    vm_object_chain_enter(parent_host,
						  (ipc_port_t)memory_object[i],
						  vmv->vmv_size,
						  (ipc_port_t)shadow[i],
						  vmv->vmv_shadow_size,
						  vmv->vmv_shadow_soffset,
						  (boolean_t)
						  vmv->vmv_shadow_shadowed))
			    == VM_OBJECT_NULL) {
			result = KERN_INVALID_ARGUMENT;
			tr1("NULL object from vm_object_chain_enter");
			for (j = 0; j < i; j++)
				if (object[j] != VM_OBJECT_NULL)
					vm_object_deallocate(object[j]);
			goto cleanup;
		}
		assert(object[i] == VM_OBJECT_NULL ||
		       object[i]->ref_count > 0);
	}

	/*
	 * Secondly, under destination map locked, complete entries validation
	 * and insert them.
	 */
	vm_map_lock(map);
	if (remap) {
		*address = trunc_page(*address);
		result = vm_remap_range_allocate(map, address, size,
					 mask, anywhere, &insp_entry);
		if (result != KERN_SUCCESS) {
			tr1("vm_remap_range_allocate failure");
			vm_map_unlock(map);
			for (j = 0; j < count; j++)
				if (object[j] != VM_OBJECT_NULL)
					vm_object_deallocate(object[j]);
			goto cleanup;
		}

		/*
		 *	Now it is safe to install vm_map entries
		 *	(no error will occur).
		 */
		for (i = 0, vmv = vmv_start; i < count; i++, vmv++)
			insp_entry =
				vm_map_entry_insert(map, insp_entry,
						    vmv->vmv_start + *address,
						    vmv->vmv_end + *address,
						    object[i], vmv->vmv_offset,
						    vmv->vmv_needs_copy,
						    vmv->vmv_is_shared, FALSE,
						    vmv->vmv_protection,
						    vmv->vmv_max_protection,
						    VM_BEHAVIOR_DEFAULT,
						    vmv->vmv_inheritance, 0);

	} else {
		/*
		 *	Verify that the address is within bounds.
		 */
		if (vmv_start->vmv_start < map->min_offset ||
		    vmv_start[count-1].vmv_end > map->max_offset) {
			vm_map_unlock(map);
			result = KERN_INVALID_ADDRESS;
			tr1("address out of bounds");
			for (j = 0; j < count; j++)
				if (object[j] != VM_OBJECT_NULL)
					vm_object_deallocate(object[j]);
			goto cleanup;
		}
		for (i = 0, vmv = vmv_start; i < count; i++, vmv++) {
			/*
			 *	Verify that the starting address isn't
			 *	allocated and that the next region doesn't
			 *	overlap the end point.
			 */
			if (vm_map_lookup_entry(map, vmv->vmv_start,
						&insp_entry) ||
			    (insp_entry->vme_next != vm_map_to_entry(map) &&
			     insp_entry->vme_next->vme_start < end)) {
				vm_map_unlock(map);
				result = KERN_NO_SPACE;
				for (j = 0; j < count; j++)
					if (object[j] != VM_OBJECT_NULL)
						vm_object_deallocate(
								object[j]);
				tr1("space problems");
				goto cleanup;
			}
		}

		/*
		 *	Now it is safe to install vm_map entries
		 *	(no error will occur).
		 */
		for (i = 0, vmv = vmv_start; i < count; i++, vmv++) {
			(void)vm_map_lookup_entry(map,
						  vmv->vmv_start, &insp_entry);
			(void)vm_map_entry_insert(map, insp_entry,
						  vmv->vmv_start, vmv->vmv_end,
						  object[i], vmv->vmv_offset,
						  vmv->vmv_needs_copy,
						  vmv->vmv_is_shared, FALSE,
						  vmv->vmv_protection,
						  vmv->vmv_max_protection,
						  VM_BEHAVIOR_DEFAULT,
						  vmv->vmv_inheritance, 0);
		}
	}
	vm_map_unlock(map);


	/* this makes sure child has reference before parent releases it */
	/* XXX could add logic about handing off temporary reference? */
	for (i = 0; i < count; i++) {
		if (object[i] == VM_OBJECT_NULL) {
			continue;
		}
		vm_object_lock(object[i]);
		assert(object[i]->ref_count > 0);
		while (! object[i]->pager_ready) {
			vm_object_wait(object[i], VM_OBJECT_EVENT_PAGER_READY,
				       FALSE);
			vm_object_lock(object[i]);
		}
		vm_object_unlock(object[i]);
	}
	result = KERN_SUCCESS;

	/*
	 * Finally, clean up memory_object and shadow references.
	 */
	for (i = 0; i < count; i++) {
		if (IP_VALID((ipc_port_t) memory_object[i])) {
			ipc_port_release_send((ipc_port_t) memory_object[i]);
		}
	}
	for (i = 0; i < count2; i++) {
		if (IP_VALID((ipc_port_t) shadow[i])) {
			ipc_port_release_send((ipc_port_t) shadow[i]);
		}
	}

	kfree((vm_offset_t) memory_object,
	      (vm_size_t) (count * sizeof(memory_object_t)));
	kfree((vm_offset_t) shadow,
	      (vm_size_t) (count2 * sizeof(memory_object_t)));

 cleanup:
	kfree((vm_offset_t)object, count * sizeof(vm_object_t));
	kr = vm_deallocate(kernel_map, (vm_offset_t)vmv_start, vmv_size);
	assert(kr == KERN_SUCCESS);
 end:
	if (result == KERN_SUCCESS)
		ipc_port_release_send(parent_host);
	else
		printf("vm_map_vector_common: result=%d/0x%x\n",
		       result, result);
	tr2("exit: 0x%x", result);
	return result;
}

kern_return_t
vm_map_vector(
	host_t		host,
	vm_map_t	map,
	vm_map_vector_t	vmv_copy,
	unsigned	vmv_count,
	memory_object_t	*memory_object,
	unsigned	count,
	memory_object_t	*shadow,
	unsigned	count2,
	ipc_port_t	parent_host)
{
	return vm_map_vector_common(host, map, vmv_copy, vmv_count,
				    memory_object, count, shadow, count2,
				    FALSE, (vm_offset_t *)0, 0, 0, FALSE,
				    parent_host);
}

kern_return_t
xmm_get_shadow(
	host_t		host,
	ipc_port_t	memory_object,
	ipc_port_t	*shadow,
	vm_size_t	*shadow_size,
	vm_offset_t	*shadow_soffset,
	boolean_t	*shadow_shadowed)
{
	vm_object_t object;

	if (host == HOST_NULL ||
	    memory_object == IP_NULL ||
	    ip_kotype(memory_object) != IKOT_PAGER) {
		return KERN_INVALID_ARGUMENT;
	}

	object = (vm_object_t) memory_object->ip_kobject;
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (object->shadow == VM_OBJECT_NULL) {
		*shadow = IP_NULL;
		*shadow_size = 0;
		*shadow_soffset = 0;
		*shadow_shadowed = FALSE;
		return KERN_SUCCESS;
	}

	vm_object_lock(object->shadow);
	if (object->shadow->internal && ! object->shadow->pager_initialized) {
		vm_object_pager_create(object->shadow);
	}
	vm_object_unlock(object->shadow);

	*shadow = ipc_port_copy_send(object->shadow->pager);
	*shadow_size = object->shadow->size;
	*shadow_soffset = object->shadow_offset;
	*shadow_shadowed = (object->shadow->shadow != VM_OBJECT_NULL);
	return KERN_SUCCESS;
}

/*
 * Upon entry, source_task_port lock is held. Upon exit, it is consumed.
 */
kern_return_t
vm_remap_remote(
	ipc_port_t	target_task_port,
	vm_offset_t	*target_address,
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	ipc_port_t	source_task_port,
	vm_offset_t	source_address,
	boolean_t	copy,
	vm_prot_t	*cur_protection,
	vm_prot_t	*max_protection,
	vm_inherit_t	inheritance)
{
	return r_norma_vm_remap(source_task_port, source_address,
				target_task_port, target_address,
				size, mask, anywhere, copy, 
				cur_protection, max_protection,
				inheritance, realhost.host_priv_self);
}

kern_return_t
norma_vm_remap(
	vm_map_t		source_map,
	vm_offset_t		source_address,
	ipc_port_t		target_task_port,
	vm_offset_t		*target_address,
	vm_size_t		size,
	vm_offset_t		mask,
	boolean_t		anywhere,
	boolean_t		copy,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	vm_inherit_t		inheritance,
	ipc_port_t		target_host)
{
	struct vm_map_header	map_header;
	kern_return_t		result;
	struct vm_map_data	vmd;
	unsigned		i;
	vm_map_copy_t		vector_copy;
	vm_map_entry_t		entry;
	memory_object_t		*memory_object_buf, *shadow_buf;
	vm_size_t		buffer_size;
	
	if (target_task_port == MACH_PORT_NULL || !IP_VALID(target_task_port)) {
		if (target_host != MACH_PORT_NULL && IP_VALID(target_host))
			ipc_port_release_send(target_host);
		return KERN_INVALID_ARGUMENT;
	}

	if (target_host == MACH_PORT_NULL || !IP_VALID(target_host)) {
		ipc_port_release_send(target_task_port);
		return KERN_INVALID_ARGUMENT;
	}

	if (size != round_page(size) || source_map == VM_MAP_NULL)
		result = KERN_INVALID_ARGUMENT;
	else
		result = vm_remap_extract(source_map, source_address, size,
					  copy, &map_header, cur_protection,
					  max_protection, inheritance, TRUE);
	if (result != KERN_SUCCESS)
		goto end;

	vmd_allocate(&vmd, map_header.nentries);

	for (entry = map_header.links.next;
	     entry != (struct vm_map_entry *)&map_header.links;
	     entry = entry->vme_next) {
		assert(!entry->is_sub_map);
		vm_object_lock(entry->object.vm_object);
		task_copy_entry(entry, entry->object.vm_object, entry->offset,
				entry->needs_copy, &vmd, __LINE__);
		vm_object_unlock(entry->object.vm_object);
	}
	vm_map_entry_reset(&map_header);

	vmd_print(&vmd);

	/*
	 * allocate a KERNEL_BUFFER copy_t to point to the vector.
	 * vm_map_vector is an rpc, so we don't need to copy the data.
	 */

	vector_copy = (vm_map_copy_t)kalloc(sizeof(struct vm_map_copy));
	assert(vector_copy != VM_MAP_COPY_NULL);
	vector_copy->type = VM_MAP_COPY_KERNEL_BUFFER;
	vector_copy->size = vmd.vmd_count * sizeof(vmd.vmd_vmv[0]);
	vector_copy->offset = 0;
	vector_copy->cpy_kdata = (vm_offset_t)vmd.vmd_vmv;
	vector_copy->cpy_kalloc_size = sizeof(struct vm_map_copy);

	buffer_size = vmd.vmd_count * sizeof(memory_object_t);
	assert(sizeof(&vmd.vmd_shadow[0]) == sizeof(&vmd.vmd_memory_object[0]));

	memory_object_buf = (memory_object_t *)kalloc(buffer_size);
	shadow_buf = (memory_object_t *)kalloc(buffer_size);

	(void) bcopy((char *)vmd.vmd_memory_object,
		     (char *)memory_object_buf, buffer_size);
	(void) bcopy((char *)vmd.vmd_shadow, (char *)shadow_buf, buffer_size);

	result = r_vm_remap_vector(target_host, target_task_port,
				   target_address, size, mask, anywhere,
				   (vm_map_vector_t)vector_copy,
				   (vmd.vmd_count * sizeof(vmd.vmd_vmv[0])) /
								sizeof(long),
				   memory_object_buf, vmd.vmd_count,
				   shadow_buf, vmd.vmd_count,
				   realhost.host_priv_self);

	for (i = 0; i < vmd.vmd_count; i++) {
		vm_object_deallocate(vmd.vmd_object[i]);
	}
	vmd_free(&vmd);
 end:
	ipc_port_release_send(target_host);
	ipc_port_release_send(target_task_port);
	return result;
}

kern_return_t
vm_remap_vector(
	host_t		host,
	vm_map_t	map,
	vm_offset_t	*address,
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	vm_map_vector_t	vmv_copy,
	unsigned	vmv_count,
	memory_object_t	*memory_object,
	unsigned	count,
	memory_object_t	*shadow,
	unsigned	count2,
	ipc_port_t	parent_host)
{
	return vm_map_vector_common(host, map, vmv_copy, vmv_count,
				    memory_object, count, shadow, count2,
				    TRUE, address, size, mask, anywhere,
				    parent_host);
}

/* VARARGS1 */
void
vm_copy_dprintf(const char *fmt, ...)
{
	va_list	listp;

	if (vm_copy_debug) {
		va_start(listp, fmt);
		_doprnt(fmt, &listp, cnputc, 0);
		va_end(listp);
	}
}

int Ax;
#undef	assert
#define	assert(b)					\
MACRO_BEGIN						\
	if (!(b))					\
		dprintf("{%d:0x%x}", __LINE__, object);	\
MACRO_END

int do_vmd_print = 0;
int do_vmd_panic = 1;

void
vmd_print(
	vm_map_data_t	vmd)
{
	int i;

	if (! do_vmd_print) {
		return;
	}
	printf("\n");
/*	printf("__ ___start:_____end __off _soff c prt i t s ___pager __object __shadow line\n");*/
	printf("      start:     end   off  soff c prt i t s    pager   object   shadow line\n");
	for (i = 0; i < vmd->vmd_count; i++) {
		vm_map_vect_t vmv = &vmd->vmd_vmv[i];

		/*      start end  off soff c  prt    i  t  s pagr objt sdw l*/
		printf("%2d %8x:%8x %5x %5x %s %x/%x %s %s %s %8x %8x %8x %4d\n",
		       i,
		       vmv->vmv_start,
		       vmv->vmv_end,
		       vmv->vmv_offset,
		       vmv->vmv_shadow_soffset,
		       (vmv->vmv_needs_copy ? "C" : "S"),
		       vmv->vmv_protection,
		       vmv->vmv_max_protection,
		       (vmv->vmv_inheritance == VM_INHERIT_NONE ? "-" :
			(vmv->vmv_inheritance == VM_INHERIT_COPY ? "C" :
			 (vmv->vmv_inheritance == VM_INHERIT_SHARE ? "S" :
			  "?"))),
		       (vmv->vmv_temporary ? "T" : "P"),
		       (vmv->vmv_shadow_shadowed ? "S" : "-"),
		       vmd->vmd_memory_object[i],
		       vmd->vmd_object[i],
		       vmd->vmd_shadow[i],
		       vmv->vmv___where);
	}
	if (do_vmd_panic) panic("do_vmd_panic");
}

void
vm_copy_print_object(
	vm_object_t	object)
{
	vm_page_t m;
	int count;

	if (! vm_copy_debug) {
		return;
	}

	if (object == VM_OBJECT_NULL) {
		dprintf("0");
		return;
	}

	do {
		if (object->internal) {
			dprintf("i");
		} else {
			dprintf("x");
			assert(object->copy_strategy !=
			       MEMORY_OBJECT_COPY_DELAY);
			switch (object->copy_strategy) {
				case MEMORY_OBJECT_COPY_NONE:
				dprintf("n");
				break;
				
				case MEMORY_OBJECT_COPY_CALL:
				dprintf("c");
				break;
				
				case MEMORY_OBJECT_COPY_DELAY:
				dprintf("d");
				break;
				
				case MEMORY_OBJECT_COPY_TEMPORARY:
				dprintf("t");
				break;
				
				case MEMORY_OBJECT_COPY_SYMMETRIC:
				dprintf("s");
				break;
				
				default:
				panic("bad strategy");
			}
		}

		count = 0;
		queue_iterate(&object->memq, m, vm_page_t, listq) {
			count++;
		}
		dprintf("(%d,%df,%dr)",
			atop(object->size), atop(object->frozen_size), count);

		if (object->shadowed) {
			dprintf("S");
		}
		if (object->shadow) {
			dprintf(" -> ");
		}
	} while (object = object->shadow);
}

void
vm_copy_print_entry(
	vm_map_entry_t	entry)
{
	vm_object_t object = entry->object.vm_object;
	
	dprintf("e=<%x..%x> object=0x%x,s=%x/f=%x r=%d,inh=%d\n",
		entry->vme_start,
		entry->vme_end,
		object,
		(object ? object->size : 0),
		(object ? object->frozen_size : 0),
		(object ? object->ref_count : 0),
		entry->inheritance);

	if (vm_copy_debug_object) {
		dprintf("[");
		vm_copy_print_object(object);
		dprintf("]\n");
	}
}

void
show_vmv(vm_map_vect_t vmv)
{
	printf("vmv 0x%x\n",(unsigned int)vmv);
	printf("   start:     end   off  soff c prt i t s    line\n");
	printf("%8x:%8x %5x %5x %s %x/%x %s %s %s %4d\n",
	       vmv->vmv_start,
	       vmv->vmv_end,
	       vmv->vmv_offset,
	       vmv->vmv_shadow_soffset,
	       (vmv->vmv_needs_copy ? "C" : "S"),
	       vmv->vmv_protection,
	       vmv->vmv_max_protection,
	       (vmv->vmv_inheritance == VM_INHERIT_NONE ? "-" :
		(vmv->vmv_inheritance == VM_INHERIT_COPY ? "C" :
		 (vmv->vmv_inheritance == VM_INHERIT_SHARE ? "S" :
		  "?"))),
	       (vmv->vmv_temporary ? "T" : "P"),
	       (vmv->vmv_shadow_shadowed ? "S" : "-"),
	       vmv->vmv___where);
}
