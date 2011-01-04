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

#include <cpus.h>
#include <debug.h>
#include <assym.s>
#include <mach_prof.h>
	
#include <mach/machine/vm_param.h>
#include <ppc/proc_reg.h>
#include <ppc/asm.h>
#include <ppc/spl.h>

/* 
 * boolean_t
 * klcopyinstr (mach_port_t target,
 *		vm_offset_t src,
 *		vm_offset_t dst,
 *		int maxcount,
 *		int *out)
 *
 * Call the 'copyin string' function from a kernel loaded server. We're
 * running in a server thread on its user stack, so switch to the thread's
 * kernel stack to perform the 'copyinstr' operation then switch back.
 */

ENTRY(klcopyinstr, TAG_NO_FRAME_USED)

	/* Save off some user state on the user stack if we need to */

	stwu	r1,	-(FM_SIZE+28)(r1)
	mflr	r0

	stw	r25,	FM_SIZE(r1)
	stw	r26,	FM_SIZE+4(r1)
	stw	r27,	FM_SIZE+8(r1)
	stw	r28,	FM_SIZE+12(r1)
	stw	r29,	FM_SIZE+16(r1)
	stw	r30,	FM_SIZE+20(r1)
	stw	r31,	FM_SIZE+24(r1)
	
	stw	r0,	FM_SIZE+28+FM_LR_SAVE(r1)
	
	/* copy our arguments into callee-saved registers now */
	mr	r31,	ARG1
	mr	r30,	ARG2
	mr	r29,	ARG3
	mr	r28,	ARG4
	
	/* Find the current thread's kernel stack to switch to */	

	mr	r25,	r1	/* temp save of user stack ptr */

	lwz	r27,	PP_CPU_DATA(r2)
	lwz	r27,	CPU_ACTIVE_THREAD(r27)
	lwz	r27,	THREAD_TOP_ACT(r27)
	lwz	r26,	ACT_MACT_PCB(r27)
	lwz	r1,	PCB_KSP(r26)

	/* reserve ourselves a frame on the stack */
	stwu	r1,	-FM_SIZE(r1)

	/* We're at the top of our kernel stack now, mark as busy */

	li	r0,	0
	stw	r0,	PCB_KSP(r26)

	/* save off the user's MSR value, since the kernel may want
	 * to disable our FPU from under our feet
	 */
	mfmsr	r0
	stw	r0,	SS_SRR1(r26)
	
#if DEBUG
	/* Mark as a kl-calling routine by some magic in pcb srr0 for debug */
	lis	r0,	0x000ca11ed@h
	ori	r0,	r0,	0x000ca11ed@l
	stw	r0,	SS_SRR0(r26)
	
	/* Store a debugging backpointer frame */
	lwz	r26,	FM_BACKPTR(r1)	
	stw	r25,	FM_BACKPTR(r26)
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	
	stwu	r1,	-FM_SIZE(r1)

	/* Assert that r2 wasn't modified by server */
	/* TODO NMGS - assumes 1-1 mapping of kernel data */
	mfsprg	r0,	0
	twne	r2,	r0
	
#endif /* DEBUG */

	/* Now do the work we actually want to do */
	
	bl	port_name_to_map

	/* Quit if we can't find a pmap */
	cmpwi	CR0,	ARG0,	0
	bne+	.L_klcopyinstr_ok

	li	r31,	1		/* Return code */
	b	.L_klcopyinstr_out

.L_klcopyinstr_ok:

	/* Save our current map */

	lwz	r26,	ACT_VMMAP(r27)
	
	/* Change the current map to that of the user process */

	stw	ARG0,	ACT_VMMAP(r27)

	mr	ARG0,	r31
	mr	ARG1,	r30
	mr	ARG2,	r29
	mr	ARG3,	r28
	
	bl	copyinstr

	/* Save  return code */
	mr	r31,	ARG0

	lwz	ARG0,	ACT_VMMAP(r27)

	bl	vm_map_deallocate

	/* restore our map */
	
	stw	r26,	ACT_VMMAP(r27)
	
