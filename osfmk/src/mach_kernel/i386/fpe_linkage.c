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
/* CMU_HIST */
/*
 * Revision 2.1.1.1.2.2  92/03/28  10:05:19  jeffreyh
 * 	Changes from MK71
 * 	[92/03/20  12:11:20  jeffreyh]
 * 
 * Revision 2.3  92/03/05  15:39:05  rpd
 * 	Fixed disable_fpe to clear fs & gs.
 * 	[92/03/05            rpd]
 * 
 * Revision 2.1.1.1.2.1  92/03/03  16:15:04  jeffreyh
 * 	Picked up from TRUNK
 * 	[92/02/26  11:07:31  jeffreyh]
 *
 * Revision 2.2  92/01/03  20:05:28  dbg
 * 	Changed trap gate for interrupt 7 to PL_Kernel.  The privilege level
 * 	affects INT $7 instructions, not coprocessor-not-present traps.
 * 	[91/12/19            dbg]
 * 
 * 	Created.
 * 	[91/10/19            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
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
 * Support routines for FP emulator.
 */

#include <fpe.h>

#include <cpus.h>

#include <mach/exception.h>
#include <mach/thread_status.h>

#include <kern/cpu_number.h>
#include <kern/thread.h>
#include <kern/exception.h>

#include <vm/vm_kern.h>

#include <i386/eflags.h>
#include <i386/proc_reg.h>
#include <i386/pmap.h>
#include <i386/seg.h>
#include <i386/thread.h>
#include <i386/fpu.h>
#include <i386/misc_protos.h>

#if	NCPUS > 1
#include <i386/mp_desc.h>
#endif

extern vm_offset_t	kvtophys(vm_offset_t addr);

/*
 * Symbols exported from FPE emulator.
 */
extern char	fpe_start[];	/* start of emulator text;
				   also emulation entry point */
extern char	fpe_end[];	/* end of emulator text */
extern int	fpe_reg_segment;
				/* word holding segment number for
				   FPE register/status area */
extern char	fpe_recover[];	/* emulation fault recovery entry point */

/*
 * Descriptor tables to be modified for FPE.
 */
extern struct fake_descriptor idt[];
extern struct fake_descriptor gdt[];

#if	NCPUS > 1
#define	curr_gdt(mycpu)		(mp_gdt[mycpu])
#define	curr_idt(mycpu)		(mp_desc_table[mycpu]->idt)
#else
#define	curr_gdt(mycpu)		(gdt)
#define	curr_idt(mycpu)		(idt)
#endif

#define	gdt_desc_p(mycpu,sel) \
	((struct real_descriptor *)&curr_gdt(mycpu)[sel_idx(sel)])
#define	idt_desc_p(mycpu,idx) \
	((struct real_gate *)&curr_idt(mycpu)[idx])

void	set_user_access(pmap_t pmap,
			vm_offset_t start,
			vm_offset_t end,
			boolean_t writable);	/* forward */

/*
 * long pointer for calling FPE register recovery routine.
 */
struct long_ptr {
	unsigned long	offset;
	unsigned short	segment;
};

struct long_ptr fpe_recover_ptr;

/*
 * Initialize descriptors for FP emulator.
 */
