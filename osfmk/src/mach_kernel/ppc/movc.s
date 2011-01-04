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

#include <debug.h>
#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <assym.s>



/* Change this when we get an assembler that understands a DCBA */

#define dcba dcbz




/*
 * void pmap_zero_page(vm_offset_t pa)
 *
 * zero a page of physical memory.
 */

#if DEBUG
	/* C debug stub in pmap.c calls this */
ENTRY(pmap_zero_page_assembler, TAG_NO_FRAME_USED)
#else
ENTRY(pmap_zero_page, TAG_NO_FRAME_USED)
#endif /* DEBUG */

		li		r0,	0						/* Ensure r0 is 0 before first sync */

		mfmsr	r6								/* Get the MSR */
		rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1	/* Turn off DR */
		li		r4,PPC_PGBYTES-CACHE_LINE_SIZE	/* Point to the end of the page */
		mtmsr	r7								/* Set MSR to DR off */
		isync									/* Ensure data translations are off */


.L_phys_zero_loop:	
		subic.	r5,r4,CACHE_LINE_SIZE			/* Point to the next one */
		dcbz	r4, r3							/* Clear the whole thing to 0s */
		subi	r4,r5,CACHE_LINE_SIZE			/* Point to the next one */
		dcbz	r5, r3							/* Clear the next to zeros */
		bgt+	.L_phys_zero_loop				/* Keep going until we do the page... */

		sync									/* Make sure they're all done */
		li		r4,PPC_PGBYTES-CACHE_LINE_SIZE	/* Point to the end of the page */

.L_inst_inval_loop:	
		subic.	r5,r4,CACHE_LINE_SIZE			/* Point to the next one */
		icbi	r4, r3							/* Clear the whole thing to 0s */
		subi	r4,r5,CACHE_LINE_SIZE			/* Point to the next one */
		icbi	r5, r3							/* Clear the next to zeros */
		bgt+	.L_inst_inval_loop				/* Keep going until we do the page... */

		sync									/* Make sure they're all done */

		mtmsr	r6		/* Restore original translations */
		isync			/* Ensure data translations are on */

		blr

/* void
 * phys_copy(src, dst, bytecount)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *      int             bytecount
 *
 * This routine will copy bytecount bytes from physical address src to physical
 * address dst. 
 */

ENTRY(phys_copy, TAG_NO_FRAME_USED)

	li		r0,	0						/* Ensure r0 is 0 before first sync */

	/* Switch off data translations */
	mfmsr	r6
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync			/* Ensure data translations are off */

	subi	ARG0,	ARG0,	4
	subi	ARG1,	ARG1,	4

	cmpwi	ARG2,	3
	ble-	.L_phys_copy_bytes
.L_phys_copy_loop:
	lwz	r0,	4(ARG0)
	addi	ARG0,	ARG0,	4
	subi	ARG2,	ARG2,	4
	stw	r0,	4(ARG1)
	addi	ARG1,	ARG1,	4
	cmpwi	ARG2,	3
	bgt+	.L_phys_copy_loop

	/* If no leftover bytes, we're done now */
	cmpwi	ARG2,	0
	beq+	.L_phys_copy_done
	
.L_phys_copy_bytes:
	addi	ARG0,	ARG0,	3
	addi	ARG1,	ARG1,	3
.L_phys_copy_byte_loop:	
	lbz	r0,	1(ARG0)
	addi	ARG0,	ARG0,	1
	subi	ARG2,	ARG2,	1
	stb	r0,	1(ARG1)
	addi	ARG1,	ARG1,	1
	cmpwi	ARG2,	0
	bne+	.L_phys_copy_loop

.L_phys_copy_done:
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are off */

	blr

/* void
 * pmap_copy_page(src, dst)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *
 * This routine will copy the physical page src to physical page dst
 * 
 * This routine assumes that the src and dst are page aligned and that the
 * destination is cached.
 *
 * We also must assume that noone will be executing within the destination
 * page.  We also assume that this will be used for paging
 *
 */

#if DEBUG
	/* if debug, we have a little piece of C around this
	 * in pmap.c that gives some trace ability
	 */
