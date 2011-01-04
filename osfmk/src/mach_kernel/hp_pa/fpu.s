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
 * Copyright (c) 1990,1991,1992,1993,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: fpu.s 1.3 94/12/14$
 *	Author: Bob Wheeler & Leigh Stoller of University of Utah CSL
 */

#include <machine/asm.h>
#include <assym.s>
#include <hp_pa/thread.h>
	
	.space	$PRIVATE$
	.subspa	$DATA$

	.import active_pcbs
	.import	fpu_pcb
	.import	fpu_zero


	.space $TEXT$
	.subspa $CODE$

/*
 * void fpu_switch()
 *
 * Called only from the trap table in locore.s.
 */
	.export	fpu_switch,entry
	.proc
	.callinfo
fpu_switch
	.entry
	mtctl   t1,tr3
	mtctl   t2,tr4
	mtctl   t3,tr5

#ifdef MACH_COUNTERS
	.import fpu_trap_count,data
	ldil	L%fpu_trap_count,t1
	ldw	R%fpu_trap_count(t1),t2
#endif

	/*
	 * Turn the FPU back on.
	 */
	/* XXX Bogus in a really BIG way */
	ldi	0xC0,t3

#ifdef MACH_COUNTERS
	/* Two lines moved down to avoid load/use stall on t2 */
	ldo	1(t2),t2
	stw	t2,R%fpu_trap_count(t1)
#endif

	ldil	L%active_pcbs,t1
	ldw	R%active_pcbs(t1),t1

	ldil	L%fpu_pcb,t2
	ldw	R%fpu_pcb(t2),t2

	mtctl	t3,ccr

	/*
	 * See if the current thread owns the FPU. If so, we just return.
	 */
	combt,=	t1,t2,$fpu_done
	ldo	PCB_SF(t2),t3		
	
	/*
	 * See if any thread owns the FPU. If not, we can skip the state save.
	 */
	combt,=,n t2,r0,$fpu_no_owner
	
	
	/*
	 * This thread wants to use the FPU (but does not own it). Save off
	 * the current FPU state into the PCB of the thread that owns it.
	 * Restore FPU state from the PCB of the current thread and make
	 * it the owner.
	 */
	fstds,ma fr0,8(t3)
	fstds,ma fr1,8(t3)
	fstds,ma fr2,8(t3)
	fstds,ma fr3,8(t3)
	fstds,ma fr4,8(t3)
	fstds,ma fr5,8(t3)
	fstds,ma fr6,8(t3)
	fstds,ma fr7,8(t3)
	fstds,ma fr8,8(t3)
	fstds,ma fr9,8(t3)
	fstds,ma fr10,8(t3)
	fstds,ma fr11,8(t3)
	fstds,ma fr12,8(t3)
	fstds,ma fr13,8(t3)
	fstds,ma fr14,8(t3)
#ifdef TIMEX
	fstds,ma fr15,8(t3) 
	.word	 0x2E901230	# fstds,ma fr16,8(t3)
	.word	 0x2E901231	# fstds,ma fr17,8(t3)
	.word	 0x2E901232	# fstds,ma fr18,8(t3)
	.word	 0x2E901233	# fstds,ma fr19,8(t3)
	.word	 0x2E901234	# fstds,ma fr20,8(t3)
	.word	 0x2E901235	# fstds,ma fr21,8(t3)
	.word	 0x2E901236	# fstds,ma fr22,8(t3)
	.word	 0x2E901237	# fstds,ma fr23,8(t3)
	.word	 0x2E901238	# fstds,ma fr24,8(t3)
	.word	 0x2E901239	# fstds,ma fr25,8(t3)
	.word	 0x2E90123a	# fstds,ma fr26,8(t3)
	.word	 0x2E90123b	# fstds,ma fr27,8(t3)
	.word	 0x2E90123c	# fstds,ma fr28,8(t3)
	.word	 0x2E90123d	# fstds,ma fr29,8(t3)
	.word	 0x2E90123e	# fstds,ma fr30,8(t3)
	.word	 0x2E80121f	# fstds    fr31,0(t3)
