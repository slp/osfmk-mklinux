/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Machine-independent code to handle traps (system calls and exceptions).
 */
#include <linux/autoconf.h>

#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/exception.h>
#include <mach/thread_status.h>
#include <mach/mach_port.h>
#include <mach/mach_host.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#include <mach/message.h>
#include <mach/exc_server.h>
#include <mach/exc_user.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/parent_server.h>
#include <osfmach3/assert.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/serv_port.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/sys.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/interrupt.h>

#include <asm/ptrace.h>

extern void ux_server_add_port(mach_port_t trap_port);
extern void ux_server_destroy_port(mach_port_t trap_port);

extern boolean_t in_kernel;
extern boolean_t use_activations;
extern mach_port_t exc_subsystem_port;

extern boolean_t trace_user_mach_threads;

mach_port_t		parent_server_syscall_port;
exception_behavior_t	parent_server_syscall_behavior;
thread_state_flavor_t	parent_server_syscall_flavor;

exception_behavior_t	thread_exception_behavior;
thread_state_flavor_t	thread_exception_flavor;

mach_port_t		server_exception_port = MACH_PORT_NULL;
boolean_t		server_exception_in_progress = FALSE;
mach_port_t		user_trap_port = MACH_PORT_NULL;
unsigned long		user_trap_port_refs = 0;


/*
 * This message server catches server exceptions. It runs in a dedicated thread.
 */
void *
server_exception_catcher(
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

	cthread_set_name(cthread_self(), "server exc catcher");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	kr = vm_allocate(mach_task_self(),
			 (vm_address_t *) &msg_buffer_1,
			 2 * sizeof *msg_buffer_1,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("server_exception_catcher: vm_allocate"));
		panic("server_exception_catcher");
	}

	msg_buffer_2 = msg_buffer_1 + 1;
	request = &msg_buffer_1->hdr;
	reply = &msg_buffer_2->death_pill;

	do {
		kr = mach_msg(request, MACH_RCV_MSG,
			      0, sizeof *msg_buffer_1,
			      server_exception_port,
			      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		if (kr != MACH_MSG_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("server_exception_catcher: mach_msg"));
			panic("server_exception_catcher: receive");
		}

		if (exc_server(request, &reply->Head)) {}
		else {
			printk("server_exception_catcher: invalid message"
			       "(id = %d = 0x%x)\n",
			       request->msgh_id, request->msgh_id);
		}
		panic("server_exception_catcher: what now ?");
	} while (1);
       
	cthread_detach(cthread_self());
	cthread_exit((void *) 0);
	/*NOTREACHED*/
	return (void *) 0;
}

/*
 * This message server catches user task exceptions. Most user exceptions
 * will be received on the thread exception port. This server servers
 * only exceptions from unknown threads or from external debuggers.
 * It runs in a dedicated thread.
 */
void *
task_exception_catcher(
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

	cthread_set_name(cthread_self(), "task exc catcher");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

	kr = vm_allocate(mach_task_self(),
			 (vm_address_t *) &msg_buffer_1,
			 2 * sizeof *msg_buffer_1,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("task_exception_catcher: vm_allocate"));
		panic("task_exception_catcher");
	}

	msg_buffer_2 = msg_buffer_1 + 1;
	request = &msg_buffer_1->hdr;
	reply = &msg_buffer_2->death_pill;

	do {
		uniproc_exit();
		kr = mach_msg(request, MACH_RCV_MSG,
			      0, sizeof *msg_buffer_1,
			      user_trap_port,
			      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		if (kr != MACH_MSG_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("task_exception_catcher: mach_msg"));
			panic("task_exception_catcher: receive");
		}
		uniproc_enter();

		if (exc_server(request, &reply->Head)) {}
		else {
			printk("trap_exception_catcher: invalid message"
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
				panic("task_exception_catcher: send");
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
osfmach3_trap_syscall_self(
	boolean_t		catch_them,
	exception_behavior_t	behavior,
	thread_state_flavor_t	flavor)
{
	kern_return_t		kr;
	exception_mask_t	mask;
	mach_port_t		syscall_port;
	exception_behavior_t	syscall_behavior;
	thread_state_flavor_t	syscall_flavor;

	if (current != &init_task)
		panic("osfmach3_trap_syscall_self: "
		      "current=0x%p is not init_task\n",
		      current);

	if (! parent_server)
		return;
	
	/*
	 * Change the way the server syscalls are handled.
	 */
	if (catch_them) {
		/* catch our own syscalls */
		syscall_port = current->osfmach3.thread->mach_trap_port;
		syscall_behavior = behavior;
		syscall_flavor = flavor;
	} else {
		/* let our syscalls go to the parent server */
		syscall_port = parent_server_syscall_port;
		syscall_behavior = parent_server_syscall_behavior;
		syscall_flavor = parent_server_syscall_flavor;
	}
	mask = parent_server_syscall_exc_mask();

	kr = task_set_exception_ports(current->osfmach3.task->mach_task_port,
				      mask,
				      syscall_port,
				      syscall_behavior,
				      syscall_flavor);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_syscall_self(catch_them=%d): "
			     "thread_set_exception_ports",
			     catch_them));
		panic("osfmach3_trap_syscall_self: can't set exception port");
	}

}

