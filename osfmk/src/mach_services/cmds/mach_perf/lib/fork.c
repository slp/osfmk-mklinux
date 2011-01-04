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


#include <mach_perf.h>

boolean_t __in_kernel;
boolean_t __in_kernel_init = FALSE;

mach_port_t bg_comms_port = MACH_PORT_NULL;	/* for use by synthetic.c */

mach_port_t 	mach_fork_child_task;
volatile int	mach_fork_sync;

int		norma_node = NORMA_NODE_NULL;

#define	    FORK_INIT 			0
#define     FORK_PARENT_IDLE		1
#define	    FORK_PARENT_SUSPENDED	2
#define	    FORK_CHILD_READY		3

/* Fork time machine specific actions */

void	(*machine_fork)(mach_port_t 		parent_thread,
			mach_port_t 		child_thread,
			thread_state_t 		state,
			mach_msg_type_number_t	count);

void	(*_atfork_child_routine)(void);

int	is_master_task = 1;	/*
				 * governs global behavior: if master, should
				 * always stay true.
				 */
int	is_child_task = 0;	/*
				 *  temporary set to 1 during fork for child.
				 */

fork_slave(parent_thread)
mach_port_t parent_thread;
{
	thread_state_t 		state;
	mach_msg_type_number_t 	count;
	mach_port_t 		child_thread;
	mach_port_t 		task = mach_task_self();
	mach_port_t 		bootport, child_bootport;
	extern boolean_t 	ipc_inherit_server();

	if (debug > 1) {
		printf("fork_slave()\n");
	}

	while (mach_fork_sync != FORK_PARENT_IDLE)
		thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, 10);
	MACH_CALL( thread_suspend, (parent_thread));
	mach_fork_sync = FORK_PARENT_SUSPENDED;
	/*
	 * Set up inherited items for the child to receive.
	 */
	is_child_task = 1;	/* inherited by child */
	MACH_CALL( task_get_bootstrap_port, (task, &bootport));
	MACH_CALL( mach_port_allocate, (task, MACH_PORT_RIGHT_RECEIVE,
					&child_bootport));
	MACH_CALL( mach_port_insert_right, (task,
					    child_bootport,
					    child_bootport,
					    MACH_MSG_TYPE_MAKE_SEND));
	MACH_CALL( task_set_bootstrap_port, (task, child_bootport));

	if (norma_node != NORMA_NODE_NULL) {
		MACH_CALL( norma_task_create, (mach_task_self(),
					       TRUE, 
					       norma_node, 
					       &mach_fork_child_task));
	} else {
               MACH_CALL( task_create, (mach_task_self(),
                                         (ledger_port_array_t)0, 0, TRUE,
                                         &mach_fork_child_task));
	}
	/*
	 * Clean-up after fork.
	 */
	MACH_CALL( task_set_bootstrap_port, (task, bootport));	
#if 0
	MACH_CALL( mach_port_deallocate, (task, child_bootport)); /* sright */
