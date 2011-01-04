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
 *	File:	dipc/special_ports.h
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Public interfaces to functions supporting
 *	NORMA special ports.
 */

#include <kern/host.h>
#include <ipc/ipc_port.h>

/*
 *	The norma_get/set_special_port functions are
 *	the implementations of the MIG definitions.
 */
extern kern_return_t	norma_set_special_port(
				host_t		host,
				int		id,
				ipc_port_t	port);

extern kern_return_t	norma_get_special_port(
				host_t		host,
				int		node,
				int		id,
				ipc_port_t	*portp);

/*
 *	The remote_* functions return a port corresponding
 *	to the remote entity on the specified node, or
 *	IP_NULL if the node doesn't exist, the port can't
 *	be created, etc.
 */

ipc_port_t	dipc_device_port(
				node_name	node);

ipc_port_t	dipc_host_port(
				node_name	node);

ipc_port_t	dipc_host_priv_port(
				node_name	node);

/*
 *	Destroy a port's special table association.
 */
void		dipc_special_port_destroy(
				ipc_port_t	port);

/*
 *	Bootstrap special ports subsystem.
 */
extern void	norma_special_ports_initialization(void);
