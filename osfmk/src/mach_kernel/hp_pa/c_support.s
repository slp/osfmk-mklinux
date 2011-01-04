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
 * Copyright (c) 1990,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: c_support.s 1.8 94/12/14$
 *	Author: Bob Wheeler, University of Utah CSL
 */

/*
 * hppa routines to move to and from control registers from C
 *
 * int ldsid(p)			- load space register from pointer
 * void mtctl(reg, value)	- move to control register
 * int mfctl(reg)		- move from control register
 * int mtsp(sreg, value)	- move to space register
 * int mfsr(sreg)		- move from space register
 * int ssm(mask)		- set system mask
 * int rsm(mask)		- reset system mask
 * int mtsm(mask)		- move to system mask
 *
 */

#include <machine/asm.h>

/*
 * int ldsid(p)
 * 	vm_offset_t p;
 *
 * Get space register for an address
 */

        .space  $TEXT$
        .subspa $CODE$

        .export ldsid,entry
        .proc
        .callinfo

ldsid
	bv 0(r2)
        ldsid   (arg0),ret0
        .procend

/*
 * void mtctl(reg, value)
 *	int	reg;
 *	int	value;
 *
 * Move the specified value into the control register. The register is taken
 * modulo 32. If the register is invalid the operation is ignored.
 */

	.export	mtctl,entry
	.proc
	.callinfo
mtctl

/*
 * take the register number modulo 32
 */
	ldi	31,t1
	and	t1,arg0,arg0

/*
 * write the value to the specified register
 */

	blr,n	arg0,r0
	nop

	bv 0(r2)
	mtctl	arg1,cr0
	bv 0(r2)
	nop			/* No access to cr1 */
	bv 0(r2)
	nop			/* No access to cr2 */
	bv 0(r2)
	nop			/* No access to cr3 */
	bv 0(r2)
	nop			/* No access to cr4 */
	bv 0(r2)
	nop			/* No access to cr5 */
	bv 0(r2)
	nop			/* No access to cr6 */
	bv 0(r2)
	nop			/* No access to cr7 */
	bv 0(r2)
	mtctl	arg1,cr8
	bv 0(r2)
	mtctl	arg1,cr9
	bv 0(r2)
	mtctl	arg1,cr10
	bv 0(r2)
	mtctl	arg1,cr11
	bv 0(r2)
	mtctl	arg1,cr12
	bv 0(r2)
	mtctl	arg1,cr13
	bv 0(r2)
	mtctl	arg1,cr14
	bv 0(r2)
	mtctl	arg1,cr15
	bv 0(r2)
	mtctl	arg1,cr16
	bv 0(r2)
	mtctl	arg1,cr17
	bv 0(r2)
	mtctl	arg1,cr18
	bv 0(r2)
	mtctl	arg1,cr19
	bv 0(r2)
	mtctl	arg1,cr20
	bv 0(r2)
	mtctl	arg1,cr21
	bv 0(r2)
	mtctl	arg1,cr22
	bv 0(r2)
	mtctl	arg1,cr23
	bv 0(r2)
	mtctl	arg1,cr24
	bv 0(r2)
	mtctl	arg1,cr25
	bv 0(r2)
	mtctl	arg1,cr26
	bv 0(r2)
	mtctl	arg1,cr27
	bv 0(r2)
	mtctl	arg1,cr28
	bv 0(r2)
	mtctl	arg1,cr29
	bv 0(r2)
	mtctl	arg1,cr30
	bv 0(r2)
	mtctl	arg1,cr31

	.procend


/*
 * int mfctl(reg)
 *	int	reg;
 *
 * Return the contents of the specified control register. The register is taken
 * modulo 32. If the register is invalid the operation is ignored.
 */

	.export	mfctl,entry
	.proc
	.callinfo
mfctl	

/*
 * take the register number modulo 32
 */
	ldi	31,t1
	and	t1,arg0,arg0

