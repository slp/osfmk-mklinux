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

#include <mach.h>
#include <mach/mach_traps.h>
#include <threads.h>

void mig_init(int first);	/* Shut gcc up. */

void mig_init(int first)
{
	register i;

	if (first == 0) {
		for (i = 0; i < MAX_THREADS; i++)
			threads[i].mig_reply_port = MACH_PORT_NULL;
	}
}

mach_port_t
mig_get_reply_port()
{
	mach_thread_t th;
	register mach_port_t port;

	th = thread_self();
	if ((port = th->mig_reply_port) == MACH_PORT_NULL)
		th->mig_reply_port = port = mach_reply_port();

	return port;
}

void
mig_dealloc_reply_port(mach_port_t reply_port)
{
	mach_port_t port;
	mach_thread_t th;

	th = thread_self();
	port = th->mig_reply_port;
	th->mig_reply_port = MACH_PORT_NULL;

	(void) mach_port_mod_refs(mach_task_self(), port,
				  MACH_PORT_RIGHT_RECEIVE, -1);
}

void
mig_put_reply_port(
	mach_port_t	reply_port)
{
}