void
osfmach3_trap_setup_task(
	struct task_struct	*task,
	exception_behavior_t	behavior,
	thread_state_flavor_t	flavor)
{
	kern_return_t		kr;
	mach_port_t		trap_port_name, trap_port;
	mach_port_t		task_port, thread_port;
	exception_mask_t	mask;

	if (task == &init_task) 
		return;

	task_port = task->osfmach3.task->mach_task_port;
	thread_port = task->osfmach3.thread->mach_thread_port;

	if (task->pid == 1) {
#if 0
		/*
		 * Remove the boostrap port for init.
		 */
		kr = task_set_bootstrap_port(task_port, MACH_PORT_NULL);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_setup_task: "
				     "task_set_bootstrap_port"));
			panic("osfmach3_trap_setup_task: "
			      "can't unset bootstrap port");
		}
#endif

		/*
		 * For the first process, setup a task exception port
		 * for all exceptions. These exception will be
		 * directed to the global "user_trap_port" and will
		 * allow us to detect threads created by user programs
		 * directly (using Mach system calls).
		 * This exception port will be inherited by the
		 * other tasks on task_create().
		 */
		kr = task_set_exception_ports(task_port,
					      (EXC_MASK_ALL &
					       ~EXC_MASK_RPC_ALERT),
					      user_trap_port,
					      EXCEPTION_STATE_IDENTITY,
					      flavor);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_trap_setup_task: "
				     "task_set_exception_ports"));
			panic("osfmach3_trap_setup_task: "
			      "can't set global user task exception ports");
		}
	}

	trap_port_name = ((mach_port_t) task) + 1;
	if (use_activations) {
		kr = serv_port_allocate_subsystem(exc_subsystem_port,
						  &trap_port,
						  (void *) trap_port_name);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_trap_setup_task: "
				     "serv_port_allocate_subsystem(%x)",
				     trap_port_name));
			panic("osfmach3_trap_setup_task: "
			      "can't allocate exception port");
		}
	} else {
		kr = serv_port_allocate_name(&trap_port,
					     (void *) trap_port_name);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_trap_setup_task: "
				     "serv_port_allocate_name(%x)",
				     trap_port_name));
			panic("osfmach3_trap_setup_task: "
			      "can't allocate exception port");
		}
	}
	task->osfmach3.thread->mach_trap_port = trap_port;
	task->osfmach3.thread->mach_trap_port_srights = 0;

	kr = mach_port_insert_right(mach_task_self(),
				    trap_port,
				    trap_port,
				    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_setup_task: "
			     "mach_port_insert_right"));
		panic("osfmach3_trap_setup_task: can't insert send right");
	}

	if (thread_port != MACH_PORT_NULL) {
		mask = EXC_MASK_ALL & ~EXC_MASK_RPC_ALERT;
		if (suser() || task->osfmach3.task->mach_aware) {
			/* let Mach syscalls go to Mach */
			mask &= ~EXC_MASK_MACH_SYSCALL;
			/* the exception port is inherited from the parent */
		}

		kr = thread_set_exception_ports(thread_port,
						mask,
						trap_port,
						behavior,
						flavor);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_trap_setup_task: "
				     "thread_set_exception_ports"));
			panic("osfmach3_trap_setup_task: "
			      "can't set user thread exception ports");
		}
	} else {
		/* we'll set the exception ports later... maybe */
	}

	/*
	 * Add the exception port to the port set.
	 * The exceptions from the server task itself go to 
	 * the "server_exception_catcher" thread.
	 */
	ux_server_add_port(trap_port);
}

