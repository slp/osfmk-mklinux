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
/* This file was formerly named kern/act.c; history is below:
 *
 * Revision 1.1.2.16  1994/02/26  17:38:36  bolinger
 * 	Add profiling initialization/finalization to activation
 * 	functions.
 * 	[1994/02/26  17:35:50  bolinger]
 *
 * Revision 1.1.2.15  1994/02/09  00:42:33  dwm
 * 	Put a variety of debugging code under MACH_ASSERT,
 * 	to enhance PROD performance a bit.
 * 	Promote switch_ktss() to act_machine_switch_pcb().
 * 	[1994/02/09  00:35:10  dwm]
 * 
 * Revision 1.1.2.14  1994/02/04  18:02:13  bolinger
 * 	Merge up to colo_shared.
 * 	[1994/02/04  17:58:20  bolinger]
 * 
 * 	Update AST context in act_attach() (as well as in act_detach());
 * 	check explicitly for null activation in user-callable interface
 * 	functions.  [dwm]
 * 	[1994/02/04  15:35:15  bolinger]
 * 
 * Revision 1.1.2.13  1994/02/04  03:46:08  condict
 * 	Return error in thread_terminate, if act is NULL.
 * 	[1994/02/04  03:41:40  condict]
 * 
 * 	Optimize act_attach/detach by removing act references.
 * 	Also, put if MACH_ASSERT around debugging code.
 * 	[1994/02/02  22:11:17  condict]
 * 
 * Revision 1.1.2.12  1994/02/02  18:58:52  bolinger
 * 	Add fix from Dave to check ast's in new current activation in
 * 	act_detach().  [dwm]
 * 	[1994/02/02  18:58:25  bolinger]
 * 
 * Revision 1.1.2.11  1994/01/31  16:30:07  bolinger
 * 	Make "pushing" and "popping" activations look more like
 * 	context switches (add pmap and kernel TSS manipulations).
 * 	[1994/01/31  16:29:47  bolinger]
 * 
 * Revision 1.1.2.10  1994/01/26  17:34:45  dwm
 * 	Change back to active_threads[] as root of current_*() access.
 * 	[1994/01/26  17:31:17  dwm]
 * 
 * Revision 1.1.2.9  1994/01/26  15:47:19  bolinger
 * 	Merge up to colo_shared.
 * 	[1994/01/25  22:57:40  bolinger]
 * 
 * 	Fix "kernel_loaded" handling for nested activations.  Update
 * 	kernel's cached idea of current kernel stack when necessary
 * 	in switching activations.
 * 	[1994/01/25  18:51:09  bolinger]
 * 
 * Revision 1.1.2.8  1994/01/25  18:38:01  dwm
 * 	move ast indicator into act from thread.
 * 	[1994/01/25  18:36:54  dwm]
 * 
 * Revision 1.1.2.7  1994/01/22  03:39:26  bolinger
 * 	Check for waiters (and awaken) when adding a thread to a
 * 	thread pool.
 * 	[1994/01/22  03:31:43  bolinger]
 * 
 * Revision 1.1.2.6  1994/01/21  23:45:35  dwm
 * 	Thread_pools are now embedded directly in ports/psets.
 * 	[1994/01/21  23:43:39  dwm]
 * 
 * Revision 1.1.2.5  1994/01/17  18:08:45  dwm
 * 	Tweak tracing code; nudge suspended act if it's terminated.
 * 	[1994/01/17  16:06:42  dwm]
 * 
 * Revision 1.1.2.4  1994/01/14  19:08:22  bolinger
 * 	In act_detach(), call thread_pool_wakeup() if anyone's waiting
 * 	for activations from the current pool.
 * 	[1994/01/14  19:07:57  bolinger]
 * 
 * Revision 1.1.2.2  1994/01/14  18:27:04  dwm
 * 	Coloc: clean up nudge() since we don't use kernel acts.
 * 	rearrange act_[gs]et_state to properly decide whether to
 * 	use blocking or non-blocking method, so (eg) signals work.
 * 	This need review if we start using kernel acts.
 * 	Comment out act_yank until it's both needed and finished.
 * 	[1994/01/14  17:59:20  dwm]
 * 
 * Revision 1.1.2.1  1994/01/12  17:53:07  dwm
 * 	Coloc: initial restructuring to follow Utah model.
 * 	removed MACH_IPC_COMPAT, added debug printf control, minor changes.
 * 	Some "in progress" code from Utah left in place for now.
 * 	[1994/01/12  17:15:16  dwm]
 */
/*
 * Copyright (c) 1993 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 *	Author:	Bryan Ford, University of Utah CSS
 *
 *	Thread_Activation management routines
 */

#include <cpus.h>
#include <task_swapper.h>
#include <mach/kern_return.h>
#include <mach/alert.h>
#include <kern/etap_macros.h>
#include <kern/mach_param.h>
#include <kern/zalloc.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/task.h>
#include <kern/task_swap.h>
#include <kern/thread_act.h>
#include <kern/thread_pool.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/exception.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/profile.h>
#include <kern/machine.h>
#include <kern/spl.h>
#include <kern/syscall_subr.h>
#include <kern/sync_lock.h>
#include <ipc/mach_msg.h>
#include <mach_prof.h>
#include <mach/rpc.h>

/*
 * Debugging printf control
 */
#if	MACH_ASSERT
unsigned int	watchacts =	  0 /* WA_ALL */
				    ;	/* Do-it-yourself & patchable */
#endif

/*
 * Track the number of times we need to swapin a thread to deallocate it.
 */
int act_free_swapin = 0;
extern boolean_t thread_swap_unwire_stack;

#if	THREAD_SWAP_UNWIRE_USER_STACK
extern boolean_t thread_swap_unwire_user_stack;
#else	/* THREAD_SWAP_UNWIRE_USER_STACK */
#define thread_swap_unwire_user_stack	FALSE
#endif	/* THREAD_SWAP_UNWIRE_USER_STACK */

/*
 * Forward declarations for functions local to this file.
 */
kern_return_t	act_abort( thread_act_t, int);
void		special_handler(ReturnHandler *, thread_act_t);
void		nudge(thread_act_t);
kern_return_t	act_set_state_locked(thread_act_t, int,
					thread_state_t,
					mach_msg_type_number_t);
kern_return_t	act_get_state_locked(thread_act_t, int,
					thread_state_t,
					mach_msg_type_number_t *);
void		act_set_apc(thread_act_t);
void		act_clr_apc(thread_act_t);
void		act_user_to_kernel(thread_act_t);
void		act_ulock_release_all(thread_act_t thr_act);

kern_return_t	terminate_empty_act(thread_act_t);
void		install_special_handler_locked(thread_act_t);

static zone_t	thr_act_zone;

/*
 * Thread interfaces accessed via a thread_activation:
 */

/*
 * Terminate a thread.  Called with nothing locked.
 * Returns same way.
 */
kern_return_t
thread_terminate(
	register thread_act_t	thr_act)
{
	thread_t	thread;
	task_t		task;
	struct ipc_port	*iplock;
	kern_return_t	ret = KERN_INVALID_ARGUMENT;
#if	NCPUS > 1
	boolean_t	held;
#endif	/* NCPUS > 1 */

	if (thr_act == THR_ACT_NULL)
		return(ret);

#if	THREAD_SWAPPER
	thread_swap_disable(thr_act);
#endif	/* THREAD_SWAPPER */

	thread = act_lock_thread(thr_act);

#if	NCPUS > 1
	/* 
	 * Make sure this thread enters the kernel
	 */
	if (thread != THREAD_NULL && thread != current_thread()) {
		thread_hold(thr_act);
		act_unlock_thread(thr_act);
		if (!thread_stop_wait(thread)) {
			ret = KERN_ABORTED;
			(void)act_lock_thread(thr_act);
			thread_release(thr_act);
			act_unlock_thread(thr_act);
			return (ret);
		}
		held = TRUE;
		(void)act_lock_thread(thr_act);
	} else {
		held = FALSE;
	}
#endif	/* NCPUS > 1 */

	/*
	 * Lock task containing target.
	 * Use mutex_try to respect lock ordering.
	 */

	task = thr_act->task;
	if (!mutex_try(&task->lock)) {
		act_unlock_thread(thr_act);
		task_lock(task);
		thread = act_lock_thread(thr_act);
	}
		

	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		task_unlock(task);
		ret = KERN_TERMINATED;
		goto out;
	}

	/*
	 *	Break IPC control over the thread.
	 */
	ipc_thr_act_disable_act_locked(thr_act);

	/*
	 * Check for terminating an empty thread_act -- detach it from
	 * its task, then deallocate its remaining refs here.
	 */
	if (thr_act->thread == THREAD_NULL) {
		iplock = thr_act->pool_port;	/* remember for unlock call */
		ret = terminate_empty_act( thr_act );

		/*
		 * Release locks individually. (`act_unlock_thread()'
		 * will not work since `pool_port' field has changed.)
		 */
		ip_unlock(iplock);
		act_unlock(thr_act);
	}
	else {
		assert(thr_act->active);
		act_disable_task_locked(thr_act);
		ret = act_abort(thr_act,FALSE);
		act_unlock_thread(thr_act);
	}

	task_unlock(task);
