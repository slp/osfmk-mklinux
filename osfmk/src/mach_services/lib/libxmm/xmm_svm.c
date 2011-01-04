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
 * MkLinux
 */
/*
 *	File:	norma/xmm_svm.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer providing consistent shared virtual memory.
 */

#ifdef	MACH_KERNEL
#include <norma/xmm_obj.h>
#include <mach/vm_param.h>
#else	/* MACH_KERNEL */
#include "xmm_obj.h"
#endif	/* MACH_KERNEL */

#ifdef	MACH_KERNEL
#define	vm_page_size	PAGE_SIZE
#endif	/* MACH_KERNEL */

int xmm_svm_debug = 0;

typedef struct request *	request_t;

#define	REQUEST_NULL		((request_t) 0)

#define	MOBJ_STATE_UNCALLED	0
#define	MOBJ_STATE_CALLED	1
#define	MOBJ_STATE_READY	2

#define	DATA_NONE		((vm_offset_t) 0)
#define	DATA_UNAVAILABLE	((vm_offset_t) 1)
#define	DATA_ERROR		((vm_offset_t) 2)

#ifdef	MACH_KERNEL
/* XXX kalloc/malloc definition stuff */
/*char *kalloc();*/
#endif	/* MACH_KERNEL */

#define	K			((struct kobj *) k)

/*
 * lock[] is set when pager gives us a message.
 * prot[] is set when we send message to kernels;
 * it should simply reflect max of all kobj->prot.
 */
struct mobj {
	struct xmm_obj	obj;
	xmm_obj_t	kobj_list;
	int		state;
	int		num_pages;
	request_t	request;
	vm_prot_t	*prot;			/* kernel access */
	vm_prot_t	*lock;			/* lock by pager */
	boolean_t	may_cache;
};

/*
 * XXX some of these fields could be aliased to save space
 * XXX eg: needs_data,should_clean; lock_value,desired_access
 *
 * XXX should probably add ref counts to kobjs....
 */
struct request {
	xmm_obj_t	kobj;
	int		m_count;		/* -> m_yield_count */
	int		k_count;		/* -> m_yield_count */
	boolean_t	is_kernel;
	boolean_t	needs_data;		/* ours alone */
	boolean_t	should_clean;		/* same as needs_data? */
	boolean_t	should_flush;
	vm_prot_t	desired_access;
	vm_prot_t	lock_value;
	vm_offset_t	offset;			/* -> page */
	request_t	next_eq;
	request_t	next_ne;
};

struct kobj {
	struct xmm_obj	obj;
	vm_prot_t	*prot;
	xmm_obj_t	next;
};

kern_return_t m_svm_init();
kern_return_t m_svm_terminate();
kern_return_t m_svm_data_request();
kern_return_t m_svm_data_unlock();
kern_return_t m_svm_data_write();
kern_return_t m_svm_lock_completed();
kern_return_t m_svm_data_return();
kern_return_t k_svm_data_provided();
kern_return_t k_svm_data_unavailable();
kern_return_t k_svm_lock_request();
kern_return_t k_svm_data_error();
kern_return_t k_svm_set_attributes();
kern_return_t k_svm_destroy();
kern_return_t k_svm_data_supply();

xmm_decl(svm_mclass,
	/* m_init		*/	m_svm_init,
	/* m_terminate		*/	m_svm_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_svm_data_request,
	/* m_data_unlock	*/	m_svm_data_unlock,
	/* m_data_write		*/	m_svm_data_write,
	/* m_lock_completed	*/	m_svm_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_svm_data_provided,
	/* k_data_unavailable	*/	k_svm_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_svm_lock_request,
	/* k_data_error		*/	k_svm_data_error,
	/* k_set_attributes	*/	k_svm_set_attributes,
	/* k_destroy		*/	k_svm_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"svm.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(svm_kclass,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_svm_data_provided,
	/* k_data_unavailable	*/	k_svm_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_svm_lock_request,
	/* k_data_error		*/	k_svm_data_error,
	/* k_set_attributes	*/	k_svm_set_attributes,
	/* k_destroy		*/	k_svm_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"svm.k",
	/* size			*/	sizeof(struct kobj)
);

#define	Mcheck { if (mobj->class != &svm_mclass) {\
	panic("mcheck:%s.%d\n", __FILE__, __LINE__); }}