void
osfmach3_changed_identity(
	struct task_struct *tsk)
{
	mach_port_t	trap_port;
	kern_return_t	kr;

	if (suser()) {
		trap_port = mach_host_self();
	} else {
		trap_port = tsk->osfmach3.thread->mach_trap_port;
	}
	kr = thread_set_exception_ports(tsk->osfmach3.thread->mach_thread_port,
					EXC_MASK_MACH_SYSCALL,
					trap_port,
					thread_exception_behavior,
					thread_exception_flavor);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_changed_uid: "
			     "thread_set_exception_ports"));
		printk("osfmach3_changed_uid: can't set exception port.\n");
	}
}

void
osfmach3_trap_mach_aware(
	struct task_struct	*task,
	boolean_t		mach_aware,
	exception_behavior_t	behavior,
	thread_state_flavor_t	flavor)
{
	kern_return_t	kr;
	mach_port_t	trap_port;

	if (task->osfmach3.task->mach_aware == mach_aware)
		return;

	task->osfmach3.task->mach_aware = TRUE;
	if (mach_aware) {
		/*
		 * Enable Mach privilege for this task.
		 */
		trap_port = mach_host_self();
		printk("Granting Mach access privileges to process %d (%s)\n",
		       task->pid, task->comm);
	} else {
		/*
		 * Revoke Mach privilege for this task.
		 */
		trap_port = task->osfmach3.thread->mach_trap_port;
		printk("Revoking Mach access privileges from process %d (%s)\n",
		       task->pid, task->comm);
	}
	kr = thread_set_exception_ports(task->osfmach3.thread->mach_thread_port,
					EXC_MASK_MACH_SYSCALL,
					trap_port,
					behavior,
					flavor);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_mach_aware: "
			     "thread_set_exception_ports"));
		panic("osfmach3_trap_mach_aware: can't set exception port");
	}
}

void
osfmach3_trap_terminate_task(
	struct task_struct	*task)
{
	mach_port_t	trap_port;
	kern_return_t	kr;

	trap_port = task->osfmach3.thread->mach_trap_port;

	/*
	 * Get rid of the extra send right we got from the exception
	 * message.
	 */
	kr = mach_port_deallocate(mach_task_self(), trap_port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(2, kr,
			    ("mach_trap_terminate_task: "
			     "mach_port_deallocate(0x%x)",
			     trap_port));
	}

	/*
	 * Destroy the receive right.
	 */
	ux_server_destroy_port(trap_port);
}

kern_return_t
catch_exception_raise(
	mach_port_t			port,
	mach_port_t			thread_port,
	mach_port_t			task_port,
	exception_type_t		exc_type,
	exception_data_t		code,
	mach_msg_type_number_t		code_count)
{
	panic("catch_exception_raise");
}

