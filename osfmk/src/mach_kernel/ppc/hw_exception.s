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

/* Low level routines dealing with exception entry and exit.
 * There are various types of exception:
 *
 *    Interrupt, trap, system call and debugger entry. Each has it's own
 *    handler since the state save routine is different for each. The
 *    code is very similar (a lot of cut and paste).
 *
 *    The code for the FPU disabled handler (lazy fpu) is in cswtch.s
 */

#include <debug.h>
#include <mach_assert.h>

#include <mach_kgdb.h>
#include <mach/exception.h>
#include <mach/ppc/vm_param.h>

#include <assym.s>

#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <ppc/trap.h>
#include <ppc/exception.h>
#include <ppc/spl.h>
	
/*
 * thandler(type)
 *
 * ENTRY:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info 
 *		r3 = exception type (one of T_...)
 */

/*
 * If pcb.ksp == 0 then the kernel stack is already busy,
 *                 we save ppc_saved state below the current stack pointer,
 *		   leaving enough space for the `red zone' in case the
 *		   trapped thread was in the middle of saving state below
 *		   its stack pointer.
 *
 * otherwise       we save a ppc_saved_state in the pcb, and switch to
 * 		   the kernel stack (setting pcb.ksp to 0)
 *
 * on return, we do the reverse, the last state is popped from the pcb
 * and pcb.ksp is set to the top of stack below the kernel state + frame
 * TODO NMGS - is this frame used? I don't think so
 *
 *                    Diagram of a thread's kernel stack
 *
 *               --------------- 	TOP OF STACK
 *              |kernel_state   |
 *              |---------------|
 *              |backpointer FM |
 *              |---------------|                         	   	
 *              |... C usage ...|                         	   	
 *              |               |                         	   	
 *              |---------------|                       TRAP IN KERNEL CODE
 *              |ppc_saved_state|                       STATE SAVED HERE   
 *              |---------------|                         		
 *              |backpointer FM |                                       
 *              |---------------|                         
 *              |... C usage ...|                         
 *              |               |                         
 *              |               |                         
 *              |               |                         
 *              |               |                         
 */


#if DEBUG

/* TRAP_SPACE_NEEDED is the space assumed free on the kernel stack when
 * another trap is taken. We need at least enough space for a saved state
 * structure plus two small backpointer frames, and we add a few
 * hundred bytes for the space needed by the C (which may be less but
 * may be much more). We're trying to catch kernel stack overflows :-)
 */

#define TRAP_SPACE_NEEDED	FM_REDZONE+SS_SIZE+(2*FM_SIZE)+256

#endif /* DEBUG */

	.text

ENTRY(thandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

#if MACH_KGDB
		/* If we're on the gdb stack, there has probably been
		 * a fault reading user memory or something like that,
		 * so we should pass this to the gdb handler. NOTE
		 * we may have entered gdb through an interrupt handler
		 * (keyboard or serial line, for example), so interrupt
		 * stack may be busy too.
		 */
	lwz	r1, PP_GDBSTACKPTR(r2)
	cmpwi	CR0,	r1,	0
	beq-	CR0,	gdbhandler
#endif /* MACH_KGDB */
	
#if DEBUG
		/* Make sure we're not on the interrupt stack */
	lwz	r1,	PP_ISTACKPTR(r2)
	cmpwi	CR0,	r1,	0

	/* If we are on the interrupt stack, treat as an interrupt,
	 * the interrupt handler will panic with useful info.
	 */

	beq-	CR0,	ihandler
	
#endif /* DEBUG */

	lwz	r3,	PP_CPU_DATA(r2)

	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r3,	THREAD_TOP_ACT(r3)
	lwz	r3,	ACT_MACT_PCB(r3)
	lwz	r1,	PCB_KSP(r3)

	cmpwi	CR1,	r1,	0	/* zero implies already on kstack */
	bne	CR1,	.L_kstackfree	/* This test is also used below */

	mfsprg	r1,	1		/* recover previous stack ptr */

	/* On kernel stack, allocate stack frame and check for overflow */
#if DEBUG
        /*
         * Test if we will overflow the Kernel Stack. We 
         * check that there is at least TRAP_SPACE_NEEDED bytes
         * free on the kernel stack
	 */

	/* We don't have any free registers, trash r3 and reload it */

	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r3,	THREAD_KERNEL_STACK(r3)
	addi	r3,	r3,	TRAP_SPACE_NEEDED
	cmp	CR0,	r1,	r3
	bgt+	.L_no_ks_overflow

	b	gdbhandler
	
.L_no_ks_overflow:
	/* Reload r3 with current pcb */
	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r3,	THREAD_TOP_ACT(r3)
	lwz	r3,	ACT_MACT_PCB(r3)

#endif /* DEBUG */

	/* Move stack pointer below redzone + reserve a saved_state */

	subi	r1,	r1,	FM_REDZONE+SS_SIZE 

	b	.L_kstack_save_state

.L_kstackfree:
	mr	r1,	r3		/* r1 points to save area of pcb */

.L_kstack_save_state:	

	/* Once we reach here, r1 contains the place
         * where we can store a ppc_saved_state structure. This may
	 * or may not be part of a pcb, we test that again once
	 * we've saved state. (CR1 still holds test done on ksp)
	 */

	stw	r0,	SS_R0(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)
	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)

	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 * pointed to by r2
	 */

	/* WARNING! These two instructions assume that we didn't take
	 * any type of exception whilst saving state, it's a bit late
	 * for that!
	 * TODO NMGS move these up the code somehow, put in PROC_REG?
	 */

	mfdsisr	ARG2			/* r4	*/
	mfdar	ARG3			/* r5	*/

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r0,	PP_SAVE_CTR(r2)
	stw	r0,	SS_CTR(r1)

	lwz	ARG6,	PP_SAVE_SRR0(r2)	/* ARG6 used below */
	stw	ARG6,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	/* work out if we will reenable interrupts or not depending
	 * upon the state which we came from, store as tmp in ARG5
	 */
	li	ARG5,	MSR_SUPERVISOR_INT_OFF
	rlwimi	ARG5,	r0,	0,	MSR_EE_BIT,	MSR_EE_BIT

	mfxer	r0
	stw	r0,	SS_XER(r1)

	mflr	r0
	stw	r0,	SS_LR(r1)

	/* Save MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	mfspr	r0,	mq
	stw	r0,	SS_MQ(r1)
	li	r0,	0		/* ensure R0 is zero on 601 before sync */
	isync
0:

	/* Free the reservation whilst saving SR_COPYIN */

	sync
	mfsr	ARG7,	SR_COPYIN
	li	r0,	SS_SR_COPYIN
	stwcx.	ARG7,	r1,	r0
	stw	ARG7,	SS_SR_COPYIN(r1)
	
	/* r3 still holds our pcb, CR1 still holds test to see if we're
	 * in the pcb or have saved state on the kernel stack */

	mr	ARG1,	r1		/* Preserve saved_state ptr in ARG1 */

	beq	CR1,	.L_state_on_kstack/* using above test for pcb/stack */

	/* We saved state in the pcb, recover the stack pointer */
	lwz	r1,	PCB_KSP(r3)

	/* Mark that we're occupying the kernel stack for sure now */	
	li	r0,	0
	stw	r0,	PCB_KSP(r3)

.L_state_on_kstack:	
		
	/* Phew!
	 *
	 * To summarise, when we reach here, we have filled out
	 * a ppc_saved_state structure either in the pcb or on
	 * the kernel stack, and the stack is marked as busy.
	 * r4 holds a pointer to this state, r1 is now the stack
	 * pointer no matter where the state was savd.
	 * We now generate a small stack frame with backpointers
	 * to follow the calling
	 * conventions. We set up the backpointers to the trapped
	 * routine allowing us to backtrace.
	 */

/* WARNING!! Using mfsprg below assumes interrupts are still off here */
	
	subi	r1,	r1,	FM_SIZE
	mfsprg	r0,	1
	stw	r0,	FM_BACKPTR(r1)	/* point back to previous stackptr */

#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 */
	stw	ARG6,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */

	stwu	r1,	-FM_SIZE(r1)	/* and make new frame */
