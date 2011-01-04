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


#define TEST_NAME task

#include <mach_perf.h>

char *private_options = "\n\
\t[-max_tasks <count>]     Set maximum task count to <count>\n\
\t                         (default is 600)\n\
";

int	max_tasks = 600;	/* update help message */

#define ON 1
#define OFF 0

int task_suspend_resume_test();
int task_create_terminate_test();
int task_create_test();
int task_terminate_test();
int task_self_test();
int task_threads_test();
int task_threads_deallocate_test();
int task_info_test();
int task_sample_test();

struct test tests[] = {
"task_self",			0, task_self_test, 0, 0, 0, 0,
"task_create",			0, task_create_test, 0, 0, 0, 0,
"task_create + inherit *",	0, task_create_test, 1, 0, 0, 0,
"task_terminate",		0, task_terminate_test, 0, 0, 0, 0,
"task_create/terminate/destroy",0, task_create_terminate_test, 0, 0, 0, 0,
"task_suspend/resume *",	0, task_suspend_resume_test, 0, 0, 0, 0,
"task_threads",			0, task_threads_test, 0, 0, 0, 0,
"task_threads + vm_deallocate",	0, task_threads_deallocate_test, 0, 0, 0, 0,
"task_info",			0, task_info_test, TASK_BASIC_INFO, 0, 0, 0,
"task_sample (on)",		0, task_sample_test, ON, 0, 0, 0,
"task_sample (off)",		0, task_sample_test, OFF, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

int sync;

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-max_tasks")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
		  	if (!atod(argv[i], &max_tasks)) 
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests,
				       private_options))
			usage();

	if (in_kernel) {
		/* XXX be silent about disabled tests? */
		if (!topt) {
			topt = TRUE;
			enable_all_tests(tests);
		}
		disable_test_number(2,tests);	/* task_create + inherit */
		disable_test_number(5,tests);	/* task_suspend/resume */
	}

	run_tests(tests);
	test_end();
}

task_self_test()
{
	mach_port_t task;
	register i;

	start_time();

	for (i=loops; i--;)  {
		MACH_FUNC(task, mach_task_self, ());
	}
	stop_time();
}

task_create_terminate_test(inherit)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t child_task;

	set_max_loops(max_tasks);
	start_time();

	for (i=loops; i--;)  {
                MACH_CALL( task_create, (task,
                        (ledger_port_array_t)0, 0, inherit, &child_task));
		MACH_CALL( task_terminate, (child_task));
		MACH_CALL( mach_port_destroy, (task, child_task));
	}
	stop_time();
	test_cache(task_create_terminate_test, (inherit));
}

task_create_test(inherit)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *tasks, *child_task;

	set_max_loops(max_tasks);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&tasks,
				  loops*sizeof(mach_port_t),
				  TRUE));
	start_time();

	for (i=loops, child_task = tasks; i--; child_task++)  {
                MACH_CALL( task_create, (task,
                        (ledger_port_array_t)0, 0, inherit, child_task));
	}
	stop_time();

	for (i=loops, child_task = tasks; i--; child_task++)  {
		MACH_CALL( task_terminate, (*child_task));
		MACH_CALL( mach_port_destroy, (task, *child_task));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )tasks,
				  loops*sizeof(mach_port_t)));
}

task_terminate_test(inherit)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *tasks, *child_task;

	set_max_loops(max_tasks);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&tasks,
				  loops*sizeof(mach_port_t),
				  TRUE));

	for (i=loops, child_task = tasks; i--; child_task++)  {
                MACH_CALL( task_create, (task,
                        (ledger_port_array_t)0, 0, inherit, child_task));
	}
	start_time();

	for (i=loops, child_task = tasks; i--; child_task++)  {
		MACH_CALL( task_terminate, (*child_task));
	}
	stop_time();

	for (i=loops, child_task = tasks; i--; child_task++)  {
		MACH_CALL( mach_port_destroy, (task, *child_task));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )tasks,
				  loops*sizeof(mach_port_t)));
}

task_suspend_resume_test()
{
	mach_port_t child_task;
	register i;

	if (!(child_task = mach_fork())) {
		while(1) {
			MACH_CALL( thread_switch, (0, SWITCH_OPTION_DEPRESS,
						   1000));
		}
	}
	start_time();

	for (i=loops; i--;)  {
		MACH_CALL( task_suspend, (child_task));
		MACH_CALL( task_resume, (child_task));
	}
	stop_time();
	MACH_CALL( task_terminate, (child_task));
	MACH_CALL( mach_port_destroy, (mach_task_self(), child_task));
	
}

