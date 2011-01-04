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
/*
 * Mach Operating System
 * Copyright (c) 1989 Carnegie Mellon University.
 * Copyright (c) 1990,1991 The University of Utah and
 * the Center for Software Science (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
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
 *
 * 	Utah $Hdr$
 */

#include <cthreads.h>
#include "cthread_internals.h"

#include <mach.h>
#include <mach/mach_traps.h>

/*
 * Set up the initial state of a MACH thread so that it will invoke
 * routine(child) when it is resumed.
 */
void
cthread_setup(cthread_t child, thread_port_t thread, cthread_fn_t routine)
{
	int *top = (int *) (child->stack_base + child->stack_size);
	struct ppc_thread_state state;
	kern_return_t r;
	unsigned size;

	/*
	 * Set up ppc call frame and registers.
	 */
	size = PPC_THREAD_STATE_COUNT;
	MACH_CALL(thread_get_state(thread, PPC_THREAD_STATE, 
				   (thread_state_t) &state, &size), r);

	/*
	 * set the PC to point to routine.
	 */
	state.srr0 = (unsigned)routine;

	/*
	 * setup the first argument to routine to be the address of child.
	 */
	state.r3 = (int) child;

	/*
	 * establish a user stack for this thread. There is no guarentee
	 * that the stack is aligned correctly. We have to align it to
	 * a double word boundary.  (In fact, if CTHREAD_STACK_OFFSET
	 * is aligned, then so is the stack.  But little things can go
	 * wrong.)
	 */
	top = (int *)((int)top - CTHREAD_STACK_OFFSET);
	top -= 16; /* Top frame on stack */
	*top = 0;  /* Mark NULL backpointer */
	state.r1 = (unsigned int) top;

	MACH_CALL(thread_set_state(thread, PPC_THREAD_STATE,
				   (thread_state_t) &state,
				   PPC_THREAD_STATE_COUNT),r);
}

#ifdef	cthread_sp
#undef	cthread_sp
#endif

void *
cthread_sp(void)
{
	void *x;
#ifdef	__GNUC__
	/* GCC generates a warning if we return the address of a local
	 * variable, so we special-case this
	 */
	__asm__ ("mr %0,1" : "=r" (x));
	return x;
#else	/* __GNUCC__ */
	return (void *)&x;
#endif	/* __GNUCC__ */
}

boolean_t IN_KERNEL(void *addr /* NOT USED, TODO NMGS remove warning? */)
{
	kern_return_t kr;
	vm_address_t addr;

	/* return a cached value if already calculated */

	static boolean_t in_kernel_initialised  = FALSE;
	static boolean_t in_kernel_flag		= FALSE;

	if (in_kernel_initialised)
		return in_kernel_flag;

	/* Try to allocate, or protect page 0, if we can't then
	 * we must be collocated
	 */

	addr = 0;
	kr = vm_allocate(mach_task_self(), &addr, 1, FALSE);

	if (kr == KERN_SUCCESS) {
		/* We are probably a second server */

		vm_deallocate(mach_task_self(), addr, 1);
		in_kernel_flag = FALSE;
	} else {
		kr = vm_protect(mach_task_self(), (vm_address_t) 0, 1,
			FALSE, VM_PROT_NONE);

		if (kr == KERN_INVALID_ADDRESS) {
			/* page 0 is out of submap for collocated servers */
			in_kernel_flag = TRUE;
		} else {
			/* page was already allocated by bootstrap, but we
			 * can set access permissions, we're in user mode.
			 */
			in_kernel_flag = FALSE;
		}
	}

	in_kernel_initialised = TRUE;

	return in_kernel_flag;
}
