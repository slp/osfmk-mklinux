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
 * Revision 2.9.9.3  92/03/03  16:27:13  jeffreyh
 * 	Add shadowed bit to vm_object structure.
 * 	[92/02/19  14:28:59  dlb]
 * 
 * 	Add use_shared_copy bit to vm_object.  Remove
 * 	new temporary object copy strategies.
 * 	[92/01/07  11:16:53  dlb]
 * 
 * 	Add definitions for temporary object copy strategies.
 * 	[92/01/06  16:26:07  dlb]
 * 	Changes from TRUNK
 * 	[92/02/26  12:35:56  jeffreyh]
 * 
 * Revision 2.10  92/01/14  16:48:24  rpd
 * 	Added vm_object_bootstrap, vm_object_lookup_name.
 * 	[91/12/31            rpd]
 * 
 * Revision 2.9.9.2  92/02/21  11:29:04  jsb
 * 	Unswapped incorrectly swapped NORMA_VM and non-NORMA_VM cases.
 * 	[92/02/10  09:31:40  jsb]
 * 
 * 	Added declaration of pager_request_t, which is an xmm_obj_t in NORMA_VM
 * 	systems, ipc_port_t otherwise. Removed pager_mobj field.
 * 	[92/02/10  08:49:16  jsb]
 * 
 * 	NORMA_VM: add pager_mobj to vm_object structure.
 * 	[92/02/09  14:46:08  jsb]
 * 
 * Revision 2.9.9.1  92/02/18  19:26:04  jeffreyh
 * 	Fixed references to  LockHolder when VM_OBJECT_DEBUG is true
 * 	[91/09/09            bernadat]
 * 
 * Revision 2.9  91/08/28  11:18:50  jsb
 * 	single_use --> use_old_pageout in vm_object structure.
 * 	[91/07/03  14:18:59  dlb]
 * 
 * Revision 2.8  91/06/25  10:34:39  rpd
 * 	Changed mach_port_t to ipc_port_t where appropriate.
 * 	[91/05/28            rpd]
 * 
 * Revision 2.7  91/05/14  17:50:38  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/02/05  17:59:42  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:33:56  mrt]
 * 
 * Revision 2.5  90/11/05  14:34:48  rpd
 * 	Added vm_object_lock_taken.
 * 	[90/11/04            rpd]
 * 
 * Revision 2.4  90/06/02  15:11:47  rpd
 * 	Converted to new IPC.
 * 	[90/03/26  23:17:24  rpd]
 * 
 * Revision 2.3  90/02/22  20:06:39  dbg
 * 	Changed declarations of vm_object_copy routines.
 * 	[90/01/25            dbg]
 * 
 * Revision 2.2  90/01/11  11:48:13  dbg
 * 	Pick up some changes from mainline:
 * 		Fix vm_object_absent_assert_wait to use vm_object_assert_wait
 * 		instead of vm_object_wait.  Also expanded object lock macros
 * 		under VM_OBJECT_DEBUG to check LockHolder.
 * 		[89/12/21            dlb]
 * 
 * 		Add lock_in_progress, lock_restart fields;
 * 		VM_OBJECT_EVENT_LOCK_IN_PROGRESS event.
 * 		Remove vm_object_request_port().
 * 		[89/08/07            mwyoung]
 * 	 
 * 		Removed object_list.
 * 		[89/08/31  19:45:22  rpd]
 * 	 
 * 		Optimizations from NeXT:  changed ref_count and
 * 		resident_page_count
 * 		to be shorts.  Put LockHolder under VM_OBJECT_DEBUG.  Also,
 * 		added last_alloc field for the "deactivate-behind" optimization.
 * 		[89/08/19  23:48:21  rpd]
 * 
 * 	Coalesced some bit fields (pager_created, pager_initialized,
 * 	pager_ready) into same longword with most others.
 * 	Changes for MACH_KERNEL:
 * 	. Export vm_object_copy_slowly.
 * 	. Remove non-XP definitions.
 * 	[89/04/28            dbg]
 * 
 * Revision 2.1  89/08/03  16:45:44  rwd
 * Created.
 * 
 * Revision 2.12  89/04/18  21:26:50  mwyoung
 * 	Recent history:
 * 		Improved event mechanism.
 * 		Added absent_count, to detect/prevent memory overcommitment.
 * 
 * 	All other history has been incorporated into the documentation
 * 	in this module.  Also, see the history for the implementation
 * 	file ("vm/vm_object.c").
 * 	[89/04/18            mwyoung]
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
 *	File:	vm_object.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Virtual memory object module definitions.
 */

