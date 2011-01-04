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



#define TEST_NAME sched

#include <mach_perf.h>

typedef int priority_t;

struct policy_state saved_policy;
static int timeout = 10;

char *private_options = "\n\
\t[-timeout <timeout>      Set depression abort timeout\n\
";

int thread_switch_test();
int double_thread_switch_test();
int sync_thread_test();
int sync_task_test();

struct test tests[] = {
"thread_switch(0, NONE)",	0, thread_switch_test, 0,
			           SWITCH_OPTION_NONE, 0, 0,
"thread_switch(0, DEPRESS)",	0, thread_switch_test, 0,
			           SWITCH_OPTION_DEPRESS, 0, 0,
"2 threads switch(0, DEPRESS)",	0, sync_thread_test, 0,
			           SWITCH_OPTION_DEPRESS, 0, 0,
"2 threads switch(handoff, NONE)",	0, sync_thread_test, 0,
			           SWITCH_OPTION_NONE, 1, 0,
"2 threads switch(0, NONE)",	0, sync_thread_test, 0,
			           SWITCH_OPTION_NONE, 0, 0,
"2 task switch(0, DEPRESS) *",	0, sync_task_test, 0,
			           SWITCH_OPTION_DEPRESS, 0, 0,
"2 task switch(0, NONE) *",	0, sync_task_test, 0,
			           SWITCH_OPTION_NONE, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

extern void	sched_build_sched_attr(struct 	policy_state *old_ps,
				       struct 	policy_state *new_ps,
				       int 		switch_option);

int *sync;

main(argc, argv)
int argc;
char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-timeout")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			timeout = atoi(argv[i]);
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	get_thread_sched_attr(mach_thread_self(), &saved_policy, FALSE);
	if (in_kernel) {
		/* XXX be silent about disabled tests? */
		if (!topt) {
			topt = TRUE;
			enable_all_tests(tests);
		}
		disable_test_number(5,tests);	/* 2 task switch */
		disable_test_number(6,tests);	/* 2 task switch */
	}

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&sync,
				  vm_page_size,
				  TRUE));
	MACH_CALL( vm_inherit, (mach_task_self(),
			 (vm_offset_t)sync,
			 vm_page_size,
			 VM_INHERIT_SHARE));


	run_tests(tests);
	test_end();
}

thread_switch_test(switch_option)
{
	register int i;
	
	start_time();
	for (i=loops; i--;)  {
		MACH_CALL( thread_switch, (0, switch_option, timeout));
	}
	stop_time();
}

volatile mach_port_t slave_thread;
mach_port_t master_thread;
extern int slave();

struct slave_args {
	int switch_option;
	int handoff;
} slave_args;

/* eliminate possible macro conflicts, these are private to this file */
#undef  OLD
#undef  NEW
#define OLD 0
#define NEW 1

cooperate(thread, switch_option, slave)
mach_port_t thread;
{
	register int i;
	register ocount;
	int failed = 0;
	register cached_debug = debug;

	if (debug)
		printf("start %s\n", slave ? "slave" : "master");

	if (slave) {
		*(sync+1) = -1;
		while (*(sync+1) != 0)
			thread_switch(MACH_PORT_NULL,
				      SWITCH_OPTION_DEPRESS,
				      10);
		if (cached_debug > 1)
			printf("continue slave\n");
		ocount = 0;
	} else {
		while (*(sync+1) != -1)
			thread_switch(MACH_PORT_NULL,
				      SWITCH_OPTION_DEPRESS,
				      10);
		if (cached_debug > 1)
			printf("continue master\n");
		*(sync+1) = 0;
		ocount = -1;
	}

	if (!slave)
		start_time();
	for (i=loops; i--;)  {
		while (*sync == ocount && *sync > 0) {
			failed++;
			MACH_CALL( thread_switch,  (thread,
						    switch_option,
						    timeout));
		}
		ocount = *sync + 1;
		(*sync)++;
		if (cached_debug > 1)
			printf("%s switch(%x, %x) loop %d\n",
			       slave ? "slave" : "master",
			       thread, switch_option, i);
		MACH_CALL( thread_switch, (thread, switch_option, timeout));
	}
	if (!slave)
		stop_time();

	if (failed && cached_debug)
		printf("%d failures for %s\n", failed, slave ? "slave" : "master");
	if (debug)
		printf("%s done\n", slave ? "slave" : "master");
}

