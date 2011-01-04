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

#include <cpus.h>
#include <mach_kdb.h>
#include <debug.h>

#include <ddb/db_output.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/startup.h>
#include <kern/thread.h>
#include <kern/cpu_data.h>
#include <kern/queue.h>

#if	MACH_KDB
#include <ppc/db_machdep.h>
#endif	/* MACH_KDB */
#include <ppc/io_map_entries.h>
#include <ppc/misc_protos.h>
#include <ppc/pmap.h>
#include <ppc/pmap_internals.h>
#include <ppc/FirmwareCalls.h>
#include <ppc/POWERMAC/rtclock_entries.h>
#include <ppc/POWERMAC/mp/mp.h>
#include <ppc/POWERMAC/mp/MPPlugIn.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/spl.h>
#include <ppc/asm.h>

MPPlugInSpec MPspec;							/* An area for the MP interfaces */
MPEntryPts	 MPEntries;							/* Real addresses of plugin routines */

extern unsigned int intstack[];					/* declared in start.s */
extern unsigned int intstack_top_ss;			/* declared in start.s */
extern void (*exception_handlers[])(void);		/* declared in ppc_int.c */
#if	MACH_KGDB
extern unsigned int gdbstackptr;				/* declared in start.s */
extern unsigned int gdbstack_top_ss;			/* declared in start.s */
#endif	/* MACH_KGDB */

extern unsigned int MPPIwork;

void Dumphex(int cnt, unsigned long *dat);		/* (TEST/DEBUG) */

#if	NCPUS > 1

void	mp_card10_interrupt(int device, void *ssp);
void	mp_card10_interrupt(int device, void *ssp)
{
#if DEBUG
	printf("mp: Ignoring CARD10 interrupt\n");
#endif
	return;
}


/*
 *		We're gonna try to get the other processor(s) to come up into mach now.
 *		Because we don't actually know what memory is actually mapped via PTEs,
 *		we need to insure that all parameters to the secondary processors can be 
 *		mapped a a known address via BATs and real addressing.  This guy needs to 
 *		collect them and pass them on.
 *
 *		Also note that even though we initialize all fields in the state (CSA), the startup 
 *		code won't actually use 'em all (note how I cleverly used a second apostrophe to fix
 *		up the syntax coloring).  Look at the code in MPinterfaces.s to see which are actually
 *		used.
 */

