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

#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <mach_debug.h>
#include <assym.s>
#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <ppc/POWERMAC/powermac.h>

/*
 * vm_offset_t getrpc(void) - Return address of the function
 *	                      that called the current function
 */

/* By using this function, we force the caller to save its LR in a known
 * location, which we can pick up and return. See PowerPC ELF specs.
 */
ENTRY(getrpc, TAG_NO_FRAME_USED)
	lwz	ARG0,	FM_BACKPTR(r1)		/* Load our backchain ptr */
	lwz	ARG0,	FM_LR_SAVE(ARG0)	/* Load previously saved LR */
	blr					/* And return */


/* Mask and unmask interrupts at the processor level */	
ENTRY(interrupt_disable, TAG_NO_FRAME_USED)
	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_EE_BIT+1,	MSR_EE_BIT-1
	mtmsr	r0
	blr

ENTRY(interrupt_enable, TAG_NO_FRAME_USED)

	li	r0,	0	/* Ensure R0 is 0 on 601 for sync */
	cmpwi	r0,	0
	bne+	0f
	sync			/* Flush any outstanding writes */
0:
	mfmsr	r0
	ori	r0,	r0,	MASK(MSR_EE)
	mtmsr	r0
	isync
	blr

#if	MACH_KDB
/*
 * Kernel debugger versions of the spl*() functions.  This allows breakpoints
 * in the spl*() functions.
 */

/* Mask and unmask interrupts at the processor level */	
ENTRY(db_interrupt_disable, TAG_NO_FRAME_USED)
	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_EE_BIT+1,	MSR_EE_BIT-1
	mtmsr	r0
	isync
	blr

ENTRY(db_interrupt_enable, TAG_NO_FRAME_USED)
	mfmsr	r0
	ori	r0,	r0,	MASK(MSR_EE)
	isync
	sync			/* Flush any outstanding writes */
	mtmsr	r0
	isync
	blr
#endif	/* MACH_KDB */

#if MACH_KGDB
/* void call_kgdb_with_ctx(int type, int code, struct ppc_saved_state *ssp)
 *
 * Switch stacks to the kgdb stack, enter the debugger. On return,
 * switch back to the previous stack
 *
 * If the kgdb stack is not free, we allocate ourselves a frame below
 * the current kgdb frame. This should never occur in a perfect world.
 */

ENTRY(call_kgdb_with_ctx, TAG_NO_FRAME_USED)
	
	/* Switch off interrupts */

	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	/* Save the link register in the caller's space */
	
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	
	/* Find the gdb stack and move on to it */
		
	lwz	r9,	PP_GDBSTACKPTR(r2)
	cmpwi	r9,	0
	bne	0f

	/* The stack wasn't free - oops! recover old sp and allocate space */

	lwz	r9,	SS_R1(ARG2) /* r9 holds stack ptr of trapped */

	/* Make room for our saved state structure */
	subi	r9,	r9,	FM_REDZONE + SS_SIZE

0:
	/* Here, r9 points to a valid kgdb stack address with a
	 * saved state structure above it, which we only use for r1
	 */
	stw	r1,	SS_R1(r9)		/* Store old stack pointer */

	li	r0,	0
	stw	r0,	PP_GDBSTACKPTR(r2)	/* Mark gdb stack as busy */
	
	/* Move to new stack, making room for a dummy frame */
	subi	r1,	r9,	FM_SIZE
	stw	r0,	FM_BACKPTR(r1)

	/* Call debugger */
	
	bl	EXT(kgdb_trap)

	/* Ok, back from C.
	 */

	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0
	isync

	addi	r1,	r1,	FM_SIZE
	
	/* Is this the first frame on the stack? */

	lwz	r4,	PP_GDBSTACKPTR(r2)
	cmp	CR0,	r4,	r1
	bne	CR0,	0f
	
	/* We are the last frame, mark stack as free */

	stw	r1,	PP_GDBSTACKPTR(r2)	/* Mark gdb stack as free */

	/* Switch onto old stack and recover old lr before returning */
0:		
	lwz	r1,	SS_R1(r1)
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0

	blr
#endif /* MACH_KGDB */


/* The following routines are for C-support. They are usually
 * inlined into the C using the specifications in proc_reg.h,
 * but if optimisation is switched off, the inlining doesn't work
 */

ENTRY(get_got, TAG_NO_FRAME_USED)
	mr	ARG0,	r2
	blr
	
ENTRY(mflr, TAG_NO_FRAME_USED)
	mflr	ARG0
	blr

ENTRY(mfpvr, TAG_NO_FRAME_USED)
	mfpvr	ARG0
	blr

ENTRY(mtmsr, TAG_NO_FRAME_USED)
	mtmsr	ARG0
	isync
	blr

ENTRY(mfmsr, TAG_NO_FRAME_USED)
	mfmsr	ARG0
	blr

ENTRY(mtsrin, TAG_NO_FRAME_USED)
	isync
	mtsrin	ARG0,	ARG1
	isync
	blr

ENTRY(mfsrin, TAG_NO_FRAME_USED)
	mfsrin	ARG0,	ARG0
	blr

ENTRY(mtsdr1, TAG_NO_FRAME_USED)
	mtsdr1	ARG0
	blr

ENTRY(mtdar, TAG_NO_FRAME_USED)
	mtdar	ARG0
	blr

ENTRY(mfdar, TAG_NO_FRAME_USED)
	mfdar	ARG0
	blr

ENTRY(mtdec, TAG_NO_FRAME_USED)
	mtdec	ARG0
	blr

/* Decrementer frequency and realtime|timebase processor registers
 * are different between ppc601 and ppc603/4, we define them all.
 */

ENTRY(isync_mfdec, TAG_NO_FRAME_USED)
	isync
	mfdec	ARG0
	blr


ENTRY(mftb, TAG_NO_FRAME_USED)
	mftb	ARG0
	blr

ENTRY(mftbu, TAG_NO_FRAME_USED)
	mftbu	ARG0
	blr

ENTRY(mfrtcl, TAG_NO_FRAME_USED)
	mfspr	ARG0,	5
	blr

ENTRY(mfrtcu, TAG_NO_FRAME_USED)
	mfspr	ARG0,	4
	blr

ENTRY(tlbie, TAG_NO_FRAME_USED)
	tlbie	ARG0
	blr
