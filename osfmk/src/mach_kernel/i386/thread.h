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
 * Revision 2.9.3.3  92/04/30  11:51:21  bernadat
 * 	Insert is_user_thread() and user_pc() macros for profiling
 * 	[92/04/14            bernadat]
 * 
 * Revision 2.9.3.2  92/03/28  10:05:45  jeffreyh
 * 	Changes from MK71
 * 	[92/03/20  12:14:01  jeffreyh]
 * 
 * Revision 2.11  92/03/03  14:22:42  rpd
 * 	Added dummy definition of syscall_emulation_sync.
 * 	[92/03/03            rpd]
 * 
 * Revision 2.10  92/01/03  20:09:15  dbg
 * 	Add lock to PCB to govern separate state fields (e.g.
 * 	floating-point status, io_tss, user_ldt).
 * 	[91/11/01            dbg]
 * 
 * 	Add user_ldt pointer to machine_state.
 * 	[91/08/20            dbg]
 * 
 * Revision 2.9  91/07/31  17:41:31  dbg
 * 	Save user regs directly in PCB on trap, and switch to separate
 * 	kernel stack.
 * 
 * 	Add fields for v86 interrupt simulation.
 * 	[91/07/30  16:58:18  dbg]
 * 
 * Revision 2.8  91/05/14  16:17:39  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/05/08  12:43:08  dbg
 * 	Change ktss to iopb_tss in pcb.
 * 	[91/04/26  14:39:10  dbg]
 * 
 * Revision 2.6  91/03/16  14:45:21  rpd
 * 	Removed k_ipl from i386_kernel_state.
 * 	[91/03/01            rpd]
 * 
 * 	Pulled i386_fpsave_state out of i386_machine_state.
 * 	[91/02/18            rpd]
 * 
 * 	Renamed unused field in i386_saved_state to cr2.
 * 	Removed switch_thread_context.
 * 	[91/02/05            rpd]
 * 
 * Revision 2.5  91/02/05  17:15:03  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:28  mrt]
 * 
 * Revision 2.4  91/01/09  22:41:49  rpd
 * 	Added dummy switch_thread_context macro.
 * 	Added ktss to i386_machine_state.
 * 	Removed user_regs, k_stack_top.
 * 	[91/01/09            rpd]
 * 
 * Revision 2.3  91/01/08  15:11:12  rpd
 * 	Added i386_machine_state.
 * 	[91/01/03  22:05:01  rpd]
 * 
 * 	Reorganized the pcb.
 * 	[90/12/11            rpd]
 * 
 * Revision 2.2  90/05/03  15:37:59  dbg
 * 	Created.
 * 	[90/02/08            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 *	File:	machine/thread.h
 *
 *	This file contains the structure definitions for the thread
 *	state as applied to I386 processors.
 */

#ifndef	_I386_THREAD_H_
#define _I386_THREAD_H_

#include <mach/boolean.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/fp_reg.h>

#include <kern/lock.h>

#include <i386/iopb.h>
#include <i386/seg.h>
#include <i386/tss.h>
#include <i386/eflags.h>
#include <i386/thread_act.h>

/*
 *	i386_exception_link:
 *
 *	This structure lives at the high end of the kernel stack.
 *	It points to the current thread`s user registers.
 */
struct i386_exception_link {
	struct i386_saved_state *saved_state;
};


/*
 *	On the kernel stack is:
 *	stack:	...
 *		struct i386_exception_link
 *		struct i386_kernel_state
 *	stack+KERNEL_STACK_SIZE
 */

#define STACK_IKS(stack)	\
	((struct i386_kernel_state *)((stack) + KERNEL_STACK_SIZE) - 1)
#define STACK_IEL(stack)	\
	((struct i386_exception_link *)STACK_IKS(stack) - 1)

#if	NCPUS > 1
#include <i386/mp_desc.h>
#endif

/*
 * Boot-time data for master (or only) CPU
 */
extern struct fake_descriptor	idt[IDTSZ];
extern struct fake_descriptor	gdt[GDTSZ];
extern struct fake_descriptor	ldt[LDTSZ];
extern struct i386_tss		ktss;
#if	MACH_KDB
extern char			db_stack_store[];
extern char			db_task_stack_store[];
extern struct i386_tss		dbtss;
extern void			db_task_start(void);
#endif	/* MACH_KDB */
#if	NCPUS > 1
#define	curr_gdt(mycpu)		(mp_gdt[mycpu])
#define	curr_ktss(mycpu)	(mp_ktss[mycpu])
#else
#define	curr_gdt(mycpu)		(gdt)
#define	curr_ktss(mycpu)	(&ktss)
#endif

#define	gdt_desc_p(mycpu,sel) \
	((struct real_descriptor *)&curr_gdt(mycpu)[sel_idx(sel)])

/*
 * Return address of the function that called current function, given
 *	address of the first parameter of current function.
 */
#define	GET_RETURN_PC(addr)	(*((vm_offset_t *)addr - 1))

/*
 * Defining this indicates that MD code will supply an exception()
 * routine, conformant with kern/exception.c (dependency alert!)
 * but which does wonderfully fast, machine-dependent magic.
 */
#define MACHINE_FAST_EXCEPTION 1

/*
 * MD Macro to fill up global stack state,
 * keeping the MD structure sizes + games private
 */
#define MACHINE_STACK_STASH(stack)                                      \
MACRO_BEGIN								\
	mp_disable_preemption();					\
	kernel_stack[cpu_number()] = (stack) +                          \
	    (KERNEL_STACK_SIZE - sizeof (struct i386_exception_link)    \
				- sizeof (struct i386_kernel_state)),   \
		active_stacks[cpu_number()] = (stack);			\
	mp_enable_preemption();						\
MACRO_END

#endif	/* _I386_THREAD_H_ */