.L_klcopyinstr_out:

	/* We require the value that we're going to return to be in r31 */

.L_klcopyinstr_check_ast:
	
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_klcopyinstr_no_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */

	/* disable interrupts */

	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	isync		/* For mtmsr */

	li	ARG0,	0
	li	ARG1,	AST_ALL


	/* Perform splhigh and put old spl value in ARG2.
	 * Interrupts have already been disabled
	 */

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
		/* loop back and check again */
	b	.L_klcopyinstr_check_ast

.L_klcopyinstr_no_ast:

	/* recover return code */
	
	mr	ARG0,	r31

	/* free the kernel stack , unwind the user stack and return */
	
#if DEBUG
	/* skip debugging frame */
	lwz	r1,	FM_BACKPTR(r1)
#endif /* DEBUG */
	/* skip back over our frame */
	lwz	r1,	FM_BACKPTR(r1)
	
	/* reset the server's MSR which may or may not have had its FPU
	 * disabled
	 */
	lwz	r26,	ACT_MACT_PCB(r27)
	lwz	r0,	SS_SRR1(r26)
	mtmsr	r0
	isync
	
	stw	r1,	PCB_KSP(r26)

	/* back on to user stack ... */
	
	mr	r1,	r25
		
	/* recover callee saved registers */

	lwz	r0,	FM_SIZE+28+FM_LR_SAVE(r1)

	lwz	r25,	FM_SIZE(r1)
	lwz	r26,	FM_SIZE+4(r1)
	lwz	r27,	FM_SIZE+8(r1)
	lwz	r28,	FM_SIZE+12(r1)
	lwz	r29,	FM_SIZE+16(r1)
	lwz	r30,	FM_SIZE+20(r1)
	lwz	r31,	FM_SIZE+24(r1)

	mtlr	r0
	
	lwz	r1,	FM_BACKPTR(r1)

	blr

/* 
 * boolean_t
 * klcopyin (mach_port_t target,
 *	     vm_offset_t src,
 *	     vm_offset_t dst,
 *	     int count)
 *
 * Call the 'copyin' function from a kernel loaded server. We're
 * running in a server thread on its user stack, so switch to the thread's
 * kernel stack to perform the 'copyin' operation then switch back.
 */

ENTRY(klcopyin, TAG_NO_FRAME_USED)

	/* Save off some user state on the user stack if we need to */

	stwu	r1,	-(FM_SIZE+28)(r1)
	mflr	r0

	stw	r25,	FM_SIZE(r1)
	stw	r26,	FM_SIZE+4(r1)
	stw	r27,	FM_SIZE+8(r1)
/*	stw	r28,	FM_SIZE+12(r1) */
	stw	r29,	FM_SIZE+16(r1)
	stw	r30,	FM_SIZE+20(r1)
	stw	r31,	FM_SIZE+24(r1)
	
	stw	r0,	FM_SIZE+28+FM_LR_SAVE(r1)
	
	/* copy our arguments into callee-saved registers now */
	mr	r31,	ARG1
	mr	r30,	ARG2
	mr	r29,	ARG3
/*	mr	r28,	ARG4 */
	
	/* Find the current thread's kernel stack to switch to*/	

	mr	r25,	r1	/* temp save of user stack ptr */

	lwz	r27,	PP_CPU_DATA(r2)
	lwz	r27,	CPU_ACTIVE_THREAD(r27)
	lwz	r27,	THREAD_TOP_ACT(r27)
	lwz	r26,	ACT_MACT_PCB(r27)
	lwz	r1,	PCB_KSP(r26)

	/* reserve ourselves a frame on the stack */
	stwu	r1,	-FM_SIZE(r1)

	/* We're at the top of our kernel stack now, mark as busy */

	li	r0,	0
	stw	r0,	PCB_KSP(r26)

	/* save off the user's MSR value, since the kernel will want
	 * to disable our FPU from under our feet
	 */
	mfmsr	r0
	stw	r0,	SS_SRR1(r26)
	
