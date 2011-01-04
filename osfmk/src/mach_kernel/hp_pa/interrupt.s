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
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: interrupt.s 1.39 94/12/14$
 */

#include <cpus.h>
#include <mach_rt.h>
#include <machine/asm.h>
#include <assym.s>
#include <machine/thread.h>
#include <machine/trap.h>
#include <machine/psw.h>
#include <machine/spl.h>
#include <hp_pa/pte.h>
#include <machine/pmap.h>
#include <mach/machine/vm_param.h>
#include <kgdb.h>

/*
 * Data space variables imported by this file.
 */

	.space	$PRIVATE$
	.subspa	$DATA$

	.import istackptr
	.import $global$
	.import	cpu_data
	.import active_stacks
	.import active_pcbs
	.import	active_kloaded
	.import intstack_top
	.import	need_ast

/*
 * Allocate hp700_trap_status area
 */
active_traps
	.blockz TRAP_SIZE * NCPUS

#ifdef DEBUG
/* zero indicates interrupt stack has overflowed */
istk_overflow
	.word	1
#endif

	.space $TEXT$
	.subspa $CODE$

/*
 * ihandler(type)
 *
 * Build a saved state structure on the interrupt stack, switch the processor
 * into virtual addressing mode and call the routine interrupt().
 */

#if MACH_RT
	.import kernel_ast
#endif
	.import hp_pa_astcheck

#if KGDB
#define TF_SIZE		roundup((FM_SIZE+FM_SIZE+SS_SIZE+ARG_SIZE),FR_ROUND)
#else
#define TF_SIZE		roundup((FM_SIZE+SS_SIZE+ARG_SIZE),FR_ROUND)
#endif

#define KERNEL_STACK_MASK  (KERNEL_STACK_SIZE-1)


ROUNDED_SS_SIZE	.equ TF_SIZE-FM_SIZE-ARG_SIZE


	.export	ihandler
	.proc
	.callinfo calls, frame=ROUNDED_SS_SIZE, save_sp, hpux_int

ihandler

	/*
	 * get the value of istackptr, if it's zero then we're already on the
	 * interrupt stack.
	 */

	ldil	L%istackptr,t1
	ldw	R%istackptr(t1),sp
	comb,<>,n	sp,r0,$istackatbase

	/*
	 * We're already on the interrupt stack, get the old stack pointer back
	 * that we shoved off into tr3. Then put a value into t1 to
	 * indicate that we are in the situation that the previous stack
	 * pointer was the kernel stack and we are in an interrupt.
	 */

	mfctl	tr3,sp
	movib,tr SS_PSPKERNEL+SS_ININT,t1,$istacknotatbase
	/* XXX note branch is not nullified.  */

$istackatbase

	/* Allocate a stack frame.  */
	ldo	TF_SIZE(sp),sp
	/*
	 * We're not on the interrupt stack. The value of istackptr is the
	 * interrupt stack.
	 */

	stw	r0,R%istackptr(t1)
	ldi	SS_ININT,t1

$istacknotatbase

	/*
	 * We are creating a standard stack frame here for us to put our save
	 * state onto.
	 */

	stw	r0,FM_PSP(sp)
	stw	t1,-TF_SIZE+SS_FLAGS(sp)

$intcontinue

	/*
	 * Save away r26, r30 and r22. This gets us completely out of the
	 * temporary registers.
	 */

	mfctl	tr2,t1
	stw	t1,-TF_SIZE+SS_R26(sp)
	mfctl	tr3,t1
	stw	t1,-TF_SIZE+SS_R30(sp)
	mfctl	tr4,t1
	stw	t1,-TF_SIZE+SS_R22(sp)

	/*
	 * save registers r21 (t2) and r20 (t3) for our use
	 */

	stw	t3,-TF_SIZE+SS_R20(sp)
	stw	t2,-TF_SIZE+SS_R21(sp)

#if MACH_RT
	/*
	 * disable the preemption.
	 */
	ldil	L%cpu_data,t1
	ldo	R%cpu_data(t1),t1
	ldw	CPU_PREEMPTION_LEV(t1),t2
	addi	1,t2,t2
	stw	t2,CPU_PREEMPTION_LEV(t1)
#endif /* MACH_RT */

	/*
	 * Now we're about to turn on the PC queue.  We'll also go to virtual
	 * mode in the same step. We need to get the space registers pointing
	 * at the kernel.
	 */

	mfsp	sr4,t1
	stw	t1,-TF_SIZE+SS_SR4(sp)

	mfsp	sr5,t1
	stw	t1,-TF_SIZE+SS_SR5(sp)

	mfsp	sr6,t1
	stw	t1,-TF_SIZE+SS_SR6(sp)

	mfsp	sr7,t1
	stw	t1,-TF_SIZE+SS_SR7(sp)

	mfsp	sr1,t1
	stw	t1,-TF_SIZE+SS_SR1(sp)

	mtsp	r0,sr4
	mtsp	r0,sr5
	mtsp	r0,sr6
	mtsp	r0,sr7
	mtsp	r0,sr1


	/*
	 * save the protection ID registers and then set them to be kernel
	 * only. We shouldn't be touching user space since this is an
	 * interrupt...
	 */
	mfctl	pidr1,t1
	stw	t1,-TF_SIZE+SS_CR8(sp)

	mfctl	pidr2,t1
	stw	t1,-TF_SIZE+SS_CR9(sp)

	mfctl	pidr3,t1
	stw	t1,-TF_SIZE+SS_CR12(sp)

	mfctl	pidr4,t1
	stw	t1,-TF_SIZE+SS_CR13(sp)

	ldi	HP700_PID_KERNEL,t1
	mtctl	t1,pidr1
	mtctl	t1,pidr2
	mtctl	t1,pidr3
	mtctl	t1,pidr4

	/*
	 * Now, save away other volatile state that prevents us from turning
	 * the PC queue back on, namely, the pc queue and ipsw.  Note that
	 * I don't have to save the iir, isr or ior because I'm an interrupt,
	 * and not a fault, trap or check.
	 */
	mfctl	pcoq,t1
	stw	t1,-TF_SIZE+SS_IIOQH(sp)

	mtctl	r0,pcoq			/* advance the queue */

	mfctl	pcoq,t1
	stw	t1,-TF_SIZE+SS_IIOQT(sp)


	mfctl	pcsq,t1
	stw	t1,-TF_SIZE+SS_IISQH(sp)

	mtctl	r0,pcsq			/* advance the queue */

	mfctl	pcsq,t1
	stw	t1,-TF_SIZE+SS_IISQT(sp)

	mfctl	eiem,t1
	stw	t1,-TF_SIZE+SS_CR15(sp)

	mfctl	ipsw,t2
	stw	t2,-TF_SIZE+SS_CR22(sp)

	/*
	 * stuff the space queue
	 */

	mtctl	r0,pcsq
	mtctl	r0,pcsq

	/*
	 * set the new psw to be data and code translation, interrupts
	 * disabled, protection enabled, Q bit on
	 */

	ldil	L%KERNEL_PSW,t1
	ldo	R%KERNEL_PSW(t1),t1

	mtctl	t1,ipsw

	/*
	 * Load up a real value into eiem to reflect an spl level of splhigh.
	 * That's the SPL level we will be calling interrupt() with.
	 * Right now interrupts are still off.
	 */
	ldi	SPLHIGH,t1
	mtctl	t1,eiem

	/*
	 * Load in the address to "return" to with the rfi instruction.
	 */

	ldil	L%$intnowvirt,t1
	ldo	R%$intnowvirt(t1),t1

	/*
	 * now stuff the offset queue
	 */

	mtctl	t1,pcoq
	ldo	4(t1),t1
	mtctl	t1,pcoq

	/*
	 * Must do rfir not rfi since we may be called from tlbmiss routine
	 * (to handle page fault) and it uses the shadowed registers.
	 */
	rfir
	nop

