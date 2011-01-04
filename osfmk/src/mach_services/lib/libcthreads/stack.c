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
 * Revision 2.13.1.1  92/06/22  11:50:28  rwd
 * 	Update to reflect movement of cproc globals into cthread_status.
 * 	[92/02/10            rwd]
 * 
 * Revision 2.13  92/01/14  16:48:54  rpd
 * 	Fixed addr_range_check to deallocate the object port from vm_region.
 * 	[92/01/14            rpd]
 * 
 * Revision 2.12  92/01/03  20:37:10  dbg
 * 	Export cthread_stack_size, and use it if non-zero instead of
 * 	probing the stack.  Fix error in deallocating unused initial
 * 	stack (STACK_GROWTH_UP case).
 * 	[91/08/28            dbg]
 * 
 * Revision 2.11  91/07/31  18:39:34  dbg
 * 	Fix some bad stack references (stack direction).
 * 	[91/07/30  17:36:50  dbg]
 * 
 * Revision 2.10  91/05/14  17:58:49  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/02/14  14:21:08  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:41:35  mrt]
 * 
 * Revision 2.8  90/11/05  18:10:46  rpd
 * 	Added cproc_stack_base.  Add stack_fork_child().
 * 	[90/11/01            rwd]
 * 
 * Revision 2.7  90/11/05  14:37:51  rpd
 * 	Fixed addr_range_check for new vm_region semantics.
 * 	[90/11/02            rpd]
 * 
 * Revision 2.6  90/10/12  13:07:34  rpd
 * 	Deal with positively growing stacks.
 * 	[90/10/10            rwd]
 * 	Deal with initial user stacks that are not perfectly aligned.
 * 	[90/09/26  11:51:46  rwd]
 * 
 * 	Leave extra stack page around in case it is needed before we
 * 	switch stacks.
 * 	[90/09/25            rwd]
 * 
 * Revision 2.5  90/08/07  14:31:46  rpd
 * 	Removed RCS keyword nonsense.
 * 
 * Revision 2.4  90/06/02  15:14:18  rpd
 * 	Moved cthread_sp to machine-dependent files.
 * 	[90/04/24            rpd]
 * 	Converted to new IPC.
 * 	[90/03/20  20:56:35  rpd]
 * 
 * Revision 2.3  90/01/19  14:37:34  rwd
 * 	Move self pointer to top of stack
 * 	[89/12/12            rwd]
 * 
 * Revision 2.2  89/12/08  19:49:52  rwd
 * 	Back out change from af.
 * 	[89/12/08            rwd]
 * 
 * Revision 2.1.1.3  89/12/06  12:54:17  rwd
 * 	Gap fix from af
 * 	[89/12/06            rwd]
 * 
 * Revision 2.1.1.2  89/11/21  15:01:40  rwd
 * 	Add RED_ZONE ifdef.
 * 	[89/11/20            rwd]
 * 
 * Revision 2.1.1.1  89/10/24  13:00:44  rwd
 * 	Remove conditionals.
 * 	[89/10/23            rwd]
 * 
 * Revision 2.1  89/08/03  17:10:05  rwd
 * 	Created.
 * 
 * 18-Jan-89  David Golub (dbg) at Carnegie-Mellon University
 * 	Altered for stand-alone use:
 * 	use vm_region to probe for the bottom of the initial thread's
 * 	stack.
 * 
 *
 * 01-Dec-87  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Changed cthread stack allocation to use aligned stacks
 * 	and store self pointer at base of stack.
 * 	Added inline expansion for cthread_sp() function.
 */
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
/*
 * 	File: 	stack.c
 *	Author:	Eric Cooper, Carnegie Mellon University
 *	Date:	Dec, 1987
 *
 * 	C Thread stack allocation.
 *
 */

#include <cthreads.h>
#include "cthread_internals.h"
#include <mach/vm_region.h>
#include <mach/task_special_ports.h>
#include <mach/machine/vm_param.h>
#include <mach/mach_host.h>
#include <mach/bootstrap.h>
#include <stdio.h>