#if DEBUG
	/* Mark as a kl-calling routine by some magic in pcb srr0 for debug */
	lis	r0,	0x000ca11ed@h
	ori	r0,	r0,	0x000ca11ed@l
	stw	r0,	SS_SRR0(r26)
	
	/* Store a debugging backpointer frame */
	lwz	r26,	FM_BACKPTR(r1)	
	stw	r25,	FM_BACKPTR(r26)

	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	
	stwu	r1,	-FM_SIZE(r1)

	/* Assert that r2 wasn't modified by server */
	/* TODO NMGS - assumes 1-1 mapping of kernel data */
	mfsprg	r0,	0
	twne	r2,	r0
#endif /* DEBUG */

	/* Now do the work we actually want to do */
	
	bl	port_name_to_map

	/* Quit if we can't find a pmap */
	cmpwi	CR0,	ARG0,	0
	bne+	.L_klcopyin_ok

	li	r31,	1		/* Return code */
	b	.L_klcopyin_out

.L_klcopyin_ok:

	/* Save our current map */

	lwz	r26,	ACT_VMMAP(r27)
	
	/* Change the current map to that of the user process */

	stw	ARG0,	ACT_VMMAP(r27)

	mr	ARG0,	r31
	mr	ARG1,	r30
	mr	ARG2,	r29

	bl	copyin

	/* Save return code */
	mr	r31,	ARG0

	lwz	ARG0,	ACT_VMMAP(r27)

	bl	vm_map_deallocate

	/* restore our map */
	
	stw	r26,	ACT_VMMAP(r27)
	
.L_klcopyin_out:
	/* We require the value that we're going to return to be in r31 */
	
	/* We require the value that we're going to return to be in r31 */

.L_klcopyin_check_ast:
	
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_klcopyin_no_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */

	/* disable interrupts */

	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	isync		/* For mtmsr */

	li	ARG0,	0
	li	ARG1,	AST_ALL


	/* Perform splhigh and put old spl value in ARG2.
	 * Interrupts have already been disabled
	 */

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
		/* loop back and check again */
	b	.L_klcopyin_check_ast

.L_klcopyin_no_ast:

	/* recover return code */
	
	mr	ARG0,	r31

	/* free the kernel stack , unwind the user stack and return */
	
#if DEBUG
	/* skip debugging frame */
	lwz	r1,	FM_BACKPTR(r1)
#endif /* DEBUG */
	/* skip back over our frame */
	lwz	r1,	FM_BACKPTR(r1)
	
	/* reset the server's MSR which may or may not have had its FPU
	 * disabled
	 */
	lwz	r26,	ACT_MACT_PCB(r27)
	lwz	r0,	SS_SRR1(r26)
	mtmsr	r0
	isync
	
	stw	r1,	PCB_KSP(r26)

	/* back on to user stack ... */
	
	mr	r1,	r25
		
	/* recover callee saved registers */

	lwz	r0,	FM_SIZE+28+FM_LR_SAVE(r1)

	lwz	r25,	FM_SIZE(r1)
	lwz	r26,	FM_SIZE+4(r1)
	lwz	r27,	FM_SIZE+8(r1)
/*	lwz	r28,	FM_SIZE+12(r1) */
	lwz	r29,	FM_SIZE+16(r1)
	lwz	r30,	FM_SIZE+20(r1)
	lwz	r31,	FM_SIZE+24(r1)

	mtlr	r0
	
	lwz	r1,	FM_BACKPTR(r1)

	blr

/* 
 * boolean_t
 * klcopyout (mach_port_t target,
 *	      vm_offset_t src,
 *	      vm_offset_t dst,
 *	      int count)
 *
 * Call the 'copyout' function from a kernel loaded server. We're
 * running in a server thread on its user stack, so switch to the thread's
 * kernel stack to perform the 'copyout' operation then switch back.
 */

