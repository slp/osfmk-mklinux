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
 * Revision 2.10.3.1  92/03/28  10:10:31  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:17:45  jeffreyh]
 * 
 * Revision 2.11  92/02/19  16:06:53  elf
 * 	Change calls to compute_priority.
 * 	[92/01/19            rwd]
 * 	Changed thread_depress_priority to not schedule a timeout when
 * 	time is 0.
 * 	[92/01/10            rwd]
 * 
 * Revision 2.10  91/07/31  17:48:19  dbg
 * 	Fix timeout race.
 * 	[91/07/30  17:05:37  dbg]
 * 
 * Revision 2.9  91/05/18  14:33:47  rpd
 * 	Changed to use thread->depress_timer.
 * 	[91/03/31            rpd]
 * 
 * Revision 2.8  91/05/14  16:47:24  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/05/08  12:48:54  dbg
 * 	Add volatile declarations.
 * 	Removed history for non-existent routines.
 * 	[91/04/26  14:43:58  dbg]
 * 
 * Revision 2.6  91/03/16  14:51:54  rpd
 * 	Renamed ipc_thread_will_wait_with_timeout
 * 	to thread_will_wait_with_timeout.
 * 	[91/02/17            rpd]
 * 	Added swtch_continue, swtch_pri_continue, thread_switch_continue.
 * 	[91/01/17            rpd]
 * 
 * Revision 2.5  91/02/05  17:29:34  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:18:14  mrt]
 * 
 * Revision 2.4  91/01/08  15:17:15  rpd
 * 	Added continuation argument to thread_run.
 * 	[90/12/11            rpd]
 * 	Added continuation argument to thread_block.
 * 	[90/12/08            rpd]
 * 
 * Revision 2.3  90/11/05  14:31:36  rpd
 * 	Restored missing multiprocessor untimeout failure code.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.2  90/06/02  14:56:17  rpd
 * 	Updated to new scheduling technology.
 * 	[90/03/26  22:19:48  rpd]
 * 
 * Revision 2.1  89/08/03  15:52:39  rwd
 * Created.
 * 
 *  3-Aug-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed all non-MACH code.
 *
 *  6-Dec-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Removed old history.
 *
 * 19-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	MACH_TT: boolean for swtch and swtch_pri is now whether there is
 *	other work that the kernel could run instead of this thread.
 *
 *  7-May-87  David Black (dlb) at Carnegie-Mellon University
 *	New versions of swtch and swtch_pri for MACH_TT.  Both return a
 *	boolean indicating whether a context switch was done.  Documented.
 *
 * 31-Jul-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Changed TPswtch_pri to set p_pri to 127 to make sure looping
 *	processes which want to simply reschedule do not monopolize the
 *	cpu.
 *
 *  3-Jul-86  Fil Alleva (faa) at Carnegie-Mellon University
 *	Added TPswtch_pri().  [Added to Mach, 20-jul-86, mwyoung.]
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

#include <cpus.h>

#include <mach/boolean.h>
#include <mach/thread_switch.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <kern/counters.h>
#include <kern/etap_macros.h>
#include <kern/ipc_kobject.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/ipc_sched.h>
#include <kern/spl.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/time_out.h>
#include <kern/ast.h>
#include <mach/policy.h>

#include <kern/syscall_subr.h>
#include <mach/mach_host_server.h>
#include <mach/mach_syscalls.h>

/* Forwards */
void	thread_depress_priority(
		thread_t		thread,
		mach_msg_timeout_t	depress_time);

/*
 *	swtch and swtch_pri both attempt to context switch (logic in
 *	thread_block no-ops the context switch if nothing would happen).
 *	A boolean is returned that indicates whether there is anything
 *	else runnable.
 *
 *	This boolean can be used by a thread waiting on a
 *	lock or condition:  If FALSE is returned, the thread is justified
 *	in becoming a resource hog by continuing to spin because there's
 *	nothing else useful that the processor could do.  If TRUE is
 *	returned, the thread should make one more check on the
 *	lock and then be a good citizen and really suspend.
 */

boolean_t
swtch(void)
{
	register processor_t	myprocessor;
	boolean_t		retval;

#if	NCPUS > 1
	mp_disable_preemption();
	myprocessor = current_processor();
	if (myprocessor->runq.count == 0 &&
	    myprocessor->processor_set->runq.count == 0) {
		mp_enable_preemption();
		return(FALSE);
	}
	mp_enable_preemption();
#endif	/* NCPUS > 1 */

	counter(c_swtch_block++);
	thread_block((void (*)(void))0);
	mp_disable_preemption();
	myprocessor = current_processor();
	retval = (
#if	NCPUS > 1
	       myprocessor->runq.count > 0 ||
#endif	/*NCPUS > 1*/
	       myprocessor->processor_set->runq.count > 0);
	mp_enable_preemption();
	return retval;
}


