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
 * Revision 2.12.5.2  92/03/03  16:16:15  jeffreyh
 * 	Pick up changes from TRUNK
 * 	[92/02/26  11:28:14  jeffreyh]
 * 
 * Revision 2.13  92/01/03  20:08:34  dbg
 * 	Disable thread_set_state of ISA_PORT_MAP, but have it still
 * 	return KERN_SUCCESS (for DOS emulator compatibility).
 * 	[91/12/06            dbg]
 * 
 * 	Add user ldt management.  Move floating-point state
 * 	manipulation to i386/fpu.{c,h}.
 * 
 * 	Add user_stack_low and set_user_regs for passing control to
 * 	bootstrap in user space.
 * 	[91/10/30            dbg]
 * 
 * Revision 2.12.5.1  92/02/18  18:48:24  jeffreyh
 * 	Bind v86 threads on master processor for Corollary
 * 	[91/09/09            bernadat]
 * 
 * Revision 2.12  91/10/09  16:07:08  af
 * 	Set value of kernel_stack field in stack_handoff().
 * 	[91/08/29            tak]
 * 
 * Revision 2.11  91/07/31  17:39:56  dbg
 * 	Add thread_set_syscall_return.
 * 
 * 	Save user regs directly in PCB on trap, and switch to separate
 * 	kernel stack.
 * 
 * 	Add v8086 mode interrupt support.
 * 	[91/07/30  16:56:09  dbg]
 * 
 * Revision 2.10  91/05/14  16:13:06  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/05/08  12:40:34  dbg
 * 	Use iopb_tss_t for IO permission bitmap.
 * 	[91/03/21            dbg]
 * 
 * Revision 2.8  91/03/16  14:44:51  rpd
 * 	Pulled i386_fpsave_state out of i386_machine_state.
 * 	Added pcb_module_init.
 * 	[91/02/18            rpd]
 * 
 * 	Replaced stack_switch with stack_handoff and
 * 	switch_task_context with switch_context.
 * 	[91/02/18            rpd]
 * 	Added active_stacks.
 * 	[91/01/29            rpd]
 * 
 * Revision 2.7  91/02/05  17:13:19  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:36:24  mrt]
 * 
 * Revision 2.6  91/01/09  22:41:41  rpd
 * 	Revised the pcb yet again.
 * 	Picked up i386_ISA_PORT_MAP_STATE flavors.
 * 	Added load_context, switch_task_context cover functions.
 * 	[91/01/09            rpd]
 * 
 * Revision 2.5  91/01/08  15:10:58  rpd
 * 	Removed pcb_synch.  Added pcb_collect.
 * 	[91/01/03            rpd]
 * 
 * 	Split i386_machine_state off of i386_kernel_state.
 * 	Set k_stack_top correctly for V8086 threads.
 * 	[90/12/31            rpd]
 * 	Added stack_switch.  Moved stack_alloc_try, stack_alloc,
 * 	stack_free, stack_statistics to kern/thread.c.
 * 	[90/12/14            rpd]
 * 
 * 	Reorganized the pcb.
 * 	Added stack_attach, stack_alloc, stack_alloc_try,
 * 	stack_free, stack_statistics.
 * 	[90/12/11            rpd]
 * 
 * Revision 2.4  90/08/27  21:57:34  dbg
 * 	Return correct count from thread_getstatus.
 * 	[90/08/22            dbg]
 * 
 * Revision 2.3  90/08/07  14:24:47  rpd
 * 	Include seg.h for segment names.
 * 	[90/07/17            dbg]
 * 
 * Revision 2.2  90/05/03  15:35:51  dbg
 * 	Created.
 * 	[90/02/08            dbg]
 * 
 */
/* CMU_ENDHIST */
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

#include <cpus.h>
#include <mach_rt.h>
#include <mach_debug.h>
#include <mach_ldebug.h>

#include <mach/kern_return.h>
#include <mach/thread_status.h>
#include <mach/vm_param.h>
#include <mach/rpc.h>

#include <kern/counters.h>
#include <kern/mach_param.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/spl.h>
#include <ipc/ipc_port.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>

#include <device/device_typedefs.h>

#include <i386/thread.h>
#include <i386/eflags.h>
#include <i386/proc_reg.h>
#include <i386/seg.h>
#include <i386/tss.h>
#include <i386/user_ldt.h>
#include <i386/fpu.h>
#include <i386/iopb_entries.h>

/*
 * Maps state flavor to number of words in the state:
 */
unsigned int state_count[] = {
	/* FLAVOR_LIST */ 0,
	i386_THREAD_STATE_COUNT,
	i386_FLOAT_STATE_COUNT,
	i386_ISA_PORT_MAP_STATE_COUNT,
	i386_V86_ASSIST_STATE_COUNT,
	i386_REGS_SEGS_STATE_COUNT,
	i386_THREAD_SYSCALL_STATE_COUNT,
	/* THREAD_STATE_NONE */ 0,
	i386_SAVED_STATE_COUNT,
};

/* Forward */

void act_machine_throughcall(thread_act_t thr_act);
extern thread_t		Switch_context(
				thread_t		old,
				void			(*cont)(void),
				thread_t		new);
extern void		Thread_continue(void);
extern void		Load_context(
				thread_t		thread);