#endif /* DEBUG */


	/* call trap handler proper, with
	 *   ARG0 = type		(not yet, holds pcb ptr)
	 *   ARG1 = saved_state ptr	(already there)
	 *   ARG2 = dsisr		(already there)
	 *   ARG3 = dar			(already there)
	 */

	/* This assumes that no (non-tlb) exception/interrupt has occured
	 * since PP_SAVE_* get clobbered by an exception...
	 */
	lwz	ARG0,	PP_SAVE_EXCEPTION_TYPE(r2)

	/* Reenable interrupts if they were enabled before we came here */
	sync		/* flush prior writes */
	mtmsr	ARG5
	isync

	/* syscall exception might warp here if there's nothing left
	 * to do except generate a trap
	 */
.L_call_trap:	
	bl	EXT(trap)

	/*
	 * Ok, return from C function
	 *
	 * This is also the point where new threads come when they are created.
	 * The new thread is setup to look like a thread that took an 
	 * interrupt and went immediatly into trap.
	 *
	 * r3 must hold the pointer to the saved state, either on the
	 * stack or in the pcb.
	 */

thread_return:
	/* Reload the saved state */

	/* r0-3 will be restored last, use as temp for now */

	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	lwz	r6,	SS_R6(r3)
	lwz	r7,	SS_R7(r3)
	lwz	r8,	SS_R8(r3)
	lwz	r9,	SS_R9(r3)
	lwz	r10,	SS_R10(r3)
	lwz	r11,	SS_R11(r3)
	lwz	r12,	SS_R12(r3)
	lwz	r13,	SS_R13(r3)
	lwz	r14,	SS_R14(r3)
	lwz	r15,	SS_R15(r3)
	lwz	r16,	SS_R16(r3)
	lwz	r17,	SS_R17(r3)
	lwz	r18,	SS_R18(r3)
	lwz	r19,	SS_R19(r3)
	lwz	r20,	SS_R20(r3)
	lwz	r21,	SS_R21(r3)
	lwz	r22,	SS_R22(r3)
	lwz	r23,	SS_R23(r3)
	lwz	r24,	SS_R24(r3)
	lwz	r25,	SS_R25(r3)
	lwz	r26,	SS_R26(r3)
	lwz	r27,	SS_R27(r3)
	lwz	r28,	SS_R28(r3)
	lwz	r29,	SS_R29(r3)
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)

	lwz	r0,	SS_XER(r3)
	mtxer	r0
	lwz	r0,	SS_LR(r3)
	mtlr	r0
	lwz	r0,	SS_CTR(r3)
	mtctr	r0
	lwz	r0,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN,	r0
	isync

	/* Restore MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	lwz	r0,	SS_MQ(r3)
	mtspr	mq,	r0
	li	r0,	0		/* Ensure R0 is zero on 601's before sync */
	isync
0:

	/* Disable interrupts */
	mfmsr	r1
	rlwinm	r1,	r1,	0,	MSR_EE_BIT+1,	MSR_EE_BIT-1
	sync		/* flush prior writes */
	mtmsr	r1


	/* Is this the last saved state, found in the pcb? */
	/* TODO NMGS optimise this by spreading it through the code above? */

	/* After this we no longer to keep &per_proc_info in r2 */
	
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r2,	THREAD_TOP_ACT(r1)
	lwz	r0,	ACT_MACT_PCB(r2)

	cmp	CR0,	r0,	r3
	bne	CR0,	.L_notthelast_trap

	/* our saved state is actually part of the thread's pcb so
	 * we need to mark that we're leaving the kernel stack and
	 * jump into user space
	 */

	/* Mark the kernel stack as free */

	/* There may be a critical region here for traps(interrupts?)
	 * once the stack is marked as free, PCB_SR0 may be trampled on
	 * so interrupts should be switched off
	 */
	/* Release any processor reservation we may have had too */

	lwz	r0,	THREAD_KERNEL_STACK(r1)
		/* Can't use addi since addi converts r0 to 0 value */
	ori	r0,	r0,	KERNEL_STACK_SIZE-KS_SIZE-FM_SIZE
/* we have to use an indirect store to clear reservation */
	li	r2,	PCB_KSP
	sync
	stwcx.	r0,	r2,	r3		/* clear reservation */
	stw	r0,	PCB_KSP(r3)		/* mark stack as free */
	
	/* We may be returning to something in the kernel space.
	 * If we are, we can skip the trampoline and just rfi,
	 * since we don't want to restore the user's space regs
	 */
	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	beq-	.L_trap_ret_to_kspace
	
	/* If jumping into user space, we should restore the user's
	 * segment register 0. We jump via a trampoline in physical mode
	 */
#if DEBUG
	/* Assert that PCB_SR0 isn't in kernel space */
	lwz	r2,	PCB_SR0(r3)
	rlwinm.	r2,	r2,	0,	8,	27
	bne+	00f
	BREAKPOINT_TRAP
00:
#endif /* DEBUG */
		
	lwz	r0,	SS_CR(r3)
	mtcr	r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r1,	0,	exception_exit@ha
	addi	r1,	r1,	exception_exit@l
	lwz	r1,	0(r1)
	mtsrr0	r1
	li	r1,	MSR_VM_OFF
	mtsrr1	r1

	
	lwz	r1,	PCB_SR0(r3)	/* For trampoline */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */

	rfi
	.fill	32			/* 8 words of zeros after rfi for 601*/

.L_trap_ret_to_kspace:	
.L_notthelast_trap:
	/* If we're not the last trap on the kernel stack life is easier,
	 * we don't need to switch back into the user's segment. we can
	 * simply restore the last registers and rfi
	 */

	lwz	r0,	SS_CR(r3)
	mtcr	r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
	/* critical region for traps(interrupt?) since r1 no longer points
	 * to bottom of stack. Could be fixed. But interrupts are off (?).
	 */

	/* set r2 from per_proc info, not from SS */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r2,	0
#if	DEBUG
	/* assert r2 is kosha */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r4,	0
	twne	r2,	r4
#endif	/* DEBUG */
	/* r3 restored last */
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	/* and lastly... */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */
	.fill	32			/* 8 words of zeros after rfi for 601*/

/*
 * shandler(type)
 *
 * ENTRY:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of T_...)
 */

/*
 * If pcb.ksp == 0 then the kernel stack is already busy,
 *                 this is an error - jump to the debugger entry
 * 
 * otherwise       we save a (partial - TODO ) ppc_saved_state
 *                 in the pcb, and, depending upon the type of
 *                 syscall, look it up in the kernel table
 *		   or pass it to the server.
 *
 * on return, we do the reverse, the state is popped from the pcb
 * and pcb.ksp is set to the top of stack.
 */

ENTRY(shandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

#if DEBUG
		/* Make sure we're not on the interrupt stack */
	lwz	r1,	PP_ISTACKPTR(r2)
	cmpwi	CR0,	r1,	0

	/* If we are on the interrupt stack, treat as an interrupt,
	 * the interrupt handler will panic with useful info.
	 */
	beq	ihandler
	
#endif /* DEBUG */
	lwz	r3,	PP_CPU_DATA(r2)

	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r1,	THREAD_TOP_ACT(r3)
	lwz	r1,	ACT_MACT_PCB(r1)
#if DEBUG
		/* Check that we're not on kernel stack already */
	lwz	r1,	PCB_KSP(r1)

	/* If we are on a kernel stack, treat as a interrupt
	 * the interrupt handler will panic with useful info.
	 */
	cmpwi	CR1,	r1,	0
	/* tell the handler that we performed a syscall from this loc */
	li	r3,	T_SYSTEM_CALL
	beq	CR1,	ihandler

/* Reload active thread into r3 and PCB into r1 as before */
	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r1,	THREAD_TOP_ACT(r3)
	lwz	r1,	ACT_MACT_PCB(r1)
#endif /* DEBUG */

	/* Once we reach here, r1 contains the pcb
         * where we can store a partial ppc_saved_state structure,
	 * and r3 contains the active thread structure (used later)
	 */

	/* TODO NMGS - could only save callee saved regs for
	 * many(all?) Mach syscalls, not for unix since might be fork() etc
	 */
	stw	r0,	SS_R0(r1)    /* Save trap number for debugging */

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	/* SAVE ARG REGISTERS? - YES, needed by server system calls */
	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)

	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)

		/*
		 * Callee saved state, need to save in case we
		 * are executing a 'fork' unix system call or similar
		 */

	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)

	/* We use these registers in the code below, save them */
	
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)
	
	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r29,	PP_SAVE_SRR0(r2)  /* Save SRR0 in debug call frame */
	stw	r29,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	oris	r0,	r0,	MSR_SYSCALL_MASK >> 16 /* Mark syscall state */
	stw	r0,	SS_SRR1(r1)
	
	mflr	r0
	stw	r0,	SS_LR(r1)

	lwz	r0,	PP_SAVE_CTR(r2)
	stw	r0,	SS_CTR(r1)

	mfxer	r0
	stw	r0,	SS_XER(r1)

	/* Save MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	mfspr	r0,	mq
	stw	r0,	SS_MQ(r1)
	li	r0,	0	/* Ensure R0 is 0 on 601 before sync */
	isync
