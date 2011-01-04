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
 * Interface to new debugger.
 */
#include <cpus.h>
#include <platforms.h>
#include <mach_mp_debug.h>

#include <kern/spl.h>

#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <kgdb/kgdb_defs.h>
#include <i386/thread.h>
#include <i386/seg.h>
#include <i386/eflags.h>
#include <i386/misc_protos.h>
#include <intel/pmap.h>

#ifdef	NOT_USED_YET
#include <time_stamp.h>

#include <kern/kern_types.h>
#include <sys/reboot.h>

#include <i386/thread.h>

#include <vm/vm_map.h>
#include <kern/thread.h>
#include <kern/task.h>

typedef int spl_t;    /* it will be i386/spl.h XXX */

#endif	/* NOT_USED_YET */


int	 kgdb_active = 0;

/*
 * Current kgdb base page table pointer - read from cr3 on entry to kgdb.
 */
pt_entry_t	*kgdb_curmap;

#if	MACH_MP_DEBUG
extern int masked_state_cnt[];
#endif	/* MACH_MP_DEBUG */

/*
 *	Enter KDB through a keyboard trap.
 *	We show the registers as of the keyboard interrupt
 *	instead of those at its call to KDB.
 */
struct int_regs {
	int	gs;
	int	fs;
	int	edi;
	int	esi;
	int	ebp;
	int	ebx;
	struct i386_interrupt_state *is;
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
 * kgdb_machine_init - machine specific initialization
 */
void
kgdb_machine_init(void)
{
}

/*
 *  kgdb_trap - field a TRACE or BPT trap
 */
spl_t	kgdb_saved_efl[NCPUS];	/* EFL at trap */

int
kgdb_trap(
	int			type,
	int			code,
	struct i386_saved_state *regs)
{
	int		generic_type = -1;
	kgdb_jmp_buf_t	jmp_buf;

	kgdb_curmap = (pt_entry_t *) phystokv(get_cr3());

	kgdb_saved_efl[cpu_number()] = sploff();

	KGDB_DEBUG(("kgdb_trap:  type = 0x%x code = 0x%x regs = 0x%x\n", 
		    type, code, regs));

	if (kgdb_active) {
	    KGDB_DEBUG(("kgdb_trap:  unexpected trap in kgdb:\n"));
	    KGDB_DEBUG(("  gs  0x%8x fs  0x%8x es  0x%8x ds  0x%8x\n",
	               regs->gs,  regs->fs,  regs->es,  regs->ds));
	    KGDB_DEBUG(("  edi 0x%8x esi 0x%8x ebp 0x%8x cr2 0x%8x\n",
	               regs->edi, regs->esi, regs->ebp, regs->cr2));
	    KGDB_DEBUG(("  ebx 0x%8x edx 0x%8x ecx 0x%8x eax 0x%8x\n",
	               regs->ebx, regs->edx, regs->ecx, regs->eax));
	    KGDB_DEBUG(("  trp 0x%8x err 0x%8x eip 0x%8x cs  0x%8x\n",
	               regs->trapno, regs->err, regs->eip, regs->cs));
	    KGDB_DEBUG(("  efl 0x%8x uesp 0x%8x ss 0x%8x\n",
	               regs->efl, regs->uesp, regs->ss));
	    printf("kgdb_trap:  unexpected trap in kgdb:\n");
	    printf("  gs  0x%8x fs  0x%8x es  0x%8x ds  0x%8x\n",
	               regs->gs,  regs->fs,  regs->es,  regs->ds);
	    printf("  edi 0x%8x esi 0x%8x ebp 0x%8x cr2 0x%8x\n",
	               regs->edi, regs->esi, regs->ebp, regs->cr2);
	    printf("  ebx 0x%8x edx 0x%8x ecx 0x%8x eax 0x%8x\n",
	               regs->ebx, regs->edx, regs->ecx, regs->eax);
	    printf("  trp 0x%8x err 0x%8x eip 0x%8x cs  0x%8x\n",
	               regs->trapno, regs->err, regs->eip, regs->cs);
	    printf("  efl 0x%8x uesp 0x%8x ss 0x%8x\n",
	               regs->efl, regs->uesp, regs->ss);
	    generic_type = KGDB_KEXCEPTION;
	    kgdb_active--;
	}
	else if (kgdb_setjmp((kgdb_recover = &jmp_buf))) {
	    KGDB_DEBUG(("kgdb_trap:  interal error:  %s\n", kgdb_panic_msg));
	    printf("kgdb_trap: internal error: %s\n", kgdb_panic_msg);
	    generic_type = KGDB_KERROR;
	    type = 0;
	}


