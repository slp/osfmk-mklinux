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

#define TEST_NAME	latency

#include <mach_perf.h>

char *private_options = "\n\
\t[-clock_id <id>]         Use <id> as kernel clock id.\n\
\t                         0: Realtime clock (default)\n\
\t                         1: Battery clock\n\
\t                         2: High resolution clock\n\
\t[-sync]                  Synchronize with clock ticks (default)\n\
\t[-no_sync]               Do not synchronize with clock ticks\n\
\t[-clockres <us>]         Set the clock resolution to <us> microseconds\n\
";

int latency_test();

extern kern_return_t	change_clock_resolution(mach_port_t	clockpriv,
						int             *clockresp);

struct test tests[] = {
"latency",			0, latency_test, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

static mapped_tvalspec_t *MapTime;
static boolean_t	sync;
static int user_clockres;
static int clock_id = REALTIME_CLOCK;
static tvalspec_t	max_time, min_time;

#define get_time(time)							      \
	if (timer == USER_CLOCK) {					      \
		(*user_timer)(time);					      \
	} else {							      \
		TIME_MACH_CALL(clock_get_time, (clock, time));	      	      \
	} 								  

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	mach_port_t 	clock, clockpriv, pager;
	mach_port_t	task = mach_task_self();
	mach_msg_type_number_t sz;
	int clockres, clockminres, clockmaxres, clockcurres;

	/*
	 * Reset to defaults each time the test is run.
	 */
	user_clockres = -1;
	sync = TRUE;
	max_time.tv_sec = max_time.tv_nsec = 0;
	min_time.tv_sec = min_time.tv_nsec = 99999999; /* XXX */

	test_init();
	filter = FALSE;	/* We measure latencies, really want to know real
			   min, max and deviations values. This applies to
			   all latency tests*/
	nruns = 100;

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-clock_id")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &clock_id))
				usage();
		} else if (!strcmp(argv[i], "-clockres")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &user_clockres))
				usage();
		} else if (!strcmp(argv[i], "-sync")) {
			sync = TRUE;
		} else if (!strcmp(argv[i], "-no_sync")) {
			sync = FALSE;
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	if (!iopt) {
		loops = 1;
		iopt = TRUE;
	}

	if (!overhead_opt) {
		timer_overhead = 0;	/* since we dont use start/stop_time */
		overhead_opt = TRUE;    /* as intended */ 
	}
	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   clock_id,
					   &clock));
	/*
	 * Mapped time is needed to synchronize with the internal
	 * clock ticks.
	 */
	MACH_CALL(clock_map_time, (clock, &pager));
	MACH_CALL(vm_map, (task, (vm_address_t *)&MapTime,
			   sizeof(mapped_tvalspec_t), 0, TRUE,
			   pager, 0, 0,
			   VM_PROT_READ, VM_PROT_READ, VM_PROT_NONE));
	MACH_CALL(mach_port_deallocate, (task, pager));

	MACH_CALL(host_get_clock_control, (privileged_host_port,
					   clock_id,
					   &clockpriv));

#define clockfreq(res)	NSEC_PER_SEC/(res)
	sz = 1;	/* size in int units */
	MACH_CALL(clock_get_attributes, 
		        (clockpriv, CLOCK_ALARM_MINRES,
			(clock_attr_t)&clockminres,
			&sz));
	if (verbose)
		printf("Minimal clock frequency: %d Hz (resolution= %d ns)\n",
		       clockfreq(clockminres), clockminres);

	sz = 1;
	MACH_CALL(clock_get_attributes, 
		        (clockpriv, CLOCK_ALARM_MAXRES,
			(clock_attr_t)&clockmaxres,
			&sz));
	if (verbose)
		printf("Maximal clock frequency: %d Hz (resolution= %d ns)\n",
		       clockfreq(clockmaxres), clockmaxres);

	sz = 1;
	MACH_CALL(clock_get_attributes, 
		        (clockpriv, CLOCK_ALARM_CURRES,
			(clock_attr_t)&clockcurres,
			&sz));
	printf("Current clock frequency: %d Hz (resolution= %d ns)\n",
	       clockfreq(clockcurres), clockcurres);

	if (user_clockres < 0)
		user_clockres = clockcurres;
	else {
		user_clockres *= 1000; /* us -> ns */
		if (user_clockres > clockminres ||
		    user_clockres < clockmaxres) {
			printf("Clock resolution has to be within %d and %d us\n",
			       clockminres/1000, clockmaxres/1000);
			usage();
		}
	}

	if (clockminres != clockmaxres) {
		MACH_CALL(change_clock_resolution,
			  (clockpriv, (clock_attr_t)&user_clockres));
		printf("Clock frequency is now: %d Hz (resolution= %d ns)\n",
		       clockfreq(user_clockres), user_clockres);
	}

	run_tests(tests);

	if (clockminres != clockmaxres) {
		MACH_CALL(change_clock_resolution,
			  (clockpriv, (clock_attr_t)&clockcurres));
	}
	MACH_CALL(mach_port_deallocate, (task, clockpriv));
	MACH_CALL(mach_port_deallocate, (task, clock));

	/*
	 * The peak values should be corrected by the overhead
	 * of the accouting as is with the elapsed time.
	 */
	printf("Minimum user latency:  %8.2f\n",
	       tval_to_usec(&min_time));
	printf("Maximum user latency:  %8.2f\n",
	       tval_to_usec(&max_time));

	test_end();
}

