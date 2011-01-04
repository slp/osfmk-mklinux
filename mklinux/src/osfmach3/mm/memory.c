/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/mach_interface.h>

#include <osfmach3/mach3_debug.h>

#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/segment.h>

/*
 * This routine is used to map in a page into an address space: needed by
 * execve() for the initial stack and environment pages.
 */
unsigned long put_dirty_page(struct task_struct * tsk, unsigned long page, unsigned long address)
{
	if (page >= high_memory)
		printk("put_dirty_page: trying to put page %08lx at %08lx\n",page,address);
	if (mem_map[MAP_NR(page)].count != 1)
		printk("mem_map disagrees with %08lx at %08lx\n",page,address);

	memcpy_tofs((void *) address, (void *) page, PAGE_SIZE);

	free_page(page);

	return page;	/* XXX */
}
