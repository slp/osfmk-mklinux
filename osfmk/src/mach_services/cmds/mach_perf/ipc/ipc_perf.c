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


#define TEST_NAME	ipc

#include <mach_perf.h>
#include <ipc_types.h>

mach_port_t 	first_port_name = 50000;	/* !! Update help message */
int 		sparse_factor = 7; 		/* !! Update help message */
int		rename_server_ports = 0;
int		rename_member_ports = 0;

extern ipc_server_main();

char *private_options = "\n\
\t[-show_server]           Show server side timings.\n\
\t[-kill_server]           Kill server upon test completion.\n\
\t[-client]                Only start client side task.\n\
\t[-server]                Only start server side task.\n\
\t[-norma <node>]          Start server task on node <node>\n\
\t[-sparse <factor>]       When using non generic port names,the value\n\
\t                         <factor> is added to the last port name \n\
\t                         to allocate a new name (default is 7)\n\
\t[-first_port <name>]     When using non generic port names\n\
\t                         the value <name> is used as the first name\n\
\t                         (default is 50000)\n\
\t[-rename_server_ports <size>]\n\
\t                         Server side task will use non generic\n\
\t                         port names and populates its IPC name space\n\
\t                         with <size> ports.\n\
\t[-rename_member_ports]   Use non generic port names for ports inserted\n\
\t                         in port sets on server side\n\
";

#define NULL_ACCESS 0
#define READ_ACCESS 1
#define WRITE_ACCESS 2

int null_snd_rcv_test();
int null_ipc_test();
int make_send_ipc_test();
int copy_send_ipc_test();
int inline_ipc_test();
int ool_ipc_test();
int null_rpc_to_set_test();
int random_null_rpc_test();
mach_port_t *allocate_ports();

int inline_128_ipc(), inline_1024_ipc(), inline_4096_ipc();


struct test tests[] = {
"Server in same task",		0, 0, 0, 0, 0, 0,
"null rpc (gen port)",		0, null_snd_rcv_test, 0, 1, 0, 0, 
"Server in separate task",	0, 0, 0, 0, 0, 0,
"null rpc (gen port)",		0, null_snd_rcv_test, 0, 0, 0, 0, 
"null rpc (renamed port)",	0, null_snd_rcv_test, 1, 0, 0, 0, 
"null ipc (no receiver)",	0, null_ipc_test, TRUE, 0, 0, 0, 
"null ipc (active receiver)",	0, null_ipc_test, 0, 0, 0, 0, 
"1 make_send_right rpc",	0, make_send_ipc_test, 0, 0, 0, 0,
"1 copy_send_right rpc",	0, copy_send_ipc_test, 0, 0, 0, 0,
"128 bytes inline rpc",		0, inline_ipc_test, (int)inline_128_ipc,
				0, 0, 0,	
"1024 bytes inline rpc",	0, inline_ipc_test, (int)inline_1024_ipc,
				0, 0, 0,	
"1024 bytes ool rpc + touch",	0, ool_ipc_test, 1024, READ_ACCESS, 0, 0,
"4096 bytes inline rpc",	0, inline_ipc_test, (int)inline_4096_ipc,
				0, 0, 0,	
"4096 bytes ool rpc",		0, ool_ipc_test, 4096, 0, 0, 0,
"4096 bytes ool rpc + read",	0, ool_ipc_test, 4096, READ_ACCESS, 0, 0,
"4096 bytes ool rpc + write",	0, ool_ipc_test, 4096, WRITE_ACCESS, 0, 0,
"8192 bytes ool rpc",		0, ool_ipc_test, 8192, 0, 0, 0,
"null rpc to port set",		0, null_rpc_to_set_test, 0, 0, 0, 0, 
"tests with +100 port names",	0, 0, 0, 0, 0, 0,
"null rpc (rand gen port)",	0, random_null_rpc_test, 100, 0, 0, 0, 
"null rpc (rand renamed ports)",	0, random_null_rpc_test, 100, 1, 0, 0, 
"tests with +1000 port names",	0, 0, 0, 0, 0, 0,
"null rpc (rand gen port)",	0, random_null_rpc_test, 1000, 0, 0, 0, 
"null rpc (rand renamed ports)",0, random_null_rpc_test, 1000, 1, 0, 0, 
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
int argc;
char *argv[];
{
	int i, server_death_warrant = 0;

	test_init();

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-sparse")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &sparse_factor))
				usage();
		} else if (!strcmp(argv[i], "-first_port")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], (int *)&first_port_name))
				usage();
		} else if (!strcmp(argv[i], "-show_server")) {
		  	server_times = 1; 
#if	!MONITOR
		} else if (!strcmp(argv[i], "-client")) {
		  	client_only = 1; 
		} else if (!strcmp(argv[i], "-server")) {
		  	server_only = 1;
		} else if (!strcmp(argv[i], "-kill_server")) {
		  	server_death_warrant = 1;  
#endif	/* !MONITOR */
		} else if (!strcmp(argv[i], "-rename_server_ports")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &rename_server_ports))
				usage();
		} else if (!strcmp(argv[i], "-rename_member_ports")) {
			rename_member_ports++;
		} else if (!strcmp(argv[i], "-norma")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &norma_node))
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	if (!client_only)
		run_server((void (*)())ipc_server_main);
	else {
		server_lookup(IPC_SERVER_NAME);
	}
	
	if (server_only)
		return(0);

	if (debug) 
		printf("server %x\n", server);

	if (in_kernel) {
		/* XXX be silent about disabled tests? */
		if (!topt) {
			topt = TRUE;
			enable_all_tests(tests);
		}
		disable_test_number(2,tests);	/* renamed ports */
		disable_test_number(17,tests);	/* renamed ports */
		disable_test_number(19,tests);	/* renamed ports */
	}

	run_tests(tests);

	if (!client_only || server_death_warrant)
		kill_server(server);

	test_end();
}

