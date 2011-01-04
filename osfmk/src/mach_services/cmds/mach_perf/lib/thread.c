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
 * We use kernel threads instead of kernel threads to have better
 * control of allocation/deallocation of threads and stacks.
 */


#include <mach_perf.h>

boolean_t threads_initialized = FALSE;

/*
 * Setup the first thread.
 *	Register its stack and address.
 */

threads_init() 
{
  	int i;

	if (threads_initialized)
		return;	
	threads[0].thread = mach_thread_self();
	threads[0].stack = thread_stack(&i);
	set_thread_self(&threads[0]);
	if (debug)
		dump_mallocs(&threads[0]);
	threads_initialized = TRUE;
}

kern_return_t
new_thread(thread, pc, arg)
mach_thread_t *thread;
vm_offset_t pc;
vm_offset_t arg;
{
	register i;
	thread_state_t state;
	mach_thread_t th;
	unsigned int count = THREAD_STATE_COUNT;

	if (debug > 1)
		printf("new_thread()\n");
	for (th = &threads[0], i=0; i<MAX_THREADS; i++, th++)
		if (th->thread == MACH_PORT_NULL)
			break;

	if (i >= MAX_THREADS)
		return(KERN_FAILURE);
	
	MACH_CALL( thread_create, (mach_task_self(),
				  &th->thread));

	th->stack = stack_alloc();
	set_thread_self(th);

	MACH_CALL( vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&state,
				  THREAD_STATE_COUNT,
				  TRUE));

	if (debug > 1)
		printf("get_state()\n");
	MACH_CALL( thread_get_state, (th->thread, THREAD_STATE,
			       state, &count));
	thread_set_regs(state, pc, arg, th->stack, STACK_SIZE);
	if (debug > 1)
		printf("thread_set_state()\n");
	MACH_CALL( thread_set_state, (th->thread, THREAD_STATE,
			       state, THREAD_STATE_COUNT));
	if (debug > 1)
		printf("thread_resume()\n");
	MACH_CALL( thread_resume, (th->thread));

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t)state,
				  THREAD_STATE_COUNT));
	*thread = th;
}

kill_thread(thread)
mach_thread_t thread;
{
	register i;

	MACH_CALL(thread_suspend, (thread->thread));
	MACH_CALL(thread_terminate, (thread->thread));
	MACH_CALL(mach_port_destroy, (mach_task_self(), thread->thread));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  thread->stack,
				  STACK_SIZE));
	if (thread->mig_reply_port != MACH_PORT_NULL) {
		MACH_CALL(mach_port_destroy, (mach_task_self(),
					      thread->mig_reply_port));
		thread->mig_reply_port = MACH_PORT_NULL;
	}
	clean_mallocs(thread);
	thread->thread = MACH_PORT_NULL;
}

/*
 *	Initialize threads table just after a mach_fork()
 */

child_threads()
{
	mach_thread_t me,th;
	register i, j;
	
	me = thread_self();
	me->thread = mach_thread_self();
	for (i=0, th = &threads[0]; i < MAX_THREADS; i++, th++) {
		clean_mallocs(th);
		if (th == me)
		  	continue;
		if (th->thread != MACH_PORT_NULL) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  th->stack,
						  STACK_SIZE));
			th->thread = MACH_PORT_NULL;
			th->stack = 0;
		}
	}
}

stack_alloc()
{
	vm_offset_t addr;
	vm_offset_t stack;
	vm_size_t unused;
	vm_size_t size;

	/*
	 * Allocate stack aligned on a STACK_SIZE boundary.
	 */
	size = STACK_SIZE;
	MACH_CALL( vm_allocate, (mach_task_self(),
				  &addr,
				  2*STACK_SIZE,
				  TRUE));

	stack = thread_stack(addr+size-1);
	if (debug > 1)
		printf("addr %x, stack %x\n", addr, stack);
	if ((unused = stack - addr)) {
		if (debug > 1)
			printf("free %x bytes at %x\n", unused, addr);
		MACH_CALL( vm_deallocate, (mach_task_self(),
				  addr,
				  unused));
	}

	/*
	 *	Deallocate the upper unused memory.
	 *	Assumption: 2*STACK_SIZE - size == size
	 */
	if ((unused = addr + size - stack)) {
		if (debug > 1)
			printf("free %x bytes at %x\n", unused, stack + size);
		MACH_CALL( vm_deallocate, (mach_task_self(),
				  stack + size,
				  unused));
	}

	/*
	 * 	Wire down a stack, on behalf of a kernel-loaded client (who
	 * 	can't tolerate kernel exception handling on a pageable stack).
	 */
	if (in_kernel)
		MACH_CALL( vm_wire, (privileged_host_port,
				     mach_task_self(),
				     stack, size, (VM_PROT_READ|VM_PROT_WRITE)));

	return(stack);
}

void mig_init(first)
int	first;
{
	register i;

	if (first == 0) {
		for (i = 0; i < MAX_THREADS; i++)
			threads[i].mig_reply_port = MACH_PORT_NULL;
	}
}

