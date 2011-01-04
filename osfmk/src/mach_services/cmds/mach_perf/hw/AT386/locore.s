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

#include <i386/asm.h>

/*
 * Copy of kernel routines used to copy and clear pages
 */

/ copy_page(from, to, byte count)

ENTRY(hw_copy_page)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG0,%esi
	movl	B_ARG1,%edi
bcopy_common:
	movl	B_ARG2,%edx
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
	leave
	ret

/*
 * zero_page(char * addr, unsigned int length)
 */

ENTRY(hw_zero_page)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	movl	B_ARG1,%edx
	movl	B_ARG0,%edi
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
	leave
	ret
/*
 * access_page(char * addr, unsigned int length)
 */

ENTRY(hw_access_page)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	movl	B_ARG1,%edx
	movl	B_ARG0,%esi
	cld
/ access longs
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep
	lodsl
/ access bytes
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	lodsb
	popl	%esi
	leave
	ret
