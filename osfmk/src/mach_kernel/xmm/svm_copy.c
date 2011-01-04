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
 *	Joseph S. Barrera III 1992
 *	Copy-on-write for distributed memory.
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <kern/misc_protos.h>
#include <xmm/xmm_svm.h>
#include <xmm/xmm_methods.h>

xmm_decl_prototypes(ksvm)

/*
 * Forward.
 */
void		svm_create_new_copy(
			xmm_obj_t	mobj);

#if	MACH_PAGEMAP
void		svm_create_pagemap_for_copy(
			xmm_obj_t	old_copy,
			xmm_obj_t	shadow);
#endif	/* MACH_PAGEMAP */


#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
int svm_copy_call_push_page_to_kernel = COPY_CALL_PUSH_PAGE_TO_KERNEL;
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */

kern_return_t
m_ksvm_copy(
	xmm_obj_t	kobj)
{
	kern_return_t kr;
	xmm_obj_t mobj = KOBJ->mobj;
	xmm_obj_t old_copy;
	kern_return_t result;
	ipc_port_t port;

#ifdef	lint
	(void) M_COPY(kobj);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);

	/*
	 * If we can use a preexisting copy object, do so.
	 * Otherwise, create a new copy object.
	 */
	xmm_obj_lock(mobj);
	while (old_copy = svm_get_stable_copy(mobj)) {
		xmm_obj_lock(old_copy);
		if (OLD_COPY->dirty_copy ||
		    OLD_COPY->state == MOBJ_STATE_TERMINATED ||
		    OLD_COPY->state == MOBJ_STATE_SHOULD_TERMINATE) {
			xmm_obj_unlock(old_copy);
			break;
		}
		xmm_obj_unlock(mobj);
		xmm_obj_unlock(old_copy);

		/*
		 * Reference the local XMM old_copy stack to prevent any call
		 * to xmm_obj_destroy during the copy in order to avoid a
		 * deadlock between a terminating memory_object and the
		 * creation of an XMM stack based on the same memory_object.
		 */
		kr = xmm_object_reference(OLD_COPY->memory_object);
		if (kr == KERN_ABORTED) {
			xmm_obj_lock(mobj);
			break;
		}
		assert(kr == KERN_SUCCESS);

		port = ipc_port_copy_send(OLD_COPY->memory_object);
		assert(IP_VALID(port));
		kr = K_CREATE_COPY(kobj, port, &result);
		assert(kr == KERN_SUCCESS);

		/*
		 * Release the reference of the XMM stack.
		 */
		xmm_object_release(OLD_COPY->memory_object);

		if (result == KERN_SUCCESS || result == KERN_ABORTED) {
#if	MACH_ASSERT
			xmm_obj_lock(mobj);
			old_copy = svm_get_stable_copy(mobj);
			xmm_obj_unlock(mobj);
#endif	/* MACH_ASSERT */
			assert(!OLD_COPY->dirty_copy &&
			       OLD_COPY->state != MOBJ_STATE_TERMINATED &&
			       OLD_COPY->state != MOBJ_STATE_SHOULD_TERMINATE);
			return KERN_SUCCESS;
		}

		assert(result == KERN_INVALID_ARGUMENT);
		xmm_obj_lock(mobj);
	}

	svm_create_new_copy(mobj);
	xmm_obj_unlock(mobj);
	return KERN_SUCCESS;
}

#if	MACH_PAGEMAP
/*
 * MP note : Upon return, the old_copy lock is consumed.
 */
void
svm_create_pagemap_for_copy(
	xmm_obj_t	old_copy,
	xmm_obj_t	shadow)
{
	assert(xmm_obj_lock_held(old_copy));
	assert(OLD_COPY->pagemap == VM_EXTERNAL_NULL);

	/*
	 * Create a pagemap and enter all pages which have
	 * been pushed by this object's shadow.
	 */
	xmm_obj_unlock(old_copy);
	OLD_COPY->existence_size = ptoa(SHADOW->num_pages);
	OLD_COPY->pagemap = vm_external_create(OLD_COPY->existence_size);

	/*
	 * The M_SVM layer lock ordering strategy states that it is safe
	 * to take a MOBJ lock, and then the MOBJ->copy lock. So, the
	 * shadow lock must be taken before the old_copy.
	 */
	xmm_obj_lock(shadow);
	xmm_obj_lock(old_copy);
	if (OLD_COPY->pagemap == VM_EXTERNAL_NULL) {
		OLD_COPY->existence_size = 0;
		xmm_obj_unlock(old_copy);
		xmm_obj_unlock(shadow);
		return;
	}
	svm_extend_update_pagemap(SHADOW->bits.bitmap, SHADOW->bits.level,
				  SHADOW->bits.range, old_copy, 0);
	/*
	 * Should we do this as part of the K_CREATE_COPY called by
	 * svm_create_new_copy?
	 */
	xmm_obj_unlock(shadow);
	svm_distribute_pagemap(old_copy); /* consumes lock */
}
#endif	/* MACH_PAGEMAP */