/*
 * consider_machine_collect:
 *
 *	Try to collect machine-dependent pages
 */
void
consider_machine_collect()
{
}

/*
 *	machine_kernel_stack_init:
 *
 *	Initialize a kernel stack which has already been
 *	attached to its thread_activation.
 */

void
machine_kernel_stack_init(
	thread_t	thread,
	void		(*continuation)(void))
{
	thread_act_t	thr_act = thread->top_act;
	vm_offset_t	stack;

	assert(thr_act);
	stack = thread->kernel_stack;
	assert(stack);

#if	MACH_ASSERT
	if (watchacts & WA_PCB) {
		printf("machine_kernel_stack_init(thr=%x,stk=%x,cont=%x)\n",
				thread,stack,continuation);
		printf("\tstack_iks=%x, stack_iel=%x\n",
			STACK_IKS(stack), STACK_IEL(stack));
	}
#endif	/* MACH_ASSERT */

	/*
	 *	We want to run continuation, giving it as an argument
	 *	the return value from Load_context/Switch_context.
	 *	Thread_continue takes care of the mismatch between
	 *	the argument-passing/return-value conventions.
	 *	This function will not return normally,
	 *	so we don`t have to worry about a return address.
	 */
	STACK_IKS(stack)->k_eip = (int) Thread_continue;
	STACK_IKS(stack)->k_ebx = (int) continuation;
	STACK_IKS(stack)->k_esp = (int) STACK_IEL(stack);

	/*
	 *	Point top of kernel stack to user`s registers.
	 */
	STACK_IEL(stack)->saved_state = &thr_act->mact.pcb->iss;
}


#if	NCPUS > 1
#define	curr_gdt(mycpu)		(mp_gdt[mycpu])
#define	curr_ktss(mycpu)	(mp_ktss[mycpu])
#else
#define	curr_gdt(mycpu)		(gdt)
#define	curr_ktss(mycpu)	(&ktss)
#endif

#define	gdt_desc_p(mycpu,sel) \
	((struct real_descriptor *)&curr_gdt(mycpu)[sel_idx(sel)])

void
act_machine_switch_pcb( thread_act_t new_act )
{
	pcb_t			pcb = new_act->mact.pcb;
	int			mycpu;
    {
	register iopb_tss_t	tss = pcb->ims.io_tss;
	vm_offset_t		pcb_stack_top;

        assert(new_act->thread != NULL);
        assert(new_act->thread->kernel_stack != 0);
        STACK_IEL(new_act->thread->kernel_stack)->saved_state =
                &new_act->mact.pcb->iss;

	/*
	 *	Save a pointer to the top of the "kernel" stack -
	 *	actually the place in the PCB where a trap into
	 *	kernel mode will push the registers.
	 *	The location depends on V8086 mode.  If we are
	 *	not in V8086 mode, then a trap into the kernel
	 *	won`t save the v86 segments, so we leave room.
	 */

	pcb_stack_top = (pcb->iss.efl & EFL_VM)
			? (int) (&pcb->iss + 1)
			: (int) (&pcb->iss.v86_segs);

	mp_disable_preemption();
	mycpu = cpu_number();

	if (tss == 0) {
	    /*
	     *	No per-thread IO permissions.
	     *	Use standard kernel TSS.
	     */
	    if (!(gdt_desc_p(mycpu,KERNEL_TSS)->access & ACC_TSS_BUSY))
		set_tr(KERNEL_TSS);
	    curr_ktss(mycpu)->esp0 = pcb_stack_top;
	}
	else {
	    /*
	     * Set the IO permissions.  Use this thread`s TSS.
	     */
	    *gdt_desc_p(mycpu,USER_TSS)
	    	= *(struct real_descriptor *)tss->iopb_desc;
	    tss->tss.esp0 = pcb_stack_top;
	    set_tr(USER_TSS);
	    gdt_desc_p(mycpu,KERNEL_TSS)->access &= ~ ACC_TSS_BUSY;
	}
    }

    {
	register user_ldt_t	ldt = pcb->ims.ldt;
	/*
	 * Set the thread`s LDT.
	 */
	if (ldt == 0) {
	    /*
	     * Use system LDT.
	     */
	    set_ldt(KERNEL_LDT);
	}
	else {
	    /*
	     * Thread has its own LDT.
	     */
	    *gdt_desc_p(mycpu,USER_LDT) = ldt->desc;
	    set_ldt(USER_LDT);
	}
    }
	mp_enable_preemption();
	/*
	 * Load the floating-point context, if necessary.
	 */
	fpu_load_context(pcb);

}

/*
 * flush out any lazily evaluated HW state in the
 * owning thread's context, before termination.
 */
void
thread_machine_flush( thread_act_t cur_act )
{
    fpflush(cur_act);
}

/*
 * Switch to the first thread on a CPU.
 */
void
load_context(
	thread_t		new)
{
	act_machine_switch_pcb(new->top_act);
	Load_context(new);
}

/*
 * Number of times we needed to swap an activation back in before
 * switching to it.
 */
int switch_act_swapins = 0;

/*
 * machine_switch_act
 *
 * Machine-dependent details of activation switching.  Called with
 * RPC locks held and preemption disabled.
 */
