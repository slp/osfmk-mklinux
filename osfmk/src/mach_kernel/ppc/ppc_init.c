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

#include <mach_debug.h>
#include <mach_kdb.h>
#include <debug.h>
#include <mach_kgdb.h>
#include <cpus.h>

#include <mach/vm_types.h>
#include <mach/vm_param.h>
#include <mach/thread_status.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>

#include <ppc/proc_reg.h>
#include <ppc/boot.h>
#include <ppc/misc_protos.h>
#include <ppc/pmap.h>
#include <ppc/pmap_internals.h>
#include <ppc/mem.h>
#include <ppc/exception.h>
#include <ppc/gdb_defs.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/POWERMAC/video_pdm.h>
#if NCPUS > 1
#include <ppc/mp.h>
#include <ppc/POWERMAC/mp/mp.h>
#endif /* NCPUS > 1 */

/* External references */

#if	MACH_KDB
extern vm_offset_t	kern_sym_start;
extern vm_size_t	kern_sym_size;
extern const char *getenv(const char *name);
#endif	/* MACH_KDB */
extern unsigned int intstack[];	/* declared in start.s */
extern unsigned int intstack_top_ss;	/* declared in start.s */
#if	MACH_KGDB
extern unsigned int gdbstackptr;	/* declared in start.s */
extern unsigned int gdbstack_top_ss;	/* declared in start.s */
#endif	/* MACH_KGDB */

/* Stuff declared in kern/bootstrap.c which we may need to initialise */

extern vm_offset_t     boot_start;
extern vm_size_t       boot_size;
extern vm_offset_t     boot_region_desc;
extern vm_size_t       boot_region_count;
extern int             boot_thread_state_flavor;
extern thread_state_t  boot_thread_state;
extern unsigned int    boot_thread_state_count;

/* Trap handling function prototypes */

extern void thandler(void);	/* trap handler */
extern void ihandler(void);	/* interrupt handler */
extern void shandler(void);	/* syscall handler */
extern void gdbhandler(void);	/* debugger handler */
extern void fpu_switch(void);	/* fp handler */

/* definitions */

struct ppc_thread_state boot_task_thread_state;

void (*exception_handlers[])(void) = {
	thandler,	/* 0x000   INVALID EXCEPTION */
	thandler,	/* 0x100   System reset */
	thandler,	/* 0x200   Machine check */
	thandler,	/* 0x300   Data access */
	thandler,	/* 0x400   Instruction access */
	ihandler,	/* 0x500   External interrupt */
	thandler,	/* 0x600   Alignment */
	thandler,	/* 0x700   Program - fp exc, ill/priv instr, trap */
	fpu_switch,	/* 0x800   Floating point disabled */
	ihandler,	/* 0x900   Decrementer */
	thandler,	/* 0xA00   I/O controller interface */
	thandler,	/* 0xB00   INVALID EXCEPTION */
	shandler,	/* 0xC00   System call exception */
	thandler,	/* 0xD00   Trace */
	thandler,	/* 0xE00   FP assist */
	thandler,	/* 0xF00   INVALID EXCEPTION */
	thandler,	/* 0x1000  INVALID EXCEPTION */
	thandler,	/* 0x1100  INVALID EXCEPTION */
	thandler,	/* 0x1200  INVALID EXCEPTION */
	thandler,	/* 0x1300  instruction breakpoint */
	thandler,	/* 0x1400  system management */
	thandler,	/* 0x1500  INVALID EXCEPTION */
	thandler,	/* 0x1600  INVALID EXCEPTION */
	thandler,	/* 0x1700  INVALID EXCEPTION */
	thandler,	/* 0x1800  INVALID EXCEPTION */
	thandler,	/* 0x1900  INVALID EXCEPTION */
	thandler,	/* 0x1A00  INVALID EXCEPTION */
	thandler,	/* 0x1B00  INVALID EXCEPTION */
	thandler,	/* 0x1C00  INVALID EXCEPTION */
	thandler,	/* 0x1D00  INVALID EXCEPTION */
	thandler,	/* 0x1E00  INVALID EXCEPTION */
	thandler,	/* 0x1F00  INVALID EXCEPTION */
	thandler,	/* 0x2000  Run Mode/Trace */
	ihandler	/* 0x2100  Signal processor */
};

