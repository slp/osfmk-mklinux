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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

/*
 * MkLinux
 */
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: model_dep.c 1.34 94/12/14$
 */

#include <mach_kgdb.h>
#include <debug.h>
#include <mach_kdb.h>
#include <db_machine_commands.h>
#include <cpus.h>

#include <kern/thread.h>
#include <machine/pmap.h>
#include <machine/mach_param.h>
#include <device/param.h>
#include <device/ds_routines.h>
#include <device/subrs.h>
#include <device/device_typedefs.h>
#include <device/conf.h>
#include <device/misc_protos.h>

#include <mach/vm_param.h>
#include <mach/clock_types.h>
#include <mach/machine.h>
#include <machine/spl.h>
#include <ppc/boot.h>

#include <kern/misc_protos.h>
#include <kern/startup.h>
#include <ppc/misc_protos.h>
#include <ppc/proc_reg.h>
#include <ppc/thread.h>
#include <ppc/asm.h>
#include <ppc/mem.h>
#include <ppc/POWERMAC/pci.h>
#if NCPUS > 1 
#include <ppc/FirmwareCalls.h>
#include <ppc/POWERMAC/mp/mp.h>
#endif /* NCPUS > 1 */

#include <kern/clock.h>
#include <machine/trap.h>
#include <kern/spl.h>
#include <ppc/POWERMAC/serial_io.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_gestalt.h>
#include <ppc/POWERMAC/device_tree.h>

#if MACH_KGDB
#include <kgdb/kgdb_defs.h>
#endif

#if	MACH_KDB
#include <ddb/db_aout.h>
#include <ddb/db_output.h>
#include <ddb/db_command.h>
#include <machine/db_machdep.h>

extern struct db_command ppc_db_commands[];
#endif	/* MACH_KDB */

void parse_args(boot_args *args);

char kernel_args_buf[256] = "/mach_kernel";
char boot_args_buf[256] = "/mach_servers/bootstrap";
char env_buf[256];

/* These should all be exported from kern/bootstrap.c */
extern vm_offset_t	kern_args_start;
extern vm_size_t	kern_args_size;

extern vm_offset_t	boot_args_start;
extern vm_size_t	boot_args_size;		

extern vm_offset_t	env_start;
extern vm_offset_t	env_size;

int	boottype = 0;	/* bootstrap needs this defined! */


int 	halt_in_debugger = 0;

extern const char version[];
/*
 *	Cpu initialization.  Running virtual, but without MACH VM
 *	set up (1-1 BAT mappings).  First C routine called.
 */
void
machine_startup(unsigned int memory_size, boot_args *args)
{

	/*
	 * Global mem_size initialisation, may be overridden by
	 * command line arguments
	 */
	if (mem_size == 0)
		mem_size = memory_size;

	/* meb 11/2/95 - temporary hack for serial */
	cons_find(0);

	/*
	 * VM initialization, after this we're using page tables...
	 */
	ppc_vm_init(mem_size, args);

#if	MACH_KDB

	/*
	 * Initialize the kernel debugger.
	 */
#if	DB_MACHINE_COMMANDS
	db_machine_commands_install(ppc_db_commands);
#endif	/* DB_MACHINE_COMMANDS */
	ddb_init();

#endif	/* MACH_KDB */

#if	MACH_KDB || MACH_KGDB
	/*
	 * Cause a breakpoint trap to the debugger before proceeding
	 * any further if the proper option bit was specified in
	 * the boot flags.
	 *
	 * XXX use -a switch to invoke kdb, since there's no
	 *     boot-program switch to turn on RB_HALT!
	 */
	if (halt_in_debugger) {
		printf("inline call to debugger(machine_startup)\n");
	        Debugger("");
	}
#endif /* MACH_KDB || MACH_KGDB */

	machine_conf();

	/*
	 * Start the system.
	 */
	setup_main();

	/* Should never return */
}

/*
 * Return boot information in buf.
 */
#include <device/disk_status.h>
#include <scsi/rz.h>

