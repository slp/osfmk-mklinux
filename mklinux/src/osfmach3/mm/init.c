/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/config.h>

#include <mach/vm_statistics.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_interface.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/swap.h>

extern mach_port_t	privileged_host_port;

extern unsigned long initial_start_mem;

extern void show_net_buffers(void);

void show_mem(void)
{
	struct sysinfo si;
	int i,free = 0,total = 0,reserved = 0;
	int shared = 0;

	si_swapinfo(&si);

	printk("Mem-info:\n");
	show_free_areas();
	printk("Free swap:       %6ldkB\n", si.freeswap >> 10);
	i = MAP_NR(high_memory);
	while (i-- > 0) {
		total++;
		if (PageReserved(mem_map+i))
			reserved++;
		else if (!mem_map[i].count)
			free++;
		else
			shared += mem_map[i].count-1;
	}
	printk("%d pages of RAM\n",total);
	printk("%d free pages\n",free);
	printk("%d reserved pages\n",reserved);
	printk("%d pages shared\n",shared);
	show_buffers();
#ifdef CONFIG_INET
	show_net_buffers();
#endif
}

extern unsigned long free_area_init(unsigned long, unsigned long);

unsigned long paging_init(unsigned long start_mem, unsigned long end_mem)
{
	return free_area_init(start_mem, end_mem);
}

void mem_init(unsigned long start_mem, unsigned long end_mem)
{
	kern_return_t		kr;
	struct vm_statistics	vm_stats;
	struct task_basic_info 	task_basic_info;
	mach_msg_type_number_t	count;
	int			avail_pages, total_pages;
	int			code_size, reserved_size, data_size;
	unsigned long		tmp;
	extern int		_etext, _stext;

	start_mem = PAGE_ALIGN(start_mem);
	kr = vm_deallocate(mach_task_self(), (vm_offset_t) start_mem, 
			   end_mem - start_mem);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("mem_init: vm_deallocate"));
	}
	end_mem = start_mem;

	count = HOST_VM_INFO_COUNT;
	kr = host_statistics(privileged_host_port, HOST_VM_INFO,
			     (host_info_t) &vm_stats, &count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("mem_init: host_statistics"));
	}

	count = TASK_BASIC_INFO_COUNT;
	kr = task_info(mach_task_self(),
		       TASK_BASIC_INFO,
		       (task_info_t) &task_basic_info,
		       &count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(2, kr, ("mem_init: task_info"));
	}

	for (tmp = PAGE_OFFSET;
	     tmp < high_memory;
	     tmp += PAGE_SIZE) {
		clear_bit(PG_reserved, &mem_map[MAP_NR(tmp)].flags);
		mem_map[MAP_NR(tmp)].count = 1;
		free_page(tmp);
	}

	avail_pages = (vm_stats.free_count + vm_stats.active_count +
		       vm_stats.inactive_count);
	total_pages = avail_pages + vm_stats.wire_count;
	code_size = ((long) &_etext - (long) &_stext);
	reserved_size = end_mem - initial_start_mem;
	data_size = task_basic_info.virtual_size - code_size - reserved_size;
	
	printk("Memory: %luk/%luk available (%dk kernel code, %dk reserved, %dk data)\n",
	       avail_pages * (PAGE_SIZE / 1024),
	       total_pages * (PAGE_SIZE / 1024),
	       code_size / 1024,
	       reserved_size / 1024,
	       data_size / 1024);
	       
}

void si_meminfo(struct sysinfo *val)
{
#if 0
	int i;

	i = MAP_NR(high_memory);
	val->totalram = 0;
	val->sharedram = 0;
	val->freeram = nr_free_pages << PAGE_SHIFT;
	val->bufferram = buffermem;
	while (i-- > 0)  {
		if (PageReserved(mem_map+i))
			continue;
		val->totalram++;
		if (!mem_map[i].count)
			continue;
		val->sharedram += mem_map[i].count-1;
	}
	val->totalram <<= PAGE_SHIFT;
	val->sharedram <<= PAGE_SHIFT;
	return;
#else
	extern struct vm_statistics osfmach3_vm_stats;

	osfmach3_update_vm_info();
	val->totalram = osfmach3_mem_size;
	val->freeram = osfmach3_vm_stats.free_count << PAGE_SHIFT;
	val->bufferram = buffermem;
	val->sharedram = 0;
#endif
}
