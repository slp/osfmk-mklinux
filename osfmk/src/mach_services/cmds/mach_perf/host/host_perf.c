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

#define TEST_NAME	host

#include <mach_perf.h>

int host_self_test();
int host_statistics_test();

char *private_options = "\n\
";

struct test tests[] = {
"host_self",			0, host_self_test, 0, 0, 0, 0,
"host_statistics (LOAD_INFO)",	0, host_statistics_test,
                                HOST_LOAD_INFO, 0, 0, 0,
"host_statistics (VM_INFO)",	0, host_statistics_test,
				HOST_VM_INFO, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++)
		if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();
	run_tests(tests);
	test_end();
}

host_self_test()
{
	register 		i;
	mach_port_t 		port;

	start_time();
	for (i=loops; i--;)  {
		MACH_FUNC(port, mach_host_self, ());
	}
	stop_time();
	MACH_CALL(mach_port_deallocate, (mach_task_self(), port));
}

host_statistics_test(flavor)
int flavor;
{
	register 		i;
	mach_msg_type_number_t	count;
	host_info_t		info;	
	struct host_load_info 	load_info;
	struct vm_statistics	vm_info;

	switch(flavor) {
	case HOST_LOAD_INFO:
		info = (host_info_t)&load_info;
		count = sizeof(load_info);
		break;
	case HOST_VM_INFO:
		info = (host_info_t)&vm_info;
		count = sizeof(vm_info);
		break;
	}
	start_time();
	for (i=loops; i--;)  {
		MACH_CALL(host_statistics, (privileged_host_port,
					    flavor, info, &count));
	}
	stop_time();
}