kern_return_t
catch_exception_raise_state_identity(
	mach_port_t			port,
	mach_port_t			thread_port,
	mach_port_t			task_port,
	exception_type_t		exc_type,
	exception_data_t		code,
	mach_msg_type_number_t		code_count,
	int				*flavor,
	thread_state_t			in_state,
	mach_msg_type_number_t		icnt,
	thread_state_t			out_state,
	mach_msg_type_number_t 		*ocnt)
{
	int i;
	int pid;
	kern_return_t kr;
	struct task_struct *parent;
	mach_port_t trap_port;

	parent = NULL;
	for (i = 1; i < NR_TASKS; i++) {
		if (task[i]) {
			if (task[i]->osfmach3.thread->mach_thread_port ==
			    thread_port) {
				break;
			}
			if (parent == NULL &&
			    task[i]->osfmach3.task->mach_task_port == 
			    task_port) {
				if (task[i]->osfmach3.thread->mach_thread_count
				    > 0 ) {
					parent = task[i];
					/* take a ref so it won't exit */
					parent->osfmach3.thread->mach_thread_count++;
				}
			}
		}
	}
	if (i < NR_TASKS) {
		/*
		 * An existing thread: it's supposed to have a thread exception
		 * port, so if we've received an exception on the task port,
		 * it must come from an exception forwarded to an external
		 * debugger.
		 * Signal to the thread processing the original exception
		 * that the debugger hasn't processed the exception by itself
		 * but expects us to process it.
		 */
		task[i]->osfmach3.thread->exception_completed = FALSE;
		 
		*ocnt = icnt;
		memcpy(out_state, in_state, icnt * sizeof (int));

		/* release our extra ref on the "parent" Linux task */
		if (parent) {
			parent->osfmach3.thread->mach_thread_count--;
		}

		return KERN_SUCCESS;
	}

	if (parent == NULL) {
		/*
		 * This thread belongs to an unknown task. We can't integrate
		 * it to a Linux task so reject it.
		 */
		printk("catch_exception_raise_state_identity(0x%x,0x%x): "
		       "can't accept thread of unkown task\n",
		       thread_port, task_port);

		return KERN_FAILURE;
	}

	/*
	 * A new thread !
	 */
	if (trace_user_mach_threads) {
		printk("Detected new Mach thread 0x%x of Mach task 0x%x "
		       "[P%d %s]\n",
		       thread_port, task_port,
		       parent->pid, parent->comm);
	}

	/* asynchronous fork of the "parent" thread into the new thread */
	pid = osfmach3_do_fork(parent,
			       (SIGCHLD | 
				CLONE_VM | CLONE_FS | CLONE_FILES |
				CLONE_SIGHAND | CLONE_PID),
			       thread_port, NULL);

	/* release our reference on the parent thread */
	parent->osfmach3.thread->mach_thread_count--;

	if (pid < 0) {
		printk("catch_exception_raise_state_identity(0x%x,0x%x): "
		       "do_fork returned %d.\n",
		       thread_port, task_port, pid);

		return KERN_FAILURE;
	}

	for (i = 0; i < NR_TASKS; i++) {
		if (task[i] &&
		    task[i]->osfmach3.thread->mach_thread_port == thread_port)
			break;
	}
	if (i == NR_TASKS) {
		printk("catch_exception_raise_state(0x%x,0x%x): "
		       "can't find new thread (pid %d) after fork!\n",
		       thread_port, task_port, pid);
		return KERN_FAILURE;
	}
	trap_port = task[i]->osfmach3.thread->mach_trap_port;

	/*
	 * Restore the state to what it was before the exception.
	 */
	kr = osfmach3_trap_unwind(trap_port,
				  exc_type, code, code_count, flavor,
				  in_state, icnt,
				  out_state, ocnt);
	return kr;
}

