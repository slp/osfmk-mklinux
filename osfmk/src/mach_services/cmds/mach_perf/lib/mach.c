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
#include <mach/thread_switch.h>
#include <mach/thread_status.h>

mach_port_t privileged_host_port = MACH_PORT_NULL;
mach_port_t host_port = MACH_PORT_NULL;

mach_port_t master_host_port = MACH_PORT_NULL;
mach_port_t master_device_port = MACH_PORT_NULL;

mach_port_t root_ledger_paged=MACH_PORT_NULL;
mach_port_t root_ledger_wired=MACH_PORT_NULL;
mach_port_t security_port=MACH_PORT_NULL;
security_token_t 	mach_perf_token;

/*
 * Get kernel task port 
 */

task_port_t
get_kernel_task()
{
   	task_port_t *tasks_list;
    	unsigned int tasksCnt;
	task_port_t kernel_task;
	register i;

	tasksCnt = get_tasks(&tasks_list);

	kernel_task = tasks_list[0];
	for (i=1; i < tasksCnt; i++)
		MACH_CALL(mach_port_deallocate, (mach_task_self(),
						 tasks_list[i]));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) tasks_list,
				  sizeof(*tasks_list)*tasksCnt));
	
	return(kernel_task);
}

processor_set_control_port_t
get_processor_set()
{
	mach_port_t proc_set;
    	unsigned int proc_setsCnt;
   	task_port_t *tasks_list;
    	unsigned int tasksCnt;
    	processor_set_name_port_t *proc_set_names;


	/* get processor sets */

	MACH_CALL(host_processor_sets, 
		  (privileged_host_port, &proc_set_names, &proc_setsCnt));

	/* convert processor set 0 name to send right privileged port */

	MACH_CALL(host_processor_set_priv,
		  (privileged_host_port, proc_set_names[0], &proc_set));

        MACH_CALL(vm_deallocate, (mach_task_self(),
                                  (vm_offset_t) proc_set_names,
                                  sizeof(*proc_set_names)*proc_setsCnt));

	return(proc_set);
}

mach_port_t
rename_to_gen_port(old_port)
mach_port_t old_port;
{
	mach_port_t port;
	MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&port));
	MACH_CALL( mach_port_destroy, (mach_task_self(),
				       port));
	MACH_CALL( mach_port_rename, (mach_task_self(),
				      old_port, port));
	return(port);
}

/*
 * Same effect as vm_allocate(), but we want to be sure
 * that the underlying object is not shared with lower 
 * memory. Otherwise vm_deallocate will not free 
 * physical memory
 */
kern_return_t
vm_allocate_temporary(task, buf, size, anywhere)
mach_port_t task;
vm_offset_t *buf;
vm_size_t size;
boolean_t anywhere;
{
	kern_return_t ret;

	if (!anywhere)
		return(vm_allocate(task, buf, size, FALSE));
	ret = vm_allocate(task, buf, size + vm_page_size, TRUE);
	if (ret != KERN_SUCCESS)
		return ret;
	MACH_CALL( vm_deallocate, (task, *buf, size + vm_page_size));
	*buf += vm_page_size;
	return(vm_allocate (task, buf, size, FALSE));
}

privileged_user()
{
	if (privileged_host_port == MACH_PORT_NULL) {
         
		MACH_FUNC(host_port, mach_host_self, ());
        	MACH_CALL(task_get_special_port, (mach_task_self(),
                                       TASK_BOOTSTRAP_PORT,
                                       &bootstrap_port));
        	/*
	         * Ask for the privileged ports
		*/
		MACH_CALL(bootstrap_ports, (bootstrap_port,
					       &privileged_host_port,
					       &master_device_port,
					       &root_ledger_wired,
					       &root_ledger_paged,
					       &security_port));
		get_security_token(&mach_perf_token);
	}
}

/*
 * Get tasks list
 */

int
get_tasks(tasks)
task_port_t **tasks;
{
	processor_set_name_port_t proc_set;
    	unsigned int proc_setsCnt;
    	unsigned int tasksCnt;
    	processor_set_name_port_t *proc_set_names;
	register i;

	/* get processor sets */

	MACH_CALL(host_processor_sets, 
		  (privileged_host_port, &proc_set_names, &proc_setsCnt));

	/* convert processor set 0 name to send right privileged port */

	MACH_CALL(host_processor_set_priv,
		  (privileged_host_port, proc_set_names[0], &proc_set));


	for (i = 0; i < proc_setsCnt; i++) {
		MACH_CALL( mach_port_deallocate, (mach_task_self(),
						  proc_set_names[i]));
	}

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) proc_set_names,
				  sizeof(*proc_set_names)*proc_setsCnt));
	/* get tasks */

	MACH_CALL(processor_set_tasks, 
		  (proc_set, tasks, &tasksCnt));

	MACH_CALL( mach_port_deallocate, (mach_task_self(),
					  proc_set));
	return(tasksCnt);
}

