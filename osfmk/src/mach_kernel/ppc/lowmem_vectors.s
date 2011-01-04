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

/*
 * Low-memory exception vector code for PowerPC MACH
 *
 * These are the only routines that are ever run with
 * VM instruction translation switched off.
 *
 * The PowerPC is quite strange in that rather than having a set
 * of exception vectors, the exception handlers are installed
 * in well-known addresses in low memory. This code must be loaded
 * at ZERO in physical memory. The simplest way of doing this is
 * to load the kernel at zero, and specify this as the first file
 * on the linker command line.
 *
 * When this code is loaded into place, it is loaded at virtual
 * address KERNELBASE, which is mapped to zero (physical).
 *
 * This code handles all powerpc exceptions and is always entered
 * in supervisor mode with translation off. It saves the minimum
 * processor state before switching back on translation and
 * jumping to the approprate routine.
 *
 * Vectors from 0x100 to 0x3fff occupy 0x100 bytes each (64 instructions)
 *
 * We use some of this space to decide which stack to use, and where to
 * save the context etc, before	jumping to a generic handler.
 */

#include <assym.s>
#include <debug.h>
#include <cpus.h>
#include <db_machine_commands.h>
	
#include <mach_debug.h>
#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <ppc/exception.h>
#include <mach/ppc/vm_param.h>
#if	NCPUS > 1
#include <ppc/POWERMAC/mp/MPPlugIn.h>
#endif	/* NCPUS > 1 */
	
	.text

	.globl _ExceptionVectorsStart
	.type  _ExceptionVectorsStart,@function
_ExceptionVectorsStart:	/* Used if relocating the exception vectors */
baseR:			/* Used so we have more readable code */

/* 
 * System reset - call debugger
 */


	. = 0x100
.L_handler100:	

	mtsprg	3,	r3
	li	r3,	T_RESET
	b	.L_exception_entry

#if	DB_MACHINE_COMMANDS
/*
 *	This is the data used by the exception trace code
 */

	. = 0x180
.L_TraceData:

			.globl traceMask
			.type  traceMask,@object
traceMask:									/* Allowable trace types indexed by  vector >> 8 */
			.int	0xFFFFFFFF 				/* All enabled */
/*			.int	0xFBBFFFFF	*/			/* EXT and DEC disabled */

			.globl traceCurr
			.type  traceCurr,@object
traceCurr:	.int	traceTableBeg-_ExceptionVectorsStart	/* The next trace entry to use */

			.globl traceStart
			.type  traceStart,@object
traceStart:	.int	traceTableBeg-_ExceptionVectorsStart	/* Start of the trace table */

			.globl traceEnd
			.type  traceEnd,@object
traceEnd:	.int	traceTableEnd-_ExceptionVectorsStart	/* End (wrap point) of the trace */

debsave0:	.int	0
debsave1:	.int	0
debsave2:	.int	0
debsave3:	.int	0
debsave4:	.int	0
debsave5:	.int	0
debsave6:	.int	0
debsave7:	.int	0
debsave8:	.int	0
debsave9:	.int	0
debsave10:	.int	0

#endif	/* DB_MACHINE_COMMANDS */

/*
 * Machine check (physical bus error) - call debugger
 */

	. = 0x200
.L_handler200:

	mtsprg	3,	r3
	li	r3,	T_MACHINE_CHECK
	b	.L_exception_entry

/*
 * Data access - page fault, invalid memory rights for operation
 */

	. = 0x300
.L_handler300:

	mtsprg	3,	r3
	li	r3,	T_DATA_ACCESS
	b	.L_exception_entry

/*
 * Instruction access - as for data access
 */

	. = 0x400
.L_handler400:

	mtsprg	3,	r3
	li	r3,	T_INSTRUCTION_ACCESS
	b	.L_exception_entry

/*
 * External interrupt
 */

	. = 0x500
.L_handler500:

	mtsprg	3,	r3
	li	r3,	T_INTERRUPT
	b	.L_exception_entry

/*
 * Alignment - many reasons
 */

	. = 0x600
.L_handler600:
	
	mtsprg	3,	r3
	li	r3,	T_ALIGNMENT
	b	.L_exception_entry

/*
 * Program - floating point exception, illegal inst, priv inst, user trap
 */

	. = 0x700
.L_handler700:

	mtsprg	3,	r3
	li	r3,	T_PROGRAM
	b	.L_exception_entry

/*
 * Program - floating point disabled, illegal inst, priv inst, user trap
 */

	. = 0x800
.L_handler800:

	mtsprg	3,	r3
	li	r3,	T_FP_UNAVAILABLE
	b	.L_exception_entry


/*
 * Decrementer - DEC register has passed zero.
 */

	. = 0x900
.L_handler900:

	mtsprg	3,	r3
	li	r3,	T_DECREMENTER
	b	.L_exception_entry

/*
 * I/O controller interface error - MACH does not use this
 */

	. = 0xA00
.L_handlerA00:

	mtsprg	3,	r3
	li	r3,	T_IO_ERROR
	b	.L_exception_entry

/*
 * Reserved
 */

	. = 0xb00
.L_handlerB00:

	mtsprg	3,	r3
	li	r3,	T_RESERVED
	b	.L_exception_entry

/*
 * System call - generated by the sc instruction
 */

	. = 0xc00
.L_handlerC00:

	mtsprg	3,	r3
	li	r3,	T_SYSTEM_CALL
	b	.L_exception_entry

/*
 * Trace - generated by single stepping a 603/604
 */

	. = 0xd00
