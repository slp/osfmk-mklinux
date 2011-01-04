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
/*
 *	File:	kern/sync_lock.c
 *	Author:	Joseph CaraDonna
 *
 *	Contains RT distributed lock synchronization services.
 */

#include <kern/etap_macros.h>
#include <kern/misc_protos.h>
#include <kern/sync_lock.h>
#include <kern/sync_policy.h>
#include <kern/sched_prim.h>
#include <kern/ipc_kobject.h>
#include <kern/ipc_sync.h>
#include <kern/etap_macros.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>

/*
 *	Ulock ownership MACROS
 *
 *	Assumes: ulock internal lock is held 
 */

#define ulock_ownership_set(ul, th)				\
	MACRO_BEGIN						\
	thread_act_t _th_act;					\
	_th_act = (th)->top_act;				\
	act_lock(_th_act);					\
	enqueue (&_th_act->held_ulocks, (queue_entry_t) (ul));  \
	act_unlock(_th_act);					\
	(ul)->owner = _th_act;					\
	MACRO_END

#define ulock_ownership_clear(ul)				\
	MACRO_BEGIN						\
	thread_act_t _th_act;					\
	_th_act = (ul)->owner;					\
	act_lock(_th_act);					\
	remqueue(&_th_act->held_ulocks, (queue_entry_t) (ul));  \
	act_unlock(_th_act);					\
	(ul)->owner = THR_ACT_NULL;				\
	MACRO_END

#define ulock_ownership_clear_inactive(ul)			   \
	MACRO_BEGIN						   \
	remqueue(&(ul)->owner->held_ulocks, (queue_entry_t) (ul)); \
	(ul)->owner = THR_ACT_NULL;				   \
	MACRO_END

/*
 *	Lock set ownership MACROS
 */

#define lock_set_ownership_set(ls, t)				\
	MACRO_BEGIN						\
	task_lock((t));						\
	enqueue_head(&(t)->lock_set_list, (queue_entry_t) (ls));\
	(t)->lock_sets_owned++;					\
	task_unlock((t));					\
	(ls)->owner = (t);					\
	MACRO_END

#define lock_set_ownership_clear(ls, t)				\
	MACRO_BEGIN						\
	task_lock((t));						\
	remqueue(&(t)->lock_set_list, (queue_entry_t) (ls));	\
	(t)->lock_sets_owned--;					\
	task_unlock((t));					\
	MACRO_END

/*
 *	ROUTINE:	lock_set_create		[exported]
 *
 *	Creates a lock set.
 *	The port representing the lock set is returned as a parameter.
 */      
kern_return_t
lock_set_create (
	task_t		task,
	lock_set_t	*new_lock_set,
	int		n_ulocks,
	int		policy)
{
	lock_set_t 	lock_set = LOCK_SET_NULL;
	ulock_t		ulock;
	int 		size;
	int 		x;

	*new_lock_set = LOCK_SET_NULL;

	if (task == TASK_NULL || n_ulocks <= 0 || policy > SYNC_POLICY_MAX)
		return KERN_INVALID_ARGUMENT;

	size = sizeof(struct lock_set) + (sizeof(struct ulock) * (n_ulocks-1));
	lock_set = (lock_set_t) kalloc (size);

	if (lock_set == LOCK_SET_NULL)
		return KERN_RESOURCE_SHORTAGE; 


	lock_set_lock_init(lock_set);
	lock_set->n_ulocks = n_ulocks;
	lock_set->policy = policy;
	lock_set->ref_count = 1;
	queue_init(&lock_set->handoff_ulocks);
	queue_init(&lock_set->held_ulocks);

	/*
	 *  Create and initialize the lock set port
	 */

	lock_set->port = ipc_port_alloc_kernel();
	if (lock_set->port == IP_NULL) {	
		/* This will deallocate the lock set */
		lock_set_dereference(lock_set);
		return KERN_RESOURCE_SHORTAGE; 
	}

	ipc_kobject_set (lock_set->port,
			(ipc_kobject_t) lock_set,
			IKOT_LOCK_SET);

	/*
	 *  Initialize each ulock in the lock set
	 */

	for (x=0; x < n_ulocks; x++) {
		ulock = (ulock_t) &lock_set->ulock_list[x];
		ulock_lock_init(ulock);
		ulock->lock_set  = lock_set;
		ulock->owner	 = THR_ACT_NULL;
		ulock->stable	 = TRUE;
		ulock->state	 = ULOCK_FREE;
		ulock->ho_thread = THREAD_NULL;
		ulock->blocked_count = 0;
		queue_init(&ulock->blocked_threads);
	}

	lock_set_ownership_set(lock_set, task);

	lock_set->active = TRUE;
	*new_lock_set = lock_set;

	return KERN_SUCCESS;
}

