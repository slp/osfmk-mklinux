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

#include <ppc/asm.h>
#include <ppc/proc_reg.h>

#	
# void setbit(int bitno, int *s)
# 
# Set indicated bit in bit string.
#     Note:	being big-endian, bit 0 is 0x80000000.
	
ENTRY(setbit,TAG_NO_FRAME_USED)
	cmpwi	ARG0,	BITS_PER_WORD	/* Is bit in first word?     */
	blt+	.L_setbit_bits 		/* Predict bit in first word */

	/* calculate byte offset of word - 32 bits represent 4 bytes.
	 * (shift right 3 bits) and align to word boundary
	 */
	rlwinm	r5,	ARG0,	29,	3,	29

	add	ARG1,	ARG1,	r5	/* move pointer to correct word */
	andi.	ARG0,	ARG0,	BITS_PER_WORD-1 /* ARG0 == offset into word */

.L_setbit_bits:	
	/* TODO NMGS - optimise setbit function further */

	li	r6,	BITS_PER_WORD-1
	sub	ARG0,	r6,	ARG0		/* ARG0 = 31-ARG0 */

	lwz	r5,	0(ARG1)
	li	r0,	1
	rotlw	r0,	r0,	ARG0
	or	r5,	r5,	r0
	stw	r5,	0(ARG1)

	blr	
	
#	
# void clrbit(int bitno, int *s)
# 
# Clear indicated bit in bit string.
#     Note:	being big-endian, bit 0 is 0x80000000.
	
ENTRY(clrbit,TAG_NO_FRAME_USED)
	cmpwi	ARG0,	BITS_PER_WORD	/* Is bit in first word?     */
	blt+	.L_clrbit_bits 		/* Predict bit in first word */

	/* calculate byte offset of word - 32 bits represent 4 bytes.
	 * (shift right 3 bits) and align to word boundary
	 */
	rlwinm	r5,	ARG0,	29,	3,	29

	add	ARG1,	ARG1,	r5	/* move pointer to correct word */
	andi.	ARG0,	ARG0,	BITS_PER_WORD-1 /* ARG0 == offset into word */

.L_clrbit_bits:	
	/* TODO NMGS - optimise clrbit function */

	li	r6,	BITS_PER_WORD-1
	sub	ARG0,	r6,	ARG0		/* ARG0 = 31-ARG0 */

	lwz	r5,	0(ARG1)
	li	r0,	1
	rotlw	r0,	r0,	ARG0
	andc	r5,	r5,	r0
	stw	r5,	0(ARG1)

	blr	

# /*
#  * Find first bit set in bit string.
#  */
# int
# ffsbit(int *s)
#
# Returns the bit index of the first bit set (starting from 0)
# Assumes pointer is word-aligned

ENTRY(ffsbit, TAG_NO_FRAME_USED)
	lwz	r0,	0(ARG0)
		mr	ARG1,	ARG0	/* Free up ARG0 for result */

	cmpwi	r0,	0		/* Check against zero... */
		cntlzw	ARG0,	r0	/* Free inst... find the set bit... */
	bnelr+				/* Return if bit in first word */

.L_ffsbit_lp:
	lwz	r0,	4(ARG1)
	addi	ARG1,	ARG1,	4
	cmpwi	r0,	0		/* Check against zero... */
		cntlzw	r12,	r0
		add	ARG0,	ARG0,	r12	/* ARG0 keeps bit count */
	beq+	.L_ffsbit_lp
	blr
	
/*
 * int tstbit(int bitno, int *s)
 *
 * Test indicated bit in bit string.
 *	Note:	 being big-endian, bit 0 is 0x80000000.
 */

ENTRY(tstbit, TAG_NO_FRAME_USED)
	cmpwi	ARG0,	BITS_PER_WORD	/* is bit in first word ? */
	blt+	.L_tstbit_bits		/* predict bit in first word */

	/*
	 * Calculate byte offset of word - 32 bits represent 4 bytes.
	 * (shift right 3 bits) and align to word boundary.
	 */
	rlwinm	r5,	ARG0,	29,	3,	29

	add	ARG1,	ARG1,	r5	/* move pointer to correct word */
	andi.	ARG0,	ARG0,	BITS_PER_WORD-1	/* ARG0 == offset into word */

.L_tstbit_bits:
	li	r6,	BITS_PER_WORD-1
	sub	r6,	r6,	ARG0	/* r6 = 31 - ARG0 */

	lwz	ARG0,	0(ARG1)
	li	r0,	1
	rotlw	r0,	r0,	r6
	and	ARG0,	ARG0,	r0	/* ARG0 is return value */
	
	blr