char *
machine_boot_info(char *buf, vm_size_t size)
{
	dev_ops_t disk_ops;
	int unit, len;

	strcpy(buf, "boot_device");

	if (dev_name_lookup(buf, &disk_ops, &unit)) {
	    char *p;

	   /*
 	    * The controller number is currently ignored,
	    * to logically "merge" the two busses into
	    * one for the POWERMAC.
	    */

#ifdef	SCSI_SEQUENTIAL_DISKS
	   /* Attempt to pass in the indirect name... */
	   if (!dev_find_indirect(disk_ops, (unit & ~(RZPARTITION_MASK)), buf)) 
#endif
	   {
	    	strcpy(buf, disk_ops->d_name);
	    	itoa(rzslave(unit), buf + strlen(buf));
	    }
		
	    p = buf + strlen(buf);
	    *p++ = rzpartition(unit) + 'a';
	    *p = 0;
	} 

	if (kern_args_size > 0) {
		char *dst = buf + strlen(buf);
		char *src = (char*)kern_args_start;
		while (src < (char*) (kern_args_start + kern_args_size)) {
			int len;
			*dst++ = ' ';
			len = strlen(src);
			memcpy(dst, src, len+1);
			dst += len;
			src += len+1;
		}
	}
	return buf;
}

extern vm_offset_t env_start;
extern vm_size_t env_size;

const char * getenv(const char *name);

const char * getenv(const char *name)
{
	int len = strlen(name);
	const char *p = (const char *)env_start;
	const char *endp = p + env_size;

	while (p < endp) {
		if (len >= endp - p)
			break;
		if (strncmp(name, p, len) == 0 && *(p + len) == '=')
			return p + len + 1;
		while (*p++)
			;
	}
	return NULL;
}

void
machine_conf(void)
{
	/* XXX - we assume 1 cpu */
	machine_slot[0].is_cpu = TRUE;
	machine_slot[0].running = TRUE;

	/*
	 * We return the processor info, nothing on the machine
	 */

	machine_slot[0].cpu_type = CPU_TYPE_POWERPC;
	machine_slot[0].cpu_subtype = mfpvr();

	machine_info.max_cpus = NCPUS;
	machine_info.avail_cpus = 1;
	machine_info.memory_size = mem_size;
}

void
machine_init(void)
{
	dev_ops_t boot_dev_ops;
	int unit;
	char bootdev_name[10];
	const char *p;
	int n;

	autoconf();

	if (p = getenv("rootdev")) {
		/*
		 * Translate from a Linux Style to
		 * a MACH style one..
		 * Linux           MACH
		 * -----------------------------
 		 * Unit A-Z  -->   0 - NN
		 * Part 1-9  -->   A - Z
		 */

		/* Skip over the /dev/ prefix if present */

		if (strncmp(p, "/dev/", 5) == 0)
			p += 5;

		if (strncmp(p, "cdrom", 5) == 0) {
			strcpy(bootdev_name, p);
			bootdev_name[5] = p[5] ? p[5] : '0';
		} else if (strncmp(p, "scd", 3) == 0) {
			strcpy(bootdev_name, "cdrom");
			bootdev_name[5] = p[3];
			bootdev_name[6] = 0;
		} else {
			bootdev_name[0] = p[0];
			bootdev_name[1] = p[1];
			bootdev_name[2] = (p[2] - 'a') + '0';
			bootdev_name[3] = (atoi((u_char *)&p[3]) +
				   	('a'-1));	/* from 1-N to a-z*/
			bootdev_name[4] = '\0';
		}

		if (dev_name_lookup(bootdev_name, &boot_dev_ops, &unit)) 
			dev_set_indirection("boot_device", boot_dev_ops, unit);
		else
			printf("Warning: unable to set boot_device '%s'\n",
			       bootdev_name);
	} else {
		/*
	 	* Set the boot device to sd7a or whatever
	 	*/
		if (p = getenv("BOOTDEV"))
			strcpy(bootdev_name, p);

		if (p = getenv("BOOTUNIT")) 
			strcpy(bootdev_name + strlen(bootdev_name), p);

		if (dev_name_lookup(bootdev_name, &boot_dev_ops, &unit)) {
	
			if (p = getenv("BOOTPART")) {
				n = 0;
				while (*p >= '0' && *p <= '9')
					n = (n * 10) + (*p++ - '0');
			} else
				n = ((boottype >> 8) & 0xff);

			dev_set_indirection("boot_device", boot_dev_ops, unit+n);
		} else
			printf("Warning: unable to set boot_device '%s'\n", bootdev_name);
	}
	
	clock_config();
}

/* Routine to pre-initialise anything in IO space once powermac_io_base_v
 * has been initialised to new value and DMA space has been allocated
 */
