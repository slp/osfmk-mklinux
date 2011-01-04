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
#include <mach/norma_special_ports.h>

struct test server_test = {
"             server side:", 0, 0, 0, 0, 0, 0,
};

static jmp_buf 		server_jmp_buf;
boolean_t		server_server();
int 			server_times = 0;
int			server_count = 0;
boolean_t 		server_only = FALSE;
boolean_t 		client_only = FALSE;
mach_port_t		server = MACH_PORT_NULL;
mach_port_t		server_task = MACH_PORT_NULL;
boolean_t		_ignore_server = FALSE;

/*
 * Variables used when server thread is created in same task
 */

boolean_t		server_in_same_task = FALSE;

mach_port_t		saved_server; /* Used to save the thread
				       * port of the server task 
				       * when we create a server
				       * thread in the client
				       * task */
mach_port_t		server_thread;

/*
 * Create a server task, and establish server/client channel
 * with it.
 *	create_server(func, serv_task, serv_port)
 *		func: entry point for server
 *	   out	serv_task: mach_port_t for server task
 *	   out	serv_port: send right to server
 */

kern_return_t
create_server(func, serv_task, serv_port)
void		(*func)();
mach_port_t	*serv_task, *serv_port;
{
	mach_port_t bootstrap_port;

	if (debug)
		printf("create_server\n");

	MACH_CALL( mach_port_allocate, (mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE,
				 &bootstrap_port));
	MACH_CALL( mach_port_insert_right, (mach_task_self(),
					    bootstrap_port,
					    bootstrap_port,
					    MACH_MSG_TYPE_MAKE_SEND));
	MACH_CALL(task_set_bootstrap_port, (mach_task_self(),
					    bootstrap_port));

	if (server_task = mach_fork()) {
		server_wait(bootstrap_port);
		*serv_port = server;
		*serv_task = server_task;
	} else {
		mach_port_t server_port;

		MACH_CALL(task_get_bootstrap_port, (mach_task_self(),
						  &bootstrap_port));
        	MACH_CALL(bootstrap_ports,    (bootstrap_port,
                                               &privileged_host_port,
                                               &master_device_port,
                                               &root_ledger_wired,
                                               &root_ledger_paged,
                                               &security_port));

		MACH_CALL(mach_port_allocate, (mach_task_self(),
						MACH_PORT_RIGHT_RECEIVE,
						&server_port));
		need_banner = FALSE;
		prof_opt = 0;
		timer = NULL_CLOCK;
		use_timer = FALSE;
		timer_port = MACH_PORT_NULL;
		/*
		 * The task is now responsible for publishing the
		 * port on which it will service requests.
		 * This is performed by server_announce_port(server_port).
		 */
		(*func)(server_port);
	}
	return(KERN_SUCCESS);
}	

boolean_t server_demux(in, out)
{
	if (server_server(in, out))
		return(TRUE);
	else
	    return(bootstrap_server(in, out));
}

server_wait(bootstrap_port)
mach_port_t bootstrap_port;
{
	thread_malloc_state_t mallocs = save_mallocs(thread_self());
	if (debug > 1)
		printf("server_wait()\n");
	if (mach_setjmp(server_jmp_buf) == 0) {
		MACH_CALL( mach_msg_server, (server_demux,
				     256,
				     bootstrap_port,
				     MACH_MSG_OPTION_NONE));
	} else {
		restore_mallocs(thread_self(), mallocs);
		MACH_CALL(mach_port_destroy, (mach_task_self(),
					      bootstrap_port));
	}
}

do_send_server_port(boostrap_port, server_port)
mach_port_t boostrap_port;
mach_port_t server_port;
{
	server = server_port;
	mach_longjmp(server_jmp_buf, 1);
}


/*
 * Services used between client and server for test statistics and
 * synchronization
 */

do_server_start_test(server, show_time, _verbose, server_thread)
mach_port_t server;
mach_port_t *server_thread;
{
 	server_stats.thread = mach_thread_self();
	*server_thread = server_stats.thread;
	if (!(is_master_task || server_in_same_task)) {
		resources_start();
		server_times = show_time;
		verbose = _verbose;
	}
	return(KERN_SUCCESS);
}

do_server_stop_test(server)
mach_port_t server;
{
	if (!(is_master_task || server_in_same_task)) {
		resources_stop();
		resources_use("Server side ");   
	}
	return(KERN_SUCCESS);
}

