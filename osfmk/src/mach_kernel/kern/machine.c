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
 * Revision 2.12.2.1  92/01/09  18:44:10  jsb
 * 	From durriya@ri.osf.org: added host_get_boot_info.
 * 	[92/01/08  16:38:55  jsb]
 * 
 * Revision 2.12  91/07/31  17:46:12  dbg
 * 	Removed interrupt_stack - it's machine-dependent.
 * 	[91/07/26            dbg]
 * 
 * Revision 2.11  91/05/18  14:32:39  rpd
 * 	Picked up processor_doaction fix from dlb.
 * 	[91/04/08            rpd]
 * 
 * Revision 2.10  91/05/14  16:44:36  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/05/08  12:47:37  dbg
 * 	Add volatile declarations.
 * 
 * 	Preserve the control port for a processor when shutting
 * 	it down.
 * 	[91/04/26  14:42:42  dbg]
 * 
 * Revision 2.8  91/03/16  14:50:54  rpd
 * 	Added action_thread_continue.
 * 	[91/01/22            rpd]
 * 
 * Revision 2.7  91/02/05  17:28:02  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:15:17  mrt]
 * 
 * Revision 2.6  91/01/08  15:16:29  rpd
 * 	Added continuation argument to thread_block.
 * 	[90/12/08            rpd]
 * 
 * Revision 2.5  90/08/27  22:02:56  dbg
 * 	Correct PMAP_DEACTIVATE calls.
 * 	[90/07/18            dbg]
 * 
 * Revision 2.4  90/06/02  14:55:18  rpd
 * 	Updated to new host/processor technology.
 * 	[90/03/26  22:12:59  rpd]
 * 
 * Revision 2.3  90/01/11  11:43:37  dbg
 * 	Make host_reboot return SUCCESS if Debugger returns.  Remove
 * 	lint.
 * 	[89/12/06            dbg]
 * 
 * Revision 2.2  89/09/25  11:00:54  rwd
 * 	host_reboot can now enter debugger.
 * 	[89/09/20            rwd]
 * 
 * Revision 2.1  89/08/03  15:49:03  rwd
 * Created.
 * 
 * 14-Jan-89  David Golub (dbg) at Carnegie-Mellon University
 *	Changed xxx_port_allocate to port_alloc_internal.  Added
 *	host_reboot stub.
 *
 *  6-Sep-88  David Golub (dbg) at Carnegie-Mellon University
 *	Replaced old privileged-user check in cpu_control by check for
 *	host_port.  Added host_init to allocate the host port.
 *
 *  9-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	Removed next_thread check.  Handled by idle_thread now.
 *
 * 26-May-88  David Black (dlb) at Carnegie-Mellon University
 *	Add interrupt protection to cpu_doshutdown.
 *
 * 20-May-88  David Black (dlb) at Carnegie-Mellon University
 *	Added shutdown thread.  This replaces should_exit logic.
 *	Only needed for multiprocessors.
 *
 * 24-Mar-88  David Black (dlb) at Carnegie-Mellon University
 *	Maintain cpu state in cpu_up and cpu_down.
 *
 * 15-Sep-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 17-Jul-87  David Black (dlb) at Carnegie-Mellon University
 *	Bug fix to cpu_down - update slot structure correctly.
 *
 * 28-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
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
 *	File:	kern/machine.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1987
 *
 *	Support for machine independent machine abstraction.
 */

#include <cpus.h>
#include <mach_host.h>
#include <dipc.h>
#include <norma_scsi.h>
#include <dipc_xkern.h>

#include <string.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/host_info.h>
#include <mach/host_reboot.h>
#include <kern/counters.h>
#include <kern/ipc_host.h>
#include <kern/host.h>
#include <kern/lock.h>
#include <kern/machine.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/misc_protos.h>

#include <mach/mach_host_server.h>


/*
 *	Exported variables:
 */

struct machine_info	machine_info;
struct machine_slot	machine_slot[NCPUS];

queue_head_t	action_queue;	/* assign/shutdown queue */
decl_simple_lock_data(,action_lock)

/* Forwards */
processor_set_t	processor_request_action(
			processor_t	processor,
			processor_set_t	new_pset);

void		processor_doaction(
			processor_t	processor);

