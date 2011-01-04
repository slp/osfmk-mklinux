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
 * Revision 2.5  91/05/14  16:39:41  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:25:28  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:10:52  mrt]
 * 
 * Revision 2.3  90/11/05  14:30:41  rpd
 * 	Changed assert to use Assert instead of Debugger.
 * 	[90/11/04            rpd]
 * 
 * Revision 2.2  90/08/27  22:01:51  dbg
 * 	Declare 'Debugger' to avoid more lint.
 * 	[90/08/13            dbg]
 * 
 * Revision 2.1  89/08/03  15:43:22  rwd
 * Created.
 * 
 * Revision 2.7  88/12/19  02:41:59  mwyoung
 * 	It appears to be impossible to avoid lint for expressions
 * 	of the form (constant1 < constant2).  Make assert_static empty
 * 	for lint.
 * 	[88/12/17            mwyoung]
 * 
 * Revision 2.6  88/10/18  03:37:27  mwyoung
 * 	Use MACRO_BEGIN, MACRO_END.
 * 	[88/10/11            mwyoung]
 * 	
 * 	Avoid lint warnings about constants in the "while" clause.
 * 	[88/10/06            mwyoung]
 * 
 * Revision 2.5  88/10/01  21:58:26  rpd
 * 	Changed CS_ASSERT to CMUCS_ASSERT.
 * 	[88/10/01  21:32:39  rpd]
 * 
 * Revision 2.4  88/09/25  22:15:40  rpd
 * 	Changed to use Debugger instead of panic.
 * 	[88/09/12  23:04:20  rpd]
 * 
 * Revision 2.3  88/08/24  02:22:40  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:08:36  mwyoung]
 * 
 * Revision 2.2  88/07/20  16:44:48  rpd
 * Modify assert for kernel use.
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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

#ifndef	_KERN_ASSERT_H_
#define	_KERN_ASSERT_H_

/*	assert.h	4.2	85/01/21	*/

#include <mach_assert.h>

#include <kern/macro_help.h>

#if	MACH_ASSERT
/* Assert error */
extern void	Assert(
			const char	*file,
			int		line,
			const char	*expression);

#define assert(ex)							\
MACRO_BEGIN								\
	if (!(ex))							\
		Assert(__FILE__, __LINE__, # ex);			\
MACRO_END

#ifdef	lint
#define	assert_static(x)
#else	/* lint */
#define	assert_static(x)	assert(x)
#endif	/* lint */

#else	/* MACH_ASSERT */
#define assert(ex)		((void)0)
#define assert_static(ex)
#endif	/* MACH_ASSERT */

#endif	/* _KERN_ASSERT_H_ */
