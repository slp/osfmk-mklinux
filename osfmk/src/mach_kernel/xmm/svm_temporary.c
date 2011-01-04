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
 *	Routines for implementing distributed temporary objects.
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

#include <mach_assert.h>
#include <kern/misc_protos.h>
#include <kern/xpr.h>
#include <xmm/xmm_svm.h>
#include <xmm/xmm_methods.h>

xmm_decl_prototypes(ksvm)
xmm_decl_prototypes(msvm)

/*
 * Forward.
 */
void		svm_disable_active_temporary(
			xmm_obj_t	mobj);

void		svm_mobj_cleanup(
			xmm_obj_t	mobj);

void
xmm_svm_create(
	xmm_obj_t	old_mobj,
	ipc_port_t	memory_object,
	xmm_obj_t	*new_mobj)
{
	xmm_obj_t mobj;

	xmm_split_create(old_mobj, &old_mobj);
	xmm_obj_allocate(&msvm_class, old_mobj, &mobj);
	MOBJ->kobj_list = XMM_OBJ_NULL;
	MOBJ->kobj_count = 0;
	MOBJ->shadow = XMM_OBJ_NULL;
	MOBJ->copy = XMM_OBJ_NULL;
	MOBJ->terminate_mobj = XMM_OBJ_NULL;
	MOBJ->state = MOBJ_STATE_UNCALLED;
	MOBJ->num_pages = 0;
	MOBJ->request_count = 0;
	queue_init(&MOBJ->request_list);
	MOBJ->last_found = REQUEST_NULL;
	MOBJ->change = CHANGE_NULL;
	svm_state_init(&MOBJ->bits);
	MOBJ->memory_object = memory_object; /* sright shared with xmm_user */
	MOBJ->may_cache = FALSE;
	MOBJ->copy_strategy = MEMORY_OBJECT_COPY_NONE;
	MOBJ->temporary = FALSE;
	MOBJ->temporary_disabled = FALSE;
	MOBJ->destroyed = FALSE;
	MOBJ->copy_in_progress = FALSE;
	MOBJ->copy_wanted = FALSE;
	MOBJ->ready_wanted = FALSE;
	MOBJ->disable_in_progress = FALSE;
	MOBJ->term_sent = FALSE;
	MOBJ->k_count = 0;
	MOBJ->destroy_needed = FALSE;
	MOBJ->extend_in_progress = FALSE;
	MOBJ->extend_wanted = FALSE;
#if	MACH_PAGEMAP
	MOBJ->pagemap = VM_EXTERNAL_NULL;
	MOBJ->existence_size = 0;
#endif	/* MACH_PAGEMAP */
	MOBJ->dirty_copy = FALSE;
	queue_init(&MOBJ->sync_requests);
	xmm_list_lock_init(MOBJ);
	*new_mobj = mobj;
}

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
void
xmm_ksvm_create(
	xmm_obj_t	mobj,
	xmm_obj_t	*new_kobj,
	boolean_t	local)
#else	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
void
xmm_ksvm_create(
	xmm_obj_t	mobj,
	xmm_obj_t	*new_kobj)
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
{
	xmm_obj_t kobj, k;

	assert(mobj->class == &msvm_class);

	xmm_obj_allocate(&ksvm_class, XMM_OBJ_NULL, &kobj);

	svm_state_init(&KOBJ->bits);

	KOBJ->k_count = 0;
	KOBJ->initialized = FALSE;
	KOBJ->active = FALSE;
	KOBJ->readied = FALSE;
	KOBJ->terminated = FALSE;
	KOBJ->needs_terminate = FALSE;
#if	MACH_PAGEMAP
	KOBJ->has_pagemap = FALSE;
#endif	/* MACH_PAGEMAP */
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	KOBJ->local = local;
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	KOBJ->mobj = mobj;
	KOBJ->lock_gather = GATHER_NULL;

	/* XXX check destroyed? */

	xmm_obj_lock(mobj);
	svm_extend_allocate(mobj, kobj);

	/*
	 * If the object is temporary, queue all kobjs after the first
	 * one.  This will take advantage of the fact that temporary
	 * objects are scattered in the mesh and the current set
	 * of algorithms will make more requests to the first kobj
	 * in the list, giving us reduced messaging and lower
	 * latency.
	 */
	if (MOBJ->temporary && MOBJ->kobj_list) {
		k = MOBJ->kobj_list;
		KOBJ->next = K->next;
		K->next = kobj;
	} else {
		KOBJ->next = MOBJ->kobj_list;
		MOBJ->kobj_list = kobj;
	}
	MOBJ->kobj_count++;
	xmm_obj_unlock(mobj);

	*new_kobj = kobj;
}

/*
 * Called in place of K_SET_READY, so that:
 *
 *	return value is checked (asserted)
 *
 *	temporary flags are disabled if necessary
 *
 *	existence info is provided when valid,
 *	and at most once to each kernel
 *
 * MP note : Upon return, kobj and mobj locks are consumed.
 */
