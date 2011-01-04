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
 * Copyright (c) 1990,1991 The University of Utah and
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
 * 	Utah $Hdr: csw.s 1.5 92/07/08$
 *	Author: Bob Wheeler, University of Utah CSS
 */

/*
 * Context switch for hppa
 */

#include <machine/asm.h>

	.space	$TEXT$
	.subspa	$CODE$

/*
 * void* cthread_sp(void)
 *
 */
	.export	cthread_sp,entry
	.proc
	.callinfo
cthread_sp
	bv	0(rp)
	or	r0,sp,ret0
	.procend
	
	.export	IN_KERNEL,entry
	.proc
	.callinfo
IN_KERNEL
	copy     r0,ret0
	extru,<> rp,31,2,r0
	addi	 1,ret0,ret0

	bv	0(rp)
	nop

	.procend

/*
 * Cthread context switching.
 *
 * If cth is a cthread, cth->context locates a two-part context
 * structure for as long as that thread is executing in the cthreads
 * library.
 *
 * The two words before cth->context represent the continuation function
 * to be called when switching back to this cthread, and its argument.
 * They are stored by cthread_prepare or cthread_internal_build.  The
 * latter also switches to another (or possibly the same) thread and
 * invokes its continuation.
 *
 * Before these two words there is, optionally, a simplified
 * setjmp buffer.  This is established by cthread_user_build before a
 * thread does a blocking cthreads operation such as mutex_lock_solid
 * or cthread_yield.  Subsequently, cthread_user_invoke will longjmp
 * back to this context in order to return to the user.
 *
 * cth->context is initially cthread_stack_base(cth, CTHREAD_STACK_OFFSET).
 * It is because of this rather stack-grows-down oriented usage that we
 * put the continuation information before the context address.  Once the
 * cthread is in use, cth->context will be a context established by
 * cthread_user_build.  It is only in the latter case that we create or
 * use the setjmp part of the context.
 */

frame_size .equ 48	/* Procedures expect to have a frame this big
			   that they can violate. */
saved_integers .equ 18	/* r3--r18, sr3, cr11.  Must be even. */
saved_doubles .equ 10	/* fr12--fr21 */
continuation_size .equ 4*2 /* function, argument */
local_stack_space .equ 4*saved_integers + 8*saved_doubles + continuation_size
user_build_frame_size .equ frame_size + local_stack_space
	.if	saved_integers%1
	.error	alignment
	.endif

/*
 * cthread_user_build(contextp, func, arg0, arg1)
 * Essentially equivalent to:
 *	if (!setjmp(contextp))
 *		(*func)(arg0, arg1);
 */
	.export	cthread_user_build,entry
	.proc
	.callinfo frame=user_build_frame_size,calls,save_rp,save_sp,entry_gr=18
cthread_user_build
	stw	rp,-20(sp)

	/*
	 * set up registers for debugger backtrace
	 */
	copy	r3,r1
	copy	sp,r3


	/*
	 * Reserve space for the saved registers, continuation function
	 * and argument.
	 */
	stwm	r1,local_stack_space(sp)

	stw	r4,4(r3)
	stw	r5,8(r3)
	stw	r6,12(r3)
	stw	r7,16(r3)
	stw	r8,20(r3)
	stw	r9,24(r3)
	stw	r10,28(r3)
	stw	r11,32(r3)
	stw	r12,36(r3)
	stw	r13,40(r3)
	stw	r14,44(r3)
	stw	r15,48(r3)
	stw	r16,52(r3)
	stw	r17,56(r3)
	stw	r18,60(r3)
	mfsp	sr3,t1
	stw	t1,64(r3)
	mfctl	cr11,t1
	stw	t1,68(r3)
	.if	4*saved_integers - 72
	.error	saved_integers
	.endif

	ldo	72(r3),t1
	fstds,ma fr12,8(t1)
	fstds,ma fr13,8(t1)
	fstds,ma fr14,8(t1)
	fstds,ma fr15,8(t1)
	fstds,ma fr16,8(t1)
	fstds,ma fr17,8(t1)
	fstds,ma fr18,8(t1)
	fstds,ma fr19,8(t1)
	fstds,ma fr20,8(t1)
	fstds,ma fr21,8(t1)

	/*
	 * save the context pointer
	 */
	ldo	continuation_size(t1),t1
	stw	t1,0(arg0)

	/*
	 * Finish building the backtrace context.
	 */
	ldo	frame_size(sp),sp

	/*
	 * Call the new function.  We go through an extra step in order
	 * to get a value into rp, which will enable kgdb to find the
	 * correct size of this stack frame.  The called function never
	 * returns normally (it always uses cthread_user_invoke), so the
	 * exact value of rp doesn't matter.
	 */
	bl	$cthread_call,rp
	copy	arg2,arg0
	nop	/* Ensure that rp is in cthread_user_build, not $cthread_call */

	.procend
	.proc
	.callinfo

