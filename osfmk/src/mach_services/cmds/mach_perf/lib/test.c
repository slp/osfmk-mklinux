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

char *version_string = "MPTS 1.1";

int		loops;			/* number of iterations */
int		max_loops;		/* Maximum number of iterations per run*/
int		min_loops;		/* Minimu number of iterations per run */
boolean_t	iopt;			/* User specified number of iterations */
int		nruns;			/* number of runs */
int             background = 0;         /* non-zero == background task */
test_state_t	test_status;		/* enum type */
boolean_t	vm_opt;			/* vm_stats option turned on */
boolean_t	verbose;		/* print detailed timing informations */
boolean_t	topt;			/* User specified  a test list */
int		no_cache;		/* do not cache */
int		(*more_measures)();	/* test specific */
int		(*start_test)();	/* test specific */
int		(*stop_test)();		/* test specific */
sync_vec	*synthetic_fn = NULL;	/* standalone/not test specific */
char	 	*command_name;		/* "ipc", "port", "vm", ... */
boolean_t	standalone;		/* running standalone mode ? */
security_token_t standalone_token;
boolean_t	norma_kernel;		/* running on a NORMA kernel ? */
boolean_t	need_banner = FALSE;	/* need to print the banner ? */
boolean_t	halt_opt;		/* hal option turned on */
int             silent = 0;		/* silent if zero */
boolean_t	filter;			/* filter runs */
double		filter_min;		/* low limit factor when filtering */
double		filter_max;		/* high limit factor when filtering */
void		machine_init();		/* machine dependent init */
boolean_t	overhead_opt;		/* time overhead option specified */
double		min_elapsed; 		/* time each measure must last */
double		max_elapsed_ratio;	/* min_elapsed <= measured_time
					 * <= min_elapsed*max_elapsed_ratio */
int             pass;                   /* exported index to current test */
int             sched_policy = -1 ;     /* scheduling policy */
int             base_pri = -1 ;         /* scheduling base priority */
int             max_pri = -1 ;          /* scheduling max priority */
int             quantum = -1 ;          /* RR scheduling quantum */

#define FILTER_DEFAULT 		5		/* 5 % */
#define MIN_VALID_TIME 		ONE_MS		/* min_elapsed minimum value */	
#define OLD_MIN_VALID_TIME 	(50 * ONE_MS)	/* with old timer */
#define MIN_LOOPS 		10		/* start value for loops */	
#define MAX_CALIBRATIONS 	20		/* maximum number of calibration
						   tentatives */
#define MAX_FILTER 		100		/* maximum number of runs that
						   we can filter out */
#define MAX_ELAPSED_RATIO	1.5		/* max_elapsed_ratio */

#undef tests
#undef private_options

null_func() {
}

reset_options()
{
	loops = 0;
	pass = 0;
	nruns = 3;	
	verbose = FALSE;	
	min_elapsed = 0.0; 	
	iopt = FALSE;		
	topt = FALSE;		
	max_loops = 0;
	min_loops = MIN_LOOPS;	
	max_elapsed_ratio = MAX_ELAPSED_RATIO;
	debug_all = 0;
	ndebugs = 0;
	ntraces = 0;
	debug_resources = 0;
	need_banner = TRUE;
	more_measures = server_time;
	start_test = start_server_test;
	stop_test = stop_server_test;
	no_cache = 0;
	vm_opt = FALSE;
	client_only = FALSE;
	server_only = FALSE;
	server_times = 0;
	server = MACH_PORT_NULL;
	server_task = MACH_PORT_NULL;
	use_opt = 0;
	norma_node = NORMA_NODE_NULL;
	norma_kernel = kernel_is_norma();
	prof_opt = FALSE;
	prof_trunc = 0;
	prof_print_routine = (void *)0;
#if     OLD_CLOCK
        use_timer = FALSE;
        timer = NULL_CLOCK;
#else
	use_timer = TRUE;
	timer = DEFAULT_CLOCK;
#endif  /* OLD_CLOCK */
	reset_time_stats(&server_stats);
	thread_stats = TRUE;
	overhead_opt = FALSE;
	timer_overhead = 0;
	filter = TRUE;
	filter_min = FILTER_DEFAULT;
	filter_min = 1 - filter_min/100;
	filter_max = FILTER_DEFAULT;
	filter_max = 1 + filter_max/100;
}