.L_handlerD00:

	mtsprg	3,	r3
	li	r3,	T_TRACE
	b	.L_exception_entry

/*
 * Floating point assist
 */

	. = 0xe00
.L_handlerE00:

	mtsprg	3,	r3
	li	r3,	T_FP_ASSIST
	b	.L_exception_entry

/*
 * Instruction translation miss - we inline this code.
 * Upon entry (done for us by the machine):
 *     srr0 :	 addr of instruction that missed
 *     srr1 :	 bits 0-3   = saved CR0
 *                    4     = lru way bit
 *                    16-31 = saved msr
 *     msr[tgpr] = 1  (so gpr0-3 become our temporary variables)
 *     imiss:	 ea that missed
 *     icmp :	 the compare value for the va that missed
 *     hash1:	 pointer to first hash pteg
 *     hash2:	 pointer to 2nd hash pteg
 *
 * Register usage:
 *     tmp0:	 saved counter
 *     tmp1:	 junk
 *     tmp2:	 pointer to pteg
 *     tmp3:	 current compare value
 *
 * This code is taken from the 603e User's Manual with
 * some bugfixes and minor improvements to save bytes and cycles
 */

	. = 0x1000
.L_handler1000:
	mfspr	tmp2,	hash1
	mfctr	tmp0				/* use tmp0 to save ctr */
	mfspr	tmp3,	icmp

.L_imiss_find_pte_in_pteg:
	li	tmp1,	8			/* count */
	subi	tmp2,	tmp2,	8		/* offset for lwzu */
	mtctr	tmp1				/* count... */
	
.L_imiss_pteg_loop:
	lwz	tmp1,	8(tmp2)			/* check pte0 for match... */
	addi	tmp2,	tmp2,	8
	cmpw	CR0,	tmp1,	tmp3
#if 0	
	bdnzf+	CR0,	.L_imiss_pteg_loop
#else	
	bc	CR0,2,	.L_imiss_pteg_loop
#endif	
	beq+	CR0,	.L_imiss_found_pte

	/* Not found in PTEG, we must scan 2nd then give up */

	andi.	tmp1,	tmp3,	MASK(PTE0_HASH_ID)
	bne-	.L_imiss_do_no_hash_exception		/* give up */

	mfspr	tmp2,	hash2
	ori	tmp3,	tmp3,	MASK(PTE0_HASH_ID)
	b	.L_imiss_find_pte_in_pteg

.L_imiss_found_pte:

	lwz	tmp1,	4(tmp2)				/* get pte1_t */
	andi.	tmp3,	tmp1,	MASK(PTE1_WIMG_GUARD)	/* Fault? */
	bne-	.L_imiss_do_prot_exception		/* Guarded - illegal */

	/* Ok, we've found what we need to, restore and rfi! */

	mtctr	tmp0					/* restore ctr */
	mfsrr1	tmp3
	mfspr	tmp0,	imiss
	mtcrf	0x80,	tmp3				/* Restore CR0 */
	mtspr	rpa,	tmp1				/* set the pte */
	ori	tmp1,	tmp1,	MASK(PTE1_REFERENCED)	/* set referenced */
	tlbli	tmp0
	sth	tmp1,	6(tmp2)

	rfi
	
.L_imiss_do_prot_exception:
	/* set up srr1 to indicate protection exception... */
	mfsrr1	tmp3
	andi.	tmp2,	tmp3,	0xffff
	addis	tmp2,	tmp2,	MASK(SRR1_TRANS_PROT) >> 16
	b	.L_imiss_do_exception
	
.L_imiss_do_no_hash_exception:
	/* clean up registers for protection exception... */
	mfsrr1	tmp3
	andi.	tmp2,	tmp3,	0xffff
	addis	tmp2,	tmp2,	MASK(SRR1_TRANS_HASH) >> 16
	
	/* And the entry into the usual instruction fault handler ... */
.L_imiss_do_exception:

	mtctr	tmp0					/* Restore ctr */
	mtsrr1	tmp2					/* Set up srr1 */
	mfmsr	tmp0					
	xoris	tmp0,	tmp0,	MASK(MSR_TGPR)>>16	/* no TGPR */
	mtcrf	0x80,	tmp3				/* Restore CR0 */
	mtmsr	tmp0					/* reset MSR[TGPR] */
	b	.L_handler400				/* Instr Access */
	
/*
 * Data load translation miss
 *
 * Upon entry (done for us by the machine):
 *     srr0 :	 addr of instruction that missed
 *     srr1 :	 bits 0-3   = saved CR0
 *                    4     = lru way bit
 *                    5     = 1 if store
 *                    16-31 = saved msr
 *     msr[tgpr] = 1  (so gpr0-3 become our temporary variables)
 *     dmiss:	 ea that missed
 *     dcmp :	 the compare value for the va that missed
 *     hash1:	 pointer to first hash pteg
 *     hash2:	 pointer to 2nd hash pteg
 *
 * Register usage:
 *     tmp0:	 saved counter
 *     tmp1:	 junk
 *     tmp2:	 pointer to pteg
 *     tmp3:	 current compare value
 *
 * This code is taken from the 603e User's Manual with
 * some bugfixes and minor improvements to save bytes and cycles
 */

	. = 0x1100
.L_handler1100:
	mfspr	tmp2,	hash1
	mfctr	tmp0				/* use tmp0 to save ctr */
	mfspr	tmp3,	dcmp

.L_dlmiss_find_pte_in_pteg:
	li	tmp1,	8			/* count */
	subi	tmp2,	tmp2,	8		/* offset for lwzu */
	mtctr	tmp1				/* count... */
	