#define	Kcheck { if (kobj->class != &svm_kclass) {\
	panic("kcheck:%s.%d\n", __FILE__, __LINE__); }}

/* XXX should make these names more unique */
boolean_t	add_request();
request_t	lookup_request();
void		satisfy_request();
void		satisfy_kernel_request();
void		satisfy_pager_request();

zone_t		xmm_svm_request_zone;

int C_mobj_prot = 0;
int C_mobj_lock = 0;
int C_user_prot = 0;

/* XXX should be implemented by kalloc.c */
/* XXX should kalloc have asm help for round-to-power-of-two? */
krealloc(old_buf_p, old_size, new_size, counter)
	char **old_buf_p;
	int old_size;
	int new_size;
	int *counter;
{
	char *new_buf;

	new_buf = (char *) kalloc(new_size);
	if (new_buf == (char *) 0) {
		panic("krealloc");
	}
	if (old_size > 0) {
		bcopy(*old_buf_p, new_buf, old_size);
		kfree(*old_buf_p, old_size);
	}
	*counter += (new_size - old_size);
	*old_buf_p = new_buf;
}

kern_return_t
m_svm_extend(mobj, new_num_pages)
	xmm_obj_t mobj;
	int new_num_pages;
{
	xmm_obj_t kobj;

	int page, i, old_num_pages = MOBJ->num_pages;

	for (i = 4; i < new_num_pages; i += i) {
		continue;
	}
	new_num_pages = i;
	MOBJ->num_pages = new_num_pages;
/*	assert(new_num_pages > old_num_pages);*/
	krealloc((char **) &MOBJ->prot,
		 old_num_pages * sizeof(vm_prot_t),
		 new_num_pages * sizeof(vm_prot_t),
		 &C_mobj_prot);
	krealloc((char **) &MOBJ->lock,
		 old_num_pages * sizeof(vm_prot_t),
		 new_num_pages * sizeof(vm_prot_t),
		 &C_mobj_lock);
	for (kobj = MOBJ->kobj_list; kobj; kobj = KOBJ->next) {
		krealloc((char **) &KOBJ->prot,
			 old_num_pages * sizeof(vm_prot_t),
			 new_num_pages * sizeof(vm_prot_t),
			 &C_user_prot);
	}
	for (page = old_num_pages; page < new_num_pages; page++) {
		MOBJ->prot[page] = VM_PROT_NONE;
		MOBJ->lock[page] = VM_PROT_ALL;
		for (kobj = MOBJ->kobj_list; kobj; kobj = KOBJ->next) {
			KOBJ->prot[page] = VM_PROT_NONE;
		}
	}
	return KERN_SUCCESS;
}

kern_return_t
xmm_svm_create(old_mobj, new_mobj)
	xmm_obj_t old_mobj;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&svm_mclass, old_mobj, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->num_pages = 0;
	MOBJ->kobj_list = XMM_OBJ_NULL;
	MOBJ->prot = (vm_prot_t *) 0;
	MOBJ->lock = (vm_prot_t *) 0;
	MOBJ->request = REQUEST_NULL;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

