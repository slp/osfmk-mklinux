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

/* If we are permitted to use floating point registers, this goes faster
 * for cache-aligned copies
 */

#define USE_FLOAT 0	

/* registers used: */

/* IMPORTANT - copyin/copyout assumes that bcopy won't trash r10 */

#define	tmp0		0
#define retValue	3
#define src		4
#define byteCount	5
#define wordCount	6
#define dst		7
#define bufA		8
#define bufB		9
#define bufC		0

#define	CACHE_FLAG	4

/*
 * bcopy(const char *from, chat *to, vm_size_t nbytes)
 *
 * bcopy_nc is simply bcopy except the dcbz & dcbst
 * instructions are not to be used. This is for non-cached
 * areas of memory.
 * 
 * Various device drivers heavily use this routine.
 */

ENTRY(bcopy_nc, TAG_NO_FRAME_USED)
	crclr	CACHE_FLAG
	b	.L_bcopy_common

/*	
 * void bcopy(const char *from, char *to, vm_size_t nbytes)
 *
 * bcopy uses memcpy, shouldn't this be done with a #define??
 */

ENTRY(bcopy, TAG_NO_FRAME_USED)
	crset	CACHE_FLAG

.L_bcopy_common:
	cmpwi	CR0, byteCount, 0
	beqlr-	CR0

	/* Convert to memcpy style arguments */
	mr	dst,	ARG1	/* Move dst from arg to 'dst' */
	mr	ARG1,	ARG0	/* put src in expected register too */
	b	.L_memcpy_bcopy

/* The 601 and 603 cope well with unaligned word accesses, so we can
 * forget about worrying about word alignment issues - the only exception
 * to this is on a page boundary. This means that we can go faster than
 * on a 604
 */

#if	0	/* ndef	PPC604 */

/* For the moment, just use the 604 version, TODO NMGS optimise */

#else	/* PPC604 */

/*
 * Copyright (C) 1993, 1994, 1995  Tim Olson
 *
 * This software is distributed absolutely without warranty. You are
 * free to use and modify the software as you wish.  You are also free
 * to distribute the software as long as you retain the above copyright
 * notice, and you make clear what your modifications were.
 *
 * Send comments and bug reports to tim@apple.com
 */


	.section ".data"
	.align	ALIGN
	.align	2
	.type 	EXT(memcpyAlignVector),@object
	.size	EXT(memcpyAlignVector),4*64
        .globl  EXT(memcpyAlignVector)

/* TODO NMGS This doesn't need to be exported, it should be static.
 * It's just that I copied this declaration stuff from elsewhere!
 */	
EXT(memcpyAlignVector):
	.long	.mm0s0c0
	.long	.mm0s0c1
	.long	.mm0s0c2
	.long	.mm0s0c3
	.long	.mm0s1c0
	.long	.mm0s1c1
	.long	.mm0s1c2
	.long	.mm0s1c3
	.long	.mm0s2c0
	.long	.mm0s2c1
	.long	.mm0s2c2
	.long	.mm0s2c3
	.long	.mm0s3c0
	.long	.mm0s3c1
	.long	.mm0s3c2
	.long	.mm0s3c3
	.long	.mm1s0c0
	.long	.mm1s0c1
	.long	.mm1s0c2
	.long	.mm1s0c3
	.long	.mm1s1c0
	.long	.mm1s1c1
	.long	.mm1s1c2
	.long	.mm1s1c3
	.long	.mm1s2c0
	.long	.mm1s2c1
	.long	.mm1s2c2
	.long	.mm1s2c3
	.long	.mm1s3c0
	.long	.mm1s3c1
	.long	.mm1s3c2
	.long	.mm1s3c3
	.long	.mm2s0c0
	.long	.mm2s0c1
	.long	.mm2s0c2
	.long	.mm2s0c3
	.long	.mm2s1c0
	.long	.mm2s1c1
	.long	.mm2s1c2
	.long	.mm2s1c3
	.long	.mm2s2c0
	.long	.mm2s2c1
	.long	.mm2s2c2
	.long	.mm2s2c3
	.long	.mm2s3c0
	.long	.mm2s3c1
	.long	.mm2s3c2
	.long	.mm2s3c3
	.long	.mm3s0c0
	.long	.mm3s0c1
	.long	.mm3s0c2
	.long	.mm3s0c3
	.long	.mm3s1c0
	.long	.mm3s1c1
	.long	.mm3s1c2
	.long	.mm3s1c3
	.long	.mm3s2c0
	.long	.mm3s2c1
	.long	.mm3s2c2
	.long	.mm3s2c3
	.long	.mm3s3c0
	.long	.mm3s3c1
	.long	.mm3s3c2
	.long	.mm3s3c3

	.text	

		
