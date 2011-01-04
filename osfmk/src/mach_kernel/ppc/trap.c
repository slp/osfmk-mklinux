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

#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <debug.h>
#include <cpus.h>
#include <kern/thread.h>
#include <kern/exception.h>
#include <kern/syscall_sw.h>
#include <kern/cpu_data.h>
#include <mach/thread_status.h>
#include <vm/vm_fault.h>
#include <vm/vm_kern.h> 	/* For kernel_map */
#include <kgdb/kgdb_defs.h>	/* for kgdb_printf */
#include <kgdb/gdb_defs.h>	/* For sigtrap */
#include <ppc/misc_protos.h>
#include <ppc/trap.h>
#include <ppc/exception.h>
#include <ppc/proc_reg.h>	/* for SR_xxx definitions */
#include <ppc/pmap.h>
#include <ppc/fpu_protos.h>
#include <ppc/POWERMAC/powermac.h>

#if	MACH_KDB
#include <ddb/db_watch.h>
#include <ddb/db_run.h>
#include <ddb/db_break.h>
#include <ddb/db_trap.h>

boolean_t let_ddb_vm_fault = FALSE;
boolean_t	debug_all_traps_with_kdb = FALSE;
extern struct db_watchpoint *db_watchpoint_list;
extern boolean_t db_watchpoints_inserted;
extern boolean_t db_breakpoints_inserted;

#if	NCPUS > 1
extern int kdb_active[NCPUS];
#endif	/* NCPUS > 1 */

#endif	/* MACH_KDB */

#if DEBUG
#define TRAP_ALL 		-1
#define TRAP_ALIGNMENT		0x01
#define TRAP_DATA		0x02
#define TRAP_INSTRUCTION 	0x04
#define TRAP_AST		0x08
#define TRAP_TRACE		0x10
#define TRAP_PROGRAM		0x20
#define TRAP_EXCEPTION		0x40
#define TRAP_UNRESOLVED		0x80
#define TRAP_SYSCALL		0x100	/* all syscalls */
#define TRAP_HW			0x200	/* all in hw_exception.s */
#define TRAP_MACH_SYSCALL	0x400
#define TRAP_SERVER_SYSCALL	0x800

int trapdebug=0; /* TRAP_SERVER_SYSCALL;*/

#define TRAP_DEBUG(LEVEL, A) {if ((trapdebug & LEVEL)==LEVEL){kgdb_printf A;}}
#else
#define TRAP_DEBUG(LEVEL, A)
#endif

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

#define IS_KLOADED(pc) ((vm_offset_t)pc > VM_MIN_KERNEL_LOADED_ADDRESS)

/* A useful macro to update the ppc_exception_state in the PCB
 * before calling doexception
 */
#define UPDATE_PPC_EXCEPTION_STATE {					      \
	thread_act_t thr_act = current_act();				      \
	struct ppc_exception_state *es = &thr_act->mact.pcb->es;	      \
	es->dar = dar;							      \
	es->dsisr = dsisr;						      \
	es->exception = trapno / T_VECTOR_SIZE;	/* back to powerpc */ \
}

static void unresolved_kernel_trap(int trapno,
				   struct ppc_saved_state *ssp,
				   unsigned int dsisr,
				   unsigned int dar,
				   char *message);

