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
#include <mach/mach_interface.h>

#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/queue.h>
#include <osfmach3/assert.h>
#include <osfmach3/mach3_debug.h>

#include <linux/kernel.h>
#include <linux/sched.h>

#include "serv_callback_server.h"
#include "serv_callback_user.h"

#if	CONFIG_OSFMACH3_DEBUG
#define FAKE_INTR_DEBUG	1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	FAKE_INTR_DEBUG
int fake_intr_debug = 0;
#endif	/* FAKE_INTR_DEBUG */

queue_head_t		fake_interrupt_queue;
struct condition	fake_interrupt_cond;

extern boolean_t	in_kernel;
extern boolean_t	use_activations;

/*
 * Request a fake interrupt to put task "p" back under server control.
 */
void
generate_fake_interrupt(
	struct task_struct	*p)
{
	if (p->osfmach3.thread->in_interrupt_list) {
		/* already in the list */
		return;
	}
	if (p->osfmach3.thread->fake_interrupt_count > 1) {
		/* enough interrupts in progress */
		return;
	}
	if (p->state == TASK_ZOMBIE ||
	    /* PTRACE_KILL makes the process runnable again, so: */
	    p->osfmach3.thread->mach_thread_port == MACH_PORT_NULL) {
		/* task is being terminated */
		return;
	}

#ifdef	FAKE_INTR_DEBUG
	if (fake_intr_debug) {
		printk("generate_fake_interrupt: enqueueing task 0x%p \"%s\"\n",
		       p, p->comm);
	}
#endif	/* FAKE_INTR_DEBUG */

	p->osfmach3.thread->in_interrupt_list = TRUE;
	queue_enter(&fake_interrupt_queue, p,
		    struct task_struct *, osfmach3.thread->interrupt_list);
	condition_signal(&fake_interrupt_cond);
}

/*
 * Cancel a fake interrupt request (on process termination).
 */
void
cancel_fake_interrupt(
	struct task_struct	*p)
{
	if (p->osfmach3.thread->in_fake_interrupt) {
		/*
		 * We're in a fake interrupt.
		 */
		ASSERT(p->state == TASK_ZOMBIE);
		ASSERT(p->osfmach3.thread->fake_interrupt_count > 0);
		p->osfmach3.thread->fake_interrupt_count--;
	}
	while (p->osfmach3.thread->in_interrupt_list ||
	       p->osfmach3.thread->fake_interrupt_count > 0) {
		/*
		 * Another fake interrupt is on its way, but we want 
		 * the current one to be fatal. Wait until the other one
		 * gives up.
		 */
		osfmach3_yield();
	}
}

void *
fake_interrupt_handler(
	void	*trap_port_handle)
{
	struct server_thread_priv_data	priv_data;
	mach_port_t			trap_port;

	cthread_set_name(cthread_self(), "fake intr");
	server_thread_set_priv_data(cthread_self(), &priv_data);
	uniproc_enter();

	trap_port = (mach_port_t) trap_port_handle;

	/*
	 * Set up the jump buf in case the process gets terminated.
	 */
	(void) serv_callback_fake_interrupt(trap_port);

	uniproc_exit();
	cthread_detach(cthread_self());
	cthread_exit((void *) 0);
	/*NOTREACHED*/
	panic("fake_intr_handler: the zombie walks!");
}

/*
 * Interrupt the given task (if possible).
 */
boolean_t
task_fake_interrupt(
	struct task_struct	*p)
{
	kern_return_t	kr;
	boolean_t	suspended;
	mach_port_t	thread_port;

	suspended = FALSE;

	if (p->osfmach3.thread->under_server_control) {
		/*
		 * The task is back under server control, no need to interrupt
		 * it anymore.
		 */
#ifdef	FAKE_INTR_DEBUG
		if (fake_intr_debug) {
			printk("task_fake_interrupt(0x%p): under control\n", p);
		}
#endif	/* FAKE_INTR_DEBUG */
		p->osfmach3.thread->in_interrupt_list = FALSE;
		p->osfmach3.thread->fake_interrupt_count--;
		return TRUE;
	}

	if (p->state == TASK_ZOMBIE) {
		/*
		 * The task is being terminated.
		 */
		p->osfmach3.thread->in_interrupt_list = FALSE;
		p->osfmach3.thread->fake_interrupt_count--;
		return TRUE;
	}

	if (p->osfmach3.thread->fake_interrupt_count > 1) {
		/*
		 * There's another fake interrupt in progress but it
		 * might be already past the signal checkpoint and there
		 * might be new signals to process, so don't give up but
		 * try again later.
		 */
		p->osfmach3.thread->fake_interrupt_count--;
		goto try_again_later;
	}

	thread_port = p->osfmach3.thread->mach_thread_port;
	server_thread_blocking(FALSE);

