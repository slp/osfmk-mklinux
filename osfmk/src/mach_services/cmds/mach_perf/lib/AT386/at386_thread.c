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

#include <mach_perf.h>
#include <mach/i386/thread_status.h>

extern thread_start();

kern_return_t
thread_set_regs(state, pc, arg0, sp, ssize)
struct i386_thread_state *state;
unsigned int pc, arg0, sp, ssize;
{
	state->eip = (unsigned int) thread_start;
	state->eax = pc;	
	state->ebx = arg0;
	state->uesp = sp + ssize;
}

/*
 * For i386, thread pointer is saved in lowest word of the stack
 */

#define mach_thread(addr)	(*(struct mach_thread **)thread_stack(addr))

mach_thread_t
thread_self()
{
	int i;

	if (threads_initialized)
	 	return(mach_thread(&i));
	else
		return(&threads[0]);
}

set_thread_self(thread)
mach_thread_t thread;
{
  	mach_thread_t *th_pt;

	th_pt = (mach_thread_t *)thread->stack;
	*th_pt = thread;
}