$intnowvirt

	/*
	 * Now we're in virtual mode.
	 */

#ifdef DEBUG
	/* Test if we've overflowed the interrupt stack.
	 *
	 * If so, there is an extra page beyond intstack_top which has no
	 * access rights. We change this so that we can now access it and
	 * adjust the interrupt type parameter to indicate an overflow error.
	 * However, if this is a recursive overflow then do nothing: the
	 * access rights of the extra page have already been changed and
	 * we're probably just trying to sync the discs.  This is dangerous,
	 * however.
	 */

	/*
	 * get the value of the top of the interrupt stack
	 */

	ldil	L%intstack_top,t1
	ldo	R%intstack_top(t1),t1

	/*
	 * are we into the red zone page?
	 */

	comb,<,n  sp,t1,$noistkovfl

	/*
	 * We've overflowed, load the overflow flag
	 */

	ldil	L%istk_overflow,t1
	ldw	R%istk_overflow(t1),t1

	/*
	 * have we already overflowed before?
	 */

	comb,=,n  r0,t1,$noistkovfl

	/*
	 * This is the first overflow, clear the overflow flag
	 */

	ldil	L%istk_overflow,t1
	stw	r0,R%istk_overflow(t1)

	/*
	 * Change the access rights for the interrupt stack buffer page so
	 * that we can use it as part of the interrupt stack while we panic.
	 */

	ldil	L%intstack_top,t1
	ldo	R%intstack_top(t1),t1

	/*
	 * flush the TLB entry for the top of the interrupt stack
	 */

	pdtlb	r0(sr1,t1)

	/*
	 * We need to change the access rights on this page.  Basically
	 * we will chase down the mapping entry (which will be easy since
	 * it's equivalently mapped) and then change the protection.
	 * We will then change the interrupt to be I_IS_OVFL which will
	 * cause a panic.
	 */

	/*
	 * XXX - this needs to be changed if we change the page size
	 */

	/*
	 * Convert the physical page number into the form we need to index
	 * the ptov table and locate the correct entry.
	 */

	depi    0,31,12,t1
	extrs	t1,23,24,t1
	mfctl	ptov,t2
	add	t1,t2,t1

	/*
	 * t1 now points to the ptov table entry for this physical page.
	 * For this page the protection is in the ptov table since it's
	 * an equivalently mapped part of the kernel.
	 */

	ldw	PTOV_TLBPROT(t1),t2

	/*
	 * load up the protection that we want on this page
 	 */

	ldil	L%TLB_AR_KRW,t3

	/*
	 * put the new protection into the protection field
	 */

	dep	t3,11,7,t2
	stw	t2,PTOV_TLBPROT(t1)

	/*
	 * change the interrupt type to interrupt control stack overflow
	 */

	ldil	L%I_IS_OVFL,arg0

$noistkovfl
#endif

	/*
	 * Save all the general registers
	 */

	stw	r1,-TF_SIZE+SS_R1(sp)
	stw	r2,-TF_SIZE+SS_R2(sp)
	stw	r3,-TF_SIZE+SS_R3(sp)
	stw	r4,-TF_SIZE+SS_R4(sp)
	stw	r5,-TF_SIZE+SS_R5(sp)
	stw	r6,-TF_SIZE+SS_R6(sp)
	stw	r7,-TF_SIZE+SS_R7(sp)
	stw	r8,-TF_SIZE+SS_R8(sp)
	stw	r9,-TF_SIZE+SS_R9(sp)
	stw	r10,-TF_SIZE+SS_R10(sp)
	stw	r11,-TF_SIZE+SS_R11(sp)
	stw	r12,-TF_SIZE+SS_R12(sp)
	stw	r13,-TF_SIZE+SS_R13(sp)
	stw	r14,-TF_SIZE+SS_R14(sp)
	stw	r15,-TF_SIZE+SS_R15(sp)
	stw	r16,-TF_SIZE+SS_R16(sp)
	stw	r17,-TF_SIZE+SS_R17(sp)
	stw	r18,-TF_SIZE+SS_R18(sp)
	stw	r19,-TF_SIZE+SS_R19(sp)

	/*
	 * r20 (t3) was saved above
	 * r21 (t2) was saved above
	 * r22 (t1) was saved above
	 */

	stw	r23,-TF_SIZE+SS_R23(sp)
	stw	r24,-TF_SIZE+SS_R24(sp)
	stw	r25,-TF_SIZE+SS_R25(sp)

	/*
	 * r26 (arg0) was saved above
	 */

	stw	r27,-TF_SIZE+SS_R27(sp)
	stw	r28,-TF_SIZE+SS_R28(sp)
	stw	r29,-TF_SIZE+SS_R29(sp)

	/*
	 * r30 (sp) was saved above
	 */

	stw	r31,-TF_SIZE+SS_R31(sp)


	/*
	 * Save all the space registers
	 */

	mfsp	sr0,t1
	stw	t1,-TF_SIZE+SS_SR0(sp)

	/*
	 * sr1 was saved earlier
	 */

	mfsp	sr2,t1
	stw	t1,-TF_SIZE+SS_SR2(sp)

	mfsp	sr3,t1
	stw	t1,-TF_SIZE+SS_SR3(sp)

	/*
	 * sr4 - sr7 already saved above.
	 */

	/*
	 * Save the necessary control registers that were not already saved.
	 */

	mfctl	rctr,t1
	stw	t1,-TF_SIZE+SS_CR0(sp)

	mfctl	sar,t1
	stw	t1,-TF_SIZE+SS_CR11(sp)

	/*
	 * set up dp for the kernel
	 */

	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	/*
	 * interrupt type was setup back before the call to ihandler.
	 * The save state is at the start of locals
	 */

	ldo	-TF_SIZE(sp),arg1

#if KGDB
	/*
	 * Artificially create another frame so that gdb will
	 * show real interrupted routine.
	 */
	stw	r2,FM_CRP-(FM_SIZE+ARG_SIZE)(sp)
	stw	r3,-(FM_SIZE+ARG_SIZE)(sp)	/* this overwrites ARG11 */
	ldo	-(FM_SIZE+ARG_SIZE)(sp),r3