ENTRY(klcopyout, TAG_NO_FRAME_USED)

	/* Save off some user state on the user stack if we need to */

	stwu	r1,	-(FM_SIZE+28)(r1)
	mflr	r0

	stw	r25,	FM_SIZE(r1)
	stw	r26,	FM_SIZE+4(r1)
	stw	r27,	FM_SIZE+8(r1)
/*	stw	r28,	FM_SIZE+12(r1) */
	stw	r29,	FM_SIZE+16(r1)
	stw	r30,	FM_SIZE+20(r1)
	stw	r31,	FM_SIZE+24(r1)
	
	stw	r0,	FM_SIZE+28+FM_LR_SAVE(r1)
	
	/* copy our arguments into callee-saved registers now */
	mr	r31,	ARG1
	mr	r30,	ARG2
	mr	r29,	ARG3
/*	mr	r28,	ARG4 */
	
	/* Find the current thread's kernel stack to switch to*/	

	mr	r25,	r1	/* temp save of user stack ptr */

	lwz	r27,	PP_CPU_DATA(r2)
	lwz	r27,	CPU_ACTIVE_THREAD(r27)
	lwz	r27,	THREAD_TOP_ACT(r27)
	lwz	r26,	ACT_MACT_PCB(r27)
	lwz	r1,	PCB_KSP(r26)

	/* reserve ourselves a frame on the stack */
	stwu	r1,	-FM_SIZE(r1)

	/* We're at the top of our kernel stack now, mark as busy */

	li	r0,	0
	stw	r0,	PCB_KSP(r26)

	/* save off the user's MSR value, since the kernel will want
	 * to disable our FPU from under our feet
	 */
	mfmsr	r0
	stw	r0,	SS_SRR1(r26)
	
#if DEBUG
	/* Mark as a kl-calling routine by some magic in pcb srr0 for debug */
	lis	r0,	0x000ca11ed@h
	ori	r0,	r0,	0x000ca11ed@l
	stw	r0,	SS_SRR0(r26)
	
	/* Store a debugging backpointer frame */
	lwz	r26,	FM_BACKPTR(r1)	
	stw	r25,	FM_BACKPTR(r26)

	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	
	stwu	r1,	-FM_SIZE(r1)

	/* Assert that r2 wasn't modified by server */
	/* TODO NMGS - assumes 1-1 mapping of kernel data */
	mfsprg	r0,	0
	twne	r2,	r0
	
#endif /* DEBUG */

	/* Now do the work we actually want to do */
	
	bl	port_name_to_map

	/* Quit if we can't find a pmap */
	cmpwi	CR0,	ARG0,	0
	bne+	.L_klcopyout_ok

	li	r31,	1		/* return code */
	b	.L_klcopyout_out

.L_klcopyout_ok:

	/* Save our current map */

	lwz	r26,	ACT_VMMAP(r27)
	
	/* Change the current map to that of the user process */

	stw	ARG0,	ACT_VMMAP(r27)

	mr	ARG0,	r31
	mr	ARG1,	r30
	mr	ARG2,	r29

	bl	copyout

	/* Save return code */
	mr	r31,	ARG0

	lwz	ARG0,	ACT_VMMAP(r27)

	bl	vm_map_deallocate

	/* restore our map */
	
	stw	r26,	ACT_VMMAP(r27)

.L_klcopyout_out:

	/* We require the value that we're going to return to be in r31 */
	
	/* We require the value that we're going to return to be in r31 */

.L_klcopyoutcheck_ast:
	
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_klcopyoutno_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */

	/* disable interrupts */

	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	isync		/* For mtmsr */

	li	ARG0,	0
	li	ARG1,	AST_ALL


	/* Perform splhigh and put old spl value in ARG2.
	 * Interrupts have already been disabled
	 */

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
		/* loop back and check again */
	b	.L_klcopyoutcheck_ast

.L_klcopyoutno_ast:

	/* recover return code */
	
	mr	ARG0,	r31

	/* free the kernel stack , unwind the user stack and return */
	
#if DEBUG
	/* skip debugging frame */
	lwz	r1,	FM_BACKPTR(r1)
