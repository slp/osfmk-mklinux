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
 *  (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: trap.c 1.49 94/12/14$
 */

#include <mach_rt.h>
#include <kgdb.h>

#include <kern/etap_macros.h>
#include <kern/thread.h>
#include <mach/thread_status.h>
#include <kern/exception.h>
#include <mach/vm_param.h>
#include <machine/trap.h>
#include <machine/psw.h>
#include <machine/regs.h>
#include <machine/break.h>
#include <mach_kdb.h>
#include <kern/spl.h>
#include <vm/vm_fault.h>
#include <machine/pdc.h>
#include <machine/iomod.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/c_support.h>
#include <hp_pa/fpu_protos.h>

#if KGDB
#include <kgdb_lan.h>
#include <kgdb/kgdb_stub.h>
#if KGDB_LAN
#include <machine/setjmp.h>
extern jmp_buf_t *kgdb_recover;
extern boolean_t kgdb_initialized;
#define kgdb_enabled kgdb_initialized
#else /* KGDB_LAN */
extern int kgdb_enabled;
#endif /* KGDB_LAN */
#endif /* KGDB */

#define IS_KLOADED(pc) ((vm_offset_t)pc > VM_MIN_KERNEL_LOADED_ADDRESS)

/*
 * XXX don't pass VM_PROT_EXECUTE to vm_fault(), execute permission is implied
 * in either R or RW (note: the pmap module knows this).  This is done for the
 * benefit of programs that execute out of their data space (ala lisp).
 * If we didn't do this in that scenerio, the ITLB miss code would call us
 * and we would call vm_fault() with RX permission.  However, the space was
 * probably vm_allocate()ed with just RW and vm_fault would fail.  The "right"
 * solution to me is to have the un*x server always allocate data with RWX for
 * compatibility with existing binaries.
 */
#define	PROT_EXEC	(VM_PROT_READ)
#define PROT_RO		(VM_PROT_READ)
#define PROT_RW		(VM_PROT_READ|VM_PROT_WRITE)

#define FAULT_TYPE(op) (OPCODE_IS_LOAD(op) ? PROT_RO : PROT_RW)

extern int align_index[];
extern int align_ldst[];
extern int align_coproc[];

/*
 * alignment restrictions for load and store instructions
 */

#define OPCODE_UNALIGNED(opcode, offset) \
	(((((opcode) & 0xd0000000) == 0x40000000) && \
	  (offset & align_ldst[opcode >> 26 & 0x03])) || \
	 ((((opcode) & 0xf4000200) == 0x24000200) && \
	  (offset & align_coproc[opcode >> 27 & 0x01])) || \
	 ((((opcode) & 0xfc000200) == 0x0c000200) && \
	  (offset & align_index[opcode >> 6 & 0x03])))

/*
 * interrupt switch table
 */
void
int_illegal(int int_bit)
{
	printf("Unexpected interrupt on EIRR bit %d\n",int_bit);
}
void
ignore_int(int int_bit)
{
}

struct intrtab itab_proc[] = {
	{ ignore_int,  INTPRI_00, 0 },	/* hard clock interrupt */
	{ int_illegal, INTPRI_01, 1},
	{ int_illegal, INTPRI_02, 2},
	{ int_illegal, INTPRI_03, 3},
	{ int_illegal, INTPRI_04, 4},
	{ int_illegal, INTPRI_05, 5},
	{ int_illegal, INTPRI_06, 6},
	{ int_illegal, INTPRI_07, 7},
	{ int_illegal, INTPRI_08, 8},
	{ int_illegal, INTPRI_09, 9},
	{ int_illegal, INTPRI_10, 10},
	{ int_illegal, INTPRI_11, 11},
	{ int_illegal, INTPRI_12, 12},
	{ int_illegal, INTPRI_13, 13},
	{ int_illegal, INTPRI_14, 14},
	{ int_illegal, INTPRI_15, 15},
	{ int_illegal, INTPRI_16, 16},
	{ int_illegal, INTPRI_17, 17},
	{ int_illegal, INTPRI_18, 18},
	{ int_illegal, INTPRI_19, 19},
	{ int_illegal, INTPRI_20, 20},
	{ int_illegal, INTPRI_21, 21},
	{ int_illegal, INTPRI_22, 22},
	{ int_illegal, INTPRI_23, 23},
	{ int_illegal, INTPRI_24, 24},
	{ int_illegal, INTPRI_25, 25},
	{ int_illegal, INTPRI_26, 26},
	{ int_illegal, INTPRI_27, 27},
	{ int_illegal, SPL0, 0},
	{ int_illegal, SPL0, 0},
	{ int_illegal, SPL0, 0},
	{ int_illegal, SPL0, 0}
};

/*
 * names of traps for panic calls
 */

static char *trap_type[] = {TRAP_NAMES};

#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

extern vm_map_t kernel_map;
extern void thread_syscall_return(unsigned);
extern void unresolved_kernel_trap(
				   int type,
				   struct hp700_saved_state *ssp,
				   char *message);

/*#define FTRACE_SPACE 6*/
#ifdef FTRACE_SPACE
space_t	ftspace = FTRACE_SPACE;
#endif /* FTRACE_SPACE */

#ifdef DEBUG
volatile int traptrace=0;

void dump_ssp(struct hp700_saved_state *ssp);
void dump_fp(struct hp700_float_state *ssp);
#endif /* DEBUG */

void
trap(
	int type,
	struct hp700_saved_state *ssp)
{
	int exception=0;
	int code;
	int subcode;
	vm_offset_t offset;
	space_t space;
	unsigned  opcode;
	thread_act_t		thr_act;
	register vm_map_t map;
	spl_t s;
	thread_t thread;
#if ETAP_EVENT_MONITOR
	etap_data_t probe_data;
	mach_port_t thread_exc_port, task_exc_port;

	thr_act = THR_ACT_NULL;
#endif	/* ETAP_EVENT_MONITOR */

#ifdef DEBUG
	if (!(type < TRAP_TYPES))
		panic("Unknown trap type 0x%x", type);

	/*
	 * mark the saved state structure with the trap we came in on
	 * for debugging...
	 */
	ssp->flags |= type << 16;
#endif /* DEBUG */
	if ((ssp->ipsw & PSW_Q) == 0) {
		dump_ssp(ssp);
		panic("Unexpected trap with PSW-Q = 0");
	}

	/*
	 * if the tail is in privileged mode make sure the head is too 
	 */

	if (!USERMODE_PC(ssp->iioq_tail))
		ssp->iioq_head &= ~PC_PRIV_USER;

	/*
 	 * Entered at spl7. We will lower the spl level to the level it was
 	 * when we took the trap with a minimum level of spl0.
	 */
	splx(ssp->eiem);

	/*
	 * handle kernel traps first.
	 */
	if (!USERMODE(ssp)) {
#ifdef DEBUG
		if(traptrace)
		{
			if(IS_KLOADED(ssp->iioq_head))
				printf("kloaded %s (%d) ", trap_type[type], type);
			else
				printf("kernel %s (%d) ", trap_type[type], type);
			
			printf("pc = 0x%x\n", ssp->iioq_head);
		}
#endif /* DEBUG */

		switch (type) {
			
			/*
			 * These trap types should never be seen by trap(). 
			 * Most are interrupts that should be seen by 
			 * interrupt() others just don't happen because they 
			 * are handled in locore. Some could happen but are
			 * considered to be fatal in kernel mode.
			 */
		case I_NONEXIST:      /* shouldn't happen */
		case I_HPMACH_CHK:    /* handled as interrupt */
		case I_POWER_FAIL:    /* handled as interrupt */
		case I_EXT_INTP:      /* handled as interrupt */
		case I_LPMACH_CHK:    /* handled as interrupt */
		case I_DPGFT_NA:      /* converted to DPGFT in locore */
		case I_IMEM_PROT:     /* fatal for kernel */
		case I_UNIMPL_INST:   /* fatal for kernel */
		case I_PRIV_OP:       /* fatal for kernel */
		case I_PRIV_REG:      /* fatal for kernel */
		case I_OVFLO:         /* fatal for kernel */
		case I_EXCEP:         /* fatal for kernel */
		case I_TLB_DIRTY:     /* handled in locore */
		case I_EMULAT:        /* fatal for kernel */
		case I_IS_OVFL:       /* fatal for kernel */
		case I_KS_OVFL:       /* fatal for kernel */
		case I_IS_LOST:       /* fatal for kernel */
		default:
			unresolved_kernel_trap(type, ssp, (char *)0);
			break;
			
		case I_LPRIV_XFR:	/* AST delivery from syscall */
			if(IS_KLOADED(ssp->iioq_head)) {
				s = splhigh();
				do {
					ast_taken(FALSE, AST_ALL, s);
					(void)splhigh();
				} while (ast_needed(cpu_number()));
				return;
			}
			panic(trap_type[type]);

		case I_RECOV_CTR:
			if(IS_KLOADED(ssp->iioq_head)) {
			       assert(current_act());

			       ssp->rctr = 0;
			       exception = EXC_BREAKPOINT;
			       code = EXC_HP700_BREAKPOINT;
			       subcode = ssp->iir;
			       break;
			 }

			printf("Kernel mode R-bit trap\n");
#ifdef DEBUG
			dump_ssp(ssp);
#endif /* DEBUG */
			ssp->ipsw &= ~PSW_R;
			break;

		case I_IPGFT:
		case I_IPGFT_NA:

			if (type == I_IPGFT_NA) {
				offset = (vm_offset_t) ssp->ior;
				space = (space_t) ssp->isr;
			} else {
				offset = (vm_offset_t) ssp->iioq_head;
				space = (space_t) ssp->iisq_head;
			}

			/*
			 * We don't want to run any user code as the kernel...
			 */
			if (space != 0)	{
			  	unresolved_kernel_trap(
					     type, ssp,
					     "Kernel instruction reference in user space");
				break;
			}

			code = vm_fault(kernel_map, trunc_page(offset),	PROT_EXEC, FALSE);
#ifdef FTRACE_SPACE
			if (space == ftspace)
				printf("type %d: vm_fault(%x, %x(%x), 5, 0) -> %d\n",
				       type+100, kernel_map, trunc_page(offset),
				       offset, code);
#endif /* FTRACE_SPACE */
			if (code == KERN_SUCCESS)
				break;

			unresolved_kernel_trap(
				type, ssp,
				"Kernel instruction page fault unresolved");
			break;

		case I_UNALIGN:
			dump_ssp(ssp);
			panic("Unaligned kernel data access\n");

		case I_DMEM_ACC:
		case I_DMEM_PID:

		case I_DMEM_PROT:
			thr_act = current_act();
			assert(thr_act);
			/*
			 * Unaligned kernel data access, die.
			 */
			if (OPCODE_UNALIGNED(ssp->iir, ssp->ior)) {
				printf("trap(%x): act %x, task %x\n",
		 		        type, thr_act, thr_act->task);
				unresolved_kernel_trap(type, ssp,
						       "Unaligned kernel data access");
				break;
			}
			/* else fall into... */

		case I_DPGFT:
			thr_act = current_act();
			assert(thr_act);
#if KGDB_LAN
			if (kgdb_recover) {
				printf("trap: data page/protection fault in debugger, kva=%x.%x\n", ssp->isr, ssp->ior);
				_longjmp(kgdb_recover, 1);
			}
#endif /* KGDB_LAN */
			offset = (vm_offset_t) ssp->ior;
			space = (space_t) ssp->isr;
			opcode = ssp->iir;

			/*
			 * Copyin/copyout operations will appear as kernel
			 * faults.  They are detected by comparing the
			 * faulting space with that of the current task.
			 */
			if (space == HP700_SID_KERNEL) {
				if (IS_KLOADED(ssp->iioq_head))
					map = thr_act->map;
				else
					map = kernel_map;
			}
			else {
				map = thr_act->map;
				if (space != map->pmap->space) {
					dump_ssp(ssp);
					panic("Illegal kernel space reference");
				}
			}

			/*
			 * Normal data page/protection fault, call vm_fault
			 * to deal with it.
			 */
			code = vm_fault(map, trunc_page(offset), FAULT_TYPE(opcode), FALSE);

#ifdef FTRACE_SPACE
			if (space == ftspace)
				printf("type %d: vm_fault(%x, %x(%x), %x, 0) -> %d\n",
				       type+100, map, trunc_page(offset), offset,
				       FAULT_TYPE(opcode), code);
#endif /* FTRACE_SPACE */
			if (code == KERN_SUCCESS)
				break;

			/*
			 * If this was a user access and there is a recovery
			 * pointer, the kernel was anticipating a problem with
			 * this user access.  Return to the recovery location.
			 */
			thread = thr_act->thread;
			assert(thread != THREAD_NULL);
			if (space != HP700_SID_KERNEL && thread->recover) {
				ssp->iioq_head = (int) thread->recover;
				ssp->iioq_tail = (int) thread->recover + 4;
				ssp->ret0 = (int) -1;
				break;
			}

			if (IS_KLOADED(ssp->iioq_head)) {
				exception = EXC_BAD_ACCESS;
				subcode = offset;
				break;				
			}

			/*
			 * An unresolved kernel fault, die.
			 */
			unresolved_kernel_trap(type, ssp, 0);
			break;

		case I_COND:
			dump_ssp(ssp);
			panic("Integer divide by zero in kernel");
			break;

		case I_BRK_INST:
			if (break5(ssp->iir) != BREAK_KERNEL)
				if(IS_KLOADED(ssp->iioq_head))
				{
					exception = EXC_BREAKPOINT;
					code = EXC_HP700_BREAKPOINT;
					subcode = ssp->iir;
					break;
				}
				else 
					panic("unknown break instruction in kernel");

			switch(break13(ssp->iir)) {

			case BREAK_KERNPRINT:
				kern_print(type, ssp);
				break;

#if KGDB
			case BREAK_KGDB:
				kgdb_trap(type, ssp);
				break;
#endif /* KGDB */
#if    MACH_KDB
			case BREAK_MACH_DEBUGGER:
				kdb_trap(ssp, 1);
				break;
#endif /* MACH_KDB */
			default:
				unresolved_kernel_trap(type, ssp, 0);
				break;
			}

			break;


		case I_TAKEN_BR:
#if KGDB
			if (kgdb_enabled) {
				kgdb_trap(type, ssp);
				break;
			}
			/* fall into... */
#endif /* KGDB */
		case I_DMEM_BREAK:
		case I_PAGE_REF:
		case I_HPRIV_XFR:
			unresolved_kernel_trap(type, ssp, "unexpected trap");
#ifdef DEBUG
			printf("unexpected trap %d, ignored\n", type);
			dump_ssp(ssp);
#endif /* DEBUG */
			break;
		}
	} else {
		thr_act = current_act();
		assert(thr_act);
		/*
		 * The thread was running in user mode when exception occured
		 */
#ifdef DEBUG
		if(traptrace)
		{
			printf("user trap %s(%d) ", trap_type[type], type);
			printf("pc = 0x%x\n", ssp->iioq_head);
		}
#endif /* DEBUG */

		switch (type) {

			/*
			 * These trap types should never be seen by trap(). 
			 * Most are interrupts that should be seen by 
			 * interrupt() others just don't happen because they 
			 * are handled in locore.
			 */

		case I_NONEXIST:	/* shouldn't happen */
		case I_HPMACH_CHK:	/* handled as interrupt */
		case I_POWER_FAIL:	/* handled as interrupt */
		case I_EXT_INTP:	/* handled as interrupt */
		case I_LPMACH_CHK:	/* handled as interrupt */
		case I_DPGFT_NA:	/* converted to DPGFT in locore */
		case I_IS_OVFL:		/* handled as interrupt */
		case I_KS_OVFL:		/* handled as interrupt */
		case I_IS_LOST:		/* handled as interrupt */

		default:
			printf("Illegal trap in user mode\n");
#ifdef DEBUG
			dump_ssp(ssp);
#endif /* DEBUG */
			if (type < TRAP_TYPES)
				panic(trap_type[type]);
			else
				panic("Unknown trap type 0x%x", type);
			break;
			
		case I_RECOV_CTR:	/* single step */
			ssp->rctr = 0;
			exception = EXC_BREAKPOINT;
			code = EXC_HP700_BREAKPOINT;
			subcode = ssp->iir;
			break;

		case I_LPRIV_XFR:	/* AST delivery from syscall */
			/*
			 * just need a hook, this will drop into AST delivery
			 * below
			 */
			if (IS_KLOADED(ssp->iioq_head) &&
			   ssp->iisq_head == 0 && ssp->iisq_tail == 0) {
				ssp->iioq_head &= ~PC_PRIV_USER;
				ssp->iioq_tail &= ~PC_PRIV_USER;
			}

			ssp->ipsw &= ~PSW_L;
			s = splhigh();
			do {
				ast_taken(FALSE, AST_ALL, s);
				(void)splhigh();
			} while (ast_needed(cpu_number()));
			return;

		case I_IPGFT:
		case I_IPGFT_NA:
			if (type == I_IPGFT_NA) {
				offset = (vm_offset_t) ssp->ior;
				space = (space_t) ssp->isr;
			} else {
				offset = (vm_offset_t) ssp->iioq_head;
				space = (space_t) ssp->iisq_head;
			}

			map = thr_act->map;
			assert(map->pmap->space != 0);
			if (space != map->pmap->space) 
				code = EXC_HP700_BADSPACE;
			else {
				
				/*
				 * normal instruction page fault
				 */
				
				code = vm_fault(map, trunc_page(offset), PROT_EXEC, FALSE);

#ifdef FTRACE_SPACE
				if (space == ftspace)
					printf("type %d: vm_fault(%x, %x(%x), 5, 0) -> %d\n",
					       type, map, trunc_page(offset), offset, code);
#endif /* FTRACE_SPACE */
				if (code == KERN_SUCCESS)
					break;
			}
			
			exception = EXC_BAD_ACCESS;
			subcode = offset;
			break;

		case I_IMEM_PROT:
			offset = (vm_offset_t) ssp->iioq_head;
			space = (space_t) ssp->iisq_head;
			
			map = thr_act->map;

			if (space != map->pmap->space) 
				code = EXC_HP700_BADSPACE;
			else {
				code = vm_fault(map, trunc_page(offset), PROT_EXEC, FALSE);

#ifdef FTRACE_SPACE
				if (space == ftspace)
					printf("type %d: vm_fault(%x, %x(%x), 5, 0) -> %d\n",
					       type, map, trunc_page(offset), offset, code);
#endif /* FTRACE_SPACE */
				if (code == KERN_SUCCESS)
					break;
			}
			
			exception = EXC_BAD_ACCESS;
			subcode = offset;
			break;

		case I_UNIMPL_INST:
			exception = EXC_BAD_INSTRUCTION;
			code = EXC_HP700_UNIPL_INST;
			subcode = ssp->iioq_head;
			break;

		case I_PRIV_OP:
			exception = EXC_BAD_INSTRUCTION;
			code = EXC_HP700_PRIVINST;
			subcode = ssp->iioq_head;
			break;
		
		case I_PRIV_REG:
			exception = EXC_BAD_INSTRUCTION;
			code = EXC_HP700_PRIVREG;
			subcode = ssp->iioq_head;
			break;

		case I_OVFLO:
			exception = EXC_ARITHMETIC;
			code = EXC_HP700_OVERFLOW;
			subcode = ssp->iioq_head;
			fpu_flush();
			break;
			
		case I_EXCEP:
			exception = EXC_ARITHMETIC;
			code = EXC_HP700_FLT_INEXACT;
			subcode = ssp->iioq_head;
			fpu_flush();

			/*
			 * XXX - fill in the right type of coprocessor 
			 * exception in code (so that UNIX can set them all to 
			 * floating point exception :-) )
			 */
			break;

		case I_COND:
			exception = EXC_ARITHMETIC;
			code = EXC_HP700_ZERO_DIVIDE;
			subcode = ssp->iioq_head;
			fpu_flush();
			break;

		case I_DMEM_ACC:
		case I_DMEM_PID:
		case I_UNALIGN:
			/*
			 * These traps replace I_DMEM_PROT (T-chip only).
			 */
			/* fall through */

		case I_DMEM_PROT:
#ifdef HPOSFCOMPAT
			if (handle_alignment_fault(ssp))
				break;
#ifdef DEBUG
			if (OPCODE_UNALIGNED(ssp->iir, ssp->ior))
				dump_ssp(ssp);
#endif /* DEBUG */
#endif /* HPOSFCOMPAT */
			space = (space_t) ssp->isr;
			opcode = ssp->iir;
			if (OPCODE_UNALIGNED(ssp->iir, ssp->ior)) {
				code = EXC_HP700_UNALIGNED;
				exception = EXC_BAD_ACCESS;
				subcode = (int) ssp->ior;
				break;
			}

			/* else fall into... */

		case I_DPGFT:
			offset = (vm_offset_t) ssp->ior;
			space = (space_t) ssp->isr;
			opcode = ssp->iir;

			map = thr_act->map;
			assert(map != 0);
			if (space != map->pmap->space)
				code = EXC_HP700_BADSPACE;
			else {
				/*
				 * Normal data page/protection fault, 
				 * call vm_fault to deal with it.
				 */
				code = vm_fault(map, trunc_page(offset),FAULT_TYPE(opcode), FALSE);

#ifdef FTRACE_SPACE
				if (space == ftspace)
					printf("type %d: vm_fault(%x, %x(%x), %x, 0) -> %d\n",
					       type, map, trunc_page(offset), offset,
					       FAULT_TYPE(opcode), code);
#endif /* FTRACE_SPACE */
				if (code == KERN_SUCCESS)
					break;
			}

			exception = EXC_BAD_ACCESS;
			subcode = offset;
			break;

		case I_EMULAT:
			exception = EXC_ARITHMETIC;
			code = EXC_HP700_NOEMULATION;
			subcode = ssp->iioq_head;
			fpu_flush();
			break;

		case I_TLB_DIRTY:
			panic("TLB dirty trap");
			break;

		case I_BRK_INST:

			/*
			 * if this isn't a kernel break then pass it up to 
			 * the server
			 */

			if (break5(ssp->iir) != BREAK_KERNEL) {
				exception = EXC_BREAKPOINT;
				code = EXC_HP700_BREAKPOINT;
				subcode = ssp->iir;
				break;
			}

			switch(break13(ssp->iir)) {

			case BREAK_KERNPRINT:
				kern_print(type, ssp);
				break;

#if KGDB
			case BREAK_KGDB:
				if (kgdb_enabled)
					kgdb_trap(type, ssp);
				else {
					exception = EXC_BREAKPOINT;
					code = EXC_HP700_BREAKPOINT;
					subcode = ssp->iir;
				}
				break;
#endif /* KGDB */
			case BREAK_PDC_IODC_CALL:
				/*
				 * hook for mach kdb
				 *
				 * fall through for now...
				 */
#ifdef DEBUG
				dump_ssp(ssp);
#endif /* DEBUG */
				ssp->iioq_head = ssp->iioq_tail;
				ssp->iioq_tail = ssp->iioq_head + 4;
				break;

#if    MACH_KDB
			case BREAK_MACH_DEBUGGER:
				kdb_trap(ssp, 1);
				break;
#endif /* MACH_KDB */

			default:
				exception = EXC_BREAKPOINT;
				code = EXC_HP700_BREAKPOINT;
				subcode = ssp->iir;
				break;
			}

			break;

		case I_DMEM_BREAK:
		case I_PAGE_REF:
		case I_HPRIV_XFR:
#ifdef DEBUG
			printf("unexpected trap %d, ignored\n", type);
			dump_ssp(ssp);
#endif /* DEBUG */
			break;

		case I_TAKEN_BR:
			exception = EXC_BREAKPOINT;
			code = EXC_HP700_TAKENBRANCH;
			subcode = ssp->iir;
			break;
		}
	}

#if ETAP_EVENT_MONITOR
	if (thr_act != THR_ACT_NULL && thr_act->thread != THREAD_NULL) {
		ETAP_DATA_LOAD(probe_data[0], type);
		ETAP_DATA_LOAD(probe_data[1],
			       thr_act->exc_actions[exception].port);
		ETAP_DATA_LOAD(probe_data[2],
			       thr_act->task->exc_actions[exception].port);
		ETAP_PROBE_DATA(ETAP_P_EXCEPTION,
				0,
				thr_act->thread,
				&probe_data,
				ETAP_DATA_ENTRY*3);
	}
#endif	/* ETAP_EVENT_MONITOR */

	if (exception)
		doexception(exception, code, subcode);

	/*
	 * ast delivery 
	 * Check if we need an AST and if so then we'll take care of it here...
	 */
	s = splhigh();
	while(ast_needed(cpu_number()))
		if ((USERMODE(ssp)) || IS_KLOADED(ssp->iioq_head)) {
			ast_taken(FALSE, AST_ALL, s);
			(void)splhigh();
#if MACH_RT
		} else if ((need_ast[0] & AST_URGENT) &&
			 (get_preemption_level() == 0)) {
			kernel_ast(current_thread(),
				   s, (vm_offset_t)ssp->iioq_head);
#endif /* MACH_RT */
	        } else
			break;
}

/*
 * interrupt handler 
 */

void
interrupt(int type, struct hp700_saved_state *ssp)
{
	if ((ssp->ipsw & PSW_Q) == 0) {
		printf("\ninterrupt(type = %x)\n", type);
		printf("eiem = %x, eirr = %x\n", 
		       mfctl(CR_EIEM), mfctl(CR_EIRR));
		panic("Unexpected interrupt with PSW-Q = 0");
	}

#ifdef DEBUG
	/*
	 * mark the saved state structure with the interrupt we came in on
	 * for debugging...
	 */
	ssp->flags |= type << 16;
#endif /* DEBUG */

	switch (type) {
			
		/*
		 * These trap types should never be seen by interrupt(). 
		 * Most are traps that should be seen by trap(). Others just 
		 * don't happen because they are handled in locore. Some 
		 * could happen but are considered to be fatal in kernel mode.
		 */

	case I_DPGFT:         /* fatal on interrupt stack */
	case I_DMEM_PROT:     /* fatal on interrupt stack */
	case I_DMEM_ACC:      /* fatal on interrupt stack */
	case I_DMEM_PID:      /* fatal on interrupt stack */
	case I_UNALIGN:       /* fatal on interrupt stack */
#if KGDB_LAN
		if (kgdb_recover) {
			printf("interrupt: data page/protection fault in debugger, kva=%x.%x\n", ssp->isr, ssp->ior);
			_longjmp(kgdb_recover, 1);
		}
		/* fall into... */
#endif /* KGDB_LAN */
	case I_NONEXIST:      /* shouldn't happen */
	case I_HPMACH_CHK:    /* fatal */
	case I_POWER_FAIL:    /* fatal */
	case I_LPMACH_CHK:    /* fatal */
	case I_DPGFT_NA:      /* converted to DPGFT in locore */
	case I_IMEM_PROT:     /* fatal on interrupt stack */
	case I_UNIMPL_INST:   /* fatal on interrupt stack */
	case I_PRIV_OP:       /* fatal on interrupt stack */
	case I_PRIV_REG:      /* fatal on interrupt stack */
	case I_OVFLO:         /* fatal on interrupt stack */
	case I_COND:          /* handled in locore */
	case I_EXCEP:         /* fatal on interrupt stack */
	case I_TLB_DIRTY:     /* handled in locore */
	case I_EMULAT:        /* fatal on interrupt stack */
	case I_IPGFT:	      /* fatal on interrupt stack */
	case I_IPGFT_NA:      /* fatal on interrupt stack */
	case I_IS_OVFL:       /* fatal on interrupt stack */
	case I_KS_OVFL:       /* fatal on interrupt stack */
	case I_IS_LOST:       /* fatal on interrupt stack */
	case I_LPRIV_XFR:     /* shouldn't happen */
	case I_RECOV_CTR:     /* shouldn't happen */
	default:
		printf("Illegal trap in kernel mode\n");
		printf("interrupt(type = %x)\n", type);

		dump_ssp(ssp);

		if (type < TRAP_TYPES)
			panic(trap_type[type]);
		else
			panic("Unknown interrupt type");
		break;
		

	case I_EXT_INTP:
	{
		int ibit;
		spl_t s;

		/*
		 * keep looping as long as the interrupt level we came in on
		 * allows us to service a pending interrupt
		 */
		while((ibit = ffset(mfctl(CR_EIRR) & ssp->eiem)) >= 0) {
			/*
			 * Clear this interrupt pending bit in EIRR.
			 */
			mtctl(CR_EIRR, 1 << 31 - ibit);
			assert(itab_proc[ibit].intpri <= ssp->eiem);
			splx(itab_proc[ibit].intpri);
				
			ETAP_PROBE_DATA(ETAP_P_INTERRUPT, EVENT_BEGIN,
					current_thread(), &ibit, sizeof ibit);

			if(ibit == SPL_CLOCK_BIT) 
				(*itab_proc[ibit].handler)((int)ssp);				
			else
				(*itab_proc[ibit].handler)(itab_proc[ibit].arg0);

			/*
			 * We must notify the ASP/LASI/WAX that we have dealt
			 * with this interrupt so it can unmask it again.
			 *
			 * XXX right now, any post-handler routine is recorded
			 * in the otherwise unused arg1 field.
			 */
			if (ibit && itab_proc[ibit].arg1)
				(*((void (*)(int))itab_proc[ibit].arg1))(ibit);

			ETAP_PROBE_DATA(ETAP_P_INTERRUPT, EVENT_END,
					current_thread(), (ibit = 0, &ibit),
					sizeof ibit);

			(void) splhigh();
		}
		break;
	}

	case I_BRK_INST:
		if (break5(ssp->iir) != BREAK_KERNEL) {
			panic("unknown break instruction in kernel");
		}

		switch(break13(ssp->iir)) {

		case BREAK_KERNPRINT:
			kern_print(type, ssp);
			break;

#if KGDB
		case BREAK_KGDB:
			kgdb_trap(type, ssp);
			break;
#endif /* KGDB */

#if    MACH_KDB
		case BREAK_MACH_DEBUGGER:
			kdb_trap(ssp, 1);
			break;
#endif /* MACH_KDB */

		default:
			dump_ssp(ssp);
			panic("unknown break instruction in kernel");
			break;
		}
		break;

	case I_DMEM_BREAK:
	case I_PAGE_REF:
	case I_HPRIV_XFR:
	case I_TAKEN_BR:
#if KGDB
		if (kgdb_enabled) {
			kgdb_trap(type, ssp);
			break;
		}
		/* fall into... */
#endif /* KGDB */
#ifdef DEBUG
		printf("unexpected trap %d, ignored\n", type);
		dump_ssp(ssp);
#endif /* DEBUG */
		break;
	}
}

/*
 * called from syscall if there is an error
 */

int
syscall_error(
	int exception,
	int code,
	int subcode,
	struct hp700_saved_state *ssp)
{
	register thread_t thread;

	thread = current_thread();

	if (thread == 0)
	    panic("syscall error in boot phase");

	if (!USERMODE(ssp))
		panic("system call called from kernel");

	doexception(exception, code, subcode);

	return 0;
}

void
doexception(
	    int exc,
	    int code,
	    int sub)
{
	exception_data_type_t   codes[EXCEPTION_CODE_MAX];

	codes[0] = code;	
	codes[1] = sub;
	exception(exc, codes, 2);
}

#if 0
void
set_interrupt(int level)
{
	/*
	 * XXX - this assumes that all soft interrupts occure through the
	 * monarch processor.
	 */
	PAGE0->mem_hpa->io_eir = level;
}
#endif

/*
 * This is the default routine used if interrupts are not configured
 * for a specific eirr bit. Going to this routine means that we got
 * an eirr on a bit that has no IO device.
 */

/*
 * interrupt service routine for power failure
 */
void
powerfail(int int_bit)
{
        panic("remote power failure");
}

void
unresolved_kernel_trap(
	int type,
	struct hp700_saved_state *ssp,
	char *message)
{

	if (!message) {
		if ((unsigned)type < TRAP_TYPES)
			message = trap_type[type];
		else
			message = "Unknown trap type";
	}
#if KGDB || MACH_KDB
	if (IS_KLOADED(ssp->iioq_head)) {
		int exception;

		printf("unresolved trap %d in kernel loaded task 0x%x\n",
		       type, current_task());
		printf("%s: sending exception message\n", message);
#if KGDB
		switch(type) {
		case I_IPGFT:
		case I_IPGFT_NA:
		case I_IMEM_PROT:
		case I_DMEM_PROT:
		case I_DMEM_ACC:     
		case I_DMEM_PID:      
		case I_UNALIGN:       
		case I_DPGFT:
			exception = EXC_BAD_ACCESS;
			break;
		case I_UNIMPL_INST:
		case I_PRIV_OP:
		case I_PRIV_REG:
			exception = EXC_BAD_INSTRUCTION;
			break;
		case I_OVFLO:
		case I_EXCEP:
		case I_COND:
		case I_EMULAT:
			exception = EXC_ARITHMETIC;
			break;
		case I_RECOV_CTR:
		case I_BRK_INST:
		case I_TAKEN_BR:
		case BREAK_KGDB:
		default:
			exception = EXC_BREAKPOINT;
			break;
		}
#endif /* KGDB */
#if MACH_KDB
		exception = EXC_BREAKPOINT;
#endif /* MACH_KDB */
		doexception(exception, 0, 0);
		return;
	}
#endif /* KGDB || MACH_KDB */

	dump_ssp(ssp);

	printf("unresolved kernel trap type %d\n", type);
	panic(message);
}

void
dump_ssp(struct hp700_saved_state *ssp)
{
	printf("saved state @ %8.8x:\n", ssp);

	printf("flg %8.8x	r16  %8.8x	sr0   %8.8x	iisqh %8.8x\n",
	       ssp->flags, ssp->r16, ssp->sr0, ssp->iisq_head);
	printf("r1  %8.8x	r17  %8.8x	sr1   %8.8x	iisqt %8.8x\n",
	       ssp->r1, ssp->r17, ssp->sr1, ssp->iisq_tail);
	printf("rp  %8.8x	r18  %8.8x	sr2   %8.8x	iioqh %8.8x\n",
	       ssp->rp, ssp->r18, ssp->sr2, ssp->iioq_head);
	printf("r3  %8.8x	t4   %8.8x	sr3   %8.8x	iioqt %8.8x\n",
	       ssp->r3, ssp->t4, ssp->sr3, ssp->iioq_tail);
	printf("r4  %8.8x	t3   %8.8x	sr4   %8.8x	iir   %8.8x\n",
	       ssp->r4, ssp->t3, ssp->sr4, ssp->iir);
	printf("r5  %8.8x	t2   %8.8x	sr5   %8.8x	isr   %8.8x\n",
	       ssp->r5, ssp->t2, ssp->sr5, ssp->isr);
	printf("r6  %8.8x	t1   %8.8x	sr6   %8.8x	ior   %8.8x\n",
	       ssp->r6, ssp->t1, ssp->sr6, ssp->ior);
	printf("r7  %8.8x	arg3 %8.8x	sr7   %8.8x	ipsw  %8.8x\n",
	       ssp->r7, ssp->arg3, ssp->sr7, ssp->ipsw);
	printf("r8  %8.8x	arg2 %8.8x\n",
	       ssp->r8, ssp->arg2);
	printf("r9  %8.8x	arg1 %8.8x	rctr %8.8x\n",
	       ssp->r9, ssp->arg1, ssp->rctr);
	printf("r10 %8.8x	arg0 %8.8x	pidr1 %8.8x\n",
	       ssp->r10, ssp->arg0, ssp->pidr1);
	printf("r11 %8.8x	dp   %8.8x	pidr2 %8.8x\n",
	       ssp->r11, ssp->dp, ssp->pidr2);
	printf("r12 %8.8x	ret0 %8.8x	sar   %8.8x\n",
	       ssp->r12, ssp->ret0, ssp->sar);
	printf("r13 %8.8x	ret1 %8.8x	pidr3 %8.8x\n",
	       ssp->r13, ssp->ret1, ssp->pidr3);
	printf("r14 %8.8x	sp   %8.8x	pidr4 %8.8x\n",
	       ssp->r14, ssp->sp, ssp->pidr4);
	printf("r15 %8.8x	r31  %8.8x	eiem  %8.8x\n",
	       ssp->r15, ssp->r31, ssp->eiem);
}

#ifdef DEBUG
void
dump_fp(struct hp700_float_state *ssp)
{
	printf("fr0  %x %x\n", *((int *)&ssp->fr0),  *((int *)&ssp->fr0 + 1));
	printf("fr1  %x %x\n", *((int *)&ssp->fr1),  *((int *)&ssp->fr1 + 1));
	printf("fr2  %x %x\n", *((int *)&ssp->fr2),  *((int *)&ssp->fr2 + 1));
	printf("fr3  %x %x\n", *((int *)&ssp->fr3),  *((int *)&ssp->fr3 + 1));
	printf("fr4  %x %x\n", *((int *)&ssp->farg0),  *((int *)&ssp->farg0 + 1));
	printf("fr5  %x %x\n", *((int *)&ssp->farg1),  *((int *)&ssp->farg1 + 1));
	printf("fr6  %x %x\n", *((int *)&ssp->farg2),  *((int *)&ssp->farg2 + 1));
	printf("fr7  %x %x\n", *((int *)&ssp->farg3),  *((int *)&ssp->farg3 + 1));
	printf("fr8  %x %x\n", *((int *)&ssp->fr8),  *((int *)&ssp->fr8 + 1));
	printf("fr9  %x %x\n", *((int *)&ssp->fr9),  *((int *)&ssp->fr9 + 1));
	printf("fr10 %x %x\n", *((int *)&ssp->fr10), *((int *)&ssp->fr10 + 1));
	printf("fr11 %x %x\n", *((int *)&ssp->fr11), *((int *)&ssp->fr11 + 1));
	printf("fr12 %x %x\n", *((int *)&ssp->fr12), *((int *)&ssp->fr12 + 1));
	printf("fr13 %x %x\n", *((int *)&ssp->fr13), *((int *)&ssp->fr13 + 1));
	printf("fr14 %x %x\n", *((int *)&ssp->fr14), *((int *)&ssp->fr14 + 1));
	printf("fr15 %x %x\n", *((int *)&ssp->fr15), *((int *)&ssp->fr15 + 1));
}
#endif /* DEBUG */

#if MACH_RT

#ifdef DEBUG
#define PREEMPT_INDEX_MAX	100
vm_offset_t preempt_buf[NCPUS][PREEMPT_INDEX_MAX];
unsigned preempt_index[NCPUS];
unsigned preempt_wrapping[NCPUS];
#endif /* DEBUG */

volatile boolean_t ast_urgent[NCPUS];

void
kernel_ast(thread_t self, spl_t s, vm_offset_t pc)
{
	int	mycpu = cpu_number();

	assert(cpu_data[mycpu].preemption_level == 0);
	assert(self);
	assert(ast_needed(mycpu) & AST_URGENT);

	/*
	 * If we have been already preempted, then register
	 * the new ast to be handled in next loop.
	 */
	switch(self->preempt) {
	case TH_PREEMPTED:
	    ast_urgent[mycpu] = TRUE;
	    /* FALL THRU */

	case TH_NOT_PREEMPTABLE:
	    ast_off(mycpu, AST_URGENT);
	    break;

	default:
	    /*
	     * We don't want to process any AST if we were in
	     * kernel-mode and the current thread is in any
	     * funny state (waiting, suspended and/or halted).
	     */
	    thread_lock(self);
	    if (thread_not_preemptable(self)) {
		ast_off(mycpu, AST_URGENT);
		thread_unlock(self);
		break;
	    }

	    /*
	     * TH_NOT_PREEMPTABLE *MUST* be set before unlocking
	     * thread to avoid possible recursion in kernel_ast.
	     */
	    self->preempt = TH_NOT_PREEMPTABLE;
	    thread_unlock(self);
#if DEBUG
	    if (++preempt_index[mycpu] == PREEMPT_INDEX_MAX) {
		preempt_index[mycpu] = 0;
		preempt_wrapping[mycpu]++;
	    }
	    preempt_buf[mycpu][preempt_index[mycpu]] = pc;
#endif /* DEBUG */

	    ast_taken(TRUE, AST_PREEMPT, s);

	    /*
	     * Reinstalled masked AST that might have been set
	     * before splhigh()
	     */
	    (void) splhigh();
	    mycpu = cpu_number();
	    if (ast_urgent[mycpu]) {
		ast_urgent[mycpu] = FALSE;
		ast_on(mycpu, AST_URGENT);
	    }
	    self->preempt = TH_PREEMPTABLE;
	    break;
	}
}

void
kernel_preempt_check(
    vm_offset_t pc)
{
    spl_t s = splhigh();
    while (ast_needed(cpu_number()) & AST_URGENT)
	kernel_ast(current_thread(), s, pc);
    splx(s);
}
#endif /* MACH_RT */

spl_t
hp_pa_astcheck(void)
{
	spl_t s = splsched();

	while(ast_needed(cpu_number())) {
		ast_taken(FALSE, AST_ALL, s);
		(void) splsched();
	}

	/* return interrupts disabled */
	return s;
}