.L_dlmiss_pteg_loop:
	lwz	tmp1,	8(tmp2)			/* check pte0 for match... */
	addi	tmp2,	tmp2,	8
	cmpw	CR0,	tmp1,	tmp3
#if 0 /* How to write this correctly? */	
	bdnzf+	CR0,	.L_dlmiss_pteg_loop
#else	
	bc	CR0,2,	.L_dlmiss_pteg_loop
#endif	
	beq+	CR0,	.L_dmiss_found_pte

	/* Not found in PTEG, we must scan 2nd then give up */

	andi.	tmp1,	tmp3,	MASK(PTE0_HASH_ID)	/* already at 2nd? */
	bne-	.L_dmiss_do_no_hash_exception		/* give up */

	mfspr	tmp2,	hash2
	ori	tmp3,	tmp3,	MASK(PTE0_HASH_ID)
	b	.L_dlmiss_find_pte_in_pteg

.L_dmiss_found_pte:

	lwz	tmp1,	4(tmp2)				/* get pte1_t */

	/* Ok, we've found what we need to, restore and rfi! */

	mtctr	tmp0					/* restore ctr */
	mfsrr1	tmp3
	mfspr	tmp0,	dmiss
	mtcrf	0x80,	tmp3				/* Restore CR0 */
	mtspr	rpa,	tmp1				/* set the pte */
	ori	tmp1,	tmp1,	MASK(PTE1_REFERENCED)	/* set referenced */
	tlbld	tmp0					/* load up tlb */
	sth	tmp1,	6(tmp2)				/* sth is faster? */

	rfi
	
	/* This code is shared with data store translation miss */
	
.L_dmiss_do_no_hash_exception:
	/* clean up registers for protection exception... */
	mfsrr1	tmp3
	/* prepare to set DSISR_WRITE_BIT correctly from srr1 info */
	rlwinm	tmp1,	tmp3,	9,	6,	6
	addis	tmp1,	tmp1,	MASK(DSISR_HASH) >> 16

	/* And the entry into the usual data fault handler ... */

	mtctr	tmp0					/* Restore ctr */
	andi.	tmp2,	tmp3,	0xffff			/* Clean up srr1 */
	mtsrr1	tmp2					/* Set srr1 */
	mtdsisr	tmp1
	mfspr	tmp2,	dmiss
	mtdar	tmp2
	mfmsr	tmp0
	xoris	tmp0,	tmp0,	MASK(MSR_TGPR)>>16	/* no TGPR */
	mtcrf	0x80,	tmp3				/* Restore CR0 */
	sync						/* Needed on some */
	mtmsr	tmp0					/* reset MSR[TGPR] */
	b	.L_handler300				/* Data Access */
	
/*
 * Data store translation miss (similar to data load)
 *
 * Upon entry (done for us by the machine):
 *     srr0 :	 addr of instruction that missed
 *     srr1 :	 bits 0-3   = saved CR0
 *                    4     = lru way bit
 *                    5     = 1 if store
 *                    16-31 = saved msr
 *     msr[tgpr] = 1  (so gpr0-3 become our temporary variables)
 *     dmiss:	 ea that missed
 *     dcmp :	 the compare value for the va that missed
 *     hash1:	 pointer to first hash pteg
 *     hash2:	 pointer to 2nd hash pteg
 *
 * Register usage:
 *     tmp0:	 saved counter
 *     tmp1:	 junk
 *     tmp2:	 pointer to pteg
 *     tmp3:	 current compare value
 *
 * This code is taken from the 603e User's Manual with
 * some bugfixes and minor improvements to save bytes and cycles
 */

	. = 0x1200
.L_handler1200:
	mfspr	tmp2,	hash1
	mfctr	tmp0				/* use tmp0 to save ctr */
	mfspr	tmp3,	dcmp

.L_dsmiss_find_pte_in_pteg:
	li	tmp1,	8			/* count */
	subi	tmp2,	tmp2,	8		/* offset for lwzu */
	mtctr	tmp1				/* count... */
	
.L_dsmiss_pteg_loop:
	lwz	tmp1,	8(tmp2)			/* check pte0 for match... */
	addi	tmp2,	tmp2,	8

		cmpw	CR0,	tmp1,	tmp3
#if 0 /* I don't know how to write this properly */	
	bdnzf+	CR0,	.L_dsmiss_pteg_loop
#else	
	bc	CR0,2,	.L_dsmiss_pteg_loop
#endif	
	beq+	CR0,	.L_dsmiss_found_pte

	/* Not found in PTEG, we must scan 2nd then give up */

	andi.	tmp1,	tmp3,	MASK(PTE0_HASH_ID)	/* already at 2nd? */
	bne-	.L_dmiss_do_no_hash_exception		/* give up */

	mfspr	tmp2,	hash2
	ori	tmp3,	tmp3,	MASK(PTE0_HASH_ID)
	b	.L_dsmiss_find_pte_in_pteg

.L_dsmiss_found_pte:

	lwz	tmp1,	4(tmp2)				/* get pte1_t */
	andi.	tmp3,	tmp1,	MASK(PTE1_CHANGED)	/* unchanged, check? */
	beq-	.L_dsmiss_check_prot			/* yes, check prot */

.L_dsmiss_resolved:
	/* Ok, we've found what we need to, restore and rfi! */

	mtctr	tmp0					/* restore ctr */
	mfsrr1	tmp3
	mfspr	tmp0,	dmiss
	mtcrf	0x80,	tmp3				/* Restore CR0 */
	mtspr	rpa,	tmp1				/* set the pte */
	tlbld	tmp0					/* load up tlb */
	rfi
	
