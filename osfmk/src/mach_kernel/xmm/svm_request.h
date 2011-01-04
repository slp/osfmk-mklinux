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
 *	Definitions for reconciling kernel and pager requests.
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

#include <mach_kdb.h>
#include <mach_assert.h>
#include <mach/vm_sync.h>
#include <kern/macro_help.h>

typedef struct request		*request_t;
typedef	struct gather		*gather_t;
#define	REQUEST_NULL		((request_t) 0)
#define	GATHER_NULL		((gather_t) 0)

#define	DATA_NONE		((vm_map_copy_t) 0)
#define	DATA_UNAVAILABLE	((vm_map_copy_t) 1)
#define	DATA_ERROR		((vm_map_copy_t) 2)

/*
 * KOBJ list protection. 3 macros now manage accesses to the list:
 *	svm_klist_first(), svm_klist_next[_req]() and svm_klist_abort_req().
 */

/*
 * svm_klist_first(m, k)
 *	xmm_obj_t m;
 *	xmm_obj_t *k;
 *
 * It initializes the first non-terminating element (k). Its k_count
 *	is incremented to prevent it to be released from the list.
 */
#define	svm_klist_first(m, k)					\
MACRO_BEGIN							\
	*(k) = ((struct mobj *)(m))->kobj_list;			\
	while (*(k) != XMM_OBJ_NULL) {				\
		xmm_obj_lock(*(k));				\
		if (!((*(struct kobj **)(k))->terminated ||	\
		    (*(struct kobj **)(k))->needs_terminate)) {	\
			(*(struct kobj **)(k))->k_count++;	\
			break;					\
		}						\
		xmm_obj_unlock(*(k));				\
		*(k) = (*(struct kobj **)(k))->next;		\
	}							\
MACRO_END

/*
 * svm_klist_next(m, k, t)	   svm_klist_next_req(m, k, r)
 *	xmm_obj_t m;			xmm_obj_t m;
 *	xmm_obj_t *k;			xmm_obj_t *k;
 *	boolean_t t;			request_t r;
 *
 * They initialize the next non-terminating element (k) from the current
 *	one (k) in the k->next list. The k_count of the next element is
 *	incremented to insure that it will still exist after termination
 *	work on the current element has been achieved (it may be destroyed
 *	if needed). If t, then return with m locked, otherwise, m must
 *	be unlocked.
 */
#define	svm_klist_next(m, k, t)					\
MACRO_BEGIN							\
	xmm_obj_t _n = (*(struct kobj **)(k))->next;		\
	while (_n != XMM_OBJ_NULL) {				\
		xmm_obj_lock(_n);				\
		if (!(((struct kobj *)_n)->terminated ||	\
		    ((struct kobj *)_n)->needs_terminate)) {	\
			((struct kobj *)_n)->k_count++;		\
			xmm_obj_unlock(_n);			\
			break;					\
		}						\
		xmm_obj_unlock(_n);				\
		_n = ((struct kobj *)_n)->next;			\
	}							\
								\
	if (--(*(struct kobj **)(k))->k_count == 0 &&		\
	    (*(struct kobj **)(k))->terminated) {		\
		xmm_obj_unlock(m);				\
		xmm_svm_terminate(*(k), TRUE);			\
		if (t)						\
			xmm_obj_lock(m);			\
	} else {						\
		xmm_obj_unlock(*(k));				\
		if (!t)						\
			xmm_obj_unlock(m);			\
	}							\
								\
	if (_n != XMM_OBJ_NULL)					\
		xmm_obj_lock(_n);				\
	*(k) = _n;						\
MACRO_END

