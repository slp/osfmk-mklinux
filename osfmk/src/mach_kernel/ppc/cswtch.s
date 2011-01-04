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

#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <cpus.h>
#include <assym.s>
#include <debug.h>
#include <mach_kgdb.h>
#include <mach/ppc/vm_param.h>
	
	.text
	
/*
 * void     load_context(thread_t        thread)
 *
 * Load the context for the first kernel thread, and go.
 *
 * NOTE - if DEBUG is set, the former routine is a piece
 * of C capable of printing out debug info before calling the latter,
 * otherwise both entry points are identical.
 */

#if DEBUG && MACH_KGDB
ENTRY(Load_context, TAG_NO_FRAME_USED)
#else
ENTRY2(load_context, Load_context, TAG_NO_FRAME_USED)
#endif /* DEBUG && MACH_KGDB */	

#if	DEBUG
	/* assert r2 is kosha */
	/* TODO NMGS assumes 1-1 data mapping */
	mfsprg	r11,	0
	twne	r2,	r11
#endif	/* DEBUG */

	/*
	 * Since this is the first thread, we came in on the interrupt
	 * stack. The first thread never returns, so there is no need to
	 * worry about saving its frame, hence we can reset the istackptr
	 * back to the saved_state structure at it's top
	 */

	lwz	r0,	PP_INTSTACK_TOP_SS(r2)
	stw	r0,	PP_ISTACKPTR(r2)

        /*
         * get new thread pointer and set it into the active_threads pointer
         *
         * Never set any break points in these instructions since it
         * may cause a kernel stack overflow. (warning from HP code)
         */

	lwz	r11,	PP_CPU_DATA(r2)
	stw	ARG0,	CPU_ACTIVE_THREAD(r11)

	/* Find the new stack and store it in active_stacks */

	lwz	r1,	THREAD_KERNEL_STACK(ARG0)
	lwz	r12,	PP_ACTIVE_STACKS(r2)
	stw	r1,	0(r12)

        /*
         * This is the end of the no break points region.
         */

	/* Restore the callee save registers. The KS constants are
	 * from the bottom of the stack, pointing to a structure
	 * at the top of the stack
	 */
	lwz	r13,	KS_R13(r1)
	lwz	r14,	KS_R13+1*4(r1)
	lwz	r15,	KS_R13+2*4(r1)
	lwz	r16,	KS_R13+3*4(r1)
	lwz	r17,	KS_R13+4*4(r1)
	lwz	r18,	KS_R13+5*4(r1)
	lwz	r19,	KS_R13+6*4(r1)
	lwz	r20,	KS_R13+7*4(r1)
	lwz	r21,	KS_R13+8*4(r1)
	lwz	r22,	KS_R13+9*4(r1)
	lwz	r23,	KS_R13+10*4(r1)
	lwz	r24,	KS_R13+11*4(r1)
	lwz	r25,	KS_R13+12*4(r1)
	lwz	r26,	KS_R13+13*4(r1)
	lwz	r27,	KS_R13+14*4(r1)
	lwz	r28,	KS_R13+15*4(r1)
	lwz	r29,	KS_R13+16*4(r1)
	lwz	r30,	KS_R13+17*4(r1)
	lwz	r31,	KS_R13+18*4(r1)
	
        /*
         * Since this is the first thread, we make sure that thread_continue
         * gets a zero as its argument.
         */

	li	ARG0,	0

	lwz	r0,	KS_CR(r1)
	mtcr	r0
	lwz	r0,	KS_LR(r1)
	mtlr	r0
	lwz	r1,	KS_R1(r1)		/* Load new stack pointer */
	stw	ARG0,	FM_BACKPTR(r1)		/* zero backptr */
	blr			/* Jump to the continuation */
	
/* struct thread_shuttle *Switch_context(struct thread_shuttle   *old,
 * 				      	 void                    (*cont)(void),
 *				         struct thread_shuttle   *new)
 *
 * Switch from one thread to another. If a continuation is supplied, then
 * we do not need to save callee save registers.
 *
 */