.L_dsmiss_check_prot:
	/* PTE is unchanged, we must check that we can write */
	rlwinm.	tmp3,	tmp1,	30,	0,	1	/* check PP[1] */
	bge-	.L_dsmiss_check_prot_user_kern
	andi.	tmp3,	tmp1,	1			/* check PP[0] */
	beq+	.L_dsmiss_check_prot_ok
	
.L_dmiss_do_prot_exception:
	/* clean up registers for protection exception... */
	mfsrr1	tmp3
	/* prepare to set DSISR_WRITE_BIT correctly from srr1 info */
	rlwinm	tmp1,	tmp3,	9,	6,	6
	addis	tmp1,	tmp1,	MASK(DSISR_PROT) >> 16

	/* And the entry into the usual data fault handler ... */

	mtctr	tmp0					/* Restore ctr */
	andi.	tmp2,	tmp3,	0xffff			/* Clean up srr1 */
	mtsrr1	tmp2					/* Set srr1 */
	mtdsisr	tmp1
	mfspr	tmp2,	dmiss
	mtdar	tmp2
	mfmsr	tmp0
	xoris	tmp0,	tmp0,	MASK(MSR_TGPR)>>16	/* no TGPR */
	mtcrf	0x80,	tmp3				/* Restore CR0 */
	sync						/* Needed on some */
	mtmsr	tmp0					/* reset MSR[TGPR] */
	b	.L_handler300				/* Data Access */
	
/* NB - if we knew we were on a 603e we could test just the MSR_KEY bit */
.L_dsmiss_check_prot_user_kern:
	mfsrr1	tmp3
	andi.	tmp3,	tmp3,	MASK(MSR_PR)
	beq+	.L_dsmiss_check_prot_kern
	mfspr	tmp3,	dmiss				/* check user privs */
	mfsrin	tmp3,	tmp3				/* get excepting SR */
	andis.	tmp3,	tmp3,	0x2000			/* Test SR ku bit */
	beq+	.L_dsmiss_check_prot_ok
	b	.L_dmiss_do_prot_exception

.L_dsmiss_check_prot_kern:
	mfspr	tmp3,	dmiss				/* check kern privs */
	mfsrin	tmp3,	tmp3
	andis.	tmp3,	tmp3,	0x4000			/* Test SR Ks bit */
	bne-	.L_dmiss_do_prot_exception

.L_dsmiss_check_prot_ok:
	/* Ok, mark as referenced and changed before resolving the fault */
	ori	tmp1,	tmp1,	(MASK(PTE1_REFERENCED)|MASK(PTE1_CHANGED))
	sth	tmp1,	6(tmp2)
	b	.L_dsmiss_resolved
	
/*
 * Instruction address breakpoint
 */

	. = 0x1300
.L_handler1300:
	mtsprg	3,	r3
	li	r3,	T_INSTRUCTION_BKPT
	b	.L_exception_entry


/*
 * System management interrupt
 */

	. = 0x1400
.L_handler1400:
	mtsprg	3,	r3
	li	r3,	T_SYSTEM_MANAGEMENT
	b	.L_exception_entry


/*
 * There is now a large gap of reserved traps
 */

/*
 * Run mode/ trace exception - single stepping on 601 processors
 */

	. = 0x2000
.L_handler2000:

	mtsprg	3,	r3
	li	r3,	T_RUNMODE_TRACE
	b	.L_exception_entry

/*
 * .L_exception_entry(type)
 *
 * This is the common exception handling routine called by any
 * type of system exception.
 *
 * ENTRY:	via a system exception handler, thus interrupts off, VM off.
 *              r3 has been saved in sprg3 and now contains a number
 *              representing the exception's origins
 *
 * EXIT:	srr0 and srr1 saved in per_proc_info structure
 *              r3 (supplied) saved in per_proc_info structure
 *              cr            saved in per_proc_info structure
 *              original r1-3 saved in sprg1-3.
 *                 r1 - is scratch
 *                 r2 - byte offset into per_proc_info of our CPU (SMP only)
 *                 r3  -contains exception info as for entry
 *
 *              The exception's handler is entered with
 *              VM on, interrupts still switched off
 */

#define PRINT1H(XX, nn)		\
0:	lbz	r2,0(r1)	; \
	andi.	r2,r2,0x04	; \
	beq	00b		; \
	XX			; \
	srwi	r2,r2,nn	; \
	andi.	r2,r2,0x0F	; \
	cmpi	0,r2,0x0A	; \
	blt	1f		; \
	subi	r2,r2,0x0A	; \
	addi	r2,r2,'A'-'0'	; \
1:	addi	r2,r2,'0'	; \
	stb	r2,4(r1)	;
#define PRINT1H_(XX)		\
0:	lbz	r2,0(r1)	; \
	andi.	r2,r2,0x04	; \
	beq	00b		; \
	XX			; \
	andi.	r2,r2,0x0F	; \
	cmpi	0,r2,0x0A	; \
	blt	1f		; \
	subi	r2,r2,0x0A	; \
	addi	r2,r2,'A'-'0'	; \
1:	addi	r2,r2,'0'	; \
	stb	r2,4(r1)	;
#define PRINT8H(XX)		\
	PRINT1H(XX, 28)		\
	PRINT1H(XX, 24)		\
	PRINT1H(XX, 20)		\
	PRINT1H(XX, 16)		\
	PRINT1H(XX, 12)		\
	PRINT1H(XX,  8)		\
	PRINT1H(XX,  4)		\
	PRINT1H_(XX)