#ifndef	_VM_VM_OBJECT_H_
#define _VM_VM_OBJECT_H_

#include <mach_pagemap.h>
#include <task_swapper.h>
#include <norma_vm.h>

#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/memory_object.h>
#include <mach/port.h>
#include <mach/vm_prot.h>
#include <mach/machine/vm_types.h>
#include <kern/queue.h>
#include <kern/lock.h>
#include <kern/assert.h>
#include <kern/macro_help.h>
#include <ipc/ipc_types.h>
#include <vm/pmap.h>
#include <kern/misc_protos.h>

#if	MACH_PAGEMAP
#include <vm/vm_external.h>
#endif	/* MACH_PAGEMAP */

#if	NORMA_VM
typedef struct xmm_obj *	pager_request_t;
#else	/* NORMA_VM */
typedef struct ipc_port *	pager_request_t;
#endif	/* NORMA_VM */
#define	PAGER_REQUEST_NULL	((pager_request_t) 0)

/*
 *	Types defined:
 *
 *	vm_object_t		Virtual memory object.
 *
 *	We use "struct ipc_port *" instead of "ipc_port_t"
 *	to avoid include file circularities.
 */

struct vm_object {
	queue_chain_t		memq;		/* Resident memory */
	decl_mutex_data(,	Lock)		/* Synchronization */

	vm_size_t		size;		/* Object size (only valid
						 * if internal)
						 */
	vm_size_t		frozen_size;	/* How much has been marked
						 * copy-on-write (only
						 * valid if copy_symmetric)
						 */
	short			ref_count;	/* Number of references */
#if	TASK_SWAPPER
	short			res_count;	/* Residency references (swap)*/
#endif	/* TASK_SWAPPER */
	unsigned int		resident_page_count;
						/* number of resident pages */

	struct vm_object	*copy;		/* Object that should receive
						 * a copy of my changed pages,
						 * for copy_delay, or just the
						 * temporary object that
						 * shadows this object, for
						 * copy_call.
						 */
	struct vm_object	*shadow;	/* My shadow */
	vm_offset_t		shadow_offset;	/* Offset into shadow */

	struct ipc_port		*pager;		/* Where to get data */
	vm_offset_t		paging_offset;	/* Offset into memory object */
	pager_request_t		pager_request;	/* Where data comes back */
	queue_chain_t		msr_q;		/* memory object synchronise
						   request queue */

	memory_object_copy_strategy_t
				copy_strategy;	/* How to handle data copy */

	unsigned int		absent_count;	/* The number of pages that
						 * have been requested but
						 * not filled.  That is, the
						 * number of pages for which
						 * the "absent" attribute is
						 * asserted.
						 */

	unsigned int		paging_in_progress;
						/* The memory object ports are
						 * being used (e.g., for pagein
						 * or pageout) -- don't change
						 * any of these fields (i.e.,
						 * don't collapse, destroy or
						 * terminate)
						 */
	unsigned int
	/* boolean_t array */	all_wanted:11,	/* Bit array of "want to be
						 * awakened" notations.  See
						 * VM_OBJECT_EVENT_* items
						 * below */
	/* boolean_t */	pager_created:1,	/* Has pager been created? */
	/* boolean_t */	pager_initialized:1,	/* Are fields ready to use? */
	/* boolean_t */	pager_ready:1,		/* Will pager take requests? */

