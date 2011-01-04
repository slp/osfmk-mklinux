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

#include <kern/thread.h>
#include <mach/ppc/thread_status.h>
#include <ppc/proc_reg.h>
#include <kern/misc_protos.h>
#include <ppc/exception.h>
#include <ppc/fpu_protos.h>
#include <ppc/misc_protos.h>
/*
 * Maps state flavor to number of words in the state:
 */
unsigned int state_count[] = {
	/* FLAVOR_LIST */ 0,
	PPC_THREAD_STATE_COUNT,
	PPC_FLOAT_STATE_COUNT,
	PPC_EXCEPTION_STATE_COUNT,
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
	printf("act_%x act_machine_get_state(thr_act=%x,flav=%x,st=%x,cnt@%x=%x)\n",
	       current_act(), thr_act, flavor, tstate,
	       count, (count ? *count : 0));
#endif	/* MACH_ASSERT */

    switch (flavor) {
    case THREAD_STATE_FLAVOR_LIST:
	if (*count < 3)
	    return (KERN_INVALID_ARGUMENT);

	tstate[0] = PPC_THREAD_STATE;
	tstate[1] = PPC_FLOAT_STATE;
	tstate[2] = PPC_EXCEPTION_STATE;
	*count = 3;

	return KERN_SUCCESS;

    case PPC_THREAD_STATE:
    {
	register struct ppc_thread_state *state;
	register struct ppc_saved_state  *saved_state = USER_REGS(thr_act);

	if (*count < PPC_THREAD_STATE_COUNT)
	    return KERN_INVALID_ARGUMENT;

	state = (struct ppc_thread_state *) tstate;

	state->r0	= saved_state->r0;
	state->r1	= saved_state->r1;
	state->r2	= saved_state->r2;
	state->r3	= saved_state->r3;
	state->r4	= saved_state->r4;
	state->r5	= saved_state->r5;
	state->r6	= saved_state->r6;
	state->r7	= saved_state->r7;
	state->r8	= saved_state->r8;
	state->r9	= saved_state->r9;
	state->r10	= saved_state->r10;
	state->r11	= saved_state->r11;
	state->r12	= saved_state->r12;
	state->r13	= saved_state->r13;
	state->r14	= saved_state->r14;
	state->r15	= saved_state->r15;
	state->r16	= saved_state->r16;
	state->r17	= saved_state->r17;
	state->r18	= saved_state->r18;
	state->r19	= saved_state->r19;
	state->r20	= saved_state->r20;
	state->r21	= saved_state->r21;
	state->r22	= saved_state->r22;
	state->r23	= saved_state->r23;
	state->r24	= saved_state->r24;
	state->r25	= saved_state->r25;
	state->r26	= saved_state->r26;
	state->r27	= saved_state->r27;
	state->r28	= saved_state->r28;
	state->r29	= saved_state->r29;
	state->r30	= saved_state->r30;
	state->r31	= saved_state->r31;

	state->cr	= saved_state->cr;
	state->xer	= saved_state->xer;
	state->lr	= saved_state->lr;
	state->ctr	= saved_state->ctr;
	state->srr0	= saved_state->srr0;
	/* 
	 * Only export the meaningful bits of the msr
	 */
	state->srr1 = MSR_REMOVE_SYSCALL_MARK(saved_state->srr1);

	state->mq	= saved_state->mq;	/* MQ register (601 only) */

	*count = PPC_THREAD_STATE_COUNT;
	return KERN_SUCCESS;
    }

    case PPC_EXCEPTION_STATE:
    {
	register struct ppc_exception_state *state;
	register struct ppc_exception_state *pcb_state = &thr_act->mact.pcb->es;

	if (*count < PPC_EXCEPTION_STATE_COUNT)
	    return KERN_INVALID_ARGUMENT;

	state = (struct ppc_exception_state *) tstate;

	bcopy((char *)pcb_state, (char *)state, sizeof(*state));

	*count = PPC_EXCEPTION_STATE_COUNT;
	return KERN_SUCCESS;
    }

    case PPC_FLOAT_STATE:
    {
	register struct ppc_float_state *state;
	register struct ppc_float_state *float_state = &thr_act->mact.pcb->fs;

	if (*count < PPC_FLOAT_STATE_COUNT)
	    return KERN_INVALID_ARGUMENT;

	fpu_save();

	state = (struct ppc_float_state *) tstate;

	bcopy((char *)float_state, (char *)state, sizeof(*state));

	*count = PPC_FLOAT_STATE_COUNT;
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
	printf("act_%x act_machine_set_state(thr_act=%x,flav=%x,st=%x,cnt=%x)\n",
	       current_act(), thr_act, flavor, tstate, count);
#endif	/* MACH_ASSERT */

    switch (flavor) {
    case PPC_THREAD_STATE:
    {
	register struct ppc_thread_state *state;
	register struct ppc_saved_state  *saved_state = USER_REGS(thr_act);

	state = (struct ppc_thread_state *)tstate;

	saved_state->r0		= state->r0;
	saved_state->r1		= state->r1;
	saved_state->r2		= state->r2;
	saved_state->r3		= state->r3;
	saved_state->r4		= state->r4;
	saved_state->r5		= state->r5;
	saved_state->r6		= state->r6;
	saved_state->r7		= state->r7;
	saved_state->r8		= state->r8;
	saved_state->r9		= state->r9;
	saved_state->r10	= state->r10;
	saved_state->r11	= state->r11;
	saved_state->r12	= state->r12;
	saved_state->r13	= state->r13;
	saved_state->r14	= state->r14;
	saved_state->r15	= state->r15;
	saved_state->r16	= state->r16;
	saved_state->r17	= state->r17;
	saved_state->r18	= state->r18;
	saved_state->r19	= state->r19;
	saved_state->r20	= state->r20;
	saved_state->r21	= state->r21;
	saved_state->r22	= state->r22;
	saved_state->r23	= state->r23;
	saved_state->r24	= state->r24;
	saved_state->r25	= state->r25;
	saved_state->r26	= state->r26;
	saved_state->r27	= state->r27;
	saved_state->r28	= state->r28;
	saved_state->r29	= state->r29;
	saved_state->r30	= state->r30;
	saved_state->r31	= state->r31;

	saved_state->cr		= state->cr;
	saved_state->xer	= state->xer;
	saved_state->lr		= state->lr;
	saved_state->ctr	= state->ctr;
	saved_state->srr0	= state->srr0;

	/* 
	 * Make sure that the MSR contains only user-importable info
	 */
	saved_state->srr1 	= MSR_PREPARE_FOR_IMPORT(saved_state->srr1,
							 state->srr1);

	/* User activations must not run privileged */
	if ((!kernel_act) && ((saved_state->srr1 & MASK(MSR_PR)) == 0)) {
		printf("WARNING - system privilage bit shouldn't be set "
		       "in user task - MSR=0x%08x\n",saved_state->srr1);
		saved_state->srr1 |= MASK(MSR_PR);
	}

	saved_state->mq		= state->mq;	/* MQ register (601 only) */

	return KERN_SUCCESS;
    }

    case PPC_EXCEPTION_STATE:
    {
	register struct ppc_exception_state *state;
	register struct ppc_exception_state *pcb_state = &thr_act->mact.pcb->es;
	state = (struct ppc_exception_state *) tstate;

	/* Store the state whole, no security problems */

	bcopy((char *)state, (char *)pcb_state, sizeof(*state));

	return KERN_SUCCESS;
    }

    case PPC_FLOAT_STATE:
    {
	register struct ppc_float_state *state;
	register struct ppc_float_state *float_state = &thr_act->mact.pcb->fs;

	state = (struct ppc_float_state *) tstate;

	bcopy((char *)state, (char *)float_state, sizeof(*state));
	if ((pcb_t)(per_proc_info[cpu_number()].fpu_pcb) == thr_act->mact.pcb)
		per_proc_info[cpu_number()].fpu_pcb = 0; /* must restore fpu */
	/* TODO NMGS - do we need to set fpscr to prevent latent traps? */
	return KERN_SUCCESS;
    }
    default:
	    return KERN_INVALID_ARGUMENT;
    }
}