jmp_buf test_jmp_buf;
int skip_test;

#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x > y) ? x : y)

boolean_t
measure(test)
struct test *test;
{
	int 		calibrations = 0;
	int 		prev_loops = 0;
	int 		reversions = 0;	/* number of times calibrate()
					   decreased loops */
	int		filtered = 0;	/* number of times we filtered */
	double 		base = 0.0;	/* base time */
	double 		tt;		/* total time */
	double 		min_tt=9999999.0;	/* minimum total time */
	double 		max_tt=0.0;	/* maximum total time */
	int		low_count = 0;	/* low figure detection count */
	int		high_count = 0;	/* high figure detection count */

	min_loops = MIN_LOOPS;
	if (!iopt)
		loops = min_loops;
	max_loops = 0;
	base = -1.0;
	cur_traced_file = 0;
	resource_check_on = FALSE;
	_ignore_server = FALSE;
	reset_time_stats(&client_stats);
	reset_time_stats(&server_stats);
	prof_reset();
	server_in_same_task = FALSE;
	if (!silent) {
		if (verbose) {
			printf("\n");
			print_header(test);
			printf("\n");
		} else
		 	print_header(test);
	}

	/*
	 * loop a given number of times, or infinitely if a bg bench
         */
	for (pass = 0; pass < nruns || 
	               (background && (test_status != INIT_TIMER));) {
		    
	        if (!background) {
		    if (calibrations >= MAX_CALIBRATIONS) {
			if (!silent)
			    printf("Could not calibrate after %d runs, use -v\n",
				   calibrations);
			return FALSE;
		    }
		    if (filtered >= MAX_FILTER) {
			if (!silent)
			    printf("Already filtered out %d runs, use -v\n",
				   filtered);
			return FALSE;
		    }
		}
		client_stats.loops = loops;
		server_stats.loops = loops;
		prev_loops = loops;	       
		resources_start();
		if (verbose)
			printf("run %3d: %8d loops ", pass+1, loops);
		if (mach_setjmp(test_jmp_buf)) {
			skip_test = 0;
			printf(" test failed (use -v)\n");
			return FALSE;
		} else {
			skip_test = 1;
			(*start_test)();
			(*test->func)(test->arg0, test->arg1,
				      test->arg2, test->arg3);
			(*stop_test)();
			skip_test = 0;
		}
		resources_stop();

		/* XXX: Thread basic info still returns microseconds ! */
		fix_thread_time_stats(&client_stats);

		/* Old clock service returns microseconds */
		if (!use_timer)
			fix_elapsed_time_stat(&client_stats);

		if (verbose) {
			resources_use("Test ");
		}       

		/*
		 * if we're running a synthetic benchmark, call
		 * the synthetic filter routine to verify that
		 * the various foreground/background timings are legal
		 */
	        if (synthetic_fn && 
		    (synthetic_fn->filter) (&client_stats) != 0) {
		    if (test_status == CALIBRATING) {
		    	calibrations++;
			calibrate(test, (reversions > 1));
		        if (verbose)
				printf(" (C)  ");
			reversions++;
		    }
		    else {
		        if (verbose)
				printf(" (S)  ");
		    	filtered++;
		    }
		    if (verbose)
			print_one_sample(0, &client_stats);
		    prof_drop();
		    continue;
		}

		if (test_status == FIRST_RUN)
		    test_status = RUNNING;

		if (base < 0.0 && !run_is_ok()) {
		    	calibrations++;
			calibrate(test, (reversions > 1));
			if (loops < prev_loops) {
				if (verbose)
					printf(" (C)  ");
				reversions++;
			} else if (verbose)
				printf(" (c)  ");
			if (verbose)
				print_one_sample(0, &client_stats);
			prof_drop();
			continue;
		} else {
			calibrations = 0;
			reversions = 0;

			if (test_status == CALIBRATING)
			    test_status = FIRST_RUN;
		}

		tt = (total_time(&client_stats)-client_stats.timer_overhead);
		tt /= loops;
		min_tt = min(tt, min_tt);
		max_tt = max(tt, max_tt);

		if (tt < 0.0) {
			if (verbose) {
			  	printf(" (z)  ");
				print_one_sample(0, &client_stats);
			}
			prof_drop();
			filtered++;
			low_count = 0;
			high_count = 0;
			continue;
		}
		if (filter) {
 			double min_lim = filter_min * base;
 			double max_lim = filter_max * base;

			if (base < 0.0) {
			  	base = tt;
				low_count = 0;
				high_count = 0;
			} else if (tt < min_lim &&
				   (min_lim - tt >= 1 || 
				    filter_min == filter_max)) {
				if (verbose) {
					printf(" (f)  ");
					print_one_sample(0, &client_stats);
				}
				if (!low_count++) {
					prof_drop();
				} else {
					reset_time_stats(&client_stats);
					reset_time_stats(&server_stats);
					base = tt;
					pass = 0;
					prof_reset();
			  		low_count = 0;
				}
				filtered++;
				high_count = 0;
				continue;
			} else if (tt > max_lim &&
				   (tt - max_lim >= 1 || 
				    filter_min == filter_max)) {
				if (verbose) {
					printf(" (F)  ");
					print_one_sample(0, &client_stats);
				}
				if (high_count++ > 5) {
					reset_time_stats(&client_stats);
					reset_time_stats(&server_stats);
					base = tt;
					pass = 0;
					prof_reset();
			  		high_count = 0;
				} else {
					prof_drop();
				}
				filtered++;
			  	low_count = 0;
				continue;
			}
		}

		if (verbose) {
			printf("      ");
			print_one_sample(0, &client_stats);
		}
		collect_stats(&client_stats);
		prof_save();
		(*more_measures)(pass+1);
		pass++;
	}
	if (!silent) {
		if (verbose)
			printf("Average                       ");
		print_results(&client_stats);
		if (verbose)
			printf("Min : %.2f Max : %.2f\n", min_tt,max_tt);
	}
	resources_use("Test ");       
	if (!silent)
		(*more_measures)(0);
	prof_print();
	return(TRUE);
} 