mach_port_t
mig_get_reply_port()
{
	mach_thread_t th;
	register mach_port_t port;

	th = thread_self();
	if ((port = th->mig_reply_port) == MACH_PORT_NULL)
		th->mig_reply_port = port = mach_reply_port();

	return port;
}

void
mig_dealloc_reply_port(
	mach_port_t	reply_port)
{
	mach_port_t port;
	mach_thread_t th;

	th = thread_self();
	port = th->mig_reply_port;
	th->mig_reply_port = MACH_PORT_NULL;

	(void) mach_port_mod_refs(mach_task_self(), port,
				  MACH_PORT_RIGHT_RECEIVE, -1);
}

void
mig_put_reply_port(
	mach_port_t	reply_port)
{
}

kern_return_t
thread_register_malloc(
	vm_offset_t addr,
	vm_size_t   size)
{
	mach_thread_t th;
	register i;

	th = thread_self();
	for (i=0; i < MAX_MALLOCS; i++)
		if (th->mallocs[i].size == 0) {
			th->mallocs[i].seq = th->malloc_seq++;
			th->mallocs[i].addr = addr;
			th->mallocs[i].size = size;
			return(KERN_SUCCESS);
		}
	if (debug)
		dump_mallocs(th);
	return(KERN_FAILURE);
}

kern_return_t
thread_cancel_malloc(
	vm_offset_t addr,
	vm_size_t   *size)
{
	mach_thread_t th;
	register i;

	th = thread_self();
	for (i=0; i < MAX_MALLOCS; i++)
		if (th->mallocs[i].addr == addr) {
			*size = th->mallocs[i].size;
			th->mallocs[i].size = 0;
			th->mallocs[i].addr = 0;
			return(KERN_SUCCESS);
		}
	if (debug)
		dump_mallocs(th);
	return(KERN_FAILURE);
}

dump_mallocs(thread)
mach_thread_t thread;
{
	register i;

	printf("mallocs for thread %x (port %x)\n", thread, thread->thread);
	for (i=0; i < MAX_MALLOCS; i++) {
		if (thread->mallocs[i].size != 0) {
			printf("address %x size %x\n",
			       thread->mallocs[i].size,
			       thread->mallocs[i].addr);
		}
	}
}

clean_mallocs(thread)
mach_thread_t thread;
{
	register i;

	for (i=0; i < MAX_MALLOCS; i++) {
		if (thread->mallocs[i].size != 0) {
			MACH_CALL(vm_deallocate,
				  (mach_task_self(),
				   thread->mallocs[i].addr,
				   thread->mallocs[i].size));
			thread->mallocs[i].size = 0;
			thread->mallocs[i].addr = 0;
		}
	}
}

thread_malloc_state_t
save_mallocs(thread)
mach_thread_t thread;
{
	return(thread->malloc_seq);
}

kern_return_t
restore_mallocs(thread, seq)
mach_thread_t thread;
thread_malloc_state_t seq;
{
	register i;

	for (i=0; i < MAX_MALLOCS; i++) {
		if (thread->mallocs[i].size != 0 &&
		    thread->mallocs[i].seq >= seq) {
			MACH_CALL(vm_deallocate,
				  (mach_task_self(),
				   thread->mallocs[i].addr,
				   thread->mallocs[i].size));
			thread->mallocs[i].size = 0;
			thread->mallocs[i].addr = 0;
		}
	}
	return(KERN_SUCCESS);
}

/*
 * Malloc/free are needed for mach_msg_server().
 * We need to register these memory reservations
 * so that we can free it when killing the corresponding
 * thread.
 */

void *
malloc(vm_size_t size)
{
  	void *ret;

	if (debug > 1)
		printf("thread %x: malloc %x bytes at %x ",
		       thread_self(), size, *(&size-1));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&ret,
				(vm_size_t)size,
				TRUE));
	MACH_CALL(thread_register_malloc, ((vm_offset_t)ret, size));

	if (ret == 0) {
		printf("malloc(): allocated at offset 0 - null pointers may affect data\nExiting...\n");
		leave(1);
	}
 	if (debug > 1)
		printf("returns %x\n", ret);

	return ret;
}

void
free(void *data)
{
	vm_size_t size = 0;

	if (debug > 1)
		printf("thread %x: free %x at %x\n", 
		       thread_self(), data, *(&data-1));
	MACH_CALL(thread_cancel_malloc, ((vm_offset_t)data, &size));
	MACH_CALL(vm_deallocate, (mach_task_self(),
				(vm_offset_t)data,
				size));
}

void *
realloc(void *data, size_t size)
{
	void *newdata = malloc(size);
	vm_size_t oldsize;

	MACH_CALL(thread_cancel_malloc, ((vm_offset_t)data, &oldsize));
	if (newdata)
		memcpy(newdata, data, oldsize);
	MACH_CALL(vm_deallocate, (mach_task_self(),
		(vm_offset_t)data,
		oldsize));
	return (newdata);
}

/*
 * To be provided for use with maxonstack by MIG when collocated
 */
char *
mig_user_allocate(vm_size_t msg_size)
{
	return (char *)malloc(msg_size);
}

void
mig_user_deallocate(char *msg, vm_size_t msg_size)
{
	free(msg);
}
