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
 *  (c) Copyright 1986 HEWLETT-PACKARD COMPANY
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

#include <machine/asm.h>


	.SPACE $TEXT$
	.SUBSPA $MILLICODE$

$$divI_2
	comclr,>= r26,r0,r0
	addi 1,r26,r26
	bv r0(r31)
	extrs r26,30,31,ret1
$$divI_4
	comclr,>= r26,r0,r0
	addi 3,r26,r26
	bv r0(r31)
	extrs r26,29,30,ret1
$$divI_8
	comclr,>= r26,r0,r0
	addi 7,r26,r26
	bv r0(r31)
	extrs r26,28,29,ret1
$$divI_16
	comclr,>= r26,r0,r0
	addi 0xf,r26,r26
	bv r0(r31)
	extrs r26,27,28,ret1
	
	.export $$divI_3
$$divI_3
	comb,<,n r26,r0,$neg3
	addi 1,r26,r26
	extru r26,1,2,ret1
	sh2add r26,r26,r26
	b $pos
	addc ret1,r0,ret1
$neg3
	subi 1,r26,r26
	extru r26,1,2,ret1
	sh2add r26,r26,r26
	b $neg
	addc ret1,r0,ret1
	
	.export $$divU_3
$$divU_3
	addi 1,r26,r26
	addc r0,r0,ret1
	shd ret1,r26,30,r25
	sh2add r26,r26,r26
	b $pos
	addc ret1,r25,ret1
	
	.export $$divI_5
$$divI_5
	comb,<,n r26,r0,$neg5
	addi 3,r26,r25
	sh1add r26,r25,r26
	b $pos
	addc r0,r0,ret1
$neg5
	sub r0,r26,r26
	addi 1,r26,r26
	shd r0,r26,31,ret1
	sh1add r26,r26,r26
	b $neg
	addc ret1,r0,ret1
$$divU_5
	addi 1,r26,r26
	addc r0,r0,ret1
	shd ret1,r26,31,r25
	sh1add r26,r26,r26
	b $pos
	addc r25,ret1,ret1
$$divI_6
	comb,<,n r26,r0,$neg6
	extru r26,30,31,r26
		addi 5,r26,r25
	sh2add r26,r25,r26
	b $pos
	addc r0,r0,ret1
$neg6
	subi 2,r26,r26
	extru r26,30,31,r26
	shd r0,r26,30,ret1
	sh2add r26,r26,r26
	b $neg
	addc ret1,r0,ret1
$$divU_6
	extru r26,30,31,r26
	addi 1,r26,r26
	shd r0,r26,30,ret1
	sh2add r26,r26,r26
	b $pos
	addc ret1,r0,ret1

	.export $$divU_10	
$$divU_10
	extru r26,30,31,r26
	addi 3,r26,r25
	sh1add r26,r25,r26
	addc r0,r0,ret1
$pos
	shd ret1,r26,28,r25
	shd r26,r0,28,r1
	add r26,r1,r26
	addc ret1,r25,ret1
$pos_for_17
	shd ret1,r26,24,r25
	shd r26,r0,24,r1
	add r26,r1,r26
	addc ret1,r25,ret1
	shd ret1,r26,16,r25
	shd r26,r0,16,r1
	add r26,r1,r26
	bv r0(r31)
	addc ret1,r25,ret1
	
	.export $$divI_10
$$divI_10
	comb,< r26,r0,$neg10
	copy r0,ret1
	extru r26,30,31,r26
	addibf 1,r26,$pos
	sh1add r26,r26,r26
$neg10
	subi 2,r26,r26
	extru r26,30,31,r26
	sh1add r26,r26,r26
$neg
	shd ret1,r26,28,r25
	shd r26,r0,28,r1
	add r26,r1,r26
	addc ret1,r25,ret1
$neg_for_17
	shd ret1,r26,24,r25
	shd r26,r0,24,r1
	add r26,r1,r26
	addc ret1,r25,ret1
	shd ret1,r26,16,r25
	shd r26,r0,16,r1
	add r26,r1,r26
	addc ret1,r25,ret1
	bv r0(r31)
	sub r0,ret1,ret1
