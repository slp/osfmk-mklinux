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

#include <mach.h>
#include <threads.h>
#include <tgdb.h>
#include <mach/mach_traps.h>

static vm_offset_t stack_alloc(vm_size_t);

struct mach_thread threads[MAX_THREADS];

boolean_t threads_initialized = FALSE;

/*
 * Setup the first thread.
 *	Register its stack and address.
 */

void
threads_init(void) 
{
  	int i;

	if (threads_initialized)
		return;	
	threads[0].thread = mach_thread_self();
	threads[0].stack = thread_stack(&i);
	set_thread_self(&threads[0]);
	threads_initialized = TRUE;
}

thread_port_t 
tgdb_thread_create(vm_offset_t entry)
{
	kern_return_t	rc;
	mach_thread_t th;
	int i;

	for (th = &threads[0], i=0; i<MAX_THREADS; i++, th++)
		if (th->thread == MACH_PORT_NULL)
			break;

	if((rc = thread_create(mach_task_self(), &th->thread)) != KERN_SUCCESS) {
		printf("tgdb: thread_create returned %d\n", rc);
		return 0;
	}

	th->stack = stack_alloc(STACK_SIZE);
	set_thread_self(th);

	thread_set_regs(entry, th);

	return th->thread;
}

static vm_offset_t
stack_alloc(vm_size_t size)
{
	vm_offset_t addr;
	vm_offset_t stack;
	vm_size_t unused;
	kern_return_t rc;

	if((rc = vm_allocate(mach_task_self(),
				  &addr,
				  2*STACK_SIZE,
				  TRUE)) != KERN_SUCCESS)
	{
		printf("stack_alloc: vm_allocate failed\n");
		return 0;
	}

	stack = thread_stack(addr+size-1);

	if ((unused = stack - addr)) {
		if((rc = vm_deallocate(mach_task_self(),
				  addr,
				  unused)) != KERN_SUCCESS) {
			printf("stack_alloc: vm_allocate failed\n");
			return 0;
		}
	}

	if ((unused = addr + size - stack)) {
		if((rc = vm_deallocate(mach_task_self(),
				  stack + size,
				  unused)) != KERN_SUCCESS) {
			printf("stack_alloc: vm_allocate failed\n");
			return 0;
		}
	}

	return(stack);
}







