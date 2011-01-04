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


#define TEST_NAME	exc

#include <mach_perf.h>
#include <exc_types.h>

char *private_options = "\n\
\t[-show_server]           Show server side timings.\n\
\t[-kill_server]           Kill server upon test completion.\n\
\t[-client]                Only start client side task.\n\
\t[-server]                Only start server side task.\n\
\t[-norma <node>]          Start server task on node <node>\n\
";

int max_port_names = 20000;

#define THREAD	1
#define TASK 	2

#define TRAP_CALL	0
#define MSG_CALL	1

extern exc_server_main();

int null_trap_test();
int catch_exception_test();
int thread_set_exception_ports_test();
int task_set_exception_ports_test();
int mach_call_test_with_clock_get_time();
int mach_call_test_with_task_set_special_port();

struct test tests[] = {
"null trap",		0, null_trap_test, 0, 0, 0, 0,
"trap kcall (clock_get_time)",		0, mach_call_test_with_clock_get_time,
					TRAP_CALL, 0, 0, 0,
"msg kcall (mig_clock_get_time)",	0, mach_call_test_with_clock_get_time,
					MSG_CALL, 0, 0, 0,
"trap kcall (task_set_special_port)",	0, mach_call_test_with_task_set_special_port,
					TRAP_CALL, 0, 0, 0,
"msg kcall (mig_task_set_special_port)",	0, mach_call_test_with_task_set_special_port,
					MSG_CALL, 0, 0, 0,
"",					0, 0, 0, 0, 0, 0,
"thread_set_exc_ports",	0, thread_set_exception_ports_test, 0, 0, 0, 0,
"task_set_exc_ports",	0, task_set_exception_ports_test, 0, 0, 0, 0,
"Server thread in a same task",		0, 0, 0, 0, 0, 0,
"exc_raise (task exc port)",		0, catch_exception_test,
					TASK, THREAD, EXCEPTION_DEFAULT, 0,
"exc_raise (thread exc port)",		0, catch_exception_test,
					THREAD, THREAD, EXCEPTION_DEFAULT, 0,
"exc_raise_state (task exc port)",	0, catch_exception_test,
					TASK, THREAD, EXCEPTION_STATE, 0,
"exc_raise_state (thread exc port)",	0, catch_exception_test,
					THREAD, THREAD, EXCEPTION_STATE, 0,
"Server thread in a separate task",	0, 0, 0, 0, 0, 0,
"exc_raise (task exc port)",		0, catch_exception_test,
					TASK, TASK, EXCEPTION_DEFAULT, 0,
"exc_raise (thread exc port)",		0, catch_exception_test,
					THREAD, TASK, EXCEPTION_DEFAULT, 0,
"exc_raise_state (task exc port)",	0, catch_exception_test,
					TASK, TASK, EXCEPTION_STATE, 0,
"exc_raise_state (thread exc port)",	0, catch_exception_test,
					THREAD, TASK, EXCEPTION_STATE, 0,
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
	int argc;
	char *argv[];
{
	int i, server_death_warrant = 0;
 
	test_init();

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-show_server")) {
		  	server_times = 1; 
#if	!MONITOR
		} else if (!strcmp(argv[i], "-client")) {
		  	client_only = 1; 
		} else if (!strcmp(argv[i], "-server")) {
		  	server_only = 1; 
		} else if (!strcmp(argv[i], "-kill_server")) {
		  	server_death_warrant = 1; 
#endif	/* !MONITOR */
		} else if (!strcmp(argv[i], "-norma")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &norma_node))
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	if (!client_only)
		run_server((void(*)())exc_server_main);
	else {
		server = server_lookup(EXC_SERVER_NAME);
		MACH_CALL(set_printer, (server, printf_port));
	}
	
	if (server_only)
		return(0);

	run_tests(tests);

	if (!client_only || server_death_warrant)
		kill_server(server);

	test_end();
}

