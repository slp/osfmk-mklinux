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
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
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

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: locore.s 1.62 94/12/15$
 */

#include <mach_rt.h>	
#include <machine/asm.h>
#include <machine/locore.h>
#include <assym.s>
#include <mach/machine/vm_param.h>
#include <hp_pa/pte.h>
#include <machine/pmap.h>	
#include <machine/psw.h>
#include <machine/trap.h>
#include <machine/break.h>
#include <machine/iomod.h>
#include <machine/rpb.h>
#include <machine/pdc.h>
#include <machine/spl.h>
#include <etap.h>
#include <kgdb.h>

	.space  $TEXT$
#ifdef __ELF__
	.subspa	$CODE$
#else /* __ELF__ */
	.subspa $FIRST$
#endif /* __ELF__ */
	.export	start_text,entry
start_text	
	
#ifndef __ELF__	
	.subspa $UNWIND_START$,QUAD=0,ALIGN=8,ACCESS=0x2c,SORT=56
	.export $UNWIND_START,data
$UNWIND_START

        .subspa $UNWIND_END$,QUAD=0,ALIGN=8,ACCESS=0x2c,SORT=73
        .export $UNWIND_END,data
$UNWIND_END

        .subspa $RECOVER_START$,QUAD=0,ALIGN=4,ACCESS=0x2c,SORT=74
        .export $RECOVER_START,data
$RECOVER_START

        .subspa $RECOVER$MILLICODE$,QUAD=0,ALIGN=4,ACCESS=0x2c,SORT=78
        .subspa $RECOVER_END$,QUAD=0,ALIGN=4,ACCESS=0x2c,SORT=88
        .export $RECOVER_END,data
$RECOVER_END
#endif /* __ELF__ */
	
/*
 * Declare data sections
 */
	.space  $PRIVATE$
	.subspa $DATA$

/*
 * allocate the interrupt stack and a page after it for a red zone...
 */
	.align  HP700_PGBYTES
	.export intstack_base,data
intstack_base

	.blockz INTSTACK_SIZE

	.export intstack_top,data
intstack_top

	/* 
	 * interrupt stack red zone
	 */
	.blockz HP700_PGBYTES
	.align  HP700_PGBYTES

dumpstk
recoverstack
	.export panic_stack,data
panic_stack
	.blockz 8192
	.align  HP700_PGBYTES

#ifdef	TLBSTATS
	.export	dtlb_statbase,data
	.export	itlb_statbase,data
	.export	tlbd_statbase,data
	.align	64
dtlb_statbase
	.word	0	/* misses  	+00 */
	.word	0	/* io		+04 */
	.word	0	/* Miss Time 	+08 */
	.word	0	/* Miss Faults	+12 */
	.word	0	/* Miss Insts. 	+16 */
	.word	0	/* Cache Hits	+20 */
	.word	0	/* tmp 		+24 */
	.word	0	/* tmp 		+28 */
itlb_statbase
	.word	0	/* misses  	+00 */
	.word	0	/* gateway	+04 */
	.word	0	/* Miss Time 	+08 */
	.word	0	/* Miss Faults	+12 */
	.word	0	/* Miss Insts. 	+16 */
	.word	0	/* Cache Hits	+20 */
	.word	0	/* tmp 		+24 */
	.word	0	/* tmp 		+28 */
tlbd_statbase
	.word	0	/* dirty  	+00 */
	.word	0	/* flushes	+04 */
	.word	0	/* Miss Time 	+08 */
	.word	0	/* Miss Faults	+12 */
	.word	0	/* Miss Insts. 	+16 */
	.word	0	/* tmp 		+20 */
	.word	0	/* tmp 		+24 */
	.word	0	/* tmp 		+28 */
#endif
mchkstr
	.stringz "mchk"
tocstr
	.stringz "forced crash"

#ifdef	GPROF
	/*
	 * We want these on 1 cache line to make the interrupt handler
	 * as fast as possible.
	 */
	.align	32
	.export profiling,data
	.export s_lowpc,data
	.export kcount,data
	.export s_textsize,data
profiling
	.word	PROFILING_OFF
s_lowpc
	.word	0
kcount
	.word	0
s_textsize
	.word	0
#endif

/*
 * start of kernel data
 */
	.export start_data,data
start_data

/*
 * Other data items that we need
 */
	.import dcache_stride,data
	.import icache_stride,data
	.import io_end,data
	.import istackptr,data
	.import tmp_saved_state,data
	.import fpu_zero,data
	.import	boothowto,data
	.import	bootdev,data
	.import cur_pcb,data

        .subspa $CODE$
        .export start,entry
start

/*
 * This is the starting location for the kernel
 */

/*
 *
 * Initialization
 *
 * We RFI here from the boot program.  The PSW Q & I bits are
 * set (collect interrupt state & allow external interrupts).
 */
	/*
	 * disable interrupts and turn off all bits in the psw so that
	 * we start in a known state.
	 */
	rsm	RESET_PSW,r0

	/*
	 * to keep the spl() routines consistent we need to put the correct
	 * spl level into eiem
	 */
	ldi	SPLHIGH,r1
	mtctl	r1,eiem

	/*
	 * set up the dp pointer so that we can do quick references off of it
	 */
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	/*
	 * establish an interrupt stack
	 */
	ldil	L%intstack_base,sp
	ldo	R%intstack_base(sp),sp

	/*
	 * clear intstackptr to indicate that we are on the interrupt stack
	 */
	ldil	L%istackptr,t1
	stw	r0,R%istackptr(t1)

#if KGDB	
	/*
	 * clear GCC frame pointer to avoid back-tracing off the end
	 */
	copy	r0,r3
#endif
	
	/*
	 * We need to set the Q bit so that we can take TLB misses after we
	 * turn on virtual memory.
	 */
	mtctl	r0,pcsq
	mtctl	r0,pcsq
	ldil	L%$qisnowon,t1
	ldo	R%$qisnowon(t1),t1
	mtctl	t1,pcoq	
	ldo	4(t1),t1
	mtctl	t1,pcoq	
	ldi	PSW_Q,t1
	mtctl	t1,ipsw
	rfi
	nop

$qisnowon

	ldil	L%istackptr,r1
	stw	r0,R%istackptr(r1)

	/* 
	 * Initialize the external interrupt request register
	 */
	ldi    	-1,r1
	mtctl   r1,eirr

	/* 
	 * load address of interrupt vector table
	 */
	ldil	L%$ivaaddr,r2
	ldo     R%$ivaaddr(r2),r2
	mtctl   r2,iva

#define FR_SIZE roundup((FM_SIZE+ARG_SIZE),FR_ROUND)

	/*
	 * Create a stack frame for us to call C with. Clear out the previous
	 * sp marker to mark that this is the first frame on the stack.
	 */
	ldo     FR_SIZE(sp),sp
	stw     r0,FM_PSP(sp)

	/*
	 * disable all coprocessors
	 */
	mtctl   r0,ccr

	/*
	 * call C routine hp700_init() to initialize VM
	 */
	.import hp700_init,code
	ldil	L%hp700_init,r1
	ldo	R%hp700_init(r1),r1
	.call
	blr     r0,rp
	bv,n    (r1)
	nop

	/*
	 * go to virtual mode...
	 * get things ready for the kernel to run in virtual mode
	 */
	ldi	HP700_PID_KERNEL,r1
	mtctl	r1,cr8
	mtctl	r1,cr9
	mtctl	r1,cr12
	mtctl	r1,cr13
	mtsp	r0,sr0
	mtsp	r0,sr1
	mtsp	r0,sr2
	mtsp	r0,sr3
	mtsp	r0,sr4
	mtsp	r0,sr5
	mtsp	r0,sr6
	mtsp	r0,sr7

	/*
	 * Cannot change the queues or IPSW with the Q-bit on
	 */
	rsm	RESET_PSW,r0

	/*
	 * We need to do an rfi to get the C bit set
	 */
	mtctl	r0,pcsq
	mtctl	r0,pcsq
	ldil	L%$virtual_mode,t1
	ldo	R%$virtual_mode(t1),t1
	mtctl	t1,pcoq	
	ldo	4(t1),t1
	mtctl	t1,pcoq	
	ldil	L%KERNEL_PSW,t1
	ldo	R%KERNEL_PSW(t1),t1
	mtctl	t1,ipsw
	rfi
	nop

$virtual_mode

	/*
	 * we're in virtual mode now... call setup_main() to start MACH
	 */
	.import setup_main,code
	ldil	L%setup_main,r1
	ldo	R%setup_main(r1),r1
	.call
	blr     r0,rp
	bv,n    (r1)
	nop

	/*
	 * should never return...
	 */


	.align  HP700_PGBYTES
/*
 * Kernel Gateway Page (must be at known address)
 *    System Call Gate
 *    Signal Return Gate
 *
 * GATEway instructions have to be at a fixed known locations
 * because their addresses are hard coded in routines such as
 * those in the C library.
 */
$gateway_page
	nop				/* @ 0.C0000000 (Nothing)  */
	gate,n	$bsd_syscall,r0         /* @ 0.C0000004 (HPUX/BSD) */
	bl,n	$osf_syscall,r0         /* @ 0.C0000008 (HPOSF UNIX) */
	bl,n	$osf_syscall,r0         /* @ 0.C000000C (HPOSF Mach) */
	nop

$osf_syscall
	/*
	 * Convert HPOSF system call to a BSD one by stashing arg4 and arg5 
	 * back into the frame, and moving the system call number into r22.
	 * Fortunately, the HPOSF compiler has a bigger stack frame, which
	 * allows this horrible hack.
	 *
	 * We also need to save r29 (aka ret1) for the emulator since it may
	 * get clobbered between here and there.
	 */
	stw	r22,VA_ARG4(sp)
	stw	r21,VA_ARG5(sp)
	stw	r29,FM_SL(sp)
	gate	$bsd_syscall,r0
	copy	r1,r22

$bsd_syscall
	/*
	 * set up a space register and a protection register so that 
	 * we can access kernel memory
	 */
	mtsp	r0,sr1
	mfctl	pidr2,r28
	ldi	HP700_PID_KERNEL,r1
	mtctl	r1,pidr2

	/*
	 * now call the syscall handler
	 */
	.import $syscall
	.call
	ldil    L%$syscall,r1
	be      R%$syscall(sr1,r1)
	ldil	L%cpu_data,r1		/* From syscall() */

	/*
	 * Don't let anything else get on the gateway page.
	 */
	.align  HP700_PGBYTES

	/*
	 * Support for single stepping.
	 * We put user_sstep in the tail of the pc queue so that
	 * linear execution of a non-nullified instruction
	 * will hit the break at that address.
	 * If the instruction executes linearly but nullifies
	 * the break at user_sstep, the break at user_nullify
	 * is there to force the trap anyway.
	 * Taken branches are handled with taken branch traps.
	 */
	.export	break_page,entry
	.export	user_sstep,entry
	.export	user_nullify,entry