$$divI_12
	comb,< r26,r0,$neg12
	copy r0,ret1
	extru r26,29,30,r26
	addibf 1,r26,$pos
	sh2add r26,r26,r26
$neg12
	subi 4,r26,r26
	extru r26,29,30,r26
	b $neg
	sh2add r26,r26,r26
	
	.export $$divU_12
$$divU_12
	extru r26,29,30,r26
	addi 5,r26,r25
	sh2add r26,r25,r26
	b $pos
	addc r0,r0,ret1
$$divI_15
	comb,< r26,r0,$neg15
	copy r0,ret1
	addibf 1,r26,$pos+4
	shd ret1,r26,28,r25
$neg15
	b $neg
	subi 1,r26,r26
$$divU_15
	addi 1,r26,r26
	b $pos
	addc r0,r0,ret1
$$divI_17
	comb,<,n r26,r0,$neg17
	addi 1,r26,r26
	shd r0,r26,28,r25
	shd r26,r0,28,r1
	sub r1,r26,r26
	b $pos_for_17
	subb r25,r0,ret1
$neg17
	subi 1,r26,r26
	shd r0,r26,28,r25
	shd r26,r0,28,r1
	sub r1,r26,r26
	b $neg_for_17
	subb r25,r0,ret1
$$divU_17
	addi 1,r26,r26
	addc r0,r0,ret1
	shd ret1,r26,28,r25
	shd r26,r0,28,r1
	sub r1,r26,r26
	b $pos_for_17
	subb r25,ret1,ret1
$$divI_7
	comb,<,n r26,r0,$neg7
$7
	addi 1,r26,r26
	shd r0,r26,29,ret1
	sh3add r26,r26,r26
	addc ret1,r0,ret1
$pos7
	shd ret1,r26,26,r25
	shd r26,r0,26,r1
	add r26,r1,r26
	addc ret1,r25,ret1
	shd ret1,r26,20,r25
	shd r26,r0,20,r1
	add r26,r1,r26
	addc ret1,r25,r25
	copy r0,ret1
	shd,= r25,r26,24,r25
$1
	addbf r25,ret1,$2
	extru r26,31,24,r26
	bv,n r0(r31)
$2
	addbf r25,r26,$1
	extru,= r26,7,8,r25
$neg7
	subi 1,r26,r26
$8
	shd r0,r26,29,ret1
	sh3add r26,r26,r26
	addc ret1,r0,ret1
$neg7_shift
	shd ret1,r26,26,r25
	shd r26,r0,26,r1
	add r26,r1,r26
	addc ret1,r25,ret1
	shd ret1,r26,20,r25
	shd r26,r0,20,r1
	add r26,r1,r26
	addc ret1,r25,r25
	copy r0,ret1
	shd,= r25,r26,24,r25
$3
	addbf r25,ret1,$4
	extru r26,31,24,r26
	bv r0(r31)
	sub r0,ret1,ret1
$4
	addbf r25,r26,$3
	extru,= r26,7,8,r25
$$divU_7
	addi 1,r26,r26
	addc r0,r0,ret1
	shd ret1,r26,29,r25
	sh3add r26,r26,r26
	b $pos7
	addc r25,ret1,ret1
$$divI_9
	comb,<,n r26,r0,$neg9
	addi 1,r26,r26
	shd r0,r26,29,r25
	shd r26,r0,29,r1
	sub r1,r26,r26
	b $pos7
	subb r25,r0,ret1
$neg9
	subi 1,r26,r26
	shd r0,r26,29,r25
	shd r26,r0,29,r1
	sub r1,r26,r26
	b $neg7_shift
	subb r25,r0,ret1
$$divU_9
	addi 1,r26,r26
	addc r0,r0,ret1
	shd ret1,r26,29,r25
	shd r26,r0,29,r1
	sub r1,r26,r26
	b $pos7
	subb r25,ret1,ret1
$$divI_14
	comb,<,n r26,r0,$neg14
