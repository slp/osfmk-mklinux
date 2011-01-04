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
 *	File:	kern/sync_sema.c
 *	Author:	Joseph CaraDonna
 *
 *	Contains RT distributed semaphore synchronization services.
 */
/*
 *	Semaphores: Rule of Thumb
 *
 *  	A non-negative semaphore count means that the blocked threads queue is
 *  	empty.  A semaphore count of negative n means that the blocked threads
 *  	queue contains n blocked threads.
 */

#include <kern/etap_macros.h>
#include <kern/misc_protos.h>
#include <kern/sync_sema.h>
#include <kern/sync_policy.h>
#include <kern/spl.h>
#include <kern/ipc_kobject.h>
#include <kern/ipc_sync.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <kern/host.h>

/*
 *	Routine:	semaphore_create
 *
 *	Creates a semaphore.
 *	The port representing the semaphore is returned as a parameter.
 */
kern_return_t
semaphore_create (
	task_t		task,
	semaphore_t	*new_semaphore,
	int		policy,
	int		value)
{
	semaphore_t s  = SEMAPHORE_NULL;
	*new_semaphore = SEMAPHORE_NULL;


	if (task == TASK_NULL || value < 0 || policy > SYNC_POLICY_MAX)
		return KERN_INVALID_ARGUMENT;

	s = (semaphore_t) kalloc (sizeof(struct semaphore));

	if (s == SEMAPHORE_NULL)
		return KERN_RESOURCE_SHORTAGE; 

	semaphore_lock_init(s);
	queue_init(&s->blocked_threads);
	s->count  = value;
	s->ref_count = 1;
	s->policy = policy;

	/*
	 *  Create and initialize the semaphore port
	 */

	s->port	= ipc_port_alloc_kernel();
	if (s->port == IP_NULL) {	
		/* This will deallocate the semaphore */	
		semaphore_dereference(s);
		return KERN_RESOURCE_SHORTAGE; 
	}

	ipc_kobject_set (s->port,
			(ipc_kobject_t) s,
			IKOT_SEMAPHORE);

	/*
	 *  Associate the new semaphore with the task by adding
	 *  the new semaphore to the task's semaphore list.
	 *
	 *  Associate the task with the new semaphore by having the
	 *  semaphores task pointer point to the owning task's structure.
	 */

	task_lock(task);
	enqueue_head(&task->semaphore_list, (queue_entry_t) s);
	task->semaphores_owned++;
	task_unlock(task);
	s->owner = task;

	/*
	 *  Activate semaphore
	 */

	s->active = TRUE;
	*new_semaphore = s;

	return KERN_SUCCESS;
}		  

/*
 *	Routine:	semaphore_destroy
 *
 *	Destroys a semaphore.  This call will only succeed if the
 *	specified task is the SAME task name specified at the semaphore's
 *	creation.
 *
 *	All threads currently blocked on the semaphore are awoken.  These
 *	threads will return with the KERN_TERMINATED error.
 */
kern_return_t
semaphore_destroy (task_t task, semaphore_t semaphore)
{
	thread_t	thread;
	spl_t		spl_level;


	/*
	 *  XXX INSERT: access check
	 *
	 *  If this request is comming from a remote node
	 *  return with failure.  
	 *
	 *  Semaphores are always created and destroyed locally.
	 */

	if (task == TASK_NULL || semaphore == SEMAPHORE_NULL)
		return KERN_INVALID_ARGUMENT;

	if (semaphore->owner != task)
		return KERN_INVALID_RIGHT;

	
	spl_level = splsched();
	semaphore_lock(semaphore);

	if (!semaphore->active) {
		semaphore_unlock(semaphore);
		splx(spl_level);
		return KERN_TERMINATED;
	}

	/*
	 *  Deactivate semaphore
	 */

	semaphore->active = FALSE;

	/*
	 *  Wakeup blocked threads  
	 *  Blocked threads will receive a KERN_TERMINATED error
	 */

	if (semaphore->count < 0)
		while (semaphore->count++ < 0) {
			thread = sync_policy_dequeue(semaphore->policy,
						   &semaphore->blocked_threads);

			if (thread == THREAD_NULL)
				panic("semaphore_destroy: null thread dq");

			SEMAPHORE_OPERATION_COMPLETE(thread);
			clear_wait(thread, KERN_SEMAPHORE_DESTROYED, TRUE);
		}

	semaphore_unlock(semaphore);

	/*
	 *  Disown semaphore
	 */

	task_lock(task);
	remqueue(&task->semaphore_list, (queue_entry_t) semaphore);
	task->semaphores_owned--;
	task_unlock(task);

	splx(spl_level);

	/*
	 *  Deallocate
	 *
	 *  Drop the semaphore reference, which inturn deallocates the
	 *  semaphore structure if the reference count goes to zero.
	 */

	ipc_port_dealloc_kernel(semaphore->port);
	semaphore_dereference(semaphore);

	return KERN_SUCCESS;
}

