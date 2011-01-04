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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * MkLinux
 */

/*	setjmp.h	4.1	83/05/03	*/

#ifndef _SETJMP_H_PROCESSED_
#define _SETJMP_H_PROCESSED_ 1

#ifdef multimax
typedef int jmp_buf[10];
#endif /* multimax */

#ifdef	balance
typedef int jmp_buf[11];	/* 4 regs, ... */
#endif	/* balance */

#ifdef sun3
typedef int jmp_buf[15];	/* pc, sigmask, onsstack, d2-7, a2-7 */
#endif /* sun3 */

#ifdef ibmrt
typedef int jmp_buf[16];
#endif /* imbrt */

#ifdef vax
typedef int jmp_buf[10];
#endif /* vax */

#ifdef mips
typedef int jmp_buf[75];
#endif /* mips */

#ifdef i386
typedef int jmp_buf[21];
#endif /* i386 */

#ifdef luna88k
typedef int jmp_buf[19]; /* r1, r14-r31 */
#endif /* luna88k */

#ifdef i860
typedef int jmp_buf[32];  /* f2 - f15, r1 - r16 registers */
#endif /* i860 */

#if	__alpha
typedef long jmp_buf[84];
#endif

#if defined(CMUCS) && defined(__STDC__)
extern int setjmp (jmp_buf);
extern void longjmp (jmp_buf, int);
extern int _setjmp (jmp_buf);
extern void _longjmp (jmp_buf, int);
#endif

#endif /* _SETJMP_H_PROCESSED_ */