0:

	/* Free the reservation whilst saving SR_COPYIN */

	sync
	mfsr	r31,	SR_COPYIN
	li	r0,	SS_SR_COPYIN
	stwcx.	r31,	r1,	r0
	stw	r31,	SS_SR_COPYIN(r1)
	
	/* We saved state in the pcb, recover the stack pointer */
	lwz	r31,	PCB_KSP(r1)	/* Get ksp */

	li	r0,	0
	stw	r0,	PCB_KSP(r1)	/* Mark stack as busy with 0 val */
	
	/* Phew!
	 *
	 * To summarise, when we reach here, we have filled out
	 * a (partial) ppc_saved_state structure in the pcb, moved
	 * to kernel stack, and the stack is marked as busy.
	 * r1 holds a pointer to this state, and r3 holds a
	 * pointer to the active thread. r31 holds kernel stack ptr.
	 * We now move onto the kernel stack and generate a small
	 * stack frame to follow the calling
	 * conventions. We set up the backpointers to the calling
	 * routine allowing us to backtrace.
	 */

	mr	r30,	r1		/* Save pointer to state in r30 */
	mr	r1,	r31		/* move to kernel stack */
	mfsprg	r0,	1		/* get old stack pointer */
	stw	r0,	FM_BACKPTR(r1)	/* store as backpointer */
		
#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 */
	stw	r29,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */
	stwu	r1,	-FM_SIZE(r1)	/* point back to previous frame */
#endif /* DEBUG */

	stwu	r1,	-(FM_SIZE+ARG_SIZE)(r1)

	/* switch on interrupts now kernel stack is busy and valid */
	li	r0,	0
	ori	r0,	r0,	MSR_SUPERVISOR_INT_ON
	sync		/* flush prior writes */
	mtmsr	r0

	/* we should still have r1=ksp, r3(ie ARG0)=current-thread,
	 * r30 = pointer to saved state (in pcb)
	 */
	
	/* Work out what kind of syscall we have to deal with. 
	 */

#if MACH_ASSERT
	/* Call a function that can print out our syscall info */
	mr	r4,	r30
	bl	syscall_trace
	/* restore those volatile argument registers */
	lwz	r4,	SS_R4(r30)
	lwz	r5,	SS_R5(r30)
	lwz	r6,	SS_R6(r30)
	lwz	r7,	SS_R7(r30)
	lwz	r8,	SS_R8(r30)
	lwz	r9,	SS_R9(r30)
	lwz	r10,	SS_R10(r30)
	
#endif /* MACH_ASSERT */
	lwz	r0,	SS_R0(r30)
		
	cmpwi	CR0,	r0,	0
	blt-	.L_kernel_syscall	/* -ve syscalls go to kernel */

	/* +ve syscall - go to server */

.L_call_server_syscall_exception:		
	li	ARG0,	EXC_SYSCALL	/* doexception(EXC_SYSCALL, num, 1) */
.L_call_server_exception:
	mr	ARG1,	r0
	li	ARG2,	1
	
	b	doexception	
	/* Never returns */

/* The above, but with EXC_MACH_SYSCALL */
.L_call_server_mach_syscall:
	li	ARG0,	EXC_MACH_SYSCALL
	b	.L_call_server_exception
	/* Never returns */

	
.L_kernel_syscall:	
	/* Once here, we know that the syscall was -ve
	 * we should still have r1=ksp,
	 * r3(ie ARG0) = pointer to current thread,
	 * r0          = syscall number
	 * r30         = pointer to saved state (in pcb)
	 */

	/*
	 * Are we allowed to do mach system calls?
	 */

	lwz	r29,	THREAD_TOP_ACT(r3)
	lwz	r29,	ACT_MACH_EXC_PORT(r29)

	addis	r31,	0,	realhost@ha
	addi	r31,	r31,	realhost@l
	lwz	r31,	HOST_SELF(r31)

	/* If thread_exception_port == realhost->hostname do syscall */	
	cmpw	r29,	r31
	beq+	.L_syscall_do_mach_syscall

	/* If thread_exception_port != NULL call handler */
	cmpwi	r29,	0
	bne+	.L_call_server_mach_syscall

	lwz	r29,	THREAD_TOP_ACT(r3)
	lwz	r29,	ACT_TASK(r29)
	lwz	r29,	TASK_MACH_EXC_PORT(r29)

	/* If task_exception_port == realhost->hostname do syscall */	
	cmpw	r29,	r31
	beq+	.L_syscall_do_mach_syscall

	/* If task_exception_port != NULL call handler */
	cmpwi	r29,	0
	bne+	.L_call_server_mach_syscall

	/* else the syscall has failed, treat as priv instruction trap,
	 * set SRR1 to indicate privileged instruction
	 */

	lwz	r29,	SS_SRR1(r30)
	oris	r29,	r29,	MASK(SRR1_PRG_PRV_INS)>>16
	stw	r29,	SS_SRR1(r30)
	
	li	ARG0,	T_PROGRAM
	mr	ARG1,	r30
	li	ARG2,	0
	li	ARG3,	0
	b	.L_call_trap

	/* When here, we know that we're allowed to do a mach syscall,
	 * and still have syscall number in r0, pcb pointer in r30
	 */

.L_syscall_do_mach_syscall:
	neg	r31,	r0		/* Make number +ve and put in r31*/

	/* If out of range, call server with syscall exception */
	addis	r29,	0,	mach_trap_count@ha
	addi	r29,	r29,	mach_trap_count@l
	lwz	r29,	0(r29)

	cmp	CR0,	r31,	r29
	bge-	CR0,	.L_call_server_syscall_exception

	addis	r29,	0,	mach_trap_table@ha
	addi	r29,	r29,	mach_trap_table@l
	
	/* multiply the trap number to get offset into table */
	slwi	r31,	r31,	MACH_TRAP_OFFSET_POW2

	/* r31 now holds offset into table of our trap entry,
	 * add on the table base, and it then holds pointer to entry
	 */
	add	r31,	r31,	r29

	/* If the function is kern_invalid, prepare to send an exception.
	   This is messy, but parallels the x86.  We need it for task_by_pid,
	   at least.  */
	lis	r29,	kern_invalid@ha
	addi	r29,	r29,	kern_invalid@l
	lwz	r0,	MACH_TRAP_FUNCTION(r31)
	cmp	CR0,	r0,	r29
	beq-	.L_call_server_syscall_exception

	/* get arg count. If argc > 8 then not all args were in regs,
	 * so we must perform copyin.
	 */
	lwz	r29,	MACH_TRAP_ARGC(r31)
	cmpwi	CR0,	r29,	8
	ble+	.L_syscall_got_args

	/* argc > 8  - perform a copyin */

	/* if the syscall came from kernel space, we can just copy */

	lwz	r0,	SS_SRR1(r30)
	andi.	r0,	r0,	MASK(MSR_PR)
	bne+	.L_syscall_arg_copyin

	/* we came from a privilaged task, just do a copy */

	/* get user's stack pointer */
	lwz	r28,	SS_R1(r30)

	/* change r29 to hold number of args we need to copy */
	subi	r29,	r29,	8

	/* Set user stack pointer to point at copyin args - 4 */
	addi	r28,	r28,	COPYIN_ARG0_OFFSET - 4
	
	/* Make r27 point to address-4 of where we will store copied args */
	addi	r27,	r1,	FM_ARG0-4