/*
 * read the value from the specified register
 */

	blr,n	arg0,r0
	nop

	bv	0(r2)
	mfctl	cr0,ret0
	bv	0(r2)
	nop			# No access to cr1
	bv	0(r2)
	nop			# No access to cr2
	bv	0(r2)
	nop			# No access to cr3
	bv	0(r2)
	nop			# No access to cr4
	bv	0(r2)
	nop			# No access to cr5
	bv	0(r2)
	nop			# No access to cr6
	bv	0(r2)
	nop			# No access to cr7
	bv	0(r2)
	mfctl	cr8,ret0
	bv	0(r2)
	mfctl	cr9,ret0
	bv	0(r2)
	mfctl	cr10,ret0
	bv	0(r2)
	mfctl	cr11,ret0
	bv	0(r2)
	mfctl	cr12,ret0
	bv	0(r2)
	mfctl	cr13,ret0
	bv	0(r2)
	mfctl	cr14,ret0
	bv	0(r2)
	mfctl	cr15,ret0
	bv	0(r2)
	mfctl	cr16,ret0
	bv	0(r2)
	mfctl	cr17,ret0
	bv	0(r2)
	mfctl	cr18,ret0
	bv	0(r2)
	mfctl	cr19,ret0
	bv	0(r2)
	mfctl	cr20,ret0
	bv	0(r2)
	mfctl	cr21,ret0
	bv	0(r2)
	mfctl	cr22,ret0
	bv	0(r2)
	mfctl	cr23,ret0
	bv	0(r2)
	mfctl	cr24,ret0
	bv	0(r2)
	mfctl	cr25,ret0
	bv	0(r2)
	mfctl	cr26,ret0
	bv	0(r2)
	mfctl	cr27,ret0
	bv	0(r2)
	mfctl	cr28,ret0
	bv	0(r2)
	mfctl	cr29,ret0
	bv	0(r2)
	mfctl	cr30,ret0
	bv	0(r2)
	mfctl	cr31,ret0

	.procend


/*
 * int mtsp(sr, value)
 *	int	sr;
 *	int	value;
 *
 * Move the specified value into a space register. The space register is taken
 * modulo 8.
 */

	.export	mtsp,entry
	.proc
	.callinfo
mtsp

/*
 * take the register number modulo 8
 */
	ldi	7,t1
	and	t1,arg0,arg0

/*
 * write the value to the specified register
 */

	blr,n	arg0,r0
	nop

	bv	0(r2)
	mtsp	arg1,sr0
	bv	0(r2)
	mtsp	arg1,sr1
	bv	0(r2)
	mtsp	arg1,sr2
	bv	0(r2)
	mtsp	arg1,sr3
	bv	0(r2)
	mtsp	arg1,sr4
	bv	0(r2)
	mtsp	arg1,sr5
	bv	0(r2)
	mtsp	arg1,sr6
	bv	0(r2)
	mtsp	arg1,sr7

	.procend


/*
 * int mfsr(reg)
 *	int	reg;
 *
 * Return the contents of the specified space register. The space register is 
 * taken modulo 8.
 */

	.export	mfsp,entry
	.proc
	.callinfo
mfsp

/*
 * take the register number modulo 8
 */
	ldi	7,t1
	and	t1,arg0,arg0

/*
 * write the value to the specified register
 */

	blr,n	arg0,r0
	nop

	bv	0(r2)
	mfsp	sr0,ret0
	bv	0(r2)
	mfsp	sr1,ret0
	bv	0(r2)
	mfsp	sr2,ret0
	bv	0(r2)
	mfsp	sr3,ret0
	bv	0(r2)
	mfsp	sr4,ret0
	bv	0(r2)
	mfsp	sr5,ret0
	bv	0(r2)
	mfsp	sr6,ret0
	bv	0(r2)
	mfsp	sr7,ret0

	.procend


/*
 * int ssm(mask)
 *	int	mask;
 *
 * Set system mask. This call will not set the Q bit even if it is 
 * specified.
 *
 * Returns the old system mask
 */

	.export	ssm,entry
	.proc
	.callinfo