sync_slave(args)
struct slave_args *args;
{
	mach_port_t thread = 0;
	int switch_option = args->switch_option;

	slave_thread = mach_thread_self();
	if (args->handoff)
		thread = master_thread;
	cooperate(thread, switch_option, 1);
	*(sync+1) = 1;
	while(1)
		thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, 10);
}

sync_thread_test(thread, switch_option, handoff)
{
	mach_thread_t child;
	struct policy_state new_ps;

	set_thread_sched_attr(mach_thread_self(), &saved_policy, FALSE);
	sched_build_sched_attr(&saved_policy, &new_ps, switch_option);

	slave_thread = 0;
	*sync = 0;
	*(sync+1) = 0;
	master_thread = mach_thread_self();
	slave_args.switch_option = switch_option;
	slave_args.handoff = handoff;
	new_thread(&child, sync_slave, &slave_args);
	while (!slave_thread)
		thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, 10);
	set_thread_sched_attr(mach_thread_self(), &new_ps, FALSE);
	set_thread_sched_attr(slave_thread, &new_ps, FALSE);
	if (handoff)
		thread = slave_thread;
	cooperate(thread, switch_option, 0);
	if (debug > 1)
		printf("master waits %d\n", slave_thread);
	while(!*(sync+1))
		thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, 10);
	kill_thread(child);
	if (debug > 1)
		printf("master exits\n");
}

mach_port_t
slave_task_thread(mach_port_t task)
{
	thread_port_array_t thread_list;
	mach_msg_type_number_t thread_count;
	mach_port_t thread;

	MACH_CALL(task_threads, (task, &thread_list, &thread_count));
	thread = *thread_list;
	MACH_CALL(vm_deallocate,
			  (mach_task_self(), (vm_offset_t ) thread_list, 
			   thread_count * sizeof(thread_port_t)));
	return(thread);
}

sync_task_test(thread, switch_option)
{
	mach_port_t slave_task;
	mach_port_t slave_thread;
	struct policy_state new_ps;

	set_thread_sched_attr(mach_thread_self(), &saved_policy, FALSE);
	sched_build_sched_attr(&saved_policy, &new_ps, switch_option);

	*sync = 0;
	*(sync+1) = 0;

	if (!(slave_task = mach_fork())) {
		cooperate(0, switch_option, 1);
		*(sync+1) = 1;
		MACH_CALL(task_terminate, (mach_task_self()));
	} else {
	  	MACH_FUNC(slave_thread, slave_task_thread, (slave_task));
	  	set_thread_sched_attr(mach_thread_self(), &new_ps, FALSE);
	  	set_thread_sched_attr(slave_thread, &new_ps, FALSE);
		MACH_CALL(mach_port_deallocate, (mach_task_self(),
						 slave_thread));
		cooperate(0, switch_option, 0);
		while(!*(sync+1))
			thread_switch(MACH_PORT_NULL,
				      SWITCH_OPTION_DEPRESS,
				      10);
		task_terminate(slave_task);
		MACH_CALL(mach_port_destroy, (mach_task_self(), slave_task));
	}
}

void
sched_build_sched_attr(
	struct 	policy_state *old_ps,
	struct 	policy_state *new_ps,
	int 	switch_option)
{
	if (sched_policy == -1) {
		switch (switch_option) {
		case SWITCH_OPTION_DEPRESS:
			new_ps->policy = POLICY_TIMESHARE;
			break;
		default:
			new_ps->policy = POLICY_FIFO;
			break;
		}
	} else {
		new_ps->policy = sched_policy;
	}

	set_base_pri(new_ps, base_pri);
	set_max_pri(new_ps, max_pri);
	set_quantum(new_ps, quantum);
	if (get_base_pri(new_ps) == -1)
		set_base_pri(new_ps, get_base_pri(old_ps));
	if (get_max_pri(new_ps) == -1)
		set_max_pri(new_ps, get_max_pri(old_ps));
	if (get_quantum(new_ps) == -1)
	  	set_quantum(new_ps, get_quantum(old_ps));

}