#endif

	.import	interrupt
	ldil	L%interrupt,t1
	ldo	R%interrupt(t1),t1
	.call
	blr     r0,rp
	bv,n    0(t1)

	/*
	 * Ok, back from C. Disable interrupts while we restore things
	 */
	rsm	PSW_I,r0

	/*
	 * Restore most of the state, up to the point where we need to turn
	 * off the PC queue.  Going backwards, starting with control regs.
	 */

	ldw	-TF_SIZE+SS_CR15(sp),t1
	mtctl	t1,eiem

	ldw	-TF_SIZE+SS_CR11(sp),t1
	mtctl	t1,sar

	ldw	-TF_SIZE+SS_CR0(sp),t1
	mtctl	t1,rctr

	/*
	 * Restore the lower space registers, we'll restore sr5 - sr7 after
	 * we have turned off translations
	 */

	ldw	-TF_SIZE+SS_SR0(sp),t1
	mtsp	t1,sr0

	ldw	-TF_SIZE+SS_SR1(sp),t1
	mtsp	t1,sr1

	ldw	-TF_SIZE+SS_SR2(sp),t1
	mtsp	t1,sr2

	ldw	-TF_SIZE+SS_SR3(sp),t1
	mtsp	t1,sr3

	/*
	 * restore most of the general registers
	 */

	ldw	-TF_SIZE+SS_R1(sp),r1
	ldw	-TF_SIZE+SS_R2(sp),r2
	ldw	-TF_SIZE+SS_R3(sp),r3
	ldw	-TF_SIZE+SS_R4(sp),r4
	ldw	-TF_SIZE+SS_R5(sp),r5
	ldw	-TF_SIZE+SS_R6(sp),r6
	ldw	-TF_SIZE+SS_R7(sp),r7
	ldw	-TF_SIZE+SS_R8(sp),r8
	ldw	-TF_SIZE+SS_R9(sp),r9
	ldw	-TF_SIZE+SS_R10(sp),r10
	ldw	-TF_SIZE+SS_R11(sp),r11
	ldw	-TF_SIZE+SS_R12(sp),r12
	ldw	-TF_SIZE+SS_R13(sp),r13
	ldw	-TF_SIZE+SS_R14(sp),r14
	ldw	-TF_SIZE+SS_R15(sp),r15
	ldw	-TF_SIZE+SS_R16(sp),r16
	ldw	-TF_SIZE+SS_R17(sp),r17
	ldw	-TF_SIZE+SS_R18(sp),r18
	ldw	-TF_SIZE+SS_R19(sp),r19

	/*
	 * r20 (t3) is used as a temporary and will be restored later
	 * r21 (t2) is used as a temporary and will be restored later
	 * r22 (t1) is used as a temporary and will be restored later
	 */

	ldw	-TF_SIZE+SS_R23(sp),r23
	ldw	-TF_SIZE+SS_R24(sp),r24
	ldw	-TF_SIZE+SS_R25(sp),r25
	ldw	-TF_SIZE+SS_R26(sp),r26
	ldw	-TF_SIZE+SS_R27(sp),r27
	ldw	-TF_SIZE+SS_R28(sp),r28
	ldw	-TF_SIZE+SS_R29(sp),r29

	/*
	 * r30 (sp) will be restored later
	 */

#if MACH_RT
	/*
	 * reenable the preemption.
	 */
	ldil	L%cpu_data,t1
	ldo	R%cpu_data(t1),t1
	ldw	CPU_PREEMPTION_LEV(t1),t2
	addi	-1,t2,t2
	stw	t2,CPU_PREEMPTION_LEV(t1)
#endif /* MACH_RT */

	b	$int_new_page
	ldw	-TF_SIZE+SS_R31(sp),r31

	/*
	 * clear the system mask, this puts us back into physical mode.
	 *
	 * N.B: We will start running with code translation on, data
	 * translation off, interrupts off and the Q bit is off. What this
	 * means is that we can not tolerate a code translation miss from here
	 * until we do the rfi. We will align the next address to be on a page
	 * boundary so that we will be guarenteed that the page is in the TLB.
	 * All of the code from here to the rfi must be within one page.
	 */

	.align  HP700_PGBYTES
$int_new_page
	rsm	RESET_PSW,r0

	/*
	 * restore the protection ID registers
	 */

	ldw	-TF_SIZE+SS_CR13(sp),t1
	mtctl	t1,pidr4

	ldw	-TF_SIZE+SS_CR12(sp),t1
	mtctl	t1,pidr3

	ldw	-TF_SIZE+SS_CR9(sp),t1
	mtctl	t1,pidr2

	ldw	-TF_SIZE+SS_CR8(sp),t1
	mtctl	t1,pidr1

	/*
	 * restore space registers sr4 - sr7
	 */

	ldw	-TF_SIZE+SS_SR4(sp),t1
	mtsp	t1,sr4

	ldw	-TF_SIZE+SS_SR5(sp),t1
	mtsp	t1,sr5

	ldw	-TF_SIZE+SS_SR6(sp),t1
	mtsp	t1,sr6

	ldw	-TF_SIZE+SS_SR7(sp),t1
	mtsp	t1,sr7

	/*
	 * Cut back the interrupt stack. First get the flags from the saved
	 * state area to see if this is the last frame on the stack
	 */

	ldw	-TF_SIZE+SS_FLAGS(sp),t1

	/*
	 * Check the SS_PSPKERNEL bit in the saved state flags
	 */

	bb,<,n  t1,SS_PSPKERNEL_POS,$notlastistk

	/*
	 * SS_PSPKERNEL bit not set, this means that we are at the base of the
	 * interrupt stack.
	 *
	 * This means that -TF_SIZE(sp) is the original value of istackptr
	 * before this stack frame. Save the value into the istackptr variable,
	 * this indicates that we're not on the interrupt stack.
	 */

	ldo     -TF_SIZE(sp),t1
	ldil    L%istackptr,t2
	stw     t1,R%istackptr(t2)

	/*
	 * Now restore the IIA space queue and the IIA offset queue from the
	 * saved state. Save the PC we are going to to check if it is in
	 * "user" mode.
	 */
	ldw	-TF_SIZE+SS_IIOQH(sp),t2
	mtctl	t2,pcoq

	ldw	-TF_SIZE+SS_IIOQT(sp),t1
	mtctl	t1,pcoq

	ldw	-TF_SIZE+SS_IISQH(sp),t3
	mtctl	t3,pcsq

	ldw	-TF_SIZE+SS_IISQT(sp),t1
	mtctl	t1,pcsq

	/*
	 * get the ipsw we are returning to.
	 */
	ldw	-TF_SIZE+SS_CR22(sp),t1
	mtctl	t1,ipsw

	/*
	 * See if we need to have an AST delivered.
	 */
	ldil	L%need_ast,t1
	ldw	R%need_ast(t1),t1
	comib,=,n 0,t1,no_int_ast

	/*
	 * We only want to check for ASTs if we would be in user mode when
	 * it is delivered.  "User mode" means the privilege level is non-
	 * zero and space is non-zero (taking an AST when priv is non-zero
	 * but space is zero, i.e. early in the gateway page, can cause
	 * grief for the signal code.)
	 */
	extru,<> t2,31,2,r0
	b,n	check_int_ast
	comib,=,n 0,t3,no_int_ast