void
fpe_init(void)
{
	register struct real_descriptor *gdt_p;
	register struct real_gate *idt_p;

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	/*
	 * Map in the pages for the FP emulator:
	 * read-only, user-accessible.
	 */
	set_user_access(pmap_kernel(),
			(vm_offset_t)fpe_start,
			(vm_offset_t)fpe_end,
			FALSE);

	/*
	 * Put the USER_FPREGS segment value in the FP emulator.
	 */
	fpe_reg_segment = USER_FPREGS;

	/*
	 * Change exception 7 gate (coprocessor not present)
	 * to a trap gate to the FPE code segment.
	 */
	idt_p = idt_desc_p(cpu_number(), 7);
	idt_p->offset_low  = 0;			/* offset of FPE entry */
	idt_p->offset_high = 0;
	idt_p->selector	  = FPE_CS;		/* FPE code segment */
	idt_p->word_count = 0;
	idt_p->access 	  = ACC_P|ACC_PL_K|ACC_TRAP_GATE;
						/* trap gate */
						/* kernel privileges only,
						   so INT $7 does not call
						   the emulator */

	/*
	 * Build GDT entry for FP code segment.
	 */
	addr = (vm_offset_t) fpe_start;
	addr += LINEAR_KERNEL_ADDRESS;
	gdt_p = gdt_desc_p(cpu_number(), FPE_CS);
	gdt_p->base_low   = addr & 0xffff;
	gdt_p->base_med   = (addr >> 16) & 0xff;
	gdt_p->base_high  = addr >> 24;
	gdt_p->limit_low  = (vm_offset_t) fpe_end
			  - (vm_offset_t) fpe_start
			  - 1;
	gdt_p->limit_high = 0;
	gdt_p->granularity = SZ_32;
	gdt_p->access	  = ACC_P|ACC_PL_K|ACC_CODE_CR;
						/* conforming segment,
						   usable by kernel */

	/*
	 * Build GDT entry for user FP state area - template,
	 * since each thread has its own.
	 */
	gdt_p = gdt_desc_p(cpu_number(), USER_FPREGS);
	/* descriptor starts as 0 */
	gdt_p->limit_low  = sizeof(struct i386_fp_save)
			  + sizeof(struct i386_fp_regs)
			  - 1;
	gdt_p->limit_high = 0;
	gdt_p->granularity = 0;
	gdt_p->access = ACC_PL_U|ACC_DATA_W;
					/* start as "not present" */

	/*
	 * Set up the recovery routine pointer
	 */
	fpe_recover_ptr.offset = fpe_recover - fpe_start;
	fpe_recover_ptr.segment = FPE_CS;

	/*
	 * Set i386 to emulate coprocessor.
	 */
	set_cr0((get_cr0() & ~CR0_MP) | CR0_EM);

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */
}

/*
 * Enable FPE use for a new thread.
 * Allocates the FP save area.
 */
boolean_t
fp_emul_error(
	struct i386_saved_state *regs)
{
	register struct i386_fpsave_state *ifps;
	register vm_offset_t	start_va;

	if ((regs->err & 0xfffc) != (USER_FPREGS & ~SEL_PL))
	    return FALSE;

	/*
	 * Make the FPU save area user-accessible (by FPE)
	 */
	ifps = current_act()->mact.pcb->ims.ifps;
	if (ifps == 0) {
	    /*
	     * No FP register state yet - allocate it.
	     */
	    fp_state_alloc();
	    ifps = current_act()->mact.pcb->ims.ifps;
	}
	    
	start_va = (vm_offset_t) &ifps->fp_save_state;
	set_user_access(current_map()->pmap,
		start_va,
		start_va + sizeof(struct i386_fp_save),
		TRUE);

	/*
	 * Enable FPE use for this thread
	 */
	enable_fpe(ifps);

	return TRUE;
}

/*
 * Enable FPE use.  ASSUME that kernel does NOT use FPU
 * except to handle user exceptions.
 */
void
enable_fpe(
	struct i386_fpsave_state *ifps)
{
	struct real_descriptor *dp;
	vm_offset_t	start_va;

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	dp = gdt_desc_p(cpu_number(), USER_FPREGS);
	start_va = (vm_offset_t)&ifps->fp_save_state;
	start_va += LINEAR_KERNEL_ADDRESS;

	dp->base_low = start_va & 0xffff;
	dp->base_med = (start_va >> 16) & 0xff;
	dp->base_high = start_va >> 24;
	dp->access |= ACC_P;

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */
}

