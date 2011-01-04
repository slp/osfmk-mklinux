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
 *	File:	norma/xmm.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Common xmm support routines.
 */

#ifdef	MACH_KERNEL
#include <norma/xmm_obj.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#else
#include "xmm_obj.h"
#endif

kern_return_t
xmm_obj_allocate(class, old_mobj, new_mobj)
	xmm_class_t class;
	xmm_obj_t old_mobj;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;

	if (class->c_zone == ZONE_NULL) {
		char *zone_name;

		zone_name = (char *) kalloc(strlen(class->c_name) + 5);
		bcopy("xmm.", zone_name, 4);
		bcopy(class->c_name, zone_name + 4, strlen(class->c_name) + 1);
		class->c_zone = zinit(class->c_size, 512*1024, class->c_size,
				      FALSE, zone_name);
	}
	mobj = (xmm_obj_t) zalloc(class->c_zone);
	bzero((char *)mobj, class->c_size);
	mobj->class = class;
	mobj->refcount = 1;
	mobj->m_kobj = XMM_OBJ_NULL;
	mobj->k_kobj = XMM_OBJ_NULL;
	mobj->m_mobj = old_mobj;
	if (old_mobj) {
		old_mobj->k_mobj = mobj;
	}
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

xmm_obj_deallocate(mobj)
	xmm_obj_t mobj;
{
	xmm_class_t class;
	extern struct xmm_class invalid_mclass;

	/*
	 * Tag it as invalid before freeing for debugging.
	 *
	 * Terminate protocol:
	 * Each layer terminates layer beneath it before deallocating.
	 * Bottom layers disable outside upcalls before blocking.
	 * (For example, xmm_user.c does kobject_set(NULL) before
	 * calling memory_object_terminate.)
	 */
	class = mobj->class;
	mobj->class = &invalid_mclass;
	mobj->refcount = (int) class->c_name;
	zfree(class->c_zone, mobj);
}

xmm_init()
{
	xmm_svm_init();
	xmm_split_init();
}