user_ast

	/*
	 * If we do need an AST we massage things to make it look like
	 * we took a trap and jump into the trap handler.  To do this
	 * we essentially back out of the interrupt and jump to the
	 * trap handler.
	 */
	ldi	I_LPRIV_XFR,arg0

	/*
	 * Determine the location of the PCB saved state for this thread
	 * and copy over the original values of registers still in use
	 * (t1, t2, arg0, sp, t3).
	 */
        ldil    L%active_pcbs,t1
        ldw     R%active_pcbs(t1),t1
	ldo	PCB_SS(t1),t2

	ldw	-TF_SIZE+SS_R22(sp),t3
	stw	t3,SS_R22(t2)
	ldw	-TF_SIZE+SS_R21(sp),t3
	stw	t3,SS_R21(t2)
	ldw	-TF_SIZE+SS_R26(sp),t3
	stw	t3,SS_R26(t2)
	ldw	-TF_SIZE+SS_R30(sp),t3
	stw	t3,SS_R30(t2)
	ldw	-TF_SIZE+SS_R20(sp),t3
	stw	t3,SS_R20(t2)

	/*
	 * Switch over to the kernel stack, creating a stack frame
	 * indicating that we are a trap and at the base of the stack.
	 */
	ldw	PCB_KSP(t1),sp
	stw	r0,PCB_KSP(t1)
	ldo	TF_SIZE(sp),sp

	/*
	 * Save PC queues, IPSW and EIEM
	 */
	mfctl	pcoq,t1
	stw	t1,SS_IIOQH(t2)
	mtctl	r0,pcoq
	mfctl	pcoq,t1
	stw	t1,SS_IIOQT(t2)
	mfctl	pcsq,t1
	stw	t1,SS_IISQH(t2)
	mtctl	r0,pcsq
	mfctl	pcsq,t1
	stw	t1,SS_IISQT(t2)

	mfctl	ipsw,t1
	stw	t1,SS_CR22(t2)
	mfctl	eiem,t1
	stw	t1,SS_CR15(t2)

	/*
	 * Save SIDs and PIDs and set them to reflect kernel mode
	 * before we re-enable mapping.
	 *
	 * Note we leave pidr1 pointing to user space.
	 */
	mfsp	sr4,t1
	stw	t1,SS_SR4(t2)
	mfsp	sr5,t1
	stw	t1,SS_SR5(t2)
	mfsp	sr6,t1
	stw	t1,SS_SR6(t2)
	mfsp	sr7,t1
	stw	t1,SS_SR7(t2)

	mtsp	r0,sr4
	mtsp	r0,sr5
	mtsp	r0,sr6
	mtsp	r0,sr7

	mfctl	pidr2,t1
	stw	t1,SS_CR9(t2)
	mfctl	pidr3,t1
	stw	t1,SS_CR12(t2)
	mfctl	pidr4,t1
	stw	t1,SS_CR13(t2)

	ldi	HP700_PID_KERNEL,t1
	mtctl	t1,pidr2
	mtctl	t1,pidr3
	mtctl	t1,pidr4

	/*
	 * Load space and offset queues, IPSW and EIEM for jump to
	 * virtual mode.  We will land down in the thandler code.
	 *
	 * Note we use rfi and not rfir since we haven't touched the
	 * shadowed registers and they may have been clobbered
	 * anyway when we went virtual for the call to interrupt().
	 */
	ldil	L%KERNEL_PSW,t1
	ldo	R%KERNEL_PSW(t1),t1
	mtctl	t1,ipsw

	ldi	SPLHIGH,t1
	mtctl	t1,eiem

	mtctl	r0,pcsq
	mtctl	r0,pcsq
	ldil	L%$trapnowvirt,t1
	ldo	R%$trapnowvirt(t1),t1
	mtctl	t1,pcoq
	ldo	4(t1),t1
	mtctl	t1,pcoq

	ldi	SS_INTRAP,t1

	rfi
	nop

	/*
	 * if kloaded server, give it a chance to be preempted.
	 */
check_int_ast
	ldil	L%VM_MIN_KERNEL_LOADED_ADDRESS,t3
	ldo	R%VM_MIN_KERNEL_LOADED_ADDRESS(t3),t3
	combt,<< t3,t2,user_ast
	nop

#if MACH_RT
	.import $locore_end
	.import $syscall
	.import $syscall_end
	.import $movc_start
	.import $movc_end

	bb,>=,n  t1,25,no_int_ast	/* AST_URGENT = 0x40 */

	ldil	L%cpu_data,t1
	ldo	R%cpu_data(t1),t1
	ldw	CPU_PREEMPTION_LEV(t1),t3
	combf,=,n r0,t3,no_int_ast

	ldil	L%$locore_end,t3
	ldo	R%$locore_end(t3),t3
	combt,<<,n t2,t3,no_int_ast

	ldil	L%$syscall,t3
	ldo	R%$syscall(t3),t3
	combt,<<,n t2,t3,$kcont

	ldil	L%$syscall_end,t3
	ldo	R%$syscall_end(t3),t3
	combt,<<,n t2,t3,no_int_ast
$kcont
	ldil	L%$movc_start,t3
	ldo	R%$movc_start(t3),t3
	combt,<<,n t2,t3,$kcont1

	ldil	L%$movc_end,t3
	ldo	R%$movc_end(t3),t3
	combt,<<,n t2,t3,no_int_ast
$kcont1
	ldw	CPU_ACTIVE_THREAD(t1),arg0

	ldil	L%KERNEL_PSW,t1
	ldo	R%KERNEL_PSW(t1),t1
	mtctl	t1,ipsw

	ldil	L%preempt_virt,t1
	ldo	R%preempt_virt(t1),t1
	mtctl	t1,pcoq
	ldo	4(t1),t1
	mtctl	t1,pcoq

	rfi
	nop

preempt_virt
	ldw	-TF_SIZE+SS_R30(sp),t2
	stw	t2,SS_R30(t2)

	ldw	-TF_SIZE+SS_R18(sp),t1
	stw	t1,SS_R18(t2)
	ldw	-TF_SIZE+SS_R17(sp),t1
	stw	t1,SS_R17(t2)
	ldw	-TF_SIZE+SS_R16(sp),t1
	stw	t1,SS_R16(t2)

	mfctl	eiem,r17
	stw	r17,SS_CR15(t2)
	copy	arg0,r18

	ldi	SPLHIGH,t1
	mtctl	t1,eiem

	ldw	-TF_SIZE+SS_IIOQH(sp),r16
	stw	r16,SS_IIOQH(t2)
	ldw	-TF_SIZE+SS_IIOQT(sp),t1
	stw	t1,SS_IIOQT(t2)
	ldw	-TF_SIZE+SS_CR22(sp),t1
	stw	t1,SS_CR22(t2)

	ldw	-TF_SIZE+SS_R22(sp),t1
	stw	t1,SS_R22(t2)
	ldw	-TF_SIZE+SS_R21(sp),t1
	stw	t1,SS_R21(t2)
	ldw	-TF_SIZE+SS_R20(sp),t1
	stw	t1,SS_R20(t2)
	ldw	-TF_SIZE+SS_R1(sp),t1
	stw	t1,SS_R1(t2)

	ldw	-TF_SIZE+SS_R25(sp),t1
	stw	t1,SS_R25(t2)
	ldw	-TF_SIZE+SS_R26(sp),t1
	stw	t1,SS_R26(t2)

	ldw	-TF_SIZE+SS_FLAGS(sp),t1
	stw	t1,SS_FLAGS(t2)

	stw	r2,SS_R2(t2)
	stw	r19,SS_R19(t2)
	stw	r23,SS_R23(t2)
 	stw	r24,SS_R24(t2)
	stw	r28,SS_R28(t2)
	stw	r29,SS_R29(t2)
	stw	r31,SS_R31(t2)

	ldo	TF_SIZE(t2),sp

