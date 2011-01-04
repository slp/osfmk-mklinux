/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <cthreads.h>

#include <mach/policy.h>
#include <mach/thread_info.h>
#include <mach/mach_interface.h>
#include <mach/mach_traps.h>
#include <mach/mach_host.h>
#include <mach/thread_switch.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/assert.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/malloc.h>

extern mach_port_t	default_processor_set;
int			host_cpus;

char		*cpu_name;
char		*cpu_subname;
cpu_type_t	cpu_type;
cpu_subtype_t	cpu_subtype;
extern	int slot_name(cpu_type_t	cpu_type,
		      cpu_subtype_t	cpu_subtype,
		      char		**cpu_name,
		      char		**cpu_subname);

void *
server_thread_bootstrap(
	void *dummy)
{
	struct server_thread_priv_data	priv_data;
	void **args;
	int (*fn)(void *);
	void *arg;
	struct task_struct *tsk;
	int ret;
	osfmach3_jmp_buf jmp_buf;
	extern int sys_exit(int error_code);

	args = (void **) dummy;
	fn = (int(*)(void *)) args[0];
	arg = args[1];
	tsk = (struct task_struct *) args[2];

	cthread_set_name(cthread_self(), "kernel thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);
	priv_data.current_task = tsk;
#if 0	/* XXX ? */
	tsk->osfmach3.thread->mach_thread_port = mach_thread_self();
#endif
	tsk->osfmach3.thread->under_server_control = TRUE;
	tsk->osfmach3.thread->active_on_cthread = cthread_self();

	uniproc_enter();

	priv_data.jmp_buf = &jmp_buf;
	if (osfmach3_setjmp(priv_data.jmp_buf)) {
		/*
		 * The kernel thread is being terminated.
		 */
		uniproc_exit();
		cthread_set_name(cthread_self(), "dead kernel thread");
		cthread_detach(cthread_self());
		cthread_exit((void *) 0);
		/*NOTREACHED*/
		panic("server_thread_bootstrap: the zombie cthread walks !\n");
	}

	kfree(args);
	while (current->state != TASK_RUNNING) {
		schedule();	/* wait until we're resumed by our parent */
	}
	ret = (*fn)(arg);
	sys_exit(ret);

	/*NOTREACHED*/
	panic("server_thread_bootstrap: the zombie kernel thread walks !\n");
}

kern_return_t
server_thread_start(
	void	*(*func)(void *),
	void	*arg)
{
	cthread_t			child;

	child = cthread_fork(func, arg);
	if (child == CTHREAD_NULL) {
		panic("server_thread_start: cthread_fork failed");
		return KERN_FAILURE;
	}

	/* let the new thread start */
	osfmach3_yield();

	return KERN_SUCCESS;
}



void
server_thread_set_kernel_limit(void)
{
	cthread_set_kernel_limit(0);
}

void
server_thread_init(void)
{
	kern_return_t		kr;
	static int		first_time = 1;
	policy_rr_base_data_t	rr_base;
	policy_rr_limit_data_t	rr_limit;
	struct host_basic_info	hbi;
	mach_msg_type_number_t	count;

	if (first_time)
		first_time = 0;
	else
		panic("server_thread_init called again");

 	/*
 	 * Set the server's task scheduling to be POLICY_RR.
 	 */
	rr_base.quantum = 10;	/* XXX should be larger ? */
	rr_base.base_priority = BASEPRI_SERVER;
	rr_limit.max_priority = BASEPRI_SERVER;
 
	kr = task_set_policy(mach_task_self(), default_processor_set,
			     POLICY_RR,
			     (policy_base_t) &rr_base, POLICY_RR_BASE_COUNT,
			     (policy_limit_t) &rr_limit, POLICY_RR_LIMIT_COUNT,
			     TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(3, kr, ("server_thread_init: task_set_policy"));
	}

	count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), HOST_BASIC_INFO,
		       (host_info_t) &hbi, &count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("server_thread_init: host_info"));
		host_cpus = 1;
	} else {
		host_cpus = hbi.avail_cpus;
	}

	cpu_type = hbi.cpu_type;
	cpu_subtype = hbi.cpu_subtype;
	slot_name(hbi.cpu_type, hbi.cpu_subtype, &cpu_name, &cpu_subname);

#if 	CONFIG_OSFMACH3_DEBUG
	printk("Using %d processor%s (type \"%s\" subtype \"%s\").\n",
	       host_cpus, host_cpus > 1 ? "s" : "",
	       cpu_name, cpu_subname);
#endif	/* CONFIG_OSFMACH3_DEBUG */

	server_thread_set_kernel_limit();
}