void		processor_doshutdown(
			processor_t	processor);

void		action_thread_continue(void);

int		thread_stop_freeze( thread_t, processor_set_t ); 

/*
 *	cpu_up:
 *
 *	Flag specified cpu as up and running.  Called when a processor comes
 *	online.
 */
void
cpu_up(
	int	cpu)
{
	register struct machine_slot	*ms;
	register processor_t	processor;
	spl_t	s;

	processor = cpu_to_processor(cpu);
	/*
	 * Can't risk blocking with no current thread established.
	 * Just twiddle our thumbs; we've got nothing better to do
	 * yet, anyway.
	 */
	while (!pset_lock_try(&default_pset))
		;
	s = splsched();
	processor_lock(processor);
#if	NCPUS > 1
	init_ast_check(processor);
#endif	/* NCPUS > 1 */
	ms = &machine_slot[cpu];
	ms->running = TRUE;
	machine_info.avail_cpus++;
	pset_add_processor(&default_pset, processor);
	processor->state = PROCESSOR_RUNNING;
	processor_unlock(processor);
	splx(s);
	pset_unlock(&default_pset);
}

/*
 *	cpu_down:
 *
 *	Flag specified cpu as down.  Called when a processor is about to
 *	go offline.
 */

void
cpu_down(
	int	cpu)
{
	register struct machine_slot	*ms;
	register processor_t	processor;
	spl_t	s;

	processor = cpu_to_processor(cpu);
	s = splsched();
	processor_lock(processor);
	ms = &machine_slot[cpu];
	ms->running = FALSE;
	machine_info.avail_cpus--;
	/*
	 *	processor has already been removed from pset.
	 */
	processor->processor_set_next = PROCESSOR_SET_NULL;
	processor->state = PROCESSOR_OFF_LINE;
	processor_unlock(processor);
	splx(s);
}

kern_return_t
host_reboot(
	host_t	host,
	int	options)
{
	if (host == HOST_NULL)
		return (KERN_INVALID_HOST);

	if (options & HOST_REBOOT_DEBUGGER) {
		Debugger("Debugger");
	} else {
		halt_all_cpus(!(options & HOST_REBOOT_HALT));
	}
	return (KERN_SUCCESS);
}

#if	NCPUS > 1
/*
 *	processor_request_action - common internals of processor_assign
 *		and processor_shutdown.  If new_pset is null, this is
 *		a shutdown, else it's an assign and caller must donate
 *		a reference.  For assign operations, it returns 
 *		an old pset that must be deallocated if it's not NULL.  For
 *		shutdown operations, it always returns PROCESSOR_SET_NULL.
 */
processor_set_t
processor_request_action(
	processor_t	processor,
	processor_set_t	new_pset)
{
    register processor_set_t pset, old_next_pset;

    /*
     *	Processor must be in a processor set.  Must lock its idle lock to
     *	get at processor state.
     */
    pset = processor->processor_set;
    simple_lock(&pset->idle_lock);

    /*
     *	If the processor is dispatching, let it finish - it will set its
     *	state to running very soon.
     */
    while (*(volatile int *)&processor->state == PROCESSOR_DISPATCHING)
    	continue;

    /*
     *	Now lock the action queue and do the dirty work.
     */
    simple_lock(&action_lock);

    switch (processor->state) {
	case PROCESSOR_IDLE:
	    /*
	     *	Remove from idle queue.
	     */
	    queue_remove(&pset->idle_queue, processor, 	processor_t,
		processor_queue);
	    pset->idle_count--;

	    /* fall through ... */
	case PROCESSOR_RUNNING:
	    /*
	     *	Put it on the action queue.
	     */
	    queue_enter(&action_queue, processor, processor_t,
		processor_queue);

	    /* fall through ... */
	case PROCESSOR_ASSIGN:
	    /*
	     * And ask the action_thread to do the work.
	     */

	    if (new_pset == PROCESSOR_SET_NULL) {
		processor->state = PROCESSOR_SHUTDOWN;
		old_next_pset = PROCESSOR_SET_NULL;
	    }
	    else {
		processor->state = PROCESSOR_ASSIGN;
	        old_next_pset = processor->processor_set_next;
	        processor->processor_set_next = new_pset;
	    }
	    break;

	default:
	    printf("state: %d\n", processor->state);
	    panic("processor_request_action: bad state");
    }
    simple_unlock(&action_lock);
    simple_unlock(&pset->idle_lock);

    thread_wakeup((event_t)&action_queue);

    return(old_next_pset);
}