out:
#if	NCPUS > 1
	if (held) {
		thread_unstop(thread);
		(void)act_lock_thread(thr_act);
		thread_release(thr_act);
		act_unlock_thread(thr_act);
	}
#endif	/* NCPUS > 1 */
	return(ret);
}

/*
 * Called with empty thread activation and its pool port locked.
 * Activation is currently "active."
 * Task containing activation is also locked.
 * Returns the same way.
 */
kern_return_t
terminate_empty_act( thread_act_t thr_act ) {
	/*
	 * remove the activation from the pool port
	 * (this prevents it from being used)
	 */
	if (thr_act->pool_port) {
		act_locked_act_set_thread_pool(thr_act, IP_NULL);
	}

	/*
	 * disassociate the thread from the task
	 */
	assert(thr_act->active);
	act_disable_task_locked( thr_act );

	return KERN_SUCCESS;
}

/*
 *	thread_hold:
 *
 *	Suspend execution of the specified thread.
 *	This is a recursive-style suspension of the thread, a count of
 *	suspends is maintained.
 *
 *	Called with thr_act locked "appropriately" for synchrony with
 *	RPC (see act_lock_thread()).  Returns same way.
 */
void
thread_hold(
	register thread_act_t	thr_act)
{
	if (thr_act->suspend_count++ == 0) {
		install_special_handler(thr_act);
		nudge(thr_act);
	}
}

/*
 * Decrement internal suspension count for thr_act, setting thread
 * runnable when count falls to zero.
 *
 * Called with thr_act locked "appropriately" for synchrony
 * with RPC (see act_lock_thread()).
 */
void
thread_release(
	register thread_act_t	thr_act)
{
	if( thr_act->suspend_count &&
		(--thr_act->suspend_count == 0) )
		nudge( thr_act );
}

kern_return_t
thread_suspend(
	register thread_act_t	thr_act)
{
	thread_t		thread;

	if (thr_act == THR_ACT_NULL) {
		return(KERN_INVALID_ARGUMENT);
	}
	thread = act_lock_thread(thr_act);
	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return(KERN_TERMINATED);
	}
	if (thr_act->user_stop_count++ == 0 &&
		thr_act->suspend_count++ == 0 ) {
		install_special_handler(thr_act);
		if (thread &&
			thr_act == thread->top_act && thread != current_thread()) {
			nudge(thr_act);
			act_unlock_thread(thr_act);
			(void)thread_wait(thread);
		}
		else {
			/*
			 * No need to wait for target thread
			 */
			act_unlock_thread(thr_act);
		}
	}
	else {
		/*
		 * Thread is already suspended
		 */
		act_unlock_thread(thr_act);
	}
	return(KERN_SUCCESS);
}

kern_return_t
thread_resume(
	register thread_act_t	thr_act)
{
	register kern_return_t	ret;
	spl_t			s;
	thread_t		thread;

	if (thr_act == THR_ACT_NULL)
		return(KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	ret = KERN_SUCCESS;

	if (thr_act->active) {
		if (thr_act->user_stop_count > 0) {
		    	if( --thr_act->user_stop_count == 0 ) {
				--thr_act->suspend_count;
				nudge( thr_act );
			}
		}
		else
			ret = KERN_FAILURE;
	}
	else
		ret = KERN_TERMINATED;
	act_unlock_thread( thr_act );
	return ret;
}

/*
 * Already locked: all RPC-related locks for thr_act (see
 * act_lock_thread()).
 */
kern_return_t
act_abort( thread_act_t thr_act, int chain_break )
{
	spl_t			spl;
	thread_t		thread;
	struct ipc_port		*iplock = thr_act->pool_port;
	thread_act_t		orphan;
	kern_return_t		kr;
	etap_data_t		probe_data;

	/*
	 * mark the activation for abort,
	 * update the suspend count,
	 * always install the special handler
	 */
	install_special_handler(thr_act);

	ETAP_DATA_LOAD(probe_data[0], thr_act);
	ETAP_DATA_LOAD(probe_data[1], thr_act->thread);
	ETAP_PROBE_DATA(ETAP_P_ACT_ABORT,
			0,
			current_thread(),
			&probe_data,
			ETAP_DATA_ENTRY*2);

	spl = splsched();
	thread_lock( thr_act->thread );

	/*
	 *  If the target thread activation is not the head... 
	 */
	if ( thr_act->thread->top_act != thr_act ) {
#ifdef AGRESSIVE_ABORT
		/* release state buffer for target's outstanding invocation */
		if (unwind_invoke_state(thr_act) != KERN_SUCCESS) {
			panic("unwind_invoke_state failure");
		}

		/* release state buffer for target's incoming invocation */
		if (thr_act->lower != THR_ACT_NULL) {
			if (unwind_invoke_state(thr_act->lower)
			    != KERN_SUCCESS) {
				panic("unwind_invoke_state failure");
			}
		}

		/* unlink target thread activation from shuttle chain */
		if ( thr_act->lower == THR_ACT_NULL ) {
			/*
			 * This is the root thread activation of the chain.
			 * Unlink the root thread act from the bottom of
			 * the chain.
			 */
			thr_act->higher->lower = THR_ACT_NULL;
		} else {
			/*
			 * This thread act is in the middle of the chain.
			 * Unlink the thread act from the middle of the chain.
			 */
			thr_act->higher->lower = thr_act->lower;
			thr_act->lower->higher = thr_act->higher;

			/* set the terminated bit for RPC return processing */
			thr_act->lower->alerts |= SERVER_TERMINATED;
		}

		orphan = thr_act->higher;

		/* remove the activation from its thread pool */
		/* (note: this is okay for "rooted threads," too) */
		act_locked_act_set_thread_pool(thr_act, IP_NULL);

		/* (just to be thorough) release the IP lock */
		if (iplock != IP_NULL) ip_unlock(iplock);

		/* release one more reference for a rooted thread */
		if (iplock == IP_NULL) act_locked_act_deallocate(thr_act);

		/* Presumably, the only reference to this activation is
		 * now held by the caller of this routine. */
		assert(thr_act->ref_count == 1);
#else /*AGRESSIVE_ABORT*/
		/* If there is a lower activation in the RPC chain... */
		if (thr_act->lower != THR_ACT_NULL) {
			/* ...indicate the server activation was terminated */
			thr_act->lower->alerts |= SERVER_TERMINATED;
		}
		/* Mark (and process) any orphaned activations */
		orphan = thr_act->higher;
#endif /*AGRESSIVE_ABORT*/

		/* indicate client of orphaned chain has been terminated */
		orphan->alerts |= CLIENT_TERMINATED;

		/* 
		 * Chase the chain, setting alert bits and installing
		 * special handlers for each thread act.
		 */
		do {
			orphan->alerts |= ORPHANED;
			install_special_handler_locked(orphan);
			orphan = orphan->higher;
		} while (orphan != THR_ACT_NULL);

		/*
		 * Get attention of head of RPC chain.
		 */
		thread_unlock(thr_act->thread);
		nudge(thr_act->thread->top_act);
		splx(spl);
		return (KERN_SUCCESS);
	}

	/*
	 * If the target thread is the end of the chain, the thread
	 * has to be marked for abort and rip it out of any wait.
	 */
	if (thr_act->thread->top_act == thr_act) {
	    thr_act->thread->state |= TH_ABORT;
	    thread_unlock(thr_act->thread);
	    install_special_handler(thr_act);
	    nudge( thr_act );

	    /*
	     * if it's waiting, clear it.
	     */
	    if( thr_act->thread->state & TH_WAIT ){
		clear_wait(thr_act->thread, THREAD_INTERRUPTED, TRUE);
		splx(spl);
		return KERN_SUCCESS;
	    }
	}
	else {
		thread_unlock(thr_act->thread);
	}
	splx(spl);
	return KERN_SUCCESS;
}
	
kern_return_t
thread_abort(
	register thread_act_t	thr_act)
{
	int		ret;
	thread_t		thread;

	if (thr_act == THR_ACT_NULL || thr_act == current_act())
		return (KERN_INVALID_ARGUMENT);
	/*
	 *	Lock the target thread and the current thread now,
	 *	in case thread_halt() ends up being called below.
	 */
	thread = act_lock_thread(thr_act);
	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return(KERN_TERMINATED);
	}

	if( thread == THREAD_NULL ) {
		act_unlock_thread(thr_act);
		return KERN_NO_THREAD;
	}

	ret = act_abort( thr_act, FALSE );
	act_unlock_thread( thr_act );
	return ret;
}

