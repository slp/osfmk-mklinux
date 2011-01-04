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

/* Routines to perform high-speed scrolling, assuming that the memory is
 * non-cached, and that the amount of memory to be scrolled is a multiple
 * of (at least) 16.
 */

#include <ppc/asm.h>
#include <ppc/proc_reg.h>

/*
 * void video_scroll_up(unsigned long start,
 *		        unsigned long end,
 *		        unsigned long dest)
 */

ENTRY(video_scroll_up, TAG_NO_FRAME_USED)

	/* Save off the link register, we want to call fpu_save. We assume
	 * that fpu_save leaves ARG0-5 intact and that it doesn't need
	 * a frame, otherwise we'd need one too.
	 */
	
	mflr	ARG3

	bl	fpu_save

	mtlr	ARG3

	/* ok, now we can use the FPU registers to do some fast copying,
	 * maybe this can be more optimal but it's not bad
	 */

.L_vscr_up_loop:
	lfd	f0,	0(ARG0)
	lfd	f1,	8(ARG0)

	addi	ARG0,	ARG0,	16
	
	stfd	f0,	0(ARG2)

	cmpl	CR0,	ARG0,	ARG1

	stfd	f1,	8(ARG2)

	addi	ARG2,	ARG2,	16

	blt+	CR0,	.L_vscr_up_loop

	/* re-disable the FPU */
	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	sync
	mtmsr	r0
	isync
	
	blr

/*
 * void video_scroll_down(unsigned long start,   HIGH address to scroll from
 *		          unsigned long end,     LOW address 
 *		          unsigned long dest)    HIGH address
 */

ENTRY(video_scroll_down, TAG_NO_FRAME_USED)

	/* Save off the link register, we want to call fpu_save. We assume
	 * that fpu_save leaves ARG0-5 intact and that it doesn't need
	 * a frame, otherwise we'd need one too.
	 */
	
	mflr	ARG3

	bl	fpu_save

	mtlr	ARG3

	/* ok, now we can use the FPU registers to do some fast copying,
	 * maybe this can be more optimal but it's not bad
	 */

.L_vscr_down_loop:
	lfd	f0,	-16(ARG0)
	lfd	f1,	-8(ARG0)

	subi	ARG0,	ARG0,	16
	
	stfd	f0,	-16(ARG2)

	cmpl	CR0,	ARG0,	ARG1

	stfd	f1,	-8(ARG2)

	subi	ARG2,	ARG2,	16

	bgt+	CR0,	.L_vscr_down_loop

	/* re-disable the FPU */
	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	sync
	mtmsr	r0
	isync
	
	blr

