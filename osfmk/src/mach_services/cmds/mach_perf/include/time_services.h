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

#ifndef _TIME_SERVICES_H
#define _TIME_SERVICES_H

#include <mach_perf.h>

/*
 * All Units expressed in microseconds.
 */

#define ONE_SEC 1000000				/* One second */
#define ONE_MS	1000				/* One millisecond */

#define USER_CLOCK 3				/* XXX assumes only 0,1 & 2
						   are kernel defined */
#define NULL_CLOCK -1				/* XXX assumes only 0,1 & 2
						   are kernel defined */

/* defines to control the minimum elapsed time calculated by init_timer() */
#define	RUN_FACTOR 1000 /* overhead * RUN_FACTOR gives min elapsed time */
#define	RES_FACTOR 50 /* for low resolution timers resolution * RES_FACTOR */
		      /* gives the min elapsed time */

extern void			(*user_timer)(); 	/* User provided timer */
extern void			(*user_timer_init)(); 	/* User provided timer */

extern mach_port_t 		my_host;	/* My host */

extern boolean_t		use_timer;
extern boolean_t		thread_stats;
extern mach_port_t		timer_port;
extern int			timer_overhead;
extern boolean_t		overhead_opt;
extern int			timer;
extern int			new_stats;
extern boolean_t		filter;

struct time_info {
	struct thread_basic_info 	thread_basic_info;
	tvalspec_t 			timeval;
};

struct time_stats {
	mach_port_t		thread;
	struct time_info	before;
	struct time_info	after;
	struct stats		total_per_op;
	struct stats 		user_per_op;
	struct stats 		system_per_op;
	struct stats 		total;
	int 			loops;
	int			timer_overhead;	/* us */
};

extern struct time_stats 	client_stats; /* time stats for client side */
extern struct time_stats 	server_stats; /* time stats for server side */
extern mach_msg_type_number_t	thread_info_count;

#if	CALL_TRACE
#define TIME_MACH_CALL MACH_CALL
#else	/* CALL_TRACE */
#define TIME_MACH_CALL(a, args) (void) a args
#endif	/* CALL_TRACE */

#define thread_stat_macro(thread, info) {				      \
	TIME_MACH_CALL(thread_info, (thread,	 			      \
				     THREAD_BASIC_INFO,		  	      \
				     (thread_info_t)info,		      \
				     &thread_info_count));		      \
}

#define thread_stats_macro(when) {					      \
	if (server) {							      \
		thread_stat_macro(server_stats.thread,			      \
				  &server_stats.when.thread_basic_info);      \
	}							              \
	thread_stat_macro(client_stats.thread,			 	      \
			  &client_stats.when.thread_basic_info);	      \
}
	
#if 	OLD_CLOCK
#define old_clock_macro(when) 						      \
	else {								      \
		(void) host_get_time(my_host, &client_stats.when.timeval);    \
	}
#else	/* OLD_CLOCK */
#define old_clock_macro(when)
#endif	/* OLD_CLOCK */

#define time_macro(when) {						      \
	if (timer == USER_CLOCK) {					      \
		(*user_timer)(&client_stats.when.timeval);		      \
	} else if (use_timer) {						      \
		TIME_MACH_CALL(clock_get_time, (timer_port,		      \
					   &client_stats.when.timeval));      \
	} 								      \
	old_clock_macro(when);						      \
}

#define	start_time_macro() { 				      		      \
	if (server && !_ignore_server) {				      \
		TIME_MACH_CALL(start_server, (server, loops));		      \
	}								      \
        if (synthetic_fn != NULL)                                             \
            (synthetic_fn->start_time)();				      \
	if (prof_opt)							      \
		do_prof_start();					      \
	if (thread_stats)						      \
		thread_stats_macro(before);				      \
	time_macro(before);						      \
}

#define	stop_time_macro() {						      \
	time_macro(after);						      \
	if (thread_stats)						      \
		thread_stats_macro(after);				      \
	if (prof_opt)							      \
		do_prof_stop();						      \
	if (server && !_ignore_server) {				      \
		TIME_MACH_CALL(stop_server, (server));			      \
	}								      \
}

extern void 	start_time_call();
extern void 	stop_time_call();

#if	TIME_MACROS
#define do_time_start	start_time_macro
#define do_time_stop	stop_time_macro
#else	/* TIME_MACROS */
#define do_time_start	start_time_call
#define do_time_stop	stop_time_call
#endif	/* TIME_MACROS */

#if	CALL_TRACE
#define do_time_trace(op, file, line) 					      \
	if (ntraces) 					      		      \
		mach_call_print(op, file, line, 0);
#else	/* CALL_TRACE */
#define do_time_trace(op, file, line) 
#endif	/* CALL_TRACE */

#define start_time() { 							      \
	do_time_trace("start_time", __FILE__, __LINE__);		      \
	do_time_start();						      \
	}

#define stop_time() { 							      \
	do_time_stop();							      \
	do_time_trace("stop_time", __FILE__, __LINE__);			      \
	}

/*
 * Various defines for measuring test overhead
 */


#define NULL_TEST_HOW(how, op) {					      \
	do_time_trace("start_time", __FILE__, __LINE__);		      \
	start_time_ ## how();						      \
	for (i=loops; i--;)						      \
		op;							      \
	stop_time_ ## how();						      \
	do_time_trace("stop_time", __FILE__, __LINE__);			      \
	}

#define NULL_TEST(op)							      \
	if (with_macro)							      \
		NULL_TEST_HOW(macro, op)				      \
	else								      \
		NULL_TEST_HOW(call, op)

extern int null;

#define DECL_NULL_TEST(null_name)					      \
null_name(								      \
	boolean_t with_mach_call, 					      \
	boolean_t with_trace, 						      \
	boolean_t with_macro) 						      \
{									      \
	register i;							      \
	if (!with_mach_call)						      \
		NULL_TEST( /**/)					      \
	else if (!with_trace)						      \
		NULL_TEST( MACH_CALL_NO_TRACE(null, /* No args */) )   	      \
	else								      \
		NULL_TEST( MACH_CALL_WITH_TRACE(null, /* No args */) )        \
}

extern int      time_init();
extern double   tval_to_usec(tvalspec_t *time);
extern double   elapsed(tvalspec_t *before_time, tvalspec_t *after_time);
extern double   total_time(struct time_stats *ts);
extern double   user_time(struct time_stats *ts);
extern double   system_time(struct time_stats *ts);
extern int      collect_stats(struct time_stats *ts);

#endif /* _TIME_SERVICES_H */