void
start_other_cpus(void)
{
	int cpu, i, status, ccnt;
	unsigned int tospin;
	vm_offset_t	intstack_start;
#if	MACH_KGDB
	vm_offset_t	gdbstack_start;
#endif	/* MACH_KGDB */
	
	ccnt=0;										/* Clear CPU count */
	
	/* XXX Use the pre-allocated stacks (see start.s) */
	intstack_start = (vm_offset_t) intstack + INTSTACK_SIZE;
#if	MACH_KGDB
	gdbstack_start = (vm_offset_t) gdbstack + KERNEL_STACK_SIZE;
#endif	/* MACH_KGDB */

	for(cpu=0; cpu < NCPUS; cpu++) {				/* Cycle through all supported CPUs */
	
		/* Only start up wncpu-1 additional processors */
		if ((ccnt+1) >= wncpu)
			break;

		if(cpu==cpu_number()) continue;			/* Don't self abuse... */
		if(!machine_slot[cpu].is_cpu) continue;	/* Don't even try if no CPU here... */

		printf("Starting CPU %d\n", cpu);		/* Say what we're doing */
		
		sync();									/* Just make sure everything is done */

/*
 *		First set up the per_proc_info to the processor 
 */
 
		per_proc_info[cpu].cpu_number = cpu;		/* Set CPU number */ 
		per_proc_info[cpu].istackptr =			/* Set stack pointer */
		per_proc_info[cpu].intstack_top_ss =		/* and top-of-stack */
			intstack_start + INTSTACK_SIZE -
			sizeof (struct ppc_saved_state);
		intstack_start += INTSTACK_SIZE;
#if	MACH_KGDB
		per_proc_info[cpu].gdbstackptr =
		per_proc_info[cpu].gdbstack_top_ss =
			gdbstack_start + KERNEL_STACK_SIZE -
			sizeof (struct ppc_saved_state);
		gdbstack_start += KERNEL_STACK_SIZE;
#endif	/* MACH_KGDB */

		per_proc_info[cpu].phys_exception_handlers =
			kvtophys((vm_offset_t)&exception_handlers);
		per_proc_info[cpu].virt_per_proc_info = (unsigned int)&per_proc_info[cpu];
		per_proc_info[cpu].active_kloaded = (unsigned int)&active_kloaded[cpu];
		per_proc_info[cpu].cpu_data = (unsigned int)&cpu_data[cpu];
		per_proc_info[cpu].active_stacks = (unsigned int)&active_stacks[cpu];
		per_proc_info[cpu].need_ast = (unsigned int)&need_ast[cpu];
		per_proc_info[cpu].fpu_pcb = 0;

/*		Now set up the CPU specific area */

		CSA[cpu].state			= kSIGPResetState;	/* Set into reset state (we already should be) */
		CSA[cpu].regsAreValid	= 0;			/* Set register state to invalid */
		
		for(i=0; i<32; i++) {					/* Set all general registers */
			CSA[cpu].gpr[i]		= 0xDEADF1D0;	/* Clear general register */
		}
	
		for(i=0; i<32; i++) {					/* Set all floating point registers */
			CSA[cpu].fpr[i].lo	= 0;			/* Clear bottom of floating point */
			CSA[cpu].fpr[i].hi	= 0;			/* Clear top of floating point */
		}
	
		CSA[cpu].cr				= 0;			/* Clear CRs */
		CSA[cpu].fpscr			= 0;			/* Clear FPSCR */
		CSA[cpu].xer			= 0;			/* Clear XER */
		CSA[cpu].lr				= 0;			/* Clear LR */
		CSA[cpu].ctr			= 0;			/* Clear CTR */
		CSA[cpu].tbu			= 0;			/* Clear TBU */
		CSA[cpu].tbl			= 0;			/* Clear TBL */
		CSA[cpu].pvr			= 0;			/* Clear PVR (like we can, even) */
	
		for(i=0; i<4; i++) {					/* Set all IBAT registers */
			CSA[cpu].ibat[i].upper	= 0;		/* Clear IBAT */
			CSA[cpu].ibat[i].lower	= 0;		/* Clear IBAT */
		}
	
		for(i=0; i<4; i++) {					/* Set all DBAT registers */
			CSA[cpu].dbat[i].upper	= 0;		/* Clear DBAT */
			CSA[cpu].dbat[i].lower	= 0;		/* Clear DBAT */
		}
	
		CSA[cpu].sdr1			= powermac_info.hashTableAdr;	/* Initialize SDR1 */
	
		for(i=0; i<16; i++) {					/* Set all segment registers */
			CSA[cpu].sr[i]		= KERNEL_SEG_REG0_VALUE | i;	/* Set SRs to kernel values */
		}
		
		CSA[cpu].dar			= 0;			/* Clear DAR */
		CSA[cpu].dsisr			= 0;			/* Clear DSISR */
		

		CSA[cpu].sprg[0]		= kvtophys((vm_offset_t)&per_proc_info[cpu]);	/* Physical address of per proc */
		for(i=1; i<4; i++) {					/* Set almost all system programming registers */
			CSA[cpu].sprg[i]	= 0;			/* Clear SPRG */
		}
	
		CSA[cpu].srr0			= 0;			/* Clear SRR0 */
		CSA[cpu].srr1			= 0;			/* Clear SRR1 */
		CSA[cpu].dec			= 0x7FFFFFFF;	/* Max DEC */
		CSA[cpu].dabr			= 0;			/* Clear DABR */
		CSA[cpu].iabr			= 0;			/* Clear IABR */
		CSA[cpu].ear			= 0;			/* Clear EAR */
		
		for(i=0; i<16; i++) {					/* Set all hardware independent registers */
			CSA[cpu].hid[i]		= 0;			/* Clear HID */
		}
		
		for(i=0; i<2; i++) {					/* Set all monitor registers */
			CSA[cpu].mmcr[i]	= 0;			/* Clear MMCR */
		}
		
		for(i=0; i<4; i++) {					/* Set all performance counter registers */
			CSA[cpu].pmc[i]		= 0;			/* Clear PMC */
		}
		
		CSA[cpu].pir			= cpu;			/* Set CPU number */
		CSA[cpu].sia			= 0;			/* Clear SIA */
		CSA[cpu].mq				= 0;			/* Clear MQ*/

		CSA[cpu].msr			= MSR_SUPERVISOR_INT_OFF;	/* Set MSR to nowmal */
		CSA[cpu].pc				= (unsigned int)slave_main;	/* Clear PC */
	

		for(i=0; i<kSysRegCount; i++) {			/* Set all extra system registers */
			CSA[cpu].sysregs[i].regno		= 0;	/* Clear registers */
			CSA[cpu].sysregs[i].contents	= 0;	/* Clear registers */
		}
		
		CSA[cpu].gpr[1]			= per_proc_info[cpu].intstack_top_ss;	/* Set the initial stack */
		((unsigned int *)(per_proc_info[cpu].intstack_top_ss))[FM_BACKPTR/4]=0;	/* Clear the backpointer in the stack */
		
		CSA[cpu].gpr[2]=(unsigned int)&per_proc_info[cpu];	/* Set virtual address of per_proc_info */
		
		sync();									/* Make sure it's out there */

		CSA[cpu].regsAreValid	= 1;			/* Indicate that the CSA is valid and ready */

		status=MPstart(	cpu,					/* CPU to be started */
						(unsigned int)start_secondary-KERNELBASE_TEXT_OFFSET,	/* Processor initialization code address */
						(unsigned int)&CSA[cpu]);	/* Get the address of the CSA */
		
		if(status) {							/* Did it work? */
			printf("Start CPU %d failed: %d\n", cpu, status);	/* Nope , tell them 'bout it */
			CSA[cpu].state=kSIGPErrorState;		/* Set error state */
		}
		else {
#if 1
			tospin=0;							/* (TEST/DEBUG) Clear TO counter */
			while((volatile int)CSA[cpu].regsAreValid)	{	/* (TEST/DEBUG) Wait until processor starts */
				tospin++;						/* (TEST/DEBUG) Count the try */
				if(tospin>SpinTimeOut/2) panic("Timed out attached processor start up\n");	/* (TEST/DEBUG) We timed out */
				isync();						/* Make sure we don't prefetch valid flag */
			}
#endif
			printf("CPU %d started\n", cpu);	/* Say it worked */
			CSA[cpu].state=kSIGPOperatingState;	/* Set operational */
			ccnt++;								/* Count the started CPU */
		}
	}
	
	printf("Started %d attached processors\n", ccnt);	/* Say how many there were */
	return;										/* Time to leave... */

}

