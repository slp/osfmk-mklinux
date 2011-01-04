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
 * Thread scheduling policy management
 */


#include <mach_perf.h>
	
extern void	print_policy(struct policy_state 	*ps);

int
get_base_pri(struct policy_state *ps)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
		return(ps->base.tr_base.base_priority);
	case POLICY_RR:
		return(ps->base.rr_base.base_priority);
	case POLICY_FIFO:
		return(ps->base.ff_base.base_priority);
	default:
		test_error("get_base_pri", "invalid policy");
	}
}
	
void
set_base_pri(struct policy_state *ps, int base_pri)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
		ps->base.tr_base.base_priority = base_pri;
		return;
	case POLICY_RR:
		ps->base.rr_base.base_priority = base_pri;
		return;
	case POLICY_FIFO:
		ps->base.ff_base.base_priority = base_pri;
		return;
	default:
		test_error("set_base_pri", "invalid policy");
	}
}

int
get_max_pri(struct policy_state *ps)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
		return(ps->limit.tr_limit.max_priority);
	case POLICY_RR:
		return(ps->limit.rr_limit.max_priority);
	case POLICY_FIFO:
		return(ps->limit.ff_limit.max_priority);
	default:
		test_error("get_max_pri", "invalid policy");
	}
}

void
set_max_pri(struct policy_state *ps, int max_pri)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
		ps->limit.tr_limit.max_priority = max_pri;
		return;
	case POLICY_RR:
		ps->limit.rr_limit.max_priority = max_pri;
		return;
	case POLICY_FIFO:
		ps->limit.ff_limit.max_priority = max_pri;
		return;
	default:
		test_error("set_max_pri", "invalid policy");
	}
}

int
get_quantum(struct policy_state *ps)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
		return(-1);
	case POLICY_RR:
		return(ps->base.rr_base.quantum);
	case POLICY_FIFO:
		return(-1);
	default:
		test_error("get_quantum", "invalid policy");
	}
}

void
set_quantum(struct policy_state *ps, int quantum)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
		return;
	case POLICY_RR:
		ps->base.rr_base.quantum = quantum;
		return;
	case POLICY_FIFO:
		return;
	default:
		test_error("set_quantum", "invalid policy");
	}
}

kern_return_t
get_sched_attr(
	mach_port_t		task,
	mach_port_t		thread,
	struct policy_state 	*ps,
	boolean_t		print)
{
	struct thread_basic_info 	thread_basic_info;
	struct task_basic_info 		task_basic_info;
	mach_msg_type_number_t 		info_count;
	struct policy_state 		tmp_ps;

	if (!ps)
	  	ps = &tmp_ps;

	if ((task & thread) || ((!task) && (!thread)))
		test_error("get_sched_attr", "invalid arguments");

	if (task) {
		info_count = sizeof(struct task_basic_info);
		MACH_CALL(task_info, (task,
				      TASK_BASIC_INFO,
				      (task_info_t)&task_basic_info,
				      &info_count));
		ps->policy = task_basic_info.policy;
	} else {
		info_count = sizeof(struct thread_basic_info);
		MACH_CALL(thread_info, (thread,
					THREAD_BASIC_INFO,
					(thread_info_t)&thread_basic_info,
					&info_count));
		ps->policy = thread_basic_info.policy;
	}

	switch(ps->policy) {
	case POLICY_TIMESHARE: {
		if (task) {
			struct policy_timeshare_base time_share;
			info_count = POLICY_TIMESHARE_BASE_COUNT;

			MACH_CALL(task_info, (task,
					      TASK_SCHED_TIMESHARE_INFO,
					      (task_info_t)&time_share,
					      &info_count));
			ps->base.tr_base.base_priority =
			  			time_share.base_priority;
			ps->limit.tr_limit.max_priority = -1;
			  			
		} else {
			struct policy_timeshare_info time_share;

			info_count = POLICY_TIMESHARE_INFO_COUNT;
			MACH_CALL(thread_info, (thread,
						THREAD_SCHED_TIMESHARE_INFO,
						(thread_info_t)&time_share,
						&info_count));
			ps->base.tr_base.base_priority =
			  			time_share.base_priority;
			ps->limit.tr_limit.max_priority =
			  			time_share.max_priority;
		}
		break;
		}
	case POLICY_RR: {
		if (task) {
			struct policy_rr_base round_robin;

			info_count = POLICY_RR_BASE_COUNT;
			MACH_CALL(task_info, (task,
					      TASK_SCHED_RR_INFO,
					      (task_info_t)&round_robin,
					      &info_count));
			ps->base.rr_base.base_priority = 
			  			round_robin.base_priority;
			ps->base.rr_base.quantum = -1;
			ps->limit.rr_limit.max_priority = -1;
		} else {
			struct policy_rr_info round_robin;

			info_count = POLICY_RR_INFO_COUNT;
			MACH_CALL(thread_info, (thread,
						THREAD_SCHED_RR_INFO,
						(thread_info_t)&round_robin,
						&info_count));
			ps->base.rr_base.base_priority = 
			  			round_robin.base_priority;
			ps->base.rr_base.quantum = round_robin.quantum;
			ps->limit.rr_limit.max_priority =
						round_robin.max_priority;
		}
		break;
		}
	case POLICY_FIFO: {
		if (task) {
			struct policy_fifo_base fifo;

			info_count = POLICY_FIFO_BASE_COUNT;
			MACH_CALL(task_info, (task,
					      TASK_SCHED_FIFO_INFO,
					      (task_info_t)&fifo,
					      &info_count));
			ps->base.ff_base.base_priority = fifo.base_priority;
			ps->limit.ff_limit.max_priority = -1;
		} else {
			struct policy_fifo_info fifo;

			info_count = POLICY_FIFO_INFO_COUNT;
			MACH_CALL(thread_info, (thread,
						THREAD_SCHED_FIFO_INFO,
						(thread_info_t)&fifo,
						&info_count));
			ps->base.ff_base.base_priority = fifo.base_priority;
			ps->limit.ff_limit.max_priority = fifo.max_priority;
		}
		break;
		}
	default:
	  	printf("get_sched_attr: unknown policy 0x%x: ", ps->policy);
	}
	if (debug || print)
		print_policy(ps);
	return(KERN_SUCCESS);
}