kern_return_t
thread_abort_safely(
	register thread_act_t	thr_act)
{
	boolean_t		r;
	thread_t		cur_thr = current_thread();
	thread_t		thread, othread;
	thread_act_t		cur_thr_act = current_act();
	spl_t			spl;

	if (thr_act == THR_ACT_NULL || thr_act == current_act())
		return(KERN_INVALID_ARGUMENT);

	othread = act_lock_thread(thr_act);
	if (othread == THREAD_NULL) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_ARGUMENT);
	}
	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return(KERN_TERMINATED);
	}
	if (othread->top_act != thr_act) {
		act_unlock_thread(thr_act);
		return(KERN_FAILURE);
	}

	/*
	 * This is the head of the chain.  Stop it and check to see if
	 * it's in a condition which can be interrupted and restarted and,
	 * if so, clear the wait.
	 */
	thread_hold(thr_act);
	act_unlock_thread(thr_act);
	r = thread_stop_wait(othread);
	if ((thread = act_lock_thread(thr_act)) != othread || !r) {
		thread_release(thr_act);
		act_unlock_thread(thr_act);
		if (r) {
			thread_unstop(othread);
			return (KERN_FAILURE);
		}
		else
		    return(KERN_ABORTED);
	}

	assert(thread == thr_act->thread);

	spl = splsched();
	thread_lock(thread);
	
	if( (thread->state&(TH_RUN|TH_UNINT|TH_WAIT)) == TH_WAIT ){
	    switch( thread->at_safe_point ) {
		    case SAFE_INTERNAL_RECEIVE:
			/* never abortable */
			break;
		    case SAFE_EXTERNAL_RECEIVE:
			if (!mach_msg_interruptible(thread))
			    break;
			/* fall through if interruptible */
		    case SAFE_EXCEPTION_RETURN:
		    case SAFE_BOOTSTRAP_RETURN:
		    case SAFE_SUSPENDED:
		    case SAFE_THR_DEPRESS:
		    case SAFE_MISCELLANEOUS:
			/*
			 * It's an interruptible wait, clear it, then
			 * let the thread go and return successfully.
			 */
			thread_unlock(thread);
			splx(spl);
			clear_wait(thread, THREAD_INTERRUPTED, TRUE);
			thread_unstop(thread);
			thread_release(thr_act);
			act_unlock_thread(thr_act);
			return KERN_SUCCESS;
		    default:
			    /* break; */ ;
	    }
	}
	/*
	 * if not stopped at a safepoint, just let it go and return failure.
	 */
	thread_unlock(thread);
	splx(spl);
	thread_unstop(thread);
	thread_release(thr_act);
	act_unlock_thread(thr_act);
	return KERN_FAILURE;
}

/*** backward compatibility hacks ***/
#include <mach/thread_info.h>
#include <mach/thread_special_ports.h>
#include <ipc/ipc_port.h>
#include <mach/mach_server.h>

kern_return_t
thread_info(
	thread_act_t		thr_act,
	thread_flavor_t		flavor,
	thread_info_t		thread_info_out,
	mach_msg_type_number_t	*thread_info_count)
{
	thread_t		thread;
	kern_return_t	kr;

	if (thr_act == THR_ACT_NULL)
		return (KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return(KERN_TERMINATED);
	}
	if (thread == THREAD_NULL) {
		act_unlock_thread(thr_act);
		return (KERN_NO_THREAD);
	}
	kr = Thread_info(thread, thr_act,
		flavor, thread_info_out, thread_info_count);
	act_unlock_thread(thr_act);
	return (kr);
}

/*
 *	Routine:	thread_get_special_port [kernel call]
 *	Purpose:
 *		Clones a send right for one of the thread's
 *		special ports.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		KERN_SUCCESS		Extracted a send right.
 *		KERN_INVALID_ARGUMENT	The thread is null.
 *		KERN_FAILURE		The thread is dead.
 *		KERN_INVALID_ARGUMENT	Invalid special port.
 */

kern_return_t
thread_get_special_port(
	thread_act_t	thr_act,
	int		which,
	ipc_port_t	*portp)
{
	ipc_port_t	*whichp;
	ipc_port_t	port;
	thread_t	thread;

#if	MACH_ASSERT
	if (watchacts & WA_PORT)
	    printf("thread_get_special_port(thr_act=%x, which=%x port@%x=%x\n",
		thr_act, which, portp, (portp ? *portp : 0));
#endif	/* MACH_ASSERT */

	if (!thr_act)
		return KERN_INVALID_ARGUMENT;
 	thread = act_lock_thread(thr_act);
	switch (which) {
		case THREAD_KERNEL_PORT:
			whichp = &thr_act->ith_sself;
			break;

		default:
			act_unlock_thread(thr_act);
			return KERN_INVALID_ARGUMENT;
	}

	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return KERN_FAILURE;
	}

	port = ipc_port_copy_send(*whichp);
	act_unlock_thread(thr_act);

	*portp = port;
	return KERN_SUCCESS;
}

/*
 *	Routine:	thread_set_special_port [kernel call]
 *	Purpose:
 *		Changes one of the thread's special ports,
 *		setting it to the supplied send right.
 *	Conditions:
 *		Nothing locked.  If successful, consumes
 *		the supplied send right.
 *	Returns:
 *		KERN_SUCCESS		Changed the special port.
 *		KERN_INVALID_ARGUMENT	The thread is null.
 *		KERN_FAILURE		The thread is dead.
 *		KERN_INVALID_ARGUMENT	Invalid special port.
 */

kern_return_t
thread_set_special_port(
	thread_act_t	thr_act,
	int		which,
	ipc_port_t	port)
{
	ipc_port_t	*whichp;
	ipc_port_t	old;
	thread_t	thread;

#if	MACH_ASSERT
	if (watchacts & WA_PORT)
		printf("thread_set_special_port(thr_act=%x,which=%x,port=%x\n",
			thr_act, which, port);
#endif	/* MACH_ASSERT */

	if (thr_act == 0)
		return KERN_INVALID_ARGUMENT;

	thread = act_lock_thread(thr_act);
	switch (which) {
		case THREAD_KERNEL_PORT:
			whichp = &thr_act->ith_self;
			break;

		default:
			act_unlock_thread(thr_act);
			return KERN_INVALID_ARGUMENT;
	}

	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return KERN_FAILURE;
	}

	old = *whichp;
	*whichp = port;
	act_unlock_thread(thr_act);

	if (IP_VALID(old))
		ipc_port_release_send(old);
	return KERN_SUCCESS;
}

/*
 *  thread state should always be accessible by locking the thread
 *  and copying it.  The activation messes things up so for right
 *  now if it's not the top of the chain, use a special handler to
 *  get the information when the shuttle returns to the activation.
 */
kern_return_t
thread_get_state(
	register thread_act_t	thr_act,
	int			flavor,
	thread_state_t		state,	/* pointer to OUT array */
	mach_msg_type_number_t	*state_count)	/*IN/OUT*/
{
	kern_return_t		ret;
	thread_t		thread, nthread;

	if (thr_act == THR_ACT_NULL || thr_act == current_act())
		return (KERN_INVALID_ARGUMENT);

	thread = act_lock_thread(thr_act);
	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return(KERN_TERMINATED);
	}

	thread_hold(thr_act);
	while (1) {
	    	if (!thread || thr_act != thread->top_act)
			break;
		act_unlock_thread(thr_act);
		(void)thread_stop_wait(thread);
		nthread = act_lock_thread(thr_act);
		if (nthread == thread)
			break;
		thread_unstop(thread);
		thread = nthread;
	}
	ret = act_machine_get_state(thr_act, flavor,
					state, state_count);
	if (thread && thr_act == thread->top_act)
	    thread_unstop(thread);
	thread_release(thr_act);
	act_unlock_thread(thr_act);

	return(ret);
}

/*
 *	Change thread's machine-dependent state.  Called with nothing
 *	locked.  Returns same way.
 */
