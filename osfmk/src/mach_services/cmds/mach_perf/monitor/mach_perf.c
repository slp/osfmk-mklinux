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
 * OLD HISTORY
 * Revision 1.1.45.1  1996/03/27  13:23:55  yp
 * 	Moved is_standalone to lib/test.c.
 * 	[96/03/14            yp]
 * 
 * 	Hooked up semaphore and lock set tests.
 * 	Hooked up latency test.
 * 	[96/02/19            yp]
 * 
 * Revision 1.1.19.8  1996/01/25  11:33:26  bernadat
 * 	Detect standalone mode using thread EXC_SYSCALL exception port
 * 	value. (Number of tasks is not reliable, since other server
 * 	tasks such as tgdb can be active).
 * 	[96/01/02            bernadat]
 * 
 * 	Remote execution support.
 * 	Added policy command.
 * 	Moved synthetic_init to synthetic.c (avoids
 * 	enforcing RR policy for all tests )
 * 	[95/11/30            bernadat]
 * 
 * 	MPTS 1.0
 * 	[95/11/27            bernadat]
 * 
 * Revision 1.1.19.7  1995/11/03  17:09:33  bruel
 * 	Added debugger()
 * 	[95/11/03            bernadat]
 * 
 * 	Added exception_init() and zones command.
 * 	Clarified usage message.
 * 	[95/10/31            bernadat]
 * 
 * 	Use get_task_cnt() inside getstandalone().
 * 	It deallocates tasks send rights.
 * 	[95/10/24            bernadat]
 * 	[95/11/03            bruel]
 * 
 * Revision 1.1.19.6  1995/04/12  12:27:58  madhu
 * 	Reintroduce synthetic_init after fixing synthetic.c
 * 	[1995/04/12  12:13:13  madhu]
 * 
 * 	Revision 1.1.7.11  1995/01/11  10:34:39  madhu
 * 	Synthetic tests are not shown by default.
 * 	[1995/01/11  10:07:11  madhu]
 * 
 * 	Revision 1.1.7.10  1994/12/09  14:03:26  madhu
 * 	sa.c has been modified so that standalone can run on top of Unix.
 * 	This depends on the number of tasks that are running.
 * 	Also incorporated N.Stephen's synthetic workload changes.
 * 	[1994/11/29  09:10:23  madhu]
 * 
 * 	Revision 1.1.7.9  1994/09/21  08:23:19  bernadat
 * 	Fixed resource misspelling.
 * 	Added strchr(), read_char()
 * 	Added <more> capability,
 * 	Moved misc routines to misc.c
 * 	Moved console handling to cons.c
 * 	Improved usage message.
 * 	Detect -debug, -trace and -debug_resources option
 * 	earlier to facilitate debuging of standalone code.
 * 	[94/09/13            bernadat]
 * 
 * 	Moved do_bootstrap_privileged_ports() to lib/server.c
 * 	Removed use of size_t.
 * 	[94/08/23            bernadat]
 * 
 * Revision 1.1.19.4  1995/01/10  08:08:36  devrcs
 * 	mk6 CR801 - copyright marker not _FREE
 * 	[1994/12/01  21:35:29  dwm]
 * 
 * 	mk6 CR668 - 1.3b26 merge
 * 	Replace bootstrap_privileged_ports() with bootstrap_ports().  Update
 * 	device_open() signature.  Provide new bootstrap server implementation.
 * 	[1994/11/07  15:53:03  bolinger]
 * 
 * Revision 1.1.19.2  1994/09/10  21:51:40  bolinger
 * 	Merge up to NMK17.3
 * 	[1994/09/08  19:44:56  bolinger]
 * 
 * Revision 1.1.19.1  1994/06/18  18:45:27  bolinger
 * 	Import NMK17.2 sources into MK6.
 * 	[1994/06/18  18:35:57  bolinger]
 * 
 * Revision 1.1.7.8  1994/07/06  07:55:13  bernadat
 * 	Added version id.
 * 	[94/07/04            bernadat]
 * 
 * 	Let standalone tests be interruptible.
 * 	[94/07/04            bernadat]
 * 
 * Revision 1.1.7.7  1994/06/21  17:01:54  barbou
 * 	Renamed device_server_port to master_device_port.
 * 	Fixed usage of mach_longjmp and mach_setjmp.
 * 	Added "device" test package.
 * 	[1994/06/21  16:05:14  barbou]
 * 
 * Revision 1.1.7.6  1994/04/28  09:53:18  bernadat
 * 	Removed empty prof routines.
 * 	[94/04/18            bernadat]
 * 
 * Revision 1.1.7.5  1994/04/14  13:55:01  bernadat
 * 	Added machine dependent test directory.
 * 	[94/04/14            bernadat]
 * 
 * Revision 1.1.7.4  1994/03/24  07:52:34  bernadat
 * 	Added __iob and setvbuf().
 * 	[94/03/24            bernadat]
 * 
 * Revision 1.1.7.3  1994/03/09  14:36:01  bernadat
 * 	Added pager test.
 * 	Initialize host_port.
 * 	[94/03/09            bernadat]
 * 
 * Revision 1.1.7.2  1994/02/22  12:41:52  bernadat
 * 	Moved longjmp/setjmp to machinde dependant file.
 * 	[94/02/21            bernadat]
 * 
 * Revision 1.1.7.1  1994/02/21  09:24:05  bernadat
 * 	Shorten line length to 79 chars for PC/AT VGA
 * 	Added reboot command.
 * 	Print kernel version.
 * 	Remove cthreads, use kernel threads instead.
 * 	Removed bcopy(), strncmp().
 * 	ANSI C cleanup
 * 	[94/02/18            bernadat]
 * 
 * 	Checked in NMK16_1 revision
 * 	[94/02/11            bernadat]
 * 
 * Revision 1.1.2.4  1993/11/02  12:33:20  bernadat
 * 	Added a console command to open com/console alternatively.
 * 	Adapted so that it can also run on kernels with old bootstrap
 * 	port initialization.
 * 	Print vm_stats.
 * 	Added a "*" command to run all tests.
 * 	[93/10/22            bernadat]
 * 
 * Revision 1.1.2.3  1993/09/29  10:23:49  bernadat
 * 	Check number of arguments, otherwise panics.
 * 	[93/09/29            bernadat]
 * 
 * Revision 1.1.2.2  1993/09/21  06:20:28  bernadat
 * 	Initial Revision.
 * 	[93/09/20            bernadat]
 * 
 */