void
machine_switch_act( 
	thread_t	thread,
	thread_act_t	old,
	thread_act_t	new,
	unsigned	cpu)
{
	/*
	 *	Switch the vm, ast and pcb context. 
	 *	Save FP registers if in use and set TS (task switch) bit.
	 */
	fpu_save_context(thread);

	ast_context(new, cpu);

	PMAP_SWITCH_CONTEXT(old, new, cpu);
	act_machine_switch_pcb(new);
}

/*
 * Switch to a new thread.
 * Save the old thread`s kernel state or continuation,
 * and return it.
 */
thread_t
switch_context(
	thread_t		old,
	void			(*continuation)(void),
	thread_t		new)
{
	register thread_act_t	old_act = old->top_act,
				new_act = new->top_act;

#if MACH_RT
        assert(old_act->kernel_loaded ||
               active_stacks[cpu_number()] == old_act->thread->kernel_stack);
        assert (get_preemption_level() == 1);
#endif
	check_simple_locks();

	/*
	 *	Save FP registers if in use.
	 */
	fpu_save_context(old);

#if	MACH_ASSERT
	if (watchacts & WA_SWITCH)
		printf("\tswitch_context(old=%x con=%x new=%x)\n",
					    old, continuation, new);
#endif	/* MACH_ASSERT */

	/*
	 *	Switch address maps if need be, even if not switching tasks.
	 *	(A server activation may be "borrowing" a client map.)
	 */
    {
	int	mycpu = cpu_number();

	PMAP_SWITCH_CONTEXT(old_act, new_act, mycpu)
    }

	/*
	 *	Load the rest of the user state for the new thread
	 */
	act_machine_switch_pcb(new_act);
	return(Switch_context(old, continuation, new));
}

void
pcb_module_init(void)
{
	fpu_module_init();
	iopb_init();
}

void
pcb_init( register thread_act_t thr_act )
{
	register pcb_t pcb;

	assert(thr_act->mact.pcb == (pcb_t)0);
	pcb = thr_act->mact.pcb = &thr_act->mact.xxx_pcb;

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("pcb_init(%x) pcb=%x\n", thr_act, pcb);
#endif	/* MACH_ASSERT */

	/*
	 *	We can't let random values leak out to the user.
	 * (however, act_create() zeroed the entire thr_act, mact, pcb)
	 * bzero((char *) pcb, sizeof *pcb);
	 */
	simple_lock_init(&pcb->lock, ETAP_MISC_PCB);

	/*
	 *	Guarantee that the bootstrapped thread will be in user
	 *	mode.
	 */
	pcb->iss.cs = USER_CS;
	pcb->iss.ss = USER_DS;
	pcb->iss.ds = USER_DS;
	pcb->iss.es = USER_DS;
	pcb->iss.fs = USER_DS;
	pcb->iss.gs = USER_DS;
	pcb->iss.efl = EFL_USER_SET;
}

/*
 * Adjust saved register state for thread belonging to task
 * created with kernel_task_create().
 */
void
pcb_user_to_kernel(
	thread_act_t thr_act)
{
	register pcb_t pcb = thr_act->mact.pcb;

	pcb->iss.cs = KERNEL_CS;
	pcb->iss.ss = KERNEL_DS;
	pcb->iss.ds = KERNEL_DS;
	pcb->iss.es = KERNEL_DS;
	pcb->iss.fs = KERNEL_DS;
	pcb->iss.gs = CPU_DATA;
}

void
pcb_terminate(
	register thread_act_t thr_act)
{
	register pcb_t	pcb = thr_act->mact.pcb;

	assert(pcb);

	if (pcb->ims.io_tss != 0)
		iopb_destroy(pcb->ims.io_tss);
	if (pcb->ims.ifps != 0)
		fp_free(pcb->ims.ifps);
	if (pcb->ims.ldt != 0)
		user_ldt_free(pcb->ims.ldt);
	thr_act->mact.pcb = (pcb_t)0;
}

/*
 *	pcb_collect:
 *
 *	Attempt to free excess pcb memory.
 */

void
pcb_collect(
	register thread_act_t  thr_act)
{
	/* accomplishes very little */
}


/*
 *	act_machine_set_state:
 *
 *	Set the status of the specified thread.  Called with "appropriate"
 *	thread-related locks held (see act_lock_thread()), so
 *	thr_act->thread is guaranteed not to change.
 */

