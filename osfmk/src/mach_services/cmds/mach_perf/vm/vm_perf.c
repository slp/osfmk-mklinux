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


#define TEST_NAME	vm


#include <mach_perf.h>


int  		simple_mem_test();
int  		mem_test();
int		host_page_size_test();

int  		vm_allocate_test();
int 		vm_allocate_deallocate_test();
int		vm_behavior_set_test();
int		vm_copy_test();
int  		vm_deallocate_test();
int		vm_inherit_test();
int		vm_machine_attribute_test();
int		vm_map_null_test();
int		vm_map_null_deallocate_test();
int		vm_msync_test();
int		vm_protect_test();
int		vm_read_test();
int		vm_read_ovwr_test();
int		vm_region_test();
int		vm_remap_test();
int		vm_reserve_test();
int		vm_reserve_unreserve_test();
int		vm_unreserve_test();
int		vm_wire_test();
int		vm_write_test();

int  		nmaps = 0;
vm_offset_t	find_hole();

char *private_options = "\n\
\t[-mem <% memory_use>]    Percentage of free+active memory to use.\n\
\t                         (default is 60 %)\n\
\t[-nmap <nb_map_entries>] Number of map entries in the task vm map.\n\
\t                         (defaults are 10, 100 and 1000)\n\
\t[-norma <node>]          Start child task on node <node>\n\
";

#define ALLOCATE 0
#define DEALLOCATE 1
#define REGION 2

#define ZERO_PAGE 1
#define SIMPLE_ACCESS 2
#define COPY_PAGE 3
#define BZERO_PAGE 4
#define BCOPY_PAGE 5
#define CZERO_PAGE 6
#define CCOPY_PAGE 7

#define ZERO_FILL 1
#define SIMPLE_ACCESS 2
#define LAZY_FAULT 3

struct test tests[] = {
#if 0
"1 word access",		0, simple_mem_test, SIMPLE_ACCESS, 0, 0, 0,
#endif /* 0 */
"clear page (kernel)",		0, simple_mem_test, ZERO_PAGE, 0, 0, 0,
#if 0
"clear page (c)",		0, simple_mem_test, CZERO_PAGE, 0, 0, 0,
"clear page (library)",		0, simple_mem_test, BZERO_PAGE, 0, 0, 0,
#endif /* 0 */
"copy page (kernel)",		0, simple_mem_test, COPY_PAGE, 0, 0, 0,
#if 0
"copy page (c)",		0, simple_mem_test, CCOPY_PAGE, 0, 0, 0,
"copy page (library)",		0, simple_mem_test, BCOPY_PAGE, 0, 0, 0,
#endif /* 0 */
"host_page_size",		0, host_page_size_test, 0, 0, 0, 0,

"tests with + 10 active map entries",	0, 0, 0, 0, 0, 0,

"zero_fill (random)",		0, mem_test, ZERO_FILL, 10, 0, 0,
"zero_fill (seq)",		0, mem_test, ZERO_FILL, 10, 1, 0,
"lazy fault (random)",		0, mem_test, LAZY_FAULT, 10, 0, 0,
"lazy fault (seq)",		0, mem_test, LAZY_FAULT, 10, 1, 0,
"zero_fill (seq reversed)",	0, mem_test, ZERO_FILL, 10, 1, 1,
"lazy fault (seq reversed)",	0, mem_test, LAZY_FAULT, 10, 1, 1,
"vm_allocate (anywhere)",	0, vm_allocate_test, 10, 0, 0, 0,
"vm_allocate/deallocate",	0, vm_allocate_deallocate_test, 10, 0, 0, 0,
"vm_allocate(2p)/deallocate(1p)",0, vm_allocate_deallocate_test, 10, 1, 0, 0,
#ifdef	NOT_YET
"vm_behavior_set",		0, vm_behavior_set_test, 10, 0, 0, 0,
#endif
"vm_deallocate",		0, vm_deallocate_test, 10, 0, 0, 0,
"vm_inherit",			0, vm_inherit_test, 10, 0, 0, 0,
#ifdef	NOT_YET
"vm_machine_attribute",		0, vm_machine_attribute_test, 10, 0, 0, 0,
#endif
"vm_map 1 page (null object)",	0, vm_map_null_test, 10, -1, 0, 0,
"vm_map 1p/dealloc (null obj)",	0, vm_map_null_deallocate_test, 10, -1, 0, 0,
#ifdef	NOT_YET
"vm_map(null object) + touch",	0, vm_map_null_test, 10, -1, 1, 0,
"vm_msync 1p !invalidate",	0, vm_msync_test, 10, -1, 0, 0,
"vm_msync 1p invalidate",	0, vm_msync_test, 10, -1, 1, 0,
"vm_msync all !invalidate",	0, vm_msync_test, 10, 0, 0, 0,
"vm_msync all invalidate",	0, vm_msync_test, 10, 0, 1, 0,
#endif
"vm_protect 1 page",			0, vm_protect_test, 10, 0, 0, 0,
"vm_region",			0, vm_region_test, 10, 0, 0, 0,
#ifdef	NOT_YET
"vm_reserve",			0, vm_reserve_test, 10, 0, 0, 0,
"vm_reserve/unreserve",		0, vm_reserve_unreserve_test, 10, 0, 0, 0,
"vm_unreserve",			0, vm_unreserve_test, 10, 0, 0, 0,
#endif
"vm_wire 1 page",		0, vm_wire_test, 10, 0, 0, 0,

"tests with + 100 active map entries",	0, 0, 0, 0, 0, 0,

"zero_fill (random)",		0, mem_test, ZERO_FILL, 100, 0, 0,
"zero_fill (seq)",		0, mem_test, ZERO_FILL, 100, 1, 0,
"lazy fault (random)",		0, mem_test, LAZY_FAULT, 100, 0, 0,
"lazy fault (seq)",		0, mem_test, LAZY_FAULT, 100, 1, 0,
"vm_allocate/deallocate",	0, vm_allocate_deallocate_test, 100, 0, 0, 0,
"vm_region",			0, vm_region_test, 100, 0, 0, 0,

"tests with + 1000 active map entries",	0, 0, 0, 0, 0, 0,

"zero_fill (random)",		0, mem_test, ZERO_FILL, 1000, 0, 0,
"zero_fill (seq)",		0, mem_test, ZERO_FILL, 1000, 1, 0,
"lazy fault (random)",		0, mem_test, LAZY_FAULT, 1000, 0, 0,
"lazy fault (seq)",		0, mem_test, LAZY_FAULT, 1000, 1, 0,
"vm_allocate/deallocate",	0, vm_allocate_deallocate_test, 1000, 0, 0, 0,
"vm_region",			0, vm_region_test, 1000, 0, 0, 0,

"tests performed on a child task",	0, 0, 0, 0, 0, 0,

"vm_copy 1 page",		0, vm_copy_test, 10, 10, -1, 0,
"vm_copy 1 page (same page)",	0, vm_copy_test, 10, 10, -1, 1,
"vm_read 1 page", 		0, vm_read_test, 10, 10, -1, 0,
"vm_read 4 bytes",		0, vm_read_test, 10, 10, 4, 0,
"vm_read 4 pages",		0, vm_read_test, 10, 10, 4 * -1, 0,
"vm_read/deallocate 1 page",	0, vm_read_test, 10, 10, -1, 1,
"vm_read_overwrite 1 page",	0, vm_read_ovwr_test, 10, 10, -1, 0,
"vm_read_overwrite 4 bytes",	0, vm_read_ovwr_test, 10, 10, 4, 0,
"vm_read_overwrite 4 pages",	0, vm_read_ovwr_test, 10, 10, 4 * -1, 0,
"vm_remap 1 page + touch",	0, vm_remap_test, 10, 10, -1, 0,
#if 0
"vm_remap 1 p (inh copy) + touch",0, vm_remap_test, 10, 10, -1, 1,
#endif /* 0 */
"vm_write 1 page",	 	0, vm_write_test, 10, 10, -1, 0,
"vm_write 4 bytes",		0, vm_write_test, 10, 10, 4, 0,
"vm_write 4 pages",		0, vm_write_test, 10, 10, 4 * -1, 0,

"child task with 100 entries",	0, 0, 0, 0, 0, 0,

"vm_copy 1 page",		0, vm_copy_test, 10, 100, -1, 0,
"vm_read/deallocate 1 page",	0, vm_read_test, 10, 100, -1, 1,
"vm_read_overwrite 1 page",	0, vm_read_ovwr_test, 10, 100, -1, 0,
"vm_remap 1 page + touch ",	0, vm_remap_test, 10, 100, -1, 0,
"vm_write 1 page",	 	0, vm_write_test, 10, 100, -1, 0,

"child task with 1000 entries",	0, 0, 0, 0, 0, 0,

"vm_copy 1 page",		0, vm_copy_test, 10, 1000, -1, 0,
"vm_read/deallocate 1 page",	0, vm_read_test, 10, 1000, -1, 1,
"vm_read_overwrite 1 page",	0, vm_read_ovwr_test, 10, 1000, -1, 0,
"vm_remap 1 page + touch",	0, vm_remap_test, 10, 1000, -1, 0,
"vm_write 1 page",	 	0, vm_write_test, 10, 1000, -1, 0,

0, 0, 0, 0, 0, 0, 0
};