void
svm_do_set_ready(
	xmm_obj_t	kobj,
	xmm_reply_t	reply)
{
	register xmm_obj_t mobj = KOBJ->mobj;
	kern_return_t kr;
	vm_external_map_t existence_map = VM_EXTERNAL_NULL;
	vm_offset_t existence_size = 0;

	assert(xmm_obj_lock_held(mobj));
	assert(xmm_obj_lock_held(kobj));

#if	MACH_PAGEMAP
	/*
	 * Don't send a pagemap if there isn't one or if the kernel already
	 * has a copy.
	 */
	if ((MOBJ->pagemap == VM_EXTERNAL_NULL) || KOBJ->has_pagemap) {
		existence_map = VM_EXTERNAL_NULL;
		existence_size = 0;
	} else {
		existence_map = MOBJ->pagemap;
		existence_size = MOBJ->existence_size;
		KOBJ->has_pagemap = TRUE;
	}
#endif	/* MACH_PAGEMAP */

	/*
	 * Mark the kernel active the first time it is sent K_SET_READY.
	 */
	if (! KOBJ->readied) {
		KOBJ->readied = TRUE;
		KOBJ->active = TRUE;
	}
	xmm_obj_unlock(kobj);

	XPR(XPR_XMM,
		"svm_do_set_ready: kobj %X mobj %X strat %d emap %X esize %x\n",
		(integer_t)kobj, (integer_t)mobj, MOBJ->copy_strategy,
		(integer_t)existence_map, existence_size);
	if (MOBJ->temporary_disabled || MOBJ->disable_in_progress) {
		assert(MOBJ->temporary);
		xmm_obj_unlock(mobj);
		kr = K_SET_READY(kobj, (boolean_t) MOBJ->may_cache,
				 (memory_object_copy_strategy_t)
				 MOBJ->copy_strategy, MOBJ->cluster_size,
				 reply, FALSE, existence_map, existence_size);
	} else {
		assert(! MOBJ->temporary || svm_get_active_count(mobj));
		assert(! (MOBJ->may_cache && MOBJ->temporary));
		xmm_obj_unlock(mobj);
		kr = K_SET_READY(kobj, (boolean_t) MOBJ->may_cache,
				 (memory_object_copy_strategy_t)
				 MOBJ->copy_strategy, MOBJ->cluster_size, 
				 reply, (boolean_t) MOBJ->temporary,
				 existence_map, existence_size);
	}
	assert(kr == KERN_SUCCESS);
}

void
svm_disable_active_temporary(
	xmm_obj_t	mobj)
{
	xmm_obj_t kobj;

	assert(mobj->class == &msvm_class);
	assert(xmm_obj_lock_held(mobj));
	assert(svm_get_active_count(mobj) == 1);
	kobj = svm_get_active(mobj);
	assert(kobj);
	assert(xmm_obj_lock_held(kobj));
	assert(MOBJ->temporary);
	assert(KOBJ->active);
	assert(! MOBJ->temporary_disabled);
	assert(MOBJ->state != MOBJ_STATE_SHOULD_TERMINATE &&
	       MOBJ->state != MOBJ_STATE_TERMINATED);

	/*
	 * Only generate one disable temporary request.
	 */
	if (! MOBJ->disable_in_progress) {
		MOBJ->disable_in_progress = TRUE;

		/*
		 * increment KOBJ->k_count so that this kobj doesn't
		 * terminate while we're waiting for this change
		 * request to be processed.  This kobj will be stored
		 * in the chosen_kobj field in the change request; this
		 * k_count reference will be donated to svm_do_set_ready
		 * when it sees that chosen_kobj is set.
		 */
		KOBJ->k_count++;
		xmm_obj_unlock(kobj);
		xmm_obj_unlock(mobj);

		svm_queue_temporary_change(kobj);

		xmm_obj_lock(mobj);
		assert(MOBJ->state != MOBJ_STATE_SHOULD_TERMINATE &&
		       MOBJ->state != MOBJ_STATE_TERMINATED);
	} else {
		xmm_obj_unlock(kobj);
	}

	/*
	 * Wait for the reply.
	 */
	while (MOBJ->disable_in_progress) {
		assert_wait(svm_disable_event(mobj), FALSE);
		xmm_obj_unlock(mobj);
		thread_block((void (*)(void)) 0);
		xmm_obj_lock(mobj);
	}
}

