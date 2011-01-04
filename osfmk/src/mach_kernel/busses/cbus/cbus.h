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
 * Revision 2.1.9.3  92/06/24  17:58:11  jeffreyh
 * 	Due to tty code, had to change the at386_io_lock() routine,
 * 	drivers now can sleep while locked on master.
 * 	Supressed redundant define statements
 * 	[92/06/03            bernadat]
 * 
 * Revision 2.1.9.2  92/04/30  11:45:02  bernadat
 * 	Moved shared CBUS/MBUS code to machine/mp
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.1.9.1  92/02/18  18:35:39  jeffreyh
 * 	Add is_at_io_space() macro
 * 	[91/07/16            bernadat]
 * 
 * 	Created
 * 	[91/06/27  05:01:17  bernadat]
 * 
 */
/* CMU_ENDHIST */

#ifndef _CBUS_CBUS_H_
#define _CBUS_CBUS_H_

#include <cpus.h>

#include <machine/mp/mp.h>

#define MB(n)		(1024 * 1024 * n)
#define CBUS_START	MB(64)
#define CBUS_END	(CBUS_START + MB(64))
#define CBUS_IO		CBUS_END

/*
 * The AT Bus space is mapped on CBUS space on the main processor
 * Convert CBUS address to AT BUS address and reverse 		
 * only for adresses < MB(15)	
 */

#define cbus2at(x)	((x) - CBUS_START)
#define at2cbus(x)	((x) + CBUS_START)

/*
 * CBUS IO space description
 *
 *		0000 1001 00xx xx00 0000 0000 xxxx xxxx
 *		     |      |   	      |	
 *	IO	<----|      |         	      |
 *	BOARD # <-----------|         	      |
 *	OP CODE	<-----------------------------|
 */		

/* op codes */

#define CBUS_CRESET		0x0		/* reset */
#define CBUS_SRESET		0x10	
#define	CBUS_CONTEND		0x20		
#define	CBUS_SETIDA		0x30		
#define CBUS_CSWI		0x40		/* software interrupt */
#define CBUS_SSWI		0x50
#define CBUS_CNMI		0x60		/* NMI */
#define CBUS_SNMI		0x70
#define CBUS_CLED		0x90		/* LED */
#define CBUS_SLED		0x80

/* convert op code to address */

#define CBUS_CIO_OP2AD(board, op)	((CBUS_IO | (board) << 18) | (op) | 0x1000000)

#define CBUS_NSLOTS		16
#define CBUS_NCPUS		7	/* including host */

/* Specific slots */

#define CBUS_ALL_SLOTS		0xf
#define CBUS_CONTEND_SLOT	0x0
#define CBUS_BRIDGE_SLOT	0xe
#define CBUS_CPUID_MIN		0x1
#define CBUS_CPUID_MAX		0xd

#define CBUS_STARTVEC		(CBUS_END - 16)	/* vector used at reset time */
#define CBUS_STARTVEC_VAD	phystokv(0xfffff0)

/*
 * translation windows 
 * Each window maps one page
 * DMA controlers must use windows to access memory above 16 Megs.
 * CBUS io space must be accessed through windows
 * To program them, just write page number into the associated register
 */

#define CBUS_WIN_BASE	0xf00000	/* virtual addresses for windows */
#define CBUS_WIN_REG	0xfdf000	/* regs to program windows (shorts) */
#define CBUS_N_WIN	223		/* number of windows */

#ifndef	ASSEMBLER
extern	char 	*cbus_regs;		/* where are mapped previous items */
#endif	/* ASSEMBLER */

#define	CBUS_REG2KV(x)	(cbus_regs + (x - CBUS_WIN_BASE))

#define cbus_set_win(window, add) \
	*(u_short *)(CBUS_REG2KV(CBUS_WIN_REG) + (window) * sizeof(short)) \
	= (u_int)(add) >> I386_PGSHIFT

#define cbus_get_win_vadd(window) \
	(CBUS_REG2KV(CBUS_WIN_BASE) + (window) * I386_PGBYTES)

#define cbus_get_win_padd(window) \
	(CBUS_WIN_BASE + (window) * I386_PGBYTES)

#define	CBUS_CBUS_STAT	0xfdf800	/* cbus status register */

/* CBUS STAT mask values */

#define CBUS_STAT_ARB_MASK	0xf		/* arbitration */
#define CBUS_STAT_FAIR_MASK	0x10		/* fair line */
#define CBUS_STAT_LED_MASK	0x20		/* LED control signal */
#define CBUS_STAT_INTR_MASK	0x40		/* Interrupt signal */