.L_syscall_copy_word_loop:
	subi	r29,	r29,	1

	lwz	r0,	4(r28)	/* SHOULD NOT CAUSE PAGE FAULT! */
	addi	r28,	r28,	4

	mr.	r29,	r29

	stw	r0,	4(r27)
	addi	r27,	r27,	4

	bne-	.L_syscall_copy_word_loop

	/* restore the callee-saved registers we just used for copy */
	lwz	r27,	SS_R27(r30)
	lwz	r28,	SS_R28(r30)

	b	.L_syscall_got_args


.L_syscall_arg_copyin:

	/* we came from a user task, pay the price of a real copyin */	

	/* set recovery point */
	addis	r28,	0,	.L_syscall_copyin_recover@ha
	addi	r28,	r28,	.L_syscall_copyin_recover@l
	stw	r28,	THREAD_RECOVER(ARG0) /* ARG0 still holds thread ptr */

	/* We can manipulate the COPYIN segment register quite easily
	 * here, but we've also got to make sure we don't go over a
	 * segment boundary - hence some mess.
	 * Registers from 12-29 are free for our use.
	 */
	lwz	r28,	SS_R1(r30) /* get user's stack pointer */

	/* change r29 to hold number of args we need to copy */
	subi	r29,	r29,	8

	/* Set user stack pointer to point at copyin args */
	addi	r28,	r28,	COPYIN_ARG0_OFFSET

	/* set up SR_COPYIN to allow us to copy, we may need to loop
	 * around if we change segments. We know that this previously
	 * pointed to user space, so the sid doesn't need setting.
	 */
.L_syscall_copyin_seg_loop:
	lwz	r27,	SS_SR_COPYIN(r30)
	/* Mask off the lowest 4 bits holding segment number */
	rlwinm	r27,	r27,	0,	0,	27
	/* insert the user stack pointer's segment number */
	rlwimi	r27,	r28,	4,	28,	31
	mtsr	SR_COPYIN,	r27
	isync

	/* adjust the stack pointer to map into the SR_COPYIN segment */
	rlwinm	r26,	r28,	0,	4,	31
	oris	r26,	r26,	(SR_COPYIN << (28-16))
	
	/* Make r27 point to address-4 of where we will store copied args */
	addi	r27,	r1,	FM_ARG0-4
	
.L_syscall_copyin_word_loop:
	lwz	r0,	0(r26)	/* MAY CAUSE PAGE FAULT! */

	stw	r0,	4(r27)
	addi	r27,	r27,	4
	subi	r29,	r29,	1
	mr.	r29,	r29
	beq	.L_syscall_copyin_done
	
	/* Move r26 to point to next user address and check if
	 * we've changed segment or not. We update the user stack
	 * pointer too, in case there's a change of segment.
	 * TODO NMGS some quicker implementation?
	 */
	addi	r26,	r26,	4
	addi	r28,	r28,	4

	/* Mask off segment and bytes to see if we've changed segment */
	rlwinm.	r0,	r26,	0,	4,	29
	bne	.L_syscall_copyin_word_loop
	b	.L_syscall_copyin_seg_loop	/* On new segment! remap */

.L_syscall_copyin_done:	
	/* Don't bother restoring SR_COPYIN, we can leave it trashed */

	/* clear thread recovery as we're done touching user data */
	li	r0,	0
	stw	r0,	THREAD_RECOVER(ARG0) /* ARG0 still holds thread ptr */
	
	/* restore the callee-saved registers we just used for copyin */
	lwz	r26,	SS_R26(r30)
	lwz	r27,	SS_R27(r30)
	lwz	r28,	SS_R28(r30)

.L_syscall_got_args:
	/* Once we are here, we've got all the syscall's arguments in
	 * the right places, except ARG0 which is in the PCB state (not
	 * in sprg3 any more since we may have been preempted)
	 */
	lwz	r29,	SS_R29(r30) /* restore callee saved r29 */
	
	lwz	ARG0,	SS_R3(r30)  /* Restore ARG0 so all args in place */

	lwz	r0,	MACH_TRAP_FUNCTION(r31)

	/* calling this function, all the callee-saved registers are
	 * still valid except for r30 and r31 which are in the PCB
	 * r30 holds pointer to saved state (ie. pcb)
	 * r31 is scrap
	 */
	mtctr	r0
	bctrl			/* perform the actual syscall */

	/* 'standard' syscall returns here - INTERRUPTS ARE STILL ON */

	/* r3 contains value that we're going to return to the user
	 */

.L_syscall_return:		
	/*
	 * Ok, return from C function, ARG0 = return value
	 *
	 * get the active thread's PCB pointer and thus pointer to user state
	 */
	lwz	r31,	PP_CPU_DATA(r2)
	lwz	r31,	CPU_ACTIVE_THREAD(r31)

	/* Store return value into saved state structure, since
	 * we need to pick up the value from here later - the
	 * syscall may perform a thread_set_syscall_return
	 * followed by a thread_exception_return, ending up
	 * at thread_syscall_return below, with SS_R3 having
	 * been set up already
	 */
	
	/* When we are here, r31 should point to the current thread,
	 *                   r30 should point to the current pcb
	 */
	
	/* save off return value, we must load it
	 * back anyway for thread_exception_return
	 * TODO NMGS put in register?
	 */
	stw	r3,	SS_R3(r30)

.L_thread_syscall_ret_check_ast:	
	/* Disable interrupts */
	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	isync

	/* Check to see if there's an outstanding AST */
		
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_syscall_no_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */

#if	DEBUG
	/* debug assert - make sure that we're not returning to kernel */
	lwz	r3,	SS_SRR1(r30)
	andi.	r3,	r3,	MASK(MSR_PR)
	bne+	0f		/* returning to user level, check */
	lwz	r3,	SS_SRR0(r30)
	addis	r4,	0,	VM_MIN_KERNEL_LOADED_ADDRESS >> 16
	cmpl	CR0,	r3,	r4
	bgt+	CR0,	0f	/* returning to kloaded, check */

	BREAKPOINT_TRAP
0:		
#endif	/* DEBUG */
	
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
	
	/* check again for AST (rare) */
	b	.L_thread_syscall_ret_check_ast

.L_syscall_no_ast:

.L_thread_syscall_return:

	/* thread_exception_return returns to here, almost all
	 * registers intact. It expects a full context restore
	 * of what it hasn't restored itself (ie. what we use).
	 *
	 * In particular for us,
	 * we still have     r31 points to the current thread,
	 *                   r30 points to the current pcb
	 */

	mr	r3,	r30
	mr	r1,	r31
	/* r0-2 will be restored last, use as temp for now */


	/* Callee saved state was saved and restored by the functions
	 * that we have called, assuming that we performed a standard
	 * function calling sequence. We only restore those that we
	 * are currently using.
	 *
	 * thread_exception_return arrives here, however, and it
	 * expects the full state to be restored
	 */
#if DEBUG
	/* the following callee-saved state should already be restored */
	
	lwz	r30,	SS_R14(r3); twne	r30,	r14
	lwz	r30,	SS_R15(r3); twne	r30,	r15
	lwz	r30,	SS_R16(r3); twne	r30,	r16
	lwz	r30,	SS_R17(r3); twne	r30,	r17
	lwz	r30,	SS_R18(r3); twne	r30,	r18
	lwz	r30,	SS_R19(r3); twne	r30,	r19
	lwz	r30,	SS_R20(r3); twne	r30,	r20
	lwz	r30,	SS_R21(r3); twne	r30,	r21
	lwz	r30,	SS_R22(r3); twne	r30,	r22
	lwz	r30,	SS_R23(r3); twne	r30,	r23
	lwz	r30,	SS_R24(r3); twne	r30,	r24
	lwz	r30,	SS_R25(r3); twne	r30,	r25
	lwz	r30,	SS_R26(r3); twne	r30,	r26
	lwz	r30,	SS_R27(r3); twne	r30,	r27
	lwz	r30,	SS_R28(r3); twne	r30,	r28
	lwz	r30,	SS_R29(r3); twne	r30,	r29