int *sync;
int vm_size;
int vm_debug;

vm_offset_t *create_map_entries();
vm_offset_t *reserve_map_entries();

#ifndef	VM_MAX_KERNEL_LOADED_ADDRESS
#	define VM_MAX_KERNEL_LOADED_ADDRESS	VM_MAX_KERNEL_ADDRESS
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++) 
		if (!strcmp(argv[i], "-mem")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
		  	mem_use = atoi(argv[i]); 
			use_opt = 1;
		} else if (!strcmp(argv[i], "-nmap")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
		  	nmaps = atoi(argv[i]); 
		} else if (!strcmp(argv[i], "-norma")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
			if (!atod(argv[i], &norma_node))
				usage();
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&sync,
				  vm_page_size,
				  TRUE));
	MACH_CALL( vm_inherit, (mach_task_self(),
			 (vm_offset_t)sync,
			 vm_page_size,
			 VM_INHERIT_SHARE));
	if (!in_kernel)
		remove_leading_holes(VM_MIN_ADDRESS,
			     find_hole(VM_MIN_ADDRESS,
				       VM_MAX_ADDRESS,
				       10000000));
	else
		remove_leading_holes(VM_MIN_KERNEL_LOADED_ADDRESS,
			     find_hole(VM_MIN_KERNEL_LOADED_ADDRESS,
				       VM_MAX_KERNEL_LOADED_ADDRESS,
				       10000000));
	
	vm_debug = debug;

	vm_size = mem_size();
	if (vm_debug) {
		if (!in_kernel)
			printf("%d map entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d map entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));
	}

	norma_kernel = kernel_is_norma();
	if (norma_kernel) {
		printf("Running on a NORMA kernel: vm_remap tests disabled.\n");
	}

	run_tests(tests);
	test_end();

}

vm_machine_attribute_test(nmap)
{
	register int i, j;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	int npages;	/* pages per entry */
	vm_offset_t addr;
	vm_machine_attribute_val_t value;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	set_max_loops(vm_size * 80 / 100);

	/*
	 * Arrange so that the map entry that will be looked for
	 * is uniformely distributed among the nmap entries.
	 */

	npages = ((loops + (nmap -1)) / nmap) + 1;
	size *= npages;

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be looked for.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be looked for.
	 */

	if (vm_debug)
		printf("selecting target pages to machine_attribute\n");
	if (vm_debug > 1)
		printf("target pages: ");

	for (i = 0; i < npages; i++) {
		random(nmap, &buf_list[i * nmap], 0);
		for (j = 0, buf = &buf_list[i * nmap]; j < nmap; j++, buf++) {
			/* always use 1 page of map entry */
			*buf *= npages;
		}
	}

	if (vm_debug > 1) {
		for (i=0; i < loops; i++) {
			printf("%x ", active_list[buf_list[i]]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));
	if (vm_debug)
		printf("vm_machine_attribute in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		value = MATTR_VAL_GET;
		MACH_CALL(vm_machine_attribute, (task,
						 active_list[*buf],
						 vm_page_size,
						 MATTR_CACHE,
						 &value));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}

vm_inherit_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	int npages;	/* pages per entry */
	vm_offset_t addr;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	set_max_loops(vm_size * 90 / 100);

	/*
	 * Arrange so that the map entry that will be looked for
	 * is uniformely distributed among the nmap entries.
	 */

	npages = ((loops + (nmap -1)) / nmap) + 1;
	size *= npages;

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be looked for.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be looked for.
	 */

	if (vm_debug)
		printf("selecting target pages to inherit\n");
	if (vm_debug > 1)
		printf("target pages: ");

	random(loops, &buf_list[0], 0);
	if (vm_debug > 1) {
		for (i=0; i < loops; i++) {
			printf("%x ", active_list[buf_list[i]]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));
	if (vm_debug)
		printf("vm_inherit in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_inherit, (task,
					active_list[*buf],
					vm_page_size,
					VM_INHERIT_NONE));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}

#ifdef	NOT_YET
vm_unreserve_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	vm_offset_t start;
	int npages;	/* pages per entry */
	
	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;
	
	/* 
	 * Arrange so that the map entry in which the page will
	 * be unreserved is uniformely distributed among the nmap
	 * entries.
	 */

	npages = ((loops + (nmap - 1)) / nmap) + 1;
	size *= (npages);

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = reserve_map_entries(nmap, npages, 1, 0, 1);

	/*
	 * allocate space for list of pages to be deleted
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be unreserved.
	 * The order is important, the unreserved page must always be the
	 * first one of an entry to keep the number of map entries
	 * identical.
	 */

	for (i=0; i < npages-1; i++)
		random(nmap, &buf_list[i*nmap], 0);

	if (vm_debug)
		printf("selecting target pages to unreserve\n");
	if (vm_debug > 1)
		printf("target pages: ");

	for (i=0, buf = buf_list; i < loops; i++, buf++) {
	  *buf = (*buf)*npages + (i/nmap);
		if (vm_debug > 1)
			printf("%x ", active_list[*buf]);
	}
		
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("unreserve in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_unreserve, (task,
				  active_list[*buf],
				  vm_page_size));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
		active_list[*buf] = 0;
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}
#endif

#ifdef	NOT_YET
vm_reserve_unreserve_test(nmap, diff_size)
{
	register int i;
	vm_offset_t *buf_list, *active_list, *dealloc_list;
	register vm_offset_t *buf, *act;
	mach_port_t task = mach_task_self();
	vm_offset_t start;
	int size = vm_page_size;	
	vm_offset_t *random_gaps, *gaps;
	int npass, npages;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	npages = ((loops + (nmap - 1)) / nmap) + 1;

	npass = (loops+nmap-1)/nmap;
	if (diff_size)
		size *= 2;

	MACH_CALL( vm_allocate, (mach_task_self(),
			  (vm_offset_t *)&random_gaps,
			  (npass*nmap)*sizeof(vm_offset_t),
			  TRUE));

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&gaps,
				  (nmap)*sizeof(vm_offset_t),
				  TRUE));
	
	for (i=0; i < nmap; i++)
		gaps[i] = 0;

	
	random(nmap, random_gaps, npass-1);
	for (i=0; i < loops; i++)
		gaps[random_gaps[i]] = gaps[random_gaps[i]] + npages + 1;

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)random_gaps,
			    (npass*nmap)*sizeof(vm_offset_t)));
	
	active_list = reserve_map_entries(nmap, npages, 1, gaps, 1);

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)gaps,
				  (nmap)*sizeof(vm_offset_t)));

	/*
	 * allocate space for list of pages we will reserve
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (loops)*sizeof(vm_offset_t),
				  TRUE));

	for (i=0, buf = buf_list; i < loops; i++, buf++)
		*buf = active_list[i] + vm_page_size * npages;

	/*
	 * allocate space for list of pages to delete
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&dealloc_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));


	/*
	 * Generate index (into active_list) of pages to be deleted.
	 * The order is important, the deleted page must always be the
	 * first one of an entry to keep the number of map entries
	 * identical.
	 */

	for (i=0; i < npages-1; i++)
		random(nmap, &dealloc_list[i*nmap], 0);

	if (vm_debug)
		printf("selecting target pages to unreserve\n");
	if (vm_debug > 1)
		printf("target pages:\n");
	for (i=0, buf = dealloc_list; i < loops; i++, buf++) {
	  *buf = (*buf)*npages + (i/nmap);
		if (vm_debug > 1)
			printf("%x ", active_list[*buf]);
	}
		
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("reserve/unreserve in loop\n");
	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_reserve, (task,
				  *buf,
				  size));
		if (vm_debug > 1)
			printf("reserve %x ", *buf);
		MACH_CALL( vm_unreserve, (task,
				  active_list[dealloc_list[i]],
				  vm_page_size));
		if (vm_debug > 1)
			printf(" unreserve %x\n",
			       active_list[dealloc_list[i]]);
		active_list[dealloc_list[i]] = 0;
	}
	stop_time();
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("unreserve reserved pages\n");

	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_unreserve, (task,
				  *buf,
				  size));
		if (vm_debug > 1)
			printf(" %x", *buf);
	}

	if (vm_debug)
		printf("deallocate list of reserved pages\n");

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (loops)*sizeof(vm_offset_t)));

	if (vm_debug)
		printf("deallocate list of pages to unreserve\n");

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)dealloc_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}
#endif

