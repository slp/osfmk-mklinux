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
 * 
 */
/*
 * MkLinux
 */

#include <mach.h>
#include <mach/message.h>

kern_return_t
mach_port_allocate_full(
	task_port_t		task,
	mach_port_right_t	right,
	mach_port_t		subsystem,
	mach_port_qos_t		* qosp,
	mach_port_t		* namep)
{
	kern_return_t		kr;

	kr = syscall_mach_port_allocate_full (task, right, subsystem,
						qosp, namep);
	if (kr == MACH_SEND_INTERRUPTED)
	    kr = mig_mach_port_allocate_full (task, right, subsystem,
						qosp, namep);

	return (kr);
}