	/* boolean_t */		pager_trusted:1,/* The pager for this object
						 * is trusted. This is true for
						 * all internal objects (backed
						 * by the default pager)
						 */
	/* boolean_t */		can_persist:1,	/* The kernel may keep the data
						 * for this object (and rights
						 * to the memory object) after
						 * all address map references 
						 * are deallocated?
						 */
	/* boolean_t */		internal:1,	/* Created by the kernel (and
						 * therefore, managed by the
						 * default memory manger)
						 */
	/* boolean_t */		temporary:1,	/* Permanent objects may be
						 * changed externally by the 
						 * memory manager, and changes
						 * made in memory must be
						 * reflected back to the memory
						 * manager.  Temporary objects
						 * lack both of these
						 * characteristics.
						 */
	/* boolean_t */		private:1,	/* magic device_pager object,
						 * holds private pages only */
	/* boolean_t */		pageout:1,	/* pageout object. contains
						 * private pages that refer to
						 * a real memory object. */
	/* boolean_t */		alive:1,	/* Not yet terminated */

	/* boolean_t */		lock_in_progress:1,
						/* Is a multi-page lock
						 * request in progress?
						 */
	/* boolean_t */		lock_restart:1,
						/* Should lock request in
						 * progress restart search?
						 */
#if	NORMA_VM
	/* boolean_t */		pagemap_exported:1,
						/* pagemap has been given to
						 * memory manager */
	/* boolean_t */		temporary_cached:1,
	/* boolean_t */		temporary_caching:1,
	/* boolean_t */		temporary_uncaching:1,
#endif	/* NORMA_VM */
	/* boolean_t */		shadowed:1,	/* Shadow may exist */
	/* boolean_t */		silent_overwrite:1,
						/* Allow full page overwrite
						 * without data_request if
						 * page is absent */
	/* boolean_t */		advisory_pageout:1;
						/* Instead of sending page
						 * via OOL, just notify
						 * pager that the kernel
						 * wants to discard it, page
						 * remains in object */

	queue_chain_t		cached_list;	/* Attachment point for the
						 * list of objects cached as a
						 * result of their can_persist
						 * value
						 */
	vm_offset_t		last_alloc;	/* last allocation offset */
	vm_size_t		cluster_size;	/* size of paging cluster */
#if	MACH_PAGEMAP
	vm_external_map_t	existence_map;	/* bitmap of pages written to
						 * backing storage */
#endif	/* MACH_PAGEMAP */
#if	MACH_ASSERT
	struct vm_object	*paging_object;	/* object which pages to be
						 * swapped out are temporary
						 * put in current object
						 */
#endif
};

typedef struct vm_object	*vm_object_t;
#define VM_OBJECT_NULL		((vm_object_t) 0)

extern
vm_object_t	kernel_object;		/* the single kernel object */

int		vm_object_absent_max;	/* maximum number of absent pages
					   at a time for each object */

# define	VM_MSYNC_INITIALIZED			0
# define	VM_MSYNC_SYNCHRONIZING			1
# define	VM_MSYNC_DONE				2

struct msync_req {
	queue_chain_t		msr_q;		/* object request queue */
	queue_chain_t		req_q;		/* vm_msync request queue */
	unsigned int		flag;
	vm_offset_t		offset;
	vm_offset_t		length;
	vm_object_t		object;		/* back pointer */
	decl_mutex_data(,	msync_req_lock)	/* Lock for this structure */
};

typedef struct msync_req	*msync_req_t;
#define MSYNC_REQ_NULL		((msync_req_t) 0)

/*
 * Macros to allocate and free msync_reqs
 */
#define msync_req_alloc(msr)						\
	MACRO_BEGIN							\
        (msr) = (msync_req_t)kalloc(sizeof(struct msync_req));		\
        mutex_init(&(msr)->msync_req_lock, ETAP_VM_MSYNC);		\
	msr->flag = VM_MSYNC_INITIALIZED;				\
        MACRO_END

#define msync_req_free(msr)						\
	(kfree((vm_offset_t)(msr), sizeof(struct msync_req)))

#define msr_lock(msr)   mutex_lock(&(msr)->msync_req_lock)
#define msr_unlock(msr) mutex_unlock(&(msr)->msync_req_lock)