extern ipc_thread();

null_snd_rcv_test(renamed, same_task)
{
	register int i;
	mach_port_t	old_server, *port_list;

	if (same_task)
		start_server_thread((void (*)())ipc_thread);

	if (renamed) {
		port_list = allocate_ports(renamed, 1);
		old_server = server;
		server = port_list[renamed-1] + sparse_factor;
		MACH_CALL(mach_port_rename, (mach_task_self(),
					     old_server,
					     server));
	}
	start_time();
	for (i=loops; i--;)
		null_snd_rcv(server);
	stop_time();
	if (renamed) {
		MACH_CALL(mach_port_rename, (mach_task_self(),
					     server,
					     old_server));
		server = old_server;
		deallocate_ports(port_list, renamed);
	}
	if (same_task)
		stop_server_thread();
}

null_ipc_test(no_server)
{
	register int i, j;
	mach_port_t	*port_list, *port;
	unsigned int nports = loops/MACH_PORT_QLIMIT_MAX + 1;
	struct mach_port_limits limits;
	mach_msg_type_number_t count;

	if (!no_server) {
		start_time();
		for (i=loops; i--;)
			null_ipc(server);
		stop_time();
		return;
	}

	ignore_server();
	port_list = allocate_ports(nports, 0);
	count = sizeof limits;
	MACH_CALL(mach_port_get_attributes,
		  (mach_task_self(),
		   *port_list,
		   MACH_PORT_LIMITS_INFO,
		   (mach_port_info_t)&limits,
		   &count));
	limits.mpl_qlimit = MACH_PORT_QLIMIT_MAX;
	for (i=0; i<nports; i++) {
		MACH_CALL(mach_port_set_attributes,
			  (mach_task_self(),
			   *(port_list+i),
			   MACH_PORT_LIMITS_INFO,
			   (mach_port_info_t)&limits,
			   count));
	}
	start_time();
	for (i=loops, j=0, port = port_list ; i--; j++) {
		if (j == MACH_PORT_QLIMIT_MAX) {
			port++;
			j = 0;
		}
		null_ipc(*port);
	}			
	stop_time();
	deallocate_ports(port_list, nports);
}

make_send_ipc_test()
{
	register int i;
	mach_port_t port;
	kern_return_t	ret;


	MACH_CALL (mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&port));

	start_time();
	for (i=loops; i--;)
		make_send_ipc(server, port);
	stop_time();

	MACH_CALL(mach_port_insert_right, (mach_task_self(),
					   port,
					   port,
					   MACH_MSG_TYPE_MAKE_SEND));

	MACH_CALL(port_cleanup, (server, port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), port));
}

