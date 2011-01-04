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

#include	"machine/asm.h"
#include	"boot.h"

	.globl EXT(bootdev)
	.globl EXT(howto)
	.globl EXT(start)

	.text

/*
 * start() -- indirect bootstrap start
 *
 * Environment:
 *	protected mode, flat 32-bit address space,
 *	all segments in the GDTR table, no interrupts.
 *	(Code/data/stack segments have base == 0, limit == 4G)
 *	WARNING: The GDTR table may be bogus if overwritten by the program.
 */

EXT(start):
#ifdef	DEBUG
	movl	$'A', %ebx
	call	output
#endif
	/*
	 * Lilo loads boot at 0x1000;
	 * with %cs == %ds but %es pointing to linux data area
	 */

	movl	$0x1000, %ebx
	cmpl	$BOOTOFFSET, %ebx
	jz	same_address

#ifdef	DEBUG
	movl	$'B', %ebx
	call	output
#endif
	/*
	 * Copy code to original bootstrap place. 
	 */
	movw	%cs, %ax
	movw	%ax, %es		/ reload %es segment
	movl	%ebx, %esi		/ source
	movl	$BOOTOFFSET, %edi	/ destination
	movl	$end, %ecx		/ get size of program
	rep
	movsb

same_address:

	/*
	 * Enter Real Address mode
	 */
	mov	%cr0, %eax
	and 	$0xFFFFFFFE, %eax
	mov	%eax, %cr0

	/*
	 * Restart program at the correct place.
	 */
	ljmp	$0x100, $restart_real
restart_real:
	/*
	 * Reload correct ds segment
	 */
	movw	%cs, %ax
	movw	%ax, %ds

	/ adjust the GDT in the original bootstrap
	leal	EXT(Gdtr), %eax
	leal	BOOTOFFSET+EXT(Gdt),%edx	/ GDT address wrt BOOTOFFSET
	movl	%edx,2(%eax)			/ [limit.w, GDT address.l]
	lgdt	(%eax)

	/ set the PE bit of CR0
	mov	%cr0, %eax
	or	$0x1, %eax
	mov	%eax, %cr0 

	/*
	 * Enter Protected mode with new GDTR.
	 */
	ljmp	$0x08,	$restart
restart:
	movw	$0x10,	%ax
	movw	%ax,	%ds
	movw	%ax,	%es
	movw	%ax,	%fs
	movw	%ax,	%gs
#if 0
	movw	$0x30,	%ax		/ provision for separate stack segment
#endif
	movw	%ax,	%ss


	/*
	 * Reload stack address.
	 */
	movl	$BOOTSTACK, %esp

#ifdef	DEBUG
	movl	$'D', %ebx
	call	output
#endif
	
	/*
	 * Reload Idt, lilo breaks it
	 * We need one for bios calls
	 */

	lea	EXT(Idtr), %eax
	lidt	(%eax)

	/*
	 * Start executing bootstrap.
	 */
	pushl 	env_size
	pushl 	env_start
	call	EXT(get_lilo_env)
	pushl	EXT(bootdev)
	pushl	EXT(howto)
	call	EXT(mach_boot)
	ret

#ifdef	DEBUG
	.align 2
/* 
 * write on COM1: the character passed in argument
 */
output:
        pushl	%edx
	pushl	%eax

	movw	$0x3fd, %dx
1:	
	xorb	%al, %al
	inb	%dx, %al
	testb	$0x20, %al	/ wait till transmit ready
	jz	1b

	movw	$0x3f8, %dx	/ send the character
	movb	%bl, %al
	outb	%al, %dx
	
        popl	%eax
	popl	%edx
        ret
#endif
	

		.align 4
EXT(bootdev):	.long 0
EXT(howto):	.long 0
env_start:	.long 0
env_size:	.long 0

/ the last 2 bytes in the sector 0 contain the signature
	. = EXT(start) + 0x1fe