/*
 *			Cthread Stacks
 *
 * Set up a stack segment for a cthread.  If the compile time option RED_ZONE
 * is defined, then the stack segment has an invalid page allocated for early
 * detection of stack overflow.
 *
 * If the compile time option STACK_GROWTH_UP is defined, the stack is set up
 * to accomodate stack growth up instead of down. 
 *
 * The cthread_self pointer is stored at the top of the stack
 *
 *	--------- (stack base)
 *	| self	|
 *	|  ...	|
 *	|	|
 *	| stack	|
 *	|	|
 *	|  ...	|
 *	|	|
 *	---------
 *	|	|
 *	|invalid|
 *	|	|
 *	--------- (stack base)
 *	--------- (low address)
 *
 * or the reverse, if the stack grows up.
 */

/* Global variables. */
vm_size_t cthread_stack_size;           /* If non-zero the size of cthread */
                                        /* stacks. */
vm_size_t cthread_red_zone_size;        /* If non-zero the size of red_zone */
private vm_size_t access_stack_size;    /* Accessible size of stack (allocated
					 * size of stack minus red_zone) 
					 * This is set by program linking
					 * to cthreads, must be a multiple
					 * of page size */

int cthread_stack_chunk_count = 8;
private boolean_t red_zone = FALSE;

/* Externals */
extern char	etext,end;

/* Private variables. */
private vm_address_t next_stack_base;    /* The next address to use for stack */
					 /* allocation. */

private vm_address_t stack_pool;
private spin_lock_t stack_pool_lock = SPIN_LOCK_INITIALIZER;


#ifndef VM_MAX_KERNEL_LOADED_ADDRESS
#define VM_MAX_KERNEL_LOADED_ADDRESS \
	(VM_MIN_KERNEL_LOADED_ADDRESS + (128 * 1024 * 1024))
#endif	/* VM_MAX_KERNEL_LOADED_ADDRESS */

/* Local static function prototypes. */
staticf void probe_stack(vm_offset_t *stack_bottom, vm_offset_t *stack_top);
staticf void setup_stack(cthread_t, vm_address_t);
staticf vm_offset_t addr_range_check(vm_offset_t start_addr,
				     vm_offset_t end_addr,
				     vm_prot_t desired_protection);
staticf void size_stack(void);


/*
 * Function: setup_stack - Setup a cthread's stack.
 *
 * Description:
 *	Set the cthread's stack values.  
 *
 * Arguments:
 *	p - The cthread.
 *	base - The base of the cthread's stack.
 *
 * Return Value:
 *	None.
 * 
 * Comments:
 */

void 
setup_stack(p, base)
	register cthread_t p;
	register vm_address_t base;
{
	p->stack_base = base;
	/*
	 * Stack size is segment size minus size of self pointer
	 */
	p->stack_size = cthread_status.stack_size;
	/*
	 * Store self pointer.
	 */
	*(cthread_t *)&_cthread_ptr(base) = p;
}

/*
 * Function: addr_range_check - Check the validity of an address range.
 *
 * Description:
 *
 * Arguments:
 *	start_addr - The start of the address range.
 *	end_addr - The end of the address range.
 *	desired_protection - The desired protection on the address range.
 *
 * Return Value:
 * 	If the address range is not valid, 0 is returned.  If it is valid, the
 * end of the valid range is returned.
 */

vm_offset_t
addr_range_check(start_addr, end_addr, desired_protection)
	vm_offset_t	start_addr, end_addr;
	vm_prot_t	desired_protection;
{
	register vm_offset_t	addr;

	addr = start_addr;
	while (addr < end_addr) {
	    vm_offset_t		r_addr;
	    vm_size_t		r_size;
	    kern_return_t	kr;
	    natural_t   	count;
	    memory_object_name_t	r_object_name;
	    vm_region_basic_info_data_t	info;

	    r_addr = addr;
	    count = VM_REGION_BASIC_INFO_COUNT;
	    kr = vm_region(mach_task_self(), &r_addr, &r_size,
			   VM_REGION_BASIC_INFO, (vm_region_info_t) &info, 
			   &count, &r_object_name);
	    if ((kr == KERN_SUCCESS) && MACH_PORT_VALID(r_object_name))
		(void) mach_port_deallocate(mach_task_self(), r_object_name);

	    if ((kr != KERN_SUCCESS) ||
		(r_addr > addr) ||
		((info.protection & desired_protection) != desired_protection))
		return (0);
	    if (r_addr < start_addr)
		start_addr = r_addr;
	    addr = r_addr + r_size;
	}
#ifdef	STACK_GROWTH_UP
	addr = start_addr;
#endif	/* STACK_GROWTH_UP */
	return (addr);
}

