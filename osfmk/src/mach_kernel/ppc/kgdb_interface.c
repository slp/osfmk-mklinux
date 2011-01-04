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
 * 
 */
/*
 * MkLinux
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
 */

/*
 * 	Interface to new debugger.
 */
#include <cpus.h>
#include <platforms.h>
#include <mach_mp_debug.h>
#include <debug.h>
#include <mach_kgdb.h>

#include <kern/spl.h>

#include <kern/kern_types.h>
#include <kern/cpu_number.h>
#include <kern/cpu_data.h>
#include <kern/misc_protos.h>

#include <mach/machine/vm_param.h>
#include <kgdb/kgdb_defs.h>
#include <ppc/thread.h>
#include <ppc/proc_reg.h>
#include <ppc/pmap.h>
#include <ppc/exception.h>
#include <ppc/trap.h>
#include <ppc/kgdb_defs.h>
#include <ppc/misc_protos.h>
#include <ppc/mem.h> /* for help in translating addresses */


#if	MACH_KGDB

/* Define a kgdb stack with it's pointer set to the top */
char	kgdb_stack[KERNEL_STACK_SIZE];
char*	kgdb_stack_ptr = kgdb_stack+KERNEL_STACK_SIZE;

boolean_t kgdb_active = FALSE;
boolean_t kgdb_kernel_in_pmap = FALSE;  /* TRUE when we can use pmap */

/*
 *	int_regs
 *
 *	When kgdb is entered from the keyboard or com port interrupt
 *	handler, and we were not interrupted from an interrupt handler
 *	(i.e. we were on a kernel or user stack and *not* the interrupt
 *	stack), then the register state passed to kgdb_kentry() has the
 *	following form. See the kdb_from_iret case in locore.
 */
struct int_regs {
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int r13;
	unsigned int r14;
	unsigned int r15;
	unsigned int r16;
	unsigned int r17;
	unsigned int r18;
	unsigned int r19;
	unsigned int r20;
	unsigned int r21;
	unsigned int r22;
	unsigned int r23;
	unsigned int r24;
	unsigned int r25;
	unsigned int r26;
	unsigned int r27;
	unsigned int r28;
	unsigned int r29;
	unsigned int r30;
	unsigned int r31;
	struct ppc_interrupt_state *is;
};

kgdb_regs_t kgdb_regs;

extern char 	*trap_type[];
extern int	TRAP_TYPES;

/* Forward */

extern boolean_t kgdb_to_phys_addr(
				   vm_offset_t		addr,
				   unsigned		*kaddr);

extern void	kgdb_kentry(
			    struct int_regs         *int_regs);


#if	NCPUS > 1
extern int	kdb_enter(void);
extern void	kdb_leave(void);
extern void	lock_kdb(void);
extern void	unlock_kdb(void);
#endif	/* NCPUS > 1 */

/*
 * 	kgdb_machine_init 
 *
 *	Machine specific initialization
 */
void
kgdb_machine_init(void)
{
}

/*
 *	boolean_t kgdb_user_breakpoint(struct ppc_saved_state *ssp);
 *
 *	return TRUE if breakpoint should be passed to kgdb. kgdb should
 *	really keep track of any breakpoints set in user tasks and
 *	only intercept those breakpoints. This is complicated. In the
 *	meantime, allow the value of a variable to be set from within
 *	kgdb before the said breakpoint occurs.
 */

/* variable which can be modified by the user from within kgdb to take
 * control of future user breakpoints.
 */

boolean_t kgdb_user_breakpoint_enabled = FALSE;

boolean_t kgdb_user_breakpoint(struct ppc_saved_state *ssp)
{
	return kgdb_user_breakpoint_enabled;
}

/*
 *  	kgdb_trap
 *
 *	Enter kgdb. This function assumes interrupts have been disabled
 *	at the chip ("MSR(EE)==0") when it is called. This allows one to set 
 *	breakpoints in the spl code, and single step the spl code.
 */

struct ppc_saved_state	*kgdb_saved_state;

spl_t	kgdb_saved_spl[NCPUS];	/* SPL at trap */

int
kgdb_trap(int			type,
	  int			code,
	  struct ppc_saved_state *regs)
{
	kgdb_jmp_buf_t	jmp_buf;
	int generic_type = -1;
	
#if 0
	kgdb_saved_spl[cpu_number()] = sploff();
#endif	
	kgdb_saved_state = regs;
	