kern_return_t
thread_set_state(
	register thread_act_t	thr_act,
	int			flavor,
	thread_state_t		state,
	mach_msg_type_number_t	state_count)
{
	kern_return_t		ret;
	thread_t		thread, nthread;

	if (thr_act == THR_ACT_NULL || thr_act == current_act())
		return (KERN_INVALID_ARGUMENT);
	/*
	 * We have no kernel activations, so Utah's MO fails for signals etc.
	 *
	 * If we're blocked in the kernel, use non-blocking method, else
	 * pass locked thr_act+thread in to "normal" act_[gs]et_state().
	 */

	thread = act_lock_thread(thr_act);
	if (!thr_act->active) {
		act_unlock_thread(thr_act);
		return(KERN_TERMINATED);
	}

	thread_hold(thr_act);
	while (1) {
	    	if (!thread || thr_act != thread->top_act)
			break;
		act_unlock_thread(thr_act);
		(void)thread_stop_wait(thread);
		nthread = act_lock_thread(thr_act);
		if (nthread == thread)
			break;
		thread_unstop(thread);
		thread = nthread;
	}
	ret = act_machine_set_state(thr_act, flavor,
					state, state_count);
	if (thread && thr_act == thread->top_act)
	    thread_unstop(thread);
	thread_release(thr_act);
	act_unlock_thread(thr_act);

	return(ret);
}

/*
 * Kernel-internal "thread" interfaces used outside this file:
 */


/*
 *	thread_setstatus:
 *
 *	Set the status of the specified thread.
 *	Called with (and returns with) no locks held.
 */
kern_return_t
thread_setstatus(
	thread_act_t		thr_act,
	int			flavor,
	thread_state_t		tstate,
	mach_msg_type_number_t	count)
{
	kern_return_t		kr;
	thread_t		thread;

	thread = act_lock_thread(thr_act);
	assert(thread);
	assert(thread->top_act == thr_act);
	kr = act_machine_set_state(thr_act, flavor, tstate, count);
	act_unlock_thread(thr_act);
	return(kr);
}

/*
 *	thread_getstatus:
 *
 *	Get the status of the specified thread.
 */
kern_return_t
thread_getstatus(
	thread_act_t		thr_act,
	int			flavor,
	thread_state_t		tstate,
	mach_msg_type_number_t	*count)
{
	kern_return_t		kr;
	thread_t		thread;

	thread = act_lock_thread(thr_act);
	assert(thread);
	assert(thread->top_act == thr_act);
	kr = act_machine_get_state(thr_act, flavor, tstate, count);
	act_unlock_thread(thr_act);
	return(kr);
}

/*
 * Kernel-internal thread_activation interfaces used outside this file:
 */

/*
 * act_init()	- Initialize activation handling code
 */
void
act_init()
{
	thr_act_zone = zinit(
			sizeof(struct thread_activation),
			ACT_MAX * sizeof(struct thread_activation), /* XXX */
			ACT_CHUNK * sizeof(struct thread_activation),
			"activations");
	act_machine_init();
}


/*
 * act_create	- Create a new activation in a specific task.
 */
kern_return_t
act_create(task_t task, vm_offset_t user_stack, vm_size_t stack_size,
	   thread_act_t *new_act)
{
	thread_act_t thr_act;
	int rc;
	vm_map_t map;

	thr_act = (thread_act_t)zalloc(thr_act_zone);
	if (thr_act == 0)
		return(KERN_RESOURCE_SHORTAGE);

#if	MACH_ASSERT
	if (watchacts & WA_ACT_LNK)
		printf("act_create(task=%x,stk=%x,thr_act@%x=%x)\n",
			task, user_stack, new_act, thr_act);
#endif	/* MACH_ASSERT */

	/* Start by zeroing everything; then init non-zero items only */
	bzero((char *)thr_act, sizeof(*thr_act));

	/* Start with one reference for being active, another for the caller */
	act_lock_init(thr_act);
	thr_act->ref_count = 2;

	/* Latch onto the task.  */
	thr_act->task = task;
	task_reference(task);
	thr_act->active = 1;
	thr_act->user_stack = user_stack;
	thr_act->user_stack_size = stack_size;
#if	THREAD_SWAPPER
	thr_act->swap_state = TH_SW_IN;
#if	MACH_ASSERT
	thr_act->kernel_stack_swapped_in = TRUE;
#if	THREAD_SWAP_UNWIRE_USER_STACK
	thr_act->user_stack_swapped_in = TRUE;
#endif	/* THREAD_SWAP_UNWIRE_USER_STACK */
#endif	/* MACH_ASSERT */
#endif	/* THREAD_SWAPPER */

	/* special_handler will always be last on the returnhandlers list.  */
	thr_act->special_handler.next = 0;
	thr_act->special_handler.handler = special_handler;

#if	MACH_PROF
	thr_act->act_profiled = FALSE;
	thr_act->act_profiled_own = FALSE;
	thr_act->profil_buffer = NULLPROFDATA;
#endif

	/* Initialize the held_ulocks queue as empty */
	queue_init(&thr_act->held_ulocks);

	/* Inherit the profiling status of the parent task */
	act_prof_init(thr_act, task);

	ipc_thr_act_init(task, thr_act);
	act_machine_create(task, thr_act);

	/*
	 * If thr_act created in kernel-loaded task, alter its saved
	 * state to so indicate
	 */
	if (task->kernel_loaded) {
		act_user_to_kernel(thr_act);
		/*
		 * If a user_stack was given, assume this is not a "base"
		 * activation -- assert that it's fully kernel-loaded
		 * already.
		 */
		if (user_stack) {
			thr_act->kernel_loading = FALSE;
			thr_act->kernel_loaded = TRUE;
		}
	}

	task_lock(task);

	/* Chain the thr_act onto the task's list */
	mutex_lock(&task->act_list_lock);
	thr_act->thr_acts.next = task->thr_acts.next;
	thr_act->thr_acts.prev = &task->thr_acts;
	task->thr_acts.next->prev = &thr_act->thr_acts;
	task->thr_acts.next = &thr_act->thr_acts;
	task->thr_act_count++;
	/* no need to check for transition from 0 -> 1 here */
	task->res_act_count++;
	mutex_unlock(&task->act_list_lock);

	task_unlock(task);

	/* Cache the task's map and take a reference to it */
	thr_act->map = task->map;

	/* Inline vm_map_reference cause we don't want to increment res_count */
	map = task->map;
	mutex_lock(&map->s_lock);
#if	TASK_SWAPPER
	assert(map->res_count > 0);
	assert(map->ref_count >= map->res_count);
#endif	/* TASK_SWAPPER */
	map->ref_count++;
	mutex_unlock(&map->s_lock);

	*new_act = thr_act;
	return KERN_SUCCESS;
}

/*
 * act_free	- called when an thr_act's ref_count drops to zero.
 *
 * This can only happen when thread is zero (not in use),
 * thread_pool is zero (not attached to any thread_pool),
 * and active is false (terminated).  Called with act locked,
 * so that its shuttle pointer can be zeroed atomically.
 */
#if	MACH_ASSERT
int	dangerous_bzero = 1;	/* paranoia & safety */
#endif

void
act_free(thread_act_t thr_act)
{
	thread_t	thr;
	vm_map_t	map;

#if	MACH_ASSERT
	if (watchacts & WA_EXIT)
		printf("act_free(%x(%d)) thr=%x tsk=%x(%d) pport=%x%sactive\n",
			thr_act, thr_act->ref_count, thr_act->thread,
			thr_act->task,
			thr_act->task ? thr_act->task->ref_count : 0,
			thr_act->pool_port,
			thr_act->active ? " " : " !");
#endif	/* MACH_ASSERT */

	/* Drop final ref to shuttle (not really ours, but we're
	 * allowed to handle it). */
	if (thr = thr_act->thread) {
		thr_act->thread = 0;
		act_unlock(thr_act);
		thread_deallocate(thr);
	}
	else
		act_unlock(thr_act);

#if	THREAD_SWAPPER
	thread_swap_disable(thr_act);
	assert(thr_act->kernel_stack_swapped_in);
#endif	/* THREAD_SWAPPER */

	assert(!thr_act->active);
	assert(!thr_act->pool_port);

	act_machine_destroy(thr_act);
	ipc_thr_act_disable(thr_act);
	ipc_thr_act_terminate(thr_act);

	act_prof_deallocate(thr_act);

	/*
	 * Drop the cached map reference.
	 * Inline version of vm_map_deallocate() because we
	 * don't want to decrement the map's residence count here.
	 */
	map = thr_act->map;
	mutex_lock(&map->s_lock);
#if	TASK_SWAPPER
	assert(map->res_count >= 0);
	assert(map->ref_count > map->res_count);
#endif	/* TASK_SWAPPER */
	/* the task must still have a reference on the map */
	if (--map->ref_count == 0)
		panic("thread_deallocate: released last reference on map");
	mutex_unlock(&map->s_lock);

	/* Drop the task reference.  */
	task_deallocate(thr_act->task);

#if	MACH_ASSERT
	if (dangerous_bzero)	/* dangerous if we're still using it! */
		bzero((char *)thr_act, sizeof(*thr_act));
#endif	/* MACH_ASSERT */
	/* Put the thr_act back on the thr_act zone */
	zfree(thr_act_zone, (vm_offset_t)thr_act);
}