/*
 * If a copy is currently being created, wait for it.
 */
xmm_obj_t
svm_get_stable_copy(
	xmm_obj_t	mobj)
{
	assert(xmm_obj_lock_held(mobj));
	while (MOBJ->copy_in_progress) {
		MOBJ->copy_wanted = TRUE;
		assert_wait(svm_copy_event(mobj), FALSE);
		xmm_obj_unlock(mobj);
		thread_block((void (*)(void)) 0);
		xmm_obj_lock(mobj);
	}
	return MOBJ->copy;
}

void
svm_create_new_copy(
	xmm_obj_t	mobj)
{
	xmm_obj_t old_copy, new_copy, kobj;
	kern_return_t kr;
	ipc_port_t old_copy_pager;
	ipc_port_t new_copy_pager;
	int k_copy;
	kern_return_t result;

	assert(xmm_obj_lock_held(mobj));

	/*
	 * Prevent others from examining copy until we are done.
	 */
	assert(! MOBJ->copy_in_progress);
	MOBJ->copy_in_progress = TRUE;
	xmm_obj_unlock(mobj);

	new_copy_pager = ipc_port_alloc_kernel();
	if (new_copy_pager == IP_NULL) {
		panic("svm_create_new_copy: ipc_port_alloc_kernel");
	}

	/* we hold a naked receive right for new_copy_pager */
	(void) ipc_port_make_send(new_copy_pager);
	/* now we also hold a naked send right for new_copy_pager */

	xmm_user_create(new_copy_pager, &new_copy);
	xmm_svm_create(new_copy, new_copy_pager, &new_copy);

	kr = xmm_memory_manager_export(new_copy, new_copy_pager);
	if (kr != KERN_SUCCESS) {
		panic("m_ksvm_copy: xmm_memory_manager_export: %x\n", kr);
	}

	/* xmm_user should now have a send right; we will share it */
	ipc_port_release_send(new_copy_pager);
	assert((new_copy_pager)->ip_srights > 0);

	/*
	 * Copy-call objects are temporary and noncachable.
	 */
	NEW_COPY->temporary = TRUE;
	NEW_COPY->may_cache = FALSE;

	/*
	 * Link old copy with new.
	 *
	 * XXX
	 * Grabbing references creates an object collapsing problem.
	 * Need to add vm_object_collapse-like code for when objs
	 * are only referenced by their copies.
	 */
	xmm_obj_lock(mobj);
	if ((old_copy = MOBJ->copy) != XMM_OBJ_NULL) {
		xmm_obj_lock(old_copy);
		xmm_obj_unlock(mobj);
		/*
		 * Remember old_copy pager, which must be valid
		 */
		old_copy_pager = OLD_COPY->memory_object;
		assert(IP_VALID(old_copy_pager));

		/*
		 * New copy steals reference to mobj from old copy.
		 * Old copy creates a reference to new copy.
		 */
		OLD_COPY->shadow = new_copy;
		NEW_COPY->copy = old_copy;
		xmm_obj_reference(new_copy);

		/*
		 * old_copy must not be destoyed before the new copy is stable.
		 */
		OLD_COPY->k_count++;

#if	MACH_PAGEMAP
		/*
		 * We freeze the old copy, now that it is isolated
		 * from the permanent object by the new copy and
		 * therefore will receive no more page pushes.
		 * Note that the old copy might already be frozen
		 * if it was the first copy of this object (see below).
		 */
		if (OLD_COPY->pagemap == VM_EXTERNAL_NULL) {
			svm_create_pagemap_for_copy(old_copy, mobj);
		} else
			xmm_obj_unlock(old_copy);
#else	/* MACH_PAGEMAP */
		xmm_obj_unlock(old_copy);
#endif	/* MACH_PAGEMAP */

		/*
		 * Create immediately attributes array for the new copy.
		 */
		NEW_COPY->bits.range = MOBJ->bits.range;
		NEW_COPY->num_pages = MOBJ->num_pages;
		svm_extend_new_copy(MOBJ->bits.bitmap,
				    MOBJ->bits.level, new_copy, 0);
	} else {
		/*
		 * Remember old_copy pager, which is null
		 */
		old_copy_pager = IP_NULL;

		/*
		 * New copy must create its own reference to mobj.
		 */
		NEW_COPY->copy = XMM_OBJ_NULL;
		xmm_obj_reference(mobj);
		xmm_obj_unlock(mobj);

#if	MACH_PAGEMAP
		/*
		 * We freeze the first copy of a copy-call object
		 * as soon as that copy is created (with no pages
		 * in it), to take advantage of the fact that many
		 * permanent objects never push any pages into
		 * their copies: executable files, for example.
		 */
		assert(NEW_COPY->pagemap == VM_EXTERNAL_NULL);
		NEW_COPY->existence_size = ptoa(MOBJ->num_pages);
		NEW_COPY->pagemap =
			vm_external_create(NEW_COPY->existence_size);
#endif	/* MACH_PAGEMAP */
	}

	/*
	 * Link mobj with its new copy.
	 * Appropriate references have been taken above.
	 */
	NEW_COPY->shadow = mobj;
	xmm_obj_lock(mobj);
	MOBJ->copy = new_copy;

	/*
	 * Set each page needs_copy so that we know to push a copy
	 * of the page if that page gets written.
	 *
	 * One might think that we should remove write protection
	 * on all pages here, since we know for a fact that the
	 * kernel has done so. However, this is not correct.
	 * We need to remember that the pages are possibly
	 * dirty; however, we cannot set them dirty, since
	 * they might not be. If we set a page dirty, then
	 * it must be dirty. If we leave a page written, then
	 * we know that we have to ask the kernel for the page,
	 * since it might be dirty, but we can tolerate it if
	 * the page is in fact not dirty.
	 *
	 * In other words, we have chosen to have write access
	 * be a hint that the page is dirty, but to have the
	 * dirty flag be truth, not a hint.
	 *
	 * This would seem to introduce a new problem, namely
	 * a writer making a write request. However, this isn't
	 * a new problem; it's just one we haven't seen or
	 * thought of before.
	 *
	 * XXX
	 * Pushing zero-fill pages seems like a total waste.
	 *
	 * XXX
	 * We used to set (obsoleted) readonly flag here -- why?
	 *
	 * Note that this object may previously have had a copy
	 * object and thus we may be resetting some set bits here.
	 * The fact that we always set all bits when we attach
	 * a new copy object is what makes it correct for
	 * M_GET_NEEDS_COPY to ignore needs_copy bits when there
	 * is no copy object. That is, we don't have to make the
	 * bits consistent when there is no copy object because
	 * we will make them consistent here when we attach a
	 * new copy object.
	 */
	svm_extend_set_copy(MOBJ->bits.bitmap, MOBJ->bits.level);

	/*
	 * Tell all kernels about new copy object, while
	 * asking them to protect their pages in the copied
	 * object.
	 *
	 * XXX
	 * We should mark "copy in progress" for any other kernels
	 * that come in here with an m_ksvm_copy, so that we can
	 * return the same copy.
	 *
	 * The memory_object_create_copy call is a synchronous rpc.
	 * This ensures that the pages are protected by the time we
	 * exit the loop. We could have an explicit return message
	 * to reduce latency.
	 *
	 * XXX
	 * We could reduce the number of calls in some cases
	 * by calling only those kernels which either have some
	 * write access or which have a shadow chain for this object.
	 */
	svm_klist_first(mobj, &kobj);
	xmm_obj_unlock(mobj);

	/*
	 * Reference the XMM memory_object port in order to avoid calling
	 * xmm_obj_destroy during copy creation,
	 */
	kr = xmm_object_reference(new_copy_pager);
	assert(kr == KERN_SUCCESS);

	while (kobj) {
		xmm_obj_unlock(kobj);
		ipc_port_copy_send(new_copy_pager);
		kr = K_CREATE_COPY(kobj, new_copy_pager, &result);
		assert(kr == KERN_SUCCESS);
		xmm_obj_lock(mobj);
		xmm_obj_lock(kobj);
		svm_klist_next(mobj, &kobj, FALSE);
	}

	/*
	 * Release the XMM memory_object port, destroying the new object
	 * if needed.
	 */
	xmm_obj_lock(new_copy);
	if (NEW_COPY->state == MOBJ_STATE_UNCALLED)
		svm_mobj_initialize(new_copy, TRUE, 0);
	xmm_obj_unlock(new_copy);
	xmm_object_release(new_copy_pager);

	/*
	 * Wake up anyone waiting for this copy to complete.
	 */
	xmm_obj_lock(mobj);
	MOBJ->copy_in_progress = FALSE;
	if (MOBJ->copy_wanted) {
		MOBJ->copy_wanted = FALSE;
		thread_wakeup(svm_copy_event(mobj));
	}

	/*
	 * Release the old_copy extra reference if needed.
	 */
	if (old_copy != XMM_OBJ_NULL) {
		xmm_obj_lock(old_copy);
		if (--OLD_COPY->k_count == 0 &&
		    OLD_COPY->state == MOBJ_STATE_SHOULD_TERMINATE) {
			xmm_obj_unlock(mobj);
			xmm_terminate_pending--;
			xmm_svm_destroy(old_copy); /* consumes lock */
			xmm_obj_lock(mobj);
		} else
			xmm_obj_unlock(old_copy);
	}
}