#ifdef	NOT_YET
vm_reserve_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	mach_port_t task = mach_task_self();
	vm_offset_t start;
	int size = vm_page_size;	
	vm_offset_t *random_gaps, *gaps;
	int npass;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	/* 
	 * Arrange so that the map entry in which the page will
	 * be reserved is uniformely distributed among the nmap
	 * entries.
	 */

	npass = (loops+nmap-1)/nmap;

	/*
	 * Allocate memory to generate index of vm regions where
	 * to insert each free page of gap
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
			  (vm_offset_t *)&random_gaps,
			  (npass*nmap)*sizeof(vm_offset_t),
			  TRUE));

	/*
	 * Allocate memory for list of gap sizes between vm regions
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&gaps,
				  (nmap)*sizeof(vm_offset_t),
				  TRUE));
	
	for (i=0; i < nmap; i++)
		gaps[i] = 0;

	
	random(nmap, random_gaps, npass-1);

	for (i=0; i < loops; i++)
		gaps[random_gaps[i]]++;

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)random_gaps,
			    (npass*nmap)*sizeof(vm_offset_t)));
	/*
	 * Create vm regions of 1 pages, that will grow
	 * at allocate time
	 */

	active_list = create_map_entries(nmap, 1, 0, gaps, 0);

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)gaps,
				  (nmap)*sizeof(vm_offset_t)));

	/*
	 * allocate space for list of pages we will reserve
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (loops)*sizeof(vm_offset_t),
				  TRUE));

	for (i=0, buf = buf_list; i < loops; i++, buf++)
		*buf = active_list[i] + vm_page_size;

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("\nreserving in loop:\n");
	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_reserve, (task,
					*buf,
					size));
		if (vm_debug > 1)
			printf(" %x", *buf);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_unreserve, (task,
					  *buf,
					  size));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (loops)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, 1);

}
#endif

#ifdef	NOT_YET
vm_behavior_set_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	int npages;	/* pages per entry */
	vm_offset_t addr;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	set_max_loops(vm_size * 90 / 100);

	/*
	 * Arrange so that the map entry that will be looked for
	 * is uniformely distributed among the nmap entries.
	 */

	npages = ((loops + (nmap -1)) / nmap) + 1;
	size *= npages;

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be looked for.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be looked for.
	 */

	if (vm_debug)
		printf("selecting target pages to behavior_set\n");
	if (vm_debug > 1)
		printf("target pages: ");

	random(loops, &buf_list[0], 0);
	if (vm_debug > 1) {
		for (i=0; i < loops; i++) {
			printf("%x ", active_list[buf_list[i]]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_behavior_set in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_behavior_set, (task,
					     active_list[*buf],
					     vm_page_size,
					     VM_BEHAVIOR_DEFAULT));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}
#endif

vm_msync_test(int nmap_child, int size, boolean_t invalidate)
{
	mach_port_t	child_task;
	int		npages_child, i;
	unsigned int	data_count;
	vm_offset_t	*active_list_child, *page;
	vm_offset_t	*buf, *buf_list, *data;

	if (nmaps) {
		nmap_child = nmaps;
		printf("nmap_child forced to %d\n", nmap_child);
	}
	if (!nmap_child)
		nmap_child++;
	if (size < 0)
		size = -size * vm_page_size;

	set_max_loops(vm_size * 80 / 100);

	/*
	 * Arrange so that the map entry that will be synced
	 * is uniformely distributed among the nmap entries.
	 */

	npages_child = ((loops + (nmap_child -1)) / nmap_child)
		+ ((size + vm_page_size - 1) / vm_page_size) + 1;
	active_list_child = create_map_entries(nmap_child, npages_child,
					       1, 0, 0);

	*sync = 0;
	child_task = mach_fork();
	if (! child_task) {
		if (vm_debug)
			printf("child task\n");

		/*
		 * Touch the memory
		 */
		for (i = nmap_child * npages_child, data = active_list_child;
		     i > 0; i--, data++) {
			*((int *)*data) = 0xf;
		}
			
		*sync = 1;
		MACH_CALL(task_suspend, (mach_task_self()));
	}

	if (vm_debug)
		printf("waiting child task\n");
	while (!*sync)
		MACH_CALL(thread_switch,
			  (0, SWITCH_OPTION_NONE, 0));

	/*
	 * Deallocate child map's memory in the parent.
	 */
	if (vm_debug) 
		printf("deleting child map entries()\n");
	if (vm_debug > 1)
		printf("deallocated pages:\n ");
	for (i=npages_child * nmap_child, page = active_list_child;
	     i--; page++) {
		if (!*page)
			continue;
		MACH_CALL( vm_deallocate, (mach_task_self(),
				  *page,
				  vm_page_size));
		if (vm_debug > 1)
			printf(" %x", *page);
	}

	/*
	 * allocate space for list of pages to be synced.
	 */

	if (size != 0) {
		MACH_CALL( vm_allocate, (mach_task_self(),
					 (vm_offset_t *)&buf_list,
					 loops * sizeof(vm_offset_t),
					 TRUE));
	}

	/*
	 * Generate index (into active_list_child) of pages to be synced.
	 */

	if (vm_debug)
		printf("selecting target pages to sync\n");
	if (vm_debug > 1)
		printf("target pages: ");

	if (size > vm_page_size) {
		for (i=0; i < loops; i += nmap_child)
			random(nmap_child, &buf_list[i], 0);
		for (i=0, buf = buf_list; i < loops; i++, buf++) {
			*buf = (*buf)*npages_child;
			if (vm_debug > 1)
				printf("%x ", active_list_child[*buf]);
		}
	} else if (size != 0) {
		random(loops, &buf_list[0], 0);
		if (vm_debug > 1) {
			for (i = 0, buf = buf_list; i < loops; i++, buf++)
				printf("%x ", active_list_child[*buf]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));


	if (vm_debug)
		printf("vm_msync in loop:\n");
	if (size == 0) {
		vm_offset_t	begin_offset, size_offset;

		if (!in_kernel) {
			begin_offset = VM_MIN_ADDRESS;
			size_offset = VM_MAX_ADDRESS - VM_MIN_ADDRESS;
		} else {
			begin_offset = VM_MIN_KERNEL_LOADED_ADDRESS;
			size_offset = VM_MAX_KERNEL_LOADED_ADDRESS -
				VM_MIN_KERNEL_LOADED_ADDRESS;
		}
		start_time();
		for (i = loops; i > 0; i--) {
			MACH_CALL(vm_msync, (child_task,
					     begin_offset, size_offset,
					     invalidate?VM_SYNC_INVALIDATE:0));
		}
		stop_time();
	} else {
		start_time();
		for (i = loops, buf = &buf_list[0];
		     i > 0; i--, buf++) {
			MACH_CALL(vm_msync, (child_task,
					     active_list_child[*buf],
					     size,
					     invalidate?VM_SYNC_INVALIDATE:0));
			if (vm_debug)
				printf(" %x", active_list_child[*buf]);
		}
		stop_time();
	}

	MACH_CALL(task_terminate, (child_task));
	MACH_CALL(mach_port_destroy,
		  (mach_task_self(), child_task));

	delete_map_entries(active_list_child, nmap_child, npages_child);
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) active_list_child,
				  nmap_child*npages_child*sizeof(vm_offset_t)));
	if (size != 0) {
		MACH_CALL(vm_deallocate, (mach_task_self(),
					  (vm_offset_t) buf_list,
					  loops * sizeof (vm_offset_t)));
	}
}

vm_map_null_deallocate_test(int nmap, int size, boolean_t access)
{
	mach_port_t	task = mach_task_self();
	int		npages, i;
	unsigned int	data_size;
	vm_offset_t	*active_list;
	vm_offset_t	dealloc;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;
	if (size < 0)
		size = -size * vm_page_size;

	npages = ((loops + (nmap -1)) / nmap) + 1;
	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_map/deallocate in loop:\n");
	start_time();
	for (i = loops; i > 0; i--) {
		MACH_CALL(vm_map, (task,
				   &dealloc,
				   size,
				   0,
				   TRUE,
				   MEMORY_OBJECT_NULL,
				   0,
				   TRUE,
				   VM_PROT_READ|VM_PROT_WRITE,
				   VM_PROT_READ|VM_PROT_WRITE,
				   VM_INHERIT_NONE));
		if (access)
			*((int *)(dealloc)) = 0xf;
		MACH_CALL(vm_deallocate, (task, dealloc, size));
	}
	stop_time();

	delete_map_entries(active_list, nmap, npages);
}

vm_map_null_test(int nmap, int size, boolean_t access)
{
	mach_port_t	task = mach_task_self();
	int		npages, i;
	unsigned int	data_size;
	vm_offset_t	*active_list;
	vm_offset_t	*dealloc, *dealloc_list;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;
	if (size < 0)
		size = -size * vm_page_size;

	set_max_loops(vm_size * 90 / 100);

	npages = ((loops + (nmap -1)) / nmap) + 1;
	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	MACH_CALL(vm_allocate, (task,
				(vm_offset_t *) &dealloc_list,
				loops * sizeof (vm_offset_t),
				TRUE));

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_map in loop:\n");
	start_time();
	for (i = loops, dealloc = &dealloc_list[0];
	     i > 0; i--, dealloc++) {
		MACH_CALL(vm_map, (task,
				   dealloc,
				   size,
				   0,
				   TRUE,
				   MEMORY_OBJECT_NULL,
				   0,
				   TRUE,
				   VM_PROT_READ|VM_PROT_WRITE,
				   VM_PROT_READ|VM_PROT_WRITE,
				   VM_INHERIT_NONE));
		if (access)
			*((int *)(*dealloc)) = 0xf;
	}
	stop_time();

	delete_map_entries(active_list, nmap, npages);
	for (i = 0; i < loops; i++) {
		MACH_CALL(vm_deallocate, (task, dealloc_list[i], size));
	}
	MACH_CALL(vm_deallocate, (task, (vm_offset_t) dealloc_list,
				  loops * sizeof (vm_offset_t)));
}

