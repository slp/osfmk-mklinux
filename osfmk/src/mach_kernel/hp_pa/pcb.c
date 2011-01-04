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
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: pcb.c 1.23 92/06/27$
 */

#include <cpus.h>
#include <mach_rt.h>
#include <mach_debug.h>
#include <mach_ldebug.h>

#include <types.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <mach/thread_status.h>
#include <machine/spl.h>
#include <machine/psw.h>
#include <vm/vm_kern.h>
#include <kern/mach_param.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/fpu_protos.h>
#include <kern/spl.h>
#include <hp_pa/pmap.h>
#include <hp_pa/physmem.h>
#include <machine/asm.h>

struct physmem_queue pcb_queue;	/* queue of used pcb */
pcb_t fpu_pcb = 0;		/* PCB of the thread owning the FPU */
pcb_t active_pcbs[NCPUS];	/* PCB belonging to the active thread */
#if DEBUG
int   fpu_trap_count = 0;
int   fpu_switch_count = 0;
#endif

boolean_t machine_collect_allowed = TRUE;
unsigned machine_collect_last_tick;
unsigned machine_collect_max_rate;             /* in ticks */

/*
 * consider_machine_collect: try to collect machine-dependent pages
 */
void
consider_machine_collect()
{
    /*
     * By default, don't attempt machine-dependent collection
     * more frequently than once a second.
     */
    if (machine_collect_max_rate == 0)
	machine_collect_max_rate = hz;

    if (machine_collect_allowed && (sched_tick > (machine_collect_last_tick +
						  machine_collect_max_rate))) {
	machine_collect_last_tick = sched_tick;
	physmem_collect_scan();
    }
}

/*
 * stack_attach: Attach a kernel stack to a thread.
 */
void
machine_kernel_stack_init(
	struct thread_shuttle *thread,
	void		(*continuation)(void))
{
    vm_offset_t	stack;
    struct hp700_kernel_state *ksp;

    stack = thread->kernel_stack;

    assert(stack);

#if	MACH_ASSERT
    if (watchacts & WA_PCB)
	printf("machine_kernel_stack_init(thr=%x,stk=%x,cont=%x)\n", thread,stack,continuation);
#endif	/* MACH_ASSERT */

    /*
     * Initialize saved state
     */
    ksp = (struct hp700_kernel_state *)stack;    
    bzero((char *)ksp, sizeof (*ksp));

    /*
     * Set up new thread to start running at continuation in kernel mode
     */
    ksp->rp = ((int) continuation & ~PC_PRIV_MASK) | PC_PRIV_KERN;

    /*
     * Build a kernel state area + arg frame on the stack for the initial
     * switch into the thread. We also store a zero into the kernel
     * stack pointer so the trap code knows there is already a frame
     * on the kernel stack.
     */
    ksp->sp = (vm_offset_t) ((int) stack + KF_SIZE);
    thread->top_act->mact.pcb->ksp = 0;
}

/*
 * Alter the thread`s state so that a following thread_exception_return
 * will make the thread return 'retval' from a syscall.
 */
void
thread_set_syscall_return(
	struct thread_shuttle *thread,
	kern_return_t	retval)
{
	struct hp700_saved_state *ssp = &thread->top_act->mact.pcb->ss;

	if ((ssp->flags & SS_INSYSCALL) == 0)
		panic("thread_set_syscall_return: not in syscall");

	ssp->ret0 = retval;
}

/*
 * Initialize the machine-dependent state for a new thread.
 */
kern_return_t
thread_machine_create(
		      struct thread_shuttle *thread,
		      thread_act_t thr_act,
		      void (*start_pos)(void))
{
#if	MACH_ASSERT
    if (watchacts & WA_PCB)
	printf("thread_machine_create(thr=%x,thr_act=%x,st=%x)\n", thread, thr_act, start_pos);
#endif	/* MACH_ASSERT */

    thread->kernel_stack = (int)stack_alloc();
    assert(thread->kernel_stack);

    /*
     * Utah code fiddles with pcb here - (we don't need to)
     */
    return(KERN_SUCCESS);
}

