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
 * int strncmp(const char *s1, const char *s2, size_t cnt)
 */
ENTRY(strncmp)
	movl	S_ARG2,%ecx		/* load cnt into %ecx */
	testl	%ecx,%ecx
	je	2f			/* count == 0 then return 0 ? */
	pushl	%esi
	movl	4+S_ARG0,%edx		/* load s1 into %edx */
	movl	4+S_ARG1,%esi		/* load s2 into %esi */
	cld
0:	lodsb
	cmpb	%al,(%edx)		/* *s1 == *s2 ? */
	jne	3f			/* no, calculate difference */
	testb	%al,%al			/* compare until nul byte found */
	je	1f
	incl	%edx
	decl	%ecx
	testl	%ecx,%ecx
	jne	0b			/* or cnt == 0 */
1:	popl	%esi
2:	xorl	%eax,%eax		/* return zero */
	ret
3:	movzbl	(%edx),%eax
	movzbl	-1(%esi),%ecx
	subl	%ecx,%eax		/* return difference in bytes */
	popl	%esi
	ret
END(strncmp)