vm_remap_test(
	      int nmap_parent,
	      int nmap_child,
	      int size,
	      boolean_t copy)
{
	mach_port_t	child_task;
	int		npages_child, npages_parent, i;
	unsigned int	data_size;
	vm_offset_t	*active_list_child, *active_list_parent, *page;
	vm_offset_t	*buf, *buf_list, *dealloc_list, *data, *child_list;
	vm_prot_t	cur_prot, max_prot;
	boolean_t	touch_bug = TRUE; /* XXX Otherwise kernel panics */
	boolean_t	touch = FALSE;

	if (norma_kernel) {
		/*
		 * vm_remap is not supported on NORMA kernels.
		 * This will eventually result in a
		 * "Could not calibrate" message.
		 */
		return;
	}

	if (nmaps) {
		nmap_parent = nmaps;
		printf("nmap_parent forced to %d\n", nmap_parent);
	}
	if (!nmap_parent)
		nmap_parent++;
	if (!nmap_child)
		nmap_child++;
	if (size < 0)
		size = -size * vm_page_size;

	/*
	 * Arrange so that the map entry from which we will read
	 * is uniformely distributed among the nmap entries.
	 */

	npages_child = ((loops + (nmap_child -1)) / nmap_child)
		+ ((size + vm_page_size - 1) / vm_page_size) + 1;

	/*
	 * Allocate a page to share child page list
	 */

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(vm_inherit, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				VM_INHERIT_SHARE));

	/*
	 * allocate space for list of pages to be accessed.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  loops * sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list_child) of pages to be accessed.
	 */

	if (vm_debug)
		printf("selecting target pages to access\n");
	if (vm_debug > 1)
		printf("target pages: ");

	if (size > vm_page_size) {
		for (i=0; i < loops; i += nmap_child)
			random(nmap_child, &buf_list[i], 0);
		for (i=0, buf = buf_list; i < loops; i++, buf++) {
			*buf = (*buf)*npages_child;
			if (vm_debug > 1)
				printf("#%x ", *buf);
		}
	} else {
		random(loops, &buf_list[0], 0);
		if (vm_debug > 1) {
			for (i = 0, buf = buf_list; i < loops; i++, buf++)
				printf("#%x ", *buf);
		}
	}

	*sync = 0;
	child_task = mach_fork();
	if (! child_task) {
		if (vm_debug)
			printf("child task\n");
		
		/*
		 * Arrange so that the map entry from which we will read
		 * is uniformely distributed among the nmap entries.
		 */

		active_list_child = create_map_entries(nmap_child,
						       npages_child,
						       1, 0, 0);

		bcopy((char *)active_list_child, (char *)child_list,
		      nmap_child*npages_child*sizeof(vm_offset_t));

		/*
		 * XXX Fault in target pages to avoid kernel bug
		 */

		if (touch_bug)
	  		for (i = loops, buf = &buf_list[0];
			     i > 0; i--, buf++) {
				if (vm_debug) {
					printf("fault %x %x\n",
					       *buf, child_list[*buf]);
				}
				*(char *)(child_list[*buf]) = 1;
			}
		*sync = 1;
		MACH_CALL(task_suspend, (mach_task_self()));
	}

	npages_parent = ((loops + (nmap_parent -1)) / nmap_parent) + 1;
	active_list_parent = create_map_entries(nmap_parent, npages_parent,
						1, 0, 0);

	if (vm_debug)
		printf("waiting child task\n");
	while (!*sync)
		MACH_CALL(thread_switch,
			  (0, SWITCH_OPTION_NONE, 0));

	/*
	 * allocate space for addresses to deallocate on termination.
	 */
	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &dealloc_list,
				loops * sizeof (vm_offset_t),
				TRUE));

	/*
	 * Fault it in
	 */

	for (i=0; i < loops * sizeof (vm_offset_t); i += vm_page_size)
		*(((char *)dealloc_list)+i) = 1;

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	/*
	 * Fault in child list
	 */

	for (i=0;
	     i < nmap_child*npages_child;
	     i += vm_page_size/sizeof(vm_offset_t))
		page += *(child_list+i);

	if (vm_debug)
		printf("vm_remap in loop:\n");
	start_time();
	for (i = loops, buf = &buf_list[0], data = &dealloc_list[0];
	     i > 0; i--, buf++, data++) {
		MACH_CALL(vm_remap, (mach_task_self(),
				     data,
				     size,
				     0,
				     TRUE,
				     child_task,
				     child_list[*buf],
				     copy,
				     &cur_prot,
				     &max_prot,
				     VM_INHERIT_NONE));
		if (touch)
			**(int **)data = 0x1f;
		if (vm_debug)
			printf(" %x", child_list[*buf]);
	}
	stop_time();

	MACH_CALL(task_terminate, (child_task));
	MACH_CALL(mach_port_destroy,
		  (mach_task_self(), child_task));

	for (i = loops, data = &dealloc_list[0]; i > 0; i--, data++) {
		MACH_CALL(vm_deallocate, (mach_task_self(),
					  *data, size));
	}
	delete_map_entries(active_list_parent, nmap_parent, npages_parent);
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) dealloc_list,
				  loops * sizeof (vm_offset_t)));
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) buf_list,
				  loops * sizeof (vm_offset_t)));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t)));
}

vm_protect_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	int npages;	/* pages per entry */
	vm_offset_t addr;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	set_max_loops(vm_size * 80 / 100);

	/*
	 * Arrange so that the map entry that will be looked for
	 * is uniformely distributed among the nmap entries.
	 */

	npages = ((loops + (nmap -1)) / nmap) + 1;
	size *= npages;

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be looked for.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be looked for.
	 */

	if (vm_debug)
		printf("selecting target pages to protect\n");
	if (vm_debug > 1)
		printf("target pages: ");

	random(loops, &buf_list[0], 0);
	if (vm_debug > 1) {
		for (i=0; i < loops; i++) {
			printf("%x ", active_list[buf_list[i]]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_protect in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_protect, (task,
					active_list[*buf],
					vm_page_size,
					TRUE,
					VM_PROT_READ));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}

vm_copy_test(int nmap_parent, int nmap_child, int size, int same_page)
{
	mach_port_t	child_task;
	int		npages_child, npages_parent, i;
	unsigned int	data_count;
	vm_offset_t	*active_list_child, *active_list_parent;
	vm_offset_t	*child_list, *page;
	vm_offset_t	*buf, *buf_list, *data;
	boolean_t	touch_bug = TRUE; /* XXX Otherwise kernel panics */

	if (nmaps) {
		nmap_parent = nmaps;
		printf("nmap_parent forced to %d\n", nmap_parent);
	}
	if (!nmap_parent)
		nmap_parent++;
	if (!nmap_child)
		nmap_child++;
	if (size < 0)
		size = -size * vm_page_size;

	set_max_loops(vm_size * 45 / 100);

	npages_child = ((loops + (nmap_child -1)) / nmap_child)
	  + ((size + vm_page_size - 1) / vm_page_size) + 1;
	npages_child += (1-same_page);

	/*
	 * Allocate a page to share child page list
	 */

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(vm_inherit, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				VM_INHERIT_SHARE));

	/*
	 * allocate space for list of pages to be copied.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  loops * sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into child_list) of pages to be accessed.
	 */

	if (vm_debug)
		printf("selecting target pages to access\n");
	if (vm_debug > 1)
		printf("target pages: ");

	if (size > vm_page_size) {
		for (i=0; i < loops; i += nmap_child)
			random(nmap_child, &buf_list[i], 0);
		for (i=0, buf = buf_list; i < loops; i++, buf++) {
			*buf = (*buf)*npages_child;
			if (vm_debug > 1)
				printf("#%x ", *buf);
		}
	} else {
		random(loops, &buf_list[0], 0);
		if (vm_debug > 1) {
			for (i = 0, buf = buf_list; i < loops; i++, buf++)
				printf("#%x ", *buf);
		}
	}

	*sync = 0;
	child_task = mach_fork();
	if (! child_task) {
		if (vm_debug)
			printf("child task\n");

		/*
		 * Arrange so that the map entry from which we will read
		 * is uniformely distributed among the nmap entries.
		 */

		active_list_child = create_map_entries(nmap_child,
						       npages_child,
						       0, 0, 0);

		bcopy((char *)active_list_child, (char *)child_list,
		      nmap_child*npages_child*sizeof(vm_offset_t));

		/*
		 * XXX Fault in target pages to avoid kernel bug
		 */

		if (touch_bug)
	  		for (i = loops, buf = &buf_list[0];
			     i > 0; i--, buf++) {
				if (vm_debug) {
					printf("fault %x %x %x %x\n",
					       *buf, child_list[*buf],
					       *(buf+1-same_page),
					       child_list[*(buf+1-same_page)]);
				}
				*(char *)(child_list[*buf]) = 1;
				*(char *)(child_list[*(buf+1-same_page)]) = 1;
			}
		

		*sync = 1;
		MACH_CALL(task_suspend, (mach_task_self()));
	}

	npages_parent = ((loops + (nmap_parent -1)) / nmap_parent) + 1;
	active_list_parent = create_map_entries(nmap_parent, npages_parent,
						1, 0, 0);

	if (vm_debug)
		printf("waiting child task\n");
	while (!*sync)
		MACH_CALL(thread_switch,
			  (0, SWITCH_OPTION_NONE, 0));

	/*
	 * Fault in child list
	 */

	for (i=0;
	     i < nmap_child*npages_child;
	     i += vm_page_size/sizeof(vm_offset_t))
		page = (vm_offset_t *)*(child_list+i);

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_copy in loop:\n");
	start_time();
	for (i = loops, buf = &buf_list[0];
	     i > 0; i--, buf++) {
		if (vm_debug)
			printf(" %x", child_list[*buf]);
		MACH_CALL(vm_copy, (child_task,
				    child_list[*buf],
				    size,
				    child_list[*(buf+1-same_page)]));
	}
	stop_time();

	MACH_CALL(task_terminate, (child_task));
	MACH_CALL(mach_port_destroy,
		  (mach_task_self(), child_task));

	delete_map_entries(active_list_parent, nmap_parent, npages_parent);
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) buf_list,
				  loops * sizeof (vm_offset_t)));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t)));

}

