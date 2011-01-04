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

#include <machine/asm.h>

#define CACHE_LINE_SIZE 32
	
/*
 * void hw_zero_page(char *addr, unsigned int length)
 *
 * zero a region of physical memory. We assume that pa is
 * on a 32 byte boundary and that length is a multiple of 32
 */

ENTRY(hw_zero_page, TAG_NO_FRAME_USED)

.Lhw_zero_page_loop:	
	subic.	ARG1, ARG1, CACHE_LINE_SIZE
	ble-	.Lhw_zero_page_done
	dcbz	ARG0, ARG1
	b	.Lhw_zero_page_loop

.Lhw_zero_page_done:	

	blr

/* void
 * hw_copy_page(src, dst, bytecount)
 *      char*		src; 
 *      char*           dst;
 *      unsigned int    bytecount;
 *
 * This routine will copy bytecount bytes 
 *
 * This routine assumes that the src and dst are page aligned and that 
 * bytecount is divisible by 4.
 */

ENTRY(hw_copy_page, TAG_NO_FRAME_USED)

	subi	ARG0,	ARG0,	4
	subi	ARG1,	ARG1,	4

	cmpwi	ARG2,	0
	beq-	.Lhw_copy_page_done
.Lhw_copy_page_loop:
	lwz	r0,	4(ARG0)
	addi	ARG0,	ARG0,	4
	stw	r0,	4(ARG1)
	addi	ARG1,	ARG1,	4
	subi	ARG2,	ARG2,	4
	cmpwi	ARG2,	0
	bne+	.Lhw_copy_page_loop

.Lhw_copy_page_done:

	blr