	KGDB_DEBUG(("kgdb_trap: type=%d reg=0x%x active=%d\n", 
		    type, regs, kgdb_active));
	
	/*	regDump(regs);*/
	
	if (kgdb_active) {
		KGDB_DEBUG(("kgdb_trap:  unexpected trap in kgdb:\n"));
		kgdb_longjmp(kgdb_recover, 1);
#if	DEBUG
		if (kgdb_debug)
			regDump(regs);
#endif	/* DEBUG */
		generic_type = KGDB_KEXCEPTION;
	}
	else if (kgdb_setjmp((kgdb_recover = &jmp_buf))) {
		KGDB_DEBUG(("kgdb_trap: internal error: %s\n", kgdb_panic_msg));
		generic_type = (char)KGDB_KERROR;
		type = 0;
	}
	
	/*
	 * Convert machine specific trap into kgdb generic type
	 */
	if (generic_type == -1) {
		switch (type) {
		case T_RUNMODE_TRACE:
			generic_type = KGDB_SINGLE_STEP;
			break;
		case T_PROGRAM:
			/* 
			 * Breakpoint set by remote gdb or Debugger trap
			 */
			generic_type = KGDB_BREAK_POINT;
			break;
		case T_INTERRUPT:
			/* NMI hotkey? */
		case -1:
			/*
			 * Interrupt from remote gdb
			 */
			generic_type = KGDB_COMMAND;
			break;
		default:
			/*
			 * Unexpected trap
			 */
			generic_type = KGDB_EXCEPTION;
			break;
		}
	}
#if	NCPUS > 1
	if (!kdb_enter())
		goto kdb_exit;
#endif	/* NCPUS > 1 */
	
	KGDB_DEBUG(("kgdb_trap in:  regs 0x%x &regs 0x%x\n", regs, &regs));
	
	/* 
  	 * Package the registers for gdb.
	 */ 
	kgdb_regs.r0 = regs->r0;
	kgdb_regs.r1 = regs->r1;
	kgdb_regs.r2 = regs->r2;
	kgdb_regs.r3 = regs->r3;
	kgdb_regs.r4 = regs->r4;
	kgdb_regs.r5 = regs->r5;
	kgdb_regs.r6 = regs->r6;
	kgdb_regs.r7 = regs->r7;
	kgdb_regs.r8 = regs->r8;
	kgdb_regs.r9 = regs->r9;
	kgdb_regs.r10 = regs->r10;
	kgdb_regs.r11 = regs->r11;
	kgdb_regs.r12 = regs->r12;
	kgdb_regs.r13 = regs->r13;
	kgdb_regs.r14 = regs->r14;
	kgdb_regs.r15 = regs->r15;
	kgdb_regs.r16 = regs->r16;
	kgdb_regs.r17 = regs->r17;
	kgdb_regs.r18 = regs->r18;
	kgdb_regs.r19 = regs->r19;
	kgdb_regs.r20 = regs->r20;
	kgdb_regs.r21 = regs->r21;
	kgdb_regs.r22 = regs->r22;
	kgdb_regs.r23 = regs->r23;
	kgdb_regs.r24 = regs->r24;
	kgdb_regs.r25 = regs->r25;
	kgdb_regs.r26 = regs->r26;
	kgdb_regs.r27 = regs->r27;
	kgdb_regs.r28 = regs->r28;
	kgdb_regs.r29 = regs->r29;
	kgdb_regs.r30 = regs->r30;
	kgdb_regs.r31 = regs->r31;
	
	kgdb_regs.srr0 = regs->srr0;
	kgdb_regs.srr1 = regs->srr1;
	kgdb_regs.cr  = regs->cr;
	kgdb_regs.lr  = regs->lr;
	kgdb_regs.ctr = regs->ctr;
	kgdb_regs.xer = regs->xer;
	kgdb_regs.mq  = regs->mq;
	
#if 0 /* TODO NMGS remove */
	/* This is done in kgdb_entry now */
	kgdb_regs.reason = (unsigned char)reason;
#endif
	
	/* TODO NMGS What about fp state?! */
	
	kgdb_active = TRUE;
/*	cnpollc(TRUE);*/
	kgdb_enter(generic_type, type, &kgdb_regs);
/*	cnpollc(FALSE);*/
	kgdb_active = FALSE;
	
