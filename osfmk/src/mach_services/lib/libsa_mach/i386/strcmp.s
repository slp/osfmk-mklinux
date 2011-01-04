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

#include <machine/asm.h>

/*
 * int strcmp(const char *s1, const char *s2)
 */
ENTRY(strcmp)
	pushl	%esi
	movl	4+S_ARG0,%edx		/* load s1 into %edx */
	movl	4+S_ARG1,%esi		/* load s2 into %esi */
	cld
0:	lodsb
	cmpb	%al,(%edx)		/* *s1 == *s2 ? */
	jne	1f			/* no, calculate difference */
	incl	%edx
	testb	%al,%al			/* compare until nul byte found */
	jne	0b
	xorl	%eax,%eax		/* return zero */
	popl	%esi
	ret
1:	movzbl	(%edx),%eax
	movzbl	-1(%esi),%ecx
	subl	%ecx,%eax		/* return difference in bytes */
	popl	%esi
	ret
END(strcmp)