void
disable_fpe(void)
{
	/*
	 *	The kernel might be running with fs & gs segments
	 *	which refer to USER_FPREGS, if we entered the kernel
	 *	from a FP-using thread.  We have to clear these segments
	 *	lest we get a Segment Not Present trap.  This would happen
	 *	if the kernel took an interrupt or fault after clearing
	 *	the present bit but before exiting to user space (which
	 *	would reset fs & gs from the current user thread).
	 */

	__asm__ volatile("xorl %eax, %eax");
	__asm__ volatile("movw %ax, %fs");
#if	!MACH_RT
	__asm__ volatile("movw %ax, %gs");
#endif	/* !MACH_RT */

#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */

	gdt_desc_p(cpu_number(), USER_FPREGS)->access &= ~ACC_P;

#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */
}

void
set_user_access(
	pmap_t		pmap,
	vm_offset_t	start,
	vm_offset_t	end,
	boolean_t	writable)
{
	register vm_offset_t	va;
	register pt_entry_t *	dirbase = pmap->dirbase;
	register pt_entry_t *	ptep;
	register pt_entry_t *	pdep;

	start = i386_trunc_page(start);
	end   = i386_round_page(end);

	for (va = start; va < end; va += I386_PGBYTES) {

	    pdep = &dirbase[pdenum(pmap, va)];
	    *pdep |= INTEL_PTE_USER;
	    ptep = (pt_entry_t *)ptetokv(*pdep);
	    ptep = &ptep[ptenum(va)];
	    *ptep |= INTEL_PTE_USER;
	    if (!writable)
		*ptep &= ~INTEL_PTE_WRITE;
	}
}

/*
 * Route exception through emulator fixup routine if
 * it occurred within the emulator.
 */
void
fpe_exception_fixup(
	int	exc,
	int	code,
	int	subcode)
{
	thread_act_t	thr_act = current_act();
	pcb_t			pcb = thr_act->mact.pcb;
	exception_data_type_t   codes[EXCEPTION_CODE_MAX];


	if (pcb->iss.efl & EFL_VM) {
	    /*
	     * The emulator doesn`t handle V86 mode.
	     * If this is a GP fault on the emulator`s
	     * code segment, change it to an FP not present
	     * fault.
	     */
	    if (exc == EXC_BAD_INSTRUCTION
	     && code == EXC_I386_GPFLT
	     && subcode == FPE_CS + 1)
	    {
		exc = EXC_ARITHMETIC;	/* arithmetic error: */
		code = EXC_I386_NOEXT;	/* no FPU */
		subcode = 0;
	    }
	}
	else
	if ((pcb->iss.cs & 0xfffc) == FPE_CS) {
	    /*
	     * Pass registers to emulator,
	     * to let it fix them up.
	     * The emulator fixup routine knows about
	     * an i386_thread_state.
	     */
	    struct i386_thread_state	tstate;
	    unsigned int		count;

	    count = i386_THREAD_STATE_COUNT;
	    (void) thread_getstatus(thr_act,
				i386_REGS_SEGS_STATE,
				(thread_state_t) &tstate,
				&count);

	    /*
	     * long call to emulator register recovery routine
	     */
	    __asm__ volatile("pushl %0; lcall %1; addl $4,%%esp"
			     :
			     : "r" (&tstate),
			     "m" (*(char *)&fpe_recover_ptr) );

	    (void) thread_setstatus(thr_act,
				i386_REGS_SEGS_STATE,
				(thread_state_t) &tstate,
				count);
	    /*
	     * In addition, check for a GP fault on 'int 16' in
	     * the emulator, since the interrupt gate is protected.
	     * If so, change it to an arithmetic error.
	     */
	    if (exc == EXC_BAD_INSTRUCTION
	     && code == EXC_I386_GPFLT
	     && subcode == 8*16+2)	/* idt[16] */
	    {
		exc = EXC_ARITHMETIC;
		code = EXC_I386_EXTERR;
		subcode = pcb->ims.ifps->fp_save_state.fp_status;
	    }
	}
	codes[0] = code;	/* new exception interface */
	codes[1] = subcode;
	exception(exc, codes, 2);
}