/*
 * switch_act
 *
 * If a new activation is given, switch to it. If not,
 * switch to the lower activation (pop). Returns the old
 * activation. This is for RPC support.
 */
thread_act_t
switch_act( 
	thread_act_t act)
{
	thread_t	thread;
	thread_act_t	old, new;
	unsigned	cpu;
	processor_t	myprocessor;
	
	spl_t           s;

	disable_preemption();

	cpu = cpu_number();
	thread  = current_thread();

	/*
	 *	Find the old and new activation for switch.
	 */
	old = thread->top_act;

	if (act) {
		task_t		new_task;

		new = act;
                new->thread = thread;
		/* If we're switching into a task that has a higher priority
		   or a more real-time scheduling policy than us, temporarily
		   assume its priority and policy.  Save the old values in
		   the old activation, with a stamp so we don't attempt to
		   restore them if the thread's priority has been explicitly
		   altered in the meantime.  */
		new_task = new->task;
		if (new_task->policy > thread->policy
		    || new_task->priority < thread->priority) {
		    if (!mutex_try(&new_task->lock)) {
			act_unlock_thread(new);
        		enable_preemption();
			task_lock(new_task);
			act_lock_thread(new);
			disable_preemption();
			cpu = cpu_number();
		    }
		    if (new_task->policy > thread->policy
			|| new_task->priority < thread->priority) {
			old->saved_policy = thread->policy;
			old->saved_base_priority = thread->priority;
			old->saved_sched_change_stamp =
			    thread->sched_change_stamp;
			thread->policy = new_task->policy;
			thread->priority = new_task->priority;
			compute_priority(thread, FALSE);

			/* Do we still deserve the CPU ? */
			myprocessor = current_processor();
			if (csw_needed(thread, myprocessor)) {
				ast_on(cpu_number(),
				       (myprocessor->first_quantum ?
					AST_BLOCK :
					AST_QUANTUM));
			}
		    }
		    task_unlock(new_task);
		}
	}
	else {
		new = old->lower;
		/* If we assumed the priority of the task we had switched
		   into, put back the previous values.  */
		if (new->saved_sched_change_stamp ==
			thread->sched_change_stamp) {
		    int priority;

		    new->saved_sched_change_stamp = 0;
		    thread->policy = new->saved_policy;
		    priority = new->saved_base_priority;
		    if (priority < thread->max_priority)
			priority = thread->max_priority;
		    if (thread->depress_priority < 0)
			thread->priority = priority;
		    else
			thread->depress_priority = priority;
		    compute_priority(thread, TRUE);
		    /* Note that we deliberately do not reschedule here,
		       even though the loss of priority may mean that we
		       are no longer the (or a) highest-priority thread.
		       Tests showed that we incurred a significant penalty
		       under heavy load by rescheduling.  */

		    /* Do we still deserve the CPU ? */
		    myprocessor = current_processor();
		    if (csw_needed(thread, myprocessor)) {
			    ast_on(cpu_number(),
				   (myprocessor->first_quantum ?
				    AST_BLOCK :
				    AST_QUANTUM));
		    }
		}
	}

	assert(new != THR_ACT_NULL);
#if     THREAD_SWAPPER
	assert(new->swap_state != TH_SW_OUT &&
	       new->swap_state != TH_SW_COMING_IN);
#endif	/* THREAD_SWAPPER */

	/*
	 *	Switch the vm, ast and pcb context. 
	 */
        assert(cpu_data[cpu].active_thread == thread);
	active_kloaded[cpu] = (new->kernel_loaded) ? new : 0;

	machine_switch_act(thread, old, new, cpu);

	/*
	 *	Push or pop an activation on the chain.
	 */
	if (act) 
		act_attach(new, thread, 0);
	else 
		act_detach(old);

        enable_preemption();

	return(old);
}

/*
 * act_attach	- Attach an thr_act to the top of a thread ("push the stack").
 *
 * The thread_shuttle must be either the current one or a brand-new one.
 * Assumes the thr_act is active but not in use, also, that if it is
 * attached to an thread_pool (i.e. the thread_pool pointer is nonzero),
 * the thr_act has already been taken off the thread_pool's list.
 *
 * Already locked: thr_act plus "appropriate" thread-related locks
 * (see act_lock_thread()).
 */
void 
act_attach(
	thread_act_t	thr_act,
	thread_t	thread,
	unsigned	init_alert_mask)
{
        thread_act_t    lower;

#if	MACH_ASSERT
	assert(thread == current_thread() || thread->top_act == THR_ACT_NULL);
	if (watchacts & WA_ACT_LNK)
		printf("act_attach(thr_act %x(%d) thread %x(%d) mask %d)\n",
		       thr_act, thr_act->ref_count, thread, thread->ref_count,
		       init_alert_mask);
#endif	/* MACH_ASSERT */

	/* 
	 *	Chain the thr_act onto the thread's thr_act stack.  
	 *	Set mask and auto-propagate alerts from below.
	 */
	thr_act->thread = thread;
	thr_act->higher = THR_ACT_NULL;  /*safety*/
	thr_act->alerts = 0;
	thr_act->alert_mask = init_alert_mask;
	lower = thr_act->lower = thread->top_act;

        if (lower != THR_ACT_NULL) {
                lower->higher = thr_act;
                thr_act->alerts = (lower->alerts & init_alert_mask);
        }

	thread->top_act = thr_act;
}

/*
 * 	act_detach	
 *
 *	Remove the current thr_act from the top of the current thread, i.e.
 *	"pop the stack". Assumes already locked: thr_act plus "appropriate"
 * 	thread-related locks (see act_lock_thread).
 */
void 
act_detach(
	thread_act_t	cur_act)
{
	thread_t	cur_thread = cur_act->thread;

#if	MACH_ASSERT
	if (watchacts & (WA_EXIT|WA_ACT_LNK))
		printf("act_detach: thr_act %x(%d), thrd %x(%d) task=%x(%d)\n",
		       cur_act, cur_act->ref_count,
		       cur_thread, cur_thread->ref_count,
		       cur_act->task,
		       cur_act->task ? cur_act->task->ref_count : 0);
#endif	/* MACH_ASSERT */

	/* Unlink the thr_act from the thread's thr_act stack */
	cur_thread->top_act = cur_act->lower;
	cur_act->thread = 0;

	thread_pool_put_act(cur_act);

#if	MACH_ASSERT
	cur_act->lower = cur_act->higher = THR_ACT_NULL; 
	if (cur_thread->top_act)
		cur_thread->top_act->higher = THR_ACT_NULL;
#endif	/* MACH_ASSERT */

	return;
}


/*
 * Synchronize a thread operation with RPC.  Called with nothing
 * locked.   Returns with thr_act locked, plus one of four
 * combinations of other locks held:
 *	none - for new activation not yet associated with thread_pool
 *		or shuttle
 *	rpc_lock(thr_act->thread) only - for base activation (one
 *		without pool_port)
 *	ip_lock(thr_act->pool_port) only - for empty activation (one
 *		with no associated shuttle)
 *	both locks - for "active" activation (has shuttle, lives
 *		on thread_pool)
 * If thr_act has an associated shuttle, this function returns
 * its address.  Otherwise it returns zero.
 */
thread_t
act_lock_thread(
	thread_act_t thr_act)
{
	ipc_port_t pport;

	/*
	 * Allow the shuttle cloning code (q.v., when it
	 * exists :-}) to obtain ip_lock()'s while holding
	 * an rpc_lock().
	 */
	while (1) {
		act_lock(thr_act);
		pport = thr_act->pool_port;
		if (!pport || ip_lock_try(pport)) {
			if (!thr_act->thread)
				break;
			if (rpc_lock_try(thr_act->thread))
				break;
			if (pport)
				ip_unlock(pport);
		}
		act_unlock(thr_act);
		mutex_pause();
	}
	return (thr_act->thread);
}

/*
 * Unsynchronize with RPC (i.e., undo an act_lock_thread() call).
 * Called with thr_act locked, plus thread locks held that are
 * "correct" for thr_act's state.  Returns with nothing locked.
 */
void
act_unlock_thread(thread_act_t	thr_act)
{
	if (thr_act->thread)
		rpc_unlock(thr_act->thread);
	if (thr_act->pool_port)
		ip_unlock(thr_act->pool_port);
	act_unlock(thr_act);
}