/*
 *	ROUTINE:	lock_set_destroy	[exported]
 *	
 *	Destroys a lock set.  This call will only succeed if the
 *	specified task is the SAME task name specified at the lock set's
 *	creation.
 *
 *	NOTES:
 *	- All threads currently blocked on the lock set's ulocks are awoken.
 *	- These threads will return with the KERN_LOCK_SET_DESTROYED error.
 */
kern_return_t
lock_set_destroy (task_t task, lock_set_t lock_set)
{
	thread_t	thread;
	ulock_t		ulock;


	/*
	 *  INSERT: access check
	 *
	 *  If this request is comming from a remote node
	 *  return with failure.  
	 *
	 *  Locks are always created and destroyed locally.
	 */

	if (task == TASK_NULL || lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_set->owner != task)
		return KERN_INVALID_RIGHT;


	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	/*
	 *  Deactivate lock set
	 */

	lock_set->active = FALSE;

	/*
	 *  If a ulock is currently held in the target lock set:
	 *
	 *  1) Wakeup all threads blocked on the ulock (if any).
	 *     Blocked threads will return with a KERN_LOCK_SET_DESTROYED error.
	 *
	 *  2) ulock ownership is cleared.
	 *     The thread currently holding the ulock is revoked of its
	 *     ownership.
	 *
	 */

	while (!queue_empty(&lock_set->held_ulocks)) {

		queue_remove_last(&lock_set->held_ulocks,
				   ulock,
				   ulock_t,
				   held_link);

		ulock_lock(ulock);
		while (!queue_empty(&ulock->blocked_threads)) {
 
			thread = sync_policy_dequeue(lock_set->policy,
						     &ulock->blocked_threads);

			if (thread == THREAD_NULL)
				panic("lock_set_destroy: 1");

			ulock->blocked_count--;

			LOCK_OPERATION_COMPLETE(thread);
			clear_wait(thread, KERN_LOCK_SET_DESTROYED, TRUE);
		}
		ulock_unlock(ulock);
		ulock_ownership_clear(ulock);
	}

	/*
	 *  Check to see if any threads are blocked on a ulock 
	 *  due to a lock hand-off operation (handoff or accept).
	 *
	 *  If so, wake them up.
	 */

	while (!queue_empty(&lock_set->handoff_ulocks)) {

		queue_remove_last(&lock_set->handoff_ulocks,
				   ulock,
				   ulock_t,
				   handoff_link);

		if ((thread = ulock->ho_thread) == THREAD_NULL)
			panic("lock_set_destroy: 2");

		LOCK_OPERATION_COMPLETE(thread);			
		clear_wait(thread, KERN_LOCK_SET_DESTROYED, TRUE);
	}

	lock_set_unlock(lock_set);
	lock_set_ownership_clear(lock_set, task);

	/*
	 *  Deallocate	
	 *
	 *  Drop the lock set reference, which inturn destroys the
	 *  lock set structure if the reference count goes to zero.
	 */

	ipc_port_dealloc_kernel(lock_set->port);
	lock_set_dereference(lock_set);

	return KERN_SUCCESS;
}

