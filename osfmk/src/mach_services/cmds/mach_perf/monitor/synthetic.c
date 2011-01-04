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
/*
 * HISTORY
 * Revision 1.1.13.1  1996/03/27  13:24:04  yp
 * 	Bg_comms_port is now passed during fork() via a message interface
 * 	and not anymore by mach_port_insert_right.
 * 	[96/03/08            yp]
 * 
 * Revision 1.1.5.2  1996/01/25  11:33:33  bernadat
 * 	Invoke synthetic_init() when entering main().
 * 	Use policy management services from lib.
 * 	[95/12/22            bernadat]
 * 
 * Revision 1.1.5.1  1995/04/12  12:28:05  madhu
 * 	Reintroduced sched_set_attr with changes.
 * 	[1995/04/12  11:30:28  madhu]
 * 
 * 	copyright marker not _FREE
 * 
 * Revision 1.1.3.2  1995/01/11  10:34:46  madhu
 * 	Removed for the time being, scheduling policy changes from the user
 * 	interface. Many tweaks and twiddles in the area of foreground/background
 * 	synchronization.
 * 	[1995/01/11  10:10:08  madhu]
 * 
 * Revision 1.1.3.1  1994/12/09  14:03:33  madhu
 * 	changes by Nick for synthetic tests.
 * 	[1994/12/09  13:49:14  madhu]
 * 
 */

/* This file deals with the execution of multiple benchmarks in parallel,
 * in order to try and measure effects of different types of kernel
 * loading upon a given benchmark. Multiple benchmarks are executed,
 * there being a single foreground test which is measured whilst all
 * the other tests are executing.
 */

#define TEST_NAME synthetic

#include <mach_perf.h>

extern struct test_dir *test_dirs[];

/* Some variables used internally */

int                     bg_tasks = 0;
task_port_t             bg_task[MAX_BACKGROUND_TASKS];
extern mach_port_t	bg_comms_port;
struct host_sched_info  sched_info;

/* function prototypes */

void synthetic_start_time();
void wait_for_backgrounds();
int  synthetic_filter();

void fg(int (*func)(), int argc, char *argv[],int proc_argc,char *proc_argv[]);
void bg(int (*func)(), int argc, char *argv[],int proc_argc,char *proc_argv[]);

/* a pointer is set to this vector when running synthetic benchmarks.*/

sync_vec synthetic_fn_vec = { synthetic_start_time, synthetic_filter };

void synthetic_init(void)
{
    /*
     * the only initialisation we need to do is to switch
     * to RR scheduling with the smallest quantum possible.
     * this could probably be tidied up
     */
     if (sched_policy == -1) {
     	sched_policy = POLICY_RR;
     	quantum = 0;
     	update_policy(TRUE);
     	sched_policy = -1;	/* POLICY_RR is now the default */
     	quantum = -1;
     } else {
     	update_policy(TRUE);
     }

}

/*
 * this is the synthetic benchmark routine, called in the
 * eventuality that the command line isn't recognised
 */

