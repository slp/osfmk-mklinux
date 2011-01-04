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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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

#define	MACH_INIT_SLOTS		1
#include <mach.h>
#include "externs.h"

mach_port_t	bootstrap_port = MACH_PORT_NULL;
mach_port_t	name_server_port = MACH_PORT_NULL;
mach_port_t	environment_port = MACH_PORT_NULL;
mach_port_t	service_port = MACH_PORT_NULL;

void
mach_init_ports(void)
{
	mach_port_array_t	ports;
	mach_msg_type_number_t	ports_count;
	kern_return_t		kr;

	/*
	 *	Find those ports important to every task.
	 */
	kr = task_get_special_port(mach_task_self(),
				   TASK_BOOTSTRAP_PORT,
				   &bootstrap_port);
	if (kr != KERN_SUCCESS)
	    return;

	kr = mach_ports_lookup(mach_task_self(), &ports,
			       &ports_count);
	if ((kr != KERN_SUCCESS) ||
	    (ports_count < MACH_PORTS_SLOTS_USED))
	    return;

	name_server_port = ports[NAME_SERVER_SLOT];
	environment_port = ports[ENVIRONMENT_SLOT];
	service_port     = ports[SERVICE_SLOT];

	/* get rid of out-of-line data so brk has a chance of working */

	(void) vm_deallocate(mach_task_self(),
			     (vm_offset_t) ports,
			     (vm_size_t) (ports_count * sizeof *ports));
}


#ifndef	lint
/*
 *	Routines which our library must suck in, to avoid
 *	a later library from referencing them and getting
 *	the wrong version.
 */
extern void _replacements(void);

void
_replacements(void)
{
	(void)sbrk(0);			/* Pull in our sbrk/brk */
	(void)malloc(0);		/* Pull in our malloc package */
}
#endif	/* lint */