task_threads_test()
{
	register i;
	mach_port_t task = mach_task_self();
	thread_port_array_t *thread_lists, *threads;
	mach_msg_type_number_t thread_count;

	MACH_CALL(vm_allocate_temporary,
		  (task,
		   (vm_offset_t *)&thread_lists,
		   loops*sizeof(thread_port_array_t),
		   TRUE));
	start_time();
	for (i=loops, threads = thread_lists; i--; threads++)
		MACH_CALL(task_threads, (task, threads, &thread_count));
	stop_time();

	for (i=loops, threads = thread_lists; i--; threads++) {
		register j;

		for (j=0; j < thread_count; j++) {
			if (debug > 1) {
				printf("threads %x\n", threads);
				printf("*threads %x\n", *threads);
				printf("**threads %x\n", **threads);
			}
			MACH_CALL(mach_port_deallocate, (task,
							 *((*threads)+j))); 
		}
		MACH_CALL(vm_deallocate,
			  (task, (vm_offset_t ) *threads, 
			   thread_count * sizeof(thread_port_t)));
	}
	MACH_CALL(vm_deallocate, (task, (vm_offset_t )thread_lists,
				  loops*sizeof(thread_port_array_t)));
}

/*
 * Deallocate thread list each time instead of increasing vm space
 */

task_threads_deallocate_test()
{
	register i;
	mach_port_t task = mach_task_self();
	thread_port_array_t threads;
	mach_msg_type_number_t thread_count;


	start_time();
	for (i=loops; i--;)  {
		MACH_CALL(task_threads, (task, &threads, &thread_count));
		MACH_CALL(vm_deallocate,
			  (task, (vm_offset_t ) threads, 
			   thread_count * sizeof(thread_port_t)));
	}
	stop_time();

}

task_info_test(flavor)
int flavor;
{
	register i;
	mach_port_t task = mach_task_self();
	mach_msg_type_number_t count;
	task_info_t info;
	struct task_basic_info basic_info;
	
	switch(flavor) {
	case TASK_BASIC_INFO:
		info = (task_info_t)&basic_info;
		count = sizeof basic_info;
		break;
	}
	start_time();
	for (i=loops; i--;) {
		MACH_CALL(task_info, (task, flavor, info, &count));
	}
	stop_time();
}

task_sample_test(on)
boolean_t on;
{
	register i, j;
	mach_port_t task = mach_task_self();
	mach_port_t *tasks, *child_task;
	mach_port_t *threads, *child_thread;
	mach_port_t prof_port;

	set_max_loops(max_tasks);

	MACH_FUNC( prof_port, mach_reply_port, ());

	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&tasks,
				  loops*sizeof(mach_port_t),
				  TRUE));
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&threads,
				  loops*sizeof(mach_port_t),
				  TRUE));

	for (i=loops, child_task = tasks, child_thread = threads
	     ; i--;
	     child_task++, child_thread++)  {
                MACH_CALL( task_create, (task,
                        (ledger_port_array_t)0, 0, 0, child_task));
		MACH_CALL( thread_create, (*child_task, child_thread));
	}

	if (on)
		start_time();

	for (i=loops, child_task = tasks; i--; child_task++)  {
		MACH_CALL( task_sample, (*child_task, prof_port));
	}

	if (!on) {
		start_time();
		for (i=loops, child_task = tasks; i--; child_task++) {
			MACH_CALL( task_sample, (*child_task,
						   MACH_PORT_NULL));
		}
	}

	stop_time();

	for (i=loops, child_task = tasks, child_thread = threads
	     ; i--;
	     child_task++, child_thread++)  {
		MACH_CALL( mach_port_destroy, (task, *child_thread));
		MACH_CALL( task_terminate, (*child_task));
		MACH_CALL( mach_port_destroy, (task, *child_task));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )tasks,
				  loops*sizeof(mach_port_t)));
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )threads,
				  loops*sizeof(mach_port_t)));
	MACH_CALL( mach_port_destroy, (mach_task_self(), prof_port));
}