vm_wire_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	int npages;	/* pages per entry */
	vm_offset_t addr;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	set_max_loops(vm_size * 80 / 100);

	/*
	 * Arrange so that the map entry that will be looked for
	 * is uniformely distributed among the nmap entries.
	 */

	npages = ((loops + (nmap -1)) / nmap) + 1;
	size *= npages;

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be looked for.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be looked for.
	 */

	if (vm_debug)
		printf("selecting target pages to wire\n");
	if (vm_debug > 1)
		printf("target pages: ");

	random(loops, &buf_list[0], 0);
	if (vm_debug > 1) {
		for (i=0; i < loops; i++) {
			printf("%x ", active_list[buf_list[i]]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	/*
	 * Touch the pages first
	 */

	for (i=0, buf = buf_list; i < loops; i++, buf++)
		*(char *)active_list[*buf] = 1;

	if (vm_debug)
		printf("vm_wire in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_wire, (privileged_host_port,
				     task,
				     active_list[*buf],
				     vm_page_size,
				     VM_PROT_READ));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}

vm_write_test(int nmap_parent, int nmap_child, int size)
{
	mach_port_t	child_task;
	int		npages_child, npages_parent, i;
	vm_offset_t	*active_list_child, *active_list_parent, *page;
	vm_offset_t	*buf, *buf_list, data, *child_list;
	boolean_t	touch_bug = TRUE; /* XXX Otherwise kernel panics */

	if (nmaps) {
		nmap_parent = nmaps;
		printf("nmap_parent forced to %d\n", nmap_parent);
	}
	if (!nmap_parent)
		nmap_parent++;
	if (!nmap_child)
		nmap_child++;
	if (size < 0)
		size = -size * vm_page_size;

	set_max_loops(vm_size * 80 / 100);


	/*
	 * Arrange so that the map entry written into
	 * is uniformely distributed among the nmap entries.
	 */

	npages_child = ((loops + (nmap_child -1)) / nmap_child)
		+ ((size + vm_page_size - 1) / vm_page_size) + 1;

	/*
	 * Allocate a page to share child page list
	 */

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(vm_inherit, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				VM_INHERIT_SHARE));

	
	/*
	 * allocate space for list of pages to be accessed.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  loops * sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list_child) of pages to be accessed.
	 */

	if (vm_debug)
		printf("selecting target pages to write\n");
	if (vm_debug > 1)
		printf("target pages: ");

	if (size > vm_page_size) {
		for (i=0; i < loops; i += nmap_child)
			random(nmap_child, &buf_list[i], 0);
		for (i=0, buf = buf_list; i < loops; i++, buf++) {
			*buf = (*buf)*npages_child;
			if (vm_debug > 1)
				printf("#%x ", *buf);
		}
	} else {
		random(loops, &buf_list[0], 0);
		if (vm_debug > 1) {
			for (i = 0, buf = buf_list; i < loops; i++, buf++)
				printf("#%x ", *buf);
		}
	}

	*sync = 0;
	child_task = mach_fork();
	if (! child_task) {
		if (vm_debug)
			printf("child task\n");
		/*
		 * Arrange so that the map entry written into
		 * is uniformely distributed among the nmap entries.
		 */

		active_list_child = create_map_entries(nmap_child,
						       npages_child,
						       0, 0, 0);
		bcopy((char *)active_list_child, (char *)child_list,
		      nmap_child*npages_child*sizeof(vm_offset_t));

		/*
		 * XXX Fault in target pages to avoid kernel bug
		 */

		if (touch_bug)
	  		for (i = loops, buf = &buf_list[0];
			     i > 0; i--, buf++) {
				if (vm_debug) {
					printf("fault %x %x\n",
					       *buf, child_list[*buf]);
				}
				*(char *)(child_list[*buf]) = 1;
			}

		*sync = 1;
		MACH_CALL(task_suspend, (mach_task_self()));
	}

	npages_parent = ((loops + (nmap_parent -1)) / nmap_parent) + 1;
	active_list_parent = create_map_entries(nmap_parent, npages_parent,
						1, 0, 0);
	if (vm_debug)
		printf("waiting child task\n");
	while (!*sync)
		MACH_CALL(thread_switch,
			  (0, SWITCH_OPTION_NONE, 0));

	/*
	 * allocate source data to be written.
	 */
	MACH_CALL(vm_allocate, (mach_task_self(), &data,
				size, TRUE));

	/*
	 * Fault in data
	 */

	for (i=0; i < size; i += vm_page_size)
		*(((char *)data)+i) = 1;


	/*
	 * Fault in child list
	 */

	for (i=0;
	     i < nmap_child*npages_child;
	     i += vm_page_size/sizeof(vm_offset_t))
		page = (vm_offset_t *)*(child_list+i);

	
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_write in loop:\n");
	start_time();
	for (i = loops, buf = &buf_list[0];
	     i > 0; i--, buf++) {
		MACH_CALL(vm_write, (child_task, child_list[*buf],
				     data, size));
		if (vm_debug)
			printf(" %x", child_list[*buf]);
	}
	stop_time();

	MACH_CALL(task_terminate, (child_task));
	MACH_CALL(mach_port_destroy,
		  (mach_task_self(), child_task));

	delete_map_entries(active_list_parent, nmap_parent, npages_parent);
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) buf_list,
				  loops * sizeof (vm_offset_t)));
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data, size));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t)));
}

vm_read_ovwr_test(int nmap_parent, int nmap_child, int size)
{
	mach_port_t	child_task;
	int		npages_child, npages_parent, i;
	unsigned int	data_count;
	vm_offset_t	*active_list_child, *active_list_parent, *page;
	vm_offset_t	*buf, *buf_list, *data, *child_list;
	boolean_t	touch_bug = TRUE; /* XXX Otherwise kernel panics */

	if (nmaps) {
		nmap_parent = nmaps;
		printf("nmap_parent forced to %d\n", nmap_parent);
	}
	if (!nmap_parent)
		nmap_parent++;
	if (!nmap_child)
		nmap_child++;
	if (size < 0)
		size = -size * vm_page_size;

	set_max_loops(vm_size * 45 / 100);

	/*
	 * Arrange so that the map entry from which we will read
	 * is uniformely distributed among the nmap entries.
	 */

	npages_child = ((loops + (nmap_child -1)) / nmap_child)
		+ ((size + vm_page_size - 1) / vm_page_size) + 1;

	/*
	 * Allocate a page to share child page list
	 */

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(vm_inherit, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				VM_INHERIT_SHARE));

	/*
	 * allocate space for list of pages to be accessed.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  loops * sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list_child) of pages to be accessed.
	 */

	if (vm_debug)
		printf("selecting target pages to access\n");
	if (vm_debug > 1)
		printf("target pages: ");

	if (size > vm_page_size) {
		for (i=0; i < loops; i += nmap_child)
			random(nmap_child, &buf_list[i], 0);
		for (i=0, buf = buf_list; i < loops; i++, buf++) {
			*buf = (*buf)*npages_child;
			if (vm_debug > 1)
				printf("#%x ", *buf);
		}
	} else {
		random(loops, &buf_list[0], 0);
		if (vm_debug > 1) {
			for (i = 0, buf = buf_list; i < loops; i++, buf++)
				printf("#%x ", *buf);
		}
	}

	*sync = 0;
	child_task = mach_fork();
	if (! child_task) {
		if (vm_debug)
			printf("child task\n");
		/*
		 * Arrange so that the map entry from which we will read
		 * is uniformely distributed among the nmap entries.
		 */

		active_list_child = create_map_entries(nmap_child,
						       npages_child,
						       1, 0, 0);

		bcopy((char *)active_list_child, (char *)child_list,
		      nmap_child*npages_child*sizeof(vm_offset_t));

		/*
		 * XXX Fault in target pages to avoid kernel bug
		 */

		if (touch_bug)
	  		for (i = loops, buf = &buf_list[0];
			     i > 0; i--, buf++) {
				if (vm_debug) {
					printf("fault %x %x\n",
					       *buf, child_list[*buf]);
				}
				*(char *)(child_list[*buf]) = 1;
			}
		*sync = 1;
		MACH_CALL(task_suspend, (mach_task_self()));
	}

	npages_parent = ((loops + (nmap_parent -1)) / nmap_parent)
		+ ((size + vm_page_size - 1) / vm_page_size) + 1;
	active_list_parent = create_map_entries(nmap_parent, npages_parent,
						1, 0, 0);

	if (vm_debug)
		printf("waiting child task\n");
	while (!*sync)
		MACH_CALL(thread_switch,
			  (0, SWITCH_OPTION_NONE, 0));

	/*
	 * Fault in child list
	 */

	for (i=0;
	     i < nmap_child*npages_child;
	     i += vm_page_size/sizeof(vm_offset_t))
		page = (vm_offset_t *)*(child_list+i);

	/*
	 * Fault in pages where data will be writen
	 */

	for (i = loops, buf = &buf_list[0]; i > 0; i--, buf++) 
		*(char *)(active_list_parent[*buf]) = 1;

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_read_overwrite in loop:\n");
	start_time();
	for (i = loops, buf = &buf_list[0];
	     i > 0; i--, buf++) {
		MACH_CALL(vm_read_overwrite, (child_task,
					      child_list[*buf],
					      size, active_list_parent[*buf],
					      &data_count));
		if (vm_debug)
			printf(" %x", child_list[*buf]);
	}
	stop_time();

	MACH_CALL(task_terminate, (child_task));
	MACH_CALL(mach_port_destroy,
		  (mach_task_self(), child_task));

	delete_map_entries(active_list_parent, nmap_parent, npages_parent);
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) buf_list,
				  loops * sizeof (vm_offset_t)));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t)));
}

