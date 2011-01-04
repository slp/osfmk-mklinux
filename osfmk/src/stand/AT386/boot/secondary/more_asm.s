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
 * 
 */
/*
 * MkLinux
 */



	.file "more_asm.s"

#include <machine/asm.h>

	.text

#if defined(DEBUG)
.MY_REGISTERS:
	.ascii "\
eax %x  ebx %x  ecx %x  edx %x\12\
esi %x  edi %x  esp %x  ebp %x\12\
 cs %x   ds %x   es %x   ss %x\12\
lim %x  gdt %x  eip %x  efl %x\12\0"
	.text
	.align	4
.PSEUDO_DESC:
        .type    .PSEUDO_DESC,@object
        .size    .PSEUDO_DESC,8
        .word	0
        .word	0
        .word 	0
	.space	2
	.align 4
/
/	should have called this routine: fed_up_with_blind_debug
ENTRY(print_registers)
	pushl	%ebp
	movl	%esp, %ebp
	pusha

	pushfl
	pushl	4(%ebp)		/ return address = eip
	
	leal	.PSEUDO_DESC, %eax
	sgdt	(%eax)
	pushl	2(%eax)
	pushl	%ebx
	xorl	%ebx,%ebx
	movw	(%eax),%bx
	movl	%ebx,%eax
	popl	%ebx
	pushl	%eax
	
	xorl	%eax,%eax
	movw	%ss,%ax
	pushl	%eax
	movw	%es,%ax
	pushl	%eax
	movw	%ds,%ax
	pushl	%eax
	movw	%cs,%ax
	pushl	%eax

	pushl	0(%ebp)		/ bp on entry of this routine
	pushl	%ebp		/ sp before
	pushl	%edi
	pushl	%esi

	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	-4(%ebp)	/ eax

	pushl	$.MY_REGISTERS
	call	printf
	addl	$68,%esp
	
	popa
	popl	%ebp
	ret
#endif

/*
 * delay(microseconds)
 */

ENTRY(delay)
	movl	4(%esp),%eax 
	testl	%eax, %eax
	jle	3f
	movl	EXT(delaycount), %ecx
1:
	movl	%ecx, %edx
2:
	decl	%edx
	jne	2b
	decl	%eax
	jne	1b
3:
	ret
	
/
/ pcpy_in(src, dst, cnt)
/	where:	
/	src is an address related to the whole memory segment (from 0 to 4GB)
/	dst is an address related to the BOOT segment and limits
/	cnt is the count of bytes to be copied from src to dst
/

ENTRY(pcpy_in)
	push	%ebp
	mov	%esp, %ebp
	push	%fs
	push	%esi
	push	%edi
	push	%ecx

	cld

	/ set %fs to point at the flat segment
	movl	$0x20, %eax
	movw	%ax, %fs

	mov	B_ARG0, %esi		/ source
	mov	B_ARG1, %edi		/ destination
	mov	B_ARG2, %ecx		/ count

	rep
	fs
	movsb

	pop	%ecx
	pop	%edi
	pop	%esi
	pop	%fs
	pop	%ebp

	ret

/
/ pcpy16_out(src, dst, cnt)
/	same as pcpy() but takes advantages of board 16bits addressing mode
/	where:	
/	src is an address related to the BOOT segment and limits
/	dst is an address related to the whole memory segment (from 0 to 4GB)
/	cnt is the count of bytes to be copied from src to dst
/		(memory is copied by words and adjustement is made in
/		case of odd number of bytes)

ENTRY(pcpy16_out)
	push	%ebp
	mov	%esp, %ebp
	push	%es
	push	%esi
	push	%edi
	push	%ebx
	push	%ecx

	cld

	/ set %es to point at the flat segment
	mov	$0x20, %eax
	movw	%ax, %es

	mov	0x8(%ebp), %esi		/ source
	mov	0xc(%ebp), %edi		/ destination
	mov	0x10(%ebp), %ebx	/ count
	movl	%ebx, %ecx
	sarl	$1, %ecx		/ count/2 for words

	rep
	movsw

	andl	$1, %ebx
	je	end16o
	movb	0x0(%esi), %al
	movb	%al, %es:0x0(%edi)
end16o:

	pop	%ecx
	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%es
	pop	%ebp

	ret

/
/ pcpy16_in(src, dst, cnt)
/	same as pcpy_in() but takes advantages of board 16bits addressing mode
/	where:	
/	src is an address related to the whole memory segment (from 0 to 4GB)
/	dst is an address related to the BOOT segment and limits
/	cnt is the count of bytes to be copied from src to dst
/		(memory is copied by words and adjustement is made in
/		case of odd number of bytes)
ENTRY(pcpy16_in)
	push	%ebp
	mov	%esp, %ebp
	push	%fs
	push	%esi
	push	%edi
	push	%ebx
	push	%ecx

	cld

	/ set %fs to point at the flat segment
	mov	$0x20, %eax
	movw	%ax, %fs

	mov	0x8(%ebp), %esi		/ source
	mov	0xc(%ebp), %edi		/ destination
	mov	0x10(%ebp), %ebx	/ count
	movl	%ebx, %ecx
	sarl	$1, %ecx		/ count/2 for words

	rep
	fs
	movsw

	andl	$1, %ebx
	je	end16
	movb	%fs:(%esi), %al
	movb	%al, (%edi)
end16:

	pop	%ecx
	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%fs
	pop	%ebp

	ret