break_page
user_sstep
	break	BREAK_KERNEL, BREAK_KGDB
user_nullify
	break	BREAK_KERNEL, BREAK_KGDB

	.align  HP700_PGBYTES

/*
 * interrupt vector table
 */
	.space  $TEXT$
	.subspa $CODE$
	.align 1024
	.export $ivaaddr
$ivaaddr

	/*
	 * invalid interrupt vector (0)
	 */
	THANDLER(I_NONEXIST)

	/*
 	 * high priority machine check (1)
	 */
	.align  32
	.export i_hpmach_chk,entry
i_hpmach_chk
	nop
	ldo	R%mchkstr(r0),arg0
	.import crashdump,code
	b	crashdump
	addi,tr 0,r0,arg0		# Skip first instr at target.
	break	0,0
hpmc_cksum
	.WORD	0			# HPMC checksum.
	.WORD	crashdump		# Target address.
	.WORD	0			# Length of target code.

	/*
	 * power failure (2)
	 */
	.align	32
	.export $i_power_fail
$i_power_fail
	IHANDLER(I_POWER_FAIL)

	/*
	 * recovery counter trap (3)
	 */
	.align  32
	.export $i_recov_ctr
$i_recov_ctr
#ifdef	GPROF
	/*
	 * We return from the interrupt if the pcoq is in user mode
	 * (lower 2 bites non-zero). Unconditionally reset the recovery
	 * counter for the next time.
	 */
	ldi	PROFILING_INT,r8
	mtctl	r8,cr0
	mfctl	pcoq,r25
	extru,= r25,31,2,r0
	rfir
	b,n	gprof_handler
	nop
#else
	THANDLER(I_RECOV_CTR)
#endif

	/*
 	 * external interrupt (4)
	 */
	.align  32
	.export $i_ext_in
$i_ext_in
	IHANDLER(I_EXT_INTP)

	/*
	 * low-priority machine check (5)
	 */
	.align  32
	.export $i_lpmach_chk
$i_lpmach_chk
	IHANDLER(I_LPMACH_CHK)

	/*
	 * instruction TLB miss fault (6)
 	 */
	.align  32
	.export $i_itlb_miss
$i_itlb_miss
	mfsp	sr1,r17		/* Need a space reg, so save it in a shadow */
	mfctl   pcoq,r9		/* Offset */
	mfctl   pcsq,r8		/* Space  */
	depi    0,31,12,r9	/* Clear the byte offset into the page */
	b       $itlbmiss
	mtsp	r8,sr1		/* Put the space id where we can use it */

	/*
	 * instruction memory protection trap (7)
	 */
	.align  32
	.export $i_imem_prot
$i_imem_prot 
	THANDLER(I_IMEM_PROT)

	/*
	 * Illegal instruction trap (8)
	 */
	.align  32
	.export $i_unimpl_inst
$i_unimpl_inst
	THANDLER(I_UNIMPL_INST)

	/*
	 * break instruction trap (9)
	 */
	.align  32
	.export $i_brk_inst
$i_brk_inst
	mtctl	t1,tr2
	mtctl	t2,tr3
	b	bhandler
	mtctl	t3,tr4

	/*
	 * privileged operation trap (10)
	 */
	.align  32
	.export     $i_priv_op
$i_priv_op
	THANDLER(I_PRIV_OP)

	/*
 	 * privileged register trap (11)
	 */
	.align  32
	.export $i_priv_reg
$i_priv_reg
	THANDLER(I_PRIV_REG)

	/*
	 * overflow trap (12)
	 */
	.align  32
	.export $i_ovflo
$i_ovflo
	THANDLER(I_OVFLO)

	/*
	 * conditional trap (13)
	 */
	.align  32
	.export $i_cond
$i_cond
	THANDLER(I_COND)

	/*
	 * assist exception trap (14)
	 */
	.align  32
	.export $i_excep
$i_excep
        mtctl   r31,tr2
        .import $exception_trap,code
        ldil    L%$exception_trap,r31
        .call
        be,n    R%$exception_trap(sr4,r31)

	/*
	 * data TLB miss fault (15)
 	 */
	.align  32
	.export $i_dtlb_miss
$i_dtlb_miss
	mfsp	sr1,r17		/* Need a space reg, so save it in a shadow */
	mfctl   ior,r9		/* Offset */
	mfctl   isr,r8		/* Space  */
	depi    0,31,12,r9	/* Clear the byte offset into the page */
	b	$tlbmiss
	mtsp	r8,sr1		/* Put the space id where we can use it */

	/*
	 * instruction TLB non-access miss fault (16)
 	 */
	.align  32
	.export $i_itlbna_miss
$i_itlbna_miss
	mfsp	sr1,r17		/* Need a space reg, so save it in a shadow */
	mfctl   ior,r9
	depi    0,31,12,r9	/* Clear the byte offset into the page */
	mfctl   isr,r8
	b       $itlbmiss
	mtsp	r8,sr1		/* Put the space id where we can use it */

	/*
	 * data TLB non-access miss fault (17)
 	 */
	.align  32
	.export $i_dtlbna_miss
$i_dtlbna_miss
	mfsp	sr1,r17		/* Need a space reg, so save it in a shadow */
	mfctl   ior,r9		/* Space  */
	mfctl   isr,r8		/* Offset */
	depi    0,31,12,r9	/* Clear the byte offset into the page */
	b	$tlbmiss
	mtsp	r8,sr1		/* Put the space id where we can use it */

	/*
	 * data memory protection trap/unalligned data reference trap (18)
	 */
	.align 32
	.export $i_dmem_prot
$i_dmem_prot
	THANDLER(I_DMEM_PROT)

	/*
	 * data memory break trap (19)
 	 */
	.align  32
	.export     $i_dmem_break
$i_dmem_break
	THANDLER(I_DMEM_BREAK)

	/*
	 * TLB dirty bit trap (20)
	 */
	.align  32
	.export $i_tlb_dirty
$i_tlb_dirty
	mfsp	sr1,r16		/* Need a space reg, so save it in a temp */
	mtctl	r16,tr7
	mfctl   ior,r9		/* Space  */
	mfctl   isr,r8		/* Offset */
	depi    0,31,12,r9	/* Clear the byte offset into the page */
	b       $tlbdtrap
	mtsp	r8,sr1		/* Put the space id where we can use it */

	/*
	 * page reference trap (21)
	 */
	.align  32
	.export $i_page_ref
$i_page_ref
	THANDLER(I_PAGE_REF)

	/*
	 * assist emulation trap (22)
	 */
	.align  32
	.export $i_emulat
$i_emulat
        mtctl   r31,tr2
        .import fpu_switch
        ldil    L%fpu_switch,r31
        .call
        be,n    R%fpu_switch(sr4,r31)

	/*
	 * higher-privilege transfer trap (23)
	 */
	.align  32
	.export $i_hpriv_xfr
$i_hpriv_xfr
	THANDLER(I_HPRIV_XFR)

	/*
	 * lower-privilege transfer trap (24)
	 */
	.align  32
	.export $i_lpriv_xfr
$i_lpriv_xfr
	THANDLER(I_LPRIV_XFR)

	/*
	 * taken branch trap (25)
	 */
	.align  32
	.export $i_taken_br
$i_taken_br
	THANDLER(I_TAKEN_BR)

	/*
	 * data memory access rights trap (26)  T-chip only
	 */
	.align	32
	.export	$i_dmem_acc
$i_dmem_acc
	THANDLER(I_DMEM_ACC)

	/*
	 * data memory protection ID trap (27)  T-chip only
	 */
	.align	32
	.export	$i_dmem_pid
$i_dmem_pid
	THANDLER(I_DMEM_PID)

	/*
	 * unaligned data reference trap (28)  T-chip only
	 */
	.align	32
	.export	$i_unalign
$i_unalign
	THANDLER(I_UNALIGN)


/*
 * gprof_handler
 */
#ifdef	GPROF
	.align	32
	.export gprof_handler,entry
gprof_handler
	/*
	 * See if profiling is turned on. 
	 */
	ldil	L%profiling,r8
	ldw	R%profiling(r8),r8
	comib,<=,n        2,r8,$gprof_exit

	/*
	 * Make sure the faulting address is within kernel text.
	 */
	ldil	L%s_lowpc,r16
	ldw	R%s_lowpc(r16),r16
	sub	r25,r16,r8
	ldil	L%s_textsize,r25
	ldw	R%s_textsize(r25),r25
	comb,<,n	r8,r0,$gprof_exit
	comb,>=,n	r8,r25,$gprof_exit

	/*
	 * Compute the offset into the kcount array, and add 1 to the count,
	 * only if it would not overflow the count.
	 */
	ldil	L%kcount,r24
	ldw	R%kcount(r24),r24
	extru   r8,29,30,r25
	sh1add  r25,r24,r16
	ldh	0(r16),r8
	zdepi   -1,30,15,r17
	comb,<<=,n      r17,r8,$gprof_exit
	ldo     1(r8),r8
	sth	r8,0(r16)
$gprof_exit
	rfir
	nop
#endif

/*
 * bhandler
 * 
 * deal with break instruction and see if we recognize it as either a 
 * kernel debugger break or a PDC call. Everything else gets dealt with
 * in trap() after we have gone to virtual mode and interrupts are enabled.
 */
	.export bhandler,entry
bhandler

	/*
	 * get us a few temporary registers
	 */
	mfctl	iir,t1
	extru	t1,31,5,t2
	comib,<>,n BREAK_KERNEL,t2,$break_not_special

	/*
	 * If this was called by a user process then always pass it to trap().
	 */
	mfctl   pcoq,t2
	extru,= t2,31,2,r0
	b,n     $break_not_special

	.import etext
	ldil	L%etext,t3
	ldo	R%etext(t3),t3
	combf,<<,n t2,t3,$break_not_special
	nop

	/*
	 * get the other field and see if we know how to deal with it
	 */
	extru	t1,18,13,t2
	comib,=,n BREAK_PDC_CALL,t2,$pdc_break

$break_not_special

	/*
	 * This isn't a break we recognize. restore register and call the
	 * trap handler to deal with it.
	 */
	mfctl	tr2,t1
	mfctl	tr3,t2
	mfctl	tr4,t3
	THANDLER(I_BRK_INST)

/*
 * handle the call PDC break instruction
 */