kern_return_t
act_machine_set_state(
	thread_act_t thr_act,
	thread_flavor_t flavor,
	thread_state_t tstate,
	mach_msg_type_number_t count)
{
	int 			kernel_act = thr_act->kernel_loading ||
					thr_act->kernel_loaded;

#if	MACH_ASSERT
	if (watchacts & WA_STATE)
	    printf("act_%x act_m_set_state(thr_act=%x,flav=%x,st=%x,cnt=%x)\n",
		    current_act(), thr_act, flavor, tstate, count);
#endif	/* MACH_ASSERT */

	switch (flavor) {
	    case THREAD_SYSCALL_STATE:
	    {
		register struct thread_syscall_state *state;
		register struct i386_saved_state *saved_state = USER_REGS(thr_act);

		state = (struct thread_syscall_state *) tstate;
		saved_state->eax = state->eax;
		saved_state->edx = state->edx;
		if (kernel_act)
			saved_state->efl = state->efl;
		else
			saved_state->efl = (state->efl & ~EFL_USER_CLEAR) | EFL_USER_SET;
		saved_state->eip = state->eip;
		saved_state->uesp = state->esp;
		break;
	    }

	    case i386_SAVED_STATE:
	    {
		register struct i386_saved_state	*state;
		register struct i386_saved_state	*saved_state;

		if (count < i386_SAVED_STATE_COUNT) {
		    return(KERN_INVALID_ARGUMENT);
		}

		state = (struct i386_saved_state *) tstate;

		saved_state = USER_REGS(thr_act);

		/*
		 * General registers
		 */
		saved_state->edi = state->edi;
		saved_state->esi = state->esi;
		saved_state->ebp = state->ebp;
		saved_state->uesp = state->uesp;
		saved_state->ebx = state->ebx;
		saved_state->edx = state->edx;
		saved_state->ecx = state->ecx;
		saved_state->eax = state->eax;
		saved_state->eip = state->eip;
		if (kernel_act)
			saved_state->efl = state->efl;
		else
			saved_state->efl = (state->efl & ~EFL_USER_CLEAR)
						| EFL_USER_SET;

		/*
		 * Segment registers.  Set differently in V8086 mode.
		 */
		if (state->efl & EFL_VM) {
		    /*
		     * Set V8086 mode segment registers.
		     */
		    saved_state->cs = state->cs & 0xffff;
		    saved_state->ss = state->ss & 0xffff;
		    saved_state->v86_segs.v86_ds = state->ds & 0xffff;
		    saved_state->v86_segs.v86_es = state->es & 0xffff;
		    saved_state->v86_segs.v86_fs = state->fs & 0xffff;
		    saved_state->v86_segs.v86_gs = state->gs & 0xffff;

		    /*
		     * Zero protected mode segment registers.
		     */
		    saved_state->ds = 0;
		    saved_state->es = 0;
		    saved_state->fs = 0;
		    saved_state->gs = 0;

		    if (thr_act->mact.pcb->ims.v86s.int_table) {
			/*
			 * Hardware assist on.
			 */
			thr_act->mact.pcb->ims.v86s.flags =
			    state->efl & (EFL_TF | EFL_IF);
		    }
#if	NCPUS > 1 && defined(CBUS)
		    thread_bind(thr_act->thread, master_processor);
#endif	/* NCPUS > 1 && defined(CBUS) */
		}
		else if (!kernel_act) {
		    /*
		     * 386 mode.  Set segment registers for flat
		     * 32-bit address space.
		     */
		    saved_state->cs = USER_CS;
		    saved_state->ss = USER_DS;
		    saved_state->ds = USER_DS;
		    saved_state->es = USER_DS;
		    saved_state->fs = USER_DS;
		    saved_state->gs = USER_DS;
		}
		else {
		    /*
		     * User setting segment registers.
		     * Code and stack selectors have already been
		     * checked.  Others will be reset by 'iret'
		     * if they are not valid.
		     */
		    saved_state->cs = state->cs;
		    saved_state->ss = state->ss;
		    saved_state->ds = state->ds;
		    saved_state->es = state->es;
		    saved_state->fs = state->fs;
		    saved_state->gs = state->gs;
		}
		break;
	    }

	    case i386_THREAD_STATE:
	    case i386_REGS_SEGS_STATE:
	    {
		register struct i386_thread_state	*state;
		register struct i386_saved_state	*saved_state;

		if (count < i386_THREAD_STATE_COUNT) {
		    return(KERN_INVALID_ARGUMENT);
		}

		if (flavor == i386_REGS_SEGS_STATE) {
		    /*
		     * Code and stack selectors must not be null,
		     * and must have user protection levels.
		     * Only the low 16 bits are valid.
		     */
		    state->cs &= 0xffff;
		    state->ss &= 0xffff;
		    state->ds &= 0xffff;
		    state->es &= 0xffff;
		    state->fs &= 0xffff;
		    state->gs &= 0xffff;

		    if (!kernel_act &&
			(state->cs == 0 || (state->cs & SEL_PL) != SEL_PL_U
		        || state->ss == 0 || (state->ss & SEL_PL) != SEL_PL_U))
			return KERN_INVALID_ARGUMENT;
		}

		state = (struct i386_thread_state *) tstate;

		saved_state = USER_REGS(thr_act);

		/*
		 * General registers
		 */
		saved_state->edi = state->edi;
		saved_state->esi = state->esi;
		saved_state->ebp = state->ebp;
		saved_state->uesp = state->uesp;
		saved_state->ebx = state->ebx;
		saved_state->edx = state->edx;
		saved_state->ecx = state->ecx;
		saved_state->eax = state->eax;
		saved_state->eip = state->eip;
		if (kernel_act)
			saved_state->efl = state->efl;
		else
			saved_state->efl = (state->efl & ~EFL_USER_CLEAR)
						| EFL_USER_SET;

		/*
		 * Segment registers.  Set differently in V8086 mode.
		 */
		if (state->efl & EFL_VM) {
		    /*
		     * Set V8086 mode segment registers.
		     */
		    saved_state->cs = state->cs & 0xffff;
		    saved_state->ss = state->ss & 0xffff;
		    saved_state->v86_segs.v86_ds = state->ds & 0xffff;
		    saved_state->v86_segs.v86_es = state->es & 0xffff;
		    saved_state->v86_segs.v86_fs = state->fs & 0xffff;
		    saved_state->v86_segs.v86_gs = state->gs & 0xffff;

		    /*
		     * Zero protected mode segment registers.
		     */
		    saved_state->ds = 0;
		    saved_state->es = 0;
		    saved_state->fs = 0;
		    saved_state->gs = 0;

		    if (thr_act->mact.pcb->ims.v86s.int_table) {
			/*
			 * Hardware assist on.
			 */
			thr_act->mact.pcb->ims.v86s.flags =
			    state->efl & (EFL_TF | EFL_IF);
		    }
#if	NCPUS > 1 && defined(CBUS)
		    thread_bind(thr_act->thread, master_processor);
#endif	/* NCPUS > 1 && defined(CBUS) */
		}
		else if (flavor == i386_THREAD_STATE && !kernel_act) {
		    /*
		     * 386 mode.  Set segment registers for flat
		     * 32-bit address space.
		     */
		    saved_state->cs = USER_CS;
		    saved_state->ss = USER_DS;
		    saved_state->ds = USER_DS;
		    saved_state->es = USER_DS;
		    saved_state->fs = USER_DS;
		    saved_state->gs = USER_DS;
		}
		else {
		    /*
		     * User setting segment registers.
		     * Code and stack selectors have already been
		     * checked.  Others will be reset by 'iret'
		     * if they are not valid.
		     */
		    saved_state->cs = state->cs;
		    saved_state->ss = state->ss;
		    saved_state->ds = state->ds;
		    saved_state->es = state->es;
		    saved_state->fs = state->fs;
		    saved_state->gs = state->gs;
		}
		break;
	    }

	    case i386_FLOAT_STATE: {

		if (count < i386_FLOAT_STATE_COUNT)
			return(KERN_INVALID_ARGUMENT);

		return fpu_set_state(thr_act,(struct i386_float_state*)tstate);
	    }

	    /*
	     * Temporary - replace by i386_io_map
	     */
	    case i386_ISA_PORT_MAP_STATE: {
		register struct i386_isa_port_map_state *state;
		register iopb_tss_t	tss;

		if (count < i386_ISA_PORT_MAP_STATE_COUNT)
			return(KERN_INVALID_ARGUMENT);

#if	NCPUS > 1 && defined(CBUS)
		thread_bind(thr_act->thread, master_processor);
#endif	/* NCPUS > 1 && defined(CBUS) */
		break;
	    }

	    case i386_V86_ASSIST_STATE:
	    {
		register struct i386_v86_assist_state *state;
		vm_offset_t	int_table;
		int		int_count;

		if (count < i386_V86_ASSIST_STATE_COUNT)
		    return KERN_INVALID_ARGUMENT;

		state = (struct i386_v86_assist_state *) tstate;
		int_table = state->int_table;
		int_count = state->int_count;

		if (int_table >= VM_MAX_ADDRESS ||
		    int_table +
			int_count * sizeof(struct v86_interrupt_table)
			    > VM_MAX_ADDRESS)
		    return KERN_INVALID_ARGUMENT;

		thr_act->mact.pcb->ims.v86s.int_table = int_table;
		thr_act->mact.pcb->ims.v86s.int_count = int_count;

		thr_act->mact.pcb->ims.v86s.flags =
			USER_REGS(thr_act)->efl & (EFL_TF | EFL_IF);
#if	NCPUS > 1 && defined(CBUS)
		thread_bind(thr_act->thread, master_processor);
#endif	/* NCPUS > 1 && defined(CBUS) */
		break;
	    }

	    default:
		return(KERN_INVALID_ARGUMENT);
	}

	return(KERN_SUCCESS);
}