#if	MACH_HOST
/*
 *	processor_assign() changes the processor set that a processor is
 *	assigned to.  Any previous assignment in progress is overriden.
 *	Synchronizes with assignment completion if wait is TRUE.
 */
kern_return_t
processor_assign(
	processor_t	processor,
	processor_set_t	new_pset,
	boolean_t	wait)
{
    spl_t	s;
    register processor_set_t	old_next_pset;

    /*
     *	Check for null arguments.
     *  XXX Can't assign master processor.
     */
    if (processor == PROCESSOR_NULL || new_pset == PROCESSOR_SET_NULL ||
	processor == master_processor) {
	    return(KERN_FAILURE);
    }

    /*
     *	Get pset reference to donate to processor_request_action.
     */
    pset_reference(new_pset);

    s = splsched();
    processor_lock(processor);
    if(processor->state == PROCESSOR_OFF_LINE ||
	processor->state == PROCESSOR_SHUTDOWN) {
	    /*
	     *	Already shutdown or being shutdown -- Can't reassign.
	     */
	    processor_unlock(processor);
	    splx(s);
	    pset_deallocate(new_pset);
	    return(KERN_FAILURE);
    }

    old_next_pset = processor_request_action(processor, new_pset);


    /*
     *	Synchronization with completion.
     */
    if (wait) {
	while (processor->state == PROCESSOR_ASSIGN ||
	    processor->state == PROCESSOR_SHUTDOWN) {
		assert_wait((event_t)processor, TRUE);
		processor_unlock(processor);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		processor_lock(processor);
	}
    }
    processor_unlock(processor);
    splx(s);
    
    if (old_next_pset != PROCESSOR_SET_NULL)
    	pset_deallocate(old_next_pset);

    return(KERN_SUCCESS);
}

#else	/* MACH_HOST */

kern_return_t
processor_assign(
	processor_t	processor,
	processor_set_t	new_pset,
	boolean_t	wait)
{
#ifdef	lint
	processor++; new_pset++; wait++;
#endif	/* lint */
	return(KERN_FAILURE);
}

#endif	/* MACH_HOST */

/*
 *	processor_shutdown() queues a processor up for shutdown.
 *	Any assignment in progress is overriden.  It does not synchronize
 *	with the shutdown (can be called from interrupt level).
 */
kern_return_t
processor_shutdown(
	processor_t	processor)
{
    spl_t s;

    s = splsched();
    processor_lock(processor);
    if(processor->state == PROCESSOR_OFF_LINE ||
	processor->state == PROCESSOR_SHUTDOWN) {
	    /*
	     *	Already shutdown or being shutdown -- nothing to do.
	     */
	    processor_unlock(processor);
	    splx(s);
	    return(KERN_SUCCESS);
    }

    (void) processor_request_action(processor, PROCESSOR_SET_NULL);
    processor_unlock(processor);
    splx(s);

    return(KERN_SUCCESS);
}

/*
 *	action_thread() shuts down processors or changes their assignment.
 */

void
action_thread(void)
{
	register processor_t	processor;
	spl_t			s;

	thread_swappable(current_act(), FALSE);

	while (TRUE) {
		s = splsched();
		simple_lock(&action_lock);
		while ( !queue_empty(&action_queue)) {
			processor = (processor_t) queue_first(&action_queue);
			queue_remove(&action_queue, processor, processor_t,
				     processor_queue);
			simple_unlock(&action_lock);
			splx(s);

			processor_doaction(processor);

			s = splsched();
			simple_lock(&action_lock);
		}

		assert_wait((event_t) &action_queue, FALSE);
		simple_unlock(&action_lock);
		splx(s);
		counter(c_action_thread_block++);
		thread_block((void (*)(void)) 0);
	}
}