vm_read_test(int nmap_parent, int nmap_child, int size, boolean_t dealloc)
{
	mach_port_t	child_task;
	int		npages_child, npages_parent, i;
	unsigned int	data_size;
	vm_offset_t	*active_list_child, *active_list_parent, *page;
	vm_offset_t	*buf, *buf_list, *dealloc_list, *data, *child_list;
	boolean_t	touch_bug = TRUE; /* XXX Otherwise kernel panics */

	if (nmaps) {
		nmap_parent = nmaps;
		printf("nmap_parent forced to %d\n", nmap_parent);
	}
	if (!nmap_parent)
		nmap_parent++;
	if (!nmap_child)
		nmap_child++;
	if (size < 0)
		size = -size * vm_page_size;

	/*
	 * Arrange so that the map entry from which we will read
	 * is uniformely distributed among the nmap entries.
	 */

	npages_child = ((loops + (nmap_child -1)) / nmap_child)
		+ ((size + vm_page_size - 1) / vm_page_size) + 1;

	/*
	 * Allocate a page to share child page list
	 */

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				TRUE));

	MACH_CALL(vm_inherit, (mach_task_self(),
				(vm_offset_t)child_list,
				nmap_child*npages_child*sizeof(vm_offset_t),
				VM_INHERIT_SHARE));

	/*
	 * allocate space for list of pages to be accessed.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  loops * sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list_child) of pages to be accessed.
	 */

	if (vm_debug)
		printf("selecting target pages to access\n");
	if (vm_debug > 1)
		printf("target pages: ");

	if (size > vm_page_size) {
		for (i=0; i < loops; i += nmap_child)
			random(nmap_child, &buf_list[i], 0);
		for (i=0, buf = buf_list; i < loops; i++, buf++) {
			*buf = (*buf)*npages_child;
			if (vm_debug > 1)
				printf("#%x ", *buf);
		}
	} else {
		random(loops, &buf_list[0], 0);
		if (vm_debug > 1) {
			for (i = 0, buf = buf_list; i < loops; i++, buf++)
				printf("#%x ", *buf);
		}
	}

	*sync = 0;
	child_task = mach_fork();
	if (! child_task) {
		if (vm_debug)
			printf("child task\n");
		/*
		 * Arrange so that the map entry from which we will read
		 * is uniformely distributed among the nmap entries.
		 */

		active_list_child = create_map_entries(nmap_child,
						       npages_child,
						       1, 0, 0);

		bcopy((char *)active_list_child, (char *)child_list,
		      nmap_child*npages_child*sizeof(vm_offset_t));

		/*
		 * XXX Fault in target pages to avoid kernel bug
		 */

		if (touch_bug)
	  		for (i = loops, buf = &buf_list[0];
			     i > 0; i--, buf++) {
				if (vm_debug) {
					printf("fault %x %x\n",
					       *buf, child_list[*buf]);
				}
				*(char *)(child_list[*buf]) = 1;
			}
		*sync = 1;
		MACH_CALL(task_suspend, (mach_task_self()));
	}

	npages_parent = ((loops + (nmap_parent -1)) / nmap_parent) + 1;
	active_list_parent = create_map_entries(nmap_parent, npages_parent,
						1, 0, 0);

	if (vm_debug)
		printf("waiting child task\n");
	while (!*sync)
		MACH_CALL(thread_switch,
			  (0, SWITCH_OPTION_NONE, 0));

	/*
	 * allocate space for addresses to deallocate on termination.
	 */

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &dealloc_list,
				loops * sizeof (vm_offset_t),
				TRUE));

	/*
	 * Fault in child list
	 */

	for (i=0;
	     i < nmap_child*npages_child;
	     i += vm_page_size/sizeof(vm_offset_t))
		page = (vm_offset_t *)*(child_list+i);

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_read in loop:\n");
	start_time();
	for (i = loops, buf = &buf_list[0], data = &dealloc_list[0];
	     i > 0; i--, buf++, data++) {
		MACH_CALL(vm_read, (child_task, child_list[*buf],
				    size, data, &data_size));
		if (vm_debug)
			printf(" %x", child_list[*buf]);
		if (dealloc) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  *data, size));
		}
	}
	stop_time();

	MACH_CALL(task_terminate, (child_task));
	MACH_CALL(mach_port_destroy,
		  (mach_task_self(), child_task));

	delete_map_entries(active_list_parent, nmap_parent, npages_parent);
	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)child_list,
			    nmap_child*npages_child*sizeof(vm_offset_t)));
	if (! dealloc) {
		for (i = loops, data = &dealloc_list[0]; i > 0; i--, data++) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  *data, size));
		}
	}
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) dealloc_list,
				  loops * sizeof (vm_offset_t)));
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) buf_list,
				  loops * sizeof (vm_offset_t)));
}

vm_region_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	int npages;	/* pages per entry */
	vm_offset_t addr;
	vm_size_t sz;
	vm_region_basic_info_data_t info;
	mach_msg_type_number_t count;
	mach_port_array_t obj_names;
	mach_port_t obj_name;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	/*
	 * Allocate port arrays for region names.
	 */
	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &obj_names,
				nmap * sizeof (mach_port_t),
				TRUE));

	/*
	 * Arrange so that the map entry that will be looked for
	 * is uniformely distributed among the nmap entries.
	 */

	npages = ((loops + (nmap -1)) / nmap) + 1;
	size *= npages;

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be looked for.
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be looked for.
	 */

	if (vm_debug)
		printf("selecting target pages to look for\n");
	if (vm_debug > 1)
		printf("target pages: ");

	random(loops, &buf_list[0], 0);
	if (vm_debug > 1) {
		for (i=0; i < loops; i++) {
			printf("%x ", active_list[buf_list[i]]);
		}
	}

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("vm_region in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		addr = active_list[*buf];
		sz = vm_page_size;
		count = VM_REGION_BASIC_INFO_COUNT;
		MACH_CALL( vm_region, (task,
				       &addr,
				       &sz,
				       VM_REGION_BASIC_INFO,
				       (vm_region_info_t) &info,
				       &count,
				       &obj_name));
		obj_names[*buf / npages] = obj_name;
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

	for (i = 0; i < nmap; i++) {
		if (obj_names[i] != MACH_PORT_NULL) {
			MACH_CALL(mach_port_destroy, (mach_task_self(),
						      obj_names[i]));
		}
	}
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) obj_names,
				  nmap * sizeof (mach_port_t)));
}

vm_deallocate_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	int size = vm_page_size;
	mach_port_t task = mach_task_self();
	vm_offset_t start;
	int npages;	/* pages per entry */
	
	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;
	
	/* 
	 * Arrange so that the map entry in which the page will
	 * be deallocated is uniformely distributed among the nmap
	 * entries.
	 */

	npages = ((loops + (nmap - 1)) / nmap) + 1;
	size *= (npages);

	if (vm_debug)
		printf("loops %d, nmap %d, npages %d, size %x\n",
		       loops, nmap, npages, size);

	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	/*
	 * allocate space for list of pages to be deleted
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));

	/*
	 * Generate index (into active_list) of pages to be deleted.
	 * The order is important, the deleted page must always be the
	 * first one of an entry to keep the number of map entries
	 * identical.
	 */

	for (i=0; i < npages-1; i++)
		random(nmap, &buf_list[i*nmap], 0);

	if (vm_debug)
		printf("selecting target pages to deallocate\n");
	if (vm_debug > 1)
		printf("target pages: ");

	for (i=0, buf = buf_list; i < loops; i++, buf++) {
	  *buf = (*buf)*npages + (i/nmap);
		if (vm_debug > 1)
			printf("%x ", active_list[*buf]);
	}
		
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("deallocate in loop:\n");

	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_deallocate, (task,
				  active_list[*buf],
				  vm_page_size));
		if (vm_debug > 1)
			printf(" %x", active_list[*buf]);
		active_list[*buf] = 0;
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}

