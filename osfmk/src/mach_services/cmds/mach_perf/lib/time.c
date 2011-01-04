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

mach_port_t 		my_host;		/* Used to acquire time */
struct time_stats 	client_stats;
struct time_stats 	server_stats;
mach_msg_type_number_t	thread_info_count;
mach_port_t 		mach_clock;

boolean_t		thread_stats = TRUE; /* Collect thread time stats ? */

/*
 * conbert tvalspec_t to microseconds (double)
 */

double
tval_to_usec(time)
tvalspec_t *time;
{
	double us;

	us = time->tv_sec * ONE_SEC;

  	/*
	 * Convert to microseconds 
	 */

	us +=time->tv_nsec/NSEC_PER_USEC;

	return us;
}

/*
 * Returns elapsed microseconds
 */

double
elapsed(before_time , after_time)
tvalspec_t *before_time, *after_time;
{
	double start, stop;

	start = before_time->tv_sec * ONE_SEC;
	stop = after_time->tv_sec * ONE_SEC;

  	/*
	 * Convert to microseconds 
	 */

	start += before_time->tv_nsec/NSEC_PER_USEC;
	stop += after_time->tv_nsec/NSEC_PER_USEC;

	return stop-start;
}

double
total_time(ts)
struct time_stats *ts;
{
	double res;

	res = elapsed(&ts->before.timeval, &ts->after.timeval);
	if (use_timer && fabs(res) < ts->timer_overhead)
		res = timer_overhead;
	return(res);
}

double
user_time(ts)
struct time_stats *ts;
{
	if (!thread_stats)
		return(0.0);
	return(elapsed((tvalspec_t *)&ts->before.thread_basic_info.user_time,
		       (tvalspec_t *)&ts->after.thread_basic_info.user_time));
}

double
system_time(ts)
struct time_stats *ts;
{
	if (!thread_stats)
		return(0.0);
	return(elapsed((tvalspec_t *)&ts->before.thread_basic_info.system_time,
		       (tvalspec_t *)&ts->after.thread_basic_info.system_time));
}

int time_initialized = 0;

time_init()
{
	tvalspec_t time = {0, 0};

	my_host = mach_host_self();
	client_stats.thread = mach_thread_self();
	thread_info_count = sizeof (struct thread_basic_info);
	MACH_CALL(host_get_clock_service, (my_host,
					   REALTIME_CLOCK,
					   &mach_clock));
	MACH_CALL(clock_get_time, (mach_clock, &time));
	reset_time_stats(&client_stats);
	reset_time_stats(&server_stats);

}

hertz()
{
	unsigned resolution;
	unsigned count;

	count = 1;
	if(clock_get_attributes(mach_clock,
					  CLOCK_ALARM_CURRES,
					  (clock_attr_t)&resolution,
					  &count) != KERN_SUCCESS)
		resolution = (10 * ONE_MS)/NSEC_PER_USEC;

	return (NSEC_PER_SEC/resolution);
}
       
boolean_t		use_timer;
mach_port_t		timer_port;
int			timer_overhead;
int			timer;

int null = 0;

DECL_NULL_TEST(overhead_test)

struct test null_tests[] = {
"null_test",	0, overhead_test, TRUE, CALL_TRACE, TIME_MACROS, 0,
0, 		0, 0, 0, 0, 0, 0
};