restart_ast
	/*
	 * arg0 already loaded
	 */
	copy	r17, arg1
	bl	kernel_ast,rp
	copy	r16, arg2

	/*
	 * Check if a new AST has been delivered
	 */
	ldil	L%need_ast,t1
	ldw	R%need_ast(t1),t1
	bb,<,n	t1,25,restart_ast	/* AST_URGENT = 0x40 */
	copy	r18,arg0

	/*
	 * kernel_ast returns with all interrupts masked
	 */
	ldw	-TF_SIZE+SS_R31(sp),r31
	ldw	-TF_SIZE+SS_R29(sp),r29
	ldw	-TF_SIZE+SS_R28(sp),r28
	ldw	-TF_SIZE+SS_R26(sp),r26
	ldw	-TF_SIZE+SS_R25(sp),r25
	ldw	-TF_SIZE+SS_R24(sp),r24
	ldw	-TF_SIZE+SS_R23(sp),r23
	ldw	-TF_SIZE+SS_R20(sp),r20
	ldw	-TF_SIZE+SS_R19(sp),r19
	ldw	-TF_SIZE+SS_R18(sp),r18
	ldw	-TF_SIZE+SS_R17(sp),r17
	ldw	-TF_SIZE+SS_R16(sp),r16
	ldw	-TF_SIZE+SS_R2(sp),r2
	ldw	-TF_SIZE+SS_R1(sp),r1

	ldil	L%active_traps,t2
        ldo	R%active_traps(t2),t2

	ldw	-TF_SIZE+SS_IIOQH(sp),t1
	stw	t1, TRAP_IIOQH(t2)
	ldw	-TF_SIZE+SS_IIOQT(sp),t1
	stw	t1, TRAP_IIOQT(t2)
	ldw	-TF_SIZE+SS_CR22(sp),t1
	stw	t1, TRAP_CR22(t2)

	ldw	-TF_SIZE+SS_R22(sp),t1
	stw	t1, TRAP_R22(t2)
	ldw	-TF_SIZE+SS_R21(sp),t1
	stw	t1, TRAP_R21(t2)

	ldw	-TF_SIZE+SS_CR15(sp),t1
	ldw	-TF_SIZE+SS_R30(sp),sp

	rsm	RESET_PSW,r0
	mtctl	t1,eiem

	ldw	TRAP_IIOQH(t2),t1
	mtctl	t1,pcoq
	ldw	TRAP_IIOQT(t2),t1
	mtctl	t1,pcoq
	ldw	TRAP_CR22(t2),t1
	mtctl	t1,ipsw

	ldw	TRAP_R22(t2),t1
	ldw	TRAP_R21(t2),t2

	rfi
	nop

#else /* MACH_RT */
	b,n	no_int_ast
#endif /* MACH_RT */

$notlastistk
	ldw	-TF_SIZE+SS_IIOQH(sp),t1
	mtctl	t1,pcoq
	ldw	-TF_SIZE+SS_IIOQT(sp),t1
	mtctl	t1,pcoq
	ldw	-TF_SIZE+SS_IISQH(sp),t1
	mtctl	t1,pcsq
	ldw	-TF_SIZE+SS_IISQT(sp),t1
	mtctl	t1,pcsq
	ldw	-TF_SIZE+SS_CR22(sp),t1
	mtctl	t1,ipsw

no_int_ast
	/*
	 * restore all the registers that we used above
	 */
	ldw	-TF_SIZE+SS_R22(sp),t1
	ldw	-TF_SIZE+SS_R21(sp),t2
	ldw	-TF_SIZE+SS_R20(sp),t3
	ldw	-TF_SIZE+SS_R30(sp),sp

	rfi
	nop

	.procend


/*
 * thandler(type)
 *
 * Build a saved state structure on the kernel stack, switch the processor
 * into virtual addressing mode and call the routine trap().
 */

	.export	thandler
	.proc
	.callinfo calls, frame=ROUNDED_SS_SIZE, save_sp, hpux_int
	.align  HP700_PGBYTES

thandler

#if KGDB

	ldil	L%istackptr,t1
	ldw	R%istackptr(t1),t1

	comb,<>,n t1,r0,$handle_as_trap

	/*
	 * We are currently running on the interrupt stack so use sp from the
	 * interrupt stack
	 *
	 * Create a stack frame and clear the previous stack pointer word.
	 * Then save some of the state that we will need.
	 */

	ldo	TF_SIZE(sp),sp
	stw	r0,FM_PSP(sp)

	mfctl	ior,t1
	stw	t1,-TF_SIZE+SS_CR21(sp)

	mfctl	isr,t1
	stw	t1,-TF_SIZE+SS_CR20(sp)

	mfctl	iir,t1
	stw	t1,-TF_SIZE+SS_CR19(sp)

	ldi	SS_PSPKERNEL+SS_INTRAP,t1

	/*
	 * process the trap in ihandler since we are on the interrupt stack
	 */
	b 	$intcontinue
	stw	t1,-TF_SIZE+SS_FLAGS(sp)

$handle_as_trap

#endif

	/*
	 * get the current pcb pointer and then the kernel stack pointer
	 */

        ldil    L%active_pcbs,t1
        ldw     R%active_pcbs(t1),t1
	ldw	PCB_KSP(t1),sp

	/*
	 * if the kernel stack pointer is zero then we are already on the
	 * kernel stack. If not zero we are at the base of the kernel stack.
	 */

	comb,<>	r0,sp,$kstackatbase
	mtctl	t2,tr5			/* Moved down from above.  */

	/*
	 * Set the saved state flag to be previous stack pointer on kernel
	 * and mark this stack frame as a trap frame.
	 *
	 * Retrieve the old stack pointer from within the delay slot.
	 */

	b	$kstacknotatbase
	mfctl	tr3,sp

