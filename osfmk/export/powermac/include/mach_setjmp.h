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
/*
 * OLD HISTORY
 * Revision 1.2.5.2  1995/01/06  20:59:53  devrcs
 * 	Merge in 1.3 changes
 * 	[1994/10/21  20:18:55  dswartz]
 * 
 * Revision 1.2.5.1  1994/09/23  06:16:08  ezf
 * 	change marker to not FREE
 * 	[1994/09/23  05:35:54  ezf]
 * 
 * Revision 1.2.2.4  1994/05/06  19:04:43  tmt
 * 	Support Alpha.
 * 	[1994/05/06  16:05:20  tmt]
 * 
 * Revision 1.2.2.2  1993/06/02  21:50:46  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  20:48:00  gm]
 * 
 * Revision 1.2  1992/05/12  14:45:18  devrcs
 * 	Created for OSF/1 MK
 * 	[1992/05/05  01:13:17  condict]
 * 
 * Revision 2.8  91/09/04  11:29:03  jsb
 * 	From Intel SSD: added i860 support.
 * 	[91/09/04  10:14:32  jsb]
 * 
 * Revision 2.7  91/07/09  23:24:06  danner
 * 	Added luna88k support, some missing comments.
 * 	[91/06/27            danner]
 * 
 * Revision 2.6  91/05/14  17:54:56  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/14  14:18:43  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:44:53  mrt]
 * 
 * Revision 2.4  90/11/05  14:36:03  rpd
 * 	Changed the mips jmp_buf definition to the normal size.
 * 	[90/10/30            rpd]
 * 
 * Revision 2.3  90/05/03  15:54:23  dbg
 * 	Add i386 definitions.
 * 	[90/02/05            dbg]
 * 
 * Revision 2.2  89/11/29  14:18:52  af
 * 	Added mips defs.  This file is only ok for standalone Mach use,
 * 	not inside U*x.
 * 	[89/10/28  10:23:02  af]
 * 
 * Revision 2.1  89/08/03  17:06:57  rwd
 * 	Created.
 * 
 * Revision 2.3  88/12/22  17:06:02  mja
 * 	Correct __STDC__ return value type and allow for recursive inclusion.
 * 	[88/12/20            dld]
 * 
 * Revision 2.2  88/12/14  23:34:14  mja
 * 	Added ANSI-C (and C++) compatible argument declarations.
 * 	[88/01/18            dld@cs.cmu.edu]
 * 
 * Revision 0.0  87/09/03            mbj
 * 	Added definition of jump buffer for Multimax.
 * 	[87/09/03            mbj]
 * 
 * Revision 0.0  87/05/29            rvb
 * 	ns32000 jmp_buf for sequent and possibly mmax
 * 	[87/05/29            rvb]
 * 
 * Revision 0.0  86/11/17            jjc
 * 	Added defintion of jump buffer for SUN.
 * 	[86/11/17            jjc]
 * 
 * Revision 0.0  86/09/22            gm0w
 * 	Added changes for IBM RT.
 * 	[86/09/22            gm0w]
 * 
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

#ifdef	hp_pa
#ifndef	_JBLEN
#define _JBLEN  64      /* regs, fp regs, cr, sigmask, context, etc. */
#endif
typedef int jmp_buf[_JBLEN];
#endif	/* hp_pa */

#ifdef	ppc
typedef int jmp_buf[68];
#endif	/* ppc */

#if defined(CMUCS) && defined(__STDC__)
extern int setjmp (jmp_buf);
extern void longjmp (jmp_buf, int);
extern int _setjmp (jmp_buf);
extern void _longjmp (jmp_buf, int);
#endif

#endif /* _SETJMP_H_PROCESSED_ */