int main(int argc, char *argv[])
{
    register                	i, j;
    int                     	found;
    mach_msg_type_number_t  	count;
    struct test_dir         	*td, **tds;
    mach_port_t             	saved_test_task;
    proc_vec		    	*procp;
    cmd_vec		    	*cmdp;

    /*
     * firstly check if the given command is a known proc
     */

    if ((procp = find_proc(argv[0])) == NULL) {
	test_error("synthetic","unknown proc");
    }

    synthetic_init();

    /* we're already in a child process. we need to do a very similar
     * thing to what was done in the sa.c, ie find the command and
     * spawn off each background task. They communicate back.
     */

    bg_tasks = 0;

    /*
     * synthetic_sync is a pointer which is only initialised when doing
     * a synthetic benchmark
     */
    synthetic_fn = &synthetic_fn_vec;

    /*
     * Store scheduling info in a static struct for quick reference
     */
    count = sizeof(sched_info);
    MACH_CALL(host_info,(mach_host_self(),
			 HOST_SCHED_INFO,
			 (host_info_t) &sched_info,
	                 &count));

    /* Create a port for communications between the various tasks */
    MACH_CALL( mach_port_allocate, (mach_task_self(),
				    MACH_PORT_RIGHT_RECEIVE,
				    &bg_comms_port));
    MACH_CALL( mach_port_insert_right, (mach_task_self(),
					bg_comms_port,
					bg_comms_port,
					MACH_MSG_TYPE_MAKE_SEND));
    if (debug > 1)
	printf("created bg_comms port %x\n",bg_comms_port);

    /*
     * just for the moment we cheat for the fg command taking it from
     *  the command line
     */

    MACH_FUNC (saved_test_task, mach_thread_self, ()); /* for debug */

    /*
     * launch all the background tasks and let them auto-calibrate
     * one at a time. The first test is the foreground test so skip
     * over that
     */

    if (procp->cmdhead == NULL)
	test_error("synthetic","proc must include at least one test");

    for (cmdp = procp->cmdhead->next; cmdp != NULL; cmdp = cmdp->next) {
        for (found = FALSE, tds = test_dirs; *tds && !found; tds++) {
	    for (td = *tds; td->name && !found; td++) {
		if (!strcmp(cmdp->argv[0],td->name)) {
		    if (td->is_a_test) {
			bg(td->func,
			   cmdp->argc,
			   cmdp->argv,
			   argc,argv);
			found = TRUE;
		    }
		}
	    }
	}
        if (!found) {
	    printf("synthetic benchmark : unrecognised test #%d : %s\n",
		   i, cmdp->argv[0]);
	    test_error("synthetic","");
	}
    }

    /* now we've got all the background tasks going, suspended just
     * before their main tests. We can worry about the foreground
     * task now
     */
    
    printf("calibration of background tests completed, running foreground test\n");

    cmdp = procp->cmdhead;
    for (found = FALSE, tds = test_dirs; *tds && !found; tds++)
      for (td = *tds; td->name && !found; td++)
        if (!strcmp(cmdp->argv[0],td->name)) {
            if (td->is_a_test) {
                found = TRUE;
		break;
            }
        }
    if (!found) {
	printf("synthetic benchmark : unrecognised foreground test : %s\n",
	       cmdp->argv[0]);
	test_error("synthetic","");
    }

    fg(*td->func, cmdp->argc,cmdp->argv,argc,argv);

    /*
     * We only do our own cleanup here, assuming that the
     * tests clean up for themselves. Any resource
     * leak will be caught by interruptible_cmd anyway
     */
       
    wait_for_backgrounds();
    for (i=0; i< bg_tasks; i++) {
	MACH_CALL( task_terminate, (bg_task[i]));
    }
    MACH_CALL (mach_port_deallocate, (mach_task_self(),
				      bg_comms_port));
    bg_comms_port = MACH_PORT_NULL;
    return 0;
}

/*
 * this routine deschedules the current thread for a given time
 */

void nap(int sec, int ns)
{
    mach_port_t clock;
    tvalspec_t              time = {0, 0};
    tvalspec_t              wakeup;
    MACH_CALL(host_get_clock_service, (mach_host_self(),
				       REALTIME_CLOCK,
				       &clock));
    time.tv_sec = sec;
    time.tv_nsec  = ns;

    MACH_CALL(clock_sleep, (clock, TIME_RELATIVE, time, &wakeup));
}

void fg(int (*func)(), int argc, char *argv[],int proc_argc,char *proc_argv[])
{
    int i;

    cmd_vec	cmd;
    
    /* construct a new command line including the arguments from the proc */
    cmd.argc = argc+proc_argc-1;

    if (cmd.argc > MAX_ARGS) {
	test_error("synthetic","too many arguments for test\n");
    }
    for (i = 0; i<argc; i++)
	cmd.argv[i] = argv[i];
    for (i = 1; i<proc_argc; i++)
	cmd.argv[argc+i-1] = proc_argv[i];

    /* Analyse the foreground task's arguments */
    for (i = 1; i < cmd.argc; i++)
        is_gen_opt(cmd.argc, cmd.argv, &i, 0, 0);
    /* and run the benchmark */
    (func) (cmd.argc,cmd.argv);
}