/*
 *	Declare procedures that operate on VM objects.
 */

extern void		vm_object_bootstrap(void);

extern void		vm_object_init(void);

extern vm_object_t	vm_object_allocate(
					vm_size_t	size);

#if	MACH_ASSERT
extern void		vm_object_reference(
					vm_object_t	object);
#else	/* MACH_ASSERT */
#define	vm_object_reference(object)			\
MACRO_BEGIN						\
	vm_object_t Object = (object);			\
	if (Object) {					\
		vm_object_lock(Object);			\
		Object->ref_count++;			\
		vm_object_res_reference(Object);	\
		vm_object_unlock(Object);		\
	}						\
MACRO_END
#endif	/* MACH_ASSERT */

#if	NORMA_VM
extern void		vm_object_deallocate_lazy(
					vm_object_t	object);
#endif	/* NORMA_VM */

extern void		vm_object_deallocate(
					vm_object_t	object);

extern void		vm_object_pmap_protect(
					vm_object_t	object,
					vm_offset_t	offset,
					vm_offset_t	size,
					pmap_t		pmap,
					vm_offset_t	pmap_start,
					vm_prot_t	prot);

extern void		vm_object_page_remove(
					vm_object_t	object,
					vm_offset_t	start,
					vm_offset_t	end);

extern boolean_t	vm_object_coalesce(
					vm_object_t	prev_object,
					vm_object_t	next_object,
					vm_offset_t	prev_offset,
					vm_offset_t	next_offset,
					vm_size_t	prev_size,
					vm_size_t	next_size);

extern boolean_t	vm_object_shadow(
					vm_object_t	*object,
					vm_offset_t	*offset,
					vm_size_t	length);

extern void		vm_object_collapse(
					vm_object_t	object);

extern vm_object_t	vm_object_lookup(
					ipc_port_t	port);

extern ipc_port_t	vm_object_name(
					vm_object_t	object);

extern boolean_t	vm_object_copy_quickly(
					vm_object_t	*_object,
					vm_offset_t	src_offset,
					vm_size_t	size,
					boolean_t	*_src_needs_copy,
					boolean_t	*_dst_needs_copy);

extern kern_return_t	vm_object_copy_strategically(
					vm_object_t	src_object,
					vm_offset_t	src_offset,
					vm_size_t	size,
					vm_object_t	*dst_object,
					vm_offset_t	*dst_offset,
					boolean_t	*dst_needs_copy);

extern kern_return_t	vm_object_copy_slowly(
					vm_object_t	src_object,
					vm_offset_t	src_offset,
					vm_size_t	size,
					boolean_t	interruptible,
					vm_object_t	*_result_object);

extern void		vm_object_pager_create(
					vm_object_t	object);

extern void		vm_object_destroy(
					ipc_port_t	pager);

extern void		vm_object_pager_wakeup(
					ipc_port_t	pager);

extern void		vm_object_page_map(
					vm_object_t	object,
					vm_offset_t	offset,
					vm_size_t	size,
					vm_offset_t	(*map_fn)
						(void *, vm_offset_t),
					void 		*map_fn_data);

#if	TASK_SWAPPER

extern void		vm_object_res_reference(
					vm_object_t object);
extern void		vm_object_res_deallocate(
					vm_object_t object);
#define	VM_OBJ_RES_INCR(object)	(object)->res_count++
#define	VM_OBJ_RES_DECR(object)	(object)->res_count--

#else	/* TASK_SWAPPER */

#define	VM_OBJ_RES_INCR(object)
#define	VM_OBJ_RES_DECR(object)
#define vm_object_res_reference(object)
#define vm_object_res_deallocate(object)

#endif	/* TASK_SWAPPER */

extern vm_object_t	vm_object_enter(
					ipc_port_t	pager,
					vm_size_t	size,
					boolean_t	internal,
					boolean_t	init);

#if	NORMA_VM
extern void		vm_object_deallocate_lazy(
					vm_object_t	object);

extern void		vm_object_export_pagemap_try(
					vm_object_t	object,
					vm_size_t	size);

