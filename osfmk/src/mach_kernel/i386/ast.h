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
 * Revision 2.4.3.2  92/04/08  15:43:14  jeffreyh
 * 	Define MACHINE_AST_PER_THREAD (from David Golub).
 * 	[92/04/05            dlb]
 * 
 * Revision 2.4.3.1  92/03/03  16:14:16  jeffreyh
 * 	Pick up changes from MK68
 * 	[92/02/26  11:04:27  jeffreyh]
 * 
 * Revision 2.5  92/01/03  20:04:28  dbg
 * 	Add AST_I386_FP to handle delayed floating-point exceptions.
 * 	[91/10/29            dbg]
 * 
 * Revision 2.4  91/05/14  16:03:02  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:10:46  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:30:36  mrt]
 * 
 * Revision 2.2  90/05/03  15:24:36  dbg
 * 	Created.
 * 	[89/02/21            dbg]
 * 
 */
/* CMU_ENDHIST */
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
/*
 */

#ifndef	_I386_AST_H_
#define	_I386_AST_H_

/*
 * Machine-dependent AST file for machines with no hardware AST support.
 *
 * For the I386, we define AST_I386_FP to handle delayed
 * floating-point exceptions.  The FPU may interrupt on errors
 * while the user is not running (in kernel or other thread running).
 */

#define	AST_I386_FP	0x80000000
#define MACHINE_AST_PER_THREAD  AST_I386_FP


#endif	/* _I386_AST_H_ */