#endif /* DEBUG */
		
	li	r0,	0		/* Ensure R0 is 0 on 601 for sync */

	lwz	r30,	SS_R30(r3)

	lwz	r31,	SS_LR(r3)
	mtlr	r31

	lwz	r31,	SS_CTR(r3)
	mtctr	r31

	lwz	r31,	SS_XER(r3)
	mtxer	r31

	lwz	r31,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN,	r31
	isync

	lwz	r31,	SS_R31(r3)

	/* mark kernel stack as free before restoring r30, r31 */

	/* we no longer need r2 pointer to per_proc_info at this point */
	
	/* There may be a critical region here for traps(interrupts?)
	 * once the stack is marked as free, PCB_SR0 may be trampled on
	 * so interrupts must be off.
	 */
	/* Clear reservation at the same time */
	lwz	r0,	THREAD_KERNEL_STACK(r1)
		/* Can't use addi since addi converts r0 to 0 value */
	ori	r0,	r0,	KERNEL_STACK_SIZE-KS_SIZE-FM_SIZE
/* we have to use an indirect store to clear reservation */
	li	r2,	PCB_KSP
	sync
	stwcx.	r0,	r2,	r3		/* clear reservation */
	stw	r0,	PCB_KSP(r3)		/* mark stack as free */

	/* We may be returning to something in the kernel space.
	 * If we are, we can skip the trampoline and just rfi,
	 * since we don't want to restore the user's space regs
	 */
	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	bne+	.L_syscall_returns_to_user

	/* TODO NMGS - is this code in common with interrupts and traps?*/
	/* the syscall is returning to something in
	 * priviliged mode, can just rfi without modifying
	 * space registers
	 */

	lwz	r0,	SS_CR(r3)
	mtcr	r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
	/* critical region for traps(interrupt?) since r1 no longer points
	 * to bottom of stack. Could be fixed. But interrupts are off (?).
	 */

#if 0
	lwz	r2,	SS_R2(r3)
#else
	/* set r2 from per_proc info, not from SS */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r2,	0
#endif	
	/* r3 restored last */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */
	.fill	32			/* 8 words of zeros after rfi for 601*/

.L_syscall_returns_to_user:	

	/* If jumping into user space, we should restore the user's
	 * segment register 0. We jump via a trampoline in physical mode
	 * TODO NMGS this trampoline code probably isn't needed
	 */

#if DEBUG
	/* Assert that PCB_SR0 isn't in kernel space */
	lwz	r2,	PCB_SR0(r3)
	rlwinm.	r2,	r2,	0,	8,	27
	bne+	00f
	BREAKPOINT_TRAP
00:
#endif /* DEBUG */
	lwz	r0,	SS_CR(r3)
	mtcr	r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)
	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r1,	0,	exception_exit@ha
	addi	r1,	r1,	exception_exit@l
	lwz	r1,	0(r1)
	mtsrr0	r1
	li	r1,	MSR_VM_OFF
	mtsrr1	r1

	lwz	r1,	PCB_SR0(r3)	/* For trampoline */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */

	rfi
	.fill	32			/* 8 words of zeros after rfi for 601*/

.L_syscall_copyin_recover:

	/* This is the catcher for any data faults in the copyin
	 * of arguments from the user's stack.
	 * r30 still holds a pointer to the PCB
	 *
	 * call syscall_error(EXC_BAD_ACCESS, EXC_PPC_VM_PROT_READ, sp, ssp),
	 *
	 * we already had a frame so we can do this
	 */	
	
	li	ARG0,	EXC_BAD_ACCESS
	li	ARG1,	EXC_PPC_VM_PROT_READ
	lwz	ARG2,	SS_R1(r30)
	mr	ARG3,	r30

	/* restore the callee-saved registers we just used for copyin */
	lwz	r26,	SS_R26(r30)
	lwz	r27,	SS_R27(r30)
	lwz	r28,	SS_R28(r30)
	lwz	r29,	SS_R29(r30)

	bl	syscall_error

        /*
	 * just handle this like a normal return from a system call...
	 */
	b	.L_syscall_return


/*
 * thread_exception_return()
 *
 * Return to user mode directly from within a system call.
 */

ENTRY(thread_exception_return, TAG_NO_FRAME_USED)

.L_thread_exc_ret_check_ast:	

	/* Disable interrupts */
	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	/* Check to see if there's an outstanding AST */
	/* We don't bother establishing a call frame even though CHECK_AST
	   can invoke ast_taken(), because it can just borrow our caller's
	   frame, given that we're not going to return.  */
		

	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	CR0,	.L_exc_ret_no_ast

	/* Yes there is, call ast_taken 
	 * pretending that the user thread took an AST exception here,
	 * ast_taken will save all state and bring us back here
	 */
	
#if	DEBUG
	/* debug assert - make sure that we're not returning to kernel
	 *
	 * get the active thread's PCB pointer and thus pointer to user state
	 */
	lwz	r30,	PP_CPU_DATA(r2)
	lwz	r30,	CPU_ACTIVE_THREAD(r30)
	lwz	r30,	THREAD_TOP_ACT(r30)
	lwz	r30,	ACT_MACT_PCB(r30)

	lwz	r3,	SS_SRR1(r30)
	andi.	r3,	r3,	MASK(MSR_PR)
	bne+	0f		/* returning to user level, check */
	lwz	r3,	SS_SRR0(r30)
	addis	r4,	0,	VM_MIN_KERNEL_LOADED_ADDRESS >> 16
	cmpl	CR0,	r3,	r4
	bgt+	CR0,	0f	/* returning to kloaded, check */

	BREAKPOINT_TRAP
0:		
#endif	/* DEBUG */

	li	ARG0,	0
	li	ARG1,	AST_ALL

	/* Perform splhigh and put old spl value in ARG2.
	 * Interrupts have already been disabled
	 */

	/* Disable interrupts */
	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
	
	/* check for a second AST (rare)*/
	b	.L_thread_exc_ret_check_ast
	
.L_exc_ret_no_ast:
	/* arriving here, interrupts should be disabled */

	/* Get the active thread's PCB pointer to restore regs
	 */
	
	lwz	r31,	PP_CPU_DATA(r2)
	lwz	r31,	CPU_ACTIVE_THREAD(r31)
	lwz	r30,	THREAD_TOP_ACT(r31)
	lwz	r30,	ACT_MACT_PCB(r30)

	/* If the MSR_SYSCALL_MASK isn't set, then we came from a trap,
	 * so warp into the return_from_trap (thread_return) routine,
	 * which takes PCB pointer in ARG0, not in r30!
	 */
	lwz	r0,	SS_SRR1(r30)
	mr	ARG0,	r30	/* Copy pcb pointer into ARG0 in case */

	/* test top half of msr */
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	MSR_SYSCALL_MASK >> 16
	bne-	CR0,	thread_return

	/* Otherwise, go to thread_syscall return, which requires
	 * r31 holding current thread, r30 holding current pcb
	 */

	/*
	 * restore saved state here
	 * except for r0-2, r3, r29, r30 and r31 used
	 * by thread_syscall_return,
	 */
	lwz	r4,	SS_R4(r30)
	lwz	r5,	SS_R5(r30)
	lwz	r6,	SS_R6(r30)
	lwz	r7,	SS_R7(r30)
	lwz	r8,	SS_R8(r30)
	lwz	r9,	SS_R9(r30)
	lwz	r10,	SS_R10(r30)
	lwz	r11,	SS_R11(r30)
	lwz	r12,	SS_R12(r30)
	lwz	r13,	SS_R13(r30)
	lwz	r14,	SS_R14(r30)
	lwz	r15,	SS_R15(r30)
	lwz	r16,	SS_R16(r30)
	lwz	r17,	SS_R17(r30)
	lwz	r18,	SS_R18(r30)
	lwz	r19,	SS_R19(r30)
	lwz	r20,	SS_R20(r30)
	lwz	r21,	SS_R21(r30)
	lwz	r22,	SS_R22(r30)
	lwz	r23,	SS_R23(r30)
	lwz	r24,	SS_R24(r30)
	lwz	r25,	SS_R25(r30)
	lwz	r26,	SS_R26(r30)
	lwz	r27,	SS_R27(r30)
	lwz	r28,	SS_R28(r30)
	lwz	r29,	SS_R29(r30)

	b	.L_thread_syscall_return

