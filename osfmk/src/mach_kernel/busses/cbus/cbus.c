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
 * Revision 2.1.9.5  92/09/15  17:13:53  jeffreyh
 * 	Fixed call to clock interrupt for accurate profiling
 * 	[92/08/20            bernadat]
 * 
 * Revision 2.1.9.4  92/06/24  17:58:08  jeffreyh
 * 	Due to tty code, had to change the at386_io_lock() routine,
 * 	drivers now can sleep while locked on master.
 * 	[92/06/03            bernadat]
 * 
 * Revision 2.1.9.3  92/05/26  11:18:10  jeffreyh
 * 	Fixed cbus_io_lock/cbus_io_unlock not to unbind
 * 	bound threads like X server. (This bug appeared
 * 	when merge with compaq code was done).
 * 	[92/05/18            bernadat]
 * 
 * Revision 2.1.9.2  92/04/30  11:44:54  bernadat
 * 	Moved shared CBUS/MBUS code to machine/mp
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.1.9.1  92/02/18  18:35:25  jeffreyh
 * 	Added routines for converting virt. addr. to AT bus phys. addr.
 * 	[91/12/10            bernadat]
 * 
 * 	Added Missing call to ast_check
 * 	Fixed parameters passed to interrupt
 * 	Changed call to clock interrupt for profiling
 * 	[91/09/16            bernadat]
 * 
 * 	Created
 * 	[91/06/27  05:01:04  bernadat]
 * 
 */
/* CMU_ENDHIST */

/*
 * Corollary 386 MP
 */

#include <platforms.h>
#include <eisa.h>
#include <mach_prof.h>
#include <mach_kdb.h>
#include <cpus.h>

#include <types.h>
#include <mach/i386/vm_types.h>

#include <mach/boolean.h>
#include <kern/thread.h>
#include <kern/zalloc.h>

#include <chips/busses.h>			/* For prototyping */
#include <kern/misc_protos.h>
#include <i386/AT386/misc_protos.h>
#include <i386/misc_protos.h>

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
#include <i386/eflags.h>
#include <i386/pio.h>
#include <i386/hardclock_entries.h>

#include <machine/eisa.h>
#include <machine/mp/mp.h>
#include <machine/mp/boot.h>
#include <busses/cbus/cbus.h>

#include <mach/machine.h>

#include <kern/time_out.h>
#include <i386/ipl.h>

#include <device/tty.h>
#include <device/dev_master.h>
#include <i386/db_machdep.h>

/*
 * Forward declarations
 */
void freeze_all_processors(void);

void thaw_all_processors(void);

void stamp_rate(void);

char 	*cbus_regs;	/* where cbus window and status registers	*/
			/* are mapped, for master only			*/
int	cbus_free_wind = 0;		/* first availbale free window	*/

int	cbus_cpu[CBUS_NSLOTS];	/* each entry gives the cpu number or	*/
				/* -1 if empty slot			*/
int	cbus_slot[CBUS_NCPUS];	/* each entry gives the slot number or	*/
				/* -1 if no such cpu */

#ifndef	Z1000
vm_offset_t cbus_io_slot[CBUS_NSLOTS];	/* pointer to io space, indexed */
					/* by slot number		*/
vm_offset_t cbus_io_cpu[CBUS_NCPUS];	/* pointer to io space, indexed */
					/* by cpu number 		*/
#else	/* Z1000 */
	/* for Z1000, the io space must be accessed through cbus 	*/
	/* windows from master						*/
vm_offset_t master_io_addr[CBUS_NCPUS];	/* pointer to IO space, for	*/
					/* master only, indexed by cpu	*/ 
vm_offset_t slave_io_addr[CBUS_NCPUS];	/* pointer to IO space, for 	*/
					/* slaves only, indexed by cpu	*/
#endif	/* Z1000 */

/* Write to CBUS IO space, through windows, used at init time from master */

void
w_cbus_io(
	int	slot, 
	int	opcode) 
{
#ifdef	Z1000
	cbus_set_win(cbus_free_wind, CBUS_CIO_OP2AD(slot, opcode));
	*(unsigned char *)(cbus_get_win_vadd(cbus_free_wind)+opcode) = 0;
#else	/* Z1000 */
	*(unsigned char *)(cbus_io_slot[slot]+opcode) = 0;
#endif	/* Z1000 */
}

#ifdef	Z1000
/* Write to CBUS IO space */

void
cbus_op(
	int	cpu,
	int	opcode)
{
#if	NCPUS > 1
	if (cpu_number())
		*((unsigned char *)slave_io_addr[cpu] + opcode) = 0;
	else
#endif	/* NCPUS > 1 */
		*((unsigned char *)master_io_addr[cpu] + opcode) = 0;
}
#endif	/* Z1000 */

void
cpu_interrupt(
	int	cpu)
{
	cbus_set_intr(cpu);
}