kern_return_t
m_ksvm_init(
	xmm_obj_t	kobj,
	vm_size_t	pagesize,
	boolean_t	internal,
	vm_size_t	size)
{
	xmm_obj_t mobj;
	kern_return_t kr;

	assert(kobj->class == &ksvm_class);
#ifdef	lint
	(void) M_INIT(kobj, pagesize, internal, size);
#endif	/* lint */
	assert(pagesize == PAGE_SIZE);
	xmm_obj_lock(kobj);
	assert(KOBJ->initialized == FALSE);
	KOBJ->initialized = TRUE;
	xmm_obj_unlock(kobj);
	mobj = KOBJ->mobj;
	xmm_obj_lock(mobj);

	/*
	 * A copy_call created object (which will have a non-null shadow)
	 * should be created with internal TRUE, so that a new default
	 * pager object is created to back it.
	 */
	if (MOBJ->shadow) {
		internal = TRUE;
	}

	/*
	 * Initialize this kernel, or ask for an initialization
	 * from the memory object.
	 */
	switch (MOBJ->state) {

	case MOBJ_STATE_UNCALLED:

		/*
		 *	Haven't sent the init operation to
		 *	the pager yet.  Mark the object and do it.
		 */
		MOBJ->state = MOBJ_STATE_CALLED;
		xmm_obj_unlock(mobj);
		kr = M_INIT(mobj, PAGE_SIZE, internal, size);
		assert(kr == KERN_SUCCESS);
		break;

	case MOBJ_STATE_CALLED:

		/*
		 *	Pager has already been called; set_ready
		 *	will come back and init this kernel in response.
		 *	Nothing else to do here.
		 */
		xmm_obj_unlock(mobj);
		break;

	case MOBJ_STATE_READY:

		/*
		 *	This object is already ready; tell the new kernel.
		 *	First, however, we must disable temporary termination
		 *	if there is already an active kernel.
		 */
		if (MOBJ->temporary && ! MOBJ->temporary_disabled &&
		    svm_get_active_count(mobj) > 0)
			do {
				svm_disable_active_temporary(mobj);
				/*
				 *	The temporary termination behaviour
				 *	must be enabled back if the last
				 *	active kobj has been terminated.
				 */
				if (svm_get_active_count(mobj) == 0) {
					MOBJ->temporary_disabled = FALSE;
					break;
				}
			} while (! MOBJ->temporary_disabled);

		xmm_obj_lock(kobj);
		svm_do_set_ready(kobj, XMM_REPLY_NULL);
		break;

#if	MACH_ASSERT
	case MOBJ_STATE_SHOULD_TERMINATE:
	case MOBJ_STATE_TERMINATED:
	default:
		/*
		 *	Can't happen.  Something is really wrong.
		 */
		xmm_obj_unlock(mobj);
		panic("m_ksvm_init: illegal MOBJ state=%d", MOBJ->state);
		break;
#endif	/* MACH_ASSERT */
	}

	return KERN_SUCCESS;
}

/*
 * Called when we need to write out pages to an object before
 * it has been initialized by a kernel.
 */
void
svm_mobj_initialize(
	xmm_obj_t	mobj,
	boolean_t	internal,
	vm_size_t	size)
{
	kern_return_t kr;

	assert(xmm_obj_lock_held(mobj));
	assert(MOBJ->state == MOBJ_STATE_UNCALLED ||
	       MOBJ->state == MOBJ_STATE_CALLED);
	if (MOBJ->state == MOBJ_STATE_UNCALLED) {
		MOBJ->state = MOBJ_STATE_CALLED;
		xmm_obj_unlock(mobj);
		kr = M_INIT(mobj, PAGE_SIZE, internal, size);
		assert(kr == KERN_SUCCESS);
		xmm_obj_lock(mobj);
	}
	while (MOBJ->state == MOBJ_STATE_CALLED) {
		MOBJ->ready_wanted = TRUE;
		assert_wait(svm_ready_event(mobj), FALSE);
		xmm_obj_unlock(mobj);
		thread_block((void (*)(void)) 0);
		xmm_obj_lock(mobj);
	}
}

/*
 * MP note: Upon return, the kobj lock has been released.
 */