m_svm_init(mobj, k_kobj, memory_object_name, memory_object_page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t memory_object_page_size;
{
	xmm_obj_t kobj;

#if     lint
	memory_object_name++;
#endif  /* lint */
	if (memory_object_page_size != vm_page_size) {
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}

	xmm_obj_allocate(&svm_kclass, XMM_OBJ_NULL, &kobj);
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;

	if (MOBJ->num_pages > 0) {
		KOBJ->prot = (vm_prot_t *) kalloc(MOBJ->num_pages
						  * sizeof(vm_prot_t));
#if 9
		C_user_prot += (MOBJ->num_pages * sizeof(vm_prot_t));
#endif
		if (! KOBJ->prot) {
			panic("m_svm_init");
		}
		memset(KOBJ->prot, 0, MOBJ->num_pages * sizeof(vm_prot_t));
	}

	KOBJ->next = MOBJ->kobj_list;
	MOBJ->kobj_list = kobj;

	if (MOBJ->state == MOBJ_STATE_READY) {
		K_SET_ATTRIBUTES(kobj, TRUE, MOBJ->may_cache,
				 MEMORY_OBJECT_COPY_NONE);
	} else if (MOBJ->state == MOBJ_STATE_UNCALLED) {
		MOBJ->state = MOBJ_STATE_CALLED;
		M_INIT(mobj, mobj, MACH_PORT_NULL, vm_page_size);
	}

	return KERN_SUCCESS;
}

m_svm_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
	xmm_obj_t kobj_terminated, *kp;

	/*
	 * Free kobj's resources and remove from list.
	 * Return if there are more kobjs.
	 */
	if (MOBJ->num_pages > 0) {
		kfree(KOBJ->prot, MOBJ->num_pages * sizeof(vm_prot_t));
#if 9
		C_user_prot -= (MOBJ->num_pages * sizeof(vm_prot_t));
#endif
	}
	kobj_terminated = kobj;
	for (kp = &MOBJ->kobj_list; kobj = *kp; kp = &KOBJ->next) {
		if (kobj == kobj_terminated) {
			*kp = KOBJ->next;
			break;
		}
	}
	xmm_obj_deallocate(kobj_terminated);
	if (MOBJ->kobj_list != XMM_OBJ_NULL) {
		return KERN_SUCCESS;
	}

	/*
	 * There are no more kobjs. Free mobj's resources.
	 * XXX need locking here, and automatic deallocate of mobj?
	 */
	M_TERMINATE(mobj, mobj, memory_object_name);
	if (MOBJ->num_pages > 0) {
		kfree(MOBJ->prot, MOBJ->num_pages * sizeof(vm_prot_t));
		kfree(MOBJ->lock, MOBJ->num_pages * sizeof(vm_prot_t));
#if 9
		C_mobj_prot -= (MOBJ->num_pages * sizeof(vm_prot_t));
		C_mobj_lock -= (MOBJ->num_pages * sizeof(vm_prot_t));
#endif
	}
	xmm_obj_deallocate(mobj);
	return KERN_SUCCESS;
}

m_svm_request(mobj, r)
	xmm_obj_t mobj;
	request_t r;
{
Mcheck
	if((unsigned long)atop(r->offset) >= MOBJ->num_pages) {
#if 0
		K_DATA_ERROR(r->kobj, r->offset, vm_page_size, KERN_FAILURE);
		return KERN_FAILURE;
#else
		m_svm_extend(mobj, atop(r->offset) + 1);
#endif
	}
	if (add_request(mobj, r)) {
		return process_kernel_request(mobj, r);
	}
	return KERN_SUCCESS;
}

m_svm_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	request_t r;

Mcheck
Kcheck
	if (length != vm_page_size) {
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);
		return KERN_FAILURE;
	}
	r = (request_t) zalloc(xmm_svm_request_zone);
	r->kobj = kobj;
	r->m_count = 0;
	r->k_count = 0;
	r->is_kernel = TRUE;
	r->needs_data = TRUE;
	r->should_clean = FALSE;
	r->should_flush = FALSE;
	r->desired_access = desired_access;
	r->offset = offset;
	r->next_ne = 0;
	r->next_eq = 0;
	return m_svm_request(mobj, r);
}

m_svm_data_unlock(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	request_t r;

Mcheck
Kcheck
	if (length != vm_page_size) {
		K_DATA_ERROR(kobj, offset, length, KERN_FAILURE);
		return KERN_FAILURE;
	}
	r = (request_t) zalloc(xmm_svm_request_zone);
	r->kobj = kobj;
	r->m_count = 0;
	r->k_count = 0;
	r->is_kernel = TRUE;
	r->needs_data = FALSE;
	r->should_clean = FALSE;
	r->should_flush = FALSE;
	r->desired_access = desired_access;
	r->offset = offset;
	r->next_ne = 0;
	r->next_eq = 0;
	return m_svm_request(mobj, r);
}

m_svm_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	int length;
{
Mcheck
Kcheck
	/* make sanity checks */
	M_DATA_WRITE(mobj, mobj, offset, data, length);
	return KERN_SUCCESS;
}

m_svm_lock_completed(mobj, kobj, offset, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	request_t r;

Mcheck
Kcheck
#if     lint
	length++;
#endif  /* lint */
	/* make sanity checks */
	r = lookup_request(mobj, offset);
	if (r == REQUEST_NULL) {
		printf("how strange, lock_completed for nothing!\n");
		return KERN_FAILURE;
	}
	if (--r->k_count == 0 && r->m_count == 0) {
		satisfy_request(mobj, r, DATA_NONE);
	}
	return KERN_SUCCESS;
}

