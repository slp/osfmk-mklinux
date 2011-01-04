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
#include <mach/message.h>
#include <mach/exception.h>
#include <mach/mach_host.h>
#include <mach/mach_syscalls.h>
#include <mach/mach_migcalls.h>

kern_return_t
task_set_exception_ports(
	task_port_t		task,
	exception_mask_t	new_mask,
	mach_port_t		new_port,
	exception_behavior_t	new_behavior,
	thread_state_flavor_t	new_flavor)
{
	kern_return_t result;

	result = syscall_task_set_exception_ports(task, new_mask, new_port,
						  new_behavior, new_flavor);
	if (result == MACH_SEND_INTERRUPTED)
		result = mig_task_set_exception_ports(task, new_mask, new_port,
						      new_behavior, new_flavor);
	return(result);
}
