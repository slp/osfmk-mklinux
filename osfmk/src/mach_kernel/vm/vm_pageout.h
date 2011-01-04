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
 * Revision 2.4  91/05/14  17:51:11  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  18:00:18  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:34:35  mrt]
 * 
 * Revision 2.2  90/06/02  15:12:02  rpd
 * 	Added declaration of vm_pageout_setup.
 * 	[90/05/31            rpd]
 * 
 * 	Changes for MACH_KERNEL:
 * 	. Remove non-XP declarations.
 * 	[89/04/28            dbg]
 * 
 * Revision 2.1  89/08/03  16:45:59  rwd
 * Created.
 * 
 * Revision 2.9  89/04/18  21:28:02  mwyoung
 * 	No relevant history.
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
 *	File:	vm/vm_pageout.h
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1986
 *
 *	Declarations for the pageout daemon interface.
 */

#ifndef	_VM_VM_PAGEOUT_H_
#define _VM_VM_PAGEOUT_H_

#include <norma_vm.h>

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>

/*
 *	The following ifdef only exists because XMM must (currently)
 *	be given a page at a time.  This should be removed
 *	in the future.
 */
#if	NORMA_VM
#define	DATA_WRITE_MAX	1
#define	POINTER_T(copy)	(copy)
#else	/* NORMA_VM */
#define	DATA_WRITE_MAX	32
#define	POINTER_T(copy)	(pointer_t)(copy)
#endif	/* NORMA_VM */


/*
 *	Exported routines.
 */
extern void		vm_pageout(void);

extern vm_object_t	vm_pageout_object_allocate(
					vm_page_t	m,
					vm_size_t	size,
					vm_offset_t	offset);

extern void		vm_pageout_object_terminate(
					vm_object_t	object);

extern vm_page_t	vm_pageout_setup(
					vm_page_t	m,
					vm_object_t	new_object,
					vm_offset_t	new_offset);

extern void		vm_pageout_cluster(
					vm_page_t	m);

extern void		vm_pageout_initialize_page(
					vm_page_t	m);

extern void		vm_pageclean_setup(
					vm_page_t	m,
					vm_page_t	new_m,
					vm_object_t	new_object,
					vm_offset_t	new_offset);

extern void		vm_pageclean_copy(
					vm_page_t	m,
					vm_page_t	new_m,
					vm_object_t	new_object,
					vm_offset_t	new_offset);

#endif	/* _VM_VM_PAGEOUT_H_ */
