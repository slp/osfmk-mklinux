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
 * Interface to new debugger.
 */
#include <cpus.h>
#include <platforms.h>
#include <time_stamp.h>
#include <mach_mp_debug.h>
#include <mach_ldebug.h>
#include <db_machine_commands.h>

#include <kern/spl.h>
#include <kern/cpu_number.h>
#include <kern/kern_types.h>
#include <kern/misc_protos.h>
#include <vm/pmap.h>

#include <ppc/mem.h>
#include <ppc/thread.h>
#include <ppc/db_machdep.h>
#include <ppc/trap.h>
#include <ppc/setjmp.h>
#include <ppc/pmap.h>
#include <ppc/misc_protos.h>
#include <ppc/exception.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/serial_io.h>
#include <ppc/db_machdep.h>

#include <mach/vm_param.h>
#include <vm/vm_map.h>
#include <kern/thread.h>
#include <kern/task.h>

#include <ddb/db_command.h>
#include <ddb/db_task_thread.h>
#include <ddb/db_run.h>
#include <ddb/db_trap.h>
#include <ddb/db_output.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_break.h>
#include <ddb/db_watch.h>

int	 db_active = 0;
int	 db_pass_thru[NCPUS];
struct	 ppc_saved_state *ppc_last_saved_statep;
struct	 ppc_saved_state ppc_nested_saved_state;
unsigned ppc_last_kdb_sp;

vm_offset_t db_stacks[NCPUS];

extern	thread_act_t db_default_act;

#if	MACH_MP_DEBUG
extern int masked_state_cnt[];
#endif	/* MACH_MP_DEBUG */

/*
 *	Enter KDB through a keyboard trap.
 *	We show the registers as of the keyboard interrupt
 *	instead of those at its call to KDB.
 */
struct int_regs {
	/* XXX more registers ? */
	struct ppc_interrupt_state *is;
};

extern char *	trap_type[];
extern int	TRAP_TYPES;

/* Forward */

extern void	kdbprinttrap(
			int			type,
			int			code,
			int			*pc,
			int			sp);
extern int	db_user_to_kernel_address(
			task_t			task,
			vm_offset_t		addr,
			unsigned		*kaddr,
			int			flag);
extern void	db_write_bytes_user_space(
			vm_offset_t		addr,
			int			size,
			char			*data,
			task_t			task);
extern int	db_search_null(
			task_t			task,
			unsigned		*svaddr,
			unsigned		evaddr,
			unsigned		*skaddr,
			int			flag);
extern int	kdb_enter(int);
extern void	kdb_leave(void);
extern void	lock_kdb(void);
extern void	unlock_kdb(void);

#if DB_MACHINE_COMMANDS
struct db_command	ppc_db_commands[] = {
	{ "lt",		db_low_trace,	CS_MORE|CS_SET_DOT,	0 },
	{ (char *)0, 	0,		0,			0 }
};
#endif /* DB_MACHINE_COMMANDS */

/*
 *  kdb_trap - field a TRACE or BPT trap
 */


extern jmp_buf_t *db_recover;
spl_t	saved_ipl[NCPUS];	/* just to know what IPL was before trap */
struct ppc_saved_state *saved_state[NCPUS];

int
kdb_trap(
	int			type,
	int			code,
	struct ppc_saved_state *regs)
{
	extern char 		etext;
	boolean_t		trap_from_user;
	spl_t			s = db_splhigh();
	int			previous_console_device;

	previous_console_device=switch_to_serial_console();

	switch (type) {
	    case T_TRACE:	/* single_step */
	    {
#if 0
	    	extern int dr_addr[];
		int addr;
	    	int status = dr6();

		if (status & 0xf) {	/* hmm hdw break */
			addr =	status & 0x8 ? dr_addr[3] :
				status & 0x4 ? dr_addr[2] :
				status & 0x2 ? dr_addr[1] :
					       dr_addr[0];
			regs->efl |= EFL_RF;
			db_single_step_cmd(addr, 0, 1, "p");
		}
#endif
	    }
	    case T_PROGRAM:	/* breakpoint */
#if 0
	    case T_WATCHPOINT:	/* watchpoint */
#endif
	    case -1:	/* keyboard interrupt */
		break;

	    default:
		if (db_recover) {
		    ppc_nested_saved_state = *regs;
		    db_printf("Caught ");
		    if (type > TRAP_TYPES)
			db_printf("type %d", type);
		    else
			db_printf("%s", trap_type[type]);
		    db_printf(" trap, code = %x, pc = %x\n",
			      code, regs->srr0);
		    db_splx(s);
		    db_error("");
		    /*NOTREACHED*/
		}
		kdbprinttrap(type, code, (int *)&regs->srr0, regs->r1);
	}

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	saved_ipl[cpu_number()] = s;
	saved_state[cpu_number()] = regs;

