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

#define TEST_NAME	hw

#include <mach_perf.h>
#include <string.h>

char *private_options = "\n\
\t[-size bytes [k]]        copy/clear sizes. Unit is bytes or kbytes\n\
\t                         if 'k' is added.\n\
\t[-offset bytes]          Used to measure data cache side effects.\n\
\t                         This offset is added to buffer addresses.\n\
\t[-alt]                   Used for the register loop test. The code\n\
\t                         location can influence performance. Using this\n\
\t                         option the code is executed from a distinct\n\
\t                         location in memory.\n\
\t[-use_libc_funcs]        Use libc routines for copy and clear tests.\n\
\t[-use_kernel_funcs]      Use same code as microkernel one for\n\
\t                         copy and clear tests.\n\
\t[-use_c_lang_funcs]      Use simple C code for copy and clear tests.\n\
";

int register_loop();
int copy_loop();
int clear_loop();
#ifndef	NO_LOCK
int lock_loop();
#endif /* NO_LOCK */

int bcopy(), bzero();
int hw_copy_page(), hw_zero_page();
static int c_bcopy(), c_bzero();
int alternate;

int	(*copy_func)() = hw_copy_page;
int	(*clear_func)() = hw_zero_page;

#define CHIP_CACHE_SIZE (4*1024)
#define CACHE_SIZE (16*1024)
#define MEM_SIZE 1024*1024
#define ONE_K 1024

struct test tests[] = {
"register loop",		0, register_loop, 0, 0, 0, 0,
"mem copy, 4 KB space, KB/s",	0, copy_loop, CHIP_CACHE_SIZE, 0, 0, 0,
"mem copy, 16 KB space, KB/s",	0, copy_loop, CACHE_SIZE, 0, 0, 0,
"mem copy  1 MB space, KB/s",	0, copy_loop, MEM_SIZE, 0, 0, 0,
"mem clear, 4 KB space, KB/s",	0, clear_loop, CHIP_CACHE_SIZE, 0, 0, 0,
"mem clear, 16 KB space, KB/s",	0, clear_loop, CACHE_SIZE, 0, 0, 0,
"mem clear  1 MB space, KB/s",	0, clear_loop, MEM_SIZE, 0, 0, 0,
#ifndef	NO_LOCK
"lock/unlock  1 Mbytes",	0, lock_loop, 0, 0, 0, 0,
#endif	/* NO_LOCK */
0, 0, 0, 0, 0, 0, 0
};

int sync;
int req_size;
int offset;
int last_copy_size;
char *mem = (char *) 0;

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	req_size = 0;
	last_copy_size = 0;
	offset = 0;
	alternate = 0;
	test_init();
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-size")) {
			if (i+1 >= argc || *argv[i+1] == '-')
				usage();
			else if (!atod(argv[++i], &req_size))
				usage();
			if (i+1 < argc &&
			    (*argv[i+1] == 'K' || *argv[i+1] == 'k')) {
				i++;
				req_size *= 1024;
			}
		} else if (!strcmp(argv[i], "-off")) {
			if (i+1 >= argc || *argv[i+1] == '-')
				usage();
			else if (!atod(argv[++i], &offset))
				usage();
		} else if (!strcmp(argv[i], "-alt")) {
		  	alternate = 1;
		} else if  (!strcmp(argv[i], "-use_libc_funcs")) {
		  	copy_func = (int(*) ()) bcopy;
		  	clear_func = (int(*) ()) bzero;
		} else if (!strcmp(argv[i], "-use_kernel_funcs")) {
		  	copy_func = hw_copy_page;
		  	clear_func = hw_zero_page;
		} else if (!strcmp(argv[i], "-use_c_lang_funcs")) {
		  	copy_func = c_bcopy;
			clear_func = c_bzero;
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();
	if (!mem) {
		MACH_CALL( vm_allocate_temporary, (mach_task_self(),
					 (vm_offset_t *)&mem,
					 MEM_SIZE,
					 TRUE));
	}
	run_tests(tests);
	if (mem) {
		MACH_CALL( vm_deallocate, (mach_task_self(),
					 (vm_offset_t) mem,
					 MEM_SIZE));
		mem = (char *)0;
	}
	test_end();

}

extern alt_register_loop();

register_loop()
{
	unsigned register i = loops;
	
	if (alternate) {
		if (alternate == 1) {
		     printf("using alternate code @ %x instead of %x\n",
			    alt_register_loop, register_loop);
		     alternate++;
		}
		return(alt_register_loop());
	}
	start_time();
	while(i--);
	stop_time();
	return(i);
}

alt_register_loop()
{
	unsigned register i = loops;
	
	start_time();
	while(i--);
	stop_time();
	return(i);
}

copy_loop(size)
{
  	unsigned register i;
	char *from, *to;

	if (req_size && req_size != size ) {
	  	if (last_copy_size != req_size)
			printf("size changed from %d bytes to %d bytes\n",
			       size, req_size);
		last_copy_size = req_size;
		size = req_size;
	}
	size /= 2;
	set_min_loops(size/ONE_K);
	i = (loops * ONE_K)/size;
	from = &mem[0]+offset;
	to = &mem[size]+offset;

	start_time();
	while(i--)
		(*copy_func)(from, to, size);
	stop_time();
	return(i);
}

clear_loop(size)
{
  	unsigned register i;
	char *addr;

	set_min_loops(size/ONE_K);
	i = (loops * ONE_K)/size;
	addr = &mem[0]+offset;

	start_time();
	while(i--)
		(*clear_func)(addr, size);
	stop_time();
	return(i);
}

static
c_bcopy(from, to, count)
register char *from, *to;
register count;
{
	while (count--)
		*to++ = *from++;
}

static
c_bzero(addr, count)
register char *addr;
register count;
{
	while (count--)
		*addr++ = 0;
}