#define	svm_klist_next_req(m, k, r)				\
MACRO_BEGIN							\
	xmm_obj_t _n = (*(struct kobj **)(k))->next;		\
	while (_n != XMM_OBJ_NULL) {				\
		xmm_obj_lock(_n);				\
		if (!(((struct kobj *)_n)->terminated ||	\
		    ((struct kobj *)_n)->needs_terminate)) {	\
			((struct kobj *)_n)->k_count++;		\
			xmm_obj_unlock(_n);			\
			break;					\
		}						\
		xmm_obj_unlock(_n);				\
		_n = ((struct kobj *)_n)->next;			\
	}							\
								\
	if (--(*(struct kobj **)(k))->k_count == 0 &&		\
	    (*(struct kobj **)(k))->terminated) {		\
		xmm_obj_unlock(m);				\
		xmm_req_unlock(r);				\
		xmm_svm_terminate(*(k), TRUE);			\
		xmm_req_lock(r);				\
		xmm_obj_lock(m);				\
	} else							\
		xmm_obj_unlock(*(k));				\
								\
	if (_n != XMM_OBJ_NULL)					\
		xmm_obj_lock(_n);				\
	*(k) = _n;						\
MACRO_END

/*
 * svm_klist_abort(m, k)	   svm_klist_abort_req(r, k)
 *	xmm_obj_t m;			request_t r;
 *	xmm_obj_t *k;			xmm_obj_t *n;
 *
 * They abort a run through a klist. They decrements the k_count counter
 *	of the current element (k). Don't forget to destroy this element
 *	if needed.
 */
#define	svm_klist_abort(m, k)					\
MACRO_BEGIN							\
	if (--(*(struct kobj **)(k))->k_count == 0 &&		\
	    (*(struct kobj **)(k))->terminated) {		\
		xmm_obj_unlock(m);				\
		xmm_svm_terminate(*(k), TRUE);			\
		xmm_obj_lock(m);				\
	} else							\
		xmm_obj_unlock(*(k));				\
MACRO_END

#define	svm_klist_abort_req(r, k)				\
MACRO_BEGIN							\
	if (--(*(struct kobj **)(k))->k_count == 0 &&		\
	    (*(struct kobj **)(k))->terminated) {		\
		xmm_req_unlock(r);				\
		xmm_svm_terminate(*(k), TRUE);			\
		xmm_req_lock(r);				\
	} else							\
		xmm_obj_unlock(*(k));				\
MACRO_END

extern void	svm_add_kernel_request(
			xmm_obj_t	kobj,
			vm_offset_t	offset,
			boolean_t	needs_data,
			boolean_t	wants_copy,
			vm_prot_t	desired_access);

extern void	svm_kernel_completed(
			xmm_obj_t	kobj,
			vm_offset_t	offset);

extern void	svm_release_lock_request(
			xmm_obj_t	mobj);

extern void	svm_kernel_supply(
			xmm_obj_t	kobj,
			vm_offset_t	offset,
			vm_map_copy_t	data,
			boolean_t	dirty,
			boolean_t	kernel_copy);

extern void	svm_pager_supply(
			xmm_obj_t	mobj,
			vm_offset_t	offset,
			vm_map_copy_t	data,
			vm_prot_t	lock,
			boolean_t	precious,
			xmm_reply_t	reply,
			kern_return_t	error);

extern void	svm_pager_request(
			xmm_obj_t		mobj,
			vm_offset_t		offset,
			memory_object_return_t	should_return,
			boolean_t		flush,
			vm_prot_t		lock,
			xmm_reply_t		reply);

extern void	svm_request_cleanup(
			xmm_obj_t		mobj);

extern void	svm_synchronize(
			xmm_obj_t	kobj,
			vm_offset_t	offset,
			vm_size_t	length,
			vm_sync_t	sync_flags);

extern void	svm_synchronize_completed(
			xmm_obj_t	mobj,
			vm_offset_t	offset,
			vm_size_t	length);

extern void	svm_request_init(void);

#if	MACH_KDB
extern void	g_svm_print(
			gather_t	g);

#endif	/* MACH_KDB */

#if	MACH_ASSERT
extern void	svm_check_dirty_invariant(
			xmm_obj_t	mobj,
			unsigned int	page,
			request_t	req,
			boolean_t	we_have_data);
#else	/* MACH_ASSERT */
#define	svm_check_dirty_invariant(mobj, page, req, we_have_data)	\
MACRO_BEGIN								\
MACRO_END
#endif	/* MACH_ASSERT */
