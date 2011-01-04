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
 * Revision 2.11.7.3  92/04/30  11:49:54  bernadat
 * 	Support for SystemPro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.11.7.2  92/03/03  16:14:19  jeffreyh
 * 	Pick up changes from TRUNK
 * 	[92/02/26  11:04:46  jeffreyh]
 * 
 * Revision 2.12  92/01/03  20:04:43  dbg
 * 	Floating-point state is now saved by caller.
 * 	[91/10/20            dbg]
 * 
 * Revision 2.11  91/07/31  17:34:56  dbg
 * 	Push user state directly in PCB and switch to kernel stack on
 * 	trap entry.
 * 	[91/07/30  16:48:18  dbg]
 * 
 * Revision 2.10  91/06/19  11:54:57  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:44:37  rvb]
 * 
 * Revision 2.9  91/05/14  16:04:42  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/05/08  12:31:19  dbg
 * 	Put parentheses around substituted immediate expressions, so
 * 	that they will pass through the GNU preprocessor.
 * 
 * 	Handle multiple CPUs.  Save floating-point state when saving
 * 	context or switching stacks.  Add switch_to_shutdown_context to
 * 	handle CPU shutdown.
 * 	[91/03/21            dbg]
 * 
 * Revision 2.7  91/03/16  14:43:51  rpd
 * 	Renamed Switch_task_context to Switch_context.
 * 	[91/02/17            rpd]
 * 	Added active_stacks.
 * 	[91/01/29            rpd]
 * 
 * Revision 2.6  91/02/05  17:10:57  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:30:50  mrt]
 * 
 * Revision 2.5  91/01/09  22:41:09  rpd
 * 	Renamed to Load_context and Switch_task_context.
 * 	Removed ktss munging.
 * 	[91/01/09            rpd]
 * 
 * Revision 2.4  91/01/08  15:10:09  rpd
 * 	Minor cleanup.
 * 	[90/12/31            rpd]
 * 	Added switch_task_context, switch_thread_context.
 * 	[90/12/12            rpd]
 * 
 * 	Reorganized the pcb.
 * 	[90/12/11            rpd]
 * 
 * Revision 2.3  90/12/20  16:35:48  jeffreyh
 * 	Changes for __STDC__
 * 
 * 	Changes for __STDC__
 * 	[90/12/07  15:45:37  jeffreyh]
 * 
 *
 * Revision 2.2  90/05/03  15:25:00  dbg
 * 	Created.
 * 	[90/02/08            dbg]
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

#include <cpus.h>
#include <platforms.h>

#include <i386/asm.h>
#include <i386/proc_reg.h>
#include <assym.s>

#if	NCPUS > 1

#ifdef	SYMMETRY
#include <sqt/asm_macros.h>
#endif

#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

#ifdef	MBUS
#include <busses/mbus/mbus.h>
#endif	/* MBUS */

#define	CX(addr, reg)	addr(,reg,4)

#else	/* NCPUS == 1 */

#define	CPU_NUMBER(reg)
#define	CX(addr,reg)	addr

#endif	/* NCPUS == 1 */

/*
 * Context switch routines for i386.
 */

Entry(Load_context)
	movl	S_ARG0,%ecx			/ get thread
	movl	TH_KERNEL_STACK(%ecx),%ecx	/ get kernel stack
	lea	KERNEL_STACK_SIZE-IKS_SIZE-IEL_SIZE(%ecx),%edx
						/ point to stack top
	CPU_NUMBER(%eax)
	movl	%ecx,CX(EXT(active_stacks),%eax) / store stack address
	movl	%edx,CX(EXT(kernel_stack),%eax)	/ store stack top

	movl	KSS_ESP(%ecx),%esp		/ switch stacks
	movl	KSS_ESI(%ecx),%esi		/ restore registers
	movl	KSS_EDI(%ecx),%edi
	movl	KSS_EBP(%ecx),%ebp
	movl	KSS_EBX(%ecx),%ebx
	xorl	%eax,%eax			/ return zero (no old thread)
	jmp	*KSS_EIP(%ecx)			/ resume thread