#endif /* DEBUG */
	/* skip back over our frame */
	lwz	r1,	FM_BACKPTR(r1)
	
	/* reset the server's MSR which may or may not have had its FPU
	 * disabled
	 */
	lwz	r26,	ACT_MACT_PCB(r27)
	lwz	r0,	SS_SRR1(r26)
	mtmsr	r0
	isync
	
	stw	r1,	PCB_KSP(r26)

	/* back on to user stack ... */
	
	mr	r1,	r25
		
	/* recover callee saved registers */

	lwz	r0,	FM_SIZE+28+FM_LR_SAVE(r1)

	lwz	r25,	FM_SIZE(r1)
	lwz	r26,	FM_SIZE+4(r1)
	lwz	r27,	FM_SIZE+8(r1)
/*	lwz	r28,	FM_SIZE+12(r1) */
	lwz	r29,	FM_SIZE+16(r1)
	lwz	r30,	FM_SIZE+20(r1)
	lwz	r31,	FM_SIZE+24(r1)

	mtlr	r0
	
	lwz	r1,	FM_BACKPTR(r1)

	blr

/*
 * kern_return_t
 * klthread_switch(mach_port_t thread, int option, int option_time)
 *
 * Call the kernel's thread_switch function from a kernel-loaded
 * server.  We assume that we're running in a server thread on its
 * user stack, so just switch to the thread's kernel stack "around"
 * the call.
 */

ENTRY(klthread_switch, TAG_NO_FRAME_USED)


	/* Save off some user state on the user stack if we need to */

	stwu	r1,	-(FM_SIZE+12)(r1)
	mflr	r0

	stw	r31,	FM_SIZE+8(r1)
	stw	r30,	FM_SIZE+4(r1)
	stw	r29,	FM_SIZE(r1)
	
	stw	r0,	FM_SIZE+12+FM_LR_SAVE(r1)
	
	/* Find the current thread's kernel stack to switch to*/	

	mr	r31,	r1	/* temp save of user stack ptr */

#ifdef	DEBUG
	/* Assert that r2 wasn't modified by server */
	/* TODO NMGS - assumes 1-1 mapping of kernel data */
	mfsprg	r0,	0
	twne	r2,	r0
#endif	/* DEBUG */
	
	lwz	r30,	PP_CPU_DATA(r2)
	lwz	r30,	CPU_ACTIVE_THREAD(r30)
	lwz	r30,	THREAD_TOP_ACT(r30)
	lwz	r29,	ACT_MACT_PCB(r30)
	lwz	r1,	PCB_KSP(r29)

	/* reserve ourselves a frame on the stack */
	stwu	r1,	-FM_SIZE(r1)

	/* We're at the top of our kernel stack now, mark as busy */

	li	r0,	0
	stw	r0,	PCB_KSP(r29)

	/* save off the user's MSR value, since the kernel might want
	 * to disable our FPU from under our feet
	 */
	mfmsr	r0
	stw	r0,	SS_SRR1(r29)
	
#if DEBUG
	/* Mark as a kl-calling routine by some magic in pcb srr0 for debug */
	lis	r0,	0x000ca11ed@h
	ori	r0,	r0,	0x000ca11ed@l
	stw	r0,	SS_SRR0(r29)
	
	/* Store a debugging backpointer frame */
	lwz	r29,	FM_BACKPTR(r1)
	stw	r31,	FM_BACKPTR(r29)
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	
	stwu	r1,	-FM_SIZE(r1)

#endif /* DEBUG */
		
	/* Now do the work we actually want to do */
	
	bl	syscall_thread_switch

	/* Save result for after AST check */
	
	mr	r29,	ARG0
	
	/* Check if there's an AST outstanding, we're about to
	 * "leave the kernel"
	 */
.L_klthread_switch_check_ast:	
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_klthread_switch_no_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */

	/* disable interrupts */

	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	isync		/* For mtmsr */

	li	ARG0,	0
	li	ARG1,	AST_ALL


	/* Perform splhigh and put old spl value in ARG2.
	 * Interrupts have already been disabled
	 */

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
		/* loop back and check again */
	b	.L_klthread_switch_check_ast