int exit_in_progress;

test_exit(format, routine, file, line, error)
char *format, *routine, *file, *error;
{
	if (exit_in_progress)
		return;
	if (skip_test && !verbose)
		mach_longjmp(test_jmp_buf, 1);
	printf(format, routine, file, line, error);
	if (halt_opt) {
		debugger();
	}
	if (is_master_task) {
		if ((server != MACH_PORT_NULL) && !client_only) {
			if (debug)
				printf("kill_server(%x)\n", server);
			kill_server(server);
		}
		leave(1);
	}
	else
		task_terminate(mach_task_self());
}

test_failed(
	char 		*test,
	char 		*file,
	int 		line,
	char 		*reason)
{
	test_exit("%s() in %s at line %d: %s\n",
		  test, file, line, reason);
}

print_banner()
{
	if (silent || ((!prof_opt) && (!need_banner)))
		return;
	need_banner = FALSE;
	printf("%d run(s), ", nruns);
	if (!iopt) {
		printf("minimum elapsed time per run %.3f ms\n\n",
		       min_elapsed/ONE_MS);
	} else {
	  	printf("%d iterations per run\n\n", loops);
	}

	printf("test name                     nops/sec  Elapsed     User   System  +/- %%  Total\n");
	printf("                                         us/op     us/op    us/op          ms\n\n");

}

char *strncpy();

print_header(test)
struct test *test;
{
	char name[31], len;

	print_banner();
	strncpy(name, test->name, 31);
	if ((len = strlen(test->name)) < 30)
	while (len < 30)
		name[len++] = ' ';
	name[30] = 0;
	printf("%s", name);
}

print_results(ts)
struct time_stats *ts;
{
	double avg;
	double dev;

	if (ts == &server_stats) {
		if (!thread_stats)
			return;
		printf("                  %8.2f %8.2f\n",
		       average(&ts->user_per_op),
		       average(&ts->system_per_op));
		return;
	}
	avg = average(&ts->total_per_op);
	dev = deviation(&ts->total_per_op)*100/average(&ts->total_per_op);
	if (dev < 0.005) /* Otherwise standalone printf formats incorrectly */
		dev = 0.0;
	printf("%8.0f", ONE_SEC/avg);
	printf(" %8.2f %8.2f %8.2f %5.2f %7.1f\n",
	       average(&ts->total_per_op),
	       average(&ts->user_per_op),
	       average(&ts->system_per_op),
	       deviation(&ts->total_per_op)*100/average(&ts->total_per_op),
	       ts->total.xsum/ONE_MS);
}