$$divU_14
	b $7
	extru r26,30,31,r26
$neg14
	subi 2,r26,r26
	b $8
	extru r26,30,31,r26
$$divoI
	comib,=,n -1,r25,negative1
	
	.export $$divI,millicode	
$$divI
	comibf,<<,n 0xf,r25,small_divisor
	add,>= r0,r26,ret1
normal
	sub r0,ret1,ret1
	sub r0,r25,r1
	ds r0,r1,r0
	add ret1,ret1,ret1
	ds r0,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	xor,>= r26,r25,r0
	sub r0,ret1,ret1
	bv,n r0(r31)
	nop
small_divisor
	blr,n r25,r0
	nop
		addit,= 0,r25,r0
	nop
	bv r0(r31)
	copy r26,ret1
	b,n $$divI_2
	nop
	b,n $$divI_3
	nop
	b,n $$divI_4
	nop
	b,n $$divI_5
	nop
	b,n $$divI_6
	nop
	b,n $$divI_7
	nop
	b,n $$divI_8
	nop
	b,n $$divI_9
	nop
	b,n $$divI_10
	nop
	b normal
	add,>= r0,r26,ret1
	b,n $$divI_12
	nop
	b normal
	add,>= r0,r26,ret1
	b,n $$divI_14
	nop
	b,n $$divI_15
	nop
negative1
	sub r0,r26,ret1
	bv r0(r31)
	addo r26,r25,r0
	
	.export $$divU,millicode	
$$divU
	comibf,< 0xf,r25,special_divisor
	sub r0,r25,r1
	ds r0,r1,r0
normalU
	add r26,r26,ret1
	ds r0,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	bv r0(r31)
	addc ret1,ret1,ret1
special_divisor
	comibf,<= 0,r25,big_divisor
	nop
	blr r25,r0
	nop
	addit,= 0,r25,r0
	nop
	bv r0(r31)
	copy r26,ret1
	bv r0(r31)
	extru r26,30,31,ret1
	b,n $$divU_3
	nop
	bv r0(r31)
	extru r26,29,30,ret1
	b,n $$divU_5
	nop
	b,n $$divU_6
	nop
	b,n $$divU_7
	nop
	bv r0(r31)
	extru r26,28,29,ret1
	b,n $$divU_9
	nop
	b,n $$divU_10
	nop
	b normalU
	ds r0,r1,r0
	b,n $$divU_12
	nop
	b normalU
	ds r0,r1,r0
	b,n $$divU_14
	nop
	b,n $$divU_15
	nop
big_divisor
	sub r26,r25,r0
	bv r0(r31)
	addc r0,r0,ret1
	
	.export $$remI,millicode	
$$remI
	addit,= 0,r25,r0
	add,>= r0,r26,ret1
	sub r0,ret1,ret1
	sub r0,r25,r1
	ds r0,r1,r0
	copy r0,r1
	add ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	ds r1,r25,r1
	addc ret1,ret1,ret1
	movb,>=,n r1,ret1,finish
	add,< r25,r0,r0
	add,tr r1,r25,ret1
	sub r1,r25,ret1
finish
	add,>= r26,r0,r0
	sub r0,ret1,ret1
	bv r0(r31)
	nop
	
	.export $$remU,millicode	
$$remU
	comibf,<,n 0,r25,special_case
	sub r0,r25,ret1
	ds r0,ret1,r0
	add r26,r26,r1
	ds r0,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	addc r1,r1,r1
	ds ret1,r25,ret1
	comiclr,<= 0,ret1,r0
	add ret1,r25,ret1
	bv,n r0(r31)
	nop
special_case
	addit,= 0,r25,r0
	sub,>>= r26,r25,ret1
	copy r26,ret1
	bv,n r0(r31)
	nop

	

	.subspa $MILLICODE$
	.align 16
$$mulI: 

	.proc
	.callinfo NO_CALLS
	.export $$mulI, millicode
	combt,<<=	%r25,%r26,l4	# swap args if unsigned %r25>%r26
	copy		0,%r29		# zero out the result
	xor		%r26,%r25,%r26	# swap %r26 & %r25 using the
	xor		%r26,%r25,%r25	#  old xor trick
	xor		%r26,%r25,%r26
