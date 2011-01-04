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
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * MkLinux
 */
/* CMU_HIST */
/*
 * Revision 2.1.9.3  92/09/15  17:13:57  jeffreyh
 * 	fixed minor bug preventing STD+COROLLARY single cpu to boot
 * 	[92/07/02            bernadat]
 * 
 * 	Fixes to be able to compile STD+COROLLARY (Single cpu)
 * 	[92/06/09            bernadat]
 * 
 * Revision 2.1.9.2  92/04/30  11:45:18  bernadat
 * 	Moved shared CBUS/MBUS code to machine/mp
 * 	[92/04/08            bernadat]
 * 
 * 	Numbers of cpus to boot must be detected earlier because
 * 	of RB_ASKNAME << RB_SHIFT (see model_dep.c)
 * 	[92/03/19            bernadat]
 * 
 * Revision 2.1.9.1  92/02/18  18:36:06  jeffreyh
 * 	Renamed cbus_get_win_add Macro
 * 	Adapted to new MI configuration
 * 	[91/09/27            bernadat]
 * 
 * 	Added code to detect number of cpus before vm initialized
 * 	to allocate interrupt stacks earlier.
 * 	High resolution clock initialization (Jimmy Benjamin @ osf.org)
 * 	[91/08/22            bernadat]
 * 
 * 	Created
 * 	[91/06/27  05:01:40  bernadat]
 * 
 */
/* CMU_ENDHIST */

#include <cpus.h>
#include <platforms.h>
#include <hi_res_clock.h>
#include <time_stamp.h>

#include <types.h>
#include <mach/i386/vm_types.h>

#include <mach/boolean.h>
#include <kern/thread.h>
#include <kern/zalloc.h>

#include <kern/lock.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <vm/vm_page.h>

#include <mach/i386/vm_param.h>
#include <i386/machparam.h>
#include <i386/trap.h>
#include <i386/tss.h>
#include <i386/seg.h>

#include <kern/spl.h>			/* for prototyping */
#include <kern/misc_protos.h>
#include <i386/misc_protos.h>
#include <i386/fpu.h>
#include <i386/cpuid.h>

#include <machine/mp/boot.h>
#include <busses/cbus/cbus.h>

#include <mach/machine.h>

#include <chips/busses.h>
#include <i386/ipl.h>
#include <i386/seg.h>
#include <i386/hi_res_clock.h>
#include <chips/busses.h>
#include <machine/mp/mp.h>

/*
 * Forward declarations
 */
int cbus_init(
	caddr_t	port,
	void	*ui_in);

void slave_boot(
	int	cpu);

void start_slave(
	int	cpu);

void load_slave_vec(void);

void cbus_cpuarb(void);

struct cbus_conf *cbus_get_conf(void);

/*
 * Config information located somewhere below 0x100000, usually around 0xe0000
 * Having no available document or sources, we guessed all these informations.
 * For the Z1000 H/W magic_2 is null and the master cpu is not registered here.
 */

struct cbus_conf {
	unsigned magic_1;		/* must be 0xdeadbeef */
	unsigned char mem[64];		/* one byte per MB */
	unsigned char filler[20];
	unsigned char cpu[16];		/* one byte per slot */
	unsigned magic_2;		/* should be 0xfeedbeef */
};

#define is_mem(a)	((a) == 1)
#define is_cpu(a)	((a) > 0 && (a) < 8)

vm_offset_t cbus_startvec;	/* to map slave boot vector		*/
int	cbus_ncpus = 1;

extern nulldev(void);

caddr_t	cbus_std[1] = { 0 };
struct	bus_device *cbus_info[1];
struct	bus_ctlr *cbus_minfo[1];
struct	bus_driver cbusdriver = {
	cbus_init,					/* probe */
	(int (*)(struct bus_device *, caddr_t))nulldev,	/* slave */
	(void (*)(struct bus_device *))nulldev,		/* attach */
	0,						/* start transfer */
	cbus_std,					/* csr addresses */
	"cbus",						/* device name */
	cbus_info,					/* init structs */
	"cbus",						/* controller name */
	cbus_minfo,					/* init structs */
	0						/* flags */
};

