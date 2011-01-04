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

#define TEST_NAME	clock

#include <mach_perf.h>

extern int null_test();
int clock_get_time_test();
int clock_alarm_test();
int clock_sleep_test();
int clock_get_attributes_test();
int host_get_time_test();
int user_timer_test();

char *private_options = "\n\
\t[-clock_id <id>]         use <id> as kernel clock id.\n\
\t                         0: Realtime clock (default)\n\
\t                         1: Battery clock\n\
\t                         2: High resolution clock\n\
\t[-alarm <time>]          use <time> seconds for alarm test\n\
\t                         Default is 3.\n\
";

struct test tests[] = {
"null_test",			0, null_test, 0, 0, 0, 0,
"null_test using MACH_CALL",	0, null_test, TRUE, 0, 0, 0,
"null_test using MACH_CALL+trace", 0, null_test, TRUE, TRUE, 0, 0,
"Using macros for timers:",	0, 0, 0, 0, 0, 0,
"null_test",			0, null_test, 0, 0, TRUE, 0,
"null_test using MACH_CALL",	0, null_test, TRUE, 0, TRUE, 0,
"null_test using MACH_CALL+trace",0 , null_test, TRUE, TRUE, TRUE, 0,
"",				0, 0, 0, 0, 0, 0,
"clock_get_time",		0, clock_get_time_test,	0, 0, 0, 0,
"clock_alarm()",		0, clock_alarm_test, 0, 0, 0, 0,
"clock_sleep(0)",		0, clock_sleep_test, 0, 0, 0, 0,
"clock_get_attributes",		0, clock_get_attributes_test,
				CLOCK_GET_TIME_RES, 0, 0, 0,
"host_get_time (obsolete)",	0, host_get_time_test,	0, 0, 0, 0,
"user_timer",			0, user_timer_test, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

int clock_id = REALTIME_CLOCK;
#define ALARM_DEFAULT 		1	/* 1 seconds */

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-clock_id")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &clock_id))
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	if (user_timer_init)
		(*user_timer_init)();
	run_tests(tests);
	test_end();
}

DECL_NULL_TEST(null_test)

clock_get_time_test()
{
	tvalspec_t 		time = {0, 0};
	register 		i;
	mach_port_t 		clock;

	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   clock_id,
					   &clock));
	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(clock_get_time, (clock, &time));
	}
	stop_time();
	MACH_CALL(mach_port_deallocate, (mach_task_self(), clock));
}

clock_alarm_test()
{
	tvalspec_t 		wakeup, time;
	register 		i;
	mach_port_t 		clock, reply;
	tvalspec_t		elapsed_time = {0, 0};
	tvalspec_t 		alarm_time = {0, NSEC_PER_SEC/100};
					       /* 1/100 sec */

	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   clock_id,
					   &clock));
	MACH_FUNC(reply, mach_reply_port, ());

	do {
		ADD_TVALSPEC(&alarm_time, &elapsed_time);

		start_time();

		for (i=loops; i--;)  {
			MACH_CALL(clock_alarm, (clock, TIME_RELATIVE,
					alarm_time, reply));
		}

		stop_time();

		elapsed_time = client_stats.after.timeval;
		SUB_TVALSPEC(&elapsed_time, &client_stats.before.timeval);
		if (debug && CMP_TVALSPEC(&elapsed_time, &alarm_time) > 0) 
			printf("retry alarm: alarm %d:%09d elapsed %d:%09d\n",
			       alarm_time.tv_sec, alarm_time.tv_nsec,
			       elapsed_time.tv_sec, elapsed_time.tv_nsec);
	} while (CMP_TVALSPEC(&elapsed_time, &alarm_time) > 0);

	MACH_CALL(mach_port_destroy, (mach_task_self(), reply));

	/*
	 * Wait until all alarms expired to prevent noise
	 * for next tests.
	 */

	MACH_CALL(clock_get_time, (clock, &time));
	ADD_TVALSPEC(&time, &alarm_time);
	MACH_CALL(clock_sleep, (clock, TIME_ABSOLUTE, time, &wakeup));

	MACH_CALL(mach_port_deallocate, (mach_task_self(), clock));
}

clock_sleep_test(seconds)
{
	tvalspec_t 		time = {0, 0};
	tvalspec_t 		wakeup;
	register 		i;
	mach_port_t 		clock;

	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   clock_id,
					   &clock));

	time.tv_sec = seconds;

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(clock_sleep, (clock, TIME_RELATIVE, time, &wakeup));
	}

	stop_time();

	MACH_CALL(mach_port_deallocate, (mach_task_self(), clock));
}

clock_get_attributes_test(flavor)
{
	register 		i;
	mach_port_t 		clock;
	clock_attr_t		value;
	unsigned int		count = 1;

	MACH_CALL(host_get_clock_service, (mach_host_self(),
					   clock_id,
					   &clock));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL( clock_get_attributes, (clock,
						  flavor,
						  &value,
						  &count));
	}

	stop_time();

	MACH_CALL(mach_port_deallocate, (mach_task_self(), clock));
}


host_get_time_test()
{
	time_value_t 		time = {0, 0};
	register 		i;
	mach_port_t 		clock;
	mach_port_t 		host;

	host = mach_host_self();

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(host_get_time, (host, &time));
	}
	stop_time();
}

user_timer_test()
{
  	tvalspec_t start, stop;
	register i;


	if (!user_timer_init) {
		test_error("user_timer_test", "no such timer");
	}

	start_time();
	for (i=loops; i--;)  {
		(*user_timer)(&start);
	}
	stop_time();

	if (debug) {
		(*user_timer)(&start);
		(*user_timer)(&stop);
		printf("elapsed %8.2f\n", elapsed(&start, &stop));
	}
}


