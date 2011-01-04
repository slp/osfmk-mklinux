/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/mach_interface.h>
#include <mach/notify_server.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>

#include <linux/sched.h>

mach_port_t osfmach3_notify_port = MACH_PORT_NULL;

boolean_t trace_user_mach_threads = FALSE;

void *
osfmach3_notify_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t			kr;
#define MSG_BUFFER_SIZE	8192
	union request_msg {
		mach_msg_header_t	hdr;
		mig_reply_error_t	death_pill;
		char			space[MSG_BUFFER_SIZE];
	} *msg_buffer_1, *msg_buffer_2;
	mach_msg_header_t		*request;
	mig_reply_error_t		*reply;
	mach_msg_header_t		*tmp;

	cthread_set_name(cthread_self(), "notify thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

	kr = vm_allocate(mach_task_self(),
			 (vm_address_t *) &msg_buffer_1,
			 2 * sizeof *msg_buffer_1,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("osfmach3_notify_thread: vm_allocate"));
		panic("osfmach3_notify_thread");
	}

	msg_buffer_2 = msg_buffer_1 + 1;
	request = &msg_buffer_1->hdr;
	reply = &msg_buffer_2->death_pill;

	do {
		uniproc_exit();
		kr = mach_msg(request, MACH_RCV_MSG,
			      0, sizeof *msg_buffer_1,
			      osfmach3_notify_port,
			      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		if (kr != MACH_MSG_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_notify_thread: mach_msg"));
			panic("osfmach3_notify_thread: receive");
		}
		uniproc_enter();

		if (notify_server(request, &reply->Head)) {}
		else {
			printk("osfmach3_notify_thread: invalid message"
			       "(id = %d = 0x%x)\n",
			       request->msgh_id, request->msgh_id);
		}

		if (reply->Head.msgh_remote_port == MACH_PORT_NULL) {
			/* no reply port, just get another request */
			continue;
		}
		if (!(reply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) &&
		    reply->RetCode == MIG_NO_REPLY) {
			/* deallocate reply port right */
			(void) mach_port_deallocate(mach_task_self(),
						    reply->Head.msgh_remote_port);
			continue;
		}

		/* Send reply to request and receive another */
		uniproc_exit();
		kr = mach_msg(&reply->Head,
			      MACH_SEND_MSG,
			      reply->Head.msgh_size,
			      0,
			      MACH_PORT_NULL,
			      MACH_MSG_TIMEOUT_NONE,
			      MACH_PORT_NULL);
		uniproc_enter();

		if (kr != MACH_MSG_SUCCESS) {
			if (kr == MACH_SEND_INVALID_DEST) {
				/* deallocate reply port right */
				/* XXX should destroy reply msg */
				(void) mach_port_deallocate(mach_task_self(),
							    reply->Head.msgh_remote_port);
			} else {
				MACH3_DEBUG(0, kr, ("mach_msg"));
				panic("osfmach3_notify_thread: send");
			}
		}

		tmp = request;
		request = (mach_msg_header_t *) reply;
		reply = (mig_reply_error_t *) tmp;

	} while (1);
       
	cthread_detach(cthread_self());
	cthread_exit((void *) 0);
	/*NOTREACHED*/
	return (void *) 0;
}

void
osfmach3_notify_init(void)
{
	kern_return_t	kr;

	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_RECEIVE,
				&osfmach3_notify_port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_notify_init: mach_port_allocate"));
		panic("osfmach3_notify_init: can't allocate notification port");
	}

	kr = mach_port_insert_right(mach_task_self(),
				    osfmach3_notify_port,
				    osfmach3_notify_port,
				    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_notify_init: mach_port_insert_right"));
		panic("osfmach3_notify_init: can't insert send right "
		      "for notofication port");
	}

	server_thread_start(osfmach3_notify_thread, (void *) 0);
}

void
osfmach3_notify_register(
	struct task_struct *tsk)
{
	kern_return_t	kr;
	mach_port_t	previous, thread_port, task_port;

	task_port = tsk->osfmach3.task->mach_task_port;
	thread_port = tsk->osfmach3.thread->mach_thread_port;
	if (thread_port == MACH_PORT_NULL)
		return;

	kr = mach_port_request_notification(mach_task_self(),
					    task_port,
					    MACH_NOTIFY_DEAD_NAME, 1,
					    osfmach3_notify_port,
					    MACH_MSG_TYPE_MAKE_SEND_ONCE,
					    &previous);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_notify_register(%p): "
			     "mach_port_request_notification(task=0x%x)",
			     tsk, task_port));
		previous = MACH_PORT_NULL;
	}
	if (MACH_PORT_VALID(previous)) {
		kr = mach_port_deallocate(mach_task_self(),
					  previous);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_notify_register(%p): "
				     "mach_port_deallocate(0x%x)",
				     tsk, previous));
		}
	}

	kr = mach_port_request_notification(mach_task_self(),
					    thread_port,
					    MACH_NOTIFY_DEAD_NAME, 1,
					    osfmach3_notify_port,
					    MACH_MSG_TYPE_MAKE_SEND_ONCE,
					    &previous);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_notify_register(%p): "
			     "mach_port_request_notification(thread=0x%x)",
			     tsk, thread_port));
		return;
	}
	if (MACH_PORT_VALID(previous)) {
		kr = mach_port_deallocate(mach_task_self(),
					  previous);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_notify_register(%p): "
				     "mach_port_deallocate(0x%x)",
				     tsk, previous));
		}
	}
}
					    
