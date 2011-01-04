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
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * Copyright(c) 1989 Robert V. Baron
 */


#include <machine/asm.h>

ENTRY(__fixdfsi)		/ df at 04(%esp) for 8 bytes
	subl	$4, %esp	/ df at 08(%esp)
	subl	$4, %esp	/ df at 0c(%esp); ret at 8(%esp)
				/ rval at 4(%esp); cw at 0(%esp)
	fstcw	(%esp)		
	pushl	(%esp)		/ make copy
	orl	$0xc00, (%esp)
	fldcw	(%esp)		/ truncate
	addl	$4, %esp	/ flush change
	fldl	0x0c(%esp)
	fistpl	4(%esp)
	fldcw	(%esp)		/ restore
	addl	$4, %esp
	popl	%eax
	ret
