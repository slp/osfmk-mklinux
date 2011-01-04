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
 * Revision 2.1.3.1  92/04/30  11:59:47  bernadat
 * 	Code for MBUS based machines: SystemPro
 * 	[92/04/08            bernadat]
 * 
 */

/*
 * Support for the COMPAQ SystemPro MBUS architecture
 *
 * WARNING: 	The only tested configurations are 2 or 2 486 cpus with
 * 		8 or 40 Megs of memory.
 *
 * The hd and wd drivers have been parallelized. The com, kd and fd
 * ones didnt seem to have any trouble. The fd driver has been changed
 * tu support memory addresses above 16 Megs.
 */

#include <mach_mp_debug.h>
#include <mach_lock_mon.h>
#include <time_stamp.h>
#include <mach_kdb.h>
#include <hi_res_clock.h>

#include <types.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/vm_param.h>
#include <i386/machparam.h>
#include <i386/trap.h>
#include <chips/busses.h>
#include <kern/time_out.h>
#include <i386/setjmp.h>
#include <i386/ipl.h>
#include <i386/db_machdep.h>
#include <mach/machine.h>
#include <kern/cpu_number.h>
#include <machine/mp/boot.h>
#include <i386/seg.h>
#include <i386/cpuid.h>
#include <i386/proc_reg.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <machine/mp/mp.h>
#include <busses/mbus/mbus.h>

/*#include <chips/busses.h>			 For prototyping */
#include <kern/misc_protos.h>
#include <i386/misc_protos.h>
#include <i386/pio.h>
#include <i386/fpu.h>
#include <i386/hi_res_clock.h>
#include <i386/hardclock_entries.h>
#include <i386/AT386/misc_protos.h>

#if	NCPUS > 1

/* Forward declarations */
int	mbus_init(
	caddr_t	port,
	void	*ui_in);

void	mbus_clear_intr(
	int	cpu); 

caddr_t	mbus_std[1] = { 0 };
struct	bus_device *mbus_info[1];
struct	bus_ctlr *mbus_minfo[1];
struct	bus_driver	mbusdriver = {
	mbus_init, 					/* probe */
	(int (*)(struct bus_device *, caddr_t))nulldev,	/* slave */
	(void (*)(struct bus_device *))nulldev,		/* attach */
	NO_DGO, 					/* start transfer */
	mbus_std, 					/* csr addresses */
	"mbus", 					/* device name */
	mbus_info, 					/* init structs */
	"mbus", 					/* controller name */
	mbus_minfo, 					/* init structs */
	0						/* flags */
};

#if	HI_RES_CLOCK
int     *high_res_clock;                 /* high-resolution clock             */
#define TIGHT_LOOP_ITERATIONS_PER_TICK 5 /* on 16MHz i386, 1 tick = 2E-6 sec  */
                                         /* T_L_I_P_T vs time given in clock.s*/
int     hr_clock_cpu;                       /* remembers last cpu started up,    */
                                         /* will use it to update the clock   */
#endif	/* HI_RES_CLOCK */

unsigned	time_stamp;

int
mbus_init(
	caddr_t	port, 	
	void	*ui_in)
{
	struct bus_ctlr *ui = (struct bus_ctlr *) ui_in;

	printf("P1_ID: %d, P2_ID %d\n", inb(P1_ID), inb(P2_ID));
/*	take_ctlr_irq(ui); Uses same level as fpu, see i386a/pic_isa.c */
	return 1;
}

int	mp_nintr[NCPUS];	/* total number of interrupts		*/
int	mp_nclock[NCPUS];	/* number of clock interrupts		*/
int	mp_nast[NCPUS];	/* number of remote ASTs 		*/
int	mp_ntlb[NCPUS];	/* number of tlb shutdowns		*/

void
mp_intr(
	int				vec, 
	int				old_ipl, 
	char 				*ret_addr,  /* ret addr in handler */
	struct i386_interrupt_state	*regs)
{
	register mycpu = cpu_number();
	volatile int	*my_word = &cpu_int_word[mycpu];
	extern char	return_to_iret[];
	extern void	softclock(void);

	if(!(inb((mycpu) ? P2_CTRL : P1_CTRL) & MBUS_INT_SET)) {
		fpintr();
		return;
	}

	mp_nintr[mycpu]++;
	do {
		if (i_bit(MP_CLOCK, my_word)) {
			i_bit_clear(MP_CLOCK, my_word);
			mp_nclock[mycpu]++;
			hardclock(vec, old_ipl, ret_addr, regs);
		} else if (i_bit(MP_TLB_FLUSH, my_word)) {
			i_bit_clear(MP_TLB_FLUSH, my_word);
			mp_ntlb[mycpu]++;
			pmap_update_interrupt();
		} else if (i_bit(MP_SOFTCLOCK, my_word)) {
			i_bit_clear(MP_SOFTCLOCK, my_word);
			softclock();
		} else if (i_bit(MP_AST, my_word)) {
			i_bit_clear(MP_AST, my_word);
			mp_nast[mycpu]++;
			ast_check();
		} else if (i_bit(MP_KDB, my_word)) {
			extern kdb_is_slave[];

			i_bit_clear(MP_KDB, my_word);
			kdb_is_slave[mycpu]++;
			kdb_kintr();
		}
		mbus_clear_intr(mycpu);
	} while (*my_word);
	return;
}

