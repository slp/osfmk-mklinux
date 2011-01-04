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
 * File: policy.c
 *	
 * Implementation of R-Thread policy routines.
 *
 */

#include <rthreads.h>
#include <rthread_filter.h>
#include <rthread_internals.h>
#include <trace.h>
#include <mach/mach_traps.h>
#include <mach/mach_host.h>

/*
 *	Forward Internal Declarations
 */

void		rthread_policy_info(rthread_t, boolean_t);
kern_return_t	rthread_set_default_policy_real(policy_t, boolean_t);
void		rthread_set_default_attributes(rthread_t);

/*
 *  ROUTINE:	rthread_policy_info	[internal]
 *
 *  FUNCTION:	Get the specified rthread's current policy state from the
 *		kernel and update the rthread's policy state.
 *
 *		If the init parameter is TRUE, the rthread package's default
 *		policy state is set to the specified rthread's current policy
 *		state (This should only be TRUE when initializing the rthread
 *		package).
 *
 *		If the init parameter is FALSE, the rthread package's default 
 *		policy state is left untouched.
 */
 
void
rthread_policy_info(rthread_t th, boolean_t init)
{
	struct thread_basic_info basic_info;
	unsigned int		 count;
	kern_return_t 		 r;
	policy_t		 current_policy;
	int			 base_priority 	= RTHREAD_NO_PRIORITY;
	int			 max_priority  	= RTHREAD_NO_PRIORITY;
	int			 quantum 	= RTHREAD_NO_QUANTUM;

	/*
	 * Get the current scheduling information.
	 */

	count = THREAD_BASIC_INFO_COUNT;
	thread_info (th->wired,
		     THREAD_BASIC_INFO,
		     (thread_info_t) &basic_info,
		     &count);

	current_policy = basic_info.policy;

	switch (current_policy) {
		case POLICY_RR:	{
			struct policy_rr_info rr_info;

			count = POLICY_RR_INFO_COUNT;
			r = thread_info(th->wired,
					THREAD_SCHED_RR_INFO, 
					(thread_info_t) &rr_info,
					&count);

			if (r != KERN_SUCCESS)
				break;

			base_priority = rr_info.base_priority;
			max_priority  = rr_info.max_priority;
			quantum	      = rr_info.quantum;
			break;
		}
		case POLICY_FIFO: {
			struct policy_fifo_info fifo_info;

			count = POLICY_FIFO_INFO_COUNT;
			r = thread_info(th->wired,
					THREAD_SCHED_FIFO_INFO,
					(thread_info_t) &fifo_info,
					&count);

			if (r != KERN_SUCCESS)
				break;

			base_priority = fifo_info.base_priority;
			max_priority  = fifo_info.max_priority;
			break;
		}
		case POLICY_TIMESHARE: {
			struct policy_timeshare_info ts_info;

			count = POLICY_TIMESHARE_INFO_COUNT;
			r = thread_info(th->wired,
					THREAD_SCHED_TIMESHARE_INFO,
					(thread_info_t) &ts_info,
					&count);

			if (r != KERN_SUCCESS)
				break;

			base_priority = ts_info.cur_priority;
			max_priority  = ts_info.max_priority;
			break;
		}
	}

	/*
	 * Initialize specified rthread's policy state.
	 */

	rthread_lock();	
	th->policy   = current_policy;
	th->priority = base_priority;
	th->quantum  = quantum;
	rthread_unlock();


	if (init) {
		/*
		 * Initialize the R-Thread default policy state.
		 */

		rthread_control.default_policy	= current_policy;
		rthread_control.base_priority   = base_priority;
		rthread_control.max_priority	= max_priority;

		if (quantum == RTHREAD_NO_QUANTUM) {
			struct host_sched_info h_info;
	
			count = HOST_SCHED_INFO_COUNT;	
			r = host_info(mach_host_self(),
				      HOST_SCHED_INFO,
				      (host_info_t) &h_info,
				      &count);

			if (r == KERN_SUCCESS)
				quantum = h_info.min_quantum;
		}
		rthread_control.default_quantum = quantum;
	}
}

/*
 *  ROUTINE:	rthread_set_default_policy_real		[internal]
 *
 *  FUNCTION:	Changes the rthread default policy to that specified by the
 *		policy argument.  The current rthreads are effected only if 
 *		the change argument is TRUE. 
 *
 *  NOTE:	This call will produce unpredictable behavior if the change
 *		argument is TRUE and there are multiple rthreads executing 
 *		in the task.
 */
	