$kstackatbase

	/*
	 * Since we are coming in from user mode, user state is stored in
	 * the PCB, and a kernel_state frame is created. We then jump into
	 * the register save code with the saved state pointer set.
	 *
	 * Stack adjustment by TF_SIZE moved into delay slot of
	 * movib branch below.
	 */
	ldo	PCB_SS(t1),t2

	/*
	 * The base of the kernel stack was pulled out of the PCB. Clear the
	 * kernel stack pointer in the PCB to indicate that we are now on the
	 * kernel stack.
	 */

	stw	r0,PCB_KSP(t1)
	ldo	TF_SIZE(sp),sp

	/*
	 * now start saving the temporary registers into the saved state.
	 * These four get us out of temporary registers
	 */

	mfctl	tr2,t1
	stw	t1,SS_R26(t2)

	mfctl	tr3,t1
	stw	t1,SS_R30(t2)

	mfctl	tr4,t1
	stw	t1,SS_R22(t2)

	mfctl	tr5,t1
	stw	t1,SS_R21(t2)

	/*
	 * Now, save away other volatile state that prevents us from turning
	 * the PC queue back on, namely, the pc queue and ipsw, and the
	 * interrupt information.
	 */

	mfctl	pcoq,t1
	stw	t1,SS_IIOQH(t2)

	mtctl	r0,pcoq

	mfctl	pcoq,t1
	stw	t1,SS_IIOQT(t2)

	mfctl	pcsq,t1
	stw	t1,SS_IISQH(t2)

	mtctl	r0,pcsq

	mfctl	pcsq,t1
	stw	t1,SS_IISQT(t2)

	mfctl	ipsw,t1
	stw	t1,SS_CR22(t2)

	mfctl	eiem,t1
	stw	t1,SS_CR15(t2)

	mfctl	ior,t1
	stw	t1,SS_CR21(t2)

	mfctl	isr,t1
	stw	t1,SS_CR20(t2)

	mfctl	iir,t1
	stw	t1,SS_CR19(t2)

	/*
	 * Now we're about to turn on the PC queue.  We'll also go to virtual
	 * mode in the same step. Save the space registers sr4 - sr7 and
	 * point them to the kernel space
	 */

	mfsp	sr4,t1
	stw	t1,SS_SR4(t2)

	mfsp	sr5,t1
	stw	t1,SS_SR5(t2)

	mfsp	sr6,t1
	stw	t1,SS_SR6(t2)

	mfsp	sr7,t1
	stw	t1,SS_SR7(t2)

	mtsp	r0,sr4
	mtsp	r0,sr5
	mtsp	r0,sr6
	mtsp	r0,sr7

	/*
	 * save the protection ID registers. We will keep the first one
	 * with the protection of the user's area and set the remaining
	 * ones to be the kernel.
	 */

	mfctl	pidr2,t1
	stw	t1,SS_CR9(t2)

	mfctl	pidr3,t1
	stw	t1,SS_CR12(t2)

	mfctl	pidr4,t1
	stw	t1,SS_CR13(t2)

	ldi	HP700_PID_KERNEL,t1
	mtctl	t1,pidr2
	mtctl	t1,pidr3
	mtctl	t1,pidr4

	b	$trap_in_user_mode
	ldi	SS_INTRAP,t1

$kstacknotatbase

	/*
	 * We are creating a standard stack frame here for us to put our save
	 * state onto. This means we will be able to do stack traces all the
	 * way back to thandler which contains the stack frame.
	 * We don't create the stack frame here under normal calling convention
	 * rules, but we don't have to worry because interrupts are off.
	 * Locals for this frame (the save state) are at the head of the frame.
	 */

	/*
	 * create our stack frame, don't use it for fear that we are off
	 * the end of the stack and might clobber another threads PCB.
	 */

	ldo	TF_SIZE(sp),sp

#ifdef DEBUG
	/*
	 * Test if we've overflowed the Kernel Stack.
	 *
	 * If so, switch to the Interrupt Control Stack and change the
	 * interrupt type to indicate kernel stack overflow.
	 */

	/*
	 * Determine the limit of the kernel stack.
	 *
	 * XXX WHen using only one register we can only add, at best,
	 * a signed 14-bit value to its current value in a single instruction.
	 * Since 14-bits is -8192 to 8191 that means if the stack is over
	 * one 4k page we have to use multiple instructions to find the
	 * upper bound of the stack. However, since we know the stack must
	 * be double-word aligned we can handle a 2 page stack by using
	 * a <= comparison instead of just <.
	 * Over two pages we have to use two registers.
	 */

	ldil	L%active_stacks,t2
	ldw	R%active_stacks(t2),t2
#if KERNEL_STACK_SIZE <= 8192
	ldo	KERNEL_STACK_SIZE-8(t2),t2
#else
	ldil	L%KERNEL_STACK_SIZE-8,t1
	ldo	R%KERNEL_STACK_SIZE-8(t1),t1
	add	t1,t2,t2
#endif

	/*
	 * If the stack pointer is less than t2 then we are OK.
	 */

	comb,<=,n  sp,t2,$noksovfl

	/*
	 * We've overflowed the kernel stack, move to the interrupt stack
	 * and generate an I_KS_OVFL interrupt. We know that istackptr is
	 * non zero here since we are not on the interrupt stack now.
	 * We're going to panic anyway so it really doesn't matter!
	 */

	ldil	L%istackptr,sp
	ldw	R%istackptr(sp),sp
        ldo	TF_SIZE(sp),sp

	/*
	 * Zero istackptr to indicate that we are on the interrupt stack.
 	 */

	ldil	L%istackptr,t2
	stw	r0,R%istackptr(t2)

	/*
	 * Set the interrupt type to be kernel stack overflow.
	 */

	ldi	I_KS_OVFL,arg0

$noksovfl
#endif

	/*
	 * Load the per cpu trap area for temporary saving some registers
	 * before going back to virtual mode, where these will be finally
	 * saved in the kernel stack area.
	 */
	ldil	L%active_traps,t2
        ldo	R%active_traps(t2),t2

	/*
	 * now start saving the temporary registers into the saved state.
	 * These four get us out of temporary registers
	 */

	mfctl	tr2,t1
	stw	t1,TRAP_R26(t2)

	mfctl	tr3,t1
	stw	t1,TRAP_R30(t2)

	mfctl	tr4,t1
	stw	t1,TRAP_R22(t2)

	mfctl	tr5,t1
	stw	t1,TRAP_R21(t2)

	/*
	 * Now, save away other volatile state that prevents us from turning
	 * the PC queue back on, namely, the pc queue and ipsw, and the
	 * interrupt information.
	 */

	mfctl	pcoq,t1
	stw	t1,TRAP_IIOQH(t2)

	mtctl	r0,pcoq

	mfctl	pcoq,t1
	stw	t1,TRAP_IIOQT(t2)

	mfctl	pcsq,t1
	stw	t1,TRAP_IISQH(t2)

	mtctl	r0,pcsq

	mfctl	pcsq,t1
	stw	t1,TRAP_IISQT(t2)

	mfctl	ipsw,t1
	stw	t1,TRAP_CR22(t2)

	mfctl	eiem,t1
	stw	t1,TRAP_CR15(t2)

	mfctl	ior,t1
	stw	t1,TRAP_CR21(t2)

	mfctl	isr,t1
	stw	t1,TRAP_CR20(t2)

	mfctl	iir,t1
	stw	t1,TRAP_CR19(t2)

	ldi	SS_PSPKERNEL+SS_INTRAP,t1
	ldo	-TF_SIZE(sp),t2