void powermac_io_init(vm_offset_t powermac_io_base_v)
{
	if (powermac_info.class == POWERMAC_CLASS_PDM) {
		/* Set the DMA controller up to point at our DMA memory */
		struct pdm_dma_base *pdm_dma_base =
			(struct pdm_dma_base *)
				POWERMAC_IO(PDM_DMA_CTRL_BASE_PHYS);

		pdm_dma_base->addr_1 =
			(powermac_info.dma_buffer_virt >> 24) & 0xff;
		pdm_dma_base->addr_2 =
			(powermac_info.dma_buffer_virt >> 16) & 0xff;
		pdm_dma_base->addr_3 =
			(powermac_info.dma_buffer_virt >>  8) & 0xff;
		pdm_dma_base->addr_4 =
			(powermac_info.dma_buffer_virt      ) & 0xff;

		eieio();
	}
}

#include <sys/ioctl.h>
#include <device/device_types.h>
#include <device/disk_status.h>

#ifdef PDCDUMP
void
dumpconf(void)
{
	struct partinfo pi;
	
	/*
	 * The device should already be opened, and the dump info setup,
	 * but savecore needs a bit more.
	 */
	(*dumpdev_ops->d_setstat)(dumpdev, DIOCGPART, (int *)&pi, sizeof(pi));
	dumplo = pi.part->p_size - ctod(dumpsize);

	printf("dumpconf: size = %d blocks, *disk* offset = %d blocks\n",
	       ctod(dumpsize), dumpoffset);

	if (dumpsize != btoc(mem_size))
		printf("\tWARNING: Dump partition is smaller than mem size\n");
}
#endif

#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <mach/time_value.h>
#include <machine/machparam.h>	/* for btop */

/*
 * Universal (Posix) time map. This should go away when the
 * kern/posixtime.c routine is removed from the kernel.
 */
vm_offset_t
utime_map(
	dev_t			dev,
	vm_offset_t		off,
	int			prot)
{
	extern time_value_t *mtime;

	if (prot & VM_PROT_WRITE)
		return (-1);

	return (btop(pmap_extract(pmap_kernel(), (vm_offset_t) mtime)));
}

int dofastboot = 0;		/* if set, always do a fastboot() */
int dotocdump = 1;		/* if set, do a crash dump on TOC's */

void
halt_all_cpus(boolean_t	reboot)
{
	if(reboot)
	{
		printf("MACH Reboot\n");
		powermac_reboot();
	}
	else
	{
		printf("CPU halted\n");
		powermac_powerdown();
	} 
	while(1);
}

void
halt_cpu(void)
{
        static int paniced = 0;
	Debugger("halt cpu");
        halt_all_cpus(0);
}

void parse_args(boot_args *args)
{
	char	*src, *dest;
	char	ch;

	/*
	 * Okay, first split the command line into
	 * arguments and environments... assume that
	 * environment starts with first arg that
	 * doesn't have a '-'.
	 */

	src = args->CommandLine;

	/* APPEND arguments to kernel_args_buf */
	dest = kernel_args_buf + strlen(kernel_args_buf)+1;

	while (*src == '-') {
		/* Copy argument ... */
		while (*src && *src != ' ') {
			*dest++ = *src++;
		}
		/* replace white space with NULL */
		*dest++ = '\0';
		while (*src == ' ')
			src++;
	}
	*(dest-1) = '\0';

	/* kern_args_start and kern_args_size refer to kernel args */
	kern_args_start = (vm_offset_t)&kernel_args_buf[0];
	kern_args_size  = dest - kernel_args_buf;

	/* Now copy environment */
	dest = env_buf;
	while (*src) {
		while (*src != ' ' && *src) 
			*dest++ = *src++;
		*dest++ = '\0';
		while (*src == ' ')
			src++;
	}
	*dest = '\0';
	env_start = (vm_offset_t) env_buf;
	env_size = dest - env_buf;

	/* Now scan through our kernel arguments and
	 * act on them, plus look for bootstrap args and
	 * append them to the bootstrap arguments...
	 */
	src  = kernel_args_buf;

	boot_args_size = strlen(boot_args_buf)+1;
	boot_args_start = (vm_offset_t) boot_args_buf;

	while (src < (kernel_args_buf + kern_args_size)) {
		/* skip over any arguments not starting with '-' */
		if (*src++ != '-') {
			/* Eat up this argument and move to next,
			 * make sure to step beyond the NULL
			 */
			while (*src++ != '\0')
				;
			continue;
		}
		while (ch = *src++) {
			switch (ch) {
			case 'a':	/* -a , ask bootstrap to prompt... */
				strcpy(&boot_args_buf[boot_args_size], "-a");
				boot_args_size += 3;
				break;
			case 'h':
				halt_in_debugger = 1;
				break;
			case 'm':	/* -m??:  memory size Mbytes*/
				mem_size = atoi_term(src, &src)*1024*1024;
				break;
			case 'r':
				switch_to_serial_console();
				break;
			default:
#if	NCPUS > 1
				if (ch > '0' && ch <= '9')
					wncpu = ch - '0';
#endif	/* NCPUS > 1 */
				break;
			}
		}
	}
}