kern_return_t
rthread_set_default_policy_real(policy_t policy, boolean_t change)
{
	kern_return_t	r;
	int 		base_priority;
	int 		max_priority;
	int 		quantum;


	rthread_control_policy_lock();

	if (policy == rthread_control.default_policy) {
		rthread_control_policy_unlock();
		return (KERN_SUCCESS);
	}

	base_priority 	= rthread_control.base_priority;
	max_priority  	= rthread_control.max_priority;
	quantum		= rthread_control.default_quantum;

	rthread_control_policy_unlock();

	/*
	 * Switch task's default policy
	 */

	switch (policy) {
		case POLICY_RR:	{
			struct policy_rr_base  rr_base;

			rr_base.base_priority = base_priority;
			rr_base.quantum	      = quantum;

			r = task_policy (mach_task_self(),
					 POLICY_RR,
					 (policy_base_t) &rr_base,
					 POLICY_RR_BASE_COUNT,
					 FALSE,
					 change);
			break;
		}
		case POLICY_FIFO: {
			struct policy_fifo_base  fifo_base;

			fifo_base.base_priority = base_priority;
			r = task_policy (mach_task_self(),
					 POLICY_FIFO,
					 (policy_base_t) &fifo_base,
					 POLICY_FIFO_BASE_COUNT,
					 FALSE,
					 change);
			break;
		}
		case POLICY_TIMESHARE: {
			struct policy_timeshare_base  ts_base;

			ts_base.base_priority = base_priority;
			r = task_policy (mach_task_self(),
					 POLICY_TIMESHARE,
					 (policy_base_t) &ts_base,
					 POLICY_TIMESHARE_BASE_COUNT,
					 FALSE,
					 change);
			break;
		}
		default: {
			r = KERN_INVALID_POLICY;
			break;
		}
	}

	if (r == KERN_SUCCESS) {
		rthread_control_policy_lock();
		rthread_control.default_policy = policy;
		rthread_control_policy_unlock();
	}

	return (r);
}

/*
 *  ROUTINE:	rthread_set_default_policy
 *
 *  FUNCTION:	Changes the rthread package's default scheduling policy to that
 *		specified by the policy argument.  Rthreads currently executing
 *		in the task will not be effected.
 */
	
kern_return_t
rthread_set_default_policy(policy_t policy)
{
	return (rthread_set_default_policy_real(policy, FALSE));
}

/*
 *  ROUTINE:	rthread_get_default_policy
 *
 *  FUNCTION:	Returns the rthread package's current default policy.
 */
	
policy_t
rthread_get_default_policy()
{
	kern_return_t	r;
	policy_t 	default_policy;

	rthread_control_policy_lock();
	default_policy = rthread_control.default_policy;
	rthread_control_policy_unlock();

	return (default_policy);
}

/*
 *  ROUTINE:	rthread_set_priority
 *
 *  FUNCTION:	Changes the priority of the specifed rthread.
 */
	
kern_return_t
rthread_set_priority(rthread_t th, int priority)
{
	kern_return_t	r;
	policy_t	policy;

	rthread_lock();
	if (th->priority == priority) {
		rthread_unlock();
		return (KERN_SUCCESS);
	}

	policy = th->policy;
	rthread_unlock();

	switch (policy) {
		case POLICY_RR:	{
			struct policy_rr_base  rr_base;

			rr_base.base_priority = priority;
			
			rthread_control_policy_lock();
			rr_base.quantum = rthread_control.default_quantum;
			rthread_control_policy_unlock();

			r = thread_policy(th->wired,
					  POLICY_RR,
					  (policy_base_t) &rr_base,
					  POLICY_RR_BASE_COUNT,
					  FALSE);
			break;
		}
		case POLICY_FIFO: {
			struct policy_fifo_base  fifo_base;

			fifo_base.base_priority = priority;

			r = thread_policy(th->wired,
					  POLICY_FIFO,
					  (policy_base_t) &fifo_base,
					  POLICY_FIFO_BASE_COUNT,
					  FALSE);
			break;
		}
		case POLICY_TIMESHARE: {
			struct policy_timeshare_base  ts_base;

			ts_base.base_priority = priority;

			r = thread_policy(th->wired,
					  POLICY_TIMESHARE,
					  (policy_base_t) &ts_base,
					  POLICY_TIMESHARE_BASE_COUNT,
					  FALSE);
			break;
		}
		default: {
			r = KERN_INVALID_POLICY;
			break;
		}
	}

	if (r == KERN_SUCCESS) {
		rthread_lock();
		th->priority = priority;
		rthread_unlock();
	}

	return (r);
}

