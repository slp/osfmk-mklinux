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
#include <ppc/proc_reg.h>	/* For CACHE_LINE_SIZE */

/*
 *	 void	bzero(char *addr, unsigned int length)
 *
 * bzero implementation for PowerPC
 *   - assumes cacheable memory (i.e. uses DCBZ)
 *   - assumes non-pic code
 *
 * returns start address in r3, as per memset (called by memset)
 */	
	
ENTRY(bzero, TAG_NO_FRAME_USED)

	cmpwi	CR0,	r4,	0 /* no bytes to zero? */
	mr	r7,	r3
	mr	r8,	r3	/* use r8 as counter to where we are */
	beqlr-
	cmpwi	CR0,	r4,	CACHE_LINE_SIZE /* clear less than a block? */
	li	r0,	0	 /* use r0 as source of zeros */
	blt	.L_bzeroEndWord

/* first, clear bytes up to the next word boundary */
	addis	r6,	0,	.L_bzeroBeginWord@ha
	addi	r6,	r6,	.L_bzeroBeginWord@l
		 /* extract byte offset as word offset */
	rlwinm. r5,	r8,	2,	28,	29
	addi	r8,	r8,	-1 /* adjust for update */
	beq	.L_bzeroBeginWord /* no bytes to zero */
	subfic	r5,	r5,	16 /* compute the number of instructions */
	sub	r6,	r6,	r5 /* back from word clear to execute */
	mtctr	r6
	bctr

	stbu	r0,	1(r8)
	stbu	r0,	1(r8)
	stbu	r0,	1(r8)

/* clear words up to the next block boundary */
.L_bzeroBeginWord:
	addis	r6,	0,	.L_bzeroBlock@ha
	addi	r6,	r6,	.L_bzeroBlock@l
	addi	r8,	r8,	1
	rlwinm. r5,	r8,	0,	27,	29 /* extract word offset */
	addi	r8,	r8,	-4		/* adjust for update */
	beq	.L_bzeroBlock			/* no words to zero */
		/* compute the number of instructions */
	subfic	r5,	r5,	CACHE_LINE_SIZE
	sub	r6,	r6,	r5 /* back from word clear to execute */
	mtctr	r6
	bctr

	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)

 /* clear cache blocks */
.L_bzeroBlock:
	addi	r8,	r8,	4 /* remove update adjust */
	sub	r5,	r8,	r7 /* bytes zeroed */
	sub	r4,	r4,	r5
	srwi.	r5,	r4,	CACHE_LINE_POW2 /* blocks to zero */
	beq	.L_bzeroEndWord
	mtctr	r5

.L_bzeroBlock1:
	dcbz	r0,	r8
	addi	r8,	r8,	CACHE_LINE_SIZE
	bdnz	.L_bzeroBlock1

 /* clear remaining words */
.L_bzeroEndWord:
	addis	r6,	0,	.L_bzeroEndByte@ha
	addi	r6,	r6,	.L_bzeroEndByte@l
	rlwinm. r5,	r4,	0,	27,	29 /* extract word offset */
	addi	r8,	r8,	-4		   /* adjust for update */
	beq	.L_bzeroEndByte			   /* no words to zero */
	sub	r6,	r6,	r5 /* back from word clear to execute */
	mtctr	r6
	bctr

	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)
	stwu	r0,	4(r8)

 /* clear remaining bytes */
.L_bzeroEndByte:
	addis	r6,	0,	.L_bzeroEnd@ha
	addi	r6,	r6,	.L_bzeroEnd@l
		/* extract byte offset as word offset */
	rlwinm. r5,	r4,	2,	28,	29
	addi	r8,	r8,	3 /* adjust for update */
	beqlr
	sub	r6,	r6,	r5 /* back from word clear to execute */
	mtctr	r6
	bctr

	stbu	r0,	1(r8)
	stbu	r0,	1(r8)
	stbu	r0,	1(r8)

.L_bzeroEnd:
	blr

/*
 * void *memset(void *from, int c, vm_size_t nbytes)
 *
 * almost everywhere in the kernel (apart from ppc/POWERMAC/video_console.c)
 * this appears to be called with argument c==0. We optimise for those 
 * cases and call bzero if we can.
 *
 */

ENTRY(memset, TAG_NO_FRAME_USED)

	mr.	ARG3,	ARG1
	mr	ARG1,	ARG2
	/* optimised case - do a bzero */
	beq+	bzero

	/* If count is zero, return straight away */
	cmpi	CR0,	ARG1,	0
	beqlr-	
	
	/* Now, ARG0 = addr, ARG1=len, ARG3=value */

	subi	ARG2,	ARG0,	1	/* use ARG2 as our counter */
	
0:
	subi	ARG1,	ARG1,	1
	cmpi	CR0,	ARG1,	0
	stbu	ARG3,	1(ARG2)
	bne+	0b

	/* Return original address in ARG0 */
	
	blr
