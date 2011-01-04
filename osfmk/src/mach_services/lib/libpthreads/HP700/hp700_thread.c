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
	       void (*routine)(pthread_t), 
	       vm_address_t vsp)
{
	struct hp700_thread_state state;
	struct hp700_thread_state *ts = &state;
	kern_return_t r;
	mach_msg_type_number_t count;
	int *sp = (int *) vsp;;

	/*
	 * Set up pa-risc registers & function call.
	 */
	count = HP700_THREAD_STATE_COUNT;
	MACH_CALL(thread_get_state(thread->kernel_thread,
				   HP700_THREAD_STATE,
				   (thread_state_t) &state,
				   &count),
		  r);
	ts->iioq_head = (int) routine;
	ts->iioq_tail = (int) routine + 4;
	ts->arg0 = (int) thread;
	ts->sp = vsp;
	ts->dp = _dp();
	ts->rp = 0;
	ts->r3 = 0;
	sp[-1] = sp[-5] = 0;	/* Clear saved SP and RP in frame. */
	MACH_CALL(thread_set_state(thread->kernel_thread,
				   HP700_THREAD_STATE,
				   (thread_state_t) &state,
				   HP700_THREAD_STATE_COUNT),
		  r);
}