ENTRY(Switch_context, TAG_NO_FRAME_USED)

	/*
	 * Get the old kernel stack, and store into the thread structure.
	 * See if a continuation is supplied, and skip state save if so.
	 * NB. Continuations are no longer used, so this test is omitted,
	 * as should the second argument, but it is in generic code.
	 * We always save state. This does not hurt even if continuations
	 * are put back in.
	 */

	lwz	r12,	PP_ACTIVE_STACKS(r2)
	lwz	r11,	0(r12)
#if 0
		cmpwi	CR0,	ARG1,	0		/* Continuation? */
#endif
	stw	ARG1,	THREAD_CONTINUATION(ARG0)
	stw	r11,	THREAD_KERNEL_STACK(ARG0)
#if 0
		bne	CR0,	.L_sw_ctx_no_save	/* Yes, skip save */
#endif
	/*
	 * Save all the callee save registers plus state.
	 */

	/* the KS_ constants point to the offset from the bottom of the
	 * stack to the ppc_kernel_state structure at the top, thus
	 * r11 can be used directly
	 */	
	stw	r1,	KS_R1(r11)
	stw	r13,	KS_R13(r11)
	stw	r14,	KS_R13+1*4(r11)
	stw	r15,	KS_R13+2*4(r11)
	stw	r16,	KS_R13+3*4(r11)
	stw	r17,	KS_R13+4*4(r11)
	stw	r18,	KS_R13+5*4(r11)
	stw	r19,	KS_R13+6*4(r11)
	stw	r20,	KS_R13+7*4(r11)
	stw	r21,	KS_R13+8*4(r11)
	stw	r22,	KS_R13+9*4(r11)
	stw	r23,	KS_R13+10*4(r11)
	stw	r24,	KS_R13+11*4(r11)
	stw	r25,	KS_R13+12*4(r11)
	stw	r26,	KS_R13+13*4(r11)
	stw	r27,	KS_R13+14*4(r11)
	stw	r28,	KS_R13+15*4(r11)
	stw	r29,	KS_R13+16*4(r11)
	stw	r30,	KS_R13+17*4(r11)
	stw	r31,	KS_R13+18*4(r11)
	
	mfcr	r0
	stw	r0,	KS_CR(r11)
	mflr	r0
	stw	r0,	KS_LR(r11)

	/*
	 * Make the new thread the current thread.
	 */
.L_sw_ctx_no_save:
	
	lwz	r11,	PP_CPU_DATA(r2)
	stw	ARG2,	CPU_ACTIVE_THREAD(r11)
	
	lwz	r11,	THREAD_KERNEL_STACK(ARG2)

	lwz	ARG2,	THREAD_TOP_ACT(ARG2)

	lwz	r10,	PP_ACTIVE_STACKS(r2)
	stw	r11,	0(r10)

	lwz	r10,	ACT_KLOADED(ARG2)
	cmpwi	CR0,	r10,	0
	lwz	r10,	PP_ACTIVE_KLOADED(r2)
	beq	CR0,	.L_sw_ctx_not_kld
	stw	ARG2,	0(r10)
	b	.L_sw_ctx_cont

.L_sw_ctx_not_kld:	
	li	r0,	0
	stw	r0,	0(r10)		/* act_kloaded = 0 */

