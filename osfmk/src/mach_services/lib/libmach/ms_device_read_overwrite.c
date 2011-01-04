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
#include <device/device.h>

kern_return_t
device_read_overwrite(
	mach_port_t		device,
	dev_mode_t		mode,
	recnum_t		recnum,
	io_buf_len_t		bytes_wanted,
	vm_address_t		data,
	mach_msg_type_number_t	*data_count)
{
	kern_return_t	result;

	result = syscall_device_read_overwrite(device, mode, recnum, 
					       bytes_wanted, data, data_count);
	if (result == MACH_SEND_INTERRUPTED) {
		io_buf_ptr_t		buf;
		mach_msg_type_number_t	count;

		result = mig_device_read(device, mode, recnum, bytes_wanted, 
					 &buf, &count);
		if (result == KERN_SUCCESS) {
			memcpy((void *)data, (const void *)buf, count);
			(void) vm_deallocate(mach_task_self(), 
					     (vm_offset_t) buf, count);
			*data_count = count;
		}
	}
	return(result);
}