l4: combt,<=	0,%r26,l3		# if %r26>=0 then proceed like unsigned

	zdep		%r25,30,8,%r1	# %r1 = (%r25&0xff)<<1 *********
	sub,>		0,%r25,%r1		# otherwise negate both and
	combt,<=,n	%r26,%r1,l2	#  swap back if |%r26|<|%r25|
	sub		0,%r26,%r25
	movb,tr,n	%r1,%r26,l2	# 10th inst.

l0: 	add	%r29,%r1,%r29				# add in this partial product

l1: zdep	%r26,23,24,%r26			# %r26 <<= 8 ******************

l2: zdep		%r25,30,8,%r1	# %r1 = (%r25&0xff)<<1 *********

l3: blr		%r1,0		# case on these 8 bits ******

	extru		%r25,23,24,%r25	# %r25 >>= 8 ******************

#			  %r26 <<= 8 **************************
x0: comb,<>	%r25,0,l2	; zdep	%r26,23,24,%r26	; bv,n  0(r31)	; nop

x1: comb,<>	%r25,0,l1	; 	add	%r29,%r26,%r29	; bv,n  0(r31)	; nop

x2: comb,<>	%r25,0,l1	; sh1add	%r26,%r29,%r29	; bv,n  0(r31)	; nop

x3: comb,<>	%r25,0,l0	; 	sh1add	%r26,%r26,%r1	; bv    0(r31)	; 	add	%r29,%r1,%r29

x4: comb,<>	%r25,0,l1	; sh2add	%r26,%r29,%r29	; bv,n  0(r31)	; nop

x5: comb,<>	%r25,0,l0	; 	sh2add	%r26,%r26,%r1	; bv    0(r31)	; 	add	%r29,%r1,%r29

x6: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh1add	%r1,%r29,%r29	; bv,n  0(r31)

x7: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh2add	%r26,%r29,%r29	; b,n	ret_t0

x8: comb,<>	%r25,0,l1	; sh3add	%r26,%r29,%r29	; bv,n  0(r31)	; nop

x9: comb,<>	%r25,0,l0	; 	sh3add	%r26,%r26,%r1	; bv    0(r31)	; 	add	%r29,%r1,%r29

x10: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh1add	%r1,%r29,%r29	; bv,n  0(r31)

x11: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh3add	%r26,%r29,%r29	; b,n	ret_t0

x12: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh2add	%r1,%r29,%r29	; bv,n  0(r31)

x13: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh3add	%r26,%r29,%r29	; b,n	ret_t0

x14: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x15: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; 	sh1add	%r1,%r1,%r1	; b,n	ret_t0

x16: zdep	%r26,27,28,%r1	; comb,<>	%r25,0,l1	; 	add	%r29,%r1,%r29	; bv,n  0(r31)

x17: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh3add	%r26,%r1,%r1	; b,n	ret_t0

x18: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh1add	%r1,%r29,%r29	; bv,n  0(r31)

x19: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh1add	%r1,%r26,%r1	; b,n	ret_t0

x20: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh2add	%r1,%r29,%r29	; bv,n  0(r31)

x21: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh2add	%r1,%r26,%r1	; b,n	ret_t0

x22: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x23: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x24: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh3add	%r1,%r29,%r29	; bv,n  0(r31)

x25: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; 	sh2add	%r1,%r1,%r1	; b,n	ret_t0

x26: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x27: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; 	sh3add	%r1,%r1,%r1	; b,n	ret_t0

x28: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x29: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x30: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x31: zdep	%r26,26,27,%r1	; comb,<>	%r25,0,l0	; sub	%r1,%r26,%r1	; b,n	ret_t0

x32: zdep	%r26,26,27,%r1	; comb,<>	%r25,0,l1	; 	add	%r29,%r1,%r29	; bv,n  0(r31)