#if	MACH_ASSERT

/*
 * Machine-dependent routine to fill in an array with up to callstack_max
 * levels of return pc information.
 */
void machine_callstack(
	natural_t	*buf,
	vm_size_t	callstack_max)
{
}

#endif	/* MACH_ASSERT */

void
Debugger(
	const char	*message)
{
#if	NCONSFEED >= 1
	console_feed_cancel_and_flush();
#endif	/* NCONSFEED >= 1 */

#if	MACH_KGDB
	kgdb_printf("Debugger(%s)\n", message);
	kgdb_break();
#else	/* MACH_KGDB */
#if	MACH_KDB
	if (db_active
#if	NCPUS > 1
	    && kdb_active[cpu_number()]
#endif	/* NCPUS > 1 */
	    ) {
		/* we're already in kdb: don't enter it again... */
		return;
	}
	if (db_is_alive()) {
		db_printf("Debugger(%s)\n", message);
		__asm__("tw 4,1,1");
		return;
	} else
#endif	/* MACH_KDB */
	{
		unsigned int* stackptr;
		int i;

#if	!MACH_KDB
		printf("\nNo debugger configured - "
		       "dumping debug information\n");
		printf("\nversion string : %s\n",version);
#endif	/* !MACH_KDB */
		printf("backtrace: ");

		/* Get our stackpointer for backtrace */

		/* TODO NMGS wrap this neatly in a function */

		__asm__ volatile("mr %0,	1" : "=r" (stackptr));

		for (i = 0; i < 32; i++) {

			/* Avoid causing page fault */
#if	MACH_KDB || MACH_KGDB
			if (db_find_or_allocate_pte(kernel_pmap->space,
						    trunc_page((vm_offset_t)
							       stackptr),
						    FALSE) == NULL)
				break;
#else	/* MACH_KDB || MACH_KGDB */
			if (pmap_extract(kernel_pmap, (vm_offset_t)stackptr)
			    == 0)
				break;
#endif	/* MACH_KDB || MACH_KGDB */

			stackptr = (unsigned int*)*stackptr;
#if	MACH_KDB || MACH_KGDB
			if (db_find_or_allocate_pte(kernel_pmap->space,
						    trunc_page((vm_offset_t)
							       stackptr+
							       (FM_LR_SAVE/
								sizeof(int))),
						    FALSE) == NULL)
				break;
#else	/* MACH_KDB || MACH_KGDB */
			if (pmap_extract(kernel_pmap,
					 (vm_offset_t)stackptr+
					 (FM_LR_SAVE/sizeof(int))) == 0)
				break;
#endif	/* MACH_KDB || MACH_KGDB */
			printf("%08x ",*(stackptr + (FM_LR_SAVE/sizeof(int))));
		}

#if	!MACH_KDB
		printf("\n\nPlease report the above messages to\n");
		printf("bugs@mklinux.apple.com and reboot your machine.\n");

		while(1);
#endif	/* !MACH_KDB */
	}
#endif	/* MACH_KGDB */
}

powermac_info_t	powermac_info;

static char *powermac_chips[] = {
	"gc",
	"ohare",
	"mac-io",	/* aka Heathrow */
	NULL
};

