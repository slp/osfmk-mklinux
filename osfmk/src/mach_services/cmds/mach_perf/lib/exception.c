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
 * Catch mach_perf exceptions and display status
 */

#include <mach_perf.h>
#include "exception.h"

#define EXC_MASK	(EXC_MASK_BAD_ACCESS |			\
			 EXC_MASK_BAD_INSTRUCTION |		\
			 EXC_MASK_ARITHMETIC |			\
			 EXC_MASK_EMULATION |			\
			 EXC_MASK_SOFTWARE |			\
			 EXC_MASK_BREAKPOINT |			\
			 EXC_MASK_RPC_ALERT |			\
			 EXC_MASK_MACHINE)

mach_port_t 	exception_port;
mach_thread_t	sa_exc_thread;
boolean_t	sa_exc_server();
boolean_t	is_attached();

exception_thread_body(port)
mach_port_t port;
{
	if (debug)
		printf("exception_thread_body\n");
	MACH_CALL(mach_msg_server, (sa_exc_server,
				    1024,
				    port,
				    MACH_MSG_OPTION_NONE));
}

void
exception_init(void)
{
	char **argv;
	int argc = 0;

	if (debug > 0)
		printf("exception_init\n");

	if (exception_port != MACH_PORT_NULL || is_attached())
		return;

	MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_RECEIVE,
					&exception_port));
	if (debug > 0)
		printf("exception_port %x\n", exception_port);
	new_thread(&sa_exc_thread,
		   (vm_offset_t)exception_thread_body,
		   (vm_offset_t)exception_port);

	MACH_CALL( mach_port_insert_right, (mach_task_self(),
					    exception_port,
					    exception_port,
					    MACH_MSG_TYPE_MAKE_SEND));

	/* Clear thread exception ports In case UX server
	   catches thread exceptions ... */

	MACH_CALL( thread_set_exception_ports,
		  (mach_thread_self(),
		   EXC_MASK,
		   MACH_PORT_NULL,
		   EXCEPTION_STATE_IDENTITY,
		   THREAD_STATE));

	/* Catch all exceptions but syscalls */

	MACH_CALL( task_set_exception_ports,
		  (mach_task_self(),
		   EXC_MASK,
		   exception_port,
		   EXCEPTION_STATE_IDENTITY,
		   THREAD_STATE));
}

exception_fork(mach_port_t child)
{
	mach_port_t task_exc_port;
	exception_mask_t mask;
	unsigned int exc_ports_count=1;
	exception_behavior_t task_exc_behavior;
	thread_state_flavor_t task_exc_flavor;

	if (exception_port == MACH_PORT_NULL)
		return;

	MACH_CALL( task_get_exception_ports,
		  (mach_task_self(),
		   EXC_MASK_BAD_ACCESS,
		   &mask,
		   &exc_ports_count,
		   &task_exc_port,
		   &task_exc_behavior,
		   &task_exc_flavor));

	MACH_CALL( task_set_exception_ports,
		  (child,
		   EXC_MASK,
		   task_exc_port,
		   EXCEPTION_STATE_IDENTITY,
		   THREAD_STATE));
}

kern_return_t 
sa_catch_exception_raise(
	mach_port_t 		port,
	mach_port_t 		thread,
	mach_port_t 		task,
	exception_type_t	exception,
	exception_data_t 	code,
	mach_msg_type_number_t 	codeCnt)
{
  	register i;
	printf("\nexception_raise: exception 0x%x, codes: ",
	       exception);
	for (i=0; i <codeCnt; i++)
		printf("0x%x ", *(code+i));
	printf("\n");
	debugger();
}

sa_catch_exception_raise_state(
	mach_port_t 		port,
	exception_type_t	exception,
  	exception_data_t	code,
	mach_msg_type_number_t 	codeCnt,
	thread_state_flavor_t	*flavor,
	thread_state_t 		*in_state,
	mach_msg_type_number_t	in_count,
	thread_state_t		*out_state,
	mach_msg_type_number_t	*out_count)
{
	register i,j; 

	printf("\nexception_raise_state: exception 0x%x, codes: ",
	       exception);
	for (i=0; i <codeCnt; i++)
		printf("0x%x ", *(code+i));
	printf("\nthread_state (flavor 0x%x):\n", *flavor);
	for (i=0; i < in_count; ) {
		printf("0x%08x:", i*(sizeof(natural_t)));
		for(j=0; j<4; j++, i++)
			printf(" %8x", in_state[i]);
		printf("\n"); 
	}
	debugger();
}

sa_catch_exception_raise_state_identity(
	mach_port_t 		port,
	mach_port_t 		thread,
	mach_port_t 		task,
	exception_type_t	exception,
  	exception_data_t	code,
	mach_msg_type_number_t 	codeCnt,
	thread_state_flavor_t	*flavor,
	thread_state_t 		*in_state,
	mach_msg_type_number_t	in_count,
	thread_state_t		*out_state,
	mach_msg_type_number_t	*out_count)
{
	register i,j; 

	printf("\nexception_raise_state_identity: exception 0x%x, codes: ",
	       exception);
	for (i=0; i <codeCnt; i++)
		printf("0x%x ", *(code+i));
	printf("\nthread 0x%x task 0x%x\n", thread, task);
	printf("thread_state (flavor 0x%x):\n", *flavor);
	for (i=0; i < in_count; ) {
		printf("0x%08x:", i*(sizeof(natural_t)));
		for(j=0; j<4 && i < in_count; j++, i++)
			printf(" %8x", in_state[i]);
		printf("\n"); 
	}
	debugger();
}


debugger()
{
	if (standalone) {
		MACH_CALL(host_reboot, (privileged_host_port,
						0x1000 /* RB_DEBUGGER */));
        } else {
		printf("suspending self\n");
                MACH_CALL(task_suspend, (mach_task_self()));
	}
}

/* 
 * Are we already attached to a debugger
 * Check EXC_MASK_BREAKPOINT exception port
 */

boolean_t
is_attached()
{
	mach_port_t 		task_exc_port;
	exception_behavior_t 	task_exc_behavior;
	thread_state_flavor_t 	task_exc_flavor;
	mach_port_t 		thread_exc_port;
	exception_behavior_t 	thread_exc_behavior;
	thread_state_flavor_t 	thread_exc_flavor;
	exception_mask_t 	mask;
	unsigned int 		exc_ports_count;
	boolean_t		ret = FALSE;
	
	exc_ports_count=1;
	MACH_CALL( thread_get_exception_ports,
		  (mach_thread_self(),
		   EXC_MASK_BREAKPOINT,
		   &mask,
		   &exc_ports_count,
		   &thread_exc_port,
		   &thread_exc_behavior,
		   &thread_exc_flavor));


	exc_ports_count=1;
	MACH_CALL( task_get_exception_ports,
		  (mach_task_self(),
		   EXC_MASK_BREAKPOINT,
		   &mask,
		   &exc_ports_count,
		   &task_exc_port,
		   &task_exc_behavior,
		   &task_exc_flavor));

	if (task_exc_port != MACH_PORT_NULL) {
		ret = TRUE;
		MACH_CALL( mach_port_deallocate, (mach_task_self(),
					  task_exc_port));
	} 

	if (thread_exc_port != MACH_PORT_NULL) {
		ret = TRUE;
		MACH_CALL( mach_port_deallocate, (mach_task_self(),
					  thread_exc_port));
	} 

	return(ret);
}




