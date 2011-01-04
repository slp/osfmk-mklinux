/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <cthreads.h>
#include <mach/mach_interface.h>
#include <mach/mach_host.h>
#include <mach/time_value.h>
#include <mach/task_info.h>
#include <mach/thread_info.h>
#include <mach/policy.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/setjmp.h>
#include <osfmach3/fake_interrupt.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/malloc.h>

#include <asm/page.h>

extern boolean_t	in_kernel;	/* from libcthreads */
extern boolean_t	use_activations;

extern struct osfmach3_mach_task_struct init_osfmach3_task;

asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;

	/* endless idle loop with no priority at all */
	current->priority = 1;
	osfmach3_set_priority(current);
	for (;;) {
		schedule();
	}
}

extern void osfmach3_halt_system(boolean_t reset);
void hard_reset_now(void)
{
	osfmach3_halt_system(TRUE);
}

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
}

void flush_thread(void)
{
	/* loose Mach access privileges */
	current->osfmach3.task->mach_aware = FALSE;
}

void
release_thread(struct task_struct *dead_task)
{
}

void
osfmach3_cleanup_task(struct task_struct *tsk)
{
	struct server_thread_priv_data	*priv_datap;
	kern_return_t	kr;
	mach_port_t	task_port, thread_port;

	priv_datap = server_thread_get_priv_data(cthread_self());

	/*
	 * Synchronize with pending fake interrupts.
	 */
	cancel_fake_interrupt(tsk);

	task_port = tsk->osfmach3.task->mach_task_port;
	thread_port = tsk->osfmach3.thread->mach_thread_port;

	/*
	 * Turn off notifications.
	 */
	osfmach3_notify_deregister(tsk);

	if (!--tsk->osfmach3.task->mach_task_count) {
		/*
		 * Get the Mach task info for the last time.
		 */
		osfmach3_update_task_info(tsk);

		/*
		 * Terminate the Mach task.
		 */
		kr = task_terminate(task_port);
		if (kr != KERN_SUCCESS) {
			if (kr != MACH_SEND_INVALID_DEST &&
			    kr != KERN_INVALID_ARGUMENT) {
				MACH3_DEBUG(1, kr,
					    ("osfmach3_cleanup_task: "
					     "task_terminate(task=0x%x)",
					     task_port));
			}
		}

		/*
		 * Destroy the Mach task port.
		 */
		kr = mach_port_destroy(mach_task_self(), task_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_cleanup_task: "
				     "mach_port_destroy(task_port=0x%x)",
				     task_port));
		}
		tsk->osfmach3.task->mach_task_port = MACH_PORT_NULL;
	} else if (tsk->osfmach3.task == init_task.osfmach3.task) {
		/*
		 * A kernel thread: don't terminate a server cthread's
		 * kernel thread !
		 */
		tsk->osfmach3.thread->mach_thread_port = MACH_PORT_NULL;
		thread_port = MACH_PORT_NULL;
	} else if (thread_port != MACH_PORT_NULL) {
		/*
		 * Terminate the Mach thread.
		 */
		kr = thread_terminate(thread_port);
		if (kr != KERN_SUCCESS) {
			if (kr != KERN_INVALID_ARGUMENT &&
			    kr != MACH_SEND_INVALID_DEST) {
				MACH3_DEBUG(1, kr,
					    ("osfmach3_cleanup_task: "
					     "thread_terminate(thread=0x%x)",
					     thread_port));
			}
		}
	}

	/*
	 * Destroy the exception port.
	 */
	osfmach3_trap_terminate_task(tsk);

	/*
	 * Destroy the thread port.
	 */
	if (thread_port != MACH_PORT_NULL) {
		kr = mach_port_destroy(mach_task_self(), thread_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_cleanup_task: "
				     "mach_port_destroy(thread_port=0x%x)",
				     thread_port));
		}
	}

	/*
	 * Destroy the exception reply port, since we won't send a reply.
	 */
	if (MACH_PORT_VALID(priv_datap->reply_port)) {
		kr = mach_port_destroy(mach_task_self(),
				       priv_datap->reply_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr, ("osfmach3_cleanup_task: "
					    "mach_port_destroy(reply=0x%x)",
					    priv_datap->reply_port));
		}
		priv_datap->reply_port = MACH_PORT_NULL;
	}

	if (tsk->osfmach3.task->mach_task_count == 0) {
		/*
		 * Free the structure and make sure
		 * we don't use it afterwards.
		 */
		kfree(tsk->osfmach3.task);
		tsk->osfmach3.task = NULL;
	}

	/*
	 * This allows release() to deallocate the task structure,
	 * so do it at the very last moment. We should not block
	 * between this point and the place where we switch to a less
	 * endangered context...
	 */
	tsk->osfmach3.thread->mach_thread_port = MACH_PORT_NULL;
}

