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
 * Adapted from process.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 */

#include <mach/mach_host_server.h>
#include <mach/mach_server.h>
#include <kern/ipc_sched.h>
#include <kern/thread.h>
#include <xkern/include/xkernel.h>
#include <xkern/include/assert.h>
#include <xkern/include/platform.h>
#include <xkern/include/event_i.h>
#include <uk_xkern/include/eth_support.h>

/* forwards */

static void xk_monitor(void);
static void xk_pool(void);
static void xk_loop(void);

#define MAXARGS		6

/*
 * The InfoQ is used by the xkernel master monitor (xk_loop).
 */
queue_head_t	xkInfoQ;

int xk_service_thread_out;

decl_simple_lock_data(, xkMaster_lock)
decl_simple_lock_data(, xkService_lock)

#if XK_DEBUG
int		xklockdepthreq;
int		xklockdepth;
int		tracexklock;
#endif /* XK_DEBUG */

typedef struct {
    queue_chain_t	link;	
    void		*args[MAXARGS + 2];
} argBlock;

/*
 * We're about to leave the x-kernel world.
 * Release the master lock. We may want to
 * restart the proper input queue, if we
 * decide that we'll be out for a while
 * (must_restart==TRUE).
 * Note: in case of input threads, a new PDU 
 * from the wire will restart the queue anyways.
 */
void
xk_thread_checkout(
		   boolean_t must_restart)
{
    thread_t thread = current_thread();
    
    xk_master_unlock();
    if (is_xk_input_thread(thread)) {
	thread_act_t    a_self = current_act();	
	InputPool       *pool = (InputPool *)a_self->xk_resources;
	int s;

	if (must_restart)
	    thread_wakeup_one((event_t) pool);

	s = splimp();
	simple_lock(&pool->iplock);
	pool->threads_out++;
	assert(pool->threads_out > 0 && pool->threads_out <= pool->threads_max);
	simple_unlock(&pool->iplock);
	splx(s);
    } else if (is_xk_service_thread(thread)) {
	
 	if (must_restart)
	    thread_wakeup_one((event_t) xk_loop);
	
	simple_lock(&xkService_lock);
	xk_service_thread_out++;
	simple_unlock(&xkService_lock);
    } 
}

/*
 * We're back to the x-kernel world.
 * We must update counters and grab
 * the master lock.
 */
void
xk_thread_checkin(
		  void)
{    
    thread_t thread = current_thread();

    if (is_xk_input_thread(thread)) {
	thread_act_t    a_self = current_act();	
	InputPool       *pool = (InputPool *)a_self->xk_resources;
	int s;

	s = splimp();
	simple_lock(&pool->iplock);
	pool->threads_out--;
	assert(pool->threads_out >= 0 && pool->threads_out < pool->threads_max);
	simple_unlock(&pool->iplock);
	splx(s);
    } else if (is_xk_service_thread(thread)) {
	simple_lock(&xkService_lock);
 	xk_service_thread_out--;
	simple_unlock(&xkService_lock);
    }
    xk_master_lock();
}

/*
 * A wrapper for blocking
 */ 
static __inline__ void
xk_thread_block(
		void)
{
    xk_thread_checkout(FALSE);
    thread_block((void (*)(void)) 0);
    xk_thread_checkin();
}
				
/*
 * threadInit
 */
xkern_return_t
threadInit(void)
{
    register int i;

    queue_init(&xkInfoQ);
    /*
     * Initialize a pool of xk_service_threads
     */
    simple_lock_init(&xkService_lock, ETAP_XKERNEL_EVENT);
    xk_service_thread_out = MAX_XK_SERVICE_THREADS;
    xk_thread_checkout(FALSE);
    for (i = 0; i < MAX_XK_SERVICE_THREADS; i++)
        (void)kernel_thread(kernel_task, xk_pool, (char *) 0);
    xk_thread_checkin();
    return XK_SUCCESS;
}

#if XK_DEBUG
int xk_service_queueing;
int xk_service_wakeup;
#endif /* XK_DEBUG */

/*
 * CreateProcess(function_ptr, argcount, arg1, arg2, ...)
 * Always invoked in thread context.
 */
xkern_return_t
CreateProcess( gen_func r, Event ev, int numArgs, ...)
{
    va_list	ap;
    int 	i;
    void 	**argp;
    argBlock	*info;

    va_start(ap, numArgs);
  
    if ( numArgs > MAXARGS ) {
	xTrace1(processcreation, TR_ERRORS,
		"x-kernel CreateProcess: too many arguments (%d)", numArgs);
	return XK_FAILURE;
    }


    info = ev->extra;
    xAssert(info);
    argp = info->args;
    *argp++ = (void *) r;
    *argp++ = (void *)numArgs;
    for ( i=0; i < numArgs; i++ )  
	*argp++ = va_arg(ap, void *);

    xTrace2(processcreation, TR_MAJOR_EVENTS,
	"Mach CreateProcess: func_addr %lx nargs %d", (long) r, numArgs);

    /*
     * in the kernel we can't pass args to a thread, so we have to
     * enqueue the arg block.  The thread will dequeue the args when
     * it starts up in the master monitor
     */
    simple_lock(&xkService_lock);
    queue_enter(&xkInfoQ, info, argBlock *, link);
    if (more_service_thread()) {
	thread_wakeup_one((event_t) xk_loop);
#if XK_DEBUG
	xk_service_wakeup++;
#endif
    } else {
#if XK_DEBUG
	xk_service_queueing++;
#endif
    }
    simple_unlock(&xkService_lock);
    return XK_SUCCESS;
}