x33: 	sh3add	%r26,0,%r1		; comb,<>	%r25,0,l0	; sh2add	%r1,%r26,%r1	; b,n	ret_t0

x34: zdep	%r26,27,28,%r1	; add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x35: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh3add	%r26,%r1,%r1

x36: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh2add	%r1,%r29,%r29	; bv,n  0(r31)

x37: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh2add	%r1,%r26,%r1	; b,n	ret_t0

x38: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x39: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x40: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh3add	%r1,%r29,%r29	; bv,n  0(r31)

x41: 	sh2add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; sh3add	%r1,%r26,%r1	; b,n	ret_t0

x42: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x43: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x44: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x45: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; 	sh2add	%r1,%r1,%r1	; b,n	ret_t0

x46: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; add	%r1,%r26,%r1

x47: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh1add	%r26,%r1,%r1

x48: 	sh1add	%r26,%r26,%r1		; comb,<>	%r25,0,l0	; zdep	%r1,27,28,%r1	; b,n	ret_t0

x49: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r26,%r1,%r1

x50: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x51: 	sh3add	%r26,%r26,%r1		; sh3add	%r26,%r1,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x52: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x53: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x54: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x55: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x56: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x57: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x58: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x59: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_t02a0	; 	sh1add	%r1,%r1,%r1

x60: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x61: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x62: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x63: zdep	%r26,25,26,%r1	; comb,<>	%r25,0,l0	; sub	%r1,%r26,%r1	; b,n	ret_t0

x64: zdep	%r26,25,26,%r1	; comb,<>	%r25,0,l1	; 	add	%r29,%r1,%r29	; bv,n  0(r31)

x65: 	sh3add	%r26,0,%r1		; comb,<>	%r25,0,l0	; sh3add	%r1,%r26,%r1	; b,n	ret_t0

x66: zdep	%r26,26,27,%r1	; add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x67: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x68: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x69: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x70: zdep	%r26,25,26,%r1	; sh2add	%r26,%r1,%r1	; 	b	e_t0	; sh1add	%r26,%r1,%r1

x71: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,0,%r1	; 	b	e_t0	; sub	%r1,%r26,%r1

x72: 	sh3add	%r26,%r26,%r1		; comb,<>	%r25,0,l1	; sh3add	%r1,%r29,%r29	; bv,n  0(r31)

x73: 	sh3add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; b	e_shift	; 	add	%r29,%r1,%r29

x74: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x75: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x76: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x77: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x78: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x79: zdep	%r26,27,28,%r1	; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sub	%r1,%r26,%r1

x80: zdep	%r26,27,28,%r1	; 	sh2add	%r1,%r1,%r1	; b	e_shift	; 	add	%r29,%r1,%r29

x81: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; b	e_shift	; 	add	%r29,%r1,%r29

x82: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x83: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x84: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x85: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x86: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x87: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; b	e_t02a0	; sh2add	%r26,%r1,%r1

x88: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x89: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x90: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x91: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x92: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_4t0	; sh1add	%r1,%r26,%r1

x93: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x94: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_2t0	; sh1add	%r26,%r1,%r1

x95: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x96: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x97: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x98: zdep	%r26,26,27,%r1	; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh1add	%r26,%r1,%r1

x99: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x100: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x101: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x102: zdep	%r26,26,27,%r1	; sh1add	%r26,%r1,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x103: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_t02a0	; sh2add	%r1,%r26,%r1

x104: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x105: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x106: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x107: 	sh3add	%r26,%r26,%r1		; sh2add	%r26,%r1,%r1	; b	e_t02a0	; sh3add	%r1,%r26,%r1

x108: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x109: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x110: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x111: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x112: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; zdep	%r1,27,28,%r1

x113: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_t02a0	; 	sh1add	%r1,%r1,%r1

x114: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; 	sh1add	%r1,%r1,%r1

x115: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_2t0a0	; 	sh1add	%r1,%r1,%r1

x116: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_4t0	; sh2add	%r1,%r26,%r1

x117: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh3add	%r1,%r1,%r1

x118: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_t0a0	; 	sh3add	%r1,%r1,%r1