/*
 * Synchronize with the clock.
 * Return on the next clock tick, and set time.
 *
 * Mapped time is updated every clock tick while the real-time clock
 * returns the current value. On i386, the resolution is every 10ms
 * which means that every 10ms there is a clock tick which starts the
 * execution of the alarm path. The alarm path awakes threads for which
 * the alarm time is already over or the alarms coming in the next first
 * half clock tick. We can thus be awaken too late or too early.
 * By synchronizing with the clock and asking to be awaken on a clock
 * tick, we ensure to measure a pure latency for a perfect kernel.
 * Alarm times should of course be a multiple of 10 ms as well.
 */
void
clock_sync(t)
        tvalspec_t      *t;
{
        tvalspec_t      tt;

	MTS_TO_TS(MapTime, &tt);
        *t = tt;
        while (CMP_TVALSPEC(t, &tt) == 0) {
		MTS_TO_TS(MapTime, t);
	}
}

latency_test()
{
	register 		i;
	kern_return_t		kr;
	mach_port_t		task = mach_task_self();
	mach_port_t 		clock;

	tvalspec_t 		wakeup, kwakeup, time;
	tvalspec_t 		alarm_time = {0, NSEC_PER_SEC/100}; /* 10 ms */
	tvalspec_t		elapsed_time = {0, 0};
	tvalspec_t		overhead_time = {0, 0};

	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   clock_id,
					   &clock));

	start_time();
	
	i = loops;
	if (!no_cache)
		i++;	/* cache the test, ignore first timing */
	while(i--)  { 
		if (sync)
			clock_sync(&time);
		else
			get_time(&time);
		/*
		 * Ask to be awaken after alarm_time from here
		 * clock_sleep returns KERN_SUCCESS, KERN_FAILURE
		 * (if the service is not supported) or KERN_ABORTED.
		 */
		ADD_TVALSPEC(&time, &alarm_time);
		if (clock_sleep(clock, TIME_ABSOLUTE, time, &kwakeup)
		    != KERN_SUCCESS) {
			i++;
			printf("BAD time %8.2f alarmtime %8.2f\n",
			       tval_to_usec(&time),
			       tval_to_usec(&alarm_time));
			continue;
		}
		/*
		 * Get user wake time
		 */
		get_time(&wakeup);

		if (debug > 1) {
			printf("time %8.2f alarmtime %8.2f\n",
			       tval_to_usec(&time),
			       tval_to_usec(&alarm_time));
			printf("wu %8.2f kwu %8.2f\n",
			       tval_to_usec(&wakeup),
			       tval_to_usec(&kwakeup));
		}

		/*
		 * Compute elapsed time between specified wake time
		 * and actual wake time.
		 */
		if (CMP_TVALSPEC(&wakeup, &time) < 0) {
			/* we've been awaken before time */
			i++;
			continue;
		}

		if (i >= loops && !no_cache)
		  	continue;

		SUB_TVALSPEC(&wakeup, &time);

		/*
		 * The actual elapsed time for the test is the sum of
		 * the individual latencies.
		 */
		ADD_TVALSPEC(&elapsed_time, &wakeup);

		if (verbose)
			printf("user latency time %8.2f(%8.2f)\n",
			       tval_to_usec(&wakeup),
			       tval_to_usec(&elapsed_time));

		/*
		 * Compute peak wake times.
		 */
		if (CMP_TVALSPEC(&max_time, &wakeup) < 0)
                        max_time = wakeup;
                else if (CMP_TVALSPEC(&min_time, &wakeup) > 0)
                        min_time = wakeup;
	} 
	stop_time();
		
	/*
	 * Actual stop time is start time + elapsed time (user)
	 *
	 * XXX Should check if stop_time() also includes the overhead
	 * XXX of its own measurement in order to have correct overhead
	 * XXX measurements.
	 */
	client_stats.after.timeval = elapsed_time;
	ADD_TVALSPEC(&client_stats.after.timeval,
		     &client_stats.before.timeval);
	SUB_TVALSPEC(&client_stats.after.timeval,
		     &overhead_time);

	MACH_CALL(mach_port_deallocate, (task, clock));

	if (verbose) {
		printf("\nMaximum user latency:   %8.2f\n",
		       tval_to_usec(&max_time));
		printf("Minimum user latency:   %8.2f\n",
		       tval_to_usec(&min_time));
		printf("Total user latency  :   %8.2f for %d loops\n",
		       tval_to_usec(&elapsed_time), loops);
		printf("Average user latency:   %8.2f\n",
			tval_to_usec(&elapsed_time)/loops);
	}
}


kern_return_t
change_clock_resolution(
	mach_port_t	clockpriv,
	int             *clockresp)
{
        int 			clockres;
	mach_msg_type_number_t	sz;

	MACH_CALL(clock_set_attributes,
		  (clockpriv, CLOCK_ALARM_CURRES,
		   (clock_attr_t)clockresp,
		   1));

	/*
	 * Clock resolution is adjusted on the next clock tick.
	 */
	clockres = 0;
	while (clockres != *clockresp) {
	  sz = 1;
	  MACH_CALL(clock_get_attributes, 
		    (clockpriv, CLOCK_ALARM_CURRES,
		     (clock_attr_t)&clockres,
		     &sz));
	}

	*clockresp = clockres;

	if (timer == USER_CLOCK) {
		if (user_timer_init)
			(*user_timer_init)();
		else {
			printf("No user mode timer\n");
			return(KERN_FAILURE);
		}
	}

	return KERN_SUCCESS;
}
