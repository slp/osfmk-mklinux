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
 *	Routines for reconciling kernel and pager requests.
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
#include <kern/misc_protos.h>
#include <kern/xpr.h>
#include <xmm/xmm_svm.h>
#include <xmm/xmm_methods.h>

#define	MEMORY_OBJECT_RETURN_ALWAYS	MEMORY_OBJECT_RETURN_ANYTHING

/*
 * Notes on PRECIOUS_RETURN_HACK: see vm_pageout.c
 * Keep pages as precious so we can keep track of the page.  If a kernel
 * does a data return, it may or may not keep the page, leaving XMM
 * in a lurch.  By forcing all pages as precious, we can better
 * keep track of them.  Could be smarter about this whole thing.
 */

#define	LOCK_BATCHING	1

zone_t svm_krequest_zone;
zone_t svm_prequest_zone;
zone_t svm_request_zone;
zone_t svm_gather_zone;

typedef struct krequest		*krequest_t;
typedef struct prequest		*prequest_t;

#define	KREQUEST_NULL		((krequest_t) 0)
#define	PREQUEST_NULL		((prequest_t) 0)

#define	_IS_FAKE_DATA(data)	((data == DATA_NONE) || \
				 (data == DATA_UNAVAILABLE) || \
				 (data == DATA_ERROR))
#if	MACH_ASSERT
#define	IS_GOOD_COPY(copy)	(((copy)->type == VM_MAP_COPY_OBJECT ||      \
				  (copy)->type == VM_MAP_COPY_PAGE_LIST ||   \
				  (copy)->type == VM_MAP_COPY_ENTRY_LIST) ?  \
				 TRUE :				      	     \
				 (panic("bad copy object 0x%x", copy),	     \
				  FALSE))

#define	IS_REAL_DATA(data)	(!_IS_FAKE_DATA(data) && IS_GOOD_COPY(data))
#else	/* MACH_ASSERT */
#define	IS_REAL_DATA(data)	(!_IS_FAKE_DATA(data))
#endif	/* MACH_ASSERT */

#if	MACH_KDB
#define	DD(name)	_DD(name, req)
#else	/* MACH_KDB */
#define	DD(name)
#endif	/* MACH_KDB */

/*
 * xmm_sync_req: used for memory_object_synchronize implementation
 *
 *	Used to ensure a single memory_object_synch active request per
 *	object and to keep track of which kernel requested it.
 */

struct xmm_sync_req {
	queue_chain_t	next;	/* protected by xmm_list_lock() */
	xmm_obj_t kobj;		/* requesting kernel */
	vm_offset_t	offset;
	vm_size_t	length;
	vm_sync_t	flags;	/* sync flags */
};

typedef struct xmm_sync_req		*xmm_sync_req_t;
#define XMM_SYNC_REQ_NULL		((xmm_sync_req_t) 0)
#define xmm_sync_req_alloc()	(xmm_sync_req_t) \
					kalloc(sizeof(struct xmm_sync_req))
#define xmm_sync_req_free(sync)	kfree((vm_offset_t)(sync), \
				      sizeof(struct xmm_sync_req))

/*
 * XXX
 * We should know what kernels are local so that we can start off trying to
 * get data from them in the kp_return case.
 */

struct request {
	queue_chain_t	next_chain;		/* must be first */
	/*
	 * These fields are used for both kernel and pager operations.
	 */
	xmm_obj_t	mobj;
	vm_offset_t	offset;
	krequest_t	krequest_list;
	prequest_t	prequest_list;
#if	MACH_ASSERT
	thread_t	active;
	unsigned int
#define ATRUE current_thread()
#define AFALSE THREAD_NULL
#else /*MACH_ASSERT*/
	unsigned int
		/* boolean_t */	active:1,
#define ATRUE TRUE
#define AFALSE FALSE
#endif /*MACH_ASSERT*/
		/* boolean_t */	needs_data:1,
		/* boolean_t */	clean:1,
		/* boolean_t */	kp_returning_surething:1,
		/* boolean_t */ terminating:1;
	enum {
		Nothing_Pending,
		Kernel_Pending,
		Kernel_Completed,
		Pager_Pending,
		All_Completed
	} state;
	enum {
		Kernel_Request,
		Pager_Request,
		Invalid_Request
	} type;

	/*
	 * These fields are initialized by the svm_start_*_request
	 * routines, except for data, which is initialized to DATA_NONE
	 * in svm_get_request and every time a new request is started,
	 * and error, which is valid only when data is set to DATA_ERROR.
	 */
	vm_prot_t	lock;
	vm_map_copy_t	data;
	kern_return_t	error;

	/*
	 * These fields are valid only for pending kernel operations.
	 */
	int		kp_count;
	xmm_obj_t	kp_returning;
	xmm_sync_req_t	sync_req;
#if	MACH_ASSERT
	xmm_obj_t	kp_returning_failure;
	int		last_state;	 /* XXX */
	int		kp_completed_count;
#endif	/* MACH_ASSERT */
	decl_mutex_data(,slock)
};

#define	xmm_req_lock_held(req)	(mutex_held(&(req)->slock), 1)
#define	xmm_req_lock_init(req)	mutex_init(&(req)->slock, ETAP_NORMA_XMM)
#define	xmm_req_lock(req)	mutex_lock(&(req)->slock)
#define	xmm_req_unlock(req)	mutex_unlock(&(req)->slock)

#if	MACH_ASSERT
xmm_obj_t svm_last_returning_failure;
vm_offset_t svm_last_returning_failure_offset;

#define	SJS_DEBUG(val)	req->last_state = (val);
#else	/* MACH_ASSERT */
#define SJS_DEBUG(val)
#endif	/* MACH_ASSERT */

int svm_unfortunate_flush_count = 0;
int svm_lock_request_count = 0;
int svm_merge_dumped =0;
int svm_stale_dumped = 0;
int svm_stale_changed = 0;
int c_data_return_count = 0;

/*
 * Requests made by a kernel.
 */
struct krequest {
	xmm_obj_t	kobj;
	unsigned int
		/* boolean_t */	needs_data:1,
		/* boolean_t */	wants_copy:1;
	vm_prot_t	desired_access;
	krequest_t	next;
};

/*
 * Requests made by a pager (memory manager).
 */
struct prequest {
	xmm_reply_data_t reply_data;
	memory_object_return_t
			should_return;
	boolean_t	flush;
	vm_prot_t	lock;
	prequest_t	next;
};

struct gather {
	vm_offset_t	offset;
	vm_offset_t	begin_offset;
	boolean_t	should_return;
	boolean_t	flush;
	vm_prot_t	lock;
	xmm_reply_t 	reply;
};

/*
 * Forward.
 */

#if	MACH_ASSERT
void		svm_check_writer_invariant(
        			xmm_obj_t	mobj,
        			unsigned int	page);
#endif	/* MACH_ASSERT */

void		svm_do_lock_request(
				xmm_obj_t	kobj,
				request_t	req,
				boolean_t	should_return,
				vm_prot_t	lock);

#if	LOCK_BATCHING
kern_return_t	svm_lock_request_coalesce(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				boolean_t	should_return,
				boolean_t	flush,
				vm_prot_t	lock,
				xmm_reply_t	reply);
#endif	/* LOCK_BATCHING */

void		svm_send_coalesced_lock_request(
				xmm_obj_t	kobj);

void		svm_start_kernel_operation(
				request_t	req);

void		svm_start_pager_operation(
				request_t	req);

void		svm_start_kernel_request(
				request_t	req);

void		svm_start_pager_request(
				request_t	req);

void		svm_dispose_of_data_properly(
				xmm_obj_t	mobj,
				xmm_obj_t	kobj,
				vm_map_copy_t	data,
				vm_offset_t	offset,
				request_t	req);

void		svm_satisfy_kernel_request(
				request_t	req);

void		svm_release_kernel_request(
				request_t	req);

void		svm_satisfy_pager_request(
				request_t	req);

void		svm_release_pager_request(
				request_t	req);

request_t	svm_get_request(
				xmm_obj_t	mobj,
				vm_offset_t	offset);

request_t	svm_lookup_request(
				xmm_obj_t	mobj,
				vm_offset_t	offset);

void		svm_release_request(
				request_t	req);

boolean_t	svm_merge_kernel_request(
				request_t	req,
				krequest_t	nkreq,
				krequest_t	okreq);

void		svm_ask_nonprecious_kernels(
				request_t	req);

void		svm_process_request(
				request_t	req);

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
void		svm_copy_supply(
				xmm_obj_t	kobj,
				vm_offset_t	offset,
				vm_map_copy_t	data);
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */

#if	MACH_KDB
void		_DD(
				char		*name,
				request_t	req);
#endif	/* MACH_KDB */

#if	MACH_ASSERT

/*
 * #define TRACE_RECORD 1
 * will enable primitive tracing facility
 */
#if	!TRACE_RECORD
#define	add_trace
#define	sub_trace
#define	find_trace
#endif	/* !TRACE_RECORD */

void
svm_check_writer_invariant(
        xmm_obj_t	mobj,
        unsigned int	page)
{
        xmm_obj_t writer;

	assert(xmm_obj_lock_held(mobj));
	(void) m_get_writer(mobj, page, &writer);
	if (writer)
		xmm_obj_unlock(writer);
}

void
svm_check_dirty_invariant(
	xmm_obj_t	mobj,
	unsigned int	page,
	request_t	req,
	boolean_t	we_have_data)
{
	char *m_bits;

	assert(xmm_obj_lock_held(mobj));

	/*
	 * If the page is dirty, then there must not be any nonprecious
	 * kernels. Furthermore, if we don't have a copy of the data now,
	 * then there must be a precious kernel with a copy of the data,
	 * or we must have an outstanding request for data to a kernel
	 * that we know will give us data (e.g. from a kernel that was
	 * precious but is no longer marked as so due to a flush).
	 */
	M_MAP_GET(MOBJ, page, m_bits);
	if (M_GET_DIRTY(m_bits)) {
		assert(! m_get_nonprecious_kernel(mobj, page));
		assert(we_have_data ||
		       (req && xmm_req_lock_held(req) &&
			(req->kp_returning_surething ||
			 IS_REAL_DATA(req->data))) ||
		       m_get_precious_kernel(mobj, page));
	}
}
#else	/* MACH_ASSERT */
#define	svm_check_writer_invariant(mobj, page)				\
	MACRO_BEGIN							\
	MACRO_END
#define	add_trace
#define	sub_trace
#define	find_trace
#endif	/* MACH_ASSERT */


/*
 * Start a kernel lock request operation, performing all
 * necessary bookkeeping.
 */
void
svm_do_lock_request(
	xmm_obj_t	kobj,
	request_t	req,
	boolean_t	should_return,
	vm_prot_t	lock)
{
	xmm_reply_data_t reply_data;
	unsigned int page = atop(req->offset);
	xmm_obj_t mobj = KOBJ->mobj;
	boolean_t flush;
	kern_return_t kr;
	char *k_bits;

	XPR(XPR_XMM, "s_do_lock: kobj %X req %X off %X sh_ret %d lock %x\n",
	    (integer_t) kobj, (integer_t) req, (integer_t) req->offset,
	    (integer_t) should_return, (integer_t) lock);
	assert(kobj->class == &ksvm_class);
	assert(xmm_req_lock_held(req));
	assert(xmm_obj_lock_held(mobj));
	assert(xmm_obj_lock_held(kobj));
	assert(page_aligned(req->offset));
	assert(req->state == Kernel_Pending);
	assert(should_return == MEMORY_OBJECT_RETURN_NONE ||
	       should_return == MEMORY_OBJECT_RETURN_DIRTY ||
	       should_return == MEMORY_OBJECT_RETURN_ALWAYS);

	assert(KOBJ->initialized);
	assert(!KOBJ->terminated || KOBJ->needs_terminate);
	K_MAP_GET(KOBJ, page, k_bits);
	assert(! K_GET_PENDING(k_bits));

	/*
	 * Record that we have a pending operation on this kernel
	 * for this page.
	 */
	K_SET_PENDING(k_bits);
	KOBJ->k_count++;
	xmm_obj_unlock(kobj);
	MOBJ->k_count++;
	req->kp_count++;

	svm_check_dirty_invariant(mobj, page, req, FALSE);

	/*
	 * If we ask for data from a precious kernel, then we
	 * are guaranteed that we will get it.
	 */
	if (should_return == MEMORY_OBJECT_RETURN_ALWAYS &&
	    K_GET_PRECIOUS(k_bits) &&
	    req->kp_returning == kobj) {
		req->kp_returning_surething = TRUE;
	}

	XPR(XPR_XMM,
	    "s_do_lock: kobj %X off %X K_GET_PROT %x PREC %x lock %x\n",
	    (integer_t) kobj, (integer_t) req->offset,
	    K_GET_PROT(k_bits), K_GET_PRECIOUS(k_bits), lock);

	/*
	 * Compute flush from lock.
	 * If we a kernel, then it is no longer precious.
	 */
	if (lock == VM_PROT_NO_CHANGE)
		flush = FALSE;
	else {
		flush = (lock == VM_PROT_ALL) ||
			(K_GET_PROT(k_bits) && 
			 ((K_GET_PROT(k_bits) & ~lock) == VM_PROT_NONE));
		if (flush) {
			K_CLR_PRECIOUS(k_bits);
			K_ASN_PROT(k_bits, VM_PROT_NONE);
		} else {
			K_SUB_PROT(k_bits, lock);
		}
		XPR(XPR_XMM,
	   "s_do_lock: changed kobj %X off %X K_GET_PROT %x PREC %x flush %d\n",
		    (integer_t) kobj, (integer_t) req->offset,
		    K_GET_PROT(k_bits), K_GET_PRECIOUS(k_bits), flush);
	}

	/*
	 * Set up a reply, grab a reference, and initiate the operation.
	 */
	xmm_obj_reference(mobj);
	svm_lock_request_count++;
#if	LOCK_BATCHING
	/*
	 *	lock_requests() generated by the pager tend to be
	 *	multiple pages long; try to coalesce them to reduce
	 *	message traffic.
	 */
	if (req->prequest_list && !req->needs_data && MOBJ->lock_batching) {
		xmm_obj_unlock(mobj);
		xmm_req_unlock(req);
		svm_lock_request_coalesce(kobj, req->offset,
					  should_return, flush,
					  lock, &reply_data);
		xmm_req_lock(req);
		return;
	}
#endif	/* LOCK_BATCHING */
	xmm_obj_unlock(mobj);
	xmm_reply_set(&reply_data, XMM_SVM_REPLY, req);
	xmm_req_unlock(req);
	XPR(XPR_XMM,
	    "s_do_lock: K_LOCK_REQ kobj %X off %X sh_ret %d flush %d lock %x\n",
	    (integer_t) kobj, (integer_t) req->offset,
	    (integer_t) should_return, (integer_t) flush, (integer_t) lock);
	kr = K_LOCK_REQUEST(kobj, req->offset, PAGE_SIZE, 
			    should_return, flush,
			    lock, &reply_data);
	assert(kr == KERN_SUCCESS);
	xmm_req_lock(req);
}