/*
 * Machine-dependent cleanup prior to destroying a thread
 */
void
thread_machine_destroy(thread_t thread)
{
	spl_t s;

	assert(thread->kernel_stack);
	s = splsched();
	stack_free(thread->kernel_stack);
	splx(s);
}

/*
 * flush out any lazily evaluated HW state in the
 * owning thread's context, before termination.
 */
void
thread_machine_flush( thread_act_t cur_act )
{
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
	active_stacks[cpu] = thread->kernel_stack;
	active_pcbs[cpu] = new->mact.pcb;

	ast_context(new, cpu);

	if (fpu_pcb)
		disable_fpu();
}

void
pcb_user_to_kernel(thread_act_t act)
{
	register pcb_t pcb = act->mact.pcb;

	pcb->ss.iioq_head &= ~PC_PRIV_USER;
	pcb->ss.iioq_tail &= ~PC_PRIV_USER;
}

/*
 * act_machine_destroy: Shutdown any state associated with a thread pcb.
 */
void
act_machine_destroy(thread_act_t act)
{
	register pcb_t	pcb = act->mact.pcb;

	assert(pcb);

	if (fpu_pcb == pcb)
		fpu_pcb = 0;
	physmem_free(&pcb_queue, (vm_offset_t)pcb);
	act->mact.pcb = (pcb_t)0;
}

kern_return_t
act_machine_create(task_t task, thread_act_t act)
{
	register pcb_t pcb;
	pmap_t pmap = act->task->map->pmap;
	struct hp700_float_state *fstate;

	/*
	 * Allocate a pcb and zero it to avoid random values in the registers.
	 */
	act->mact.pcb = pcb = (pcb_t)physmem_alloc(&pcb_queue);
	bzero((char*)pcb, sizeof(struct pcb));

	/*
	 * User threads will pull ther context from the pcb when first
	 * returning to user mode, so fill in all the necessary values.
	 * Kernel threads are initialized from the save state structure 
	 * at the base of the kernel stack (see stack_attach()).
	 */
	pcb->ss.flags = SS_INTRAP;	/* XXX For thread_bootstrap_return */
	pcb->ss.sr4   = (int) pmap->space;
	pcb->ss.sr5   = (int) pmap->space;
	pcb->ss.sr6   = (int) pmap->space;
	pcb->ss.pidr1   = pmap->pid;
        pcb->ss.pidr2   = pmap->pid;
        pcb->ss.pidr3   = pmap->pid;

	pcb->ss.eiem  = SPL0;
	pcb->ss.ipsw  = PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I;

	pcb->ss.iisq_head = (int) pmap->space;
	pcb->ss.iisq_tail = (int) pmap->space;
	pcb->ss.iioq_head = (int) PC_PRIV_USER;
	pcb->ss.iioq_tail = (int) PC_PRIV_USER;

	return KERN_SUCCESS;
}

void act_machine_init()
{
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
act_create_kernel(task_t task, vm_offset_t stack, vm_size_t stack_size, thread_act_t *out_act)
{
#if	MACH_ASSERT
    if (watchacts & WA_PCB)
	printf("act_create_kernel(task=%x,stk=%x,&thr_act=%x)\n",  task, stack, out_act);
#endif	/* MACH_ASSERT */
    
    return act_create(task, stack, stack_size, out_act);
}

void
act_machine_return(int code)
{
    thread_act_t thr_act = current_act();
    thread_act_t	cur_act;
    thread_t	cur_thread = current_thread();
    struct ipc_port	*iplock;

#if	MACH_ASSERT
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
	    rpc_unlock(cur_thread);
	    if (iplock) ip_unlock(iplock);	/* must be done separately */
	    act_unlock(thr_act);
	    act_deallocate(thr_act);		/* free it */
	    Load_context(cur_thread);
	    /*NOTREACHED*/

            panic("act_machine_return: TALKING ZOMBIE! (2)");
	}
}

