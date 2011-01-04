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
 * MACH_KERNEL-dependent  startup of the event package.
 */

#include <xkern/include/domain.h>
#include <mach/message.h>
#include <mach/mach_host_server.h>
#include <kern/ipc_sched.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/upi.h>
#include <xkern/include/platform.h>
#include <xkern/include/assert.h>
#include <xkern/include/event.h>
#include <xkern/include/event_i.h>
#include <xkern/include/xk_path.h>

/*
 * evInit -- initialize the timer queues and start the clock thread.
 *
 */
xkern_return_t
evInit(
	int interval)
{
    int i;

#ifdef XK_THREAD_TRACE
    localEventMap = mapCreate(BIG_N, sizeof(Event), xkSystemPath);
    threadEventMap = mapCreate(BIG_N, sizeof(THREAD_T), xkSystemPath);
    if ( ! localEventMap || ! threadEventMap ) {
	xTrace0(event, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
#endif

    for (i=0; i<BIG_N; i++) {
        evhead[i].evnext = (queue_chain_t *)&evhead[i];
        evhead[i].evprev = (queue_chain_t *)&evhead[i];
        evhead[i].func = THIS_IS_THE_HEADER;
    }
    tickmod = 0;
 
    /* initialize timers and semaphores */
    EVENT_LOCK_INIT();
  
    xTrace0(event,TR_EVENTS,"evInit starting evClock thread");

    xk_thread_checkout(FALSE);
    (void)kernel_thread(kernel_task, evClock_setup, (char *) 0);
    xk_thread_checkin();

    return XK_SUCCESS;
}

void 
evClock_setup(void)
{
    thread_t	thread = current_thread();
    int		priority = XK_CLOCK_THREAD_PRIORITY;

    stack_privilege(thread);
    /*
     * Set thread scheduling priority and policy.
     */
    if ( xkPolicyFixedFifo(&priority, sizeof(priority)) != XK_SUCCESS ) {
        printf("WARNING: the clock thread is being TIMESHARED!\n");
    }

    evClock(0);
    /* NOTREACHED */
}

/*
 * evClock --- update the event queues periodically
 */

#if XK_DEBUG
int xk_heart;
#endif /* XK_DEBUG */

void 
evClock(int foo)
{
    Event e;
    boolean_t  ret;

    for (;;) {
#if XK_DEBUG
	xk_heart++;
#endif /* XK_DEBUG */
	EVENT_LOCK();
	((Event)evhead[tickmod].evnext)->deltat--;
	tickmod = (tickmod+1)%BIG_N; /* optimize if BIG_N is power of 2*/
	for (e = (Event)evhead[tickmod].evnext; 
	     e != &evhead[tickmod] && e->deltat == 0; 
	     e = (Event)e->evnext) {
	        remque((queue_entry_t) e);
	        e->state = E_SCHEDULED;
#ifdef XK_THREAD_TRACE
	        xGetTime(&e->startTime);
#endif
	        xTrace0(event, TR_EVENTS, "evClock starting event");
	        CreateProcess((gen_func) xk_event_stub, e, 1, e);
	}
	thread_will_wait_with_timeout(current_thread(), event_granularity);
	EVENT_UNLOCK();
	thread_block((void (*)(void)) 0);
	reset_timeout_check(&current_thread()->timer);
    }
}




