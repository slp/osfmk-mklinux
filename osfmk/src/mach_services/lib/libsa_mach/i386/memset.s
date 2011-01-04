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
 * int bzero(char *dst, int cnt)
 */
ENTRY(bzero)
	pushl	%edi
	movl	4+S_ARG0,%edi		/* dst */
	movl	4+S_ARG1,%edx		/* cnt */
	xorl	%eax,%eax
	cld
/ zero longs
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep
	stosl
/ zero bytes
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	stosb
	popl	%edi
	ret
END(bzero)

/*
 * void *memset(void *dst, int chr, size_t cnt)
 */
ENTRY(memset)
	pushl	%edi
	movl	4+S_ARG0,%edi		/* dst */
	movb	4+S_ARG1,%al		/* chr */
	movl	4+S_ARG2,%edx		/* cnt */
	movb	%al,%ah
	movw	%ax,%cx
	shll	$16,%eax
	movw	%cx,%ax	
	cld
/ zero longs
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep
	stosl
/ zero bytes
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	stosb
	popl	%edi
	movl	S_ARG0,%eax		/* returns its first argument */
	ret
END(memset)
