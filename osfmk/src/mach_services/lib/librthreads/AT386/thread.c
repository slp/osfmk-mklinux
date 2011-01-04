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
 * i386/thread.c
 *
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#include <rthreads.h>
#include "rthread_internals.h"


#include <mach.h>

/*
 * Rthread library imports:
 */
extern bzero();

/*
 * Set up the initial state of a MACH thread
 * so that it will invoke rthread_body(child)
 * when it is resumed.
 */
void
rthread_setup(rthread_t child, thread_port_t thread, rthread_fn_t routine)
{
	register int *top = (int *) (child->stack_base + child->stack_size);
	struct i386_thread_state state;
	register struct i386_thread_state *ts = &state;
	kern_return_t r;
	unsigned int count;

	/*
	 * Set up i386 call frame and registers.
	 * Read registers first to get correct segment values.
	 */
	count = i386_THREAD_STATE_COUNT;
	MACH_CALL(thread_get_state(thread,i386_THREAD_STATE,(thread_state_t) &state,&count),r);

	ts->eip = (int) routine;
	top = (int *)((int)top - RTHREAD_STACK_OFFSET);
	*--top = (int) child;	/* argument to function */
	*--top = 0;		/* fake return address */
	ts->uesp = (int) top;	/* set stack pointer */
	ts->ebp = 0;		/* clear frame pointer */

	MACH_CALL(thread_set_state(thread,i386_THREAD_STATE,(thread_state_t) &state,i386_THREAD_STATE_COUNT),r);
}

#ifdef	rthread_sp
#undef	rthread_sp
#endif

int
rthread_sp()
{	int x = (int)&x;
	/* Indirect nonsense to avoid compiler warnings */
	return(x);
}