struct ppc_saved_state *trap(int trapno,
			     struct ppc_saved_state *ssp,
			     unsigned int dsisr,
			     unsigned int dar)
{
	int exception=0;
	int code;
	int subcode;
	vm_map_t map;
        unsigned int sp;
	unsigned int space,space2;
	unsigned int offset;
	thread_act_t thr_act = current_act();

	TRAP_DEBUG(TRAP_ALL,("NMGS TRAP %d srr0=0x%08x, srr1=0x%08x\n",
		     trapno/4,ssp->srr0,ssp->srr1));

#if DEBUG
	{
		/* make sure we're not near to overflowing kernel stack */
		int sp;
		__asm__ volatile("mr	%0, 1" : "=r" (sp));
		if (sp < 
		    (cpu_data[cpu_number()].active_thread->kernel_stack +
		     sizeof(struct ppc_saved_state)+256)) {
			printf("TRAP - LOW ON KERNEL STACK!\n");
#if	MACH_KGDB
			call_kgdb_with_ctx(trapno, 0, ssp);
#else	/* MACH_KGDB */
#if	MACH_KDB
			kdb_trap(trapno, 0, ssp);
#endif	/* MACH_KDB */
#endif	/* MACH_KGDB */
		}			
	}
#endif /* DEBUG */
	/* Handle kernel traps first */

	if (!USER_MODE(ssp->srr1)) {
		/*
		 * Trap came from system task, ie kernel or collocated server
		 */
	      	switch (trapno) {
			/*
			 * These trap types should never be seen by trap()
			 * in kernel mode, anyway.
			 * Some are interrupts that should be seen by
			 * interrupt() others just don't happen because they
			 * are handled elsewhere. Some could happen but are
			 * considered to be fatal in kernel mode.
			 */
		case T_DECREMENTER:
		case T_IN_VAIN:								/* Shouldn't ever see this, lowmem_vectors eats it */
		case T_RESET:
		case T_MACHINE_CHECK:
		case T_SYSTEM_MANAGEMENT:
		case T_INTERRUPT:
		case T_FP_UNAVAILABLE:
		case T_IO_ERROR:
		case T_RESERVED:
		default:
			unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			break;
			
		case T_TRACE:
		case T_RUNMODE_TRACE:
		case T_INSTRUCTION_BKPT:
#if MACH_KGDB
			call_kgdb_with_ctx(trapno, 0, ssp);
#else
#if	MACH_KDB
			if (kdb_trap(trapno, 0, ssp))
			    break;
#endif	/* MACH_KDB */
			unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif /* MACH_KGDB */
			break;

		case T_PROGRAM:
			if (ssp->srr1 & MASK(SRR1_PRG_TRAP)) {
#if MACH_KGDB
				call_kgdb_with_ctx(trapno, 0, ssp);
#else
#if	MACH_KDB
				if (kdb_trap(trapno, 0, ssp))
				    break;
#endif	/* MACH_KDB */
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif /* MACH_KGDB */
			} else {
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			}
			break;
		case T_ALIGNMENT:
			TRAP_DEBUG(TRAP_ALIGNMENT,
				   ("NMGS KERNEL ALIGNMENT_ACCESS, "
				     "DAR=0x%08x, DSISR = 0x%B\n",
				     dar, dsisr,
			     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));
			if (alignment(dsisr, dar, ssp)) {
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			}
			break;
		case T_DATA_ACCESS:
			TRAP_DEBUG(TRAP_DATA,
				   ("NMGS KERNEL DATA_ACCESS, DAR=0x%08x,"
				     "DSISR = 0x%B\n", dar, dsisr,
			     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));

#if	MACH_KDB
			if (db_active
#if	NCPUS > 1
			    && kdb_active[cpu_number()]
#endif	/* NCPUS > 1 */
			    && !let_ddb_vm_fault) {
				/*
				 * Force kdb to handle this one.
				 */
				if (kdb_trap(trapno, 0, ssp))
				    break;
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			}
#endif	/* MACH_KDB */

#if	NCPUS > 1
			/* Other processor might have been updating a
			 * pinned or wired PTE, double check there really
			 * was a pte miss and that the pte exists and
			 * is valid when we have its lock
			 */
			if ((dsisr & MASK(DSISR_HASH)) &&
			    (dar <= VM_MAX_KERNEL_ADDRESS) &&
			    (dar >= VM_MIN_KERNEL_ADDRESS)) {
				if (kvtophys(dar) != 0) {
					/* return from exception silently */
					break;
				}
			}
#endif	/* NCPUS > 1 */

			/* simple case : not SR_COPYIN segment, from kernel */
			if ((dar >> 28) != SR_COPYIN) {
				/* if from kloaded task, use task mapping */
				if(IS_KLOADED(ssp->srr0))
					map = thr_act->map;
				else 
					map = kernel_map;

				offset = dar;
				TRAP_DEBUG(TRAP_DATA,
					   ("SYSTEM FAULT FROM 0x%08x\n",
					    offset));
				code = vm_fault(map,
						trunc_page(offset),
						dsisr & MASK(DSISR_WRITE) ?
							PROT_RW : PROT_RO,
						FALSE);
				if (code != KERN_SUCCESS) {
					unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
				}
				break;
			}

			/* If we get here, the fault was due to a copyin/out */

			map = thr_act->map;

			/* Mask out SR_COPYIN and mask in original segment */

			offset = (dar & 0x0fffffff) |
				((mfsrin(dar) & 0xF) << 28);

			TRAP_DEBUG(TRAP_DATA,
				   ("sr=0x%08x, dar=0x%08x, sp=0x%x\n",
				    mfsrin(dar), dar,map->pmap->space));
			assert(((mfsrin(dar) & 0x0FFFFFFF) >> 4) ==
			       map->pmap->space);
			TRAP_DEBUG(TRAP_DATA,
				   ("KERNEL FAULT FROM 0x%x,0x%08x\n",
				    map->pmap->space, offset));

			code = vm_fault(map,
					trunc_page(offset),
					dsisr & MASK(DSISR_WRITE) ?
					PROT_RW : PROT_RO,
					FALSE);

			/* If we failed, there should be a recovery
			 * spot to rfi to.
			 */
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_DATA,
					   ("FAULT FAILED- srr0=0x%08x,"
					     "pcb-srr0=0x%08x\n",
					     ssp->srr0,
					     thr_act->mact.pcb->ss.srr0));

				if (thr_act->thread->recover) {
					act_lock_thread(thr_act);
					ssp->srr0 = thr_act->thread->recover;
					thr_act->thread->recover =
							(vm_offset_t)NULL;
					act_unlock_thread(thr_act);
				} else {
					unresolved_kernel_trap(trapno, ssp, dsisr, dar, "copyin/out has no recovery point");
				}
			}
			break;
			
		case T_INSTRUCTION_ACCESS:
			/* Colocated tasks aren't necessarily wired and
			 * may page fault
			 */
			TRAP_DEBUG(TRAP_INSTRUCTION,
				   ("NMGS KERNEL INSTRUCTION ACCESS,"
				     "DSISR = 0x%B\n", dsisr,
			     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));

#if	MACH_KDB
			if (db_active
#if	NCPUS > 1
			    && kdb_active[cpu_number()]
#endif	/* NCPUS > 1 */
			    && !let_ddb_vm_fault) {
				/*
				 * Force kdb to handle this one.
				 */
				if (kdb_trap(trapno, 0, ssp))
				    break;
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			}
#endif	/* MACH_KDB */

			/* Make sure it's not the kernel itself faulting */
			assert ((ssp->srr0 >= VM_MIN_KERNEL_LOADED_ADDRESS) &&
				(ssp->srr0 <  VM_MAX_KERNEL_LOADED_ADDRESS));

			/* Same as for data access, except fault type
			 * is PROT_EXEC and addr comes from srr0
			 */
			map = thr_act->map;
			space = PPC_SID_KERNEL;
			
			code = vm_fault(map, trunc_page(ssp->srr0),
					PROT_EXEC, FALSE);
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_INSTRUCTION,
					   ("INSTR FAULT FAILED!\n"));
				exception = EXC_BAD_ACCESS;
				subcode = ssp->srr0;
			}
			break;

		case T_AST:
			TRAP_DEBUG(TRAP_AST,
				   ("T_AST FROM KERNEL MODE\n"));
			ast_taken(FALSE, AST_ALL, splsched());
			break;
		}
	} else {

		/*
		 * Trap came from user task
		 */

	      	switch (trapno) {
			/*
			 * These trap types should never be seen by trap()
			 * Some are interrupts that should be seen by
			 * interrupt() others just don't happen because they
			 * are handled elsewhere.
			 */
		case T_DECREMENTER:
		case T_IN_VAIN:								/* Shouldn't ever see this, lowmem_vectors eats it */
		case T_MACHINE_CHECK:
		case T_INTERRUPT:
		case T_FP_UNAVAILABLE:
		case T_SYSTEM_MANAGEMENT:
		case T_RESERVED:
		case T_IO_ERROR:
			
		default:
			printf("\n\nUnexpected trap type in user mode : trap %d PC=0x%08x SRR1=0x%08x state=0x%08x\n",trapno, ssp->srr0, ssp->srr1, ssp);
#if DEBUG
			regDump(ssp);
#endif
			
			panic("Unexpected exception type");
			break;

		case T_RESET:
			powermac_reboot(); 	     /* never returns */

		case T_ALIGNMENT:
			TRAP_DEBUG(TRAP_ALIGNMENT,
				   ("NMGS USER ALIGNMENT_ACCESS, "
				     "DAR=0x%08x, DSISR = 0x%B\n", dar, dsisr,
				     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));
			if (alignment(dsisr, dar, ssp)) {
				code = EXC_PPC_UNALIGNED;
				exception = EXC_BAD_ACCESS;
				subcode = dar;
			}
			break;

		case T_TRACE:			/* Real PPC chips */
		case T_INSTRUCTION_BKPT:	/* 603  PPC chips */
		case T_RUNMODE_TRACE:		/* 601  PPC chips */
			TRAP_DEBUG(TRAP_TRACE,("NMGS TRACE TRAP\n"));
#if	MACH_KDB
			{
				thread_act_t	act;
				task_t		task;

				act = current_act();
				if (act && act->kernel_loaded) {
					task = act->task;
				} else {
					task = TASK_NULL;
				}
				if (db_find_breakpoint_here(task,
							    ssp->srr0 - 4)) {
					if (kdb_trap(trapno, 0, ssp))
						break;
					unresolved_kernel_trap(trapno, ssp,
							       dsisr, dar,
							       NULL);
				}
			}
#endif	/* MACH_KDB */
					    
			exception = EXC_BREAKPOINT;
			code = EXC_PPC_TRACE;
			subcode = ssp->srr0;
			break;

		case T_PROGRAM:
			TRAP_DEBUG(TRAP_PROGRAM,
				   ("NMGS PROGRAM TRAP\n"));
			if (ssp->srr1 & MASK(SRR1_PRG_FE)) {
				TRAP_DEBUG(TRAP_PROGRAM,
					   ("FP EXCEPTION\n"));
				fpu_save();
				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_ARITHMETIC;
				code = EXC_ARITHMETIC;
				subcode = 
					((pcb_t)(per_proc_info[cpu_number()].
						 fpu_pcb))->fs.fpscr;
			} else if (ssp->srr1 & MASK(SRR1_PRG_ILL_INS)) {
				TRAP_DEBUG(TRAP_PROGRAM,
					   ("ILLEGAL INSTRUCTION\n"));

				UPDATE_PPC_EXCEPTION_STATE
				exception = EXC_BAD_INSTRUCTION;
				code = EXC_PPC_UNIPL_INST;
				subcode = ssp->srr0;
			} else if (ssp->srr1 & MASK(SRR1_PRG_PRV_INS)) {
				TRAP_DEBUG(TRAP_PROGRAM,
					   ("PRIVILEGED INSTRUCTION\n"));

				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BAD_INSTRUCTION;
				code = EXC_PPC_PRIVINST;
				subcode = ssp->srr0;
			} else if (ssp->srr1 & MASK(SRR1_PRG_TRAP)) {
#if MACH_KGDB
				/* Give kernel debugger a chance to
				 * claim the breakpoint before passing
				 * it up as an exception
				 */
				if (kgdb_user_breakpoint(ssp)) {
					call_kgdb_with_ctx(T_PROGRAM,
							   0,
							   ssp);
					break;
				}
#endif /* MACH_KGDB */
#if	MACH_KDB
				{
				thread_act_t	act;
				task_t		task;

				act = current_act();
				if (act && act->kernel_loaded) {
					task = act->task;
				} else {
					task = TASK_NULL;
				}
				if (db_find_breakpoint_here(task,
							    ssp->srr0 - 4)) {
					if (kdb_trap(trapno, 0, ssp))
						break;
					unresolved_kernel_trap(trapno, ssp,
							       dsisr, dar,
							       NULL);
				}
				}
#endif	/* MACH_KDB */

				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BREAKPOINT;
				code = EXC_PPC_BREAKPOINT;
				subcode = ssp->srr0;
			}
			break;

		case T_DATA_ACCESS:
			TRAP_DEBUG(TRAP_DATA,
				   ("NMGS USER DATA_ACCESS, DAR=0x%08x, DSISR = 0x%B\n", dar, dsisr,"\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));
			
			map = thr_act->map;
			space = map->pmap->space;
			assert(space != 0);
#if DEBUG
			mfsr(space2, SR_UNUSED_BY_KERN);
			space2 = (space2 >> 4) & 0x00FFFFFF;

			/* Do a check that at least SR_UNUSED_BY_KERN
			 * is in the correct user space.
			 * TODO NMGS may not be true as ints are on?.
			 */
			assert(space2 == space);
#if 0
			TRAP_DEBUG(TRAP_DATA,("map = 0x%08x, space=0x%08x, addr=0x%08x\n",map, space, trunc_page(dar));
#endif
#endif /* DEBUG */
			
			code = vm_fault(map, trunc_page(dar),
				 dsisr & MASK(DSISR_WRITE) ? PROT_RW : PROT_RO,
				 FALSE);
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_DATA,("FAULT FAILED!\n"));
				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BAD_ACCESS;
				subcode = dar;
			}
			break;
			
		case T_INSTRUCTION_ACCESS:
			TRAP_DEBUG(TRAP_INSTRUCTION,("NMGS USER INSTRUCTION ACCESS, DSISR = 0x%B\n", dsisr,"\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));

			/* Same as for data access, except fault type
			 * is PROT_EXEC and addr comes from srr0
			 */
			map = thr_act->map;
			space = map->pmap->space;
			assert(space != 0);
#if DEBUG
			mfsr(space2, SR_UNUSED_BY_KERN);
			space2 = (space2 >> 4) & 0x00FFFFFF;

			/* Do a check that at least SR_UNUSED_BY_KERN
			 * is in the correct user space.
			 * TODO NMGS is this always true now ints are on?
			 */
			assert(space2 == space);
#endif /* DEBUG */
			
			code = vm_fault(map, trunc_page(ssp->srr0),
					PROT_EXEC, FALSE);
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_INSTRUCTION,
					   ("INSTR FAULT FAILED!\n"));
				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BAD_ACCESS;
				subcode = ssp->srr0;
			}
			break;

		case T_AST:
			TRAP_DEBUG(TRAP_AST,("T_AST FROM USER MODE\n"));
			ast_taken(FALSE, AST_ALL, splsched());
			break;
			
		}
	}

	if (exception) {
		TRAP_DEBUG(TRAP_EXCEPTION,
			   ("doexception (0x%x, 0x%x, 0x%x\n",
			    exception,code,subcode));
		doexception(exception, code, subcode);
		return ssp;
	}
	/* AST delivery
	 * Check to see if we need an AST, if so take care of it here
	 */
	while (ast_needed(cpu_number()) && (USER_MODE(ssp->srr1) ||
					    IS_KLOADED(ssp->srr0))) {
		ast_taken(FALSE, AST_ALL, splsched());
	}
	return ssp;
}