/*
 * Function: probe_stack - Probe for the top and bottom of the stack.
 *
 * Description:
 *
 * Arguments:
 * 	stack_bottom - OUT - The bottom of the area found for the stack.
 * 	stack_top - OUT - The top of the area found for the stack.
 *
 * Return Value:
 *	None
 *
 * Comments:
 * 	This code assumes that stacks grow down and that there is an
 * unallocated region below the stack.
 *
 * Assume:
 * 1. There is an unallocated region below the stack.
 */


void
probe_stack(stack_bottom, stack_top)
	vm_offset_t	*stack_bottom;
	vm_offset_t	*stack_top;
{
	/*
	 * Since vm_region returns the region starting at
	 * or ABOVE the given address, we cannot use it
	 * directly to search downwards.  However, we
	 * also want a size that is the closest power of
	 * 2 to the stack size (so we can mask off the stack
	 * address and get the stack base).  So we probe
	 * in increasing powers of 2 until we find a gap
	 * in the stack.
	 */
	vm_offset_t	start_addr, end_addr;
	vm_offset_t	last_start_addr, last_end_addr;
	vm_size_t	stack_size;

	/*
	 * Start with a page
	 */
	start_addr = (vm_offset_t)cthread_sp() & ~(vm_page_size - 1);
	end_addr   = start_addr + vm_page_size;

	stack_size = vm_page_size;

	/*
	 * Increase the tentative stack size, by doubling each
	 * time, until we have exceeded the stack (some of the
	 * range is not valid).
	 */
	do {
	    /*
	     * Save last addresses
	     */
	    last_start_addr = start_addr;
	    last_end_addr   = end_addr;

	    /*
	     * Double the stack size
	     */
	    stack_size <<= 1;
#ifdef	STACK_GROWTH_UP
	    end_addr = start_addr + stack_size;
#else	/* STACK_GROWTH_UP */
	    start_addr = end_addr - stack_size;
#endif	/* STACK_GROWTH_UP */

	    /*
	     * Check that the entire range exists and is writable
	     */
	} while ((
#if	STACK_GROWTH_UP
		  start_addr
#else	/* STACK_GROWTH_UP */
		  end_addr
#endif	/* STACK_GROWTH_UP */
		  = (addr_range_check(start_addr,
				      end_addr,
				      (VM_PROT_READ|VM_PROT_WRITE)))));
	/*
	 * Back off to previous power of 2.
	 */
	*stack_bottom = last_start_addr;
	*stack_top = last_end_addr;
}

/*
 * Function: stack_init - Initialize the cthread stack structures.
 *
 * Description:
 *	This function finds a location for the cthread stacks and then
 * initializes the global cthread stack structures.  In addition, the stack for
 * the initial cthread is set.
 *
 * Arguments:
 *	p - The initial cthread.
 *	newstack - IN/OUT - If 0, then the initial cthread continues to use the
 *		current stack.  If non-zero, then the initial cthread get a new
 * 		stack and the new stack address is returned.
 *
 * Return Value:
 *	None.
 */

