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
#include <mach/machine/thread_status.h>

kern_return_t
thread_set_regs(state, pc, arg, stack, ssize)
struct hp700_thread_state *state;
unsigned int pc, arg, stack, ssize;
{
    state->iioq_head = pc;
    state->iioq_tail = pc + 4;
    state->arg0 = arg;

    /* fake a stack frame */
    state->sp = stack + 64;

    /* copy data pointer */
    state->dp = get_dp();
}

void
hp700_fork(parent_thread, child_thread, state, count)
mach_port_t parent_thread;
mach_port_t child_thread;
thread_state_t state;
mach_msg_type_number_t count;
{
	struct hp700_thread_state old_state;
	struct hp700_thread_state *new_state;

	struct hp700_float_state old_float_state;
	struct hp700_float_state new_float_state;
	mach_msg_type_number_t float_count = HP700_FLOAT_STATE_COUNT;

	MACH_CALL( thread_get_state, (child_thread, THREAD_STATE,
			       (thread_state_t)&old_state, &count));

	new_state = (struct hp700_thread_state *)state;
	new_state->iisq_head = old_state.iisq_head;
	new_state->iisq_tail = old_state.iisq_tail;

	new_state->sr0 = old_state.sr0;
	new_state->sr1 = old_state.sr1;
	new_state->sr2 = old_state.sr2;
	new_state->sr3 = old_state.sr3;

	MACH_CALL( thread_get_state, (parent_thread,
				      HP700_FLOAT_STATE,
				      (thread_state_t)&old_float_state,
				      &float_count));

#if 0
	MACH_CALL( thread_get_state, (child_thread,
				      HP700_FLOAT_STATE,
				      (thread_state_t)&new_float_state,
				      &float_count));

	new_float_state.fr12 = old_float_state.fr12;
	new_float_state.fr13 = old_float_state.fr13;
	new_float_state.fr14 = old_float_state.fr14;
	new_float_state.fr15 = old_float_state.fr15;
	new_float_state.fr16 = old_float_state.fr16;
	new_float_state.fr17 = old_float_state.fr17;
	new_float_state.fr18 = old_float_state.fr18;
	new_float_state.fr19 = old_float_state.fr19;
	new_float_state.fr20 = old_float_state.fr20;
	new_float_state.fr21 = old_float_state.fr21;
	new_float_state.fr22 = old_float_state.fr22;
	new_float_state.fr23 = old_float_state.fr23;
	new_float_state.fr24 = old_float_state.fr24;
	new_float_state.fr25 = old_float_state.fr25;
	new_float_state.fr26 = old_float_state.fr26;
	new_float_state.fr27 = old_float_state.fr27;
	new_float_state.fr28 = old_float_state.fr28;
	new_float_state.fr29 = old_float_state.fr29;
	new_float_state.fr30 = old_float_state.fr30;
	new_float_state.fr31 = old_float_state.fr31;
#endif

	MACH_CALL( thread_set_state, (child_thread,
				      HP700_FLOAT_STATE,
				      (thread_state_t)&old_float_state,
				      float_count));
}

void	(*machine_fork)(mach_port_t 		parent_thread,
			mach_port_t 		child_thread,
			thread_state_t 		state,
			mach_msg_type_number_t	count) = hp700_fork;	

/*
 * For hp700, thread pointer is saved in highest word of the stack
 */
mach_thread_t
thread_self()
{
    volatile int stackp;
    mach_thread_t *th;

    if (threads_initialized)
	th = (mach_thread_t *)
	     (thread_stack(&stackp)+STACK_SIZE-sizeof(mach_thread_t));
    else
	return(&threads[0]);

    return *th;

}

void
set_thread_self(thread)
mach_thread_t thread;
{
    mach_thread_t *th_pt;

    th_pt = (mach_thread_t *)
            (thread->stack+STACK_SIZE-sizeof(mach_thread_t));
    *th_pt = thread;
}