ENTRY(pmap_copy_page_assembler, TAG_NO_FRAME_USED)
#else
ENTRY(pmap_copy_page, TAG_NO_FRAME_USED)
#endif /* DEBUG */

	/* Save off the link register, we want to call fpu_save. We assume
	 * that fpu_save leaves ARG0-5 intact and that it doesn't need
	 * a frame, otherwise we'd need one too.
	 */
	
		mflr	r7
		bl		fpu_save
		mtlr	r7

		li		r0,	0						/* Ensure r0 is 0 before first sync */

		/* Switch off data translations */
		mfmsr	r9
		rlwinm	r7,	r9,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
		mtmsr	r7
		isync			/* Ensure data translations are off */


		dcbt	0, r3						/* Start in first input line */
		addi	r6,r3,PPC_PGBYTES			/* Point to the start of the next page */
		
		mr		r8,r4						/* Save the destination */

		li		r5,	CACHE_LINE_SIZE			/* Get the line size */

.L_pmap_copy_page_loop:
		dcba	0, r4						/* Allocate a line for the output */
		lfd		f0, 0(r3)					/* Get first 8 */
		lfd		f1, 8(r3)					/* Get second 8 */
		lfd		f2, 16(r3)					/* Get third 8 */
		stfd	f0, 0(r4)					/* Put first 8 */
		dcbt	r5, r3						/* Start next line coming in */
		lfd		f3, 24(r3)					/* Get fourth 8 */
		stfd	f1,	8(r4)					/* Put second 8 */
		addi	r3,r3,CACHE_LINE_SIZE		/* Point to the next line in */
		stfd	f2,	16(r4)					/* Put third 8 */
		cmplw	cr0,r3,r6					/* See if we're finished yet */
		stfd	f3,	24(r4)					/* Put fourth 8 */
		dcbst	0,r4						/* Force it out */
		addi	r4,r4,CACHE_LINE_SIZE		/* Point to the next line out */
		blt+	.L_pmap_copy_page_loop		/* Copy the whole page */
		
		sync									/* Make sure they're all done */
		li		r4,PPC_PGBYTES-CACHE_LINE_SIZE	/* Point to the end of the page */

invalinst:	
		subic.	r5,r4,CACHE_LINE_SIZE		/* Point to the next one */
		icbi	r4, r8						/* Trash the i-cache */
		subi	r4,r5,CACHE_LINE_SIZE		/* Point to the next one */
		icbi	r5, r8						/* Trash the i-cache */
		bgt+	invalinst					/* Keep going until we do the page... */

		sync								/* Make sure all invalidates done */		
	
		mtmsr	r9							/* Restore original translations */
		isync								/* Ensure data translations are on */

		blr

/*
 * int
 * copyin(src, dst, count)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	int		count;
 *
 * We can't generate alignment exceptions here - so use bcopy which
 * is guaranteed not to do this. That means we need to be able to call
 * a sub-function. We USE r10 and assume that bcopy won't need or trash it!
 */

ENTRY2(copyin, copyinmsg, TAG_NO_FRAME_USED)
	/* Preamble allowing us to call a sub-function */
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	stwu	r1,	-FM_SIZE(r1)

	lwz	r6,	PP_CPU_DATA(r2)
	lwz	r10,	CPU_ACTIVE_THREAD(r6)
	cmpli	CR0,	ARG2,	0
	ble-	CR0,	.L_copyinout_trivial
	/* we know we have a valid copyin to do now */

	/* Set up thread_recover in case we hit an illegal address */
	addis	r7,	0,	.L_copyinout_error@ha
	addi	r7,	r7,	.L_copyinout_error@l
	stw	r7,	THREAD_RECOVER(r10)

	lwz	r8,	THREAD_TOP_ACT(r10)
	lwz	r8,	ACT_VMMAP(r8)
	lwz	r8,	VMMAP_PMAP(r8)
	lwz	r8,	PMAP_SPACE(r8)

	lis	r7,	SEG_REG_PROT>>16	/* Top byte of SR value */
	rlwimi	r7,	r8,	4,	4,	31 /* Insert space<<4   */
	rlwimi	r7,	ARG0,	4,	28,	31 /* Insert seg number */

	mtsr	SR_COPYIN,	r7
	isync

	/* ARG0 = adjusted user pointer mapping into SR_COPYIN */
	rlwinm	ARG0,	ARG0,	0,	4,	31
	oris	ARG0,	ARG0,	(SR_COPYIN << (28-16))

	/* For optimisation, we check if the copyin lies on a segment
	 * boundary. If it doesn't, we can use a simple copy. If it
	 * does, we split it into two separate copies in some C code.
	 */
	/* see if (start+size-1) is in same segment */
	add	r7,	ARG0,	ARG2
	subi	r7,	r7,	1

	rlwinm	r7,	r7,	0,	0,	3
	rlwinm	r0,	ARG0,	0,	0,	3
	cmpw	CR0,	r0,	r7
	bne-	.L_call_copyin_multiple

	/* Call bcopy, but need to keep r10 contents (thread ptr) safe */

	bl	bcopy
	
	/* Now that copyin is done, we don't need a recovery point */
	li	r7,	0
	stw	r7,	THREAD_RECOVER(r10)

	li	ARG0,	0	/* Indicate success */

	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0
	blr

