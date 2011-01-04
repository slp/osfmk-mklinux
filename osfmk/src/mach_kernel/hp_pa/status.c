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

#include <kern/thread.h>
#include <mach/thread_status.h>
#include <machine/psw.h>
#include <kern/misc_protos.h>
#include <hp_pa/fpu_protos.h>

extern pcb_t fpu_pcb;

/*
 * Maps state flavor to number of words in the state:
 */
unsigned int state_count[] = {
	/* FLAVOR_LIST */ 0,
	HP700_THREAD_STATE_COUNT,
	HP700_FLOAT_STATE_COUNT,
	HP700_SYSCALL_STATE_COUNT,
};

/*
 * thread_getstatus:
 *
 * Get the status of the specified thread.
 */

kern_return_t 
act_machine_get_state(
		      thread_act_t           thr_act,
		      thread_flavor_t        flavor,
		      thread_state_t         tstate,
		      mach_msg_type_number_t *count)
{
#if	MACH_ASSERT
    if (watchacts & WA_STATE)
	printf("act_%x act_m_get_state(thr_act=%x,flav=%x,st=%x,cnt@%x=%x)\n",
	       current_act(), thr_act, flavor, tstate,
	       count, (count ? *count : 0));
#endif	/* MACH_ASSERT */

    switch (flavor) {
    case THREAD_STATE_FLAVOR_LIST:
	if (*count < 3)
	    return (KERN_INVALID_ARGUMENT);

	tstate[0] = HP700_THREAD_STATE;
	tstate[1] = HP700_FLOAT_STATE;
	tstate[2] = THREAD_SYSCALL_STATE;
	*count = 3;

	return KERN_SUCCESS;

    case THREAD_SYSCALL_STATE:
    {
	register struct thread_syscall_state *state;
	register struct hp700_saved_state   *saved_state = USER_REGS(thr_act);

	if (*count < HP700_SYSCALL_STATE_COUNT)
	    return KERN_INVALID_ARGUMENT;

	state = (struct thread_syscall_state *) tstate;

	state->r1  = saved_state->r1;
	state->rp  = saved_state->rp;
	state->r3  = saved_state->r3;
	state->t1 = saved_state->t1;
	state->arg3 = saved_state->arg3;
	state->arg2 = saved_state->arg2;
	state->arg1 = saved_state->arg1;
	state->arg0 = saved_state->arg0;
	state->dp = saved_state->dp;
	state->ret0 = saved_state->ret0;
	state->ret1 = saved_state->ret1;
	state->sp = saved_state->sp;
    
	state->iioq_head = saved_state->iioq_head;
	state->iioq_tail = saved_state->iioq_tail;
	
	*count = HP700_SYSCALL_STATE_COUNT;
	return KERN_SUCCESS;
    }

    case HP700_THREAD_STATE:
    {
	register struct hp700_thread_state *state;
	register struct hp700_saved_state  *saved_state = USER_REGS(thr_act);

	if (*count < HP700_THREAD_STATE_COUNT)
	    return KERN_INVALID_ARGUMENT;

	state = (struct hp700_thread_state *) tstate;
	state->flags = saved_state->flags;
	state->r1  = saved_state->r1;
	state->rp  = saved_state->rp;
	state->r3  = saved_state->r3;
	state->r4  = saved_state->r4;
	state->r5  = saved_state->r5;
	state->r6  = saved_state->r6;
	state->r7  = saved_state->r7;
	state->r8  = saved_state->r8;
	state->r9  = saved_state->r9;
	state->r10 = saved_state->r10;
	state->r11 = saved_state->r11;
	state->r12 = saved_state->r12;
	state->r13 = saved_state->r13;
	state->r14 = saved_state->r14;
	state->r15 = saved_state->r15;
	state->r16 = saved_state->r16;
	state->r17 = saved_state->r17;
	state->r18 = saved_state->r18;
	state->t4 = saved_state->t4;
	state->t3 = saved_state->t3;
	state->t2 = saved_state->t2;
	state->t1 = saved_state->t1;
	state->arg3 = saved_state->arg3;
	state->arg2 = saved_state->arg2;
	state->arg1 = saved_state->arg1;
	state->arg0 = saved_state->arg0;
	state->dp = saved_state->dp;
	state->ret0 = saved_state->ret0;
	state->ret1 = saved_state->ret1;
	state->sp = saved_state->sp;
	state->r31 = saved_state->r31;
    
	state->sr0 = saved_state->sr0;
	state->sr1 = saved_state->sr1;
	state->sr2 = saved_state->sr2;
	state->sr3 = saved_state->sr3;
	state->sr4 = saved_state->sr4;
	state->sr5 = saved_state->sr5;
	state->sr6 = saved_state->sr6;

	state->rctr = saved_state->rctr;
	state->pidr1 = saved_state->pidr1;
	state->pidr2 = saved_state->pidr2;
	state->ccr = saved_state->ccr;
	state->sar = saved_state->sar;
	state->pidr3 = saved_state->pidr3;
	state->pidr4 = saved_state->pidr4;

	state->iioq_head = saved_state->iioq_head;
	state->iioq_tail = saved_state->iioq_tail;
	
	state->iisq_head = saved_state->iisq_head;
	state->iisq_tail = saved_state->iisq_tail;

	state->fpu = saved_state->fpu;

	/* 
	 * Only export the meaningful bits of the psw
	 */
	state->ipsw = saved_state->ipsw & 
	    (PSW_R | PSW_X | PSW_T | PSW_N | PSW_B | PSW_V | PSW_CB);

	*count = HP700_THREAD_STATE_COUNT;
	return KERN_SUCCESS;
    }

    case HP700_FLOAT_STATE:
    {
	register struct hp700_float_state *state;
	register struct hp700_float_state *saved_state = USER_FREGS(thr_act);

	if (*count < HP700_FLOAT_STATE_COUNT)
	    return KERN_INVALID_ARGUMENT;

	state = (struct hp700_float_state *) tstate;

	if (fpu_pcb == thr_act->mact.pcb)
		fpu_flush();

	if(thr_act->mact.pcb->ss.fpu) {
		assert(thr_act->mact.pcb->ss.fpu == 1);
		bcopy((char *)saved_state, (char *)state, sizeof(struct hp700_float_state));
	}
	else 
		bzero((char*)state, sizeof(struct hp700_float_state));

	*count = HP700_FLOAT_STATE_COUNT;
	return KERN_SUCCESS;
    }
    default:
	return KERN_INVALID_ARGUMENT;
    }
}