vm_allocate_test(nmap)
{
	register int i;
	vm_offset_t *buf_list, *active_list;
	register vm_offset_t *buf, *act;
	mach_port_t task = mach_task_self();
	vm_offset_t start;
	int size = vm_page_size;	
	vm_offset_t *random_gaps, *gaps;
	int npass;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	/* 
	 * Arrange so that the map entry in which the page will
	 * be allocated is uniformely distributed among the nmap
	 * entries.
	 */

	npass = (loops+nmap-1)/nmap;

	/*
	 * Allocate memory to generate index of vm regions where
	 * to insert each free page of gap
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
			  (vm_offset_t *)&random_gaps,
			  (npass*nmap)*sizeof(vm_offset_t),
			  TRUE));

	/*
	 * Allocate memory for list of gap sizes between vm regions
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&gaps,
				  (nmap)*sizeof(vm_offset_t),
				  TRUE));
	
	for (i=0; i < nmap; i++)
		gaps[i] = 0;

	
	random(nmap, random_gaps, npass-1);

	for (i=0; i < loops; i++)
		gaps[random_gaps[i]]++;

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)random_gaps,
			    (npass*nmap)*sizeof(vm_offset_t)));
	/*
	 * Create vm regions of 1 pages, that will grow
	 * at allocate time
	 */

	active_list = create_map_entries(nmap, 1, 0, gaps, 0);

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)gaps,
				  (nmap)*sizeof(vm_offset_t)));

	/*
	 * allocate space for list of pages we will allocate
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (loops)*sizeof(vm_offset_t),
				  TRUE));

	for (i=0, buf = buf_list; i < loops; i++, buf++)
		*buf = 0;

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("\nallocating in loop:\n");
	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_allocate, (task,
				  buf,
				  size,
				  TRUE));
		if (vm_debug > 1)
			printf(" %x", *buf);
	}
	stop_time();

	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_deallocate, (task,
				  *buf,
				  size));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (loops)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, 1);

}

vm_allocate_deallocate_test(nmap, diff_size)
{
	register int i;
	vm_offset_t *buf_list, *active_list, *dealloc_list;
	register vm_offset_t *buf, *act;
	mach_port_t task = mach_task_self();
	vm_offset_t start;
	int size = vm_page_size;	
	vm_offset_t *random_gaps, *gaps;
	int npass, npages;

	if (nmaps) {
		nmap = nmaps;
		printf("nmap forced to %d\n", nmap);
	}
	if (!nmap)
		nmap++;

	npages = ((loops + (nmap - 1)) / nmap) + 1;

	npass = (loops+nmap-1)/nmap;
	if (diff_size)
		size *= 2;

	MACH_CALL( vm_allocate, (mach_task_self(),
			  (vm_offset_t *)&random_gaps,
			  (npass*nmap)*sizeof(vm_offset_t),
			  TRUE));

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&gaps,
				  (nmap)*sizeof(vm_offset_t),
				  TRUE));
	
	for (i=0; i < nmap; i++)
		gaps[i] = 0;

	
	random(nmap, random_gaps, npass-1);
	for (i=0; i < loops; i++)
		gaps[random_gaps[i]] = gaps[random_gaps[i]] + npages + 1;

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)random_gaps,
			    (npass*nmap)*sizeof(vm_offset_t)));
	
	active_list = create_map_entries(nmap, npages, 1, 0 /* gaps */, 0);

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)gaps,
				  (nmap)*sizeof(vm_offset_t)));

	/*
	 * allocate space for list of pages we will allocate
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf_list,
				  (loops)*sizeof(vm_offset_t),
				  TRUE));

	for (i=0, buf = buf_list; i < loops; i++, buf++)
		*buf = 0;

	/*
	 * allocate space for list of pages to delete
	 */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&dealloc_list,
				  (nmap*npages)*sizeof(vm_offset_t),
				  TRUE));


	/*
	 * Generate index (into active_list) of pages to be deleted.
	 * The order is important, the deleted page must always be the
	 * first one of an entry to keep the number of map entries
	 * identical.
	 */

	for (i=0; i < npages-1; i++)
		random(nmap, &dealloc_list[i*nmap], 0);

	if (vm_debug)
		printf("selecting target pages to deallocate\n");
	if (vm_debug > 1)
		printf("target pages:\n");
	for (i=0, buf = dealloc_list; i < loops; i++, buf++) {
	  *buf = (*buf)*npages + (i/nmap);
		if (vm_debug > 1)
			printf("%x ", active_list[*buf]);
	}
		
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("allocate/deallocate in loop\n");
	start_time();
	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_allocate, (task,
				  buf,
				  size,
				  TRUE));
		if (vm_debug > 1)
			printf("allocate %x ", *buf);
		MACH_CALL( vm_deallocate, (task,
				  active_list[dealloc_list[i]],
				  vm_page_size));
		if (vm_debug > 1)
			printf(" deallocate %x\n",
			       active_list[dealloc_list[i]]);
		active_list[dealloc_list[i]] = 0;
	}
	stop_time();
	if (vm_debug)
		if (!in_kernel)
			printf("%d entries\n",
			       vm_regions(VM_MIN_ADDRESS,
					  VM_MAX_ADDRESS));
		else
			printf("%d entries\n",
			       vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
					  VM_MAX_KERNEL_LOADED_ADDRESS));

	if (vm_debug)
		printf("deallocate allocated pages\n");

	for (i=0, buf = buf_list; i < loops; i++, buf++) {
		MACH_CALL( vm_deallocate, (task,
				  *buf,
				  size));
		if (vm_debug > 1)
			printf(" %x", *buf);
	}

	if (vm_debug)
		printf("deallocate list of allocated pages\n");

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf_list,
			    (loops)*sizeof(vm_offset_t)));

	if (vm_debug)
		printf("deallocate list of pages to deallocate\n");

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)dealloc_list,
			    (nmap*npages)*sizeof(vm_offset_t)));

	delete_map_entries(active_list, nmap, npages);

}

simple_mem_test(step) {
	register int i;
	int *buf;
	int size;
	register int val, *ptr, *cpy;
	register int increment;

	size = loops * vm_page_size;

	set_min_loops(256*1024/vm_page_size);

	if (step != SIMPLE_ACCESS) {
		set_max_loops(vm_size);
		size = loops * vm_page_size;
		increment = vm_page_size / sizeof(int);
	} else {
		size = loops * sizeof(int);
		increment = 1;
	}

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf,
				  size,
				  TRUE));

	for (i = loops, ptr = buf;
	     i-- ;
	     ptr += increment)
		val = *ptr;
	if (step == SIMPLE_ACCESS) {
		start_time();
		for (i = size/sizeof(int), ptr = buf;
		     i-- ;
		     ptr += increment)
			val = *ptr;
		stop_time();
	} else if (step == BZERO_PAGE) {
		start_time();
		for (i = loops, ptr = buf; i--; ptr += increment)
			bzero((char *)ptr, vm_page_size);
		stop_time(); 
	} else if (step == ZERO_PAGE) {
		start_time();
		for (i = loops, ptr = buf; i--; ptr += increment)
			zero_page(ptr, vm_page_size);
		stop_time(); 
	} else if (step == CZERO_PAGE) {
		start_time();
		for (i = size/sizeof(int), ptr = buf;
		     i-- ;)
			*ptr++ = 0 ;
		stop_time(); 
	} else if (step == BCOPY_PAGE) {
		start_time();
		for (i = loops, ptr = buf,
		     cpy = buf + size/sizeof(int) - increment;
		     i--; ptr += increment, cpy -= increment)
			bcopy((char *)ptr, (char *)cpy, vm_page_size);
		stop_time();
	} else if (step == COPY_PAGE) {
		start_time();
		for (i = loops, ptr = buf,
		     cpy = buf + size/sizeof(int) - increment;
		     i--; ptr += increment, cpy -= increment)
			copy_page(ptr, cpy, vm_page_size);
		stop_time();
	} else if (step == CCOPY_PAGE) {
		start_time();
		for (i = size/sizeof(int), ptr = buf, cpy = buf + i - 1;
		     i-- ;)
			*cpy-- = *ptr++;
		stop_time();
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf,
			    size));
	return(val);
}


host_page_size_test()
{
	vm_offset_t size;
	mach_port_t host = mach_host_self();
	register i;

	start_time();

	for (i = loops; i > 0; i--) {
		MACH_CALL( host_page_size, (host, &size));
	}

	stop_time();

	MACH_CALL( mach_port_deallocate, (mach_task_self(), host));
}

