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
#include <mach/mach_host.h>

kern_return_t
vm_remap(
	task_port_t	target_map,
	vm_address_t	*address,
	vm_size_t	size,
	vm_address_t	mask,
	boolean_t	anywhere,
	task_port_t	src_map,
	vm_offset_t	memory_address,
	boolean_t	copy,
	vm_prot_t	*cur_protection,
	vm_prot_t	*max_protection,
	vm_inherit_t	inheritance)
{
	kern_return_t kr;

	kr = syscall_vm_remap(target_map, address, size, mask,
			anywhere, src_map, memory_address, copy,
			cur_protection, max_protection, inheritance);

	if (kr == MACH_SEND_INTERRUPTED)
		kr = mig_vm_remap(target_map, address, size, mask,
				anywhere, src_map, memory_address, copy,
				cur_protection, max_protection, inheritance);

	return kr;
}
