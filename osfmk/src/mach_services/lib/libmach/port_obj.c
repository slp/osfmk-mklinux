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
 * Define a service to map from a kernel-generated port name
 * to server-defined "type" and "value" data to be associated
 * with the port.
 */
#include <port_obj.h>
#include <mach.h>

#define DEFAULT_TABLE_SIZE	(64 * 1024)

struct port_obj_tentry *port_obj_table;
int port_obj_table_size = DEFAULT_TABLE_SIZE;

void port_obj_init(
	int maxsize)
{
	kern_return_t kr;

	kr = vm_allocate(mach_task_self(),
		(vm_offset_t *)&port_obj_table,
		(vm_size_t)(maxsize * sizeof (*port_obj_table)),
		TRUE);
	if (kr != KERN_SUCCESS)
		panic("port_obj_init: can't vm_allocate");
}