extern void release(struct task_struct *p);

void
osfmach3_terminate_task(struct task_struct *tsk)
{
	struct server_thread_priv_data	*priv_datap;

	priv_datap = server_thread_get_priv_data(cthread_self());

	ASSERT(current == tsk);

	/* cleanup the Mach resources */
	osfmach3_cleanup_task(tsk);

	/* switch to the Linux server context */
	uniproc_switch_to(current, &init_task);

	if (tsk->osfmach3.task == init_task.osfmach3.task) {
		/* a kernel thread... */
		release(tsk);	/* XXX do the equivalent of wait() */
		uniproc_exit();
		cthread_detach(cthread_self());
		cthread_exit((void *) 0);
		/*NOTREACHED*/
		panic("osfmach3_terminate_task: the zombie walks!\n");
	}

	/*
	 * Recycle this thread by jumping back to ux_server_loop().
	 */
	osfmach3_longjmp(priv_datap->jmp_buf, 1);

	panic("osfmach3_terminate_task: recycling failed");
}

void
osfmach3_update_thread_info(
	struct task_struct *tsk)
{
	kern_return_t			kr;
	struct thread_basic_info	thbi;
	mach_msg_type_number_t		count;

	if (tsk->osfmach3.task == &init_osfmach3_task ||
	    tsk->osfmach3.thread == NULL ||
	    !MACH_PORT_VALID(tsk->osfmach3.thread->mach_thread_port))
		return;

	count = THREAD_BASIC_INFO_COUNT;
	kr = thread_info(tsk->osfmach3.thread->mach_thread_port,
			 THREAD_BASIC_INFO,
			 (thread_info_t) &thbi,
			 &count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(2, kr,
			    ("osfmach3_update_thread_info(%p): "
			     "thread_info(0x%x, THREAD_BASIC_INFO)",
			     tsk, tsk->osfmach3.thread->mach_thread_port));
		return;
	}
	tsk->utime = thbi.user_time.seconds * HZ;
	tsk->utime += (thbi.user_time.microseconds * HZ) / TIME_MICROS_MAX;
	tsk->stime = thbi.system_time.seconds * HZ;
	tsk->stime += (thbi.system_time.microseconds * HZ) / TIME_MICROS_MAX;
}

void
osfmach3_update_task_info(
	struct task_struct *tsk)
{
	kern_return_t			kr;
	struct task_basic_info		tbi;
	mach_msg_type_number_t		count;

	if (tsk->osfmach3.task == &init_osfmach3_task ||
	    tsk->osfmach3.task == NULL ||
	    !MACH_PORT_VALID(tsk->osfmach3.task->mach_task_port))
		return;