thread_set_exception_ports_test()
{
	register int i;
	mach_port_t port;
	mach_port_t thread = mach_thread_self();
	mach_port_t task = mach_task_self();
	
       
	MACH_CALL( mach_port_allocate, (task,
				 MACH_PORT_RIGHT_RECEIVE,
				 &port));
	MACH_CALL( mach_port_insert_right, (task, port, port,
				     MACH_MSG_TYPE_MAKE_SEND));
	ignore_server();
	start_time();
	for (i=loops; i--;)  {
		MACH_CALL( thread_set_exception_ports,
			  (thread,
			   EXC_MASK_BAD_INSTRUCTION,
			   port,
			   EXCEPTION_DEFAULT,
			   THREAD_STATE));
	}
	stop_time();
	MACH_CALL( thread_set_exception_ports,
		  (thread,
		   EXC_MASK_BAD_INSTRUCTION,
		   MACH_PORT_NULL,
		   EXCEPTION_DEFAULT,
		   THREAD_STATE));
	MACH_CALL(mach_port_destroy, (task, port));
}
	
task_set_exception_ports_test()
{
	register int i;
	mach_port_t port;
	mach_port_t thread = mach_thread_self();
	mach_port_t task = mach_task_self();
	
	MACH_CALL( mach_port_allocate, (task,
				 MACH_PORT_RIGHT_RECEIVE,
				 &port));
	MACH_CALL( mach_port_insert_right, (task, port, port,
				     MACH_MSG_TYPE_MAKE_SEND));
	ignore_server();
	start_time();
	for (i=loops; i--;)  {
		MACH_CALL( task_set_exception_ports,
			  (task,
			   EXC_MASK_BAD_INSTRUCTION,
			   port,
			   EXCEPTION_DEFAULT,
			   THREAD_STATE));
	}
	stop_time();
	MACH_CALL( task_set_exception_ports,
		  (task,
		   EXC_MASK_BAD_INSTRUCTION,
		   MACH_PORT_NULL,
		   EXCEPTION_DEFAULT,
		   THREAD_STATE));
	MACH_CALL(mach_port_destroy, (task, port));
}

null_trap_test()
{
	register i;

	ignore_server();
	start_time();
	for (i=loops; i--;) 
		null_trap();
	stop_time();
}

extern exception_thread();

catch_exception_test(port_type, server_type, behavior)
exception_behavior_t behavior;
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t old_task_exc_port, old_thread_exc_port;
	exception_mask_t old_mask;
	unsigned int exc_ports_count=1;
	exception_behavior_t old_task_exc_behavior, old_thread_exc_behavior;
	thread_state_flavor_t old_task_exc_flavor, old_thread_exc_flavor;

	if (server_type == THREAD)
		start_server_thread((void(*)())exception_thread);

	MACH_CALL( thread_get_exception_ports,
		  (mach_thread_self(),
		   EXC_MASK_SYSCALL,
		   &old_mask,
		   &exc_ports_count,
		   &old_thread_exc_port,
		   &old_thread_exc_behavior,
		   &old_thread_exc_flavor));

	MACH_CALL( task_get_exception_ports,
		  (task,
		   EXC_MASK_SYSCALL,
		   &old_mask,
		   &exc_ports_count,
		   &old_task_exc_port,
		   &old_task_exc_behavior,
		   &old_task_exc_flavor));

	if (port_type == THREAD) {
		MACH_CALL( thread_set_exception_ports,
			  (mach_thread_self(),
			   EXC_MASK_SYSCALL,
			   server,
			   behavior,
			   THREAD_STATE));
		MACH_CALL( task_set_exception_ports,
			  (task,
			   EXC_MASK_SYSCALL,
			   MACH_PORT_NULL,
			   EXCEPTION_DEFAULT,
			   0));
	} else {
		MACH_CALL( task_set_exception_ports,
			  (task,
			   EXC_MASK_SYSCALL,
			   server,
			   behavior,
			   THREAD_STATE));

		MACH_CALL( thread_set_exception_ports,
			  (mach_thread_self(),
			   EXC_MASK_SYSCALL,
			   MACH_PORT_NULL,
			   EXCEPTION_DEFAULT,
			   0));
	}

	start_time();
	for (i=loops; i--;) {
		invalid_trap(); 
	}
	stop_time();

	invalid_trap(); 	/* cleanup in server task */

	MACH_CALL( thread_set_exception_ports,
			  (mach_thread_self(),
			   EXC_MASK_SYSCALL,
			   old_thread_exc_port,
			   old_thread_exc_behavior,
			   old_thread_exc_flavor));

	MACH_CALL( task_set_exception_ports,
			  (task,
			   EXC_MASK_SYSCALL,
			   old_task_exc_port,
			   old_task_exc_behavior,
			   old_task_exc_flavor));

	if (server_type == THREAD)
		stop_server_thread();

	if (old_task_exc_port != MACH_PORT_NULL)
		MACH_CALL( mach_port_deallocate, (task, old_task_exc_port));
	if (old_thread_exc_port != MACH_PORT_NULL)
		MACH_CALL( mach_port_deallocate, (task, old_thread_exc_port));
}