/*
 *  ROUTINE:	rthread_get_priority
 *
 *  FUNCTION:	Returns the current priority of the specified rthread.
 */
 
int
rthread_get_priority(rthread_t th)
{
	kern_return_t	r;
	int 		priority;

	if (th->status & (T_CHILD | T_MAIN)) {
		rthread_lock();
		priority = th->priority;
		rthread_unlock();
	} else  /* T_ACT */ {
		rthread_policy_info(th, FALSE);
		rthread_lock();
		priority = th->priority;
		rthread_unlock();
	}

	return (priority);
}

/*
 *  ROUTINE:	rthread_set_policy
 *
 *  FUNCTION:	Changes the scheduling policy of the specified rthread.
 */

kern_return_t
rthread_set_policy(rthread_t th, policy_t policy)
{
	kern_return_t	r;

	rthread_lock();
	if (th->policy == policy) {
		rthread_unlock();
		return (KERN_SUCCESS);
	}
	rthread_unlock();

	switch (policy) {
		case POLICY_RR:	{
			struct policy_rr_base rr_base;

			rr_base.base_priority = th->priority;
			rthread_control_policy_lock();
			rr_base.quantum = rthread_control.default_quantum;
			rthread_control_policy_unlock();

			r = thread_policy(th->wired,
					  POLICY_RR,
					  (policy_base_t) &rr_base,
					  POLICY_RR_BASE_COUNT,
					  FALSE);
			break;
		}
		case POLICY_FIFO: {
			struct policy_fifo_base fifo_base;

			fifo_base.base_priority = th->priority;

			r = thread_policy(th->wired,
					  POLICY_FIFO,
					  (policy_base_t) &fifo_base,
					  POLICY_FIFO_BASE_COUNT,
					  FALSE);
			break;
		}
		case POLICY_TIMESHARE: {
			struct policy_timeshare_base ts_base;

			ts_base.base_priority = th->priority;

			r = thread_policy(th->wired,
					  POLICY_TIMESHARE,
					  (policy_base_t) &ts_base,
					  POLICY_TIMESHARE_BASE_COUNT,
					  FALSE);
			break;
		}
		default: {
			r = KERN_INVALID_POLICY;
			break;
		}
	}		

	if (r == KERN_SUCCESS) {
		rthread_lock();
		th->policy = policy;
		rthread_unlock();
	}

	return (r);
}

/*
 *  ROUTINE:	rthread_get_policy
 *
 *  FUNCTION:	Returns the scheduling policy of the specified rthread.
 */

policy_t
rthread_get_policy(rthread_t th)
{
	kern_return_t	r;
	policy_t 	policy;

	if (th->status & (T_CHILD | T_MAIN)) {
		rthread_lock();
		policy = th->policy;
		rthread_unlock();
	} else  /* T_ACT */ {
		rthread_policy_info(th, FALSE);
		rthread_lock();
		policy = th->policy;
		rthread_unlock();
	}

	return (policy);
}

/*
 *  ROUTINE:	rthread_set_default_quantum
 *
 *  FUNCTION:	Changes the rthread package's default quantum value.
 *
 *  NOTE:	Quantums are only used by the round-robin scheduling policy.
 */
	
void
rthread_set_default_quantum(int quantum)
{
	kern_return_t	r;

	rthread_control_policy_lock();
	rthread_control.default_quantum = quantum;
	rthread_control_policy_unlock();
}

/*
 *  ROUTINE:	rthread_get_default_quantum
 *
 *  FUNCTION:	Returns the rthread package's default quantum value.
 */
	
int
rthread_get_default_quantum()
{
	kern_return_t	r;
	int 		quantum;

	rthread_control_policy_lock();
	quantum = rthread_control.default_quantum;
	rthread_control_policy_unlock();

	return (quantum);
}

/*
 *  ROUTINE:	rthread_get_max_priority
 *
 *  FUNCTION:	Returns the task's maximum priority value.
 */
 
int
rthread_get_max_priority()
{
	kern_return_t	r;
	int 		max;

	rthread_control_policy_lock();
	max = rthread_control.max_priority;
	rthread_control_policy_unlock();

	return (max);
}

/*
 *  ROUTINE:	rthread_set_default_attibutes	[internal]
 *
 *  FUNCTION:	Sets the specified rthread's scheduling attributes to the
 *		rthread package defaults.
 */

void
rthread_set_default_attributes(rthread_t th)
{
	kern_return_t	r;

	rthread_control_policy_lock();
	th->policy   = rthread_control.default_policy;
	th->priority = rthread_control.base_priority;
	th->quantum  = rthread_control.default_quantum;
	rthread_control_policy_unlock();
}