/*
 * Get threads list
 */

int
get_threads(threads)
thread_port_t **threads;
{
	processor_set_name_port_t proc_set;
    	unsigned int proc_setsCnt;
    	unsigned int threadsCnt;
    	processor_set_name_port_t *proc_set_names;
	register i;

	/* get processor sets */

	MACH_CALL(host_processor_sets, 
		  (privileged_host_port, &proc_set_names, &proc_setsCnt));

	/* convert processor set 0 name to send right privileged port */

	MACH_CALL(host_processor_set_priv,
		  (privileged_host_port, proc_set_names[0], &proc_set));


	for (i = 0; i < proc_setsCnt; i++) {
		MACH_CALL( mach_port_deallocate, (mach_task_self(),
						  proc_set_names[i]));
	}

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) proc_set_names,
				  sizeof(*proc_set_names)*proc_setsCnt));
	/* get tasks */

	MACH_CALL(processor_set_threads, 
		  (proc_set, threads, &threadsCnt));

	MACH_CALL( mach_port_deallocate, (mach_task_self(),
					  proc_set));
	return(threadsCnt);
}

int
get_task_cnt()
{
   	task_port_t *tasks_list;
    	unsigned int tasksCnt;
	register i;

	tasksCnt = get_tasks(&tasks_list);

	for (i=0; i < tasksCnt; i++)
		mach_port_deallocate(mach_task_self(),
				     tasks_list[i]);
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) tasks_list,
				  sizeof(*tasks_list)*tasksCnt));
	
	return(tasksCnt);
}

int
get_thread_cnt()
{
   	thread_port_t *threads_list;
    	unsigned int threadsCnt;
	register i;

	threadsCnt = get_threads(&threads_list);

	for (i=0; i < threadsCnt; i++)
		mach_port_deallocate(mach_task_self(),
				     threads_list[i]);
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) threads_list,
				  sizeof(*threads_list)*threadsCnt));
	
	return(threadsCnt);
}

mach_call_failed(
        char            *routine,
        char            *file,
        int             line,
        kern_return_t   code)
{
        test_exit("%s() in %s at line %d: %s\n",
                  routine, file, line, mach_error_string(code));
}

mach_func_failed(
        char    *func,
        char    *file,
        int     line)
{
        test_exit("%s() in %s at line %d: returns NULL value\n",
                  func, file, line);
}

get_security_token(security_token_t *token) {  
        unsigned int token_size; 

        token_size = TASK_SECURITY_TOKEN_COUNT;
        MACH_CALL(task_info, (mach_task_self(), TASK_SECURITY_TOKEN,
                (task_info_t)token,
                &token_size));
}

mach_sleep(microseconds)
{
	tvalspec_t 		time = {0, 0};
	tvalspec_t 		wakeup;
	register 		i;
	mach_port_t 		clock;

	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   REALTIME_CLOCK,
					   &clock));
	time.tv_sec = microseconds/USEC_PER_SEC;
	microseconds -= time.tv_sec * USEC_PER_SEC;
	time.tv_nsec = microseconds * NSEC_PER_USEC;
	MACH_CALL(clock_sleep, (clock, TIME_RELATIVE, time, &wakeup));
	MACH_CALL(mach_port_deallocate, (mach_task_self(), clock));
}

do_mach_notify_port_deleted(
	mach_port_t	port,
	int		name)			 
{
	if (debug)
		printf("mach_notify_port_deleted(), name = %x\n", name);
	return(KERN_SUCCESS);
}

kern_return_t
do_mach_notify_port_destroyed(
	mach_port_t	port,
	int		rcv_right)
{
	if (debug)
		printf("mach_notify_port_destroyed(), rcv_right = %x\n", rcv_right);
	return(KERN_SUCCESS);
}

kern_return_t
do_mach_notify_no_senders(
	mach_port_t	port,
	int		count)
{
	if (debug)
		printf("mach_notify_no_senders(), count = %x\n", count);
	return(KERN_SUCCESS);
}

kern_return_t
do_mach_notify_send_once(
	mach_port_t	port)
{
	if (debug)
		printf("mach_notify_send_once()\n");
	return(KERN_SUCCESS);
}

kern_return_t
do_mach_notify_dead_name(
	mach_port_t	port,
	int		name)			 
{
	if (debug)
		printf("mach_notify_dead_name(), name = %x\n", name);
	return(KERN_SUCCESS);
}