	/* 
  	 * Package the registers for the kernel
	 */ 
	regs->r0 = kgdb_regs.r0;
	regs->r1 = kgdb_regs.r1;
	regs->r2 = kgdb_regs.r2;
	regs->r3 = kgdb_regs.r3;
	regs->r4 = kgdb_regs.r4;
	regs->r5 = kgdb_regs.r5;
	regs->r6 = kgdb_regs.r6;
	regs->r7 = kgdb_regs.r7;
	regs->r8 = kgdb_regs.r8;
	regs->r9 = kgdb_regs.r9;
	regs->r10 = kgdb_regs.r10;
	regs->r11 = kgdb_regs.r11;
	regs->r12 = kgdb_regs.r12;
	regs->r13 = kgdb_regs.r13;
	regs->r14 = kgdb_regs.r14;
	regs->r15 = kgdb_regs.r15;
	regs->r16 = kgdb_regs.r16;
	regs->r17 = kgdb_regs.r17;
	regs->r18 = kgdb_regs.r18;
	regs->r19 = kgdb_regs.r19;
	regs->r20 = kgdb_regs.r20;
	regs->r21 = kgdb_regs.r21;
	regs->r22 = kgdb_regs.r22;
	regs->r23 = kgdb_regs.r23;
	regs->r24 = kgdb_regs.r24;
	regs->r25 = kgdb_regs.r25;
	regs->r26 = kgdb_regs.r26;
	regs->r27 = kgdb_regs.r27;
	regs->r28 = kgdb_regs.r28;
	regs->r29 = kgdb_regs.r29;
	regs->r30 = kgdb_regs.r30;
	regs->r31 = kgdb_regs.r31;
	
	regs->srr0 = kgdb_regs.srr0;
	regs->srr1 = kgdb_regs.srr1;
	regs->cr  = kgdb_regs.cr;
	regs->lr  = kgdb_regs.lr;
	regs->ctr = kgdb_regs.ctr;
	regs->xer = kgdb_regs.xer;
	regs->mq  = kgdb_regs.mq;
	
	if (type == T_PROGRAM) {
		kgdb_bkpt_inst_t	inst;
		
		(void) kgdb_read_bytes(regs->eip,
				       sizeof(kgdb_bkpt_inst_t), 
				       (char *) &inst);
		
		/* If we're continuing on a breakpoint, it's because the
		 * breakpoint was made by callDebug(). We want to continue
		 * after this
		 */
		if (inst == BKPT_INST)
			regs->eip += sizeof(kgdb_bkpt_inst_t);
	}
	
        KGDB_DEBUG(("kgdb_trap out:  regs 0x%x &regs 0x%x, active=%d\n",
		    regs, &regs, kgdb_active));
	/*	regDump(regs);*/
	
#if	NCPUS > 1
 kdb_exit:
	kdb_leave();
#endif	/* NCPUS > 1 */

#if 0
	(void) splon(kgdb_saved_spl[cpu_number()]);
#endif
	return (1);
}


boolean_t
kgdb_to_phys_addr(
		  vm_offset_t	addr,
		  unsigned	*kaddr)
{
	unsigned int space;
	pte_t *pte;
	
	/* NMGS TODO - kgdb_to_phys_addr is a hack, it doesn't translate
	 * any stomped-on segments properly
	 */
	if (!kgdb_kernel_in_pmap) {
		*kaddr = addr;		/* Assume 1-1 mapping at startup */
	} else {
		/* TODO NMGS Cheat - we should copy the kernel routines in
		 * order to be able to step through them
		 */
		space = (mfsrin(addr) & 0x00FFFFFF) >> 4;
		pte = db_find_or_allocate_pte(space, trunc_page(addr), FALSE);
		if (pte == NULL)
			return TRUE;
		*kaddr = trunc_page(pte->pte1.word)| (addr & page_mask);
	}
	
	return (*kaddr != 0);
}

/*
 * Read bytes through kernel "physical" space.
 */
boolean_t
kgdb_read_bytes(
		vm_offset_t	addr,
		int		size,
		char		*data)
{
	char		*src;
	char		*dst = data;
	int		n;
	unsigned	kern_addr;
	
	/* NMGS TODO this uses virtual addresses, not physical */
	KGDB_DEBUG(("kgdb_read_bytes:  addr 0x%x count 0x%x dest 0x%08x\n",
		    addr, size, data));
	while (--size >= 0)
		*data++ = *(char*)addr++;
	KGDB_DEBUG(("kgdb_read_bytes:  success 0x%x\n", *((int *) data)));
	return(TRUE);
}