void
osfmach3_notify_deregister(
	struct task_struct *tsk)
{
	kern_return_t	kr;
	mach_port_t	previous, thread_port, task_port;

	task_port = tsk->osfmach3.task->mach_task_port;
	thread_port = tsk->osfmach3.thread->mach_thread_port;
	if (thread_port == MACH_PORT_NULL)
		return;

	if (tsk->osfmach3.task->mach_task_count == 1) {
		/*
		 * This is the last thread of the task: deregister
		 * the task notification.
		 */
		kr = mach_port_request_notification(mach_task_self(),
						    task_port,
						    MACH_NOTIFY_DEAD_NAME, 1,
						    MACH_PORT_NULL,
						    MACH_MSG_TYPE_MAKE_SEND_ONCE,
						    &previous);
		if (kr != KERN_SUCCESS) {
			if (kr != KERN_INVALID_ARGUMENT) {
				MACH3_DEBUG(1, kr,
					    ("osfmach3_notify_deregister(%p): "
					     "mach_port_request_notification"
					     "(task=0x%x)",
					     tsk, task_port));
			}
			previous = MACH_PORT_NULL;
		}
		if (MACH_PORT_VALID(previous)) {
			kr = mach_port_deallocate(mach_task_self(),
						  previous);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("osfmach3_notify_deregister(%p): "
					     "mach_port_deallocate(0x%x)",
					     tsk, previous));
			}
		}
	}

	kr = mach_port_request_notification(mach_task_self(),
					    thread_port,
					    MACH_NOTIFY_DEAD_NAME, 1,
					    MACH_PORT_NULL,
					    MACH_MSG_TYPE_MAKE_SEND_ONCE,
					    &previous);
	if (kr != KERN_SUCCESS) {
		if (kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_notify_deregister(%p): "
				     "mach_port_request_notification(0x%x)",
				     tsk, thread_port));
		}
		return;
	}
	if (MACH_PORT_VALID(previous)) {
		kr = mach_port_deallocate(mach_task_self(),
					  previous);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_notify_deregister(%p): "
				     "mach_port_deallocate(0x%x)",
				     tsk, previous));
		}
	}

}

/*
 * Dead name notification.
 */
kern_return_t
do_mach_notify_dead_name(
	mach_port_t	notify,
	mach_port_t	name)
{
	int i;
	struct task_struct *tsk;
	boolean_t mach_task_terminated;

	for (i = 0; i < NR_TASKS; i++) {
		if (task[i] &&
		    (task[i]->osfmach3.task->mach_task_port == name ||
		     task[i]->osfmach3.thread->mach_thread_port == name)) {
			break;
		}
	}
	if (i == NR_TASKS) {
		printk("do_mach_notify_dead_name(0x%x): "
		       "can't locate dead Linux task\n",
		       name);
		return KERN_SUCCESS;
	}

	tsk = task[i];
	mach_task_terminated = (name == tsk->osfmach3.task->mach_task_port);
	if (trace_user_mach_threads) {
		printk("Mach %s 0x%x for [P%d %s] terminated: "
		       "killing Linux process.\n",
		       mach_task_terminated ? "task" : "thread",
		       name, tsk->pid, tsk->comm);
	}
	if (mach_task_terminated) {
		/*
		 * Terminate all the Linux tasks (clones) of this Mach task.
		 */
		for (i = 0; i < NR_TASKS; i++) {
			if (task[i] &&
			    task[i]->osfmach3.task->mach_task_port == name) {
				force_sig(SIGKILL, task[i]);
			}
		}
	} else {
		/*
		 * Terminate only the Linux task (clone) of this Mach thread.
		 */
		force_sig(SIGKILL, tsk);
	}

	return KERN_SUCCESS;
}

/*
 * Port deleted notification.
 */
kern_return_t
do_mach_notify_port_deleted(
	mach_port_t	notify,
	mach_port_t	name)
{
	printk("do_mach_notify_port_deleted: name=0x%x ???\n", name);
	(void) mach_port_deallocate(mach_task_self(), name);
	return KERN_FAILURE;
}

/*
 * No senders notification.
 */
kern_return_t
do_mach_notify_no_senders(
	mach_port_t		notify,
	mach_port_mscount_t	mscount)
{
	printk("do_mach_notify_no_senders: mscount=0x%x ???\n", mscount);
	return KERN_FAILURE;
}

/*
 * Port destroyed notification.
 */
kern_return_t
do_mach_notify_port_destroyed(
	mach_port_t	notify,
	mach_port_t	name)
{
	printk("do_mach_notify_port_destroyed: name=0x%x ???\n", name);
	(void) mach_port_deallocate(mach_task_self(), name);
	return KERN_FAILURE;
}

/*
 * Send once notification.
 */
kern_return_t
do_mach_notify_send_once(
	mach_port_t	notify)
{
	/*
	 * We get one of those each time we call osfmach3_notify_deregister()
	 * because the micro-kernel sends us the unused send-once right.
	 * That's annoying...
	 */
#if 0
	printk("do_mach_notify_send_once: ???\n");
#endif
	return KERN_FAILURE;
}