#define	CBUS_PRIVATE_MEM	0x80000		/* Private RAM on each board */
#define	CBUS_PRIVATE_MEM_SZ	0x1000	

/********************************************/
/* All what follows is software definitions */
/********************************************/

#ifndef	ASSEMBLER
#include <mach/machine/vm_types.h>
#include <kern/kern_types.h>
#include <i386/thread.h>

extern	int	cbus_free_wind;	/* next free window */

extern int cbus_cpu[];	/* each entry gives the cpu number or	*/
			/* -1 if empty slot			*/
extern int cbus_slot[];	/* each entry gives the slot number or	*/

#ifndef	Z1000
extern vm_offset_t cbus_io_slot[];	/* pointer to io space, indexed */
					/* by slot number		*/
extern vm_offset_t cbus_io_cpu[];	/* pointer to io space, indexed */
					/* by cpu number 		*/
#else	/* Z1000 */
	/* for Z1000, the io space must be accessed through cbus 	*/
	/* windows from master						*/
extern vm_offset_t master_io_addr[];	/* pointer to IO space, for	*/
					/* master only, indexed by cpu	*/ 
extern vm_offset_t slave_io_addr[];	/* pointer to IO space, for 	*/
					/* slaves only, indexed by cpu	*/
#endif	/* Z1000 */

extern	int	cbus_ncpus;

/* word describing the reason for the interrupt, one per cpu */

extern	cpu_int_word[];

#ifndef	Z1000
#define cbus_op(cpu, opcode) *((u_char *)cbus_io_cpu[cpu] + opcode) = 0
#endif	/* Z1000 */

#ifndef	Z1000
#define cbus_set_intr(cpu)	cbus_op(cpu, CBUS_SSWI)
#define cbus_clear_intr(cpu)	cbus_op(cpu, CBUS_CSWI)
#else	/* Z1000 */
#define cbus_set_intr(cpu)	cbus_op(cpu, ((cpu) ? CBUS_SSWI : CBUS_CSWI))
#define cbus_clear_intr(cpu)	cbus_op(cpu, ((cpu) ? CBUS_CSWI : CBUS_SSWI))
#endif	/* Z1000 */

extern vm_offset_t phys_map(
	vm_offset_t	phys_addr,
	unsigned	phys_size);

extern void w_cbus_io(
	int	slot, 
	int	opcode);

#ifdef	Z1000
extern void cbus_op(
	int	cpu,
	int	opcode);
#endif	/* Z1000 */

extern void cpu_interrupt(
	int	cpu);

extern void clear_led(
	int	cpu);

extern void set_led(
	int	cpu);

extern int cbus_getstat(void);

#if	NCPUS > 1

extern int get_ncpus(void);

extern int cbus_memsz(void);

extern void mp_intr(
	int				vec, 
	int				old_ipl, 
	char 				*ret_addr,
	struct i386_interrupt_state	*regs);

extern void cbus_putc(
	unsigned char	c);

extern int cbus_getc(
	boolean_t	wait);

extern void cbus_ill_io(
	int		pc);

extern void slave_machine_init(void);

extern void start_other_cpus(void);

#endif	/* NCPUS > 1 */

extern int cbus_alloc_win(
	int	size);

extern vm_offset_t cbus_kvtoAT_ww(
	vm_offset_t	virt,
	int		window);

extern vm_offset_t cbus_kvtoAT(
	vm_offset_t	virt);

extern boolean_t cbus_io_lock(
	processor_t	*saved_processor);

extern void cbus_io_unlock(
	processor_t	saved_processor);

#endif	/* ASSEMBLER */

/* Inter processor interrupts */

#define	CBUS_IPL			SPL6	/* software interrupt level */


/* Interrupt types */

#define CBUS_PUT_CHAR	MP_INT_AVAIL
#define CBUS_GET_CHAR	MP_INT_AVAIL+1

#if	NCPUS > 1

#ifndef	ASSEMBLER
#include <kern/processor.h>
#endif	/* ASSEMBLER */

#define at386_io_lock_state()	processor_t saved_processor = PROCESSOR_NULL;
#define at386_io_lock(op)	cbus_io_lock(&saved_processor)
#define	at386_io_unlock()	cbus_io_unlock(saved_processor)

#define is_at_io_space(pa)  	((pa < 0x100000 && pa >= 0xa0000))

#ifndef ASSEMBLER
struct rem_call_elt {
	int	(*func)(void);
	struct	rem_call_elt *next;
	int	active;
};
#endif /* ASSEMBLER */
#endif	/* NCPUS > 1 */

#endif /* _CBUS_CBUS_H_ */
