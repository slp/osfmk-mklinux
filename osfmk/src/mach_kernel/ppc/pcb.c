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
#include <debug.h>

#include <types.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <mach/thread_status.h>
#include <machine/spl.h>
#include <vm/vm_kern.h>
#include <kern/mach_param.h>

#include <kern/misc_protos.h>
#include <ppc/misc_protos.h>
#include <ppc/fpu_protos.h>
#include <ppc/exception.h>
#include <ppc/proc_reg.h>
#include <kern/spl.h>
#include <ppc/pmap.h>
#include <ppc/proc_reg.h>
#include <ppc/trap.h>
/*
 * These constants are dumb. They should not be in asm.h!
 */
#define FM_SIZE		16	/* Minimum frame size */
#define KF_SIZE		(FM_SIZE+ARG_SIZE)

#if DEBUG
int   fpu_trap_count = 0;
int   fpu_switch_count = 0;
#endif

extern struct thread_shuttle	*Switch_context(
					struct thread_shuttle 	*old,
				       	void	      		(*cont)(void),
					struct thread_shuttle 	*new);



/*
 * consider_machine_collect: try to collect machine-dependent pages
 */
void
consider_machine_collect()
{
    /*
     * none currently available
     */
	return;
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
    struct ppc_kernel_state *kss;

    assert(thread->kernel_stack);
    stack = thread->kernel_stack;

#if	MACH_ASSERT
    if (watchacts & WA_PCB)
	printf("machine_kernel_stack_init(thr=%x,stk=%x,cont=%x)\n", thread,stack,continuation);
#endif	/* MACH_ASSERT */

    kss = STACK_IKS(stack);

    /*
     * Build a kernel state area + arg frame on the stack for the initial
     * switch into the thread. We also store a zero into the kernel
     * stack pointer so the trap code knows there is already a frame
     * on the kernel stack.
     */

    kss->lr = (unsigned int) continuation;
    kss->r1 = (vm_offset_t) ((int)kss - KF_SIZE);

    *((int*)kss->r1) = 0;	/* Zero the frame backpointer */

    thread->top_act->mact.pcb->ksp = 0;

}

/* Prepare to return to a different space to that which we came from */

void pmap_switch(pmap_t map)
{
	unsigned int i;

#if DEBUG
	if (watchacts & WA_PCB) {
		printf("Switching to map at 0x%08x, space=%d\n",
		       map,map->space);
	}
#endif /* DEBUG */

#if !DEBUG
	/* when changing to kernel space (collocated servers), don't bother
	 * doing anything, the kernel is mapped from here already. Do change
	 * in the debug case, to ease debugging.
	 */
	if (map->space == PPC_SID_KERNEL)
		return;
#endif /* 0 */

	/* sr value has Ks=1, Ku=1, and vsid composed of space+seg num */
	i = SEG_REG_PROT | (map->space << 4);

	assert(SR_KERNEL == 0);
/*	mtsr(0x0, i + 0x0); SR0 is the kernel segment! */
/*	mtsr(0x1, i + 0x1); SR1 is kernel collocation space */
/*	mtsr(0x2, i + 0x2); SR2 is more   collocation space */
	mtsr(0x3, i + 0x3);
	mtsr(0x4, i + 0x4);
	mtsr(0x5, i + 0x5);
	mtsr(0x6, i + 0x6);
	mtsr(0x7, i + 0x7);
	mtsr(0x8, i + 0x8);
	mtsr(0x9, i + 0x9);
	mtsr(0xa, i + 0xa);
	mtsr(0xb, i + 0xb);
/*	mtsr(0xc, i + 0xc);	Can overwrite copyin SR with no problem */
	mtsr(0xd, i + 0xd);
	mtsr(0xe, i + 0xe);
	mtsr(0xf, i + 0xf);

	/* After any segment register changes, we have to resync if
	 * we want to use the new translations. We don't use any
	 * of the above user space until after an rfi, so no need
	 * for us to sync now
	 */
}

/*
 * switch_context: Switch from one thread to another, needed for
 * 		   switching of space
 * 
 */