#else
	fstds	 fr15,(t3)
#endif /* TIMEX */

	/*
	 * Restore FPU State
	 */
$fpu_no_owner
	ldw	PCB_SS+SS_FPU(t1),t2
	combf,= r0,t2,$fpu_load	
#ifdef TIMEX
	ldo	 PCB_SF+SF_FR31(t1),t3
#else
	ldo	 PCB_SF+SF_FR15(t1),t3
#endif /* TIMEX */

	ldil	L%fpu_zero,t3
	ldo	R%fpu_zero(t3),t3
#ifdef TIMEX
	.word	 0x2e80101f	# fldds (t3),fr31
	.word    0x2e80101e	# fldds (t3),fr30
	.word    0x2e80101d	# fldds (t3),fr29
	.word    0x2e80101c	# fldds (t3),fr28
	.word    0x2e80101b	# fldds (t3),fr27
	.word    0x2e80101a	# fldds (t3),fr26
	.word    0x2e801019	# fldds (t3),fr25
	.word    0x2e801018	# fldds (t3),fr24
	.word    0x2e801017	# fldds (t3),fr23
	.word    0x2e801016	# fldds (t3),fr22
	.word    0x2e801015	# fldds (t3),fr21
	.word    0x2e801014	# fldds (t3),fr20
	.word    0x2e801013	# fldds (t3),fr19
	.word    0x2e801012	# fldds (t3),fr18
	.word    0x2e801011	# fldds (t3),fr17
	.word    0x2e801010	# fldds (t3),fr16
#endif /* TIMEX */
	fldds	0(t3),fr15
	fldds	0(t3),fr14
	fldds	0(t3),fr13
	fldds	0(t3),fr12
	fldds	0(t3),fr11
	fldds	0(t3),fr10
	fldds	0(t3),fr9
	fldds	0(t3),fr8
	fldds	0(t3),fr7
	fldds	0(t3),fr6
	fldds	0(t3),fr5
	fldds	0(t3),fr4
	fldds	0(t3),fr3
	fldds	0(t3),fr2
	fldds	0(t3),fr1
	b	$fpu_restored
	fldds	0(t3),fr0

$fpu_load
	/*
	 * The activation frame contains a valid FPU state, so load it.
	 */
#ifdef TIMEX
	.word	 0x2E91103f	# fldds,ma -8(t3),fr31
	.word    0x2E91103e	# fldds,ma -8(t3),fr30
	.word    0x2E91103d	# fldds,ma -8(t3),fr29
	.word    0x2E91103c	# fldds,ma -8(t3),fr28
	.word    0x2E91103b	# fldds,ma -8(t3),fr27
	.word    0x2E91103a	# fldds,ma -8(t3),fr26
	.word    0x2E911039	# fldds,ma -8(t3),fr25
	.word    0x2E911038	# fldds,ma -8(t3),fr24
	.word    0x2E911037	# fldds,ma -8(t3),fr23
	.word    0x2E911036	# fldds,ma -8(t3),fr22
	.word    0x2E911035	# fldds,ma -8(t3),fr21
	.word    0x2E911034	# fldds,ma -8(t3),fr20
	.word    0x2E911033	# fldds,ma -8(t3),fr19
	.word    0x2E911032	# fldds,ma -8(t3),fr18
	.word    0x2E911031	# fldds,ma -8(t3),fr17
	.word    0x2E911030	# fldds,ma -8(t3),fr16
#endif /* TIMEX */
	fldds,ma -8(t3),fr15
	fldds,ma -8(t3),fr14
	fldds,ma -8(t3),fr13
	fldds,ma -8(t3),fr12
	fldds,ma -8(t3),fr11
	fldds,ma -8(t3),fr10
	fldds,ma -8(t3),fr9
	fldds,ma -8(t3),fr8
	fldds,ma -8(t3),fr7
	fldds,ma -8(t3),fr6
	fldds,ma -8(t3),fr5
	fldds,ma -8(t3),fr4
	fldds,ma -8(t3),fr3
	fldds,ma -8(t3),fr2
	fldds,ma -8(t3),fr1
	fldds    0(t3),fr0