$pdc_break
	/*
	 * load the value of the caller's stack pointer into ret0
	 */
	copy	sp,ret0

	/*
	 * now we need to get the Q bit on, We still won't turn on interrupts
	 * so we don't have to save everything. We know that we came in as
	 * a result of our subroutine and hence we only have to save the 
	 * callee save registers that we touch. We can use the tmp_saved_state
	 * area to offload the registers that we need to turn the Q bit on.
	 */
	ldil	L%tmp_saved_state,t1
	ldo	R%tmp_saved_state(t1),t1
	stw	sp,SS_R30(t1)

	ldil	L%istackptr,t2
	ldo	R%istackptr(t2),t2
	ldw	0(t2),sp

	/*
	 * if the value of istackptr is zero then we are already on the
	 * interrupt stack. 
 	 */
	comb,<>,n r0,sp,$pdc_not_on_istack

	/*
	 * we were already on the interrupt stack. Get the value of the
	 * old stack pointer and clear r30 in the tmp_saved_state area to 
	 * indicate that we were on the interrupt stack.
	 */
	ldw	SS_R30(t1),sp

	/*
	 * Now we need to see if there is at least 7K of space left on the
	 * interrupt stack. Add 40 for a call frame and 7168 for the PDC 
 	 * routine
	 */
	ldo	7208(sp),t2
	ldil	L%intstack_top,t3
	ldo	R%intstack_top(t3),t3
	comb,< t2,t3,$pdc_setup
	stw	r0,SS_R30(t1)

	/*
	 * we don't have the stack space to do this, panic
	 */
	IHANDLER(I_IS_OVFL)#

$pdc_not_on_istack

	/*
	 * clear the value in istackptr to indicate that we are on the 
	 * interrupt stack.
	 */
	stw	r0,0(t2)

$pdc_setup

	/*
	 * save the return pointer
	 */
	stw	r2,SS_R2(t1)

	/*
	 * Now we need to get the Q bit on. Clear out all the registers 
	 * that we will need to do this.
	 */
	mfctl	ipsw,t2
	stw	t2,SS_CR22(t1)
	mfctl	pcoq,t2
	stw	t2,SS_IIOQH(t1)
	mtctl	r0,pcoq
	mfctl	pcoq,t2
	stw	t2,SS_IIOQT(t1)
	mfctl	pcsq,t2
	stw	t2,SS_IISQH(t1)
	mtctl	r0,pcsq
	mfctl	pcsq,t2
	stw	t2,SS_IISQT(t1)

	/*
	 * now load things up like we want it
	 */
	mtctl	r0,pcsq
	mtctl	r0,pcsq
	ldil	L%$pdc_qbiton,t2
	ldo	R%$pdc_qbiton(t2),t2
	mtctl	t2,pcoq
	ldo	4(t2),t2
	mtctl	t2,pcoq

	/*
	 * We only want the Q bit on, if this is a PDC_PIM call with arg1=0
	 * then we also have to turn on the M bit. If we take a HPMC then
	 * the game is over anyway. We won't cry over the fact that we
	 * lose the state in the tmp_saved_state.
	 */
	copy	r0,t2
	comib,<> r0,arg1,$pdc_no_m_bit
	depi	1,PSW_Q_P,1,t2
	comib,<>,n PDC_PIM,arg2,$pdc_no_m_bit
	comiclr,<> PDC_PIM_HPMC,arg3,0

	/*
	 * We are requesting information about the HPMC we just took. Set
	 * the M bit as required by the architecture.
	 */
	depi	1,PSW_M_P,1,t2

$pdc_no_m_bit

	/*
	 * save the psw we want to come back with
	 */
	mtctl	t2,ipsw
	rfi
	nop

