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
 *	Routines for managing mobj and kobj per-page state.
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
#include <kern/xpr.h>

vm_prot_t
m_get_prot(
	xmm_obj_t	mobj,
	unsigned int	page)
{
	vm_prot_t prot = VM_PROT_NONE;
	xmm_obj_t k;
	char *k_bits;

	assert(xmm_obj_lock_held(mobj));
	for (k = MOBJ->kobj_list; k; k = K->next) {
		xmm_obj_lock(k);
		if (!K->terminated) {
			K_MAP_GET(K, page, k_bits);
			if (k_bits != (char *)0)
				prot |= K_GET_PROT(k_bits);
		}
		xmm_obj_unlock(k);
	}
	return prot;
}

/*
 * MP note : Upon return, a non-NULL returned writer is locked.
 */
vm_prot_t
m_get_writer(
	xmm_obj_t	mobj,
	unsigned int	page,
	xmm_obj_t	*writer)
{
	vm_prot_t p, prot = VM_PROT_NONE;
	xmm_obj_t k;
	char *k_bits;

	assert(xmm_obj_lock_held(mobj));
	*writer = XMM_OBJ_NULL;
	for (k = MOBJ->kobj_list; k; k = K->next) {
		xmm_obj_lock(k);
		if (!K->terminated) {
			K_MAP_GET(K, page, k_bits);
			if (k_bits != (char *)0) {
				p = K_GET_PROT(k_bits);
				prot |= p;
				if (p & VM_PROT_WRITE) {
					assert(*writer == XMM_OBJ_NULL);
					*writer = k;
				} else
					xmm_obj_unlock(k);
			} else
				xmm_obj_unlock(k);
		} else
			xmm_obj_unlock(k);
	}
	return prot;
}

xmm_obj_t
m_get_precious_kernel(
	xmm_obj_t	mobj,
	unsigned int	page)
{
	xmm_obj_t k;
	char *k_bits;

	assert(xmm_obj_lock_held(mobj));
	for (k = MOBJ->kobj_list; k; k = K->next) {
		xmm_obj_lock(k);
		 K_MAP_GET(K, page, k_bits);
		if (k_bits != (char *)0 && K_GET_PRECIOUS(k_bits)) {
			assert(K_GET_PROT(k_bits) != VM_PROT_NONE);
			xmm_obj_unlock(k);
			return k;
		}
		xmm_obj_unlock(k);
	}
	return XMM_OBJ_NULL;
}

xmm_obj_t
m_get_nonprecious_kernel(
	xmm_obj_t	mobj,
	unsigned int	page)
{
	xmm_obj_t k;
	char *k_bits;

	assert(xmm_obj_lock_held(mobj));
	for (k = MOBJ->kobj_list; k; k = K->next) {
		xmm_obj_lock(k);
		K_MAP_GET(K, page, k_bits);
		if (k_bits != (char *)0 &&
		    K_GET_PROT(k_bits) && ! K_GET_PRECIOUS(k_bits)) {
			xmm_obj_unlock(k);
			return k;
		}
		xmm_obj_unlock(k);
	}
	return XMM_OBJ_NULL;
}

/*
 * MP note : Upon return, a non-NULL returned kobj is locked.
 */
xmm_obj_t
svm_get_active(
	xmm_obj_t	mobj)
{
	xmm_obj_t kobj;

	assert(xmm_obj_lock_held(mobj));
	for (kobj = MOBJ->kobj_list; kobj; kobj = KOBJ->next) {
		xmm_obj_lock(kobj);
		if (KOBJ->active && !KOBJ->terminated) {
			return kobj;
		}
		xmm_obj_unlock(kobj);
	}
	return XMM_OBJ_NULL;
}

int
svm_get_active_count(
	xmm_obj_t	mobj)
{
	xmm_obj_t kobj;
	int count = 0;

	assert(xmm_obj_lock_held(mobj));
	for (kobj = MOBJ->kobj_list; kobj; kobj = KOBJ->next) {
		xmm_obj_lock(kobj);
		if (KOBJ->active && !KOBJ->terminated) {
			count++;
		}
		xmm_obj_unlock(kobj);
	}
	return count;
}