boolean_t
swtch_pri(
	  int pri)
{
	register thread_t	thread = current_thread();
	register processor_t	myprocessor;
	boolean_t		retval;

#ifdef	lint
	pri++;
#endif	/* lint */

#if	NCPUS > 1
	mp_disable_preemption();
	myprocessor = current_processor();
	if (myprocessor->runq.count == 0 &&
	    myprocessor->processor_set->runq.count == 0) {
		mp_enable_preemption();
		return(FALSE);
	}
	mp_enable_preemption();
#endif	/* NCPUS > 1 */

	/*
	 *	XXX need to think about depression duration.
	 *	XXX currently using min quantum.
	 */
	thread_depress_priority(thread, min_quantum);

	counter(c_swtch_pri_block++);
	thread_block((void (*)(void))0);

	if (thread->depress_priority != -1)
		(void) thread_depress_abort(thread->top_act);
	mp_disable_preemption();
	myprocessor = current_processor();
	retval = (
#if	NCPUS > 1
	       myprocessor->runq.count > 0 ||
#endif	/*NCPUS > 1*/
	       myprocessor->processor_set->runq.count > 0);
	mp_enable_preemption();
	return retval;
}

extern int hz;

/*
 *	thread_switch:
 *
 *	Context switch.  User may supply thread hint.
 *
 *	Fixed priority threads that call this get what they asked for
 *	even if that violates priority order.
 */
kern_return_t
syscall_thread_switch(
	mach_port_t		thread_name,
	int			option,
	mach_msg_timeout_t	option_time)
{
    register thread_t		cur_thread = current_thread();
    register processor_t	myprocessor;
    ipc_port_t			port;

    /*
     *	Process option.
     */
    switch (option) {
	case SWITCH_OPTION_NONE:
	    /*
	     *	Nothing to do.
	     */
	    break;

	case SWITCH_OPTION_DEPRESS:
	case SWITCH_OPTION_IDLE:
	    /*
	     *	Depress priority or go idle for a given time.
	     */
	    thread_depress_priority(cur_thread, option_time);
	    break;

	case SWITCH_OPTION_WAIT:
	    thread_will_wait_with_timeout(cur_thread, option_time);
	    break;

	default:
	    return(KERN_INVALID_ARGUMENT);
    }
    
    /*
     *	Check and use thr_act hint if appropriate.
     */
    if ((thread_name != 0) &&
	(ipc_port_translate_send(cur_thread->top_act->task->itk_space,
				 thread_name, &port) == KERN_SUCCESS)) {
	    /* port is locked, but it might not be active */

	    /*
	     *	Get corresponding thread.
	     */
	    if (ip_active(port) && (ip_kotype(port) == IKOT_ACT)) {
			register thread_t thread;
			register thread_act_t thr_act;
			register int s;

			thr_act = (thread_act_t) port->ip_kobject;
			assert(thr_act != THR_ACT_NULL);
			/*
			 *	Check if the thread is in the right pset. Then
			 *	pull it off its run queue.  If it
			 *	doesn't come, then it's not eligible.
			 */
			thread = act_lock_thread(thr_act);
			if (thread) {
				s = splsched();
				thread_lock(thread);
				if (thread &&
					(thread->processor_set == cur_thread->processor_set) &&
					(rem_runq(thread) != RUN_QUEUE_NULL)) {
					/*
					 *	Hah, got it!!
					 */
					thread_unlock(thread);
					splx(s);
					act_unlock_thread(thr_act);
					ip_unlock(port);
					/* XXX thread might disappear on us now? */
					if ( (thread->policy == POLICY_FIFO) ||
						 (thread->policy == POLICY_RR) ) {
						mp_disable_preemption();
						myprocessor = current_processor();
						myprocessor->quantum = thread->unconsumed_quantum;
						myprocessor->first_quantum = TRUE;
						mp_enable_preemption();
					}
					counter(c_thread_switch_handoff++);
					thread_run((void(*)(void))SAFE_THR_DEPRESS, thread);

					goto out;
				}
				thread_unlock(thread);
				splx(s);
			}
			act_unlock_thread(thr_act);
	    }
	    ip_unlock(port);
    }

    /*
     *	No handoff hint supplied, or hint was wrong.  Call thread_block() in
     *	hopes of running something else.  If nothing else is runnable,
     *	thread_block will detect this.  WARNING: thread_switch with no
     *	option will not do anything useful if the thread calling it is the
     *	highest priority thread (can easily happen with a collection
     *	of timesharing threads).
     */
    mp_disable_preemption();
    myprocessor = current_processor();
#if	NCPUS > 1
    if (myprocessor->processor_set->runq.count > 0 ||
	myprocessor->runq.count > 0)
#endif	/* NCPUS > 1 */
    {
	counter(c_thread_switch_block++);
	myprocessor->first_quantum = FALSE;
	mp_enable_preemption();
	thread_block_reason((void (*)(void))SAFE_THR_DEPRESS, AST_QUANTUM);
    }
#if	NCPUS > 1
    else {
	mp_enable_preemption();
    }
#endif	/* NCPUS > 1 */

out:
    if (option == SWITCH_OPTION_WAIT)
	reset_timeout_check(&cur_thread->timer);

    /*
     *  Restore depressed priority unless we just did a SWITCH_OPTION_IDLE.
     */
    if (cur_thread->depress_priority != -1) {
	if (option != SWITCH_OPTION_IDLE)
	    (void) thread_depress_abort(cur_thread->top_act);
    } else {
	if (option == SWITCH_OPTION_IDLE)
	    return(KERN_ABORTED);
    }
    return(KERN_SUCCESS);
}