void slave_machine_init(void)
{
	int			cpu, pvrval;

	cpu = cpu_number();
	cpu_data[cpu].interrupt_level = SPLHIGH;
	machine_slot[cpu].is_cpu = TRUE;
	machine_slot[cpu].running = TRUE;
	machine_slot[cpu].cpu_type = CPU_TYPE_POWERPC;	/* We have a PPC by definition */
	__asm__ ("mfpvr %0" : : "r" (pvrval));		/* Get the PVR from the CPU */
	machine_slot[master_cpu].cpu_subtype =
		machine_slot[cpu].cpu_subtype = pvrval>>16;	/* Set the subtype */

	/* Make sure we are going to tick correctly */
	rtc_init();
}

/*
 *		Here we will find the address of the SMP firmware's external interruption handler
 *		and plug it in so the firmware will start directing the interruptions to it.
 */

void init_ast_check(processor_t	processor) {
	
	per_proc_info[processor->slot_num].cpu_flags|=SIGPactive;	/* Tell interrupt handler that we are ready for SIGPs */
#if DEBUG
	printf("SIGP enabled on cpu %d\n", processor->slot_num);	/* (TEST/DEBUG) Tell all the world about it */
#endif /* DEBUG */
}

/*
 *		Here we will send a signal to another processor requesting an AST.
 *		We'll retry a busy 256 times because back-to-back requests will give us one.
 *		Hopefully 256 is enough to do it.  Also, if this code is disable, and I don't think
 *		it is, we will need to insert a call to MPcheckPending to eliminate
 *		potential signal deadlock.
 */