.L_sw_ctx_cont:	

	/*
	 * Restore all the callee save registers.
	 */
	lwz	r1,	KS_R1(r11)	/* Restore stack pointer */

	/* Restore the callee save registers */
	lwz	r13,	KS_R13(r11)
	lwz	r14,	KS_R13+1*4(r11)
	lwz	r15,	KS_R13+2*4(r11)
	lwz	r16,	KS_R13+3*4(r11)
	lwz	r17,	KS_R13+4*4(r11)
	lwz	r18,	KS_R13+5*4(r11)
	lwz	r19,	KS_R13+6*4(r11)
	lwz	r20,	KS_R13+7*4(r11)
	lwz	r21,	KS_R13+8*4(r11)
	lwz	r22,	KS_R13+9*4(r11)
	lwz	r23,	KS_R13+10*4(r11)
	lwz	r24,	KS_R13+11*4(r11)
	lwz	r25,	KS_R13+12*4(r11)
	lwz	r26,	KS_R13+13*4(r11)
	lwz	r27,	KS_R13+14*4(r11)
	lwz	r28,	KS_R13+15*4(r11)
	lwz	r29,	KS_R13+16*4(r11)
	lwz	r30,	KS_R13+17*4(r11)
	lwz	r31,	KS_R13+18*4(r11)

	/*
	 * Disable the FPU for the returning thread,
	 * so the new thread traps when trying to use it.
	 */
	lwz	ARG2,	ACT_MACT_PCB(ARG2)
	lwz	r0,	SS_SRR1(ARG2)
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	stw	r0,	SS_SRR1(ARG2)
	
	lwz	r0,	KS_CR(r11)
	mtcr	r0
	lwz	r0,	KS_LR(r11)
	mtlr	r0
	
	/* ARG0 still holds old thread pointer, we return this */
	
	blr		/* Jump into the new thread */
			
/*
 * void fpu_save(void)
 *
 * Called when there's an exception or when get_state is called 
 * for the FPU state. Puts a copy of the current thread's state
 * into the PCB pointed to by fpu_pcb, and leaves the FPU enabled.
 *
 * video_scroll.s assumes that ARG0-5 won't be trampled on, and
 * that fpu_save doesn't need a frame, otherwise it would need
 * a frame itself. Why not oblige!
 */

ENTRY(fpu_save, TAG_NO_FRAME_USED)

	lwz	ARG6,	PP_FPU_PCB(r2)
	
	/*
	 * Turn the FPU back on (should this be a separate routine?)
	 */
	mfmsr	r0
	ori	r0,	r0,	MASK(MSR_FP)	/* bit is in low-order 16 */
	mtmsr	r0
	isync

	/*
	 * See if any thread owns the FPU. If not, we can skip the state save.
	 */
	cmpwi	CR0,	ARG6,	0
	beq-	CR0,	.L_fpu_save_no_owner

	/*
	 * Save the current FPU state into the PCB of the thread that owns it.
	 */
        stfd    f0,   PCB_FS_F0(ARG6)
        stfd    f1,   PCB_FS_F1(ARG6)
        stfd    f2,   PCB_FS_F2(ARG6)
        stfd    f3,   PCB_FS_F3(ARG6)
        stfd    f4,   PCB_FS_F4(ARG6)
        stfd    f5,   PCB_FS_F5(ARG6)
        stfd    f6,   PCB_FS_F6(ARG6)
        stfd    f7,   PCB_FS_F7(ARG6)
                mffs    f0			/* fpscr in f0 low 32 bits*/
        stfd    f8,   PCB_FS_F8(ARG6)
        stfd    f9,   PCB_FS_F9(ARG6)
        stfd    f10,  PCB_FS_F10(ARG6)
        stfd    f11,  PCB_FS_F11(ARG6)
        stfd    f12,  PCB_FS_F12(ARG6)
        stfd    f13,  PCB_FS_F13(ARG6)
        stfd    f14,  PCB_FS_F14(ARG6)
        stfd    f15,  PCB_FS_F15(ARG6)
                stfd    f0,  PCB_FS_FPSCR(ARG6)	/* Store junk 32 bits+fpscr */
        stfd    f16,  PCB_FS_F16(ARG6)
        stfd    f17,  PCB_FS_F17(ARG6)
        stfd    f18,  PCB_FS_F18(ARG6)
        stfd    f19,  PCB_FS_F19(ARG6)
        stfd    f20,  PCB_FS_F20(ARG6)
        stfd    f21,  PCB_FS_F21(ARG6)
        stfd    f22,  PCB_FS_F22(ARG6)
        stfd    f23,  PCB_FS_F23(ARG6)
        stfd    f24,  PCB_FS_F24(ARG6)
        stfd    f25,  PCB_FS_F25(ARG6)
        stfd    f26,  PCB_FS_F26(ARG6)
        stfd    f27,  PCB_FS_F27(ARG6)
        stfd    f28,  PCB_FS_F28(ARG6)
        stfd    f29,  PCB_FS_F29(ARG6)
        stfd    f30,  PCB_FS_F30(ARG6)
        stfd    f31,  PCB_FS_F31(ARG6)

	/* Mark the FPU as having no owner now */
	li	r0,	0
	stw	r0,	PP_FPU_PCB(r2)

	lwz	r0,	SS_SRR1(ARG6)
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	stw	r0,	SS_SRR1(ARG6)
	