int
cbus_init(
	caddr_t	port,
	void	*ui_in)
{
	int	i, j;
	int	window;
	struct mp_conf	*mp;
	struct bus_ctlr	*ui = (struct bus_ctlr *)ui_in;

	/* map cbus registers */
	if ((cbus_regs = (char *)phys_map(0xf00000, 0xe0000)) == 0) {
		printf("could not map cbus regs on at bus\n");
	}

#if	NCPUS > 1
	take_ctlr_irq(ui);

	/* map start vect for extra cpus */
	if ((cbus_startvec = phys_map(CBUS_STARTVEC, 16)) == (vm_offset_t)0) {
		printf("could not map start vec on cbus\n");
	}
#endif	/* NCPUS > 1 */

#ifndef	Z1000
	for (i = 0; i < CBUS_NSLOTS; i++)
		if ((cbus_io_slot[i] = phys_map( CBUS_CIO_OP2AD(i, 0), 0x1000)) == (vm_offset_t)0)
			printf("could not map cbus io\n");
#endif	/* Z1000 */

	for (i = 0; i < CBUS_NSLOTS; i++)
		cbus_cpu[i] = -1;
	for (i = 0; i < CBUS_NCPUS; i++)
		cbus_slot[i] = -1;

#if	NCPUS > 1
	cbus_cpuarb();
#endif	/* NCPUS > 1 */

	cbus_cpu[CBUS_BRIDGE_SLOT] = 0;		/* host processor */
	cbus_slot[0] = CBUS_BRIDGE_SLOT;	/* host processor */

	for (i = 0; i < (CBUS_NCPUS < NCPUS ? CBUS_NCPUS : NCPUS); i++)
		if (cbus_slot[i] != -1)  {
			printf("cpu %d at CBUS slot %d\n", i, cbus_slot[i]);
			machine_slot[i].is_cpu = TRUE;

#ifdef	Z1000
			window = cbus_free_wind++;
			cbus_set_win(window, CBUS_CIO_OP2AD(cbus_slot[i], 0));
			master_io_addr[i] = (vm_offset_t)cbus_get_win_vadd(window);

			if ((slave_io_addr[i] = phys_map( CBUS_CIO_OP2AD(cbus_slot[i], 0), 0x1000)) == NULL)
				printf("could not map cbus regs for slave %d\n", i);
#else	/* Z1000 */
			cbus_io_cpu[i] = cbus_io_slot[cbus_slot[i]];
#endif	/* Z1000 */
		}
	cbus_clear_intr(0);

	return 1;
}

#if	NCPUS > 1

int	next_cpu;

void
slave_boot(
	int	cpu)
{
	extern char slave_boot_base[], slave_boot_end[];
	extern pstart(void);
	extern pt_entry_t	*kpde;
	register i;
	int opri;
	extern struct fake_descriptor gdtptr;
	extern struct fake_descriptor gdt[];
	int old_ncpus = cbus_ncpus;
	extern char df_stack[];

	if (cbus_slot[cpu] == -1 || machine_slot[cpu].running == TRUE) {
		printf("Invalid cpu number\n");
		return;
	}
	next_cpu = cpu;
	bcopy(slave_boot_base,
	      (char *)phystokv(MP_BOOT),
	      slave_boot_end-slave_boot_base);
	bzero((char *)(phystokv(MP_BOOTSTACK+MP_BOOT)-0x400), 0x400);
	*(int *)phystokv(MP_MACH_START+MP_BOOT) = (int)pstart;
	opri = splhi();
	load_slave_vec();
	start_slave(cpu);
	for (i=0; i <100000; i++);
	splx(opri);
	if (cbus_ncpus != old_ncpus+1)
		printf("failed to start extra cpu\n");
	kpde[0] = 0;
}

/*
 * It is important to note that there is no real CBUS memory at the
 * start-vector address (unless all 64MB exists).  The 16 bytes are
 * held in the cache only.  Therefore, from the time the 16 bytes are
 * stored, until the time the ATM cpu fetches the code, no other cpu
 * in the system can make a memory reference that will cause that 
 * cache entry to be flushed to memory.  This restriction is true
 * whenever an ATM cpu is being started.  The start-vector is always
 * the last memory to be loaded.
 */

unsigned char bootjmp[] = { 0xea, 0, 0, MP_BOOT >> 20, MP_BOOT >> 12 };
unsigned char bootjmp2[] = { 0, 0, 0, 0, 0};

void
load_slave_vec(void)
{
	unsigned char *mp = (unsigned char *)cbus_startvec;
	int n;
	pt_entry_t *pte;

	for (n = 0; n < sizeof(bootjmp); n++ )
		mp[n] = bootjmp[n];

	for (n = 0; n < sizeof(bootjmp); n++ )
		bootjmp2[n] = mp[n];
}

/*
 * write to cbus i/o registers. Must use windows, can only work 
 * from master cpu
 */

void
start_slave(
	int	cpu)
{
	cbus_op(cpu, CBUS_CRESET);
}


void
cbus_cpuarb(void)
{
	int slot;
	int max;
	int ncpus = 1;

	w_cbus_io(CBUS_ALL_SLOTS, CBUS_SRESET);
	for (max = CBUS_CPUID_MIN; max <= CBUS_CPUID_MAX; max++) {
		w_cbus_io(CBUS_CONTEND_SLOT, CBUS_CONTEND);
		w_cbus_io(CBUS_ALL_SLOTS, CBUS_CONTEND);
		slot = cbus_getstat() & CBUS_STAT_ARB_MASK;
		if (slot == 0) {
			if (ncpus != real_ncpus)
				printf("cbus_cpuarb() found %d cpus instead of %d\n", ncpus, real_ncpus);
			return;
		}
		w_cbus_io(slot, CBUS_SETIDA);
		if ((slot >= CBUS_CPUID_MIN) && (slot < CBUS_CPUID_MAX)) {
			cbus_cpu[slot] = ncpus;
			cbus_slot[ncpus++] = slot;
		}
		slot = cbus_getstat() & CBUS_STAT_ARB_MASK;
	};
	printf("cpu arbitration failed to complete\n");
}