/*
 * Push data from a copy-call strategy object into its copy.
 */
void
svm_copy_push_page(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_map_copy_t	data)
{
	unsigned int page = atop(offset);
	xmm_obj_t copy;
	kern_return_t kr;
	char *m_bits;

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	xmm_obj_t kobj;
	char *k_bits;
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */

	/*
	 * We have a real page that much be copied into the copy object
	 * before the kernel can be permitted to modify it.
	 * We will need to return the page to the copy object's pager
	 * and mark the page as no longer needing to be copied.
	 *
	 * Make sure we have a copy mobj. If the current copy mobj is frozen,
	 * create and use a new one. In either case, mark the copy mobj dirty.
	 */
	xmm_obj_lock(mobj);
	for (;;) {
		copy = svm_get_stable_copy(mobj);

		/*
		 * A MP race condition may occur between the copy detection
		 * made in svm_satisfy_kernel_request() and now. If the copy
		 * has been destroyed, then release data and return.
		 */
		if (copy == XMM_OBJ_NULL) {
			M_MAP_GET(MOBJ, page, m_bits);
			assert(!M_GET_NEEDS_COPY(m_bits));
			xmm_obj_unlock(mobj);
			vm_map_copy_discard(data);
			return;
		}

		xmm_obj_lock(copy);
		if (COPY->state == MOBJ_STATE_TERMINATED) {
			xmm_obj_unlock(copy);
			xmm_obj_unlock(mobj);
			vm_map_copy_discard(data);
			return;
		}
		
#if	MACH_PAGEMAP
		if (COPY->pagemap != VM_EXTERNAL_NULL) {
			xmm_obj_unlock(copy);
			svm_create_new_copy(mobj);
			continue;
		}
#endif	/* MACH_PAGEMAP */
		break;
	}
	M_MAP_GET(MOBJ, page, m_bits);
	M_CLR_NEEDS_COPY(m_bits);
	xmm_obj_unlock(mobj);

	COPY->dirty_copy = TRUE;
	COPY->k_count++;

	/*
	 * Initialize copy if we need to.
	 */
	if (COPY->state == MOBJ_STATE_UNCALLED ||
	    COPY->state == MOBJ_STATE_CALLED) {
		svm_mobj_initialize(copy, TRUE, 0);
	}
	assert(COPY->state == MOBJ_STATE_READY ||
	       COPY->state == MOBJ_STATE_SHOULD_TERMINATE);

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	/*
	 * If there is a local kobj associated to the copy object to which
	 * the page must be returned, then the page is supplied to the local
	 * kernel and marked precious.
	 */
	for (kobj = COPY->kobj_list; kobj; kobj = KOBJ->next)
		if (KOBJ->local)
			break;
	if (kobj != XMM_OBJ_NULL && svm_copy_call_push_page_to_kernel) {
		/* consumes locks */
		svm_copy_supply(kobj, offset, data);
	} else
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */

	/*
	 * Write page into pager backing copy object. The page is dirty, and
	 * the copy object itself has not kept a copy of the data. Mark the
	 * page as no longer needing to be copied.
	 */
	{
		xmm_obj_unlock(copy);
		svm_prepare_copy(data);

		kr = M_DATA_RETURN(copy, offset, data,
				   PAGE_SIZE, TRUE, FALSE);
		assert(kr == KERN_SUCCESS);
	}

	xmm_obj_lock(copy);
	if (--COPY->k_count == 0 &&
	    COPY->state == MOBJ_STATE_SHOULD_TERMINATE) {
		xmm_terminate_pending--;
		xmm_svm_destroy(copy); /* consumes lock */
	} else
		xmm_obj_unlock(copy);

	/*
	 * We don't have to flush page from any kernel.
	 * If we are ready to grant write permission
	 * to this kernel, then no other kernel can
	 * have this page mapped. Thus we only have to
	 * worry about the requesting kernel having
	 * this page mapped for the copy. However,
	 * vm_fault_page will handle this case for us:
	 * granting write permission to a page in
	 * an object with a copy_call strategy will
	 * cause the page to be unmapped from all
	 * address spaces before write permission
	 * is enabled.
	 */
}