print_one_sample(test, ts) 
struct test *test;
struct time_stats *ts;
{
	double tot;

	if (ts == &server_stats) {
		if (!thread_stats)
			return;
		if (test && !silent) {
		  	boolean_t saved_prof_opt = prof_opt;
			prof_opt = FALSE;
			print_header(test);
			prof_opt = saved_prof_opt;
		}
		printf("                  %8.2f %8.2f\n",
		       user_time(ts)/ts->loops,
		       system_time(ts)/ts->loops);
		return;
	}
	if (test && !silent)
	       	print_header(test);
	tot = (total_time(ts)/ONE_SEC);
	printf("%8.0f", ts->loops/tot);
	printf(" %8.2f %8.2f %8.2f       %7.1f\n",
	       (total_time(ts)-ts->timer_overhead)/ts->loops,
	       user_time(ts)/ts->loops,
	       system_time(ts)/ts->loops,
	       total_time(ts)/ONE_MS);
}

calibrate(test, smooth)
struct test *test;
boolean_t smooth;
{
	double new_loops = loops;
	struct time_stats *ts = &client_stats;
	double total = total_time(ts);

	if (total < min_elapsed/20) /* get meaningful relative time 5 % */
		new_loops *= 10;
	else if (total < min_elapsed)
		if (smooth)
			new_loops += new_loops * 
			             ((min_elapsed / total_time(ts)) - 1) *
				     0.5 ;
		else
			new_loops *= min_elapsed * 1.05 / total_time(ts);
	else
		new_loops *= min_elapsed * 0.95 / total_time(ts);
	if (max_loops && (new_loops > max_loops))
		new_loops = max_loops;
	if (new_loops < min_loops)
		new_loops = min_loops;
	loops = new_loops;
}

char *
get_basename(name)
char *name;
{
	char *cn = name; 

	while (*name)
		if (*name++ == '/')
			cn = name;
	return(cn);
}