kern_return_t
xmm_svm_terminate(
	xmm_obj_t	kobj,
	boolean_t	release)
{
	xmm_obj_t mobj;
	xmm_obj_t kobj_terminated, *kp;

	assert(xmm_obj_lock_held(kobj));
	KOBJ->terminated = TRUE;
	/*
	 * If there are outstanding requests, do not actually tear
	 * things down yet.  When the completions get back, they
	 * will check for KOBJ->terminated and k_count==0 and do
	 * the teardown then.
	 */
	if (KOBJ->k_count != 0) {
		KOBJ->needs_terminate = TRUE;
		xmm_obj_unlock(kobj);
		return KERN_SUCCESS;
	}

	KOBJ->needs_terminate = FALSE;

	/*
	 * Take a reference on the kobj structure in order to prevent it
	 *	to be freed during K_RELEASE_ALL call.
	 */
	xmm_obj_reference(kobj);
	xmm_obj_unlock(kobj);

	K_RELEASE_ALL(kobj);

	/*
	 * Remove kobj from list and free its resources.
	 */
	mobj = KOBJ->mobj;
	xmm_obj_lock(mobj);
	kobj_terminated = kobj;
	for (kp = &MOBJ->kobj_list; kobj = *kp; kp = &KOBJ->next) {
		if (kobj == kobj_terminated) {
			*kp = KOBJ->next;
			MOBJ->kobj_count--;
			break;
		}
	}
	kobj = kobj_terminated;
	xmm_obj_lock(kobj);
	KOBJ->mobj = XMM_OBJ_NULL;

	/*
	 * We have lost an active kernel reference, and thus we should
	 * reconsider our temporary termination policy and existence.
	 */
	if (KOBJ->active && MOBJ->temporary) {
		xmm_obj_unlock(kobj);
		svm_reconsider_temporary(mobj); /* consumes lock */
		xmm_obj_lock(kobj);
	} else
		xmm_obj_unlock(mobj);

	/*
	 * Now, it is safe to have the kobj structure released.
	 */
	xmm_obj_release(kobj); /* consumes lock */

	/*
	 * Release one reference to xmm object. If there are no
	 * more references, then svm_destroy will be called.
	 */
	xmm_object_release(MOBJ->memory_object);

	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_terminate(
	xmm_obj_t	kobj,
	boolean_t	release)
{
#ifdef	lint
	(void) M_TERMINATE(kobj, release);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);

	xmm_obj_lock(kobj);
	return xmm_svm_terminate(kobj, release); /* consumes lock */
}

/*
 * MP note: Upon return, the mobj lock has been released.
 */
void
xmm_svm_destroy(
	xmm_obj_t	mobj)
{
	kern_return_t kr;
	xmm_obj_t copy, shadow;
	boolean_t term_sent;
	unsigned page;
	char *m_bits;

	assert(xmm_obj_lock_held(mobj));

	/*
	 * If an mobj cannot terminate because it still has a copy,
	 * then fill in the terminate_mobj field and return.  This
	 * is a loop to follow chains where this condition has been
	 * detected.
	 */
	do {
		assert(mobj->class == &msvm_class);

		/*
		 * If waiting for a kernel reply, don't do anything.
		 * The k_count logic should get us back in here when
		 * MOBJ->k_count goes to zero.
		 */
		if (MOBJ->k_count > 0) {
			assert(MOBJ->state != MOBJ_STATE_TERMINATED);
			MOBJ->state = MOBJ_STATE_SHOULD_TERMINATE;
			xmm_obj_unlock(mobj);
			xmm_terminate_pending++;
			return;
		}

		/*
		 * Our copy contains a reference to us,
		 * and thus we cannot be destroyed before it is.
		 */
		copy = svm_get_stable_copy(mobj);
		if (copy != XMM_OBJ_NULL) {
			xmm_obj_lock(copy);
			assert(COPY->terminate_mobj == XMM_OBJ_NULL);
			COPY->terminate_mobj = mobj;
			xmm_obj_unlock(copy);
			assert(MOBJ->state != MOBJ_STATE_TERMINATED);
			MOBJ->state = MOBJ_STATE_SHOULD_TERMINATE;
			xmm_obj_unlock(mobj);
			xmm_terminate_pending++;
			return;
		}
		copy = MOBJ->terminate_mobj;
		MOBJ->terminate_mobj = XMM_OBJ_NULL;

		MOBJ->state = MOBJ_STATE_TERMINATED;
		if (!(term_sent = MOBJ->term_sent))
			MOBJ->term_sent = TRUE;

		/*
		 * We must release our reference to our shadow, if any.
		 * Before cleaning the copy field of the shadow, it is
		 * mandatory to test if the current stable copy of the shadow
		 * is still mobj. In that case, we *must* clear it, as well as
		 * its NEEDS_COPY state.
		 *
		 * MP notes: XMM lock ordering policy implies that the mobj
		 *	must be taken first before taking its copy mobj lock.
		 *	Therefore, the mobj lock must be released before
		 *	taking its shadow mobj lock.
		 */
		for (;;) {
			shadow = MOBJ->shadow;
			xmm_obj_unlock(mobj);
			if (shadow == XMM_OBJ_NULL)
				break;
			xmm_obj_lock(shadow);
			if (svm_get_stable_copy(shadow) == mobj) {
				SHADOW->copy = XMM_OBJ_NULL;
				svm_extend_clear_copy(SHADOW->bits.bitmap,
						      SHADOW->bits.level);
				xmm_obj_release(shadow); /* consumes lock */
				break;
			}
			xmm_obj_unlock(shadow);
			xmm_obj_lock(mobj);
		}

		if (!term_sent) {
			kr = M_TERMINATE(mobj, FALSE);	/* m_o_terminate */
			assert(kr == KERN_SUCCESS);
		}
		kr = M_TERMINATE(mobj, TRUE);		/* release */
		assert(kr == KERN_SUCCESS || kr == KERN_FAILURE);

		if (copy) {
			mobj = copy;
			xmm_terminate_pending--;
			xmm_obj_lock(mobj);
			assert(MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE);
		} else
			mobj = XMM_OBJ_NULL;
	} while (mobj);
}

