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


#define TEST_NAME	pager

#include <mach_perf.h>
#include <pager_types.h>

extern	pager_server_main();
int	vm_size;
int 	pager_debug;

char *private_options = "\n\
\t[-mem <% memory_use>]    Percentage of free+active memory to use.\n\
\t                         (default is 60 %)\n\
\t[-show_server]           Show server side timings.\n\
\t[-kill_server]           Kill server upon test completion.\n\
\t[-client]                Only start client side task.\n\
\t[-server]                Only start server side task.\n\
\t[-norma <node>]          Start server task on node <node>\n\
";

int vm_map_test(int, boolean_t);
int vm_unmap_test(boolean_t);
int pager_rw_test(int, int);
int attr_test(int, int);
int msync_test(int, int, int);
int data_supply_test (int, int);
#define DEALLOCATE 1

#define READ 0
#define WRITE 1
#define LOCK 2
#define NO_ACCESS 3

struct test tests[] = {
"vm_map/MO_init/change_attributes",0, vm_map_test, 0, 0, 0, 0,
"vm_map on a cached object",	0, vm_map_test,	CACHED, 0, 0, 0,
"",				0, 0, 0, 0, 0, 0,
"vm_deallocate/MO_terminate",	0, vm_unmap_test, NO_TERMINATE, 0, 0, 0,
"vm_deallocate on a cached object",0, vm_unmap_test, CACHED, 0, 0, 0,
"",				0, 0, 0, 0, 0, 0,
"Read/MO_data_request/unavailable",0, pager_rw_test, READ, 0, 0, 0,
"Read/MO_data_request/supply",	0, pager_rw_test, READ, FILL, 0, 0,
"Read on a cached object",	0, pager_rw_test, READ, CACHED|FILL, 0, 0, 
"",				0, 0, 0, 0, 0, 0,
"Write/MO_data_request/unavailable",0, pager_rw_test, WRITE, 0, 0, 0,
"Write/MO_data_request/supply",	0, pager_rw_test, WRITE, FILL, 0, 0,
"Write/MO_data_unlock/lock_req",0, pager_rw_test, LOCK, FILL, 0, 0, 
"Write on a cached object",	0, pager_rw_test, WRITE, CACHED|FILL, 0, 0, 
"",				0, 0, 0, 0, 0, 0,
"vm_msync/MO_data_ret/sync/completed",	0, msync_test, CACHED, WRITE,
				VM_SYNC_SYNCHRONOUS, 0, 
"vm_msync/invalidate/MO_sync/completed", 0, msync_test, CACHED, READ, 
				VM_SYNC_SYNCHRONOUS|VM_SYNC_INVALIDATE, 0, 
"vm_msync only (data untouched)",	0, msync_test, 0, NO_ACCESS, 
				VM_SYNC_SYNCHRONOUS, 0, 
"",				0, 0, 0, 0, 0, 0,
"MO_data_supply",		0, data_supply_test, FILL, 0, 0, 0,
"MO_get_attributes",		0, attr_test, GET_ATTR, 0, 0, 0,
"MO_change_attributes",		0, attr_test, SET_ATTR, 0, 0, 0,
"tests with precious pages (pager gives away pages)",	0, 0, 0, 0, 0, 0,

"Read/MO_data_request/supply",	0, pager_rw_test, READ, PRECIOUS|FILL, 0, 0,
"Write/MO_unlock/lock/data_ret",0, pager_rw_test, LOCK, PRECIOUS|FILL, 0, 0, 
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
int argc;
char *argv[];
{
	int i, server_death_warrant = 0;

	test_init();

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-mem")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
		  	mem_use = atoi(argv[i]); 
			use_opt = 1;
#if	!MONITOR
		} else if (!strcmp(argv[i], "-client")) {
                        client_only = 1;
                } else if (!strcmp(argv[i], "-server")) {
                        server_only = 1;
		} else if (!strcmp(argv[i], "-kill_server")) {
		  	server_death_warrant = 1; 
#endif	/* !MONITOR */
                } else if (!strcmp(argv[i], "-show_server")) {
                        server_times = 1;
		} else if (!strcmp(argv[i], "-norma")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &norma_node))
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
                        usage();

	pager_debug = debug;

	if (!client_only)
        	run_server((void(*)())pager_server_main);
	else {
		server_lookup(PAGER_SERVER_NAME);
	}

        if (server_only)
                return(0);

	vm_size = mem_size();
	run_tests(tests);
	
	if (!client_only || server_death_warrant)
                kill_server(server);

	test_end();
}