#define PRINT1C(XX)		\
0:	lbz	r2,0(r1)	; \
	andi.	r2,r2,0x04	; \
	beq	00b		; \
	li	r2,XX		; \
	stb	r2,4(r1)	;
	
	.data
	.align	ALIGN
	.type	EXT(exception_entry),@object
	.size	EXT(exception_entry), 4
	.globl	EXT(exception_entry)
EXT(exception_entry):
	.long	.L_exception_entry-_ExceptionVectorsStart /* phys addr of fn */

	.text
		

.L_exception_entry:
	/* r3 has been saved in sprg3 and now contains exception info */
	mtsprg	2,	r2							/* Setup some work registers */
	mtsprg	1,	r1

#if	NCPUS == 1

	/* Original code which just does the minimal amount of things */
	/* Save SRR0 and SRR1 plus cr and r3 into PER_PROC structure */
	
	mfsprg	r2,	0		/* sprg0 = phys addr of per-proc */
	stw	r3,	PP_SAVE_EXCEPTION_TYPE(r2)
	mfsrr0	r1
	stw	r1,	PP_SAVE_SRR0(r2)
	mfsrr1	r1
	stw	r1,	PP_SAVE_SRR1(r2)
	mfcr	r1
	stw	r1,	PP_SAVE_CR(r2)
	mfctr	r1
	stw	r1,	PP_SAVE_CTR(r2)

#else	/* NCPUS == 1 */
	
/*
 *
 *	Here we will save off a mess of registers, the special ones and R0-R12.  We use the DCBZ
 *	instruction to clear and allcoate a line in the cache.  This way we won't take any cache
 *	misses, so these stores won't take all that long.
 *
 *	One thing though, this is not standard throughout the rest of Mach, so we need to put the
 *	registers back to what was assumed.  Really, though we should go downstream and make some
 *	changes.  Ahhh, for the want of some time...
 *
 *	All this extra register saving was put in for SMP and to facilitate an exception trace.
 *
 */

/*
 *			Warning: OBAR code follows
 *					 (Optimized Beyond All Recognition)
 */

		
			mfsprg	r2,	0							/* sprg0 = phys addr of per-proc */

			li		r1,3*32							/* Displacement to fourth hunk of register saves */
		
			stw		r3,	PP_SAVE_EXCEPTION_TYPE(r2)	/* Save the exception type code */
			
			dcbz	r1,r2							/* Clear third part of save area for performance sake */
			li		r1,2*32							/* Displacement to third hunk of register saves */
			stw		r0,PP_SAVE_R0(r2)				/* Save off R0 */
			mr		r0,r3							/* Save the interrupt code */
			mfsrr0	r3								/* Get SRR0 */
			dcbz	r1,r2							/* Clear second part */
			lis		r1,KERNEL_SEG_REG0_VALUE@h		/* Get the high half of the kernel SR0 value */
			stw		r4,PP_SAVE_R4(r2)				/* Save off R4 */
			ori		r1,r1,KERNEL_SEG_REG0_VALUE@l	/* Slam in the low half of SR0 */
			mfsr	r4,0							/* Get the interrupt time SR0 */
			stw		r5,PP_SAVE_R5(r2)				/* Save off R5 */
			mtsr	0,r1							/* Set SR0 */
			stw		r4,PP_SAVE_SR0(r2)				/* Save the interrupt time SR0 */
			mfsprg	r4,1							/* Get back register 1 */
			stw		r6,PP_SAVE_R6(r2)				/* Save off R6 */
			mfsprg	r5,2							/* Get back R2 */
			li		r1,4*32							/* Displacement to fifth line */
			stw		r7,PP_SAVE_R7(r2)				/* Save off R7 */
			dcbz	r1,r2							/* Clear fourth part */
			stw		r4,PP_SAVE_R1(r2)				/* Save off R1 */
			mfsprg	r7,3							/* Get back R3 */
			stw		r5,PP_SAVE_R2(r2)				/* Save off R2 */
			mfsrr1	r1								/* Get SRR1 */	
			stw		r7,PP_SAVE_R3(r2)				/* Save off R3 */
			stw		r3,	PP_SAVE_SRR0(r2)			/* Save SRR0 */
			mfcr	r4								/* Get the CR */
			stw		r1,	PP_SAVE_SRR1(r2)			/* Save SRR1 */
			mflr	r3								/* Get the LR */
			stw		r4,PP_SAVE_CR(r2)				/* Save off the CR */
			mfctr	r1								/* Get the CTR */
			stw		r3,PP_SAVE_LR(r2)				/* Save off the LR */
			mfdsisr	r4								/* Get the DSISR */
			stw		r1,PP_SAVE_CTR(r2)				/* Save off the CTR */
			mfdar	r3								/* Get the DAR */
			stw		r4,PP_SAVE_DSISR(r2)			/* Save off the DSISR */
			mfxer	r1								/* Get the XER */
			stw		r8,PP_SAVE_R8(r2)				/* Save off R8 */
			lis		r4,MPspec@h						/* Get the MP control block */
			stw		r3,PP_SAVE_DAR(r2)				/* Save the DAR */
			ori		r4,r4,MPspec@l					/* Get the bottom half of the MP control block */
			stw		r1,PP_SAVE_XER(r2)				/* Save off the XER */
			lwz		r4,MPSSIGPhandler(r4)			/* Get the address of the SIGP interrupt filter */
			mr		r7,r0							/* Move the interrupt code */
			stw		r9,PP_SAVE_R9(r2)				/* Save off R9 */
			cmplwi	cr0,r7,T_INTERRUPT				/* Do we have an external interrupt? */
			stw		r10,PP_SAVE_R10(r2)				/* Save off R10 */
			cmplwi	cr1,r4,0						/* Check if signal filter is initialized yet */
			stw		r11,PP_SAVE_R11(r2)				/* Save off R11 */
			lhz		r3,PP_CPU_FLAGS(r2)				/* Get the CPU flags */
			stw		r12,PP_SAVE_R12(r2)				/* Save off R12 */

			bne+	chekIfTrc						/* This is not an external 'rupt, can't be a signal... */
			
			andi.	r3,r3,SIGPactive@l				/* See if this processor has started up */
			beq-	cr1,chekIfTrc					/* We don't have a filter yet... */
			beq-	chekIfTrc						/* This processor hasn't started filtering yet... */
			mtlr	r4								/* Load up filter address */
			
			blrl									/* Filter the interrupt */