/*
 * Synchronize with RPC given a pointer to a shuttle (instead of an
 * activation).  Called with nothing locked; returns with all
 * "appropriate" thread-related locks held (see act_lock_thread()).
 */
thread_act_t
thread_lock_act(
	thread_t	thread)
{
	thread_act_t	thr_act;

	while (1) {
		rpc_lock(thread);
		thr_act = thread->top_act;
		if (!thr_act)
			break;
		if (!act_lock_try(thr_act)) {
			rpc_unlock(thread);
			mutex_pause();
			continue;
		}
		if (thr_act->pool_port &&
			!ip_lock_try(thr_act->pool_port)) {
			rpc_unlock(thread);
			act_unlock(thr_act);
			mutex_pause();
			continue;
		}
		break;
	}
	return (thr_act);
}

/*
 * Unsynchronize with RPC starting from a pointer to a shuttle.
 * Called with RPC-related locks held that are appropriate to
 * shuttle's state; any activation is also locked.
 */
void
thread_unlock_act(
	thread_t	thread)
{
	thread_act_t 	thr_act;

	if (thr_act = thread->top_act) {
		if (thr_act->pool_port)
			ip_unlock(thr_act->pool_port);
		act_unlock(thr_act);
	}
	rpc_unlock(thread);
}

/*
 * install_special_handler
 *	Install the special returnhandler that handles suspension and
 *	termination, if it hasn't been installed already.
 *
 * Already locked: RPC-related locks for thr_act, but not
 * scheduling lock (thread_lock()) of the associated thread.
 */
void
install_special_handler(
	thread_act_t	thr_act)
{
	spl_t		spl;
	thread_t	thread = thr_act->thread;

#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
	    printf("act_%x: install_special_hdlr(%x)\n",current_act(),thr_act);
#endif	/* MACH_ASSERT */

	spl = splsched();
	if (thread)
		thread_lock(thread);
	install_special_handler_locked(thr_act);
	act_set_apc(thr_act);
	if (thread)
		thread_unlock(thread);
	splx(spl);
}

/*
 * install_special_handler_locked
 *	Do the work of installing the special_handler.
 *
 * Already locked: RPC-related locks for thr_act, plus the
 * scheduling lock (thread_lock()) of the associated thread.
 */
void
install_special_handler_locked(
	thread_act_t	thr_act)
{
	ReturnHandler	**rh;
	thread_t	thread = thr_act->thread;

	/* The work handler must always be the last ReturnHandler on the list,
	   because it can do tricky things like detach the thr_act.  */
	for (rh = &thr_act->handlers; *rh; rh = &(*rh)->next)
		/* */ ;
	if (rh != &thr_act->special_handler.next) {
		*rh = &thr_act->special_handler;
	}
	if (thread && thr_act == thread->top_act) {
		/*
		 * Temporarily undepress, so target has
		 * a chance to do locking required to
		 * block itself in special_handler().
		 */
		if (thread->depress_priority >= 0) {
			thread->priority = thread->depress_priority;
			/* Use special value -2 to indicate need
			 * to redepress priority in special_handler
			 * as thread blocks
			 */
			thread->depress_priority = -2;
			compute_priority(thread, FALSE);
		}	
	}
	act_set_apc(thr_act);
}

/*
 * act_set_thread_pool	- Assign an activation to a specific thread_pool.
 * Fails if the activation is already assigned to another pool.
 * If thread_pool == 0, we remove the thr_act from its thread_pool.
 *
 * Called the port containing thread_pool already locked.
 * Returns the same way.
 */
kern_return_t act_set_thread_pool(
	thread_act_t	thr_act,
	ipc_port_t	pool_port)
{
	thread_pool_t	thread_pool;

#if	MACH_ASSERT
	if (watchacts & WA_ACT_LNK)
		printf("act_set_thread_pool: %x(%d) -> %x\n",
			thr_act, thr_act->ref_count, thread_pool);
#endif	/* MACH_ASSERT */

	if (pool_port == 0) {
		thread_act_t *lact;

		if (thr_act->pool_port == 0)
			return KERN_SUCCESS;
		thread_pool = &thr_act->pool_port->ip_thread_pool;

		for (lact = &thread_pool->thr_acts; *lact;
					lact = &((*lact)->thread_pool_next)) {
			if (thr_act == *lact) {
				*lact = thr_act->thread_pool_next;
				break;
			}
		}
		act_lock(thr_act);
		thr_act->pool_port = 0;
		thr_act->thread_pool_next = 0;
		act_unlock(thr_act);
		act_deallocate(thr_act);
		return KERN_SUCCESS;
	}
	if (thr_act->pool_port != pool_port) {
		thread_pool = &pool_port->ip_thread_pool;
		if (thr_act->pool_port != 0) {
#if	MACH_ASSERT
			if (watchacts & WA_ACT_LNK)
			    printf("act_set_thread_pool found %x!\n",
							thr_act->pool_port);
#endif	/* MACH_ASSERT */
			return(KERN_FAILURE);
		}
		act_lock(thr_act);
		thr_act->pool_port = pool_port;

		/* The pool gets a ref to the activation -- have
		 * to inline operation because thr_act is already
		 * locked.
		 */
		act_locked_act_reference(thr_act);

		/* If it is available,
		 * add it to the thread_pool's available-activation list.
		 */
		if ((thr_act->thread == 0) && (thr_act->suspend_count == 0)) {
			thr_act->thread_pool_next = thread_pool->thr_acts;
			pool_port->ip_thread_pool.thr_acts = thr_act;
			if (thread_pool->waiting)
				thread_pool_wakeup(thread_pool);
		}
		act_unlock(thr_act);
	}

	return KERN_SUCCESS;
}

/*
 * act_locked_act_set_thread_pool- Assign activation to a specific thread_pool.
 * Fails if the activation is already assigned to another pool.
 * If thread_pool == 0, we remove the thr_act from its thread_pool.
 *
 * Called the port containing thread_pool already locked.
 * Also called with the thread activation locked.
 * Returns the same way.
 *
 * This routine is the same as `act_set_thread_pool()' except that it does
 * not call `act_deallocate(),' which unconditionally tries to obtain the
 * thread activation lock.
 */
kern_return_t act_locked_act_set_thread_pool(
	thread_act_t	thr_act,
	ipc_port_t	pool_port)
{
	thread_pool_t	thread_pool;

#if	MACH_ASSERT
	if (watchacts & WA_ACT_LNK)
		printf("act_set_thread_pool: %x(%d) -> %x\n",
			thr_act, thr_act->ref_count, thread_pool);
#endif	/* MACH_ASSERT */

	if (pool_port == 0) {
		thread_act_t *lact;

		if (thr_act->pool_port == 0)
			return KERN_SUCCESS;
		thread_pool = &thr_act->pool_port->ip_thread_pool;

		for (lact = &thread_pool->thr_acts; *lact;
					lact = &((*lact)->thread_pool_next)) {
			if (thr_act == *lact) {
				*lact = thr_act->thread_pool_next;
				break;
			}
		}

		thr_act->pool_port = 0;
		thr_act->thread_pool_next = 0;
		act_locked_act_deallocate(thr_act);
		return KERN_SUCCESS;
	}
	if (thr_act->pool_port != pool_port) {
		thread_pool = &pool_port->ip_thread_pool;
		if (thr_act->pool_port != 0) {
#if	MACH_ASSERT
			if (watchacts & WA_ACT_LNK)
			    printf("act_set_thread_pool found %x!\n",
							thr_act->pool_port);
#endif	/* MACH_ASSERT */
			return(KERN_FAILURE);
		}
		thr_act->pool_port = pool_port;

		/* The pool gets a ref to the activation -- have
		 * to inline operation because thr_act is already
		 * locked.
		 */
		act_locked_act_reference(thr_act);

		/* If it is available,
		 * add it to the thread_pool's available-activation list.
		 */
		if ((thr_act->thread == 0) && (thr_act->suspend_count == 0)) {
			thr_act->thread_pool_next = thread_pool->thr_acts;
			pool_port->ip_thread_pool.thr_acts = thr_act;
			if (thread_pool->waiting)
				thread_pool_wakeup(thread_pool);
		}
	}

	return KERN_SUCCESS;
}

/*
 * Activation control support routines internal to this file:
 */

/*
 * act_execute_returnhandlers()	- does just what the name says
 *
 * This is called by system-dependent code when it detects that
 * thr_act->handlers is non-null while returning into user mode.
 * Activations linked onto an thread_pool always have null thr_act->handlers,
 * so RPC entry paths need not check it.
 */