kern_return_t
lock_acquire (lock_set_t lock_set, int lock_id)
{
	ulock_t   ulock;
	thread_t  thread;


	if (lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_id < 0 || lock_id >= lock_set->n_ulocks)
		return KERN_INVALID_ARGUMENT;


	thread = current_thread();

	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock = (ulock_t) &lock_set->ulock_list[lock_id];
	ulock_lock(ulock);

	/*
	 *  Block the current thread if the lock is already held.
	 */

	if (ulock->state == ULOCK_HELD) {
		lock_set_unlock(lock_set);

		sync_policy_enqueue(lock_set->policy,
				    &ulock->blocked_threads,
				    thread);

		ulock->blocked_count++;
		ulock_unlock(ulock);

		/*
		 *  Block - Wait for lock to become available.
		 */

		assert_wait((event_t)0, TRUE);
 		ETAP_SET_REASON(thread, BLOCKED_ON_LOCK);
		thread_block((void (*)(void)) SAFE_MISCELLANEOUS);

		/*
		 *  Operation abort check
		 *
		 *  If thread was woken-up via some action other than
		 *  lock_release or lock_set_destroy (i.e. thread_terminate()),
		 *  then we need to dequeue the thread from the ulocks
		 *  blocked_threads list.
		 */

		if (LOCK_OPERATION_ABORTED(thread)) {
			ulock_lock(ulock);

			sync_policy_remqueue(lock_set->policy,
					     &ulock->blocked_threads,
					     thread);

			ulock->blocked_count--;
			ulock_unlock(ulock);

			return KERN_ABORTED;
		}
		return (thread->wait_result);
	}

	/*
	 *  Add the ulock to the lock set's held_ulocks list.
	 */

	queue_enter(&lock_set->held_ulocks, ulock, ulock_t, held_link);
	lock_set_unlock(lock_set);

	/*
	 *  Assign lock ownership
	 */

	ulock_ownership_set(ulock, thread);
	ulock->state = ULOCK_HELD;
	ulock_unlock(ulock);

	if (ulock->stable)
		return KERN_SUCCESS;
	else
		return KERN_LOCK_UNSTABLE;
}

kern_return_t
lock_release (lock_set_t lock_set, int lock_id)
{
	ulock_t	 ulock;

	if (lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_id < 0 || lock_id >= lock_set->n_ulocks)
		return KERN_INVALID_ARGUMENT;

	ulock = (ulock_t) &lock_set->ulock_list[lock_id];

	return (lock_release_internal(ulock, current_act()));
}

kern_return_t
lock_try (lock_set_t lock_set, int lock_id)
{
	ulock_t   ulock;


	if (lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_id < 0 || lock_id >= lock_set->n_ulocks)
		return KERN_INVALID_ARGUMENT;


	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock = (ulock_t) &lock_set->ulock_list[lock_id];
	ulock_lock(ulock);

	/*
	 *  If the lock is already owned, we return without blocking.
	 *
	 *  An ownership status is returned to inform the caller as to
	 *  whether it already holds the lock or another thread does.
	 */

	if (ulock->state == ULOCK_HELD) {
		lock_set_unlock(lock_set);

		if (ulock->owner == current_act()) {
			ulock_unlock(ulock);
			return KERN_LOCK_OWNED_SELF;
		}
		
		ulock_unlock(ulock);
		return KERN_LOCK_OWNED;
 	}

	/*
	 *  Add the ulock to the lock set's held_ulocks list.
	 */

	queue_enter(&lock_set->held_ulocks, ulock, ulock_t, held_link);
	lock_set_unlock(lock_set);

	ulock_ownership_set(ulock, current_thread());

	ulock->state = ULOCK_HELD;
	ulock_unlock(ulock);

	if (ulock->stable)
		return KERN_SUCCESS;
	else
		return KERN_LOCK_UNSTABLE;
}

