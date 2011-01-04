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

#include <debug.h>
#include <mach_kgdb.h>
#include <serial_console_default.h>
#include <lan.h>

#include <kern/misc_protos.h>
#include <kgdb/kgdb_defs.h>


#include <ppc/boot.h>
#include <ppc/proc_reg.h>
#include <ppc/misc_protos.h>
#include <ppc/kgdb_defs.h>
#include <ppc/pmap.h>
#include <ppc/new_screen.h>
#include <ppc/exception.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/serial_io.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_gestalt.h>

static int s=0x12345678;

extern void parse_args(boot_args *args);

extern unsigned char per_proc_area[];	/* Declared in ppc_init.c */

extern unsigned int intstack_top_ss;	/* declared in start.s */
#if	MACH_KGDB
extern unsigned int gdbstackptr;	/* declared in start.s */
extern unsigned int gdbstack_top_ss;	/* declared in start.s */
#endif	/* MACH_KGDB */

#if DEBUG

boot_args* boot_args_debug;

extern void test_varargs(int count,...);
extern int pmap_test_bitfields(void);

void test_varargs(int count,...)
{
	int i,j;
	va_list listp;

	i = (int)&count;
	va_start(listp, count);

	for (i=0; i< count; i++) {
		cnputc(48+i);
		cnputc(':');
		j = va_arg(listp, int);
		cnputc(48+j);
		cnputc(' ');
		if (i+1 != j) {
			printf("ERROR IN VARARGS!\n");
			while (1)
				;
		}
	}
	return;
}

#endif /* DEBUG */

extern int exception_exit;
extern const char version[];

extern unsigned long _ExceptionVectorsStart;
extern unsigned long _ExceptionVectorsEnd;


unsigned int kernel_seg_regs[] = {
  KERNEL_SEG_REG0_VALUE,	/* 0 */
  KERNEL_SEG_REG0_VALUE + 1,	/* 1 */
  KERNEL_SEG_REG0_VALUE + 2,	/* 2 */
  SEG_REG_INVALID, /* 3 */
  SEG_REG_INVALID, /* 4 */
  KERNEL_SEG_REG5_VALUE, /* 5 - I/O segment */
  SEG_REG_INVALID, /* 6 */
  SEG_REG_INVALID, /* 7 */
  KERNEL_SEG_REG8_VALUE, /* 8-F are possible IO space */
  KERNEL_SEG_REG9_VALUE,
  KERNEL_SEG_REG10_VALUE,
  KERNEL_SEG_REG11_VALUE,
  KERNEL_SEG_REG12_VALUE,
  KERNEL_SEG_REG13_VALUE,
  KERNEL_SEG_REG14_VALUE, /* 14 - A/V video */
  KERNEL_SEG_REG15_VALUE /* 15 - NuBus etc */
};