void
powermac_define_machine(int machineGestalt)
{
	int		i;
	device_node_t	*chipdev;

	/* Everything starts out zeroed... */
	bzero((void*) &powermac_info, sizeof(powermac_info_t));

	powermac_info.machine = machineGestalt;

	switch (machineGestalt) {
	case gestaltPowerMac6100_60:
	case gestaltPowerMac6100_66:
	case gestaltPowerMac6100_80:
	case gestaltPowerMac7100_66:
	case gestaltPowerMac7100_80_chipped:
	case gestaltPowerMac7100_80:
	case gestaltPowerMac8100_80:
	case gestaltPowerMac8100_100:
	case gestaltPowerMac8100_110:
	case gestaltPowerMac8100_120:
	case gestaltAWS9150_80:
	case gestaltAWS9150_120:
		powermac_info.class   		   = POWERMAC_CLASS_PDM;
		powermac_info.io_base_phys	   = PDM_IO_BASE_ADDR;
		powermac_info.io_size		   = PDM_IO_SIZE;
		powermac_info.dma_buffer_size	   = PDM_DMA_BUFFER_SIZE;
		powermac_info.dma_buffer_alignment = PDM_DMA_BUFFER_ALIGNMENT;
		break;

	case gestaltPowerMac5200:
	case gestaltPowerMac6200:
		powermac_info.class		   = POWERMAC_CLASS_PERFORMA;
		powermac_info.io_base_phys	   = PDM_IO_BASE_ADDR;
		powermac_info.io_size		   = PDM_IO_SIZE;
		powermac_info.dma_buffer_size	   = PDM_DMA_BUFFER_SIZE;
		powermac_info.dma_buffer_alignment = PDM_DMA_BUFFER_ALIGNMENT;
		break;

	case gestaltPowerBook5300:
	case gestaltPowerBookDuo2300:
		powermac_info.class		   = POWERMAC_CLASS_POWERBOOK;
		powermac_info.io_base_phys	   = PDM_IO_BASE_ADDR;
		powermac_info.io_size		   = PDM_IO_SIZE;
		powermac_info.dma_buffer_size	   = PDM_DMA_BUFFER_SIZE;
		powermac_info.dma_buffer_alignment = PDM_DMA_BUFFER_ALIGNMENT;
		break;

	case gestaltPowerMac7200:
	case gestaltPowerMac7500:
	case gestaltPowerMac8500:
	case gestaltPowerMac9500:
	default:
		powermac_info.class		   = POWERMAC_CLASS_PCI;
		powermac_info.io_base_phys	   = PCI_IO_BASE_ADDR;
		powermac_info.io_size		   = PCI_IO_SIZE;

		for (i = 0; powermac_chips[i]  ; i ++)  {
			if ((chipdev = find_devices(powermac_chips[i])) == NULL)
				continue;

			powermac_info.io_base_phys = chipdev->addrs[0].address;
			powermac_info.io_size = chipdev->addrs[0].size;
			break;
		}
		break;

	}
	/* Now set up the clock */
	if (PROCESSOR_VERSION == PROCESSOR_VERSION_601) {
		if (powermac_info.class == POWERMAC_CLASS_PDM) {
			/* PDM 601 doesn't quite tick at nsec precision */
			powermac_info.proc_clock_to_nsec_numerator   = 4085;
			powermac_info.proc_clock_to_nsec_denominator = 4096;
		} else {
			powermac_info.proc_clock_to_nsec_numerator   = 1;
			powermac_info.proc_clock_to_nsec_denominator = 1;
		}
	} else {
		const char *var;

		var = getenv("bus_speed_hz");
		if (var != NULL) {
			powermac_info.bus_clock_rate_hz = atoi((u_char*)var);
			/* Sanity check */
			if (powermac_info.bus_clock_rate_hz < 20000000)
				powermac_info.bus_clock_rate_hz	= 
					40000000; /* 40 MHz*/
			powermac_info.proc_clock_to_nsec_numerator = 1000 * 4;
			powermac_info.proc_clock_to_nsec_denominator = 
				powermac_info.bus_clock_rate_hz/1000000;
		} else {
			device_node_t *cpu;
			unsigned long *prop_freq;

			if ((cpu = find_type("cpu")) != NULL) {
				if (prop_freq = (unsigned long *)
				    		     get_property(cpu,
						     "timebase-frequency",
								  NULL)) {
					powermac_info.bus_clock_rate_hz = *prop_freq;
					powermac_info.proc_clock_to_nsec_numerator = 60000;
					powermac_info.proc_clock_to_nsec_denominator = (*prop_freq*60)/1000000;
				} 
			} else {
				powermac_info.bus_clock_rate_hz	= 40000000;

				/* Default to 40 MHz */
				powermac_info.proc_clock_to_nsec_numerator = 1000 * 4;
				powermac_info.proc_clock_to_nsec_denominator = 
					powermac_info.bus_clock_rate_hz/1000000;
			}
		}
	}

	/* IO mapping starts off as 1-1 on all machines */
	powermac_info.io_base_virt = powermac_info.io_base_phys;

	powermac_info.io_coherent  = powermac_is_coherent();
}
