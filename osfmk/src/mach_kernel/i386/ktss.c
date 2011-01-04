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
 * Revision 2.5  91/05/14  16:10:37  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:12:39  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:35:28  mrt]
 * 
 * Revision 2.3  90/08/27  21:57:07  dbg
 * 	New selector names from new seg.h.
 * 	[90/07/25            dbg]
 * 
 * Revision 2.2  90/05/03  15:33:35  dbg
 * 	Created.
 * 	[90/02/15            dbg]
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
 * Kernel task state segment.
 *
 * We don't use the i386 task switch mechanism.  We need a TSS
 * only to hold the kernel stack pointer for the current thread.
 *
 * XXX multiprocessor??
 */
#include <i386/tss.h>
#include <i386/seg.h>
#include <mach_kdb.h>

struct i386_tss	ktss = {
	0,				/* back link */
	0,				/* esp0 */
	KERNEL_DS,			/* ss0 */
	0,				/* esp1 */
	0,				/* ss1 */
	0,				/* esp2 */
	0,				/* ss2 */
	0,				/* cr3 */
	0,				/* eip */
	0,				/* eflags */
	0,				/* eax */
	0,				/* ecx */
	0,				/* edx */
	0,				/* ebx */
	0,				/* esp */
	0,				/* ebp */
	0,				/* esi */
	0,				/* edi */
	0,				/* es */
	0,				/* cs */
	0,				/* ss */
	0,				/* ds */
	0,				/* fs */
	0,				/* gs */
	KERNEL_LDT,			/* ldt */
	0,				/* trace_trap */
	0x0FFF				/* IO bitmap offset -
					   beyond end of TSS segment,
					   so no bitmap */
};

#if	MACH_KDB

struct i386_tss	dbtss = {
	0,				/* back link */
	0,				/* esp0 */
	KERNEL_DS,			/* ss0 */
	0,				/* esp1 */
	0,				/* ss1 */
	0,				/* esp2 */
	0,				/* ss2 */
	0,				/* cr3 */
	0,				/* eip */
	0,				/* eflags */
	0,				/* eax */
	0,				/* ecx */
	0,				/* edx */
	0,				/* ebx */
	0,				/* esp */
	0,				/* ebp */
	0,				/* esi */
	0,				/* edi */
	KERNEL_DS,			/* es */
	KERNEL_CS,			/* cs */
	KERNEL_DS,			/* ss */
	KERNEL_DS,			/* ds */
	KERNEL_DS,			/* fs */
	KERNEL_DS,			/* gs */
	KERNEL_LDT,			/* ldt */
	0,				/* trace_trap */
	0x0FFF				/* IO bitmap offset -
					   beyond end of TSS segment,
					   so no bitmap */
};

#endif	/* MACH_KDB */