/*
 *			Remember that the filter would have gone translation on so it could
 *			read some hardware, so make sure the DSISR and DAR are restored.
 *			But, and it's a big but: Bertha Butts--there had better never, never, NEVER, be a DSI, 'cause 
 *			the per processor block could be trashed.
 */
		
			mfsprg	r2,0							/* Make sure we have the per processor block */			
			cmplwi	cr0,r3,kMPIOInterruptPending	/* See what the filter says */
			lwz		r5,PP_SAVE_LR(r2)				/* Get back the LR */	
			li		r7,T_INTERRUPT					/* Assume we have a regular external 'rupt */
			lwz		r12,PP_SAVE_R12(r2)				/* Get back R12 */
			beq+	modRupt							/* Yeah, we figured it would be... */
			li		r7,T_SIGP						/* Assume we had a signal processor interrupt */
			bgt+	modRupt							/* Yeah, at this point we would assume so... */
			li		r7,T_IN_VAIN					/* Nothing there actually, so eat it */
			
modRupt:	lwz		r9,PP_SAVE_DAR(r2)				/* Get back the damn DAR */
			lwz		r11,PP_SAVE_R11(r2)				/* Get back R11 */
			stw		r7,PP_SAVE_EXCEPTION_TYPE(r2)	/* Set that it was either in vain or a SIGP */
			mtdar	r9								/* Restore the DAR 'cause it's expected later */
			lwz		r8,PP_SAVE_DSISR(r2)			/* Get back the DSISR */
			mtlr	r5								/* Restore the LR to entry conditions */
			lwz		r10,PP_SAVE_R10(r2)				/* Restore R10 */
			mtdsisr	r8								/* Restore the DSISR */
			lwz		r9,PP_SAVE_R9(r2)				/* Restore R9 */
			lwz		r8,PP_SAVE_R8(r2)				/* Restore R8 */


/*
 *			Note that some of the trace flags do double duty; there are more than 32, so shift will wrap,
 *			e.g., T_IN_VAIN is also T_RUNMODE_TRACE, T_RESET is T_SIGP.
 */

chekIfTrc:	rlwinm	r0,r7,30,0,31					/* Save 'rupt code shifted right 2 */
			lwz		r3,traceMask-_ExceptionVectorsStart(r0)	/* Get the trace mask */
			rlwnm.	r0,r3,r0,0,0					/* Set CR0_EQ to 0 if tracing allowed */
			lwz		r0,PP_SAVE_R0(r2)				/* Restore register 0 */

/*
 *			Let's trace this exception.  We'll make sure that no other processors
 *			steal our entry.  We'll slap in a processor ID, a timestamp, type of exception,
 *			and a whole flock of registers.
 *
 *			We need to fix this 'cause we spend about a zillion cycles and a zillion
 *			cache accesses to build the trace entry.  We should weave this code into the
 *			register save stuff done above.  It would cut the whole thing in half.
 *			But, I don't wanna...  Yet...
 *
 *			So there...
 */
 
			beq-	cr0,skipTrace					/* Don't want to trace this kind... */
			
			li		r6,traceCurr-_ExceptionVectorsStart		/* Point to the trace current */
			lwz		r4,traceEnd-_ExceptionVectorsStart(r0)	/* Get the end pointer */

 getTrcEnt:	lwarx	r3,0,r6							/* Get the current entry */
			addi	r5,r3,LTR_size					/* Point to the next slot */
			cmplw	r4,r5							/* Are we at the last one? */
			bne+	tryTrcEnt						/* Nope... */
 			lwz		r5,traceStart-_ExceptionVectorsStart(r0)	/* Wrap to the first one */

tryTrcEnt:	stwcx.	r5,0,r6							/* See if someone else got this one */
			bne-	getTrcEnt						/* Yeah, damn... */

			dcbz	0,r3							/* Clear the first part first */
			li		r1,32							/* Second line of entry */