struct thread_shuttle*
switch_context(
	struct thread_shuttle *old,
	void (*continuation)(void),
	struct thread_shuttle *new)
{
	register thread_act_t old_act = old->top_act, new_act = new->top_act;
	register struct thread_shuttle* retval;
	pmap_t	new_pmap;
#if MACH_RT
	assert(old_act->kernel_loaded ||
	       active_stacks[cpu_number()] == old_act->thread->kernel_stack);
	assert (get_preemption_level() == 1);
#endif
	check_simple_locks();

#if	NCPUS > 1
	/* Our context might wake up on another processor, so we must
	 * not keep hot state in our FPU, it must go back to the pcb
	 * so that it can be found by the other if needed
	 */
	fpu_save();
#endif	/* NCPUS > 1 */

#if DEBUG
	if (watchacts & WA_PCB) {
		printf("switch_context(0x%08x, 0x%x, 0x%08x)\n",
		       old,continuation,new);
	}
#endif /* DEBUG */

	/*
	 * We do not have to worry about the PMAP module, so switch.
	 *
	 * We must not use top_act->map since this may not be the actual
	 * task map, but the map being used for a klcopyin/out.
	 */

	new_pmap = new_act->task->map->pmap;
	if (old_act->task->map->pmap != new_pmap) {
		pmap_switch(new_pmap);
	}

	/* Sanity check - is the stack pointer inside the stack that
	 * we're about to switch to? Is the execution address within
	 * the kernel's VM space??
	 */
	assert((new->kernel_stack < STACK_IKS(new->kernel_stack)->r1) &&
	       ((unsigned int)STACK_IKS(new->kernel_stack) >
		STACK_IKS(new->kernel_stack)->r1));
	assert(STACK_IKS(new->kernel_stack)->lr < VM_MAX_KERNEL_ADDRESS);

	if (!USER_MODE(new_act->mact.pcb->ss.srr1)) {
		/* Make sure that in kernel space r2 always points
		 * to the current CPU's proc_info
		 */
		new_act->mact.pcb->ss.r2 = (unsigned int)current_proc_info();
	}
	retval = Switch_context(old, continuation, new);
	assert(retval != (struct thread_shuttle*)NULL);

	/* We've returned from having switched context, so we should be
	 * back in the original context. Do a quick assert
	 */

#if 0 && DEBUG
	/* TODO NMGS on 601 all segs 8-f are initially mapped as IO so
	 * this isn't true any more. Change SR_UNUSED_BY_KERN? can ignore
	 * this anyway
	 */
	{
		unsigned int space;
		mfsr(space, SR_UNUSED_BY_KERN);
		space = (space >> 4) & 0x00FFFFFF;

		/* Do a check that at least SR_UNUSED_BY_KERN
		 * is in the correct user space.
		 */
		assert(space == old_act->task->map->pmap->space);
	}
#endif /* DEBUG */

	return retval;
}

/*
 * Alter the thread's state so that a following thread_exception_return
 * will make the thread return 'retval' from a syscall.
 */
void
thread_set_syscall_return(
	struct thread_shuttle *thread,
	kern_return_t	retval)
{
	struct ppc_saved_state *ssp = &thread->top_act->mact.pcb->ss;

	assert(MSR_WAS_SYSCALL(ssp->srr1));
#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("thread_set_syscall_return(thr=%x,retval=%d)\n", thread,retval);
#endif	/* MACH_ASSERT */

        ssp->r3 = retval;
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
thread_machine_destroy( thread_t thread )
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
	pmap_t		new_pmap;

#if	NCPUS != 1
	/* We can't be lazy - the old activation might migrate
	 * to another CPU so we must not keep hot state in our FPU
	 */
	fpu_save();
#endif	/* NCPUS == 1 */

	/* lazy context switch the FPU on the new thread */
	new->mact.pcb->ss.srr1 &= ~ MASK(MSR_FP);

	active_stacks[cpu] = thread->kernel_stack;

	ast_context(new, cpu);

	/* Activations might have different pmaps 
	 * (process->kernel->server, for example).
	 * Change space if needed
	 */
	new_pmap = new->task->map->pmap;
	if (old->task->map->pmap != new_pmap) {
		pmap_switch(new_pmap);
	}
}

void
pcb_user_to_kernel(thread_act_t act)
{
	act->mact.pcb->ss.srr1 &= ~MASK(MSR_PR);
	/* kernel-loaded servers need to maintain the value of kernel's r2
	 */
	act->mact.pcb->ss.r2 = (vm_offset_t) (&per_proc_info[cpu_number()]);
}

/*
 * act_machine_destroy: Shutdown any state associated with a thread pcb.
 */
void
act_machine_destroy(thread_act_t act)
{
	register pcb_t	pcb = act->mact.pcb;

	assert(pcb);

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("act_machine_destroy(0x%x)\n", act);
#endif	/* MACH_ASSERT */

	/* TODO NMGS we must assert that there are no outstanding
	 * FPU exceptions... not easy
	 */
	if ((pcb_t)(per_proc_info[cpu_number()].fpu_pcb) == pcb)
		per_proc_info[cpu_number()].fpu_pcb = 0;

	act->mact.pcb = (pcb_t)0;
}