	ppc_last_saved_statep = regs;
	ppc_last_kdb_sp = (unsigned) &type;

#if	NCPUS > 1
	if (!kdb_enter(regs->srr0))
		goto kdb_exit;
#endif	/* NCPUS > 1 */

	/*  Should switch to kdb's own stack here. */

	if (!IS_USER_TRAP(regs, &etext)) {
		bzero((char *)&ddb_regs, sizeof (ddb_regs));
		ddb_regs = *regs;
		trap_from_user = FALSE;
	}
	else {
		ddb_regs = *regs;
		trap_from_user = TRUE;
	}
#if 0
	if (!trap_from_user) {
	    /*
	     * Kernel mode - esp and ss not saved
	     */
	    ddb_regs.uesp = (int)&regs->uesp;	/* kernel stack pointer */
	    ddb_regs.ss   = KERNEL_DS;
	}
#endif

	db_active++;
	db_task_trap(type, code, trap_from_user);
	db_active--;

	*regs = ddb_regs;

	if ((type == T_PROGRAM) &&
	    (db_get_task_value(regs->srr0,
			       BKPT_SIZE,
			       FALSE,
			       db_target_space(current_act(),
					       trap_from_user))
	                      == BKPT_INST))
	    regs->srr0 += BKPT_SIZE;

#if	NCPUS > 1
kdb_exit:
	kdb_leave();
#endif	/* NCPUS > 1 */

	saved_state[cpu_number()] = 0;

#if	MACH_MP_DEBUG
	masked_state_cnt[cpu_number()] = 0;
#endif	/* MACH_MP_DEBUG */

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */

	switch_to_old_console(previous_console_device);

	db_splx(s);

	return (1);
}


/*
 * Print trap reason.
 */

void
kdbprinttrap(
	int	type,
	int	code,
	int	*pc,
	int	sp)
{
	printf("kernel: ");
	if (type > TRAP_TYPES)
	    db_printf("type %d", type);
	else
	    db_printf("%s", trap_type[type]);
	db_printf(" trap, code=%x pc@%x = %x sp=%x\n",
		  code, pc, *(int *)pc, sp);
	db_run_mode = STEP_CONTINUE;
}

int
db_user_to_kernel_address(
	task_t		task,
	vm_offset_t	addr,
	unsigned	*kaddr,
	int		flag)
{
	unsigned int	sr_val;
	pte_t		*pte;

	pte = db_find_or_allocate_pte(task->map->pmap->space, trunc_page(addr),
				      FALSE);
	if (pte == PTE_NULL) {
	    if (flag) {
		db_printf("\nno memory is assigned to address %08x\n", addr);
		db_error(0);
		/* NOTREACHED */
	    }
	    return -1;
	}
	sr_val = SEG_REG_PROT |
		(task->map->pmap->space << 4) |
		((addr >> 28) & 0xff);
	mtsr(SR_COPYIN, sr_val);
	sync();
	*kaddr = (addr & 0x0fffffff) | (SR_COPYIN << 28);
	return(0);
}
	
/*
 * Read bytes from kernel address space for debugger.
 */