#if	MACH_ASSERT
boolean_t
m_check_kernel_prots(
	xmm_obj_t	mobj,
	unsigned int	page)
{
	xmm_obj_t k;
	vm_prot_t lock;
	char *k_bits;
	char *m_bits;

	assert(xmm_obj_lock_held(mobj));
	M_MAP_GET(MOBJ, page, m_bits);
	lock = M_GET_LOCK(m_bits);
	for (k = MOBJ->kobj_list; k; k = K->next) {
		xmm_obj_lock(k);
		K_MAP_GET(K, page, k_bits);
		if (k_bits != (char *)0 && (K_GET_PROT(k_bits) & lock)) {
			panic("m_check_kernel_prots: k=0x%x\n", k);
			xmm_obj_unlock(k);
			return FALSE;
		}
		xmm_obj_unlock(k);
	}
	return TRUE;
}
#endif	/* MACH_ASSERT */

/*
 * Forward.
 */
union svm_state_bitmap	*svm_extend_mobj_create(
				xmm_obj_t		mobj,
				unsigned long		page,
				union svm_state_bitmap	***last);

void			svm_extend_kobj_create(
				xmm_obj_t	kobj,
				unsigned long	page);

void			svm_extend_allocate_level(
				union svm_state_bitmap	*sp,
				unsigned		level,
				xmm_obj_t		kobj,
				unsigned long		page_base);



/*
 * Go through indirect levels to find a direct bitmap.
 */
union svm_state_bitmap *
svm_extend_get(
	union svm_state_bitmap	*sp,
	unsigned long		page,
	unsigned		level)
{
	unsigned page_base;

	assert(level > 0);

	page_base = XMM_STATE_DIRECT + (level - 1) * XMM_STATE_INDIRECT;
	sp = sp->next[(page >> page_base) & (XMM_STATE_INDIRECT_SIZE - 1)];

	if (level == 1 || sp == (union svm_state_bitmap *)0)
		return (sp);
	return (svm_extend_get(sp, page % (1 << page_base), level - 1));
}

/*
 * Create all indirection bitmaps to create a page for a Mobject.
 */
union svm_state_bitmap *
svm_extend_mobj_create(
	xmm_obj_t		mobj,
	unsigned long		page,
	union svm_state_bitmap	***last)
{
	union svm_state_bitmap *new;
	union svm_state_bitmap **sp;
	unsigned page_base;
	unsigned i;
	unsigned level;

	XPR(XPR_XMM,"svm_extend_mobj_create mobj=0x%x page=0x%x\n",
	    (integer_t)mobj, page, 0, 0, 0);
	if (MOBJ->bits.bitmap == (union svm_state_bitmap *)0 &&
	    page >= XMM_STATE_DIRECT_SIZE) {
		/*
		 * MP note: MOBJ->bits.range can be initialized without any
		 *	lock held, since MOBJ->bits.bitmap is still NULL.
		 */
		MOBJ->bits.range = page & ~(XMM_STATE_DIRECT_SIZE - 1);
	}

	while (page < MOBJ->bits.range ||
	       page >= (MOBJ->bits.range +
			(1 << (XMM_STATE_DIRECT +
			       MOBJ->bits.level * XMM_STATE_INDIRECT)))) {
		/*
		 * Increase level of indirection.
		 */
		new = (union svm_state_bitmap *)
			kalloc(XMM_STATE_INDIRECT_SIZE *
			       sizeof (union svm_state_bitmap *));
		for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
			new->next[i] = (union svm_state_bitmap *)0;

		xmm_obj_lock(mobj);
		level = XMM_STATE_DIRECT +
			MOBJ->bits.level * XMM_STATE_INDIRECT;
		i = (MOBJ->bits.range >> level) &
			(XMM_STATE_INDIRECT_SIZE - 1);
		new->next[i] = MOBJ->bits.bitmap;
		MOBJ->bits.level++;
		level += XMM_STATE_INDIRECT;
		if (MOBJ->bits.range)
			MOBJ->bits.range = (MOBJ->bits.range >> level)
				<< level;
		MOBJ->bits.bitmap = new;
		xmm_obj_unlock(mobj);
	}

