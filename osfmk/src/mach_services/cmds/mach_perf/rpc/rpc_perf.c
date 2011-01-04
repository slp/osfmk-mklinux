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
 *	File:	rpc_perf.c
 *	Author:	Yves Paindaveine
 *
 *	Main RPC test --- client side.
 */	

#define TEST_NAME	rpc

#include <mach_perf.h>
#include <rpc_types.h>
#include <rpc_user.h>

extern rpc_server_main();

char *private_options = "\n\
\t[-show_server]           Show server side timings.\n\
\t[-kill_server]           Kill server upon test completion.\n\
\t[-client]                Only start client side task.\n\
\t[-server]                Only start server side task.\n\
\t[-norma <node>]          Start server task on node <node>\n\
";

#define NULL_ACCESS 0
#define READ_ACCESS 1
#define WRITE_ACCESS 2

int null_rpc_test();
int make_send_rpc_test();
int copy_send_rpc_test();
int inline_rpc_test();
int inline_128_rpc(), inline_1024_rpc(), inline_4096_rpc();
int ool_rpc_test();


struct test tests[] = {
"null rpc",			0, null_rpc_test, 0, 0, 0, 0, 
"1 make_send_right rpc",	0, make_send_rpc_test, 0, 0, 0, 0,
"1 copy_send_right rpc",	0, copy_send_rpc_test, 0, 0, 0, 0,
"128 bytes inline rpc",		0, inline_rpc_test, (int)inline_128_rpc,
				0, 0, 0,	
"1024 bytes inline rpc",	0, inline_rpc_test, (int)inline_1024_rpc,
				0, 0, 0,	
"1024 bytes ool rpc + touch",	0, ool_rpc_test, 1024, READ_ACCESS, 0, 0,
"4096 bytes inline rpc",	0, inline_rpc_test, (int)inline_4096_rpc,
				0, 0, 0,	
"4096 bytes ool rpc",		0, ool_rpc_test, 4096, 0, 0, 0,
"4096 bytes ool rpc + read",	0, ool_rpc_test, 4096, READ_ACCESS, 0, 0,
"4096 bytes ool rpc + write",	0, ool_rpc_test, 4096, WRITE_ACCESS, 0, 0,
"8192 bytes ool rpc",		0, ool_rpc_test, 8192, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
int argc;
char *argv[];
{
	int i, server_death_warrant = 0;

	test_init();

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-show_server")) {
		  	server_times = 1; 
#if	!MONITOR
		} else if (!strcmp(argv[i], "-client")) {
		  	client_only = 1; 
		} else if (!strcmp(argv[i], "-server")) {
		  	server_only = 1; 
		} else if (!strcmp(argv[i], "-kill_server")) {
		  	server_death_warrant = 1; 
#endif	/* !MONITOR */
		} else if (!strcmp(argv[i], "-norma")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &norma_node))
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	if (!client_only)
		run_server((void (*)())rpc_server_main);
	else {
		server_lookup(RPC_SERVER_NAME);
	}
	
	if (server_only)
		return(0);

	if (debug) 
		printf("server %x\n", server);

	run_tests(tests);

	if (!client_only || server_death_warrant)
		kill_server(server);

	test_end();
}

null_rpc_test()
{
	register int i;
	kern_return_t kr;

	start_time();
	for (i=loops; i--;)
		null_rpc(server);
	stop_time();
}

#ifdef	AUTOTEST
kern_return_t
client_null_rpc(server)
	mach_port_t server;
{
	return KERN_SUCCESS;
}
#endif	/* AUTOTEST */


make_send_rpc_test()
{
	register int i;
	mach_port_t port;
	kern_return_t	ret;


	MACH_CALL (mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&port));

	start_time();
	for (i=loops; i--;)
		make_send_rpc(server, port);
	stop_time();

	MACH_CALL(mach_port_insert_right, (mach_task_self(),
					   port,
					   port,
					   MACH_MSG_TYPE_MAKE_SEND));

	MACH_CALL(rpc_port_cleanup, (server, port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), port));
}

copy_send_rpc_test()
{
	register int i;
	mach_port_t port;

	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &port));
	
	MACH_CALL(mach_port_insert_right, (mach_task_self(),
					   port,
					   port,
					   MACH_MSG_TYPE_MAKE_SEND));

	start_time();
	for (i=loops; i--;)
		copy_send_rpc(server, port);
	stop_time();
	MACH_CALL(rpc_port_cleanup, (server, port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), port));
}

inline_rpc_test(rpc)
kern_return_t (*rpc)();
{
	register int i;
	inline_8192_t *buf;

	MACH_CALL(vm_allocate, (mach_task_self(), 
				(vm_offset_t *)&buf,
				sizeof (inline_8192_t),
				TRUE));

	start_time();
	for (i=loops; i--;) {
		(*rpc)(server, *buf);
	}
	stop_time();
	MACH_CALL(vm_deallocate, (mach_task_self(), 
				(vm_offset_t )buf,
				sizeof (inline_8192_t)));
}

ool_rpc_test(size, touch)
int	size;
int	touch;
{
	register int i;
	char *buf;
	kern_return_t	ret;

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&buf,
				size,
				TRUE));

	if (touch) {
		*buf = (char) 0xaa;
		*(buf+2048) = (char) 0xbb;
	}
	start_time();
	for (i=loops; i--;)
		ool_rpc(server, touch, buf, (mach_msg_type_number_t)size);
	stop_time();
	MACH_CALL(vm_deallocate, (mach_task_self(), 
				  (vm_offset_t )buf,
				  size));
}
