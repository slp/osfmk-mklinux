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
 * Revision 2.6  91/05/14  16:42:41  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/03/16  14:50:17  rpd
 * 	Replaced ipc_thread_switch with thread_handoff.
 * 	Renamed ipc_thread_{go,will_wait,will_wait_with_timeout}
 * 	to thread_{go,will_wait,will_wait_with_timeout}.
 * 	Removed ipc_thread_block.
 * 	[91/02/17            rpd]
 * 
 * Revision 2.4  91/02/05  17:26:58  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:13:32  mrt]
 * 
 * Revision 2.3  91/01/08  15:15:58  rpd
 * 	Removed ipc_thread_go_and_block.
 * 	Added ipc_thread_switch.
 * 	Added continuation argument to ipc_thread_block.
 * 	[90/12/08            rpd]
 * 
 * Revision 2.2  90/06/02  14:54:27  rpd
 * 	Created for new IPC.
 * 	[90/03/26  23:46:16  rpd]
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

#ifndef	_KERN_IPC_SCHED_H_
#define	_KERN_IPC_SCHED_H_

#include <mach/boolean.h>
#include <kern/macro_help.h>
#include <kern/thread.h>

/* Start a thread running */
extern void thread_go(
	thread_t	thread);

/* Assert that the thread intends to block */
extern void thread_will_wait(
	thread_t	thread);

/* Assert that the thread intends to block with a timeout */
extern void thread_will_wait_with_timeout(
	thread_t		thread,
	mach_msg_timeout_t	msecs);

/*
 *	Convert a timeout in milliseconds (mach_msg_timeout_t)
 *	to a timeout in ticks (for use by set_timeout).
 *	This conversion rounds UP so that small timeouts
 *	at least wait for one tick instead of not waiting at all.
 */

#define convert_ipc_timeout_to_ticks(millis)	\
		(((millis) * hz + 999) / 1000)


#endif	/* _KERN_IPC_SCHED_H_ */
