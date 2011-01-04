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
 * Revision 2.6.4.2  92/04/08  15:45:18  jeffreyh
 * 	Back out changes from trunk. Back to a revision 2.6 base.
 * 	[92/04/07  10:32:44  jeffreyh]
 * 
 * Revision 2.6  91/08/28  11:15:44  jsb
 * 	single_use --> use_old_pageout to match vm_object.h
 * 	[91/08/05  17:41:53  dlb]
 * 
 * Revision 2.5  91/05/14  17:04:08  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:38:13  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:29:30  mrt]
 * 
 * Revision 2.3  90/10/12  18:07:41  rpd
 * 	Removed the vir_copy_on_write field.
 * 	[90/10/08            rpd]
 * 
 * Revision 2.2  90/06/02  15:00:49  rpd
 * 	Created.
 * 	[90/04/20            rpd]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 *	File:	mach_debug/vm_info.h
 *	Author:	Rich Draves
 *	Date:	March, 1990
 *
 *	Definitions for the VM debugging interface.
 */

#ifndef	_MACH_DEBUG_VM_INFO_H_
#define _MACH_DEBUG_VM_INFO_H_

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_inherit.h>
#include <mach/vm_prot.h>
#include <mach/memory_object.h>

/*
 *	Remember to update the mig type definitions
 *	in mach_debug_types.defs when adding/removing fields.
 */

typedef struct vm_info_region {
	vm_offset_t vir_start;		/* start of region */
	vm_offset_t vir_end;		/* end of region */
	vm_offset_t vir_object;		/* the mapped object */
	vm_offset_t vir_offset;		/* offset into object */
	boolean_t vir_needs_copy;	/* does object need to be copied? */
	vm_prot_t vir_protection;	/* protection code */
	vm_prot_t vir_max_protection;	/* maximum protection */
	vm_inherit_t vir_inheritance;	/* inheritance */
	natural_t vir_wired_count;	/* number of times wired */
	natural_t vir_user_wired_count; /* number of times user has wired */
} vm_info_region_t;


typedef struct vm_info_object {
	vm_offset_t vio_object;		/* this object */
	vm_size_t vio_size;		/* object size (valid if internal) */
	unsigned int vio_ref_count;	/* number of references */
	unsigned int vio_resident_page_count; /* number of resident pages */
	unsigned int vio_absent_count;	/* number requested but not filled */
	vm_offset_t vio_copy;		/* copy object */
	vm_offset_t vio_shadow;		/* shadow object */
	vm_offset_t vio_shadow_offset;	/* offset into shadow object */
	vm_offset_t vio_paging_offset;	/* offset into memory object */
	memory_object_copy_strategy_t vio_copy_strategy;
					/* how to handle data copy */
	vm_offset_t vio_last_alloc;	/* offset of last allocation */
	/* many random attributes */
	unsigned int vio_paging_in_progress;
	boolean_t vio_pager_created;
	boolean_t vio_pager_initialized;
	boolean_t vio_pager_ready;
	boolean_t vio_can_persist;
	boolean_t vio_internal;
	boolean_t vio_temporary;
	boolean_t vio_alive;
	boolean_t vio_lock_in_progress;
	boolean_t vio_lock_restart;
} vm_info_object_t;

typedef vm_info_object_t *vm_info_object_array_t;

#endif	/* _MACH_DEBUG_VM_INFO_H_ */