/*
 * Look for the configuration data in memory.
 * Need to check consitency, 0xdeadbeef appears sevreal time in memory
 */

struct cbus_conf *
cbus_get_conf(void)
{
	register i;
	register j;
	struct cbus_conf *cf;

	for (i = MB(1) - sizeof(int); i >= 0; i --) {
		cf = (struct cbus_conf *)phystokv(i);
		if (cf->magic_1 == 0xdeadbeef) {
			for (j=0; j<4; j++)
				if (!is_mem(cf->mem[j]))
					break;
			if (j == 4)
				return(cf);
		}
	}
	printf("could not find CBUS configuration information\n");
	return(0);
}

/*
 * Find out how many real cpus there are
 */

int
get_ncpus(void)
{
	register i;
	unsigned char *cpu;
	int ncpus = 0;
	struct cbus_conf *cf;

#if	Z1000
	ncpus++;
#endif	/* Z1000 */

	cf = cbus_get_conf();
	if (!cf) {
		printf("assuming single cpu\n");
		return 1;
	}
	for (i=0; i<16; i++)
		if (is_cpu(cf->cpu[i]))
			ncpus++;
	return(ncpus);
}

#endif	/* NCPUS > 1 */

/*
 * Find out memory size, returned size is in MBytes
 */

int
cbus_memsz(void)
{
	struct cbus_conf *cf;
	int size = 0;
	register i;

	cf = cbus_get_conf();
	if (!cf) {
		printf("Assuming 8 Mbytes\n");
		return(8);	/* assume 8 megabytes */
	}
	for (i=0; i<64; i++)
		if (is_mem(cf->mem[i]))
			size++;
	return(size);
}

#if	NCPUS > 1

#if	HI_RES_CLOCK
int     *high_res_clock;                 /* high-resolution clock             */
#define TIGHT_LOOP_ITERATIONS_PER_TICK 4 /* on 16MHz i386, 1 tick = 2E-6 sec  */
                                         /* T_L_I_P_T vs time given in clock.s*/
int     hr_clock_cpu;                    /* remembers last cpu started up,    */
                                         /* will use it to update the clock   */
#endif	/* HI_RES_CLOCK */

unsigned	time_stamp;

#if	TIME_STAMP
int	clock_cpu = -1;
#endif	/* TIME_STAMP */

void
slave_machine_init(void)
{
	register	my_cpu;
	extern	int	curr_ipl[];

	my_cpu = cpu_number();
	curr_ipl[my_cpu] = SPLHI;
	if (cpuid_family > CPUID_FAMILY_386) {
	  	int cr0 = get_cr0();
		
		cr0 &= ~(CR0_CD|CR0_NW); /* set cache on */
		cr0 |= (CR0_NE);	 /* No External interrups */
		set_cr0(cr0);
	}

#if	HI_RES_CLOCK
	if (my_cpu == hr_clock_cpu)  {  /* start clock thread on last cpu */
              if (kmem_alloc(kernel_map, (vm_offset_t *) &high_res_clock, 
			     PAGE_SIZE) == KERN_SUCCESS) {
                    *high_res_clock = 0;
                    printf("cpu %d starts high-resolution clock\n", my_cpu);
                    clock_thread_386 (TIGHT_LOOP_ITERATIONS_PER_TICK,
                                      high_res_clock);  /* doesn't return */
              }
              printf ("high-res clock on cpu %d failed -- kmem_alloc returned 0\n", my_cpu);
              printf ("booting cpu %d as a normal slave processor\n", my_cpu);
        }
#endif	/* HI_RES_CLOCK */

#if	TIME_STAMP
	if (my_cpu == clock_cpu)
		while(1)
			time_stamp++;
#endif	/* TIME_STAMP */
	machine_slot[my_cpu].is_cpu = TRUE;
	machine_slot[my_cpu].running = TRUE;
	machine_slot[my_cpu].cpu_type = cpuid_cputype(my_cpu);
	machine_slot[master_cpu].cpu_subtype =
		machine_slot[my_cpu].cpu_subtype = CPU_SUBTYPE_CBUS;
	init_fpu();
	printf("cpu %d active\n", my_cpu);
}

extern void master_up(void);

void
start_other_cpus(void)
{
	register i;

	master_up();

#if	HI_RES_CLOCK
	hr_clock_cpu = wncpu - 1;
#endif	/* HI_RES_CLOCK */
	if (!cpu_number())
		for (i=1; i<wncpu; i++)
			if (cbus_slot[i] != -1)
				slave_boot(i);
}

#endif	/* NCPUS > 1 */