	/*
	 * Create all intermediate indirect levels.
	 */
	sp = &MOBJ->bits.bitmap;
	level = MOBJ->bits.level;
	page -= MOBJ->bits.range;
	while (level > 0) {
		page_base = XMM_STATE_DIRECT + --level * XMM_STATE_INDIRECT;
		if (*sp == (union svm_state_bitmap *)0) {
			new = (union svm_state_bitmap *)
				kalloc(XMM_STATE_INDIRECT_SIZE *
				       sizeof (union svm_state_bitmap *));
			for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
				new->next[i] = (union svm_state_bitmap *)0;
			xmm_obj_lock(mobj);
			*sp = new;
			xmm_obj_unlock(mobj);
		}
		sp = &((*sp)->next[(page >> page_base) &
				   (XMM_STATE_INDIRECT_SIZE - 1)]);
	}

	/*
	 * Create final bitmap.
	 */
	if (*sp != (union svm_state_bitmap *)0) {
		*last = (union svm_state_bitmap **)0;
		return (*sp);
	}

	*last = sp;
	return ((union svm_state_bitmap *)kalloc(XMM_STATE_DIRECT_SIZE));
}

/*
 * Create all indirection bitmaps to create a page for a Kobject.
 */
void
svm_extend_kobj_create(
	xmm_obj_t	kobj,
	unsigned long	page)
{
	union svm_state_bitmap *new;
	union svm_state_bitmap **sp;
	unsigned page_base;
	unsigned i;
	unsigned level;

	if (KOBJ->bits.bitmap == (union svm_state_bitmap *)0 &&
	    page >= XMM_STATE_DIRECT_SIZE) {
		/*
		 * MP note: KOBJ->bits.range can be initialized without any
		 *	lock held, since KOBJ->bits.bitmap is still NULL.
		 */
		KOBJ->bits.range = page & ~(XMM_STATE_DIRECT_SIZE - 1);
	}

	while (page < KOBJ->bits.range ||
	       page >= (KOBJ->bits.range +
			(1 << (XMM_STATE_DIRECT +
			       KOBJ->bits.level * XMM_STATE_INDIRECT)))) {
		/*
		 * Increase level of indirection.
		 */
		new = (union svm_state_bitmap *)
			kalloc(XMM_STATE_INDIRECT_SIZE *
			       sizeof (union svm_state_bitmap *));
		for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
			new->next[i] = (union svm_state_bitmap *)0;

		xmm_obj_lock(kobj);
		level = XMM_STATE_DIRECT +
			KOBJ->bits.level * XMM_STATE_INDIRECT;
		i = (KOBJ->bits.range >> level) &
			(XMM_STATE_INDIRECT_SIZE - 1);
		new->next[i] = KOBJ->bits.bitmap;
		KOBJ->bits.level++;
		level += XMM_STATE_INDIRECT;
		if (KOBJ->bits.range)
			KOBJ->bits.range = (KOBJ->bits.range >> level)
				<< level;
		KOBJ->bits.bitmap = new;
		xmm_obj_unlock(kobj);
	}

	/*
	 * Create all intermediate indirect levels.
	 */
	sp = &KOBJ->bits.bitmap;
	level = KOBJ->bits.level;
	page -= KOBJ->bits.range;
	while (level > 0) {
		page_base = XMM_STATE_DIRECT + --level * XMM_STATE_INDIRECT;
		if (*sp == (union svm_state_bitmap *)0) {
			new = (union svm_state_bitmap *)
				kalloc(XMM_STATE_INDIRECT_SIZE *
				       sizeof (union svm_state_bitmap *));
			for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
				new->next[i] = (union svm_state_bitmap *)0;
			xmm_obj_lock(kobj);
			*sp = new;
			xmm_obj_unlock(kobj);
		}
		sp = &((*sp)->next[(page >> page_base) &
				   (XMM_STATE_INDIRECT_SIZE - 1)]);
	}

	/*
	 * Create final bitmap.
	 */
	if (*sp == (union svm_state_bitmap *)0) {
		new = (union svm_state_bitmap *)kalloc(XMM_STATE_DIRECT_SIZE);
		bzero((char *)new, XMM_STATE_DIRECT_SIZE);
		xmm_obj_lock(kobj);
		*sp = new;
		xmm_obj_unlock(kobj);
	}
}