void
stack_init(cthread_t p, vm_offset_t *newstack)
{

	vm_offset_t	stack_bottom,
			stack_top,
			start;
	vm_size_t	size;
	kern_return_t	r;

	in_kernel = IN_KERNEL(&etext);

	/*
	 * If we're running in a kernel-loaded application, force thread
	 * stacks to be small (but don't overrule our client).
	 */
	if (in_kernel && cthread_stack_size == 0)
		cthread_stack_size = IN_KERNEL_STACK_SIZE;

	/*
	 * Probe for bottom and top of stack, as a power-of-2 size.
	 */
	probe_stack(&stack_bottom, &stack_top);

	/*
	 * Use the stack size found for the Cthread stack size,
	 * if not already specified.
	 */
	if (cthread_stack_size == 0)
	    access_stack_size = stack_top - stack_bottom;
	else
	    access_stack_size = cthread_stack_size;

	/* 
	 * This routine will re-size cthread_red_zone_size if 
	 * needed to get the stack alligned properly
	 */
	size_stack();
	cthread_status.stack_size = access_stack_size + 
				cthread_red_zone_size;


    /* Set the stack mask based on the stack growth direction. */
#ifdef	STACK_GROWTH_UP
	cthread_status.stack_mask = ~(cthread_status.stack_size - 1);
#else	/*STACK_GROWTH_UP*/
	cthread_status.stack_mask = cthread_status.stack_size - 1;
#endif	/*STACK_GROWTH_UP*/

	if (cthread_stack_chunk_count <= 0)
	    cthread_stack_chunk_count = 1;

	/*
	 * Guess at first available region for stack.
	 */
	next_stack_base = cthread_status.stack_size;

	if (in_kernel)
#ifdef	STACK_GROWTH_UP
		next_stack_base = (vm_address_t)(&end + next_stack_base) &
			cthread_status.stack_mask;
#else	/* STACK_GROWTH_UP */
		next_stack_base = (vm_address_t)(&end + next_stack_base) &
			~cthread_status.stack_mask;
#endif	/* STACK_GROWTH_UP */

	/*
	 * See if we want to run on old stack.
	 */
	if (newstack == (vm_offset_t *) 0) {
 	    if (cthread_stack_size != stack_top - stack_bottom) {
#ifdef	STACK_GROWTH_UP
		start = stack_bottom + cthread_stack_size;
		size = stack_top - start;
		stack_top = start;
#else	/* STACK_GROWTH_UP */
		start = stack_bottom;
		stack_bottom = stack_top - cthread_stack_size;
		size = stack_bottom - start;
#endif	/* STACK_GROWTH_UP */
		MACH_CALL(vm_deallocate(mach_task_self(), start, size), r);
	    }
	    setup_stack(p, stack_bottom);
	    return;
	}

	/*
	 * Set up stack for main thread.
	 */
	alloc_stack(p);

	/*
	 * Delete rest of old stack.
	 */

#ifdef	STACK_GROWTH_UP
	start = ((vm_offset_t)cthread_sp() | (vm_page_size - 1)) + 1 + vm_page_size;
	size = stack_top - start;
#else	/*STACK_GROWTH_UP*/
	start = stack_bottom;
	size = ((vm_offset_t)cthread_sp() & ~(vm_page_size - 1)) - 
		stack_bottom - vm_page_size;
#endif	/*STACK_GROWTH_UP*/
	MACH_CALL(vm_deallocate(mach_task_self(),start,size),r);

	/*
	 * Return new stack; 
	 */
	*newstack = cthread_stack_base(p, CTHREAD_STACK_OFFSET);
}

/*
 * Here we will examine various stack size paremeters and make 
 * sure they make over all sense.
 * This involves taking cthread_red_zone_size and if set turning on 
 * red_zone, then re-sizing cthread_red_zone_size to make sure 
 * the overall stack size is a power of 2.
 */
void
size_stack()
{
	if (cthread_red_zone_size > 0) {
		red_zone = TRUE;
		if (cthread_red_zone_size < access_stack_size) {
			cthread_red_zone_size = access_stack_size;
		} else {
			vm_size_t max_stack_size;
			int po2 = 0;

			max_stack_size = cthread_red_zone_size + 
					access_stack_size - 1;
			while (max_stack_size != 0) {
				max_stack_size = max_stack_size >> 1;
				po2++;
			}
			max_stack_size = 0x1 << po2;
			cthread_red_zone_size = max_stack_size - 
					access_stack_size;
		}
	}
}

#ifdef	WIRE_IN_KERNEL_STACKS

/*
 * Wire down a stack, on behalf of a kernel-loaded client (who
 * can't tolerate kernel exception handling on a pageable stack).
 */
void
stack_wire(vm_address_t base, vm_size_t length)
{

	static mach_port_t security_port = MACH_PORT_NULL;
	static mach_port_t root_ledger_wired = MACH_PORT_NULL;
	static mach_port_t root_ledger_paged = MACH_PORT_NULL;
	static mach_port_t priv_host_port = MACH_PORT_NULL;
	static mach_port_t unused_device_port = MACH_PORT_NULL;
	kern_return_t result;

	if (priv_host_port == MACH_PORT_NULL) {
		result = task_get_special_port(mach_task_self(),
			TASK_BOOTSTRAP_PORT, &bootstrap_port);
		if (result != KERN_SUCCESS) {
			fprintf_stderr("cthreads: task_get_special_port "
					"fails with result %x\n", result);
			exit(result);
		}

		result = bootstrap_ports(bootstrap_port, 
					&priv_host_port,
					&unused_device_port,
					&root_ledger_wired,
					&root_ledger_paged,
					&security_port);

		if (result != KERN_SUCCESS) {
			fprintf_stderr("cthreads: bootstrap_privileged_ports "
					"fails with result %x\n", result);
			exit(result);
		}
	}
	result = vm_wire(priv_host_port,
		mach_task_self(), base, length, (VM_PROT_READ|VM_PROT_WRITE));
	if (result != KERN_SUCCESS) {
		fprintf_stderr("cthreads: vm_wire "
				"fails with result %x\n", result);
		exit(result);
	}
}