kern_return_t
semaphore_signal_common (semaphore_t semaphore, thread_act_t thread_act,
			 boolean_t all);
/*
 *	Routine:	semaphore_signal_common
 *
 *	Common code for semaphore_signal, semaphore_signal_all, and
 *	semaphore_signal_thread.
 */
kern_return_t
semaphore_signal_common (semaphore_t semaphore, thread_act_t thread_act,
			 boolean_t all)
{
	thread_t	thread;
	spl_t		spl_level;
	kern_return_t	ret = KERN_SUCCESS;


	if (semaphore == SEMAPHORE_NULL)
		return KERN_INVALID_ARGUMENT;

	spl_level = splsched();
	semaphore_lock(semaphore);

	if (!semaphore->active) {
		semaphore_unlock(semaphore);
		splx(spl_level);
		return KERN_TERMINATED;
	}

	do {
		if (thread_act != THR_ACT_NULL) {
			thread = (thread_t)
				 queue_first(&semaphore->blocked_threads);
			while (1) {
				if (queue_end(&semaphore->blocked_threads,
					      (queue_entry_t) thread)) {
					thread = NULL;
					break;
				}
				if (thread == thread_act->thread)
					break;
				thread = (thread_t)
					 queue_next(&thread->wait_link);
			}
			if (thread == NULL) {
				ret = KERN_NOT_WAITING;
				break;
			}
			queue_remove(&semaphore->blocked_threads, thread,
				     thread_t, wait_link);
			semaphore->count++;
		} else {
			if (semaphore->count++ >= 0)
				break;
			thread = sync_policy_dequeue(semaphore->policy,
						     &semaphore->blocked_threads);
		}

		if (thread == THREAD_NULL)
			panic("semaphore_signal: null thread dq");

		SEMAPHORE_OPERATION_COMPLETE(thread);
		clear_wait(thread, KERN_SUCCESS, TRUE);
	} while (all && semaphore->count < 0);

	if (all)
		semaphore->count = 0;

	semaphore_unlock(semaphore);
	splx(spl_level);

	return ret;
}

/*
 *	Routine:	semaphore_signal
 *
 *	Increments the semaphore count by one.  If any threads are blocked
 *	on the semaphore ONE is woken up.
 */
kern_return_t
semaphore_signal (semaphore_t semaphore)
{
	return semaphore_signal_common (semaphore, THR_ACT_NULL, FALSE);
}

/*
 *	Routine:	semaphore_signal_all
 *
 *	Awakens ALL threads currently blocked on the semaphore.
 *	The semaphore count returns to zero.
 */
kern_return_t
semaphore_signal_all (semaphore_t semaphore)
{
	return semaphore_signal_common (semaphore, THR_ACT_NULL, TRUE);
}

/*
 *	Routine:	semaphore_signal_thread
 *
 *	If the specified thread_act is blocked on the semaphore, it is
 *	woken up and the semaphore count is incremented by one.  Otherwise
 *	the caller gets KERN_NOT_WAITING and the semaphore is unchanged.
 */
kern_return_t
semaphore_signal_thread (semaphore_t semaphore, thread_act_t thread_act)
{
	return semaphore_signal_common (semaphore, thread_act, FALSE);
}

kern_return_t semaphore_wait_common (semaphore_t seamphore,
				     tvalspec_t *wait_time);

/*
 *	Routine:	semaphore_wait
 *
 *	Decrements the semaphore count by one.  If the count is negative
 *	after the decrement, the calling thread blocks.
 *
 *	Assumes: Never called from interrupt context.
 */
kern_return_t
semaphore_wait (semaphore_t semaphore)
{	
	return (semaphore_wait_common(semaphore, (tvalspec_t *) 0));
}

