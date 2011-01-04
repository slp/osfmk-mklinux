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
 * MkLinux
 */
/*
 * Revision 2.8.2.1  92/06/22  11:50:23  rwd
 * 	Changes for single internal lock version of cproc.c
 * 	[92/01/09            rwd]
 * 
 * Revision 2.8  91/05/14  17:58:35  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/02/14  14:21:03  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:41:31  mrt]
 * 
 * Revision 2.6  90/09/09  14:35:04  rpd
 * 	Remove special option , debug_mutex and thread_calls.
 * 	[90/08/24            rwd]
 * 
 * Revision 2.5  90/06/02  15:14:14  rpd
 * 	Removed RCS Source, Header lines.
 * 	[90/05/03  00:07:27  rpd]
 * 
 * Revision 2.4  90/03/14  21:12:15  rwd
 * 	Added new option:
 * 		WAIT_DEBUG:	keep track of who a blocked thread is
 * 				waiting for.
 * 	[90/03/01            rwd]
 * 
 * Revision 2.3  90/01/19  14:37:25  rwd
 * 	New option:
 * 		THREAD_CALLS:	cthread_* version of thread_* calls.
 * 	[90/01/03            rwd]
 * 
 * Revision 2.2  89/12/08  19:54:09  rwd
 * 	Added code:
 * 		MUTEX_SPECIAL:	Have extra kernel threads available for
 * 				special mutexes to avoid deadlocks
 * 	Removed options:
 * 		MSGOPT, RECEIVE_YIELD
 * 	[89/11/25            rwd]
 * 	Added option:
 * 		MUTEX_SPECIAL:	Allow special mutexes which will
 * 				garuntee the resulting threads runs
 * 				on a mutex_unlock
 * 	[89/11/21            rwd]
 * 	Options added are:
 * 		STATISTICS:	collect [kernel/c]thread state stats.
 * 		SPIN_RESCHED:	call swtch_pri(0) when spin will block.
 * 		MSGOPT:		try to minimize message sends
 * 		CHECK_STATUS:	check status of mach calls
 * 		RECEIVE_YIELD:	yield thread if no waiting threads after
 * 				cthread_msg_receive
 * 		RED_ZONE:	make redzone at end of stacks
 * 		DEBUG_MUTEX:	in conjunction with same in cthreads.h
 * 				use slow mutex with held=cproc_self().
 * 	[89/11/13            rwd]
 * 	Added copyright.  Removed all options.
 * 	[89/10/23            rwd]
 * 
 */
/*
 * options.h
 */

/*#define STATISTICS*/
/*#define CHECK_STATUS*/

#define SPIN_RESCHED

/*#define WAIT_DEBUG*/
/*#define THREAD_CALLS*/

#define SPIN_POLL_MUTEX
#define SPIN_POLL_IDLE
#define BUSY_SPINNING
/*#define TRLOG*/