/*
 * high-performance memcpy implementation for 604
 * uses aligned transfers plus alignment shuffling code
 */

	
ENTRY(memcpy, TAG_NO_FRAME_USED)

	cmpwi	CR0, byteCount, 0
	beqlr-	CR0
	mr	dst,	retValue	/* Move dst from retval to 'dst' */
	crset	CACHE_FLAG
.L_memcpy_bcopy:
		/* (jumped to) entry point for bcopy */
	
	cmpwi	CR0, byteCount, 8
	bge	.awm2
	
	/* handle a byte at a time for short moves */
	mtctr	byteCount
	addi	src, src, -1
	addi	dst, dst, -1	
.awm1:	
	lbz	tmp0, 1(src)
	addi	src,	src, 1
	stb	tmp0, 1(dst)
	addi	dst,	dst, 1
	bdnz	.awm1
	blr


.awm2:
	/* special case long, cache-block aligned transfers */
	andi.	tmp0, src, CACHE_LINE_SIZE-1	/* cache block aligned? */
	bne	.awm3
	andi.	tmp0, dst, CACHE_LINE_SIZE-1
	bne	.awm3
	andi.	tmp0, byteCount, CACHE_LINE_SIZE-1
	bne	.awm3

	srwi	wordCount, byteCount, 5	    /* compute blocks to transfer */
#if USE_FLOAT
	/* Using floating point regs implies accesses 8 bytes at a time */
	addi	src, src, -8
	addi	dst, dst, -8
#else
	/* Using ints implies accesses 4 bytes at a time */
	addi	src, src, -4
	addi	dst, dst, -4
#endif
	mtctr	wordCount
	li	bufB, 32
.mmc0:

#if CACHE_LINE_SIZE < 32
#error code assumes CACHE_LINE_SIZE >= 32, and prefers it at 32 exactly
#endif
        bf      CACHE_FLAG,.L_bcopy_skip_dcb	/* Skip the cache instructions if not cached */


	dcbz	bufB, dst
	dcbt	bufB, src

.L_bcopy_skip_dcb:

#if USE_FLOAT
		/* We can use floating point regs, this zooms */
	lfd	bufA, 8(src)
	addi	src,	src, 8
	stfd	bufA, 8(dst)
	addi	dst,	dst, 8
	lfd	bufA, 8(src)
	addi	src,	src, 8
	stfd	bufA, 8(dst)
	addi	dst,	dst, 8
	lfd	bufA, 8(src)
	addi	src,	src, 8
	stfd	bufA, 8(dst)
	addi	dst,	dst, 8
	lfd	bufA, 8(src)
	addi	src,	src, 8
	stfd	bufA, 8(dst)
	addi	dst,	dst, 8
#else
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
#endif /* USE_FLOAT */
	
	bdnz	.mmc0
	
	blr
		
.awm3:	
	/* compute alignment transfer vector */
	addis	bufB, 0,	memcpyAlignVector@ha
	addi	bufB, bufB,	memcpyAlignVector@l

	srwi.	wordCount, byteCount, 2		/* compute words to transfer */
	rlwinm	bufA, dst, 6, 24, 25
	rlwimi	bufA, src, 4, 26, 27
	rlwimi	bufA, byteCount, 2, 28, 29
	lwzx	tmp0, bufA, bufB
	mtctr	tmp0
	andi.	byteCount, byteCount, 3
	bctr
	
	
/* forward copy destination aligned at 0, source aligned at 0, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = 0123 4567 89ab xxxx */
	.align	3
.mm0s0c0:
	srwi	tmp0,	wordCount,	1
	addi	src, src, -4
	addi	dst, dst, -4
	cmpi	CR0,	wordCount,	1
	mtctr	tmp0

	ble-	.mm0s0c0a1

.mm0s0c0a:
	lwz	bufA, 4(src)
	lwz	bufB, 8(src)
	addi	src,	src, 8
	stw	bufA, 4(dst)
	stw	bufB, 8(dst)
	addi	dst,	dst, 8
	bdnz	.mm0s0c0a

	/* if even number of words, return */
	andi.	wordCount,	wordCount,	1
	beqlr

	/* otherwise copy last word */
.mm0s0c0a1:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
		
	blr

/* forward copy destination aligned at 0, source aligned at 0, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = 0123 4567 89ab cxxx */
	.align	3
.mm0s0c1:
	mtctr	wordCount
	addi	src, src, -4
	addi	dst, dst, -4

.mm0s0c1a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm0s0c1a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 0, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = 0123 4567 89ab cdxx */
	.align	3
.mm0s0c2:
	mtctr	wordCount
	addi	src, src, -4
	addi	dst, dst, -4

.mm0s0c2a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm0s0c2a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 0, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = 0123 4567 89ab cdex */
	.align	3