getTB:		mftbu	r4								/* Get the upper timebase */
			mftb	r5								/* Get the lower timebase */
			mftbu	r6								/* Get the upper one again  */
			cmplw	r4,r6							/* Did the top tick? */
			bne-	getTB							/* Yeah, need to get it again... */
		
			dcbz	r1,r3							/* Zap the second half */
			
			stw		r4,LTR_timeHi(r3)				/* Set the upper part of TB */
			mfspr	r1,pir							/* Get the processor address */
			stw		r5,LTR_timeLo(r3)				/* Set the lower part of TB */
			rlwinm	r1,r1,0,27,31					/* Cut the crap */
			mfsprg	r4,1							/* Get 'rupt time R1 */
			sth		r1,LTR_cpu(r3)					/* Stash the cpu address */
			stw		r0,LTR_r0(r3)					/* Save off register 0 */			
			mfsprg	r5,2							/* Get 'rupt time R2 */
			stw		r4,LTR_r1(r3)					/* Save R1 value */
			mfsprg	r1,3							/* Get back 'rupt time R3 */
			stw		r5,LTR_r2(r3)					/* Save R2 */
			stw		r1,LTR_r3(r3)					/* Save R3 */
			
			lwz		r1,PP_SAVE_CR(r2)				/* Get the CR */
			lwz		r5,PP_SAVE_SRR0(r2)				/* Get the SSR0 */
			lwz		r6,PP_SAVE_SRR1(r2)				/* Get the SRR1 */
			stw		r1,LTR_cr(r3)					/* Save the CR */
			stw		r5,LTR_srr0(r3)					/* Save the SSR0 */
			stw		r6,LTR_srr1(r3)					/* Save the SRR1 */

			lwz		r4,PP_SAVE_DAR(r2)				/* Get the DAR */
			lwz		r5,PP_SAVE_DSISR(r2)			/* Get the DSISR */
			mflr	r6								/* Get the LR */
			stw		r4,LTR_dar(r3)					/* Save the CR */
			mfctr	r4								/* Get the CTR */
			stw		r5,LTR_dsisr(r3)				/* Save the SSR0 */
			stw		r6,LTR_lr(r3)					/* Save the LR */
			lwz		r5,PP_SAVE_R5(r2)				/* Get register 5 back */
			stw		r4,LTR_ctr(r3)					/* Save off the CTR */
			sth		r7,LTR_excpt(r3)				/* Save the exception type */
			lwz		r4,PP_SAVE_R4(r2)				/* Restore R4 */
			stw		r5,LTR_r5(r3)					/* Save this one too */
			stw		r4,LTR_r4(r3)					/* Save the register */
			b		skipLoads						/* Skip some unneeded loads... */

skipTrace:	lwz		r4,PP_SAVE_R4(r2)				/* Restore R4 */
			lwz		r5,PP_SAVE_R5(r2)				/* Restore R5 */

skipLoads:	mtcrf	0x80,r0							/* Set our CR0 to the high nybble of the request code */
			rlwinm	r6,r0,1,0,31					/* Move sign bit to the end */
			cmplwi	cr1,r7,T_SYSTEM_CALL			/* Did we get a system call? */
			crandc	cr0_lt,cr0_lt,cr0_gt			/* See if we have R0 equal to 0b10xx...x */
			cmplwi	cr3,r7,T_IN_VAIN				/* Was this all in vain? All for nothing? */
			cmplwi	cr2,r6,1						/* See if original R0 had the CutTrace request code in it */
			cmplwi	cr4,r7,T_SIGP					/* Indicate if we had a SIGP 'rupt */
			
			beq-	cr3,forNaught					/* Interrupt was all for nothing... */
			bne+	cr1,noCutT						/* Not a system call... */
			bnl+	cr0,noCutT						/* R0 not 0b10xxx...x, can't be any kind of magical system call... */
			beq+	cr2,isCutTrace					/* This is a CutTrace system call */
			
			lis		r1,FirmwareCall@h				/* Top half of firmware call handler */
			lis		r3,KERNELBASE_TEXT_OFFSET@h		/* Top half of virtual-to-physical conversion */
			ori		r1,r1,FirmwareCall@l			/* Bottom half of it */
			ori		r3,r3,KERNELBASE_TEXT_OFFSET@l	/* Low half of virtual-to-physical conversion */
			sub		r1,r1,r3						/* Get the physical address of the firmware call handler routine */
			lwz		r3,PP_SAVE_R3(r2)				/* Restore the first parameter, the rest are ok already */
			mtlr	r1								/* Get it in the link register */
			blrl									/* Call the handler */

/*			We will return here only if the firmware can't handle the call.  The FW needs to restore any registers */
/*			it uses. CR, LR, R1, R2, R6, R7 and R3, forget about 'em, we'll handle them. */

			mfsprg	r2,0							/* Restore the per_processor area */
			lwz		r7,PP_SAVE_LR(r2)				/* Restore the entry LR */
			lwz		r1,PP_SAVE_CR(r2)				/* Restore the entry CR */
			mtlr	r7								/* Move the old LR back */
			lwz		r3,PP_SAVE_EXCEPTION_TYPE(r2)	/* Get back the exception type */
			b		noSIGP							/* Go to the normal system call handler */
			

/*			Note that we haven't turned on translation, so SRR0 and SRR1 shouldn't have been able to change */
			
isCutTrace:	li		r7,-32768						/* Get a 0x8000 for the exception code */
			sth		r7,LTR_excpt(r3)				/* Modify the exception type to a CutTrace */

forNaught:	lwz		r3,PP_SAVE_CR(r2)				/* Get the CR back */
			lwz		r6,PP_SAVE_R6(r2)				/* Restore R6 */
			lwz		r7,PP_SAVE_SR0(r2)				/* Get the interrupt time SR0 */
			mtcr	r3								/* Restore the CR */
			mfsprg	r1,1							/* Restore the original R1 */
			mtsr	0,r7
			mfsprg	r3,3							/* Restore original R3 */
			lwz		r7,PP_SAVE_R7(r2)				/* Restore R7 */
			mfsprg	r2,2							/* Restore original R2 */
			
			rfi										/* Return to the guy what called the tracer */
			
			.long	0								/* These are for the ancient 601ism */
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			