$trap_in_user_mode

	/*
	 * Save trap flags
	 */
	mtctl	t1,tr6

	/*
	 * load the space queue
	 */

	mtctl	r0,pcsq
	mtctl	r0,pcsq

	/*
	 * set the new psw to be data and code translation, interrupts
	 * disabled, protection enabled, Q bit on
	 */

	ldil	L%KERNEL_PSW,t1
	ldo	R%KERNEL_PSW(t1),t1
	mtctl	t1,ipsw

	/*
	 * Load up a real value into eiem to reflect an spl level of splhigh.
	 * Right now interrupts are still off.
	 */
	ldi	SPLHIGH,t1
	mtctl	t1,eiem

	/*
	 * load in the address to "return" to with the rfi instruction
	 */

	ldil	L%$trapnowvirt,t1
	ldo	R%$trapnowvirt(t1),t1

	/*
	 * load the offset queue
	 */

	mtctl	t1,pcoq
	ldo	4(t1),t1
	mtctl	t1,pcoq

	/*
	 * Restore trap flags
	 */

	mfctl	tr6,t1

	/*
	 * Must do rfir not rfi since we may be called from tlbmiss routine
	 * (to handle page fault) and it uses the shadowed registers.
	 */
	rfir
	nop

$trapnowvirt
	/*
	 * t2 contains the virtual address of the saved status area
	 * t1 contains the trap flags
	 * sp contains the virtual address of the stack pointer
	 */

	stw	t1,SS_FLAGS(t2)

	/*
	 * Save all general registers that we haven't saved already
	 */

	stw	r1,SS_R1(t2)
	stw	r2,SS_R2(t2)
	stw	r3,SS_R3(t2)
	stw	r4,SS_R4(t2)
	stw	r5,SS_R5(t2)
	stw	r6,SS_R6(t2)
	stw	r7,SS_R7(t2)
	stw	r8,SS_R8(t2)
	stw	r9,SS_R9(t2)
	stw	r10,SS_R10(t2)
	stw	r11,SS_R11(t2)
	stw	r12,SS_R12(t2)
	stw	r13,SS_R13(t2)
	stw	r14,SS_R14(t2)
	stw	r15,SS_R15(t2)
	stw	r16,SS_R16(t2)
	stw	r17,SS_R17(t2)
	stw	r18,SS_R18(t2)
	stw	r19,SS_R19(t2)
	stw	r20,SS_R20(t2)

	/*
	 * r21 already saved
	 * r22 already saved
	 */

	stw	r23,SS_R23(t2)
	stw	r24,SS_R24(t2)
	stw	r25,SS_R25(t2)

	/*
	 * r26 already saved
	 */

	stw	r27,SS_R27(t2)
	stw	r28,SS_R28(t2)
	stw	r29,SS_R29(t2)

 	/*
	 * r30 already saved
	 */

	stw	r31,SS_R31(t2)

	/*
	 * Save the space registers.
	 */

	mfsp	sr0,t1
	stw	t1,SS_SR0(t2)

	mfsp	sr1,t1
	stw	t1,SS_SR1(t2)

	mfsp	sr2,t1
	stw	t1,SS_SR2(t2)

	mfsp	sr3,t1
	stw	t1,SS_SR3(t2)

	/*
	 * Save the necessary control registers that were not already saved.
	 */

	mfctl	rctr,t1
	stw	t1,SS_CR0(t2)

	mfctl	pidr1,t1
	stw	t1,SS_CR8(t2)

	mfctl	sar,t1
	stw	t1,SS_CR11(t2)

	/*
	 * load the global pointer for the kernel
	 */

	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	ldw	SS_FLAGS(t2),r18
	comib,= SS_INTRAP,r18,$call_trap
	stw	r0,FM_PSP(sp)

	/*
	 * Save temporary area state to the right save_state area
	 */

	ldil	L%active_traps,t1
        ldo	R%active_traps(t1),t1

	ldw	TRAP_R21(t1),r4
	ldw	TRAP_R22(t1),r5
	ldw	TRAP_R26(t1),r6
	ldw	TRAP_R30(t1),r7
	stw	r4,SS_R21(t2)
	stw	r5,SS_R22(t2)
	stw	r6,SS_R26(t2)
	stw	r7,SS_R30(t2)

	ldw	TRAP_IIOQH(t1),r4
	ldw	TRAP_IIOQT(t1),r5
	ldw	TRAP_IISQH(t1),r6
	ldw	TRAP_IISQT(t1),r7
	stw	r4,SS_IIOQH(t2)
	stw	r5,SS_IIOQT(t2)
	stw	r6,SS_IISQH(t2)
	stw	r7,SS_IISQT(t2)

	ldw	TRAP_CR15(t1),r10
	ldw	TRAP_CR19(t1),r11
	ldw	TRAP_CR20(t1),r12
	ldw	TRAP_CR21(t1),r13
	ldw	TRAP_CR22(t1),r14
	stw	r10,SS_CR15(t2)
	stw	r11,SS_CR19(t2)
	stw	r12,SS_CR20(t2)
	stw	r13,SS_CR21(t2)
	stw	r14,SS_CR22(t2)

$call_trap

	/*
	 * call the C routine trap. Interrupt type (arg0) was setup back before
	 * the call to thandler.
	 */
	copy	t2,arg1

#if KGDB
	/*
	 * Artificially create another frame so that gdb will
	 * show real trapped routine.
	 */
	stw	r2,FM_CRP-(FM_SIZE+ARG_SIZE)(sp)
	stw	r3,-(FM_SIZE+ARG_SIZE)(sp)	/* this overwrites ARG11 */
	ldo	-(FM_SIZE+ARG_SIZE)(sp),r3
#endif

	.import	trap
	ldil	L%trap,t1
	ldo	R%trap(t1),t1
	.call
	blr     r0,rp
	bv,n    0(t1)

	/*
	 * Ok, return from C function
	 * trap() returns with all interrupts masked
	 */
	comib,= SS_INTRAP,r18,$thread_reload

	ldil	L%active_traps,t2
        ldo	R%active_traps(t2),t2
	ldo	-TF_SIZE(sp),t3

	ldw	SS_IISQT(t3),r4
	ldw	SS_IISQH(t3),r5
	ldw	SS_IIOQT(t3),r6
	ldw	SS_IIOQH(t3),r7
	stw	r4,TRAP_IISQT(t2)
	stw	r5,TRAP_IISQH(t2)
	stw	r6,TRAP_IIOQT(t2)
	stw	r7,TRAP_IIOQH(t2)

	ldw	SS_CR22(t3),r4
	ldw	SS_R30(t3),r5
	ldw	SS_R22(t3),r6
	ldw	SS_R21(t3),r7
	stw	r4,TRAP_CR22(t2)
	stw	r5,TRAP_R30(t2)
	stw	r6,TRAP_R22(t2)
	b	$thread_return
	stw	r7,TRAP_R21(t2)

$thread_reload

	ldil	L%active_pcbs,t1
	ldw	R%active_pcbs(t1),t1
	ldo	PCB_SS(t1),t3

	/*
	 *
	 * This is also the point where new threads come when they are created.
	 * The new thread is setup to look like a thread that took an
	 * interrupt and went immediatly into trap.
	 */

	.export $thread_return