#if	LOCK_BATCHING
/*
 *	svm_lock_request_coalesce
 *
 *	Coalesce as many lock_requests as possible into one.  Any
 *	variation in parameters will require the current batch to
 *	be sent off, holding onto the newer set.
 */
kern_return_t
svm_lock_request_coalesce(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	boolean_t	should_return,
	boolean_t	flush,
	vm_prot_t	lock,
	xmm_reply_t	reply)
{
	gather_t g;
	gather_t newg;

	XPR(XPR_XMM,
	    "s_lr_coal: kobj %X off %X sh_ret %d flush %d lock %x\n",
	    (integer_t) kobj, (integer_t) offset,
	    (integer_t) should_return, (integer_t) flush, (integer_t) lock);
	newg = GATHER_NULL;
	xmm_obj_lock(kobj);
	for (;;) {
		g = KOBJ->lock_gather;
		if (g == GATHER_NULL) {
			if (newg != GATHER_NULL)
				break;
			/*
			 *	If this is the first coalesce effort, allocate
			 *	a structure into the kobj.  This will remain
			 *	until the lock_request is flushed by
			 *	svm_send_coalesced_lock_request().
			 */
			xmm_obj_unlock(kobj);
			newg = g = (gather_t) zalloc(svm_gather_zone);
			g->begin_offset = offset;
			g->offset = offset;
			g->should_return = should_return;
			g->flush = flush;
			g->lock  = lock;
			g->reply = reply;

		} else {
			if (offset == g->offset + PAGE_SIZE &&
			    should_return == g->should_return &&
			    flush == g->flush &&
			    lock  == g->lock) {
				/*
				 * Next page in a request -just bump the offset
				 */
				g->offset = offset;
				break;
			}

			/*
			 * Send what we have and start a new gather list.
			 */
			svm_send_coalesced_lock_request(kobj);
		}
		xmm_obj_lock(kobj);
	}

	if (g == GATHER_NULL) {
		KOBJ->lock_gather = newg;
		newg = GATHER_NULL;
	}
	xmm_obj_unlock(kobj);
	if (newg != GATHER_NULL) {
		/*
		 * An unused gather element has been zalloc()ed.
		 *	zfree() it now !
		 */
		zfree(svm_gather_zone, (vm_offset_t) newg);
	}
	return KERN_SUCCESS;
}

#endif	/* LOCK_BATCHING */

/*
 * MP note : Upon call, kobj is locked; Upon return, lock is consumed.
 */
void
svm_send_coalesced_lock_request(
	xmm_obj_t	kobj)
{
	gather_t g;
	xmm_reply_data_t reply_data;

	assert(xmm_obj_lock_held(kobj));
	g = KOBJ->lock_gather;
	KOBJ->lock_gather = GATHER_NULL;
	xmm_obj_unlock(kobj);
	
	assert(g);
	xmm_reply_set(&reply_data, XMM_SVM_REPLY, g->reply);

	XPR(XPR_XMM,
	    "s_send_coal: kobj %X off %X sh_ret %d flush %d lock %x\n",
	    (integer_t) kobj, (integer_t) g->begin_offset,
	    (integer_t) g->should_return, (integer_t) g->flush,
	    (integer_t) g->lock);
	(void) K_LOCK_REQUEST(kobj,
			      g->begin_offset,
			      g->offset - g->begin_offset + PAGE_SIZE,
			      g->should_return,
			      g->flush,
			      g->lock,
			      &reply_data);

	zfree(svm_gather_zone, (vm_offset_t)g);
}

void
svm_release_lock_request(
	xmm_obj_t	mobj)
{
	xmm_obj_t kobj;
	gather_t g;

	assert(xmm_obj_lock_held(mobj));
	svm_klist_first(mobj, &kobj);
	while (kobj) {
		if (KOBJ->lock_gather) {
			xmm_obj_unlock(mobj);
			svm_send_coalesced_lock_request(kobj);
			xmm_obj_lock(mobj);
			xmm_obj_lock(kobj);
		}
		svm_klist_next(mobj, &kobj, TRUE);
	}
}

/*
 * Start a kernel operation on behalf of either a kernel or a pager request.
 */
void
svm_start_kernel_operation(
	request_t	req)
{
	xmm_obj_t mobj = req->mobj;
	vm_prot_t usage;
	xmm_obj_t kobj, requesting_kobj, writer;
	unsigned int page = atop(req->offset);
	boolean_t must_lock, will_write;
	char *k_bits;
	char *m_bits;

	DD("svm_start_kernel_operation");
	assert(xmm_req_lock_held(req));

	/*
	 * Move from state Nothing_Pending to Kernel_Pending.
	 * If we are lucky, we will progress to Kernel_Completed.
	 * Initialize Kernel_Pending specific fields.
	 */
	assert(req->state == Nothing_Pending);

	SJS_DEBUG(1);	/* req->last_state = 1; */
	req->state = Kernel_Pending;
	req->kp_count = 0;
	req->kp_returning = XMM_OBJ_NULL;
	req->sync_req = XMM_SYNC_REQ_NULL;
	req->kp_returning_surething = FALSE;
#if	MACH_ASSERT
	req->kp_returning_failure = XMM_OBJ_NULL;
	req->kp_completed_count = 0;
#endif	/* MACH_ASSERT */

	XPR(XPR_XMM,
	    "s_s_k_op: mobj %X req %X off %X clean %d needs_data %d\n",
	    (integer_t) mobj, (integer_t) req, (integer_t) req->offset,
	    (integer_t) req->clean, (integer_t) req->needs_data);

	/*
	 * See how the page is being used. If it's not
	 * being used, then there is nothing to do.
	 */
	xmm_obj_lock(mobj);
	usage = m_get_writer(mobj, page, &writer);
	assert(writer == XMM_OBJ_NULL || xmm_obj_lock_held(writer));
	if (usage == VM_PROT_NONE) {
		xmm_obj_unlock(mobj);
		SJS_DEBUG(2);	/* req->last_state = 2; */
		req->state = Kernel_Completed;
		XPR(XPR_XMM, "s_s_k_op: PROT_NONE mobj %X req %X off %X\n",
		    (integer_t)mobj,(integer_t)req,(integer_t)req->offset,0,0);
		return;
	}

	/*
	 * Make sure that we are using the clean option properly.
	 * A pager will specify clean if it doesn't know whether the
	 * page is dirty. If it knows that the page is dirty, or
	 * if it needs data for other reasons (precious or always),
	 * then it should specify needs_data instead.
	 */
#if	MACH_ASSERT
	if (req->clean) {
		assert(! req->needs_data);
		M_MAP_GET(MOBJ, page, m_bits);
		assert(! M_GET_DIRTY(m_bits));
		assert(! M_GET_PRECIOUS(m_bits));
	} else {
		m_bits = (char *)0;
	}
#endif	/*MACH_ASSERT*/

	/*
	 * Find the kernel that initiated this request, if any.
	 */
	if (req->krequest_list != KREQUEST_NULL) {
		requesting_kobj = req->krequest_list->kobj;
	} else {
		requesting_kobj = XMM_OBJ_NULL;
	}

	/*
	 * If there is a writer, then it is the only user.
	 * Handle this special case now.
	 *
	 * This is the only case where the clean flag is used.
	 */
	if (writer) {
		memory_object_return_t mreturn;
		vm_prot_t lock;

		/*
		 * If we have a writer, then clean it.
		 * Otherwise, the clean flag can be ignored
		 * for the remainder of this routine.
		 */
		assert(! req->kp_returning);
		assert(! req->kp_returning_surething);
		if (req->clean) {
			req->kp_returning = writer;
			mreturn = MEMORY_OBJECT_RETURN_DIRTY;
		} else if (req->needs_data) {
			req->kp_returning = writer;
			mreturn = MEMORY_OBJECT_RETURN_ALWAYS;
		} else {
			mreturn = MEMORY_OBJECT_RETURN_NONE;
		}

#if	NORMA_VM_PRECIOUS_RETURN_HACK
		/*
		 * Don't lock the writer if he initiated this request.
		 */
		if (writer == requesting_kobj) {
			lock = VM_PROT_NO_CHANGE;
		} else if (req->lock & VM_PROT_WRITE) {
			lock = req->lock;
		} else {
		        lock = VM_PROT_ALL;
		}
#else	/* NORMA_VM_PRECIOUS_RETURN_HACK */
		/*
		 * If we are getting data from a non-precious kernel, then
		 * we must flush the page; otherwise we might violate the
		 * invariant that all kernels are precious whenever the page
		 * is either dirty or precious.
		 */
		if (mreturn) {
			K_MAP_GET(WRITER, page, k_bits);
			if (! K_GET_PRECIOUS(k_bits)) {
				svm_unfortunate_flush_count++;
				lock = VM_PROT_ALL;
			} else
				lock = req->lock;
		} else {
			lock = req->lock;
		}
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */

		/*
		 * Perform the lock request, and return.
		 */
		if (mreturn != MEMORY_OBJECT_RETURN_NONE ||
		    lock != VM_PROT_NO_CHANGE)
			svm_do_lock_request(writer, req, mreturn, lock);
		else {
			xmm_obj_unlock(writer);
			xmm_obj_unlock(mobj);
		}
		if (req->kp_count == 0) {
			SJS_DEBUG(3);	/* req->last_state = 3; */
			req->state = Kernel_Completed;
		}
		XPR(XPR_XMM,
		    "s_s_k_op: writer req %X off %X clean %d needs_data %d req->lock %x\n",
		    (integer_t) req, (integer_t) req->offset,
		    (integer_t) req->clean, (integer_t) req->needs_data,
		    (integer_t) req->lock);
		return;
	}

	/*
	 * If we don't need to return data, and lock does not
	 * conflict with usage, then there is nothing to do.
	 */
	will_write = !(req->lock & VM_PROT_WRITE);
#if	MACH_ASSERT
	if (m_bits == (char *)0)
#endif	/* MACH_ASSERT */
		M_MAP_GET(MOBJ, page, m_bits);
	must_lock = (((usage & req->lock) != VM_PROT_NONE) ||
		     (M_GET_LOCK(m_bits) & ~req->lock) ||
		     (will_write && usage));

	if (! req->needs_data && ! must_lock) {
		xmm_obj_unlock(mobj);
		SJS_DEBUG(4);	/* req->last_state = 4; */
		req->state = Kernel_Completed;
		XPR(XPR_XMM,
		    "s_s_k_op: do nothing %X off %X clean %d needs_data %d req->lock %x\n",
		    (integer_t) req, (integer_t) req->offset,
		    (integer_t) req->clean, (integer_t) req->needs_data,
		    (integer_t) req->lock);
		return;
	}

	/*
	 * If we need to return data, then we will want to know if
	 * we have a precious kernel, and if so, who it is.
	 */
	if (req->needs_data) {
		assert(! req->kp_returning);
		assert(! req->kp_returning_surething);
		req->kp_returning = m_get_precious_kernel(mobj, page);
#if	MACH_ASSERT
		if (req->kp_returning == XMM_OBJ_NULL) {
			/*
			 * If we don't have a precious kernel, then
			 * we must be neither precious nor dirty.
			 */
			if (m_bits == (char *)0)
				M_MAP_GET(MOBJ, page, m_bits);
			assert(! M_GET_PRECIOUS(m_bits));
			assert(! M_GET_DIRTY(m_bits));
		}
#endif	/*MACH_ASSERT*/
	}

	/*
	 * If lock does not conflict with usage, then ask for data now.
	 */
	if (! must_lock) {

		/*
		 * If we don't need to lock, then we must need data.
		 */
		assert(req->needs_data);
		assert(! req->data);

		/*
		 * If we have a precious kernel, then ask it for data.
		 * N.B. A kobj holding a precious page can't be terminated.
		 */
		if (req->kp_returning) {
			xmm_obj_lock(req->kp_returning);
			svm_do_lock_request(req->kp_returning, req,
					    (req->needs_data ?
					     MEMORY_OBJECT_RETURN_ALWAYS :
					     MEMORY_OBJECT_RETURN_NONE),
					    req->lock);
		} else
			xmm_obj_unlock(mobj);

		/*
		 * If we didn't have a precious kernel or if the precious
		 * kernel has already completed, and we still don't have
		 * data, then we ask nonprecious kernels.
		 */
		if (! req->kp_returning && ! req->data) {
			xmm_obj_lock(mobj);
			svm_ask_nonprecious_kernels(req);
			xmm_obj_unlock(mobj);
		}

		/*
		 * If we aren't waiting for a reply, then either we have data,
		 * or we don't have any kernels left to ask.
		 */
		if (req->kp_count == 0) {
#if	MACH_ASSERT
			if (!req->data) {
				xmm_obj_lock(mobj);
				assert(! m_get_prot(mobj, page));
				xmm_obj_unlock(mobj);
			}
#endif	/* MACH_ASSERT */
			SJS_DEBUG(5);	/* req->last_state = 5; */
			req->state = Kernel_Completed;
		}
		XPR(XPR_XMM,
		    "s_s_k_op: needed data req %X off %X clean %d needs_data %d req->lock %x\n",
		    (integer_t) req, (integer_t) req->offset,
		    (integer_t) req->clean, (integer_t) req->needs_data,
		    (integer_t) req->lock);
		return;
	}

	/*
	 * Lock conflicts with usage.
	 * If not locking against write then flush all readers
	 */
	assert(must_lock);
	assert(writer == XMM_OBJ_NULL);

	/*
	 * Iterate over all kernels, flushing all with any usage, except for
	 * the requesting kernel.
	 *
	 * We may need to return data. If we don't have a precious kernel, then
	 * we just ask the first kernel that we need to flush.
	 */
	if (!(req->lock & VM_PROT_WRITE)) {
	    svm_klist_first(mobj, &kobj);
	    while (kobj) {
		K_MAP_GET(KOBJ, page, k_bits);
		if (k_bits != (char *)0 && K_GET_PROT(k_bits) &&
		    kobj != requesting_kobj && !KOBJ->terminated) {
			if (req->needs_data &&
			    ! req->kp_returning &&
			    ! req->data) {
				assert(! req->kp_returning_surething);
				req->kp_returning = kobj;
			}
			svm_do_lock_request(kobj, req,
			    (kobj == req->kp_returning ?
			     MEMORY_OBJECT_RETURN_ALWAYS :
			     MEMORY_OBJECT_RETURN_NONE),
			    VM_PROT_ALL);
			xmm_obj_lock(mobj);
			xmm_obj_lock(kobj);
		}
		/*
		 * consumes current kobj lock and takes next kobj lock if any.
		 */
		svm_klist_next_req(mobj, &kobj, req);
	    }
	}

	/*
	 * If we wanted data and didn't get it, and there is a requesting
	 * kobj, and it has some access to the page, and all operations
	 * have completed or the requesting kobj is the returning kobj (i.e.
	 * if we have not asked any kobj for data), then ask it for data.
	 */
	if (req->needs_data && ! req->data && requesting_kobj &&
	    (req->kp_count == 0 || requesting_kobj == req->kp_returning)) {
		xmm_obj_lock(requesting_kobj);
		K_MAP_GET(KK(requesting_kobj), page, k_bits);
		if (K_GET_PROT(k_bits)) {
#if	NORMA_VM_PRECIOUS_RETURN_HACK
			/*
			 * If a dirty page is returned, the kernel will have
			 * marked its copy of the page precious.
			 */
			svm_do_lock_request(requesting_kobj, req,
					    MEMORY_OBJECT_RETURN_ALWAYS,
					    VM_PROT_NO_CHANGE);
#else	/* NORMA_VM_PRECIOUS_RETURN_HACK */
			/*
			 * If the kernel does not have this page precious,
			 * then we must flush the page, so that we don't
			 * violate the invariant that all users of a dirty
			 * page are precious. In this case, we change kreq
			 * so that we provide the kernel with a copy of the
			 * page, even though it did not ask for one.
			 */
			if (K_GET_PRECIOUS(k_bits)) {
				svm_do_lock_request(requesting_kobj, req,
						   MEMORY_OBJECT_RETURN_ALWAYS,
						   VM_PROT_NO_CHANGE);
			} else {
				krequest_t kreq = req->krequest_list;
				assert(req->type == Kernel_Request);
				assert(kreq->kobj == requesting_kobj);
				assert(! kreq->needs_data);
				kreq->needs_data = TRUE;
				svm_do_lock_request(requesting_kobj, req,
						   MEMORY_OBJECT_RETURN_ALWAYS,
						   VM_PROT_ALL);
			}
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
		} else {
			xmm_obj_unlock(requesting_kobj);
			xmm_obj_unlock(mobj);
		}
	} else
		xmm_obj_unlock(mobj);

	/*
	 * If all operations have completed, then we are done.
	 */
	if (req->kp_count == 0) {
		SJS_DEBUG(6);	/* req->last_state = 6; */
		req->state = Kernel_Completed;
	}
}

