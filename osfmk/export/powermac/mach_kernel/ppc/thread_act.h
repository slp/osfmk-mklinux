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

#ifndef	_PPC_THREAD_ACT_H_
#define	_PPC_THREAD_ACT_H_

#include <mach_kgdb.h>
#include <mach/boolean.h>
#include <mach/ppc/vm_types.h>
#include <mach/thread_status.h>
#include <kern/lock.h>


/*
 * Kernel state structure
 *
 * This holds the kernel state that is saved and restored across context
 * switches. This is kept at the top of the kernel stack.
 *
 * XXX Some state is saved only because it is not saved on entry to the
 * kernel from user mode. This needs to be straightened out.
 */

/*
 * PPC process control block
 *
 * In the continuation model, the PCB holds the user context. It is used
 * on entry to the kernel from user mode, either by system call or trap,
 * to store the necessary user registers and other state.
 */
struct pcb
{
	struct ppc_saved_state ss;
	struct ppc_exception_state es;
	struct ppc_float_state fs;
	unsigned ksp;			/* points to TOP OF STACK or zero */
	unsigned sr0;		/* TODO NMGS hack?? sort out user space */
};

typedef struct pcb *pcb_t;

/*
 * ppc_kernel_state is used to switch between two kernel contexts in
 * a cooperative manner. Since we always rest in the kernel context,
 * we do not need to save/restore the kernel GOT pointer (r2)
 */

struct ppc_kernel_state
{
  	pcb_t	 pcb;		/* Efficiency hack, I am told :-) */
	unsigned r1;		/* Stack pointer */
	unsigned r13[32-13];	/* Callee saved `non-volatile' registers */
	unsigned lr;		/* `return address' used in continuation */
	unsigned cr;
				/* Floating point not used, so not saved */
};

/*
 * Maps state flavor to number of words in the state:
 */
extern unsigned int state_count[];

#define USER_REGS(ThrAct)	(&(ThrAct)->mact.pcb->ss)

#define	user_pc(ThrAct)		((ThrAct)->mact.pcb->ss.srr0)

#define act_machine_state_ptr(ThrAct)	(thread_state_t)USER_REGS(ThrAct)

typedef struct MachineThrAct {
	/*
	 * pointer to process control block
	 *	(actual storage may as well be here, too)
	 */
	struct pcb xxx_pcb;
	pcb_t pcb;

} MachineThrAct, *MachineThrAct_t;

#endif	/* _PPC_THREAD_ACT_H_ */
