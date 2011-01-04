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
 * Revision 2.5.1.1  92/05/27  00:46:11  jeffreyh
 * 	Added decls for MCMSG system calls
 * 	[regnier@ssd.intel.com]
 * 
 * Revision 2.5  91/05/18  14:33:56  rpd
 * 	Added thread_depress_timeout.
 * 	[91/03/31            rpd]
 * 
 * Revision 2.4  91/05/14  16:47:37  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:29:40  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:18:24  mrt]
 * 
 * Revision 2.2  90/06/02  14:56:22  rpd
 * 	Created.
 * 	[90/03/26  23:52:40  rpd]
 * 
 * Revision 2.5  89/10/11  14:27:28  dlb
 * 	Add thread_switch, remove kern_timestamp.
 * 	[89/09/01  17:43:07  dlb]
 * 
 * Revision 2.4  89/10/10  10:54:34  mwyoung
 * 	Add a new call to create an RFS link, a special form of
 * 	symbolic link that may contain null characters in the target.
 * 	[89/10/01            mwyoung]
 * 
 * Revision 2.3  89/05/01  17:28:32  rpd
 * 	Removed the ctimes() declaration; it's history.
 * 
 * Revision 2.2  89/05/01  17:01:50  rpd
 * 	Created.
 * 	[89/05/01  14:00:51  rpd]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

#ifndef	_KERN_SYSCALL_SUBR_H_
#define _KERN_SYSCALL_SUBR_H_

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/port.h>
#include <kern/thread.h>

/* Attempt to context switch */
extern boolean_t	swtch(void);

/* Attempt to context switch */
extern boolean_t	swtch_pri(
				int		pri);

/*
 * NB:  prototype for syscall_thread_switch() (nee thread_switch())
 * is in mach/mach_syscalls.h.
 */

/* Attempt to context switch */
extern void		thread_depress_timeout( thread_t );
extern kern_return_t 	thread_depress_abort( thread_act_t );
extern kern_return_t 	thread_depress_abort_fast( thread_t );

#include <machine/syscall_subr.h>

#endif	/* _KERN_SYSCALL_SUBR_H_ */
