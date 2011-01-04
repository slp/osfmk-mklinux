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
 * Copyright (c) 1989-1992 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
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
 *	Utah $Hdr: setjmp.s 1.2 92/05/22$
 */
/* This version modified by Patrick Dalton 04/02/92
 * from version in /usr/src/lib/libc/hppa/gen/setjmp.s
 *
 * All code that is dependent on signal has been taken out.
 * Code modified for internal kernel use only.
 */

#include <hp_pa/asm.h>

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers.
 */

/*
 * Layout of `jmp_buf' (40 32_bits_words).
 *
 *	0	saved general registers (gr3-18,27)
 *	17	saved return pointer
 *	18	saved stack pointer
 *	19	sr3
 *  *** 8 byte aligned ***
 *	20	fr12-fr21
 *	40
 */

        .SPACE $TEXT$
        .SUBSPA $CODE$
	.EXPORT mach_setjmp
mach_setjmp	.proc
	.callinfo

/*
 * Save the other general registers whose contents are expected to remain
 * across function calls.  According to the "HP9000 Series 800 Assembly
 * Language Reference Manual", procedures can use general registers 19-26,
 * 28, 29, 1, and 31 without restoring them.  Hence, we do not save these.
 *
 */	
	stwm	r3,4(arg0)
	stwm	r4,4(arg0)
	stwm	r5,4(arg0)
	stwm	r6,4(arg0)
	stwm	r7,4(arg0)
	stwm	r8,4(arg0)
	stwm	r9,4(arg0)
	stwm	r10,4(arg0)
	stwm	r11,4(arg0)
	stwm	r12,4(arg0)
	stwm	r13,4(arg0)
	stwm	r14,4(arg0)
	stwm	r15,4(arg0)
	stwm	r16,4(arg0)
	stwm	r17,4(arg0)
	stwm	r18,4(arg0)
	stwm	r27,4(arg0)	; Good idea to save the data pointer (dp)
	stwm    rp,4(arg0)	; Save the return pointer
	stwm	sp,4(arg0)	; Save the original stack pointer

	mfsp	sr3,t1
	stwm	t1,4(arg0)

	/* Float fr12, fr13, fr14, fr15 */

	/*
	 * If the address pointed to by arg0 is aligned then this is easy
	 */
	bb,>=,n	arg0,29,$setjmp_aligned

	/*
	 * If the address pointed to by arg0 is not aligned,
	 *	then store fp registers word by word
	 */
	fstws,ma %fr12R,4(arg0)
	fstws,ma %fr12,4(arg0)
	fstws,ma %fr13R,4(arg0)
	fstws,ma %fr13,4(arg0)
	fstws,ma %fr14R,4(arg0)
	fstws,ma %fr14,4(arg0)
	fstws,ma %fr15R,4(arg0)
	fstws,ma %fr15,4(arg0)
	fstws,ma %fr16R,4(arg0)
	fstws,ma %fr16,4(arg0)
	fstws,ma %fr17R,4(arg0)
	fstws,ma %fr17,4(arg0)
	fstws,ma %fr18R,4(arg0)
	fstws,ma %fr18,4(arg0)
	fstws,ma %fr19R,4(arg0)
	fstws,ma %fr19,4(arg0)
	fstws,ma %fr20R,4(arg0)
	fstws,ma %fr20,4(arg0)
	fstws,ma %fr21R,4(arg0)
	b	$setjmp_exit
	fstws	 %fr21,0(arg0)

$setjmp_aligned
	fstds,ma fr12,8(arg0)
	fstds,ma fr13,8(arg0)
	fstds,ma fr14,8(arg0)
	fstds,ma fr15,8(arg0)
	fstds,ma fr16,0(arg0)
	fstds,ma fr17,0(arg0)
	fstds,ma fr18,0(arg0)
	fstds,ma fr19,0(arg0)
	fstds,ma fr20,0(arg0)
	fstds	 fr21,0(arg0)

$setjmp_exit
	bv      0(rp)
	or      r0,r0,ret0

	.procend

	.export mach_longjmp
mach_longjmp
	.proc
	.callinfo

/*
 * Restore general registers.
 */
	ldwm	4(arg0),r3
	ldwm	4(arg0),r4
	ldwm	4(arg0),r5
	ldwm	4(arg0),r6
	ldwm	4(arg0),r7
	ldwm	4(arg0),r8
	ldwm	4(arg0),r9
	ldwm	4(arg0),r10
	ldwm	4(arg0),r11
	ldwm	4(arg0),r12
	ldwm	4(arg0),r13
	ldwm	4(arg0),r14
	ldwm	4(arg0),r15
	ldwm	4(arg0),r16
	ldwm	4(arg0),r17
	ldwm	4(arg0),r18
	ldwm	4(arg0),r27
	ldwm	4(arg0),rp	; Restore return address pointer,
	ldwm	4(arg0),sp	; stack pointer,

	ldwm	4(arg0),t1
	mtsp	t1,sr3

	/* Float fr12, fr13, fr14, fr15 */

	/*
	 * If the address pointed to by arg0 is aligned then this is easy
	 */
	bb,>=,n	arg0,29,$longjmp_aligned

	/*
	 * If the address pointed to by arg0 is not aligned,
	 *	then store fp registers word by word
	 */
	fldws,ma 4(arg0),%fr12R
	fldws,ma 4(arg0),%fr12
	fldws,ma 4(arg0),%fr13R
	fldws,ma 4(arg0),%fr13
	fldws,ma 4(arg0),%fr14R
	fldws,ma 4(arg0),%fr14
	fldws,ma 4(arg0),%fr15R
	fldws,ma 4(arg0),%fr15
	fldws,ma 4(arg0),%fr16R
	fldws,ma 4(arg0),%fr16
	fldws,ma 4(arg0),%fr17R
	fldws,ma 4(arg0),%fr17
	fldws,ma 4(arg0),%fr18R
	fldws,ma 4(arg0),%fr18
	fldws,ma 4(arg0),%fr19R
	fldws,ma 4(arg0),%fr19
	fldws,ma 4(arg0),%fr20R
	fldws,ma 4(arg0),%fr20
	fldws,ma 4(arg0),%fr21R
	b	$longjmp_exit
	fldws	 0(arg0),%fr21

$longjmp_aligned
	fldds,ma 8(arg0),fr12
	fldds,ma 8(arg0),fr13
	fldds,ma 8(arg0),fr14
	fldds,ma 8(arg0),fr15
	fldds,ma 8(arg0),fr16
	fldds,ma 8(arg0),fr17
	fldds,ma 8(arg0),fr18
	fldds,ma 8(arg0),fr19
	fldds,ma 8(arg0),fr20
	fldds	 0(arg0),fr21

$longjmp_exit

	bv	0(rp)		;   and return.
	or	arg1,r0,ret0	; Move return value to where it belongs.

	.procend

/*
 * int
 * get_dp()
 *
 * Return the contents of the dp register.
 */

	.export	get_dp,entry
	.proc
	.callinfo
get_dp
	bv	0(rp)
	copy	dp,ret0
	.procend

	.end

