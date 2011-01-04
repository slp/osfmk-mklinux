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
 * Revision 2.4.3.9  92/09/15  17:35:31  jeffreyh
 * 	Ignore COPY_TEMPORARY for shared objects in k_svm_set_ready
 * 	pending real implementation of this.
 * 	[92/08/14            dlb]
 * 
 * 	Changed satisfy_request routines to allow error code to be passed
 * 	through.
 * 	[92/07/17            sjs]
 * 
 * 	Extend svm_state in chunks in m_svm_extend.  Add count of kobjs
 * 	to mobj, and use to pass access through if there's only one kernel.
 * 	Rewrite m_svm_data_supply() to include initial lock logic.
 * 	[92/07/06            dlb]
 * 
 * Revision 2.4.3.8  92/06/24  18:03:24  jeffreyh
 * 	Massive changes for XMM Framework Cleanup:
 * 	  Lots of calling sequence changes.  No more xmm_kobj_link().
 * 	  Reworked ksvm and msvm objects.  ksvm is now used only for
 * 	  communication with kernels, msvm only for communication with
 * 	  managers.  ksvm object is now created explicitly by
 * 	  xmm_ksvm_create instead of implicitly by m_svm_init.
 * 	  Only this module knows about their relationship.
 * 	  Add SHOULD_TERMINATE state for use in waiting for pending
 * 	  replies.  Add missing initializations.  Reorganize termination
 * 	  logic, including discarding pending changes and requests.
 * 	  Add logic to avoid switching order of terminate and change_
 * 	  completed if terminate came first.  Don't pass user-specified
 * 	  copy strategies to kernel through this layer.
 * 	[92/06/24            dlb]
 * 
 * 	Add k_count to objects (mobj and kobj); use to synchronize terminate
 * 	against pending replies from kernels.  Add release
 * 	logic to lock_completed.
 * 	[92/06/09            dlb]
 * 
 * 	use_old_pageout --> use routine. Implement data_initialize
 * 	logic in m_svm_data_write_return, including MOBJ size check.
 * 	[92/06/04            dlb]
 * 
 * Revision 2.4.3.7  92/05/27  10:09:52  jeffreyh
 * 	Correct typos.
 * 
 * Revision 2.4.3.6  92/05/27  00:56:36  jeffreyh
 * 	Rework storage of prot and lock to cut down on memory usage.
 * 	Also add CALLED case to m_svm_init and cleaned up code.
 * 	[92/05/26            dlb]
 * 
 * 	Fix m_svm_process_pager_request to check for pending operation
 * 	counts and move them to the next request.  Also apply some cleanup
 * 	and optimization to the VM_PROT_NO_CHANGE code.
 * 	[92/05/26            dlb]
 * 
 * Revision 2.4.3.5.1.1  92/05/06  17:42:52  jeffreyh
 * 	Handle VM_PROT_NO_CHANGE in m_svm_process_pager_request.
 * 	Some of these changes are brute force and will benefit from
 * 	optimizations.
 * 	[92/04/29            dlb]
 * 
 * Revision 2.4.3.5  92/04/08  15:46:48  jeffreyh
 * 	Put markers in for new data_supply functionality.
 * 	[92/04/08  15:12:47  jeffreyh]
 * 
 * Revision 2.4.3.4  92/03/28  10:13:31  jeffreyh
 * 	Changed k_svm_data_write_return to use the passed in release
 * 	flag, rather than counting on the MOBJ being set up.  Fixes
 * 	problem of reply going to m_o_data_return rather than
 * 	m_o_data_write.
 * 	[92/03/26            sjs]
 * 	Fix k_svm_set_ready() logic to process changes after the
 * 	object is set ready if no reply has been requested.
 * 	[92/03/25            dlb]
 * 
 * 	Changed M_CHANGE_COMPLETED call to conditionally provide a
 * 	release flag - svm_set_ready() will typically not do a release
 * 	of the object.
 * 	[92/03/25            sjs]
 * 	Changed data_write to data_write_return and deleted data_return
 * 	 method.  Changed the mobj to handle multiple change requests and
 * 	 provided logic to contact all kernels and synchronize requests
 * 	 when a reply exists.  Changed delete and change_completed; all
 * 	 in support of m_o_change_attributes().
 * 	[92/03/20            sjs]
 * 
 * Revision 2.4.3.3  92/02/21  11:28:23  jsb
 * 	Replaced xmm_reply_allocate_mobj with m_svm_do_request, which now takes
 * 	a reference to mobj. m_svm_lock_completed now deallocates reply as well
 * 	as reference to mobj.
 * 	[92/02/18  17:31:27  jsb]
 * 
 * 	Cosmetic changes, including vm_page_size -> PAGE_SIZE.
 * 	[92/02/18  08:01:18  jsb]
 * 
 * 	Explicitly provide name parameter to xmm_decl macro.
 * 	Added MOBJ_STATE_TERMINATED to detect init/terminate race.
 * 	Added memory_object parameter to xmm_svm_create, and memory_object
 * 	field to struct mobj, so that m_svm_terminate can call
 * 	xmm_object_release on memory_object. Move M_TERMINATE call
 * 	to new routine xmm_svm_destroy, which is called from xmm_object
 * 	module only when there are no references to xmm object.
 * 	This fixes race between xmm_object_by_memory_object (where someone
 * 	decides to use our existing svm stack) and m_svm_terminate
 * 	(where we used to tear down the stack as soon as all the kernels
 * 	we knew about had terminated the object).
 * 	[92/02/16  15:50:31  jsb]
 * 
 * 	Changed copy strategy management to handle (naively)
 * 	MEMORY_OBJECT_COPY_TEMPORARY (by passing it up unchanged).
 * 	This will break in its current form when we enable VM_INHERIT_SHARE.
 * 	(Added appropriate checks to panic in this case.) Removed dead
 * 	routines xmm_svm_{set_access,initialize}. Changed debugging printfs.
 * 	[92/02/11  11:32:58  jsb]
 * 
 * 	Use new xmm_decl, and new memory_object_name and deallocation protocol.
 * 	Use xmm_buffer layer to buffer data writes of migrating pages.
 * 	General cleanup.
 * 	[92/02/09  13:58:24  jsb]
 * 
 * Revision 2.4.3.1  92/01/21  21:54:57  jsb
 * 	De-linted. Supports new (dlb) memory object routines.
 * 	Supports arbitrary reply ports to lock_request, etc.
 * 	Converted mach_port_t (and port_t) to ipc_port_t.
 * 	[92/01/20  17:46:55  jsb]
 * 
 * 	Fixes from OSF.
 * 	[92/01/17  14:15:53  jsb]
 * 
 * Revision 2.4.1.1  92/01/15  12:17:45  jeffreyh
 * 	Deallocate memory_object_name port when not propagating
 *	termination. (dlb)
 * 
 * Revision 2.4  91/08/03  18:19:47  jsb
 * 	Added missing type cast.
 * 	[91/07/17  14:07:46  jsb]
 * 
 * Revision 2.3  91/07/01  08:26:40  jsb
 * 	Now allow objects to grow in size (as temporary objects do).
 * 	Merged user_t and kobj structures. Do garbage collection.
 * 	Now pass up all set_attribute calls, not just first.
 * 	Use zone for request structures.
 * 	[91/06/29  15:43:16  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:43  jsb
 * 	First checkin.
 * 	[91/06/17  11:04:03  jsb]
 * 
 */
/* CMU_ENDHIST */
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
 */
/*
 *	File:	norma/xmm_svm.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer providing consistent shared virtual memory.
 */

#include <kern/misc_protos.h>
#include <xmm/xmm_svm.h>
#include <xmm/xmm_methods.h>

/*
 *	The SVM layer splits the XMM interfact into two halves.
 *	The mobj is the manager side xmm_obj; the only invocations
 *	valid on it are those generated by a manager, i.e., the k_
 *	invocations directed at the kernel.  The kobj is the kernel
 *	(there is one kobj per kernel, hence a single mobj may have
 *	multiple kobj's).  The only invocations valid on it are those
 *	generated by the kernel (i.e., the m_ invocations directed at
 *	the manager).
 *
 *	NOTE: The get_attributes invocation is absorbed by the
 *	user layer, and so should never reach here.  Both
 *	supply_completed and copy are to be implemented.  XXX XXX
 */

#define	m_msvm_init		m_invalid_init
#define	m_msvm_terminate	m_invalid_terminate
#define	m_msvm_copy		m_invalid_copy
#define	m_msvm_data_request	m_invalid_data_request
#define	m_msvm_data_unlock	m_invalid_data_unlock
#define	m_msvm_data_return	m_invalid_data_return
#define	m_msvm_lock_completed	m_invalid_lock_completed
#define	m_msvm_supply_completed	m_invalid_supply_completed
#define	m_msvm_change_completed	m_invalid_change_completed
#define	m_msvm_synchronize	m_invalid_synchronize
#define	m_msvm_freeze		m_invalid_freeze
#define	m_msvm_share		m_invalid_share
#define	m_msvm_declare_page	m_invalid_declare_page
#define	m_msvm_declare_pages	m_invalid_declare_pages
#define	m_msvm_caching		m_invalid_caching
#define	m_msvm_uncaching	m_invalid_uncaching
#define	m_msvm_debug		m_invalid_debug
#define	k_msvm_create_copy	k_invalid_create_copy
#define	k_msvm_uncaching_permitted k_invalid_uncaching_permitted

xmm_decl(msvm, "msvm", sizeof(struct mobj));

#define	m_ksvm_supply_completed	m_invalid_supply_completed
#define	k_ksvm_data_unavailable	k_invalid_data_unavailable
#define	k_ksvm_get_attributes	k_invalid_get_attributes
#define	k_ksvm_lock_request	k_invalid_lock_request
#define	k_ksvm_data_error	k_invalid_data_error
#define	k_ksvm_set_ready	k_invalid_set_ready
#define	k_ksvm_destroy		k_invalid_destroy
#define	k_ksvm_data_supply	k_invalid_data_supply
#define	k_ksvm_create_copy	k_invalid_create_copy
#define	k_ksvm_uncaching_permitted k_invalid_uncaching_permitted
#define	k_ksvm_release_all	k_invalid_release_all
#define	k_ksvm_synchronize_completed k_invalid_synchronize_completed

xmm_decl(ksvm, "ksvm", sizeof(struct kobj));

int xmm_terminate_pending = 0;

zone_t		svm_change_zone;

kern_return_t
m_ksvm_data_request(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	boolean_t wants_copy;

#ifdef	lint
	(void) M_DATA_REQUEST(kobj, offset, length, desired_access);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(page_aligned(offset));
	assert(length == PAGE_SIZE);
	if (desired_access & VM_PROT_WANTS_COPY) {
		wants_copy = TRUE;
		desired_access &= ~VM_PROT_WANTS_COPY;
	} else {
		wants_copy = FALSE;
	}
	svm_add_kernel_request(kobj, offset, TRUE, wants_copy, desired_access);
	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_data_unlock(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_prot_t	desired_access)
{
	xmm_obj_t mobj = KOBJ->mobj;

#ifdef	lint
	(void) M_DATA_UNLOCK(kobj, offset, length, desired_access);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(page_aligned(offset));
	assert(length == PAGE_SIZE);
	assert(mobj->class == &msvm_class);

	/*
	 * Forbid illegal write requests.
	 * XXX
	 * These checks also belong in data_return, once we make
	 * temporary objects write out their pages as precious, not dirty.
	 */
	xmm_obj_lock(mobj);
#if	MACH_PAGEMAP
	if ((MOBJ->pagemap != VM_EXTERNAL_NULL) &&
	    (offset < MOBJ->existence_size) &&
	    (vm_external_state_get(MOBJ->pagemap, offset) !=
	    VM_EXTERNAL_STATE_EXISTS)) {
		xmm_obj_unlock(mobj);
		panic("data_unlock on non-existent page in frozen object");
		return KERN_MEMORY_ERROR;
	}
#endif	/* MACH_PAGEMAP */
	if (MOBJ->shadow) {
		xmm_obj_unlock(mobj);
		panic("data_unlock on copy-call object\n");
		return KERN_MEMORY_ERROR;
	}
	xmm_obj_unlock(mobj);
	svm_add_kernel_request(kobj, offset, FALSE, FALSE, desired_access);
	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_data_return(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	boolean_t	dirty,
	boolean_t	kernel_copy)
{
#ifdef	lint
	(void) M_DATA_RETURN(kobj, offset, data, length, dirty, kernel_copy);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(page_aligned(offset));
	assert(length == PAGE_SIZE);

	svm_kernel_supply(kobj, offset, data, dirty, kernel_copy);
	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_change_completed(
	xmm_obj_t			kobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	xmm_reply_t			reply)
{
#ifdef	lint
	(void) M_CHANGE_COMPLETED(kobj, may_cache, copy_strategy, reply);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(reply->type == XMM_SVM_REPLY);
	svm_change_completed(kobj);
	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_lock_completed(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	xmm_reply_t	reply)
{
	unsigned count;
	xmm_obj_t mobj = KOBJ->mobj;

#ifdef	lint
	(void) M_LOCK_COMPLETED(kobj, offset, length, reply);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(mobj->class == &msvm_class);
	assert(page_aligned(offset));
	assert(reply->type == XMM_SVM_REPLY);
	
	for (count = 0; length; count++) {
		svm_kernel_completed(kobj, offset);
		length -= PAGE_SIZE;
		offset += PAGE_SIZE;
	}

	xmm_obj_lock(kobj);
	assert(KOBJ->k_count >= count);
	KOBJ->k_count -= count;
	if (KOBJ->needs_terminate && (KOBJ->k_count == 0))
		xmm_svm_terminate(kobj, FALSE); /* consumes lock */
	else
		xmm_obj_unlock(kobj);

	xmm_obj_lock(mobj);
	assert(MOBJ->k_count >= count);
	MOBJ->k_count -= count;
	while (count-- > 0) {
		xmm_obj_release_quick(mobj); /* ref got by do_lock_request */
	}
	if (MOBJ->k_count == 0 &&
	    MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE) {
		xmm_terminate_pending--;
		xmm_svm_destroy(mobj); /* consumes lock */
	} else
		xmm_obj_unlock(mobj);

	return KERN_SUCCESS;
}

kern_return_t
m_ksvm_synchronize(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_sync_t	sync_flags)
{
#ifdef	lint
	(void) M_SYNCHRONIZE(kobj, offset, length, sync_flags);
#endif	/* lint */
	assert(kobj->class == &ksvm_class);
	assert(page_aligned(offset));

	svm_synchronize(kobj, offset, length, sync_flags);
	return KERN_SUCCESS;
}

kern_return_t
k_msvm_data_supply(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_size_t	length,
	vm_prot_t	lock,
	boolean_t	precious,
	xmm_reply_t	reply)
{
#ifdef	lint
	(void) K_DATA_SUPPLY(mobj, offset, data, length, lock, precious,
			     reply);
#endif	/* lint */
	assert(mobj->class == &msvm_class);
	assert(page_aligned(offset));			/* XXX */
	assert(length == PAGE_SIZE);			/* XXX */
	assert(! (lock & ~VM_PROT_ALL));

	svm_pager_supply(mobj, offset, data, lock, precious, reply,
			 KERN_SUCCESS);
	return KERN_SUCCESS;
}

kern_return_t
k_msvm_synchronize_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
	assert(mobj->class == &msvm_class);

	svm_synchronize_completed(mobj, offset, length);
	return KERN_SUCCESS;
}

kern_return_t
k_msvm_data_unavailable(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
#ifdef	lint
	(void) K_DATA_UNAVAILABLE(mobj, offset, length);
#endif	/* lint */
	assert(mobj->class == &msvm_class);
	assert(page_aligned(offset));		/* XXX */
	assert(length == PAGE_SIZE);		/* XXX */

	svm_pager_supply(mobj, offset, DATA_UNAVAILABLE, VM_PROT_NONE, FALSE,
			 XMM_REPLY_NULL, KERN_SUCCESS);
	return KERN_SUCCESS;
}

int c_lock_request_terminate;	/* XXX curiosity counter. Should be removed */

kern_return_t
k_msvm_lock_request(
	xmm_obj_t		mobj,
	vm_offset_t		offset,
	vm_size_t		length,
	memory_object_return_t	should_return,
	boolean_t		should_flush,
	vm_prot_t		lock_value,
	xmm_reply_t		reply)
{
#ifdef	lint
	(void) K_LOCK_REQUEST(mobj, offset, length, should_return,
			      should_flush, lock_value, reply);
#endif	/* lint */
	assert(mobj->class == &msvm_class);
	assert(page_aligned(offset));

	/*
	 *	Always take a reference to prevent the XMM stack
	 *	from being deallocated.
	 */
	xmm_obj_lock(mobj);
	if (MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE ||
	    MOBJ->state == MOBJ_STATE_TERMINATED) {
		xmm_obj_unlock(mobj);
		return (KERN_INVALID_ARGUMENT);
	}
	MOBJ->k_count++;
	if (length & LOCK_REQ_START)
		MOBJ->lock_batching++;
	xmm_obj_unlock(mobj);
	
	svm_pager_request(mobj, offset, should_return, should_flush,
			  lock_value, reply);

	/*
	 *	Release all of the coalesced lock_requests if the
	 *	LOCK_REQ_STOP flag has been seen; once seen, it
	 *	is safe to release the reference on the mobj.
	 */
	xmm_obj_lock(mobj);
	if (length & LOCK_REQ_STOP)
		MOBJ->lock_batching--;
	if (MOBJ->lock_batching == 0)
		svm_release_lock_request(mobj);

	/*
	 *	By taking a reference, we may have deferred termination.
	 *	Check it here just in case (and prevent a nasty leak).
	 */
	if (--MOBJ->k_count == 0 &&
	    MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE) {
		xmm_terminate_pending--;
		xmm_svm_destroy(mobj); /* consumes lock */
		c_lock_request_terminate++;	/* XXX curiosity counter */
	} else
		xmm_obj_unlock(mobj);

	return KERN_SUCCESS;
}

kern_return_t
k_msvm_data_error(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	kern_return_t	error_value)
{
#ifdef	lint
	(void) K_DATA_ERROR(mobj, offset, length, error_value);
#endif	/* lint */
	assert(mobj->class == &msvm_class);
	assert(page_aligned(offset));		/* XXX */
	assert(length == PAGE_SIZE);		/* XXX */

	svm_pager_supply(mobj, offset, DATA_ERROR, VM_PROT_NO_CHANGE,
			 FALSE, XMM_REPLY_NULL, error_value);
	return KERN_SUCCESS;
}

kern_return_t
k_msvm_destroy(
	xmm_obj_t	mobj,
	kern_return_t	reason)
{
	xmm_obj_t kobj;
	kern_return_t kr;

#ifdef	lint
	(void) K_DESTROY(mobj, reason);
#endif	/* lint */
	assert(mobj->class == &msvm_class);

	xmm_obj_lock(mobj);
	if (MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE ||
	    MOBJ->state == MOBJ_STATE_TERMINATED) {
		xmm_obj_unlock(mobj);
		return (KERN_INVALID_ARGUMENT);
	}
	MOBJ->destroyed = TRUE;
	svm_klist_first(mobj, &kobj);
	xmm_obj_unlock(mobj);
	while (kobj) {
		xmm_obj_unlock(kobj);
		/* XXX check initialized? */
		kr = K_DESTROY(kobj, reason);
		assert(kr == KERN_SUCCESS);
		xmm_obj_lock(mobj);
		xmm_obj_lock(kobj);
		svm_klist_next(mobj, &kobj, FALSE);
	}
	return KERN_SUCCESS;
}

kern_return_t
k_msvm_release_all(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) K_RELEASE_ALL(mobj);
#endif	/* lint */
	assert(mobj->class == &msvm_class);
	assert(MOBJ->state == MOBJ_STATE_TERMINATED ||
	       MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE);
	xmm_obj_unlink(mobj);
	return (KERN_SUCCESS);
}

void
xmm_svm_init(void)
{
	svm_request_init();
	svm_change_init();
}

#include <mach_kdb.h>
#if	MACH_KDB
#include <ddb/db_output.h>
#include <vm/vm_print.h>

#define	printf	kdbprintf

/*
 * Forward.
 */
void		m_svm_print(
			xmm_obj_t	mobj);

void		k_svm_print(
			xmm_obj_t	kobj);

/*
 *	Routine:	m_svm_print
 *	Purpose:
 *		Pretty-print an svm mobj.
 */

void
m_svm_print(
	xmm_obj_t	mobj)
{
	printf("svm mobj 0x%x\n", mobj);

	indent += 2;

	iprintf("shadow=0x%x", MOBJ->shadow);
	if (MOBJ->shadow) {
		register i = 0;
		xmm_obj_t shadow = mobj;
		while (shadow = SHADOW->shadow)
			i++;
		printf("(%d)", i);
	}

	printf(", copy=0x%x", MOBJ->copy);
	if (MOBJ->copy) {
		register i = 0;
		xmm_obj_t copy = mobj;
		while (copy = COPY->copy)
			i++;
		printf("(%d)", i);
	}
	printf("\n");

	iprintf("kobj_list=0x%x(%d), ", MOBJ->kobj_list, MOBJ->kobj_count);
	printf("num_pages=0x%x, ", MOBJ->num_pages);

	switch (MOBJ->state) {

	case MOBJ_STATE_UNCALLED:
		printf("uncalled\n");
		break;

	case MOBJ_STATE_CALLED:
		printf("called\n");
		break;

	case MOBJ_STATE_READY:
		printf("ready\n");
		break;

	case MOBJ_STATE_SHOULD_TERMINATE:
		printf("should_terminate\n");
		break;

	case MOBJ_STATE_TERMINATED:
		printf("terminated\n");
		break;

	default:
		printf("state=%d?\n", MOBJ->state);
	}

	iprintf("request_list{next=0x%x,prev=0x%x} ",
		MOBJ->request_list.next,
		MOBJ->request_list.prev);
	printf("count=0x%x last_found=0x%x\n",
		MOBJ->request_count,
		MOBJ->last_found);

	svm_state_print(&MOBJ->bits);
	iprintf("terminate_mobj=0x%x, %smay_cache",
		MOBJ->terminate_mobj,
		BOOL(MOBJ->may_cache));

	switch (MOBJ->copy_strategy) {

	case MEMORY_OBJECT_COPY_NONE:
		printf(", copy_none\n");
		break;

	case MEMORY_OBJECT_COPY_CALL:
		printf(", copy_call\n");
		break;

	case MEMORY_OBJECT_COPY_DELAY:
		printf(", copy_delay\n");
		break;

	case MEMORY_OBJECT_COPY_SYMMETRIC:
		printf(", copy_symmetric\n");
		break;

	default:
		printf(", copy_strategy=%d?\n", MOBJ->copy_strategy);
	}

	iprintf("memory_object=0x%x\n",
		MOBJ->memory_object);

	iprintf("%stemporary, %stemp_disabled, change=0x%x, k_count=%d, ",
		BOOL(MOBJ->temporary),
		BOOL(MOBJ->temporary_disabled),
		MOBJ->change,
		MOBJ->k_count);
	printf("lock_batching=%d\n", MOBJ->lock_batching);

	iprintf("%sdirty_copy, %sdestroyed, ",
		BOOL(MOBJ->dirty_copy),
		BOOL(MOBJ->destroyed));
	printf("%scopy_in_progress, %scopy_wanted, %sterm_sent\n",
	       BOOL(MOBJ->copy_in_progress),
	       BOOL(MOBJ->copy_wanted),
	       BOOL(MOBJ->term_sent));

	iprintf("%sextend_in_progress, %sextend_wanted, ",
		BOOL(MOBJ->extend_in_progress),
		BOOL(MOBJ->extend_wanted));
	printf("%sdisable_in_progress, %sdestroy_needed\n",
	       BOOL(MOBJ->disable_in_progress),
		BOOL(MOBJ->destroy_needed));

#if	MACH_PAGEMAP
	printf(", existence_size=0x%x\n", MOBJ->existence_size);
	iprintf("pagemap=");
	(void) vm_external_print(MOBJ->pagemap, MOBJ->existence_size);
#endif	/* MACH_PAGEMAP */
	printf("\n");

	indent -=2;
}

/*
 *	Routine:	k_svm_print
 *	Purpose:
 *		Pretty-print an svm kobj.
 */

void
k_svm_print(
	xmm_obj_t	kobj)
{
	printf("svm kobj 0x%x\n", kobj);

	indent += 2;

	svm_state_print(&KOBJ->bits);
	iprintf("mobj=0x%x, next=0x%x\n", KOBJ->mobj, KOBJ->next);

	iprintf("%sinitialized", BOOL(KOBJ->initialized));
	printf(", %sactive", BOOL(KOBJ->active));
#if	MACH_PAGEMAP
	printf(", %shas_pagemap", BOOL(KOBJ->has_pagemap));
#endif	/* MACH_PAGEMAP */
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	printf(", %slocal", BOOL(KOBJ->local));
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	printf(", %sreadied, %sterminated\n", 
	       BOOL(KOBJ->readied),
	       BOOL(KOBJ->terminated));
	iprintf("k_count=0x%x %sneeds_terminate\n", 
		KOBJ->k_count,
		BOOL(KOBJ->needs_terminate));
	g_svm_print(KOBJ->lock_gather);

	indent -=2;
}

void
m_ksvm_debug(
	xmm_obj_t	kobj,
	int		flags)
{
	xmm_obj_t mobj;

	xmm_obj_print((db_addr_t)kobj);
	indent += 4;
	k_svm_print(kobj);
	mobj = KOBJ->mobj;
	indent -= 4;
	xmm_obj_print((db_addr_t)mobj);
	indent += 4;
	iprintf("%stemporary, request_count=%d, kobj_count=%d\n",
		BOOL(MOBJ->temporary),
		MOBJ->request_count,
		MOBJ->kobj_count);
	iprintf("k_count=%d lock_batching=%d\n",
		MOBJ->k_count,
		MOBJ->lock_batching);
	indent -= 4;
	M_DEBUG(mobj, flags);
}

#endif	/* MACH_KDB */