kern_return_t
lock_make_stable (lock_set_t lock_set, int lock_id)
{
	ulock_t	 ulock;


	if (lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_id < 0 || lock_id >= lock_set->n_ulocks)
		return KERN_INVALID_ARGUMENT;


	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock = (ulock_t) &lock_set->ulock_list[lock_id];
	ulock_lock(ulock);
	lock_set_unlock(lock_set);

	if (ulock->owner != current_act()) {
		ulock_unlock(ulock);
		return KERN_INVALID_RIGHT;
	}

	ulock->stable = TRUE;
	ulock_unlock(ulock);

	return KERN_SUCCESS;
}

/*
 *	ROUTINE:	lock_make_unstable	[internal]
 *
 *	Marks the lock as unstable.
 *
 *	NOTES:
 *	- All future acquisitions of the lock will return with a
 *	  KERN_LOCK_UNSTABLE status, until the lock is made stable again.
 */
kern_return_t
lock_make_unstable (ulock_t ulock, thread_act_t thr_act)
{
	lock_set_t	lock_set;


	lock_set = ulock->lock_set;
	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock_lock(ulock);
	lock_set_unlock(lock_set);

	if (ulock->owner != thr_act) {
		ulock_unlock(ulock);
		return KERN_INVALID_RIGHT;
	}

	ulock->stable = FALSE;
	ulock_unlock(ulock);

	return KERN_SUCCESS;
}

/*
 *	ROUTINE:	lock_release_internal	[internal]
 *
 *	Releases the ulock.
 *	If any threads are blocked waiting for the ulock, one is woken-up.
 *
 */
