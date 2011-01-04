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
 * Revision 2.1.2.1  92/04/30  11:54:09  bernadat
 * 	Copied from main line
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.2  92/04/04  11:36:29  rpd
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[92/04/03            rvb]
 * 	Need to zero dh on hd path; at least for an adaptec card.
 * 	[92/01/14            rvb]
 * 
 * 	From 2.5 boot:
 * 	Flush digit printing.
 * 	Fuse floppy and hd boot by using Int 21 to tell
 * 	boot type (slightly dubious since Int 21 is DOS
 * 	not BIOS)
 * 	[92/03/30            mg32]
 * 
 * Revision 2.2  91/04/02  14:42:04  mbj
 * 	Fix the BIG boot bug.  We had missed a necessary data
 * 	before a xor that was clearing a register used later
 * 	as an index register.
 * 	[91/03/01            rvb]
 * 	Remember floppy type for swapgeneric
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

#include	<machine/asm.h>

	.file	"start.s"

BOOTSEG		=	0x100	/ boot will be loaded at 4k
BOOTSTACK	=	0xf000	/ boot stack
SIGNATURE	=	0xaa55
LOADSZ		=	16	/ size of unix boot
PARTSTART	=	0x1be	/ starting address of partition table
NUMPART		=	4	/ number of partitions in partition table
PARTSZ		=	16	/ each partition table entry is 16 bytes
BOOTABLE	=	0x80	/ value of boot_ind, means bootable partition


	.text	

ENTRY(boot1)

	/ boot1 is loaded at 0x0:0x7c00
	/ ljmp to the next instruction to set up %cs
	data16
	ljmp $0x7c0, $start

start:
	/ set up %ds
	mov	%cs, %ax
	mov	%ax, %ds

	/ set up %ss and %esp
	data16
	mov	$BOOTSEG, %eax
	mov	%ax, %ss
	data16
	mov	$BOOTSTACK, %esp

	/set up %es
	mov	%ax, %es
#if	DEBUG
	movb	$0x41, %al
	data16
	call	outchr
#endif
	cmpb	$0x80, %dl
	data16
	je	hd

fd:
#if	DEBUG
	movb	$0x42, %al
	data16
	call	outchr
#endif
	movb	$0x0, %dl

/	reset the disk system
	movb	$0x0, %ah
	int	$0x13

#if	DEBUG
	movb	$0x43, %al
	data16
	call	outchr
#endif
/	read geometry info
	push	%es
	push	%edx
	movb	$0x8, %ah
	int	$0x13
	data16
	jnb	ok
#if	DEBUG
	movb	$0x44, %al
	data16
	call	outchr
#endif
	movb	$0x0f, %cl
ok:	
#if	DEBUG
	movb	%cl, %al
	data16
	call	outhex
	movb	%dl, %al
	data16
	call	outhex
#endif
	andb	$0x3f, %cl
	pop	%edx
	pop	%es
	push	%ecx
#if	DEBUG
	movb	$0x45, %al
	data16
	call	outchr
#endif
	movb	$0x2, %ah
	movb	%cl, %al
	xor	%ebx, %ebx	/ %bx = 0
	data16
	mov	$0x0001, %ecx	/ cyl 0, sector 1
	xorb	%dh, %dh	/ starting head
	int	$0x13
	data16
	jb	read_error
	incb	%dh		/ increment head
	pop	%ecx
#if	DEBUG
	movb	$0x46, %al
	data16
	call	outchr
#endif
	movb	$0x2, %ah
	movb	%cl, %al
	xor	%ebx, %ebx
	movb	%cl, %bl
	data16
	shl	$9, %ebx
	data16
	mov	$0x0001, %ecx	/ cyl 0, sector 1
	int	$0x13
	data16
	jb	read_error

	/ ljmp to the second stage boot loader (boot2).
	/ After ljmp, %cs is BOOTSEG and boot1 (512 bytes) will be used
	/ as an internal buffer "intbuf".

#if	DEBUG
	movb	$0x47,%al
	data16
	call	outchr
#endif
	data16
	ljmp	$BOOTSEG, $EXT(boot2)

hd:	
#if	DEBUG
	movb	$0x48, %al
	data16
	call	outchr