void
clear_led(
	int	cpu)
{
	cbus_op(cpu, CBUS_CLED);
}

void
set_led(
	int	cpu)
{
	cbus_op(cpu, CBUS_SLED);
}

int
cbus_getstat(void)
{
#ifdef	Z1000
	return(*(unsigned char *)CBUS_REG2KV(CBUS_CBUS_STAT));
#else	/* Z1000 */
	return(inb(0xf1));
#endif	/* Z1000 */
}

/*
 * Map some physical space into virtual memory with R/W access
 * phys_addr:	Physical byte address
 * phys_size:	size in bytes
 *
 * returns virtual address
 *
 * Warning: Performs roundings to map on page boundaries, might map
 * more than needed
 */

vm_offset_t
phys_map(
	vm_offset_t	phys_addr,
	unsigned	phys_size)
{
	vm_offset_t	vad, pad;
	unsigned length;

	pad = trunc_page(phys_addr);
	length = round_page((phys_addr - pad) + phys_size);
	if (kmem_alloc_pageable(kernel_map, &vad, length) != KERN_SUCCESS) {
		printf("phys_map(%x , %x) failed, could not allocate virtual memory\n", phys_addr, phys_size);
		return(vad);
	}
	(void)pmap_map_bd(vad, pad, 
			(vm_offset_t)pad+length, 
			VM_PROT_READ | VM_PROT_WRITE);
	return(vad + (phys_addr - pad));
}

#if	NCPUS > 1

pcb_t	prev_pcb[NCPUS];	/* used at thread switch time		*/

void
freeze_all_processors(void)
{
	printf("dont know how to freeze other processors\n");
}

void
thaw_all_processors(void) {
	printf("dont know how to thaw other processors\n");
}

/* Word describing origin of interrupt */

int	cpu_int_word[NCPUS];

unsigned char	cbus_ochar, cbus_ichar;
int		cbus_wait_char;		/* must wait for character	*/
int		cbus_nintr[NCPUS];	/* total number of interrupts	*/
int		cbus_nclock[NCPUS];	/* number of clock interrupts	*/
int		cbus_nast[NCPUS];	/* number of remote ASTs 	*/
int		cbus_ntlb[NCPUS];	/* number of tlb shutdowns	*/


/* CBUS interrupt routine */
/* Need to replace this code with some new one using ffs() */

void
mp_intr(
	int				vec,
	int				old_ipl,
	char				*ret_addr, /* ret addr in  handler */
	struct i386_interrupt_state	*regs)
{
	register mycpu = cpu_number();
	volatile int	*my_word = &cpu_int_word[mycpu];
	extern char	return_to_iret[];
	extern void	softclock(void);

	cbus_nintr[mycpu]++;
	do {
		if (i_bit(MP_CLOCK, my_word)) {
			i_bit_clear(MP_CLOCK, my_word);
			cbus_nclock[mycpu]++;
			hardclock(vec, old_ipl, ret_addr, regs);
		} else if (i_bit(MP_TLB_FLUSH, my_word)) {
			i_bit_clear(MP_TLB_FLUSH, my_word);
			cbus_ntlb[mycpu]++;
			pmap_update_interrupt();
		} else if (i_bit(MP_SOFTCLOCK, my_word)) {
			i_bit_clear(MP_SOFTCLOCK, my_word);
			softclock();
		} else if (i_bit(MP_AST, my_word)) {
			i_bit_clear(MP_AST, my_word);
			cbus_nast[mycpu]++;
			ast_check();
		} else if (i_bit(CBUS_PUT_CHAR, my_word)) {
			volatile unsigned char c = cbus_ochar;
			i_bit_clear(CBUS_PUT_CHAR, my_word);
			cnputc(c);
		} else if (i_bit(CBUS_GET_CHAR, my_word)) {
			if (cbus_wait_char)
				cbus_ichar = cngetc();
			else
				cbus_ichar = cnmaygetc();
			i_bit_clear(CBUS_GET_CHAR, my_word);
#if	MACH_KDB
		} else if (i_bit(MP_KDB, my_word)) {
			extern kdb_is_slave[];

			i_bit_clear(MP_KDB, my_word);
			kdb_is_slave[mycpu]++;
			kdb_kintr();
#endif	/* MACH_KDB */
		}
		cbus_clear_intr(mycpu);
	} while (*my_word);
}

/* print a character on master cpu console, called from a slave */

void
cbus_putc(
	unsigned char	c)
{
	volatile int	*master_word = &cpu_int_word[0];

	while(i_bit(CBUS_PUT_CHAR, master_word));
	cbus_ochar = c;
	i_bit_set(CBUS_PUT_CHAR, master_word);
	cpu_interrupt(0);
}

/* print a character on master cpu console, called from a slave */

int
cbus_getc(
	boolean_t	wait)
{
	volatile int	*master_word = &cpu_int_word[0];

	cbus_wait_char = wait;
	i_bit_set(CBUS_GET_CHAR, master_word);
	cpu_interrupt(0);
	while(i_bit(CBUS_GET_CHAR, master_word));
	return(cbus_ichar);
}


