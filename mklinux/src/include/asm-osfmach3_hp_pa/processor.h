/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */


#ifndef _ASM_OSFMACH3_MACHINE_PROCESSOR_H
#define _ASM_OSFMACH3_MACHINE_PROCESSOR_H

#include <mach/machine/vm_param.h>

#define STACK_GROWTH_UP 1

#define TASK_SIZE ((unsigned long)0xc0000000)

static __inline__ void
osfmach3_state_to_ptregs(
	struct hp700_thread_state *state,
	struct pt_regs		 *regs)
{
	if (regs != (struct pt_regs *) state) {
		regs->state = *state;
	}
}

static __inline__ void
osfmach3_ptregs_to_state(
	struct pt_regs		 *regs,
	struct hp700_thread_state *state)
{
	if ((struct pt_regs *) state != regs) {
		*state = regs->state;
	}
}

#include <osfmach3/processor.h>
			    
#endif /* _ASM_OSFMACH3_MACHINE_PROCESSOR_H */


