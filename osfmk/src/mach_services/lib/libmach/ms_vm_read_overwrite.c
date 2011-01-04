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
#include <string.h>
#include <mach/message.h>
#include <mach/mach_syscalls.h>

kern_return_t 
vm_read_overwrite(
	mach_port_t	task,
	vm_address_t	address,
	vm_size_t	size,
	vm_address_t	data,
	vm_size_t	*data_count)
{
	kern_return_t result;

	result = syscall_vm_read_overwrite(task, address, size, data, 
					   data_count);
	if (result == MACH_SEND_INTERRUPTED) {
		vm_offset_t		buf;
		mach_msg_type_number_t	count;
		
		result = vm_read(task, address, size, &buf, &count);
		if (result == KERN_SUCCESS) {
			memcpy((char *)data, (char *)buf, count);
			(void) vm_deallocate(mach_task_self(), 
					    (vm_address_t) buf, count);
			*data_count = count;
		}
	}
	return(result);
}
