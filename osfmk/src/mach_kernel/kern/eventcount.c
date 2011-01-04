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
/* CMU_HIST */
/*
 * Revision 2.3.2.1  92/03/03  16:19:57  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  11:53:57  jeffreyh]
 * 
 * Revision 2.4  92/01/03  20:40:02  dbg
 * 	Made user-safe with a small translation table.
 * 	This all will be refined at a later time.
 * 	[91/12/27            af]
 * 
 * Revision 2.3  91/12/14  14:31:43  jsb
 * 	Replaced gimmeabreak calls with panics.
 * 
 * Revision 2.2  91/12/13  14:54:44  jsb
 * 	Created.
 * 	[91/11/01            af]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
/*
 */
/*
 *	File:	eventcount.c
 *	Author:	Alessandro Forin
 *	Date:	10/91
 *
 *	Eventcounters, for user-level drivers synchronization
 *
 */


#include <cpus.h>

#include <mach/machine.h>
#include <kern/ast.h>
#include <kern/cpu_number.h>
#include <kern/lock.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <mach/mach_syscalls.h>

#include <kern/eventcount.h>

#define	MAX_EVCS	10		/* XXX for now */
evc_t	all_eventcounters[MAX_EVCS];


/*
 * Initialization
 */
void
evc_init(
	evc_t	ev)
{
	int i;

	bzero((char *)ev, sizeof(*ev));

	/* keep track of who is who */
	for (i = 0; i < MAX_EVCS; i++)
		if (all_eventcounters[i] == 0) break;
	if (i == MAX_EVCS) {
		printf("Too many eventcounters\n");
		return;
	}

	all_eventcounters[i] = ev;
	ev->ev_id = i;
	ev->sanity = ev;
	ev->waiting_thread = THREAD_NULL;
	simple_lock_init(&ev->lock, ETAP_MISC_EVENT);
}

/*
 * Finalization
 */
void
evc_destroy(
	evc_t	ev)
{
	evc_signal(ev);
	ev->sanity = 0;
	if (all_eventcounters[ev->ev_id] == ev)
		all_eventcounters[ev->ev_id] = 0;
	ev->ev_id = -1;
}


/*
 * User-trappable
 */
kern_return_t
evc_wait(
	unsigned ev_id)
{
	spl_t	s;
	kern_return_t	ret;
	evc_t	ev;

	if ((ev_id >= MAX_EVCS) ||
	    ((ev = all_eventcounters[ev_id]) == 0) ||
	    (ev->ev_id != ev_id) || (ev->sanity != ev))
		return KERN_INVALID_ARGUMENT;

	s = splsched();
	simple_lock(&ev->lock);
	if (ev->count) {
		ev->count--;
		ret = KERN_SUCCESS;
	} else {
		if (ev->waiting_thread == THREAD_NULL) {
			ev->waiting_thread = current_thread();
			assert_wait( 0, TRUE);	/* ifnot race */
			simple_unlock(&ev->lock);
			thread_block((void(*)(void))0);
			return(KERN_SUCCESS);
		}
		ret = KERN_NO_SPACE; /* XXX */
	}
	simple_unlock(&ev->lock);
	splx(s);

	return ret;
}

/*
 * Called exclusively from interrupt context
 */
void
evc_signal(
	evc_t	ev)
{
	register thread_t	thread;
	register int		state;
	spl_t	s;

	if (ev->sanity != ev)
		return;

	s = splsched();
	simple_lock(&ev->lock);

	ev->count++;
	thread = ev->waiting_thread;
	ev->waiting_thread = 0;
	if (thread == THREAD_NULL)
		goto done;

	/* make thread runnable on this processor */
	/* taken from clear_wait */

	state = thread->state;

/*	reset_timeout_check(&thread->timer);	*/

	switch (state & TH_SCHED_STATE) {
	    case	  TH_WAIT | TH_SUSP | TH_UNINT:
	    case	  TH_WAIT	    | TH_UNINT:
	    case	  TH_WAIT:
		/*
		 *	Sleeping and not suspendable - put
		 *	on run queue.
		 */
		thread->state = (state &~ TH_WAIT) | TH_RUN;
#if	THREAD_SWAPPER
		if (state & TH_SWAPPED_OUT)
			thread_swapin(thread->top_act, FALSE);
		else
#endif	/* THREAD_SWAPPER */
			thread_setrun(thread, TRUE, HEAD_Q);
		break;

	    case	  TH_WAIT | TH_SUSP:
	    case TH_RUN | TH_WAIT:
	    case TH_RUN | TH_WAIT | TH_SUSP:
	    case TH_RUN | TH_WAIT	    | TH_UNINT:
	    case TH_RUN | TH_WAIT | TH_SUSP | TH_UNINT:
		/*
		 *	Either already running, or suspended.
		 */
		panic("evc_signal.1");
		thread->state = state &~ TH_WAIT;
		break;

	    default:
		/*
		 *	Not waiting.
		 */
		panic("evc_signal.2");
		break;
	}
done:
	simple_unlock(&ev->lock);
	splx(s);
}