/*
 * Allocate recursively a bitmap for a kobj.
 */
void
svm_extend_allocate_level(
	union svm_state_bitmap	*sp,
	unsigned		level,
	xmm_obj_t		kobj,
	unsigned long		page_base)
{
	unsigned i;

	if (level == 0) {
		svm_extend_kobj_create(kobj, page_base);
		return;
	}

	for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
		if (sp->next[i] != (union svm_state_bitmap *)0)
			svm_extend_allocate_level(sp->next[i], level - 1, kobj,
						  (page_base + i) << 
						  (level > 1 ?
						   XMM_STATE_INDIRECT :
						   XMM_STATE_DIRECT));
}

/*
 * Allocate a kobj state tree that has exactly the same structure as its mobj.
 */
void
svm_extend_allocate(
	xmm_obj_t	mobj,
	xmm_obj_t	kobj)
{
	assert(xmm_obj_lock_held(mobj));
	if (MOBJ->bits.bitmap == (union svm_state_bitmap *)0)
		return;

	while (MOBJ->extend_in_progress) {
		MOBJ->extend_wanted = 1;
		assert_wait(svm_extend_event(mobj), FALSE);
		xmm_obj_unlock(mobj);
		thread_block((void (*)(void)) 0);
		xmm_obj_lock(mobj);
	}
	MOBJ->extend_in_progress = 1;
	xmm_obj_unlock(mobj);

	if (MOBJ->bits.bitmap != (union svm_state_bitmap *)0) {
		svm_extend_allocate_level(MOBJ->bits.bitmap,
					  MOBJ->bits.level, kobj, 0);
		assert(KOBJ->bits.range == 0);
		KOBJ->bits.range = MOBJ->bits.range;
	}

	xmm_obj_lock(mobj);
	MOBJ->extend_in_progress = 0;
	if (MOBJ->extend_wanted) {
		MOBJ->extend_wanted = 0;
		thread_wakeup(svm_extend_event(mobj));
	}
}

/*
 * Deallocate a complete state tree.
 */
void
svm_extend_deallocate(
	union svm_state_bitmap	*sp,
	unsigned		level)
{
	unsigned i;

	if (sp == (union svm_state_bitmap *)0)
		return;

	if (level == 0) {
		kfree((vm_offset_t)sp, XMM_STATE_DIRECT_SIZE);
		return;
	}

	for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
		if (sp->next[i] != (union svm_state_bitmap *)0)
			svm_extend_deallocate(sp->next[i], level-1);
	kfree((vm_offset_t)sp, (XMM_STATE_INDIRECT_SIZE *
				 sizeof (union svm_state_bitmap *)));
}

void
svm_extend(
	xmm_obj_t	mobj,
	unsigned long	page)
{
	xmm_obj_t kobj;
	char bits;
	union svm_state_bitmap *sp;
	union svm_state_bitmap **last;
	unsigned i;

	XPR(XPR_XMM,"svm_extend mobj=0x%x page=0x%x\n",
	    (integer_t)mobj, page, 0, 0, 0);
	assert(xmm_obj_lock_held(mobj));
	assert(MOBJ->extend_in_progress == 0);
	MOBJ->extend_in_progress = 1;

	/*
	 * First start by K objects extension.
	 */
	svm_klist_first(mobj, &kobj);
	xmm_obj_unlock(mobj);
	while (kobj) {
		xmm_obj_unlock(kobj);
		svm_extend_kobj_create(kobj, page);
		xmm_obj_lock(mobj);
		xmm_obj_lock(kobj);
		svm_klist_next(mobj, &kobj, FALSE);
	}

	/*
	 * Continue by M object extension.
	 */
	sp = svm_extend_mobj_create(mobj, page, &last);
	xmm_obj_lock(mobj);
	if (last != (union svm_state_bitmap **)0) {
		/*
		 * Initialize bit map.
		 */
		XPR(XPR_XMM,"svm_extend initialize mobj=0x%x page=0x%x\n",
		    (integer_t)mobj, page, 0, 0, 0);
		bits = 0;
		M_ASN_LOCK(&bits, VM_PROT_ALL);
		M_CLR_PRECIOUS(&bits);
		M_CLR_DIRTY(&bits);
		if (svm_get_stable_copy(mobj)) {
			M_SET_NEEDS_COPY(&bits);
		} else {
			M_CLR_NEEDS_COPY(&bits);
		}
		for (i = 0; i <  XMM_STATE_DIRECT_SIZE; i++)
			sp->bits[i] = bits;
		*last = sp;
	}

	MOBJ->extend_in_progress = 0;
	if (MOBJ->extend_wanted) {
		MOBJ->extend_wanted = 0;
		thread_wakeup(svm_extend_event(mobj));
	}
}