	count = TASK_BASIC_INFO_COUNT;
	kr = task_info(tsk->osfmach3.task->mach_task_port,
		       TASK_BASIC_INFO,
		       (task_info_t) &tbi,
		       &count);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(2, kr,
				    ("osfmach3_update_task_info(%p): "
				     "task_info(0x%x, TASK_BASIC_INFO)",
				     tsk, tsk->osfmach3.task->mach_task_port));
		}
		return;
	}
	if (tsk->mm != &init_mm) {
		tsk->mm->rss = tbi.resident_size >> PAGE_SHIFT;	/* XXX */
	}
#ifdef	PPC
	/*
	 * In flush_thread(), we allocate this amount of memory to
	 * prevent users from using it (it's not available with the
	 * Linux/PPC kernel and we want to be binary compatible).
	 */
	if (tbi.virtual_size > (VM_MAX_ADDRESS - TASK_SIZE))
		tbi.virtual_size -= VM_MAX_ADDRESS - TASK_SIZE;
#endif	/* PPC */
	tsk->swap_cnt = tbi.virtual_size;	/* XXX */

	osfmach3_update_thread_info(tsk);
}

void
osfmach3_fork_resume(
	struct task_struct	*p,
	unsigned long		clone_flags)
{
	kern_return_t	kr;

	if (clone_flags & CLONING_KERNEL) {
		/*
		 * Creating a "kernel thread": we forked a new server cthread,
		 * which doesn't need us to get resumed...
		 */
	} else if (clone_flags & CLONING_INIT) {
		/*
		 * Creating the task for the "init" process.
		 * We've created an empty task and its state hasn't been
		 * initialized yet, so don't resume the thread.
		 */
		ASSERT(p == task[smp_num_cpus]);
	} else if (p->p_opptr != current) {
		/*
		 * It's an asynchronous fork: we're forking a thread
		 * for another task, so we haven't actually created 
		 * a new Mach thread. No point in resuming it then.
		 */
	} else {
		kr = thread_resume(p->osfmach3.thread->mach_thread_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_fork_resume(p=%p): "
				     "thread_resume(0x%x)",
				     p,
				     p->osfmach3.thread->mach_thread_port));
			panic("can't resume thread");
		}
	}
}

void
osfmach3_fork_cleanup(
	struct task_struct	*p,
	unsigned long		clone_flags)
{
	osfmach3_cleanup_task(p);
}

void
osfmach3_set_priority(
	struct task_struct *tsk)
{
	long				mach_priority;
	long				linux_priority;
	long				linux_max_pri, linux_min_pri;
	extern mach_port_t		default_processor_set;
	kern_return_t			kr;

	if (tsk->osfmach3.thread->mach_thread_port == MACH_PORT_NULL) {
		/* no Mach thread yet... */
		return;
	}

	switch (tsk->policy) {
	    case SCHED_FIFO:
	    case SCHED_RR:
		linux_max_pri = 99;
		linux_min_pri = 1;
		break;
	    case SCHED_OTHER:
		/* XXX these is how sys_setpriority() computes them... */
		linux_max_pri = (20 * DEF_PRIORITY + 10) / 20 + DEF_PRIORITY;
		linux_min_pri = 2 * DEF_PRIORITY - linux_max_pri;
		break;
	    default:
		printk("osfmach3_set_priority: unknown policy %ld\n",
		       tsk->policy);
		return;
	}
	linux_priority = tsk->priority;
	if (linux_priority > linux_max_pri)
		linux_priority = linux_max_pri;
	else if (linux_priority < linux_min_pri)
		linux_priority = linux_min_pri;

	/*
	 * Linux priorities go from "linux_min_pri" to "linux_max_pri".
	 * Mach priorities go from BASEPRI_MINPRI-1 to BASEPRI_USER.
	 */
	mach_priority = (((linux_priority - linux_min_pri) *
			  (BASEPRI_MINPRI-1 - BASEPRI_USER))
			 / (linux_max_pri - linux_min_pri)) + BASEPRI_USER;

#if 0
	printk("osfmach3_set_priority: linux=%ld mach=%ld\n",
	       tsk->priority, mach_priority);
#endif
	
