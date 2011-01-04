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
 * int bcmp(char *s1, char *s2, int cnt)
 */
ENTRY(bcmp)
	jmp	0f
END(bcmp)

/*
 * int memcmp(const void *s1, const void *s2, size_t cnt)
 */
ENTRY(memcmp)
0:	pushl	%esi
	pushl	%edi
	xorl	%eax,%eax		/* eax (ax, al) starts zero */
	movl	8+S_ARG0,%esi		/* 8 for the 2 pushes above */
	movl	8+S_ARG1,%edi
	cmpl	%esi,%edi
	je	0f			/* equal pointers == equal memory */
	movl	8+S_ARG2,%edx
	movl	%edx,%ecx

/* compare longs */
	cld
	sarl	$2,%ecx
	je	1f			/* no whole longwords */
	 repe; cmpsl
	je	1f			/* do bytes if long count expired */

/* long miscompare - back up and find it */
	movl	$4,%ecx
	subl	$4,%esi
	subl	$4,%edi
	jmp	2f
1:
/* compare bytes */
	movl	%edx,%ecx
	andl	$3,%ecx
2:
	 repe; cmpsb
	je	0f
	movzbl	-1(%esi),%eax
	movzbl	-1(%edi),%ecx
	subl    %ecx,%eax
0:	popl	%edi
	popl	%esi
	ret
END(memcmp)