vm_map_test(int flags, boolean_t deallocate)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *mobjs, *mobj;
	vm_offset_t *addrs, *addr;
	mach_msg_type_number_t	count;

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&addrs,
				loops*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(create_memory_objects, (server,
					  loops,
					  &mobjs,
					  &count,
					  vm_page_size,
					  flags,	/* flags */
					  VM_PROT_ALL));
					 
	if (!(flags & CACHED)) {
	  	if (flags & NO_LISTEN) {
			ignore_server();
		}
		start_time();
	}

	if (pager_debug)
		printf("vm_map (pass 1)\n");

	for (i= 0, mobj = mobjs, addr = addrs;
	     i < loops;
	     i++, mobj++, addr++)  {
		MACH_CALL(vm_map, (task,
				   addr,
				   vm_page_size,
				   0,		/* mask */
				   TRUE,	/* anywhere */
				   *mobj,
				   0,		/* offset */
				   FALSE,	/* copy */
				   VM_PROT_READ,
				   VM_PROT_ALL,
				   VM_INHERIT_COPY));
		if (deallocate) {
			if (pager_debug)
				printf("vm_deallocate (first pass)\n");
			MACH_CALL(vm_deallocate, (task,
						  (vm_offset_t)*addr,
						  vm_page_size));
		}

	}

	if (!(flags & CACHED)) {
		stop_time();
	} else {
		/* 
		 * touch memory to be sure object initialization
		 * has completed
		 */
		if (pager_debug)
			printf("access memory\n");
		for (i=loops, addr = addrs; i--; addr++)  {
			count += *(int *)*addr;
		}
	}
	  	

	if (!deallocate) {
		if (pager_debug)
			printf("vm_deallocate (pass 1)\n");
		for (i=loops, addr = addrs; i--; addr++)  {
			MACH_CALL(vm_deallocate, (task,
						  *addr,
						  vm_page_size));
		}
	}

	if (flags & CACHED) {
		if (pager_debug)
			printf("vm_map (pass 2)\n");

		ignore_server();

		start_time();
		for (i= 0, mobj = mobjs, addr = addrs;
		     i < loops;
		     i++, mobj++, addr++)  {
			MACH_CALL(vm_map, (task,
					   addr,
					   vm_page_size,
					   0,		/* mask */
					   TRUE,	/* anywhere */
					   *mobj,
					   0,		/* offset */
					   FALSE,	/* copy */
					   VM_PROT_READ,
					   VM_PROT_ALL,
					   VM_INHERIT_COPY));
			if (deallocate) {
				if (pager_debug)
					printf("vm_deallocate (pass 2)\n");
				MACH_CALL(vm_deallocate, (task,
							  (vm_offset_t)*addr,
							  vm_page_size));
			}
		}
		stop_time();
		if (!deallocate) {
			if (pager_debug)
				printf("vm_deallocate (pass 2)\n");
			for (i=loops, addr = addrs; i--; addr++)  {
				MACH_CALL(vm_deallocate, (task,
							  *addr,
							  vm_page_size));
			}
		}
	}

	MACH_CALL(vm_deallocate, (task,
				  (vm_offset_t)addrs,
				  loops*sizeof(vm_offset_t)));

	if (pager_debug > 1)
		printf("destroy_memory_objects\n");

	MACH_CALL(destroy_memory_objects, (server,
					  mobjs,
					  loops));
					 
}

