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
	
	.globl EXT(start)

	.text
/*
 * start() -- indirect bootstrap start
 *
 * Environment:
 *	protected mode, flat 32-bit address space,
 *	all segments in the GDTR table, no interrupts.
 *	(Code/data/stack segments have base == 0, limit == 4G)
 */

EXT(start):
#ifdef	DEBUG
	movl	$'A', %ebx
	call	output
#endif
	movl	$KERNEL_BOOT_ADDR, %ebx
	/*
	 * Save program arguments.
	 */
	movl	0x0(%esp), %ecx
	cmpl	$-1, %ecx
	je	new_boot
old_boot:
	/*
	 * a.out format boot, old arg passing convention:
	 *	sp+0	bootdev
	 *	sp+4	extmem
	 *	sp+8	cnvmem
	 *	sp+C	howto
	 */
	leal	KERNEL_BOOT_ADDR+bootdev, %eax
	movl	0x0(%esp), %ecx		/ get bootdev
	movl	%ecx, 0(%eax)		/ save bootdev

	leal	KERNEL_BOOT_ADDR+howto, %eax
	movl	0xC(%esp), %ecx		/ get howto
	movl	%ecx, 0(%eax)		/ save howto
	jmp	common
new_boot:
/*
 * Elf Format boot, new arg passing convention
 *
 * %esp+0 ->	-1
 *	4	size of extended memory (K)
 *	8	size of conventional memory (K)
 *	C	kern_sym_start
 *	10	kern_sym_size
 *	14	kern_args_start
 *	18	kern_args_size
 *	1C	boot_sym_start
 *	20	boot_sym_size
 *	24	boot_args_start
 *	28	boot_args_size
 *	2C	boot_start
 *	30	boot_size
 *	34	boot_region_desc
 *	38	boot_region_count
 *	3C	boot_thread_state_flavor
 *	40	boot_thread_state
 *	44	boot_thread_state_count
 *	48	env_start
 *	4C	env_size
 *	50	top of loaded memory
 */
	leal	KERNEL_BOOT_ADDR+bootdev, %eax
	movl	0x0(%esp), %ecx		/ get bootdev (should be -1)
	movl	%ecx, 0(%eax)		/ save bootdev

	/*
	 * Move environment to env
	 */ 
	leal	KERNEL_BOOT_ADDR+env_start, %eax
	movl	0x48(%esp), %esi	/ get env_start
	leal	EXT(env), %edi		
	movl	%edi, 0(%eax)		/ save env_start

	leal	KERNEL_BOOT_ADDR+env_size, %eax
	movl	0x4C(%esp), %ecx	/ get env_size
	movl	%ecx, 0(%eax)		/ save env_size

	addl	%ebx, %edi
	cld
	rep
	movsb

	/*
	 * look for '-r' in passed kern arguments
	 */ 
	movl	0x14(%esp), %eax	/ c = arg_start
	movl	0x18(%esp), %esi	/ si = arg size
	addl	%eax, %esi		/ si = argzone's end
1:					/ while(TRUE) {
	cmpl	%eax, %esi		/ 	for(; c < argzone's end
	jbe	common
	
	cmpb	$' ', (%eax)		/	 	&& c == ' '; 
	jne	2f
3:	
	incl	%eax			/	 	c++); 
	jmp	1b

2:		
	cmpb	$'-', (%eax)		/ 	if (c != '-')
	jne	3b			/		c++ and continue; 

	incl	%eax			/	c++
	cmpb	$'r', (%eax)		/	if (c == 'r')
	je 	found_remote_console	/		break and remote console
	incl	%eax			/	c++
	jmp	1b			/ }
	
found_remote_console:
	leal	KERNEL_BOOT_ADDR+howto, %eax
	orl	$RB_ALTBOOT, 0(%eax)

common:
	/ this bootstrap is still in ebx = KERNEL_BOOT_ADDR
	/ ds.base = es.base = 0

	/ 1. put the original bootstrap in BOOTOFFSET
	/ 2. install the GDT as provided by our table
	/ 3. load it
	/ 4. jump to the original place

	/ Copy code to original bootstrap place.
	movl	%ebx, %esi		/ source
	movl	$BOOTOFFSET, %edi	/ destination
	movl	$end, %ecx		/ get size of program
	cld
	rep
	movsb

	/ from here, don't call 'output' function since
	/ the segment descriptors have been overwritten
	
	/ adjust the GDT in the original bootstrap
	leal	BOOTOFFSET+EXT(Gdtr),%eax	/ pseudo descriptor
	leal	BOOTOFFSET+EXT(Gdt),%edx	/ GDT address wrt BOOTOFFSET
	movl	%edx,2(%eax)			/ [limit.w, GDT address.l]
	lgdt	(%eax)

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
	 * Start executing bootstrap.
	 */
	pushl 	env_size
	pushl 	env_start
	pushl	bootdev
	pushl	howto

	call	EXT(mach_boot)
	ret
	

#ifdef	DEBUG
	.align 2
output:
/ output the character in %ebx to the screen
/ put some characters on the screen to verify that boot worked
/ we output 3 rows of 80 characters to b8000 and b0000 just to make sure
/ that we hit the screen.
        pusha
	pushl	%gs
	mov	$0x20,%ax
	movw	%ax,%gs
        mov     $0xb8000,%eax
        mov     $240,%ecx      / fill 3 * 80 bytes of screen 
1:      movb    %ebx,%gs:(%eax)
        movb    $7,%gs:1(%eax)
        add     $2,%eax
        loop    1b

        mov     $0xb0000,%eax
        mov     $240,%ecx      / fill 3 * 80 bytes of screen 
1:      movb    %ebx,%gs:(%eax)
        movb    $7,%gs:1(%eax)
        add     $2,%eax
        loop    1b

/ cause a delay
        mov     $1000000,%ecx
1:
        loop    1b
	popl	%gs
        popa
        ret

/ end of loop to put characters on the screen
#endif
	
		.align 4
bootdev	:	.long 0
howto	:	.long 0
env_start:	.long 0
env_size:	.long 0

/ the last 2 bytes in the sector 0 contain the signature
	. = EXT(start) + 0x1fe