/*
 * thread_bootstrap_return()
 *
 */

ENTRY(thread_bootstrap_return, TAG_NO_FRAME_USED)

	/* Check for any outstanding ASTs and deal with them */

	lwz	r31,	PP_CPU_DATA(r2)
	lwz	r31,	CPU_ACTIVE_THREAD(r31)
	lwz	r31,	THREAD_TOP_ACT(r31)

.L_bootstrap_ret_check_ast:
	lwz	r3,	ACT_AST(r31)

	cmpi	CR0,	r3,	0

	beq	CR0,	.L_bootstrap_ret_no_ast

	/* we have a pending AST, now is the time to jump, args as below
	 * void ast_taken(boolean_t preemption, ast_t mask, spl_t old_spl)
	 */
	
	li	ARG0,	0
	li	ARG1,	AST_ALL
	
	/* Perform splhigh giving previous spl level in ARG2 */
	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	lwz	ARG3,	PP_CPU_DATA(r2)
	lwz	ARG2,	CPU_INTERRUPT_LEVEL(ARG3)
	li	r0,	SPLHIGH
	stw	r0,	CPU_INTERRUPT_LEVEL(ARG3)
	
	bl	ast_taken
		/* Check for another AST (rare) */
	b	.L_bootstrap_ret_check_ast
	
.L_bootstrap_ret_no_ast:
	/* Back from dealing with ASTs, if there were any.
	 * r31 still holds pointer to ACT.
	 * 
	 * Time to deal with kloading or kloaded threads
	 */
	lwz	r3,	ACT_KLOADING(r31)
	cmpi	CR0,	r3,	0
	beq+	.L_bootstrap_ret_no_kload
	li	r0,	0
	stw	r0,	ACT_KLOADING(r31)
	li	r0,	1
	stw	r0,	ACT_KLOADED(r31)
	lwz	r3,	PP_ACTIVE_KLOADED(r2)
	stw	r31,	0(r3)

.L_bootstrap_ret_no_kload:
	/* Ok, we're all set, jump to thread_return as if we
	 * were just coming back from a trap (ie. r3 set up to point to pcb)
	 */	

	lwz	ARG0,	ACT_MACT_PCB(r31)

	b	thread_return
	
/*
 * create_fake_interrupt()
 *
 * ENTRY:	callable from a kernel thread when an interrupt should
 *              have occurred but didn't because it was masked by spl,
 *              this behaves like a software T_INTERRUPT and calls
 *              ihandler(below) as if a hardware interrupt had arrived.
 *
 *              PROCESSOR INTERRUPTS MUST ALREADY BE SWITCHED OFF
 *		r2 points to our per-processor structure
 *
 * EXIT:        does not return - the ihandler will return for us!
 */

ENTRY(create_fake_interrupt, TAG_NO_FRAME_USED)

	/* Set up the conditions as required by ihandler below */

	mtsprg	1,	r1
	mtsprg	2,	r2
	mtsprg	3,	r3		/* In fact, this can be trashed */

	li	r3,	T_INTERRUPT
	stw	r3,	PP_SAVE_EXCEPTION_TYPE(r2)

	mfcr	r0
	stw	r0,	PP_SAVE_CR(r2)	/* could also be trashed */

	mfctr	r0
	stw	r0,	PP_SAVE_CTR(r2)	/* could also be trashed */

	mflr	r0			/* use our return address for srr0 */
	stw	r0,	PP_SAVE_SRR0(r2)
	mfmsr	r0			/* use our msr for srr1 */
	stw	r0,	PP_SAVE_SRR1(r2)

	b	ihandler

	/* Does not return */

	
/*
 * ihandler(type)
 *
 * ENTRY:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc structure
 *              original cr            saved in per_proc structure
 *              exception type (r3)    saved in per_proc structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of T_...) also in per_proc
 *
 * gdbhandler is a close derivative, bugfixes to one may apply to the other!
 */

/* Build a saved state structure on the interrupt stack and call
 * the routine interrupt()
 */

ENTRY(ihandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

	/*
	 * get the value of istackptr, if it's zero then we're already on the
	 * interrupt stack, otherwise it points to a saved_state structure
	 * at the top of the interrupt stack.
	 */

	lwz	r1,	PP_ISTACKPTR(r2)
	cmpwi	CR0,	r1,	0
	bne	CR0,	.L_istackfree

	/* We're already on the interrupt stack, get back the old
	 * stack pointer and make room for a frame
	 */

	mfsprg	r1,	1	/* recover old stack pointer */

	/* Move below the redzone where the interrupted thread may have
	 * been saving its state and make room for our saved state structure
	 */
	subi	r1,	r1,	FM_REDZONE+SS_SIZE

	
	
.L_istackfree:

	/* Once we reach here, r1 contains the adjusted stack pointer
         * where we can store a ppc_saved_state structure.
	 */

	stw	r0,	SS_R0(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)

#if 0
		/* Callee saved state - don't bother saving */
	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)
#endif /* 0 */
	
	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r0,	PP_SAVE_CTR(r2)
	stw	r0,	SS_CTR(r1)

	lwz	r5,	PP_SAVE_SRR0(r2)	/* r5 holds srr0 used below */
	stw	r5,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	mfxer	r0
	stw	r0,	SS_XER(r1)

	mflr	r0
	stw	r0,	SS_LR(r1)

	/* Save MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	mfspr	r0,	mq
	stw	r0,	SS_MQ(r1)
	li	r0,	0	/* Ensure r0 is zero on 601 for sync */
	isync
0:

	/* Free the reservation whilst saving SR_COPYIN */

	sync
	mfsr	r4,	SR_COPYIN
	li	r0,	SS_SR_COPYIN
	stwcx.	r4,	r1,	r0
	stw	r4,	SS_SR_COPYIN(r1)
	
	/* Mark that we're occupying the interrupt stack for sure now */

	li	r0,	0
	stw	r0,	PP_ISTACKPTR(r2)

	/*
	 * To summarise, when we reach here, we have filled out
	 * a ppc_saved_state structure on the interrupt stack, and
	 * the stack is marked as busy. We now generate a small
	 * stack frame with backpointers to follow the calling
	 * conventions. We set up the backpointers to the trapped
	 * routine allowing us to backtrace.
	 */
	
	mr	r4,	r1		/* Preserve saved_state in ARG1 */
	subi	r1,	r1,	FM_SIZE
	mfsprg	r0,	1
	stw	r0,	FM_BACKPTR(r1)	/* point back to previous stackptr */

#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 */

	stw	r5,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */

	stwu	r1,	-FM_SIZE(r1)	/* Mak new frame for C routine */
#endif /* DEBUG */

	/* r3 still holds the reason for the interrupt */
	/* and r4 holds a pointer to the saved state */
	mfdsisr	ARG2
	mfdar	ARG3
	
	bl	EXT(interrupt)

	.globl EXT(ihandler_ret)
LEXT(ihandler_ret)						/* Marks our return point from debugger entry */

	/* interrupt() returns a pointer to the saved state in r3
	 *
	 * Ok, back from C. Disable interrupts while we restore things
	 */

	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	/* Reload the saved state */

	/* r0-2 will be restored last, use as temp for now */

	/* We don't restore r3-5, these are restored differently too.
	 * see trampoline code TODO NMGS evaluate need for this
	 */

	lwz	r6,	SS_R6(r3)
	lwz	r7,	SS_R7(r3)
	lwz	r8,	SS_R8(r3)
	lwz	r9,	SS_R9(r3)
	lwz	r10,	SS_R10(r3)
	lwz	r11,	SS_R11(r3)
	lwz	r12,	SS_R12(r3)
	lwz	r13,	SS_R13(r3)
