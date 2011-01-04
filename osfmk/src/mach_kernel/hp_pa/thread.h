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
/* 
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: thread.h 1.13 92/02/02$
 */

/*
 *      This file defines machine specific, thread related structures,
 *      variables and macros.
 */

#ifndef	_HP_PA_THREAD_H_
#define _HP_PA_THREAD_H_

#ifndef ASSEMBLER

#include <mach/kern_return.h>

/*
 * pcb_synch is unneeded on the HP700.
 */
#define pcb_synch(thread)

extern vm_offset_t getrpc(void);
#define	GET_RETURN_PC(addr)	getrpc()

#define STACK_IKS(stack) (stack)

#endif /* ASSEMBLER */

/*
 * Save State Flags
 */
#define	SS_INTRAP	0x01	/* State saved from trap */
#define	SS_INSYSCALL	0x02	/* State saved from system call */
#define SS_ININT	0x04	/* On the interrupt stack */
#define	SS_PSPKERNEL	0x08	/* Previous context stack pointer kernel */
#define	SS_RFIRETURN	0x10	/* Must RFI from syscall to restore
				   complete user state (i.e. sigreturn) */

#define	SS_INTRAP_POS	 31	/* State saved from trap */
#define	SS_INSYSCALL_POS 30	/* State saved from system call */
#define SS_ININT_POS	 29	/* On the interrupt stack */
#define	SS_PSPKERNEL_POS 28	/* Previous context stack pointer kernel */
#define	SS_RFIRETURN_POS 27	/* RFI from syscall */

#define syscall_emulation_sync(task)	/* do nothing */


/*
 * Defining this indicates that MD code will supply an exception()
 * routine, conformant with kern/exception.c (dependency alert!)
 * but which does wonderfully fast, machine-dependent magic.
 */
#define MACHINE_FAST_EXCEPTION 1

#endif	/* _HP_PA_THREAD_H_ */
