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
 * char *strncat(char *dst, const char *src, size_t cnt)
 */
ENTRY(strncat)
	movl	S_ARG2,%ecx		/ load cnt into %ecx
	testl	%ecx,%ecx
	je	2f
	pushl	%edi
	pushl	%esi
	movl	8+S_ARG0,%esi		/ load dst into %esi
	cld
0:	lodsb
	testb	%al,%al			/ find the nul byte in dst
	jne	0b
	movl	%esi,%edi		/ now move into %edi
	decl	%edi			/ backup to nul byte
	movl	8+S_ARG1,%esi		/ load src into %esi
0:	lodsb
	stosb
	testb	%al,%al			/ copy until nul byte in src
	je	1f
	decl	%ecx
	testl	%ecx,%ecx
	jne	0b
	xorb	%al,%al			/ else need to add a nul byte
	movb	%al,-1(%edi)
1:	popl	%esi
	popl	%edi
2:	movl	S_ARG0,%eax		/ load dst as return value
	ret
END(strncat)