void
svm_start_pager_operation(
	request_t	req)
{
	xmm_obj_t mobj = req->mobj;
	unsigned int page = atop(req->offset);
	boolean_t needs_data;
	vm_prot_t m_lock, desired_access;
	kern_return_t kr;
	char *m_bits;

	DD("svm_start_pager_operation");
	assert(xmm_req_lock_held(req));
	assert(req->active != AFALSE);
	assert(req->state == Kernel_Completed);
	assert(req->prequest_list == PREQUEST_NULL);
	assert(req->type == Kernel_Request);

	/*
	 * If we don't need data and we don't need an unlock, then
	 * we are done.
	 */
	needs_data = (req->data == DATA_NONE && req->needs_data);
	xmm_obj_lock(mobj);
	M_MAP_GET(MOBJ, page, m_bits);
	m_lock = M_GET_LOCK(m_bits);
	if (! needs_data && ! (m_lock & ~req->lock)) {
		xmm_obj_unlock(mobj);
		SJS_DEBUG(9);	/* req->last_state = 9; */
		req->state = All_Completed;
		return;
	}

	/*
	 * If we need data, then the page must be neither precious
	 * nor dirty. In fact, the page must not be in use at all.
	 */
#if	MACH_ASSERT
	if (needs_data) {
		assert(! M_GET_PRECIOUS(m_bits));
		assert(! M_GET_DIRTY(m_bits));
		assert(m_get_prot(mobj, page) == VM_PROT_NONE);
	}
#endif	/* MACH_ASSERT */
	xmm_obj_unlock(mobj);

	/*
	 * We cannot hold onto data in the request while waiting for
	 * a reply from a memory manager, since such a wait can take
	 * arbitrarily long. Thus we return now, after downgrading
	 * req->lock and kreq->desired_access, without doing a
	 * data_unlock. If the data is destined for the kernel, then
	 * it will generate a data_unlock of its own, regardless of
	 * whether we have one in progress. Note that if we have data,
	 * then the pager must already allow us at least read access
	 * to the page.
	 */
	if (req->data) {
		assert(IS_REAL_DATA(req->data));
		assert(m_lock != VM_PROT_ALL);
		if ((req->lock & m_lock)
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
		    && svm_copy_call_push_page_to_kernel == 0
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
		    ) {
			panic("XXX this code is wrong");
			req->lock = m_lock ^ VM_PROT_ALL;
			req->krequest_list->desired_access = req->lock;
		}
		SJS_DEBUG(10);	/* req->last_state = 10; */
		req->state = All_Completed;
		return;
	}

	/*
	 * Compute desired access from lock value.
	 */

	/*XXX Check this XXX*/
	desired_access = req->lock ^ VM_PROT_ALL;

	/*
	 * Make the appropriate request, based on whether we need data.
	 * The pager reply will advance state to All_Completed when it arrives.
	 */
	SJS_DEBUG(11);	/* req->last_state = 11; */
	req->state = Pager_Pending;
	if (req->needs_data) {
		xmm_req_unlock(req);
		kr = M_DATA_REQUEST(req->mobj, req->offset, PAGE_SIZE,
				    desired_access);
	} else {
		xmm_req_unlock(req);
		kr = M_DATA_UNLOCK(req->mobj, req->offset, PAGE_SIZE,
				   desired_access);
	}
	assert(kr == KERN_SUCCESS);
	xmm_req_lock(req);
}

/*
 * Compute needs_data, clean, and lock values for req, based on kreq,
 * and start any needed kernel operation.
 */
void
svm_start_kernel_request(
	request_t	req)
{
	unsigned int page;
	xmm_obj_t kobj, mobj;
	krequest_t kreq;
	char *k_bits;
	char *m_bits;

	DD("svm_start_kernel_request");
	assert(xmm_req_lock_held(req));
	assert(req->krequest_list != KREQUEST_NULL);
	assert(req->prequest_list == PREQUEST_NULL);
	page = atop(req->offset);
	mobj = req->mobj;
	kreq = req->krequest_list;
	kobj = kreq->kobj;

	assert(mobj->class == &msvm_class);
	assert(kobj->class == &ksvm_class);

	/*
	 * Claim the current request as ours.
	 */
	req->type = Kernel_Request;

	/*
	 * Compute lock value from desired access.
	 *
	 * XXX
	 * The svm_process_request routine is reponsible for optimizing
	 * away the lock of a reader asking for write.
	 */

	req->lock = kreq->desired_access ^ VM_PROT_ALL;

	/*
	 * Update our notion of kernel usage based on the request;
	 * if it asks us for something, it must no longer have it.
	 *
	 * If the kernel needs data, then it must not have any
	 * current usage (the page is absent). It must not be the
	 * case that we thought that it was precious.
	 *
	 * If the kernel does not need data, then it must already have
	 * some access, and thus must be asking for write access,
	 * which tells us that it does not have write access.
	 * (This all follows from the fact that we never let a kernel
	 * hold data with no access.)
	 *
	 * XXX
	 * Except... that intervening lock requests made of this
	 * kernel may have made its queued request obsolete...
	 */
	xmm_obj_lock(mobj);
	svm_check_dirty_invariant(mobj, page, req, FALSE);

	xmm_obj_lock(kobj);
	K_MAP_GET(KOBJ, page, k_bits);
	if (kreq->needs_data) {
		assert(! K_GET_PRECIOUS(k_bits));
		K_ASN_PROT(k_bits, VM_PROT_NONE);
		XPR(XPR_XMM,
	    "s_s_k_req: mobj %X req %X off %X lock %x needs_data K_PROT NONE\n",
			(integer_t)mobj, (integer_t)req, req->offset,
			req->lock, 0);
	} else if (K_GET_PROT(k_bits) == VM_PROT_NONE) {
		/*
		 * Some kernel thinks it still has data, and we know
		 * it does not; svm probably cleaned it while this
		 * request was in the mail.  Give its data back with
		 * the requested prots.
		 */
		kreq->needs_data = TRUE;
		XPR(XPR_XMM,
	    "s_s_k_req: mobj %X req %X off %X lock %x SET needs_data K_PROT NONE\n",
			(integer_t)mobj, (integer_t)req, req->offset,
			req->lock, 0);
	}
	xmm_obj_unlock(kobj);

	/*
	 * We need data if either the kernel needs data, or if data
	 * is needed for a push into a copy object.
	 */
	svm_check_dirty_invariant(mobj, page, req, FALSE);
	if (kreq->needs_data) {
		req->needs_data = TRUE;
		XPR(XPR_XMM,
		  "s_s_k_req: mobj %X req %X off %X nd TRUE kreq->needs_data\n",
			(integer_t)mobj, (integer_t)req,
			(integer_t)req->offset, 0, 0);
	} else if (req->lock & VM_PROT_WRITE) {
		req->needs_data = FALSE;
		XPR(XPR_XMM,
		    "s_s_k_req: mobj %X req %X off %X nd FALSE lock&WRITE\n",
			(integer_t)mobj, (integer_t)req,
			(integer_t)req->offset, 0, 0);
	} else if (MOBJ->copy_in_progress) {
		req->needs_data = TRUE;
		XPR(XPR_XMM,
		    "s_s_k_req: mobj %X req %X off %X nd TRUE copy_in_prog\n",
			(integer_t)mobj, (integer_t)req,
			(integer_t)req->offset, 0, 0);
	} else if (MOBJ->copy == XMM_OBJ_NULL) {
		req->needs_data = FALSE;
		XPR(XPR_XMM,
		    "s_s_k_req: mobj %X req %X off %X nd FALSE copy==NULL\n",
			(integer_t)mobj, (integer_t)req,
			(integer_t)req->offset, 0, 0);
	} else {
		M_MAP_GET(MOBJ, page, m_bits);
		req->needs_data = M_GET_NEEDS_COPY(m_bits);
		XPR(XPR_XMM, "mobj %X req %X off %X nd %d M_GET_NEEDS_COPY\n",
			(integer_t)mobj, (integer_t)req,
			(integer_t)req->offset, req->needs_data, 0);
	}
	xmm_obj_unlock(mobj);

	/*
	 * We never need to specify clean, since clean means return data
	 * only if it is dirty. If we don't have data, then we will have
	 * specified needs_data, that is, always return data. If we do
	 * have data, then no other kernel can have newly dirty data,
	 * since memory is kept consistent.
	 */
	req->clean = FALSE;

	/*
	 * Start any required kernel operation.
	 */
	svm_start_kernel_operation(req);
}

/*
 * Compute needs_data, clean, and lock values for req, based on preq,
 * and start any needed kernel operation.
 */
void
svm_start_pager_request(
	request_t	req)
{
	xmm_obj_t mobj = req->mobj;
	prequest_t preq = req->prequest_list;
	unsigned int page = atop(req->offset);
	memory_object_return_t should_return;
	char *m_bits;
	char *k_bits;

	DD("svm_start_pager_request");
	assert(mobj->class == &msvm_class);
	assert(xmm_req_lock_held(req));
	assert(req->data == DATA_NONE);
	assert(preq);

	/*
	 * Claim the current request as ours.
	 */
	req->type = Pager_Request;

	should_return = preq->should_return;

	req->lock = preq->lock;

	/*
	 * If we are locking against all access, but not flushing,
	 * then we don't want to lose the page. We must therefore
	 * make sure that should_return preserves valuable pages.
	 */
	xmm_obj_lock(mobj);
	if (should_return != MEMORY_OBJECT_RETURN_ALWAYS) {
		xmm_obj_t k;
		vm_prot_t p;
		svm_klist_first(mobj, &k);
		while (k) {
			K_MAP_GET(K, page, k_bits);
			p = K_GET_PROT(k_bits);
			if ((p & ~req->lock) == VM_PROT_NONE) {
				should_return = MEMORY_OBJECT_RETURN_ALL;
				svm_klist_abort(mobj, &k);
				break;
			}
			svm_klist_next(mobj, &k, TRUE);
		}
	}

	/*
	 * Asking for a flush is the same as locking against all access,
	 * except for the page returning implications, which we have
	 * dealt with immediately above.
	 */
	if (preq->flush) {
		req->lock = VM_PROT_ALL;
	}

	/*
	 * Asking for RETURN_ALL when the page is precious is
	 * equivalent to asking for RETURN_ALWAYS; asking for
	 * RETURN_ALL when the page is not precious is equivalent
	 * to asking for RETURN_DIRTY.
	 */
	if (should_return == MEMORY_OBJECT_RETURN_ALL) {
		M_MAP_GET(MOBJ, page, m_bits);
		if (M_GET_PRECIOUS(m_bits)) {
			should_return = MEMORY_OBJECT_RETURN_ALWAYS;
		} else {
			should_return = MEMORY_OBJECT_RETURN_DIRTY;
		}
	} else {
		m_bits = (char *)0;
	}

	/*
	 * Asking for RETURN_DIRTY when the page is known
	 * to be dirty is equivalent to asking for RETURN_ALWAYS.
	 */
	if (should_return == MEMORY_OBJECT_RETURN_DIRTY) {
		if (m_bits == (char *)0)
			M_MAP_GET(MOBJ, page, m_bits);
		if (M_GET_DIRTY(m_bits))
			should_return = MEMORY_OBJECT_RETURN_ALWAYS;
	}

	/*
	 * Compute req->needs_data and req->clean from should_return.
	 * Note that needs_data and clean are mutually exclusive.
	 * Note also that we have eliminated MEMORY_OBJECT_RETURN_ALL.
	 */
	if (should_return == MEMORY_OBJECT_RETURN_ALWAYS) {
		req->clean = FALSE;
		req->needs_data = TRUE;
	} else if (should_return == MEMORY_OBJECT_RETURN_DIRTY) {
		req->clean = TRUE;
		req->needs_data = FALSE;
	} else {
		/*
		 * If we are asking for data to be destroyed, then
		 * turn off the precious and dirty bits, so that the
		 * code does not worry about the fact that it is
		 * dropping the data on the floor (something it
		 * otherwise works quite hard not to do).
		 */
		assert(should_return == MEMORY_OBJECT_RETURN_NONE);
		req->clean = FALSE;
		req->needs_data = FALSE;
		if (m_bits == (char *)0)
			M_MAP_GET(MOBJ, page, m_bits);
		M_CLR_PRECIOUS(m_bits);
		M_CLR_DIRTY(m_bits);
	}
	xmm_obj_unlock(mobj);

	/*
	 * Start any required kernel operation.
	 */
	svm_start_kernel_operation(req);
}

/*
 * "Please dispose of properly" -- discard or return unneeded data.
 *
 * We simply discard the page when we can, returning the page
 * to the pager only when we have to, which is when we have the
 * only copy of a precious or dirty page.
 *
 * If kobj == XMM_OBJ_NULL, then data is from an unknown kernel.
 */