.L_fpu_save_no_owner:	
	blr

/*
 * fpu_switch()
 *
 * Jumped to by the floating-point unavailable exception handler to
 * switch fpu context in a lazy manner.
 *
 * This code is run in virtual address mode with interrupts off.
 * It is assumed that the pcb in question is in memory
 *
 * Upon exit, the code returns to the users context
 *
 * ENTRY:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 *
 * r1 is used as a temporary register throught this code, not as a stack
 * pointer.
 * 
 */

	.text
ENTRY(fpu_switch, TAG_NO_FRAME_USED)

#if DEBUG
		/* Increment counter of number of floating point traps */
	addis	r3,	0,	fpu_trap_count@ha
	addi	r3,	r3,	fpu_trap_count@l
	lwz	r1,	0(r3)
	addi	r1,	r1,	1
	stw	r1,	0(r3)
#endif /* DEBUG */

	/*
	 * Turn the FPU back on
	 */
	mfmsr	r3
	ori	r3,	r3,	MASK(MSR_FP)	/* bit is in low-order 16 */
	mtmsr	r3
	isync

	/* See if the current thread owns the FPU, if so, just return */

	lwz	r3,	PP_FPU_PCB(r2)

	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_TOP_ACT(r1)
	lwz	r1,	ACT_MACT_PCB(r1)

	cmp	CR0,	r1,	r3	/* If we own FPU, just return */
	beq	CR0,	.L_fpu_switch_return
	cmpwi	CR1,	r3,	0	/* If no owner, skip save */
	beq	CR1,	.L_fpu_switch_load_state

/*
 * Identical code to that of fpu_save but is inlined
 * to avoid creating a stack frame etc.
 */
        stfd    f0,   PCB_FS_F0(r3)
        stfd    f1,   PCB_FS_F1(r3)
        stfd    f2,   PCB_FS_F2(r3)
        stfd    f3,   PCB_FS_F3(r3)
        stfd    f4,   PCB_FS_F4(r3)
        stfd    f5,   PCB_FS_F5(r3)
        stfd    f6,   PCB_FS_F6(r3)
        stfd    f7,   PCB_FS_F7(r3)
                mffs    f0			/* fpscr in f0 low 32 bits*/
        stfd    f8,   PCB_FS_F8(r3)
        stfd    f9,   PCB_FS_F9(r3)
        stfd    f10,  PCB_FS_F10(r3)
        stfd    f11,  PCB_FS_F11(r3)
        stfd    f12,  PCB_FS_F12(r3)
        stfd    f13,  PCB_FS_F13(r3)
        stfd    f14,  PCB_FS_F14(r3)
        stfd    f15,  PCB_FS_F15(r3)
                stfd    f0,  PCB_FS_FPSCR(r3)	/* Store junk 32 bits+fpscr */
        stfd    f16,  PCB_FS_F16(r3)
        stfd    f17,  PCB_FS_F17(r3)
        stfd    f18,  PCB_FS_F18(r3)
        stfd    f19,  PCB_FS_F19(r3)
        stfd    f20,  PCB_FS_F20(r3)
        stfd    f21,  PCB_FS_F21(r3)
        stfd    f22,  PCB_FS_F22(r3)
        stfd    f23,  PCB_FS_F23(r3)
        stfd    f24,  PCB_FS_F24(r3)
        stfd    f25,  PCB_FS_F25(r3)
        stfd    f26,  PCB_FS_F26(r3)
        stfd    f27,  PCB_FS_F27(r3)
        stfd    f28,  PCB_FS_F28(r3)
        stfd    f29,  PCB_FS_F29(r3)
        stfd    f30,  PCB_FS_F30(r3)
        stfd    f31,  PCB_FS_F31(r3)

	/* Now load in the current threads state */

