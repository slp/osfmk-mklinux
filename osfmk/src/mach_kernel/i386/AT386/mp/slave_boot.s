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

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

/* CMU_HIST */
/*
 * Revision 2.1.3.2  92/05/26  11:23:30  jeffreyh
 * 	Removed debug trace as it was altering random memory
 * 	when running in real mode (observed on Compaq SystemPro).
 * 	[92/05/07            bernadat]
 * 
 * Revision 2.1.3.1  92/04/30  11:57:56  bernadat
 * 	Moved from cbus to here, applies to both Corollary
 * 	and SystemPro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.1.9.1  92/02/18  18:34:59  jeffreyh
 * 	Created
 * 	[91/06/27  05:00:51  bernadat]
 * 
 */

#include "machine/asm.h"
#include "machine/mp/boot.h"

#define CR0_PE_ON	0x1
#define CR0_PE_OFF	0xfffffffe

	.file	"slave_boot.s"

	.text	

#define	LJMP(segment,address)	\
	.byte	0xea		;\
	.long	address-EXT(slave_boot_base)		;\
	.word	segment

#define	LGDT(address)	\
	.word	0x010f ;\
	.byte	0x15 ;\
	.long	address-EXT(slave_boot_base)

ENTRY(slave_boot_base)
	/ code is loaded at 0x0:0x1000
	/ ljmp to the next instruction to set up %cs
	data16
	LJMP(MP_BOOTSEG, EXT(slave_pstart))

ENTRY(slave_pstart)
	/ set up %ds
	mov	%cs, %ax
	mov	%ax, %ds

	/ set up %ss and %esp
	data16
	mov	$MP_BOOTSEG, %eax
	mov	%ax, %ss
	data16
	mov	$MP_BOOTSTACK, %esp

	/set up %es
	mov	%ax, %es

	/* change to protected mode */
	data16
	call	EXT(real_to_prot)

	push	MP_MACH_START
	call	EXT(startprog)

/
/ real_to_prot()
/ 	transfer from real mode to protected mode.
/

ENTRY(real_to_prot)
	/ guarantee that interrupt is disabled when in prot mode
	cli

	/ load the gdtr
	addr16
	data16
	LGDT(EXT(gdtr))

	/ set the PE bit of CR0
	mov	%cr0, %eax

	data16
	or	$CR0_PE_ON, %eax
	mov	%eax, %cr0 

	/ make intrasegment jump to flush the processor pipeline and
	/ reload CS register
	data16
	LJMP(0x08, xprot)

xprot:
	/ we are in USE32 mode now
	/ set up the protective mode segment registers : DS, SS, ES
	mov	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

	ret

/
/ startprog(phyaddr)
/	start the program on protected mode where phyaddr is the entry point
/

ENTRY(startprog)
	push	%ebp
	mov	%esp, %ebp
	
	mov	0x8(%ebp), %ecx		/ entry offset 
	mov	$0x28, %ebx		/ segment
	push	%ebx
	push	%ecx

	/ set up %ds and %es
	mov	$0x20, %ebx
	movw	%bx, %ds
	movw	%bx, %es

	lret


	. = MP_GDT-MP_BOOT	/ GDT location
ENTRY(Gdt)

/*  Segment Descriptor
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|0|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |
 * |                             |                            |
 * ------------------------------------------------------------
 */
	.word	0,0		/* 0x0 : null */
	.byte	0,0,0,0

	.word	0xffff,MP_BOOT	/* 0x8 : boot code */
	.byte	0,0x9e,0x40,0

	.word	0xffff,MP_BOOT	/* 0x10 : boot data */
	.byte	0,0x92,0x40,0

	.word	0xffff,MP_BOOT	/* 0x18 : boot code, 16 bits */
	.byte	0,0x9e,0x0,0

	.word	0xffff,0	/* 0x20 : init data */
	.byte	0,0x92,0xcf,0

	.word	0xffff,0	/* 0x28 : init code */
	.byte	0,0x9e,0xcf,0

ENTRY(gdtr)
	.short	48		/* limit (8*6 segs) */
	.short	MP_GDT		/* base low */
	.short	0		/* base high */

ENTRY(slave_boot_end)