/* 
 * Keeps track of number of wired cthreads.  No other function is allowed to
 * call cthread_(un)wire, or deadlock may result due to the server loop not
 * knowing how many wired threads exist.
 */
void
server_thread_wire(void)
{
}

void
server_thread_unwire(void)
{
}

#ifdef	CONFIG_OSFMACH3_DEBUG
void
server_thread_blocking(
	boolean_t	preemptible)
{
	if (preemptible) {
		uniproc_preemptible();
	} else {
		uniproc_exit();
	}
}

void
server_thread_unblocking(
	boolean_t	preemptible)
{
	if (preemptible) {
		uniproc_unpreemptible();
	} else {
		uniproc_enter();
	}
}
#endif	/* CONFIG_OSFMACH3_DEBUG */

kern_return_t
server_thread_priorities(
	int	priority,
	int	max_priority)
{
	kern_return_t	kr;
	struct thread_basic_info ti;
	mach_msg_type_number_t ti_size;

	/*
	 * First we have to get the current scheduling policy, because there
	 * is no way to set the priority without also setting the policy.
	 */
	ti_size = THREAD_BASIC_INFO_COUNT;
	kr = thread_info(mach_thread_self(), THREAD_BASIC_INFO,
			 (thread_info_t)&ti, &ti_size);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(2, kr, ("server_thread_priorities: thread_info"));
		return kr;
	}

	/*
	 * Set the priorities using the correct scheduling policy.
	 */
	switch (ti.policy) {
	    case POLICY_RR: {
		    struct policy_rr_base rr_base;
		    struct policy_rr_limit rr_limit;

		    rr_base.quantum = 10;	/* XXX should be larger ? */
		    rr_base.base_priority = priority;
		    rr_limit.max_priority = max_priority;
		    kr = thread_set_policy(mach_thread_self(),
					   default_processor_set,
					   POLICY_RR,
					   (policy_base_t) &rr_base,
					   POLICY_RR_BASE_COUNT,
					   (policy_limit_t) &rr_limit,
					   POLICY_RR_LIMIT_COUNT);
		    break;
	    }
	    case POLICY_TIMESHARE: {
		    struct policy_timeshare_base ts_base;
		    struct policy_timeshare_limit ts_limit;

		    ts_base.base_priority = priority;
		    ts_limit.max_priority = max_priority;
		    kr = thread_set_policy(mach_thread_self(),
					   default_processor_set,
					   POLICY_TIMESHARE,
					   (policy_base_t) &ts_base,
					   POLICY_TIMESHARE_BASE_COUNT,
					   (policy_limit_t) &ts_limit,
					   POLICY_TIMESHARE_LIMIT_COUNT);
		    break;
	    }
	    case POLICY_FIFO: {
		    struct policy_fifo_base ff_base;
		    struct policy_fifo_limit ff_limit;

		    ff_base.base_priority = priority;
		    ff_limit.max_priority = max_priority;
		    kr = thread_set_policy(mach_thread_self(),
					   default_processor_set,
					   POLICY_FIFO,
					   (policy_base_t) &ff_base,
					   POLICY_FIFO_BASE_COUNT,
					   (policy_limit_t) &ff_limit,
					   POLICY_FIFO_LIMIT_COUNT);
		    break;
	    }
	    default:
		kr = KERN_FAILURE;
		break;
	}

	return kr;
}

void
server_thread_yield(
	mach_msg_timeout_t	time)
{
	if (time) {
		/* depress our priority for "time" milliseconds */
		(void) thread_switch(MACH_PORT_NULL,
				     SWITCH_OPTION_DEPRESS,
				     time);
	} else {
		/* just switch to another thread */
		(void) thread_switch(MACH_PORT_NULL,
				     SWITCH_OPTION_NONE,
				     0);
	}
}

void
osfmach3_yield(void)
{
	if (intr_count == 0 && (bh_active & bh_mask)) {
		intr_count++;
		do_bottom_half();
		intr_count--;
	}

	uniproc_exit();
	server_thread_yield(10);		/* yield for 10 ms */
	/* 
	 * The other threads waiting for the uniproc mutex should be
	 * able to run here (the mutex queue is a FIFO).
	 */
	uniproc_enter();
}

void
osfmach3_setrun(
	struct task_struct *task)
{
	ASSERT(holding_uniproc());
	ASSERT(task->state != TASK_ZOMBIE);
	ASSERT(task->state >= TASK_RUNNING && task->state <= TASK_SWAPPING);
	condition_signal(&task->osfmach3.thread->mach_wait_channel);
}