/*
 * thread_setstatus:
 *
 * Set the status of the specified thread.
 */
kern_return_t 
act_machine_set_state(
		      thread_act_t	     thr_act,
		      thread_flavor_t	     flavor,
		      thread_state_t	     tstate,
		      mach_msg_type_number_t count)
{
    int	kernel_act = thr_act->kernel_loading ||	thr_act->kernel_loaded;

#if	MACH_ASSERT
    if (watchacts & WA_STATE)
	printf("act_%x act_m_set_state(thr_act=%x,flav=%x,st=%x,cnt=%x)\n",
	       current_act(), thr_act, flavor, tstate, count);
#endif	/* MACH_ASSERT */

    switch (flavor) {
    case THREAD_SYSCALL_STATE:
    {
	register struct thread_syscall_state *state;
	register struct hp700_saved_state   *saved_state = USER_REGS(thr_act);

	state = (struct thread_syscall_state *)tstate;

	saved_state->r1 = state->r1;
	saved_state->rp = state->rp;
	saved_state->r3 = state->r3;
	saved_state->t1 = state->t1;
	saved_state->arg3 = state->arg3;
	saved_state->arg2 = state->arg2;
	saved_state->arg1 = state->arg1;
	saved_state->arg0 = state->arg0;
	saved_state->dp = state->dp;
	saved_state->ret0 = state->ret0;
	saved_state->ret1 = state->ret1;
	saved_state->sp = state->sp;
	if(kernel_act)
	{
	    saved_state->iioq_head = state->iioq_head;
	    saved_state->iioq_tail = state->iioq_tail;
	}
	else
	{
	    saved_state->iioq_head = state->iioq_head | PC_PRIV_USER;
	    saved_state->iioq_tail = state->iioq_tail | PC_PRIV_USER;
	}

	return KERN_SUCCESS;
    }
    case HP700_THREAD_STATE:
    {
	register struct hp700_thread_state *state;
	register struct hp700_saved_state  *saved_state = USER_REGS(thr_act);

	state = (struct hp700_thread_state *)tstate;

	if(state->flags & (SS_INSYSCALL|SS_INTRAP))
		saved_state->flags = state->flags;
	saved_state->r1 = state->r1;
	saved_state->rp = state->rp;
	saved_state->r3 = state->r3;
	saved_state->r4 = state->r4;
	saved_state->r5 = state->r5;
	saved_state->r6 = state->r6;
	saved_state->r7 = state->r7;
	saved_state->r8 = state->r8;
	saved_state->r9 = state->r9;
	saved_state->r10 = state->r10;
	saved_state->r11 = state->r11;
	saved_state->r12 = state->r12;
	saved_state->r13 = state->r13;
	saved_state->r14 = state->r14;
	saved_state->r15 = state->r15;
	saved_state->r16 = state->r16;
	saved_state->r17 = state->r17;
	saved_state->r18 = state->r18;
	saved_state->t4 = state->t4;
	saved_state->t3 = state->t3;
	saved_state->t2 = state->t2;
	saved_state->t1 = state->t1;
	saved_state->arg3 = state->arg3;
	saved_state->arg2 = state->arg2;
	saved_state->arg1 = state->arg1;
	saved_state->arg0 = state->arg0;
	saved_state->dp = state->dp;
	saved_state->ret0 = state->ret0;
	saved_state->ret1 = state->ret1;
	saved_state->sp = state->sp;
	saved_state->r31 = state->r31;
	saved_state->sar = state->sar;

	if(kernel_act) {
	    saved_state->iioq_head = state->iioq_head;
	    saved_state->iioq_tail = state->iioq_tail;
	}
	else {
	    saved_state->iioq_head = state->iioq_head | PC_PRIV_USER;
	    saved_state->iioq_tail = state->iioq_tail | PC_PRIV_USER;
	}
	
	saved_state->iisq_head = state->iisq_head;
	saved_state->iisq_tail = state->iisq_tail;

	saved_state->sr0 = state->sr0;
	saved_state->sr1 = state->sr1;
	saved_state->sr2 = state->sr2;
	saved_state->sr3 = state->sr3;

	saved_state->fpu = state->fpu;

	/*
	 * Make sure only PSW_T, PSW_X, PSW_N, PSW_B, PSW_V and PSW_CB 
	 * bits are set by users.
	 */
	saved_state->ipsw = state->ipsw &
		(PSW_R | PSW_T | PSW_X | PSW_N | PSW_B | PSW_V | PSW_CB);

	/*
	 * Always make sure that PSW_C, PSW_Q, PSW_P, PSW_D are set. The PSW_I
	 * bit is set according to eiem at context switch.
	 */
	saved_state->ipsw |= PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I;

	/*
	 * if single step, set the count to 0 so that 1 instruction
         * is excecuted before trapping.
         */
	if(saved_state->ipsw & PSW_R)
		saved_state->rctr = 0;

	return KERN_SUCCESS;
    }
    case HP700_FLOAT_STATE:
    {
	register struct hp700_float_state *state;
	register struct hp700_float_state *saved_state = USER_FREGS(thr_act);

	state = (struct hp700_float_state *) tstate;

	if (fpu_pcb == thr_act->mact.pcb)
		fpu_pcb = 0;	/* must restore coprocessor */

	bcopy((char *)state, (char *)saved_state,
	      sizeof(struct hp700_float_state));
	thr_act->mact.pcb->ss.fpu = 1;
	
	return KERN_SUCCESS;
    }
    default:
	return KERN_INVALID_ARGUMENT;
    }
}