/*
 * MP note : Upon return, the kobj lock has to be released to follow the
 *	M_DEALLOCATE locking strategy (locked upon call, unlocked upon return).
 */
void
m_ksvm_deallocate(
	xmm_obj_t	kobj)
{
	assert(xmm_obj_lock_held(kobj));

	/*
	 * Free kobj's resources.
	 */
	assert(kobj->class == &ksvm_class);
	xmm_obj_unlock(kobj);
	svm_state_exit(&KOBJ->bits);
}

/*
 * MP note : Upon return, the mobj lock has to be released to follow the
 *	M_DEALLOCATE locking strategy (locked upon call, unlocked upon return).
 */
void
m_msvm_deallocate(
	xmm_obj_t	mobj)
{
	assert(xmm_obj_lock_held(mobj));
	xmm_obj_unlock(mobj);

	/*
	 * Free mobj's resources.
	 */
	assert(mobj->class == &msvm_class);
	svm_state_exit(&MOBJ->bits);
#if	MACH_PAGEMAP
	if (MOBJ->pagemap != VM_EXTERNAL_NULL) {
		vm_external_destroy(MOBJ->pagemap, MOBJ->existence_size);
	}
#endif	/* MACH_PAGEMAP */
	svm_mobj_cleanup(mobj);
}

/*
 * Called when we lose an active kernel reference or in-transit reference
 * to mobj. May result in either reenabling of temporary termination
 * (should it be disabled), or destruction of the object.
 *
 * MP note : Upon return, the mobj lock is consumed.
 */
void
svm_reconsider_temporary(
	xmm_obj_t	mobj)
{
	int active_count;
	xmm_obj_t kobj;

	assert(xmm_obj_lock_held(mobj));
	assert(MOBJ->temporary);
	assert(MOBJ->state == MOBJ_STATE_READY);
	
	/*
	 * Likewise, if there is more than one active kernel, then we cannot
	 * reenable temporary termination (since one kernel could discard
	 * pages as it terminated, stranding the other kernel), and we
	 * certainly should not destroy the object.
	 */
	active_count = svm_get_active_count(mobj);
	if (active_count > 1) {
		xmm_obj_unlock(mobj);
		return;
	}

	/*
	 * If there are no active kernels left (and no in-transit references),
	 * then destroy the object.
	 */
	if (active_count == 0) {
		for (kobj = MOBJ->kobj_list; kobj; kobj = KOBJ->next) {
			/*
			 * A kobj is going to be added. Don't destroy mobj, and
			 * let k_msvm_ready() deal with temporary termination.
			 */
			if (!KOBJ->readied) {
				xmm_obj_unlock(mobj);
				return;
			}
		}

		xmm_obj_unlock(mobj);
		(void) k_msvm_destroy(mobj, KERN_SUCCESS);
		return;
	}

	/*
	 * There is one one active kernel left, and no in-transit references.
	 * If temporary termination has been disabled, we reenable it.
	 * The kernel is then free to discard dirty pages when terminating
	 * the object.
	 */
	if (MOBJ->temporary_disabled) {
		xmm_obj_t kobj;

		/*
		 * Find remaining active kernel.
		 */
		kobj = svm_get_active(mobj);
		assert(kobj);
		assert(xmm_obj_lock_held(kobj));
		
		/*
		 * Mark temporary as reenabled, and send a change_request
		 * to the kernel to let it know that temporary is reenabled.
		 */
       		MOBJ->temporary_disabled = FALSE;
		svm_do_set_ready(kobj, XMM_REPLY_NULL); /* consumes locks */
	} else
		xmm_obj_unlock(mobj);
}

#if	MACH_PAGEMAP
/*
 * MP note: Upon return, the mobj lock is consumed.
 */
void
svm_distribute_pagemap(
	xmm_obj_t	mobj)
{
	xmm_obj_t kobj;

	XPR(XPR_XMM, "svm_dist_pagemap: mobj %X\n",
		(integer_t)mobj, 0, 0, 0, 0);
	assert(xmm_obj_lock_held(mobj));
	assert(MOBJ->pagemap != VM_EXTERNAL_NULL);
	svm_klist_first(mobj, &kobj);
	while (kobj) {
		if (KOBJ->initialized && ! KOBJ->has_pagemap && KOBJ->active) {
			svm_do_set_ready(kobj, XMM_REPLY_NULL);
			assert(KOBJ->has_pagemap);
			xmm_obj_lock(mobj);
			xmm_obj_lock(kobj);
		}
		svm_klist_next(mobj, &kobj, TRUE);
	}
	xmm_obj_unlock(mobj);
}
#endif	/* MACH_PAGEMAP */