void
cbus_ill_io(
	int	pc)
{
	printf("386 IO instruction not allowed on slave cpus, called by %x\n", pc);
#if	MACH_KDB
	if (!db_active)
		Debugger("Illegal instruction");
#endif	/* MACH_KDB */
}

#endif	/* NCPUS > 1 */

/*
 * Allocate cbus windows
 */

int
cbus_alloc_win(
	int	size)
{
	int rc;

	if (is_eisa_bus)
		panic("No CBUS windows with EISA bus");
	if (cbus_free_wind + size > CBUS_N_WIN)
		panic("Out of Cbus windows");
	rc = cbus_free_wind;
	cbus_free_wind += size;
	return(rc);
}

#if	NCPUS > 1 && TIME_STAMP

/*
 *	Print time stamp rate
 */

void
stamp_rate(void)
{
	extern unsigned time_stamp, time_stamp_cum, nstamps, old_time_stamp;
	
	printf("%d stamps per clock interrupt\n", time_stamp_cum/nstamps);

}

#endif	/* NCPUS > 1 && TIME_STAMP */


	/* 
         * I/O support.
	 * Convert virtual addresses to AT physical addresses.
         *
	 * for CBUS, addresses start at 4000000 (CBUS_START) and are
	 * not visible to AT boards. We must use regular AT addresses
         * which point to the same physical location. This only works
         * for addresses below 16 Megs. Otherwise must use windows.
	 */
vm_offset_t
cbus_kvtoAT_ww(
	vm_offset_t	virt,
	int		window)
{
	cbus_set_win(window, kvtophys(virt));
	return(cbus_get_win_padd(window) + (virt & (I386_PGBYTES - 1)));
}

vm_offset_t
cbus_kvtoAT(
	vm_offset_t	virt)
{
	vm_offset_t phys = kvtophys(virt);
	
 	if (phys > CBUS_START + MB(15) ||
 	    ((phys >= CBUS_START+0xA0000) && (phys < CBUS_START+0x100000))) {
		printf("cbus_kvtophys(%x), phys = %x\n", virt, phys);
		panic("Address not remapped to AT bus");
	}

	return(cbus2at(phys));
}

/*
 * lock on master for I/Os
 */

extern vm_offset_t	int_stack_top[];

boolean_t
cbus_io_lock(
	processor_t	*saved_processor)
{
 	thread_t thread;
	
	if (get_interrupt_level() > 0)
		return(TRUE);	/* Interrupt mode */
	thread = current_thread();
	if (thread == 0)
		return(TRUE);	/* Coldstart sequence */
	*saved_processor = thread->bound_processor;
	thread_bind(thread, master_processor);
	if (current_processor() != master_processor)
	    thread_block((void (*)(void)) 0);
	return(TRUE);
}

void
cbus_io_unlock(
	processor_t	saved_processor)
{
	if (get_interrupt_level() > 0)
		return;	/* Interrupt mode */
	if (current_thread() == 0)
		return;	/* Coldstart sequence */
	thread_bind(current_thread(), saved_processor);
	if ( saved_processor != PROCESSOR_NULL &&
	    current_processor() != saved_processor)
	    	thread_block((void (*)(void)) 0); 
}

#if	(NCPUS > 1) && MACH_KDB

extern unsigned char	cbus_ochar, cbus_ichar;
extern int		cbus_wait_char;

/*
 * Machine dependant code for kdb
 *
 * Console interrupt only pop on master CPU. When looping in lcok_kdb,
 * master cpu must check for console interrupts and pass characters
 * to ddb on the active cpu.
 * Also Characters can only be printed by master CPU, so
 * when looping in lock_kdb(), master must also be prepared to receive
 * characters to print from the ddb on the active cpu.
 */

void
kdb_console(void)
{
	int	my_cpu = cpu_number();
	volatile int	*my_word = &cpu_int_word[my_cpu];

	if (!my_cpu) {
		if (i_bit(CBUS_PUT_CHAR, my_word)) {
			volatile unsigned char c = cbus_ochar;
			i_bit_clear(CBUS_PUT_CHAR, my_word);
			cnputc(c);
		} else if (i_bit(CBUS_GET_CHAR, my_word)) {
			if (cbus_wait_char)
				cbus_ichar = cngetc();
			else
				cbus_ichar = cnmaygetc();
			i_bit_clear(CBUS_GET_CHAR, my_word);
		/* trying to detect break */
#ifndef notdef
		} else if (!cnmaygetc()) { 
#else	/* notdef */
		} else if (com_is_char() && !com_getc(TRUE)) {
#endif /* notdef */
			simple_unlock(&kdb_lock);
			kdb_cpu = my_cpu;
		}
	}
}

#endif	/* (NCPUS > 1) && MACH_KDB */