k_svm_data_provided(mobj, offset, data, length, lock_value)
	xmm_obj_t mobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	request_t r;

Mcheck
#if     lint
	length++;
#endif  /* lint */
	/* make sanity checks */

	/*
	 * XXX what do we do if this restricts access???
	 * XXX should probably do whatever lock_request does.
	 */
	if (lock_value & ~MOBJ->lock[atop(offset)]) {
		printf("XXX data_provided: lock=0x%x -> 0x%x\n",
		       MOBJ->lock[atop(offset)], lock_value);
	}

	MOBJ->lock[atop(offset)] = lock_value;

	r = lookup_request(mobj, offset);
	if (r == REQUEST_NULL) {
		printf("how strange, data_provided for nothing!\n");
		return KERN_FAILURE;
	}
	if (--r->m_count == 0 && r->k_count == 0) {
		satisfy_request(mobj, r, data);
	} else {
		printf("how strange, data provided but still other things\n");
		return KERN_FAILURE;
	}
	return KERN_SUCCESS;
}

k_svm_data_unavailable(mobj, offset, length)
	xmm_obj_t mobj;
	vm_offset_t offset;
	vm_size_t length;
{
	request_t r;

Mcheck
#if     lint
	length++;
#endif  /* lint */
	/* make sanity checks */

	/* XXX is this absolutely correct? */
	MOBJ->lock[atop(offset)] = VM_PROT_NONE;

	r = lookup_request(mobj, offset);
	if (r == REQUEST_NULL) {
		printf("how strange, data_unavailable for nothing!\n");
		return KERN_FAILURE;
	}
	if (--r->m_count == 0 && r->k_count == 0) {
		satisfy_request(mobj, r, DATA_UNAVAILABLE);
	}
	return KERN_SUCCESS;
}

k_svm_lock_request(mobj, offset, length, should_clean, should_flush,
		   lock_value, robj)
	xmm_obj_t mobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t robj;
{
	request_t r, r0;

Mcheck
if(xmm_svm_debug)printf("k_svm_lock_request!\n");

	if (length != vm_page_size) {
if(xmm_svm_debug)printf("kern . failure %d %x %x\n", __LINE__, length, vm_page_size);
		if (length < vm_page_size) {
			length = vm_page_size;
			if(xmm_svm_debug)printf("... Saved!\n");
		} else {
			return KERN_FAILURE;
		}
	}
	if((unsigned long)atop(offset) >= MOBJ->num_pages) {
		m_svm_extend(mobj, atop(offset) + 1);
	}

	r0 = lookup_request(mobj, offset);

	/*
	 * If we are not increasing lock value, flushing, or cleaning,
	 * then we set simply set lock value, without creating a request.
	 * However, we do need to see whether we can satisfy a kernel request.
	 */
	if (! (lock_value & ~MOBJ->lock[atop(offset)])
	    && ! should_clean && ! should_flush) {
		MOBJ->lock[atop(offset)] = lock_value;
		if (r0 && r0->is_kernel && !(lock_value & r0->desired_access)
		    && r0->m_count > 0 && --r0->m_count == 0
		    && r0->k_count == 0) {
			satisfy_kernel_request(mobj, r0, DATA_NONE);
		}
		return KERN_SUCCESS;
	}

	/*
	 * We need to submit a request. Create the request.
	 */
if(xmm_svm_debug)printf("** lock_request: submitting request\n");
	r = (request_t) zalloc(xmm_svm_request_zone);
	r->kobj = robj;		/* so should we rename to r->xobj? */
	r->m_count = 0;
	r->k_count = 0;
	r->is_kernel = FALSE;
	r->needs_data = FALSE;
	r->should_clean = should_clean;
	r->should_flush = should_flush;
	r->lock_value = lock_value;
	r->offset = offset;
	r->next_ne = 0;
	r->next_eq = 0;

	/*
	 * If there are no requests, then add new request and process it.
	 */
	if (! r0) {
if(xmm_svm_debug)printf("- no reqs\n");
		(void) add_request(mobj, r); /* will be true */
		(void) process_pager_request(mobj, r);
		return KERN_SUCCESS;
	}

	/*
	 * If first request is pager request, then place new request
	 * after all pager requests, but before any kernel requests.
	 */
	if (! r0->is_kernel) {
if(xmm_svm_debug)printf("- only pager reqs\n");
		while (r0->next_eq && ! r0->next_eq->is_kernel) {
			r0 = r0->next_eq;
		}
		r->next_eq = r0->next_eq;
		r0->next_eq = r;
		return KERN_SUCCESS;
	}

	/*
	 * First request is a kernel request.
	 * To avoid deadlock, pager requests have priority.
	 * Thus, if first request is a kernel, then all are.
	 * In this case, we place new request at the top
	 * (before all kernel requests) and process it immediately.
	 *
	 * XXXO
	 * This is slightly pessimal because we just ignore any
	 * request that the kernel request made to the other kernels.
	 */
	if (r0->is_kernel) {
		request_t *rp;
		for (rp = &MOBJ->request; r0 = *rp; rp = &r0->next_ne) {
			if (r0->offset == offset) {
				break;
			}
		}
		if (! r0) {
			printf("oops, oh my\n");
			return KERN_FAILURE;
		}
		*rp = r;
		r->next_ne = r0->next_ne;
		r->next_eq = r0;
		if (r0->m_count) {
			printf("This could get confusing\n");
		}
		r->m_count = r0->m_count;
		r->k_count = r0->k_count;
		r0->m_count = 0;
		r0->k_count = 0;	/* XXXO */
		(void) process_pager_request(mobj, r);
		return KERN_SUCCESS;
	}
	return KERN_SUCCESS;
}

