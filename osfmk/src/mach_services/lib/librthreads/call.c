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
 * call.c
 *
 * System call wrappers.
 */

#include <rthreads.h>
#include <rthread_internals.h>
#include <options.h>


kern_return_t
rthread_abort(rthread_t th)
{
	return(thread_abort(th->wired));
}

kern_return_t
rthread_resume(rthread_t th)
{
	return(thread_resume(th->wired));
}

kern_return_t
rthread_suspend(rthread_t th)
{
	return(thread_suspend(th->wired));
}

kern_return_t
rthread_switch(rthread_t th, int option, int timeout)
{
	if (th != RTHREAD_NULL)
		return(thread_switch(th->wired, option, timeout));
	else
		return(thread_switch(MACH_PORT_NULL, SWITCH_OPTION_NONE, 0));
}

/* Include on those platforms where ETAP is implemented in the microkernel. */
#if	defined(i386) || defined(hp_pa)
kern_return_t
rthread_trace(rthread_t th, boolean_t active)
{
	return(etap_trace_thread(th->wired, active));
}
#endif