void bg(int (*func)(), int argc, char *argv[],int proc_argc,char *proc_argv[])
{
    int         i, j;
    kern_return_t kr;
    boolean_t   waiting_for_task;
    task_basic_info_data_t info;
    mach_msg_type_number_t count;
    mach_port_t bootstrap_port;

    cmd_vec	cmd;
    
    /* construct a new command line including the arguments from the proc */
    cmd.argc = argc+proc_argc-1;

    if (cmd.argc > MAX_ARGS) {
	test_error("synthetic","too many arguments for test\n");
    }
    for (i = 0; i<argc; i++)
	cmd.argv[i] = argv[i];
    for (i = 1; i<proc_argc; i++)
	cmd.argv[argc+i-1] = proc_argv[i];

    /* fork off the background task */

    if ((bg_task[bg_tasks] = mach_fork())) {
	/*
	 * The child task received the right to send to the comms port
	 * at fork time (mach_fork() supra).
	 */
	printf("calibration of background test number %d :",bg_tasks+1);
	for (i=0; i<cmd.argc; i++)
	    printf(" %s",cmd.argv[i]);
	printf("\n");
	MACH_CALL(task_resume,(bg_task[bg_tasks]));

	bg_tasks++;
	 
	/*
	 * Wait for an asynchronous reply from the background task
	 * to indicate that the task has calibrated itself and is ready
	 * to run.
	 */

	WAIT_REPLY(bg_server, bg_comms_port, BG_DATA_SIZE);
    } else {
	/* We have to do a little bit of initialisation for child tasks */
	mach_port_t root_ledger_paged;
	mach_port_t root_ledger_wired;
	mach_port_t security_port;


        MACH_CALL(task_get_special_port, (mach_task_self(),
                                       TASK_BOOTSTRAP_PORT,
                                       &bootstrap_port));
	MACH_CALL(bootstrap_ports, (bootstrap_port,
					&privileged_host_port,
					&master_device_port,
					&root_ledger_wired,
					&root_ledger_paged,
					&security_port));

	MACH_CALL(task_suspend,(mach_task_self()));

	/* Analyse the background task's arguments */
	for (i = 1; i < cmd.argc; i++)
	    is_gen_opt(cmd.argc, cmd.argv, &i, 0, 0);
	silent = TRUE;
	background = bg_tasks+1;

	/* the test, if running in the background, should never terminate */

	(func) (cmd.argc,cmd.argv);

	printf("synthetic benchmark : unexpected termination of background test #%d\n",background);
	MACH_CALL( task_terminate, (mach_task_self())); 
    }
}

/*
 * In order to measure synthetic benchmarks accurately, it is necessary
 * to ensure that all the benches get launched simultaneously. This
 * function is called in the start_time macro by all the various
 * benchmark tasks being called, and it is responsible for
 * the initial synchronisation between the tasks.
 */

void synthetic_start_time(void)
{
    int i;

    if (background) {
	/*
	 * Only start synchronising when we're onto useful tests
         */

	if (test_status == RUNNING) {
	    
	    task_suspend(mach_task_self());
	}
    } else {
	if (test_status != INIT_TIMER) {
		wait_for_backgrounds() ;
	    /*
	     * once we know they are all suspended, we can restart them
	     * all as simultaneously as possible
	     */
	       
	    for (i=0; i < bg_tasks; i++) {
		MACH_CALL(task_resume,(bg_task[i]));
	    }
	    /*
	     * give a chance for the other tasks to start their work,
	     * this should give them a couple of scheduler slices each
	     * to give them time to get into their timed loops.
	     */
	    /* nap(0, (bg_tasks * 9900 * NSEC_PER_USEC)); */

	    MACH_CALL(thread_switch, (MACH_PORT_NULL,
			  SWITCH_OPTION_DEPRESS,
			  bg_tasks * sched_info.min_quantum));

	}
    }
}