.L_klthread_switch_no_ast:

	/* recover the result we want to return */

	mr	ARG0,	r29
	
	/* free the kernel stack , unwind the user stack and return */
	
#if DEBUG
	/* skip debugging frame */
	lwz	r1,	FM_BACKPTR(r1)
#endif /* DEBUG */
	/* skip back over our frame */
	lwz	r1,	FM_BACKPTR(r1)
	

	/* reset the server's MSR which may or may not have had its FPU
	 * disabled
	 */
	lwz	r29,	ACT_MACT_PCB(r30)
	lwz	r0,	SS_SRR1(r29)
	mtmsr	r0
	isync
	
	stw	r1,	PCB_KSP(r29)

	/* back on to user stack ... */
	
	mr	r1,	r31
		
	/* recover callee saved registers */

	lwz	r0,	FM_SIZE+12+FM_LR_SAVE(r1)

	lwz	r29,	FM_SIZE(r1)
	lwz	r30,	FM_SIZE+4(r1)
	lwz	r31,	FM_SIZE+8(r1)

	mtlr	r0
	
	lwz	r1,	FM_BACKPTR(r1)

	blr


/* 
 * kern_return_t
 * klthread_depress_abort(mach_port_t name)
 *
 * Call the kernel's thread_depress_abort function from a kernel-loaded
 * server.  We assume that we're running in a server thread on its
 * user stack, so just switch to the thread's kernel stack "around"
 * the call.
 */

ENTRY(klthread_depress_abort, TAG_NO_FRAME_USED)


	/* Save off some user state on the user stack if we need to */

	stwu	r1,	-(FM_SIZE+12)(r1)
	mflr	r0

	stw	r31,	FM_SIZE+8(r1)
	stw	r30,	FM_SIZE+4(r1)
	stw	r29,	FM_SIZE(r1)
	
	stw	r0,	FM_SIZE+12+FM_LR_SAVE(r1)
	
	/* Find the current thread's kernel stack to switch to*/	

	mr	r31,	r1	/* temp save of user stack ptr */

	lwz	r30,	PP_CPU_DATA(r2)
	lwz	r30,	CPU_ACTIVE_THREAD(r30)
	lwz	r30,	THREAD_TOP_ACT(r30)
	lwz	r29,	ACT_MACT_PCB(r30)
	lwz	r1,	PCB_KSP(r29)

	/* reserve ourselves a frame on the stack */
	stwu	r1,	-FM_SIZE(r1)

	/* We're at the top of our kernel stack now, mark as busy */

	li	r0,	0
	stw	r0,	PCB_KSP(r29)

	/* save off the user's MSR value, since the kernel might want
	 * to disable our FPU from under our feet
	 */
	mfmsr	r0
	stw	r0,	SS_SRR1(r29)
	
#if DEBUG
	/* Mark as a kl-calling routine by some magic in pcb srr0 for debug */
	li	r0,	0x000ca11ed@h
	ori	r0,	r0,	0x000ca11ed@l
	stw	r0,	SS_SRR0(r29)
	
	/* Store a debugging backpointer frame */
	lwz	r29,	FM_BACKPTR(r1)	
	stw	r31,	FM_BACKPTR(r29)

	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	
	stwu	r1,	-FM_SIZE(r1)

	/* Assert that r2 wasn't modified by server */
	/* TODO NMGS - assumes 1-1 mapping of kernel data */
	mfsprg	r0,	0
	twne	r2,	r0
#endif /* DEBUG */

	/* Now do the work we actually want to do */
	
	bl	syscall_thread_depress_abort

	/* Save result for after AST check */
	
	mr	r29,	ARG0
	
	/* Check if there's an AST outstanding, we're about to
	 * "leave the kernel"
	 */

.L_klthread_depress_abort_check_ast:	
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_klthread_depress_abort_no_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */

	/* disable interrupts */

	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	isync

	li	ARG0,	0
	li	ARG1,	AST_ALL

	/* Perform splhigh and put old spl value in ARG2.
	 * Interrupts have already been disabled
	 */

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
	/* check again before we go out */
	b	.L_klthread_depress_abort_check_ast