/* per_proc_info is accessed with VM switched off via sprg0 */
/* Note that we always get enough space for an extra cache line.  That is because
   for performance, we need to align the struct on a cache boundary.  With the
   extra space, we can shift the array up to 31 bytes to align */


/*	 struct per_proc_info per_proc_info[NCPUS];		*/		/* Unaligned here */
unsigned char per_proc_area[NCPUS*sizeof(struct per_proc_info)+32]; 
struct per_proc_info *per_proc_info; 

vm_offset_t mem_size;  /* Size of actual physical memory present in bytes */

mem_region_t pmap_mem_regions[PMAP_MEM_REGION_MAX];
int	 pmap_mem_regions_count = 0;	/* No non-contiguous memory regions */

mem_region_t free_regions[FREE_REGION_MAX];
int	     free_regions_count;

vm_offset_t virtual_avail, virtual_end;

extern unsigned long etext;
extern unsigned long _ExceptionVectorsStart;
extern unsigned long _ExceptionVectorsEnd;

#if 1 /* TODO NMGS - vm_map_steal_memory shouldn't use these - remove */
vm_offset_t avail_start;
vm_offset_t avail_end;
#endif 
unsigned int avail_remaining = 0;
vm_offset_t first_avail;

#if	NCPUS > 1
void initPerProc(void);
vm_offset_t	interrupt_stack[NCPUS];
#endif	/* NCPUS > 1 */

/* This is the first C function to be called. It is called with VM
 * switched ON, with the bottom 2M of memory mapped into KERNELBASE_TEXT
 * via BAT0, and the region in 2-4M mapped 1-1 (KERNELBASE_DATA) via BAT1
 *
 * The IO space is mapped 1-1 either via a segment register or via
 * BAT2, depending upon the processor type.
 *
 * printf already works
 * 
 * First initialisation of memory.
 *  - zero bss,
 *  - invalidate some seg regs,
 *  - set up hash tables
 */

