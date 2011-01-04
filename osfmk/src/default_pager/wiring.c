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
/* CMU_HIST */
/*
 * Revision 2.3  92/01/14  16:43:25  rpd
 * 	Fixed wire_all_memory to deallocate ports from vm_region.
 * 	[92/01/14            rpd]
 * 
 * Revision 2.2  92/01/03  19:58:37  dbg
 * 	Don't wire memory unless for current task.
 * 	[91/10/17            dbg]
 * 
 * 	Created.
 * 	[91/04/29            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
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
 */

/*
 * Package to wire current task's memory.
 */
#include <mach.h>
#include <mach/mach_host.h>
#include <mach/mach_traps.h>
#include <mach/vm_region.h>
#include <mach/machine/vm_param.h>

#include "wiring.h"
#include "externs.h"

mach_port_t	this_task;		/* our task */
mach_port_t	priv_host_port = MACH_PORT_NULL;
					/* the privileged host port */

void
wire_setup(mach_port_t host_priv)
{
	priv_host_port = host_priv;
	this_task = mach_task_self();
}

void
wire_memory(vm_address_t start, vm_size_t size, vm_prot_t prot)
{
	kern_return_t	kr;

	if (priv_host_port == MACH_PORT_NULL)
	    return;

	kr = vm_wire(priv_host_port,
		     this_task,
		     start, size, prot);
	if (kr != KERN_SUCCESS)
	    panic("mem_wire: %d", kr);
}

void
wire_thread(void)
{
	kern_return_t	kr;

	if (priv_host_port == MACH_PORT_NULL)
	    return;

	kr = thread_wire(priv_host_port,
			 mach_thread_self(),
			 TRUE);
	if (kr != KERN_SUCCESS)
	    panic("wire_thread: %d", kr);
}

void
wire_all_memory(void)
{
	register kern_return_t kr;
	vm_offset_t			address;
	vm_size_t			size;
	mach_msg_type_number_t		count;
	memory_object_name_t 		object;
	vm_region_basic_info_data_t	info;

	if (priv_host_port == MACH_PORT_NULL)
	    return;

	/* iterate thru all regions, wiring */
	address = 0;
	count = VM_REGION_BASIC_INFO_COUNT;
	while (
	    (kr = vm_region(this_task, &address,
	    		&size,
			VM_REGION_BASIC_INFO,
			(vm_region_info_t) &info,
			&count,
			&object))
		== KERN_SUCCESS)
	{
	    if (MACH_PORT_VALID(object))
		(void) mach_port_deallocate(this_task, object);
	    wire_memory(address, size, info.protection);
	    address += size;
	}
}

/*
 * Alias for vm_allocate to return wired memory.
 */
kern_return_t
vm_allocate(mach_port_t task,
	    vm_address_t *address,
	    vm_size_t size,
	    boolean_t anywhere)
{
	kern_return_t	kr;

	if (anywhere)
	    *address = VM_MIN_ADDRESS;
	kr = vm_map(task,
		address, size, (vm_offset_t) 0, anywhere,
		MEMORY_OBJECT_NULL, (vm_offset_t)0, FALSE,
		VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT);
	if (kr != KERN_SUCCESS)
	    return kr;

	if (task == this_task)
	    (void) vm_wire(priv_host_port, task, *address, size,
			VM_PROT_DEFAULT);
	return KERN_SUCCESS;
}