#endif	/* WIRE_IN_KERNEL_STACKS */

/*
 * Function: alloc_stack - Allocate a stack segment for a thread.
 *
 * Description:
 *	Allocate memory for a cthread's stack, and setup the cthread to point
 * to the stack.
 *
 * Arguments:
 * 	p - The cthread to allocate a stack for.
 *
 * Return Value:
 *	None.
 *
 * Comments:
 *	The variable next_stack_base is used to align stacks. It may be updated
 * by several threads in parallel, but mutual exclusion is unnecessary: at
 * worst, the vm_allocate will fail and the thread will try again.
 * 
 *	Stacks are never deallocated.
 */

void
alloc_stack(p)
	cthread_t p;
{

	vm_address_t	base = next_stack_base;

	if (stack_pool) {
	    spin_lock(&stack_pool_lock);
	    if (stack_pool) {
		base = stack_pool;
		stack_pool = *(vm_address_t *)stack_pool;
		spin_unlock(&stack_pool_lock);
		goto got_stack;
	    }
	    spin_unlock(&stack_pool_lock);
	}
		
	for (base = next_stack_base;
	     base < VM_MAX_KERNEL_LOADED_ADDRESS  &&
	     vm_allocate(mach_task_self(), 
			 &base, 
			 cthread_status.stack_size * cthread_stack_chunk_count, 
			 FALSE) != KERN_SUCCESS;
	     base += cthread_status.stack_size*cthread_stack_chunk_count);
	if (base >= VM_MAX_KERNEL_LOADED_ADDRESS) {
		fprintf_stderr("cthreads: alloc_stack fails\n");
		exit(1);
	}
	next_stack_base = base + cthread_status.stack_size*cthread_stack_chunk_count;
#ifdef	WIRE_IN_KERNEL_STACKS
	/*
	 * If we're running in a kernel-loaded application, wire the stack
	 * we just allocated.
	 */
	if (in_kernel)
		stack_wire(base, cthread_status.stack_size*cthread_stack_chunk_count);
#endif	/* WIRE_IN_KERNEL_STACKS */
        {
	    int i;
	    vm_address_t *loop = (vm_address_t *)base;
	    for(i=1;i<cthread_stack_chunk_count;i++) {
		loop = (vm_address_t *)((int)loop + cthread_status.stack_size);
		*loop = (int)loop + cthread_status.stack_size;
	    }
	    if (loop != (vm_address_t *)base) {
		spin_lock(&stack_pool_lock);
		*loop = stack_pool;
		stack_pool = base + cthread_status.stack_size;
		spin_unlock(&stack_pool_lock);
	    }
	}
	
      got_stack:

	/*
	 * Protect the red zone at far end of stack:
         */
	if (red_zone) {
		kern_return_t   r;
		vm_address_t red_zone_base;

#ifdef	STACK_GROWTH_UP
		red_zone_base = base + access_stack_size;
#else	/*STACK_GROWTH_UP*/
		red_zone_base = base;
#endif	/*STACK_GROWTH_UP*/
		MACH_CALL(vm_protect(mach_task_self(), 
					red_zone_base,
					cthread_red_zone_size,
					FALSE, VM_PROT_NONE), r);
	}

	setup_stack(p, base);
}

/*
 * Function: stack_fork_child - Reset the stack base after a fork.
 *
 * Called in the child after a fork().  Resets stack data structures to
 * coincide with the reality that we now have a single cthread.
 *
 *
 * Arguments:
 *	None
 *
 * Return Value:
 * 	None.
 */


void stack_fork_child()
{
    next_stack_base = 0;
}