void act_execute_returnhandlers(
	void)
{
	spl_t		s;
	thread_t	thread;
	thread_act_t	thr_act = current_act();

#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
		printf("execute_rtn_hdlrs: thr_act=%x\n", thr_act);
#endif	/* MACH_ASSERT */

	s = splsched();
	act_clr_apc(thr_act);
	spllo();
	while (1) {
		ReturnHandler *rh;

		/* Grab the next returnhandler */
		thread = act_lock_thread(thr_act);
		(void)splsched();
		thread_lock(thread);
		rh = thr_act->handlers;
		if (!rh) {
			thread_unlock(thread);
			splx(s);
			act_unlock_thread(thr_act);
			return;
		}
		thr_act->handlers = rh->next;
		thread_unlock(thread);
		spllo();
		act_unlock_thread(thr_act);

#if	MACH_ASSERT
		if (watchacts & WA_ACT_HDLR)
		    printf( (rh == &thr_act->special_handler) ?
			"\tspecial_handler\n" : "\thandler=%x\n",
				    rh->handler);
#endif	/* MACH_ASSERT */

		/* Execute it */
		(*rh->handler)(rh, thr_act);
	}
}

/*
 * special_handler	- handles suspension, termination.  Called
 * with nothing locked.  Returns (if it returns) the same way.
 */
void
special_handler(
	ReturnHandler	*rh,
	thread_act_t	cur_act)
{
	spl_t		s;
	thread_t	lthread;
	thread_t	thread = act_lock_thread(cur_act);
	unsigned	alert_bits;
	exception_data_type_t
			codes[EXCEPTION_CODE_MAX];
	kern_return_t	kr;
	kern_return_t	exc_kr;

	assert(thread != THREAD_NULL);
#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
	    printf("\t\tspecial_handler(thr_act=%x(%d))\n", cur_act,
				(cur_act ? cur_act->ref_count : 0));
#endif	/* MACH_ASSERT */

	s = splsched();

	/*
	 * If someone has killed this invocation,
	 * invoke the return path with a terminated exception.
	 */
	if (!cur_act->active) {
		splx(s);
		act_unlock_thread(cur_act);
		act_machine_return(KERN_TERMINATED);
	}

	thread_lock(thread);
	thread->state &= ~TH_ABORT;	/* clear any aborts */
	thread_unlock(thread);
	splx(s);

	/* strip server terminated bit */
	alert_bits = cur_act->alerts & (~SERVER_TERMINATED);

	/* clear server terminated bit */
	cur_act->alerts &= ~SERVER_TERMINATED;

	if ( alert_bits ) {
		/*
		 * currently necessary to coordinate with the exception 
		 * code -fdr
		 */
		act_unlock_thread(cur_act);

		/* upcall exception/alert port */
		codes[0] = alert_bits;

		/*
		 * Exception makes a lot of assumptions. If there is no
		 * exception handler or the exception reply is broken, the 
		 * thread will be terminated and exception will not return. If
		 * we decide we don't like that behavior, we need to check
		 * for the existence of an exception port before we call 
		 * exception.
		 */
		exc_kr = alert_exception( EXC_RPC_ALERT, codes, 1 );

		/* clear the orphaned indication */
		cur_act->alerts &= ~ORPHANED;

		/* if this orphaned activation should be terminated... */
		if (exc_kr == KERN_RPC_TERMINATE_ORPHAN) {
			/*
			 * ... terminate the activation
			 *
			 * This is done in two steps.  First, the activation is
			 * disabled (prepared for termination); second, the
			 * `special_handler()' is executed again -- this time
			 * to terminate the activation.
			 * (`act_disable_task_locked()' arranges for the
			 * additional execution of the `special_handler().')
			 */

#if	THREAD_SWAPPER
			thread_swap_disable(cur_act);
#endif	/* THREAD_SWAPPER */

			/* acquire appropriate locks */
			task_lock(cur_act->task);
			act_lock_thread(cur_act);

			/* detach the activation from its task */
			kr = act_disable_task_locked(cur_act);
			assert( kr == KERN_SUCCESS );

			/* release locks */
			task_unlock(cur_act->task);
		}
		else {
			/* acquire activation lock again (released below) */
			act_lock_thread(cur_act);
			s = splsched();
			thread_lock(thread);
			if (thread->depress_priority == -2) {
				/* We were temporarily undepressed by
				 * install_special_handler; restore priority
				 * depression.
				 */
				thread->depress_priority = thread->priority;
				thread->priority = DEPRESSPRI;
				thread->sched_pri = DEPRESSPRI;
			}
			thread_unlock(thread);
			splx(s);
		}
	}

	/*
	 * If we're suspended, go to sleep and wait for someone to wake us up.
	 */
	if (cur_act->suspend_count) {
		if( cur_act->handlers == NULL ) {
			assert_wait((event_t)&cur_act->suspend_count, TRUE);
			act_unlock_thread(cur_act);
			thread_block((void (*)(void)) SAFE_SUSPENDED);
			act_lock_thread(cur_act);
		}

		/*
		 * If we're still (or again) suspended, go to sleep again
		 * after executing any new handlers that may have appeared.
		 */
		if (cur_act->suspend_count)
			install_special_handler(cur_act);
		else {
			s = splsched();
			thread_lock(thread);
			if (thread->depress_priority == -2) {
				/* We were temporarily undepressed by
				 * install_special_handler; restore priority
				 * depression.
				 */
				thread->depress_priority = thread->priority;
				thread->priority = DEPRESSPRI;
				thread->sched_pri = DEPRESSPRI;
			}
			thread_unlock(thread);
			splx(s);
		}
	}

	act_unlock_thread(cur_act);
}

/*
 * Try to nudge a thr_act into executing its returnhandler chain.
 * Ensures that the activation will execute its returnhandlers
 * before it next executes any of its user-level code.
 *
 * Called with thr_act's act_lock() and "appropriate" thread-related
 * locks held.  (See act_lock_thread().)  Returns same way.
 */
void
nudge(thread_act_t	thr_act)
{
#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
	    printf("\tact_%x: nudge(%x)\n", current_act(), thr_act);
#endif	/* MACH_ASSERT */

	/*
	 * Don't need to do anything at all if this thr_act isn't the topmost.
	 */
	if (thr_act->thread && thr_act->thread->top_act == thr_act) {
		/*
		 * If it's suspended, wake it up. 
		 * This should nudge it even on another CPU.
		 */
		thread_wakeup((event_t)&thr_act->suspend_count);
	}
}

/*
 * Update activation that belongs to a task created via kernel_task_create().
 */
void
act_user_to_kernel(
	thread_act_t	thr_act)
{
	pcb_user_to_kernel(thr_act);
	thr_act->kernel_loading = TRUE;
}

/*
 * Already locked: thr_act->task, RPC-related locks for thr_act
 *
 * Detach an activation from its task, and prepare it to terminate
 * itself.
 */
kern_return_t
act_disable_task_locked(
	thread_act_t	thr_act)
{
	thread_t	thread = thr_act->thread;
	task_t		task = thr_act->task;

#if	MACH_ASSERT
	if (watchacts & WA_EXIT) {
		printf("act_%x: act_disable_tl(thr_act=%x(%d))%sactive task=%x(%d)",
			       current_act(), thr_act, thr_act->ref_count,
			       (thr_act->active ? " " : " !"),
			       thr_act->task, thr_act->task? thr_act->task->ref_count : 0);
		if (thr_act->pool_port)
			printf(", pool_port %x", thr_act->pool_port);
		printf("\n");
		(void) dump_act(thr_act);
	}
#endif	/* MACH_ASSERT */
	if (thr_act->thr_acts.next) {
		/* Unlink the thr_act from the task's thr_act list,
		 * so it doesn't appear in calls to task_threads and such.
		 * The thr_act still keeps its ref on the task, however.
		 */
		mutex_lock(&task->act_list_lock);
		thr_act->thr_acts.next->prev = thr_act->thr_acts.prev;
		thr_act->thr_acts.prev->next = thr_act->thr_acts.next;
		thr_act->thr_acts.next = 0;

		task->thr_act_count--;

#if	THREAD_SWAPPER
		/*
		 * Thread is supposed to be unswappable by now...
		 */
		assert(thr_act->swap_state == TH_SW_UNSWAPPABLE ||
		       !(thread_swap_unwire_stack ||
			 thread_swap_unwire_user_stack));
#endif	/* THREAD_SWAPPER */
		task->res_act_count--;
		mutex_unlock(&task->act_list_lock);

		/* This will allow no more control ops on this thr_act. */
		thr_act->active = 0;

		/* Clean-up any ulocks that are still owned by the thread
		 * activation (acquired but not released or handed-off).
		 */
		act_ulock_release_all(thr_act);

		/* If not an empty activation, install special handler */
		if (thr_act->thread != THREAD_NULL) {

			/* When the special_handler gets executed,
			 * it will see the terminated condition and exit
			 * immediately.
			 */
			install_special_handler(thr_act);
		}

		/* If the target happens to be suspended,
		 * give it a nudge so it can exit.
		 */
		if (thr_act->suspend_count)
			nudge(thr_act);
		/* Drop the thr_act reference taken for being active.
		 * (There is still at least one reference left:
		 * the one we were passed.)
		 * Inline the deallocate because thr_act is locked.
		 */
		act_locked_act_deallocate(thr_act);
	}

	return(KERN_SUCCESS);
}

