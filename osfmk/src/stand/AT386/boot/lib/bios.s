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
/* CMU_HIST */
/*
 * Revision 2.1.2.1  92/04/30  11:52:28  bernadat
 * 	Copied from main line
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.2  92/04/04  11:34:26  rpd
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[92/04/03            rvb]
 * 	From 2.5 version
 * 	[92/03/30            mg32]
 * 
 * Revision 2.2  91/04/02  14:35:21  mbj
 * 	Add Intel copyright
 * 	[90/02/09            rvb]
 * 
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 */


/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

	.file	"bios.s"

#include <machine/asm.h>
#include "boot.h"
	.text

/ biosread(dev, cyl, head, sec, nsec, offset)
/	Read <nsec> sector from disk into the internal buffer 
/	at offset <offset>
/ BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
/	Call with	%ah = 0x2
/			%al = number of sectors
/			%ch = cylinder
/			%cl = sector
/			%dh = head
/			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
/			%es:%bx = segment:offset of buffer
/	Return:		
/			%al = 0x0 on success; err code on failure

#define B_ARG4	24(%ebp)
#define B_ARG5	28(%ebp)

ENTRY(biosread)
	push	%ebp
	mov	%esp, %ebp

	push	%ebx
	push	%ecx
	push	%edx
	push	%es
	push	%esi
	push	%edi

	movb	B_ARG2, %dh
	movw	B_ARG1, %cx
	xchgb	%ch, %cl	/ cylinder; the highest 2 bits of cyl is in %cl
	rorb	$2, %cl
	movb	B_ARG3, %al
	orb	%al, %cl
	incb	%cl		/ sector; sec starts from 1, not 0
	movb	B_ARG0, %dl	/ device
	movl	B_ARG5, %esi	/ base -> %esi
	xorw	%si, %si	
	addl	$BOOTOFFSET, %esi
	shrl	$4, %esi
	movl	B_ARG4, %edi	/ nsect -> %edi
	movl	B_ARG5, %ebx	/ offset 
	andl	$0xffff, %ebx
	call	EXT(prot_to_real)	/ enter real mode
	movw	%di, %ax	/ number of sectors 
	movw	%si, %es
	movb	$0x2, %ah	/ subfunction

	sti
	int	$0x13
	cli

	mov	%eax, %ebx	/ save return value

	data16
	call	EXT(real_to_prot) / back to protected mode

	xor	%eax, %eax
	movb	%bh, %al	/ return value in %ax

	pop	%edi
	pop	%esi
	pop	%es
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%ebp

	ret

/ putc(ch)
/ BIOS call "INT 10H Function 0Eh" to write character to console
/	Call with	%ah = 0x0e
/			%al = character
/			%bh = page
/			%bl = foreground color ( graphics modes)


ENTRY(putc)
	push	%ebx
	push	%ecx

	movb	8+S_ARG0, %cl

	call	EXT(prot_to_real)

	data16
	mov	$0x1, %ebx	/ %bh=0, %bl=1 (blue)
	movb	$0xe, %ah
	movb	%cl, %al
	sti
	int	$0x10		/ display a byte
	cli

	data16
	call	EXT(real_to_prot)

	pop	%ecx
	pop	%ebx
	ret


/ getc()
/ BIOS call "INT 16H Function 00H" to read character from keyboard
/	Call with	%ah = 0x0
/	Return:		%ah = keyboard scan code
/			%al = ASCII character

ENTRY(getc)
	push	%ebx		/ save %ebx

	call	EXT(prot_to_real)

	movb	$0x0, %ah
	sti
	int	$0x16
	cli

	movb	%al, %bl	/ real_to_prot uses %eax

	data16
	call	EXT(real_to_prot)

	xor	%eax, %eax
	movb	%bl, %al

	pop	%ebx
	ret

/ ischar()
/       if there is a character pending, return it; otherwise return 0
/ BIOS call "INT 16H Function 01H" to check whether a character is pending
/	Call with	%ah = 0x1
/	Return:
/		If key waiting to be input:
/			%ah = keyboard scan code
/			%al = ASCII character
/			Zero flag = clear
/		else
/			Zero flag = set

ENTRY(ischar)
	push	%ebx

	call	EXT(prot_to_real)		/ enter real mode

	xor	%ebx, %ebx
	movb	$0x1, %ah
	sti
	int	$0x16
	cli
	data16
	jz	nochar
	movb	%al, %bl