do_server_print_time(server)
mach_port_t	server;
{
	if (thread_stats) {
		print_header(&server_test);
		print_results(&server_stats);
	}
	return(KERN_SUCCESS);
}

do_set_printer(server, printer)
mach_port_t	server;
mach_port_t printer;
{
	if (server_only && !debug) {
		printf_port = printer;
		is_master_task = 0;
	}
	return(KERN_SUCCESS);
}

do_stop_server(server)
mach_port_t	server;
{
	if (debug > 1)
		printf("stop_server()\n");
	if (server_count) {
		printf("Invalid number of loops for server: %d\n",
			       server_count);
	}
	return(KERN_SUCCESS);
}

do_start_server(server, new_count)
mach_port_t	server;
int new_count;
{
	loops = server_count = new_count;
	if (debug > 1)
		printf("start_server(%d)\n", new_count);
	return(KERN_SUCCESS);
}

do_kill_server(server)
mach_port_t	server;
{
	if (debug > 1)
		printf("kill_server\n");
	MACH_CALL(task_terminate, (mach_task_self())); 
	MACH_CALL(mach_port_destroy, (mach_task_self(),
					      server));
	server = MACH_PORT_NULL;
	return (KERN_SUCCESS);
}

server_time(run)
{
	if ((!server_times) || (server == MACH_PORT_NULL))
		return;

	if (run) {
		/* XXX: Thread basic info still returns microseconds ! */
		fix_thread_time_stats(&server_stats);
		collect_stats(&server_stats);
		if (verbose)
			print_one_sample(&server_test,
					 &server_stats);
	} else if (thread_stats) {
		boolean_t saved_prof_opt = prof_opt;
		prof_opt = FALSE;
		print_header(&server_test);
		prof_opt = saved_prof_opt;
		print_results(&server_stats);
	}	
}

mach_port_t lookup(name)
	char *name;
{
	mach_port_t	port;
	kern_return_t	ret;

	MACH_CALL(netname_look_up, (name_server_port, "*", name, &port));
	return(port);
}

mach_port_t
server_lookup(name)
char *name;
{
	if (client_only) {
		server = lookup(name);
		if (server == MACH_PORT_NULL) {
			printf("unable to find server.\n");
			leave(2);
		}
		MACH_CALL(set_printer, (server, printf_port));
		return(server);
	}
	return(MACH_PORT_NULL);
}

start_server_test()
{
	if (server != MACH_PORT_NULL) {
		MACH_CALL(server_start_test, (server,
					      server_times,
					      verbose,
					      &server_stats.thread));
	}
}

stop_server_test()
{
	if (server != MACH_PORT_NULL && !server_in_same_task) {
		MACH_CALL(server_stop_test, (server));
	}
	if (server_stats.thread != MACH_PORT_NULL) {
		MACH_CALL(mach_port_deallocate, (mach_task_self(),
							  server_stats.thread));
		server_stats.thread = MACH_PORT_NULL;
	}
}

run_server(func)
void		(*func)();
{
	/* keep server_announce_port() in sync with this test */
	if (server_only && is_master_task) {
		timer = NULL_CLOCK;
		use_timer = FALSE;
		timer_port = MACH_PORT_NULL;
		(*func)(0);
		printf("server died\n");
		leave(1);
	}
	if (in_kernel) {
		/*
		 * Collocate another task linked at the same offsets?
		 */
		printf("Cannot clone collocated task.\n");
		leave(1);
	}
	MACH_CALL(create_server, (func,
				  &server_task,
				  &server));
}

/*
 * Start a server thread in the same task
 */

start_server_thread(func)
void (*func)();
{
	mach_port_t port;

	if (debug)
		printf("start server thread(%x)\n", func);
	
	MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&port));
	MACH_CALL( mach_port_insert_right, (mach_task_self(),
					    port, port,
					    MACH_MSG_TYPE_MAKE_SEND));
	MACH_CALL(new_thread, (&server_thread, func, port));
	saved_server = server;
	server = port;
	if (server_stats.thread != MACH_PORT_NULL) {
		MACH_CALL(mach_port_deallocate, (mach_task_self(),
						 server_stats.thread));
	}
	server_in_same_task = TRUE;
	start_server_test();
}