vm_unmap_test(flags)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *mobjs, *mobj;
	vm_offset_t *addrs, *addr;
	mach_msg_type_number_t	count;

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&addrs,
				loops*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(create_memory_objects,
		  (server,
		   loops,
		   &mobjs,
		   &count,
		   vm_page_size,
		   flags,
		   VM_PROT_ALL));
					 
	if (pager_debug > 1)
		printf("vm_unmap_test, count %d, mobjs (%x) = %x\n",
		       count, mobjs, *mobjs);
	
	for (i= 0, mobj = mobjs, addr = addrs;
	     i < loops;
	     i++, mobj++, addr++)  {
		MACH_CALL(vm_map, (task,
				   addr,
				   vm_page_size,
				   0,		/* mask */
				   TRUE,	/* anywhere */
				   *mobj,
				   0,		/* offset */
				   FALSE,	/* copy */
				   VM_PROT_READ,
				   VM_PROT_ALL,
				   VM_INHERIT_COPY));
	}

	if (flags & CACHED) {
		ignore_server();
	}

	/* 
	 * touch memory to be sure object initialization
	 * has completed
	 */
	if (pager_debug)
	  	printf("access memory\n");
	for (i=loops, addr = addrs; i--; addr++)  {
	  	count += *(int *)*addr;
	}

	start_time();

	for (i= 0, mobj = mobjs, addr = addrs;
	     i < loops;
	     i++, mobj++, addr++)  {
		MACH_CALL(vm_deallocate, (task,
					  (vm_offset_t)*addr,
					  vm_page_size));
	}

	stop_time();

	MACH_CALL(vm_deallocate, (task,
				  (vm_offset_t)addrs,
				  loops*sizeof(vm_offset_t)));

	if (pager_debug > 1)
		printf("vm_unmap_test, mobjs (%x) = %x\n", mobjs, *mobjs);
	MACH_CALL(destroy_memory_objects, (server,
					  mobjs,
					  loops));
					 
}

pager_rw_test(op, flags)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *mobj, *mobjs;
	vm_offset_t *addrs, *addr;
	mach_msg_type_number_t	count;
	vm_prot_t prot = VM_PROT_NONE;

	set_max_loops((flags & PRECIOUS) ? vm_size : vm_size/2);

	/*
	 * Kernel caches a maximun of 500 pages today
	 */
	if (flags & CACHED) 
		set_max_loops(500);

	switch(op) {
	case WRITE:
		prot |= VM_PROT_WRITE;
	case READ:
	case LOCK:
		prot |= VM_PROT_READ;
	}

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&addrs,
				loops*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(create_memory_objects, (server,
					  loops,
					  &mobjs,
					  &count,
					  vm_page_size,
					  flags,
					  prot));

					 
#define RW_MAP()						\
	for (i= 0, mobj = mobjs, addr = addrs;			\
	     i < loops;						\
	     i++, mobj++, addr++)  {				\
		MACH_CALL(vm_map, (task,			\
				   addr,			\
				   vm_page_size,		\
				   0,		/* mask */	\
				   TRUE,	/* anywhere */	\
				   *mobj,			\
				   0,		/* offset */	\
				   FALSE,	/* copy */	\
				   VM_PROT_ALL,			\
				   VM_PROT_ALL,			\
				   VM_INHERIT_COPY));		\
	}

#define RW_UNMAP()						\
	for (i= 0, mobj = mobjs, addr = addrs;			\
	     i < loops;						\
	     i++, mobj++, addr++)  {				\
		MACH_CALL(vm_deallocate, (task,			\
					  (vm_offset_t)*addr,	\
					  vm_page_size));	\
	}

#define RW_TEST()							\
	for (i = 0, addr = addrs; 					\
	     i < loops;							\
	     i++, addr++)  {				\
		if (op == READ) {					\
		    if (*(int *)*addr != READ_PATTERN && (flags & FILL)) \
			printf("read: invalid content (%x instead of %x)\n",\
			       *(int *)*addr, READ_PATTERN);		\
	        } else 							\
		    *(int *)*addr = WRITE_PATTERN;			\
	}

	if (flags & CACHED) {
		RW_MAP();
		RW_TEST();
		RW_UNMAP();
	}

	if (op == LOCK) {
		op = READ;
		RW_MAP();
		RW_TEST();
		op = WRITE;
	} else {
		RW_MAP();
	}

	if (flags & CACHED) {
		ignore_server();
	}

	start_time();
	RW_TEST();
	stop_time();

	RW_UNMAP();

	MACH_CALL(vm_deallocate, (task,
				  (vm_offset_t)addrs,
				  loops*sizeof(vm_offset_t)));

	MACH_CALL(destroy_memory_objects, (server, mobjs, loops));
}