void
svm_dispose_of_data_properly(
	xmm_obj_t	mobj,
	xmm_obj_t	kobj,
	vm_map_copy_t	data,
	vm_offset_t	offset,
	request_t	req)
{
	unsigned int page = atop(offset);
	boolean_t dirty;
	kern_return_t kr;
	char *m_bits;
	xmm_obj_t precious_kernel;
	boolean_t kernel_copy;

	assert(xmm_obj_lock_held(mobj));
	if (req)
		assert(xmm_req_lock_held(req));

	/*
	 * This routine is only called on real data.
	 */
	assert(IS_REAL_DATA(data));

	/*
	 * We may discard data if this page is not precious and not dirty.
	 */
	M_MAP_GET(MOBJ, page, m_bits);
	dirty = M_GET_DIRTY(m_bits);
	if (! M_GET_PRECIOUS(m_bits) && ! dirty) {
		xmm_obj_unlock(mobj);
		if (req)
			xmm_req_unlock(req);
		vm_map_copy_discard(data);
		return;
	}

	/*
	 * We may also discard data if there is a precious kernel.
	 *
	 * Note that the only time that a page can be precious or dirty,
	 * yet have no precious kernel, is when the data lives only in
	 * a request. In particular, if there is no precious kernel for
	 * this page, then no kernel has any access to this page.
	 */

	precious_kernel = m_get_precious_kernel(mobj, page);

	if (precious_kernel &&
	    (precious_kernel != kobj
	     || (m_get_prot(mobj, page) & VM_PROT_WRITE) == 0)) {
		xmm_obj_unlock(mobj);
		if (req)
			xmm_req_unlock(req);
		vm_map_copy_discard(data);
		return;
	}

	if (precious_kernel) {
		assert((m_get_prot(mobj, page) & VM_PROT_WRITE));
		/*
		 * Even though it returns data, this kernel
		 * keeps write access to it. This data return is
		 * explicitely made by the kernel but is
		 * not the result of a lock/unlock request and is
		 * not the result of pageout activity.
		 * It can be invoked for reasons such
		 * as memory_object_synchronize requests,
		 * the data must be returned to the pager.
		 * We must not clear the precious status, as
		 * this kernel keeps a copy and informs the
		 * pager it is doing so.
		 */
		kernel_copy = TRUE;
		M_CLR_DIRTY(m_bits);
	} else {
		assert(m_get_prot(mobj, page) == VM_PROT_NONE);

		/*
		 * This data is important, and we have the only copy.
		 * Return it to the memory manager. Unset precious and dirty.
		 * In M_DATA_RETURN, dirty is what it was, and kernel_copy
		 * is false (since no kernel has any access to this page).
		 */

		kernel_copy = FALSE;
		M_CLR_PRECIOUS(m_bits);
		M_CLR_DIRTY(m_bits);
	}

	xmm_obj_unlock(mobj);
	if (req) {
		xmm_req_unlock(req);
	} else
		assert(kobj != XMM_OBJ_NULL);
	
	kr = M_DATA_RETURN(mobj, offset, data, PAGE_SIZE, dirty, kernel_copy);
	assert(kr == KERN_SUCCESS);
}

void
svm_satisfy_kernel_request(
	request_t	req)
{
	krequest_t kreq;
	unsigned int page;
	xmm_obj_t mobj, kobj;
	boolean_t precious;
	kern_return_t kr;
	vm_prot_t current_lock, net_lock, net_access;
	vm_map_copy_t data;
	char *k_bits;
	char *m_bits;

	DD("svm_satisfy_kernel_request");
	assert(xmm_req_lock_held(req));
	kreq = req->krequest_list;
	assert(req->state == All_Completed);
	assert(req->type == Kernel_Request);
	assert(kreq);

	mobj = req->mobj;
	kobj = kreq->kobj;
	page = atop(req->offset);
	assert(mobj->class == &msvm_class);

	assert(kobj->class == &ksvm_class);

	xmm_obj_lock(mobj);
	M_MAP_GET(MOBJ, page, m_bits);

	/*
	 * If we originally faulted in a copy of this object,
	 * but a page has been pushed into a copy object,
	 * then we must restart the fault at the top of the
	 * shadow chain so that we obtain the pushed page
	 * instead of this page which will be changing.
	 */
	if (kreq->wants_copy) {
		while (MOBJ->copy_in_progress) {
			xmm_req_unlock(req);
			MOBJ->copy_wanted = TRUE;
			assert_wait(svm_copy_event(mobj), FALSE);
			xmm_obj_unlock(mobj);
			thread_block((void (*)(void)) 0);
			xmm_req_lock(req);
			xmm_obj_lock(mobj);
		}

		if (MOBJ->copy && ! M_GET_NEEDS_COPY(m_bits)) {
			if (IS_REAL_DATA(req->data)) {
				data = req->data;
				req->data = DATA_NONE;
				svm_dispose_of_data_properly(mobj,
							     XMM_OBJ_NULL,
							     data, req->offset,
							     req);
			} else {
				xmm_obj_unlock(mobj);
				xmm_req_unlock(req);
			}
			kr = K_DATA_ERROR(kobj, req->offset, PAGE_SIZE,
					  KERN_MEMORY_DATA_MOVED);
			assert(kr == KERN_SUCCESS);
			xmm_req_lock(req);
			return;
		}
	}

	/*
	 * If we need to push a real page into this object's copy, do so now.
	 * DATA_ERROR: copied object isn't really changing,
	 * DATA_UNAVAILABLE: copy will be restarted at vm_fault layer if needed
	 */
	if (IS_REAL_DATA(req->data) && !(req->lock & VM_PROT_WRITE) &&
	    (MOBJ->copy_in_progress || 
	     (MOBJ->copy && M_GET_NEEDS_COPY(m_bits)))) {
		xmm_obj_unlock(mobj);
		assert(req->needs_data);

		/*
		 * If the kernel also needs this data, then
		 * push a copy; otherwise, push the page
		 * itself, and mark it consumed.
		 */
		if (!kreq->needs_data) {
			data = req->data;
			req->data = DATA_NONE;
		}
		xmm_req_unlock(req);

		if (kreq->needs_data)
			data = svm_copy_page(req->data);

		svm_copy_push_page(mobj, req->offset, data);
		xmm_req_lock(req);
	} else
		xmm_obj_unlock(mobj);

	xmm_obj_lock(mobj);
	current_lock = M_GET_LOCK(m_bits);
	xmm_obj_unlock(mobj);
	net_access = kreq->desired_access & ~current_lock;
	net_lock = VM_PROT_ALL & ~net_access;

	/*
	 * If kernel did not request data, then satisfy its request
	 * with a lock_request. We must not have data in this case.
	 */
	if (! kreq->needs_data) {
		assert(req->data == DATA_NONE);
		xmm_obj_lock(kobj);
		K_MAP_GET(KOBJ, page, k_bits);
		K_ASN_PROT(k_bits, net_access);
		XPR(XPR_XMM,
		    "s_sat_k_req: !nd mobj %X req %X off %X K_PROT %x\n",
			(integer_t)mobj, (integer_t)req, req->offset,
			K_GET_PROT(k_bits), 0);
		xmm_obj_unlock(kobj);
#if	MACH_ASSERT
		xmm_obj_lock(mobj);
		svm_check_writer_invariant(mobj, page);
		xmm_obj_unlock(mobj);
#endif	/* MACH_ASSERT */
		xmm_req_unlock(req);
		kr = K_LOCK_REQUEST(kobj, req->offset, PAGE_SIZE,
				    MEMORY_OBJECT_RETURN_NONE,
				    FALSE, net_lock,
				    XMM_REPLY_NULL);
		assert(kr == KERN_SUCCESS);
		xmm_req_lock(req);
		return;
	}

	/*
	 * The kernel requested data.
	 *
	 * If data was not available, then forward the data_unavailable
	 * to the kernel, after granting full access.
	 */
	if (req->data == DATA_UNAVAILABLE) {
#if	MACH_PAGEMAP || MACH_ASSERT
		xmm_obj_lock(mobj);
#endif	/* MACH_PAGEMAP || MACH_ASSERT */
		xmm_obj_lock(kobj);
#if	MACH_PAGEMAP
		if (KOBJ->has_pagemap &&
		    (req->offset < MOBJ->existence_size) &&
		    (vm_external_state_get(MOBJ->pagemap, req->offset) == 
		    VM_EXTERNAL_STATE_EXISTS)) {
			xmm_obj_unlock(kobj);
			xmm_obj_unlock(mobj);
			xmm_req_unlock(req);
			kr = K_DATA_ERROR(kobj, req->offset, PAGE_SIZE,
					  KERN_MEMORY_DATA_MOVED);
			assert(kr == KERN_SUCCESS);
			xmm_req_lock(req);
			return;
		}
#endif	/* MACH_PAGEMAP */

		K_MAP_GET(KOBJ, page, k_bits);
		K_ASN_PROT(k_bits, VM_PROT_ALL);
		XPR(XPR_XMM,
		    "s_sat_k_req: unavail mobj %X req %X off %X K_PROT %x\n",
			(integer_t)mobj, (integer_t)req, req->offset,
			K_GET_PROT(k_bits), 0);
		xmm_obj_unlock(kobj);
		svm_check_writer_invariant(mobj, page);
		assert(m_check_kernel_prots(mobj, page));
#if	MACH_PAGEMAP || MACH_ASSERT
		xmm_obj_unlock(mobj);
#endif	/* MACH_PAGEMAP || MACH_ASSERT */
		xmm_req_unlock(req);
		kr = K_DATA_UNAVAILABLE(kobj, req->offset, PAGE_SIZE);
		assert(kr == KERN_SUCCESS);
		xmm_req_lock(req);
		return;
	}

	/*
	 * Likewise, forward data errors to the kernel.
	 */
	if (req->data == DATA_ERROR) {
		xmm_req_unlock(req);
		kr = K_DATA_ERROR(kobj, req->offset, PAGE_SIZE, req->error);
		assert(kr == KERN_SUCCESS);
		xmm_req_lock(req);
		return;
	}

	/*
	 * The kernel requested data, and we have data for it.
	 */
	assert(req->needs_data);
	assert(IS_REAL_DATA(req->data));
	
	/* We have an implicit assumption here that we do not give
	 * the kernel data if it does not have any access to it.
	 * There is a later assumption that asking for data that
	 * the kernel does have means a request for write.  This
	 * needs to be addressed
         */
	if (net_lock == VM_PROT_ALL) {
		/*
		 * We got back data but were not allowed any access.
		 * Toss the data and send back an error.
		 */
		xmm_obj_lock(mobj);
		M_ASN_LOCK(m_bits, VM_PROT_ALL);
		xmm_obj_unlock(mobj);
		xmm_obj_lock(kobj);
		K_MAP_GET(KOBJ, page, k_bits);
		K_ASN_PROT(k_bits, VM_PROT_NONE);
		XPR(XPR_XMM,
		    "s_sat_k_req: no acc mobj %X req %X off %X K_PROT %x\n",
			(integer_t)mobj, (integer_t)req, req->offset,
			K_GET_PROT(k_bits), 0);
		xmm_obj_unlock(kobj);
		req->data = DATA_NONE;
		xmm_req_unlock(req);
		vm_map_copy_discard(req->data);
		kr = K_DATA_ERROR(kobj, req->offset, PAGE_SIZE,
						KERN_PROTECTION_FAILURE);
		assert(kr == KERN_SUCCESS);
		xmm_req_lock(req);
		return;
	}
	
	/*
	 * Supply the data to the kernel. Tell the kernel to treat
	 * the data as precious if it is either precious or dirty.
	 *
	 * XXX
	 * We only need one kernel to keep it as precious.
	 * If that kernel gives it up but some other kernel
	 * is still using it, how do we make the still-using
	 * kernel now treat data as precious?
	 *
	 * It may make sense to have the last kernel be the
	 * one to keep the data as precious. On the other
	 * hand, it is convenient to have the kernel local
	 * to the consistency module have the precious page.
	 *
	 * Could use a bit for most-recently-supplied kernel.
	 */
	xmm_obj_lock(mobj);
	xmm_obj_lock(kobj);
	K_MAP_GET(KOBJ, page, k_bits);
	assert(! K_GET_PRECIOUS(k_bits));
	if (precious = (M_GET_PRECIOUS(m_bits) ||
			M_GET_DIRTY(m_bits))) {
		K_SET_PRECIOUS(k_bits);
	}
	K_ASN_PROT(k_bits, net_access);
	XPR(XPR_XMM,
		"s_sat_k_req: supply mobj %X req %X off %X K_PROT %x prec %d\n",
		(integer_t)mobj, (integer_t)req, req->offset,
		K_GET_PROT(k_bits), precious);
	xmm_obj_unlock(kobj);
	svm_check_writer_invariant(mobj, page);

	/*
	 * XXX need to do lock_requests to other kernels here if
	 * access is decreasing
	 */
	assert(m_check_kernel_prots(mobj, page));

	xmm_obj_unlock(mobj);
	xmm_req_unlock(req);
	kr = K_DATA_SUPPLY(kobj, req->offset, req->data, PAGE_SIZE,
			   net_lock, precious, XMM_REPLY_NULL);

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	if (svm_copy_call_push_page_to_kernel && kr == KERN_INVALID_ARGUMENT) {
		/*
		 * Copy pages returned to the kernel may have been sent to a
		 *	[to be] terminated vm_object. Return them to the pager.
		 */
		assert(precious);
		xmm_obj_lock(kobj);
		K_CLR_PRECIOUS(k_bits);
		K_ASN_PROT(k_bits, VM_PROT_NONE);
		XPR(XPR_XMM,
		"s_sat_k_req: push return mobj %X req %X off %X K_PROT %x\n",
			(integer_t)mobj, (integer_t)req, req->offset,
			K_GET_PROT(k_bits), 0);
		xmm_obj_unlock(kobj);

		/*
		 * Take a reference to prevent premature termination.
		 */
		xmm_obj_lock(mobj);
		MOBJ->k_count++;
		/* consumes all locks */
		(void)svm_data_return(mobj, req->offset, req->data,
				      PAGE_SIZE, FALSE, FALSE, req);
	} else
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	if (kr != KERN_SUCCESS) {
		panic("K_DATA_SUPPLY returned 0x%x; will retry\n", kr);
		kr = K_DATA_SUPPLY(kobj, req->offset, req->data, PAGE_SIZE,
				   net_lock, precious, XMM_REPLY_NULL);
		printf("K_DATA_SUPPLY returned 0x%x\n", kr);
	}
	xmm_req_lock(req);

	/*
	 * Mark data as consumed.
	 */
	req->data = DATA_NONE;
}

void
svm_release_kernel_request(
	request_t	req)
{
	register krequest_t kreq;

	DD("svm_release_kernel_request");
	assert(xmm_req_lock_held(req));
	kreq = req->krequest_list;
	req->krequest_list = kreq->next;
	zfree(svm_krequest_zone, (vm_offset_t) kreq);
}

