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

#include <mach_perf.h>
#include <mach/machine/thread_status.h>

kern_return_t
thread_set_regs(state, pc, arg, stack, ssize)
struct ppc_thread_state *state;
unsigned int pc, arg, stack, ssize;
{
    state->srr0 = pc;
    state->r3 = arg;

    /* fake a stack frame */
    state->r1 = stack + ssize - 64;
    *(unsigned int*)(state->r1) = 0; /* Mark empty backpointer */
}

/*
 * For ppc, thread pointer is saved in lowest word of the stack
 */
mach_thread_t
thread_self()
{
    volatile int stackp;
    mach_thread_t *th;

    if (threads_initialized)
	th = (mach_thread_t *) (thread_stack(&stackp));
    else
	return(&threads[0]);

    return *th;

}

void
set_thread_self(thread)
mach_thread_t thread;
{
    mach_thread_t *th_pt;

    th_pt = (mach_thread_t *) thread->stack;
    *th_pt = thread;
}