#endif
	is_child_task = 0;	/* actually, we are the parent */

	if (!standalone) {
	  	/*
		 * OSF/1 server might panic otherwise
		 */

		MACH_CALL( task_set_exception_ports,
			  (mach_fork_child_task,
			   EXC_MASK_ALL,
			   MACH_PORT_NULL,
			   EXCEPTION_DEFAULT,
			   0));
	}

	MACH_CALL( thread_create, (mach_fork_child_task, &child_thread));

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&state,
				  THREAD_STATE_COUNT,
				  TRUE));

	if (debug > 1)
		printf("get state parent\n");
	count = THREAD_STATE_COUNT;
	MACH_CALL( thread_get_state, (parent_thread, THREAD_STATE,
				       state, &count));


	if (machine_fork)
		(*machine_fork)(parent_thread, child_thread, state, count);

	if (debug > 1) {
		register i;
		for (i=0; i<THREAD_STATE_COUNT; i++)
			printf("%x/", state[i]);
		printf("\n");
	}
	if (debug > 1)
		printf("thread_set_state (task %x thread %x\n",
		       mach_fork_child_task,
		       child_thread);
	MACH_CALL( thread_set_state, (child_thread, THREAD_STATE,
			       state, count));

	if (debug > 1) {
	  	register i;
		for (i=0; i<THREAD_STATE_COUNT; i++)
			printf("%x/", state[i]);
		printf("\n");
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)state,
				  THREAD_STATE_COUNT));

	exception_fork(mach_fork_child_task);

	MACH_CALL( thread_resume, (child_thread));
	MACH_CALL( mach_port_deallocate, (mach_task_self(), child_thread));

	/*
	 * Wait for the child to setup and to ask bootstrap ports.
	 * This code is free of race since the child bootport is
	 * only known by the child which we expect a reply from.
	 */

	MACH_CALL (mach_msg_server_once,
		  (&ipc_inherit_server,
		   4096,
		   child_bootport,
		   MACH_MSG_OPTION_NONE));
	MACH_CALL( mach_port_destroy, (task, child_bootport));	  /* rright */

	mach_fork_sync = FORK_CHILD_READY;
	MACH_CALL( thread_resume, (parent_thread));
	while(1)
		thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, 10);
}

mach_port_t
mach_fork()
{
	register i;
	mach_port_t parent_task = mach_task_self();
	mach_port_t parent_thread;
	mach_port_t port;
	mach_thread_t slave;

	if (debug > 1)
		printf("mach_fork()\n");
	mach_fork_sync = FORK_INIT;
	parent_thread = mach_thread_self();
	mach_fork_child_task = 0;
	new_thread(&slave, (vm_offset_t)fork_slave, (vm_offset_t)parent_thread);
	mach_fork_sync = FORK_PARENT_IDLE;
  	while(mach_fork_sync == FORK_PARENT_IDLE)
		continue;
	if (is_child_task) {
		mach_port_t bootport, actual_bootport;

		is_master_task = 0;	/* now, modify the global switch */
		if (_atfork_child_routine)
                        (*_atfork_child_routine)();
		__in_kernel_init = FALSE;
		if (norma_node != NORMA_NODE_NULL) {
			privileged_host_port = MACH_PORT_NULL;
			master_device_port = MACH_PORT_NULL;
			norma_node = NORMA_NODE_NULL;
		}
	   	child_threads();
		/*
		 * Retrieve essential ports from our parent.
		 *
		 * Note: printf doesn't work up until we get printf_port.
		 */
		MACH_CALL( task_get_bootstrap_port, (mach_task_self(),
						     &bootport));
		MACH_CALL( parent_ports, (bootport,
					  &actual_bootport,
					  &printf_port,
					  &prof_cmd_port,
					  &bg_comms_port));

		MACH_CALL( task_set_bootstrap_port, (mach_task_self(),
						     actual_bootport));
		MACH_CALL( mach_port_deallocate, (mach_task_self(),
						  bootport));
		return(MACH_PORT_NULL);
	} else {
		while(mach_fork_sync != FORK_CHILD_READY)
			thread_switch(MACH_PORT_NULL, SWITCH_OPTION_NONE, 0);
		kill_thread(slave);
		return(mach_fork_child_task);
	}
}

/*
 * Acts like bootstrap_ports for a child task wrt us
 */
kern_return_t
do_parent_ports(
	mach_port_t	parent_port,
	mach_port_t	*child_bootstrap_port,	/* bootstrap port */
	mach_port_t	*child_printf_port,	/* printf port */
	mach_port_t	*child_prof_cmd_port,	/* prof cmd port */
	mach_port_t	*child_bg_comms_port)	/* bg comms port */
{
	MACH_CALL( task_get_bootstrap_port, (mach_task_self(),
					     child_bootstrap_port));
	*child_printf_port = printf_port;
	*child_prof_cmd_port = prof_cmd_port;
	*child_bg_comms_port = bg_comms_port;
	return(KERN_SUCCESS);
}