/*
 * thread_stop_freeze
 *	Block the thread in the kernel and freeze the processor set.
 * return value:
 *	TRUE - the thread has blocked interruptibly, is stopped, and
 *		the processor set assignment is frozen
 *	FALSE - the thread is no longer in the processor set, so it
 *		isn't stopped, and the processor set assignment
 *		is released.
 */
int
thread_stop_freeze( thread_t thread, processor_set_t pset )
{
	thread_act_t	thr_act;
	spl_t	s;

	/*
	 * hold it, and wait for it to stop.
	 */
	thr_act = thread_lock_act(thread);
	thread_hold(thr_act);
	act_unlock_thread(thr_act);

	thread_stop(thread);

	s = splsched();
	wake_lock(thread);
        while( thread->state & (TH_RUN|TH_UNINT) ) {
                thread->wake_active = TRUE;
                assert_wait((event_t)&thread->wake_active, FALSE);
                wake_unlock(thread);
                splx(s);
                thread_block( (void (*)(void)) 0 );
                (void) splsched();
                wake_lock(thread);
        }

	/*
	 * Now, the thread has blocked uninterruptibly; freeze the 
	 * assignment and make sure it's still part of the processor set.
	 */
	wake_unlock(thread);
	thread_freeze(thread);
	thread_lock(thread);

	/*
	 * if the processor set has changed, release the freeze and
	 * then unstop it.
	 */
	if( thread->processor_set != pset ) {
		thread_unlock(thread);
		splx(s);
		thread_unfreeze(thread);
		thread_unstop(thread);
		return FALSE;
	}
	thread_unlock(thread);
	splx(s);
	return TRUE;
}

/*
 *	processor_doaction actually does the shutdown.  The trick here
 *	is to schedule ourselves onto a cpu and then save our
 *	context back into the runqs before taking out the cpu.
 */

