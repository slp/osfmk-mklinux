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
 *	Definitions for managing mobj and kobj per-page state.
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

#include <mach_pagemap.h>
#include <xmm/xmm_obj.h>

/*
 * Page state bit maps are split between multiple levels to be able to handle
 * large virtual object without allocating all page bit maps, and to allocate
 * new bit maps without locking the object requesting a page that already has
 * a valid bit map.
 *
 * At XMM object level, there is a xmm_state structure. Page bits maps
 * which number is between 0 and XMM_STATE_DIRECT-1 are located in the direct
 * page structure. Those which number are between XMM_STATE_DIRECT and
 * XMM_STATE_DIRECT * XMM_STATE_INDIRECT - 1 are located in the tree of
 * xmm_state_direct structure allocated from 1 to XMM_STATE_INDIRECT-1 in
 * the xmm_state_indirect structure. Notice that the first element of the
 * table which should correspond to the [0.. XMM_STATE_DIRECT-1] page bit
 * maps (that is already allocated in the first xmm_state_direct structure)
 * is reserved for the next indirect level.
 *
 * An implemented optimization permits to remove unused indirect levels, i.e.
 * levels that have only one next bitmap in their next pointers array. The
 * level still represents the number of indirections in the bitmap tree;
 * The real page number is still computed by its position in the bitmap tree,
 * but it is now translated by the range, that is the virtual offset due to
 * unused indirect levels.
 */
#define	XMM_STATE_INDIRECT	4	/* (1<<4) entries per indirect level */
#define	XMM_STATE_DIRECT	6	/* (1<<6) entries per direct level */
#define	XMM_STATE_INDIRECT_SIZE	(1 << XMM_STATE_INDIRECT)
#define	XMM_STATE_DIRECT_SIZE	(1 << XMM_STATE_DIRECT)

union svm_state_bitmap {
	char			bits[XMM_STATE_DIRECT_SIZE];
	union svm_state_bitmap	*next[XMM_STATE_INDIRECT_SIZE];
};

typedef struct svm_state {
	unsigned		level;		/* first level bitmap */
	unsigned long		range;		/* first page range */
	union svm_state_bitmap	*bitmap;	/* kernel access bitmap */
} svm_state_data_t;

#define	X_MAP_GET(state, page)						\
	((state)->level == 0 ?						\
	 (state)->bitmap : svm_extend_get((state)->bitmap,		\
					  (page) - (state)->range,	\
					  (state)->level))

#define	_Ps_		3		/* shift for precious */
#define	_Ds_		4		/* shift for dirty or pending */
#define	_Ns_		5		/* shift for needs_copy */

#define	E_SUB_VAL(x, shift, val)	((x) & ~((val) << (shift)))
#define	E_ADD_VAL(x, shift, val)	((x) | ((val) << (shift)))
#define	E_ASN_VAL(x, shift, mask, val)	E_ADD_VAL(E_SUB_VAL(x, shift, mask),\
						  shift, val)

#define	X_GET_VAL(x, shift, mask)	((*(x) >> (shift)) & (mask))
#define	X_SUB_VAL(x, shift, val)	(*(x) = E_SUB_VAL(*(x), shift, val))
#define	X_ADD_VAL(x, shift, val)	(*(x) = E_ADD_VAL(*(x), shift, val))
#define	X_ASN_VAL(x, shift, mask, val)	(*(x) = E_ASN_VAL(*(x), shift, \
							  mask, val))

#define	X_GET_BIT(x, shift)		X_GET_VAL(x, shift, 1)
#define	X_CLR_BIT(x, shift)		X_SUB_VAL(x, shift, 1)
#define	X_SET_BIT(x, shift)		X_ADD_VAL(x, shift, 1)
#define	X_ASN_BIT(x, shift, bit)	X_ASN_VAL(x, shift, 1, bit)

#define	_VP_ALL			VM_PROT_ALL

/*
 *	M macros
 */

#define	M_MAP_GET(M, page, x_bits)					\
MACRO_BEGIN								\
	union svm_state_bitmap *_state = X_MAP_GET(&(M)->bits, (page));	\
	if (_state == (union svm_state_bitmap *)0)			\
		panic("XMM mobj (0x%x) with NULL status at page %d\n",	\
		      (M), (page));					\
	else								\
		(x_bits) = &_state->bits[(page) &			\
					 (XMM_STATE_DIRECT_SIZE - 1)];	\
MACRO_END

#define	M_GET_PRECIOUS(bitmap)		X_GET_BIT(bitmap, _Ps_)
#define	M_SET_PRECIOUS(bitmap)		X_SET_BIT(bitmap, _Ps_)
#define	M_CLR_PRECIOUS(bitmap)		X_CLR_BIT(bitmap, _Ps_)

#define	M_GET_DIRTY(bitmap)		X_GET_BIT(bitmap, _Ds_)
#define	M_SET_DIRTY(bitmap)		X_SET_BIT(bitmap, _Ds_)
#define	M_CLR_DIRTY(bitmap)		X_CLR_BIT(bitmap, _Ds_)

#define	M_GET_NEEDS_COPY(bitmap)	X_GET_BIT(bitmap, _Ns_)
#define	M_SET_NEEDS_COPY(bitmap)	X_SET_BIT(bitmap, _Ns_)
#define	M_CLR_NEEDS_COPY(bitmap)	X_CLR_BIT(bitmap, _Ns_)

#define	M_GET_LOCK(bitmap)		X_GET_VAL(bitmap, 0, _VP_ALL)
#define	M_SUB_LOCK(bitmap, p)		X_SUB_VAL(bitmap, 0, p)
#define	M_ASN_LOCK(bitmap, p)		X_ASN_VAL(bitmap, 0, _VP_ALL, p)