void
thread_machine_set_current(struct thread_shuttle *thread)
{
    register int	my_cpu = cpu_number();

    cpu_data[my_cpu].active_thread = thread;
    active_pcbs[my_cpu] = thread->top_act->mact.pcb;
    active_kloaded[my_cpu] = thread->top_act->kernel_loaded ? thread->top_act : THR_ACT_NULL;
}



void
thread_machine_init(void)
{
    physmem_add(&pcb_queue, sizeof (struct pcb), ETAP_MISC_PCB);
}

#ifdef DEBUG
extern thread_t Switch_context(thread_t,
			       void (*)(void),
			       thread_t);

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
#if MACH_RT
        assert(active_stacks[cpu_number()] ==
	       old->top_act->thread->kernel_stack);
        assert (get_preemption_level() == 1);
	assert((old->state &
		(TH_HALTED|TH_RUN|TH_WAIT|TH_IDLE)) != (TH_HALTED|TH_RUN));
	assert(old->preempt != TH_PREEMPTED);

#endif
	check_simple_locks();
	return(Switch_context(old, continuation, new));
}
#endif

#if MACH_DEBUG
void
dump_pcb(pcb_t pcb)
{
	printf("pcb @ %8.8x:\n", pcb);
	printf("pc  %8.8x %8.8x\n", pcb->ss.iioq_head, pcb->ss.iioq_tail);
	printf("rp  %8.8x	r12  %8.8x	sr3   %8.8x	rctr  %8.8x\n",
	       pcb->ss.rp, pcb->ss.r12, pcb->ss.sr3, pcb->ss.rctr);
	printf("r3  %8.8x	r13  %8.8x	sr4   %8.8x	pidr1 %8.8x\n",
	       pcb->ss.r3, pcb->ss.r13, pcb->ss.sr4, pcb->ss.pidr1);
	printf("r4  %8.8x	r14  %8.8x	sr5   %8.8x	pidr2 %8.8x\n",
	       pcb->ss.r4, pcb->ss.r14, pcb->ss.sr5, pcb->ss.pidr2);
	printf("r5  %8.8x	r15  %8.8x	sr6   %8.8x	sar   %8.8x\n",
	       pcb->ss.r5, pcb->ss.r15, pcb->ss.sr6, pcb->ss.sar);
	printf("r6  %8.8x	r16  %8.8x	sr7   %8.8x	pidr3 %8.8x\n",
	       pcb->ss.r6, pcb->ss.r16, pcb->ss.sr7, pcb->ss.pidr3);
	printf("r7  %8.8x	r17  %8.8x			pidr4 %8.8x\n",
	       pcb->ss.r7, pcb->ss.r17, pcb->ss.pidr4);
	printf("r8  %8.8x	r18  %8.8x			eiem  %8.8x\n",
	       pcb->ss.r8, pcb->ss.r18, pcb->ss.eiem);
	printf("r9  %8.8x	dp   %8.8x\n",
	       pcb->ss.r9, pcb->ss.dp);
	printf("r10 %8.8x	sp   %8.8x			psw   %8.8x\n",
	       pcb->ss.r10, pcb->ss.sp, pcb->ss.ipsw);
	printf("r11 %8.8x					ksp   %8.8x\n",
	       pcb->ss.r11, pcb->ksp);
}

void
dump_thread(thread_t th)
{
	printf(" thread @ 0x%x:\n", th);
}
#endif

#if MACH_ASSERT
int
    dump_act(thread_act_t thr_act)
{
    if (!thr_act)
	return(0);

    printf("thr_act(0x%x)(%d): thread=%x(%d) task=%x(%d)\n",
	   thr_act, thr_act->ref_count,
	   thr_act->thread, thr_act->thread ? thr_act->thread->ref_count:0,
	   thr_act->task,   thr_act->task   ? thr_act->task->ref_count : 0);

    printf("\talerts=%x mask=%x susp=%x active=%x hi=%x lo=%x\n",
	   thr_act->alerts, thr_act->alert_mask,
	   thr_act->suspend_count, thr_act->active,
	   thr_act->higher, thr_act->lower);

    return((int)thr_act);
}

#endif