void
svm_satisfy_pager_request(
	request_t	req)
{
	xmm_obj_t mobj = req->mobj;
	prequest_t preq = req->prequest_list;
	kern_return_t kr;
	char *m_bits;

	assert(mobj->class == &msvm_class);
	assert(xmm_req_lock_held(req));
	assert(preq);

	DD("svm_satisfy_pager_request");

	/*
	 * Return data, if any.
	 * XXX
	 * Should be done in kernel_supply? But that complicates
	 * tests for multiple returns of data, etc.
	 */
	xmm_obj_lock(mobj);
	if (req->data) {
		boolean_t dirty, precious, kernel_copy;
		unsigned int page = atop(req->offset);

		assert(IS_REAL_DATA(req->data));
		assert(req->clean || req->needs_data);
		M_MAP_GET(MOBJ, page, m_bits);
		dirty = M_GET_DIRTY(m_bits);
		M_CLR_DIRTY(m_bits);
		precious = M_GET_PRECIOUS(m_bits);
		kernel_copy = (m_get_prot(mobj, page) != VM_PROT_NONE);
		if (precious) {
			if (! kernel_copy) {
				M_CLR_PRECIOUS(m_bits);
			} else {
				assert(m_get_precious_kernel(mobj, page));
			}
		}
		xmm_obj_unlock(mobj);
		xmm_req_unlock(req);
		kr = M_DATA_RETURN(mobj, req->offset, req->data, PAGE_SIZE,
				   dirty, kernel_copy);
		assert(kr == KERN_SUCCESS);
		xmm_req_lock(req);
		xmm_obj_lock(mobj);
		req->data = DATA_NONE;
	} else {
		assert(! req->needs_data);
		m_bits = (char *)0;
	}

	/*
	 * Update page lock.
	 */
	if (preq->lock != VM_PROT_NO_CHANGE) {
		if (m_bits == (char *)0)
			M_MAP_GET(MOBJ, atop(req->offset), m_bits);
		M_ASN_LOCK(m_bits, preq->lock);
		assert(m_check_kernel_prots(mobj, atop(req->offset)));
	}
	xmm_obj_unlock(mobj);

	/*
	 * Send reply, if any, to pager.
	 */
	if (!xmm_reply_empty(&preq->reply_data)) {
		kern_return_t kr;
		xmm_req_unlock(req);
		kr = M_LOCK_COMPLETED(mobj, req->offset, PAGE_SIZE,
				      &preq->reply_data);
		assert(kr == KERN_SUCCESS);
		xmm_req_lock(req);
	}
}

void
svm_release_pager_request(
	request_t	req)
{
	register prequest_t preq;

	DD("svm_release_pager_request");
	assert(xmm_req_lock_held(req));
	preq = req->prequest_list;
	req->prequest_list = preq->next;
	zfree(svm_prequest_zone, (vm_offset_t) preq);
}

unsigned int	c_max_request_chain = 0; /* alan */
unsigned int	c_max_gr_loop = 0; /* alan */
unsigned int	c_svm_get_request_calls = 0; /* alan */
#define update_gr_counters					\
	if (MOBJ->request_count > c_max_request_chain)		\
		c_max_request_chain = MOBJ->request_count;	\
	if (c_loop > c_max_gr_loop)				\
		c_max_gr_loop = c_loop;

request_t
svm_get_request(
	xmm_obj_t	mobj,
	vm_offset_t	offset)
{
	request_t req, r;
	unsigned int	c_loop = 0;	/* alan */

	++c_svm_get_request_calls;

	xmm_obj_lock(mobj);
	svm_extend_if_needed(mobj, atop(offset));
	xmm_obj_unlock(mobj);

	r = svm_lookup_request(mobj, offset);
	if (r != REQUEST_NULL) {
		assert(xmm_req_lock_held(r));
		xmm_list_unlock(mobj);
		return r;
	}

	req = (request_t) zget(svm_request_zone);
	if (req == REQUEST_NULL) {
		/* must block, so must make sure things haven't changed */
		xmm_list_unlock(mobj);
		req = (request_t) zalloc(svm_request_zone);
		r = svm_lookup_request(mobj, offset);
		if (r != REQUEST_NULL) {
			assert(xmm_req_lock_held(r));
			xmm_list_unlock(mobj);
			zfree(svm_request_zone, (vm_offset_t) req);
			return r;
		}
	}

	req->mobj = mobj;
	req->offset = offset;
	req->krequest_list = KREQUEST_NULL;
	req->prequest_list = PREQUEST_NULL;
	req->needs_data = FALSE;
	req->active = AFALSE;
	req->terminating = FALSE;
	SJS_DEBUG(14);	/* req->last_state = 14; */
	req->state = Nothing_Pending;
	req->data = DATA_NONE;
	req->type = Invalid_Request;
	xmm_req_lock_init(req);

	for (r = (request_t) queue_first(&MOBJ->request_list);
	     !queue_end(&MOBJ->request_list, (queue_head_t *) r);
	     r = (request_t) queue_next(&r->next_chain)) {
		++c_loop;
		if (r->offset < req->offset)
			break;
	}
	enqueue_tail(&r->next_chain, &req->next_chain);
	MOBJ->request_count++;
	xmm_req_lock(req);
	xmm_list_unlock(mobj);
	update_gr_counters;		/* alan */

	return req;
}

unsigned int	c_svm_lookup_request = 0;
unsigned int	c_svm_lookup_request_backward = 0;
unsigned int	c_svm_lookup_request_hit = 0;
unsigned int	c_svm_lookup_fast_small = 0;
unsigned int	c_max_lr_outer_loop = 0; /* alan */
#define	update_lr_counters					\
	if (c_outer > c_max_lr_outer_loop)			\
		c_max_lr_outer_loop = c_outer;

/*
 * MP note : Upon return, mobj list lock is held.
 */
request_t
svm_lookup_request(
	xmm_obj_t	mobj,
	vm_offset_t	offset)
{
	request_t	r, last, q_end;
	vm_offset_t	smallest_offset;
	boolean_t	forward;
	int		delta_smallest;
	unsigned int	c_outer;	/* alan */

	assert(mobj->class == &msvm_class);
	++c_svm_lookup_request;
	xmm_obj_lock(mobj);
	svm_extend_if_needed(mobj, atop(offset));
	xmm_obj_unlock(mobj);

	/*
	 *	Fast hit on previous lookup.  Optimizes	lookup/remove case.
	 */
	xmm_list_lock(mobj);
	if ((r = MOBJ->last_found) != REQUEST_NULL && r->offset == offset) {
		++c_svm_lookup_request_hit;
		MOBJ->last_found = REQUEST_NULL;
		xmm_req_lock(r);
		return r;
	}

	/*
	 *	Decide in which direction to search along
	 *	the queue.  The queue is ordered with the
	 *	largest offset first, i.e., queue_first on
	 *	the header returns the request with the
	 *	largest offset.  Default search is largest
	 *	towards smallest.  Switch direction if
	 *	the desired offset is within a few pages of
	 *	the smallest offset.
	 */
	forward = TRUE;
	if (MOBJ->request_count > 4) {
		last = (request_t) queue_last(&MOBJ->request_list);
		if ((smallest_offset = last->offset) >= offset) {
			++c_svm_lookup_fast_small;
			if (smallest_offset == offset) {
				MOBJ->last_found = last;
				xmm_req_lock(last);
				return last;
			}
			return REQUEST_NULL;
		}
		delta_smallest = (int) (offset - smallest_offset);
		assert(delta_smallest > 0);
		if (delta_smallest < 3 * PAGE_SIZE) {
			forward = FALSE;
			++c_svm_lookup_request_backward;
		}
	}

	/*
	 *	Traverse the queue.  When travelling forwards,
	 *	finding a request with an offset less than the
	 *	one sought implies the request queue doesn't
	 *	contain the offset.  The opposite condition
	 *	applies when travelling backwards.
	 */
	r = (forward == TRUE ?
	     (request_t) queue_first(&MOBJ->request_list) :
	     (request_t) queue_last(&MOBJ->request_list));
	c_outer = 0;			/* alan */
	while (!queue_end(&MOBJ->request_list, (queue_head_t *) r)) {
		++c_outer;	/* alan */
		if ((forward == TRUE && r->offset < offset) ||
		    (forward == FALSE && r->offset > offset))
			break;
		if (r->offset == offset) {
			MOBJ->last_found = r;
			update_lr_counters; /* alan */
			xmm_req_lock(r);
			return r;
		}
		r = (forward == TRUE ?
		     (request_t) queue_next(&r->next_chain) :
		     (request_t) queue_prev(&r->next_chain));
	}
	update_lr_counters;	/* alan */
	return REQUEST_NULL;
}

unsigned int	c_svm_release_request_calls = 0;

/*
 * MP note : Upon return, req lock has been consumed.
 */
void
svm_release_request(
	request_t	req)
{
	xmm_obj_t mobj = req->mobj;
	request_t r;

	DD("svm_release_request");
	assert(xmm_req_lock_held(req));
	++c_svm_release_request_calls;
	/*
	 * Locks have to be taken in order. So, we have to release request
	 *      lock in order to take MOBJ->request_list lock. So, request
	 *      might have been taken back for another kernel or pager request.
	 *      In that case, release request is not issued.
	 *
	 * Terminating flag is set for multiprocessor purposes. When a thread
	 *	wants to release a request and that request is used by another
	 *	thread at the same time, it may happen that both threads want
	 *	to release the request at the same time. The request will be
	 *      released by exactly one thread (depending on the scheduling).
	 */
	if (req->terminating) {
		xmm_req_unlock(req);
		return;
	}
	req->terminating = TRUE;
	xmm_req_unlock(req);

	xmm_list_lock(mobj);
	assert(MOBJ->request_count > 0);
	xmm_req_lock(req);
	if (req->prequest_list != PREQUEST_NULL ||
	    req->krequest_list != KREQUEST_NULL) {
		/*
		 * The request is being used by another thread. So, make it
		 * come back to life.
		 */
		req->terminating = FALSE;
		xmm_req_unlock(req);
		xmm_list_unlock(mobj);
		return;
	}

	if (MOBJ->last_found == req)
		MOBJ->last_found = REQUEST_NULL;
	MOBJ->request_count--;
	(void) remque(&req->next_chain);
	assert(MOBJ->request_count > 0 || queue_empty(&MOBJ->request_list));
	xmm_list_unlock(mobj);
	xmm_req_unlock(req);
	zfree(svm_request_zone, (vm_offset_t) req);
}

void
svm_add_kernel_request(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	boolean_t	needs_data,
	boolean_t	wants_copy,
	vm_prot_t	desired_access)
{
	request_t req;
	krequest_t kreq, *kp, kr;
	vm_prot_t cur_prot;
	unsigned int page = atop(offset);
	char *k_bits;

	/*
	 * Create a kernel request.
	 */
	kreq = (krequest_t) zalloc(svm_krequest_zone);
	kreq->kobj = kobj;
	kreq->needs_data = needs_data;
	kreq->wants_copy = wants_copy;
	kreq->desired_access = desired_access;

	/*
	 * Get current request.
	 */
	req = svm_get_request(KOBJ->mobj, offset);
	assert(req);
	assert(xmm_req_lock_held(req));

	/*
	 * Put kernel request at end of the list. Don't merge with any
	 *	krequest if fake data is present in the request, because of a
	 *	possible concurrency problem with the VM component (only
	 *	on MP systems):
	 *
	 * 1) VM sends a DATA_REQUEST to XMM which forwards it to the pager.
	 * 2) Thread (A) receives a DATA_UNAVAILABLE from the pager and
	 *	forwards it to the kernel which will wakeup a thread (B)
	 *	waiting for an answer.
	 * 3) Thread (B) removes the page since absent at VM layer.
	 * 4) Another thread (C) sends a DATA_REQUEST to XMM for that page.
	 * 5) If thread (A) has not yet removed its krequest, then thread (C)
	 *	merges its request to the current one, and returns.
	 * 6) Thread (A) is now back to XMM and removes its krequest, thus
	 *	leaving to a DATA_REQUEST in VM without any answer.
	 */
	if (req->data == DATA_UNAVAILABLE || req->data == DATA_ERROR)
		for (kp = &req->krequest_list; kr = *kp; kp = &kr->next)
			continue;
	else
		for (kp = &req->krequest_list; kr = *kp; kp = &kr->next) {
			if (kobj == kr->kobj) {
				if (svm_merge_kernel_request(req, kreq, kr)) {
					xmm_req_unlock(req);
					svm_merge_dumped++;
					zfree(svm_krequest_zone, 
					      (vm_offset_t) kreq);
					return;
				}
			}
		}

	/*
	 * See if this is a stale request.  If we don't change the lock
	 * value in an interesting way, drop the request.  This situation 
	 * happens when a page is bouncing back and forth and:
	 *	- Kernel's A and B have the page read only.  Both
	 *	  try to write the page at about the same time, issuing
	 *	  data_unlock requests.
	 *	- Kernel A's data_unlock is processed first.  This
	 *	  results in a lock_request to kernel B to flush.
	 *	- Kernel B's data_unlock is queued because we are
	 *	  working on kernel A's.
	 *	- Kernel B responds to our lock_request to flush; it
	 *	  immediately issues a data_request because it still
	 *        wants to write the page.
	 *	- Meanwhile, kernel A discards its page and issues a
	 *	  data_request for it.
	 *	- We get kernel B's lock_completed and issue a data_supply
	 *	  to kernel A.
	 *	- Now we unqueue Kernel B's data_unlock, and observe
	 *	  that kernel B no longer has data.  At this point we
	 *	  could discard kernel B's data_unlock.  Instead
	 *	  we mark it needs_data, and send a lock_request to
	 *	  clean and flush to kernel A.
	 * Notice that both kernel A and kernel B have "stale" data
	 * requests on their way to us at this point.  Kernel B's will
	 * be dropped by the merge code above (it merges with the data_unlock
	 * in progress).  Kernel A's is dropped by the code below.
	 */

	if (kreq->needs_data) {
		xmm_obj_lock(kobj);
		K_MAP_GET(KOBJ, page, k_bits);
		if (K_GET_PRECIOUS(k_bits)) {
			/*
			 * see if we need to change lock value - go for
			 * greater access
			 */
			cur_prot = K_GET_PROT(k_bits);
			if ((cur_prot | desired_access) == cur_prot) {
				xmm_obj_unlock(kobj);
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
				/*
				 * If there is no other requests,
				 *	then release structure.
				 */
				if (svm_copy_call_push_page_to_kernel == 0 ||
				    req->prequest_list || req->krequest_list) {
					xmm_req_unlock(req);
				} else {
					/* consumes lock */
					svm_release_request(req);
				}
#else	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
				xmm_req_unlock(req);
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
				svm_stale_dumped++;
				zfree(svm_krequest_zone, (vm_offset_t) kreq);
				return;
			}
			svm_stale_changed++;
			kreq->needs_data = FALSE;
		}
		xmm_obj_unlock(kobj);
	}

	*kp = kreq;
	kreq->next = KREQUEST_NULL;

	/*
	 * Start processing request if appropriate.
	 */
	if (req->state == Nothing_Pending)
		svm_process_request(req);
	else
		xmm_req_unlock(req);
}

/*
 * Found a kernel request for the same page as an outstanding request.
 * Return TRUE if we successfully merge the request.
 */
boolean_t
svm_merge_kernel_request(
	request_t	req,
	krequest_t	nkreq,
	krequest_t	okreq)
{
	if (req->krequest_list == okreq) {
		/*
		 * Assume in progress.  New lock value set to maximum.
		 */
		nkreq->desired_access |= okreq->desired_access;
		nkreq->needs_data = (nkreq->needs_data && !okreq->needs_data);
		if (! nkreq->needs_data && 
		    nkreq->desired_access == okreq->desired_access)
			/*
			 * nothing unique to add, consider merged
			 */
			return(TRUE);

		return (FALSE);
	}
       /*
	* coalesce new into old request
	*/
	okreq->desired_access |= nkreq->desired_access;
	okreq->needs_data |= nkreq->needs_data;

	return (TRUE);
}

/*
 * Try to satisfy a request for data by asking all kernels which
 * might have data. Called only when there are no precious kernels
 * to try, which can only be true if the page is neither precious
 * nor dirty. Also, not called when lock conflicts with usage;
 * that case handles data requesting in the process of locking kernels.
 * (We may however be called after flushing all nonrequesting kernels.)
 */