#include <mach_perf.h>

mach_port_t privileged_host_port;
mach_port_t master_device_port;
mach_port_t name_server_port;
mach_port_t root_ledger_paged;
mach_port_t root_ledger_wired;
mach_port_t security_port;

volatile mach_port_t	test_task = MACH_PORT_NULL; /* task running the test */

jmp_buf 		sa_jmp_buf;		/* Where to restart when
						   a command fails */

static jmp_buf 		intr_jmp_buf;		/* Where to restart when test
						 * task is interrupted */

#define			DONE		0
#define			INTERRUPTED	1

int			death_reason;

extern	boolean_t	is_standalone(void);



extern int clock();
extern int device();
extern int exc();
extern int hw();
extern int host();
extern int ipc();
extern int latency();
extern int pager();
extern int port();
#ifdef	i386
extern int rpc();
#endif
extern int rtsync();
extern int sched();
extern int task();
extern int thread();
extern int vm();

extern int synthetic();
extern int console();
extern int reboot();
extern int interruptible_cmd();
extern int proc();
extern int show();
extern int zones();
extern int policy();
extern int quit();

struct test_dir test_dir [] = {
	TEST(clock, 1),
	TEST(device, 1),
	TEST(exc, 1),
	TEST(hw, 1),
	TEST(host, 1),
	TEST(ipc, 1),
	TEST(latency, 1),
	TEST(pager, 1),
	TEST(port, 1),
	TEST(rtsync, 1),
#ifdef	i386
	TEST(rpc,1),
#endif
	TEST(sched, 1),
	TEST(task, 1),
	TEST(thread, 1),
	TEST(vm, 1),
	TEST(console, 0),
	TEST(policy, 0),
	TEST(proc, 0),
	TEST(quit, 0),
	TEST(reboot, 0),
	TEST(show, 0),
	TEST(zones, 0),
	0, 0, 0
};