extern void		vm_object_thread(void);
#endif	/* NORMA_VM */

/*
 *	Event waiting handling
 */

#define	VM_OBJECT_EVENT_INITIALIZED		0
#define	VM_OBJECT_EVENT_PAGER_READY		1
#define	VM_OBJECT_EVENT_PAGING_IN_PROGRESS	2
#define	VM_OBJECT_EVENT_ABSENT_COUNT		3
#define	VM_OBJECT_EVENT_LOCK_IN_PROGRESS	4
#define	VM_OBJECT_EVENT_UNCACHING		5
#define	VM_OBJECT_EVENT_COPY_CALL		6
#define	VM_OBJECT_EVENT_CACHING			7

#define	vm_object_wait(object, event, interruptible)			\
	MACRO_BEGIN							\
	(object)->all_wanted |= 1 << (event);				\
	vm_object_sleep(((vm_offset_t) object) + (event),		\
			(object),					\
			(interruptible));				\
	MACRO_END

#define	vm_object_assert_wait(object, event, interruptible)		\
	MACRO_BEGIN							\
	(object)->all_wanted |= 1 << (event);				\
	assert_wait((event_t)((vm_offset_t)(object)+(event)),(interruptible)); \
	MACRO_END

#define	vm_object_wakeup(object, event)					\
	MACRO_BEGIN							\
	if ((object)->all_wanted & (1 << (event)))			\
		thread_wakeup((event_t)((vm_offset_t)(object) + (event))); \
	(object)->all_wanted &= ~(1 << (event));			\
	MACRO_END

#define	vm_object_set_wanted(object, event)				\
	MACRO_BEGIN							\
	((object)->all_wanted |= (1 << (event)));			\
	MACRO_END

#define	vm_object_wanted(object, event)					\
	((object)->all_wanted & (1 << (event)))

/*
 *	Routines implemented as macros
 */

#define		vm_object_paging_begin(object) 				\
	MACRO_BEGIN							\
	(object)->paging_in_progress++;					\
	MACRO_END

#define		vm_object_paging_end(object) 				\
	MACRO_BEGIN							\
	assert((object)->paging_in_progress != 0);			\
	if (--(object)->paging_in_progress == 0) {			\
		vm_object_wakeup(object,				\
			VM_OBJECT_EVENT_PAGING_IN_PROGRESS);		\
	}								\
	MACRO_END

#define		vm_object_paging_wait(object, interruptible)		\
	MACRO_BEGIN							\
	while ((object)->paging_in_progress != 0) {			\
		vm_object_wait(	(object),				\
				VM_OBJECT_EVENT_PAGING_IN_PROGRESS,	\
				(interruptible));			\
		vm_object_lock(object);					\
									\
		/*XXX if ((interruptible) &&	*/			\
		    /*XXX (current_thread()->wait_result != THREAD_AWAKENED))*/ \
			/*XXX break; */					\
	}								\
	MACRO_END

#define	vm_object_absent_assert_wait(object, interruptible)		\
	MACRO_BEGIN							\
	vm_object_assert_wait(	(object),				\
			VM_OBJECT_EVENT_ABSENT_COUNT,			\
			(interruptible));				\
	MACRO_END


#define	vm_object_absent_release(object)				\
	MACRO_BEGIN							\
	(object)->absent_count--;					\
	vm_object_wakeup((object),					\
			 VM_OBJECT_EVENT_ABSENT_COUNT);			\
	MACRO_END

/*
 *	Object locking macros
 */

#define vm_object_lock_init(object)	mutex_init(&(object)->Lock, ETAP_VM_OBJ)
#define vm_object_lock(object)		mutex_lock(&(object)->Lock)
#define vm_object_unlock(object)	mutex_unlock(&(object)->Lock)
#define vm_object_lock_try(object)	mutex_try(&(object)->Lock)
#define vm_object_sleep(event, object, interruptible)			\
		thread_sleep_mutex((event_t)(event), &(object)->Lock,	\
			     (interruptible))

#endif	/* _VM_VM_OBJECT_H_ */