kern_return_t
set_sched_attr(
	mach_port_t		task,
	mach_port_t		thread,
	struct policy_state 	*ps,
	boolean_t		print)
{
	processor_set_control_port_t 	pset;
	struct thread_basic_info 	thread_basic_info;
	mach_msg_type_number_t 		info_count;

	if ((task & thread) || ((!task) && (!thread)))
		test_error("set_sched_attr", "invalid arguments");


	MACH_FUNC(pset, get_processor_set, ());

	if (ps->policy == -1)		/* dont change it */
	  	return(KERN_SUCCESS);

	switch(ps->policy) {
	case POLICY_TIMESHARE:
		if (task) {
			MACH_CALL(task_set_policy,
				  (task,
				   pset,
				   ps->policy,
				   &ps->base.tr_base,
				   POLICY_TIMESHARE_BASE_COUNT,
				   &ps->limit.tr_limit,
				   POLICY_TIMESHARE_LIMIT_COUNT,
				   TRUE));
		} else {
			MACH_CALL(thread_set_policy,
				  (thread,
				   pset,
				   ps->policy,
				   &ps->base.tr_base,
				   POLICY_TIMESHARE_BASE_COUNT,
				   &ps->limit.tr_limit,
				   POLICY_TIMESHARE_LIMIT_COUNT));
		}
		break;
	case POLICY_RR:
		if (get_quantum(ps) == -1) {
			test_error("set_sched_attr", "invalid quantum value");
		}
		if (task) {
		  	MACH_CALL(task_set_policy,
				  (task,
				   pset,
				   ps->policy,
				   &ps->base.rr_base,
				   POLICY_RR_BASE_COUNT,
				   &ps->limit.rr_limit,
				   POLICY_RR_LIMIT_COUNT,
				   TRUE));
		} else {
		  	MACH_CALL(thread_set_policy,
				  (thread,
				   pset,
				   ps->policy,
				   &ps->base.rr_base,
				   POLICY_RR_BASE_COUNT,
				   &ps->limit.rr_limit,
				   POLICY_RR_LIMIT_COUNT));
		}
		break;
	case POLICY_FIFO:
		if (task) {
			MACH_CALL(task_set_policy,
				  (task,
				   pset,
				   ps->policy,
				   &ps->base.ff_base,
				   POLICY_FIFO_BASE_COUNT,
				   &ps->limit.ff_limit,
				   POLICY_FIFO_LIMIT_COUNT,
				   TRUE));
		} else {
			MACH_CALL(thread_set_policy,
				  (thread,
				   pset,
				   ps->policy,
				   &ps->base.ff_base,
				   POLICY_FIFO_BASE_COUNT,
				   &ps->limit.ff_limit,
				   POLICY_FIFO_LIMIT_COUNT));
		}
		break;
	}
	if (debug || print)
		print_policy(ps);
	return KERN_SUCCESS;
}

void
print_policy(
	struct policy_state 	*ps)
{
	switch(ps->policy) {
	case POLICY_TIMESHARE:
	  	printf("TIMESHARE policy, base %d, max %d\n",
		       get_base_pri(ps),
		       get_max_pri(ps));
		return;
	case POLICY_RR:
	  	printf("RR policy, base %d, max %d quantum %d\n",
		       get_base_pri(ps),
		       get_max_pri(ps),
		       get_quantum(ps));
		return;
	case POLICY_FIFO:
	  	printf("FIFO policy, base %d, max %d\n",
		       get_base_pri(ps),
		       get_max_pri(ps));
		return;
	default:
		printf("UNKNOWN policy\n");
		return;
	}
}

/*
 * Update policy based on policy, base_pri, max_pri & quantum options
 * Affects current task & threads.
 */

void
update_policy(print)
{
	struct 	policy_state new_ps;
	struct 	policy_state old_ps;

	get_thread_sched_attr(mach_thread_self(), &old_ps, FALSE);
	if (sched_policy == -1) {
		if (base_pri == -1 && max_pri == -1 && quantum == -1)
			return;
		new_ps.policy = old_ps.policy;
	} else
	  	new_ps.policy = sched_policy;

	set_base_pri(&new_ps, base_pri);
	set_max_pri(&new_ps, max_pri);
	set_quantum(&new_ps, quantum);

	if (get_base_pri(&new_ps) == -1)
		set_base_pri(&new_ps, get_base_pri(&old_ps));
	if (get_max_pri(&new_ps) == -1)
		set_max_pri(&new_ps, get_max_pri(&old_ps));
	if (get_quantum(&new_ps) == -1)
	  	set_quantum(&new_ps, get_quantum(&old_ps));

	set_task_sched_attr(mach_task_self(), &new_ps, print);
}