/*
 *	Routine:	semaphore_timedwait
 *
 *	Decrements the semaphore count by one.  If the count is negative
 *	after the decrement, the calling thread blocks.  
 *	Sleep for max 'wait_time' (0=>forever)
 *
 *	Assumes: Never called from interrupt context.
 */
kern_return_t
semaphore_timedwait (semaphore_t semaphore, tvalspec_t wait_time)
{
	return (semaphore_wait_common(semaphore, &wait_time));
}

kern_return_t
semaphore_wait_common (semaphore_t semaphore, tvalspec_t *wait_time)
{
	thread_t	thread;
	spl_t		spl_level;
	int             res, ticks_to_wait;

	if (semaphore == SEMAPHORE_NULL)
		return KERN_INVALID_ARGUMENT;

	if (wait_time != 0) {
		if (BAD_TVALSPEC(wait_time) || wait_time->tv_sec < 0)
			return KERN_INVALID_VALUE;
		/* Compute the number of hz ticks to wait.  If the computation
		   overflows, the wait is very long (248 days with 32-bit
		   ints) so make it infinite.  */
		ticks_to_wait = wait_time->tv_sec * hz;
		if (ticks_to_wait < wait_time->tv_sec)
			wait_time = 0;
		else {
			int t = ticks_to_wait +
			    (wait_time->tv_nsec / (NSEC_PER_SEC)/hz);
			if (t < ticks_to_wait)
				wait_time = 0;
			else if (ticks_to_wait == 0)
				return KERN_OPERATION_TIMED_OUT;
			else ticks_to_wait = t;
		}
	}

	spl_level = splsched();
	semaphore_lock(semaphore);

	if (!semaphore->active) {
		semaphore_unlock(semaphore);
		splx(spl_level);
		return KERN_TERMINATED;
	}

	if (wait_time != 0 && ticks_to_wait == 0 && semaphore->count <= 0) {
		semaphore_unlock(semaphore);
		splx(spl_level);
		return KERN_OPERATION_TIMED_OUT;
	}

	if (--semaphore->count < 0) {

		thread = current_thread();

		sync_policy_enqueue(semaphore->policy,
				    &semaphore->blocked_threads,
				    thread);

		assert_wait(0, TRUE);
		semaphore_unlock(semaphore);
		splx(spl_level);

		ETAP_SET_REASON(thread, BLOCKED_ON_SEMAPHORE);
		if (wait_time != 0)
			thread_set_timeout(ticks_to_wait);
		thread_block((void (*)(void)) SAFE_MISCELLANEOUS);

		if (SEMAPHORE_OPERATION_ABORTED(thread)) {
			spl_level = splsched();
			semaphore_lock(semaphore);
			/* 
			 *  Make sure the semaphore is active before ripping
			 *  the thread off of the blocked_threads queue, as
			 *  semaphore_destroy may have already done this.
			 */
			if (semaphore->active) {
				sync_policy_remqueue(semaphore->policy,
						    &semaphore->blocked_threads,
						     thread);
				semaphore->count++;
			}
			semaphore_unlock(semaphore);
			splx(spl_level);

			res = thread->wait_result;
			if (res == THREAD_TIMED_OUT) {
				res = KERN_OPERATION_TIMED_OUT;
			} else {
				res = KERN_ABORTED;
			}
			return(res);
		}
		return(thread->wait_result);
	}

	semaphore_unlock(semaphore);
	splx(spl_level);

	return KERN_SUCCESS;
}

/*
 *	Routine:	semaphore_reference
 *
 *	Take out a reference on a semaphore.  This keeps the data structure
 *	in existence (but the semaphore may be deactivated).
 */
void
semaphore_reference(semaphore_t semaphore)
{
	spl_t	spl_level;

	spl_level = splsched();
	semaphore_lock(semaphore);
	semaphore->ref_count++;
	semaphore_unlock(semaphore);
	splx(spl_level);
}

/*
 *	Routine:	semaphore_dereference
 *
 *	Release a reference on a semaphore.  If this is the last reference,
 *	the semaphore data structure is deallocated.
 */
void
semaphore_dereference(semaphore_t semaphore)
{
	spl_t	spl_level;
	int	ref_count;

	spl_level = splsched();
	semaphore_lock(semaphore);
	ref_count = --(semaphore->ref_count);
	semaphore_unlock(semaphore);
	splx(spl_level);

	if (ref_count == 0)
		kfree((vm_offset_t) semaphore, sizeof(struct semaphore));
}