.L_copyinout_error:
	/* we get here via the exception handler if an illegal
	 * user memory reference was made.
	 */

	/* Now that copyin is done, we don't need a recovery point */
	li	r7,	0
	stw	r7,	THREAD_RECOVER(r10)

	li	ARG0,	1	/* Indicate error */

	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0

	blr

.L_copyinout_trivial:
	/* The copyin/out was for either 0 bytes or a negative
	 * number of bytes, return an appropriate value (0 == SUCCESS).
	 * CR0 still contains result of comparison of len with 0.
	 */
	li	ARG0,	0
	beq+	CR0,	.L_copyinout_negative
	li	ARG0,	1
.L_copyinout_negative:

	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0

	blr

.L_call_copyin_multiple:

	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0

	b	copyin_multiple		/* not a call - a jump! */

/*
 * int
 * copyout(src, dst, count)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	int		count;
 *
 * We can't generate alignment exceptions here - so use bcopy which
 * is guaranteed not to do this. That means we need to be able to call
 * a sub-function. We USE r10 and assume that bcopy won't need or trash r10!
 */

ENTRY2(copyout, copyoutmsg, TAG_NO_FRAME_USED)
	/* Preamble allowing us to call a sub-function */
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	stwu	r1,	-FM_SIZE(r1)

	lwz	r6,	PP_CPU_DATA(r2)
	lwz	r10,	CPU_ACTIVE_THREAD(r6)
	cmpli	CR0,	ARG2,	0
	ble-	CR0,	.L_copyinout_trivial
	/* we know we have a valid copyout to do now */

	/* Set up thread_recover in case we hit an illegal address */
	addis	r7,	0,	.L_copyinout_error@ha
	addi	r7,	r7,	.L_copyinout_error@l
	stw	r7,	THREAD_RECOVER(r10)

	lwz	r8,	THREAD_TOP_ACT(r10)
	lwz	r8,	ACT_VMMAP(r8)
	lwz	r8,	VMMAP_PMAP(r8)
	lwz	r8,	PMAP_SPACE(r8)

	lis	r7,	SEG_REG_PROT>>16	/* Top byte of SR value */
	rlwimi	r7,	r8,	4,	4,	31 /* Insert space<<4   */
	rlwimi	r7,	ARG1,	4,	28,	31 /* Insert seg number */

	mtsr	SR_COPYIN,	r7
	isync

	/* ARG1 = adjusted user pointer mapping into SR_COPYIN */
	rlwinm	ARG1,	ARG1,	0,	4,	31
	oris	ARG1,	ARG1,	(SR_COPYIN << (28-16))

	/* For optimisation, we check if the copyout lies on a segment
	 * boundary. If it doesn't, we can use a simple copy. If it
	 * does, we split it into two separate copies in some C code.
	 */
	/* see if (start+size-1) is in same segment */
	add	r7,	ARG1,	ARG2
	subi	r7,	r7,	1

	rlwinm	r7,	r7,	0,	0,	3
	rlwinm	r0,	ARG1,	0,	0,	3
	cmpw	CR0,	r0,	r7
	bne-	.L_call_copyout_multiple

	/* Call bcopy, but need to keep r10 contents (thread ptr) safe */

	bl	bcopy
	
	/* Now that copyout is done, we don't need a recovery point */
	li	r7,	0
	stw	r7,	THREAD_RECOVER(r10)

	li	ARG0,	0	/* Indicate success */

	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0

	blr

.L_call_copyout_multiple:
	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0

	b	copyout_multiple	/* not a call - a jump! */

