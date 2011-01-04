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