void
processor_doaction(
	processor_t	processor)
{
	thread_t			this_thread;
	spl_t				s;
	register processor_set_t	pset;
#if	MACH_HOST
	register processor_set_t	new_pset;
	register thread_t		thread;
	register thread_t		prev_thread = THREAD_NULL;
	thread_act_t			thr_act;
	boolean_t			have_pset_ref = FALSE;
#endif	/* MACH_HOST */

	/*
	 *	Get onto the processor to shutdown
	 */
	this_thread = current_thread();
	thread_bind(this_thread, processor);
	thread_block((void (*)(void)) 0);

	pset = processor->processor_set;
#if	MACH_HOST
	/*
	 *	If this is the last processor in the processor_set,
	 *	stop all the threads first.
	 */
	pset_lock(pset);
	if (pset->processor_count == 1) {
		thread = (thread_t) queue_first(&pset->threads);
		prev_thread = THREAD_NULL;
		pset->ref_count++;
		have_pset_ref = TRUE;
		pset->empty = TRUE;

		/*
		 * loop through freezing the processor set assignment
		 * and reference counting the threads;
		 */
		while (!queue_end(&pset->threads, (queue_entry_t) thread)) {
		    thread_reference(thread);
		    pset_unlock(pset);

		    /*
		     * Freeze the thread on the processor set.
		     * If it's moved, just release the reference.
		     * Get the next thread in the processor set list
		     * from the last one which was frozen.
		     */
		    if( thread_stop_freeze(thread, pset) )
		        prev_thread = thread;
		    else
			thread_deallocate(thread);

		    pset_lock(pset);
		    if( prev_thread != THREAD_NULL ) 
		        thread = (thread_t)queue_next(&prev_thread->pset_threads);
		    else
			thread = (thread_t) queue_first(&pset->threads);
		}

		/*
		 * Remove the processor from the set so that when the threads
		 * are unstopped below the ones blocked in the kernel don't
		 * start running again.
		 */
		s = splsched();
		processor_lock(processor);
		pset_remove_processor(pset, processor);

		/*
		 * Prevent race with another processor being added to the set
		 * See code after Restart_pset:
		 *   while(new_pset->empty && new_pset->processor_count > 0)
		 *
		 * ... it tests for the condition where a new processor is
		 * added to the set while the last one is still being removed.
		 */
		pset->processor_count++;	/* block new processors being added */
		assert( pset->processor_count == 1 );

		/*
		 * Release the thread assignment locks, unstop the threads and
		 * release the thread references which were taken above.
		 */
		thread = (thread_t) queue_first(&pset->threads);
		while( !queue_empty( &pset->threads) && (thread != THREAD_NULL) ) {
		    prev_thread = thread;
		    if( queue_end(&pset->threads, (queue_entry_t) thread) )
			thread = THREAD_NULL;
		    else
		        thread = (thread_t) queue_next(&prev_thread->pset_threads);
		    pset_unlock(pset);
		    thread_unfreeze(prev_thread);
		    thread_unstop(prev_thread);
		    thread_deallocate(prev_thread);
		    pset_lock(pset);
		}
		/*
		 * allow a processor to be added to the empty pset
		 */
		pset->processor_count--;
	}
	else { 
		/* not last processor in set */
#endif	/* MACH_HOST */
		/*
		 * At this point, it is ok to rm the processor from the pset.
		 */
		s = splsched();
		processor_lock(processor);
		pset_remove_processor(pset, processor);
#if	MACH_HOST
	}
	pset_unlock(pset);

	/*
	 *	Copy the next pset pointer into a local variable and clear
	 *	it because we are taking over its reference.
	 */
	new_pset = processor->processor_set_next;
	processor->processor_set_next = PROCESSOR_SET_NULL;

	if (processor->state == PROCESSOR_ASSIGN) {

Restart_pset:
	    /*
	     *	Nasty problem: we want to lock the target pset, but
	     *	we have to enable interrupts to do that which requires
	     *  dropping the processor lock.  While the processor
	     *  is unlocked, it could be reassigned or shutdown.
	     */
	    processor_unlock(processor);
	    splx(s);

	    /*
	     *  Lock target pset and handle remove last / assign first race.
	     *	Only happens if there is more than one action thread.
	     */
	    pset_lock(new_pset);
	    while (new_pset->empty && new_pset->processor_count > 0) {
		pset_unlock(new_pset);
		while (*(volatile boolean_t *)&new_pset->empty &&
		       *(volatile int *)&new_pset->processor_count > 0)
			/* spin */;
		pset_lock(new_pset);
	    }

	    /*
	     *	Finally relock the processor and see if something changed.
	     *	The only possibilities are assignment to a different pset
	     *	and shutdown.
	     */
	    s = splsched();
	    processor_lock(processor);

	    if (processor->state == PROCESSOR_SHUTDOWN) {
		pset_unlock(new_pset);
		goto shutdown; /* will release pset reference */
	    }

	    if (processor->processor_set_next != PROCESSOR_SET_NULL) {
		/*
		 *	Processor was reassigned.  Drop the reference
		 *	we have on the wrong new_pset, and get the
		 *	right one.  Involves lots of lock juggling.
		 */
		processor_unlock(processor);
		splx(s);
		pset_unlock(new_pset);
		pset_deallocate(new_pset);
		s = splsched();
	        processor_lock(processor);
		new_pset = processor->processor_set_next;
		processor->processor_set_next = PROCESSOR_SET_NULL;
		goto Restart_pset;
	    }

	    /*
	     *	If the pset has been deactivated since the operation
	     *	was requested, redirect to the default pset.
	     */
	    if (!(new_pset->active)) {
		pset_unlock(new_pset);
		pset_deallocate(new_pset);
		new_pset = &default_pset;
		pset_lock(new_pset);
		new_pset->ref_count++;
	    }

	    /*
	     *	Do assignment, then wakeup anyone waiting for it.
	     *	Finally context switch to have it take effect.
	     */
	    pset_add_processor(new_pset, processor);
	    if (new_pset->empty) {
		/*
		 *	Set all the threads loose
		 */
		thread = (thread_t) queue_first(&new_pset->threads);
		while (!queue_end(&new_pset->threads,(queue_entry_t)thread)) {
		    thr_act = thread_lock_act(thread);
		    thread_release(thread->top_act);
		    act_unlock_thread(thr_act);
		    thread = (thread_t) queue_next(&thread->pset_threads);
		}
		new_pset->empty = FALSE;
	    }
	    processor->processor_set_next = PROCESSOR_SET_NULL;
	    processor->state = PROCESSOR_RUNNING;
	    thread_wakeup((event_t)processor);
	    processor_unlock(processor);
	    splx(s);
	    pset_unlock(new_pset);

	    /*
	     *	Clean up dangling references, and release our binding.
	     */
	    pset_deallocate(new_pset);
	    if (have_pset_ref)
		pset_deallocate(pset);
	    if (prev_thread != THREAD_NULL)
		thread_deallocate(prev_thread);
	    thread_bind(this_thread, PROCESSOR_NULL);

	    thread_block((void (*)(void)) 0);
	    return;
	}

shutdown:
#endif	/* MACH_HOST */
	
	/*
	 *	Do shutdown, make sure we live when processor dies.
	 */
	if (processor->state != PROCESSOR_SHUTDOWN) {
		printf("state: %d\n", processor->state);
	    	panic("action_thread -- bad processor state");
	}
	processor_unlock(processor);
	/*
	 *	Clean up dangling references, and release our binding.
	 */
#if	MACH_HOST
	if (new_pset != PROCESSOR_SET_NULL)
		pset_deallocate(new_pset);
	if (have_pset_ref)
		pset_deallocate(pset);
	if (prev_thread != THREAD_NULL)
		thread_deallocate(prev_thread);
#endif	/* MACH_HOST */

	thread_bind(this_thread, PROCESSOR_NULL);
	switch_to_shutdown_context(this_thread,
				   processor_doshutdown,
				   processor);
	splx(s);
}