x119: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_t02a0	; 	sh3add	%r1,%r1,%r1

x120: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x121: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x122: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x123: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x124: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x125: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x126: zdep	%r26,25,26,%r1	; sub	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x127: zdep	%r26,24,25,%r1	; comb,<>	%r25,0,l0	; sub	%r1,%r26,%r1	; b,n	ret_t0

x128: zdep	%r26,24,25,%r1	; comb,<>	%r25,0,l1	; 	add	%r29,%r1,%r29	; bv,n  0(r31)

x129: zdep	%r26,24,25,%r1	; comb,<>	%r25,0,l0	; add	%r1,%r26,%r1	; b,n	ret_t0

x130: zdep	%r26,25,26,%r1	; add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x131: 	sh3add	%r26,0,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x132: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x133: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x134: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x135: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x136: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x137: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x138: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x139: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; b	e_2t0a0	; sh2add	%r1,%r26,%r1

x140: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_4t0	; 	sh2add	%r1,%r1,%r1

x141: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; b	e_4t0a0	; sh1add	%r1,%r26,%r1

x142: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,0,%r1	; 	b	e_2t0	; sub	%r1,%r26,%r1

x143: zdep	%r26,27,28,%r1	; 	sh3add	%r1,%r1,%r1	; 	b	e_t0	; sub	%r1,%r26,%r1

x144: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,0,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x145: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,0,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x146: 	sh3add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x147: 	sh3add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x148: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x149: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x150: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x151: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_2t0a0	; sh1add	%r1,%r26,%r1

x152: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x153: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x154: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x155: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x156: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_4t0	; sh1add	%r1,%r26,%r1

x157: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; b	e_t02a0	; 	sh2add	%r1,%r1,%r1

x158: zdep	%r26,27,28,%r1	; 	sh2add	%r1,%r1,%r1	; 	b	e_2t0	; sub	%r1,%r26,%r1

x159: zdep	%r26,26,27,%r1	; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sub	%r1,%r26,%r1

x160: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,0,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x161: 	sh3add	%r26,0,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x162: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x163: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; 	b	e_t0	; sh1add	%r1,%r26,%r1

x164: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x165: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x166: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x167: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; b	e_2t0a0	; sh1add	%r1,%r26,%r1

x168: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x169: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x170: zdep	%r26,26,27,%r1	; sh1add	%r26,%r1,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x171: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_t0	; 	sh3add	%r1,%r1,%r1

x172: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_4t0	; sh1add	%r1,%r26,%r1

x173: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_t02a0	; 	sh3add	%r1,%r1,%r1

x174: zdep	%r26,26,27,%r1	; sh1add	%r26,%r1,%r1	; b	e_t04a0	; 	sh2add	%r1,%r1,%r1

x175: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_5t0	; sh1add	%r1,%r26,%r1

x176: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_8t0	; add	%r1,%r26,%r1

x177: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_8t0a0	; add	%r1,%r26,%r1

x178: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; sh3add	%r1,%r26,%r1

x179: 	sh2add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_2t0a0	; sh3add	%r1,%r26,%r1

x180: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x181: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x182: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_2t0	; sh1add	%r1,%r26,%r1

x183: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_2t0a0	; sh1add	%r1,%r26,%r1

x184: 	sh2add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; 	b	e_4t0	; add	%r1,%r26,%r1

x185: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x186: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; 	b	e_2t0	; 	sh1add	%r1,%r1,%r1

x187: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_t02a0	; 	sh2add	%r1,%r1,%r1

x188: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_4t0	; sh1add	%r26,%r1,%r1

x189: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_t0	; 	sh3add	%r1,%r1,%r1

x190: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_2t0	; 	sh2add	%r1,%r1,%r1

x191: zdep	%r26,25,26,%r1	; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sub	%r1,%r26,%r1

x192: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x193: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x194: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x195: 	sh3add	%r26,0,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x196: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_4t0	; sh1add	%r1,%r26,%r1

x197: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_4t0a0	; sh1add	%r1,%r26,%r1

