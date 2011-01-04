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
 * 
 */
/*
 * MkLinux
 */

/*
 * Machine specific support for thread initialization
 */

#include "pthread_internals.h"

/*
 * Set up the initial state of a MACH thread
 */
void
_pthread_setup(pthread_t thread, 
	       void *(*routine)(void), 
	       unsigned int sp)
{
	struct ppc_thread_state state;
	struct ppc_thread_state *ts = &state;
	kern_return_t r;
	unsigned int count;

	/*
	 * Set up PowerPC registers.
	 */
	count = PPC_THREAD_STATE_COUNT;
	MACH_CALL(thread_get_state(thread->kernel_thread,
				   PPC_THREAD_STATE,
				   (thread_state_t) &state,
				   &count),
		  r);

	ts->srr0 = (int) routine;
	ts->r1 = sp;
	ts->r3 = thread;
	MACH_CALL(thread_set_state(thread->kernel_thread,
				   PPC_THREAD_STATE,
				   (thread_state_t) &state,
				   PPC_THREAD_STATE_COUNT),
		  r);
}