void
svm_ask_nonprecious_kernels(
	request_t	req)
{
	xmm_obj_t kobj, mobj = req->mobj;
	unsigned int page = atop(req->offset);
	char *k_bits;
#if	MACH_ASSERT
	char *m_bits;
#endif

	DD("svm_add_nonprecious_kernels");
	assert(xmm_req_lock_held(req));
	assert(req->active != AFALSE);
	assert(req->state == Kernel_Pending);
	assert(req->data == DATA_NONE);
	assert(req->needs_data);
	assert(req->kp_returning == XMM_OBJ_NULL);
	assert(req->kp_count == 0);
	assert(xmm_obj_lock_held(mobj));
	assert(m_get_precious_kernel(mobj, page) == XMM_OBJ_NULL);
#if	MACH_ASSERT
	M_MAP_GET(MOBJ, page, m_bits);
#endif
	assert(! M_GET_DIRTY(m_bits));
	assert(! M_GET_PRECIOUS(m_bits));
	assert(! (m_get_prot(mobj, page) & req->lock));

	/*
	 * Try each kernel in the list that is using the page.
	 * Return upon success (data has arrived) or if we need
	 * to wait for a pending kernel operation. If the operation
	 * fails to return data, it will trigger another search.
	 *
	 * XXXO
	 * Perhaps we could use a bit for the kernel most recently
	 * supplied with a particular page, to increase our chances
	 * of finding a non-precious kernel that still has data.
	 *
	 * XXX
	 * Make sure not to flush the requesting kernel ----
	 * except that I don't think we should be called in that case.
	 *
	 * Dont ask a terminated kernel for data as it is likely
	 * to have been discarded (clean-nonprecious pages).
	 *
	 * svm_klist_first() takes kobj lock if any.
	 */
	svm_klist_first(mobj, &kobj);
	while (kobj) {
		K_MAP_GET(KOBJ, page, k_bits);
		if (k_bits != (char *)0 &&
		    K_GET_PROT(k_bits) && !KOBJ->terminated) {
			assert(! req->kp_returning);
			assert(! req->kp_returning_surething);
			req->kp_returning = kobj;
			svm_do_lock_request(kobj, req,
					    MEMORY_OBJECT_RETURN_ALWAYS,
					    VM_PROT_NO_CHANGE);
			if (req->kp_returning || req->data) {
				/*
				 * consumes current kobj lock.
				 */
				xmm_obj_lock(kobj);
				svm_klist_abort_req(req, &kobj);
				xmm_obj_lock(mobj);
				return;
			}
			xmm_obj_lock(mobj);
			xmm_obj_lock(kobj);
		}
		svm_klist_next_req(mobj, &kobj, req);
	}
	assert(m_get_prot(mobj, page) == VM_PROT_NONE);
}

void
svm_kernel_completed(
	xmm_obj_t	kobj,
	vm_offset_t	offset)
{
	request_t req;
	unsigned int page = atop(offset);
	char *k_bits;

	/*
	 * A pending kernel operation has completed.
	 */
	DD("svm_kernel_completed");
	req = svm_get_request(KOBJ->mobj, offset);
	assert(xmm_req_lock_held(req));
#if	MACH_ASSERT
	req->kp_completed_count++;
#endif	/* MACH_ASSERT */

	/*
	 * The request will be pager pending if a data_return has been
	 * sent while waiting for the lock_completed to come down.  In
	 * this case the req will be active so it will take the early
	 * exit.
	 */
	assert(req->state == Kernel_Pending);

	/*
	 * Check to see whether we asked this particular kernel for data.
	 */
	xmm_obj_lock(kobj);
	K_MAP_GET(KOBJ, page, k_bits);
	if (kobj == req->kp_returning) {
		/*
		 * If we thought that this kernel had data, but it failed to
		 * return it to us when we asked, then it must not in fact
		 * have data. In this case, the kernel must not have been
		 * precious, or it is terminating (and dumped clean pages).
		 */
		if (req->needs_data && ! req->data) {
			if (KOBJ->terminated) {
/*				req->needs_data = FALSE; */
				req->kp_returning_surething = FALSE;
			}
#if	MACH_ASSERT
			else if (req->kp_returning_surething) {
				req->kp_returning_failure = kobj;
				svm_last_returning_failure = kobj;
				svm_last_returning_failure_offset = offset;
			}
#endif	/* MACH_ASSERT */
			assert(! req->kp_returning_surething);
			K_ASN_PROT(k_bits, VM_PROT_NONE);
			XPR(XPR_XMM,
		    "s_k_compl: no data kobj %X req %X off %X K_PROT NONE\n",
				(integer_t)kobj, (integer_t)req,
				req->offset, 0, 0);
		}

		/*
		 * Record that the kernel that we were counting on to return
		 * data has completed. This allows anyone actively processing
		 * this request to ask another kernel for data when appropriate
		 * by checking (! req->kp_returning && ! req->data).
		 */
		req->kp_returning = XMM_OBJ_NULL;
		req->kp_returning_surething = FALSE;
	}

	/*
	 * Record that we are no longer waiting for this request.
	 */
	assert(K_GET_PENDING(k_bits));
	K_CLR_PENDING(k_bits);
	xmm_obj_unlock(kobj);

	/*
	 * See whether we should initiate a new kernel
	 * operation or a kernel request satisfy.
	 *
	 * If there are more pending kernel operations,
	 * or if the request is active and thus someone
	 * else is already managing this request, then
	 * don't do anything.
	 */
	assert(req->kp_count > 0);
	if (--req->kp_count > 0 || req->active != AFALSE) {
		xmm_req_unlock(req);
		return;
	}

	if (req->state == Pager_Pending && req->type == Kernel_Request) {
		printf("Completed done: Pager Pending 0x%x 0x%x\n",
		       kobj, req);
		assert(0);
	}

	/*
	 * If we need data and still don't have it, then then try any
	 * kernel that might have data. Return if we have to wait
	 * for a pending kernel operation.
	 */
	if (req->needs_data && req->data == DATA_NONE) {
		/*
		 * We must not have any precious kernels, since otherwise we
		 * would already have data. Thus we ask nonprecious kernels.
		 */
		req->active = ATRUE;
		xmm_obj_lock(req->mobj);
		assert(! m_get_precious_kernel(req->mobj, page));
		svm_ask_nonprecious_kernels(req);
		xmm_obj_unlock(req->mobj);
		req->active = AFALSE;
		if (req->kp_count > 0) {
			xmm_req_unlock(req);
			return;
		}
	}

	/*
	 * There are no more operations to ask of the kernel,
	 * and thus we enter the Kernel_Completed state.
	 * Let svm_process_request continue processing for
	 * this request.
	 */
	assert(req->kp_count == 0);
	SJS_DEBUG(15);	/* req->last_state = 15; */
	req->state = Kernel_Completed;
	svm_process_request(req); /* consumes lock */
}

/*
 * Memory object synchronize kernel request
 *
 * Because data_returns (issued by memory_object_sync()) are
 * not queued at XMM level and as XMM preserves message ordering
 * There should be no data returns in progress (check it)
 * Invoke or queue a memory_object_synchonize() for the 
 * pager (One at a time).
 */

void
svm_synchronize(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	vm_sync_t	sync_flags)
{
	request_t req;
	xmm_obj_t mobj;
	xmm_sync_req_t sync_req;
	krequest_t kreq;
	int can_sync = 0;

	mobj = KOBJ->mobj;

	/*
 	 * Allocate and initialize an xmm_sync_req.
	 */

	sync_req = xmm_sync_req_alloc();
	
	sync_req->kobj = kobj;
	sync_req->offset = offset;
	sync_req->length = length;
	sync_req->flags = sync_flags;

	/*
	 * Find out related pending request.
	 * (ie: data_return on related vm region)
	 * If any, allocate a synchronize object
	 * and reference it from all related requests.
	 * The last processed pending request will call the
	 * pager's synchronize routine via xmm_sync_unref().
	 * Else, if no pending requests, try to invoke the
	 * pager's synchronize routine.
	 */

	xmm_list_lock(mobj);

#if	MACH_ASSERT
	queue_iterate(&MOBJ->request_list, req, request_t, next_chain) {
		xmm_req_lock(req);
		if ((offset - req->offset >= length) ||
		    ((!req->data) && req->needs_data)) {
			xmm_req_unlock(req);
			continue;
		}
		for (kreq = req->krequest_list; kreq; kreq = kreq->next) {
			assert (kobj != kreq->kobj || kreq->needs_data);
		}
		xmm_req_unlock(req);
	}
#endif	/* MACH_ASSERT */
		
	/* Keep a single active memory_object_synchronize per mobj */

	if (queue_empty(&MOBJ->sync_requests))
	       can_sync++;
	queue_enter(&MOBJ->sync_requests, sync_req, xmm_sync_req_t, next);
	xmm_list_unlock(mobj);
	if (can_sync)
		(void) M_SYNCHRONIZE(KOBJ->mobj, sync_req->offset,
				     sync_req->length, sync_req->flags);
}

/*
 * Memory object_synchronize completed, called from the pager.
 *
 * We have no information on which kernel as requested the sync.
 * So we always make sure that there is a single sync in progress per
 * object. All pending xmm_sync_req are linked into the mobj.
 * The active one is at the head of the queue
 *
 * Once we found the related kobj, we can call the related kernel with
 * synchronize_completed(). Then we must check if there are no
 * other sync requests to process.
 */

void
svm_synchronize_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length)
{
	xmm_obj_t kobj;
  	xmm_sync_req_t sync;
	xmm_sync_req_t next_sync;

	/* Find out corresponding kobj */

	xmm_list_lock(mobj);
	assert(!queue_empty(&MOBJ->sync_requests));
	queue_remove_first(&MOBJ->sync_requests, sync, xmm_sync_req_t, next);
	assert(sync->offset == offset && sync->length == length);

	kobj = sync->kobj;

	/* check for other pending memory_object_sync requests */

	if (!queue_empty(&MOBJ->sync_requests))
		next_sync = (xmm_sync_req_t)queue_first(&MOBJ->sync_requests);
	else
		next_sync = XMM_SYNC_REQ_NULL;

	xmm_list_unlock(mobj);

	xmm_sync_req_free(sync);
	K_SYNCHRONIZE_COMPLETED(kobj, offset, length);
	if (next_sync)
		(void) M_SYNCHRONIZE(mobj, next_sync->offset,
				     next_sync->length, next_sync->flags);
}

void
svm_process_request(
	request_t	req)
{
	xmm_obj_t mobj;

	/*
	 * Stop recursion by allowing only one instance of request
	 * management at a time.
	 */
	DD("svm_process_request");
	assert(xmm_req_lock_held(req));
	if (req->active != AFALSE) {
		xmm_req_unlock(req);
		return;
	}
	req->active = ATRUE;

	/*
	 * Take a reference to prevent premature termination
	 */
	mobj = req->mobj;
	xmm_obj_lock(mobj);
	MOBJ->k_count++;
	xmm_obj_unlock(mobj);
	
	/*
	 * Handle the resumption of request processing from an
	 * intermediate state.
	 */
	if (req->state == Kernel_Completed) {
		goto _Kernel_Completed;
	}
	if (req->state == All_Completed) {
		goto _All_Completed;
	}
	assert(req->state == Nothing_Pending);

	while (req->prequest_list || req->krequest_list) {

		/*
		 * Start processing request based on type.
		 */
		assert(req->state == Nothing_Pending);
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
		if (svm_copy_call_push_page_to_kernel == 0)
#endif	/* !COPY_CALL_PUSH_PAGE_TO_KERNEL */
			assert(req->data == DATA_NONE);
		if (req->prequest_list) {
			svm_start_pager_request(req);
			assert(req->type == Pager_Request);
		} else {
			svm_start_kernel_request(req);
			assert(req->type == Kernel_Request);
		}

		/*
		 * If there are still pending kernel operations, then let them
		 * finish before doing anything else.
		 */
		if (req->state == Kernel_Pending)
			break;

		/*
		 * All pending kernel operations have finished.
		 */
_Kernel_Completed:
		assert(req->state == Kernel_Completed);

		/*
		 * Only kernel requests need to start pager operations.
		 */
		if (req->type == Pager_Request) {
			SJS_DEBUG(16);	/* req->last_state = 16; */
			req->state = All_Completed;
			goto _All_Completed;
		}

		/*
		 * We are processing a kernel request. If a pager request
		 * has arrived, then let the pager request take over, to
		 * avoid a circular wait starting and ending with the pager.
		 *
		 * If there is data, satisfy the kernel request before
		 * starting the pager request.
		 */
		assert(req->type == Kernel_Request);
		if (req->prequest_list) {
			if (IS_REAL_DATA(req->data)) {
				SJS_DEBUG(25);	/* req->last_state = 25; */
				req->state = All_Completed;
				goto _All_Completed;
			}

			/* this is false; read-access, needs write */
			assert(req->data == DATA_NONE);	/* ??? */
			SJS_DEBUG(17);	/* req->last_state = 17; */
			req->state = Nothing_Pending;
			continue;
		}

		/*
		 * It is now safe to start a pager operation.
		 */
		svm_start_pager_operation(req);

		/*
		 * If there are still pending pager operations, then let them
		 * finish before doing anything else.
		 */
		if (req->state == Pager_Pending) {
			break;
		}

		/*
		 * All pending operations have finished.
		 * Satisfy this request based on its type.
		 */
_All_Completed:
		assert(req->state == All_Completed);
		if (req->type == Kernel_Request) {
			svm_satisfy_kernel_request(req);
			svm_release_kernel_request(req);
		} else {
			svm_satisfy_pager_request(req);
			svm_release_pager_request(req);
		}

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
		/*
		 * Make sure that we have consumed any data that we obtained.
		 * If there is still real data, then a shadow object must have
		 * put it in parallel and a krequest should still remain.
		 */
		if (! IS_REAL_DATA(req->data))
			req->data = DATA_NONE;
		else if (svm_copy_call_push_page_to_kernel)
			assert(req->krequest_list);
		else
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
		{
			/*
			 * Make sure that we have consumed any data
			 *	that we obtained.
			 */
			assert(! IS_REAL_DATA(req->data));
			req->data = DATA_NONE;
		}
#if	MACH_ASSERT
		xmm_obj_lock(mobj);
		svm_check_dirty_invariant(req->mobj, atop(req->offset), req,
					  FALSE);
		xmm_obj_unlock(mobj);
#endif	/* MACH_ASSERT */

		/*
		 * Prepare to process a new request.
		 */
		SJS_DEBUG(18);	/* req->last_state = 18; */
		req->state = Nothing_Pending;
	}

	/*
	 * If we have processed all current requests, then release structure.
	 */
	req->active = AFALSE;
	if (req->prequest_list || req->krequest_list) {
		xmm_req_unlock(req);
	} else {
		svm_release_request(req); /* consumes lock */
	}

	/*
	 * Release our reference and check for termination
	 */
	xmm_obj_lock(mobj);
	if (--MOBJ->k_count == 0 &&
	    MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE) {
		xmm_terminate_pending--;
		xmm_svm_destroy(mobj); /* consumes lock */
	} else
		xmm_obj_unlock(mobj);
}

