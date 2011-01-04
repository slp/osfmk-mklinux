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


#include <mach.h>
#include <tgdb.h>
#include <threads.h>

/*
 * For i386, thread pointer is saved in highest word of the stack
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
set_thread_self(mach_thread_t thread)
{
    mach_thread_t *th_pt;

    th_pt = (mach_thread_t *)
            (thread->stack+STACK_SIZE-sizeof(mach_thread_t));
    *th_pt = thread;
}

void thread_set_regs(vm_offset_t entry, mach_thread_t th)
{
	int *top;
	kern_return_t	rc;
	struct i386_thread_state state;
	struct i386_thread_state *ts = &state;
	unsigned size;

	size = i386_THREAD_STATE_COUNT;
	if((rc = thread_get_state(th->thread, i386_THREAD_STATE, 
	    (thread_state_t) &state, &size)) != KERN_SUCCESS) {
		printf("tgdb: thread_get_state failed\n");
		return;
	}

        ts->eip = (int) entry;
	top = (int *) (th->stack+STACK_SIZE-sizeof(mach_thread_t));
        ts->uesp = (int) top;   /* set stack pointer */
        ts->ebp = 0;            /* clear frame pointer */



	if((rc = thread_set_state(th->thread, i386_THREAD_STATE, 
                                  (thread_state_t) &state,
				  i386_THREAD_STATE_COUNT)) != KERN_SUCCESS) {
		printf("tgdb: thread_set_state failed\n");
		return;
	}
}
