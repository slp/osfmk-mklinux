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
 * Copyright (c) 1989 Carnegie Mellon University.
 * Copyright (c) 1990,1991 The University of Utah and
 * the Center for Software Science (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
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
 *
 * 	Utah $Hdr$
 */

#include <cthreads.h>
#include <mach.h>
#include <string.h>
#include "cthread_internals.h"

static __inline__ void *
get_dp(void)
{
	void *_dp__;

	__asm__ ("copy %%r27,%0" : "=r" (_dp__));
	return _dp__;
}

/*
 * Set up the initial state of a MACH thread so that it will invoke
 * routine(child) when it is resumed.
 */
void
cthread_setup(cthread_t child, thread_port_t thread, cthread_fn_t routine)
{
	struct hp700_thread_state state;
	kern_return_t r;
	unsigned size;

	/*
	 * Set up hp700 call frame and registers.
	 */
	size = HP700_THREAD_STATE_COUNT;
	MACH_CALL(thread_get_state(thread, HP700_THREAD_STATE, 
				   (thread_state_t) &state, &size), r);

	/*
	 * set the PC queue to point to routine.
	 */
	state.iioq_head = (unsigned)routine;
	state.iioq_tail = (unsigned)routine + 4;

	/*
	 * setup the first argument to routine to be the address of child.
	 */
	state.arg0 = (unsigned)child;

	state.dp = (unsigned)get_dp();

	/*
	 * establish a user stack for this thread. There is no guarentee
	 * that the stack is aligned correctly. We have to align it to
	 * a double word boundary.  (In fact, if CTHREAD_STACK_OFFSET
	 * is aligned, then so is the stack.  But little things can go
	 * wrong.)
	 */
	state.sp = (cthread_stack_base(child, CTHREAD_STACK_OFFSET) + 7) & ~7;

	/*
	 * clear off the first 48 bytes of the stack, this is going to be 
	 * our fake stack marker, we don't want any routine to think there
	 * is a stack frame below us.
	 */
	memset((char *)state.sp, '\0', 48);

	/*
	 * now set the stack pointer 48 bytes deeper, 32 for a frame marker 
	 * and 16 for the standard 4 arguments.
	 */
	state.sp += 48;

	/*
	 * Now we have to initialize the floating point status registers
	 * since they will be in a random state and could cause a trap.
	state.fr0 = 0.0;
	state.fr1 = 0.0;
	state.fr2 = 0.0;
	state.fr3 = 0.0;
	 */

	/*
	 * the psw is fine it is set up as CQPDI by the kernel. Set the
	 * registers into the saved state of the thread.
	 */
	MACH_CALL(thread_set_state(thread, HP700_THREAD_STATE,
				   (thread_state_t) &state,
				   HP700_THREAD_STATE_COUNT),r);
}