/*
 * This function is called in the main test loop when filtering out bogus
 * results. It is responsible for the sending of the timing data from the
 * background tasks to the foreground, and for the foreground task to
 * collate all of this data and decide whether or not to accept the
 * test as valid
 */

struct time_stats 	synthetic_filter_bg_timings;

int synthetic_filter(struct time_stats *ts)
{
    int i;
    int synthetic_filter_reject_count = 0;


    if (background) {
	/* only send results when we're measuring real results */
	if (test_status >= FIRST_RUN) {
	    /*
	     * Background tasks merely report their time and
	     * timer overhead to the foreground task
	     */
	    MACH_CALL(send_bg_timings, (bg_comms_port,
					background,
					ts->before.timeval,
					ts->after.timeval,
					ts->timer_overhead));
	}
	return 0;		/* background tests never get filtered */
    } else {
	if (test_status != INIT_TIMER) {
	    /*
	     * The foreground task has to collect all the
	     * background timings and make a decision on whether
	     * or not to filter out this pass.
	     */
	    for (i = 0; i < bg_tasks; i++) {
		WAIT_REPLY(bg_server, bg_comms_port, BG_DATA_SIZE);

		/*
		 * reject the test if a background test started after
		 * or finished before the foreground test
		 */
		if ((elapsed(&ts->before.timeval,
			     &synthetic_filter_bg_timings.before.timeval)
		     > 0.0) ||
		    (elapsed(&ts->after.timeval,
			     &synthetic_filter_bg_timings.after.timeval)
		     < 0.0)) {

		    synthetic_filter_reject_count++;
		    if (debug) {
		       printf("run synchronisation lost!\n");
		       printf("fg before %8d %8d  after %8d %8d\n",
		       ts->before.timeval.tv_sec,
		       ts->before.timeval.tv_nsec,
		       ts->after.timeval.tv_sec,
		       ts->after.timeval.tv_nsec);
		       printf("bg before %8d %8d  after %8d %8d\n",
		       synthetic_filter_bg_timings.before.timeval.tv_sec,
		       synthetic_filter_bg_timings.before.timeval.tv_nsec,
		       synthetic_filter_bg_timings.after.timeval.tv_sec,
		       synthetic_filter_bg_timings.after.timeval.tv_nsec);
		   }
		}
	    }
	    return synthetic_filter_reject_count;
	}
    }
    return 0; /* zero indicates that the filter doesn't reject the results */
}

void get_bg_timings(mach_port_t port,
		    int		background,
		    tvalspec_t	before,
		    tvalspec_t	after,
		    int		overhead)
{
    /*
     * this routine is called remotely via a mig stub with
     * the background benchmarks' timings
     */
    bcopy((char*)&before,
	  (char*)&synthetic_filter_bg_timings.before.timeval,
	  sizeof(tvalspec_t));
    bcopy((char*)&after,
	  (char*)&synthetic_filter_bg_timings.after.timeval,
	  sizeof(tvalspec_t));
    synthetic_filter_bg_timings.timer_overhead = overhead;
}



void
wait_for_backgrounds()
{
    int i;
    int tasks_awake = 0;
    kern_return_t kr;
    task_basic_info_data_t info;
    mach_msg_type_number_t count;

	    do {
		tasks_awake = 0;
		for (i=0 ; i < bg_tasks; i++) {
		    if (debug > 1)
			printf("fg SYNC with bg #%d\n",i+1);
		    /*
		     * the background tests should all be suspended
		     * (task_suspended). There is a critical region
		     * between the ipc from the task and its
		     * suspension which is catered for by verifying
		     * that all the tasks are indeed suspended
		     */
		    count = sizeof(task_basic_info_data_t);
		    kr = task_info(bg_task[i], TASK_BASIC_INFO,
				   (task_info_t) &info, &count);
		
		    if (info.suspend_count == 0) {
			tasks_awake++;
			if (debug)
			    printf("fg SYNC has to wait for bg task\n");
			nap(0,10 * ONE_MS); /* 10 ms, give other tasks a go */
		    }
		}
	    } while (tasks_awake);
}