void
osfmach3_trap_init(
	exception_behavior_t	behavior,
	thread_state_flavor_t	flavor)
{
	kern_return_t		kr;
	mach_port_t		trap_port_name, trap_port;
	exception_mask_t	mask;

	thread_exception_behavior = behavior;
	thread_exception_flavor = flavor;

	init_task.osfmach3.task->mach_task_port = mach_task_self();

	trap_port_name = ((mach_port_t) &init_task) + 1;

	kr = serv_port_allocate_name(&trap_port,
				     (void *) trap_port_name);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_init: "
			     "serv_port_allocate_name(%x)",
			     trap_port_name));
		panic("osfmach3_trap_init: "
		      "can't allocate exception port");
	}
	init_task.osfmach3.thread->mach_trap_port = trap_port;

	kr = mach_port_insert_right(mach_task_self(),
				    trap_port,
				    trap_port,
				    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_init: "
			     "mach_port_insert_right"));
		panic("osfmach3_trap_init: can't insert send right");
	}

	mask = EXC_MASK_ALL & ~EXC_MASK_RPC_ALERT;
	if (parent_server) {
		exception_mask_t		syscall_exc_mask;
		exception_mask_t		old_exc_mask;
		mach_msg_type_number_t		old_exc_count;
		mach_port_t			old_exc_port;
		exception_behavior_t		old_exc_behavior;
		thread_state_flavor_t		old_exc_flavor;

		/*
		 * Don't catch syscall exceptions that are directed to
		 * the parent server. But save the port, behavior and flavor
		 * to be able to restore them later.
		 */
		syscall_exc_mask = parent_server_syscall_exc_mask();
		old_exc_count = 1;
		kr = task_get_exception_ports(mach_task_self(),
					      syscall_exc_mask,
					      &old_exc_mask,
					      &old_exc_count,
					      &old_exc_port,
					      &old_exc_behavior,
					      &old_exc_flavor);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("osfmach3_trap_init(FIRST_TASK): "
				     "task_get_exception_ports(mask=0x%x)",
				     syscall_exc_mask));
			panic("can't get syscall exc port (parent server)");
		}
		if (old_exc_count == 1) {
			parent_server_syscall_port = old_exc_port;
			parent_server_syscall_behavior = old_exc_behavior;
			parent_server_syscall_flavor = old_exc_flavor;
		} else {
			printk("osfmach3_trap_init: "
			       "couldn't get our syscall exc port");
		}

		mask &= ~syscall_exc_mask;
		/* let breakpoints go to the debugger (if any) */
		mask &= ~EXC_MASK_BREAKPOINT;
		/* let Mach syscalls go to Mach */
		mask &= ~EXC_MASK_MACH_SYSCALL;
	}
	kr = task_set_exception_ports(mach_task_self(),
				      mask,
				      trap_port,
				      behavior,
				      flavor);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_init: "
			     "task_set_exception_ports"));
		panic("osfmach3_trap_init: "
		      "can't set server's task exception ports");
	}
#if 0	/* obsolete */
	if (parent_server) {
		/*
		 * Hide the EXC_BAD_INSTRUCTION exceptions to avoid 
		 * interferences from the parent_server when we do
		 * syscalls to ourselves (see start_kernel() and init()).
		 */
		kr = thread_set_exception_ports(mach_thread_self(),
						EXC_MASK_BAD_INSTRUCTION,
						MACH_PORT_NULL,
						behavior,
						flavor);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_init: "
				     "thread_set_exception_ports"));
			panic("can't unset thread exception port");
		}
	}
#endif
	ASSERT(server_exception_port == MACH_PORT_NULL);
	server_exception_port = trap_port;
	server_thread_start(server_exception_catcher, (void *) 0);

	/*
	 * Create a global exception port for user tasks to detect
	 * new user threads.
	 */
	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_RECEIVE,
				&user_trap_port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_init: "
			     "mach_port_allocate()"));
		panic("osfmach3_trap_init: "
		      "can't allocate user trap port");
	}

	kr = mach_port_insert_right(mach_task_self(),
				    user_trap_port,
				    user_trap_port,
				    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("osfmach3_trap_init: "
			     "mach_port_insert_right"));
		panic("osfmach3_trap_init: can't insert send right "
		      "for user trap port");
	}

	server_thread_start(task_exception_catcher, (void *) 0);
}