kern_return_t
act_machine_create(task_t task, thread_act_t thr_act)
{
	/*
	 * Clear & Init the pcb  (sets up user-mode s regs)
	 */

	register pcb_t pcb;
	pmap_t pmap = thr_act->task->map->pmap;
	
#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("act_machine_create(task=%x,thr_act=%x) pcb=%x\n", task,thr_act);
#endif	/* MACH_ASSERT */

	assert(thr_act->mact.pcb == (pcb_t)0);
	pcb = thr_act->mact.pcb = &thr_act->mact.xxx_pcb;
	/*
	 *	We can't let random values leak out to the user.
	 * (however, act_create() zeroed the entire thr_act, mact, pcb)
	 * bzero((char *) pcb, sizeof *pcb);
	 */

#if	MACH_ASSERT
	if (watchacts & WA_PCB)
		printf("pcb_init(%x) pcb=%x\n", thr_act, pcb);
#endif	/* MACH_ASSERT */
	/*
	 * User threads will pull ther context from the pcb when first
	 * returning to user mode, so fill in all the necessary values.
	 * Kernel threads are initialized from the save state structure 
	 * at the base of the kernel stack (see stack_attach()).
	 */

	pcb->ss.srr1 = MSR_EXPORT_MASK_SET;

	pcb->sr0     = SEG_REG_PROT | (pmap->space<<4);
	pcb->ss.sr_copyin    = SEG_REG_PROT | SR_COPYIN + (pmap->space<<4);

#if	MACH_ASSERT
	if (watchacts & WA_PCB) {
		printf("___ACT_MACHINE_CREATE____SPACE = %d_______________\n",
		       pmap->space);
		printf("pcb->sr0 = 0x%08x\n",pcb->sr0);
	}
#endif /* MACH_ASSERT */

    return KERN_SUCCESS;
}

void act_machine_init()
{
#if	MACH_ASSERT
    if (watchacts & WA_PCB)
	printf("act_machine_init()\n");
#endif	/* MACH_ASSERT */

    /* Good to verify these once */
    assert( THREAD_MACHINE_STATE_MAX <= THREAD_STATE_MAX );

    assert( THREAD_STATE_MAX >= PPC_THREAD_STATE_COUNT );
    assert( THREAD_STATE_MAX >= PPC_EXCEPTION_STATE_COUNT );
    assert( THREAD_STATE_MAX >= PPC_FLOAT_STATE_COUNT );
    assert( THREAD_STATE_MAX >= sizeof(struct ppc_saved_state)/sizeof(int));

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
	} else {
		/* terminate only this activation */
		if ( thr_act->thread->top_act != thr_act ) {
			/*
			 * this is not the top activation;
			 * if possible, we should clone the shuttle so that
			 * both the root RPC-chain and the soon-to-be-orphaned
			 * RPC-chain have shuttles
			 */
			act_unlock_thread(thr_act);
			panic("act_machine_return: "
			      "ORPHAN CASE NOT YET IMPLEMENTED");
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

void
thread_machine_set_current(struct thread_shuttle *thread)
{
    register int	my_cpu = cpu_number();

    cpu_data[my_cpu].active_thread == thread;

    active_kloaded[my_cpu] = thread->top_act->kernel_loaded ? thread->top_act : THR_ACT_NULL;
}

void
thread_machine_init(void)
{
#ifdef	MACHINE_STACK
#if KERNEL_STACK_SIZE > PPC_PGBYTES
#if MACH_ASSERT
	printf("WARNING : KERNEL_STACK_SIZE is greater than PPC_PGBYTES\n");
#endif
#endif
#endif
}

#if MACH_ASSERT
void
dump_pcb(pcb_t pcb)
{
	printf("pcb @ %8.8x:\n", pcb);
	printf("ksp       = 0x%08x\n\n",pcb->ksp);
#if DEBUG
	regDump(&pcb->ss);
#endif /* DEBUG */
}

void
dump_thread(thread_t th)
{
	printf(" thread @ 0x%x:\n", th);
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

    printf("\talerts=%x mask=%x susp=%x active=%x hi=%x lo=%x\n",
	   thr_act->alerts, thr_act->alert_mask,
	   thr_act->suspend_count, thr_act->active,
	   thr_act->higher, thr_act->lower);

    return((int)thr_act);
}

#endif



    