is_gen_opt(argc, argv, index, tests, private_options)	
char *argv[];
int argc, *index;
struct test *tests;
char *private_options;
{
	int i,j;
	int ret = 1;

	command_name = get_basename(argv[0]);

	i = *index;

	if (debug > 3)
		printf("token %s\n", argv[i]);
	if (i+1 < argc && debug)
		printf("token+1 %s\n", argv[i+1]);

	if (!strcmp(argv[i], "-debug_resources")) {
		debug_resources = 1;
		
	} else if ((!strcmp(argv[i], "-d")) || (!strcmp(argv[i], "-debug"))) {
		int level;

		if (i+1 >= argc || *argv[i+1] == '-')
			debug_all++;
		else if (atod(argv[++i], &level))
			debug_all = level;
		else if (i+1 >= argc || *argv[i+1] == '-')
			add_debug_string(argv[i], 1);
		else if (!atod(argv[++i], &level)) {
			if (tests)
				usage();
		} else
			add_debug_string(argv[i-1], level);
	} else if ((!strcmp(argv[i], "-tr")) || (!strcmp(argv[i], "-trace"))) {
		if (i+1 >= argc || *argv[i+1] == '-') {
			trace_all++;
			ntraces++;
		} else 
			add_trace_string(argv[++i]);
	} else if (!tests) {
		/* skip other options */
	} else if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "-loops"))) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		iopt = TRUE;
		if (!atod(argv[i], &loops))
			usage();
		if (loops <= MIN_LOOPS)
			thread_stats = FALSE;
	} else if (!strcmp(argv[i], "-ii")) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (!atod(argv[i], &min_loops))
			usage();
	} else if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "-runs"))) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (!atod(argv[i], &nruns))
			usage();
	} else if (!strcmp(argv[i], "-pri")) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (!atod(argv[i], &base_pri))
			usage();
	} else if  (!strcmp(argv[i], "-max_pri")) {
		if (++i >= argc || *argv[i] == '-')
		  	usage();
		if (!atod(argv[i], &max_pri))
			usage();
	} else if (!strcmp(argv[i], "-policy")) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (!strcmp(argv[i], "FIFO")) {
			sched_policy = POLICY_FIFO;
		} else if (!strcmp(argv[i], "RR")) {
			sched_policy = POLICY_RR;
		} else if (!strcmp(argv[i], "TS")) {
			sched_policy = POLICY_TIMESHARE;
	  	} else if (!atod(argv[i], &sched_policy)) {
			usage();
		}
	} else if (!strcmp(argv[i], "-quantum")) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (!atod(argv[i], &quantum))
			usage();
	} else if ((!strcmp(argv[i], "-v")) || (!strcmp(argv[i], "-verbose"))) {
		verbose = TRUE;
	} else if ((!strcmp(argv[i], "-z")) || (!strcmp(argv[i], "-zones"))) {
		zone_debug = TRUE;
	} else if ((!strcmp(argv[i], "-filter")) || (!strcmp(argv[i], "-f"))) {
	  	int value;
		filter = TRUE;
		if (!(i+1 >= argc || *argv[i+1] == '-'))
			if (!atod(argv[++i], &value))
				usage();
			else {
				filter_min = value;
				filter_min = 1 - filter_min/100;
				filter_max = value;
				filter_max = 1 + filter_max/100;
			}	
	} else if (!strcmp(argv[i], "-vm_stats")) {
		vm_opt = TRUE;
	} else if (!strcmp(argv[i], "-overhead")) {
	  	overhead_opt = TRUE;
		if (i+1 >= argc || *argv[i+1] == '-')
			usage();
		else if (!atod(argv[++i], &timer_overhead))
			usage();
	} else if (!strcmp(argv[i], "-timer")) {
		if (i+1 >= argc || *argv[i+1] == '-')
			usage();
		else if (!atod(argv[++i], &timer))
			usage();
		else if (timer > USER_CLOCK)
		  	usage();
		use_timer = TRUE;
	} else if (!strcmp(argv[i], "-min")) {
		if (++i >= argc || *argv[i] == '-' || iopt)
			usage();
		if (!atod(argv[i], &j))
			usage();
		min_elapsed = j * ONE_MS;
		if (min_elapsed < MIN_VALID_TIME && !use_timer) {
			printf("min_elapsed must be at least %d ms\n",
			       OLD_MIN_VALID_TIME/ONE_MS);
			leave(1);
		} else if (min_elapsed < MIN_VALID_TIME) {
			printf("min_elapsed must be at least %d ms\n",
			       MIN_VALID_TIME/ONE_MS);
			leave(1);
		}
	} else if (!strcmp(argv[i], "-prof")) {
		prof_opt = TRUE;
		if (standalone) {
			extern void mach_prof_print();
			prof_print_routine = mach_prof_print;
			if (i+1 < argc) {
				if ((!strcmp(argv[++i], "-trunc")) &&
				    i+1 < argc &&
				    atod(argv[++i], &j))
					prof_trunc = j;
				else
					usage();
			}
		} else {
			extern void ux_prof_print();
			prof_print_routine = ux_prof_print;
			strcpy(prof_command, "mprof -print -");
			while(i+1 < argc) {
				strcat(prof_command, " ");
				strcat(prof_command, argv[++i]);
			}
		}
	} else if (!strcmp(argv[i], "-no_cache")) {
		no_cache++;
	} else if (!strcmp(argv[i], "-no_thread_stats")) {
		thread_stats = FALSE;
	} else if (!(strcmp(argv[i], "-l") && strcmp(argv[i], "-list"))) {
		show_tests(tests);
		leave(1);
	} else if (!strcmp(argv[i], "-prof_unit")) {
		extern int prof_alloc_unit;
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (!atod(argv[i], &prof_alloc_unit))
			usage();
	} else if ((!strcmp(argv[i], "-t")) || (!strcmp(argv[i], "-test"))) {
		if (++i >= argc || *argv[i] == '-')
			usage();
		if (enable_test(argv[i], tests))
			topt= TRUE;
		else {
			printf("Invalid test name\n");
			show_tests(tests);
		}
        } else if ((!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "-halt"))) {
	  	halt_opt = 1;
		debugger();
	} else if ((!strcmp(argv[i], "-tn")) || (!strcmp(argv[i], "-test#"))) {
		if (i+1 >= argc || *argv[i+1] == '-')
			usage();
		while (i+1 < argc && *argv[i+1] != '-') {
			int tn;

			i++;
			if (!atod(argv[i], &tn))
				usage();
			if (enable_test_number(tn, tests))
				topt = TRUE;
			else {
				printf("Invalid test number\n");
				show_tests(tests);
			}
		}
	} else if ((!strcmp(argv[i], "-nt")) || (!strcmp(argv[i], "-no_test#"))) {
		if (i+1 >= argc || *argv[i+1] == '-')
			usage();
		while (i+1 < argc && *argv[i+1] != '-') {
			int tn;

			i++;
			enable_all_tests(tests);
			if (!atod(argv[i], &tn))
				usage();
			if (disable_test_number(tn, tests))
				topt = TRUE;
			else {
				printf("Invalid test number\n");
				show_tests(tests);
			}
		}
	} else
		ret = 0;
	*index = i;
	return(ret);
}