/*
 *	thread_depress_priority
 *
 *	Depress thread's priority to lowest possible for specified period.
 *	Intended for use when thread wants a lock but doesn't know which
 *	other thread is holding it.  As with thread_switch, fixed
 *	priority threads get exactly what they asked for.  Users access
 *	this by the SWITCH_OPTION_DEPRESS option to thread_switch.  A Time
 *      of zero will result in no timeout being scheduled.
 */
void
thread_depress_priority(
	register thread_t	thread,
	mach_msg_timeout_t	depress_time)
{
    natural_t ticks;
    spl_t	s;

    /* convert from milliseconds to ticks */
    ticks = convert_ipc_timeout_to_ticks(depress_time);

    s = splsched();
    thread_lock(thread);

    /*
     *	If thread is already depressed, override previous depression.
     */
    reset_timeout_check(&thread->depress_timer);

    /*
     * If we haven't already saved the priority to be restored
     * (depress_priority), then save it (this case can happen when a thread
     * does a SWITCH_OPTION_IDLE).
     */
    if (thread->depress_priority < 0)
      thread->depress_priority = thread->priority;

    /* Set the thread's priority to the DEPRESSPRI so that it runs last. */
    thread->priority = DEPRESSPRI;
    assert(thread->runq == RUN_QUEUE_NULL);
    thread->sched_pri = DEPRESSPRI;
    if (ticks != 0)
	set_timeout(&thread->depress_timer, ticks);

    thread_unlock(thread);

    ETAP_PROBE_DATA(ETAP_P_DEPRESSION, EVENT_BEGIN, thread, 0, 0);

    splx(s);
}	

/*
 *	thread_depress_timeout:
 *
 *	Timeout routine for priority depression.
 */
void
thread_depress_timeout(
	register thread_t thread)
{
    spl_t s;
    etap_data_t	probe_data;


    ETAP_DATA_LOAD(probe_data[0], 0);
    ETAP_PROBE_DATA(ETAP_P_DEPRESSION,
		    EVENT_END,
		    thread,
		    &probe_data,
		    ETAP_DATA_ENTRY);

    s = splsched();
    thread_lock(thread);

    /*
     *	If we lose a race with thread_depress_abort,
     *	then depress_priority might be -1.
     */

    if (thread->depress_priority >= 0) {
	/*
	 * Don't reschedule immediately the thread since the currently
	 * scheduled ones may own resources that it may want to acquire.
	 */
	thread->priority = thread->depress_priority;
	thread->depress_priority = -1;
	compute_priority(thread, FALSE);

    } else if (thread->depress_priority == -2) {
	/* Thread was temporarily undepressed by thread_suspend, to
	 * be redepressed in special_handler as it blocks.  We need to
	 * prevent special_handler from redepressing it, since depression
	 * has timed out:
	 */
	thread->depress_priority = -1;
    }

    thread_unlock(thread);
    splx(s);
}

/*
 *	thread_depress_abort:
 *
 *	Prematurely abort priority depression if there is one.
 */
kern_return_t
thread_depress_abort(
	register thread_act_t	thr_act)
{
    kern_return_t		kr;
    thread_t			thread;

    if (thr_act == THR_ACT_NULL)
	return(KERN_INVALID_ARGUMENT);
    thread = act_lock_thread(thr_act);

    /* if activation is terminating, this operation is not meaningful */
    if (!thr_act->active) {
	act_unlock_thread(thr_act);
	return(KERN_TERMINATED);
    }

    if (thread == THREAD_NULL) {
	act_unlock_thread(thr_act);
	return(KERN_INVALID_ARGUMENT);
    }

    kr = thread_depress_abort_fast(thread);
    act_unlock_thread(thr_act);
    return(kr);
}

/*
 * Actually abort depressed priority of thread.
 *
 * Called with all thread-related locks for thread held (see
 * act_lock_thread()).
 */
kern_return_t
thread_depress_abort_fast(
    thread_t			thread)
{
    spl_t			s;
    etap_data_t			probe_data;
    kern_return_t 		kr = KERN_SUCCESS;


    ETAP_DATA_LOAD(probe_data[0], 1);
    ETAP_PROBE_DATA(ETAP_P_DEPRESSION,
		    EVENT_END,
		    thread,
		    &probe_data,
		    ETAP_DATA_ENTRY);

    /*
     *	Only restore priority if thread is depressed.
     */
    s = splsched();
    thread_lock(thread);
    if (thread->depress_priority >= 0) {
	reset_timeout_check(&thread->depress_timer);
	thread->priority = thread->depress_priority;
	thread->depress_priority = -1;
	compute_priority(thread, TRUE);
    } else if (thread->depress_priority == -2) {
	reset_timeout_check(&thread->depress_timer);
	thread->depress_priority = -1;
	compute_priority(thread, TRUE);
    } else
	kr = KERN_NOT_DEPRESSED;
    thread_unlock(thread);
    splx(s);

    return (kr);
}