kern_return_t
attr_test(int op, int flags)
{
	mach_port_t *mobj;
	mach_msg_type_number_t	count;
	vm_offset_t	addr;

	MACH_CALL(create_memory_objects, (server,
					  1,
					  &mobj,
					  &count,
					  vm_page_size,
					  flags,
					  VM_PROT_ALL));

	MACH_CALL(vm_map, (mach_task_self(),
			   &addr,
			   vm_page_size,
			   0,		/* mask */
			   TRUE,	/* anywhere */
			   *mobj,
			   0,		/* offset */
			   FALSE,	/* copy */
			   VM_PROT_READ,
			   VM_PROT_ALL,
			   VM_INHERIT_COPY));

	measure_in_server_task( MACH_CALL( mo_attr_test,
					  (server, *mobj, op)));

	MACH_CALL(vm_deallocate, (mach_task_self(), addr, vm_page_size));
	MACH_CALL(destroy_memory_objects, (server, mobj, 1));
}

msync_test(int flags, int access, int invalidate)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *mobjs, *mobj;
	vm_offset_t *addrs, *addr;
	mach_msg_type_number_t	count;

	set_max_loops((flags & PRECIOUS) ? vm_size : vm_size/2);

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&addrs,
				loops*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(create_memory_objects,
		  (server,
		   loops,
		   &mobjs,
		   &count,
		   vm_page_size,
		   flags,
		   VM_PROT_ALL));
					 
	
	for (i= 0, mobj = mobjs, addr = addrs;
	     i < loops;
	     i++, mobj++, addr++)  {
		MACH_CALL(vm_map, (task,
				   addr,
				   vm_page_size,
				   0,		/* mask */
				   TRUE,	/* anywhere */
				   *mobj,
				   0,		/* offset */
				   FALSE,	/* copy */
				   VM_PROT_ALL,
				   VM_PROT_ALL,
				   VM_INHERIT_COPY));
	}

	/* 
	 * write memory to be sure pages will be returned
	 */
	if (pager_debug)
	  	printf("access memory\n");
	for (i=loops, addr = addrs; i--; addr++)  {
		if (access == WRITE)
		  	*(int *)*addr = WRITE_PATTERN;
		else if (access == READ)
			count += *(int *)*addr;
	}

	if (access == NO_ACCESS) {
		ignore_server();
	}

	start_time();

	for (i= 0, mobj = mobjs, addr = addrs;
	     i < loops;
	     i++, mobj++, addr++)  {
		MACH_CALL(vm_msync, (task,
				     (vm_offset_t)*addr,
				     vm_page_size,
				     invalidate));
	}

	stop_time();


	for (i= 0, mobj = mobjs, addr = addrs;
	     i < loops;
	     i++, mobj++, addr++)  {
		MACH_CALL(vm_deallocate, (task,
					  (vm_offset_t)*addr,
					  vm_page_size));
	}

	MACH_CALL(vm_deallocate, (task,
				  (vm_offset_t)addrs,
				  loops*sizeof(vm_offset_t)));

	MACH_CALL(destroy_memory_objects, (server,
					  mobjs,
					  loops));
					 
}

kern_return_t
data_supply_test(int flags, int op)
{
	mach_port_t *mobj;
	mach_msg_type_number_t	count;
	vm_offset_t	addr;

	set_max_loops((flags & PRECIOUS) ? vm_size : vm_size/2);

	MACH_CALL(create_memory_objects, (server,
					  1,
					  &mobj,
					  &count,
					  loops*vm_page_size,
					  flags,
					  VM_PROT_ALL));

	MACH_CALL(vm_map, (mach_task_self(),
			   &addr,
			   vm_page_size,
			   0,		/* mask */
			   TRUE,	/* anywhere */
			   *mobj,
			   0,		/* offset */
			   FALSE,	/* copy */
			   VM_PROT_READ,
			   VM_PROT_ALL,
			   VM_INHERIT_COPY));

	measure_in_server_task( MACH_CALL( mo_data_supply_test,
					  (server, *mobj, op)));

	MACH_CALL(vm_deallocate, (mach_task_self(), addr, vm_page_size));
	MACH_CALL(destroy_memory_objects, (server, mobj, 1));
}
