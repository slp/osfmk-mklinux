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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)DEFS.h	5.1 (Berkeley) 5/30/85
 */
/*
 * MkLinux
 */


#define S_ARG0	 4(%esp)
#define S_ARG1	 8(%esp)
#define S_ARG2	12(%esp)
#define S_ARG3	16(%esp)

#define FRAME	pushl %ebp; movl %esp, %ebp
#define EMARF	leave

#define B_ARG0	 8(%ebp)
#define B_ARG1	12(%ebp)
#define B_ARG2	16(%ebp)
#define B_ARG3	20(%ebp)

#ifdef	wheeze

#define ALIGN 4
#define EXT(x) x
#define LCL(x) ./**/x
#define LB(x,n) ./**/x
#define LBb(x,n) ./**/x
#define LBf(x,n) ./**/x

#define	SVC lcall $7,0

#define	INB	inb	(%dx)
#define	OUTB	outb	(%dx)

#define	STRING .string

#else	/* wheeze */

#define ALIGN 2
#define EXT(x) _/**/x
#define	LCL(x)	x

#define LB(x,n) n
#define LBb(x,n) n/**/b
#define LBf(x,n) n/**/f

#ifdef PS2
#define SVC .byte 0x9a; .long 0; .long 0x7
#else
#define SVC .byte 0x9a; .long 0; .word 0x7
#endif

#define	data16	.byte	0x66
#define	addr16	.byte	0x67
#define	jc jb
#define	INB	.byte	0xec
#define	OUTB	.byte	0xee

#define	OUTB_FIXED(port)	outb	%al,$ port
#define	INB_FIXED(port)	inb	$ port,%al

#define	STRING .ascii

#endif	/* wheeze */


#ifdef PROF
#define	ENTRY(x)	.globl EXT(x); .align ALIGN; EXT(x): ; \
			.data; LB(x, 9): .long 0; .text; lea LBb(x, 9),%edx; call mcount
#define	ASENTRY(x) 	.globl x; .align ALIGN; x: ; \
			.data; LB(x, 9): .long 0; .text; lea LBb(x, 9),%edx; call mcount
#else	/* PROF */
#define	ENTRY(x)	.globl EXT(x); .align ALIGN; EXT(x):
#define	ASENTRY(x)	.globl x; .align ALIGN; x:
#endif	/* PROF */

#define START 0			/* where boot is linked to */
#define START_SEG	0	/* segment for START */
#define PHYS_START	0x2000	/* where its loaded to */
#define PHYS_SEG	0x200
#define BOOT_MAGIC	0xb001c0de