$fpu_restored

	ldil	L%fpu_pcb,t2
	stw	t1,R%fpu_pcb(t2)

	ldi	1,t3
	stw	t3,PCB_SS+SS_FPU(t1)	

#ifdef MACH_COUNTERS
	.import fpu_switch_count,data
	ldil	L%fpu_switch_count,t1
	ldw	R%fpu_switch_count(t1),t2
	ldo	1(t2),t2
	stw	t2,R%fpu_switch_count(t1)
#endif

$fpu_done
	/*
	 * Done, so return to user mode, and instruction that caused the trap.
	 */
	mfctl   tr2,r31
	mfctl   tr3,t1
	mfctl   tr4,t2
	mfctl   tr5,t3
	rfi
	nop

	.exit
	.procend



/*
 * void fpu_flush()
 *
 * Flushes any residual state out of the FPU and into the appropriate activation.
 */
	.export	fpu_flush,entry
	.proc
	.callinfo
fpu_flush

	.entry
	/*
	 * Find the PCB the FPU belongs to.
	 */
	ldil	L%fpu_pcb,t2
	ldw	R%fpu_pcb(t2),t2

	/*
	 * See if anyone owns the FPU. If not, we can skip the state save.
	 */
	combt,=,n	t2,r0,$fpu_save_no_owner

	/*
	 * Turn the FPU on so we can access it.
	 */
	ldi	0xC0,t3
	mtctl	t3,ccr

	ldi	1,t3
	stw	t3,PCB_SS+SS_FPU(t2)	
		
	/*
	 * Save the current FPU state into the activation that owns it.
	 */
	ldo	PCB_SF(t2),t3
	fstds,ma fr0,8(t3)
	fstds,ma fr1,8(t3)
	fstds,ma fr2,8(t3)
	fstds,ma fr3,8(t3)
	fstds,ma fr4,8(t3)
	fstds,ma fr5,8(t3)
	fstds,ma fr6,8(t3)
	fstds,ma fr7,8(t3)
	fstds,ma fr8,8(t3)
	fstds,ma fr9,8(t3)
	fstds,ma fr10,8(t3)
	fstds,ma fr11,8(t3)
	fstds,ma fr12,8(t3)
	fstds,ma fr13,8(t3)
	fstds,ma fr14,8(t3)
#ifdef TIMEX
	fstds,ma fr15,8(t3) 
	.word	 0x2E901230	# fstds,ma fr16,8(t3)
	.word	 0x2E901231	# fstds,ma fr17,8(t3)
	.word	 0x2E901232	# fstds,ma fr18,8(t3)
	.word	 0x2E901233	# fstds,ma fr19,8(t3)
	.word	 0x2E901234	# fstds,ma fr20,8(t3)
	.word	 0x2E901235	# fstds,ma fr21,8(t3)
	.word	 0x2E901236	# fstds,ma fr22,8(t3)
	.word	 0x2E901237	# fstds,ma fr23,8(t3)
	.word	 0x2E901238	# fstds,ma fr24,8(t3)
	.word	 0x2E901239	# fstds,ma fr25,8(t3)
	.word	 0x2E90123a	# fstds,ma fr26,8(t3)
	.word	 0x2E90123b	# fstds,ma fr27,8(t3)
	.word	 0x2E90123c	# fstds,ma fr28,8(t3)
	.word	 0x2E90123d	# fstds,ma fr29,8(t3)
	.word	 0x2E90123e	# fstds,ma fr30,8(t3)
	.word	 0x2E80121f	# fstds    fr31,0(t3)
#else
	fstds	 fr15,(t3)
#endif /* TIMEX */

	/*
	 * Mark the FPU as having no owner.
	 */
	ldil	L%fpu_pcb,t2
	stw	r0,R%fpu_pcb(t2)

$fpu_save_no_owner
	bv,n	0(r2)

	.exit
	.procend

	.export disable_fpu
disable_fpu
	.proc
	.callinfo

	bv	0(rp)
	mtctl	r0,ccr
	
	.procend
	
	.end
