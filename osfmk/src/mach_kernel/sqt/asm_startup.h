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
 * Copyright (c) 1991 Carnegie Mellon University
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
 * Revision 2.3  91/05/14  17:39:57  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/05/08  12:51:56  dbg
 * 	Created.
 * 	[91/04/26  14:47:07  dbg]
 * 
 * Revision 2.1.1.1  91/02/26  10:53:49  dbg
 * 	Created.
 * 	[91/02/26            dbg]
 * 
 */

#include <ufs_pager.h>

/*
 * Get parameters for Sequent boot.
 */

#if UFS_PAGER

/*
 * Move symbol table out of the way of BSS.
 *
 * When kernel is loaded, instead of BSS we have:
 * _edata	.long symbol_table_size
 *		symbols
 *		strings
 * ...		.align 2	longword boundary.
 * all of which must be moved somewhere else, since it
 * is sitting in the kernel BSS.
 */


	lea	_edata+KVTOPHYS,%esi	/ point to symtab_size -
					/ start of area to move
	movl	(%esi),%eax		/ get symtab size
	testl	%eax,%eax		/ any symbols?
	jz	0f			/ skip if no symbols

	movl	4(%esi,%eax),%ecx	/ get size of string table
	lea	4+3(%eax,%ecx),%ecx	/ add sym_tab size word,
					/ symtab size,
					/ strtab size, and round up
	andl	$(-4),%ecx		/ to integer boundary

	lea	_end+KVTOPHYS,%edi	/ destination - after BSS
	lea	0(%edi,%ecx),%eax	/ save address of first word
					/ after symtab`s new location

	lea	-4(%esi,%ecx),%esi	/ must move backwards because
	lea	-4(%edi,%ecx),%edi	/ of overlap
	shrl	$2,%ecx			/ number of longwords
	std				/ move backwards
	rep
	movsl
	cld
0:
/*
 * %eax has long-word aligned address of end of symbol table,
 * or 0 if no symbols.
 */
#else
/*
 * All parameters are passed in config structure.
 * Nothing to do.
 */
#endif UFS_PAGER