.mm0s0c3:
	mtctr	wordCount
	addi	src, src, -4
	addi	dst, dst, -4

.mm0s0c3a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm0s0c3a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 1, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = x012 3456 789a bxxx */
	.align	3
.mm0s1c0:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src,	src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 8

.mm0s1c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c0a

	blr

/* forward copy destination aligned at 0, source aligned at 1, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = x012 3456 789a bcxx */
	.align	3
.mm0s1c1:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src,	src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 8

.mm0s1c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 1, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = x012 3456 789a bcdx */
	.align	3
.mm0s1c2:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src,	src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 8

.mm0s1c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 1, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = x012 3456 789a bcde xxxx */
	.align	3
.mm0s1c3:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src,	src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 8

.mm0s1c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 2, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = xx01 2345 6789 abxx */
	.align	3
.mm0s2c0:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 16

.mm0s2c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c0a

	blr

/* forward copy destination aligned at 0, source aligned at 2, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = xx01 2345 6789 abcx */
	.align	3
.mm0s2c1:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 16

.mm0s2c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 2, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	3
.mm0s2c2:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 16

.mm0s2c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 2, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = xx01 2345 6789 abcd exxx */
	.align	3
.mm0s2c3:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 16

.mm0s2c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c3a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 3, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = xxx0 1234 5678 9abx */
	.align	3
.mm0s3c0:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 24

.mm0s3c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c0a

	blr

/* forward copy destination aligned at 0, source aligned at 3, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	3
.mm0s3c1:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 24

.mm0s3c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 3, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	3
.mm0s3c2:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 24

.mm0s3c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c2a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 0, source aligned at 3, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = xxx0 1234 5678 9abc dexx */
	.align	3
.mm0s3c3:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	addi	dst,	dst, 0
	slwi	bufA, bufB, 24

.mm0s3c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c3a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 0, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = 0123 4567 89ab xxxx */
	.align	3
.mm1s0c0:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufA, 24

.mm1s0c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 0, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = 0123 4567 89ab cxxx */
	.align	3
.mm1s0c1:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufA, 24

.mm1s0c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c1a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 0, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = 0123 4567 89ab cdxx */
	.align	3
.mm1s0c2:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufA, 24

.mm1s0c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c2a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 0, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = 0123 4567 89ab cdex */
	.align	3
.mm1s0c3:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufA, 24

.mm1s0c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c3a

	blr

/* forward copy destination aligned at 1, source aligned at 1, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = x012 3456 789a bxxx */
	.align	3
.mm1s1c0:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1

.mm1s1c0a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm1s1c0a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 1, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = x012 3456 789a bcxx */
	.align	3
.mm1s1c1:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1

.mm1s1c1a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm1s1c1a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 1, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = x012 3456 789a bcdx */
	.align	3
.mm1s1c2:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1

.mm1s1c2a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm1s1c2a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 1, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = x012 3456 789a bcde xxxx */
	.align	3
.mm1s1c3:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1

.mm1s1c3a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm1s1c3a

	blr

/* forward copy destination aligned at 1, source aligned at 2, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = xx01 2345 6789 abxx */
	.align	3
.mm1s2c0:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 8

.mm1s2c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 2, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = xx01 2345 6789 abcx */
	.align	3
.mm1s2c1:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 8

.mm1s2c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 2, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	3
.mm1s2c2:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 8

.mm1s2c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 2, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = xx01 2345 6789 abcd exxx */
	.align	3
.mm1s2c3:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src,	src, 2
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 8

.mm1s2c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c3a

	blr

/* forward copy destination aligned at 1, source aligned at 3, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = xxx0 1234 5678 9abx */
	.align	3
.mm1s3c0:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 16

.mm1s3c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 3, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	3
.mm1s3c1:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 16

.mm1s3c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 3, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	3
.mm1s3c2:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 16

.mm1s3c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c2a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 1, source aligned at 3, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = xxx0 1234 5678 9abc dexx */
	.align	3
.mm1s3c3:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	addi	dst,	dst, -1
	slwi	bufA, bufB, 16

.mm1s3c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c3a

	blr

/* forward copy destination aligned at 2, source aligned at 0, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = 0123 4567 89ab xxxx */
	.align	3
.mm2s0c0:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 16

.mm2s0c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 0, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = 0123 4567 89ab cxxx */
	.align	3
.mm2s0c1:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 16

.mm2s0c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c1a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 0, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = 0123 4567 89ab cdxx */
	.align	3
.mm2s0c2:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 16

.mm2s0c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c2a

	blr

/* forward copy destination aligned at 2, source aligned at 0, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = 0123 4567 89ab cdex */
	.align	3
.mm2s0c3:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 16

.mm2s0c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 1, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = x012 3456 789a bxxx */
	.align	3