extern struct test_dir machine_test_dir[];

struct test_dir *test_dirs[] = {
	test_dir,
	machine_test_dir,
	0
};

void
shift_args(
	int	*argc,
	char 	*argv[])
{
  	register i;

	for (i=1; i < *argc; i++)
		argv[i-1] = argv[i];
	(*argc)--;
}

#undef main
#undef usage

void usage();
extern mach_port_t console_port;

main()
{
	mach_port_t 	bootstrap_port;
	int	 	i;
	struct test_dir *td, **tds;
	int 		all;
	kern_return_t 	kr;
	boolean_t 	found;
	int 		argc = 0;
	char 		**argv;
        vm_offset_t addr = 0;

	MACH_CALL(task_get_special_port, (mach_task_self(),
                                       TASK_BOOTSTRAP_PORT,
                                       &bootstrap_port));
	MACH_CALL(bootstrap_ports, (bootstrap_port,
					&privileged_host_port,
					&master_device_port,
					&root_ledger_wired,
					&root_ledger_paged,
					&security_port));

	MACH_FUNC(host_port, mach_host_self, ());

	standalone = is_standalone();

	if (standalone) {
		extern security_token_t standalone_token;
		MACH_CALL(device_open,
			(master_device_port,
			MACH_PORT_NULL, 
			D_WRITE, 
			standalone_token, 
			(char *)"console",
			&console_port));
	}
	/*
	 *	Protect page 0 against null pointer dereferencing.
	 */
        if (vm_allocate(mach_task_self(),
                       &addr,
                       page_size_for_vm,
                       FALSE) == KERN_SUCCESS)
                MACH_CALL(vm_protect, (mach_task_self(),
                                       addr,
                                       page_size_for_vm,
                                       FALSE,
                                       VM_PROT_NONE));

	threads_init();
	console_init();
	_printf_init();
	exception_init();
	ux_prof_init();

	printf("\n\n");
	version();
	kernel_version();
	if (standalone)
		printf("Standalone mode\n\n");

	vm_opt = 1;
	print_vm_stats();
	printf("\n");
	get_thread_sched_attr(mach_thread_self(),
			    (policy_state_t) 0,
			    TRUE);
	printf("\n");
	while(1) {
		mach_setjmp(&sa_jmp_buf);
		reset_options();
                /* synthetic benchmarks are not the default type */
                synthetic_fn = NULL;
		reset_more();
		if (!(argc = read_cmd(&argv, "mpts> ")))
			continue;
		for (i = 1; i < argc; i++)
			is_gen_opt(argc, argv, &i, 0, 0);
		if (!strcmp(argv[0],"on")) {
		  	shift_args(&argc, argv);
			if (remote_node(argc, argv)) {
				shift_args(&argc, argv);
			} else {
				interruptible_cmd(usage, 0, 0);
				continue;
			}
		} 
		if (!strcmp(argv[0],"more")) {
			shift_args(&argc, argv);
		} else
			disable_more();
		all = strcmp(argv[0],"*") ? 0 : 1;
		for (found = FALSE, tds = test_dirs; *tds && !found; tds++)
		    for (td = *tds; td->name && !found; td++)
			if ((all && td->is_a_test) 
			    || !strcmp(argv[0],td->name)) {
				if (td->is_a_test)
					printf("_______________________________________________________________________________\n");
			  	argv[0] = td->name;
				if (td->is_a_test)
					interruptible_cmd(td->func,
							  argc,
							  argv);
				else
					(*td->func) (argc, argv);
				if (!all)
					found = TRUE;
			}
		if ((!all) && (!found)) {
                    if (find_proc(argv[0]))
                        /* run synthetic benchmark if we have a proc name */
                        interruptible_cmd(synthetic,argc,argv);
                    else
			interruptible_cmd(usage, 0, 0);
		}
	}
	printf("done\n");
}

