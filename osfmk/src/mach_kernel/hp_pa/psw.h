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
 *  (c) Copyright 1987 HEWLETT-PACKARD COMPANY
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
 * Copyright (c) 1990,1991,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: psw.h 1.3 94/12/14$
 */

#ifndef _MACHINE_PSW_H_
#define _MACHINE_PSW_H_

/*
 * Processor Status Word Masks
 */

#define	PSW_T	0x01000000	/* Taken Branch Trap Enable */
#define	PSW_H	0x00800000	/* Higher-Privilege Transfer Trap Enable */
#define	PSW_L	0x00400000	/* Lower-Privilege Transfer Trap Enable */
#define	PSW_N	0x00200000	/* PC Queue Front Instruction Nullified */
#define	PSW_X	0x00100000	/* Data Memory Break Disable */
#define	PSW_B	0x00080000	/* Taken Branch in Previous Cycle */
#define	PSW_C	0x00040000	/* Code Address Translation Enable */
#define	PSW_V	0x00020000	/* Divide Step Correction */
#define	PSW_M	0x00010000	/* High-Priority Machine Check Disable */
#define	PSW_CB	0x0000ff00	/* Carry/Borrow Bits */
#define	PSW_R	0x00000010	/* Recovery Counter Enable */
#define	PSW_Q	0x00000008	/* Interruption State Collection Enable */
#define	PSW_P	0x00000004	/* Protection ID Validation Enable */
#define	PSW_D	0x00000002	/* Data Address Translation Enable */
#define	PSW_I	0x00000001	/* External, Power Failure, Low-Priority */
				/* Machine Check Interruption Enable */

/*
 * PSW Bit Positions
 */

#define	PSW_T_P	7
#define	PSW_H_P	8
#define	PSW_L_P	9
#define	PSW_N_P	10
#define	PSW_X_P	11
#define	PSW_B_P	12
#define	PSW_C_P	13
#define	PSW_V_P	14
#define	PSW_M_P	15
#define	PSW_CB_P	23
#define	PSW_R_P	27
#define	PSW_Q_P	28
#define	PSW_P_P	29
#define	PSW_D_P	30
#define	PSW_I_P	31

/*
 * Kernel PSW Masks
 */

#define KERNEL_PSW	(PSW_C | PSW_Q | PSW_P | PSW_D)
#define RESET_PSW       (PSW_R | PSW_Q | PSW_P | PSW_D | PSW_I)
/*
 * Decode privilege levels
 */

#define PC_PRIV_MASK    3
#define PC_PRIV_KERN    0
#define PC_PRIV_USER    3

#define USERMODE(ssp) \
	((((ssp)->iioq_head) & PC_PRIV_MASK) != PC_PRIV_KERN)

#define USERMODE_PC(pc) (((pc) & PC_PRIV_MASK) != PC_PRIV_KERN)

/* bits that must be set/reset in user space */
#define PSW_USER_SET	(PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I)
#define PSW_USER_RESET	(PSW_T | PSW_M | PSW_R )

#define PSW_SUPER	(PSW_C | PSW_Q | PSW_D | PSW_I)

#endif  /* _MACHINE_PSW_H_ */