/*
 * Write bytes through kernel "physical" space.
 */
boolean_t
kgdb_write_bytes(
		 vm_offset_t	addr,
		 int		size,
		 char		*data)
{
	int		n,max;
	unsigned	phys_dst;
	unsigned 	phys_src;
	
	/* These three are used to read back and verify copy was ok */
	vm_offset_t	addr_copy = addr;
	int		size_copy = size;
        char           *data_copy = data;
	
	KGDB_DEBUG(("kgdb_write_bytes:  addr 0x%x count 0x%x data 0x%x\n", 
		    addr, size, data));
	KGDB_DEBUG(("kgdb_write_bytes: data 0x%x\n", *((int *) data)));
	
	while (size > 0) {
		if (!kgdb_to_phys_addr(addr, &phys_dst)) {
			return(FALSE);
		}
		if (!kgdb_to_phys_addr((vm_offset_t)data, &phys_src)) {
			return(FALSE);
		}
		/* don't over-run any page boundaries - check src range */
		max = ppc_round_page(phys_src) - phys_src;
		if (max > size)
			max = size;
		/* Check destination won't run over boundary either */
		n = ppc_round_page(phys_dst) - phys_dst;
		if (n < max)
			max = n;
		size -= max;
		addr += max;
		KGDB_DEBUG(("calling phys_copy(0x%08x,0x%08x,%d)\n",
			    phys_src,phys_dst,max));
		phys_copy(phys_src, phys_dst, max);

		/* resync I+D caches */
		sync_cache(phys_dst, max);

		phys_src += max;
		phys_dst += max;
	}
#if DEBUG
	/* Verify that the physical write wrote to the good virtual addr */
	while (--size_copy >= 0)
		if (*(char*)addr_copy++ != *data_copy++)
			printf("WARNING!!! %c WRITE NOT CORRECT\n",7);
#endif /* DEBUG */
	return(TRUE);
}

#if	NCPUS > 1
/*
 * Code used to synchronize kdb among all cpus, one active at a time, switch
 * from on to another using kdb_on! #cpu or cpu #cpu
 */

decl_simple_lock_data(, kdb_lock)	/* kdb lock			*/

int	kdb_cpu = -1;			/* current cpu running kdb	*/
int	kdb_debug = 0;
int	kdb_is_slave[NCPUS];
int	kdb_active[NCPUS];

/*
 * Called when entering kdb:
 * Takes kdb lock. If if we were called remotly (slave state) we just
 * wait for kdb_cpu to be equal to cpu_number(). Otherwise enter kdb if
 * not active on another cpu
 */

int
kdb_enter(void)
{
	int master = 0;
	int retval;

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	kdb_active[cpu_number()]++;
	lock_kdb();
	if (!kdb_is_slave[cpu_number()])
		master = 1;

	if (kdb_debug)
		db_printf("kdb_enter: cpu %d, master %d, kdb_cpu %d, run mode %d\n", cpu_number(), master, kdb_cpu, db_run_mode);
	if (kdb_cpu == -1 && master) {
		remote_kdb();	/* stop other cpus */
		kdb_cpu = cpu_number();
		retval = 1;
	} else if (kdb_cpu == cpu_number())
		retval = 1;
	else
		retval = 0;

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */

	return(retval);
}

void
kdb_leave(void)
{
#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	if (db_run_mode == STEP_CONTINUE)
		kdb_cpu = -1;
	if (kdb_is_slave[cpu_number()])
		kdb_is_slave[cpu_number()]--;
	if (kdb_debug)
		db_printf("kdb_leave: cpu %d, kdb_cpu %d, run_mode %d\n", cpu_number(), kdb_cpu, db_run_mode);
	unlock_kdb();
	kdb_active[cpu_number()]--;

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */
}

void
lock_kdb(void)
{
	int		my_cpu;
	register	i;
	extern void	kdb_console(void);

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	my_cpu = cpu_number();

	for(;;) {
		kdb_console();
		if (kdb_cpu != -1 && kdb_cpu != my_cpu) {
			continue;
		}
		simple_lock(&kdb_lock);
		if (kdb_cpu == -1 || kdb_cpu == my_cpu)
			break;
		simple_unlock(&kdb_lock);
	} 

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */
}