#if 0
		/* Callee saved state - already restored */
	lwz	r14,	SS_R14(r3)
	lwz	r15,	SS_R15(r3)
	lwz	r16,	SS_R16(r3)
	lwz	r17,	SS_R17(r3)
	lwz	r18,	SS_R18(r3)
	lwz	r19,	SS_R19(r3)
	lwz	r20,	SS_R20(r3)
	lwz	r21,	SS_R21(r3)
	lwz	r22,	SS_R22(r3)
	lwz	r23,	SS_R23(r3)
	lwz	r24,	SS_R24(r3)
	lwz	r25,	SS_R25(r3)
	lwz	r26,	SS_R26(r3)
	lwz	r27,	SS_R27(r3)
	lwz	r28,	SS_R28(r3)
	lwz	r29,	SS_R29(r3)
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)
#endif /* 0 */
	
	lwz	r0,	SS_XER(r3)
	mtxer	r0
	lwz	r0,	SS_LR(r3)
	mtlr	r0
	lwz	r0,	SS_CTR(r3)
	mtctr	r0
	lwz	r0,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN,	r0
	isync

	/* Restore MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	lwz	r0,	SS_MQ(r3)
	mtspr	mq,	r0
	li	r0,	0	/* Ensure R0 is zero on 601 for sync */
	isync
0:

	/* Is this the first interrupt on the stack? */

	lwz		r4,PP_INTSTACK_TOP_SS(r2)
	li		r5,PP_INTSTACK_TOP_SS		/* Point to an area we know */
	cmp		CR1,r4,r3		/* Are we at the top of the stack? */
	
	sync
	stwcx.	r4,r5,r2		/* Blast any possible reservation - do this here to catch both paths out */

	bne		CR1,.L_notthelast_interrupt			/* Get going if not the top o' stack... */

	/* We're the last frame on the stack. Indicate that
	 * we've freed up the stack by putting our save state ptr in
	 * istackptr.
	 *
	 * Check for ASTs if one of the below is true:	
	 *    returning to user mode
	 *    returning to a kloaded server
	 */
	 
	lwz		r4,SS_SRR1(r3)						/* Get the MSR we will load on return */
	stw		r3,PP_ISTACKPTR(r2)					/* Save that saved state ptr */
	andi.	r4,	r4,	MASK(MSR_PR)				/* Are we going to problem state? (Sorry, ancient IBM term for non-privileged) */
	lis		r5,VM_MIN_KERNEL_LOADED_ADDRESS>>16	/* Get lowest kernel loaded server address */
	bne+	.L_check_int_ast					/* Returning to user level, check ASTs... */

	lwz		r4,SS_SRR0(r3)						/* Get instruction address */
	cmpl	CR0,r4,r5							/* Are we in the kernel proper? */
	blt+	CR0,.L_no_int_ast					/* In kernel space, no AST check... */

.L_check_int_ast:
	
	lwz	r4,	PP_NEED_AST(r2)
	lwz	r4,	0(r4)
	cmpi	CR0,	r4,	0
	beq	.L_no_int_ast

	/*
	 * There is a pending AST. Massage things to make it look like
	 * we took a trap and jump into the trap handler.  To do this
	 * we essentially pretend to return from the interrupt but
	 * at the last minute jump into the trap handler with an AST
	 * trap instead of performing an rfi.
	 */

	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_CR(r3)	/* store state in per_proc struct */
	stw	r0,	PP_SAVE_CR(r2)
	lwz	r0,	SS_CTR(r3)
	stw	r0,	PP_SAVE_CTR(r2)
	lwz	r0,	SS_SRR0(r3)
	stw	r0,	PP_SAVE_SRR0(r2)
	lwz	r0,	SS_SRR1(r3)
	stw	r0,	PP_SAVE_SRR1(r2)
	li	r0,	T_AST
	stw	r0,	PP_SAVE_EXCEPTION_TYPE(r2)
	
	lwz	r0,	SS_R0(r3)
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)

	/* r2 remains a constant - virt addr of per_proc_info */
#if	DEBUG
	/* assert r2 is kosha */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r3,	0
	twne	r2,	r3
#endif	/* DEBUG */
	li	r3,	T_AST	/* TODO r3 isn't used by thandler -optimise? */
	b	thandler		/* hyperspace into AST trap */

.L_no_int_ast:	

	/* We're committed to performing the rfi now.
	 * If returning to the user space, we should restore the user's
	 * segment registers. We jump via a trampoline in physical mode
	 * TODO NMGS this trampoline code probably isn't needed
	 */
	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	beq-	.L_interrupt_returns_to_kspace

	/* TODO NMGS would it be better to store SR0 in saved_state
	 * rather than perform this expensive lookup?
	 */
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_TOP_ACT(r1)
	lwz	r1,	ACT_MACT_PCB(r1)
	lwz	r1,	PCB_SR0(r1)	/* For trampoline */

#if DEBUG
	/* Assert that PCB_SR0 isn't in kernel space */
	rlwinm.	r0,	r1,	0,	8,	27
	bne+	00f
	BREAKPOINT_TRAP
00:
#endif /* DEBUG */


	lwz	r0,	SS_CR(r3)
	mtcr	r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)

	/* Prepare to rfi to the exception exit routine */
	addis	r2,	0,	exception_exit@ha
	addi	r2,	r2,	exception_exit@l
	lwz	r2,	0(r2)
	mtsrr0	r2
	li	r2,	MSR_VM_OFF
	mtsrr1	r2

	/* r1 already loaded above */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */
	
	rfi
	.fill	32			/* 8 words of zeros after rfi for 601*/

.L_interrupt_returns_to_kspace:	
.L_notthelast_interrupt:
	/* If we're not the last interrupt on the interrupt stack
	 * life is easier, we don't need to switch back into the
	 * user's segment. we can simply restore the last registers and rfi
	 */

	lwz	r0,	SS_CR(r3)
	mtcr	r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
#if 0
		/* Don't restore r2, we stay in the kernel space */
	lwz	r2,	SS_R2(r3)	/* r2 is a constant (&per_proc_info) */
#endif
#if	DEBUG
	/* assert r2 is kosha */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r4,	0
	twne	r2,	r4
#endif	/* DEBUG */
	/* r3 restored last */
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	/* and lastly... */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */
	.fill	32			/* 8 words of zeros after rfi for 601*/
	
/*
 * gdbhandler(type)
 *
 * ENTRY:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc structure
 *              original cr            saved in per_proc structure
 *              exception type (r3)    saved in per_proc structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of T_...) also in per_proc
 *
 *  Closely based on ihandler - bugfixes to one may apply to the other!
 */

/* build a saved state structure on the debugger stack and call
 * the routine enterDebugger()
 */

ENTRY(gdbhandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

#if !MACH_KGDB
	b	thandler
#else /* !MACH_KGDB */
	/*
	 * get the value of gdbstackptr, if it's zero then we're already on the
	 * debugger stack, otherwise it points to a saved_state structure
	 * at the top of the debugger stack.
	 */

	lwz	r1,	PP_GDBSTACKPTR(r2)
	cmpwi	CR0,	r1,	0
	bne	CR0,	.L_gdbstackfree

	/* We're already on the debugger stack, get back the old
	 * stack pointer and make room for a frame
	 */

	mfsprg	r1,	1	/* recover old stack pointer */

	/* Move below the redzone where the interrupted thread may have
	 * been saving its state and make room for our saved state structure
	 */
	subi	r1,	r1,	FM_REDZONE+SS_SIZE

	/* TODO NMGS - how about checking for stack overflow, huh?! */

.L_gdbstackfree:

	/* Once we reach here, r1 contains the adjusted stack pointer
         * where we can store a ppc_saved_state structure.
	 */

	stw	r0,	SS_R0(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)
	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)

	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r0,	PP_SAVE_CTR(r2)
	stw	r0,	SS_CTR(r1)

	lwz	r5,	PP_SAVE_SRR0(r2)	/* r5 holds srr0 used below */
	stw	r5,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	mfxer	r0
	stw	r0,	SS_XER(r1)

	mflr	r0
	stw	r0,	SS_LR(r1)

	/* Save MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	mfspr	r0,	mq
	stw	r0,	SS_MQ(r1)
	li	r0,	0	/* Ensure R0 is zero on 601 for sync */
	isync