$cthread_call
	bv      (arg1)
	copy	arg3,arg1

	.procend

/*
 * cthread_user_invoke(context, result)
 * Essentially equivalent to:
 *	longjmp(context, result);
 */
	.export	cthread_user_invoke,entry
	.proc
	.callinfo
cthread_user_invoke
	fldds,mb -8-continuation_size(arg0),fr21
	fldds,mb -8(arg0),fr20
	fldds,mb -8(arg0),fr19
	fldds,mb -8(arg0),fr18
	fldds,mb -8(arg0),fr17
	fldds,mb -8(arg0),fr16
	fldds,mb -8(arg0),fr15
	fldds,mb -8(arg0),fr14
	fldds,mb -8(arg0),fr13
	fldds,mb -8(arg0),fr12
	ldo	-72(arg0),arg0
	ldw	68(arg0),t1
	mtctl	t1,cr11
	ldw	64(arg0),t1
	mtsp	t1,sr3
	ldw	60(arg0),r18
	ldw	56(arg0),r17
	ldw	52(arg0),r16
	ldw	48(arg0),r15
	ldw	44(arg0),r14
	ldw	40(arg0),r13
	ldw	36(arg0),r12
	ldw	32(arg0),r11
	ldw	28(arg0),r10
	ldw	24(arg0),r9
	ldw	20(arg0),r8
	ldw	16(arg0),r7
	ldw	12(arg0),r6
	ldw	8(arg0),r5
	ldw	4(arg0),r4
	ldw	0(arg0),r3
	copy	arg0,sp
	ldw	-20(sp),rp

	bv	0(rp)
	copy	arg1,ret0

	.procend

/*
 * cthread_prepare(context, func, arg)
 * Next time the cthread to which this context belongs is switched to,
 * func(arg) will be called.
 */
	.export	cthread_prepare,entry
	.proc
	.callinfo
cthread_prepare
	stw	arg1,-8(arg0)	/* routine */
	bv	0(rp)
	stw	arg2,-4(arg0)	/* argument */

	.procend

/*
 * cthread_internal_build(oldcontext, func, arg, lock, newcontext)
 * Save func and arg in oldcontext (the calling cthread's context) so
 * they will be called when we switch back.  Release the lock.  Switch
 * to newcontext and call its saved (*func)(arg).  This function will
 * not return, so we don't have to give a meaningful value to rp.
 */
	.export	cthread_internal_build,entry
	.proc
	.callinfo
cthread_internal_build
	stw	arg1,-8(arg0)	/* routine */
	stw	arg2,-4(arg0)	/* argument */

	ldw	-52(sp),t1	/* get new context */
	ldw	-8(t1),rp	/* get routine */
	ldw	-4(t1),arg0	/* get argument */
	ldo	frame_size(t1),sp

	ldo	15(arg3),arg3   /* align the lock to a 16 byte boundary */
	depi	0,31,4,arg3

	ldi	-1,t1		/* clear the lock */
	bv	0(rp)
	stw	t1,0(arg3)

	.procend

/*
 * cthread_invoke_self(context, func, arg)
 * Equivalent to cthread_internal_build(context, func, arg, null, context).
 * Rewinds the stack back to the context and calls the function.
 */
	.export cthread_invoke_self,entry
	.proc
	.callinfo
cthread_invoke_self
	copy	arg0,sp
	ldo	frame_size(sp),sp
	bv	0(arg1)
	copy	arg2,arg0

	.procend

	.end