#if MACH_ASSERT
/* This routine is called from assembly before each and every system call
 * iff MACH_ASSERT is defined. It must preserve r3.
 */

extern int syscall_trace(int, struct ppc_saved_state *);


extern int pmdebug;

int syscall_trace(int retval, struct ppc_saved_state *ssp)
{
	int i, argc;

	if (trapdebug & TRAP_SYSCALL)
		trapdebug |= (TRAP_MACH_SYSCALL|TRAP_SERVER_SYSCALL);

	if (!(trapdebug & (TRAP_MACH_SYSCALL|TRAP_SERVER_SYSCALL)))
		return retval;
	if (ssp->r0 & 0x80000000) {
		/* Mach trap */
		if (!(trapdebug & TRAP_MACH_SYSCALL))
			return retval;

		printf("0x%08x : %30s (",
		       ssp->srr0, mach_trap_table[-(ssp->r0)].mach_trap_name);
		argc = mach_trap_table[-(ssp->r0)].mach_trap_arg_count;
		for (i = 0; i < argc; i++)
			printf("%08x ",*(&ssp->r3 + i));
		printf(")\n");
	} else {
		if (!(trapdebug & TRAP_SERVER_SYSCALL))
			return retval;
		printf("0x%08x : UNIX %3d (", ssp->srr0, ssp->r0);
		argc = 4; /* print 4 of 'em! */
		for (i = 0; i < argc; i++)
			printf("%08x ",*(&ssp->r3 + i));
		printf(")\n");
	}
	return retval;
}
#endif /* MACH_ASSERT */

