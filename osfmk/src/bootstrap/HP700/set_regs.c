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

#include "bootstrap.h"

#include <mach/mach_host.h>
#include <mach/machine/vm_param.h>

#define	STACK_SIZE	(64*1024)

void
set_regs(mach_port_t host_port,
	 task_port_t user_task,
	 thread_port_t user_thread,
	 struct loader_info *lp,
	 unsigned long	mapend,
	 vm_size_t arg_size)
{
	vm_offset_t	stack_start;
	vm_offset_t	stack_end;
	struct hp700_thread_state regs;
	unsigned int	reg_size;

	/*
	 * Allocate stack.
	 */

	stack_end = mapend;

	if (!stack_end)
	    stack_end = USER_STACK_END;

	stack_start = stack_end - STACK_SIZE;

	(void)vm_allocate(user_task,
			  &stack_start,
			  (vm_size_t)STACK_SIZE,
			  FALSE);

	if (mapend)
		(void)vm_wire(host_port, user_task, stack_start,
			      (vm_size_t)STACK_SIZE,
			      VM_PROT_READ|VM_PROT_WRITE);

	reg_size = HP700_THREAD_STATE_COUNT;

	(void)thread_get_state(user_thread,
			       HP700_THREAD_STATE,
			       (thread_state_t)&regs,
			       &reg_size);

	regs.iioq_head = lp->entry_1;
	regs.iioq_tail = lp->entry_1 + 4;

	regs.dp = lp->entry_2; /* dp */

	/*
	 * put the argument list at the beginning of the stack
	 */

	regs.sp = ((stack_start + arg_size + 32) & ~(sizeof(int)-1));

	(void)thread_set_state(user_thread,
			       HP700_THREAD_STATE,
			       (thread_state_t)&regs,
			       reg_size);
}