x198: zdep	%r26,25,26,%r1	; sh1add	%r26,%r1,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x199: 	sh3add	%r26,0,%r1		; sh2add	%r1,%r26,%r1	; b	e_2t0a0	; 	sh1add	%r1,%r1,%r1

x200: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x201: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x202: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x203: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_2t0a0	; sh2add	%r1,%r26,%r1

x204: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_4t0	; 	sh1add	%r1,%r1,%r1

x205: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x206: zdep	%r26,25,26,%r1	; sh2add	%r26,%r1,%r1	; b	e_t02a0	; 	sh1add	%r1,%r1,%r1

x207: 	sh3add	%r26,0,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_3t0	; sh2add	%r1,%r26,%r1

x208: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_8t0	; add	%r1,%r26,%r1

x209: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_8t0a0	; add	%r1,%r26,%r1

x210: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; 	sh2add	%r1,%r1,%r1

x211: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_2t0a0	; 	sh2add	%r1,%r1,%r1

x212: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_4t0	; sh2add	%r1,%r26,%r1

x213: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_4t0a0	; sh2add	%r1,%r26,%r1

x214: 	sh3add	%r26,%r26,%r1		; sh2add	%r26,%r1,%r1	; b	e2t04a0	; sh3add	%r1,%r26,%r1

x215: 	sh2add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_5t0	; sh1add	%r1,%r26,%r1

x216: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x217: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x218: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_2t0	; sh2add	%r1,%r26,%r1

x219: 	sh3add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x220: 	sh1add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; 	b	e_4t0	; sh1add	%r1,%r26,%r1

x221: 	sh1add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; b	e_4t0a0	; sh1add	%r1,%r26,%r1

x222: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; 	sh1add	%r1,%r1,%r1

x223: 	sh3add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_2t0a0	; 	sh1add	%r1,%r1,%r1

x224: 	sh3add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_8t0	; add	%r1,%r26,%r1

x225: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_t0	; 	sh2add	%r1,%r1,%r1

x226: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_t02a0	; zdep	%r1,26,27,%r1

x227: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_t02a0	; 	sh2add	%r1,%r1,%r1

x228: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_4t0	; 	sh1add	%r1,%r1,%r1

x229: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_4t0a0	; 	sh1add	%r1,%r1,%r1

x230: 	sh3add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_5t0	; add	%r1,%r26,%r1

x231: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_3t0	; sh2add	%r1,%r26,%r1

x232: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; 	b	e_8t0	; sh2add	%r1,%r26,%r1

x233: 	sh1add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e_8t0a0	; sh2add	%r1,%r26,%r1

x234: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; 	b	e_2t0	; 	sh3add	%r1,%r1,%r1

x235: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e_2t0a0	; 	sh3add	%r1,%r1,%r1

x236: 	sh3add	%r26,%r26,%r1		; sh1add	%r1,%r26,%r1	; b	e4t08a0	; 	sh1add	%r1,%r1,%r1

x237: zdep	%r26,27,28,%r1	; 	sh2add	%r1,%r1,%r1	; 	b	e_3t0	; sub	%r1,%r26,%r1

x238: 	sh1add	%r26,%r26,%r1		; sh2add	%r1,%r26,%r1	; b	e2t04a0	; 	sh3add	%r1,%r1,%r1

x239: zdep	%r26,27,28,%r1	; 	sh2add	%r1,%r1,%r1	; b	e_t0ma0	; 	sh1add	%r1,%r1,%r1

x240: 	sh3add	%r26,%r26,%r1		; add	%r1,%r26,%r1	; 	b	e_8t0	; 	sh1add	%r1,%r1,%r1

x241: 	sh3add	%r26,%r26,%r1		; add	%r1,%r26,%r1	; b	e_8t0a0	; 	sh1add	%r1,%r1,%r1

x242: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_2t0	; sh3add	%r1,%r26,%r1

x243: 	sh3add	%r26,%r26,%r1		; 	sh3add	%r1,%r1,%r1	; 	b	e_t0	; 	sh1add	%r1,%r1,%r1

