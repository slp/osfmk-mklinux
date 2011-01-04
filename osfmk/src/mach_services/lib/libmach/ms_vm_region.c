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
#include <mach/mach_syscalls.h>
#include <mach/mach_migcalls.h>

kern_return_t
vm_region(
	task_port_t		target_task,
	vm_address_t		*address,	/* IN/OUT */
	vm_size_t		*size,		/* OUT */
	vm_region_flavor_t	flavor,
	vm_region_info_t	infop,		/* OUT */
	mach_msg_type_number_t  *count,		/* IN/OUT */
	mach_port_t		*object_name)	/* OUT */
{
	kern_return_t result;

	result = syscall_vm_region(target_task, address, size,
					flavor, infop, count, object_name);
	if (result == MACH_SEND_INTERRUPTED)
		result = mig_vm_region(target_task, address, size,
					flavor, infop, count, object_name);
	return(result);
}