/*
 * The pager is xxx.
 */
void
svm_pager_request(
	xmm_obj_t		mobj,
	vm_offset_t		offset,
	memory_object_return_t	should_return,
	boolean_t		flush,
	vm_prot_t		lock,
	xmm_reply_t		reply)
{
	register unsigned int page = atop(offset);
	request_t req;
	prequest_t preq;
	vm_prot_t current_lock;
	char *m_bits;

	xmm_obj_lock(mobj);
	if (MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE ||
	    MOBJ->state == MOBJ_STATE_TERMINATED) {
		xmm_obj_unlock(mobj);
		return;
	}
	svm_extend_if_needed(mobj, page);
	xmm_obj_unlock(mobj);

	/*
	 * Create a pager request now if there's any chance that we need it,
	 * so that we don't block after having examined the state of the world.
	 */
	if (reply || should_return || flush || lock != VM_PROT_NO_CHANGE) {
		preq = (prequest_t) zalloc(svm_prequest_zone);
	} else {
		preq = PREQUEST_NULL;
	}

	/*
	 * Find or create current request.
	 * It must not be in an intermediate state.
	 */
	req = svm_get_request(mobj, offset);
	DD("svm_pager_request");
	assert(xmm_req_lock_held(req));
	assert(req->state == Nothing_Pending ||
	       req->state == Kernel_Pending ||
	       req->state == Pager_Pending ||
	       req->state == All_Completed);
	assert(req->state != Nothing_Pending ||
	       (req->prequest_list == PREQUEST_NULL &&
		req->krequest_list == KREQUEST_NULL));

	/*
	 * If we need a pager request, then queue it, and return if we are not
	 * the first. Otherwise, free it, and req if we created it as well.
	 */
	xmm_obj_lock(mobj);
	M_MAP_GET(MOBJ, page, m_bits);
	current_lock = M_GET_LOCK(m_bits);
	if (reply || should_return || flush ||
	    (lock != VM_PROT_NO_CHANGE && (lock & ~current_lock))) {
		/*
		 * Queue preq on req->prequest_list.
		 * Return now if we are not the first preq;
		 * In particular, do not change mobj lock or
		 * try to satisfy kernel request, since that
		 * would violate prequest ordering.
		 */
		assert(preq);
		preq->next = PREQUEST_NULL;
		if (reply)
			preq->reply_data = *reply;
		else
			xmm_reply_set(&preq->reply_data, XMM_EMPTY_REPLY, 0);
		preq->should_return = should_return;
		preq->flush = flush;
		preq->lock = lock;
		if (req->prequest_list) {
			prequest_t pr;
			for (pr = req->prequest_list; pr->next; pr = pr->next){
				continue;
			}
			pr->next = preq;
			xmm_obj_unlock(mobj);
			xmm_req_unlock(req);
			return;
		}
		req->prequest_list = preq;
		if (req->state == All_Completed) {
			xmm_obj_unlock(mobj);
			xmm_req_unlock(req);
			return;
		}
	} else {
		/*
		 * Free pager request. If we created request, then destroy
		 * it, set lock if appropriate, and return.
		 */
		if (preq) {
			zfree(svm_prequest_zone, (vm_offset_t) preq);
			preq = PREQUEST_NULL;
		}
		if (req->state == Pager_Pending)
			req->state == All_Completed;
		else {
			if (req->state == Nothing_Pending)
				svm_release_request(req); /* consumes lock */
			else {
				assert(req->state == All_Completed);
				xmm_req_unlock(req);
			}
			if (lock != VM_PROT_NO_CHANGE) {
				M_ASN_LOCK(m_bits, lock);
				assert(m_check_kernel_prots(mobj, page));
			}
			xmm_obj_unlock(mobj);
			return;
		}
	}

	/*
	 * If we are just decreasing lock, do so now,
	 * and satisfy kernel request if we can.
	 */
	if (lock != VM_PROT_NO_CHANGE &&
	    (lock & ~current_lock) == VM_PROT_NONE &&
	    preq == PREQUEST_NULL) {
		M_ASN_LOCK(m_bits, lock);
		assert(m_check_kernel_prots(mobj, page));

		/*
		 * Complete pending pager operation if possible.
		 * This saves us having to blow away the kernel
		 * request if we have to start a pager request.
		 */
		if (req->state == Pager_Pending &&
		    ((lock & ~req->lock) == VM_PROT_NONE)) {
			SJS_DEBUG(19);	/* req->last_state = 19; */
			req->state = All_Completed;
		}
	}
	xmm_obj_unlock(mobj);

	/*
	 * If there is a pending pager operation, and we have added a
	 * pager request, then blow away the pending pager operation
	 * to prevent deadlock. The pager operation will be restarted
	 * after our pager request (and any others that might exist)
	 * have been satisfied. Note that the request has no state
	 * that needs to be cleaned up, e.g., there is not data.
	 */
	if (req->state == Pager_Pending && req->prequest_list) {
		assert(req->data == DATA_NONE);
		SJS_DEBUG(20);	/* req->last_state = 20; */
		req->state = Nothing_Pending;
	}

	/*
	 * If we are now in a transitory state, start request processing.
	 */
	if (req->state == All_Completed || req->state == Nothing_Pending) {
		svm_process_request(req); /* consumes lock */
	} else
		xmm_req_unlock(req);
}

/*
 * The pager is supplying data.
 */
void
svm_pager_supply(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	vm_prot_t	lock,
	boolean_t	precious,
	xmm_reply_t	reply,
	kern_return_t	error)
{
	request_t req;
	register unsigned int page = atop(offset);
	vm_prot_t current_lock;
	char *m_bits;

	assert(data != DATA_NONE);

	xmm_obj_lock(mobj);
	if (MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE ||
	    MOBJ->state == MOBJ_STATE_TERMINATED) {
		xmm_obj_unlock(mobj);
		return;
	}
	svm_extend_if_needed(mobj, page);

	M_MAP_GET(MOBJ, page, m_bits);
	current_lock = M_GET_LOCK(m_bits);
	xmm_obj_unlock(mobj);

	/*
	 * XXX
	 * Don't yet handle reply.
	 */
	if (reply) {
		panic("svm_pager_supply: reply");
		(void) M_SUPPLY_COMPLETED(mobj, offset, PAGE_SIZE,
					  KERN_SUCCESS, 0, reply);
	}

	/*
	 * XXX
	 * Don't yet handle data supplies with increasing locks.
	 */
	if (lock != VM_PROT_NO_CHANGE && (lock & ~current_lock)) {
		panic("svm_pager_supply: increasing lock");
		lock = current_lock;
	}

	/*
	 * Find or create current request.
	 * It must not be in an intermediate state (xxx_Completed).
	 */
	req = svm_get_request(mobj, offset);
	DD("svm_pager_supply");
	assert(xmm_req_lock_held(req));
	assert(req->state == Nothing_Pending ||
	       req->state == Kernel_Pending ||
	       req->state == Pager_Pending);

	/*
	 * Update page lock.
	 */
	xmm_obj_lock(mobj);
	if (lock != VM_PROT_NO_CHANGE) {
		M_ASN_LOCK(m_bits, lock);
		assert(m_check_kernel_prots(mobj, atop(req->offset)));
	}

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	/*
	 * If pager supplies DATA_UNAVAILABLE and req->data holds real data,
	 *	then the data must have been pushed by a previous COPY_CALL.
	 */
	if (svm_copy_call_push_page_to_kernel &&
	    IS_REAL_DATA(req->data) && data == DATA_UNAVAILABLE) {
		precious = TRUE;
		data = req->data;
		req->data = DATA_NONE;
	}
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */

	/*
	 * Handle KERN_MEMORY_PRESENT.
	 *
	 * Memory is present if it is present in the request or if
	 * it might be present in the kernel.
	 */
	if (req->data || m_get_prot(mobj, page)) {
		/*
		 * If we believe data is present, then we must not need it.
		 */
		assert(req->state != Pager_Pending || ! req->needs_data);

		/*
		 * XXX
		 * Put code here. Should do reply and error_offset stuff.
		 */
		if (precious) {
			panic("svm_pager_call: KERN_MEMORY_PRESENT, precious");
		} else {
			printf("svm_pager_call: KERN_MEMORY_PRESENT\n");
		}
		if (IS_REAL_DATA(data)) {
			vm_map_copy_discard(data);
		}
		data = DATA_NONE;
	}

	/*
	 * XXX
	 * We don't yet handle unsolicited data_supplies.
	 */
	if (req->state != Pager_Pending) {
		xmm_obj_unlock(mobj);
		xmm_req_unlock(req);
		if (IS_REAL_DATA(data)) {
			if (precious) {
				/*
				 * XXX
				 * Needs more thought
				 */
				M_DATA_RETURN(mobj, req->offset, data,
					      PAGE_SIZE, FALSE, FALSE);
			} else {
				vm_map_copy_discard(data);
			}
		}
		return;
	}

	/*
	 * If this page is precious, then mark it so.
	 */
	if (precious) {
		XPR(XPR_XMM,
		    "svm_pager_supply: mobj=0x%x off=0x%x lock=%d\n",
		    (natural_t)mobj, offset, lock, 0, 0);
		M_SET_PRECIOUS(m_bits);
	}
	xmm_obj_unlock(mobj);

	/*
	 * We are waiting for data and don't already have it.
	 * Satisfy the request.
	 *
	 * XXX
	 * Should check lock as well.
	 */
	assert(req->state == Pager_Pending);
	assert(req->data == DATA_NONE);
	req->data = data;
	if (data == DATA_ERROR) {
		req->error = error;
	}
	SJS_DEBUG(21);	/* req->last_state = 21; */
	req->state = All_Completed;
	svm_process_request(req); /* consumes lock */
}

#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
/*
 * Supply a copy page to a kernel.
 */
void
svm_copy_supply(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_map_copy_t	data)
{
	xmm_obj_t mobj = KOBJ->mobj;
	unsigned int page = atop(offset);
	request_t req;
	krequest_t kreq, *kp;
	char *m_bits;
#if	MACH_ASSERT
	char *k_bits;
#endif	/* MACH_ASSERT */

	assert(svm_copy_call_push_page_to_kernel);
	assert(xmm_obj_lock_held(mobj));
	/*
	 * Since mobj lock will be released, then it is safe to take a
	 * reference count to the kobj in order to prevent its destruction.
	 */
	xmm_obj_lock(kobj);
	KOBJ->k_count++;
	xmm_obj_unlock(kobj);

	svm_extend_if_needed(mobj, page);
#if	MACH_ASSERT
	m_check_kernel_prots(mobj, page);
#endif	/* MACH_ASSERT */
	xmm_obj_unlock(mobj);

#if	MACH_ASSERT
	K_MAP_GET(KOBJ, page, k_bits);
#endif	/* MACH_ASSERT */
	assert(!K_GET_PRECIOUS(k_bits));

	/*
	 * Create a kernel request.
	 */
	kreq = (krequest_t) zalloc(svm_krequest_zone);
	kreq->kobj = kobj;
	kreq->needs_data = TRUE;
	kreq->wants_copy = FALSE;
	kreq->desired_access = VM_PROT_RX;

	/*
	 * Get current request.
	 */
	req = svm_get_request(mobj, offset);
	assert(req);
	assert(xmm_req_lock_held(req));

	/*
	 * If there is at least a krequest without any data in the request,
	 *	then put data in the request.
	 */
	if (req->krequest_list && req->data == DATA_NONE) {
		assert(req->krequest_list->needs_data);
		req->data = data;
		xmm_req_unlock(req);
		zfree(svm_krequest_zone, (vm_offset_t) kreq);
		xmm_obj_lock(kobj);
		KOBJ->k_count--;
		assert(KOBJ->k_count > 0 || !KOBJ->terminated);
		xmm_obj_unlock(kobj);
		return;
	}

	/*
	 * Put the kernel request at the end of the kreq list.
	 */
	req->data = data;
	for (kp = &req->krequest_list; *kp; kp = &(*kp)->next)
		continue;
	*kp = kreq;
	kreq->next = KREQUEST_NULL;

	xmm_obj_lock(mobj);
	M_MAP_GET(MOBJ, page, m_bits);
	XPR(XPR_XMM,
	    "svm_copy_supply: mobj=0x%x off=0x%x kobj=0x%x\n", 
	    (natural_t)mobj, offset, (natural_t)kobj, 0, 0);
	M_SET_PRECIOUS(m_bits);
	M_ASN_LOCK(m_bits, VM_PROT_WRITE);
	xmm_obj_unlock(mobj);

	/*
	 * Start processing request if not already active.
	 */
	if (req->active != AFALSE)
		xmm_req_unlock(req);
	else
		svm_process_request(req); /* consumes lock */

	xmm_obj_lock(kobj);
	if (--KOBJ->k_count == 0 && KOBJ->terminated)
		xmm_svm_terminate(kobj, FALSE); /* consumes lock */
	else
		xmm_obj_unlock(kobj);
}
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */

/*
 * The kernel is supplying data.
 */