xkern_return_t
procAllocState( Event ev, Allocator a )
{
    return (ev->extra = allocGet(a, sizeof(argBlock))) ?
      		XK_SUCCESS : XK_FAILURE;
}


void
procFreeState( Event ev )
{
    allocFree(ev->extra);
}



static void
xk_pool(void)
{
    thread_t	thread = current_thread();
    int		priority = XK_EVENT_THREAD_PRIORITY;

    /*
     * Set privileges, thread scheduling priority and policy.
     */
    thread->vm_privilege = TRUE;
    stack_privilege(thread);
    set_xk_service_thread(thread);
    if ( xkPolicyFixedFifo(&priority, sizeof(priority)) != XK_SUCCESS ) {
        printf("WARNING: xk_service_thread is being TIMESHARED!\n");
    }
    xk_loop();
    /* NOTREACHED */
}

static void
xk_loop(void)
{
    argBlock    *info;
    gen_func	user_fn;
    void 	**args;
    int		nargs, ret;

    simple_lock(&xkService_lock);
    xk_service_thread_out--;
    for (;;) {
	while (queue_empty(&xkInfoQ)) {
	       assert_wait((event_t) xk_loop, FALSE);
	       xk_service_thread_out++;
	       simple_unlock(&xkService_lock);
	       thread_block((void (*)(void)) 0);
	       simple_lock(&xkService_lock);
	       xk_service_thread_out--;
	}
        queue_remove_first(&xkInfoQ, info, argBlock *, link);
	simple_unlock(&xkService_lock);

        args = info->args;
	user_fn = (gen_func)*(args++);
	nargs = (int)*(args++);

        if (nargs > MAXARGS)
            Kabort("xk_loop: cannot handle more than 6 args.");

	xTrace1(processcreation, TR_EVENTS,
                "xk_loop: starting thread %d", current_thread());

	xk_master_lock();
	switch(nargs) {
	  case 0: (*((void (*)(void))user_fn))();
	    	break;

	  case 1: (*user_fn)(args[0]);
                break;
	  case 2: (*user_fn)(args[0],args[1]);
                break;
	  case 3: (*user_fn)(args[0],args[1],args[2]);
                break;
	  case 4: (*user_fn)(args[0],args[1],args[2],args[3]);
                break;
	  case 5: (*user_fn)(args[0],args[1],args[2],args[3],args[4]);
	    	break;
	  case 6: (*user_fn)(args[0],args[1],args[2],args[3],args[4],args[5]);
		break;
        }
	xk_master_unlock();
	simple_lock(&xkService_lock);
    }
}

/*
 * Delay
 */
void
Delay(
    int		n)	/* n in ms */
{
    thread_t	thread = current_thread();

    thread_will_wait_with_timeout(thread, n);
    xk_thread_block();
    reset_timeout_check(&thread->timer);
}

/*
 * semInit
 */
void
semInit(
    xkSemaphore		*s,
    unsigned int	n)
{
    s->count = n;
}

/*
 * realP
 */
void
realP(
    xkSemaphore	*s)
{
    if (s->count < 0) {

	xTrace2(processswitch, TR_GROSS_EVENTS,
	    "Blocking P on %#x by 0x%x", s, current_thread());
#ifdef XK_THREAD_TRACE
	evMarkBlocked(s);
#endif

	xTrace2(processswitch, TR_GROSS_EVENTS,
	    "Suspending P on %#x by 0x%x", s, current_thread());
	/*
	 * We'll wait non-interruptable.  This
	 * may not be what we want to do in general ...
	 */
	assert_wait((event_t) s, FALSE);
	xk_thread_block();

	xTrace1(processswitch, TR_GROSS_EVENTS,
	    "realP wait_result: %d", current_thread()->wait_result);
	/*
	 * Resumed.
	 */
#ifdef XK_THREAD_TRACE
	evMarkRunning();
#endif
	xTrace2(processswitch, TR_MAJOR_EVENTS,
	    "Waking-up from P on %#x by 0x%x", s, current_thread());
    }
}

/*
 * realV
 */
void
realV(
    xkSemaphore	*s)
{
    xTrace2(processswitch, TR_MAJOR_EVENTS,
	"V on %#x by 0x%x", s, current_thread());

    /*
     * Are there any threads waiting on this semaphore?
     */
    if(s->count <= 0) {
	xTrace0(processswitch, TR_GROSS_EVENTS, "Resuming via V");
	thread_wakeup_one((event_t) s);
    }
}