show_tests(tests)
struct test *tests;
{
	int i = 0;

	enable_more();
	printf("tests are:\n\n");
	while (tests->name) {
		if (tests->func) 
			printf("\ttest %3d\t%s\n", i++, tests->name);
		else if (strlen(tests->name))
			printf("\n\t%s\n\n", tests->name);
		else
			printf("\n");
		tests++;
	}
	printf("\n");
}

enable_test(test, tests)
char *test;
struct test *tests;
{
	int ret = 0;
	while (tests->name) {
		if (tests->func &&
		    !strncmp(test, tests->name, strlen(test))) {
			tests->enabled++;
			ret++;
		}
		tests++;
	}
	return(ret);
}

enable_test_number(n, tests)
struct test *tests;
{
	int ret = 0;
	int i = 0;

	while (tests->name) {
		if (tests->func && n == i++) {
			tests->enabled++;
			return(1);
		}
		tests++;
	}
	return(0);
}

disable_test_number(n, tests)
struct test *tests;
{
	int ret = 0;
	int i = 0;

	while (tests->name) {
		if (tests->func && n == i++) {
			tests->enabled = 0;
			return(1);
		}
		tests++;
	}
	return(0);
}

enable_all_tests(tests)
struct test *tests;
{
	while (tests->name) {
		if (tests->func) 
			tests->enabled++;
		tests++;
	}
}

boolean_t measuring_timer;

run_tests(tests)
struct test *tests;
{
	char *title = 0;
	int failures = 0;

	need_banner = TRUE;
	if (min_elapsed == 0.0 && (prof_opt || !use_timer))
		min_elapsed =  (1 * ONE_SEC);

	update_policy(!silent);

	if (!measuring_timer) {
	  	measuring_timer = TRUE;
		MACH_CALL(init_timer, (timer));
		measuring_timer = FALSE;
		if (!prof_opt)
			print_banner();
	} 

	prof_init();
	while (tests->name) {
		if (!tests->func)
			title = tests->name;
		else if (tests->enabled || !topt) {
			if (title && (!silent)) {
				if (strlen(title))
					printf("\n%s\n\n", title);
				else
					printf("\n");
				title = 0;
			}
			tests->enabled = 0;
			test_status = (measuring_timer ? INIT_TIMER
				                       : CALIBRATING);
			if (!measure(tests))
				failures++;
		}
		tests++;
	}
	if (!silent)
		printf("\n");
	print_vm_stats();
	prof_terminate();
	if (failures && !silent)
		printf("%d failure%s\n\n", failures, (failures > 1)?"s":"");
	return(failures);
}