void
cpu_interrupt(
	int	cpu) 
{
	outb((cpu) ? P2_CTRL : P1_CTRL, MBUS_INT_SET|MBUS_CACHE_EN);
}

void
mbus_clear_intr(
	int	cpu) 
{
	outb((cpu) ? P2_CTRL : P1_CTRL, MBUS_CACHE_EN);
}

void
slave_machine_init(void)
{
	register	my_cpu;
	extern	int	curr_ipl[];

	my_cpu = cpu_number();
	curr_ipl[my_cpu] = SPLHI;
	if (cpuid_family > CPUID_FAMILY_386) /* set cache on */
		set_cr0(get_cr0() & ~(CR0_CD | CR0_NW));

	outb(P2_VECT, 0x40+13);	/* use int 13 for processor interrupts */
	outb(P2_CTRL, MBUS_INT_DIS|MBUS_FLUSH|MBUS_CACHE_EN);

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
		machine_slot[my_cpu].cpu_subtype = CPU_SUBTYPE_MBUS;
	init_fpu();
	printf("cpu %d active\n", my_cpu);
}

extern void master_up(void);

void
start_other_cpus(void) 
{
	register i;
	extern char slave_boot_base[], slave_boot_end[];
	extern pt_entry_t	*kpde;
	extern pstart(void);

	master_up();

	if (wncpu != 2)
		return;

#if	HI_RES_CLOCK
	hr_clock_cpu = 1;	/* clock cpu is P2 */
#endif	/* HI_RES_CLOCK */

	bcopy(slave_boot_base,
	      (char *)phystokv(MP_BOOT),
	      slave_boot_end-slave_boot_base);

	bzero((char *)(phystokv(MP_BOOTSTACK+MP_BOOT)-0x400), 0x400);
	*(vm_offset_t *)phystokv(MP_MACH_START+MP_BOOT) = 
						kvtophys((vm_offset_t) pstart);

	*(unsigned short *)phystokv(P2_RESET_VECT) = 0;
	*(((unsigned short *)phystokv(P2_RESET_VECT))+1) = MP_BOOTSEG;

	outb(P2_CTRL, MBUS_INT_DIS|MBUS_CACHE_EN|MBUS_RESET);
	outb(P1_CTRL, MBUS_FLUSH|MBUS_CACHE_EN);
	kpde[0] = 0;
}

/*
 * Find out how many real cpus there are
 */

int
get_ncpus(void)
{
	int want_ncpus = wncpu;
	register i;

	if (inb(P2_ID) == MBUS_386 || inb(P2_ID) == MBUS_486)
		return(2);
	return(1);
}

/*
 * Device driver synchronization.
 * The mutual exclusion is not an issue as long as there is
 * a single device handling thread. This is the case in MK
 * right now.
 * But spl() functions are local.
 *
 * On the SystemPro, device interrupts only occur on master (P1).
 * Nevertheless, the current ipl is local, when P2 raises IPL to 6
 * this has no influence on P1. We still need to prevent interrupts
 * on P1. 
 *
 * So we still need some kind of mutual exclusion.
 *
 * For each distinct device unit we use a distinct mp_dev_lock structure.
 * Inside this structure, we register the fact that a thread has locked
 * the device, and the fact that some device operations could not
 * be processed because the device was locked at the time it was requested.
 *
 */

/*
 * Lock a device
 * if no func is specified, wait for any other thread that already locked it.
 * if func is non null and device is already locked, just queue this function.
 * Process pending interrupt/start op if clearing ipl.
 */

boolean_t
mbus_dev_lock(
	struct mp_dev_lock	*mpq, 
	int 	op)
{
	int ret = TRUE;

	thread_t self = current_thread();
	simple_lock(&mpq->lock);
	if (mpq->count && mpq->holder != self) { 
		if (op != MP_DEV_WAIT) {
			i_bit_set(op, &mpq->pending_ops);
			ret = FALSE;
		} else {
			while(mpq->holder) {
				simple_unlock(&mpq->lock);
				printf("mbus_dev_lock: busy waiting\n");
				delay(100);
				simple_lock(&mpq->lock);
			}
			mpq->holder = self;
			mpq->count++;
		}
	} else {
		mpq->holder = self;
		mpq->count++;
	}
	simple_unlock(&mpq->lock);
	return(ret);
}

void
mbus_dev_unlock(
	struct mp_dev_lock	*mpq)
{
	simple_lock(&mpq->lock);
	while (mpq->pending_ops) {
		register i;
		for (i=0; i<MP_DEV_OP_MAX; i++)
			if (i_bit(i, &mpq->pending_ops)) {
				i_bit_clear(i, &mpq->pending_ops); 
				simple_unlock(&mpq->lock);
				(*mpq->op[i])(mpq->unit);
				simple_lock(&mpq->lock);
			}
	}
	if (--mpq->count == 0)
		mpq->holder = 0;
	simple_unlock(&mpq->lock);
}

#if	MACH_KDB
void kdb_console(void) {}
#endif	/* MACH_KDB */

#endif	/* NCPUS > 1 */

