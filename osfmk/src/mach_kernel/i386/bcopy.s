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
/* CMU_HIST */
/*
 * Revision 2.5  91/05/14  16:03:58  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:10:49  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:30:41  mrt]
 * 
 * Revision 2.3  90/11/05  14:26:58  rpd
 * 	Introduce bcopy16.  For 16bit copies to bus memory
 * 	[90/11/02            rvb]
 * 
 * Revision 2.2  90/05/03  15:24:53  dbg
 * 	From Bob Baron.
 * 	[90/04/30            dbg]
 * 
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 */

#include <i386/asm.h>

/ void *memcpy((void *) to, (const void *) from, (size_t) bcount)

ENTRY(memcpy)
	pushl	%edi
	pushl	%esi
	movl	8+ 4(%esp),%edi		/* to */
	movl	%edi,%eax		/* returns its first argument */
	movl	8+ 8(%esp),%esi		/* from */
memcpy_common:
	movl	8+ 12(%esp),%edx	/* number of bytes */
	cld
/ move longs
	movl	%edx,%ecx
	sarl	$2,%ecx
	rep
	movsl
/ move bytes
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	movsb
	popl	%esi
	popl	%edi
	ret

/ void bcopy((const char *) from, (char *) to, (unsigned int) count)

ENTRY(bcopy)
	pushl	%edi
	pushl	%esi
	movl	8+ 8(%esp),%edi		/* to */
	movl	8+ 4(%esp),%esi		/* from */
	jmp	memcpy_common

/ bcopy16(from, to, bcount) using word moves

ENTRY(bcopy16)
	pushl	%edi
	pushl	%esi
 	movl	8+12(%esp),%edx		/  8 for the two pushes above
 	movl	8+ 8(%esp),%edi
 	movl	8+ 4(%esp),%esi
/ move words
0:	cld
	movl	%edx,%ecx
	sarl	$1,%ecx
	rep
	movsw
/ move bytes
	movl	%edx,%ecx
	andl	$1,%ecx
	rep
	movsb
	popl	%esi
	popl	%edi
	ret	