k_svm_data_error(mobj, offset, length, error_value)
	xmm_obj_t mobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	request_t r;

Mcheck
#if     lint
	length++;
#endif  /* lint */
	/* make sanity checks */

	/* XXX certainly questionable! */
	MOBJ->lock[atop(offset)] = VM_PROT_NONE;

	r = lookup_request(mobj, offset);
	if (r == REQUEST_NULL) {
		printf("how strange, data_unavailable for nothing!\n");
		return KERN_FAILURE;
	}
	if (--r->m_count == 0 && r->k_count == 0) {
		satisfy_request(mobj, r, DATA_ERROR);
	}
	/* XXX should keep and return error_value */
	printf("k_svm_data_error: Gack(%d)!\n", error_value);
	return KERN_SUCCESS;
}

k_svm_set_attributes(mobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t mobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	xmm_obj_t kobj;

Mcheck
#if     lint
	copy_strategy++;
#endif  /* lint */
	MOBJ->may_cache = may_cache;
	if (object_ready) {
		MOBJ->state = MOBJ_STATE_READY;
	} else if (MOBJ->state == MOBJ_STATE_READY) {
		/* XXX What should we do here? */
		printf("k_svm_set_attributes: ready -> not ready ?\n");
	}
	for (kobj = MOBJ->kobj_list; kobj; kobj = KOBJ->next) {
		K_SET_ATTRIBUTES(kobj, object_ready, may_cache,
				 MEMORY_OBJECT_COPY_NONE);
	}
	return KERN_SUCCESS;
}

k_svm_destroy(mobj, reason)
	xmm_obj_t mobj;
	kern_return_t reason;
{
Mcheck
#if     lint
	mobj++;
	reason++;
#endif  /* lint */
	printf("k_svm_destroy: Gack!\n");
}

/*
 * Place request at end of appropriate queue.
 * Return TRUE if first request in queue for this page.
 */
boolean_t
add_request(mobj, r0)
	xmm_obj_t mobj;
	request_t r0;
{
	request_t r, *rp;

Mcheck
	for (rp = &MOBJ->request; r = *rp; rp = &r->next_ne) {
if(xmm_svm_debug)printf("add_request: 0x%x 0x%x\n", r, r0);
		if (r->offset == r0->offset) {
			for (; r->next_eq; r = r->next_eq) {
				continue;
			}
			r->next_eq = r0;
			return FALSE;
		}
	}
	r0->next_ne = MOBJ->request;
	MOBJ->request = r0;
	return TRUE;
}

/*
 * Look for first request for given offset.
 * If we find such a request, move it to front of list
 * since we expect to remove it soon.
 */