/*
 * svm_extend_new_copy() is called by svm_create_new_copy() with the new
 * copy as parameter. Its goal is to set up new copy attributes when it is
 * created (rather than in initialization process), in order to take into
 * account NEEDS_COPY attribute of the shadow.
 */
void
svm_extend_new_copy(
	union svm_state_bitmap	*sp,
	unsigned		level,
	xmm_obj_t		mobj,
	unsigned long		page_base)
{
	union svm_state_bitmap *bp;
	union svm_state_bitmap **last;
	unsigned i;
	unsigned j;
	char bits;

	XPR(XPR_XMM,"svm_extend_new_copy mobj=0x%x page_base=0x%x level=%d\n",
	    (integer_t)mobj, page_base, level, 0, 0);
	if (sp == (union svm_state_bitmap *)0)
		return;

	if (level > 0) {
		for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
			if (sp->next[i] != (union svm_state_bitmap *)0)
				svm_extend_new_copy(sp->next[i], 
						    level - 1, mobj,
						    (page_base + i) <<
						     (level > 1 ?
						      XMM_STATE_INDIRECT :
						      XMM_STATE_DIRECT));
		return;
	}

	bp = (union svm_state_bitmap *)0;
	for (i = 0; i < XMM_STATE_DIRECT_SIZE; i++)
		if (!M_GET_NEEDS_COPY(&sp->bits[i])) {
			if (bp == (union svm_state_bitmap *)0) {
				bp = svm_extend_mobj_create(mobj,
							    MOBJ->bits.range +
							    page_base + i,
							    &last);
				if (last != (union svm_state_bitmap **)0) {
					/*
					 * Initialize bit map.
					 */
					XPR(XPR_XMM,
					    "svm_extend_new_copy initialize mobj=0x%x\n",
					    (integer_t)mobj, 0, 0, 0, 0);
					bits = 0;
					M_ASN_LOCK(&bits, VM_PROT_ALL);
					M_CLR_PRECIOUS(&bits);
					M_CLR_DIRTY(&bits);
					M_SET_NEEDS_COPY(&bits);
					for (j = 0;
					     j <  XMM_STATE_DIRECT_SIZE; j++)
						bp->bits[j] = bits;
					*last = bp;
				}
			}
			M_CLR_NEEDS_COPY(&bp->bits[i]);
		}
}

/*
 * svm_extend_set_copy recursively runs through all bitmaps of the
 * object in order to set NEEDS_COPY bit.
 */
void
svm_extend_set_copy(
	union svm_state_bitmap	*sp,
	unsigned		level)
{
	unsigned i;

	if (sp == (union svm_state_bitmap *)0)
		return;

	if (level > 0) {
		for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
			if (sp->next[i] != (union svm_state_bitmap *)0)
				svm_extend_set_copy(sp->next[i], level - 1);
		return;
	}

	for (i = 0; i < XMM_STATE_DIRECT_SIZE; i++)
		if (!M_GET_NEEDS_COPY(&sp->bits[i]))
			M_SET_NEEDS_COPY(&sp->bits[i]);
}

