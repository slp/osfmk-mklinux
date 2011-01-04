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
 *	Joseph S. Barrera III 1991
 *	Global definitions for svm module.
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

#include <mach_pagemap.h>
#include <mach_assert.h>

#include <xmm/xmm_obj.h>
#include <mach/vm_param.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <kern/macro_help.h>
#include <kern/lock.h>

#include <xmm/svm_state.h>
#include <xmm/svm_temporary.h>
#include <xmm/svm_request.h>
#include <xmm/svm_copy.h>
#include <xmm/svm_change.h>
#include <xmm/svm_pagecopy.h>

#define	MOBJ_STATE_UNCALLED		0
#define	MOBJ_STATE_CALLED		1
#define	MOBJ_STATE_READY		2
#define MOBJ_STATE_SHOULD_TERMINATE	3
#define	MOBJ_STATE_TERMINATED		4

#define	MM(xobj)		((struct mobj *) xobj)
#define	KK(xobj)		((struct kobj *) xobj)

#define	K			KK(k)
#define	READER			KK(reader)
#define	WRITER			KK(writer)
#define	COPY			MM(copy)
#define	NEW_COPY		MM(new_copy)
#define	OLD_COPY		MM(old_copy)
#define	SHADOW			MM(shadow)

extern struct xmm_class		ksvm_class;
extern struct xmm_class		msvm_class;

/*
 * lock is set when pager gives us a message.
 * prot is set when we send message to kernels;
 * it should simply reflect max of all corresponding kobj prot.
 */
struct mobj {
	struct xmm_obj	obj;
	xmm_obj_t	shadow;
	xmm_obj_t	copy;
	xmm_obj_t	kobj_list;
	xmm_obj_t	terminate_mobj;
	unsigned int	num_pages;
	queue_head_t	request_list;
	unsigned int	request_count;
	request_t	last_found;		/* lookup optimization */
	struct svm_state bits;			/* kernel pages access state */
	ipc_port_t	memory_object;		/* for xmm_object_release */
	change_t	change;
#if	MACH_PAGEMAP
	vm_external_map_t pagemap;		/* non-null implies frozen */
	vm_offset_t	existence_size;
#endif	/* MACH_PAGEMAP */
	vm_size_t	cluster_size;
	int		lock_batching;		/* lock coalescing counter */
	short		kobj_count;
	short		k_count;		/* outstanding kernel reqs. */
	unsigned int
	/* enum */	state:3,
	/* boolean_t */	may_cache:1,
	/* enum */	copy_strategy:3,
	/* boolean_t */	temporary:1,
	/* boolean_t */	dirty_copy:1,
	/* boolean_t */	destroyed:1,
	/* boolean_t */	copy_in_progress:1,
	/* boolean_t */	copy_wanted:1,
	/* boolean_t */ ready_wanted:1,
	/* boolean_t */ disable_in_progress:1,
	/* boolean_t */	temporary_disabled:1,	/* only valid when obj is ready
						 * and object is temporary */
	/* boolean_t */ term_sent:1,		/* sent m_o_terminate */
	/* boolean_t */ destroy_needed:1,
	/* boolean_t */	extend_in_progress:1,
	/* boolean_t */	extend_wanted:1;
	queue_head_t	sync_requests;	/* memory_object_sync reqs */
	decl_mutex_data(, mp_lock)
};

#define	svm_copy_event(mobj)	((event_t)(((int) mobj) + 1))
#define	svm_disable_event(mobj)	((event_t)(((int) mobj) + 2))
#define	svm_ready_event(mobj)	((event_t)(((int) mobj) + 2))
#define	svm_extend_event(mobj)	((event_t)(((int) mobj) + 3))

#define	xmm_list_lock_held(mobj)	(mutex_held(&MM(mobj)->mp_lock),1)
#define	xmm_list_lock_init(mobj)	mutex_init(&MM(mobj)->mp_lock, \
						   ETAP_NORMA_MP)
#define	xmm_list_lock(mobj)		mutex_lock(&MM(mobj)->mp_lock)
#define	xmm_list_unlock(mobj)		mutex_unlock(&MM(mobj)->mp_lock)

struct kobj {
	struct xmm_obj	obj;
	struct svm_state bits;			/* kernel pages status */
	xmm_obj_t	mobj;			/* back pointer */
	xmm_obj_t	next;
	gather_t	lock_gather;		/* lock_req coalesce struct */
	short		k_count;
	unsigned
	/* boolean_t */	initialized:1,		/* Ok to launch ops? */
	/* boolean_t */	active:1,		/* mobj ready, kobj !cached */
#if	MACH_PAGEMAP
	/* boolean_t */	has_pagemap:1,
#endif	/* MACH_PAGEMAP */
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	/* boolean_t */	local:1,		/* local kernel object */
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	/* boolean_t */	readied:1,		/* has been sent set_ready */
	/* boolean_t */	needs_terminate:1,	/* terminate pending */
	/* boolean_t */	terminated:1;		/* has been terminated */
};

/*
 * Notes on PRECIOUS_RETURN_HACK:  see vm_pageout.c and svm_request.c.
 * Keep pages as precious so we can keep track of the page.  If a kernel
 * does a data return, it may or may not keep the page, leaving XMM
 * in a lurch.  By forcing all pages as precious, we can better
 * keep track of them.  Could be smarter about this whole thing.
 */
#define	NORMA_VM_PRECIOUS_RETURN_HACK	1
