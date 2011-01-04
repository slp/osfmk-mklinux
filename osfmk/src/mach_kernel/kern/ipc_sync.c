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

#include <kern/ipc_kobject.h>
#include <kern/ipc_sync.h>
#include <ipc/port.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <mach/sync_server.h>
#include <mach/mach_server.h>
#include <mach/mach_port_server.h>
#include <mach/port.h>


semaphore_t
convert_port_to_semaphore (ipc_port_t port)
{
	semaphore_t semaphore = SEMAPHORE_NULL;

	if (IP_VALID (port)) {
		ip_lock(port);
		if (ip_active(port) && (ip_kotype(port) == IKOT_SEMAPHORE)) {
			semaphore = (semaphore_t) port->ip_kobject;
			semaphore_reference(semaphore);
		}
		ip_unlock(port);
	}

	return (semaphore);
}

ipc_port_t
convert_semaphore_to_port (semaphore_t semaphore)
{
	ipc_port_t port;

	if (semaphore != SEMAPHORE_NULL)
		port = ipc_port_make_send(semaphore->port);
	else
		port = IP_NULL;

	return (port);
}

lock_set_t
convert_port_to_lock_set (ipc_port_t port)
{
	lock_set_t lock_set = LOCK_SET_NULL;

	if (IP_VALID (port)) {
		ip_lock(port);
		if (ip_active(port) && (ip_kotype(port) == IKOT_LOCK_SET)) {
			lock_set = (lock_set_t) port->ip_kobject;
			lock_set_reference(lock_set);
		}
		ip_unlock(port);
	}

	return (lock_set);
}

ipc_port_t
convert_lock_set_to_port (lock_set_t lock_set)
{
	ipc_port_t port;

	if (lock_set != LOCK_SET_NULL)
		port = ipc_port_make_send(lock_set->port);
	else
		port = IP_NULL;

	return (port);
}

