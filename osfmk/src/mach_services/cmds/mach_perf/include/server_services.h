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

#ifndef _SERVER_SERVICES_H
#define _SERVER_SERVICES_H

#define	CLIENT 0
#define SERVER 1

#define BEFORE 0
#define AFTER 1

/*
 * External variables used for server task.
 */

extern mach_port_t	server;
extern mach_port_t	server_task;
extern boolean_t 	server_only;
extern boolean_t 	client_only;
extern int 		server_count;
extern int 		server_times;
extern int 		server_time(int run);
mach_port_t 		server_lookup(char *name);
extern int 		start_server_test();
extern int 		stop_server_test();
extern boolean_t 	server_in_same_task;
extern boolean_t 	_ignore_server;
extern double 		fabs(double), floor(double);

#define measure_in_server_task(a) {					      \
	MACH_CALL(start_server, (server, loops));			      \
	a;								      \
	MACH_CALL( server_get_stats,					      \
		  (server,						      \
		   CLIENT,						      \
		   BEFORE,						      \
		   &client_stats.before.timeval,			      \
		   &client_stats.before.thread_basic_info,		      \
		   &client_stats.timer_overhead));			      \
	MACH_CALL( server_get_stats,					      \
		  (server,						      \
		   CLIENT,						      \
		   AFTER,						      \
		   &client_stats.after.timeval,				      \
		   &client_stats.after.thread_basic_info,		      \
		   &client_stats.timer_overhead));			      \
}

/*
 * Do not invoke start_server()/stop_server()
 * when starting/stopping measurement.
 */

#define ignore_server()	_ignore_server = TRUE;	

/*
 * Keep the test (server_only && !standalone) in sync with run_server()
 */
#define server_announce_port(port)					\
	if (!(server_only && is_master_task)) {				\
	     mach_port_t bootstrap_port;				\
      	     MACH_CALL(task_get_bootstrap_port, (mach_task_self(),	\
					  &bootstrap_port));		\
             if (debug)							\
		 printf("calling send_server_port(%x,%x)\n",		\
                        bootstrap_port, port);				\
	     MACH_CALL(send_server_port, (bootstrap_port, port));	\
        }

extern int run_server(void (*func)());
extern int start_server_thread( void (*func)() );
extern int stop_server_thread();
extern void server_name_check_in(char *name, mach_port_t server_port);

#endif /* _SERVER_SERVICES_H */
	