/*
 * called from syscall if there is an error
 */

int syscall_error(
	int exception,
	int code,
	int subcode,
	struct ppc_saved_state *ssp)
{
	register thread_t thread;

	thread = current_thread();

	if (thread == 0)
	    panic("syscall error in boot phase");

	if (!USER_MODE(ssp->srr1))
		panic("system call called from kernel");

	doexception(exception, code, subcode);

	return 0;
}

/* Pass up a server syscall/exception */
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

char *trap_type[] = {
	"0x000   Interrupt in vain",
	"0x100   System reset",
	"0x200   Machine check",
	"0x300   Data access",
	"0x400   Instruction access",
	"0x500   External interrupt",
	"0x600   Alignment",
	"0x700   Program",
	"0x800   Floating point",
	"0x900   Decrementer",
	"0xA00   I/O controller interface",
	"0xB00   INVALID EXCEPTION",
	"0xC00   System call exception",
	"0xD00   Trace",
	"0xE00   FP assist",
	"0xF00   INVALID EXCEPTION",
	"0x1000  Instruction PTE miss",
	"0x1100  Data load PTE miss",
	"0x1200  Data store PTE miss",
	"0x1300  Instruction Breakpoint",
	"0x1400  System Management",
	"0x1500  INVALID EXCEPTION",
	"0x1600  INVALID EXCEPTION",
	"0x1700  INVALID EXCEPTION",
	"0x1800  INVALID EXCEPTION",
	"0x1900  INVALID EXCEPTION",
	"0x1A00  INVALID EXCEPTION",
	"0x1B00  INVALID EXCEPTION",
	"0x1C00  INVALID EXCEPTION",
	"0x1D00  INVALID EXCEPTION",
	"0x1E00  INVALID EXCEPTION",
	"0x1F00  INVALID EXCEPTION",
	"0x2000  Run Mode/Trace"
};
int TRAP_TYPES = sizeof (trap_type) / sizeof (trap_type[0]);