	/*
	 * Convert machine specific trap into kgdb generic type
	 */
	if (generic_type == -1) {
	    switch (type) {
		case T_DEBUG:
			generic_type = KGDB_SINGLE_STEP;
			break;
		case T_INT3:
			/* 
			 * Breakpoint set by remote gdb or Debugger trap
			 */
			generic_type = KGDB_BREAK_POINT;
			break;
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

	/* 
  	 * Package the registers for gdb.
	 */ 
	kgdb_regs.eax = regs->eax;
	kgdb_regs.edx = regs->edx;
	kgdb_regs.ecx = regs->ecx;
	kgdb_regs.ebx = regs->ebx;
	kgdb_regs.esp = regs->uesp;
	kgdb_regs.ebp = regs->ebp;
	kgdb_regs.esi = regs->esi;
	kgdb_regs.edi = regs->edi;
	kgdb_regs.eip = regs->eip;
	kgdb_regs.efl = regs->efl;
	kgdb_regs.cs = regs->cs;
	kgdb_regs.ss = regs->ss;
	kgdb_regs.ds = regs->ds;
	kgdb_regs.es = regs->es;
	kgdb_regs.fs = regs->fs;
	kgdb_regs.gs = regs->gs;
	if ((regs->cs & 0x3) == 0) {
	    /*
	     * Kernel mode - esp and ss not saved
	     */
	    kgdb_regs.esp = (int)&regs->uesp;	/* kernel stack pointer */
	    kgdb_regs.ss   = KERNEL_DS;
	}

	kgdb_active++;
	cnpollc(TRUE);
	kgdb_enter(generic_type, type, &kgdb_regs);
	cnpollc(FALSE);
	kgdb_active--;

	/* 
  	 * Package the registers for edm
	 */ 
	regs->eip    = kgdb_regs.eip;
	regs->efl    = kgdb_regs.efl;
	regs->eax    = kgdb_regs.eax;
	regs->ecx    = kgdb_regs.ecx;
	regs->edx    = kgdb_regs.edx;
	regs->ebx    = kgdb_regs.ebx;
	if (regs->cs & 0x3) {
	    /*
	     * user mode - saved esp and ss valid
	     */
	    regs->uesp = kgdb_regs.esp;		/* user stack pointer */
	    regs->ss   = kgdb_regs.ss & 0xffff;	/* user stack segment */
	}
	regs->ebp    = kgdb_regs.ebp;
	regs->esi    = kgdb_regs.esi;
	regs->edi    = kgdb_regs.edi;
	regs->es     = kgdb_regs.es & 0xffff;
	regs->cs     = kgdb_regs.cs & 0xffff;
	regs->ds     = kgdb_regs.ds & 0xffff;
	regs->fs     = kgdb_regs.fs & 0xffff;
	regs->gs     = kgdb_regs.gs & 0xffff;

	if (type == T_INT3) {
	    kgdb_bkpt_inst_t	inst;

	    (void) kgdb_read_bytes(regs->eip, sizeof(kgdb_bkpt_inst_t), 
				   (char *) &inst);
	    if (inst == BKPT_INST)
	        regs->eip += sizeof(kgdb_bkpt_inst_t);
	}

#if	NCPUS > 1
kdb_exit:
	kdb_leave();
#endif	/* NCPUS > 1 */

#if	MACH_MP_DEBUG
	masked_state_cnt[cpu_number()] = 0;
#endif	/* MACH_MP_DEBUG */

	(void) splon(kgdb_saved_efl[cpu_number()]);
	return (1);
}

/*
 *	Enter KDB through a keyboard trap.
 *	We show the registers as of the keyboard interrupt
 *	instead of those at its call to KDB.
 */

void
kgdb_kentry(
	struct int_regs		*int_regs)
{
	struct i386_interrupt_state *is = int_regs->is;

	kgdb_saved_efl[cpu_number()] = sploff();

	kgdb_curmap = (pt_entry_t *) phystokv(get_cr3());

	/* 
  	 * Package the registers for gdb.
 	 * XXX Should switch to kgdb's own stack here. 
	 */ 
	if (is->cs & 0x3) {
	    kgdb_regs.esp = ((int *)(is+1))[0];
	    kgdb_regs.ss   = ((int *)(is+1))[1];
	}
	else {
	    kgdb_regs.ss  = KERNEL_DS;
	    kgdb_regs.esp= (int)(is+1);
	}
	kgdb_regs.efl = is->efl;
	kgdb_regs.cs  = is->cs;
	kgdb_regs.eip = is->eip;
	kgdb_regs.eax = is->eax;
	kgdb_regs.ecx = is->ecx;
	kgdb_regs.edx = is->edx;
	kgdb_regs.ebx = int_regs->ebx;
	kgdb_regs.ebp = int_regs->ebp;
	kgdb_regs.esi = int_regs->esi;
	kgdb_regs.edi = int_regs->edi;
	kgdb_regs.ds  = is->ds;
	kgdb_regs.es  = is->es;
	kgdb_regs.fs  = int_regs->fs;
	kgdb_regs.gs  = int_regs->gs;

#if	NCPUS > 1
	if (!kdb_enter())
		goto kdb_exit;
#endif	/* NCPUS > 1 */

	kgdb_active++;
	cnpollc(TRUE);
	kgdb_enter(KGDB_COMMAND, 0, &kgdb_regs);
	cnpollc(FALSE);
	kgdb_active--;

	if (kgdb_regs.cs & 0x3) {
	    ((int *)(is+1))[0] = kgdb_regs.esp;
	    ((int *)(is+1))[1] = kgdb_regs.ss & 0xffff;
	}
	is->efl = kgdb_regs.efl;
	is->cs  = kgdb_regs.cs & 0xffff;
	is->eip = kgdb_regs.eip;
	is->eax = kgdb_regs.eax;
	is->ecx = kgdb_regs.ecx;
	is->edx = kgdb_regs.edx;
	int_regs->ebx = kgdb_regs.ebx;
	int_regs->ebp = kgdb_regs.ebp;
	int_regs->esi = kgdb_regs.esi;
	int_regs->edi = kgdb_regs.edi;
	is->ds  = kgdb_regs.ds & 0xffff;
	is->es  = kgdb_regs.es & 0xffff;
	int_regs->fs = kgdb_regs.fs & 0xffff;
	int_regs->gs = kgdb_regs.gs & 0xffff;

#if	NCPUS > 1
kdb_exit:
	kdb_leave();
#endif	/* NCPUS > 1 */

	splon(kgdb_saved_efl[cpu_number()]);
}

boolean_t
kgdb_to_phys_addr(
	vm_offset_t	addr,
	unsigned	*kaddr)
{
	pt_entry_t *ptp;
	pt_entry_t pte;
	
	if (kgdb_curmap == (pt_entry_t *) 0) {
		*kaddr = addr;
		return(TRUE);
	}
	/*
	 * XXX: is addr a kernel or user VA ?
	 *	Assume kernel VA.
	 */
	pte = kgdb_curmap[pdenum(kernel_pmap, addr)];
	if ((pte & INTEL_PTE_VALID) == 0) {
	    	KGDB_DEBUG(("kgdb_to_phys_addr: null pte at 0x%x\n", addr));
		return(FALSE);
	}
	ptp = (pt_entry_t *) ptetokv(pte);
	ptp = &ptp[ptenum(addr)];
	if (ptp == (pt_entry_t *) 0 || (*ptp & INTEL_PTE_VALID) == 0) {
	    KGDB_DEBUG(("kgdb_to_phys_addr: null pte at 0x%x\n", addr));
	    return(FALSE);
	}
	*kaddr = (unsigned) ptetokv(*ptp) + (addr & (I386_PGBYTES-1));
	return(TRUE);
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

	KGDB_DEBUG(("kgdb_read_bytes:  addr 0x%x count 0x%x\n", addr, size));
	while (size > 0) {
	    if (!kgdb_to_phys_addr(addr, &kern_addr)) {
		return(FALSE);
	    }
	    src = (char *)kern_addr;
	    n = i386_trunc_page(addr+I386_PGBYTES) - addr;
	    if (n > size)
		n = size;
	    size -= n;
	    addr += n;
	    while (--n >= 0)
		*dst++ = *src++;
	}
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
	char		*dst;
	char		*src = data;
	int		n;
	unsigned	kern_addr;

	KGDB_DEBUG(("kgdb_write_bytes:  addr 0x%x count 0x%x data 0x%x\n", 
		    addr, size, data));
	KGDB_DEBUG(("kgdb_write_bytes: data 0x%x\n", *((int *) data)));
	while (size > 0) {
	    if (!kgdb_to_phys_addr(addr, &kern_addr)) {
		return(FALSE);
	    }
	    dst = (char *)kern_addr;
	    n = i386_trunc_page(addr+I386_PGBYTES) - addr;
	    if (n > size)
		n = size;
	    size -= n;
	    addr += n;
	    while (--n >= 0) {
		*dst++ = *src++;
	    }
	}
	KGDB_DEBUG(("kgdb_write_bytes: addresses 0x%x (0x%x)\n", 
		    addr, kern_addr));
	KGDB_DEBUG(("kgdb_write_bytes: data 0x%x (0x%x)\n", 
		    *((int *) addr), *((int *) kern_addr)));
	return(TRUE);
}

#if	NCPUS > 1
/*
 * Code used to synchronize kdb among all cpus, one active at a time, switch
 * from on to another using kdb_on! #cpu or cpu #cpu
 */

simple_lock_data_t  kdb_lock;		/* kdb lock			*/

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

	kdb_active[cpu_number()]++;
	lock_kdb();
	if (!kdb_is_slave[cpu_number()])
		master = 1;

	if (kdb_debug)
		db_printf("kdb_enter: cpu %d, master %d, kdb_cpu %d, run mode %d\n", cpu_number(), master, kdb_cpu, db_run_mode);
	if (kdb_cpu == -1 && master) {
		remote_kdb();	/* stop other cpus */
		kdb_cpu = cpu_number();
		return(1);
	} else if (kdb_cpu == cpu_number())
		return(1);
	else
		return(0);
}

void
kdb_leave(void)
{
	if (db_run_mode == STEP_CONTINUE)
		kdb_cpu = -1;
	if (kdb_is_slave[cpu_number()])
		kdb_is_slave[cpu_number()]--;
	if (kdb_debug)
		db_printf("kdb_leave: cpu %d, kdb_cpu %d, run_mode %d\n", cpu_number(), kdb_cpu, db_run_mode);
	unlock_kdb();
	kdb_active[cpu_number()]--;
}

void
lock_kdb(void)
{
	int	my_cpu = cpu_number();
	register	i;
	extern void	kdb_console(void);

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
	regs->efl |= EFL_TF;
}

/*
 * kgdb_clear_single_step - clear the single step flag for a thread
 */
void
kgdb_clear_single_step(
	kgdb_regs_t	*regs)
{
	regs->efl &= ~EFL_TF;
}