int exc_debug;

exception_thread(port)
mach_port_t port;
{
	mach_port_t thread = 0;
	extern boolean_t exc_demux();

	exc_debug = debug;

	if (exc_debug)
		printf("calling mach_msg_server\n");
	MACH_CALL(mach_msg_server, (exc_demux,
				    EXC_MSG_BUF_SIZE,
				    port,
				    MACH_MSG_OPTION_NONE));
}

catch_exception_raise(port, thread, task, exception, code, subcode)
mach_port_t port;
mach_port_t thread;
mach_port_t task;
{
	if (exc_debug > 2)
		printf("exception, server_count %d\n", server_count);
	if (server_count-- == 0) {
		if (task != mach_task_self()) {
			MACH_CALL(mach_port_destroy, (mach_task_self(),
						      task));
			MACH_CALL(mach_port_destroy, (mach_task_self(),
						      thread));
		}
	}
	return(KERN_SUCCESS);
}

catch_exception_raise_state(port, exception, code,
			    code_count, flavor, in_state,
			    in_count, out_state, out_count)
mach_port_t port;
exception_type_t	exception, code;
mach_msg_type_number_t code_count;
thread_state_flavor_t	*flavor;
thread_state_t *in_state, *out_state;
mach_msg_type_number_t in_count, *out_count;
{
	server_count--;
	if (exc_debug > 2)
		printf("exception %d bytes\n", in_count*sizeof(int));
	bcopy((char *)in_state, (char *)out_state, in_count*sizeof(int));
	*out_count = in_count;
	return(KERN_SUCCESS);
}

catch_exception_raise_state_identity(port)
mach_port_t port;
{
	return(KERN_SUCCESS);
}

mach_call_test_with_clock_get_time(op)
{
	register i;
	tvalspec_t time;
	mach_port_t clock;
	mach_port_t host;

	host = mach_host_self();
	MACH_CALL(host_get_clock_service, (host,
					   REALTIME_CLOCK,
					   &clock));
	ignore_server();
	if (op == TRAP_CALL) {
		start_time();
		for (i=loops; i--;)
			MACH_CALL(clock_get_time, (clock, &time));
		stop_time();
	} else { 
		start_time();
		for (i=loops; i--;)
			MACH_CALL(mig_clock_get_time, (clock, &time));
		stop_time();
	}
	MACH_CALL(mach_port_deallocate, (mach_task_self(), clock));
}


mach_call_test_with_task_set_special_port(op)
{
	mach_port_t port;
	mach_port_t task = mach_task_self();
	register i;

	MACH_CALL( mach_port_allocate, (task,
					MACH_PORT_RIGHT_RECEIVE,
					&port));

	MACH_CALL( mach_port_insert_right, (task,
					    port,
					    port,
					    MACH_MSG_TYPE_MAKE_SEND));
	ignore_server();
	if (op == TRAP_CALL) {
		start_time();
		for (i=loops; i--;)
			MACH_CALL(task_set_special_port,
				  (task,
				   TASK_BOOTSTRAP_PORT,
				   port));
		stop_time();
	} else { 
		start_time();
		for (i=loops; i--;)
			MACH_CALL(mig_task_set_special_port,
				  (task,
				   TASK_BOOTSTRAP_PORT,
				   port));
		stop_time();
	}
	MACH_CALL(mach_port_destroy, (task, port));
}