void
db_read_bytes(
	vm_offset_t	addr,
	int		size,
	char		*data,
	task_t		task)
{
	register char	*src;
	register int	n;
	unsigned	kern_addr;

	src = (char *)addr;
	if (task == kernel_task || task == TASK_NULL) {
	    while (--size >= 0) {
		if (addr++ > VM_MAX_KERNEL_ADDRESS) {
		    db_printf("\nbad address %x\n", addr);
		    db_error(0);
		    /* NOTREACHED */
		}
		*data++ = *src++;
	    }
	    return;
	}
	if (task->kernel_loaded) {
		while (--size >= 0) {
			if (addr++ > VM_MAX_KERNEL_LOADED_ADDRESS) {
				db_printf("\nbad address %x\n", addr);
				db_error(0);
				/* NOTREACHED */
			}
			*data++ = *src++;
		}
		return;
	}
	while (size > 0) {
	    if (db_user_to_kernel_address(task, addr, &kern_addr, 1) < 0)
		return;
	    src = (char *)kern_addr;
	    n = ppc_trunc_page(addr+PPC_PGBYTES) - addr;
	    if (n > size)
		n = size;
	    size -= n;
	    addr += n;
	    while (--n >= 0)
		*data++ = *src++;
	}
}

/*
 * Write bytes to kernel address space for debugger.
 */

void
db_write_bytes(
	vm_offset_t	addr,
	int		size,
	char		*data,
	task_t		task)
{
	int		n,max;
	unsigned	phys_dst;
	unsigned 	phys_src;
	unsigned int	space;
	pte_t		*pte;
	
	/* These three are used to read back and verify copy was ok */
	vm_offset_t	addr_copy = addr;
	int		size_copy = size;
        char           *data_copy = data;
	
	while (size > 0) {
		space = PPC_SID_KERNEL;
		pte = db_find_or_allocate_pte(space, trunc_page(data), FALSE);
		if (pte == PTE_NULL) {
			db_printf("\nno memory is assigned to src address %08x\n",
				  data);
			db_error(0);
			/* NOTREACHED */
		}
		phys_src = (trunc_page(pte->pte1.word) |
			    (((vm_offset_t) data) & page_mask));

		/* space stays as kernel space unless in another task */
		if (task != NULL)
			space = task->map->pmap->space;

		pte = db_find_or_allocate_pte(space, trunc_page(addr), FALSE);
		/* XXX the PTE is now locked (if not NULL) */
		if (pte == PTE_NULL) {
			db_printf("\nno memory is assigned to dst address %08x\n",
				  addr);
			db_error(0);
			/* NOTREACHED */
		}
		phys_dst = trunc_page(pte->pte1.word)| (addr & page_mask);

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
		phys_copy(phys_src, phys_dst, max);

		/* resync I+D caches */
		sync_cache(phys_dst, max);

		phys_src += max;
		phys_dst += max;
	}
}
	
boolean_t
db_check_access(
	vm_offset_t	addr,
	int		size,
	task_t		task)
{
	register int	n;
	unsigned int	kern_addr;

	if (task == kernel_task || task == TASK_NULL) {
	    if (kernel_task == TASK_NULL)
	        return(TRUE);
	    task = kernel_task;
	} else if (task == TASK_NULL) {
	    if (current_act() == THR_ACT_NULL)
		return(FALSE);
	    task = current_act()->task;
	}
	while (size > 0) {
	    if (db_user_to_kernel_address(task, addr, &kern_addr, 0) < 0)
		return(FALSE);
	    n = ppc_trunc_page(addr+PPC_PGBYTES) - addr;
	    if (n > size)
		n = size;
	    size -= n;
	    addr += n;
	}
	return(TRUE);
}

boolean_t
db_phys_eq(
	task_t		task1,
	vm_offset_t	addr1,
	task_t		task2,
	vm_offset_t	addr2)
{
	unsigned	kern_addr1, kern_addr2;
	pte_t		*pte_a, *pte_b;

	if ((addr1 & (PPC_PGBYTES-1)) != (addr2 & (PPC_PGBYTES-1)))
		return FALSE;

	if (task1 == TASK_NULL) {
		if (current_act() == THR_ACT_NULL)
			return FALSE;
		task1 = current_act()->task;
	}
	pte_a = db_find_or_allocate_pte(task1->map->pmap->space,
					trunc_page(addr1),
					FALSE);
	pte_b = db_find_or_allocate_pte(task2->map->pmap->space,
					trunc_page(addr2),
					FALSE);

	if ((pte_a == PTE_NULL) || (pte_b == PTE_NULL)) {
		return FALSE;
	}
	
	return (pte_a->pte1.bits.phys_page == pte_b->pte1.bits.phys_page);
}