/*			We are here 'cause we didn't have a CutTrace system call */

noCutT:		mr		r3,r7							/* Get the exception type back */		
			bne+	cr4,noSIGP						/* Skip away if we didn't get a SIGP... */
		
			lis		r6,MPsignalFW@h					/* Top half of SIGP handler */
			lis		r7,KERNELBASE_TEXT_OFFSET@h		/* Top half of virtual-to-physical conversion */
			ori		r6,r6,MPsignalFW@l				/* Bottom half of it */
			ori		r7,r7,KERNELBASE_TEXT_OFFSET@l	/* Low half of virtual-to-physical conversion */
			sub		r6,r6,r7						/* Get the physical address of the SIGP handler routine */
			mtlr	r6								/* Get it in the link register */
			
			blrl									/* Call the handler - we'll only come back if this is an AST,  */
													/* 'cause FW can't handle that */
			cmplwi	cr0,r3,T_IN_VAIN				/* Did the signal handler eat the signal? */
		
			lwz		r7,PP_SAVE_LR(r2)				/* Restore the entry LR */
			lwz		r1,PP_SAVE_CR(r2)				/* Restore the entry CR */
			mtlr	r7								/* Restore the LR */

			beq		cr0,forNaught					/* Bail now if the signal handler processed the signal... */

noSIGP:		lwz		r6,PP_SAVE_R6(r2)				/* Restore R6 */
			lwz		r7,PP_SAVE_R7(r2)				/* Restore original R7 */
			mtcr	r1								/* Restore original CR */
#endif	/* NCPUS > 1 */
		
	/* jump into main handler code switching on VM at the same time */

	/* We assume kernel data is mapped contiguously in physical
	 * memory, otherwise we'd need to switch on (at least) virtual data.
	 */
	lis		r1,	(KERNEL_SEG_REG0_VALUE >> 16)		/* Get the top of the SR1 value */
#if	NCPUS == 1
	mtsr	0,	r1
#else	/* NCPUS == 1 */
	/* sr0 was already initialised in all the MP code above */
#endif	/* NCPUS == 1 */
	ori	r1,	r1,	1 	/* Set the bottom part up for SR1 */
	mtsr	1,	r1		/* Slam it on in */
	addi	r1,	r1,	1	/* Ditto for SR2 */
	mtsr	2,	r1
	
	lwz		r1,	PP_PHYS_EXCEPTION_HANDLERS(r2)
	lwzx	r1,	r1,	r3

#define BCTR_CODE_ENABLED 0
#if	BCTR_CODE_ENABLED
	mtctr	r1
	
	lwz	r2,	PP_VIRT_PER_PROC(r2)
	
	/* Switch on virtual addressing, I+D */
	
	li	r1,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r1
	isync
	bctr		/* into the exception handler with VM on */
#else	/* BCTR_CODE_ENABLED */
	mtsrr0	r1
	
	
	li	r1,	MSR_SUPERVISOR_INT_OFF
	lwz	r2,	PP_VIRT_PER_PROC(r2)

	mtsrr1	r1

	rfi		/* into the exception handler with VM on */

	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
#endif	/* BCTR_CODE_ENABLED */
	
/*
 * exception_exit(sr0,srr0,srr1)
 *
 * This is the trampoline code used when exiting into a foreign
 * address space.
 *
 * NMGS TODO - can't we skip exception_exit by relying on translations
 * NMGS TODO   even after we've loaded sr0? Docs aren't clear. Using
 * NMGS TODO   1-1 kernel text mapping would definately avoid this need.
 *
 * ENTRY :	 entry via rfi, MSR = MSR_VM_OFF
 *               r1-3 saved in sprg1-3
 * 		 r1 = user's sr0 - used to construct sr1 too
 *               r2 = user's srr0 (instruction pointer)
 *               r3 = user's srr1 (msr)
 *
 * EXIT :	 this routine restores the users' space and rfis.
 */
	
	.data
	.align	ALIGN
	.type	EXT(exception_exit),@object
	.size	EXT(exception_exit), 4
	.globl	EXT(exception_exit)
EXT(exception_exit):
	.long	exception_exit_fn-_ExceptionVectorsStart /* phys addr of fn */


	.text	
	
exception_exit_fn:

	mtsrr0	r2
	mtsrr1	r3

	mtsr	0,	r1		/* Restore user space SR0 */
	addi	r1,	r1,	1
	mtsr	1,	r1		/* Restore user space SR1 */
	addi	r1,	r1,	1
	mtsr	2,	r1		/* Restore user space SR2 */
			
	mfsprg	r1,	1
	mfsprg	r2,	2
	mfsprg	r3,	3
	rfi
	
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0

#if	DB_MACHINE_COMMANDS

/*
 *		Start of the trace table
 */
 
 	.align	12						/* Align to 4k boundary */
	
traceTableBeg:						/* Start of trace table */
/*	.fill	2048,4,0				   Make an 8k trace table for now */
	.fill	13760,4,0				/* Make an .trace table for now */
traceTableEnd:						/* End of trace table */

#endif	/* DB_MACHINE_COMMANDS */
	
	.globl _ExceptionVectorsEnd
	.type  _ExceptionVectorsEnd,@function
_ExceptionVectorsEnd:	/* Used if relocating the exception vectors */

	.data
	.align	ALIGN
	.type	EXT(exception_end),@object
	.size	EXT(exception_end), 4
	.globl	EXT(exception_end)
EXT(exception_end):
	.long	_ExceptionVectorsEnd - _ExceptionVectorsStart /* phys fn */