char *gen_opts = "\
\t[-min <milli_secs>]      Set minimum time for each run\n\
\t                         (exclusive with -i).\n\
\t[-runs <runs>]           Set number of runs (default is 3).\n\
\t                         alias: -r\n\
\t[-loops <iterations>]    Set number of iterations per run\n\
\t                         (exclusive with -min).\n\
\t                         alias: -i\n\
\t[-test <test_name> ]     Run tests for which name\n\
\t                         starts with <test_name>.\n\
\t                         alias: -t\n\
\t[-test# <test_id>]       Run tests number <test_id>.\n\
\t                         alias: -tn\n\
\t[-no_test# <test_id>]    Do not run tests number <test_id>.\n\
\t                         alias: -nt\n\
\t[-timer <timer_id>]      Use timer <timer_id>\n\
\t                         0: Realtime clock\n\
\t                         1: Battery clock\n\
\t                         2: High resolution clock\n\
\t                         3: User provided clock\n"\
"\t[-pri <base pri>]        Set base priority to <base pri>\n\
\t[-max_pri <max pri>]     Set max priority to <max pri>\n\
\t[-quantum <quantum>]     Set quantum to <quantum> milliseconds\n\
\t[-policy <policy>]       Set policy to <policy>\n\
\t                         1 or TS: POLICY_TIMESHARE\n\
\t                         2 or RR: POLICY_RR\n\
\t                         4 or FIFO: POLICY_FIFO\n"\
"\t[-vm_stats]              Print vm statistics.\n\
\t[-no_thread_stats]       Disable per thread time statistics.\n\
\t[-filter [deviation]]    Eliminate runs for which elapsed time\n\
\t                         deviates more than <deviation> %\n\
\t                         (default is 5 %).\n\
\t                         alias: -f\n\
\t[-verbose]               Verbose mode: print results for all runs\n\
\t                         and print extended status in case of failures.\n\
\t                         alias: -v\n\
\t[-debug [name] [level]   Set debug level to <level> for modules\n\
\t                         which source file name contains string <name>\n\
\t                         If <name> is not specified <level> applies\n\
\t                         to all modules. Default <level> is 1. \n\
\t                         alias: -d\n"\
"\t[-trace [name]           Trace microkernel calls for modules which \n\
\t                         source file name contains string <name>.\n\
\t                         If <name> is not specified all modules are\n\
\t                         traced.\n\
\t[-debug_resources]       For traced microkernel calls, print resource\n\
\t                         consumptions (ipc & vm space).\n\
\t[-zones]                 Show microkernel zones consumptions.\n\
\t                         alias: -z\n\
\t[-list]                  Print tests list.\n\
\t                         alias: -l\n\
\t[-halt]                  halt: at start time suspend thread or enter\n\
\t                         kernel debugger if running standalone.\n\
\t                         alias: -h\n\
\t[-prof [<mprof args>]]   Profile microkernel task. When running\n\
\t                         standalone, -trunc <val> is the only\n\
\t                         meaningful profile argument.\n\
";

#if 0

\t[-old_clock]             Use the obsolete host_get_time() \n\
\t                         interface instead of timers.\n\
\t[-ii <iterations>]       Start test calibration with <iterations>\n\
\t                         iterations (default is 10).\n\
\t[-no_cache]              Disables caching\n\
\t                         (some tests are executed at start an extra\n\
\t                         time to remove noise from kernel allocating\n\
\t                         large tables like ipc splay trees)\n\

#endif


print_usage(tests, private_options)
struct test *tests;
char *private_options;
{
	enable_more();
	printf("%s	", command_name);
	if (private_options && strlen(private_options))
		printf("%s\n", private_options);
	printf("%s\n", gen_opts);
/*	show_tests(tests); */
	leave(1);
}

more_test_init()
{
	vm_offset_t addr = 0;

	if (!standalone)
		exception_init();
	if (is_master_task && (!standalone) && (!server_only) ) {
		disable_more();
		version();
		kernel_version();
		print_vm_stats();
	}
	if (vm_allocate(mach_task_self(),
		       &addr,
		       page_size_for_vm,
		       FALSE) == KERN_SUCCESS)
		MACH_CALL(vm_protect, (mach_task_self(),
				       addr,
				       page_size_for_vm,
				       FALSE,
				       VM_PROT_NONE));
	time_init();
	machine_init();
}

/* 
 * Are we running standalone.
 * If thread EXC_SYSCALL port is set, assume non standalone mode
 */

