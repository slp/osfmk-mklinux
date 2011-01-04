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
 *	File:	xmm_maycache.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class to set cacheability.
 */

#include <mach.h>
#include "xmm_obj.h"

struct kobj {
	struct xmm_obj	obj;
	boolean_t	may_cache;
};

struct mobj {
	struct xmm_obj	obj;
	boolean_t	may_cache;
};

kern_return_t m_maycache_init();
kern_return_t k_maycache_set_attributes();

xmm_decl(maycache_mclass,
	/* m_init		*/	m_maycache_init,
	/* m_terminate		*/	m_interpose_terminate,
	/* m_copy		*/	m_interpose_copy,
	/* m_data_request	*/	m_interpose_data_request,
	/* m_data_unlock	*/	m_interpose_data_unlock,
	/* m_data_write		*/	m_interpose_data_write,
	/* m_lock_completed	*/	m_interpose_lock_completed,
	/* m_supply_completed	*/	m_interpose_supply_completed,
	/* m_data_return	*/	m_interpose_data_return,

	/* k_data_provided	*/	k_invalid_data_provided,
	/* k_data_unavailable	*/	k_invalid_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_invalid_data_error,
	/* k_set_attributes	*/	k_invalid_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"maycache",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(maycache_kclass,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_interpose_data_provided,
	/* k_data_unavailable	*/	k_interpose_data_unavailable,
	/* k_get_attributes	*/	k_interpose_get_attributes,
	/* k_lock_request	*/	k_interpose_lock_request,
	/* k_data_error		*/	k_interpose_data_error,
	/* k_set_attributes	*/	k_maycache_set_attributes,
	/* k_destroy		*/	k_interpose_destroy,
	/* k_data_supply	*/	k_interpose_data_supply,

	/* name			*/	"maycache",
	/* size			*/	sizeof(struct kobj)
);

kern_return_t
xmm_maycache_create(old_mobj, may_cache, new_mobj)
	xmm_obj_t old_mobj;
	boolean_t may_cache;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&maycache_mclass, old_mobj, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->may_cache = may_cache;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

m_maycache_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	xmm_obj_t kobj;

	/* XXX this stuff should be automated once I understand it */
	/* XXX eg have either a kobj allocation routine in xxx_class,
	   XXX or if it is null, reject multiple kobjs;
	   XXX also, if not null, keep list of kobjs to detect repeats?
	   XXX could repeats happen? should that be up to atrium?
	*/
	xmm_obj_allocate(&maycache_kclass, XMM_OBJ_NULL, &kobj);
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;
	KOBJ->may_cache = MOBJ->may_cache;
	M_INIT(mobj, kobj, memory_object_name, page_size);
}

k_maycache_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
#if     lint
	may_cache++;
#endif  /* lint */
	K_SET_ATTRIBUTES(kobj, object_ready, KOBJ->may_cache, copy_strategy);
}