#define DB_USER_STACK_ADDR		(0xc0000000)
#define DB_NAME_SEARCH_LIMIT		(DB_USER_STACK_ADDR-(PPC_PGBYTES*3))

int
db_search_null(
	task_t		task,
	unsigned	*svaddr,
	unsigned	evaddr,
	unsigned	*skaddr,
	int		flag)
{
	register unsigned vaddr;
	register unsigned *kaddr;

	kaddr = (unsigned *)*skaddr;
	for (vaddr = *svaddr; vaddr > evaddr; vaddr -= sizeof(unsigned)) {
	    if (vaddr % PPC_PGBYTES == 0) {
		vaddr -= sizeof(unsigned);
		if (db_user_to_kernel_address(task, vaddr, skaddr, 0) < 0)
		    return(-1);
		kaddr = (unsigned *)*skaddr;
	    } else {
		vaddr -= sizeof(unsigned);
		kaddr--;
	    }
	    if ((*kaddr == 0) ^ (flag  == 0)) {
		*svaddr = vaddr;
		*skaddr = (unsigned)kaddr;
		return(0);
	    }
	}
	return(-1);
}

void
db_task_name(
	task_t		task)
{
	register char *p;
	register int n;
	unsigned int vaddr, kaddr;

	vaddr = DB_USER_STACK_ADDR;
	kaddr = 0;

	/*
	 * skip nulls at the end
	 */
	if (db_search_null(task, &vaddr, DB_NAME_SEARCH_LIMIT, &kaddr, 0) < 0) {
	    db_printf(DB_NULL_TASK_NAME);
	    return;
	}
	/*
	 * search start of args
	 */
	if (db_search_null(task, &vaddr, DB_NAME_SEARCH_LIMIT, &kaddr, 1) < 0) {
	    db_printf(DB_NULL_TASK_NAME);
	    return;
	}

	n = DB_TASK_NAME_LEN-1;
	p = (char *)kaddr + sizeof(unsigned);
	for (vaddr += sizeof(int); vaddr < DB_USER_STACK_ADDR && n > 0; 
							vaddr++, p++, n--) {
	    if (vaddr % PPC_PGBYTES == 0) {
		(void)db_user_to_kernel_address(task, vaddr, &kaddr, 0);
		p = (char*)kaddr;
	    }
	    db_printf("%c", (*p < ' ' || *p > '~')? ' ': *p);
	}
	while (n-- >= 0)	/* compare with >= 0 for one more space */
	    db_printf(" ");
}

boolean_t
db_is_alive(void)
{
	return (db_stacks[0] != 0);
}

#if NCPUS == 1

void
db_machdep_init(void)
{
	db_stacks[0] = (vm_offset_t)(db_stack_store +
		INTSTACK_SIZE - sizeof (natural_t));
#if 0
	dbtss.esp0 = (int)(db_task_stack_store +
		INTSTACK_SIZE - sizeof (natural_t));
	dbtss.esp = dbtss.esp0;
	dbtss.eip = (int)&db_task_start;
#endif
}

#else /* NCPUS > 1 */

/*
 * Code used to synchronize kdb among all cpus, one active at a time, switch
 * from on to another using kdb_on! #cpu or cpu #cpu
 */

decl_simple_lock_data(, kdb_lock)	/* kdb lock			*/

#define	db_simple_lock_init(l, e)	hw_lock_init(&((l)->interlock))
#define	db_simple_lock_try(l)		hw_lock_try(&((l)->interlock))
#define	db_simple_unlock(l)		hw_lock_unlock(&((l)->interlock))

int			kdb_cpu = -1;	/* current cpu running kdb	*/
int			kdb_debug = 0;
int			kdb_is_slave[NCPUS];
int			kdb_active[NCPUS];
volatile unsigned int	cpus_holding_bkpts;	/* counter for number of cpus holding
						   breakpoints (ie: cpus that did not
						   insert back breakpoints) */
extern boolean_t	db_breakpoints_inserted;