/*
 *	K macros
 */

#define	K_MAP_GET(K, page, x_bits)					\
MACRO_BEGIN								\
	union svm_state_bitmap *_state = X_MAP_GET(&(K)->bits, (page));	\
	if (_state == (union svm_state_bitmap *)0)			\
		(x_bits) = (char *)0;					\
	else								\
		(x_bits) = &_state->bits[(page) &			\
					 (XMM_STATE_DIRECT_SIZE - 1)];	\
MACRO_END

#define	K_GET_PROT(bitmap)		X_GET_VAL(bitmap, 0, _VP_ALL)
#define	K_SUB_PROT(bitmap, p)		X_SUB_VAL(bitmap, 0, p)
#define	K_ASN_PROT(bitmap, p)		X_ASN_VAL(bitmap, 0, _VP_ALL, p)

#define	K_GET_PRECIOUS(bitmap)		X_GET_BIT(bitmap, _Ps_)
#define	K_SET_PRECIOUS(bitmap)		X_SET_BIT(bitmap, _Ps_)
#define	K_CLR_PRECIOUS(bitmap)		X_CLR_BIT(bitmap, _Ps_)

#define	K_GET_PENDING(bitmap)		X_GET_BIT(bitmap, _Ds_)
#define	K_SET_PENDING(bitmap)		X_SET_BIT(bitmap, _Ds_)
#define	K_CLR_PENDING(bitmap)		X_CLR_BIT(bitmap, _Ds_)

#define	svm_extend_needed(m, page)					\
	(page < MM(m)->bits.range ||					\
	 page >= (MM(m)->bits.range +					\
		  (1 << (XMM_STATE_DIRECT + 				\
			 MM(m)->bits.level * XMM_STATE_INDIRECT))) ||	\
	 (MM(m)->bits.level == 0 ? MM(m)->bits.bitmap :			\
	  svm_extend_get(MM(m)->bits.bitmap,				\
			 (page) - MM(m)->bits.range,			\
			 MM(m)->bits.level))				\
	  == (union svm_state_bitmap *)0)

#define	svm_extend_if_needed(m, page)					\
	MACRO_BEGIN							\
	assert(xmm_obj_lock_held(m));					\
	assert((long)(page) >= 0);					\
	if (page >= MOBJ->num_pages)					\
		MOBJ->num_pages = page + 1;				\
	while (svm_extend_needed(m, page)) {				\
		if (!MM(m)->extend_in_progress) {			\
			svm_extend((m), (unsigned long)(page));		\
			break;						\
		}							\
		MM(m)->extend_wanted = 1;				\
		assert_wait(svm_extend_event(m), FALSE);		\
		xmm_obj_unlock(m);					\
		thread_block((void (*)(void)) 0);			\
		xmm_obj_lock(m);					\
	}								\
	MACRO_END

#define	svm_state_init(state)						\
	MACRO_BEGIN							\
	(state)->level = 0;						\
	(state)->range = 0;						\
	(state)->bitmap = (union svm_state_bitmap *)0;			\
	MACRO_END

#define	svm_state_exit(state)						\
	svm_extend_deallocate((state)->bitmap, (state)->level)

#define	svm_state_print(state)						\
	iprintf("state: level=%d, range=%d, bitmap=0x%x\n",		\
		(state)->level,						\
		(state)->range,						\
		(state)->bitmap)

extern union svm_state_bitmap	*svm_extend_get(
					union svm_state_bitmap	*sp,
					unsigned long		page,
					unsigned		level);

extern void			svm_extend_allocate(
					xmm_obj_t		mobj,
					xmm_obj_t		kobj);

extern void			svm_extend_deallocate(
					union svm_state_bitmap	*sp,
					unsigned		level);

extern void			svm_extend(
					xmm_obj_t		mobj,
					unsigned long		page);

extern void			svm_extend_new_copy(
					union svm_state_bitmap	*sp,
					unsigned		level,
					xmm_obj_t		mobj,
					unsigned long		page_base);

extern void			svm_extend_update_copy(
					union svm_state_bitmap	*sp,
					unsigned		level);

extern void			svm_extend_set_copy(
					union svm_state_bitmap	*sp,
					unsigned		level);

extern void			svm_extend_clear_copy(
					union svm_state_bitmap	*sp,
					unsigned		level);

#if	MACH_PAGEMAP
extern void			svm_extend_update_pagemap(
					union svm_state_bitmap	*sp,
					unsigned		level,
					unsigned long		range,
					xmm_obj_t		old_copy,
					unsigned long		page_base);
#endif	/* MACH_PAGEMAP */

extern vm_prot_t		m_get_prot(
					xmm_obj_t		mobj,
					unsigned int		page);

extern vm_prot_t		m_get_writer(
					xmm_obj_t		mobj,
					unsigned int		page,
					xmm_obj_t		*writer);

extern xmm_obj_t		m_get_precious_kernel(
					xmm_obj_t		mobj,
					unsigned int		page);

extern xmm_obj_t		m_get_nonprecious_kernel(
					xmm_obj_t		mobj,
					unsigned int		page);

extern xmm_obj_t		svm_get_active(
					xmm_obj_t		mobj);

extern int			svm_get_active_count(
					xmm_obj_t		mobj);

#if	MACH_ASSERT
extern boolean_t		m_check_kernel_prots(
					xmm_obj_t		mobj,
					unsigned int		page);
#endif	/* MACH_ASSERT */