nochar:
	data16
	call	EXT(real_to_prot)

	xor	%eax, %eax
	movb	%bl, %al

	pop	%ebx
	ret

/
/ get_diskinfo():  return a word that represents the
/	max number of sectors and  heads and drives for this device
/

ENTRY(get_diskinfo)
	push	%ebp
	mov	%esp, %ebp
	push	%es
	push	%ebx
	push	%ecx
	push	%edx
	push	%edi

	movb	B_ARG0, %dl		/ diskinfo(drive #)
	call	EXT(prot_to_real)	/ enter real mode

	movb	$0x8, %ah		/ ask for disk info

	sti
	int	$0x13
	cli

	data16
	call	EXT(real_to_prot)	/ back to protected mode

	xor	%eax, %eax

/	form a longword representing all this gunk
	movb	%dh, %ah		/ # heads
	andb	$0x3f, %cl		/ mask of cylinder gunk
	movb	%cl, %al		/ # sectors
	pop	%edi
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%es
	pop	%ebp
	ret

/
/ memsize(i) :  return the memory size in KB. i == 0 for conventional memory,
/		i == 1 for extended memory
/	BIOS call "INT 12H" to get conventional memory size
/	BIOS call "INT 15H, AH=88H" to get extended memory size
/		Both have the return value in AX.
/

ENTRY(memsize)
	push	%ebx

	mov	4+S_ARG0, %ebx

	call	EXT(prot_to_real)		/ enter real mode

	
	cmpb	$0x1, %bl
	data16
	je	xext
	
	sti
	int	$0x12
	cli
	data16
	jmp	xdone

xext:	movb	$0x88, %ah
	sti
	int	$0x15
	cli

xdone:
	mov	%eax, %ebx

	data16
	call	EXT(real_to_prot)

	mov	%ebx, %eax
	pop	%ebx
	ret


/ com_init(param_byte)
/ BIOS call "INT 14H Function 00h" to read status  COM0
/	Call with	%ah = 0x00
/			%al = param_byte
/			%dx = 0 for COM0
/ param_byte: see io.c


ENTRY(com_init)
	push	%ebx
	push	%ecx

	movb	8+S_ARG0, %cl

	call	EXT(prot_to_real)

	data16
	mov	$0x0, %edx	/ COM0
	movb	$0x0, %ah
	movb	%cl, %al
	sti
	int	$0x14		/ init
	cli

	data16
	call	EXT(real_to_prot)

	pop	%ecx
	pop	%ebx
	ret

/ read_ticks(&ticks)
/	gets Tick Counter and returns TRUE if Roll-over has taken place.
/ Tick counter increments 18.2 times per second.
/ BIOS call "INT 0x1A Function 0x0" to read tick counter
/	Call with	%ah = 0x0
/
/	Return		%cx = high portion of count
/			%dx = low portion of count
/			%al = Roll-over flag

ENTRY(read_ticks)
	push	%ebp
	mov	%esp, %ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	EXT(prot_to_real)	/ enter real mode
	movb	$0, %ah			/ subfunction

	sti
	int	$0x1a			/ call BIOS
	cli

	xorl	%ebx, %ebx
	movb	%al, %bl		/ real_to_prot uses %eax

	data16
	call	EXT(real_to_prot)

	movw	%cx, %ax
	shll	$0x10, %eax
	movw	%dx, %ax 		/ eax contains ticks value
	movl	0x8(%ebp), %ecx		/ where to store tick number
	movl	%eax, 0(%ecx)		/ store ticks number
	movl	%ebx, %eax		/ get back return value

	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%ebp
	ret

/ set_ticks(ticks)
/	sets Tick Counter.
/ BIOS call "INT 0x1A Function 0x1" to set tick counter
/	Call with	%ah = 0x1
/			%cx = high portion of count
/			%dx = low portion of count
/
/	Return		None

ENTRY(set_ticks)
	push	%ebp
	mov	%esp, %ebp
	push	%ecx
	push	%edx

	movl	0x8(%ebp), %ecx
	shrl	$0x10, %ecx
	movl	0x8(%ebp), %edx
	call	EXT(prot_to_real)	/ enter real mode
	movb	$0x1, %ah		/ subfunction

	sti
	int	$0x1a			/ call BIOS
	cli

	data16
	call	EXT(real_to_prot)

	pop	%edx
	pop	%ecx
	pop	%ebp
	ret