/*
 * boolean_t
 * copyinstr(src, dst, count, maxcount)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	vm_size_t	maxcount; 
 *	vm_size_t*	count;
 *
 * Set *count to the number of bytes copied
 * 
 * If dst == NULL, don't copy, just count bytes.
 * Only currently called from klcopyinstr. 
 */

ENTRY(copyinstr, TAG_NO_FRAME_USED)
	/* Preamble allowing us to call a sub-function */
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	stwu	r1,	-FM_SIZE(r1)

	lwz	ARG4,	PP_CPU_DATA(r2)
	lwz	r10,	CPU_ACTIVE_THREAD(ARG4)

	/* Say that we copy no bytes until we say something different... */
	li	r0,	0
	stw	r0,	0(ARG3)

	cmpli	CR0,	ARG2,	0
	ble-	CR0,	.L_copyinout_trivial
	/* we know we have a valid copyinstr to do now */

	/* Set up thread_recover in case we hit an illegal address */
	addis	ARG5,	0,	.L_copyinout_error@ha
	addi	ARG5,	ARG5,	.L_copyinout_error@l
	stw	ARG5,	THREAD_RECOVER(r10)

	lwz	ARG6,	THREAD_TOP_ACT(r10)
	lwz	ARG6,	ACT_VMMAP(ARG6)
	lwz	ARG6,	VMMAP_PMAP(ARG6)
	lwz	ARG6,	PMAP_SPACE(ARG6)

	lis	ARG5,	SEG_REG_PROT>>16	/* Top byte of SR value */
	rlwimi	ARG5,	ARG6,	4,	4,	31 /* Insert space<<4   */
	rlwimi	ARG5,	ARG0,	4,	28,	31 /* Insert seg number */

	mtsr	SR_COPYIN,	ARG5
	isync

	/* ARG0 = adjusted user pointer mapping into SR_COPYIN */
	rlwinm	ARG0,	ARG0,	0,	4,	31
	oris	ARG0,	ARG0,	(SR_COPYIN << (28-16))

	/* Copy byte by byte for now - TODO NMGS speed this up with
	 * some clever (but fairly standard) logic for word copies.
	 * We don't use a copyinstr_multiple since copyinstr is called
	 * with INT_MAX in the linux server. Eugh.
	 */

	li	ARG6,	0

	/* If the destination is NULL, don't do writes,
	 * just count bytes. We set CR7 outside the loop to save time
	 */
	cmpwi	CR7,	ARG1,	0
	
	/* if we _do_ store, we do load/store with updates on ARG1*/
	
	subi	ARG1,	ARG1,	1

.L_copyinstr_loop:
	lbz	r0,	0(ARG0)

	addic.	ARG2,	ARG2,	-1

	cmpwi	CR1,	r0,	0

	addi	ARG0,	ARG0,	1

	/* Jump over store if dest == NULL */
	beq	CR7,	.L_copyinstr_no_store
	
	stb	r0,	1(ARG1)
	addi	ARG1,	ARG1,	1

.L_copyinstr_no_store:

	addi	ARG6,	ARG6,	1

	/* Did we find the zero byte? */
	beq-	CR1,	.L_copyinstr_done

	/* did we get to MAXLEN? */
	beq-	CR0,	.L_copyinstr_done
	
	/* Check to see if the copyin pointer has moved out of the
	 * copyin segment, if it has we must remap.
	 */

	rlwinm.	r0,	ARG0,	0,	4,	31
	bne+	CR0,	.L_copyinstr_loop

	/* We have gone beyond the segment boundary, remap */

	addi	ARG5,	ARG5,	1
	li		r0,	0				/* Ensure R0 is 0 before sync */
	cmpwi	r0, 0
	bne+	0f
	sync
0:
	mtsr	SR_COPYIN,	ARG5
	isync

	/* and change the pointer back to the start of the segment */

	rlwinm	ARG0,	ARG0,	0,	4,	31
	oris	ARG0,	ARG0,	(SR_COPYIN << (28-16))
	
	b	.L_copyinstr_loop
	
.L_copyinstr_done:
	/* set *count to the number of bytes copied */
	stw	ARG6,	0(ARG3)

	/* we return success */
	li	ARG0,	0
			
	/* Now that copyinstr is done, we don't need a recovery point */
	li	ARG5,	0
	stw	ARG5,	THREAD_RECOVER(r10)

	/* unwind the stack */
	addi	r1,	r1,	FM_SIZE
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0
	blr
