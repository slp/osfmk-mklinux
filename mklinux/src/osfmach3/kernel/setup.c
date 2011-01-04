/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Machine-independent routines from arch/.../kernel/setup.c.
 */

#include <mach/mach_interface.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>

#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/blk.h>

extern char *builtin_ramdisk_image;
extern long builtin_ramdisk_size;

extern int root_mountflags;
kdev_t DEFAULT_ROOT_DEV;
char *server_command_line = "ro";	/* XXX should get that from lilo */
char **server_command_line_p = &server_command_line;
#define COMMAND_LINE_SIZE 256
char saved_command_line[COMMAND_LINE_SIZE];

extern void set_rootdev(void);

unsigned long		initial_start_mem;

void setup_arch(char **cmdline_p,
	unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	kern_return_t	kr;
	unsigned long	initial_mem_size;
	char c, *cp;

	set_rootdev();
	/*
	 * Reserve a bunch of memory for subsystems initialization.
	 * The extra memory will be freed in osfmach3_mem_init after
	 * every subsystem had a chance to reserve some memory.
	 */
	initial_mem_size = 64*1024*1024;	/* 64 MB */
	kr = vm_allocate(mach_task_self(),
			 (vm_offset_t *) &initial_start_mem,
			 initial_mem_size,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("setup_arch: vm_allocate(&start_mem)"));
		panic("setup_arch: can't set memory_start");
	}

	/* Save unparsed command line copy for /proc/cmdline */
	memcpy(saved_command_line, *server_command_line_p, COMMAND_LINE_SIZE);
	saved_command_line[COMMAND_LINE_SIZE-1] = '\0';

	__mem_size = osfmach3_mem_size;
	for (c = ' ', cp = saved_command_line;;) {
		/*
		 * "mem=XXX[kKmM]" overrides the Mach-reported memory size
		 */
		if (c == ' ' && !strncmp(cp, "mem=", 4)) {
			__mem_size = simple_strtoul(cp+4, &cp, 0);
			if (*cp == 'K' || *cp == 'k') {
				__mem_size = __mem_size << 10;
				cp++;
			} else if (*cp == 'M' || *cp == 'm') {
				__mem_size = __mem_size << 20;
				cp++;
			}
		}
		c = *(cp++);
		if (!c)
			break;
	}
	     
	*memory_start_p = initial_start_mem;
	*memory_end_p = initial_start_mem + initial_mem_size;
	*cmdline_p = *server_command_line_p;

#ifdef CONFIG_BLK_DEV_INITRD
	initrd_start = (unsigned long) &builtin_ramdisk_image;
	initrd_end = initrd_start + builtin_ramdisk_size;
	printk ("Initrd at 0x%08lx to 0x%08lx (0x%08lx)\n",
		initrd_start, initrd_end, initrd_end - initrd_start);
#endif	/* CONFIG_BLK_DEV_INITRD */	
}
