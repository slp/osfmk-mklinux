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
 * 
 */
/*
 * MkLinux
 */
#ifndef	_VM_CPM_H_
#define	_VM_CPM_H_

/*
 *	File:	vm/cpm.h
 *	Author:	Alan Langerman
 *	Date:	April 1995 and January 1996
 *
 *	Contiguous physical memory allocator.
 */

#include <mach_kdb.h>
#include <mach_counters.h>

/*
 *	Return a linked list of physically contiguous
 *	wired pages.  Caller is responsible for disposal
 *	via cpm_release.
 *
 *	These pages are all in "gobbled" state when .
 */
extern kern_return_t
	cpm_allocate(vm_size_t size, vm_page_t *list, boolean_t wire);

/*
 *	CPM-specific event counters.
 */
#define	VM_CPM_COUNTERS		(MACH_KDB && MACH_COUNTERS && VM_CPM)
#if	VM_CPM_COUNTERS
#define	cpm_counter(foo)	foo
#else	/* VM_CPM_COUNTERS */
#define	cpm_counter(foo)
#endif	/* VM_CPM_COUNTERS */

#endif	/* _VM_CPM_H_ */
