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
 *	File:	dipc/dipc_node.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Routines concerning nodes in a NORMA domain.
 */

#include <dipc/dipc_funcs.h>
#include <dipc/dipc_port.h>
#include <mach/kkt_request.h>

node_name dipc_port_destination(ipc_port_t	port);

/*
 *	Check whether a node exists.  This function
 *	exists during a transition phase:  eventually,
 *	membership services and cluster management will
 *	supply a function that supplants this one.
 *
 *	Note that this function is inherently racy:
 *	unless the caller has special knowledge, there
 *	is no guarantee that the status of the requested
 *	node won't change after this call completes.
 */
boolean_t
dipc_node_is_valid(node_name	node)
{
	return (node == NODE_NAME_LOCAL ||
		KKT_NODE_IS_VALID(node));
}


node_name
dipc_node_self(void)
{
	/*
	 *	node_self() is currently a machine-dependent call.  XXX
	 */
	return KKT_NODE_SELF();
}


/*
 *	Return an indication of the node to which the port is attached.
 *	The caller must have special knowledge because this call in no
 *	way prevents the port from migrating.
 *
 *	The destination of a proxy is indicated by ip_dest_node; for
 *	completeness, we define the destination of a principal as the
 *	identity of the local node.
 */
node_name
dipc_port_destination(
	ipc_port_t	port)
{
 	return DIPC_IS_PROXY(port) ? port->ip_dest_node : dipc_node_self();
}