/*
 * XXX
 * This pagemap could be constructed from svm state
 * if we kept track of paged-out pages.
 */
kern_return_t
m_ksvm_freeze(
	xmm_obj_t	kobj,
	vm_external_map_t existence_map,
	vm_offset_t	existence_size)
{
#if	MACH_PAGEMAP
	xmm_obj_t mobj = KOBJ->mobj;
	vm_external_map_t emap;

#ifdef	lint
	(void) M_FREEZE(kobj, existence_map, existence_size);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);

	/*
	 * Save a copy of the pagemap.
	 */
	if (existence_map != VM_EXTERNAL_NULL) {
		assert(existence_size != 0);
		emap = vm_external_create(existence_size);
	} else
		emap = VM_EXTERNAL_NULL;
	xmm_obj_lock(mobj);
	assert(MOBJ->pagemap == VM_EXTERNAL_NULL);
	MOBJ->pagemap = emap;
	MOBJ->existence_size = existence_size;
	if (existence_map != VM_EXTERNAL_NULL)
		vm_external_copy(existence_map, existence_size, MOBJ->pagemap);

	XPR(XPR_XMM, "m_ksvm_freeze: kobj %X mobj %X new emap %X esize %X\n",
		(integer_t)kobj, (integer_t)mobj,
		(integer_t)MOBJ->pagemap, existence_size, 0);

	/*
	 * Record that this kernel has a pagemap, and distribute
	 * the pagemap to any other kernels that have mapped this object.
	 */
	if (MOBJ->pagemap != VM_EXTERNAL_NULL) {
		assert(! KOBJ->has_pagemap);
		KOBJ->has_pagemap = TRUE;
		xmm_obj_unlock(kobj);
		svm_distribute_pagemap(mobj); /* consumes lock */
	} else {
		MOBJ->existence_size = 0;
		xmm_obj_unlock(mobj);
	}

	return KERN_SUCCESS;
#else	/* MACH_PAGEMAP */
	return KERN_FAILURE;
#endif	/* MACH_PAGEMAP */
}

kern_return_t
m_ksvm_share(
	xmm_obj_t	kobj)
{
	xmm_obj_t mobj = KOBJ->mobj;

#ifdef	lint
	(void) M_SHARE(kobj);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	xmm_obj_lock(mobj);
	assert(MOBJ->temporary);

	/*
	 * This object is about to be shared.
	 * Change copy strategy accordingly.
	 */
	assert(MOBJ->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC);
#if	MACH_PAGEMAP
	assert(MOBJ->pagemap == VM_EXTERNAL_NULL);
#endif	/* MACH_PAGEMAP */
	MOBJ->copy_strategy = MEMORY_OBJECT_COPY_CALL;
	xmm_obj_unlock(mobj);
	return KERN_SUCCESS;
}

/*
 * The real work performed on a page by svm_declare_page.
 *
 * We started using this page without asking the default pager
 * for it, so we don't know whether it would have marked the
 * page as precious. We must assume that it would have marked
 * it precious and thus do so ourselves now, since otherwise
 * we may violate the invariant that every kernel that has
 * a copy of a precious page has a precious copy.
 *
 * It is not necessary for the kernel to mark its pages dirty,
 * since they will all already be dirty. EXCEPT... that means
 * that we cannot declare pages that can disappear... which
 * conflicts with assumptions in vm_object_declare_pages.
 * XXX NEEDS MORE THOUGHT XXX
 *
 * XXX
 * <<< There is a race between declare_pages and data_request. >>>
 *
 * XXXO
 * It may be possible to avoid this precious setting logic
 * if we never do a data_request as long as we believe that
 * there are kernels with copies and thus as long as there
 * are kernels with non-precious copies.
 */

#define	DECLARE_PAGE(m_bits, k_bits)					\
	MACRO_BEGIN							\
	M_ASN_LOCK(m_bits, VM_PROT_NONE);				\
	M_SET_DIRTY(m_bits);						\
	K_ASN_PROT(k_bits, VM_PROT_ALL);				\
	K_SET_PRECIOUS(k_bits);						\
	MACRO_END

#if	Frozen_Pages_Are_Readonly
#define	DECLARE_FROZEN_PAGE(m_bits, k_bits)				\
	MACRO_BEGIN							\
	M_ASN_LOCK(m_bits, VM_PROT_WRITE);				\
	M_SET_DIRTY(m_bits);						\
	K_ASN_PROT(k_bits, VM_PROT_READ|VM_PROT_EXECUTE);		\
	K_SET_PRECIOUS(k_bits);						\
	MACRO_END
#endif	/* Frozen_Pages_Are_Readonly */

/*
 * Declare a single page. See vm_object_declare_pages for details.
 */