kern_return_t
lock_release_internal (ulock_t ulock, thread_act_t thr_act)
{
	lock_set_t	lock_set;
	thread_t	acq_thread;	/* acquisition thread */
	int		result;


	if ((lock_set = ulock->lock_set) == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock_lock(ulock);

	if (ulock->owner != thr_act) {
		ulock_unlock(ulock);
		lock_set_unlock(lock_set);
		return KERN_INVALID_RIGHT;
	}

 	/*
	 *  If any threads are waiting for the ulock,
	 *  transfer the lock ownership to a waiting thread
	 *  and wake it up.
	 */

	if (!queue_empty(&ulock->blocked_threads)) {
		lock_set_unlock(lock_set);
	
		/*
		 *  Get the acquisition thread.
		 *
		 *  This is chosen by the policy from the list of threads
		 *  currently blocked on the ulock.
		 */

		acq_thread = sync_policy_dequeue(lock_set->policy,
						 &ulock->blocked_threads);

		if (acq_thread == THREAD_NULL)
			panic("lock_release: null thread dq");
		
		ulock->blocked_count--;

		/*
		 *  Transfer ulock ownership
		 *  from the current thread to the acquisition thread.
		 */

		if (thr_act->active)
			ulock_ownership_clear(ulock);
		else
			ulock_ownership_clear_inactive(ulock);		      
		ulock_ownership_set(ulock, acq_thread);

		if (ulock->stable)
			result = KERN_SUCCESS;
		else
			result = KERN_LOCK_UNSTABLE;
	
		ulock_unlock(ulock);

		/*
		 *  Make thread runnable
		 */

		LOCK_OPERATION_COMPLETE(acq_thread);
		clear_wait(acq_thread, result, TRUE);

		return KERN_SUCCESS;
	}

	/*
	 *  Remove the ulock from the lock set's held_ulocks list.
	 */

	queue_remove(&lock_set->held_ulocks, ulock, ulock_t, held_link);
	lock_set_unlock(lock_set);		

	/*
	 *  Disown ulock
	 */

	if (thr_act->active)
		ulock_ownership_clear(ulock);
	else
		ulock_ownership_clear_inactive(ulock);		      

	ulock->state = ULOCK_FREE;
	ulock_unlock(ulock);

	return KERN_SUCCESS;
}

kern_return_t
lock_handoff (lock_set_t lock_set, int lock_id)
{
	ulock_t   ulock;
	thread_t  ho_thread;	/* handoff thread   */
	thread_t  ac_thread;	/* accepting thread */
	int	  result;


	if (lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_id < 0 || lock_id >= lock_set->n_ulocks)
		return KERN_INVALID_ARGUMENT;


	ho_thread = current_thread();

	lock_set_lock(lock_set);

	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock = (ulock_t) &lock_set->ulock_list[lock_id];
	ulock_lock(ulock);

	if (ulock->owner != current_act()) {
		ulock_unlock(ulock);
		lock_set_unlock(lock_set);
		return KERN_INVALID_RIGHT;
	}
	
	/*
	 *  If the accepting thread (the receiver) is already waiting
	 *  to accept the lock from the handoff thread (the sender),
	 *  then perform the hand-off now.
	 */

	if ((ac_thread = ulock->ho_thread) != THREAD_NULL) {

		/*
		 *  Remove the ulock from the lock set's handoff_ulocks list.
		 */

		queue_remove(&lock_set->handoff_ulocks,
			     ulock,
			     ulock_t,
			     handoff_link);

		lock_set_unlock(lock_set);		

		/*
		 *  Transfer lock ownership
		 */
		 
		ulock_ownership_clear(ulock);
		ulock_ownership_set(ulock, ac_thread);

		ulock->ho_thread = THREAD_NULL;

		if (ulock->stable)
			result = KERN_SUCCESS;
		else
			result = KERN_LOCK_UNSTABLE;

		ulock_unlock(ulock);

		/*
		 *  Make the accepting thread runnable.
		 */

		LOCK_OPERATION_COMPLETE(ac_thread);
		clear_wait(ac_thread, result, TRUE);

		return KERN_SUCCESS;
	}		

	/*
	 *  Add the ulock to the lock set's handoff list.
	 */

	queue_enter(&lock_set->handoff_ulocks, ulock, ulock_t, handoff_link);
	lock_set_unlock(lock_set);

	ulock->ho_thread = ho_thread; 
	ulock_unlock(ulock);

	/*
	 *  Block - Wait for accepting thread.
	 */

	assert_wait((event_t)0, TRUE);
 	ETAP_SET_REASON(ho_thread, BLOCKED_ON_LOCK_HANDOFF);
	thread_block((void (*)(void)) SAFE_MISCELLANEOUS);

	/*
	 *  Operation abort check
	 *
	 *  If the thread was woken-up via some action other than
	 *  lock_handoff_accept or lock_set_destroy (i.e. thread_terminate),
	 *  then we need to clear the ulock's handoff state.
	 */

	if (LOCK_OPERATION_ABORTED(ho_thread)) {
		lock_set_lock(lock_set);
		/* 
		 *  Make sure the lock set is active before ripping the ulock
		 *  off of the handoff_ulocks queue, as lock_set_destroy may
		 *  have already done this.
		 */
		if (lock_set->active) {
			queue_remove(&lock_set->handoff_ulocks,
				     ulock,
				     ulock_t,
				     handoff_link);
			ulock_lock(ulock);
			lock_set_unlock(lock_set);
			ulock->ho_thread = THREAD_NULL; 
			ulock_unlock(ulock);
		} else
			lock_set_unlock(lock_set);

		return KERN_ABORTED;
	}

	return (ho_thread->wait_result);
}

kern_return_t
lock_handoff_accept (lock_set_t lock_set, int lock_id)
{
	ulock_t   ulock;
	thread_t  ho_thread;	/* handoff thread   */
	thread_t  ac_thread;	/* accepting thread */
	int	  result;


	if (lock_set == LOCK_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	if (lock_id < 0 || lock_id >= lock_set->n_ulocks)
		return KERN_INVALID_ARGUMENT;


	ac_thread = current_thread();

	lock_set_lock(lock_set);
	if (!lock_set->active) {
		lock_set_unlock(lock_set);
		return KERN_LOCK_SET_DESTROYED;
	}

	ulock = (ulock_t) &lock_set->ulock_list[lock_id];
	ulock_lock(ulock);

	/*
	 *  If the handoff thread (the sender) is already waiting to
	 *  hand-off the lock to the accepting thread (the receiver),
	 *  then perform the hand-off now.
	 */

	if ((ho_thread = ulock->ho_thread) != THREAD_NULL) {

		/*
		 *  The pending thread may be either an accepting thread
		 *  or a hand-off thread.  If it is an accepting thread, 
		 *  return with an error.
		 *
		 *  Only one accepting thread is allowed.
		 */

		if (ulock->owner != ho_thread->top_act) {
			ulock_unlock(ulock);
			lock_set_unlock(lock_set);
			return KERN_ALREADY_WAITING;
		}

		/*
		 *  Remove the ulock from the lock set's handoff_ulocks list.
		 */

		queue_remove(&lock_set->handoff_ulocks,
			     ulock,
			     ulock_t,
			     handoff_link);

		lock_set_unlock(lock_set);		

		/*
		 *  Transfer lock ownership
		 */

		ulock_ownership_clear(ulock);
		ulock_ownership_set(ulock, ac_thread);

		ulock->ho_thread = THREAD_NULL;
		ulock_unlock(ulock);

		/*
		 *  Make the handoff thread runnable.
		 */
		
		LOCK_OPERATION_COMPLETE(ho_thread);
		clear_wait(ho_thread, KERN_SUCCESS, TRUE);

		if (ulock->stable)
			return KERN_SUCCESS;
		else
			return KERN_LOCK_UNSTABLE;
	}		

	/*
	 *  Add the ulock to the lock set's handoff_ulocks list.
	 */

	queue_enter(&lock_set->handoff_ulocks, ulock, ulock_t, handoff_link);
	lock_set_unlock(lock_set);

	ulock->ho_thread = ac_thread; 
	ulock_unlock(ulock);

	/*
	 *  Block - Wait for handoff thread.
	 */

	assert_wait((event_t)0, TRUE);
 	ETAP_SET_REASON(ac_thread, BLOCKED_ON_LOCK_HANDOFF);
	thread_block((void (*)(void)) SAFE_MISCELLANEOUS);

	/*
	 *  Operation abort check
	 *
	 *  If the thread was woken-up via some action other than
	 *  lock_handoff or lock_set_destroy (i.e. thread_terminate),
	 *  then we need to clear the ulock's handoff state.
	 */

	if (LOCK_OPERATION_ABORTED(ac_thread)) {
		lock_set_lock(lock_set);
		/* 
		 *  Make sure the lock set is active before ripping the ulock
		 *  off of the handoff_ulocks queue, as lock_set_destroy may
		 *  have already done this.
		 */
		if (lock_set->active) {
			queue_remove(&lock_set->handoff_ulocks,
				     ulock,
				     ulock_t,
				     handoff_link);
			ulock_lock(ulock);
			lock_set_unlock(lock_set);
			ulock->ho_thread = THREAD_NULL; 
			ulock_unlock(ulock);
		} else
			lock_set_unlock(lock_set);

		return KERN_ABORTED;
	}

	return (ac_thread->wait_result);
}

/*
 *	Routine:	lock_set_reference
 *
 *	Take out a reference on a lock set.  This keeps the data structure
 *	in existence (but the lock set may be deactivated).
 */
void
lock_set_reference(lock_set_t lock_set)
{
	lock_set_lock(lock_set);
	lock_set->ref_count++;
	lock_set_unlock(lock_set);
}

/*
 *	Routine:	lock_set_dereference
 *
 *	Release a reference on a lock set.  If this is the last reference,
 *	the lock set data structure is deallocated.
 */
void
lock_set_dereference(lock_set_t lock_set)
{
	int	ref_count;
	int 	size;

	lock_set_lock(lock_set);
	ref_count = --(lock_set->ref_count);
	lock_set_unlock(lock_set);

	if (ref_count == 0) {
		size =	sizeof(struct lock_set) +
			(sizeof(struct ulock) * (lock_set->n_ulocks - 1));
		kfree((vm_offset_t) lock_set, size);
	}
}