void
osfmach3_trap_forward(
	exception_type_t		exc_type,
	exception_data_t		code,
	mach_msg_type_number_t		code_count,
	int				*flavor,
	thread_state_t			old_state,
	mach_msg_type_number_t		*icnt,
	thread_state_t			new_state,
	mach_msg_type_number_t		*ocnt)
{
	kern_return_t		kr;
	mach_msg_type_number_t	exc_count;
	exception_mask_t	exc_mask;
	mach_port_t		exc_port;
	exception_behavior_t	exc_behavior;
	thread_state_flavor_t	exc_flavor;
	mach_port_t		thread_port, task_port;

	/*
	 * Check if a debugger has changed the task exception port:
	 * if so, forward the exception to the debugger.
	 */
	exc_port = MACH_PORT_NULL;
	task_port = current->osfmach3.task->mach_task_port;
	thread_port = current->osfmach3.thread->mach_thread_port;
	exc_count = 1;
	kr = task_get_exception_ports(task_port,
				      1 << exc_type,
				      &exc_mask,
				      &exc_count,
				      &exc_port,
				      &exc_behavior,
				      &exc_flavor);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_trap_forward: "
			     "task_get_exception_ports(0x%x, 0x%x)",
			     task_port, 1 << exc_type));
		current->osfmach3.thread->exception_completed = FALSE;
		return;
	}
	if (!MACH_PORT_VALID(exc_port)) {
		current->osfmach3.thread->exception_completed = FALSE;
		return;
	}
	ASSERT(exc_mask == 1 << exc_type);
	if (exc_port == user_trap_port) {
		/*
		 * Nothing has changed. Process the exception normally.
		 */
		if (user_trap_port_refs++ >= 0x7000) {
			kr = mach_port_mod_refs(mach_task_self(),
						exc_port,
						MACH_PORT_RIGHT_SEND,
						-0x7000);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("osfmach3_trap_forward: "
					     "mach_port_mod_refs(0x%x)",
					     exc_port));
			}
			user_trap_port_refs -= 0x7000;
		}
		current->osfmach3.thread->exception_completed = FALSE;
		return;
	}
	/*
	 * A debugger has changed the exception port because it expects
	 * to intercept this type of exception. Forward the exception.
	 */
	switch (exc_behavior) {
	    case EXCEPTION_DEFAULT:
		current->osfmach3.thread->exception_completed = TRUE;
		server_thread_blocking(FALSE);
		kr = exception_raise(exc_port,
				     thread_port,
				     task_port,
				     exc_type,
				     code,
				     code_count);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_forward: "
				     "exception_raise(0x%x, 0x%x)",
				     exc_port, exc_type));
			break;
		}
		server_thread_blocking(FALSE);
		kr = thread_get_state(thread_port,
				      *flavor,
				      old_state,
				      icnt);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_forward: "
				     "thread_get_state(0x%x) [1]",
				     thread_port));
		}
		break;

	    case EXCEPTION_STATE:
		if (exc_flavor != *flavor) {
			printk("osfmach3_trap_forward: "
			       "can't forward exception_state - "
			       "got flavor 0x%x, want 0x%x\n",
			       *flavor, exc_flavor);
			current->osfmach3.thread->exception_completed = FALSE;
			break;
		}
		current->osfmach3.thread->exception_completed = TRUE;
		server_thread_blocking(FALSE);
		kr = exception_raise_state(exc_port,
					   exc_type,
					   code,
					   code_count,
					   flavor,
					   old_state,
					   *icnt,
					   new_state,
					   ocnt);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_forward: "
				     "exception_raise_state(0x%x, 0x%x)",
				     exc_port, exc_type));
		}
		break;

	    case EXCEPTION_STATE_IDENTITY:
		if (exc_flavor != *flavor) {
			printk("osfmach3_trap_forward: "
			       "can't forward exception_state_identity - "
			       "got flavor 0x%x, want 0x%x\n",
			       *flavor, exc_flavor);
			current->osfmach3.thread->exception_completed = FALSE;
			break;
		}
		current->osfmach3.thread->exception_completed = TRUE;
		server_thread_blocking(FALSE);
		kr = exception_raise_state_identity(exc_port,
						    thread_port,
						    task_port,
						    exc_type,
						    code,
						    code_count,
						    flavor,
						    old_state,
						    *icnt,
						    new_state,
						    ocnt);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_forward: "
				     "exception_raise_state_identity"
				     "(0x%x, 0x%x)",
				     exc_port, exc_type));
		}
		break;

	    default:
		printk("osfmach3_trap_forward: "
		       "unknown behavior 0x%x for task exception 0x%x\n",
		       exc_behavior, exc_type);
		current->osfmach3.thread->exception_completed = FALSE;
		break;
	}

	if (exc_port != MACH_PORT_NULL) {
		kr = mach_port_deallocate(mach_task_self(), exc_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_trap_forward: "
				     "mach_port_deallocate(0x%x)",
				     exc_port));
		}
	}
}