/*
 *	thread_getstatus:
 *
 *	Get the status of the specified thread.
 */


kern_return_t
act_machine_get_state(
	thread_act_t thr_act,
	thread_flavor_t flavor,
	thread_state_t tstate,
	mach_msg_type_number_t *count)
{
#if	MACH_ASSERT
	if (watchacts & WA_STATE)
	    printf("act_%x act_m_get_state(thr_act=%x,flav=%x,st=%x,cnt@%x=%x)\n",
		current_act(), thr_act, flavor, tstate,
		count, (count ? *count : 0));
#endif	/* MACH_ASSERT */

	switch (flavor)  {

	    case i386_SAVED_STATE:
	    {
		register struct i386_saved_state	*state;
		register struct i386_saved_state	*saved_state;

		if (*count < i386_SAVED_STATE_COUNT)
		    return(KERN_INVALID_ARGUMENT);

		state = (struct i386_saved_state *) tstate;
		saved_state = USER_REGS(thr_act);

		/*
		 * First, copy everything:
		 */
		*state = *saved_state;

		if (saved_state->efl & EFL_VM) {
		    /*
		     * V8086 mode.
		     */
		    state->ds = saved_state->v86_segs.v86_ds & 0xffff;
		    state->es = saved_state->v86_segs.v86_es & 0xffff;
		    state->fs = saved_state->v86_segs.v86_fs & 0xffff;
		    state->gs = saved_state->v86_segs.v86_gs & 0xffff;

		    if (thr_act->mact.pcb->ims.v86s.int_table) {
			/*
			 * Hardware assist on
			 */
			if ((thr_act->mact.pcb->ims.v86s.flags &
					(EFL_IF|V86_IF_PENDING)) == 0)
			    state->efl &= ~EFL_IF;
		    }
		}
		else {
		    /*
		     * 386 mode.
		     */
		    state->ds = saved_state->ds & 0xffff;
		    state->es = saved_state->es & 0xffff;
		    state->fs = saved_state->fs & 0xffff;
		    state->gs = saved_state->gs & 0xffff;
		}
		*count = i386_SAVED_STATE_COUNT;
		break;
	    }

	    case i386_THREAD_STATE:
	    case i386_REGS_SEGS_STATE:
	    {
		register struct i386_thread_state	*state;
		register struct i386_saved_state	*saved_state;

		if (*count < i386_THREAD_STATE_COUNT)
		    return(KERN_INVALID_ARGUMENT);

		state = (struct i386_thread_state *) tstate;
		saved_state = USER_REGS(thr_act);

		/*
		 * General registers.
		 */
		state->edi = saved_state->edi;
		state->esi = saved_state->esi;
		state->ebp = saved_state->ebp;
		state->ebx = saved_state->ebx;
		state->edx = saved_state->edx;
		state->ecx = saved_state->ecx;
		state->eax = saved_state->eax;
		state->eip = saved_state->eip;
		state->efl = saved_state->efl;
		state->uesp = saved_state->uesp;

		state->cs = saved_state->cs;
		state->ss = saved_state->ss;
		if (saved_state->efl & EFL_VM) {
		    /*
		     * V8086 mode.
		     */
		    state->ds = saved_state->v86_segs.v86_ds & 0xffff;
		    state->es = saved_state->v86_segs.v86_es & 0xffff;
		    state->fs = saved_state->v86_segs.v86_fs & 0xffff;
		    state->gs = saved_state->v86_segs.v86_gs & 0xffff;

		    if (thr_act->mact.pcb->ims.v86s.int_table) {
			/*
			 * Hardware assist on
			 */
			if ((thr_act->mact.pcb->ims.v86s.flags &
					(EFL_IF|V86_IF_PENDING)) == 0)
			    state->efl &= ~EFL_IF;
		    }
		}
		else {
		    /*
		     * 386 mode.
		     */
		    state->ds = saved_state->ds & 0xffff;
		    state->es = saved_state->es & 0xffff;
		    state->fs = saved_state->fs & 0xffff;
		    state->gs = saved_state->gs & 0xffff;
		}
		*count = i386_THREAD_STATE_COUNT;
		break;
	    }

	    case THREAD_SYSCALL_STATE:
	    {
		register struct thread_syscall_state *state;
		register struct i386_saved_state *saved_state = USER_REGS(thr_act);

		state = (struct thread_syscall_state *) tstate;
		state->eax = saved_state->eax;
		state->edx = saved_state->edx;
		state->efl = saved_state->efl;
		state->eip = saved_state->eip;
		state->esp = saved_state->uesp;
		*count = i386_THREAD_SYSCALL_STATE_COUNT;
		break;
	    }

	    case THREAD_STATE_FLAVOR_LIST:
		if (*count < 5)
		    return (KERN_INVALID_ARGUMENT);
		tstate[0] = i386_THREAD_STATE;
		tstate[1] = i386_FLOAT_STATE;
		tstate[2] = i386_ISA_PORT_MAP_STATE;
		tstate[3] = i386_V86_ASSIST_STATE;
		tstate[4] = THREAD_SYSCALL_STATE;
		*count = 5;
		break;

	    case i386_FLOAT_STATE: {

		if (*count < i386_FLOAT_STATE_COUNT)
			return(KERN_INVALID_ARGUMENT);

		*count = i386_FLOAT_STATE_COUNT;
		return fpu_get_state(thr_act,(struct i386_float_state *)tstate);
	    }

	    /*
	     * Temporary - replace by i386_io_map
	     */
	    case i386_ISA_PORT_MAP_STATE: {
		register struct i386_isa_port_map_state *state;
		register iopb_tss_t tss;

		if (*count < i386_ISA_PORT_MAP_STATE_COUNT)
			return(KERN_INVALID_ARGUMENT);

		state = (struct i386_isa_port_map_state *) tstate;
		tss = thr_act->mact.pcb->ims.io_tss;

		if (tss == 0) {
		    int i;

		    /*
		     *	The thread has no ktss, so no IO permissions.
		     */

		    for (i = 0; i < sizeof state->pm; i++)
			state->pm[i] = 0xff;
		} else {
		    /*
		     *	The thread has its own ktss.
		     */

		    bcopy((char *) tss->bitmap,
			  (char *) state->pm,
			  sizeof state->pm);
		}

		*count = i386_ISA_PORT_MAP_STATE_COUNT;
		break;
	    }

	    case i386_V86_ASSIST_STATE:
	    {
		register struct i386_v86_assist_state *state;

		if (*count < i386_V86_ASSIST_STATE_COUNT)
		    return KERN_INVALID_ARGUMENT;

		state = (struct i386_v86_assist_state *) tstate;
		state->int_table = thr_act->mact.pcb->ims.v86s.int_table;
		state->int_count = thr_act->mact.pcb->ims.v86s.int_count;

		*count = i386_V86_ASSIST_STATE_COUNT;
		break;
	    }

	    default:
		return(KERN_INVALID_ARGUMENT);
	}

	return(KERN_SUCCESS);
}