request_t
lookup_request(mobj, offset)
	xmm_obj_t mobj;
	vm_offset_t offset;
{
	request_t r, *rp;

Mcheck
	for (rp = &MOBJ->request; r = *rp; rp = &r->next_ne) {
		if (r->offset == offset) {
			*rp = r->next_ne;
			r->next_ne = MOBJ->request;
			MOBJ->request = r;
			return r;
		}
	}
	return REQUEST_NULL;
}

/*
 * Remove first request for given offset.
 * Return next request for same offset, if any.
 */
request_t
remove_request(mobj, offset)
	xmm_obj_t mobj;
	vm_offset_t offset;
{
	request_t r, *rp;

Mcheck
	for (rp = &MOBJ->request; r = *rp; rp = &r->next_ne) {
		if (r->offset == offset) {
			if (r->next_eq) {
				r = r->next_eq;
				r->next_ne = (*rp)->next_ne;
				*rp = r;
				return r;
			} else {
				*rp = r->next_ne;
				return REQUEST_NULL;
			}
		}
	}
	printf("remove_request: request not found!\n");
	return REQUEST_NULL;
}

/*
 * All the real work takes place in process_request and satisfy_request.
 *
 * Process_request takes a request for a page that does not already have
 * outstanding requests and generates the appropriate K/M_ requests.
 * If, after generating all apropriate K/M_ requests, there are no outstanding
 * K/M_ requests (either because no K/M_ requests were required, or because
 * they were all satisfied by the time we check), we call satisfy_request.
 *
 * Satisfy_request takes a request for a page that has had its last outstanding
 * K/M_ request satisfied, and sends the appropriate K/M_ reply to the
 * entity (kernel or memory manager) that generated the request. If more
 * requests follow the request being satisfied, satisfy_request calls
 * process_request on the first such request.
 */

/*
 * This routine does not worry about lock[page]; satisfy_request does.
 */
process_kernel_request(mobj, r)
	xmm_obj_t mobj;
	request_t r;
{
	int page;
	xmm_obj_t kobj, k;

Mcheck
	page = atop(r->offset);
	kobj = r->kobj;

	/*
	 * If requesting kernel wants to write, we must flush and lock
	 * all kernels (either readers or a single writer).
	 */
	if (r->desired_access & VM_PROT_WRITE) {
		boolean_t writing = !! (MOBJ->prot[page] & VM_PROT_WRITE);
		MOBJ->prot[page] = VM_PROT_NONE;
		r->k_count++;
		for (k = MOBJ->kobj_list; k; k = K->next) {
			if (k == kobj || K->prot[page] == VM_PROT_NONE) {
				continue;
			}
			r->k_count++;
			K->prot[page] = VM_PROT_NONE;
			K_LOCK_REQUEST(k, r->offset, vm_page_size,
				       writing, TRUE, VM_PROT_ALL, mobj);
			if (writing) {
				break;
			}
		}
		if (--r->k_count == 0 && r->m_count == 0) {
			satisfy_kernel_request(mobj, r, DATA_NONE);
		}
		return KERN_SUCCESS;
	}

	/*
	 * If requesting kernel wants to read, but the page is being written,
	 * then we must clean and lock the writer.
	 */
	if (r->desired_access && (MOBJ->prot[page] & VM_PROT_WRITE)) {
		if (KOBJ->prot[page] & VM_PROT_WRITE) {
			/*
			 * What could the writer be doing asking us for read?
			 *
			 * This can happen if page was cleaned and flushed,
			 * or (more commonly?) cleaned and then paged out.
			 *
			 * Should we give this kernel read (more concurrency)
			 * or write (on the assumption that he will want
			 * to write again)?
			 *
			 * For now, we just give him read.
			 * We have to correct our notion of how this page is
			 * used. Note that there is no problem giving
			 * him either read or write, since there is nobody
			 * else to evict.
			 */
			KOBJ->prot[page] = r->desired_access;
			MOBJ->prot[page] = r->desired_access;
			satisfy_kernel_request(mobj, r, DATA_NONE);
			return KERN_SUCCESS;
		}
		for (k = MOBJ->kobj_list; k; k = K->next) {
			if (K->prot[page] & VM_PROT_WRITE) {
				break;
			}
		}
		if (k == XMM_OBJ_NULL) {
			printf("x lost writer!\n");
			return KERN_FAILURE;
		}
		MOBJ->prot[page] = VM_PROT_READ;
		K->prot[page] = VM_PROT_READ;
		r->k_count++;
		K_LOCK_REQUEST(k, r->offset, vm_page_size, TRUE, FALSE,
			       VM_PROT_WRITE, mobj);
		return KERN_SUCCESS;
	}

	/*
	 * No current kernel use conflicts with requesting kernel's desired
	 * use. Call satisfy_kernel_request, which will handle any requests
	 * that need to be made of the pager.
	 */
	satisfy_kernel_request(mobj, r, DATA_NONE);
	return KERN_SUCCESS;
}