/*
 * act_alert	- Register an alert from this activation.
 *
 * Each set bit is propagated upward from (but not including) this activation,
 * until the top of the chain is reached or the bit is masked.
 */
kern_return_t
act_alert(thread_act_t thr_act, unsigned alerts)
{
	thread_t thread = act_lock_thread(thr_act);

#if	MACH_ASSERT
	if (watchacts & WA_ACT_LNK)
		printf("act_alert %x: %x\n", thr_act, alerts);
#endif	/* MACH_ASSERT */

	if (thread) {
		thread_act_t act_up = thr_act;
		while ((alerts) && (act_up != thread->top_act)) {
			act_up = act_up->higher;
			alerts &= act_up->alert_mask;
			act_up->alerts |= alerts;
		}
		/*
		 * XXXX If we reach the top, and it is blocked in glue
		 * code, do something to kick it.  XXXX
		 */
	}
	act_unlock_thread(thr_act);

	return KERN_SUCCESS;
}

kern_return_t act_alert_mask(thread_act_t thr_act, unsigned alert_mask)
{
	panic("act_alert_mask NOT YET IMPLEMENTED\n");
	return KERN_SUCCESS;
}

typedef struct GetSetState {
	struct ReturnHandler rh;
	int flavor;
	void *state;
	int *pcount;
	int result;
} GetSetState;

/* Local Forward decls */
kern_return_t get_set_state(
			thread_act_t thr_act, int flavor,
			thread_state_t state, int *pcount,
			void (*handler)(ReturnHandler *rh, thread_act_t thr_act));
void get_state_handler(ReturnHandler *rh, thread_act_t thr_act);
void set_state_handler(ReturnHandler *rh, thread_act_t thr_act);

/*
 * get_set_state(thr_act ...)
 *
 * General code to install g/set_state handler.
 * Called with thr_act's act_lock() and "appropriate"
 * thread-related locks held.  (See act_lock_thread().)
 */
kern_return_t
get_set_state(thread_act_t thr_act, int flavor, thread_state_t state, int *pcount,
			void (*handler)(ReturnHandler *rh, thread_act_t thr_act))
{
	GetSetState gss;
	spl_t s;

	/* Initialize a small parameter structure */
	gss.rh.handler = handler;
	gss.flavor = flavor;
	gss.state = state;
	gss.pcount = pcount;
	gss.result = KERN_ABORTED;	/* iff wait below is interrupted */

	/* Add it to the thr_act's return handler list */
	gss.rh.next = thr_act->handlers;
	thr_act->handlers = &gss.rh;

	s = splsched();
	act_set_apc(thr_act);
	splx(s);

#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR) {
	    printf("act_%x: get_set_state(thr_act=%x flv=%x state=%x ptr@%x=%x)",
		    current_act(), thr_act, flavor, state,
		    pcount, (pcount ? *pcount : 0));
	    printf((handler == get_state_handler ? "get_state_hdlr\n" :
		    (handler == set_state_handler ? "set_state_hdlr\n" :
			"hndler=%x\n")), handler); 
	}
#endif	/* MACH_ASSERT */

	assert(thr_act->thread);	/* Callers must ensure these */
	assert(thr_act != current_act());
	for (;;) {
		nudge(thr_act);
		/*
		 * Wait must be interruptible to avoid deadlock (e.g.) with
		 * task_suspend() when caller and target of get_set_state()
		 * are in same task.
		 */
		assert_wait((event_t)&gss, TRUE);
		act_unlock_thread(thr_act);
		thread_block((void(*)(void))0);	/* block self */
		if (gss.result != KERN_ABORTED)
			break;
		if (current_act()->handlers)
			act_execute_returnhandlers();
		act_lock_thread(thr_act);
	}

#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
	    printf("act_%x: get_set_state returns %x\n",
			    current_act(), gss.result);
#endif	/* MACH_ASSERT */

	return gss.result;
}

void
set_state_handler(ReturnHandler *rh, thread_act_t thr_act)
{
	GetSetState *gss = (GetSetState*)rh;

#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
		printf("act_%x: set_state_handler(rh=%x,thr_act=%x)\n",
			current_act(), rh, thr_act);
#endif	/* MACH_ASSERT */

	gss->result = act_machine_set_state(thr_act, gss->flavor,
						gss->state, *gss->pcount);
	thread_wakeup((event_t)gss);
}

void
get_state_handler(ReturnHandler *rh, thread_act_t thr_act)
{
	GetSetState *gss = (GetSetState*)rh;

#if	MACH_ASSERT
	if (watchacts & WA_ACT_HDLR)
		printf("act_%x: get_state_handler(rh=%x,thr_act=%x)\n",
			current_act(), rh, thr_act);
#endif	/* MACH_ASSERT */

	gss->result = act_machine_get_state(thr_act, gss->flavor,
			gss->state, 
			(mach_msg_type_number_t *) gss->pcount);
	thread_wakeup((event_t)gss);
}

kern_return_t
act_get_state_locked(thread_act_t thr_act, int flavor, thread_state_t state,
					mach_msg_type_number_t *pcount)
{
#if	MACH_ASSERT
    if (watchacts & WA_ACT_HDLR)
	printf("act_%x: act_get_state_L(thr_act=%x,flav=%x,st=%x,pcnt@%x=%x)\n",
		current_act(), thr_act, flavor, state, pcount,
		(pcount? *pcount : 0));
#endif	/* MACH_ASSERT */

    return(get_set_state(thr_act, flavor, state, (int*)pcount, get_state_handler));
}

kern_return_t
act_set_state_locked(thread_act_t thr_act, int flavor, thread_state_t state,
					mach_msg_type_number_t count)
{
#if	MACH_ASSERT
    if (watchacts & WA_ACT_HDLR)
	printf("act_%x: act_set_state_L(thr_act=%x,flav=%x,st=%x,pcnt@%x=%x)\n",
		current_act(), thr_act, flavor, state, count, count);
#endif	/* MACH_ASSERT */

    return(get_set_state(thr_act, flavor, state, (int*)&count, set_state_handler));
}

kern_return_t
act_set_state(thread_act_t thr_act, int flavor, thread_state_t state,
					mach_msg_type_number_t count)
{
    if (thr_act == THR_ACT_NULL || thr_act == current_act())
	    return(KERN_INVALID_ARGUMENT);

    if (act_lock_thread(thr_act) == THREAD_NULL) {
	    act_unlock(thr_act);
	    return(KERN_INVALID_ARGUMENT);
    }
    return(act_set_state_locked(thr_act, flavor, state, count));
    
}

kern_return_t
act_get_state(thread_act_t thr_act, int flavor, thread_state_t state,
					mach_msg_type_number_t *pcount)
{
    if (thr_act == THR_ACT_NULL || thr_act == current_act())
	    return(KERN_INVALID_ARGUMENT);

    if (act_lock_thread(thr_act) == THREAD_NULL) {
	    act_unlock(thr_act);
	    return(KERN_INVALID_ARGUMENT);
    }
    return(act_get_state_locked(thr_act, flavor, state, pcount));
}

/*
 * These two should be called at splsched()
 * Set/clear indicator to run APC (layered on ASTs)
 */
void
act_set_apc(thread_act_t thr_act)
{
	thread_ast_set(thr_act, AST_APC);
	if (thr_act == current_act()) {
		mp_disable_preemption();
		ast_propagate(thr_act, cpu_number());
		mp_enable_preemption();
	}
}

void
act_clr_apc(thread_act_t thr_act)
{
	thread_ast_clear(thr_act, AST_APC);
}

void
act_ulock_release_all(thread_act_t thr_act)
{
	ulock_t	ulock;

	while (!queue_empty(&thr_act->held_ulocks)) {
		ulock = (ulock_t) queue_first(&thr_act->held_ulocks);
		(void) lock_make_unstable(ulock, thr_act);
		(void) lock_release_internal(ulock, thr_act);
	}
}