void ppc_vm_init(unsigned int memory_size, boot_args *args)
{
	unsigned int htabmask;
	unsigned int i;
	vm_offset_t  addr;
	int boot_task_end_offset;
#if	NCPUS > 1
	const char *cpus;
#endif	/* NCPUS > 1 */

	printf("mem_size = %d M\n",memory_size / (1024 * 1024));

	/* Stitch valid memory regions together - they may be contiguous
	 * even though they're not already glued together
	 */
	addr = 0;	/* temp use as pointer to previous memory region... */
	for (i = 1; i < kMaxDRAMBanks; i++) {
	  	if (args->PhysicalDRAM[i].size == 0)
			continue;
	  	if (args->PhysicalDRAM[i].base ==
		    args->PhysicalDRAM[addr].base +
		    args->PhysicalDRAM[addr].size) {
		  	/* The two regions are contigous, join them */
		  printf("region 0x%08x size 0x%08x joining region "
			 "0x%08x size 0x%08x\n",
			 args->PhysicalDRAM[addr].base,
			 args->PhysicalDRAM[addr].size,
			 args->PhysicalDRAM[i].base,
			 args->PhysicalDRAM[i].size);
			args->PhysicalDRAM[addr].size +=
			  	args->PhysicalDRAM[i].size;
			args->PhysicalDRAM[i].size = 0;
			/* "last" region remains this new bigger region */
		} else {
		  	/* move "last" region up to this one */
			addr = i;
		}
	}

	/* Go through the list of memory regions passed in via the args
	 * and copy valid entries into the pmap_mem_regions table, adding
	 * further calculated entries.
	 */
	
	pmap_mem_regions_count = 0;
	addr = 0;	/* Will use to total memory found so far */
	for (i = 0; (addr < memory_size) && (i < kMaxDRAMBanks); i++) {
		if (args->PhysicalDRAM[i].size == 0)
			continue;

		/* The following should only happen if memory size has
		   been artificially reduced with -m */
		if (addr + args->PhysicalDRAM[i].size > memory_size)
			args->PhysicalDRAM[i].size = memory_size - addr;

		/* We've found a region, tally memory */

		pmap_mem_regions[pmap_mem_regions_count].start =
			args->PhysicalDRAM[i].base;
		pmap_mem_regions[pmap_mem_regions_count].end =
			args->PhysicalDRAM[i].base +
			args->PhysicalDRAM[i].size;

		/* Regions must be provided in ascending order */
		assert ((pmap_mem_regions_count == 0) ||
			pmap_mem_regions[pmap_mem_regions_count].start >
			pmap_mem_regions[pmap_mem_regions_count-1].start);

		if (pmap_mem_regions_count > 0) {		
			/* we add on any pages not in the first memory
			 * region to the avail_remaining count. The first
			 * memory region is used for mapping everything for
			 * bootup and is taken care of specially.
			 */
			avail_remaining +=
				args->PhysicalDRAM[i].size / PPC_PGBYTES;
		}
		
		/* Keep track of how much memory we've found */

		addr += args->PhysicalDRAM[i].size;

		/* incremement number of regions found */
		pmap_mem_regions_count++;
	}
	mem_size = memory_size; /* Set up global variable */
	
	/* Initialise the pmap system, using space above `first_avail'
	 * for the necessary data structures.
         * NOTE : assume that we'll have enough space mapped in already
         */

	/* Set up the various globals describing memory usage */

	/* A small table of free physical memory left behind by the
	 * early kernel initialisation from the first contiguous memory
	 * region. We pick up this memory later via the free_regions
	 * array.
	 *
	 * Free memory can be found between end of copied exception
	 * vectors and kernel text, and
	 * between the end of kernel text and the start of kernel data.
	 */

	free_regions[free_regions_count].start =
	  	round_page((unsigned int)&_ExceptionVectorsEnd -
			   (unsigned int)&_ExceptionVectorsStart);

	/* If we are on a PDM machine memory at 1M might be used
	 * for video. TODO NMGS call video driver to do this
	 * somehow
	 */
	if (IS_VIDEO_MEMORY(VPDM_PHYSADDR)) {

		/* cut this region short at the start of video */

		free_regions[free_regions_count].end   = VPDM_PHYSADDR;

		avail_remaining += (free_regions[free_regions_count].end - 
				    free_regions[free_regions_count].start) /
			PPC_PGBYTES;
		free_regions_count++;

		/* next region now starts after end of frame buffer */
		free_regions[free_regions_count].start =
			round_page(VPDM_PHYSADDR + VPDM_FRAMEBUF_MAX_SIZE);

	}

	free_regions[free_regions_count].end   = KERNELBASE_TEXT;

        avail_remaining += (free_regions[free_regions_count].end - 
                            free_regions[free_regions_count].start) /
                                    PPC_PGBYTES;

        free_regions_count++;


	free_regions[free_regions_count].start = round_page(&etext);
	free_regions[free_regions_count].end   = KERNELBASE_DATA;

        avail_remaining += (free_regions[free_regions_count].end - 
                            free_regions[free_regions_count].start) /
                                    PPC_PGBYTES;

        free_regions_count++;

	/* For PowerMac, first_avail is set to above the bootstrap task.
         * TODO NMGS - different screen modes - might free mem?
         */

	first_avail = round_page(args->first_avail);

#if	NCPUS > 1
	/*
	 * Must Allocate interrupt stacks before kdb is called and also
	 * before vm is initialized. Must find out number of cpus first.
	 */
	/*
	 * Get number of cpus to boot, passed as an optional argument
	 * boot: mach [-sah#]	# from 0 to 9 is the number of cpus to boot
	 */
	if (wncpu < 0) {
		/* If already > 0, then was command line arg, override env */
		if ((cpus = getenv("cpus")) != NULL) {
			/* only a single digit for now */
			if ((*cpus > '0') && (*cpus <= '9'))
				wncpu = *cpus - '0';
		} else {
			wncpu = NCPUS;
		}
	}
#endif	/* NCPUS > 1 */

	pmap_bootstrap(memory_size,&first_avail);

	/* map in the exception vectors */
	{
		extern unsigned long exception_entry;
		extern unsigned long exception_end;

#if DEBUG
		printf("Mapping exception entry/exit 0x%x to 0x%x size 0x%x\n",
		       trunc_page(exception_entry),
		       trunc_page(exception_entry),
		       round_page(exception_end));
#endif /* DEBUG */
		pmap_map(trunc_page(exception_entry),
			 trunc_page(exception_entry),
			 round_page(exception_end),
			 VM_PROT_READ|VM_PROT_EXECUTE);
	}
	/*
	 * map the kernel text, data and bss. Don't forget other regions too
	 */
	for (i = 0; i < args->kern_info.region_count; i++) {
#if	MACH_KDB
		if (args->kern_info.regions[i].prot == VM_PROT_NONE &&
		    i == args->kern_info.region_count - 1) {
			/* assume that's the kernel symbol table */
			kern_sym_start = args->kern_info.regions[i].addr;
			kern_sym_size = args->kern_info.regions[i].size;
			printf("kernel symbol table at 0x%x size 0x%x\n",
			       kern_sym_start, kern_sym_size);
			args->kern_info.regions[i].prot |=
				(VM_PROT_WRITE|VM_PROT_READ);
		}
#endif	/* MACH_KDB */
#if DEBUG
		printf("mapping virt 0x%08x to phys 0x%08x size 0x%x, prot=0x%b\n",
		       ppc_trunc_page(args->kern_info.regions[i].addr),
		       ppc_trunc_page(args->kern_info.base_addr + 
				      args->kern_info.regions[i].offset),
		       ppc_round_page(args->kern_info.base_addr + 
				      args->kern_info.regions[i].size),
		       args->kern_info.regions[i].prot,
		       "\x10\1READ\2WRITE\3EXEC");
#endif /* DEBUG */
		(void)pmap_map(ppc_trunc_page(args->kern_info.regions[i].addr),
			       ppc_trunc_page(args->kern_info.base_addr + 
				      args->kern_info.regions[i].offset),
			       ppc_round_page(args->kern_info.base_addr + 
				      args->kern_info.regions[i].offset +
				      args->kern_info.regions[i].size),
			 args->kern_info.regions[i].prot);
	}
	boot_region_count = args->task_info.region_count;
	boot_size = 0;
	boot_task_end_offset = 0;
	/* Map bootstrap task pages 1-1 so that user_bootstrap can find it */
	for (i = 0; i < boot_region_count; i++) {
		if (args->task_info.regions[i].mapped) {
			/* kernel requires everything page aligned */
#if DEBUG
			printf("mapping virt 0x%08x to phys 0x%08x end 0x%x, prot=0x%b\n",
				 ppc_trunc_page(args->task_info.base_addr + 
					args->task_info.regions[i].offset),
				 ppc_trunc_page(args->task_info.base_addr + 
					args->task_info.regions[i].offset),
				 ppc_round_page(args->task_info.base_addr + 
					args->task_info.regions[i].offset +
					args->task_info.regions[i].size),
				 args->task_info.regions[i].prot,
				 "\x10\1READ\2WRITE\3EXEC");
#endif /* DEBUG */

			(void)pmap_map(
				  ppc_trunc_page(args->task_info.base_addr + 
				      args->task_info.regions[i].offset),
			          ppc_trunc_page(args->task_info.base_addr + 
				      args->task_info.regions[i].offset),
			          ppc_round_page(args->task_info.base_addr +
				      args->task_info.regions[i].offset +
				      args->task_info.regions[i].size),
			          args->task_info.regions[i].prot);

			/* Count the size of mapped space */
			boot_size += args->task_info.regions[i].size;

			/* There may be an overlapping physical page
			 * mapped to two different virtual addresses
			 */
			if (boot_task_end_offset >
			    args->task_info.regions[i].offset) {
				boot_size -= boot_task_end_offset - 
					args->task_info.regions[i].offset;
#if DEBUG
				printf("WARNING - bootstrap overlaps regions\n");
#endif /* DEBUG */
			}

			boot_task_end_offset =
				args->task_info.regions[i].offset +
				args->task_info.regions[i].size;
		}
	}

	if (boot_region_count) {

		/* Add a new region to the bootstrap task for it's stack */
		args->task_info.regions[boot_region_count].addr =
			BOOT_STACK_BASE;
		args->task_info.regions[boot_region_count].size =
			BOOT_STACK_SIZE;
		args->task_info.regions[boot_region_count].mapped = FALSE;
		boot_region_count++;
		
		boot_start        = args->task_info.base_addr;
		boot_region_desc  = (vm_offset_t) args->task_info.regions;
		/* TODO NMGS need to put param info onto top of boot stack */
		boot_task_thread_state.r1   = BOOT_STACK_PTR-0x100;
		boot_task_thread_state.srr0 = args->task_info.entry;
		boot_task_thread_state.srr1 =
			MSR_MARK_SYSCALL(MSR_EXPORT_MASK_SET);
		
		boot_thread_state_flavor = PPC_THREAD_STATE;
		boot_thread_state_count  = PPC_THREAD_STATE_COUNT;
		boot_thread_state        =
			(thread_state_t)&boot_task_thread_state;
	}

	/* Map bootstrap argument structure as data */
	(void) pmap_map((vm_offset_t)args,
			(vm_offset_t)args-KERNELBASE_DATA_OFFSET,
			((vm_offset_t)args->first_avail - 1) -
				KERNELBASE_DATA_OFFSET,
			VM_PROT_READ|VM_PROT_WRITE);

	/* Map interrupt stack red-zone */
	addr = trunc_page((vm_offset_t) &intstack - PPC_PGBYTES);
	(void) pmap_map(addr,
			addr - KERNELBASE_DATA_OFFSET,
			addr - KERNELBASE_DATA_OFFSET + PPC_PGBYTES-1,
			VM_PROT_READ);

	/* Map motherboard video memory via PTEs before removing BAT */

	if (IS_VIDEO_MEMORY(VPDM_PHYSADDR)) {
		(void) pmap_map_bd(VPDM_BASEADDR,
				   VPDM_PHYSADDR,
				   round_page(VPDM_PHYSADDR +
					      VPDM_FRAMEBUF_MAX_SIZE),
				   VM_PROT_READ | VM_PROT_WRITE);
	}
	/* Set up per_proc info */
#if 0
	per_proc_info[0].cpu_number = 0;
#endif
	per_proc_info[cpu_number()].istackptr = 0; /* we're on the intr stack */
	per_proc_info[cpu_number()].intstack_top_ss = intstack_top_ss;
#if	MACH_KGDB
	per_proc_info[cpu_number()].gdbstackptr = gdbstackptr;
	per_proc_info[cpu_number()].gdbstack_top_ss = gdbstack_top_ss;
#endif	/* MACH_KGDB */
	per_proc_info[cpu_number()].phys_exception_handlers =
		kvtophys((vm_offset_t)&exception_handlers);
	per_proc_info[cpu_number()].virt_per_proc_info = (unsigned int)
		&per_proc_info[cpu_number()];
	per_proc_info[cpu_number()].active_kloaded = (unsigned int)
		&active_kloaded[cpu_number()];
	per_proc_info[cpu_number()].cpu_data = (unsigned int)
		&cpu_data[cpu_number()];
	per_proc_info[cpu_number()].active_stacks = (unsigned int)
		&active_stacks[cpu_number()];
	per_proc_info[cpu_number()].need_ast = (unsigned int)
		&need_ast[cpu_number()];
	per_proc_info[cpu_number()].fpu_pcb = 0;

	mtsprg(0, ((unsigned int)kvtophys((vm_offset_t)&per_proc_info[cpu_number()])));

	/* r2 points to our per_proc structure whilst we're in the kernel */
	__asm__ ("mr 2, %0" : : "r" (&per_proc_info[cpu_number()]));


#if MACH_KGDB
	kgdb_kernel_in_pmap = TRUE;
#endif /* MACH_KGDB */

#if DEBUG
	for (i=0 ; i < free_regions_count; i++) {
		printf("Free region start 0x%08x end 0x%08x\n",
		       free_regions[i].start,free_regions[i].end);
	}
#endif

	sync();isync();
	if (PROCESSOR_VERSION != PROCESSOR_VERSION_601) {

/*		mtdbatu(3,BAT_INVALID); mtdbatl(3,BAT_INVALID); TODO NMGS */
/* 		mtdbatu(2,BAT_INVALID); mtdbatl(2,BAT_INVALID); TODO NMGS */
		mtdbatu(1,BAT_INVALID); mtdbatl(1,BAT_INVALID);
		mtdbatu(0,BAT_INVALID); mtdbatl(0,BAT_INVALID);

		/* Clear instr BATs too on those processors that have them */
		mtibatu(3,BAT_INVALID); mtibatl(3,BAT_INVALID);
 		mtibatu(2,BAT_INVALID); mtibatl(2,BAT_INVALID);
		mtibatu(1,BAT_INVALID); mtibatl(1,BAT_INVALID);
		mtibatu(0,BAT_INVALID); mtibatl(0,BAT_INVALID);

		/* Kernel text+rodata is at 2-4M */

		/* use dbat 0 to map 2-4M 1-1 read-only for kernel only */
		mtdbatl(0, KERNELBASE_TEXT | PTE_WIMG_DEFAULT << 3 | 1);
		mtdbatu(0, KERNELBASE_TEXT | 0xf << 2 | 2);

		/* use ibat 0 to map 2-4M 1-1 executable for kernel only */
		mtibatl(0, KERNELBASE_TEXT | PTE_WIMG_DEFAULT << 3 | 1);
		mtibatu(0, KERNELBASE_TEXT | 0xf << 2 | 2);

		/* Kernel r/w data is at 4M and up */

		/* Must only map to less than first_avail
		 * since VM addresses above that may not be 1-1
		 */
		addr = 128 * 1024;
		i = 0;
		while (KERNELBASE_DATA + (addr * 2) < first_avail) {
			i = (i * 2) +1;
			addr = addr * 2;
		}

		/* use dbat 1 to map 4M-XXX 1-1 read-write for kernel only */
		mtdbatl(1, KERNELBASE_DATA | (PTE_WIMG_DEFAULT << 3) | 2);
		mtdbatu(1, KERNELBASE_DATA | (i << 2) | 2);

		/* MP plugin has relocatable code stored in data segment */
		/* use ibat 1 to map 4M-XXX 1-1 executable for kernel only */
		mtibatl(1, KERNELBASE_DATA | (PTE_WIMG_DEFAULT << 3) | 2);
		mtibatu(1, KERNELBASE_DATA | (i << 2) | 2);
	} else {
		mtibatl(3,BAT_INVALID);
		mtibatl(2,BAT_INVALID);
		mtibatl(1,BAT_INVALID);
		mtibatl(0,BAT_INVALID);
	}
	sync();isync();
	{
		int i;
		__asm__ ("mr %0, 2" : "=r" (i));
		assert(i = (int)&per_proc_info[cpu_number()]);
	}
}