#if	DEBUG
int cpu_interrupt_self[NCPUS];
int cpu_interrupt_count[NCPUS];
int cpu_interrupt_failed[NCPUS];
#endif	/* DEBUG */

kern_return_t cpu_interrupt(int cpu, int signal);

kern_return_t cpu_interrupt(int cpu, int signal)
{
	unsigned int ret, i, *svar;

	__asm__ ("mflr %0" : "=r" (i));					/* (TEST/DEBUG) */
	__asm__ ("mr %0,1" : "=r" (svar));				/* (TEST/DEBUG) */

	if (cpu == cpu_number()) {
#if DEBUG
		cpu_interrupt_self[cpu_number()]++;
#endif
		return KERN_SUCCESS;
	}

#if	DEBUG
	cpu_interrupt_count[cpu_number()]++;
#endif	/* DEBUG */

	for(i=0; i<256; i++) {							/* Keep trying for a while */
		ret=MPsignal(cpu, signal);					/* Attempt the signal */
		if(ret!=kSIGPInterfaceBusyErr) break;		/* If we did not get a busy, out of retry loop... */
	}
	
	if(ret != kSIGPnoErr) {							/* Did it work ok? */
#if	DEBUG
		cpu_interrupt_failed[cpu_number()]++;
#endif	/* DEBUG */
		return KERN_FAILURE;
	}
	
	return KERN_SUCCESS;
}

void cause_ast_check(processor_t processor)
{
	cpu_interrupt(processor->slot_num, SIGPast);
}

kern_return_t
cpu_start(
	int	slot_num)
{
	printf("cpu_start: not implemented\n");
	return KERN_FAILURE;
}

kern_return_t
cpu_control(
	int			slot_num,
	processor_info_t	info,
	unsigned int		count)
{
	printf("cpu_control: not implemented\n");
	return KERN_FAILURE;
}

int real_ncpus;
int wncpu = -1;

#if 1	

/*
 *
 *	Here's where we check if there is any MP hardware around.  For lack of a better 
 *	place, we'll initialize it all.
 *
 *	For a final thing we have a heap 'o more work to do.  This code is specific to 
 *	a single MP hardware type: the Apple/DayStar 2-way board.
 *	We will need to change the probe code to cycle through a list of supported
 *	hardware and activate the proper driver code.  It should be noted that the MP
 *	support is intended to be an extension of the hardware, not an I/O device.
 *
 */