boolean_t
is_standalone()
{
#ifdef	PURE_MACH
	return TRUE;
#else	/* PURE_MACH */
	mach_port_t thread_exc_port;
	exception_mask_t mask;
	unsigned int exc_ports_count=1;
	exception_behavior_t thread_exc_behavior;
	thread_state_flavor_t thread_exc_flavor;

	MACH_CALL( thread_get_exception_ports,
		  (mach_thread_self(),
		   EXC_MASK_SYSCALL,
		   &mask,
		   &exc_ports_count,
		   &thread_exc_port,
		   &thread_exc_behavior,
		   &thread_exc_flavor));

	if (thread_exc_port == MACH_PORT_NULL)
		return(TRUE);
	MACH_CALL( mach_port_deallocate, (mach_task_self(),
					  thread_exc_port));
	return(FALSE);
#endif	/* PURE_MACH */
}

atod(s, i)
char *s;
int *i;
{
	char c;

	*i = 0;
	while(c = *s++) {
		if (c >= '0' && c <= '9')
			*i = (*i)*10 + c - '0';
		else
			return(0);
	}
	return(1);			
}

print_command_line(argc, argv)
char *argv[];
{
	register i;

	if (!background) {
	    printf("\n");
	    printf("%s ", get_basename(argv[0]));
	    for (i=1; i < argc; i++) {
		printf("%s ", argv[i]);
	    }
	    printf("\n\n");
	}
}

kernel_version()
{
	kernel_version_t version;

	MACH_CALL(host_kernel_version, (mach_host_self(),
					&version[0]));
	printf("\n%s\n", version);
}

version()
{
	printf("Mach Microkernel benchmarks version %s\n\n", version_string);
}

leave(status)
{
	mach_thread_t thread = printf_thread;

	if (debug > 1)
		printf("leave(%d)\n", status);
	debug_all = 0;
	ndebugs = 0;
	exit_in_progress = 1;
	if ((!standalone) && thread && thread_self() != thread) {
		printf_thread = (mach_thread_t) 0;
		kill_thread(thread);
	}
	/*
	 * Since we exit the test suite,
	 * we still need to accomplish test_end() here
	 * to inform the bootstrap of our work being finished.
	 */
	if (is_master_task && standalone && !server_only) {
		(void)bootstrap_completed(bootstrap_port,
				    mach_task_self());
	}
	exit(status);
}

boolean_t
kernel_is_norma(void)
{
  	int *my_node;
	if (norma_node_self(host_port, &my_node) == KERN_SUCCESS)
		return 1;
	return 0;
}

#if PURE_MACH

void
get_args(
	int *argc,
	char ***argv) {
}

#else /* PURE_MACH */

static void
__setup_ptrs(
	vm_offset_t strs,
	vm_size_t strs_size,
	char ***ptrs_ptr,
	int *nptrs_ptr)
{
	kern_return_t kr;
	vm_offset_t ptrs;
	int nptrs;
	char *ptr;
	char *endptr;
	int i;

	nptrs = 0;
	ptr = (char *)strs;
	endptr = ptr + strs_size;
	while (ptr < endptr) {
		nptrs++;
		while (*ptr++)
			;
	}
	kr = vm_allocate(mach_task_self(), &ptrs, (nptrs+1) * sizeof(char *),
			 TRUE);
	if (kr != KERN_SUCCESS) {
		vm_deallocate(mach_task_self(), strs, strs_size);
		return;
	}
	ptr = (char *)strs;
	*ptrs_ptr = (char **)ptrs;
	*nptrs_ptr = nptrs;
	for (i = 0; i < nptrs; i++) {
		(*ptrs_ptr)[i] = ptr;
		while (*ptr++)
			;
	}
}

/*
 * Get bootstrap arguments. When running in standalone mode
 * and PURE_MACH is disbaled.
 * In this case the crt0 is the libc one and it fails retrieving
 * argc/argv.
 */

void
get_args(
	int *argc,
	char ***argv)
{
	kern_return_t kr;
	vm_offset_t arguments;
	mach_msg_type_number_t arguments_size;

	if (!standalone)
		return;

	kr = bootstrap_arguments(bootstrap_port,
				 mach_task_self(),
				 &arguments,
				 &arguments_size);
	if (kr != KERN_SUCCESS || arguments_size == 0)
		return;
	__setup_ptrs(arguments, arguments_size, argv, argc);
}

#endif /* PURE_MACH */