void go(unsigned int mem_size, boot_args *args)
{
	int i;
	unsigned long *src,*dst;
	char *str;
	unsigned long	addr;

	/*
	 * Setup per_proc info for first cpu.
	 */
	 
	/* First align the per_proc_info to a cache line */
	per_proc_info = (struct per_proc_info *)(((unsigned int)
						  (&per_proc_area[0])+31)
						 &(-32));

	per_proc_info[0].cpu_number = 0;
	per_proc_info[0].istackptr = 0;	/* we're on the interrupt stack */
	per_proc_info[0].intstack_top_ss = intstack_top_ss;
#if	MACH_KGDB
	per_proc_info[0].gdbstackptr = gdbstackptr;
	per_proc_info[0].gdbstack_top_ss = gdbstack_top_ss;
#endif	/* MACH_KGDB */
	per_proc_info[0].virt_per_proc_info = (unsigned int)
		&per_proc_info[0];
	per_proc_info[0].active_kloaded = (unsigned int)
		&active_kloaded[0];
	per_proc_info[0].cpu_data = (unsigned int)
		&cpu_data[0];
	per_proc_info[0].active_stacks = (unsigned int)
		&active_stacks[0];
	per_proc_info[0].need_ast = (unsigned int)
		&need_ast[0];
	per_proc_info[0].fpu_pcb = 0;

	/* r2 points to our per_proc structure whilst we're in the kernel */
	sync(); __asm__ ("mr 2, %0" : : "r" (&per_proc_info[0])); sync();

	/* Do this VERY early since it might contain important arguments
	 * which may even concern machine issues (bus speed, video etc)
	 */
	parse_args(args);


#if 0
	{
	volatile char *vptr=(volatile char*)0xf3012000;
	/* Write something out of both serial ports to say that we are alive */
	*(vptr+4) = 0x40;
	eieio();
	*(vptr+6) = 0x40;
	eieio();
	}
#endif /* 0 */
	/* Give ourselves the virtual map that we would like */
	if (PROCESSOR_VERSION == PROCESSOR_VERSION_601) {
		/* Set up segment registers for PDM and PCI machines */
		for (i=0; i<=15; i++) {
			isync();
			mtsrin(kernel_seg_regs[i], i * 0x10000000);
			sync();
		}
	} else {
		bat_t		      bat;

		/* Make sure that the BATs map what we expect. Note
		 * that we assume BAT0 maps kernel text, BAT1 the data
		 */

		/* 
		 * BIG ASSUMPTION..
		 *
		 * If a device tree was passed in, then its a 
	 	 * OpenFirmware class machine and typically the I/O
		 * space is up in the 0xf0000000 region.
		 *
		 * Else, its a pre-OFW machines and its I/O space
		 * lives at 0x50000000. (More like 0x50f00000).
		 */

		if (args->deviceTreeSize) 
			addr = 0xf0000000;
		else
			addr = 0x50000000;


		/* Use DBAT2 to map segment F 1-1 */
		bat.upper.word	     = addr;
		bat.upper.bits.bl    = 0x7ff;	/* size = 256M */
		bat.upper.bits.vs    = 1;
		bat.upper.bits.vp    = 0;

		bat.lower.word       = addr;
		bat.lower.bits.wimg  = PTE_WIMG_IO;
		bat.lower.bits.pp    = 2;	/* read/write access */

		sync();isync();
		mtdbatu(2, BAT_INVALID);	/* invalidate old mapping */
		mtdbatl(2, bat.lower.word);
		mtdbatu(2, bat.upper.word);	/* update with new mapping */

		/* Use DBAT3 to map the video segment */
		if ((args->Video.v_baseAddr & 0xf0000000) != addr) {
			/* start off specifying 1-1 mapping of video seg */
			bat.upper.word	     =
				args->Video.v_baseAddr & 0xf0000000;
			bat.lower.word	     = bat.upper.word;

			bat.upper.bits.bl    = 0x7ff;	/* size = 256M */
			bat.upper.bits.vs    = 1;
			bat.upper.bits.vp    = 0;

			bat.lower.bits.wimg  = PTE_WIMG_IO;
			bat.lower.bits.pp    = 2;	/* read/write access */
			
			sync();isync();
			mtdbatu(3, BAT_INVALID); /* invalidate old mapping */
			mtdbatl(3, bat.lower.word);
			mtdbatu(3, bat.upper.word);

			sync();isync();
		}
		/* Set up segment registers as VM through space 0 */
		for (i=0; i<=15; i++) {
			isync();
			mtsrin(KERNEL_SEG_REG0_VALUE | i, i * 0x10000000);
			sync();
		}

	}

	/*
	 * Setup some processor related structures to satisfy funnels.
	 * Must be done before using unparallelized device drivers.
	 */
	processor_ptr[0] = &processor_array[0];
	master_cpu = 0;
	master_processor = cpu_to_processor(master_cpu);

	/* 
	 * Setup the OpenFirmware Device Tree routines
	 * so the console can be found and the right I/O space 
	 * can be used..
	 */

	ofw_init(args);

	/* Setup machine.. */
	if (args->Version <= KBOOTARGS_VERSION_PDM_ONLY) {
		/* machine type wasn't passed in older boot info,
		 * we give a random PDM machine class as the particular
		 * PDM machine type or speed isn't needed.
		 */
		powermac_define_machine(gestaltPowerMac8100_80);
	} else {
		powermac_define_machine(args->machineType);
	}

#if !SERIAL_CONSOLE_DEFAULT
	initialize_screen((void *) &args->Video);
#endif /* !SERIAL_CONSOLE_DEFAULT */
	initialize_serial();

#if DEBUG
	cnputc('E');
	cnputc('L');
	cnputc('F');
	printf("video base address = 0x%08x\n",args->Video.v_baseAddr);
	printf("machine gestalt type = %d\n", args->machineType);
	test_varargs(10,1,2,3,4,5,6,7,8,9,10);

	printf("calling bitfield test from pmap\n");
	if (pmap_test_bitfields()) {
		printf("BITFIELD TEST FAILED\n");
		while (1)
			;
	}
#if	MACH_KGDB
	kgdb_debug=0;
#endif	/* MACH_KGDB */
#endif /* DEBUG */
#if !SERIAL_CONSOLE_DEFAULT
	for (i=0; i<strlen(version);i++)
		screen_put_char(version[i]);
#endif /* !SERIAL_CONSOLE_DEFAULT */

	printf("\n");	/* version string doesn't end in newline */

	/* Copy the exception vectors to their rightful place at 0
	 * and make sure icache is clean
	 */
	
#if DEBUG
	printf("Copying exception vectors from 0x%08x end 0x%08x to zero\n",
	       &_ExceptionVectorsStart,&_ExceptionVectorsEnd);
#endif
	src = &_ExceptionVectorsStart;
	dst = (unsigned long *) 0;

	while (src < &_ExceptionVectorsEnd) {
		*(dst++) = *(src++);
	}
	sync_cache(0, (&_ExceptionVectorsEnd - &_ExceptionVectorsStart)*sizeof(long));

#if DEBUG 
	printf("Done\n");
#endif
	/* ARGH! printf must have bss zeroed before it'll work if
	 * assertions are enabled, since it uses a lock.
	 *
	 * The elf-loader has zeroed bss
	 */
#if DEBUG
	printf("\n\n\nThis program was compiled using gcc %d.%d for powerpc\n",
	       __GNUC__,__GNUC_MINOR__);
	printf("Initialised static variable test : s=0x%x\n\n",s);
	if (s != 0x12345678) {
		printf("INITIALISED STATIC TEST FAILED\n");
		while (1)
			;
	}

	printf("mem_size = %d M\n",mem_size / (1024 * 1024));
	boot_args_debug = args;		/* keep pointer to our boot_args */

	/* Processor version information */
	{
		unsigned int pvr;
		__asm__ ("mfpvr %0" : "=r" (pvr));
		printf("processor version register : 0x%08x\n",pvr);
	}

#if DEBUG
	for (i = 0; i < kMaxDRAMBanks; i++) {
		if (args->PhysicalDRAM[i].size) 
			printf("DRAM at 0x%08x size 0x%08x\n",
			       args->PhysicalDRAM[i].base,
			       args->PhysicalDRAM[i].size);
	}
#endif /* DEBUG */
	
	for (i = 0; i < args->kern_info.region_count; i++) {
		DPRINTF(("kernel addr : 0x%08x	   size : 0x%08x    phys_addr : 0x%08x\n",
			 args->kern_info.regions[i].addr,
			 args->kern_info.regions[i].size,
			 args->kern_info.base_addr + 
			 args->kern_info.regions[i].offset));
	}
	DPRINTF(("entry : 0x%08x\n",args->kern_info.entry));
#endif /* DEBUG */

	if ((powermac_info.class != POWERMAC_CLASS_PDM && powermac_info.class != POWERMAC_CLASS_POWERBOOK) &&
	    ((args->Version < kBootHaveOFWVersion) ||
	     (args->deviceTreeSize == 0))) {
		printf("Sorry! You need to upgrade your MkLinux Booter "
		       "to boot this OSF MK kernel.\n"
		       "(It can be found in your MacOS's "
		       "System Folder:Extensions folder)\n");
	}

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
		str = "NuBus";
		break;

	case	POWERMAC_CLASS_PCI:
		str = "PCI";
		break;

	case	POWERMAC_CLASS_PERFORMA:
		str = "Performa (unsupported)";
		break;

	case	POWERMAC_CLASS_POWERBOOK:
		str = "PowerBook (unsupported)";
		break;
	default:
		str = "unsupported";
		break;
	}
	printf("MACH microkernel is booting on a "
	       "Power Macintosh %s class machine\n", str );
}