/*
 * Alter the thread`s state so that a following thread_exception_return
 * will make the thread return 'retval' from a syscall.
 */
void
thread_set_syscall_return(
	thread_t	thread,
	kern_return_t	retval)
{
	thread->top_act->mact.pcb->iss.eax = retval;
}

/*
 * Initialize the machine-dependent state for a new thread.
 */
kern_return_t
thread_machine_create(thread_t thread, thread_act_t thr_act, void (*start_pos)(void))
{
        MachineThrAct_t mact = &thr_act->mact;

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("thread_machine_create(thr=%x,thr_act=%x,st=%x)\n",
			thread, thr_act, start_pos);
#endif	/* MACH_ASSERT */

        assert(thread != NULL);
        assert(thr_act != NULL);

        /*
         *      Allocate a kernel stack per shuttle
         */
        thread->kernel_stack = (int)stack_alloc();
        assert(thread->kernel_stack != 0);

        /*
         *      Point top of kernel stack to user`s registers.
         */
        STACK_IEL(thread->kernel_stack)->saved_state = &mact->pcb->iss;

	/*
	 * Utah code fiddles with pcb here - (we don't need to)
	 */
	return(KERN_SUCCESS);
}

/*
 * Machine-dependent cleanup prior to destroying a thread
 */
void
thread_machine_destroy( thread_t thread )
{
        spl_t s;

        assert(thread->kernel_stack != 0);

        s = splsched();
        stack_free(thread->kernel_stack);
        splx(s);
}

