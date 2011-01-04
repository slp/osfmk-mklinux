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

#include <cpus.h>

#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <cpus.h>
#include <assym.s>
#include <mach_debug.h>
#include <mach/ppc/vm_param.h>

/*
 * extern void sync_cache(vm_offset_t pa, unsigned count);
 *
 * sync_cache takes a physical address and count to sync, thus
 * must not be called for multiple virtual pages.
 *
 * it writes out the data cache and invalidates the instruction
 * cache for the address range in question
 */

ENTRY(sync_cache, TAG_NO_FRAME_USED)

	/* Switch off data translations */
	mfmsr	r6
	rlwinm	r0,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r0
	isync

	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	.L_sync_check
	addi	ARG1,ARG1,CACHE_LINE_SIZE
	li	r7,(CACHE_LINE_SIZE-1)	/* Align buffer & count - avoid overflow problems */
	andc	ARG1,ARG1,r7
	andc	ARG0,ARG0,r7

.L_sync_check:
	cmpwi	ARG1,	CACHE_LINE_SIZE
	ble	.L_sync_one_line
	
	/* Make ctr hold count of how many times we should loop */
	addi	r8,	ARG1,	(CACHE_LINE_SIZE-1)
	srwi	r8,	r8,	CACHE_LINE_POW2
	mtctr	r8

	/* loop to flush the data cache */
.L_sync_data_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
	dcbst	ARG0,	ARG1
	bdnz	.L_sync_data_loop
	
	sync
	mtctr	r8

	/* loop to invalidate the instruction cache */
.L_sync_inval_loop:
	icbi	ARG0,	ARG1
	addic	ARG1,	ARG1,	CACHE_LINE_SIZE
	bdnz	.L_sync_inval_loop

.L_sync_cache_done:
	sync			/* Finish physical writes */
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */
	blr

.L_sync_one_line:
	dcbst	0,ARG0
	sync
	icbi	0,ARG0
	b	.L_sync_cache_done

/*
 * extern void flush_dcache(vm_offset_t addr, unsigned count, boolean phys);
 *
 * flush_dcache takes a virtual or physical address and count to flush
 * and (can be called for multiple virtual pages).
 *
 * it flushes the data cache
 * cache for the address range in question
 *
 * if 'phys' is non-zero then physical addresses will be used
 */

ENTRY(flush_dcache, TAG_NO_FRAME_USED)

	li	r0,	0	/* Ensure R0 is 0 before first sync */

	/* optionally switch off data translations */

	cmpwi	ARG2,	0
	mfmsr	r6
	beq+	0f
	rlwinm	r0,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r0
	isync
0:

	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	.L_flush_dcache_check
	addi	ARG1,ARG1,CACHE_LINE_SIZE
	li	r7,(CACHE_LINE_SIZE-1)	/* Align buffer & count - avoid overflow problems */
	andc	ARG1,ARG1,r7
	andc	ARG0,ARG0,r7

.L_flush_dcache_check:
	cmpwi	ARG1,	CACHE_LINE_SIZE
	ble	.L_flush_dcache_one_line
	
	/* Make ctr hold count of how many times we should loop */
	addi	r8,	ARG1,	(CACHE_LINE_SIZE-1)
	srwi	r8,	r8,	CACHE_LINE_POW2
	mtctr	r8

.L_flush_dcache_flush_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
	dcbf	ARG0,	ARG1
	bdnz	.L_flush_dcache_flush_loop

.L_flush_dcache_done:
	/* Sync restore msr if it was modified */
	cmpwi	ARG2,	0
	sync			/* make sure invalidates have completed */
	beq+	0f
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */
0:
	blr

.L_flush_dcache_one_line:
	xor	ARG1,ARG1,ARG1
	dcbf	0,ARG0
	b	.L_flush_dcache_done


/*
 * extern void invalidate_dcache(vm_offset_t va, unsigned count, boolean phys);
 *
 * invalidate_dcache takes a virtual or physical address and count to
 * invalidate and (can be called for multiple virtual pages).
 *
 * it invalidates the data cache for the address range in question
 */

ENTRY(invalidate_dcache, TAG_NO_FRAME_USED)

	li	r0,	0	/* Ensure R0 is 0 before first sync */

	/* optionally switch off data translations */

	cmpwi	ARG2,	0
	mfmsr	r6
	beq+	0f
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync
0:	

	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	.L_invalidate_dcache_check
	addi	ARG1,ARG1,CACHE_LINE_SIZE
	li	r7,(CACHE_LINE_SIZE-1)	/* Align buffer & count - avoid overflow problems */
	andc	ARG1,ARG1,r7
	andc	ARG0,ARG0,r7

.L_invalidate_dcache_check:
	cmpwi	ARG1,	CACHE_LINE_SIZE
	ble	.L_invalidate_dcache_one_line
	
	/* Make ctr hold count of how many times we should loop */
	addi	r8,	ARG1,	(CACHE_LINE_SIZE-1)
	srwi	r8,	r8,	CACHE_LINE_POW2
	mtctr	r8

.L_invalidate_dcache_invalidate_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
	dcbi	ARG0,	ARG1
	dcbi	ARG0,	ARG1
	bdnz	.L_invalidate_dcache_invalidate_loop

.L_invalidate_dcache_done:
	/* Sync restore msr if it was modified */
	cmpwi	ARG2,	0
	sync			/* make sure invalidates have completed */
	beq+	0f
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */
0:
	blr

.L_invalidate_dcache_one_line:
	xor	ARG1,ARG1,ARG1
	dcbi	0,ARG0
	dcbi	0,ARG0
	b	.L_invalidate_dcache_done

/*
 * extern void invalidate_icache(vm_offset_t addr, unsigned cnt, boolean phys);
 *
 * invalidate_icache takes a virtual or physical address and
 * count to invalidate, (can be called for multiple virtual pages).
 *
 * it invalidates the instruction cache for the address range in question.
 */

ENTRY(invalidate_icache, TAG_NO_FRAME_USED)

	li	r0,	0	/* Ensure R0 is 0 before first sync */

	/* optionally switch off data translations */
	cmpwi	ARG2,	0
	mfmsr	r6
	beq+	0f
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync
0:	

	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	.L_invalidate_icache_check
	addi	ARG1,ARG1,CACHE_LINE_SIZE
	li	r7,(CACHE_LINE_SIZE-1)	/* Align buffer & count - avoid overflow problems */
	andc	ARG1,ARG1,r7
	andc	ARG0,ARG0,r7

.L_invalidate_icache_check:
	cmpwi	ARG1,	CACHE_LINE_SIZE
	ble	.L_invalidate_icache_one_line
	
	/* Make ctr hold count of how many times we should loop */
	addi	r8,	ARG1,	(CACHE_LINE_SIZE-1)
	srwi	r8,	r8,	CACHE_LINE_POW2
	mtctr	r8

.L_invalidate_icache_invalidate_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
	icbi	ARG0,	ARG1
	icbi	ARG0,	ARG1
	bdnz	.L_invalidate_icache_invalidate_loop

.L_invalidate_icache_done:
	/* Sync restore msr if it was modified */
	cmpwi	ARG2,	0
	sync			/* make sure invalidates have completed */
	beq+	0f
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */
0:
	blr

.L_invalidate_icache_one_line:
	xor	ARG1,ARG1,ARG1
	icbi	0,ARG0
	icbi	0,ARG0
	b	.L_invalidate_icache_done