	/*
	 * Suspend the user thread.
	 */
	kr = thread_suspend(thread_port);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			server_thread_unblocking(FALSE);
			MACH3_DEBUG(1, kr,
				    ("task_fake_interrupt(0x%p): "
				     "thread_suspend(0x%x)",
				     p, thread_port));
			p->osfmach3.thread->in_interrupt_list = FALSE;
			p->osfmach3.thread->fake_interrupt_count--;
			return TRUE;
		}
	}
	suspended = TRUE;

	/*
	 * Abort any Mach system cal in progress.
	 * This should NOT abort an in-transit exception RPC to the server.
	 */
	kr = thread_abort_safely(thread_port);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			/*
			 * The thread can't be interrupted right now.
			 * Put it back in the interrupt queue and
			 * try again later.
			 */
			kr = thread_resume(thread_port);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("task_fake_interrupt(0x%p): "
					     "thread_resume(0x%x)",
					     p, thread_port));
			} else {
				suspended = FALSE;
			}
			server_thread_unblocking(FALSE);
#ifdef	FAKE_INTR_DEBUG
			if (fake_intr_debug) {
				printk("task_fake_interrupt(0x%p): "
				       "not abortable, back to queue\n", p);
			}
#endif	/* FAKE_INTR_DEBUG */
			p->osfmach3.thread->fake_interrupt_count--;
			goto try_again_later;
		}
	}
	server_thread_unblocking(FALSE);

	/*
	 * The user thread is under our control now (suspended and aborted).
	 * Let a ux_server_loop thread do whatever needs to be done to
	 * it (signal, etc...).
	 */
	ASSERT(p->osfmach3.thread->under_server_control == FALSE);
	p->osfmach3.thread->under_server_control = TRUE;
	ASSERT(p->osfmach3.thread->fake_interrupt_count == 1 ||
	       p->osfmach3.thread->fake_interrupt_count == 2);

	/* can't do that before under_server_control is set */
	p->osfmach3.thread->in_interrupt_list = FALSE;

	if (use_activations) {
		/*
		 * We can't use a one-way IPC in this
		 * case, because of the short-circuited RPC scheme.
		 * Using an RPC doesn't help, because it can block
		 * (job control signal or ptrace activity) or even not
		 * return (terminating signal).
		 * Let's launch a new server thread to process the fake
		 * interrupt.
		 */
		server_thread_start(fake_interrupt_handler,
				    (void *)p->osfmach3.thread->mach_trap_port);
		kr = KERN_SUCCESS;
	} else {
		server_thread_blocking(FALSE);
		kr = send_serv_callback_fake_interrupt(
					p->osfmach3.thread->mach_trap_port);
		server_thread_unblocking(FALSE);
	}
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("task_fake_interrupt(0x%p): "
				    "send_serv_callback_fake_interrupt(0x%x)",
				    p, p->osfmach3.thread->mach_trap_port));
		p->osfmach3.thread->fake_interrupt_count--;
		goto resume_after_failure;
	}

	/*
	 * Let serv_callback_fake_interrupt() clear the "in_interrupt_list"
	 * boolean to avoid race conditions with process termination.
	 */
	return TRUE;

    try_again_later:
	p->osfmach3.thread->in_interrupt_list = FALSE;
	generate_fake_interrupt(p);

    resume_after_failure:
	if (suspended) {
		/* resume the user thread */
		kr = thread_resume(p->osfmach3.thread->mach_thread_port);
		if (kr != KERN_SUCCESS) {
			if (kr != MACH_SEND_INVALID_DEST &&
			    kr != KERN_INVALID_ARGUMENT) {
				MACH3_DEBUG(1, kr,
					    ("task_fake_interrupt(0x%p): "
					     "thread_resume(0x%x)",
					     p,
					     p->osfmach3.thread->mach_thread_port));
			}
		}
	}
	return FALSE;
}

void *
fake_interrupt_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	struct task_struct		*p;

	cthread_set_name(cthread_self(), "fake interrupt thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

	for (;;) {
		while (queue_empty(&fake_interrupt_queue)) {
			uniproc_will_exit();
			condition_wait(&fake_interrupt_cond,
				       &uniproc_mutex);
			uniproc_has_entered();
		}
		queue_remove_first(&fake_interrupt_queue, p,
				   struct task_struct *,
				   osfmach3.thread->interrupt_list);
#ifdef	FAKE_INTR_DEBUG
		if (fake_intr_debug) {
			printk("fake_interrupt_thread: dequeued task 0x%p\n",
			       p);
		}
#endif	/* FAKE_INTR_DEBUG */

		p->osfmach3.thread->fake_interrupt_count++;
		if (! task_fake_interrupt(p)) {
			/* failed: yield to avoid looping on failure */
			osfmach3_yield();
		}
	}
}

void
fake_interrupt_init(void)
{
	queue_init(&fake_interrupt_queue);
	condition_init(&fake_interrupt_cond);
	server_thread_start(fake_interrupt_thread, (void *) 0);
}