void
svm_kernel_supply(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_map_copy_t	data,
	boolean_t	dirty,
	boolean_t	kernel_copy)
{
	xmm_obj_t mobj = KOBJ->mobj;
	request_t req;
	register unsigned int page = atop(offset);
	kern_return_t kr;
	char *k_bits;
	char *m_bits;
#if	MACH_ASSERT
	int starting_kernel_completed_count;
#endif	/* MACH_ASSERT */

	assert(IS_REAL_DATA(data));
	c_data_return_count++;

	/*
	 * Find current request.
	 */
	req = svm_lookup_request(mobj, offset);
	DD("svm_kernel_supply");
	xmm_list_unlock(mobj);
	if (req) {
		assert(xmm_req_lock_held(req));
		if (req->terminating) {
			xmm_req_unlock(req);
			req = REQUEST_NULL;
		} else {
#if	MACH_ASSERT
			starting_kernel_completed_count =
				req->kp_completed_count;
#endif	/* MACH_ASSERT */
		}
	}
	xmm_obj_lock(mobj);

#if	MACH_ASSERT
add_trace(kobj, offset, dirty, kernel_copy, req); /* XXX */
	if (req && kobj == req->kp_returning_failure) {
		panic("req=0x%x: returing_failure now returns!\n", req);
		req->kp_returning_failure = XMM_OBJ_NULL;
	}
	if (kobj == svm_last_returning_failure) {
		panic("svm_last_returing_failure, 0x%x vs. 0x%x\n",
		      svm_last_returning_failure_offset, offset);
		svm_last_returning_failure = 0;
	}

#endif	/* MACH_ASSERT */

	svm_check_dirty_invariant(mobj, page, req, TRUE);

	/*
	 * Extend the mobj state information if necessary.
	 * This should only be necessary if we don't already
	 * have a request, since creating the request will
	 * have required that the object was extended, and
	 * since extending the object now might block, resulting
	 * in a misordering between this kernel_supply and the
	 * following kernel_completed.
	 */
	assert(! svm_extend_needed(mobj, page) || ! req);
	svm_extend_if_needed(mobj, page);

	/*
	 * Update our notion of this page's usage if the kernel
	 * did not keep a copy.
	 */
	xmm_obj_lock(kobj);
	K_MAP_GET(KOBJ, page, k_bits);
	if (! kernel_copy) {
		K_CLR_PRECIOUS(k_bits);
		K_ASN_PROT(k_bits, VM_PROT_NONE);
		XPR(XPR_XMM,
		    "s_k_supply: !k_copy kobj %X req %X off %X K_PROT NONE\n",
				(integer_t)kobj, (integer_t)req,
				offset, 0, 0);
	}
	xmm_obj_unlock(kobj);

	svm_check_dirty_invariant(mobj, page, req, TRUE);

	/*
	 * Record the page as dirty if appropriate.
	 */
	if (dirty) {
		/*
		 * If the page is dirty, then there must not be
		 * any nonprecious kernels; furthermore, if
		 * this kernel did not keep a copy of the page,
		 * then no kernel can possibly have a copy.
		 */
		M_MAP_GET(MOBJ, page, m_bits);
		M_SET_DIRTY(m_bits);
		XPR(XPR_XMM,
		  "s_k_supply: dirty mobj %X kobj %X req %X off %X k_copy %d\n",
				(integer_t)mobj, (integer_t)kobj,
				(integer_t)req, offset, kernel_copy);
#if	NORMA_VM_PRECIOUS_RETURN_HACK
		if (kernel_copy) {
			xmm_obj_lock(kobj);
			K_SET_PRECIOUS(k_bits);
			xmm_obj_unlock(kobj);
		}
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
		svm_check_dirty_invariant(mobj, page, req, TRUE);
#if	MACH_ASSERT
		if (! kernel_copy) {
			assert(! m_get_prot(mobj, page));
		}
#endif	/*MACH_ASSERT*/
	} else {
		m_bits = (char *)0;
	}

	/*
	 * If there is no current request, or if it does not need or
	 * want data, or if it already has data, or if we are in a
	 * transitory state, then dispose of our data and return.
	 */
	if (! req ||
#if	COPY_CALL_PUSH_PAGE_TO_KERNEL
	    (svm_copy_call_push_page_to_kernel &&
	     req->type == Invalid_Request) ||
#endif	/* COPY_CALL_PUSH_PAGE_TO_KERNEL */
	    ! (req->needs_data || req->clean) ||
	    req->data ||
	    req->state == All_Completed || req->state == Nothing_Pending) {
		svm_check_dirty_invariant(mobj, page, req, TRUE);
		svm_dispose_of_data_properly(mobj, kobj, data, offset, req);
#if	MACH_ASSERT
		xmm_obj_lock(mobj);
		svm_check_dirty_invariant(mobj, page, REQUEST_NULL, FALSE);
		xmm_obj_unlock(mobj);
#endif	/* MACH_ASSERT */
		return;
	}

	/*
	 * See if we want/need this page
	 */
	if (req->kp_returning && kobj != req->kp_returning) {
		assert(!(K_GET_PENDING(k_bits)));
		svm_dispose_of_data_properly(mobj, kobj, data, offset, req);
		return;
	}
#if	MACH_ASSERT
	/*
	 * Check to see whether a kernel_completed has raced past us
	 * since we began processing. This could happen if we
	 * accidently blocked on something.
	 */
	assert(starting_kernel_completed_count == req->kp_completed_count);
#endif	/* MACH_ASSERT */

	/*
	 * If the request needs data, and we had it, then the request
	 * must be busy asking kernels (although possibly not us, yet).
	 */
	assert(req->state == Kernel_Pending);

	/*
	 * Insert data into the request, but don't directly satisfy it;
	 * that happens in svm_kernel_completed, which will be called
	 * by some kernel (although perhaps not us).
	 */
	req->data = data;
	svm_check_dirty_invariant(mobj, page, req, FALSE);

	/*
	 * Deal with the data.
	 * This is done here due to a deadlock that can occur on 
	 * a remote kernel: if the remote kernel is in a memory
	 * critical situation and this kernel is sending lock_requests,
	 * there will be outstanding requests here; if the remote
	 * kernel suspends the port so that no more requests are sent
	 * and then pages-out pages that requests coincidentally are
	 * outstanding for (i.e. the lock_request has not arrived at
	 * the remote kernel yet), then we are stuck waiting on
	 * lock completed messages to free the data.  Just dump it
	 * now and save the heart ache.
	 */
	/*
	 * Check the clean flag to see if the pager really wants
	 * the page back or not - it may be precious as a side
	 * effect of being dirty.
	 */
	if (! req->needs_data) {
		req->data = DATA_NONE;
		if (req->clean) {
			req->clean = FALSE;

			if (m_bits == (char *)0)
				M_MAP_GET(MOBJ, page, m_bits);
			M_CLR_DIRTY(m_bits);

			xmm_obj_unlock(mobj);
			xmm_req_unlock(req);

			/*
			 * reset the clean flag to prevent subsequent
			 * pageouts from trying to write the data page
			 * again before we are finished with this one.
			 */
			kr = M_DATA_RETURN(mobj, offset, data, PAGE_SIZE, 
					   dirty, FALSE);
			assert(kr == KERN_SUCCESS);
		} else {
			svm_dispose_of_data_properly(mobj, kobj, data,
						     offset, req);
		}

	} else {
		xmm_obj_unlock(mobj);
		xmm_req_unlock(req);
	}
}

void
svm_request_cleanup(
	xmm_obj_t	mobj)
{
	request_t req;
	krequest_t kr, kr_next;
	prequest_t pr, pr_next;

	assert(MOBJ->state == MOBJ_STATE_TERMINATED ||
	       MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE);
	xmm_list_lock(mobj);

	while ((req = (request_t) dequeue_head(&MOBJ->request_list)) !=
	       REQUEST_NULL) {
		xmm_req_lock(req);
		assert(req->active == AFALSE);
		assert(req->kp_count == 0);
		MOBJ->request_count--;
		for (kr = req->krequest_list; kr; kr = kr_next) {
			kr_next = kr->next;
			zfree(svm_krequest_zone, (vm_offset_t) kr);
		}
		for (pr = req->prequest_list; pr; pr = pr_next) {
			pr_next = pr->next;
			zfree(svm_prequest_zone, (vm_offset_t) pr);
		}
		xmm_req_unlock(req);
		zfree(svm_request_zone, (vm_offset_t) req);
	}
	assert(MOBJ->request_count == 0);
	MOBJ->last_found = REQUEST_NULL;
	xmm_list_unlock(mobj);
}

void
svm_request_init(void)
{
	svm_request_zone =
	    zinit(sizeof(struct request), (vm_size_t) (512*1024),
		  sizeof(struct request), "svm.request");
	svm_krequest_zone =
	    zinit(sizeof(struct krequest), (vm_size_t) (512*1024),
		  sizeof(struct krequest), "svm.krequest");
	svm_prequest_zone =
	    zinit(sizeof(struct prequest), (vm_size_t) (512*1024),
		  sizeof(struct prequest), "svm.prequest");
	svm_gather_zone =
	    zinit(sizeof(struct gather), (vm_size_t) 512*1024,
		  sizeof(struct gather), "svm.gather");

	zone_change(svm_request_zone, Z_COLLECT, TRUE);
	zone_change(svm_krequest_zone, Z_COLLECT, TRUE);
	zone_change(svm_prequest_zone, Z_COLLECT, TRUE);
}

#if	MACH_KDB
#include <ddb/db_output.h>

#define	printf	kdbprintf

#define	BOOL(b)		((b) ? "" : "!")

/*
 * Forward.
 */
char		*svm_request_state_name(
				int		state);

char		*svm_request_type_name(
				int		type);

void		svm_prot_print(
				vm_prot_t	prot);

void		r_svm_print(
				request_t	req);

void		kr_svm_print(
				krequest_t	kreq);

void		pr_svm_print(
				prequest_t	preq);

char *
svm_request_state_name(
	int	state)
{
	switch ((int) state) {
		case Nothing_Pending:	return "Nothing_Pending";
		case Kernel_Pending:	return "Kernel_Pending";
		case Kernel_Completed:	return "Kernel_Completed";
		case Pager_Pending:	return "Pager_Pending";
		case All_Completed:	return "All_Completed";
	}
	return "<invalid>";
}

char *
svm_request_type_name(
	int	type)
{
	switch ((int) type) {
		case Kernel_Request:	return "Kernel_Request";
		case Pager_Request:	return "Pager_Request";
	}
	return "<invalid>";
}

void
svm_prot_print(
	vm_prot_t	prot)
{
	if (prot == VM_PROT_NO_CHANGE) {
		printf("<no change>");
	} else {
		printf("0x%x:%c%c%c",
		       prot,
		       ((prot & VM_PROT_READ)    ? 'r' : '-'),
		       ((prot & VM_PROT_WRITE)   ? 'w' : '-'),
		       ((prot & VM_PROT_EXECUTE) ? 'x' : '-'));
	}
}

/*
 *	Routine:	r_svm_print
 *	Purpose:
 *		Pretty-print a request.
 */

void
r_svm_print(
	request_t	req)
{
	int i;

	indent += 2;

	iprintf("mobj=0x%x, offset=0x%x, kreq_list=0x%x, preq_list=0x%x\n",
		req->mobj,
		req->offset,
		req->krequest_list,
		req->prequest_list);
	iprintf("next_chain{next=0x%x, prev=0x%x}\n",
		req->next_chain.next,
		req->next_chain.prev);
#if MACH_ASSERT
	iprintf("active=0x%x, %sterminating, state=%s, type=%s\n",
		(int)req->active,
		BOOL(req->terminating),
		svm_request_state_name(req->state),
		svm_request_type_name(req->type));
#else /*MACH_ASSERT*/
	iprintf("%sactive, %sterminating, state=%s, type=%s\n",
		BOOL(req->active),
		BOOL(req->terminating),
		svm_request_state_name(req->state),
		svm_request_type_name(req->type));
#endif /*MACH_ASSERT*/

	iprintf("%sneeds_data, %sclean, lock=",
		BOOL(req->needs_data),
		BOOL(req->clean));
	svm_prot_print(req->lock);
	printf(", data=");

	if (req->data == DATA_NONE) {
		printf("none");
	} else if (req->data == DATA_UNAVAILABLE) {
		printf("unavailable");
	} else if (req->data == DATA_ERROR) {
		printf("error(%d/0x%x)", req->error, req->error);
	} else {
		printf("0x%x", req->data);
	}
	printf("\n");

	iprintf("kp_count=%d, kp_returning=0x%x, surething=%d\n",
		req->kp_count,
		req->kp_returning,
		req->kp_returning_surething);

#if	MACH_ASSERT
	iprintf("failure=0x%x, kp_completed_count=%d, last_state = %d\n",
		req->kp_returning_failure,
		req->kp_completed_count,
		req->last_state);
#endif	/* MACH_ASSERT */
	indent -=2;
}

/*
 *	Routine:	kr_svm_print
 *	Purpose:
 *		Pretty-print a krequest.
 */

void
kr_svm_print(
	krequest_t	kreq)
{
	indent += 2;

	iprintf("kobj=0x%x,", kreq->kobj);
	printf(" %sneeds_data, %swants_copy, desired=",
		BOOL(kreq->needs_data),
		BOOL(kreq->wants_copy));
	svm_prot_print(kreq->desired_access);
	printf("\n");
	printf("next=0x%x\n", kreq->next);

	indent -=2;
}

/*
 *	Routine:	g_svm_print
 *	Purpose:
 *		Pretty-print a gather structure
 */
void
g_svm_print(
	gather_t	g)
{
	if (g == GATHER_NULL)
		return;
	iprintf("gather: begin offset 0x%x, end offset 0x%x,",
	       g->begin_offset,
	       g->offset);
	printf(" %sshould_return, %sflush\n",
	       BOOL(g->should_return),
	       BOOL(g->flush));
	iprintf("gather:lock=");
	svm_prot_print(g->lock);
	printf(" reply %x\n", g->reply);
}

/*
 *	Routine:	pr_svm_print
 *	Purpose:
 *		Pretty-print a prequest.
 */

void
pr_svm_print(
	prequest_t	preq)
{
	indent += 2;

	switch (preq->should_return) {
		case MEMORY_OBJECT_RETURN_NONE:
		iprintf("return_none");
		break;

		case MEMORY_OBJECT_RETURN_DIRTY:
		iprintf("return_dirty");
		break;

		case MEMORY_OBJECT_RETURN_ALL:
		iprintf("return_all");
		break;

		case MEMORY_OBJECT_RETURN_ALWAYS:
		iprintf("return_always");
		break;

		default:
		iprintf("return=<%d>?",
			preq->should_return);
	}

	iprintf(", %sflush, lock=",
		BOOL(preq->flush));
	svm_prot_print(preq->lock);
	printf(", next=0x%x\n",
	       preq->next);

	xmm_reply_print((db_addr_t)&preq->reply_data);

	indent -=2;
}

#undef	printf
int dd = 0;

void
_DD(
	char		*name,
	request_t	req)
{
	if (dd) {
		if (req) {
			printf("%s: req=0x%x<%s,%s,%s>\n",
			       name,
			       req,
			       (req->active ? "active" : "inactive"),
			       svm_request_type_name(req->type),
			       svm_request_state_name(req->state));
		} else {
			printf("%s: req=0\n", name);
		}
	}
}

#if	TRACE_RECORD
#define	TRMAX	150
struct trace_rec {
	int	val[8];
} trace_rec[TRMAX];
int tr_index = 0;
int tr_count = 0;

add_trace(kobj, offset, dirty, kernel_copy, req)
{
	struct trace_rec *tr = &trace_rec[tr_index];

	while (tr->val[0] != 0) {
		tr++;
		if (++tr_index >= TRMAX) {
			tr_index = 0;
			tr = &trace_rec[0];
		}
	}
	if (tr >= &trace_rec[TRMAX]) {
		printf("Trace rec overflow!\n");
		assert(0);
		return;
	}

	tr->val[0] = kobj;
	tr->val[1] = offset;
	tr->val[2] = dirty;
	tr->val[3] = kernel_copy;
	tr->val[5] = req;
	tr->val[4] = 	tr->val[6] = 0;
	tr->val[7] = tr_count++;
}

sub_trace(kobj, offset)
{
	struct trace_rec *tr = &trace_rec[0];

	while (tr->val[0] != kobj && tr < &trace_rec[TRMAX])
		tr++;
	if (tr >= &trace_rec[TRMAX]) {
		printf ("sub_trace: kobj 0x%x off 0x%x not found\n", 
			kobj, offset);
		assert(0);
		return;
	}
	tr->val[6] = kobj;
	tr->val[0] = 0;
}

find_trace(kobj, offset)
{
	struct trace_rec *tr = &trace_rec[0];
	int i;
	boolean_t all = (offset == -1);

	printf ("KOBJ: 0x%x\n", kobj);
	for (i = 0; i < TRMAX; i++, tr++) {
		if (kobj == tr->val[0] || (all && kobj == tr->val[6]))
			if (offset == tr->val[1] || all)
				printf("   addr 0x%x off %x %sdirty %skernel_copy req %x kobj %x\n",
				       tr, tr->val[1],
				       tr->val[2] ? "" : "!",
				       tr->val[3] ? "" : "!",
				       tr->val[5], tr->val[6]);
	}
}
#endif	/* TRACE_RECORD */

#endif	/* MACH_KDB */