x244: 	sh2add	%r26,%r26,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_4t0	; sh2add	%r1,%r26,%r1

x245: 	sh3add	%r26,0,%r1		; 	sh1add	%r1,%r1,%r1	; 	b	e_5t0	; sh1add	%r1,%r26,%r1

x246: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; 	b	e_2t0	; 	sh1add	%r1,%r1,%r1

x247: 	sh2add	%r26,%r26,%r1		; sh3add	%r1,%r26,%r1	; b	e_2t0a0	; 	sh1add	%r1,%r1,%r1

x248: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; b	e_shift	; sh3add	%r1,%r29,%r29

x249: zdep	%r26,26,27,%r1	; sub	%r1,%r26,%r1	; 	b	e_t0	; sh3add	%r1,%r26,%r1

x250: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; 	b	e_2t0	; 	sh2add	%r1,%r1,%r1

x251: 	sh2add	%r26,%r26,%r1		; 	sh2add	%r1,%r1,%r1	; b	e_2t0a0	; 	sh2add	%r1,%r1,%r1

x252: zdep	%r26,25,26,%r1	; sub	%r1,%r26,%r1	; b	e_shift	; sh2add	%r1,%r29,%r29

x253: zdep	%r26,25,26,%r1	; sub	%r1,%r26,%r1	; 	b	e_t0	; sh2add	%r1,%r26,%r1

x254: zdep	%r26,24,25,%r1	; sub	%r1,%r26,%r1	; b	e_shift	; sh1add	%r1,%r29,%r29

x255: zdep	%r26,23,24,%r1	; comb,<>	%r25,0,l0	; sub	%r1,%r26,%r1	; b,n	ret_t0

ret_t0: bv    0(r31)

e_t0: 	add	%r29,%r1,%r29

e_shift: comb,<>	%r25,0,l2

	zdep	%r26,23,24,%r26	# %r26 <<= 8 ***********
	bv,n  0(r31)
e_t0ma0: comb,<>	%r25,0,l0

	sub	%r1,%r26,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e_t0a0: comb,<>	%r25,0,l0

	add	%r1,%r26,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e_t02a0: comb,<>	%r25,0,l0

	sh1add	%r26,%r1,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e_t04a0: comb,<>	%r25,0,l0

	sh2add	%r26,%r1,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e_2t0: comb,<>	%r25,0,l1

	sh1add	%r1,%r29,%r29
	bv,n  0(r31)
e_2t0a0: comb,<>	%r25,0,l0

	sh1add	%r1,%r26,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e2t04a0: sh1add	%r26,%r1,%r1

	comb,<>	%r25,0,l1
	sh1add	%r1,%r29,%r29
	bv,n  0(r31)
e_3t0: comb,<>	%r25,0,l0

		sh1add	%r1,%r1,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e_4t0: comb,<>	%r25,0,l1

	sh2add	%r1,%r29,%r29
	bv,n  0(r31)
e_4t0a0: comb,<>	%r25,0,l0

	sh2add	%r1,%r26,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e4t08a0: sh1add	%r26,%r1,%r1

	comb,<>	%r25,0,l1
	sh2add	%r1,%r29,%r29
	bv,n  0(r31)
e_5t0: comb,<>	%r25,0,l0

		sh2add	%r1,%r1,%r1
	bv    0(r31)
		add	%r29,%r1,%r29
e_8t0: comb,<>	%r25,0,l1

	sh3add	%r1,%r29,%r29
	bv,n  0(r31)
e_8t0a0: comb,<>	%r25,0,l0

	sh3add	%r1,%r26,%r1
	bv    0(r31)
		add	%r29,%r1,%r29

	.procend
	.end

	.export $$dyncall,millicode	
$$dyncall
	bb,>=,n r22,0x1e,noshlibs
	depi 0,31,2,r22
	ldw 4(sr0,r22),r19
	ldw 0(sr0,r22),r22
noshlibs
	ldsid (sr0,r22),r1
	mtsp r1,sr0
	be 0(sr0,r22)
	nop

	.end