process_pager_request(mobj, r)
	xmm_obj_t mobj;
	request_t r;
{
	int page;
	xmm_obj_t k;

Mcheck
	page = atop(r->offset);

	/*
	 * Locking against non-write implies locking all access.
	 * Is this a bug, or universal truth?
	 * Beware: code below and elsewhere depends on this mapping.
	 */
	if (r->lock_value & ~VM_PROT_WRITE) {
		r->lock_value = VM_PROT_ALL;
	}

	/*
	 * XXX we can't yet represent
	 *	(lock=write but dirty)
	 * or
	 *	(lock=all but resident)
	 *
	 * Thus we force lock=write into clean,
	 * and lock=all into flush.
	 */
	if (r->lock_value == VM_PROT_WRITE) {
		r->should_clean = TRUE;
	} else if (r->lock_value) {
		r->should_clean = TRUE;
		r->should_flush = TRUE;
	}

	/*
	 * If we need to flush, or lock all access, then we must talk
	 * to all kernels.
	 */
	if (r->should_flush || r->lock_value == VM_PROT_ALL) {
		r->k_count++;
		MOBJ->prot[page] &= ~r->lock_value;
		for (k = MOBJ->kobj_list; k; k = K->next) {
			if (K->prot[page] == VM_PROT_NONE) {
				continue;
			}
			r->k_count++;
			K->prot[page] &= ~r->lock_value;
			K_LOCK_REQUEST(k, r->offset, vm_page_size,
				       r->should_clean, r->should_flush,
				       r->lock_value, mobj);
		}
		if (--r->k_count == 0 && r->m_count == 0) {
			satisfy_request(mobj, r, DATA_NONE);
		}
		return KERN_SUCCESS;
	}

	/*
	 * If we need to clean, or lock write access, and there is in fact
	 * a writer, then we must talk to that writer.
	 */
	if ((r->should_clean || r->lock_value == VM_PROT_WRITE)
	    && (MOBJ->prot[page] & VM_PROT_WRITE)) {
		MOBJ->prot[page] &= ~r->lock_value;
		for (k = MOBJ->kobj_list; k; k = K->next) {
			if (K->prot[page] & VM_PROT_WRITE) {
				break;
			}
		}
		if (k == XMM_OBJ_NULL) {
			printf("y lost writer!\n");
			return KERN_FAILURE;
		}
		K->prot[page] &= ~r->lock_value;
		r->k_count++;
		K_LOCK_REQUEST(k, r->offset, vm_page_size,
			       r->should_clean, FALSE,
			       r->lock_value, mobj);
		return KERN_SUCCESS;
	}

	/*
	 * We didn't need to flush, clean, or lock.
	 */
	satisfy_pager_request(mobj, r);
	return KERN_SUCCESS;
}

process_request(mobj, r)
	xmm_obj_t mobj;
	request_t r;
{
Mcheck
	if (r->is_kernel) {
		return process_kernel_request(mobj, r);
	} else {
		return process_pager_request(mobj, r);
	}
}