void
print_tests(boolean_t test_ones)
{
	register i;
	struct test_dir *td, **tds;

	for (tds = test_dirs, i = 0; *tds; tds++)
	    for (td = *tds; td->name; td++)
		if (!(test_ones ^ td->is_a_test)) {
			if (i == 4) {
				printf("\n");
				i = 0;
			}
		  	if (i == 0)
				printf("\t");
			printf("%s", td->name);
			spaces(16-strlen(td->name));
			i++;
		}
}

void
usage()
{

	enable_more();
	printf("Usage: [on <node>] [more] <command>|* [<options>]\n");
	printf("\nTest commands:\n");
	print_tests(TRUE);
	printf("\n\nControl commands:\n");
	print_tests(FALSE);
	printf("\n\nThe '?' option reports usage for the specified command\n");
	printf("\nUse <CTRL> c to interrupt\n\n");
}
 
reboot()
{
	if(!standalone)
		printf("reboot - This option is not available when you are running with the Unix server\n");
	else
		MACH_CALL(host_reboot, (privileged_host_port, 0));
}

quit()
{
	task_terminate(mach_task_self());
}

interruptible_cmd(func, argc, argv) 
char *argv[];
int			(*func)();
{
	mach_port_t 		notify_port;
	mach_port_t 		port_set;
	mach_port_t 		bootstrap_port;
	mach_port_t 		prev_port;
	extern	boolean_t	sa_demux();
	mach_port_t		*tasks_before, *tasks_after;
	int			ntask_before, ntask_after;
	register 		i, j;
	mach_port_t 		*tp;

	
	resources_start();

	MACH_FUNC(ntask_before, get_tasks, (&tasks_before));

	/*
	 * We need to:
	 *	- Create a task to run the test
	 *	- Pass bootstrap ports to the new task. (As a side
	 *	  effect, the new task waits until all setups are
	 *	  done on our side, preventing noise on time stats)
	 *	- Wait for test to complete (slave task to terminate)
	 *	- Check for INTERRUPT
 	 */


	/*
	 * Create port set. 
	 * We will insert:
	 *	- a port to wait dead name notification on the created task
	 *	- a device reply port for character input.
	 *	- a bootstrap port on which the created task syncs to wait
	 *	  before it starts
	 */

	MACH_CALL(mach_port_allocate, 
		  (mach_task_self(),
		   MACH_PORT_RIGHT_PORT_SET,
		   &port_set));

	/*
	 * Set bootstrap port so that slave task can communicate
	 */ 

	MACH_CALL( mach_port_allocate, (mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE,
				 &bootstrap_port));
	MACH_CALL( mach_port_insert_right, (mach_task_self(),
					    bootstrap_port,
					    bootstrap_port,
					    MACH_MSG_TYPE_MAKE_SEND));
	MACH_CALL(task_set_bootstrap_port, (mach_task_self(),
					    bootstrap_port));
	MACH_CALL(mach_port_move_member, (mach_task_self(),
					   bootstrap_port, port_set));

	console_sync_mode(FALSE);

	/*
	 * Catch dead name notifications on task port.
	 */

	MACH_CALL(mach_port_allocate, (mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE,
				 &notify_port));
	MACH_CALL(mach_port_move_member, (mach_task_self(),
					   notify_port, port_set));

	death_reason = DONE;

	if (norma_node != NORMA_NODE_NULL)
		printf("\n*** Remote execution on node %d ***\n\n", norma_node);
	if ((test_task = mach_fork())) {
		mach_port_t		saved_test_task;
		thread_malloc_state_t	mallocs;

		MACH_FUNC (saved_test_task,
			   mach_thread_self, ()); /* for debug */
		mallocs = save_mallocs (thread_self());

		if (debug > 1)
			printf("created task %x\n", test_task);

		MACH_CALL (mach_port_request_notification,
			   (mach_task_self(),
			    test_task,
			    MACH_NOTIFY_DEAD_NAME,
			    0,
			    notify_port,
			    MACH_MSG_TYPE_MAKE_SEND_ONCE,
			    &prev_port));

		if (prev_port != MACH_PORT_NULL)
			MACH_CALL( mach_port_deallocate, (mach_task_self(),
							   prev_port));
		if (mach_setjmp(intr_jmp_buf) == 0) {
			MACH_CALL( mach_msg_server, (sa_demux,
						     256,
						     port_set,
						     MACH_MSG_OPTION_NONE));
		} else {	
			saved_test_task = test_task;
			test_task = MACH_PORT_NULL;
			restore_mallocs (thread_self(), mallocs);
		}
		disable_more();
		MACH_CALL_NO_TRACE( mach_port_destroy, (mach_task_self(),
							saved_test_task));

	} else {
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

		(*func) (argc, argv);
		MACH_CALL( task_terminate, (mach_task_self())); 
	}

	MACH_FUNC(ntask_after, get_tasks, (&tasks_after));

	console_sync_mode(TRUE);

	MACH_CALL( mach_port_destroy, (mach_task_self(), bootstrap_port));
	MACH_CALL( mach_port_destroy, (mach_task_self(), notify_port));
	MACH_CALL( mach_port_destroy, (mach_task_self(), port_set));

	if (standalone && ntask_after != ntask_before) {
		/*
		 * When running standalone, we expect to find the
		 * same number of tasks as before the test started,
		 * hence kill the extra tasks (supposedly started by us).
		 */
		for (i=0, tp = tasks_after; i < ntask_after; i++, tp++) {
			for (j = 0; j < ntask_before; j++)
				if (tasks_before[j] == *tp) {
				  	MACH_CALL(mach_port_deallocate,
						  (mach_task_self(),
						   *tp));
					*tp = 0;
					break;
				}
			if (*tp) {
				if (debug > 1)
					printf("killing task %x\n", *tp);
				task_terminate (*tp); /* Dont test ret code
							 task can terminate
							 itself */
				MACH_CALL( mach_port_deallocate,
					  (mach_task_self(), *tp));
				*tp = 0;
			}
		}
	}

	for (i=0, tp = tasks_before; i < ntask_before; i++, tp++)
		if (*tp) {
			MACH_CALL(mach_port_deallocate, (mach_task_self(),
							 *tp));
		}

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) tasks_before,
				  sizeof(*tasks_before)*ntask_before));

	for (i=0, tp = tasks_after; i < ntask_after; i++, tp++)
		if (*tp) {
			MACH_CALL(mach_port_deallocate, (mach_task_self(),
							 *tp));
		}

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) tasks_after,
				  sizeof(*tasks_after)*ntask_after));
	
	resources_stop();
	resources_use("Standalone ");       
	if (death_reason == INTERRUPTED)
		exit();
}

