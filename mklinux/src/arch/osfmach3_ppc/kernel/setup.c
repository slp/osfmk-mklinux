/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  linux/arch/osfmach_ppc/kernel/setup.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Adapted from 'alpha' version by Gary Thomas
 */

/*
 * bootup setup stuff..
 */

#include <linux/autoconf.h>

#include <mach/machine.h>
#include <mach/mach_interface.h>
#include <mach/vm_attributes.h>

#include <osfmach3/assert.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/a.out.h>
#include <linux/tty.h>

#define SIO_CONFIG_RA	0x398
#define SIO_CONFIG_RD	0x399

#include <asm/pgtable.h>
#include <asm/processor.h>

#ifndef	CONFIG_PMAC_CONSOLE
/*
 * The format of "screen_info" is strange, and due to early
 * i386-setup code. This is just enough to make the console
 * code think we're on a EGA+ colour display.
 */
struct screen_info screen_info = {
	0, 0,			/* orig-x, orig-y */
	{ 0, 0 },		/* unused */
	0,			/* orig-video-page */
	0,			/* orig-video-mode */
	80,			/* orig-video-cols */
	0,			/* unused [short] */
	0,			/* ega_bx */
	0,			/* unused [short] */
	25,			/* orig-video-lines */
	0,			/* isVGA */
	16			/* video points */
};
#endif	/* CONFIG_PMAC_CONSOLE */

asmlinkage int sys_ioperm(unsigned long from, unsigned long num, int on)
{
	if (from == 0 && num == 0) {
		/*
		 * This is a Mach-aware application telling us that it's going
		 * to use Mach services directly.
		 */
		if (!suser())
			return -EPERM;
		osfmach3_trap_mach_aware(current, (on != 0),
					 EXCEPTION_STATE, PPC_THREAD_STATE);
		return 0;
	}
	return -EIO;
}

extern int loops_per_sec;  /* For BogoMips timing */
extern int host_cpus;      /* Number of CPUs in the system */

int get_cpuinfo(char *buffer)
{
        unsigned long len = 0;
        unsigned long bogosum = 0;
        unsigned long i;

        for ( i = 0; i < host_cpus ; i++ )
        {
		len += sprintf(len+buffer,"processor\t: %lu\n",i);
		len += sprintf(len+buffer,"cpu\t\t: ");

		switch (cpu_subtype >> 16) {
		case 1:
			len += sprintf(len+buffer, "601\n");
			break;
		case 3:
			len += sprintf(len+buffer, "603\n");
			break;
		case 4:
			len += sprintf(len+buffer, "604\n");
			break;
		case 6:
			len += sprintf(len+buffer, "603e\n");
			break;
		case 7:
			len += sprintf(len+buffer, "603ev\n");
			break;
		case 8:
			len += sprintf(len+buffer, "750 (Arthur)\n");
			break;
 		case 9:
			len += sprintf(len+buffer, "604e\n");
			break;
		case 10:
			len += sprintf(len+buffer, "604ev5 (MachV)\n");
			break;
		default:
			len += sprintf(len+buffer, "unknown (%d)\n",
                                       cpu_subtype>>16);
                        break;
                }

		/* Put CPU speed here if there is a way to get it */
		/* len += sprintf(len+buffer, "clock\t\t: %ldMHz\n",
				"Unknown"); */

		len += sprintf(len+buffer, "revision\t: %d.%d\n",
				(cpu_subtype & 0xff00) >> 8, 
				 cpu_subtype & 0xff);
	/* In MkLinux is the bogomips listest per processor?  Or total? */
	/* We assume each processor has the same bogomips.. */

		len += sprintf(buffer+len, "bogomips\t: %lu.%02lu\n",
		       		(unsigned long)(loops_per_sec+2500)/500000,
		       		(unsigned long)((loops_per_sec+2500)/5000) % 100);
		bogosum += loops_per_sec;
	}

	/* If we're in an SMP environment */
	if ( host_cpus > 1 )
	        len += sprintf(buffer+len,"total bogomips\t: %lu.%02lu\n",
		  (unsigned long)((loops_per_sec+2500)/500000 * host_cpus),
		  (unsigned long)(((loops_per_sec+2500)/5000) * host_cpus) % 100);

	len += sprintf(buffer+len,"machine\t\t: PowerMac\n");

	return len;
}

void
flush_instruction_cache_range(
	struct mm_struct	*mm,
	unsigned long		start,
	unsigned long		end)
{
	/*
	 * If we have a split I/D cache, we need to maintain cache
	 * coherency after writing some instructions.
	 */
	if ((cpu_subtype >> 16) != CPU_SUBTYPE_PPC601) {
		if (mm == &init_mm) {
			unsigned long addr;

			/* no need to call to Mach to flush our data */
			for (addr = start;
			     addr < ((end+(32-1)) & ~(32-1));
			     addr += 32) {
				__asm__ __volatile__ ("dcbf 0,%0;"
						      "sync;"
						      "icbi 0,%0;"
						      "sync;"
						      "isync"
						      :	/* no output */
						      : "r" (addr));
			}
		} else {
			vm_machine_attribute_val_t	val;
			mach_port_t			mach_task;
			vm_address_t			mach_start;
			vm_size_t			mach_size;
			kern_return_t			kr;

			mach_task = mm->mm_mach_task->mach_task_port;
			mach_start = (vm_address_t) start;
			mach_size = (vm_size_t) (end - start);
			val = MATTR_VAL_CACHE_FLUSH;
			kr = vm_machine_attribute(mach_task,
						  mach_start, mach_size,
						  MATTR_CACHE, &val);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("flush_instruction_cache_range"
					     "(%p,0x%lx,0x%lx): "
					     "vm_machine_attribute"
					     "(0x%x,0x%x,0x%x)",
					     mm, start, end,
					     mach_task, mach_start, mach_size));
			}
		}
	}
}
