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
 * Revision 2.6  91/05/18  14:40:15  rpd
 * 	Added VM_FAULT_FICTITIOUS_SHORTAGE.
 * 	[91/03/29            rpd]
 * 
 * Revision 2.5  91/05/14  17:48:59  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/03/16  15:05:03  rpd
 * 	Added vm_fault_init.
 * 	[91/02/16            rpd]
 * 
 * Revision 2.3  91/02/05  17:58:12  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:32:04  mrt]
 * 
 * Revision 2.2  90/02/22  20:05:32  dbg
 * 	Add vm_fault_copy(), vm_fault_cleanup().  Remove
 * 	vm_fault_copy_entry().
 * 	[90/01/25            dbg]
 * 
 * Revision 2.1  89/08/03  16:44:57  rwd
 * Created.
 * 
 * Revision 2.6  89/04/18  21:25:22  mwyoung
 * 	Reset history.
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
 *	File:	vm/vm_fault.h
 *
 *	Page fault handling module declarations.
 */

#ifndef	_VM_VM_FAULT_H_
#define _VM_VM_FAULT_H_

#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/vm_param.h>
#include <mach/vm_behavior.h>
#include <vm/vm_page.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>

typedef	kern_return_t	vm_fault_return_t;
#define VM_FAULT_SUCCESS		0
#define VM_FAULT_RETRY			1
#define VM_FAULT_INTERRUPTED		2
#define VM_FAULT_MEMORY_SHORTAGE 	3
#define VM_FAULT_FICTITIOUS_SHORTAGE 	4
#define VM_FAULT_MEMORY_ERROR		5

extern void vm_fault_init(void);

/*
 *	Page fault handling based on vm_object only.
 */

extern vm_fault_return_t vm_fault_page(
		/* Arguments: */
		vm_object_t	first_object,	/* Object to begin search */
		vm_offset_t	first_offset,	/* Offset into object */
		vm_prot_t	fault_type,	/* What access is requested */
		boolean_t	must_be_resident,/* Must page be resident? */
		boolean_t	interruptible,	/* May fault be interrupted? */
		vm_offset_t	lo_offset,	/* Map entry start */
		vm_offset_t	hi_offset,	/* Map entry end */
		vm_behavior_t	behavior,	/* Expected paging behavior */
		/* Modifies in place: */
		vm_prot_t	*protection,	/* Protection for mapping */
		/* Returns: */
		vm_page_t	*result_page,	/* Page found, if successful */
		vm_page_t	*top_page,	/* Page in top object, if
						 * not result_page.  */
		/* More arguments: */
		kern_return_t	*error_code,	/* code if page is in error */
		boolean_t	no_zero_fill,	/* don't fill absent pages */
		boolean_t	data_supply);	/* treat as data_supply */

extern void vm_fault_cleanup(
		vm_object_t	object,
		vm_page_t	top_page);
/*
 *	Page fault handling based on vm_map (or entries therein)
 */

extern kern_return_t vm_fault(
		vm_map_t	map,
		vm_offset_t	vaddr,
		vm_prot_t	fault_type,
		boolean_t	change_wiring);

extern kern_return_t vm_fault_wire(
		vm_map_t	map,
		vm_map_entry_t	entry);

extern void vm_fault_unwire(
		vm_map_t	map,
		vm_map_entry_t	entry,
		boolean_t	deallocate);

extern kern_return_t	vm_fault_copy(
		vm_object_t	src_object,
		vm_offset_t	src_offset,
		vm_size_t	*src_size,	/* INOUT */
		vm_object_t	dst_object,
		vm_offset_t	dst_offset,
		vm_map_t	dst_map,
		vm_map_version_t *dst_version,
		boolean_t	interruptible);

#endif	/* _VM_VM_FAULT_H_ */