void
db_machdep_init(void)
{
	int c;

	db_simple_lock_init(&kdb_lock, ETAP_MISC_KDB);
	for (c = 0; c < NCPUS; ++c) {
		db_stacks[c] = (vm_offset_t) (db_stack_store +
			(INTSTACK_SIZE * (c + 1)) - sizeof (natural_t));
		if (c == master_cpu) {
#if 0
			dbtss.esp0 = (int)(db_task_stack_store +
				(INTSTACK_SIZE * (c + 1)) - sizeof (natural_t));
			dbtss.esp = dbtss.esp0;
			dbtss.eip = (int)&db_task_start;
			/*
			 * The TSS for the debugging task on each slave CPU
			 * is set up in mp_desc_init().
			 */
#endif
		}
	}
}

/*
 * Called when entering kdb:
 * Takes kdb lock. If if we were called remotely (slave state) we just
 * wait for kdb_cpu to be equal to cpu_number(). Otherwise enter kdb if
 * not active on another cpu.
 * If db_pass_thru[cpu_number()] > 0, then kdb can't stop now.
 */

int
kdb_enter(int pc)
{
	int my_cpu;
	int retval;
	
#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	my_cpu = cpu_number();

	if (db_pass_thru[my_cpu]) {
		retval = 0;
		goto kdb_exit;
	}

	kdb_active[my_cpu]++;
	lock_kdb();

	if (kdb_debug)
		db_printf("kdb_enter: cpu %d, is_slave %d, kdb_cpu %d, run mode %d pc %x (%x) holds %d\n",
			  my_cpu, kdb_is_slave[my_cpu], kdb_cpu,
			  db_run_mode, pc, *(int *)pc, cpus_holding_bkpts);
	if (db_breakpoints_inserted)
		cpus_holding_bkpts++;
	if (kdb_cpu == -1 && !kdb_is_slave[my_cpu]) {
		kdb_cpu = my_cpu;
		remote_kdb();	/* stop other cpus */
		retval = 1;
	} else if (kdb_cpu == my_cpu) 
		retval = 1;
	else
		retval = 0;

kdb_exit:
#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */

	return (retval);
}

void
kdb_leave(void)
{
	int my_cpu;
	boolean_t	wait = FALSE;

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	my_cpu = cpu_number();

	if (db_run_mode == STEP_CONTINUE) {
		wait = TRUE;
		kdb_cpu = -1;
	}
	if (db_breakpoints_inserted)
		cpus_holding_bkpts--;
	if (kdb_is_slave[my_cpu])
		kdb_is_slave[my_cpu]--;
	if (kdb_debug)
		db_printf("kdb_leave: cpu %d, kdb_cpu %d, run_mode %d pc %x (%x) holds %d\n",
			  my_cpu, kdb_cpu, db_run_mode,
			  ddb_regs.srr0, *(int *)ddb_regs.srr0,
			  cpus_holding_bkpts);
	clear_kdb_intr();
	unlock_kdb();
	kdb_active[my_cpu]--;

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */

	if (wait) {
		while(cpus_holding_bkpts);
	}
}

void
lock_kdb(void)
{
	int		my_cpu;
	register int	i;
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
		if (db_simple_lock_try(&kdb_lock)) {
			if (kdb_cpu == -1 || kdb_cpu == my_cpu)
				break;
			db_simple_unlock(&kdb_lock);
		}
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
	db_simple_unlock(&kdb_lock);
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
	db_set_breakpoints();
	db_set_watchpoints();
	kdb_cpu = cpu;
	unlock_kdb();
	lock_kdb();
	db_clear_breakpoints();
	db_clear_watchpoints();
	KDB_RESTORE_CTXT();
	if (kdb_cpu == -1)  {/* someone continued */
		kdb_cpu = cpu_number();
		db_continue_cmd(0, 0, 0, "");
	}
}

#endif	/* NCPUS > 1 */

void db_reboot(
	db_expr_t	addr,
	boolean_t	have_addr,
	db_expr_t	count,
	char		*modif)
{
	boolean_t	reboot = TRUE;
	char		*cp, c;
	
	cp = modif;
	while ((c = *cp++) != 0) {
		if (c == 'r')	/* reboot */
			reboot = TRUE;
		if (c == 'h')	/* halt */
			reboot = FALSE;
	}
	if (reboot) {
		db_printf("MACH Reboot\n");
		powermac_reboot();
	} else {
		db_printf("CPU halted\n");
		powermac_powerdown();
	}
	while (1);
}
