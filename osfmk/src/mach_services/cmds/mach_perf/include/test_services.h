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

#ifndef _TEST_SERVICES_H
#define _TEST_SERVICES_H

#include <mach_perf.h>

enum test_states 	{ INIT_TIMER, CALIBRATING, FIRST_RUN, RUNNING };
typedef int test_state_t;
extern test_state_t	test_status;

extern int		loops;
extern int		nruns;
extern int              background;
extern int		verbose;
extern int		iopt, topt;
extern double		min_elapsed;
extern double		max_elapsed_ratio;
extern int		max_loops;
extern int		min_loops;
extern int		prof_opt;
extern int		min_percent;
extern int 		show_resourcess;
extern int 		total_runs;
extern boolean_t	standalone;
extern int		no_cache;
extern int		vm_opt;
extern boolean_t	norma_kernel;

extern int		(*more_measures)();
extern int		(*start_test)();
extern int		(*stop_test)();
extern sync_vec         *synthetic_fn;
extern int		no_measure();
extern int              silent;
extern boolean_t	need_banner;
extern int              pass;
extern int             	sched_policy;
extern int             	base_pri;
extern int             	max_pri;
extern int             	quantum;

extern char *time_opts;


/* extern char *private_options; */

struct test {
	char	*name;
	int	enabled;
	int	(*func)();
	int	arg0, arg1, arg2, arg3;
};

#define __test_init() {					\
	get_args(&argc, &argv);				\
	if (argc) {					\
	  	reset_options();			\
		if (standalone)				\
			print_command_line(argc, argv);	\
	}						\
	privileged_user();				\
	threads_init();					\
	_printf_init();					\
	more_test_init();				\
}

#if	MONITOR
#define	test_init()	__test_init()
#define	test_end()
#else	/* MONITOR */
/*
 * test_init(): when started as a server_only in standalone
 * 		server_only is not yet set.
 *		The test !server_only is used when the program
 *		server_only does not create another task and
 *		starts the *_server_main from the program itself.
 */
#define	test_init() {						\
if (is_master_task && !server_only) {				\
	vm_offset_t addr = 0;					\
	MACH_CALL(task_get_special_port, (mach_task_self(),	\
					  TASK_BOOTSTRAP_PORT,	\
					  &bootstrap_port));	\
        MACH_CALL(bootstrap_ports, (bootstrap_port,		\
				    &privileged_host_port,	\
				    &master_device_port,	\
				    &root_ledger_wired,		\
				    &root_ledger_paged,		\
				    &security_port));		\
        MACH_FUNC(host_port, mach_host_self, ());		\
        if (vm_allocate(mach_task_self(),			\
		       &addr,					\
		       page_size_for_vm,			\
		       FALSE) == KERN_SUCCESS)			\
		MACH_CALL(vm_protect, (mach_task_self(),	\
				       addr,			\
				       page_size_for_vm,	\
				       FALSE,			\
				       VM_PROT_NONE));		\
	standalone = is_standalone();				\
	get_args(&argc, &argv);					\
	if (standalone) {					\
           extern mach_port_t console_port;			\
           extern security_token_t standalone_token;		\
	   MACH_CALL(device_open,				\
		(master_device_port,				\
		MACH_PORT_NULL, 				\
		D_WRITE | D_READ,				\
		standalone_token, 				\
		(char *)"console",				\
		&console_port));				\
        }							\
	threads_init();						\
	_printf_init();						\
	exception_init();					\
	printf("\n\n");						\
	version();						\
	kernel_version();					\
        reset_options();					\
        disable_more();						\
	time_init();						\
	machine_init();						\
	vm_opt = 1;						\
	print_vm_stats();					\
	printf("\n");						\
	get_thread_sched_attr(mach_thread_self(),	       	\
			    (policy_state_t) 0,			\
			    TRUE);				\
	printf("\n");						\
}								\
}
/*
 * When used in conjunction with -w in bootstrap.conf, test_end()
 * provides an easy way to synchronize server and client.
 */
#define	test_end() {							\
	if (is_master_task && standalone) {				\
                (void)bootstrap_completed(bootstrap_port,		\
				     mach_task_self()); 		\
	}								\
}
#endif	/* MONITOR */

/*
 * Did a test run long enough ? 
 */

#define run_is_ok()							     \
	(iopt ||							     \
	 (loops >= min_loops &&						     \
	  (total_time(&client_stats) < (min_elapsed * max_elapsed_ratio) ||  \
	   loops == min_loops) &&					     \
	  (((!max_loops) && total_time(&client_stats) >= min_elapsed) ||     \
	   (max_loops && (total_time(&client_stats) >= min_elapsed ||	     \
			  loops >= max_loops)))))

/*
 * Some tests imply large resources allocations 
 * of kernel objects. The goal is not to measure
 * the resources management but the cost of a single
 * kernel service. To be sure the resourcess allocation
 * does not influence this cost, we rerun the test to cache
 * this resources allocation overhead (like port name
 * splay trees)
 */

#define test_cache(func, args)				\
{							\
	static func##_cached = 0;	        	\
	if (func##_cached < loops &&			\
	    run_is_ok() &&				\
	    !no_cache) {				\
		if (debug)				\
			printf("cache loop\n");		\
		prof_drop();				\
		func##_cached = loops;			\
		func args;				\
	}						\
}

#define set_max_loops(l)			\
	max_loops = l;				\
	if (!iopt && loops > max_loops)		\
		loops = max_loops;		

#define set_min_loops(l)			\
	if (l > min_loops)			\
		min_loops = l;		

#define test_error(test, reason) \
	test_failed(test, __FILE__, __LINE__, reason)

#define usage()	print_usage(tests, private_options)

#define concatenate_2(x, y)	x##y
#define concatenate(x, y)	concatenate_2(x, y)

extern mach_port_t privileged_host_port;
extern mach_port_t host_port;

extern mach_port_t bootstrap_port;
extern mach_port_t master_host_port;
extern mach_port_t master_device_port;

extern mach_port_t root_ledger_paged;
extern mach_port_t root_ledger_wired;
extern mach_port_t security_port;
extern security_token_t mach_perf_token;

#if	MONITOR
#define main TEST_NAME

#define private_options concatenate(TEST_NAME, _private_options)
#define tests		concatenate(TEST_NAME, _tests)

#define TEST(a, b) # a, a, b 

struct test_dir {
	char		*name;
	int		(*func)();
	int		is_a_test;
};

#endif	/* MONITOR */

extern int leave(int status);
extern int reset_options();
extern int print_command_line(int argc, char *argv[]);
extern int more_test_init();
extern int print_usage(struct test *tests, char *private_options);
extern int atod(char *s, int *i);
extern int is_gen_opt(int argc, char *argv[], int *index,
                struct test *tests, char *private_options);
extern int run_tests(struct test *tests);

extern boolean_t kernel_is_norma(void);

#endif /* _TEST_SERVICES_H */