/*
 * This is used to set the current thr_act/thread
 * when starting up a new processor
 */
void
thread_machine_set_current( thread_t thread )
{
	register int	my_cpu;

	mp_disable_preemption();
	my_cpu = cpu_number();

        cpu_data[my_cpu].active_thread = thread;
	active_kloaded[my_cpu] =
		thread->top_act->kernel_loaded ? thread->top_act : THR_ACT_NULL;

	mp_enable_preemption();
}


/*
 * Pool of kernel activations.
 */

void act_machine_init()
{
	int i;
	thread_act_t thr_act;

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("act_machine_init()\n");
#endif	/* MACH_ASSERT */

	/* Good to verify this once */
	assert( THREAD_MACHINE_STATE_MAX <= THREAD_STATE_MAX );

	/*
	 * If we start using kernel activations,
	 * would normally create kernel_thread_pool here,
	 * populating it from the act_zone
	 */
}

kern_return_t
act_machine_create(task_t task, thread_act_t thr_act)
{
	MachineThrAct_t mact = &thr_act->mact;
	pcb_t pcb;

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("act_machine_create(task=%x,thr_act=%x) pcb=%x\n",
			task,thr_act, &mact->xxx_pcb);
#endif	/* MACH_ASSERT */

	/*
	 * Clear & Init the pcb  (sets up user-mode s regs)
	 */
	pcb_init(thr_act);

	return KERN_SUCCESS;
}

kern_return_t
act_create_kernel(task_t task, vm_offset_t stack, vm_size_t stack_size, 
		  thread_act_t *out_act)
{
	int rc;

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("act_create_kernel(task=%x,stk=%x,&thr_act=%x)\n",
				task, stack, out_act);
#endif	/* MACH_ASSERT */

	rc = act_create(task, stack, stack_size, out_act);
	return rc;
}

void
act_machine_destroy(thread_act_t thr_act)
{

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("act_machine_destroy(0x%x)\n", thr_act);
#endif	/* MACH_ASSERT */

	pcb_terminate(thr_act);
}

