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
 *	The node_map_t is an opaque pointer to a kkt node map.
 */

#ifndef	_MACH_KKT_TYPES_H_
#define	_MACH_KKT_TYPES_H_

typedef unsigned int	node_map_t;	/* at least 32 bits */
#define NODE_MAP_NULL	((node_map_t) 0)

/*
 *	The node_name is an opaque type that uniquely
 *	identifies a node within a NORMA domain.  The
 *	transport must be able to route a message to
 *	the node specified by a node_name.
 */
typedef unsigned int	node_name;	/* at least 32 bits */
#define	NODE_NAME_NULL	((node_name) -1)

/*
 *	The local node name is useful for having a destination node
 *	that is not node_self() but in reality will be routed to
 *	node_self().
 *
 *	N.B.  This node name is ONLY available on versions of KKT
 *	compiled with KERNEL_TEST=1.  Production kernels do NOT
 *	support this node name.
 */
#define	NODE_NAME_LOCAL	((node_name) -2)

/*
 * Handle info
 */
typedef struct handle_info {
	node_name	node;		/* remote node */
	kern_return_t	abort_code;	/* error code if handle aborted */
} *handle_info_t;

/*
 *	The handle_t uniquely identifies a transport endpoint
 *	on the local node.  A handle_t is only valid relative
 *	to the local node and may not be assumed to be valid
 *	on a remote node.
 */
typedef unsigned int	handle_t;	/* at least 32 bits */
#define	HANDLE_NULL	((handle_t) 0)

/*
 * An endpoint is defined to be two pointers quantity which is
 * opaque to the underlying transport.
 */
typedef struct endpoint {
	void	*ep[2];
} endpoint_t;

#endif	/*_MACH_KKT_TYPES_H_*/