ssm

/*
 * look at only the lower 5 bits of the mask
 */
	ldi	31,t1
	and	t1,arg0,arg0


/*
 * Set System Mask and Return
 */

	blr,n	arg0,r0
	nop

	bv	0(r2)
	ssm	0,ret0
	bv	0(r2)
	ssm	1,ret0
	bv	0(r2)
	ssm	2,ret0
	bv	0(r2)
	ssm	3,ret0
	bv	0(r2)
	ssm	4,ret0
	bv	0(r2)
	ssm	5,ret0
	bv	0(r2)
	ssm	6,ret0
	bv	0(r2)
	ssm	7,ret0
	bv	0(r2)
	ssm	0,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	1,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	2,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	3,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	4,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	5,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	6,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	7,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	16,ret0
	bv	0(r2)
	ssm	17,ret0
	bv	0(r2)
	ssm	18,ret0
	bv	0(r2)
	ssm	19,ret0
	bv	0(r2)
	ssm	20,ret0
	bv	0(r2)
	ssm	21,ret0
	bv	0(r2)
	ssm	22,ret0
	bv	0(r2)
	ssm	23,ret0
	bv	0(r2)
	ssm	16,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	17,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	18,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	19,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	20,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	21,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	22,ret0		/* can't set Q bit with ssm */
	bv	0(r2)
	ssm	23,ret0		/* can't set Q bit with ssm */

	.procend


/*
 * int rsm(mask)
 *	int	mask;
 *
 * Reset system mask. 
 *
 * Returns the old system mask
 */

	.export	rsm,entry
	.proc
	.callinfo
rsm

/*
 * look at only the lower 5 bits of the mask
 */
	ldi	31,t1
	and	t1,arg0,arg0

/*
 * Set System Mask and Return
 */

	blr,n	arg0,r0
	nop

	bv	0(r2)
	rsm	0,ret0
	bv	0(r2)
	rsm	1,ret0
	bv	0(r2)
	rsm	2,ret0
	bv	0(r2)
	rsm	3,ret0
	bv	0(r2)
	rsm	4,ret0
	bv	0(r2)
	rsm	5,ret0
	bv	0(r2)
	rsm	6,ret0
	bv	0(r2)
	rsm	7,ret0
	bv	0(r2)
	rsm	8,ret0
	bv	0(r2)
	rsm	9,ret0
	bv	0(r2)
	rsm	10,ret0
	bv	0(r2)
	rsm	11,ret0
	bv	0(r2)
	rsm	12,ret0
	bv	0(r2)
	rsm	13,ret0
	bv	0(r2)
	rsm	14,ret0
	bv	0(r2)
	rsm	15,ret0
	bv	0(r2)
	rsm	16,ret0
	bv	0(r2)
	rsm	17,ret0
	bv	0(r2)
	rsm	18,ret0
	bv	0(r2)
	rsm	19,ret0
	bv	0(r2)
	rsm	20,ret0
	bv	0(r2)
	rsm	21,ret0
	bv	0(r2)
	rsm	22,ret0
	bv	0(r2)
	rsm	23,ret0
	bv	0(r2)
	rsm	24,ret0
	bv	0(r2)
	rsm	25,ret0
	bv	0(r2)
	rsm	26,ret0
	bv	0(r2)
	rsm	27,ret0
	bv	0(r2)
	rsm	28,ret0
	bv	0(r2)
	rsm	29,ret0
	bv	0(r2)
	rsm	30,ret0
	bv	0(r2)
	rsm	31,ret0

	.procend


/*
 * int mtsm(mask)
 *	int mask;
 *
 * Move to system mask. Old value of system mask is returned.
 */

	.export	mtsm,entry
	.proc
	.callinfo
mtsm

/* 
 * Move System Mask and Return
 */
	ssm	0,ret0
	bv	0(r2)
	mtsm	arg0

	.procend
