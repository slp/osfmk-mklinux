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
#include <mach_kdb.h>
#include <mach_kgdb.h>

#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <assym.s>
	
/*
 * Interrupt and bootup stack for initial processor
 */

	.file	"start.s"
	
	.section ".data"
		/* Align on page boundry */
	.align  PPC_PGSHIFT
		/* Red zone for interrupt stack, one page (will be unmapped)*/
	.set	., .+PPC_PGBYTES
		/* intstack itself */
	.type 	EXT(intstack),@object
	.size	EXT(intstack),INTSTACK_SIZE*NCPUS
        .globl  EXT(intstack)
EXT(intstack):
	.set	., .+INTSTACK_SIZE*NCPUS
	.align	ALIGN
	.type 	EXT(intstack_top_ss),@object
	.size	EXT(intstack_top_ss),4
        .globl  EXT(intstack_top_ss)
	/* intstack_top_ss points to the topmost saved state structure
	 * in the intstack */
EXT(intstack_top_ss):
	.long	intstack+INTSTACK_SIZE-SS_SIZE
	
	/* KGDB stack - used by the debugger if present */
#if MACH_KGDB
	.type 	EXT(gdbstack),@object
	.size	EXT(gdbstack),KERNEL_STACK_SIZE*NCPUS
        .globl  EXT(gdbstack)
EXT(gdbstack):
	.set	., .+KERNEL_STACK_SIZE*NCPUS
	.align	ALIGN
	.type 	EXT(gdbstack_top_ss),@object
	.size	EXT(gdbstack_top_ss),4
        .globl  EXT(gdbstack_top_ss)
	/* gdbstack_top_ss points to the topmost saved state structure
	 * in the gdbstack */
EXT(gdbstack_top_ss):
	.long	gdbstack+KERNEL_STACK_SIZE-SS_SIZE
	.type 	EXT(gdbstackptr),@object
	.size	EXT(gdbstackptr),4
        .globl  EXT(gdbstackptr)
EXT(gdbstackptr):
	.long	gdbstack+KERNEL_STACK_SIZE-SS_SIZE
#endif /* MACH_KGDB */

#if	MACH_KDB
/*
 * Kernel debugger stack for each processor.
 */
	.type	EXT(db_stack_store),@object
	.size	EXT(db_stack_store),INTSTACK_SIZE*NCPUS
	.globl	EXT(db_stack_store)
EXT(db_stack_store):
	.set	.,.+(INTSTACK_SIZE*NCPUS)
	.align	ALIGN
#endif	/* MACH_KDB */

/*
 * All CPUs start here.
 *
 * This code is called from the MachOS Loader, with a certain amount
 * of machine state already set up:
 *
 *    Address translation is switched ON, with:
 *        I+D BAT0 mapping Physical 0-8M 1-1
 *	
 *         and I/O correctly memory mapped via a BAT or SR,
 *	   the screen might not yet be mapped.
 *
 *    We're running on a temporary stack.
 *
 *    Various arguments are passed via a table:
 *          ARG0 = contiguous physical memory size from 0
 *          ARG1 = pointer to other startup parameters
 */
	.text
	
ENTRY(_start,TAG_NO_FRAME_USED)

	/* Map in the first 8Mb, 32Mb on non-601 */

	mfpvr	r10
	lis	r9,0
	rlwinm  r10,r10,16,16,31          /* Just look for 1 (601)*/
	cmpi	0,r10,1
	bne     1f

	/* setup for 601 */
	
		/* Disable broadcast of tlbie instructions */
	mfspr	r7,	hid1
	ori	r7,	r7,	MASK(HID1_TL)
	mtspr	hid1,	r7
	
		/* use BAT0 to map bottom 8M */
	li	r7,	2		/* BLPI=0,WIM=0,Ks=Ku=0,PP=2 */
	li	r8,	0x7f		/* BPN=0,V=1,BSM=8M */

	b       2f
	
		/* Setup for 603/604 and others - [I/D]BAT0 maps bottom 32M */
1:	li	r7,	(0xff<<2+0x2)   /* BEPI=0,BL=32M,Vs=1 */
	li      r8,	((2<<3)|2)	/* BRPN=0,WIMG=M,PP=2 */
	li	r0,	0
	sync
	isync
	mtdbatu 0,	r7
	mtdbatl 0,	r8
	mtdbatu 1,	r9
	mtdbatl 1,	r9
	mtdbatu 2,	r9
	mtdbatl 2,	r9
	mtdbatu 3,	r9
	mtdbatl 3,	r9

2:
	li	r0,	0
	isync
	sync
	isync
	mtibatu 0,	r7
	mtibatl 0,	r8
	mtibatu 1,	r9
	mtibatl 1,	r9
	mtibatu 2,	r9
	mtibatl 2,	r9
	mtibatu 3,	r9
	mtibatl 3,	r9
	sync
	isync


/* 			Make sure our MSR is valid */

	li	r0,	MSR_SUPERVISOR_INT_OFF		/* Make sure we don't have DR enabled */
	mtmsr	r0		/* Set the MSR */
	isync			/* Wait for it to finish */
			
	/* move onto interrupt stack to release stack in high mem */

	/* Load address of first thing in the various BSSs (not just bss)
	 * We assume that this starts right after the data ends
	 */
	lis	r10,	_edata@ha
	addi	r10,	r10,	_edata@l

	/* load  end of data */
	lis	r9,	_end@ha
	addi	r9, r9,	_end@l

	/* Get total byte count */
	subf	r9,	r10,	r9

	/* Convert byte count into words */
	rlwinm.	r9,	r9,30,2,31
	li	r0,0
	mtctr	r9
	addi	r10,r10,-4

1:
	stw	r0,4(r10)
	addi	r10,	r10,4
	bdnz	1b

	/* initialise */

	mr	r31,	ARG0		/* Save away arguments */
	mr	r30,	ARG1

	addis	r29,	0,	intstack_top_ss@ha
	addi	r29,	r29,	intstack_top_ss@l
	lwz	r29,	0(r29)

	li	r28,	0
	stw	r28,	FM_BACKPTR(r29) /* store a null frame backpointer */

	/* move onto new stack */
	
	mr	r1,	r29

	bl	EXT(go)

	mr	ARG0,	r31		/* Recover arguments */
	mr	ARG1,	r30

	/* Pass through ARG0, ARG1 */
	
	bl	EXT(machine_startup)

	/* Should never return */

	BREAKPOINT_TRAP