#endif
	data16
	mov	$0x0201, %eax
	xor	%ebx, %ebx	/ %bx = 0
	data16
	mov	$0x0001, %ecx
	data16
	mov	$0x0080, %edx
	int	$0x13
	data16
	jb	read_error

#if	DEBUG
	movb	$0x49, %al
	data16
	call	outchr
#endif
	/ find the bootable partition
	data16
	mov	$PARTSTART, %ebx
	data16
	mov	$NUMPART, %ecx
again:
	addr16
	movb    %es:0(%ebx), %al
	cmpb	$BOOTABLE, %al
	data16
	je	found
	data16
	add	$PARTSZ, %ebx
	data16
	loop	again
	data16
	mov	$enoboot, %esi
	data16
	jmp	message

found:
#if	DEBUG
	movb	$0x4a, %al
	data16
	call	outchr
#endif
	addr16
	movb	%es:1(%ebx), %dh
	addr16
	movl	%es:2(%ebx), %ecx

load:	
#if	DEBUG
	movb	$0x4b, %al
	data16
	call	outchr
#endif
	movb	$0x2, %ah
	movb	$LOADSZ, %al
	xor	%ebx, %ebx	/ %bx = 0
	int	$0x13
	data16
	jb	read_error

	/ ljmp to the second stage boot loader (boot2).
	/ After ljmp, %cs is BOOTSEG and boot1 (512 bytes) will be used
	/ as an internal buffer "intbuf".

#if	DEBUG
	movb	$0x4c, %al
	data16
	call	outchr
#endif
	data16
	ljmp	$BOOTSEG, $EXT(boot2)

#ifdef	DEBUG

/
/	outword: print a hex word
/	%ax: word to be printed
/

outword:
	push	%eax
	shr	$8,%eax
	data16
	call	outhex		/output top nibble
	pop	%eax
/ fall thru to output bottom byte

/
/	outhex: print a hex byte
/	%al: byte to be printed

outhex:
	push	%eax
	sarb	$0x4, %al	/ top nibble
	data16
	call	outnib
	pop	%eax
/ fall thru to output bottom nibble

outnib:
	push	%eax
	push	%ebx

	andb	$0xf, %al
	orb	$0x30,%al
	cmpb	$0x3a, %al
	data16
	jb	1f

	addb	$0x61-0x3a, %al
1:
	movb	$0x01, %bl
	movb	$0xe, %ah
	int	$0x10

	push	%ecx
/ delay for a while
	xor	%ecx,%ecx
1:	loop	1b
1:	loop	1b
	pop	%ecx

	pop	%ebx
	pop	%eax
	data16
	ret

/
/	outchr: print a char
/	%al: char to be printed
/

outchr:
	push	%eax
	push	%ebx

	movb	$0x01, %bl
	movb	$0xe, %ah
	int	$0x10

	push	%ecx
/ delay for a while
	xor	%ecx,%ecx
1:	loop	1b
1:	loop	1b
	pop	%ecx

	pop	%ebx
	pop	%eax
	data16
	ret

#endif	/* DEBUG */

/
/	read_error
/

read_error:
#ifdef	DEBUG
	movb	$0x1, %ah
	int	$0x13

	movb	%ah, %al
	data16
	call	outhex
#endif

	data16
	mov	$eread, %esi

/
/	message: write the error message in %ds:%esi to console
/

message:
	/ Use BIOS "int 10H Function 0Eh" to write character in teletype mode
	/	%ah = 0xe	%al = character
	/	%bh = page	%bl = foreground color (graphics modes)

	data16
	mov	$0x0001, %ebx
	cld

nextb:
	lodsb			/ load a byte into %al
	cmpb	$0x0, %al
	data16
	je	done
	movb	$0xe, %ah
	int	$0x10		/ display a byte
	data16
	jmp	nextb
done:	data16
	jmp	done
	hlt

/
/	error messages

eread:	String		"Read error\r\n\0"
enoboot: String		"No bootable partition\r\n\0"
/ the last 2 bytes in the sector 0 contain the signature
	. = EXT(boot1) + 0x1fe
	.value	SIGNATURE