/*
 * svm_extend_clear_copy recursively runs through all bitmaps of the
 * object in order to clear NEEDS_COPY bit.
 */
void
svm_extend_clear_copy(
	union svm_state_bitmap	*sp,
	unsigned		level)
{
	unsigned i;

	if (sp == (union svm_state_bitmap *)0)
		return;

	if (level > 0) {
		for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
			if (sp->next[i] != (union svm_state_bitmap *)0)
				svm_extend_clear_copy(sp->next[i], level - 1);
		return;
	}

	for (i = 0; i < XMM_STATE_DIRECT_SIZE; i++)
		if (M_GET_NEEDS_COPY(&sp->bits[i]))
			M_CLR_NEEDS_COPY(&sp->bits[i]);
}

#if	MACH_PAGEMAP
/*
 * svm_extend_update_pagemap recursively runs through all bitmaps of the
 * shadow object in order to update pagemap of the old_copy.
 */
void
svm_extend_update_pagemap(
	union svm_state_bitmap	*sp,
	unsigned		level,
	unsigned long		range,
	xmm_obj_t		old_copy,
	unsigned long		page_base)
{
	unsigned i;

	assert(xmm_obj_lock_held(old_copy));

	if (sp == (union svm_state_bitmap *)0)
		return;

	if (level > 0) {
		for (i = 0; i < XMM_STATE_INDIRECT_SIZE; i++)
			if (sp->next[i] != (union svm_state_bitmap *)0)
				svm_extend_update_pagemap(sp->next[i], level-1,
							  range, old_copy,
							  (page_base + i) <<
							  (level > 1 ?
							   XMM_STATE_INDIRECT :
							   XMM_STATE_DIRECT));
		return;
	}

	for (i = 0; i < XMM_STATE_DIRECT_SIZE; i++)
		if (!M_GET_NEEDS_COPY(&sp->bits[i])) {
			assert(OLD_COPY->pagemap != VM_EXTERNAL_NULL);
			assert(ptoa(range + page_base + i) <
						OLD_COPY->existence_size);
			vm_external_state_set(OLD_COPY->pagemap,
					      ptoa(range + page_base + i));
		}
}
#endif

#include <mach_kdb.h>
#if	MACH_KDB
#include <ddb/db_output.h>

#define	printf	kdbprintf

/*
 * Forward.
 */
void	p_svm_print(
		xmm_obj_t	mobj,
		vm_offset_t	offset,
		int		count);

/*
 *	Routine:	p_svm_print
 *	Purpose:
 *		Pretty-print svm state information for a range of pages.
 */

void
p_svm_print(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	int		count)
{
	unsigned int page, stop_page;
	xmm_obj_t k;
	char *bits;
	static char *protstring[] = {
		"---",
		"r--",
		"-w-",
		"rw-",
		"--x",
		"r-x",
		"-wx",
		"rwx",
	};

	indent += 2;

	if ((stop_page = atop(offset) + count) > MOBJ->num_pages) {
		stop_page = MOBJ->num_pages;
	}
	for (page = atop(offset); page < stop_page; page++) {
		if (svm_extend_needed(mobj, page)) {
			iprintf("0x%08x: Page without state (never accessed)\n",
				ptoa(page));
			continue;
		}
		M_MAP_GET(MOBJ, page, bits);
		iprintf("0x%08x: m=<prec=%d,dirt=%d,lock=%s,needcpy=%d>\n",
			ptoa(page),
			M_GET_PRECIOUS(bits),
			M_GET_DIRTY(bits),
			protstring[M_GET_LOCK(bits) & 0x7],
			M_GET_NEEDS_COPY(bits));
		for (k = MOBJ->kobj_list; k; k = K->next) {
			K_MAP_GET(K, page, bits);
			iprintf("%sk=<prec=%d,pend=%d,prot=%s> k=0x%x\n",
				"            ",
				K_GET_PRECIOUS(bits),
				K_GET_PENDING(bits),
				protstring[K_GET_PROT(bits) & 0x7],
				k);
		}
	}

	indent -=2;
}

#endif	/* MACH_KDB */