.L_klthread_depress_abort_no_ast:

	/* recover the result we want to return */

	mr	ARG0,	r29
	
	/* free the kernel stack , unwind the user stack and return */
	
#if DEBUG
	/* skip debugging frame */
	lwz	r1,	FM_BACKPTR(r1)
#endif /* DEBUG */
	/* skip back over our frame */
	lwz	r1,	FM_BACKPTR(r1)

	/* reset the server's MSR which may or may not have had its FPU
	 * disabled
	 */
	lwz	r29,	ACT_MACT_PCB(r30)
	lwz	r0,	SS_SRR1(r29)
	mtmsr	r0
	isync

	stw	r1,	PCB_KSP(r29)

	/* back on to user stack ... */
	
	mr	r1,	r31
		
	/* recover callee saved registers */

	lwz	r0,	FM_SIZE+12+FM_LR_SAVE(r1)

	lwz	r29,	FM_SIZE(r1)
	lwz	r30,	FM_SIZE+4(r1)
	lwz	r31,	FM_SIZE+8(r1)

	mtlr	r0
	
	lwz	r1,	FM_BACKPTR(r1)

	blr

/*
 * void
 * call_exc_serv(
 *		 mach_port_t exc_port, exception_type_t exception,
 *		 exception_data_t code, mach_msg_type_number_t codeCnt,
 *		 int *new_sp, mig_impl_routine_t func,
 *		 int *flavor, thread_state_t statep, unsigned int *state_cnt)
 *
 * Switch stacks, call the function, and switch back.  Since
 * we're switching stacks, the asm fragment can't use auto's
 * for temporary storage; use a callee-saves register instead.
 *
 * Don't save anything in the user stack, since caller ensures that
 * all data queried from server work function resides in stable,
 * act-private storage, not on (shared) kernel stack.
 */

ENTRY(call_exc_serv, TAG_NO_FRAME_USED)

	/* reserve a frame for us on the server's stack, with an argument */
	
	subi	r31,	ARG4,	FM_SIZE+4
	
	mtctr	ARG5			/* Save off func */

	mr	ARG4,	ARG6		/* move flavor down to ARG4 */
	mr	ARG5,	ARG7		/* move statep down to ARG5 */

	lwz	ARG6,	FM_ARG0(r1)	/* Recover state_cnt from stack */
	stw	ARG6,	FM_ARG0(r31)	/* Put state_cnt into newsp->arg8 */
		
	lwz	ARG6,	0(ARG6)		/* put *state_cnt into ARG6 */

					/* still holds state_ptr : ARG7 */

	/* Find top of kernel stack and mark kernel stack as free */
	
	lwz	r29,	PP_CPU_DATA(r2)
	lwz	r29,	CPU_ACTIVE_THREAD(r29)
	lwz	r28,	THREAD_TOP_ACT(r29)
	lwz	r28,	ACT_MACT_PCB(r28)

	lwz	r27,	THREAD_KERNEL_STACK(r29)

	ori	r27,	r27,	KERNEL_STACK_SIZE-KS_SIZE-FM_SIZE
	stw	r27,	PCB_KSP(r28)		/* mark stack as free */

	lwz	r0,	SS_SRR1(r28)
	mtmsr	r0
	isync

	mr	r1,	r31	/* Move onto new stack */

	bctrl			/* call server *func */

	/* Move back onto kernel stack + mark busy
	 * using pointers still in callee saved regs
	 */
	
	mr	r1,	r27

	li	r0,	0
	stw	r0,	PCB_KSP(r28)

	/* From here, we never return */
#if DEBUG
	/* Assert that r2 wasn't modified by server */
	/* TODO NMGS - assumes 1-1 mapping of kernel data */
	mfsprg	r0,	0
	twne	r2,	r0
#endif /* DEBUG */
		
	b	exception_return_wrapper

