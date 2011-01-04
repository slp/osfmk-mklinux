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
 * Copyright (c) 1990, 1991, 1992, The University of Utah and
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
 * 	Utah $Hdr: $
 */

#ifndef	_MACH_HP700_EXCEPTION_H_
#define _MACH_HP700_EXCEPTION_H_

#define EXC_TYPES_COUNT	10		/* exception types */
#define EXCEPTION_CODE_MAX	2	/* currently code and subcode */

/*
 *	EXC_BAD_INSTRUCTION
 */

#define EXC_HP700_INVALID_SYSCALL	1    /* invalid syscall number */
#define EXC_HP700_UNIPL_INST		2    /* unimplemented instruction */
#define EXC_HP700_PRIVINST		3    /* priviledged instruction */
#define EXC_HP700_PRIVREG		4    /* priviledged register */

/*
 *	EXC_BAD_ACCESS
 *	Note: do not conflict with kern_return_t values returned by vm_fault
 */

#define EXC_HP700_VM_PROT_READ		0x101 /* error reading syscall args */
#define EXC_HP700_BADSPACE		0x102 /* bad space referenced */
#define EXC_HP700_UNALIGNED		0x103 /* unaligned data reference */

/*
 *	EXC_ARITHMETIC
 */

#define EXC_HP700_OVERFLOW		1    /* integer overflow */
#define EXC_HP700_ZERO_DIVIDE		2    /* integer divide by zero */
#define EXC_HP700_FLT_INEXACT		3    /* IEEE inexact exception */
#define EXC_HP700_FLT_ZERO_DIVIDE	4    /* IEEE zero divide */
#define EXC_HP700_FLT_UNDERFLOW		5    /* IEEE floating underflow */
#define EXC_HP700_FLT_OVERFLOW		6    /* IEEE floating overflow */
#define EXC_HP700_FLT_NOT_A_NUMBER	7    /* IEEE not a number */

	/*
	 * EXC_HP700_NOEMULATION should go away when we add software emulation
	 * for floating point. Right now we don't support this.
	 */

#define EXC_HP700_NOEMULATION		8    /* no floating point emulation */


/*
 *	EXC_SOFTWARE
 */
#define EXC_HP700_MIGRATE		0x10100		/* Time to bolt */


/*
 *	EXC_BREAKPOINT
 */

#define EXC_HP700_BREAKPOINT		1    /* breakpoint trap */
#define EXC_HP700_TAKENBRANCH		2    /* taken branch trap */

/*
 *	machine dependent exception masks
 */
#define	EXC_MASK_MACHINE	0

#endif	/* _MACH_HP700_EXCEPTION_H_ */