void
interrupt_cmd() {
	if (test_task) {
	  	disable_more();
		death_reason = INTERRUPTED;
		MACH_CALL( task_terminate, (test_task));
		while (test_task) {
			thread_switch(MACH_PORT_NULL, 
				      SWITCH_OPTION_DEPRESS, 
				      10);
		} 
	}
}

void	(*interrupt_func)() = interrupt_cmd;

exit()
{
	for (;;) 
		if (is_master_task)
			mach_longjmp(&sa_jmp_buf, 1);
		else
			MACH_CALL(task_terminate, ( mach_task_self()));
}

boolean_t
sa_demux(in, out)
{
	if (sa_notify_server(in, out))
		return(TRUE);
	else return(bootstrap_server(in, out));
}


kern_return_t
sa_mach_notify_port_deleted(
	mach_port_t	port,
	int		name)			 
{
	printf("mach_notify_port_deleted(), name = %x\n", name);
	return(KERN_SUCCESS);
}

kern_return_t
sa_mach_notify_port_destroyed(
	mach_port_t	port,
	int		rcv_right)
{
	printf("mach_notify_port_destroyed(), rcv_right = %x\n", rcv_right);
	return(KERN_SUCCESS);
}

kern_return_t
sa_mach_notify_no_senders(
	mach_port_t	port,
	int		count)
{
	printf("Spurious mach_notify_no_senders(), count = %x\n", count);
	return(KERN_SUCCESS);
}