stop_server_thread()
{
	mach_port_t port;

	if (debug)
		printf("terminate server thread\n");

	if (saved_server == MACH_PORT_NULL) {
		printf("No active server thread in current task\n");
		return;
	}
	port = server;
	server = saved_server;
	saved_server = MACH_PORT_NULL;

	kill_thread(server_thread);
	MACH_CALL( mach_port_destroy, (mach_task_self(), port));

	server_stats.thread = MACH_PORT_NULL;

	/* Do not reset server_in_same_task now, it is used by
	   server_time() */
	 
}

do_server_get_stats(server, who, when, time, thread_stats, overhead)
mach_port_t	server;
int who, when;
tvalspec_t *time;
thread_basic_info_t thread_stats;
int *overhead;
{
	struct time_stats *ts;
	struct time_info *ti;

	ts = (who == CLIENT) ? &client_stats : & server_stats;
	ti = (when == BEFORE) ? &ts->before : &ts->after;
	*time = ti->timeval;
	*thread_stats = ti->thread_basic_info;
	*overhead = ts->timer_overhead;
	return(KERN_SUCCESS);
}

void	(*user_timer_init)(); /* User provided timer */

do_server_set_timer(
	mach_port_t server,
	int	    _timer,
	mach_port_t _timer_port,
	int 	    overhead)
{

  	if (debug) {
		printf("server_set_timer(timer %x)\n");
		printf("was timer %x, use_timer %x\n", timer, use_timer);
	}
	timer_overhead = overhead;

	if (timer == _timer) {
		if (timer_port != _timer_port &&
		    norma_node == NORMA_NODE_NULL) {
			printf("server_set_time: inconsistency\n");
			return(KERN_FAILURE);
		}
	} else {
		if (timer_port != MACH_PORT_NULL) {
			MACH_CALL( mach_port_deallocate, (mach_task_self(),
							  timer_port));
		}

		timer = _timer;
		use_timer = (timer != NULL_CLOCK);

		if (timer == USER_CLOCK) {
			(*user_timer_init)();
		} else if (norma_node == NORMA_NODE_NULL) {
			timer_port = _timer_port;
		} else {
			init_timer(timer);
		}
	}
  	if (debug)
		printf("timer %d timer_overhead %d us\n",
		       timer, timer_overhead);

	reset_time_stats(&client_stats);
	reset_time_stats(&server_stats);
	return(KERN_SUCCESS);
}

void server_name_check_in(name, server_port)
char *name;
mach_port_t server_port;
{
                MACH_CALL (netname_check_in, (name_server_port,
                                              name,
                                              mach_task_self(),
                                              server_port));
}

do_bootstrap_ports(bootstrap,
	priv_host, priv_device, wired_ledger, paged_ledger, host_security)
mach_port_t bootstrap;
mach_port_t *priv_host;
mach_port_t *priv_device;
mach_port_t *wired_ledger;
mach_port_t *paged_ledger;
mach_port_t *host_security;
{
	if (norma_node == NORMA_NODE_NULL) {
		*priv_host = privileged_host_port;
		*priv_device = master_device_port;
	} else { /* return remote node priv ports */
		mach_port_t rem_priv_host;
		mach_port_t rem_priv_device;

		MACH_CALL (norma_get_host_priv_port, (privileged_host_port,
						      norma_node,
						      &rem_priv_host));
		
		MACH_CALL (norma_get_device_port, (privileged_host_port,
						   norma_node,
						   &rem_priv_device));

		*priv_host = rem_priv_host;
		*priv_device = rem_priv_device;
	}
	*wired_ledger = root_ledger_wired;
	*paged_ledger = root_ledger_paged;
	*host_security = security_port;

	return(KERN_SUCCESS);
}

kern_return_t
do_bootstrap_arguments(mach_port_t bootstrap,
               task_port_t task_port,
               vm_offset_t *args,
               mach_msg_type_number_t *args_count)
{
	*args = 0;
	*args_count = 0;
	return(KERN_SUCCESS);
}

kern_return_t
do_bootstrap_environment(mach_port_t bootstrap,
             task_port_t task_port,
             vm_offset_t *env,
             mach_msg_type_number_t *env_count)
{
	*env = 0;
	*env_count = 0;
	return(KERN_SUCCESS);
}

kern_return_t
do_bootstrap_completed(mach_port_t bootstrap,
                       task_port_t task_port)
{
	return(KERN_SUCCESS);
}
