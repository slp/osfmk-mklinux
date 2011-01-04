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
 * 
 */
/*
 * MkLinux
 */
/*
 * Context switch for PowerPC
 */

#include <ppc/asm.h>

/*
 * Cthread context switching.
 *
 * If cth is a cthread, cth->context locates a two-part context
 * structure for as long as that thread is executing in the cthreads
 * library.
 *
 * The two words above cth->context represent the continuation function
 * to be called when switching back to this cthread, and its argument.
 * They are stored by cthread_prepare or cthread_internal_build.  The
 * latter also switches to another (or possibly the same) thread and
 * invokes its continuation.
 *
 * After these two words there is, optionally, a simplified
 * setjmp buffer.  This is established by cthread_user_build before a
 * thread does a blocking cthreads operation such as mutex_lock_solid
 * or cthread_yield.  Subsequently, cthread_user_invoke will longjmp
 * back to this context in order to return to the user.
 *
 * cth->context is initially cthread_stack_base(cth, CTHREAD_STACK_OFFSET).
 * Once the cthread is in use, cth->context will be a context established by
 * cthread_user_build.  It is only in the latter case that we create or
 * use the setjmp part of the context.
 */

/* must keep stack alignment, so multiple of 16 */
#define CTHREAD_STATE_SIZE 96
	
/*
 * cthread_user_build(contextp, func, arg0, arg1)
 * Essentially equivalent to:
 *	if (!setjmp(contextp))
 *		(*func)(arg0, arg1);
 */
ENTRY(cthread_user_build, TAG_NO_FRAME_USED)


	/* Save link register as backpointer */
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)

	/* context will go below current stack pointer */
	subi	r0,	r1,	CTHREAD_STATE_SIZE

	/* Store pointer to context */
	stw	r0,	0(ARG0)

	/* Now recycle ARG0 as address of context, not pointer to context */
	mr	ARG0,	r0

	/* Now we can move the stack pointer below the saved context */

	stwu	r1,	-(CTHREAD_STATE_SIZE+FM_SIZE)(r1)

	/* we can save the context now that the stack pointer is below us */

		/* Save integer only callee-saved context */
	stw	r13,    8(ARG0)  /* GPR context. We avoid multiple-word */
	stw	r14,   12(ARG0)  /* instructions as they're slower on 604 */
	stw	r15,   16(ARG0)	
	stw	r16,   20(ARG0)	
	stw	r17,   24(ARG0)	
	stw	r18,   28(ARG0)	
	stw	r19,   32(ARG0)	
	stw	r20,   36(ARG0)	
	stw	r21,   40(ARG0)	
	stw	r22,   44(ARG0)	
	stw	r23,   48(ARG0)	
	stw	r24,   52(ARG0)	
	stw	r25,   56(ARG0)	
	stw	r26,   60(ARG0)	
	stw	r27,   64(ARG0)	
	stw	r28,   68(ARG0)	
	stw	r29,   72(ARG0)	
	stw	r30,   76(ARG0)	
	stw	r31,   80(ARG0)	

	mfcr	r0
	stw	r0,    84(ARG0)

	/* and now call the func() with its args,
	 * it shouldn't ever return normally
	 */

	mtctr	ARG1
	mr	ARG0,	ARG2
	mr	ARG1,	ARG3
	bctrl
	/* Trap to debugger if the function ever returns! */
	twge	2,2

/*
 * cthread_user_invoke(context, result)
 * Essentially equivalent to:
 *	longjmp(context, result);
 */
ENTRY(cthread_user_invoke, TAG_NO_FRAME_USED)
	
		/* restore integer only callee-saved context */
	lwz	r13,    8(ARG0)  /* GPR context. We avoid multiple-word */
	lwz	r14,   12(ARG0)  /* instructions as they're slower (?) */
	lwz	r15,   16(ARG0)	
	lwz	r16,   20(ARG0)	
	lwz	r17,   24(ARG0)	
	lwz	r18,   28(ARG0)	
	lwz	r19,   32(ARG0)	
	lwz	r20,   36(ARG0)	
	lwz	r21,   40(ARG0)	
	lwz	r22,   44(ARG0)	
	lwz	r23,   48(ARG0)	
	lwz	r24,   52(ARG0)	
	lwz	r25,   56(ARG0)	
	lwz	r26,   60(ARG0)	
	lwz	r27,   64(ARG0)	
	lwz	r28,   68(ARG0)	
	lwz	r29,   72(ARG0)	
	lwz	r30,   76(ARG0)	
	lwz	r31,   80(ARG0)	

	lwz	r0,    84(ARG0)
	mtcr	r0

	/* Recover previous stack pointer calculated from address of state */
	addi	r1,	ARG0,	CTHREAD_STATE_SIZE

	/* Recover saved LR from previous stack frame */
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0
	
	mr	ARG0,   ARG1     /* set the return value */
	blr

/*
 * cthread_prepare(context, func, arg)
 * Next time the cthread to which this context belongs is switched to,
 * func(arg) will be called.
 */

ENTRY(cthread_prepare, TAG_NO_FRAME_USED)

	stw	ARG1,	0(ARG0)
	stw	ARG2,	4(ARG0)
	blr

/*
 * cthread_internal_build(oldcontext, func, arg, lock, newcontext)
 * Save func and arg in oldcontext (the calling cthread's context) so
 * they will be called when we switch back.  Release the lock.  Switch
 * to newcontext and call its saved (*func)(arg).  This function will
 * not return, so we don't have to give a meaningful value to rp.
 */

ENTRY(cthread_internal_build, TAG_NO_FRAME_USED)

	/* Save func and arg in oldcontext) */
	stw	ARG1,	0(ARG0)
	stw	ARG2,	4(ARG0)
	
	/* Recover new context */
	lwz	ARG0,	4(ARG4)
	lwz	r0,	0(ARG4)
	mtctr	r0

	/* calculate stack pointer that goes below the new state */
	subi	r1,	ARG4,	FM_SIZE

	/* Release the lock */
	li	r0,	0
	stw	r0,	0(ARG3)

	/* And return to the function pointer we just restored */
	bctrl

	/* If we ever get back here, trap to debugger */
	twge	2,2

/*
 * cthread_invoke_self(context, func, arg)
 * Equivalent to cthread_internal_build(context, func, arg, null, context).
 * Rewinds the stack back to the context and calls the function.
 */

ENTRY(cthread_invoke_self, TAG_NO_FRAME_USED)

	/* Calculate stack pointer below address of state */
	subi	r1,	ARG0,	FM_SIZE
	mtctr	ARG1
	mr	ARG0,	ARG2
	bctrl
	/* Trap if the function returns here */
	twge	2,2