kern_return_t
m_ksvm_declare_page(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	size)
{
	xmm_obj_t mobj = KOBJ->mobj, writer;
	register unsigned int page = atop(offset);
	char *k_bits;
	char *m_bits;

#ifdef	lint
	(void) M_DECLARE_PAGE(kobj, offset, size);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(page_aligned(offset));
	assert(size == PAGE_SIZE);

	xmm_obj_lock(mobj);
	svm_extend_if_needed(mobj, page);
	svm_check_dirty_invariant(mobj, page, 0, TRUE);
	m_get_writer(mobj, page, &writer);

	XPR(XPR_XMM,
		"m_ksvm_decl_page: kobj %X off %X writer %X\n",
		(integer_t)kobj, offset, (integer_t)writer, 0, 0);

	if (writer != XMM_OBJ_NULL && writer != kobj) {
		xmm_obj_unlock(writer);
		printf("m_ksvm_declare_page: lock conflict mobj 0x%x kobj 0x%x offset 0x%x\n", mobj, kobj, offset);
	} else { 
		if (writer == XMM_OBJ_NULL)
			xmm_obj_lock(kobj);
		M_MAP_GET(MOBJ, page, m_bits);
		K_MAP_GET(KOBJ, page, k_bits);
		DECLARE_PAGE(m_bits, k_bits);
		xmm_obj_unlock(kobj);
	}
	svm_check_dirty_invariant(mobj, page, 0, TRUE);
#if 0
	M_MAP_GET(MOBJ, page, m_bits);
	assert(M_GET_LOCK(m_bits) == VM_PROT_ALL);
	assert(m_get_prot(mobj, page) == VM_PROT_NONE);

	xmm_obj_lock(kobj);
	K_MAP_GET(KOBJ, page, k_bits);
	DECLARE_PAGE(m_bits, k_bits);
#endif
	xmm_obj_unlock(mobj);
	return KERN_SUCCESS;
}

/*
 * Declare many pages, just like multiple calls to svm_declare_page.
 * Additionally, if frozen is true, then save existence info.
 */
int	sjsdecl = 0;
kern_return_t
m_ksvm_declare_pages(
	xmm_obj_t	kobj,
	vm_external_map_t existence_map,
	vm_offset_t	existence_size,
	boolean_t	frozen)
{
#if	MACH_PAGEMAP
	xmm_obj_t mobj = KOBJ->mobj, writer;
	vm_size_t offset;
	char *k_bits;
	char *m_bits;

#ifdef	lint
	(void) M_DECLARE_PAGES(kobj, existence_map, existence_size, frozen);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);

	XPR(XPR_XMM,
		"m_ksvm_decl_pages: kobj %X emap %X esize %X frozen %d\n",
		(integer_t)kobj, (integer_t)existence_map, existence_size,
		frozen, 0);
	/*
	 * Update ownership and protection information for pages
	 * that the kernel started using before the pager was created.
	 */
	xmm_obj_lock(mobj);
	for (offset = 0; offset < existence_size; offset += PAGE_SIZE) {
		if ((offset < existence_size) &&
		    (vm_external_state_get(existence_map, offset) ==
		    VM_EXTERNAL_STATE_EXISTS)) {
			register unsigned int page = atop(offset);

			svm_extend_if_needed(mobj, page);
			if (m_get_writer(mobj,page,&writer) != VM_PROT_NONE &&
			   writer != kobj) {
				printf("declare_pages: cannot 0x%x\n", offset);
				assert(sjsdecl);
				xmm_obj_unlock(writer);
			} else {
				M_MAP_GET(MOBJ, page, m_bits);
				/*
				 * If m_get_writer didn't return a
				 * writer we need to lock the kobj
				 */
				if (writer == XMM_OBJ_NULL)
					xmm_obj_lock(kobj);
				K_MAP_GET(KOBJ, page, k_bits);
				DECLARE_PAGE(m_bits, k_bits);
				xmm_obj_unlock(kobj);
			}
		}
	}

	/*
	 * Save a copy of the pagemap if we can.
	 *
	 * Don't distribute the pagemap because there are
	 * no other kernels to distribute it to.
	 */
	if (frozen) {
		vm_external_map_t emap = VM_EXTERNAL_NULL;

		assert(! KOBJ->has_pagemap);
		assert(MOBJ->pagemap == VM_EXTERNAL_NULL);
		if (existence_size != 0) {
			xmm_obj_unlock(mobj);
			emap = vm_external_create(existence_size);
			xmm_obj_lock(mobj);
		}
		MOBJ->pagemap = emap;
		MOBJ->existence_size = existence_size;
		if (MOBJ->pagemap != VM_EXTERNAL_NULL) {
			vm_external_copy(existence_map, existence_size,
								MOBJ->pagemap);
			xmm_obj_lock(kobj);
			KOBJ->has_pagemap = TRUE;
			xmm_obj_unlock(kobj);
#ifdef notdef
			if (MOBJ->kobj_count > 1) {
				svm_distribute_pagemap(mobj); /*consumes lock*/
			} else
				xmm_obj_unlock(mobj);
			assert(MOBJ->kobj_count == 1);
#else
			xmm_obj_unlock(mobj);
#endif
		} else {
			/*
			 * We get here if vm_external_create returns
			 * NULL, indicating we are out of memory.
			 */
			MOBJ->existence_size = 0;
			xmm_obj_unlock(mobj);
		}
	} else
		xmm_obj_unlock(mobj);

	return KERN_SUCCESS;