mem_test(step, nmap, seq, reverse) {
	register int i;
	int **buf;
	int size;
	register int val, **ptr;
	mach_port_t slave_task;
	vm_offset_t *active_list;
	int	npages;

	set_max_loops(vm_size);
	size = loops * sizeof(int);

	npages = ((loops + (nmap - 1)) / nmap) + 1;
	active_list = create_map_entries(nmap, npages, 1, 0, 0);

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&buf,
				  size,
				  TRUE));

	if (!seq) {
		random(loops, buf, 0);
		for (i=0; i<loops; i++)
			buf[i] = (int *)active_list[(int)buf[i]];
	} else
		for (i=0; i<loops; i++)
			if (reverse)
				buf[i] = (int *)active_list[loops-(i+1)];
			else
				buf[i] = (int *)active_list[i];

	if (step == LAZY_FAULT)
		for (i=0; i < nmap; i++) {
			MACH_CALL( vm_inherit, (mach_task_self(),
						active_list[npages*i],
						vm_page_size*npages,
						VM_INHERIT_SHARE));
		}

	if (step == ZERO_FILL) {
		start_time();
		for (i = loops, ptr = buf; i-- ; ptr++)
			val = **ptr ;
		stop_time(); 
	} else if (step == LAZY_FAULT) {
		*sync = 0;
		if ((slave_task = mach_fork())) {
			if (vm_debug)
				printf("waiting child task\n");
			while (!*sync)
				MACH_CALL(thread_switch,
					  (0, SWITCH_OPTION_NONE, 0));
			MACH_CALL(task_terminate, (slave_task));
			MACH_CALL(mach_port_destroy,
				  (mach_task_self(), slave_task));
		} else {
			if (vm_debug)
				printf("child task\n");
			for (i = loops; i--;)
				val = *buf[i];
			*sync = 1;
			if (vm_debug)
				printf("child task over\n");
			MACH_CALL(task_suspend, (mach_task_self()));
		}
		start_time();
		for (i = loops, ptr = buf; i--; ptr ++) 
			val = **ptr ;
		stop_time();
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)buf,
			    size));
	delete_map_entries(active_list, nmap, npages);
	return(val);
}

/*
 * Allocate VM in such a way that the kernel will create "nmap"
 * map entries of npages. The gap between memory regions is either
 * "gap" pages or random gap sizes specified in the "random gap"
 * array. If a gap is null, use vm_inherit so that memory region
 * will not be coallesced with previous one
 */

vm_offset_t *
create_map_entries(nmap, npages, gap, random_gap, touch_memory)
int *random_gap;
{
	vm_offset_t *page_list, *page;	/* list of allocated pages */
	vm_offset_t start;
	register i;
	vm_offset_t addr;

	/* allocate list of page entries */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&page_list,
				  nmap*npages*sizeof(vm_offset_t),
				  TRUE));

	if (!in_kernel)
		addr = find_hole(VM_MIN_ADDRESS, VM_MAX_ADDRESS, 10000000);
	else
		addr = find_hole(VM_MIN_KERNEL_LOADED_ADDRESS,
				 VM_MAX_KERNEL_LOADED_ADDRESS, 10000000);

	if (vm_debug )
		printf("create_map_entrie(s)\n ");
	if (vm_debug > 1)
		printf("allocated pages:\n ");
	for (i=0, page = page_list; i<nmap; i++) {
		int j;
		MACH_CALL( vm_allocate, (mach_task_self(),
				  &addr,
				  npages*vm_page_size,
				  FALSE));
		for(j=0; j < npages; j++, page++) {
			*page = addr + vm_page_size * j;
			if (touch_memory)
				*((char *)(addr + vm_page_size * j)) = 0xf;
			if (vm_debug > 1)
				printf(" %x", *page);
		}
		if (vm_debug > 1)
			printf("\n");
		if (random_gap)
			gap = *random_gap++;
		if (!gap) {
			MACH_CALL( vm_inherit, (mach_task_self(),
			 	(vm_offset_t)addr,
			 	npages*vm_page_size,
			 	VM_INHERIT_SHARE));
		}
		addr += (npages+gap)*vm_page_size;
		if (vm_debug > 2)
			printf(" next gap %d", gap);
	}
	return(page_list);
}

#ifdef	NOT_YET
vm_offset_t *
reserve_map_entries(nmap, npages, gap, random_gap, touch_memory)
int *random_gap;
{
	vm_offset_t *page_list, *page;	/* list of allocated pages */
	vm_offset_t start;
	register i;
	vm_offset_t addr;

	/* allocate list of page entries */

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&page_list,
				  nmap*npages*sizeof(vm_offset_t),
				  TRUE));

	if (!in_kernel)
		addr = find_hole(VM_MIN_ADDRESS, VM_MAX_ADDRESS, 10000000);
	else
		addr = find_hole(VM_MIN_KERNEL_LOADED_ADDRESS,
				 VM_MAX_KERNEL_LOADED_ADDRESS, 10000000);

	if (vm_debug )
		printf("reserve_map_entries()\n ");
	if (vm_debug > 1)
		printf("reserved pages:\n ");
	for (i=0, page = page_list; i<nmap; i++) {
		int j;
		MACH_CALL( vm_reserve, (mach_task_self(),
				  addr,
				  npages*vm_page_size,
				  FALSE));
		for(j=0; j < npages; j++, page++) {
			*page = addr + vm_page_size * j;
			if (vm_debug > 1)
				printf(" %x", *page);
		}
		if (vm_debug > 1)
			printf("\n");
		if (random_gap)
			gap = *random_gap++;
		if (!gap) {
			MACH_CALL( vm_inherit, (mach_task_self(),
			 	(vm_offset_t)addr,
			 	npages*vm_page_size,
			 	VM_INHERIT_SHARE));
		}
		addr += (npages+gap)*vm_page_size;
		if (vm_debug > 2)
			printf(" next gap %d", gap);
	}
	return(page_list);
}
#endif

delete_map_entries(page_list, nmap, npages)
vm_offset_t *page_list;
{
	vm_offset_t *page;
	register i;

	if (vm_debug) 
		printf("delete_map_entries()\n");
	if (vm_debug > 1)
		printf("deallocated pages:\n ");
	for (i=npages*nmap, page = page_list; i--; page++) {
		if (!*page)
			continue;
		MACH_CALL( vm_deallocate, (mach_task_self(),
				  *page,
				  vm_page_size));
		if (vm_debug > 1)
			printf(" %x", *page);
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
			    (vm_offset_t)page_list,
			    nmap*npages*sizeof(vm_offset_t)));
}

vm_regions(start, end)
{
	vm_offset_t	addr;
	vm_size_t	size;
        vm_prot_t       prot;
        vm_inherit_t    inherit;
	mach_port_t	name;
	int		count = 0;

	for (addr = start;
	     addr < end;
	     addr += size, count++) {
             if(get_vm_region_info(mach_task_self(),
                                        &addr, &size, &prot,
                                        &inherit, &name) != KERN_SUCCESS)
			break;
		mach_port_deallocate (mach_task_self(), name);
		if (vm_debug > 2)
			printf("%8x %8x\n", addr, size);
	}
	if (vm_debug > 2)
		printf("%d entries\n", count);
	return(count);
}

vm_offset_t
find_hole(start, end, wanted_size)
{
	vm_offset_t	addr, last_addr;
	vm_size_t	size;
        vm_prot_t       prot;
        vm_inherit_t    inherit;
	unsigned int	count = 0;
	mach_port_t	name;

	for (addr = start;
	     addr < end;
	     addr += size, count++) {
	  	last_addr = addr;
                if(get_vm_region_info(mach_task_self(),
                                        &addr, &size, &prot,
                                        &inherit, &name) == KERN_SUCCESS)
			mach_port_deallocate (mach_task_self(), name);
		if (addr - last_addr >= wanted_size) {
			return(last_addr);
		}
	}
}

remove_leading_holes(start, end)
{
	vm_offset_t	addr, last_addr;
	vm_size_t	size;
        vm_prot_t       prot;
        vm_inherit_t    inherit;
	mach_port_t	name;
	vm_offset_t	offset;
	unsigned int	count = 0;

	for (addr = start;
	     addr < end;
	     addr += size, count++) {
	  	last_addr = addr;
                get_vm_region_info(mach_task_self(),
                                        &addr, &size, &prot,
                                        &inherit, &name);
		mach_port_deallocate (mach_task_self(), name);

		if (addr > last_addr) {
			MACH_CALL(vm_allocate, (mach_task_self(),
						&last_addr,
						addr-last_addr,
						FALSE));

			MACH_CALL(vm_protect, (mach_task_self(),
					       last_addr,
					       addr-last_addr,
					       FALSE,
					       VM_PROT_NONE));
			if (vm_debug > 1)
				printf("remove %8x %8x\n", last_addr,
				       addr - last_addr);
		}
	}
}

holes_test()
{
	vm_offset_t start;

	if (!in_kernel)
		start = find_hole(VM_MIN_ADDRESS, VM_MAX_ADDRESS, 10000000);
	else
		start = find_hole(VM_MIN_KERNEL_LOADED_ADDRESS,
				 VM_MAX_KERNEL_LOADED_ADDRESS, 10000000);

	if (vm_debug > 1)
		printf("holes test: start %x\n", start);

	if (!in_kernel) {
		vm_regions(VM_MIN_ADDRESS, VM_MAX_ADDRESS);
		remove_leading_holes(VM_MIN_ADDRESS, start);
		vm_regions(VM_MIN_ADDRESS, VM_MAX_ADDRESS);
	} else {
		vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
			   VM_MAX_KERNEL_LOADED_ADDRESS);
		remove_leading_holes(VM_MIN_KERNEL_LOADED_ADDRESS, start);
		vm_regions(VM_MIN_KERNEL_LOADED_ADDRESS,
			   VM_MAX_KERNEL_LOADED_ADDRESS);
	}
}