0:

	/* Free the reservation whilst saving SR_COPYIN */

	sync
	mfsr	r4,	SR_COPYIN
	li	r0,	SS_SR_COPYIN
	stwcx.	r4,	r1,	r0
	stw	r4,	SS_SR_COPYIN(r1)
	
	/* Mark that we're occupying the gdb stack for sure now */

	li	r0,	0
	stw	r0,	PP_GDBSTACKPTR(r2)

	/*
	 * To summarise, when we reach here, we have filled out
	 * a ppc_saved_state structure on the gdb stack, and
	 * the stack is marked as busy. We now generate a small
	 * stack frame with backpointers to follow the calling
	 * conventions. We set up the backpointers to the trapped
	 * routine allowing us to backtrace.
	 *
	 * This probably isn't needed in gdbhandler, but I've left
	 * it in place
	 */
	
	mr	r4,	r1		/* Preserve saved_state in ARG1 */
	subi	r1,	r1,	FM_SIZE
	mfsprg	r0,	1
	stw	r0,	FM_BACKPTR(r1)	/* point back to previous stackptr */

#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 * TODO NMGS debugging call frame not correct yet
	 */
	stw	r5,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */
	stwu	r1,	-FM_SIZE(r1)	/* point back to previous frame */
#endif /* DEBUG */

	/* r3 still holds the reason for the trap */
	/* and r4 holds a pointer to the saved state */

	mfdsisr	ARG2

	bl	EXT(enterDebugger)

	/* enterDebugger() returns a pointer to the saved state in r3
	 *
	 * Ok, back from C. Disable interrupts while we restore things
	 */

	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	/* Reload the saved state */

	/* r0-2 will be restored last, use as temp for now */

	/* We don't restore r3-5, these are restored differently too.
	 * see trampoline code TODO NMGS evaluate need for this
	 */

	lwz	r6,	SS_R6(r3)
	lwz	r7,	SS_R7(r3)
	lwz	r8,	SS_R8(r3)
	lwz	r9,	SS_R9(r3)
	lwz	r10,	SS_R10(r3)
	lwz	r11,	SS_R11(r3)
	lwz	r12,	SS_R12(r3)
	lwz	r13,	SS_R13(r3)
	lwz	r14,	SS_R14(r3)
	lwz	r15,	SS_R15(r3)
	lwz	r16,	SS_R16(r3)
	lwz	r17,	SS_R17(r3)
	lwz	r18,	SS_R18(r3)
	lwz	r19,	SS_R19(r3)
	lwz	r20,	SS_R20(r3)
	lwz	r21,	SS_R21(r3)
	lwz	r22,	SS_R22(r3)
	lwz	r23,	SS_R23(r3)
	lwz	r24,	SS_R24(r3)
	lwz	r25,	SS_R25(r3)
	lwz	r26,	SS_R26(r3)
	lwz	r27,	SS_R27(r3)
	lwz	r28,	SS_R28(r3)
	lwz	r29,	SS_R29(r3)
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)

	lwz	r0,	SS_XER(r3)
	mtxer	r0
	lwz	r0,	SS_LR(r3)
	mtlr	r0
	lwz	r0,	SS_CTR(r3)
	mtctr	r0
	lwz	r0,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN,	r0
	isync

	/* Restore MQ register on 601 processors */
	mfpvr	r0
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	PROCESSOR_VERSION_601
	bne+	0f
	lwz	r0,	SS_MQ(r3)
	mtspr	mq,	r0
	li	r0,	0	/* Ensure R0 is zero on 601 for sync */
	isync
0:

	/* Is this the first frame on the stack? */

	lwz	r4,	PP_GDBSTACK_TOP_SS(r2)
	cmp	CR0,	r4,	r3
	bne	CR0,	.L_notthelast_gdbframe

	/* We're the last frame on the stack. Indicate that
	 * we've freed up the stack by putting our save state ptr in
	 * istackptr. Clear reservation at same time.
	 */
	mr	r4,	r2
	addi	r4,	r4,	PP_GDBSTACKPTR
/* we have to use an indirect store to clear reservation */
	sync
	stwcx.	r3,	0,	r4		/* clear reservation */
	stw	r3,	0(r4)

	/* We may be returning to something in the kernel space.
	 * If we are, we can skip the trampoline and just rfi,
	 * since we don't want to restore the user's space regs
	 */

	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	beq-	.L_gdb_ret_to_kspace

	/* If jumping into user space, we should restore the user's
	 * segment register 0. We jump via a trampoline in physical mode
	 * TODO NMGS this trampoline code probably isn't needed
	 */

	/* TODO NMGS would it be better to store SR0 in saved_state
	 * rather than perform this expensive lookup?
	 */
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_TOP_ACT(r1)
	lwz	r1,	ACT_MACT_PCB(r1)
	lwz	r1,	PCB_SR0(r1)	/* For trampoline */

#if DEBUG
	/* Assert that PCB_SR0 isn't in kernel space */
	rlwinm.	r0,	r1,	0,	8,	27
	bne+	00f
	BREAKPOINT_TRAP
00:
#endif /* DEBUG */

	lwz	r0,	SS_CR(r3)
	mtcr	r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments
	 */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)

	/* Prepare to rfi to the exception exit routine */
	addis	r2,	0,	exception_exit@ha
	addi	r2,	r2,	exception_exit@l
	lwz	r2,	0(r2)
	mtsrr0	r2
	li	r2,	MSR_VM_OFF
	mtsrr1	r2

	/* r1 already loaded above */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */
	

	rfi
	.fill	32			/* 8 words of zeros after rfi for 601*/
	
.L_gdb_ret_to_kspace:	
.L_notthelast_gdbframe:
	/* If we're not the last frame on the stack
	 * life is easier, we don't need to switch back into the
	 * user's segment. we can simply restore the last registers and rfi
	 */

	lwz	r0,	SS_CR(r3)
	mtcr	r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
#if 0
		/* Don't restore r2, we stay in the kernel space */
	lwz	r2,	SS_R2(r3)	/* r2 is a constant (&per_proc_info) */
#endif
#if	DEBUG
	/* assert r2 is kosha */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r4,	0
	twne	r2,	r4
#endif	/* DEBUG */
	/* r3 restored last */
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	/* and lastly... */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */
	.fill	32			/* 8 words of zeros after rfi for 601*/

#endif /* !MACH_KGDB */	

#if	MACH_KDB
/*
 *			Here's where we jump into the debugger.  This is called from
 *			either an MP signal from another processor, or a command-power NMI
 *			on the main processor.
 *
 *			Note that somewhere in our stack should be a return into the interrupt
 *			handler.  If there isn't, we'll crash off the end of the stack, actually,
 *			it'll just quietly return. hahahahaha.
 */

ENTRY(kdb_kintr, TAG_NO_FRAME_USED)
	
			lis		r9,EXT(ihandler_ret)@h	/* Top part of interrupt return */
			lis		r10,EXT(intercept_ret)@h	/* Top part of intercept return */
			ori		r9,r9,EXT(ihandler_ret)@l	/* Bottom part of interrupt return */
			ori		r10,r10,EXT(intercept_ret)@l	/* Bottom part of intercept return */
			
			lwz		r8,0(r1)				/* Get our caller's stack frame */

srchrets:	mr.		r8,r8					/* Have we reached the end of our rope? */
			beqlr-							/* Yeah, just bail... */
			lwz		r7,4(r8)				/* The whoever called them */
			cmplw	cr0,r9,r7				/* Was it the interrupt handler? */
			beq		srchfnd					/* Yeah... */
			lwz		r8,0(r8)				/* Chain back to the previous frame */
			b		srchrets				/* Ok, check again... */
			
srchfnd:	stw		r10,4(r8)				/* Modify return to come to us instead */
			blr								/* Finish up and get back here... */
			
/*
 *			We come here when we've returned all the way to the interrupt handler.
 *			That way we can enter the debugger with the registers and stack which
 *			existed at the point of interruption.
 *
 *			R3 points to the saved state at entry
 */
 
 ENTRY(intercept_ret, TAG_NO_FRAME_USED)

			lis		r6,kdb_trap@h			/* Get the top part of the KDB enter routine */
			mr		r5,r3					/* Move saved state to the correct parameter */
			ori		r6,r6,kdb_trap@l		/* Get the last part of the KDB enter routine */
			li		r4,0					/* Set a code of 0 */
			mr		r13,r3					/* Save the saved state pointer in a non-volatile */
			mtlr	r6						/* Set the routine address */
			li		r3,-1					/* Show we had an interrupt type */
	
			blrl							/* Go enter KDB */

			mr		r3,r13					/* Put the saved state where expected */
			b		ihandler_ret			/* Go return from the interruption... */

#endif			
