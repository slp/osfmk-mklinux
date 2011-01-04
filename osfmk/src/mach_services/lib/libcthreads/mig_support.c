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
 * Revision 2.6  91/05/14  17:57:41  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/14  14:20:30  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:41:26  mrt]
 * 
 * Revision 2.4  90/08/07  14:31:41  rpd
 * 	Removed RCS keyword nonsense.
 * 
 * Revision 2.3  90/08/07  14:27:48  rpd
 * 	When we recycle the global reply port by giving it to the first
 * 	cthread, clear the global reply port.  This will take care of
 * 	someone accidently calling this twice.
 * 	[90/08/07            rwd]
 * 
 * Revision 2.2  90/06/02  15:14:04  rpd
 * 	Converted to new IPC.
 * 	[90/03/20  20:56:50  rpd]
 * 
 * Revision 2.1  89/08/03  17:09:50  rwd
 * 	Created.
 * 
 * 18-Jan-89  David Golub (dbg) at Carnegie-Mellon University
 * 	Replaced task_data() by thread_reply().
 * 
 *
 * 27-Aug-87  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Changed mig_support.c to avoid deadlock that can occur
 * 	if tracing is turned on	during calls to mig_get_reply_port().
 * 
 * 10-Aug-87  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Changed mig_support.c to use MACH_CALL.
 * 	Changed "is_init" to "multithreaded" and reversed its sense.
 * 
 * 30-Jul-87  Mary Thompson (mrt) at Carnegie Mellon University
 * 	Created.
 */
/*
 * 	File:	mig_support.c
 *	Author:	Mary R. Thompson, Carnegie Mellon University
 *	Date:	July, 1987
 *
 * Routines to set and deallocate the mig reply port for the current thread.
 */


#include <mach.h>
#include <mach/mach_traps.h>
#include <cthreads.h>
#include "cthread_internals.h"

/* use a global reply port before becoming multi-threaded */
private boolean_t multithreaded = FALSE;
private mach_port_t mig_reply_port = MACH_PORT_NULL;

/*
 * Called by mach_init with 0 before cthread_init is
 * called and again with initial cthread at end of cthread_init.
 */
void
mig_init(cthread_t initial)
{
	if (initial == CTHREAD_NULL) {
		/* called from mach_init before cthread_init,
		   possibly after a fork.  clear global reply port. */

		multithreaded = FALSE;
		mig_reply_port = MACH_PORT_NULL;
	} else {
		/* recycle global reply port as this cthread's reply port */

		multithreaded = TRUE;
		initial->reply_port = mig_reply_port;
		mig_reply_port = MACH_PORT_NULL;
	}
}

/*
 * Called by mig interface code whenever a reply port is needed.
 */
mach_port_t
mig_get_reply_port(void)
{
	register mach_port_t reply_port;

	if (multithreaded) {
		register cthread_t self;

		self = _cthread_self();

		if ((reply_port = self->reply_port) == MACH_PORT_NULL)
			self->reply_port = reply_port = mach_reply_port();
	} else {
		if ((reply_port = mig_reply_port) == MACH_PORT_NULL)
			mig_reply_port = reply_port = mach_reply_port();
	}

	return reply_port;
}

/*
 * Called by mig interface code after a timeout on the reply port.
 * May also be called by user.
 */
void
mig_dealloc_reply_port(
	mach_port_t	port)
{
	register mach_port_t reply_port;

	if (multithreaded) {
		register cthread_t self;

		self = _cthread_self();

		reply_port = self->reply_port;
		self->reply_port = MACH_PORT_NULL;
	} else {
		reply_port = mig_reply_port;
		mig_reply_port = MACH_PORT_NULL;
	}

	(void) mach_port_mod_refs(mach_task_self(), reply_port,
				  MACH_PORT_RIGHT_RECEIVE, -1);
}

/*
 * Called by mig_interface code after each RPC.
 */
void
mig_put_reply_port(
	mach_port_t	port)
{
}