void mp_probe_cpus(void) {

	int MPstat, procCount, i, j, cnt, dbgflag;
	kern_return_t status;
	unsigned long hammerh, band1, pci1ar, enetr, grandc;	/* Logical addresses for HW stuff */
	unsigned long hasht, seg0r, hashmask, pteg, hashv, dbgpte;
	
	/* map in devices that we need to be able to access */
	hammerh=io_map(HammerHead, PAGE_SIZE);
	band1  =io_map(Bandit1,	PAGE_SIZE);
	pci1ar =io_map(PCI1AdrReg, PAGE_SIZE);

	/* NMGS TODO these are already mapped elsewhere, reuse mapping? */
	grandc=io_map(GrandCentral, PAGE_SIZE);
	enetr=io_map(EtherNetROM, PAGE_SIZE);

#if 0
		for (i=0; i<pmap_mem_regions_count; i++) {							/* (TEST/DEBUG) */
			cnt=(pmap_mem_regions[i].end-pmap_mem_regions[i].start)>>12;	/* (TEST/DEBUG) */
			printf("pmap rgn: %02X:  %08X-%08X;  size= %08X; table=%08X\n",	/* (TEST/DEBUG) */
				i,															/* (TEST/DEBUG) */
				pmap_mem_regions[i].start,									/* (TEST/DEBUG) */
				pmap_mem_regions[i].end,									/* (TEST/DEBUG) */
				cnt,														/* (TEST/DEBUG) */
				pmap_mem_regions[i].phys_table);							/* (TEST/DEBUG) */
			dbgpte=0xFFFFFFFF;												/* (TEST/DEBUG) */
			for(j=0; j<cnt; j++) {											/* (TEST/DEBUG) */
				if(dbgpte!=pmap_mem_regions[i].phys_table[j].pte1.word) {	/* (TEST/DEBUG) */
			
					dbgflag=1;												/* (TEST/DEBUG) */
					dbgpte=pmap_mem_regions[i].phys_table[j].pte1.word;		/* (TEST/DEBUG) */
					printf("   pentr:      %08X-%08X:  %08X %08X %08X %08X\n",	/* (TEST/DEBUG) */
						((unsigned long)pmap_mem_regions[i].start)+(j<<12),		/* (TEST/DEBUG) */
						((unsigned long)pmap_mem_regions[i].start)+((j+1)<<12)-1,	/* (TEST/DEBUG) */
						pmap_mem_regions[i].phys_table[j].phys_link.prev,	/* (TEST/DEBUG) */
						pmap_mem_regions[i].phys_table[j].phys_link.next,	/* (TEST/DEBUG) */
						pmap_mem_regions[i].phys_table[j].pte1.word,		/* (TEST/DEBUG) */
						pmap_mem_regions[i].phys_table[j].locked);			/* (TEST/DEBUG) */
				}
				else {
					if(dbgflag) {											/* (TEST/DEBUG) */
						printf("               .....\n");					/* (TEST/DEBUG) */
						dbgflag=0;											/* (TEST/DEBUG) */
					}
				}
			}
		}

		hasht=powermac_info.hashTableAdr&0xFFFF0000;						/* (TEST/DEBUG) */
		hashmask=(powermac_info.hashTableAdr<<16)|0x0000FFC0;				/* (TEST/DEBUG) */
		hashv=(pci1ar>>6)^(KERNEL_SEG_REG0_VALUE<<6);						/* (TEST/DEBUG) */
		hashv=hashv&hashmask;												/* (TEST/DEBUG) */
		printf("Primary hash of %08X (%08X %08X %08X)\n",					/* (TEST/DEBUG) */
			pci1ar, hashv, hashmask, hasht);								/* (TEST/DEBUG) */
		Dumphex(64, (unsigned long *)(hasht+hashv));						/* (TEST/DEBUG) */
		printf("Secondary hash of %08X (%08X %08X %08X)\n",					/* (TEST/DEBUG) */
			pci1ar, hashv^hashmask, hashmask, hasht);						/* (TEST/DEBUG) */
		Dumphex(64, (unsigned long *)(hasht+(hashv^hashmask)));				/* (TEST/DEBUG) */
			
#endif		

	/* We are going to be generating some spurious CARD 10 interrupts,
	 * so add a dummy handler
	 */
	pmac_register_int(PMAC_DEV_CARD10, SPLBIO, mp_card10_interrupt);

	/*
	 *	Check out the hardware
	 */
	
	MPspec.versionID=kMPPlugInVersionID;				/* Set the version ID */
	if(!(MPstat=MPprobe(&MPspec, hammerh))) {			/* See if there is an MP board installed */
		printf("mp_probe_cpus: No MP hardware available - rc=%d\n", MPstat); /* Say it */
		return;
	}
	
	for(i=0; i<=kMPPlugInMaxCall; i++) {
		MPEntries.EntAddr[i]=(unsigned int)MPspec.areaAddr-KERNELBASE_TEXT_OFFSET+MPspec.offsetTableAddr[i];	/* Translate to real address */
	}

#if 0
	printf("mp_probe_cpus: MP hardware detected\n");	/* (TEST/DEBUG) */
	
	printf("mp_probe_cpus: MPPlugInSpec.versionID      =%8d\n",  MPspec.versionID);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.areaAddr       =%08X\n", MPspec.areaAddr);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.areaSize       =%08X\n", MPspec.areaSize);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.offsetTableAddr=%08X\n", MPspec.offsetTableAddr);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.baseAddr       =%08X\n", MPspec.baseAddr);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.dataArea       =%08X\n", MPspec.dataArea);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.CPUArea        =%08X\n", MPspec.CPUArea);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MPPlugInSpec.SIGPhandler    =%08X\n", MPspec.SIGPhandler);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: hammerh                     =%08X\n", hammerh);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: band1                       =%08X\n", band1);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: pci1ar                      =%08X\n", pci1ar);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: grandc                      =%08X\n", grandc);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: enetr                       =%08X\n", enetr);	/* (TEST/DEBUG) */