#define do_call(str, call) 						\
    do {								\
        kern_return_t kr;						\
        kr = call;							\
	if ( kr != KERN_SUCCESS ) {					\
		sprintf(errBuf, "xkPolicy call %s returns %s", 		\
			str, mach_error_string(kr));			\
		xError(errBuf);						\
		return XK_FAILURE; 					\
	}								\
    } while (0)


#if XK_DEBUG

static xkern_return_t
threadSchedInfo( char *str )
{
    struct thread_basic_info	info;
    mach_msg_type_number_t	outCount;

    outCount = THREAD_BASIC_INFO_COUNT;
    do_call("thread_info", 
	    thread_info(current_act_fast(), THREAD_BASIC_INFO,
			(thread_info_t)&info, &outCount));
    switch (info.policy) {
      case POLICY_RR:
	{
	    struct policy_rr_info	rr_info;

	    outCount = POLICY_RR_INFO_COUNT;
	    do_call("thread_info (rr)",
		    thread_info(current_act_fast(), THREAD_SCHED_RR_INFO,
				(thread_info_t)&rr_info, &outCount));
	    xTrace4(processcreation, TR_MAJOR_EVENTS,
		    "thread policy (%s): round-robin, quantum %d, max %d, base %d",
		    str,
		    rr_info.quantum,
		    rr_info.max_priority,
		    rr_info.base_priority);
	}
	break;

      case POLICY_FIFO:
	{
	    struct policy_fifo_info	fifo_info;

	    outCount = POLICY_FIFO_INFO_COUNT;
	    do_call("thread_info (fifo)",
		    thread_info(current_act_fast(), THREAD_SCHED_FIFO_INFO,
				(thread_info_t)&fifo_info, &outCount));
	    xTrace3(processcreation, TR_MAJOR_EVENTS,
		    "thread policy (%s): fifo, max %d, base %d",
		    str,
		    fifo_info.max_priority,
		    fifo_info.base_priority);
	}
	break;

      default:
	xTrace2(processcreation, TR_MAJOR_EVENTS, "thread policy (%s): %d",
		str, info.policy);
    }
    return XK_SUCCESS;
}

#endif /* XK_DEBUG */


xkern_return_t
xkPolicyFixedRR(
    void	*arg,
    u_int	argLen)
{
    policy_rr_base_data_t rr_base;
    policy_rr_limit_data_t rr_limit;
    int	priority;

    if ( argLen != sizeof(int) ) {
	return XK_FAILURE;
    }
    priority = *(int *)arg;
    if ( priority < XK_THREAD_PRIORITY_MIN ||
	 priority > XK_THREAD_PRIORITY_MAX ) {
	return XK_FAILURE;
    }
    xTrace1(processcreation, TR_MAJOR_EVENTS, "setting round-robin priority %d",
	    priority);
#if XK_DEBUG    
    threadSchedInfo("before");
#endif

    /* Set policy: */
    rr_base.quantum = 0;
    rr_base.base_priority = priority;
    rr_limit.max_priority = priority;
    do_call("thread_set_policy",
	    thread_set_policy(current_act(),
			      current_thread()->processor_set,
			      POLICY_RR,
			      (policy_base_t)&rr_base,
			      POLICY_RR_BASE_COUNT,
			      (policy_limit_t)&rr_limit,
			      POLICY_RR_LIMIT_COUNT));
#if XK_DEBUG    
    threadSchedInfo("after");
#endif
    return XK_SUCCESS;
}


xkern_return_t
xkPolicyFixedFifo(
    void	*arg,
    u_int	argLen)
{
    policy_fifo_base_data_t	fifo_base;
    policy_fifo_limit_data_t	fifo_limit;
    int				priority;

    if ( argLen != sizeof(int) ) {
	return XK_FAILURE;
    }
    priority = *(int *)arg;
    if ( priority < XK_THREAD_PRIORITY_MIN ||
	 priority > XK_THREAD_PRIORITY_MAX ) {
	return XK_FAILURE;
    }
    xTrace1(processcreation, TR_MAJOR_EVENTS, "setting fifo priority %d",
	    priority);
#if XK_DEBUG    
    threadSchedInfo("before");
#endif

    /* Set policy: */
    fifo_base.base_priority = priority;
    fifo_limit.max_priority = priority;
    do_call("thread_set_policy",
	    thread_set_policy(current_act(),
			      current_thread()->processor_set,
			      POLICY_FIFO,
			      (policy_base_t)&fifo_base,
			      POLICY_FIFO_BASE_COUNT,
			      (policy_limit_t)&fifo_limit,
			      POLICY_FIFO_LIMIT_COUNT));
#if XK_DEBUG    
    threadSchedInfo("after");
#endif
    return XK_SUCCESS;
}