void unresolved_kernel_trap(int trapno,
			    struct ppc_saved_state *ssp,
			    unsigned int dsisr,
			    unsigned int dar,
			    char *message)
{
	unsigned int* stackptr;
	char *trap_name;
	int i;

	if(IS_KLOADED(ssp->srr0)) {
		int exception;

		TRAP_DEBUG(TRAP_UNRESOLVED,
			   ("unresolved trap %d in kernel loaded task %d\n",trapno));
		TRAP_DEBUG(TRAP_UNRESOLVED,
			   ("%s: sending exception message\n", message));
		switch(trapno) {
		case T_IN_VAIN:								/* Shouldn't ever see this, lowmem_vectors eats it */
		case T_RESET:
		case T_MACHINE_CHECK:
		case T_INTERRUPT:
		case T_DECREMENTER:
		case T_IO_ERROR:
		case T_RESERVED:
		case T_SYSTEM_CALL:
		case T_RUNMODE_TRACE:
		default:
			exception = EXC_BREAKPOINT;
			break;
		case T_ALIGNMENT:
		case T_INSTRUCTION_ACCESS:
		case T_DATA_ACCESS:
			exception = EXC_BAD_ACCESS;
			break;
		case T_PROGRAM:
			exception = EXC_BAD_INSTRUCTION;
			break;
		case T_FP_UNAVAILABLE:
			/* TODO NMGS - should this generate EXC_ARITHMETIC?*/
			exception = EXC_ARITHMETIC;
			break;
		}
		UPDATE_PPC_EXCEPTION_STATE;
		doexception(exception, 0, 0);
		return;
	}
#if DEBUG && MACH_KGDB
	regDump(ssp);
#endif /* DEBUG && MACH_KGDB */

	if ((unsigned)trapno <= T_MAX)
		trap_name = trap_type[trapno / T_VECTOR_SIZE];
	else
		trap_name = "???? unrecognised exception";
	if (message == NULL)
		message = trap_name;

	/* no KGDB so display a backtrace */
	printf("\n\nUnresolved kernel trap: %s DSISR=0x%08x DAR=0x%08x PC=0x%08x\n"
	       "generating stack backtrace prior to panic:\n\n",
	       trap_name, dsisr, dar, ssp->srr0);

	printf("backtrace: %08x ", ssp->srr0);

	stackptr = (unsigned int *)(ssp->r1);

	for (i = 0; i < 8; i++) {

		/* Avoid causing page fault */
		if (pmap_extract(kernel_pmap, (vm_offset_t)stackptr) == 0)
			break;

		stackptr = (unsigned int*)*stackptr;
		/*
		 * TODO NMGS - use FM_LR_SAVE constant, but asm.h defines
		 * too much (such as r1, used above)
		 */

		/* Avoid causing page fault */
		if (pmap_extract(kernel_pmap, (vm_offset_t)stackptr+1) == 0)
			break;

		printf("%08x ",*(stackptr + 1));
	}
	printf("\n\n");

	panic(message);
}

#if	MACH_KDB
void
thread_kdb_return(void)
{
	register thread_act_t	thr_act = current_act();
	register thread_t	cur_thr = current_thread();
	register struct ppc_saved_state *regs = USER_REGS(thr_act);

	if (kdb_trap(thr_act->mact.pcb->es.exception, 0, regs)) {
#if		MACH_LDEBUG
		assert(cur_thr->mutex_count == 0); 
#endif		/* MACH_LDEBUG */
		check_simple_locks();
		thread_exception_return();
		/*NOTREACHED*/
	}
}
#endif	/* MACH_KDB */