#else	/* MACH_PAGEMAP */
	return KERN_FAILURE;
#endif	/* MACH_PAGEMAP */
}

kern_return_t
m_ksvm_caching(
	xmm_obj_t	kobj)
{
	xmm_obj_t mobj = KOBJ->mobj;

#ifdef	lint
	(void) M_CACHING(kobj);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	xmm_obj_lock(mobj);
	xmm_obj_lock(kobj);
	assert(MOBJ->temporary);
	assert(KOBJ->active);

	if (! MOBJ->temporary_disabled) {
		/*
		 * If temporary termination is not disabled, then
		 * this must be the only active kernel. Furthermore,
		 * we must have told it that the object was no longer
		 * cachable; it must just not have received that
		 * message at the time that it sent this one.
		 *
		 * When it receives that message, it will either still
		 * have the object cached, or it will have sent an
		 * uncache request.
		 *
		 * If it has it cached, then after the
		 * call to memory_object_set_attributes_common, it will
		 * deallocate the object, which will trigger a termination.
		 *
		 * If it has sent an uncache request, then after the call
		 * to memory_object_set_attributes_common, there will still
		 * be a reference to the object, held by the thread waiting
		 * for the reply to the uncache. Thus in k_server_set_ready,
		 * we check to see if there is an outstanding uncache
		 * request, and if there is, we destroy the object, since
		 * we know that at one point the object had no active
		 * references, and it's not worth complicating the protocol
		 * to save the object in this case.
		 *
		 * Thus in either case, the kernel will take care of
		 * destroying the object for us, and thus we don't
		 * have to send any message. We do go ahead and mark
		 * the object as destroyed, which prevents any attempts
		 * to uncache the object.
		 */
#if	MACH_ASSERT
		xmm_obj_unlock(kobj);
		assert(svm_get_active_count(mobj) == 1);
		assert(kobj == svm_get_active(mobj));
		assert(xmm_obj_lock_held(kobj));
#endif	/* MACH_ASSERT */
		KOBJ->active = FALSE;
		xmm_obj_unlock(kobj);
		MOBJ->destroyed = TRUE;
		xmm_obj_unlock(mobj);
		return KERN_SUCCESS;
	}

	KOBJ->active = FALSE;
	xmm_obj_unlock(kobj);

	/*
	 * Reenable temporary if it is safe to do so.
	 */
	svm_reconsider_temporary(mobj); /* consumes lock */
	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_uncaching(
	xmm_obj_t	kobj)
{
	xmm_obj_t mobj = KOBJ->mobj;
	kern_return_t kr;

#ifdef	lint
	(void) M_UNCACHING(kobj);
#endif	/* lint */
	xmm_obj_lock(mobj);
	assert(MOBJ->temporary);
	assert(! KOBJ->active);

	/*
	 * If this object has not already been destroyed,
	 * and temporary termination is not yet disabled,
	 * then attempt to disable temporary termination.
	 *
	 * Note that there must be at least once active
	 * kernel, since otherwise the object would have
	 * been destroyed. Furthermore, there can be no
	 * more than one active kernel, since otherwise
	 * temporary termination would already be disabled.
	 */
	while (! MOBJ->destroyed && ! MOBJ->temporary_disabled) {
		assert(svm_get_active_count(mobj) == 1);
		svm_disable_active_temporary(mobj);
	}

	/*
	 * If this object has been destroyed, then
	 * the kernel will receive the destroy message
	 * as response to the uncache request. Otherwise,
	 * we mark the kernel active and send it an
	 * uncaching_permitted message.
	 */
	if (! MOBJ->destroyed) {
		xmm_obj_unlock(mobj);
		xmm_obj_lock(kobj);
		KOBJ->active = TRUE;
		xmm_obj_unlock(kobj);
		kr = K_UNCACHING_PERMITTED(kobj);
		assert(kr == KERN_SUCCESS);
	} else
		xmm_obj_unlock(mobj);

	return KERN_SUCCESS;
}

/*
 * Free all changes and requests attached to this mobj.
 */
void
svm_mobj_cleanup(
	xmm_obj_t	mobj)
{
	assert(MOBJ->kobj_list == XMM_OBJ_NULL);
	assert(MOBJ->kobj_count == 0);

	svm_change_cleanup(mobj);
	svm_request_cleanup(mobj);
}
