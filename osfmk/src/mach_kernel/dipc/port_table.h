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

/*
 *	File:	dipc/port_table.h
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Interfaces for distributed port name table management.
 */


#ifndef	_DIPC_PORT_TABLE_H_
#define	_DIPC_PORT_TABLE_H_

#include <dipc/dipc_port.h>


/*
 *	Exported interfaces.
 */

/* Assign a new UID to a port. */
dipc_return_t
dipc_uid_allocate(
	ipc_port_t	port,
	dipc_port_t	dip);


/* Install an existing UID/port relationship. */
dipc_return_t
dipc_uid_install(
	ipc_port_t	port,
	uid_t		*uid);


/* Find a port based on its UID. */
dipc_return_t
dipc_uid_lookup(
	uid_t		*uid,
	ipc_port_t	*port);


dipc_return_t
dipc_uid_lookup_fast(
	uid_t		*uidp,
	ipc_port_t	*port);

/* Convert an interrupt-level port reference to a real reference. */
dipc_return_t
dipc_uid_port_reference(
	ipc_port_t	port);


/* Eliminate a UID/port relationship. */
dipc_return_t
dipc_uid_remove(
	ipc_port_t	port);


/* Initialize the DIPC port name table. */
void
dipc_port_name_table_init(void);

#endif	/* _DIPC_PORT_TABLE_H_ */
