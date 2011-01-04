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
 * 	Utah $Hdr: cswtch.s 1.16 92/04/27$
 *	Author: Bob Wheeler & Leigh Stoller of University of Utah CSS
 */

#include <machine/asm.h>
#include <assym.s>

#include <mach/machine/vm_param.h>
#include <machine/debug.h>
#include <machine/break.h>

	.space	$PRIVATE$
	.subspa	$DATA$

	.import cpu_data
	.import active_stacks
	.import active_pcbs
	.import active_kloaded
	.import fpu_pcb
	.import intstack_base
	.import istackptr

	.space	$TEXT$
	.subspa $CODE$

/*
 * void load_context(new_thread)
 *	thread	*new_thread
 *
 * Load the context for the first kernel thread, and go.
 */
	.export	load_context,entry
	.export	Load_context,entry
	.proc
	.callinfo no_calls

Load_context
load_context

	/*
	 * Since this is the first thread, we came in on the interrupt
	 * stack. The first thread never returns, so there is no need to
	 * worry about saving its frame, hence we can reset the istackptr
	 * back to its base.
	 */

	ldil	L%intstack_base,t1
	ldo	R%intstack_base(t1),t1

	ldil	L%istackptr,t2
	stw	t1,R%istackptr(t2)

	/*
	 * get new thread pointer and set it into the active_threads pointer
	 *
	 * Never set any break points in these instructions since it
	 * may cause a kernel stack overflow.
	 */

	ldil	L%cpu_data,t1
	stw	arg0,R%cpu_data(t1)

	ldw	THREAD_KERNEL_STACK(arg0),t3
	ldil	L%active_stacks,t1
	ldw	KS_R30(t3),sp

	stw	t3,R%active_stacks(t1)

	/*
	 * This is the end of the no break points region.
	 */

	/*
	 * restore rp
	 */
	ldw	KS_R2(t3),r2

	/* 
	 * restore all of the callee save registers 
	 */
	ldw	KS_R3(t3),r3
	ldw	KS_R4(t3),r4
	ldw	KS_R5(t3),r5
	ldw	KS_R6(t3),r6
	ldw	KS_R7(t3),r7
	ldw	KS_R8(t3),r8
	ldw	KS_R9(t3),r9
	ldw	KS_R10(t3),r10
	ldw	KS_R11(t3),r11
	ldw	KS_R12(t3),r12
	ldw	KS_R13(t3),r13
	ldw	KS_R14(t3),r14
	ldw	KS_R15(t3),r15
	ldw	KS_R16(t3),r16
	ldw	KS_R17(t3),r17
		
	/*
	 * Since this is the first thread, we make sure that thread_continue
	 * gets a zero as its argument.
	 */
	copy	r0,arg0

	bv	0(r2)
	ldw	KS_R18(t3),r18		/* Moved down from above.  */
	.procend


/*
 *  thread_t switch_context(old_thread, continuation, new_thread)
 *	thread_t	old_thread, new_thread
 *	void 		(*continuation)()
 *
 * Switch from one thread to another. If a continuation is supplied, then
 * we do not need to save callee save registers.
 */
#ifdef DEBUG
	.export	Switch_context,entry
	.proc
	.callinfo

Switch_context
#else
	.export	switch_context,entry
	.proc
	.callinfo

switch_context
#endif /* DEBUG */
	ldil	L%active_stacks,t1
	ldw	R%active_stacks(t1),t3
	stw	arg1,THREAD_CONTINUATION(arg0)	/* XXX */

	/*
	 * Save all the callee save registers plus state.
	 */
	stw	r2,KS_R2(t3)
	stw	r30,KS_R30(t3)

	stw	r3,KS_R3(t3)
	stw	r4,KS_R4(t3)
	stw	r5,KS_R5(t3)
	stw	r6,KS_R6(t3)
	stw	r7,KS_R7(t3)
	stw	r8,KS_R8(t3)
	stw	r9,KS_R9(t3)
	stw	r10,KS_R10(t3)
	stw	r11,KS_R11(t3)
	stw	r12,KS_R12(t3)
	stw	r13,KS_R13(t3)
	stw	r14,KS_R14(t3)
	stw	r15,KS_R15(t3)
	stw	r16,KS_R16(t3)
	stw	r17,KS_R17(t3)
	stw	r18,KS_R18(t3)

	/*
	 * Make the new thread the current thread.
	 */
	ldil	L%cpu_data,t1
	stw	arg2,R%cpu_data(t1)

	ldw	THREAD_KERNEL_STACK(arg2),t3

	ldil	L%active_stacks,t1
	stw	t3,R%active_stacks(t1)

	ldw	THREAD_TOP_ACT(arg2),arg2
	ldw	ACT_PCB(arg2),t2
	ldil	L%active_pcbs,t1
	stw	t2,R%active_pcbs(t1)


	ldw	ACT_KLOADED(arg2),t1
	combt,= r0,t1,$0
	ldil	L%active_kloaded,t1

	b	$1
	stw	arg2,R%active_kloaded(t1)

$0
	stw	r0,R%active_kloaded(t1)
$1

	/*
	 * Restore all the callee save registers.
	 */
	ldw	KS_R2(t3),r2
	ldw	KS_R30(t3),sp

	ldw	KS_R3(t3),r3
	ldw	KS_R4(t3),r4
	ldw	KS_R5(t3),r5
	ldw	KS_R6(t3),r6
	ldw	KS_R7(t3),r7
	ldw	KS_R8(t3),r8
	ldw	KS_R9(t3),r9
	ldw	KS_R10(t3),r10
	ldw	KS_R11(t3),r11
	ldw	KS_R12(t3),r12
	ldw	KS_R13(t3),r13
	ldw	KS_R14(t3),r14
	ldw	KS_R15(t3),r15
	ldw	KS_R16(t3),r16
	ldw	KS_R17(t3),r17

	/*
	 * Disable the FPU so the new thread traps when trying to use it.
	 */
	mtctl	r0,ccr
		
	/*
	 * Store the old thread in ret0 for switch_context, and leave in
	 * arg0 for thread_continue.
	 */
	copy	arg0,ret0
	bv	0(r2)
	ldw	KS_R18(t3),r18		/* Moved down from above.  */

	.procend


	.end
