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

#ifndef _MACH_SERVICES_H
#define _MACH_SERVICES_H

#include <mach.h>

#define MACH_CALL_NO_TRACE(call, args) {				\
	register kern_return_t kern_ret;				\
	kern_ret = call args ;						\
	if (kern_ret != KERN_SUCCESS)					\
		mach_call_failed(# call, __FILE__, __LINE__, kern_ret);	\
}

#define MACH_FUNC_NO_TRACE(ret, func, args) {				\
	ret = func args ;						\
	if (!ret) 							\
		mach_func_failed(# func, __FILE__, __LINE__);		\
}

#define MACH_CALL_WITH_TRACE(call, args) {				\
	register kern_return_t kern_ret;				\
	kern_ret = call args ;						\
	if (kern_ret != KERN_SUCCESS || ntraces)			\
		mach_call_print(# call, __FILE__, __LINE__, kern_ret);	\
}

#define MACH_FUNC_WITH_TRACE(ret, func, args) {				\
	ret = func args ;						\
	if ((!ret) || ntraces) 						\
		mach_func_print(# func, __FILE__, __LINE__, ret);	\
}

#if	CALL_TRACE
#define	MACH_CALL MACH_CALL_WITH_TRACE
#define	MACH_FUNC MACH_FUNC_WITH_TRACE
#else	/* CALL_TRACE */
#define	MACH_CALL MACH_CALL_NO_TRACE
#define	MACH_FUNC MACH_FUNC_NO_TRACE
#endif	/* CALL_TRACE */


extern mach_port_t 			mach_fork();
extern processor_set_control_port_t 	get_processor_set();
extern int 			is_master_task;
extern int			norma_node;

#define		NORMA_NODE_NULL -1

#define MAX_THREADS 10
#define MAX_MALLOCS 10
#define STACK_SIZE  (64*1024)

struct malloc {
	vm_offset_t   addr;
	vm_size_t     size;
	int	      seq;
};

typedef struct malloc malloc_t;
typedef int thread_malloc_state_t;

struct mach_thread {
	thread_port_t thread;
	vm_offset_t   stack;
	mach_port_t   mig_reply_port;
	int	      malloc_seq;
	struct malloc mallocs[MAX_MALLOCS];
};

typedef struct mach_thread *mach_thread_t;

struct mach_thread threads[MAX_THREADS];

#define thread_stack(addr)	((unsigned)addr & ~(STACK_SIZE-1)) 

extern mach_thread_t thread_self();
extern boolean_t threads_initialized;
extern thread_malloc_state_t save_mallocs(mach_thread_t thread);
extern thread_malloc_state_t restore_mallocs(mach_thread_t thread,
                thread_malloc_state_t seq);
extern int privileged_user();

#define page_size_for_vm vm_page_size

#endif /* _MACH_SERVICES_H */
	