kern_return_t
sa_mach_notify_send_once(
	mach_port_t	port)
{
	printf("mach_notify_send_once()\n");
	return(KERN_SUCCESS);
}

kern_return_t
sa_mach_notify_dead_name(
	mach_port_t	port,
	int		name)			 
{
	if (debug)
		printf("mach_notify_dead_name(), name = %x\n", name);
	mach_longjmp(intr_jmp_buf, 1);
	return(KERN_SUCCESS);
}

netname_look_up()
{
	printf("not yet implemented\n");
	return(KERN_FAILURE);
}

netname_check_in()
{
	printf("not yet implemented\n");
	return(KERN_FAILURE);
}

/*
 * policy allows the setting of the scheduling policy used for all benchmark
 * tasks from the command line.
 */

void policy_usage()
{
    printf("Usage : policy"
"\t[-base <base pri>]       Set base priority to <base pri>\n"
"\t\t[-max <max pri>]         Set max priority to <max pri>\n"
"\t\t[-quantum <quantum>]     Set quantum to <quantum> milliseconds\n"
"\t\t[<policy>]               Set policy to <policy>\n"
"\t\t                         1 or TS: POLICY_TIMESHARE\n"
"\t\t                         2 or RR: POLICY_RR\n"
"\t\t                         4 or FIFO: POLICY_FIFO\n"
"\t\t[-default]               Set policy attributes to original ones\n"
    );
    leave(1);
}

struct policy_state default_policy = {-1};

int policy(argc, argv)
int argc;
char *argv[];
{
	int i;

	if (argc == 1) {
	  	get_thread_sched_attr(mach_thread_self(), 0, TRUE);
		return(0);
	}

	if (default_policy.policy == -1)
		get_thread_sched_attr(mach_thread_self(),
				      &default_policy, FALSE);

	for (i = 1; i < argc; i++)
		if ((!strcmp(argv[i], "-pri")) ||
		    (!strcmp(argv[i], "-base"))) {
			if (++i >= argc || *argv[i] == '-')
				policy_usage();
			if (!atod(argv[i], &base_pri))
				policy_usage();
		} else if  (!strcmp(argv[i], "-max")) {
		  	if (++i >= argc || *argv[i] == '-')
		  	  	policy_usage();
		  	if (!atod(argv[i], &max_pri))
			  	policy_usage();
	  	} else if (!strcmp(argv[i], "-quantum")) {
		  	if (++i >= argc || *argv[i] == '-')
			  	usage();
		  	if (!atod(argv[i], &quantum))
			  	policy_usage();
		} else if (!strcmp(argv[i], "FIFO")) {
			sched_policy = POLICY_FIFO;
		} else if (!strcmp(argv[i], "RR")) {
			sched_policy = POLICY_RR;
		} else if (!strcmp(argv[i], "TS")) {
			sched_policy = POLICY_TIMESHARE;
		} else if (!strcmp(argv[i], "-default")) {
			set_task_sched_attr(mach_task_self(),
				      &default_policy, TRUE);
			sched_policy = -1;
			base_pri = -1;
			max_pri = -1;
			quantum = -1;
	  	} else if (!atod(argv[i], &sched_policy)) {
			  	policy_usage();
		}
	update_policy(TRUE);
	return (0);
}
