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
 * Copyright (c) 1990 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: regs.h 1.5 91/09/25$
 *	Author: Bob Wheeler, University of Utah CSS
 */

/*
 * constants for registers for use with the following routines:
 * 
 *     void mtctl(reg, value)	- move to control register
 *     int mfctl(reg)		- move from control register
 *     int mtsp(sreg, value)	- move to space register
 *     int mfsr(sreg)		- move from space register
 */

#define CR_RCTR	 0
#define CR_PIDR1 8
#define CR_PIDR2 9
#define CR_CCR   10
#define CR_SAR	 11
#define CR_PIDR3 12
#define CR_PIDR4 13
#define CR_IVA	 14
#define CR_EIEM  15
#define CR_ITMR  16
#define CR_PCSQ  17
#define CR_PCOQ  18
#define CR_IIR   19
#define CR_ISR   20
#define CR_IOR   21
#define CR_IPSW  22
#define CR_EIRR  23
#define CR_PTOV  24
#define CR_VTOP  25
#define CR_TR2   26
#define CR_TR3   27
#define CR_TR4   28
#define CR_TR5   29
#define CR_TR6   30
#define CR_TR7   31

#define CCR_MASK 0xff

