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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 Sequent Computer Systems
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND SEQUENT COMPUTER SYSTEMS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * SEQUENT COMPUTER SYSTEMS DISCLAIM ANY LIABILITY OF ANY KIND FOR
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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie-Mellon University
 * Copyright (c) 1990 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/* CMU_HIST */
/*
 * Revision 2.3  91/07/31  18:04:15  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  13:00:10  dbg
 * 	Fix SPL levels to match reality.
 * 	[91/02/14            dbg]
 * 
 * 	Put parentheses around substituted immediate expressions, so
 * 	that they will pass through the GNU preprocessor.
 * 	[91/01/31            dbg]
 * 
 * 	Adapted for pure Mach kernel.
 * 	[90/10/02            dbg]
 * 
 */

/*
 * spl.s
 *
 * Implement spl's.  Should be inline asm via intctl.h
 * Used as a backup
 */

#include <assym.s>
#include <i386/asm.h>
#include <sqt/asm_macros.h>
#include <sqt/intctl.h>

ENTRY(spllo)
	SPL_ASM($(SPL0), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl1)
ENTRY(splsoftclock)
ENTRY(splvm)
	SPL_ASM($(SPL1), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl2)
	SPL_ASM($(SPL2), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl3)
	SPL_ASM($(SPL3), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl4)
ENTRY(spltty)
	SPL_ASM($(SPL4), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl5)
	SPL_ASM($(SPL5), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl6)
ENTRY(splbio)
	SPL_ASM($(SPL6), %al)
	movzbl	%al, %eax
	ret

ENTRY(spl7)
ENTRY(splsched)
ENTRY(splclock)
	SPL_ASM($(SPL7), %al)
	movzbl	%al, %eax
	ret

ENTRY(splnet)
	SPL_ASM($(SPLNET), %al)
	movzbl	%al, %eax
	ret

ENTRY(splimp)
	SPL_ASM($(SPLIMP), %al)
	movzbl	%al, %eax
	ret

ENTRY(splhi)
ENTRY(splhigh)
	SPL_ASM($(SPLHI), %al)
	movzbl	%al, %eax
	ret

/*
 * splx(newmask)
 *	Lower interrupt priority mask back to previous value.
 *
 * DEBUG version checks that mask is actually "lowering" -- ie, staying the
 * same or enabling more interrupts.  Non-debug version is in-line expanded
 * (see machine/intctl.h).
 *
 * Since 1's in the mask enable interrupts, (oldmask & newmask) == oldmask is ok.
 */

#ifdef	DEBUG
ENTRY(splx)
	movb	VA_SLIC+SL_LMASK, %ah		# old mask value from SLIC.
	movb	S_ARG0, %al			# new mask.
	movb	%al, VA_SLIC+SL_LMASK		# install new mask.
	andb	%ah, %al			# %al = old & new.
	cmpb	%ah, %al			# ok if (old & new) == old.
	jne	9f				# not ok -- down in flames.
	ret					# ok -- done.
9:	pushl	$8f				# bad news.
	call	_panic				# really bad news.
8:	.asciz	"splx: bad nesting"

#else DEBUG
ENTRY(splx)
	movb	S_ARG0, %cl
	SPL_ASM(%cl, %al)
	ret
#endif	DEBUG