$pdc_qbiton

	/*
	 * now that the Q bit is on we need to get a stack pointer that is
	 * quadword aligned. We'll shove the value of the stack pointer that
	 * we really need to restore into the tmp_saved_state structure as
	 * ret0 and then we'll get the thing quadword aligned.
	 */
	stw	sp,SS_R28(t1)

	/*
	 * now make it quadword aligned. We'll add 16 instead of 15 so that
	 * the stack is still aligned...
	 */
	ldo	16(sp),sp
	depi	0,31,4,sp

	/*
	 * set up an 12 argument stack frame to call with.
	 */
	ldo	FM_SIZE+48(sp),sp

	/*
	 * Now fill in the arguments. We'll fill in 11 arguments even though
	 * we might not need them...
	 *
	 * pdc_iodc stashed the arguments in the tmp_saved_state. We will
	 * pull them back from there, and put them in the new stack frame.
	 */
	copy	arg0,t3
	copy	arg2,arg0
	copy	arg3,arg1
	ldo	SS_SIZE(t1),t1
	ldwm	4(t1),arg2
	ldwm	4(t1),arg3
	ldwm	4(t1),t2
	stw	t2,VA_ARG4(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG5(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG6(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG7(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG8(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG9(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG10(sp)
	ldwm	4(t1),t2
	stw	t2,VA_ARG11(sp)

	/*
	 * clear the previous stack pointer to stop stack traces
	 */
	stw	r0,FM_PSP(sp)

	/*
	 * call the routine
	 */
	.call
	blr     r0,rp
	bv,n    (t3)
	nop

	/*
	 * returned from PDC call. Restore the old stack pointer and then 
	 * figure out how to get us off the interrupt stack
	 */
	ldil	L%tmp_saved_state,t1
	ldo	R%tmp_saved_state(t1),t1
	ldw	SS_R30(t1),t2

	/*
	 * if t2 is zero then we were on the stack pointer when we came in
	 * so we don't have to do anything.
	 */
	comb,= r0,t2,$pdc_sp_restored
	ldw	SS_R28(t1),sp

	/*
	 * t2 is not zero so that means that we need to put this stack pointer
	 * back into istackptr and then restore the user stack pointer from
	 * t2.
	 */
	ldil	L%istackptr,t3
	stw	sp,R%istackptr(t3)
	copy	t2,sp

$pdc_sp_restored

	/*
	 * Cannot change the queues or IPSW with the Q-bit on
	 */
	rsm	RESET_PSW,r0

	/*
	 * We only need to restore the PC queues, ipsw and the return pointer
	 * since everyone is adhering to the calling convention.
	 */
	ldw	SS_R2(t1),r2
	ldw	SS_CR22(t1),t2
	mtctl	t2,ipsw

	/*
	 * need to move PC past the break instruction
	 */
	ldw	SS_IIOQT(t1),t2
	mtctl	t2,pcoq
	ldo	4(t2),t2	
	mtctl	t2,pcoq
	ldw	SS_IISQH(t1),t2
	mtctl	t2,pcsq
	ldw	SS_IISQT(t1),t2
	mtctl	t2,pcsq
	mfctl	tr2,t1
	mfctl	tr3,t2
	rfi
	nop

/* 
 * $tlbmiss
 *
 * TLB miss handler 
 */
	.align	32
	.export $tlbmiss,entry
$tlbmiss
	/*
	 * See trap vector for setup instructions.
	 */
#ifdef	TLBSTATS
	nop
	nop
	nop	/* Nops to align the start of real code to ICACHE boundary */
	nop
	nop
	nop
	ldil	L%dtlb_statbase,r25
	ldo	R%dtlb_statbase(r25),r25
	ldw	0(r25),r16
	ldo	1(r16),r16
	stw	r16,0(r25)
	ldi	4096,r16		# do recovery counter first, since 
	mtctl	r16,cr0			# it's easy to count the actual
	ssm	PSW_R,r0		# number of overhead instructions
	mfctl	cr16,r16
	stw	r16,24(r25)
#endif
	/*
	 * Since we map the kernel region with a block TLB, there will never
	 * be a TLB miss in the equiv area. So, do not even test for it.
	 * The only other alternatives are non-equiv misses (kernel or user),
	 * and I/O pages. Since I/O faults are rare, we optimize for
	 * the common case by always looking for a mapping in the vtop table.
	 * If we cannot find one, then check to see if it is a I/O fault,
	 * punting to the page fault code if it is not.
	 */

	 /*
	  * Compute the hash index.
	  */
	extru	r9,19,20,r16  		/* r16 = (offset >> 12) */
	zdep	r8,26,16,r24	     	/* r24 = (space << 5) */
	xor	r16,r24,r1		/* r1  = r16 ^ r24 */

	/*
	 * r8 has the space of the address that had the TLB miss
	 * r9 has the offset of the address that had the TLB miss
	 * r1 has the hash index
	 */
#ifndef	HPT
	/*
	 * Compute the VTOP table entry address using the hash index.
	 */
	mfctl   vtop,r16              	   /* r16 = address of VTOP table */
	zdep	r1,VTOP_SHIFT,VTOP_LEN,r24 /* mask out field  */
	add     r24,r16,r24            	   /* r24 = VTOP entry */
#else
	/*
	 * Compute the HPT table entry address using the hash index.
	 */
	mfctl   vtop,r16              	   /* r16 = address of HPT table */
	zdep	r1,HPT_SHIFT,HPT_LEN,r24   /* mask out field  */
	add     r24,r16,r24            	   /* r24 = HPT entry */

	/*
	 * Construct the virtual address tag.
	 */
	extru	r9,14,15,r16		   /* r24 = off[0..14] */
	zdep	r16,15,15,r16		   /* r24 = tag[1..15] */
	or	r16,r8,r16		   /* or in the space id */
	depi	1,0,1,r16		   /* and set the valid bit */

	/*
	 * Compare the tag against the HPT entry. If it matches, then
	 * pull the page/prot out and do the TLB insertion.
	 */
	ldw	HPT_TAG(r24),r25
	comb,<>,n r16,r25,$dtlb_hptmiss

	ldw	HPT_PROT(r24),r25
	ldw	HPT_PAGE(r24),r16

	idtlba  r16,(sr1,r9)
	idtlbp  r25,(sr1,r9)

	/*
	 * And return ...
	 */
#ifdef	TLBSTATS
	mfctl	cr16,r24		# Final cycle count
	rsm	PSW_R,r0		# Stop the recovery counter
	ldil	L%dtlb_statbase,r25
	ldo	R%dtlb_statbase(r25),r25
	ldw	20(r25),r16
	ldo	1(r16),r16
	b	$dtlb_cache_exit
	stw	r16,20(r25)
#else
	mtsp	r17,sr1
	rfir
	nop
#endif
$dtlb_hptmiss
	/*
	 * Okay, so we missed in the HPT cache. Stash away the HPT entry
	 * pointer and the virtual tag for later ...
	 *
	 * Switch r24 to point to the corresponding VTOP table entry so
	 * we can move onto chasing the hash chain.
	 */
	mtctl	r24,tr7
	mtctl	r16,tr6
	ldw	HPT_VTOP(r24),r24
#endif

	/*
	 * Chase the list of entries for this hach bucket until we find the
	 * correct mapping or return to the head.
	 *
	 * WARNING: assumes that queue_t is the 1st element of a hash entry.
	 *	    (currently it is the only element).
	 */
	ldw	VTOP_LINKN(r24),r1
$tlb_loop
	comb,=,n r24,r1,$dtlb_maybeiopage
	ldw     HP700_MAP_OFFSET(r1),r25
	comb,<>,n r9,r25,$tlb_loop
	ldw	HP700_MAP_HLINKN(r1),r1
	ldw     HP700_MAP_SPACE(r1),r16
	comb,<>,n r8,r16,$tlb_loop
	ldw	HP700_MAP_HLINKN(r1),r1

	/*
	 * r1 now points to the mapping for this page.
	 * Check to see if the mapping has the TLB_ALIGNED bit set. If it is,
	 * then there is no need to check the PTOV table for a writer. This
	 * is the common case.
	 */
	ldw	HP700_MAP_PROT(r1),r25
	bb,>=,n	r25,TLB_ALIGNED_POS,$dtlb_maybewriter

$dtlb_insert
	/*
	 * Now set things up to enter the real mapping that we want
	 */
	ldw	HP700_MAP_PAGE(r1),r16

	/*
	 * data TLB miss
	 */
	idtlba  r16,(sr1,r9)
	idtlbp  r25,(sr1,r9)

	/*
	 * now set the software bits in the protection of this page
	 */
	or	r0,r0,r24
	depi	1,TLB_DCACHE_POS,1,r24
	depi	1,TLB_ICACHE_POS,1,r24	
	stw	r24,HP700_MAP_SW(r1)
		
	depi	1,TLB_REF_POS,1,r25
	stw	r25,HP700_MAP_PROT(r1)
	
#ifdef	HPT
	/*
	 * Load the HPT cache with the miss information for next time.
	 * The HTP entry address and virtual tag were saved above in
	 * control registers.
	 */
	bb,>=,n	r25,TLB_ALIGNED_POS,$dtlb_skiphpt
	
	mfctl	tr7,r24
	mfctl	tr6,r8

	stw	r8,HPT_TAG(r24)
	stw	r16,HPT_PAGE(r24)
	stw	r25,HPT_PROT(r24)
$dtlb_skiphpt
#endif

$dtlb_exit
	/*
	 * restore sr1. The rest of the registers are restored by the
	 * rfir.
	 */
#ifdef	TLBSTATS
	mfctl	cr16,r24		# Final cycle count
	rsm	PSW_R,r0		# Stop the recovery counter
	ldil	L%dtlb_statbase,r25	# Load base
	ldo	R%dtlb_statbase(r25),r25# Load base
$dtlb_cache_exit
	ldw	24(r25),r8		# Get initial cycle count
	sub	r24,r8,r8		# Get the difference
	ldw	8(r25),r24		# Get stored cycle count
	add	r8,r24,r24		# and add latest value
	stw	r24,8(r25)		# and store back into stats
	ldi	4093,r24		# Initial recovery value (adjusted)
	mfctl	cr0,r8			# Get final recovery value
	sub	r24,r8,r8		# Get the difference
	ldw	16(r25),r24		# Get store instruction count
	add	r8,r24,r24		# and add latest value
	stw	r24,16(r25)		# and sore back into stats
	mtctl	r0,cr0
#endif
	mtsp	r17,sr1
	rfir
	nop

$dtlb_maybewriter
	/*
	 * Find entry in the physical to virtual table for this address.
	 *
	 * WARNING: assumes 16-byte phys_table entries.
	 */
	mfctl	ptov,r16
	ldw	HP700_MAP_PAGE(r1),r25
	shd	r0,r25,1,r25
	add	r25,r16,r25

	/*
	 * Check to see if there are any mappings that have written to this 
	 * page (writer pointer will be non-null).
	 */
	ldw	 PTOV_WRITER(r25),r24
	comb,=,n r24,r0,$dtlb_insert
	ldw	HP700_MAP_PROT(r1),r25

	/*
	 * There was a writer, but if it is us, we do not need to flush.
	 */
	comb,=,n r24,r1,$dtlb_insert
	ldw	HP700_MAP_PROT(r1),r25

	/*
	 * OK, someone has written to this page, we need to first flush the 
	 * data cache for the entire page and zero the pointer. Then finally 
	 * get back to the TLB miss at hand.
	 */
	stw	r0,PTOV_WRITER(r25)

	/*
	 * we need to make sure that the TLB entry for this data page is valid.
	 */
	ldw	HP700_MAP_SPACE(r24),r8
	ldw	HP700_MAP_OFFSET(24),r25
	mtsp	r8,sr1
	ldw	HP700_MAP_PAGE(r24),r8
	ldw	HP700_MAP_PROT(r24),r16

	idtlba	r8,(sr1,r25)
	idtlbp	r16,(sr1,r25)

	/*
 	 * get the proper data cache stride into r8
	 */
	ldil    L%dcache_stride,r8 
	ldw     R%dcache_stride(r8),r8

	/*
	 * get the last byte to flush into r16
	 */
	ldo	HP700_PGBYTES(r25),r16

	/*
	 * turn on virtual addressing of data and flush the cache
	 */
	ssm	PSW_D,r0
	fdc,m   r8(sr1,r25)
$dtlb_flush
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	comb,<<,n r25,r16,$dtlb_flush
	fdc,m   r8(sr1,r25)
	sync

	/*
	 * turn off virtual addressing of data
	 */
	rsm	PSW_D,r0

	/*
	 * we need to clear the dirty bit on this page since the cache
	 * has been flushed.
	 */
	ldw	HP700_MAP_PROT(r24),r8
	depi	0,TLB_DIRTY_POS,1,r8

	/*
	 * update this new protection in the TLB
	 */
	ldw	HP700_MAP_OFFSET(r24),r16
	stw	r8,HP700_MAP_PROT(r24)
	idtlbp  r8,(sr1,r16)

	/*
	 * Get the space/offset back out of the mapping structure, and then
	 * branch back to the tlb insertion code. We flushed, so time does 
	 * not matter all that much at this point.
	 */
	ldw	HP700_MAP_SPACE(r1),r8
	ldw	HP700_MAP_OFFSET(r1),r9
	ldw	HP700_MAP_PROT(r1),r25
	b	$dtlb_insert
	mtsp	r8,sr1

$dtlb_maybeiopage
	/*
	 * Check to see if it might be an I/O page fault. The space must be
	 * 0, and the top 4 bits must be set. If not, it is a page fault.
	 */
	extru	r9,3,4,r16
	comib,<>,n 0xf,r16,$dpageflt
	comb,<>,n r0,r8,$dpageflt

	/*
	 * this is an I/O page. First see if it's in the known I/O area
	 * if it's not then it's a page fault.
	 */
	ldil	L%io_end,r25
	ldw	R%io_end(r25),r25
	comb,>,n r25,r9,$dpageflt
#ifdef IO_HACK
	/*
         * There are no ptov table entries for IO pages, so create a
	 * kernel TLB entry.
	 */
	extrs	r9,24,25,r16		/* pa >> 7 always: see pmap.c */
	ldil	L%TLB_AR_KRW,r25
	depi	1,TLB_DIRTY_POS,1,r25	/* mark dirty now, avoid later trap */
#else
	/*
	 * XXX determine which of the 4 IO quads we are in and extract
	 * the necessary info.
	 *
	 * WARNING: assumes 16-byte phys_table entries.
	 */
	extrs	r9,9,10,r25		/* (pa >> 22) ... */
	depi	0,31,4,r25		/* ... & ~15 yields -[0123] ptov ix */
	mfctl	ptov,r16
	add	r16,r25,r16
	ldw	PTOV_TLBPROT(r16),r25
	depi	1,TLB_DIRTY_POS,1,r25	/* mark dirty now, avoid later trap */
	extrs	r9,24,25,r16		/* pa >> 7 always: see pmap.c */
#endif
	idtlba	r16,(sr1,r9)
	b	$dtlb_exit
	idtlbp	r25,(sr1,r9)

$dpageflt
	/*
	 * Can't find the mapping, call it a page fault.
	 */
#ifdef	TLBSTATS
	rsm	PSW_R,r0		# Stop the recovery counter
	mtctl	r0,cr0
	ldil	L%dtlb_statbase,r25	# Load base
	ldo	R%dtlb_statbase(r25),r25# Load base
	ldw	12(r25),r16		# Load pagefault count
	ldo	1(r16),r16		# Add 1
	stw	r16,12(r25)		# and store back into stats
#endif
	/*
	 * restore sr1. the rest of the registers are restored by the
	 * rfir in thandler or possibly in ihandler.
	 */
	mtsp	r17,sr1

	THANDLER(I_DPGFT)


/* 
 * $itlbmiss
 *
 * ITLB miss handler 
 */
	.align	32
	.export $itlbmiss,entry
$itlbmiss
	/*
	 * See the trap vector for instructions. Gotta save ICACHE misses!
	 */
#ifdef	TLBSTATS
	nop
	nop
	nop	/* Nops to align the start of real code to ICACHE boundary */
	nop
	nop
	nop
	ldil	L%itlb_statbase,r25
	ldo	R%itlb_statbase(r25),r25
	ldw	0(r25),r16
	ldo	1(r16),r16
	stw	r16,0(r25)
	ldi	4096,r16		# do recovery counter first, since 
	mtctl	r16,cr0			# it's easy to count the actual
	ssm	PSW_R,r0		# number of overhead instructions
	mfctl	cr16,r16
	stw	r16,24(r25)
#endif
	/*
	 * Since we map the kernel region with a block TLB, there will never
	 * be a TLB miss in the equiv area. So, do not even test for it.
	 * The only other alternatives are non-equiv misses (kernel or user),
	 * and gateway pages. Since gateway faults are rare, we optimize for
	 * the common case by always looking for a mapping in the vtop table.
	 * If we cannot find one, then check to see if it is a gateway fault,
	 * punting to the pagefault code if it is not.
	 */

	/*
	 * First find the hash value for this virtual address
	 */
	extru	r9,19,20,r16  		/* r16 = (offset >> 12) */
	zdep	r8,26,16,r24	     	/* r24 = (space << 5) */
	xor	r16,r24,r1		/* r1  = (r16 ^ r24) */

#ifndef	HPT
	/*
	 * Compute the VTOP table entry address using the hash index.
	 */
	mfctl   vtop,r16              	   /* r16 = address of VTOP table */
	zdep	r1,VTOP_SHIFT,VTOP_LEN,r24 /* mask out field  */
	add     r24,r16,r24            	   /* r24 = VTOP entry */
#else
	/*
	 * Compute the HPT table entry address using the hash index.
	 */
	mfctl   vtop,r16              	   /* r16 = address of HPT table */
	zdep	r1,HPT_SHIFT,HPT_LEN,r24   /* mask out field  */
	add     r24,r16,r24            	   /* r24 = HPT entry */

	/*
	 * Construct the virtual address tag.
	 */
	extru	r9,14,15,r16		   /* r24 = off[0..14] */
	zdep	r16,15,15,r16		   /* r24 = tag[1..15] */
	or	r16,r8,r16		   /* or in the space id */
	depi	1,0,1,r16		   /* and set the valid bit */

	/*
	 * Compare the tag against the HPT entry. If it matches, then
	 * pull the page/prot out and do the TLB insertion.
	 */
	ldw	HPT_TAG(r24),r25
	comb,<>,n r16,r25,$itlb_hptmiss

	ldw	HPT_PROT(r24),r25
	ldw	HPT_PAGE(r24),r16

	iitlba  r16,(sr1,r9)
	iitlbp  r25,(sr1,r9)

	/*
	 * And return ...
	 */
#ifdef	TLBSTATS
	mfctl	cr16,r24		# Final cycle count
	rsm	PSW_R,r0		# Stop the recovery counter
	ldil	L%itlb_statbase,r25
	ldo	R%itlb_statbase(r25),r25
	ldw	20(r25),r16
	ldo	1(r16),r16
	b	$itlb_cache_exit
	stw	r16,20(r25)
#else
	mtsp	r17,sr1
	rfir
	nop
#endif
$itlb_hptmiss
	/*
	 * Okay, so we missed in the HPT cache. Stash away the HPT entry
	 * pointer and the virtual tag for later ...
	 *
	 * Switch r24 to point to the corresponding VTOP table entry so
	 * we can move onto chasing the hash chain.
	 */
	mtctl	r24,tr7
	mtctl	r16,tr6
	ldw	HPT_VTOP(r24),r24
#endif
	/*
	 * r8 has the space of the address that had the TLB miss
	 * r9 has the offset of the address that had the TLB miss
	 * r24 points to the vtop entry for this address
	 */

	/*
	 * Chase the list of entries for this hach bucket until we find the
	 * correct mapping or return to the head.
	 *
	 * WARNING: assumes that queue_t is the 1st element of a hash entry.
	 *	    (currently it is the only element).
	 */
	ldw	VTOP_LINKN(r24),r1

$itlb_loop
	comb,=,n r24,r1,$itlb_maybegateway
	ldw     HP700_MAP_OFFSET(r1),r25
	comb,<>,n r9,r25,$itlb_loop
	ldw	HP700_MAP_HLINKN(r1),r1
	ldw     HP700_MAP_SPACE(r1),r16
	comb,<>,n r8,r16,$itlb_loop
	ldw	HP700_MAP_HLINKN(r1),r1

	/*
	 * Now set things up to enter the real mapping that we want.
	 * r8 and r9 still point to space/offset.
	 */
	ldw	HP700_MAP_PAGE(r1),r16
	ldw	HP700_MAP_PROT(r1),r25
	
	/*
	 * instruction TLB miss
	 */
	iitlba  r16,(sr1,r9)
	iitlbp  r25,(sr1,r9)
	
	or	r0,r0,r24
	depi	1,TLB_ICACHE_POS,1,r24
	depi	1,TLB_DCACHE_POS,1,r24
	stw	r24,HP700_MAP_SW(r1)
	
	depi	1,TLB_REF_POS,1,r25
	stw	r25,HP700_MAP_PROT(r1)

#ifdef	HPT
	/*
	 * Load the HPT cache with the miss information for next time.
	 * The HTP entry address and virtual tag were saved above in
	 * control registers.
	 */
	mfctl	tr7,r24
	mfctl	tr6,r8

	stw	r8,HPT_TAG(r24)
	stw	r16,HPT_PAGE(r24)
	stw	r25,HPT_PROT(r24)
#endif

$itlb_exit
	/*
	 * restore sr1. The rest of the registers are restored by the
	 * rfir.
	 */
#ifdef	TLBSTATS
	mfctl	cr16,r24		# Final cycle count
	rsm	PSW_R,r0		# Stop the recovery counter
	ldil	L%itlb_statbase,r25	# Load base
	ldo	R%itlb_statbase(r25),r25# Load base
$itlb_cache_exit
	ldw	24(r25),r8		# Get initial cycle count
	sub	r24,r8,r8		# Get the difference
	ldw	8(r25),r24		# Get stored cycle count
	add	r8,r24,r24		# and add latest value
	stw	r24,8(r25)		# and store back into stats
	ldi	4093,r24		# Initial recovery value (adjusted)
	mfctl	cr0,r8			# Get final recovery value
	sub	r24,r8,r8		# Get the difference
	ldw	16(r25),r24		# Get store instruction count
	add	r8,r24,r24		# and add latest value
	stw	r24,16(r25)		# and sore back into stats
	mtctl	r0,cr0
#endif
	mtsp	r17,sr1
	rfir
	nop

$itlb_maybegateway
	/*
	 * The only way it can be a gateway fault is if the space is 0
	 * and the offset is the gateway offset. Punt to pagefault if not.
	 */
	ldil	L%0xC0000000,r16
	comb,<>,n r16,r9,$ipageflt
	comb,<>,n r0,r8,$ipageflt

#ifdef	TLBSTATS
	ldil	L%itlb_statbase,r25
	ldo	R%itlb_statbase(r25),r25
	ldw	4(r25),r16
	ldo	1(r16),r16
	stw	r16,4(r25)
#endif
	/*
	 * get the address into a form that we can put in the TLB
	 */
	ldil	L%$gateway_page,r16
	ldo	R%$gateway_page(r16),r16
	extrs	r16,24,25,r16		/* pa >> 7 always: see pmap.c */

	/*
	 * load the gateway protection
	 */
	ldil	L%TLB_GATE_PROT,r1

	/*
	 * load the instruction tlb and exit. 
	 */
	iitlba	r16,(sr1,r9)
	iitlbp	r1,(sr1,r9)
#ifdef	TLBSTATS
	b,n	$itlb_exit
#else
	mtsp	r17,sr1
	rfir
	nop
#endif
	
$ipageflt
	/*
	 * Can't find the mapping, call it a page fault.
	 */
#ifdef	TLBSTATS
	rsm	PSW_R,r0		# Stop the recovery counter
	mtctl	r0,cr0
	ldil	L%itlb_statbase,r25	# Load base
	ldo	R%itlb_statbase(r25),r25# Load base
	ldw	12(r25),r16		# Load pagefault count
	ldo	1(r16),r16		# Add 1
	stw	r16,12(r25)		# and store back into stats
#endif
	/*
	 * restore sr1. the rest of the registers are restored by the
	 * rfir in thandler or possibly in ihandler.
	 */
	mtsp	r17,sr1

	THANDLER(I_IPGFT)


/* 
 * TLB dirty bit trap (20)
 */
	.align	32
	.export $tlbdtrap,entry
$tlbdtrap
	/*
	 * See the trap vector for instructions. Gotta save ICACHE misses!
	 */
#ifdef	TLBSTATS
	nop
	nop
	nop	/* Nops to align the start of real code to ICACHE boundary */
	nop
	nop
	nop
	ldil	L%tlbd_statbase,r25
	ldo	R%tlbd_statbase(r25),r25
	ldw	0(r25),r16
	ldo	1(r16),r16
	stw	r16,0(r25)
	ldi	4096,r16		# do recovery counter first, since 
	mtctl	r16,cr0			# it's easy to count the actual
	ssm	PSW_R,r0		# number of overhead instructions
	mfctl	cr16,r16
	stw	r16,20(r25)
#endif
	/*
	 * Since we map the kernel region with a block TLB, and set the dirty
	 * bit on the entry, there will never be a TLB dirty trap for the 
	 * equiv region. I/O pages are also entered with the dirty bit set,
	 * so we won't see them either. The only other alternative is
	 * a non-equiv misse (kernel or user). Further, we optimize for the
	 * common case by checking for the TLB align field in mapping structure
	 * to see if we need to check for a writer.
	 */

	 /*
	  * Compute the hash index.
	  */
	extru	r9,19,20,r16  		/* r16 = (offset >> 12) */
	zdep	r8,26,16,r24	     	/* r24 = (space << 5) */
	xor	r16,r24,r1		/* r1  = (r16 ^ r24) */

#ifndef	HPT
	/*
	 * Compute the VTOP table entry address using the hash index.
	 */
	mfctl   vtop,r16              	   /* r16 = address of HPT table */
	zdep	r1,VTOP_SHIFT,VTOP_LEN,r24 /* mask out field  */
	add     r24,r16,r24            	   /* r24 = HPT entry */
#else
	/*
	 * Compute the HPT table entry address using the hash index.
	 */
	mfctl   vtop,r16              	   /* r16 = address of HPT table */
	zdep	r1,HPT_SHIFT,HPT_LEN,r24   /* mask out field  */
	add     r24,r16,r24            	   /* r24 = HPT entry */

	/*
	 * Invalidate the entry, no matter what it is. We don't take
	 * enough dirty traps for it to really matter.
	 */
	ldi	-1,r16
	depi	0,0,1,r16
	stw	r16,HPT_TAG(r24)

	/*
	 * Switch r24 to point to the associated VTOP table entry.
	 */
	ldw	HPT_VTOP(r24),r24
#endif

	/*
	 * r8 has the space of the address that had the TLB miss
	 * r9 has the offset of the address that had the TLB miss
	 * r24 points to the vtop entry for this address
	 */

	/*
	 * Chase the list of entries for this hach bucket until we find the
	 * correct mapping or return to the head.
	 *
	 * WARNING: assumes that queue_t is the 1st element of a hash entry.
	 *	    (currently it is the only element).
	 */
	ldw	VTOP_LINKN(r24),r1

$tlbdt_loop
	comb,=,n r24,r1,$dirtyfault
	ldw     HP700_MAP_OFFSET(r1),r25
	comb,<>,n r9,r25,$tlbdt_loop
	ldw	HP700_MAP_HLINKN(r1),r1
	ldw     HP700_MAP_SPACE(r1),r16
	comb,<>,n r8,r16,$tlbdt_loop
	ldw	HP700_MAP_HLINKN(r1),r1

	/*
	 * r1 now points to the mapping for this page.
	 */

	/*
	 * Find entry in the physical to virtual table for this address.
	 *
	 * WARNING: assumes 16-byte phys_table entries.
	 */
	mfctl	ptov,r16
	ldw	HP700_MAP_PAGE(r1),r25
	shd	r0,r25,1,r25
	add	r25,r16,r25

	/*
	 * Set the dirty bit for this physical page. Also check for the
	 * TLB_ALIGNED bit to see if we need to worry about flushing
	 * the cache. This bit is usually set, so optimize for the it
	 * by continuing straight through to the TLB insertion code.
	 */
	ldw	PTOV_TLBPROT(r25),r16
	depi	1,TLB_DIRTY_POS,1,r16
	bb,>=	r16,TLB_ALIGNED_POS,$tlbdt_doflush
	stw	r16,PTOV_TLBPROT(r25)			/* Not nullified */

$tlbdt_insert
	/*
	 * Do the TLB insertion. 
	 */
	ldw	HP700_MAP_PROT(r1),r25
	ldw	HP700_MAP_PAGE(r1),r16

	/*
	 * set the dirty bit for this mapping 
 	 */
	depi	1,TLB_DIRTY_POS,1,r25
	stw	r25,HP700_MAP_PROT(r1)

	/*
	 * Load the TLB.
	 */
	idtlba  r16,(sr1,r9)
	idtlbp  r25,(sr1,r9)

	/*
	 * Restore registers which will not be restored by rfir
	 */
	mfctl	tr7,r16
	mtsp	r16,sr1

#ifdef	TLBSTATS
	mfctl	cr16,r24		# Final cycle count
	rsm	PSW_R,r0		# Stop the recovery counter
	ldil	L%tlbd_statbase,r25	# Load base
	ldo	R%tlbd_statbase(r25),r25# Load base
	ldw	20(r25),r8		# Get initial cycle count
	sub	r24,r8,r8		# Get the difference
	ldw	8(r25),r24		# Get stored cycle count
	add	r8,r24,r24		# and add latest value
	stw	r24,8(r25)		# and store back into stats
	ldi	4093,r24		# Initial recovery value (adjusted)
	mfctl	cr0,r8			# Get final recovery value
	sub	r24,r8,r8		# Get the difference
	ldw	16(r25),r24		# Get store instruction count
	add	r8,r24,r24		# and add latest value
	stw	r24,16(r25)		# and sore back into stats
	mtctl	r0,cr0
#endif
	rfir
	nop

$tlbdt_doflush
	/*
	 * The TLB_ALIGNED bit is not set, so we have to go through the
	 * cache flushing drill. Set the writer pointer at this point.
	 */
	stw	r1,PTOV_WRITER(r25)

	/*
	 * Now get the pointer to the first mapping for this physical page
	 */
	ldw	 PTOV_LINKN(r25),r9

	/*
	 * get the address of this queue so that we can detect the end of
	 * the queue.
	 */
	ldo	PTOV_LINKN(r25),r24

	/*
	 * Now we need to flush the caches and purge the TLB entries far all
	 * caches that have the ICACHE or DCACHE bits set and are mapped to
	 * this physical address.
	 */
$tlbdt_cache_loop

	/* 
	 * if r9 equals r24 then no more mappings 
	 */
	comb,=,n r9,r24,$tlbdt_flushdone

	/* 
	 * if this is us, don't flush cache 
	 */
	comb,=,n r9,r1,$tlbdt_cache_loop
	ldw	HP700_MAP_PLINKN(r9),r9

#ifdef	TLBSTATS
	ldil	L%tlbd_statbase,r25
	ldo	R%tlbd_statbase(r25),r25
	ldw	4(r25),r16
	ldo	1(r16),r16
	stw	r16,4(r25)
#endif

	/*
	 * load the protection into r7 one and for all
	 */
	ldw	HP700_MAP_SW(r9),r17

	/*
	 * load the space for this mapping once and for all
	 */
	ldw	HP700_MAP_SPACE(r9),r16

	/*
	 * if the data cache bit DCACHE isn't set then we don't need to 
	 * flush the data cache. Instead jump to the data TLB purge code
	 */
	bb,>=	r17,TLB_DCACHE_POS,$tlbdt_purgedtlb	
	mtsp	r16,sr1		/* Moved into delay slot from above.  */

	/*
	 * OK, we have to flush the data cache. We need to make sure that the 
	 * TLB entry for this data page is valid.
	 */
	ldw	HP700_MAP_OFFSET(r9),r25
	ldw	HP700_MAP_PAGE(r9),r16
	idtlba	r16,(sr1,r25)
	ldw	HP700_MAP_PROT(r9),r16
	idtlbp	r16,(sr1,r25)

	/*
 	 * get the proper data cache stride into r2. 
	 */
	ldil    L%dcache_stride,r8 
	ldw     R%dcache_stride(r8),r8

	/*
	 * get the last byte of the page to flush into r4
	 */
	ldo	HP700_PGBYTES(r25),r16
	
	/*
	 * turn on virtual addressing of data and flush data cache
	 */
	ssm	PSW_D,r0
	fdc,m   r8(sr1,r25)
$tlbdt_dflush
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	fdc,m   r8(sr1,r25)
	comb,<<,n r25,r16,$tlbdt_dflush
	fdc,m   r8(sr1,r25)
	sync

	/*
	 * turn off virtual addressing of data
	 */
	rsm	PSW_D,r0

	/*
	 * OK, the cache has been flushed, Clear the DCACHE bit to indicate 
	 * that the data cache has been flushed.
	 */
	depi	0,TLB_DCACHE_POS,1,r17

$tlbdt_purgedtlb
	/*
	 * Purge the data TLB.
	 */
	ldw	HP700_MAP_OFFSET(r9),r25
	pdtlb	0(sr1,r25)

$tlbdt_checkicache

	/*
	 * if the instruction cache bit ICACHE isn't set then we don't need to 
	 * flush the instruction cache. Instead jump to the instruction TLB 
	 * purge code
	 */
	bb,>=,n	r17,TLB_ICACHE_POS,$tlbdt_purgeitlb
	
	/*
	 * OK, we have to flush the instruction cache. We need to make sure 
	 * that the TLB entry for this instruction page is valid.
	 */
	ldw	HP700_MAP_OFFSET(r9),r25
	ldw	HP700_MAP_PAGE(r9),r16
	idtlba	r16,(sr1,r25)
	ldw	HP700_MAP_PROT(r9),r16
	idtlbp	r16,(sr1,r25)

	/*
 	 * get the proper instruction cache stride into r8. 
	 */
	ldil    L%icache_stride,r8 
	ldw     R%icache_stride(r8),r8

	/*
	 * get the last byte to flush into r16
	 */
	ldo	HP700_PGBYTES(r25),r16

	/*
	 * turn on virtual addressing of data, this causes virtual instructions
	 * to be flushed.
	 */
	ssm	PSW_D,r0
	fic,m   r8(sr1,r25)
$tlbdt_iflush
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	fic,m   r8(sr1,r25)
	comb,<<,n r25,r16,$tlbdt_iflush
	fic,m   r8(sr1,r25)
	sync

	/*
	 * turn off virtual addressing of data
	 */
	rsm	PSW_D,r0

	/*
	 * remove the data TLB entry just used
	 */
	ldw	HP700_MAP_OFFSET(r9),r25
	pdtlb	0(sr1,r25)

	/*
	 * OK, the cache has been flushed, Clear the ICACHE bit to indicate 
	 * that the instruction cache has been flushed.
	 */
	depi	0,TLB_ICACHE_POS,1,r17

$tlbdt_purgeitlb
	
	/*
	 * Purge the data TLB.
	 */
	ldw	HP700_MAP_OFFSET(r9),r25
	pitlb	0(sr1,r25)

$tlbdt_cleanup

	/*
	 * save the new protection
	 */
	stw	r17,HP700_MAP_SW(r9)

	/*
	 * get the next mapping and do it
	 */
	b	$tlbdt_cache_loop
	ldw	HP700_MAP_PLINKN(r9),r9

$tlbdt_flushdone
	/*
	 * Load necessary values and branch back to insert code.
	 */
	ldw	HP700_MAP_SPACE(r1),r8
	ldw	HP700_MAP_OFFSET(r1),r9
	b	$tlbdt_insert
	mtsp	r8,sr1

$dirtyfault

	/*
	 * restore sr1 which we saved into tr7.  The rest of the registers
	 * are restored by the rfir in thandler or possibly ihandler.
	 */
	mfctl	tr7,r25
	mtsp	r25,sr1

	THANDLER(I_TLB_DIRTY)

/*
 * miscellaneous routines
 */

/*
 * ffset
 * 
 * ffset(bit_mask)
 *      u_int bit_mask#
 *
 * This routine returns the bit position of the most significant bit
 * set in bit_mask. Bit 0 is the most significant bit and bit 31 is
 * the least significant bit.
 * The routine uses a binary search to locate the highest priority bit.
 * If no bits are set in bit_mask then the routine returns -1.
 */
	.export ffset,entry
	.proc
	.callinfo 
ffset

	.entry
	/*
	 * are there any bits set?
	 */
        comb,=,n r0,arg0,$ffset_nobits

	/*
	 * initialize the bit position 31
	 */
        ldi     31,ret0

	/*
	 * extract the left half and test if zero, if zero then shift left
	 * 16 bits but don't subtract from ret0. If left half is not zero
	 * then subtract 16 from the ret0
	 */
        extru,<> arg0,15,16,r0
        shd,tr 	arg0,r0,16,arg0
        addi	-16,ret0,ret0
        extru,<> arg0,7,8,r0
        shd,tr 	arg0,r0,24,arg0
        addi    -8,ret0,ret0
        extru,<> arg0,3,4,r0
        shd,tr	arg0,r0,28,arg0
        addi    -4,ret0,ret0
        extru,<> arg0,1,2,r0
        shd,tr	arg0,r0,30,arg0
        addi    -2,ret0,ret0
        extru,= arg0,0,1,r0
        addi    -1,ret0,ret0
        bv,n	0(r2)

$ffset_nobits
	bv	0(r2)
        ldi    	-1,ret0
	.exit
        .procend


/*
 * ffs
 * 
 * ffs(bit_mask)
 *      u_int bit_mask#
 *
 * Return the position of the "most significant" bit in `bitmask'.
 * Since this is similar to the VAX ffs instruction, bits in a word
 * are numbered as "32, 31, ... 1", 0 is returned if no bits are set.
 */
	.export ffs,entry
	.proc
	.callinfo
ffs
	.entry
	movb,=,n arg0,ret0,$ffsexit	# If arg0 is 0 return 0
	ldi	32,ret0			# Set return to high bit
	extru,=	arg0,31,16,r0		# If low 16 bits are non-zero
	addi,tr	-16,ret0,ret0		#   subtract 16 from bitpos
	shd	r0,arg0,16,arg0		# else shift right 16 bits
	extru,=	arg0,31,8,r0		# If low 8 bits are non-zero
	addi,tr	-8,ret0,ret0		#   subtract 8 from bitpos
	shd	r0,arg0,8,arg0		# else shift right 8 bits
	extru,=	arg0,31,4,r0		# If low 4 bits are non-zero
	addi,tr	-4,ret0,ret0		#   subtract 4 from bitpos
	shd	r0,arg0,4,arg0		# else shift right 4 bits
	extru,=	arg0,31,2,r0		# If low 2 bits are non-zero
	addi,tr	-2,ret0,ret0		#   subtract 2 from bitpos
	shd	r0,arg0,2,arg0		# else shift right 2 bits
	extru,=	arg0,31,1,r0		# If low bit is non-zero
	addi	-1,ret0,ret0		#   subtract 1 from bitpos
$ffsexit
	bv,n	0(r2)
	.exit
        .procend

#if MACH_RT	
	.export $locore_end
$locore_end	
#endif
		
/*
 * void
 * fprinit(version)
 *	int *version;
 * 
 *
 * Initialize the floating point registers 
 */
	.export	fprinit,entry
	.proc
	.callinfo
fprinit
	.entry
	stw	r0,0(arg0)
	copr,0,0
	fstws	fr0,0(arg0)
	ldil	L%fpu_zero,t1
	ldo	R%fpu_zero(t1),t1
	fldds	0(t1),fr0
	fldds	0(t1),fr1
	fldds	0(t1),fr2
	bv	0(r2)
	fldds	0(t1),fr3
	.exit
	.procend


#ifdef TIMEX_BUG
/*
 * int
 * getfpversion()
 * 
 *
 * Return the global fpcopr_version to timex_fix because dp is not set up
 * when timex_fix is called.
 */
	.import fpcopr_version,data
	.export	getfpversion,entry
	.proc
	.callinfo

getfpversion
	.entry
	ldil	L%fpcopr_version,ret1
	ldo	R%fpcopr_version(ret1),ret1
	bv	0(r2)
	ldw	0(ret1),ret0
	.exit
	.procend
#endif /* TIMEX_BUG */

/*
 * pdc_iodc(function, pdc_flag, arg0, arg1, arg2...arg12)
 *
 * This call invokes a pdc/iodc function
 */
	.export	pdc_iodc,entry
	.proc
	.callinfo
pdc_iodc
	.entry
	/*
	 * Now generate a magic break into the kernel to do PDC calls
	 * We need to move the arguments (up to 12 of them) from the
	 * current stack into an equiv mapped area. Use part of the
	 * tmp_saved_state. The pdc_break code will then recover them.
 	 */
	ldil	L%tmp_saved_state+SS_SIZE,t1
	ldo	R%tmp_saved_state+SS_SIZE(t1),t1
	
	ldw	VA_ARG4(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG5(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG6(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG7(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG8(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG9(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG10(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG11(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG12(sp),t2
	stwm	t2,4(t1)
	ldw	VA_ARG13(sp),t2
	stwm	t2,4(t1)

	break	BREAK_KERNEL, BREAK_PDC_CALL
	nop
	nop
	bv	r0(rp)
	nop
	.exit
	.procend

/*
 * fastboot(interactive)
 *
 *	interactive - 0 if not interactive, 1 if interactive.
 *
 * Prepare for pdcboot() to load boot code (which can be no larger than
 * 256k-bytes) at LDBOOTADDR.  We are (probably) called in virtual mode
 * so we first go real.  Our dp and stack are also reinitialized.  This
 * routine will never return (nor can we go virtual again); if it isnt
 * obvious yet, we randomly trash kernel memory starting at LDBOOTADDR!
 *
 */
#define	LDBOOTADDR	0x300000
#define	BTSTKADDR	0x340000

	.IMPORT	pdcboot
	.PROC
	.CALLINFO
	.ENTRY
fastboot
	mtsm	r0				# Disable traps and interrupts.

	ldil	L%$global$,dp			# Initialize our data pointer.
	ldo	R%$global$(dp),dp

	ldi	-1,r19
	mtctl	r19,eirr			# Clear any pending interrupts.
	mtctl	r0,eiem
	addil	L%$ivaaddr-$global$,dp
	ldo	R%$ivaaddr-$global$(r1),r19
	mtctl	r19,iva				# Init interrupt vector addr.

	mtctl	r0,ccr				# Disable coprocessors and
	mtctl	r0,rctr				#   clear other ctrl regs.
	mtctl	r0,pidr1
	mtctl	r0,pidr2
	mtctl	r0,pidr3
	mtctl	r0,pidr4
	mtctl	r0,itmr
	mtctl	r0,sar

	ldil	L%LDBOOTADDR,arg1		# Set up location to load boot.
	ldo	R%LDBOOTADDR(arg1),arg1

	ldil	L%BTSTKADDR,sp			# Set up a stack (256k out)
	ldo	R%BTSTKADDR(sp),sp

	mtctl	r0,pcsq				# Clear two-level IIA Sp Queue
	mtctl	r0,pcsq				#   setting kernel space.
	addil	L%pdcboot-$global$,dp
	ldo	R%pdcboot-$global$(r1),r19	# Load pdcboot() entry pt and
	mtctl	r19,pcoq			#   stuff into head of IIA Off
	ldo	4(r19),r19			#   Queue, and entry pt + 4 in
	mtctl	r19,pcoq			#   tail of IIA Offset Queue.
	ldi	0x8,r19
	mtctl	r19,ipsw			# Set PSW Q bit, and exec...
	.EXIT
	rfi					# pdcboot(interactive,btaddr).
	.PROCEND
	.EXPORT	fastboot,ENTRY

#undef	LDBOOTADDR
#undef	BTSTKADDR

/*
 * kernel_trace()
 * 
 * Turn on/off kernel trace mode
 */
	.export	kernel_trace,entry
	.proc
	.callinfo
kernel_trace
	.entry
	nop

	/*
	 * we need to have an active thread before we try and trace
	 */
	.import cpu_data,data
	ldil	L%cpu_data,t1
	ldw	R%cpu_data(t1),t1
	subi,=	0,t1,r0
	break	BREAK_KERNEL, BREAK_KERNTRACE
	nop
	bv,n	0(rp)
	.exit
	.procend

#if MACH_KDB
	/*
	 * ddb support. Trap into kernel and call ddb stub routine.
	 */

	.export	ddb_break, entry
	.proc
	.callinfo
ddb_break
	.entry
	break	BREAK_KERNEL, BREAK_MACH_DEBUGGER
	nop
	nop
	bv,n	0(rp)
	.exit
	.procend
#endif

/*
 * Routines for getting stack traces after a panic.
 */

	/* return current stack pointer */
getsp	.proc
        .export getfp,entry
	.callinfo
	bv	0(r2)
	or	sp,r0,ret0		
	.procend
	.export	getsp,entry

        /* return current frame pointer */
getfp   .proc
        .callinfo
	bv	0(r2)
#if KGDB	
        or      r3,r0,ret0
#else
	or	r0,r0,ret0
#endif			
        .procend


	/* Return current pc */
#if ETAP
	.export etap_get_pc,entry
etap_get_pc
#endif
getpc	.proc
	.export	getpc,entry
	.callinfo
	bv	0(r2)
	or	rp,r0,ret0		# return current pc
	.procend


#if 0
	/* Return addr of $UNWIND_START */

getsun	.proc
	.callinfo
	.import	$UNWIND_START,data
	.entry
	ldil	l%$UNWIND_START,ret0	
	bv	0(r2)
	ldo	R%$UNWIND_START(ret0),ret0
	.exit
	.procend
	.export	getsun,entry

	/* Return addr of $UNWIND_END */

geteun	.proc
	.callinfo
	.import	$UNWIND_END,data
	.entry
	ldil	L%$UNWIND_END,ret0	
	bv	0(r2)
	ldo	R%$UNWIND_END(ret0),ret0
	.exit
	.procend
	.export	geteun,entry
#endif /* 0 */
	
#define CFSIZE  48	/* sizeof(struct frame_marker) + NUMARGREGS*4 */

dumpstuff
	.WORD 0
	.WORD 0
	.WORD 0

/*
 * The Transfer Of Control in Page Zero is set to `crashdump'.
 * If we receive a HPMC or LPMC, we land at `crashdump+4' (i.e. the
 * first instruction will be nullified).
 */
	.export	crashdump,entry
	.import	rpb,DATA
	.import	panicstr,DATA
	.import	dotocdump,DATA
crashdump
	ldo	R%tocstr(r0),arg0	# This was a Transfer Of Control.

	ldil	L%dotocdump,r1
	ldw	R%dotocdump(r1),r25	# Do we want to do a crash dump?
	sub,<>	r25,r0,r0		# if (dotocdump == 0) then
	b,n	busreset		#   goto busreset (no crash dump).

	ldil	L%hpmc_cksum,r1		# Clear HPMC checksum in case we
	stw	r0,R%hpmc_cksum(r1)	#   get another one!

	ldil	L%tocstr,r1
	add	arg0,r1,arg0		# Get addr of panic string into arg0,
	ldil	L%panicstr,r1		#   and store at panicstr.
	stw	arg0,R%panicstr(r1)

	ldil	L%$global$,dp		# Reinitialize our data pointer.
	ldo	R%$global$(dp),dp

	ldil	L%$ivaaddr,r1		# Retrieve our IVA (page aligned) and
	mtctl	r1,iva			#   reinitialize our IVA.

	ldil	L%rpb,r25		# Set r25 to addr of rpb and
	b	doacrashdump		#   jump into doadump().
	ldo	R%rpb(r25),r25

	break	0,0			# doacrashdump never returns.

/*
 * Do a dump.
 * Called by auto-restart.
 * This routine never returns.
 */
	.export	doadump,entry
	.import pdcdump,code
	.import splx,code
doadump
	.proc
	.callinfo caller
	.entry
	ldo	48(sp),sp

	/*
	 * Steal a couple registers by saving their values to the "fixed
	 * argument save area".  We must restore them before making a
	 * procedure call or they could get lost.  However, since this
	 * is doadump() we simply restore each one before writing it to
	 * the Restart Parameter Block.
	 */
	stw	r26,-36(sp)		# r26 (arg0) = generic temp register.
	stw	r25,-40(sp)		# r25 (arg1) = ptr to rpb struct.
	stw	r24,-44(sp)		# r24 (arg2) = 1:doadump 2:crashdump

	ldil	L%rpb,r25
	ldo	R%rpb(r25),r25		# Set r25 to addr of rpb.

	ldw	RP_FLAG(r25),r24	# Load rpb.rp_flag into r24.
	comb,<>,n r24,r0,busreset	# If rp_flag is set, abort the dump.
	addi,tr	RPB_DUMP,r0,r24		# Set r24 for a normal dump.

doacrashdump
	ldi	RPB_CRASH,r24		# Set r24 for a crash dump.

	/*
	 * There are now two possibilites.  If r24 is RPB_DUMP, this is
	 * a normal dump and we create our RPB from the current registers.
	 * If r24 is RPB_CRASH, this is a crash dump and we must do a
	 * great deal more (see below).
	 */
	bb,<	r24,RPB_B_CRASH,crash	# Is this a crash dump?
	stw	r24,RP_FLAG(r25)	# Save rpb.rp_flag.

	/*
	 * This is a normal dump, lets construct our rpb.
	 */
	stw	sp,RP_SP(r25)		# Save stack pointer (for HP-UX dbgr).
	ldo	RP_PIM(r25),r25		# Set r25 to addr of rpb.rp_pim.

	stwm	r0,4(r25)		# Save 32 general registers.
	stwm	r1,4(r25)
	stwm	r2,4(r25)
	stwm	r3,4(r25)
	stwm	r4,4(r25)
	stwm	r5,4(r25)
	stwm	r6,4(r25)
	stwm	r7,4(r25)
	stwm	r8,4(r25)
	stwm	r9,4(r25)
	stwm	r10,4(r25)
	stwm	r11,4(r25)
	stwm	r12,4(r25)
	stwm	r13,4(r25)
	stwm	r14,4(r25)
	stwm	r15,4(r25)
	stwm	r16,4(r25)
	stwm	r17,4(r25)
	stwm	r18,4(r25)
	stwm	r19,4(r25)
	stwm	r20,4(r25)
	stwm	r21,4(r25)
	stwm	r22,4(r25)
	stwm	r23,4(r25)
	ldw	-44(sp),r26		# Restore orig r24 into r26
	stwm	r26,4(r25)		#   and save
	ldw	-40(sp),r26		# Restore orig r25 into r26
	stwm	r26,4(r25)		#   and save.
	ldw	-36(sp),r26		# Restore orig r26
	stwm	r26,4(r25)		#   and save.
	stwm	r27,4(r25)
	stwm	r28,4(r25)
	stwm	r29,4(r25)
	stwm	r30,4(r25)
	stwm	r31,4(r25)

	mfctl	cr0,r26			# Save 32 control registers,
	stwm	r26,32(r25)		#   excepting cr1-7 (reserved).
	mfctl	cr8,r26
	stwm	r26,4(r25)
	mfctl	cr9,r26
	stwm	r26,4(r25)
	mfctl	cr10,r26
	stwm	r26,4(r25)
	mfctl	cr11,r26
	stwm	r26,4(r25)
	mfctl	cr12,r26
	stwm	r26,4(r25)
	mfctl	cr13,r26
	stwm	r26,4(r25)
	mfctl	cr14,r26
	stwm	r26,4(r25)
	mfctl	cr15,r26
	stwm	r26,4(r25)
	mfctl	cr16,r26
	stwm	r26,4(r25)
	mfctl	cr17,r26
	stwm	r26,4(r25)
	mfctl	cr18,r26
	stwm	r26,4(r25)
	mfctl	cr19,r26
	stwm	r26,4(r25)
	mfctl	cr20,r26
	stwm	r26,4(r25)
	mfctl	cr21,r26
	stwm	r26,4(r25)
	mfctl	cr22,r26
	stwm	r26,4(r25)
	mfctl	cr23,r26
	stwm	r26,4(r25)
	mfctl	cr24,r26
	stwm	r26,4(r25)
	mfctl	cr25,r26
	stwm	r26,4(r25)
	mfctl	cr26,r26
	stwm	r26,4(r25)
	mfctl	cr27,r26
	stwm	r26,4(r25)
	mfctl	cr28,r26
	stwm	r26,4(r25)
	mfctl	cr29,r26
	stwm	r26,4(r25)
	mfctl	cr30,r26
	stwm	r26,4(r25)
	mfctl	cr31,r26
	stwm	r26,4(r25)

	mfsp	sr0,r26			# Save 8 space registers.
	stwm	r26,4(r25)
	mfsp	sr1,r26
	stwm	r26,4(r25)
	mfsp	sr2,r26
	stwm	r26,4(r25)
	mfsp	sr3,r26
	stwm	r26,4(r25)
	mfsp	sr4,r26
	stwm	r26,4(r25)
	mfsp	sr5,r26
	stwm	r26,4(r25)
	mfsp	sr6,r26
	stwm	r26,4(r25)
	mfsp	sr7,r26
	stwm	r26,4(r25)

crash
	/*
	 * It's time to switch to `dumpstk'.  Our initial stack frame
	 * will look like:
	 *
	 *	dumpstk		+---------------------------+
	 *			|     PDC return block      |
	 *	dumpstk+128	+---------------------------+
	 *			|       5th Argument        |
	 *			|       4th Argument        |
	 *	dumpstk+136	+---------------------------+
	 *			| Fixed Args + Frame Marker |
	 *	sp		+---------------------------+
	 */
	ldil	L%dumpstk,r26
	ldo	R%dumpstk(r26),sp	# We have a new stack!

	/*
	 * We are now going to lose our temp registers to other routines.
	 * Let's move the values they contain to more stable registers.
	 * `r3' and `r4' are only used/valid when doing a crash dump.
	 */
	or	sp,r0,r3		# Crash: r3 = orig sp (for PDC arg).
	or	r25,r0,r4		# Crash: r4 = RPB addr (for PDC arg).
	or	r24,r0,r5		# r5 = type of dump (rpb.rp_flag).
	ldo	CFSIZE+136(sp),sp	# Alloc frame (8+128 for PDCarg+PDCrtn)

	/*
	 * Turn the Q bit back on, set the M bit to avoid another HPMC,
	 * but leave everything else off.
         */
	rsm	RESET_PSW,r0
	mtctl	r0,pcsq
	mtctl	r0,pcsq

	ldil	L%$crashqon,r1
	ldo	R%$crashqon(r1),r1

	mtctl	r1,pcoq	
	ldo	4(r1),r1
	mtctl	r1,pcoq	

	ldil	L%PSW_Q|PSW_M,r1
	ldo	R%PSW_Q|PSW_M(r1),r1
	mtctl	r1,ipsw

	rfi
	nop

$crashqon
	bb,<,n	r5,RPB_B_DUMP,dump	# Is this a normal doadump?

	/*
	 * This was a crash dump.  Since a variety of things could be
	 * screwed up, we take no chances.  Flush our I and D Cache,
	 * reconfigure the bus, and finally read the PDC PIM information
	 * into our Restart Parameter Block.
	 */
	.import	fcacheall,code
	.import	busconf,code
	.import	savestk,code

	bl	fcacheall,rp		# Flush the cache.
	nop

	bl	busconf,rp		# Reconfigure the bus.
	nop

	/*
	 * Get PDC PIM information saved when HPMC, LPMC or TOC occurred.
	 * In C, this call would be the equivalent of:
	 *
	 * pdc(PDC_PIM, PDC_PIM_HPMC, &dumpstk, &rpb.rp_pim, sizeof(struct pim))
	 */
	ldi	PDC_PIM,arg0
	ldi	PDC_PIM_TOC,arg1
	or	r3,r0,arg2		# Address of PDC return block.
	ldo	RP_PIM(r4),arg3		# Address of rpb.rp_pim.
	ldi	PIMSTRUCTSIZE,r1
	stw	r1,-52(sp)		# 4th Argument goes on stack.

#ifdef HPOSFCOMPAT
	ldil	L%dumpstuff,r6
	ldo	R%dumpstuff(r6),r6
	ldi	1,r7
	stw	r7,0(r6)
#endif

	/*
	 * get the address of the PDC routine from page 0 memory and call it
	 */
	ldw	MEM_PDC(r0),r1
	.call
	blr     r0,rp
	bv,n    (r1)
	nop

#ifdef HPOSFCOMPAT
	ldil	L%dumpstuff,r6
	ldo	R%dumpstuff(r6),r6
	ldi	2,r7
	stw	r7,4(r6)
	stw	ret0,8(r6)
#endif

dump
#ifdef PDCDUMP
	bl	pdcdump,rp		# Do the dump (finally),
	stw	r0,MEM_TOC(r0)		#   and clear TOC to prevent recursion.
#endif PDCDUMP


busreset				# Reset the bus (HARD boot).
	ldi	CMD_RESET,r26
	ldil	L%LBCAST_ADDR,r25
	stw	r26,R%HPA_IOCMD(r25)
forever					# Loop until bus reset takes effect.
	b,n	forever

	bv	0(rp)
	ldo	-48(sp),sp
	.exit
	.procend

/*
 * setbit(int bitno, int *s) - set bit in bit string
 */
        .export setbit,entry
        .proc
        .callinfo

setbit
	ldil	l%31,t4	
	ldo	r%31(t4),t4

$loop
	combf,<,n t4,arg0,$cont
	addi	  -32,arg0,arg0
        b    	  $loop
	addi 	  4,arg1,arg1

$cont
	mtsar   arg0
	zvdepi	1,32,t4

	ldw	0(arg1),t3
	or	t3,t4,t3
        bv      0(rp)
	stw	t3,0(arg1)

        .procend

/*
 * clrbit(int bitno, int *s) - clear bit in bit string
 */
	.export clrbit,entry
	.proc
	.callinfo

clrbit
	ldil	l%31,t4	
	ldo	r%31(t4),t4

$loop1
	combf,<,n t4,arg0,$cont1
	addi	  -32,arg0,arg0
        b     	  $loop1
	addi 	  4,arg1,arg1 

$cont1
	mtsar	arg0
	zvdepi	1,32,t4
	addi	-1,r0,t3
	xor	t4,t3,t4

	ldw	0(arg1),t3
	and	t3,t4,t3
        bv      0(rp)
	stw	t3,0(arg1)

        .procend
	
/*
 * ffsbit(int *s) - find first set bit in bit string
 */
	.export ffsbit,entry
	.proc
	.callinfo

ffsbit
	copy r0,ret0

$f
	ldw	   0(arg0),t3
	combt,<<,n r0,t3,$c
	addi	   32,ret0,ret0
        b    	   $f
	addi 	   4,arg0,arg0 

$c
        mtsar ret0

	bvb,>=,n t3,$c
	addi	1,ret0,ret0

        bv,n      0(rp)

        .procend
	
/*
 * getrpc() - Return address of the function that called current function
 */
	.export getrpc
getrpc	.proc
	.callinfo

#if KGDB
	/* check if r3 (previous frame) points into the stack */
	combt,<<,n sp,r3,getrpc1
	ldil	L%KERNEL_STACK_SIZE-1,t1
	ldo	R%KERNEL_STACK_SIZE-1(t1),t1
	andcm	sp,t1,t1
	combt,<<,n r3,t1,getrpc1

	bv	0(rp)
	/* r3 points to previous frame */
	ldw	FM_CRP(r3),ret0
getrpc1
#endif /* KGDB */
	bv	0(rp)
	/* sp points to previous frame */
	ldw	FM_CRP(sp),ret0
		
	.procend

#ifndef __ELF__
/* Leave this down here.  It avoids a nasty, hard-to-fix bug in gas.  */
	.space $PRIVATE$
	.subspa $GLOBAL$
#endif /* __ELF__ */
	
	.export $global$,data
$global$

	.end