$thread_return
	/*
	 * Restore most of the state, up to the point where we need to turn
	 * off the PC queue. Going backwards, starting with control regs.
	 */

	ldw	SS_CR15(t3),t1
	mtctl	t1,eiem

	ldw	SS_CR11(t3),t1
	mtctl	t1,sar

	ldw	SS_CR8(t3),t1
	mtctl	t1,pidr1

	ldw	SS_CR0(t3),t1
	mtctl	t1,rctr

	/*
	 * Restore the lower space registers, we'll restore sr4 - sr7 after
	 * we have turned off translations
	 */

	ldw	SS_SR0(t3),t1
	mtsp	t1,sr0

	ldw	SS_SR1(t3),t1
	mtsp	t1,sr1

	ldw	SS_SR2(t3),t1
	mtsp	t1,sr2

	ldw	SS_SR3(t3),t1
	mtsp	t1,sr3

	/*
	 * restore most of the general registers
	 */

	ldw	SS_R1(t3),r1
	ldw	SS_R2(t3),r2
	ldw	SS_R3(t3),r3
	ldw	SS_R4(t3),r4
	ldw	SS_R5(t3),r5
	ldw	SS_R6(t3),r6
	ldw	SS_R7(t3),r7
	ldw	SS_R8(t3),r8
	ldw	SS_R9(t3),r9
	ldw	SS_R10(t3),r10
	ldw	SS_R11(t3),r11
	ldw	SS_R12(t3),r12
	ldw	SS_R13(t3),r13
	ldw	SS_R14(t3),r14
	ldw	SS_R15(t3),r15
	ldw	SS_R16(t3),r16
	ldw	SS_R17(t3),r17
	ldw	SS_R18(t3),r18
	ldw	SS_R19(t3),r19

	/*
	 * r20 (t3) is used as a temporary and will be restored later
	 * r21 (t2) is used as a temporary and will be restored later
	 * r22 (t1) is used as a temporary and will be restored later
	 */

	ldw	SS_R23(t3),r23
	ldw	SS_R24(t3),r24
	ldw	SS_R25(t3),r25
	ldw	SS_R26(t3),r26
	ldw	SS_R27(t3),r27
	ldw	SS_R28(t3),r28
	ldw	SS_R29(t3),r29

	/*
	 * r30 (sp) will be restored later
	 */

	ldw	SS_R31(t3),r31

	/*
	 * Cut back the kernel stack. See if this is the last saved state
	 * structure on the stack
	 */

	ldw	SS_FLAGS(t3),t1
	bb,<	t1,SS_PSPKERNEL_POS,$finalize_trap
	ldw	SS_R20(t3),t3

	/*
	 * Determine the value of sp before this stack frame and put it
	 * in the thread's PCB kernel stack pointer.
	 */

        ldil    L%active_pcbs,t1
        ldw     R%active_pcbs(t1),t1
	ldo     -TF_SIZE(sp),t2
	stw     t2,PCB_KSP(t1)
	ldo	PCB_SS(t1),t2

	/*
	 * clear the system mask, this puts us back into physical mode.
	 *
	 * N.B: Better not be any code translation traps from this point
	 * on. Of course, we know this routine could never be *that* big.
	 */
	rsm	RESET_PSW,r0

	/*
	 * restore the protection ID registers
	 */

	ldw	SS_CR9(t2),t1
	mtctl	t1,pidr2

	ldw	SS_CR12(t2),t1
	mtctl	t1,pidr3

	ldw	SS_CR13(t2),t1
	mtctl	t1,pidr4

	/*
	 * restore the space registers
	 */

	ldw	SS_SR4(t2),t1
	mtsp	t1,sr4

	ldw	SS_SR5(t2),t1
	mtsp	t1,sr5

	ldw	SS_SR6(t2),t1
	mtsp	t1,sr6

	ldw	SS_SR7(t2),t1
	mtsp	t1,sr7

	/*
	 * finally we can restore the space and offset queues and the ipsw
	 */

	ldw	SS_IIOQH(t2),t1
	mtctl	t1,pcoq

	ldw	SS_IIOQT(t2),t1
	mtctl	t1,pcoq

	ldw	SS_IISQH(t2),t1
	mtctl	t1,pcsq

	ldw	SS_IISQT(t2),t1
	mtctl	t1,pcsq

	ldw	SS_CR22(t2),t1
	mtctl	t1,ipsw

	/*
	 * restore the last registers,r30, r22, and finally r21(t2)
	 */
	ldw	SS_R30(t2),sp
	ldw	SS_R22(t2),t1
	ldw	SS_R21(t2),t2

	rfi
	nop

$finalize_trap
	/*
	 * clear the system mask, this puts us back into physical mode
	 * for kernel space return.
	 *
	 * t2 contains the address of temporary area
	 *
	 * N.B: Better not be any code translation traps from this point
	 * on. Of course, we know this routine could never be *that* big.
	 */
	rsm	RESET_PSW,r0

	/*
	 * finally we can restore the space and offset queues and the ipsw
	 */

	ldw	TRAP_IIOQH(t2),t1
	mtctl	t1,pcoq

	ldw	TRAP_IIOQT(t2),t1
	mtctl	t1,pcoq

	ldw	TRAP_IISQH(t2),t1
	mtctl	t1,pcsq

	ldw	TRAP_IISQT(t2),t1
	mtctl	t1,pcsq

	ldw	TRAP_CR22(t2),t1
	mtctl	t1,ipsw

	/*
	 * restore the last registers, r30, r22, and finally r21(t2)
	 */
	ldw	TRAP_R30(t2),sp
	ldw	TRAP_R22(t2),t1
	ldw	TRAP_R21(t2),t2

	rfi
	nop

	.procend

/*
 * thread_bootstrap_return()
 *
 * Jump into user mode directly (Ha!) when starting a new thread.
 */
	.export	thread_bootstrap_return,entry
	.proc
	.callinfo
thread_bootstrap_return
	/*
	 * We are running on the proper stack, so knock it back to the
	 * kernel state frame at the base, and branch into bootstrap code
	 * to let it do the work.
	 */
	ldil	L%KERNEL_STACK_MASK,t1
	ldo	R%KERNEL_STACK_MASK(t1),t1
	andcm   sp,t1,sp

	ldil	L%need_ast,t2
	ldw	R%need_ast(t2),t2
	comib,= 0,t2,$bootstrap_noast
	ldo	TF_SIZE(sp),sp

	bl	hp_pa_astcheck,rp
	nop

        mtctl   ret0,eiem
	/* old spl level restored, but interrupts still disabled */

$bootstrap_noast
	ldil	L%cpu_data,t2
	ldw	R%cpu_data(t2),t2

	ldw	THREAD_TOP_ACT(t2),t2

	ldw	ACT_KLOADING(t2),t1
	combt,= r0,t1,$no_kloaded

	stw	r0,ACT_KLOADING(t2)
	addi	1,r0,t1
	stw	t1,ACT_KLOADED(t2)
	ldil	L%active_kloaded,t1
	ldo	R%active_kloaded(t1),t1
	stw	t2,0(t1)

$no_kloaded
	/*
	 * Load the PCB_SS pointer into t3 for the jump into the bootstrap
	 * code.
	 */
	ldw	ACT_PCB(t2),t2

	b	$thread_return
	ldo	PCB_SS(t2),t3

	/*
	 * Never returns.
	 */
	.procend
	.end