kern_return_t
init_timer(timer)
{
	tvalspec_t Start_time = {0, 0};
	tvalspec_t Stop_time = {0, 0};
	unsigned resolution;
	double usec_resolution;
	unsigned count = 1;
	register unsigned i,j;
	double overhead;
	double dev;
	boolean_t saved_iopt = iopt;
	int saved_loops = loops;
	int saved_nruns = nruns;
	boolean_t saved_topt = topt;
	boolean_t saved_thread_stats = thread_stats;
	mach_port_t saved_server = server;
	boolean_t saved_prof_opt = prof_opt;
	boolean_t saved_vm_opt = vm_opt;
	boolean_t saved_verbose = verbose;
	boolean_t saved_ntraces = ntraces;
	boolean_t saved_filter = filter;
	int saved_timer_overhead = timer_overhead;

	if (debug)
		printf("init_timer(%d)\n", timer);

	if (timer == NULL_CLOCK) {
		use_timer = FALSE;
		if (server) {
			MACH_CALL(server_set_timer, (server,
						     timer,
						     timer_port,
						     timer_overhead));
		}
		return(KERN_SUCCESS);
	}

	if (timer == USER_CLOCK) {
		if (user_timer_init)
			(*user_timer_init)();
		else {
			printf("No user mode timer\n");
			return(KERN_FAILURE);
		}
	} else {
		MACH_CALL(host_get_clock_service , (my_host,
						    timer,
						    &timer_port));

		if(clock_get_attributes(timer_port,
						  CLOCK_GET_TIME_RES,
						  (clock_attr_t)&resolution,
						  &count) != KERN_SUCCESS)
			usec_resolution = 10 * ONE_MS;
		else {
		 	 /* convert to micro */
		  	usec_resolution = resolution;
			usec_resolution /= NSEC_PER_USEC;
		}
	}

	loops = 1;
	nruns = 10; 
	use_timer = TRUE;
	thread_stats = FALSE;
	filter = TRUE;
	if (!debug) {
		silent++;
		verbose = FALSE;
	}

	iopt = TRUE;
	topt = FALSE;
	server = MACH_PORT_NULL;
	prof_opt = FALSE;
	vm_opt = FALSE;
	if (!debug)
		ntraces = 0;
	timer_overhead = 0;

	while (run_tests(&null_tests[0]) != 0);

	if (!debug)
	    silent--;
	iopt =  saved_iopt;
	loops = saved_loops;
	nruns = saved_nruns;
	topt = saved_topt;
	thread_stats = saved_thread_stats;
	server = saved_server;
	prof_opt = saved_prof_opt;
	vm_opt = saved_vm_opt;
	verbose = saved_verbose;
	ntraces = saved_ntraces;
	filter = saved_filter;

	overhead = average(&client_stats.total_per_op);
	dev = deviation(&client_stats.total_per_op)*100/overhead;
	if (!silent) {
	    if (timer != USER_CLOCK)
         	printf("Using timer %d (%.3f us resolution). ",
		       timer, usec_resolution);
	    else
		printf("Using user mode timer. ");
	    printf("Test overhead is %.3f us (+/- %.3f %%)\n",
		   overhead, dev);
	}

	if (!overhead_opt)
		timer_overhead = overhead;
	else {
		timer_overhead = saved_timer_overhead;
        	if (!silent)
		    printf("Test overhead forced to %d us\n",
			   timer_overhead);
	}
	if (!silent && (timer_overhead < 0 || timer_overhead >= ONE_MS)) {
		printf("invalid timer overhead (%d us) for timer %d\n",
		       timer_overhead, timer);
		if (timer != USER_CLOCK) {
			MACH_CALL(mach_port_deallocate,
				  (mach_task_self(), timer_port));
		}
		return(KERN_FAILURE);
	}
	/* if resolution is low, we force min_elapsed time to a large value.
	   we do not set timer_overhead as we don't know it and for large
 	   elapsed time the overhead should be insignificant. 
	*/
	if(timer != USER_CLOCK && usec_resolution >= ONE_MS && !min_elapsed)
		min_elapsed = ((background) ? \
			(usec_resolution * (RES_FACTOR+10)) : \
			(usec_resolution * RES_FACTOR));
	else if (min_elapsed == 0.0) {
	    if (!synthetic_fn) {
		min_elapsed = overhead * RUN_FACTOR;
	    } else {
		min_elapsed = ((background) ? \
			(overhead * RUN_FACTOR * 7) : \
			(overhead * RUN_FACTOR * 5));
	    }
	    /* round up to milliseconds */
	    min_elapsed = (floor(min_elapsed/1000) + 1) * 1000;
	}

	if (server) {
	  	MACH_CALL(server_set_timer, (server,
					     timer,
					     timer_port,
					     timer_overhead));
	}
	return(KERN_SUCCESS);
}

reset_time_stats(ts)
struct time_stats *ts;
{
	reset_sampling(&ts->total_per_op);
	reset_sampling(&ts->user_per_op);
	reset_sampling(&ts->system_per_op);
	reset_sampling(&ts->total);
	ts->before.timeval.tv_sec = 0;
	ts->before.timeval.tv_nsec = 0;
	ts->after.timeval.tv_sec = 0;
	ts->after.timeval.tv_nsec = 0;
	ts->timer_overhead = timer_overhead;
}
	

collect_stats(ts)
struct time_stats *ts;
{
	double sample;

	sample = (total_time(ts) - ts->timer_overhead) / ts->loops;
	collect_sample(&ts->total_per_op, sample);
	sample = user_time(ts)/ts->loops;
	collect_sample(&ts->user_per_op, sample);
	sample = system_time(ts)/ts->loops;
	collect_sample(&ts->system_per_op, sample);
	collect_sample(&ts->total, total_time(ts));
}

/*
 * Convert microseconds to nanoseconds
 * Some kernel interfaces used or still use microseconds.
 */

void
fix_time_stat(tv)
tvalspec_t *tv;
{
	tv->tv_nsec *= NSEC_PER_USEC;
}

void
fix_thread_time_stats(ts)
struct time_stats *ts;
{
  	/* thread user time*/
	fix_time_stat(&ts->before.thread_basic_info.user_time);
	fix_time_stat(&ts->after.thread_basic_info.user_time);

  	/* thread system time */
	fix_time_stat(&ts->before.thread_basic_info.system_time);
	fix_time_stat(&ts->after.thread_basic_info.system_time);
}

void
fix_elapsed_time_stat(ts)
struct time_stats *ts;
{
	fix_time_stat(&ts->before.timeval);
	fix_time_stat(&ts->after.timeval);
}

void start_time_call()
	start_time_macro()

void stop_time_call()
	stop_time_macro()
