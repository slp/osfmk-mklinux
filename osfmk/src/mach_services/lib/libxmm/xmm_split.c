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
 *	File:	norma/xmm_split.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer to split multi-page lock_requests.
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

typedef struct request *request_t;

#define	REQUEST_NULL	((request_t) 0)

struct mobj {
	struct xmm_obj	obj;
	request_t	r_head;
	request_t	r_tail;
};

struct request {
	vm_offset_t	r_offset;
	vm_size_t	r_length;
	vm_offset_t	r_start;
	vm_size_t	r_count;
	vm_size_t	r_resid;
	request_t	r_next;
};

#undef  KOBJ
#define KOBJ    ((struct mobj *) kobj)

kern_return_t m_split_lock_completed();
kern_return_t k_split_lock_request();

xmm_decl(split_class,
	/* m_init		*/	m_interpose_init,
	/* m_terminate		*/	m_interpose_terminate,
	/* m_copy		*/	m_interpose_copy,
	/* m_data_request	*/	m_interpose_data_request,
	/* m_data_unlock	*/	m_interpose_data_unlock,
	/* m_data_write		*/	m_interpose_data_write,
	/* m_lock_completed	*/	m_split_lock_completed,
	/* m_supply_completed	*/	m_interpose_supply_completed,
	/* m_data_return	*/	m_interpose_data_return,

	/* k_data_provided	*/	k_interpose_data_provided,
	/* k_data_unavailable	*/	k_interpose_data_unavailable,
	/* k_get_attributes	*/	k_interpose_get_attributes,
	/* k_lock_request	*/	k_split_lock_request,
	/* k_data_error		*/	k_interpose_data_error,
	/* k_set_attributes	*/	k_interpose_set_attributes,
	/* k_destroy		*/	k_interpose_destroy,
	/* k_data_supply	*/	k_interpose_data_supply,

	/* name			*/	"split",
	/* size			*/	sizeof(struct mobj)
);

zone_t		xmm_split_request_zone;

kern_return_t
xmm_split_create(old_mobj, new_mobj)
	xmm_obj_t old_mobj;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&split_class, old_mobj, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->r_head = REQUEST_NULL;
	MOBJ->r_tail = REQUEST_NULL;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

/*
 * XXX
 * Note: same old robj problem
 */
k_split_lock_request(kobj, offset, length, should_clean, should_flush,
		     lock_value, robj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t robj;
{
	request_t r;

	if (length == 0) {
#if	NOISY
		printf("k_split_lock_request: length=0\n");
#endif	/* NOISY */
		M_LOCK_COMPLETED(robj, kobj, offset, length);
		return KERN_SUCCESS;
	}
	if (robj) {
		r = (request_t) zalloc(xmm_split_request_zone);
		r->r_offset = offset;
		r->r_length = length;
		r->r_start = (offset + vm_page_size - 1) / vm_page_size;
		r->r_count = (length + vm_page_size - 1) / vm_page_size;
		r->r_resid = r->r_count;
		r->r_next = REQUEST_NULL;
		if (KOBJ->r_head) {
			KOBJ->r_tail->r_next = r;
		} else {
			KOBJ->r_head = r;
		}
		KOBJ->r_tail = r;
#if	NOISY
		printf("@r start=%d count=%d\n", r->r_start, r->r_count);
#endif	/* NOISY */
	}
	while (length > vm_page_size) {
		K_LOCK_REQUEST(kobj, offset, vm_page_size, should_clean,
			       should_flush, lock_value, robj);
		offset += vm_page_size;
		length -= vm_page_size;
	}
	if (length > 0) {
		K_LOCK_REQUEST(kobj, offset, length, should_clean,
			       should_flush, lock_value, robj);
	}
	/* XXX check return values above??? */
	return KERN_SUCCESS;
}

m_split_lock_completed(robj, kobj, offset, length)
	xmm_obj_t robj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;	/* XXX ignored */
{
	vm_offset_t start;
	request_t r, *rp, r_prev = REQUEST_NULL;

	start = (offset + vm_page_size - 1) / vm_page_size;
#if	NOISY
	printf("#r start=%d\n", start);
#endif	/* NOISY */
	for (rp = &KOBJ->r_head; r = *rp; rp = &r->r_next) {
#if	NOISY
		printf(":r start=%d..%d resid=%d\n",
		       r->r_start, r->r_start + r->r_count - 1, r->r_resid);
#endif	/* NOISY */
		if (start >= r->r_start && start < r->r_start + r->r_count) {
			break;
		}
		r_prev = r;
	}
	if (r == REQUEST_NULL) {
		panic("k_split_lock_completed: lost request\n");
	}
#if	NOISY
	printf("#r found\n");
#endif	/* NOISY */
	if (--r->r_resid > 0) {
		return KERN_SUCCESS;
	}
	if (r == KOBJ->r_tail) {
		KOBJ->r_tail = r_prev;
	}
	*rp = r->r_next;
	M_LOCK_COMPLETED(robj, kobj, r->r_offset, r->r_length);
	zfree(xmm_split_request_zone, r);
	return KERN_SUCCESS;
}

xmm_split_init()
{
	xmm_split_request_zone = zinit(sizeof(struct request), 512*1024,
				       sizeof(struct request), FALSE,
				       "xmm.split.request");
}