	/*
	 * Set the scheduling policy to time-sharing.
	 */
	switch (tsk->policy) {
	    case SCHED_FIFO: {
		policy_fifo_base_data_t		ff_base;
		policy_fifo_limit_data_t	ff_limit;

		ff_base.base_priority = mach_priority;
		ff_limit.max_priority = BASEPRI_USER;
		kr = thread_set_policy(tsk->osfmach3.thread->mach_thread_port,
				       default_processor_set,
				       POLICY_FIFO,
				       (policy_base_t) &ff_base,
				       POLICY_FIFO_BASE_COUNT,
				       (policy_base_t) &ff_limit,
				       POLICY_FIFO_LIMIT_COUNT);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_set_priority: "
				     "thread_set_policy(FIFO,%d,%d)",
				     ff_base.base_priority,
				     ff_limit.max_priority));
		}
		break;
	    }
	    case SCHED_RR: {
		policy_rr_base_data_t	rr_base;
		policy_rr_limit_data_t	rr_limit;

		rr_base.base_priority = mach_priority;
		rr_limit.max_priority = BASEPRI_USER;
		kr = thread_set_policy(tsk->osfmach3.thread->mach_thread_port,
				       default_processor_set,
				       POLICY_RR,
				       (policy_base_t) &rr_base,
				       POLICY_RR_BASE_COUNT,
				       (policy_base_t) &rr_limit,
				       POLICY_RR_LIMIT_COUNT);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_set_priority: "
				     "thread_set_policy(RR,%d,%d)",
				     rr_base.base_priority,
				     rr_limit.max_priority));
		}
		break;
	    }
	    case SCHED_OTHER: {
		policy_timeshare_base_data_t	ts_base;
		policy_timeshare_limit_data_t	ts_limit;

		ts_base.base_priority = mach_priority;
		ts_limit.max_priority = BASEPRI_USER;
		kr = thread_set_policy(tsk->osfmach3.thread->mach_thread_port,
				       default_processor_set,
				       POLICY_TIMESHARE,
				       (policy_base_t) &ts_base,
				       POLICY_TIMESHARE_BASE_COUNT,
				       (policy_base_t) &ts_limit,
				       POLICY_TIMESHARE_LIMIT_COUNT);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_set_priority: "
				     "thread_set_policy(TIMESHARE,%d,%d)",
				     ts_base.base_priority,
				     ts_limit.max_priority));
		}
		break;
	    }
	}
}

pid_t
kernel_thread(
	int (*fn)(void *),
	void *arg,
	unsigned long flags)
{
	int ret;
	void **args;		/* fn, arg, task */

	/*
	 * Create a new kernel "thread" structure: a new task_struct in fact.
	 * CAUTION: we don't need the registers in this case, so we use
	 * the regs argument to pass the arguments. do_fork() and friends
	 * should deal with that when the CLONING_KERNEL flag is raised.
	 */
	args = (void **) kmalloc(3 * sizeof (void *), GFP_KERNEL);
	if (!args) {
		printk("kernel_thread(%p,%p): couldn't allocate args struct.\n",
		       fn, arg);
		return -ENOMEM;
	}

	args[0] = (void *) fn;
	args[1] = arg;
	ret = do_fork(flags | CLONE_VM | CLONING_KERNEL, 0,
		      (struct pt_regs *) args);
	if (ret < 0) {
		printk("kernel_thread(%p,%p): do_fork returned %d.\n",
		       fn, arg, ret);
		kfree(args);
	}

	return (pid_t) ret;
}

void
osfmach3_synchronize_exit(
	struct task_struct *tsk)
{
	ASSERT(tsk->osfmach3.thread->mach_thread_count > 0);
	tsk->osfmach3.thread->mach_thread_count--;
	while (tsk->osfmach3.thread->mach_thread_count > 0) {
		osfmach3_yield();
	}
}