copy_send_ipc_test()
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
		copy_send_ipc(server, port);
	stop_time();
	MACH_CALL(port_cleanup, (server, port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), port));
}

inline_ipc_test(rpc)
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

ool_ipc_test(size, touch)
{
	register int i;
	unsigned char *buf;
	kern_return_t	ret;

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&buf,
				size,
				TRUE));

	if (touch) {
		*buf = 0xaa;
		*(buf+2048) = 0xbb;
	}
	start_time();
	for (i=loops; i--;)
		ool_ipc(server, touch, buf, size);
	stop_time();
	MACH_CALL(vm_deallocate, (mach_task_self(), 
				  (vm_offset_t )buf,
				  size));
}

null_rpc_to_set_test()
{
	register int i;
	kern_return_t ret;
	mach_port_t port, new_server, old_server;
	mach_port_t task = mach_task_self();

	MACH_CALL(mach_port_allocate, (task,
				       MACH_PORT_RIGHT_RECEIVE,
				       &port));

	new_server = port;
	MACH_CALL( mach_port_insert_right, (task, new_server,
					    port,
					    MACH_MSG_TYPE_MAKE_SEND));

	MACH_CALL(add_to_port_set, (server, &port, 1));

	MACH_CALL(enable_port_set, (server));

	old_server = server;
	server = new_server;
	start_time();
	for (i=loops; i--;)
		null_snd_rcv(server);
	stop_time();
	MACH_CALL(disable_port_set, (server));
	server = old_server;
	stop_server(server);

	MACH_CALL(mach_port_destroy, (task, new_server));
}

random_null_rpc_test(nports, sparse)
{
	register int i;
	kern_return_t ret;
	mach_port_t *port_list, *random_ports, *port, old_server;
	mach_port_t task = mach_task_self();
	int npass = (loops+nports-1)/nports;

	port_list = allocate_ports(nports, sparse);

	MACH_CALL(add_to_port_set, (server, port_list, nports));

	MACH_CALL(enable_port_set, (server));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&random_ports,
				npass*nports*sizeof(mach_port_t),
				TRUE));
	
	random(nports, random_ports, npass-1);

	for (i=0; i<loops; i++) {
		random_ports[i] = port_list[(int)random_ports[i]];
	}       

	old_server = server;
	server = port_list[0];
	start_time();
	for (i=loops, port = random_ports; i--; port++)
		null_snd_rcv(*port);
	stop_time();
	MACH_CALL(disable_port_set, (server));
	server = old_server;
	stop_server(server);

	deallocate_ports(port_list, nports);
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )random_ports,
				  npass*nports*sizeof(mach_port_t)));
}

mach_port_t *
allocate_ports(n, sparse)
{
	mach_port_t *port, *port_list;
	mach_port_t task = mach_task_self();
	kern_return_t ret;
	register i;

	port_list = 0;

	MACH_CALL(vm_allocate, (mach_task_self(),
			      (vm_offset_t *)&port_list,
				n*sizeof(mach_port_t),
				TRUE));
	
	if (sparse) for (i=0, port = port_list; i < n; i++, port++)  {
		*port = first_port_name + i * sparse_factor; 
		MACH_CALL(mach_port_allocate_name, (task,
						    MACH_PORT_RIGHT_RECEIVE,
						    *port));
	} else for (i=n, port = port_list; i--; port++)  {
		MACH_FUNC(*port, mach_reply_port, ());
	}

	for (i=n, port = port_list; i--; port++)  {
		MACH_CALL(mach_port_insert_right, (task,
						   *port,
						   *port,
						   MACH_MSG_TYPE_MAKE_SEND));
	}
	return(port_list);
}

deallocate_ports(port_list, n)
mach_port_t *port_list;
{
	mach_port_t task = mach_task_self();
	mach_port_t *port;
	kern_return_t ret;
	register i;

	for (i=n, port = port_list; i--; port++)  {
		MACH_CALL(mach_port_destroy, (task, *port));
	}
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )port_list,
				  n*sizeof(mach_port_t)));
}

ipc_thread(port)
mach_port_t port;
{
	extern boolean_t ipc_demux();

	if (debug)
		printf("calling mach_msg_server\n");
	MACH_CALL(mach_msg_server, (ipc_demux,
				    IPC_MSG_BUF_SIZE,
				    port,
				    MACH_MSG_OPTION_NONE));
}