void
satisfy_kernel_request(mobj, r, data)
	xmm_obj_t mobj;
	request_t r;
	vm_offset_t data;
{
	xmm_obj_t kobj;
	request_t r_next;

Mcheck
	kobj = r->kobj;

	if (! r->is_kernel) {
		printf("(kernel) oh no! not kernel!\n");
		return;
	}
	if (r->k_count != 0) {
		printf("(kernel) oh no! k_count = %d != 0!\n", r->k_count);
		return;
	}
	if (r->m_count != 0) {
		printf("(kernel) oh no! m_count = %d != 0!\n", r->m_count);
		return;
	}

	/*
	 * If we need an unlock or data from the pager, make the request now.
	 */
	if ((MOBJ->lock[atop(r->offset)] & r->desired_access)
	    || (r->needs_data && data == DATA_NONE)) {
		if (data) {
			M_DATA_WRITE(mobj, mobj, r->offset, data,
				     vm_page_size);
		}
		r->m_count++;
		if (r->needs_data) {
			M_DATA_REQUEST(mobj, mobj, r->offset, vm_page_size,
				       r->desired_access);
		} else {
			M_DATA_UNLOCK(mobj, mobj, r->offset, vm_page_size,
				      r->desired_access);
		}
		return;
	}

	/*
	 * We have everything we need. Satisfy the kernel request.
	 */
	if (! r->needs_data) {
		K_LOCK_REQUEST(r->kobj, r->offset, vm_page_size, FALSE, FALSE,
			       r->desired_access ^ VM_PROT_ALL, XMM_OBJ_NULL);
	} else if (data == DATA_UNAVAILABLE) {
		K_DATA_UNAVAILABLE(r->kobj, r->offset, vm_page_size);
		r->desired_access = VM_PROT_ALL;	/* XXX */
	} else if (data == DATA_ERROR) {
		K_DATA_ERROR(r->kobj, r->offset, vm_page_size, KERN_FAILURE);
		/* XXX start killing object? */
	} else {
		K_DATA_PROVIDED(r->kobj, r->offset, data, vm_page_size,
				r->desired_access ^ VM_PROT_ALL);
	}

	/*
	 * Update KOBJ->prot[] and MOBJ->prot[] values.
	 */
	MOBJ->prot[atop(r->offset)] = r->desired_access;
	KOBJ->prot[atop(r->offset)] = r->desired_access;

	/*
	 * Remove and free request.
	 */
	r_next = remove_request(mobj, r->offset);
	zfree(xmm_svm_request_zone, r);

	/*
	 * If there is another request, process it now.
	 */
	if (r_next) {
		process_request(mobj, r_next);
	}
}

void
satisfy_pager_request(mobj, r)
	xmm_obj_t mobj;
	request_t r;
{
	request_t r_next;

Mcheck
	if (r->is_kernel) {
		printf("(pager) oh no! not pager!\n");
		return;
	}
	if (r->k_count != 0) {
		printf("(pager) oh no! k_count = %d != 0!\n", r->k_count);
		return;
	}
	if (r->m_count != 0) {
		printf("(pager) oh no! m_count = %d != 0!\n", r->m_count);
		return;
	}

	/*
	 * We have everything we need. Satisfy the pager request.
	 */
	if (r->kobj) {
		M_LOCK_COMPLETED(r->kobj, mobj, r->offset, vm_page_size);
	}

	/*
	 * Update MOBJ->lock[] value.
	 */
	MOBJ->lock[atop(r->offset)] = r->lock_value;

	/*
	 * Remove and free request.
	 */
	r_next = remove_request(mobj, r->offset);
	zfree(xmm_svm_request_zone, r);

	/*
	 * If there is another request, process it now.
	 */
	if (r_next) {
		process_request(mobj, r_next);
	}
}

void
satisfy_request(mobj, r, data)
	xmm_obj_t mobj;
	request_t r;
	vm_offset_t data;
{
Mcheck
	if (r->is_kernel) {
		satisfy_kernel_request(mobj, r, data);
	} else {
		satisfy_pager_request(mobj, r);
	}
}

xmm_svm_init()
{
	xmm_svm_request_zone = zinit(sizeof(struct request), 512*1024,
				     sizeof(struct request), FALSE,
				     "xmm.svm.request");
}

#ifdef	MACH_KERNEL_XXX
/*
 * These routines allow us to stuff an svm mobj under a live vm_object.
 *
 * XXX We need to be more clever about sending data_unavailable without
 * XXX asking pager.
 *
 * XXX We really need something with a bitmap at each node,
 * XXX a smarter version of xmm_import that knows not to bother
 * XXX asking remote directory.
 */
xmm_svm_set_access(mobj, kobj, access)
	xmm_mobj_t mobj;
	xmm_mobj_t kobj;
	vm_prot_t access;
{
	
}

/*
 * Initializes mobj without calling memory_object_init on pager.
 */
xmm_svm_initialize(mobj, pager)
	xmm_mobj_t mobj;
	void *pager;
{
	if (pager) {
		panic("xmm_svm_initialize: pager!\n");
	}
}
#endif	/* MACH_KERNEL_XXX */