/*
 *	This really only has to save registers
 *	when there is no explicit continuation.
 */

Entry(Switch_context)
	CPU_NUMBER(%edx)
	movl	CX(EXT(active_stacks),%edx),%ecx / get old kernel stack

	movl	%ebx,KSS_EBX(%ecx)		/ save registers
	movl	%ebp,KSS_EBP(%ecx)
	movl	%edi,KSS_EDI(%ecx)
	movl	%esi,KSS_ESI(%ecx)
	popl	KSS_EIP(%ecx)			/ save return PC
	movl	%esp,KSS_ESP(%ecx)		/ save SP

	movl	0(%esp),%eax			/ get old thread
	movl	4(%esp),%ebx			/ get continuation
	movl	%ebx,TH_CONTINUATION(%eax)	/ save continuation

	movl	8(%esp),%esi			/ get new thread
        movl    $CPD_ACTIVE_THREAD,%ecx
        movl    %esi,%gs:(%ecx)                 / new thread is active
	movl	TH_KERNEL_STACK(%esi),%ecx	/ get its kernel stack
	lea	KERNEL_STACK_SIZE-IKS_SIZE-IEL_SIZE(%ecx),%ebx
						/ point to stack top

	movl	%ecx,CX(EXT(active_stacks),%edx) / set current stack
	movl	%ebx,CX(EXT(kernel_stack),%edx)	/ set stack top

	movl	TH_TOP_ACT(%esi),%esi		/ get new_thread->top_act
	cmpl	$0,ACT_KLOADED(%esi)		/ check kernel-loaded flag
	je	0f
	movl	%esi,CX(EXT(active_kloaded),%edx)
	jmp	1f
0:
	movl	$0,CX(EXT(active_kloaded),%edx)
1:
	movl	KSS_ESP(%ecx),%esp		/ switch stacks
	movl	KSS_ESI(%ecx),%esi		/ restore registers
	movl	KSS_EDI(%ecx),%edi
	movl	KSS_EBP(%ecx),%ebp
	movl	KSS_EBX(%ecx),%ebx
	jmp	*KSS_EIP(%ecx)			/ return old thread

Entry(Thread_continue)
	pushl	%eax				/ push the thread argument
	xorl	%ebp,%ebp			/ zero frame pointer
	call	*%ebx				/ call real continuation

#if	NCPUS > 1
/*
 * void switch_to_shutdown_context(thread_t thread,
 *				   void (*routine)(processor_t),
 *				   processor_t processor)
 *
 * saves the kernel context of the thread,
 * switches to the interrupt stack,
 * continues the thread (with thread_continue),
 * then runs routine on the interrupt stack.
 *
 * Assumes that the thread is a kernel thread (thus
 * has no FPU state)
 */
Entry(switch_to_shutdown_context)
	CPU_NUMBER(%edx)
	movl	EXT(active_stacks)(,%edx,4),%ecx / get old kernel stack
	movl	%ebx,KSS_EBX(%ecx)		/ save registers
	movl	%ebp,KSS_EBP(%ecx)
	movl	%edi,KSS_EDI(%ecx)
	movl	%esi,KSS_ESI(%ecx)
	popl	KSS_EIP(%ecx)			/ save return PC
	movl	%esp,KSS_ESP(%ecx)		/ save SP

	movl	0(%esp),%eax			/ get old thread
	movl	$0,TH_CONTINUATION(%eax)	/ clear continuation
	movl	%ecx,TH_KERNEL_STACK(%eax)	/ save old stack
	movl	4(%esp),%ebx			/ get routine to run next
	movl	8(%esp),%esi			/ get its argument

	movl	CX(EXT(interrupt_stack),%edx),%ecx / point to its intr stack
	lea	INTSTACK_SIZE(%ecx),%esp	/ switch to it (top)
	
	pushl	%eax				/ push thread
	call	EXT(thread_dispatch)		/ reschedule thread
	addl	$4,%esp				/ clean stack

	pushl	%esi				/ push argument
	call	*%ebx				/ call routine to run
	hlt					/ (should never return)

#endif	/* NCPUS > 1 */

        .text
LEXT(locore_end)