#endif

/*
 *	So, the funkyness in the next statement (the kernebase relocation garbage) assumes that
 *	the MP plugin code has been moved from where it was loaded to some other place.  It does
 *	it that way to facilitate a plug-and-play type thing-a-ma-bob. Plus, that's the way
 *	it was written originally.
 */

	status=MPinstall((unsigned int)MPspec.areaAddr-KERNELBASE_TEXT_OFFSET,	/* Pass area address */
						band1,							/* Pass Bandit1 logical address */
						hammerh,						/* Pass HammerHead logical address */
						grandc,							/* Pass GrandCentral logical address */
						pci1ar,							/* Pass PCI1 address configuration register logical address */
						enetr							/* Pass Ethernet ROM logical address */
					);									/* Install the other processor(s) */
	
	
#if 0	
	hasht=powermac_info.hashTableAdr&0xFFFF0000;						/* (TEST/DEBUG) */
	hashmask=(powermac_info.hashTableAdr<<16)|0x0000FFC0;				/* (TEST/DEBUG) */
	hashv=(pci1ar>>6)^(KERNEL_SEG_REG0_VALUE<<6);						/* (TEST/DEBUG) */
	hashv=hashv&hashmask;												/* (TEST/DEBUG) */
	printf("Primary hash of %08X (%08X %08X %08X)\n",					/* (TEST/DEBUG) */
		pci1ar, hashv, hashmask, hasht);								/* (TEST/DEBUG) */
	Dumphex(64, (unsigned long *)(hasht+hashv));						/* (TEST/DEBUG) */
	printf("Secondary hash of %08X (%08X %08X %08X)\n",					/* (TEST/DEBUG) */
		pci1ar, hashv^hashmask, hashmask, hasht);						/* (TEST/DEBUG) */
	Dumphex(64, (unsigned long *)(hasht+(hashv^hashmask)));				/* (TEST/DEBUG) */

	printf("mp_probe_cpus: Location 00000000:\n");		/* (TEST/DEBUG) */
	Dumphex(256, (unsigned long *)KERNELBASE_TEXT_OFFSET);	/* (TEST/DEBUG) */
	
	printf("mp_probe_cpus: PCI Address register: %08X; Interrupt register: %02X\n", *(unsigned long *)pci1ar,
					*(unsigned char *)(hammerh+IntReg));	/* (TEST/DEBUG) */

	printf("mp_probe_cpus: status=%08X (%d)\n", status, status);	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MP hardware installed\n");	/* (TEST/DEBUG) */
	printf("mp_probe_cpus: MP data area    - 0000: %08X %08X %08X %08X\n", MPspec.dataArea[0], MPspec.dataArea[1],
			MPspec.dataArea[2], MPspec.dataArea[3]);	/* (TEST/DEBUG) */	
	printf("mp_probe_cpus:                 - 0010: %08X %08X %08X %08X\n", MPspec.dataArea[4], MPspec.dataArea[5],
			MPspec.dataArea[6], MPspec.dataArea[7]);	/* (TEST/DEBUG) */	
	
	for(i=0; i<4; i++) {								/* (TEST/DEBUG) */
		printf("mp_probe_cpus: CPU %d data area - 0000: %08X %08X %08X %08X\n", i, MPspec.CPUArea[i*8+0], MPspec.CPUArea[i*8+1],
			MPspec.CPUArea[i*8+2], MPspec.CPUArea[i*8+3]);	/* (TEST/DEBUG) */	
		printf("mp_probe_cpus:                 - 0010: %08X %08X %08X %08X\n", MPspec.CPUArea[i*8+4], MPspec.CPUArea[i*8+5],
			MPspec.CPUArea[i*8+6], MPspec.CPUArea[i*8+7]);	/* (TEST/DEBUG) */	
	}
