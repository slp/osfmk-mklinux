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
 * Revision 2.5.2.1  92/03/03  16:21:49  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  12:01:03  jeffreyh]
 * 
 * Revision 2.6  92/01/03  20:20:33  dbg
 * 	Drop back to 1-page kernel stacks, since emulation_vector calls
 * 	now pass data out-of-line.
 * 	[92/01/03            dbg]
 * 
 * Revision 2.5  91/11/19  08:08:35  rvb
 * 	NORMA needs a larger stack so we do it for everyone,
 * 	since stack space usage does not matter anymore.
 * 
 * Revision 2.4  91/05/14  16:52:50  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:32:30  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:10:30  mrt]
 * 
 * Revision 2.2  90/05/03  15:48:20  dbg
 * 	Move page-table definitions into i386/pmap.h.
 * 	[90/04/05            dbg]
 * 
 * 	Remove misleading comment about kernel stack size.
 * 	[90/02/05            dbg]
 * 
 * Revision 1.3  89/03/09  20:20:06  rpd
 * 	More cleanup.
 * 
 * Revision 1.2  89/02/26  13:01:13  gm0w
 * 	Changes for cleanup.
 * 
 * 31-Dec-88  Robert Baron (rvb) at Carnegie-Mellon University
 *	Derived from MACH2.0 vax release.
 *
 * 16-Jan-87  David Golub (dbg) at Carnegie-Mellon University
 *	Made vax_ptob return 'unsigned' instead of caddr_t.
 *
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 * Copyright (c) 1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 */

/*
 *	File:	vm_param.h
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1985
 *
 *	I386 machine dependent virtual memory parameters.
 *	Most of the declarations are preceeded by I386_ (or i386_)
 *	which is OK because only I386 specific code will be using
 *	them.
 */

#ifndef	_MACH_I386_VM_PARAM_H_
#define _MACH_I386_VM_PARAM_H_

#define BYTE_SIZE	8	/* byte size in bits */

#define I386_PGBYTES	4096	/* bytes per 80386 page */
#define I386_PGSHIFT	12	/* number of bits to shift for pages */

/*
 *	Convert bytes to pages and convert pages to bytes.
 *	No rounding is used.
 */

#define i386_btop(x)		(((unsigned)(x)) >> I386_PGSHIFT)
#define machine_btop(x)		i386_btop(x)
#define i386_ptob(x)		(((unsigned)(x)) << I386_PGSHIFT)

/*
 *	Round off or truncate to the nearest page.  These will work
 *	for either addresses or counts.  (i.e. 1 byte rounds to 1 page
 *	bytes.
 */

#define i386_round_page(x)	((((unsigned)(x)) + I386_PGBYTES - 1) & \
					~(I386_PGBYTES-1))
#define i386_trunc_page(x)	(((unsigned)(x)) & ~(I386_PGBYTES-1))

#define VM_MIN_ADDRESS		((vm_offset_t) 0)
#define VM_MAX_ADDRESS		((vm_offset_t) 0xc0000000U)

#define LINEAR_KERNEL_ADDRESS	((vm_offset_t) 0xc0000000)

#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0x00000000U)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0x3fffffffU)

#define VM_MIN_KERNEL_LOADED_ADDRESS	((vm_offset_t) 0x0c000000U)
#define VM_MAX_KERNEL_LOADED_ADDRESS	((vm_offset_t) 0x1fffffffU)

#ifdef	MACH_KERNEL

#include <norma_vm.h>
#include <xkmachkernel.h>
#include <task_swapper.h>
#include <thread_swapper.h>

#if defined(AT386)
#include <i386/cpuid.h>
#endif

#if !NORMA_VM && !XKMACHKERNEL
#if !TASK_SWAPPER && !THREAD_SWAPPER
#define KERNEL_STACK_SIZE	(I386_PGBYTES/2)
#else
/* stack needs to be a multiple of page size to get unwired  when swapped */
#define KERNEL_STACK_SIZE	(I386_PGBYTES)
#endif	/* TASK || THREAD SWAPPER */
#define INTSTACK_SIZE		(I386_PGBYTES)	/* interrupt stack size */
#else	/* NORMA_VM  && !XKMACHKERNEL */
#define KERNEL_STACK_SIZE	(I386_PGBYTES*2)
#define INTSTACK_SIZE		(I386_PGBYTES*2)	/* interrupt stack size */
#endif	/* NORMA_VM && !XKMACHKERNEL */

#endif	/* MACH_KERNEL */

/*
 *	Conversion between 80386 pages and VM pages
 */

#define trunc_i386_to_vm(p)	(atop(trunc_page(i386_ptob(p))))
#define round_i386_to_vm(p)	(atop(round_page(i386_ptob(p))))
#define vm_to_i386(p)		(i386_btop(ptoa(p)))

/*
 *	Physical memory is mapped 1-1 with virtual memory starting
 *	at VM_MIN_KERNEL_ADDRESS.
 */
#define phystokv(a)	((vm_offset_t)(a) + VM_MIN_KERNEL_ADDRESS)

/*
 *	For 386 only, ensure that pages are installed in the
 *	kernel_pmap with VM_PROT_WRITE enabled.  This avoids
 *	code in pmap_enter that disallows a read-only mapping
 *	in the kernel's pmap.  (See ri-osc CR1387.)
 *
 *	An entry in kernel_pmap is made only by the kernel or
 *	a collocated server -- by definition (;-)), the requester
 *	is trusted code.  If it asked for read-only access,
 *	it won't attempt a write.  We don't have to enforce the
 *	restriction.  (Naturally, this assumes that any collocated
 *	server will _not_ depend on trapping write accesses to pages
 *	mapped read-only; this cannot be made to work in the current
 *	i386-inspired pmap model.)
 */

#if defined(AT386)

#define PMAP_ENTER_386_CHECK \
	if (cpuid_family == CPUID_FAMILY_386)

#else

#define PMAP_ENTER_386_CHECK

#endif

#define PMAP_ENTER(pmap, virtual_address, page, protection, wired) \
	MACRO_BEGIN					\
	vm_prot_t __prot__ =				\
		(protection) & ~(page)->page_lock;	\
							\
    	PMAP_ENTER_386_CHECK				\
	if ((pmap) == kernel_pmap)			\
		__prot__ |= VM_PROT_WRITE;		\
	pmap_enter(					\
		(pmap),					\
		(virtual_address),			\
		(page)->phys_addr,			\
		__prot__,				\
		(wired)					\
	 );						\
	MACRO_END

#endif	/* _MACH_I386_VM_PARAM_H_ */