/*
 *	Actually do the processor shutdown.  This is called at splsched,
 *	running on the processor's shutdown stack.
 */

void
processor_doshutdown(
	processor_t	processor)
{
	register int		cpu = processor->slot_num;

	timer_switch(&kernel_timer[cpu]);

	/*
	 *	Ok, now exit this cpu.
	 */
	PMAP_DEACTIVATE_KERNEL(cpu);
        cpu_data[cpu].active_thread = THREAD_NULL;
	active_kloaded[cpu] = THR_ACT_NULL;
	cpu_down(cpu);
	thread_wakeup((event_t)processor);
	halt_cpu();
	panic("zombie processor");

	/*
	 *	The action thread returns to life after the call to
	 *	switch_to_shutdown_context above, on some other cpu.
	 */

	/*NOTREACHED*/
}
#else	/* NCPUS > 1 */

kern_return_t
processor_assign(
	processor_t	processor,
	processor_set_t	new_pset,
	boolean_t	wait)
{
#ifdef	lint
	processor++; new_pset++; wait++;
#endif	/* lint */
	return(KERN_FAILURE);
}

#endif /* NCPUS > 1 */


kern_return_t
host_get_boot_info(
        host_t              priv_host,
        kernel_boot_info_t  boot_info)
{
	char *src = "";
	extern char * machine_boot_info(
				kernel_boot_info_t	boot_info,
				vm_size_t		buf_len);
#if	DIPC && (NORMA_SCSI || DIPC_XKERN)
	char *p;

#define NORMA_SCSI_BOOT_INFO \
	"ROOT_FS_NODE=0\nBOOT_NODE_LIST=0..7\nACCEPT_PROCS=1..7\nTIMESHARE_NODES=0..7\n"

#endif	/* DIPC && (NORMA_SCSI || DIPC_XKERN) */

	if (priv_host == HOST_NULL) {
		return KERN_INVALID_HOST;
	}

	/*
	 * Copy first operator string terminated by '\0' followed by
	 *	standardized strings generated from boot string.
	 */
	src = machine_boot_info(boot_info, KERNEL_BOOT_INFO_MAX);
	if (src != boot_info)
		(void) strncpy(boot_info, src, KERNEL_BOOT_INFO_MAX);

#if	DIPC && (NORMA_SCSI || DIPC_XKERN)
	/*
	 * Add NORMA boot string after a '\n' if there is enough space.
	 */
	p = boot_info;
	while (*p != '\0')
		p++;

	if (p - boot_info + strlen(NORMA_SCSI_BOOT_INFO) <
						KERNEL_BOOT_INFO_MAX - 2) {
		*p++ = '\n';
		*p = '\0';
		(void) strcpy(p, NORMA_SCSI_BOOT_INFO);
	}
#endif	/* DIPC && (NORMA_SCSI || DIPC_XKERN) */

	return KERN_SUCCESS;
}
