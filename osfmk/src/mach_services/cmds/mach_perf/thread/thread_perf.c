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


#define TEST_NAME thread

#include <mach_perf.h>

char *private_options = "\n\
";

#define ON 1
#define OFF 0

int suspend_resume_test();
int thread_create_terminate_test();
int thread_create_test();
int thread_terminate_test();
int thread_get_state_test();
int thread_set_state_test();
int thread_self_test();
int thread_info_test();
int thread_sample_test();

struct test tests[] = {
"thread_self",			0, thread_self_test, 0, 0, 0, 0,
"thread_create",		0, thread_create_test, 0, 0, 0, 0,
"thread_terminate",		0, thread_terminate_test, 0, 0, 0, 0,
"thread_create/terminate/destroy",0, thread_create_terminate_test, 0, 0, 0, 0,
"thread_suspend/resume",	0, suspend_resume_test, 0, 0, 0, 0,
"thread_get_thread_state",	0, thread_get_state_test, 0, 0, 0, 0,
"thread_set_state",		0, thread_set_state_test, 0, 0, 0, 0,
"thread_info",			0, thread_info_test, THREAD_BASIC_INFO, 0, 0, 0,
"thread_sample (on)",		0, thread_sample_test, ON, 0, 0, 0,
"thread_sample (off)",		0, thread_sample_test, OFF, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++)
		if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();


	run_tests(tests);
	test_end();

}

thread_self_test()
{
	mach_port_t thread;
	register i;

	start_time();

	for (i=loops; i--;)  {
		MACH_FUNC(thread, mach_thread_self, ());
	}
	stop_time();
}

volatile mach_port_t slave_thread;

suspend_resume_slave()
{
	slave_thread = mach_thread_self();

	while(1) {
		MACH_CALL( thread_switch, (0, SWITCH_OPTION_DEPRESS, 1000));
	}
}
	
suspend_resume_test()
{
	register i;
	mach_thread_t thread;

	slave_thread = 0;
	new_thread(&thread, (vm_offset_t)suspend_resume_slave, (vm_offset_t)0);

	while (!slave_thread);
	start_time();

	for (i=loops; i--;)  {
		MACH_CALL( thread_suspend, (slave_thread));
		MACH_CALL( thread_resume, (slave_thread));
	}
	stop_time();
	kill_thread(thread);
}

thread_create_terminate_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t slave_thread;

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL( thread_create, (task, &slave_thread));
		MACH_CALL( thread_terminate, (slave_thread));
		MACH_CALL( mach_port_destroy, (task, slave_thread));
	}
	stop_time();
}

thread_create_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *threads, *thread;

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&threads,
				  loops*sizeof(mach_port_t),
				  TRUE));
	start_time();

	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_create, (task, thread));
	}
	stop_time();

	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_terminate, (*thread));
		MACH_CALL( mach_port_destroy, (task, *thread));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )threads,
				  loops*sizeof(mach_port_t)));
}

thread_terminate_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *threads, *thread;

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&threads,
				  loops*sizeof(mach_port_t),
				  TRUE));

	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_create, (task, thread));
	}
	start_time();

	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_terminate, (*thread));
	}
	stop_time();

	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( mach_port_destroy, (task, *thread));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )threads,
				  loops*sizeof(mach_port_t)));
}

thread_get_state_test()
{
	register i;
	mach_port_t thread;
	thread_state_t state;
	mach_msg_type_number_t count = THREAD_STATE_COUNT;
	
	MACH_CALL( thread_create, (mach_task_self(), &thread));
	
	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&state,
				  THREAD_STATE_COUNT,
				  TRUE));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL( thread_get_state, (thread, THREAD_STATE,
				       state, &count));
	}
	stop_time();
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)state,
				  THREAD_STATE_COUNT));
	MACH_CALL( thread_terminate, (thread));
	MACH_CALL( mach_port_destroy, (mach_task_self(), thread));
}

thread_set_state_test()
{
	register i;
	mach_port_t thread;
	thread_state_t state;
	
	MACH_CALL( thread_create, (mach_task_self(), &thread));
	
	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&state,
				  THREAD_STATE_COUNT,
				  TRUE));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL( thread_set_state, (thread, THREAD_STATE,
				       state,
				       THREAD_STATE_COUNT));
	}
	stop_time();
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)state,
				  THREAD_STATE_COUNT));
	MACH_CALL( thread_terminate, (thread));
	MACH_CALL( mach_port_destroy, (mach_task_self(), thread));
}


thread_info_test(flavor)
int flavor;
{
	register i;
	mach_port_t thread = mach_thread_self();
	mach_msg_type_number_t count;
	thread_info_t info;
	struct thread_basic_info basic_info;
	
	switch(flavor) {
	case THREAD_BASIC_INFO:
		info = (thread_info_t)&basic_info;
		count = sizeof basic_info;
		break;
	}
	start_time();
	for (i=loops; i--;) {
		MACH_CALL(thread_info, (thread, flavor, info, &count));
	}
	stop_time();
}

thread_sample_test(on)
boolean_t on;
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *threads, *thread, prof_port;


	MACH_FUNC( prof_port, mach_reply_port, ());

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&threads,
				  loops*sizeof(mach_port_t),
				  TRUE));


	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_create, (task, thread));
	}

	if (on)
		start_time();

	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_sample, (*thread, prof_port));
	}

	if (!on) {
		start_time();
		for (i=loops, thread = threads; i--; thread++)  {
			MACH_CALL( thread_sample, (*thread, MACH_PORT_NULL));
		}
	}

	stop_time();
		
	for (i=loops, thread = threads; i--; thread++)  {
		MACH_CALL( thread_terminate, (*thread));
		MACH_CALL( mach_port_destroy, (mach_task_self(), *thread));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )threads,
				  loops*sizeof(mach_port_t)));

	MACH_CALL( mach_port_destroy, (mach_task_self(), prof_port));
}