void
act_machine_return(int code)
{
	thread_act_t	thr_act = current_act();
	thread_act_t	cur_act;
	thread_t	cur_thread = current_thread();
	vm_offset_t	pcb_stack;
	unsigned int	uesp, ueip;
	struct ipc_port	*iplock;

#if	MACH_ASSERT
	/*
	* We don't go through the locking dance here needed to
	* acquire thr_act->thread safely.
	*/

	if (watchacts & WA_EXIT)
		printf("act_machine_return(0x%x) cur_act=%x(%d) thr=%x(%d)\n",
			code, thr_act, thr_act->ref_count,
			thr_act->thread, thr_act->thread
					? thr_act->thread->ref_count : 0);
#endif	/* MACH_ASSERT */

	/*
	 * This code is called with nothing locked.
	 * It also returns with nothing locked, if it returns.
	 *
	 * This routine terminates the current thread activation.
	 * If this is the only activation associated with its
	 * thread shuttle, then the entire thread (shuttle plus
	 * activation) is terminated.
	 */
	assert( code == KERN_TERMINATED );
	assert( thr_act );

	act_lock_thread(thr_act);
	iplock = thr_act->pool_port;	/* remember for unlock call */

	/* if this is the only activation attached to the shuttle... */
	if ( (thr_act->thread->top_act == thr_act)
		&& (thr_act->lower == THR_ACT_NULL) ) {
	    /* terminate the entire thread (shuttle plus activation) */
	    act_unlock_thread(thr_act);
	    thread_terminate_self();
	    /*NOTREACHED*/
	    panic("act_machine_return: TALKING ZOMBIE! (1)");
	}
	else {
	    /* terminate only this activation */
	    if ( thr_act->thread->top_act != thr_act ) {
		/*
		 * this is not the top activation;
		 * if possible, we should clone the shuttle so that
		 * both the root RPC-chain and the soon-to-be-orphaned
		 * RPC-chain have shuttles
		 */
		act_unlock_thread(thr_act);
		panic("act_machine_return: ORPHAN CASE NOT YET IMPLEMENTED");
	    }

	    /* if there is a previous activation on the RPC chain... */
	    if ( thr_act->lower != THR_ACT_NULL ) {
		/* send it an appropriate return code */
		thr_act->lower->alerts |= SERVER_TERMINATED;
		install_special_handler(thr_act->lower);
	    }

	    /* Return to previous act with error code */
	    act_locked_act_reference(thr_act);	/* keep it around */
	    act_switch_swapcheck(cur_thread, (ipc_port_t)0);
	    (void) switch_act(THR_ACT_NULL);
	    /* assert(thr_act->ref_count == 0); */		/* XXX */
	    cur_act = cur_thread->top_act;
	    MACH_RPC_RET(cur_act) = KERN_RPC_SERVER_TERMINATED;	    
	    machine_kernel_stack_init(cur_thread, 
			(void (*)(void)) mach_rpc_return_error);
	    /*
	     * The following unlocks must be done separately since fields 
	     * used by `act_unlock_thread()' have been cleared, meaning
	     * that it would not release all of the appropriate locks.
	     */
	    rpc_unlock(cur_thread);
	    if (iplock) ip_unlock(iplock);	/* must be done separately */
	    act_unlock(thr_act);
	    act_deallocate(thr_act);		/* free it */
	    Load_context(cur_thread);
	    /*NOTREACHED*/

            panic("act_machine_return: TALKING ZOMBIE! (2)");
	}
}


/*
 * Perform machine-dependent per-thread initializations
 */
void
thread_machine_init(void)
{
	pcb_module_init();
}

/*
 * Some routines for debugging activation code
 */
static void	dump_handlers(thread_act_t);
static void	dump_regs(thread_act_t);

static void
dump_handlers(thread_act_t thr_act)
{
    ReturnHandler *rhp = thr_act->handlers;
    int	counter = 0;

    printf("\t");
    while (rhp) {
	if (rhp == &thr_act->special_handler){
	    if (rhp->next)
		printf("[NON-Zero next ptr(%x)]", rhp->next);
	    printf("special_handler()->");
	    break;
	}
	printf("hdlr_%d(%x)->",counter,rhp->handler);
	rhp = rhp->next;
	if (++counter > 32) {
		printf("Aborting: HUGE handler chain\n");
		break;
	}
    }
    printf("HLDR_NULL\n");
}

static void
dump_regs(thread_act_t thr_act)
{
	if (thr_act->mact.pcb) {
		register struct i386_saved_state *ssp = USER_REGS(thr_act);
		/* Print out user register state */
		printf("\tRegs:\tedi=%x esi=%x ebp=%x ebx=%x edx=%x\n",
		    ssp->edi, ssp->esi, ssp->ebp, ssp->ebx, ssp->edx);
		printf("\t\tecx=%x eax=%x eip=%x efl=%x uesp=%x\n",
		    ssp->ecx, ssp->eax, ssp->eip, ssp->efl, ssp->uesp);
		printf("\t\tcs=%x ss=%x\n", ssp->cs, ssp->ss);
	}
}

int
dump_act(thread_act_t thr_act)
{
	if (!thr_act)
		return(0);

	printf("thr_act(0x%x)(%d): thread=%x(%d) task=%x(%d)\n",
	       thr_act, thr_act->ref_count,
	       thr_act->thread, thr_act->thread ? thr_act->thread->ref_count:0,
	       thr_act->task,   thr_act->task   ? thr_act->task->ref_count : 0);

	if (thr_act->pool_port) {
	    thread_pool_t actpp = &thr_act->pool_port->ip_thread_pool;
	    printf("\tpool(acts_p=%x, waiting=%d) pool_next %x\n",
		actpp->thr_acts, actpp->waiting, thr_act->thread_pool_next);
	}else
	    printf("\tno thread_pool\n");

	printf("\talerts=%x mask=%x susp=%d user_stop=%d active=%x ast=%x\n",
		       thr_act->alerts, thr_act->alert_mask,
		       thr_act->suspend_count, thr_act->user_stop_count,
		       thr_act->active, thr_act->ast);
	printf("\thi=%x lo=%x\n", thr_act->higher, thr_act->lower);
	printf("\tpcb=%x, ustk=%x\n",
		       thr_act->mact.pcb, thr_act->user_stack);

	if (thr_act->thread && thr_act->thread->kernel_stack) {
	    vm_offset_t stack = thr_act->thread->kernel_stack;

	    printf("\tk_stk %x  eip %x ebx %x esp %x iss %x\n",
		stack, STACK_IKS(stack)->k_eip, STACK_IKS(stack)->k_ebx,
		STACK_IKS(stack)->k_esp, STACK_IEL(stack)->saved_state);
	}

	dump_handlers(thr_act);
	dump_regs(thr_act);
	return((int)thr_act);
}