.L_fpu_switch_load_state:	

	/* Store current PCB address in fpu_pcb to claim fpu for thread */
	stw	r1,	PP_FPU_PCB(r2)

	    lfd    f31,  PCB_FS_FPSCR(r1)	/* Load junk 32 bits+fpscr */
        lfd     f0,   PCB_FS_F0(r1)
        lfd     f1,   PCB_FS_F1(r1)
        lfd     f2,   PCB_FS_F2(r1)
        lfd     f3,   PCB_FS_F3(r1)
        lfd     f4,   PCB_FS_F4(r1)
        lfd     f5,   PCB_FS_F5(r1)
        lfd     f6,   PCB_FS_F6(r1)
        lfd     f7,   PCB_FS_F7(r1)
            mtfsf	0xff,	f31		/* fpscr in f0 low 32 bits*/
        lfd     f8,   PCB_FS_F8(r1)
        lfd     f9,   PCB_FS_F9(r1)
        lfd     f10,  PCB_FS_F10(r1)
        lfd     f11,  PCB_FS_F11(r1)
        lfd     f12,  PCB_FS_F12(r1)
        lfd     f13,  PCB_FS_F13(r1)
        lfd     f14,  PCB_FS_F14(r1)
        lfd     f15,  PCB_FS_F15(r1)
        lfd     f16,  PCB_FS_F16(r1)
        lfd     f17,  PCB_FS_F17(r1)
        lfd     f18,  PCB_FS_F18(r1)
        lfd     f19,  PCB_FS_F19(r1)
        lfd     f20,  PCB_FS_F20(r1)
        lfd     f21,  PCB_FS_F21(r1)
        lfd     f22,  PCB_FS_F22(r1)
        lfd     f23,  PCB_FS_F23(r1)
        lfd     f24,  PCB_FS_F24(r1)
        lfd     f25,  PCB_FS_F25(r1)
        lfd     f26,  PCB_FS_F26(r1)
        lfd     f27,  PCB_FS_F27(r1)
        lfd     f28,  PCB_FS_F28(r1)
        lfd     f29,  PCB_FS_F29(r1)
        lfd     f30,  PCB_FS_F30(r1)
        lfd     f31,  PCB_FS_F31(r1)

.L_fpu_switch_return:

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */

	addis	r3,	0,	exception_exit@ha
	addi	r3,	r3,	exception_exit@l
	lwz	r3,	0(r3)
	mtsrr0	r3
	li	r3,	MSR_VM_OFF
	mtsrr1	r3
	

	lwz	r1,	PCB_SR0(r1)		/* restore current sr0 */

	lwz	r3,	PP_SAVE_CR(r2)
	mtcr	r3
	lwz	r3,	PP_SAVE_CTR(r2)
	mtctr	r3
	lwz	r3,	PP_SAVE_SRR1(r2)
	lwz	r2,	PP_SAVE_SRR0(r2)
	/* Enable floating point for the thread we're returning to */
	ori	r3,	r3,	MASK(MSR_FP)	/* bit is in low-order 16 */
	
	/* Return to the trapped context */
	rfi
	.fill	32			/* 8 words of zeros after rfi for 601*/

/*
 * void fpu_disable(void)
 *
 * disable the fpu in the current msr
 *
 */

ENTRY(fpu_disable, TAG_NO_FRAME_USED)
	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	isync		/* make sure r0 contains value before sync */
	sync		/* flush any previous writes (maybe from fpu?) */
	mtmsr	r0
	isync
	blr