#if	TIME_STAMP
extern unsigned old_time_stamp;
#endif	/* TIME_STAMP */

void
unlock_kdb(void)
{
	simple_unlock(&kdb_lock);
#if	TIME_STAMP
	old_time_stamp = 0;
#endif	/* TIME_STAMP */
}


#ifdef	__STDC__
#define KDB_SAVE(type, name) extern type name; type name##_save = name
#define KDB_RESTORE(name) name = name##_save
#else	/* __STDC__ */
#define KDB_SAVE(type, name) extern type name; type name/**/_save = name
#define KDB_RESTORE(name) name = name/**/_save
#endif	/* __STDC__ */

#define KDB_SAVE_CTXT() \
	KDB_SAVE(int, db_run_mode); \
	KDB_SAVE(boolean_t, db_sstep_print); \
	KDB_SAVE(int, db_loop_count); \
	KDB_SAVE(int, db_call_depth); \
	KDB_SAVE(int, db_inst_count); \
	KDB_SAVE(int, db_last_inst_count); \
	KDB_SAVE(int, db_load_count); \
	KDB_SAVE(int, db_store_count); \
	KDB_SAVE(boolean_t, db_cmd_loop_done); \
	KDB_SAVE(jmp_buf_t *, db_recover); \
	KDB_SAVE(db_addr_t, db_dot); \
	KDB_SAVE(db_addr_t, db_last_addr); \
	KDB_SAVE(db_addr_t, db_prev); \
	KDB_SAVE(db_addr_t, db_next); \
	KDB_SAVE(db_regs_t, ddb_regs); 

#define KDB_RESTORE_CTXT() \
	KDB_RESTORE(db_run_mode); \
	KDB_RESTORE(db_sstep_print); \
	KDB_RESTORE(db_loop_count); \
	KDB_RESTORE(db_call_depth); \
	KDB_RESTORE(db_inst_count); \
	KDB_RESTORE(db_last_inst_count); \
	KDB_RESTORE(db_load_count); \
	KDB_RESTORE(db_store_count); \
	KDB_RESTORE(db_cmd_loop_done); \
	KDB_RESTORE(db_recover); \
	KDB_RESTORE(db_dot); \
	KDB_RESTORE(db_last_addr); \
	KDB_RESTORE(db_prev); \
	KDB_RESTORE(db_next); \
	KDB_RESTORE(ddb_regs); 

/*
 * switch to another cpu
 */

void
kdb_on(
	int		cpu)
{
	KDB_SAVE_CTXT();
	if (cpu < 0 || cpu >= NCPUS || !kdb_active[cpu])
		return;
	kdb_cpu = cpu;
	unlock_kdb();
	lock_kdb();
	KDB_RESTORE_CTXT();
	if (kdb_cpu == -1) /* someone continued */
		db_continue_cmd(0, 0, 0, "");
}

#endif	/* NCPUS > 1 */

/*
 * kgdb_set_single_step - set single step flag for thread
 */
void
kgdb_set_single_step(
	kgdb_regs_t	*regs)
{
	KGDB_DEBUG(("SINGLE STEP SET\n"));
	regs->srr1 |= MASK(MSR_SE);
}

/*
 * kgdb_clear_single_step - clear the single step flag for a thread
 */
void
kgdb_clear_single_step(
	kgdb_regs_t	*regs)
{
	KGDB_DEBUG(("SINGLE STEP DISABLED\n"));
	regs->srr1 &= ~MASK(MSR_SE);
}

#if 0
/* If we switch to using scc support from under chips, these can be used */
extern int	scc_putc(int unit,int line, int c);
extern int	scc_getc(int unit, int line, boolean_t wait, boolean_t raw);
extern void     switch_to_kd(void);
extern void     switch_to_com1(void);

static int kgdb_current_port = 2;

void switch_to_kd(void)
{
	kgdb_current_port = 3;
}

void switch_to_com1(void)
{
	kgdb_current_port = 2;
}

void kgdb_putc(char c)
{
	scc_putc(0, kgdb_current_port, c);
}

int kgdb_getc(void)
{
	return scc_getc(0, kgdb_current_port, TRUE, FALSE);
}

#endif

#endif	/* MACH_KGDB */