#endif

	if(status) {										/* Any errors? */
		printf("mp_probe_cpus: MP installation failed - rc=%d\n", status);	/* Say it */
		return;											/* Leave... */
	}
	
	printf("mp_probe_cpus: MP installation successful\n");	/* Say wha' happenin' */

	procCount=MPgetProcCount();							/* Count up them processors */
	if(procCount<2) {									/* Processor hardware, but only one!?! */
		printf("mp_probe_cpus: Dude!  Like, you got some kind of MP hardware, but only 1 CPU!\n");
		return;
	}
	
	MPspec.SIGPhandler=(unsigned int *)MPexternalHook();	/* Get physical address of external 'rupt filter */

	printf("mp_probe_cpus: MP installation done; %d CPUs\n", procCount);	/* Say what we got */

	real_ncpus = procCount;								/* Set the number of real CPUs */

	for (i = 0; i < real_ncpus; i++)
		machine_slot[i].is_cpu = TRUE;
}


/*
 *	Handle the highest level of signal processor requests.  Some request will have been
 *	dealt with before, so we won't be here for them.
 *
 *	If interruptions were ever enabled before we got here, pity us all.
 *
 *	Notice that we use the back up parm (MPPICParm0BU) here because as soon as MPPICPU is
 *	unlocked, MPPIParm0 may be changed by another processor.
 */

void mp_intr(void) {

	switch (MPPICPUs[cpu_number()].MPPICParm0BU) {		/* See what they wanted */
	
		case SIGPast:									/* Do they want an AST? */
			ast_check();								/* Yeah, go check it out */
			break;
			
#if	MACH_KDB
		case SIGPkdb:									/* Do they want a debugger entry? */
			kdb_is_slave[cpu_number()]++;				/* Bump up the count to show we're here */
#if 0
			Debugger("SIGPkdb");
#else
			kdb_kintr();								/* Set up to enter the debugger when we return far enough */
#endif
			break;
#endif	/* MACH_KDB */

		default:										/* This is an error, we're confused */
			printf("Invalid SIGP request: %08X\n", MPPICPUs[cpu_number()].MPPICParm0BU);
			break;

	}
}
#else	/* 1 */

void
mp_probe_cpus(void)
{
	printf("mp_probe_cpus: not implemented\n");
}

#endif	/* 1 */

void
switch_to_shutdown_context(
	thread_t	thread,
	void		(*doshutdown)(processor_t),
	processor_t	processor)
{
	printf("switch_to_shutdown_processor: not implemented\n");
}

#if	MACH_KDB
/*
 * invoke kdb on slave processors
 */

void remote_kdb(void)
{

	int		cpu;
	int		mycpu = cpu_number();

	for (cpu = 0; cpu < NCPUS; cpu++) {						/* Send the debugger request across */
		if (cpu != mycpu && machine_slot[cpu].running) {
			cpu_interrupt(cpu, SIGPkdb);
		}
	}
}


/*		This routine is supposed to clear an enter-kdb interrupt request.
 *		PowerPC doesn't need this.
 */

void clear_kdb_intr(void) {
}


/*		This routine is supposed to handle console read/writes on behalf of
 *		a non-main processor on the main processsor for the systems that can
 *		only do the I/O there.  PowerPC doesn't need this.
 */

void kdb_console(void) {
}
#endif	/* MACH_KDB */

#endif	/* NCPUS > 1 */

#if DEBUG
void Dumphex(int cnt, unsigned long *dat) {

	int ii, lines, d;
	
	lines=(cnt+31)/32;									/* Get number of lines to print */
	d=0;
	for(ii=0; ii<lines; ii++) {
		printf("    %08X  %08X %08X %08X %08X %08X %08X %08X %08X\n", &dat[d],
					dat[d+0], dat[d+1], dat[d+2], dat[d+3], dat[d+4], dat[d+5], dat[d+6], dat[d+7]);
		d+=8;
	}
	return;
}
#endif