.mm2s1c0:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 24

.mm2s1c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c0a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 1, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = x012 3456 789a bcxx */
	.align	3
.mm2s1c1:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 24

.mm2s1c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c1a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 1, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = x012 3456 789a bcdx */
	.align	3
.mm2s1c2:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 24

.mm2s1c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c2a

	blr

/* forward copy destination aligned at 2, source aligned at 1, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = x012 3456 789a bcde xxxx */
	.align	3
.mm2s1c3:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufA, 24

.mm2s1c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 2, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = xx01 2345 6789 abxx */
	.align	3
.mm2s2c0:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2

.mm2s2c0a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm2s2c0a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 2, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = xx01 2345 6789 abcx */
	.align	3
.mm2s2c1:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2

.mm2s2c1a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm2s2c1a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 2, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	3
.mm2s2c2:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2

.mm2s2c2a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm2s2c2a

	blr

/* forward copy destination aligned at 2, source aligned at 2, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = xx01 2345 6789 abcd exxx */
	.align	3
.mm2s2c3:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2

.mm2s2c3a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm2s2c3a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 3, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = xxx0 1234 5678 9abx */
	.align	3
.mm2s3c0:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufB, 8

.mm2s3c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 3, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	3
.mm2s3c1:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufB, 8

.mm2s3c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 2, source aligned at 3, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	3
.mm2s3c2:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufB, 8

.mm2s3c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c2a

	blr

/* forward copy destination aligned at 2, source aligned at 3, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = xxx0 1234 5678 9abc dexx */
	.align	3
.mm2s3c3:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src,	src, 1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	addi	dst,	dst, -2
	slwi	bufA, bufB, 8

.mm2s3c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 0, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = 0123 4567 89ab xxxx */
	.align	3
.mm3s0c0:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 8

.mm3s0c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 0, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = 0123 4567 89ab cxxx */
	.align	3
.mm3s0c1:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 8

.mm3s0c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c1a

	blr

/* forward copy destination aligned at 3, source aligned at 0, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = 0123 4567 89ab cdxx */
	.align	3
.mm3s0c2:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 8

.mm3s0c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 0, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = 0123 4567 89ab cdex */
	.align	3
.mm3s0c3:
	lwz	bufA, 0(src)
	addi	src,	src, 0
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 8

.mm3s0c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 1, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = x012 3456 789a bxxx */
	.align	3
.mm3s1c0:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 16

.mm3s1c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c0a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 1, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = x012 3456 789a bcxx */
	.align	3
.mm3s1c1:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 16

.mm3s1c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c1a

	blr

/* forward copy destination aligned at 3, source aligned at 1, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = x012 3456 789a bcdx */
	.align	3
.mm3s1c2:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 16

.mm3s1c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 1, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = x012 3456 789a bcde xxxx */
	.align	3
.mm3s1c3:
	lwz	bufA, -1(src)
	addi	src,	src, -1
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 16

.mm3s1c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 2, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = xx01 2345 6789 abxx */
	.align	3
.mm3s2c0:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 24

.mm3s2c0a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c0a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 2, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = xx01 2345 6789 abcx */
	.align	3
.mm3s2c1:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 24

.mm3s2c1a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c1a

	blr

/* forward copy destination aligned at 3, source aligned at 2, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	3
.mm3s2c2:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 24

.mm3s2c2a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 2, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = xx01 2345 6789 abcd exxx */
	.align	3
.mm3s2c3:
	lwz	bufA, -2(src)
	addi	src,	src, -2
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3
	slwi	bufA, bufA, 24

.mm3s2c3a:
	lwz	bufB, 4(src)
	addi	src,	src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c3a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 3, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = xxx0 1234 5678 9abx */
	.align	3
.mm3s3c0:
	lwz	bufA, -3(src)
	addi	src,	src, -3
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3

.mm3s3c0a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm3s3c0a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 3, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	3
.mm3s3c1:
	lwz	bufA, -3(src)
	addi	src,	src, -3
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3

.mm3s3c1a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm3s3c1a

	blr

/* forward copy destination aligned at 3, source aligned at 3, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	3
.mm3s3c2:
	lwz	bufA, -3(src)
	addi	src,	src, -3
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3

.mm3s3c2a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm3s3c2a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	blr

/* forward copy destination aligned at 3, source aligned at 3, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = xxx0 1234 5678 9abc dexx */
	.align	3
.mm3s3c3:
	lwz	bufA, -3(src)
	addi	src,	src, -3
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	addi	dst,	dst, -3

.mm3s3c3a:
	lwz	bufA, 4(src)
	addi	src,	src, 4
	stw	bufA, 4(dst)
	addi	dst,	dst, 4
	bdnz	.mm3s3c3a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	blr
#endif	/* PPC604 */
