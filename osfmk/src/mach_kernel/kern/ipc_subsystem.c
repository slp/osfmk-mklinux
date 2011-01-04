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
 *	File:		kern/ipc_subsystem.c
 *	Purpose:	Routines to support ipc semantics of new kernel
 *			RPC subsystem descriptions
 */

#include <mach/message.h>
#include <kern/ipc_kobject.h>
#include <kern/task.h>
#include <kern/ipc_subsystem.h>
#include <kern/subsystem.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>

/*
 *	Routine:	ipc_subsystem_init
 *	Purpose:
 *		Initialize ipc control of a subsystem.
 */
void
ipc_subsystem_init(
	subsystem_t		subsystem)
{
	ipc_port_t	port;

	port = ipc_port_alloc_kernel();
	if (port == IP_NULL)
		panic("ipc_subsystem_init");
	subsystem->ipc_self = port;
}

/*
 *	Routine:	ipc_subsystem_enable
 *	Purpose:
 *		Enable ipc access to a subsystem.
 */
void
ipc_subsystem_enable(
	subsystem_t		subsystem)
{
	ipc_kobject_set(subsystem->ipc_self,
			(ipc_kobject_t) subsystem, IKOT_SUBSYSTEM);
}


/*
 *      Routine:        ipc_subsystem_disable
 *      Purpose:
 *              Disable IPC access to a subsystem.
 *      Conditions:
 *              Nothing locked.
 */

void
ipc_subsystem_disable(
        subsystem_t        subsystem)
{
        ipc_port_t kport;

        kport = subsystem->ipc_self;
        if (kport != IP_NULL)
                ipc_kobject_set(kport, IKO_NULL, IKOT_NONE);
}


/*
 *	Routine:	convert_port_to_subsystem
 *	Purpose:
 *		Convert from a port to a subsystem.
 *		Doesn't consume the port ref; produces a subsystem ref,
 *		which may be null.
 *	Conditions:
 *		Nothing locked.
 */
subsystem_t
convert_port_to_subsystem(
	ipc_port_t	port)
{
	subsystem_t		subsystem = SUBSYSTEM_NULL;

	if (IP_VALID(port)) {
		ip_lock(port);
		if (ip_active(port) &&
		    (ip_kotype(port) == IKOT_SUBSYSTEM)) {
			subsystem = (subsystem_t) port->ip_kobject;
		}
		ip_unlock(port);
	}
	return (subsystem);
}


/*
 *	Routine:	convert_subsystem_to_port
 *	Purpose:
 *		Convert from a subsystem to a port.
 *		Produces a naked send right which may be invalid.
 *	Conditions:
 *		Nothing locked.
 */
ipc_port_t
convert_subsystem_to_port(
	subsystem_t		subsystem)
{
	ipc_port_t	port;

	port = ipc_port_make_send(subsystem->ipc_self);
	return (port);
}

