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
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 *	File:	dipc/norma_special.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1990
 *
 *	Originally, this file was norma/ipc_special.c;
 *	it has been substantially revised (Alan Langerman, 1994).
 *
 *	Functions to support NORMA special ports.
 */

#include <kernel_test.h>

#include <mach/norma_special_ports.h>
#include <device/device_port.h>
#include <kern/host.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_space.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_port.h>
#include <dipc/port_table.h>
#include <dipc/special_ports.h>
#include <kern/lock.h>
#include <dipc/dipc_counters.h>
#include <mach/kkt_request.h>
#include <dipc/dipc_rpc.h>


/*
 *	The special ports story.
 *
 *	Special ports are defined by the kernel and in
 *	some instances by privileged user programs.
 *	They are accessible by well-known, small integer
 *	identifiers through two user-level NORMA calls:
 *	norma_get_special_port and norma_set_special_port.
 *	These ports are used for bootstrapping the system
 *	and for bootstrapping distributed IPC.
 *
 *	The small integer identifiers, [0..MAX_SPECIAL_ID),
 *	are reserved in the UID name space.  These UIDs
 *	will never be allocated implicitly by DIPC; they
 *	are only allocated explicitly.  In the current
 *	implementation, the UID values [0..MAX_SPECIAL_KERNEL_ID)
 *	are reserved for kernel special ports and are assigned
 *	as the kernel boots.  The values [MAX_SPECIAL_KERNEL_ID..
 *	MAX_SPECIAL_ID), while reserved, remain unused.
 *
 *	The first several special IDs, [0..MAX_SPECIAL_KERNEL_ID),
 *	are themselves reserved for use by the kernel:  it is
 *	not possible for user-level code to alter these ports,
 *	although it is possible for user-level code to retrieve
 *	them.  The kernel-owned entries include the device, host
 *	and host_priv ports.  The kernel has receive rights for
 *	these ports.  Kernel special ports are entered directly
 *	in the port name table under the reserved UID, as well
 *	as in the special port table.
 *
 *	Kernel special ports do	not support NMS detection.
 *
 *	The remaining identifiers exist for use by privileged
 *	applications.  In particular, one well-known identifier
 *	is reserved for a nameserver.  These user-level special
 *	ports live in the port name table under whatever UIDs
 *	they may have been assigned, but are also accessible
 *	through the host_special_port table that matches special
 *	port IDs with actual ports.
 *
 *	When obtained from a remote node, both kernel and user
 *	special ports live only in the port name table.  These ports
 *	have no	visible properties distinguishing them from regular
 *	remotely-obtained ports.  Hence, these proxies will disappear
 *	when they are no longer used.
 *
 *	Kernel special ports do not maintain NMS information,
 *	although user special ports might.  Kernel special ports are
 *	not expected to migrate, although user special ports could;
 *	in that case, the user port loses its specialness.
 *
 *	Reference counting.  The special port table maintains no
 *	reference on a special port.  It is the responsibility of
 *	the port destruction code to remove a special port from
 *	the table.
 *
 *	Locking order.  The host_special_port_lock takes precedence
 *	over port locks so that it is possible to atomically retrieve
 *	a special port while incrementing the port reference count to
 *	prevent its disappearance.  The host_special_port_lock also
 *	takes precedence over port name table lock (dpn_table_lock
 *	in port_table.c).
 *
 *	The host_special_port_lock guards the host_special_port table
 *	and is also used to serialize insertions of kernel special
 *	ports into the port name table.
 */

#define host_special_port(id)	(host_special_port_array[id])
ipc_port_t			host_special_port_array[MAX_SPECIAL_ID];
mutex_t				host_special_port_lock;


dstat_decl(unsigned int		c_ns_user_get_remote = 0;)
dstat_decl(unsigned int		c_ns_user_get_calls = 0;)
dstat_decl(unsigned int		c_ns_special_port_destroy_calls = 0;)
dstat_decl(unsigned int		c_ns_special_port_destroy_bogons = 0;)
dstat_decl(unsigned int		c_ns_kernel_get_port_creates = 0;)
dstat_decl(unsigned int		c_kgsp_node_self = 0;)

#if	MACH_ASSERT
boolean_t	norma_special_ports_initialized = FALSE;
boolean_t	norma_special_ports_initializing = FALSE;
#endif	/* MACH_ASSERT */


#define	kernel_set_special_port(id, port)				\
	set_special_port((host_t)1, id, port)

ipc_port_t	get_special_port(
				 node_name	node,
				 unsigned long	id,
				 dipc_return_t	*dr);
kern_return_t 	set_special_port(
				 host_t		host,
				 unsigned long	id,
				 ipc_port_t	port);
void		dipc_special_port_destroy(ipc_port_t	port);


/*
 *	Register a special port.  Once registered, a remote node
 *	can find this port using a special port ID.  A user port
 *	may not override any of the kernel special port IDs,
 *	although a user port may override a previously existing
 *	user port ID.  It is possible to register a special port
 *	of IP_NULL, to clear out an existing special port entry.
 *
 *	When marking a port special for the first time, it is
 *	necessary to allocate a dipc port as well because it is
 *	only the dipc port that has the bitfield containing the
 *	ip_special flag bit.
 *
 *	For kernel special ports (0 < ID < MAX_SPECIAL_KERNEL_ID),
 *	the port must meet the following conditions:
 *		- it must be local to the calling node
 *		- it must NOT have NMS detection enabled on it
 *		- it must never before have entered DIPC
 *		- caller guarantees that there is no other
 *		way to operate on the port while this routine
 *		does its work
 *	The port will be assigned a UID based on the requested
 *	special port ID and entered in the port name table and the
 *	special port table.  Essentially, kernel special ports
 *	should be registered at system boot time.
 */
kern_return_t
set_special_port(
	host_t		host,
	unsigned long	id,
	ipc_port_t	port)
{
	boolean_t	is_user_port;
	dipc_return_t	dr;
	uid_t		uid;
	ipc_port_t	old;

#if	1
	/*
	 *	Disable No More Senders detection on kernel ports.
	 *	We do this only because ports are allocated with NMS
	 *	turned on by default, a compatibility requirement that
	 *	should disappear soon.		XXX
	 */
	if (id < MAX_SPECIAL_KERNEL_ID && port != IP_NULL)
		IP_CLEAR_NMS(port);
#endif
	assert(norma_special_ports_initialized == TRUE ||
	       (id < MAX_SPECIAL_KERNEL_ID &&
		norma_special_ports_initializing == TRUE));
	assert(id >= 0 && id < MAX_SPECIAL_ID);
	assert(id >= MAX_SPECIAL_KERNEL_ID || port == IP_NULL ||
	       !IP_NMS(port));
	assert(id >= MAX_SPECIAL_KERNEL_ID || port == IP_NULL ||
	       !DIPC_IS_DIPC_PORT(port));

	is_user_port = (id >= MAX_SPECIAL_KERNEL_ID);

	/*
	 *	Make sure the port is in DIPC with an assigned UID.
	 *	Even if a later check fails without the port ever
	 *	being used remotely, no harm has been done; the
	 *	dipc_port will be deallocated when the port eventually
	 *	is destroyed.
	 *
	 *	For kernel ports, we don't completely initialize the
	 *	dipc_port state, particularly the UID, until later.
	 *	There's no way for the port to be found at the moment --
	 *	it's not in the port name table and the caller
	 *	guarantees that no one else can find the port -- so
	 *	there's	no problem that the dipc_port is only partially
	 *	initialized.
	 */
	if (port != IP_NULL) {
		dr = dipc_port_init(port, is_user_port);
		assert(dr == DIPC_SUCCESS);
		assert(DIPC_IS_DIPC_PORT(port));
		assert(is_user_port || !DIPC_UID_VALID(&port->ip_uid));
		assert(id >= MAX_SPECIAL_KERNEL_ID || port->ip_proxy == FALSE);
	}

	/*
	 *	Verify that the port is valid and hasn't already
	 *	been registered as special.
	 */
	mutex_lock(&host_special_port_lock);
	if (port != IP_NULL) {
		ip_lock(port);
		if (!IP_VALID(port)) {
			assert(is_user_port == TRUE);
			ip_unlock(port);
			mutex_unlock(&host_special_port_lock);
			return KERN_INVALID_ARGUMENT;
		}
		if (port->ip_special == TRUE) {
			assert(is_user_port == TRUE);
			ip_unlock(port);
			mutex_unlock(&host_special_port_lock);
			return KERN_INVALID_RIGHT;
		}

		/*
		 *	The port must be marked as special so that,
		 *	during the port destruction sequence, DIPC
		 *	knows that extra handling is required.
		 *	Marking the port as special must be atomic
		 *	w.r.t. entering it into the table.
		 */
		port->ip_special = TRUE;

		/*
		 *	For kernel ports, additional actions are
		 *	required.
		 *
		 *	Create a UID for the port.  The UID must
		 *	have an origin naming the local node, since
		 *	that is where the port was created, and
		 *	must have an identifier matching the special
		 *	port ID.  A remote node will talk to this
		 *	principal by constructing its own proxy using
		 *	the node_name of this (target) node and an
		 *	identifier consisting of the special port ID.
		 *
		 *	Then install the port in the port name table;
		 *	we can do this while holding the table and
		 *	port locks.
		 *
		 *	Once we drop the table lock, remote nodes can
		 *	discover this port, via norma_get_special_port,
		 *	then send messages to it.
		 */
		if (!is_user_port) {
			DIPC_UID_MAKE(&uid, dipc_node_self(), (port_id) id);
			dr = dipc_uid_install(port, &uid);
			assert(dr == DIPC_SUCCESS);
		}
	}

	/*
	 *	Swap the new special port for whatever port may
	 *	have already been installed in the same slot in
	 *	the table.  The old port's special flag must be
	 *	cleared at the same time, to prevent races with
	 *	the port deallocation code.
	 */
	old = host_special_port(id);
	host_special_port(id) = port;
	if (port != IP_NULL)
		ip_unlock(port);
	if (old != IP_NULL) {
		ip_lock(old);
		old->ip_special = FALSE;
		ip_unlock(old);
	}
	mutex_unlock(&host_special_port_lock);

	if (old != IP_NULL)
		printf("set_special_port:  id #%d replaced\n", id);

	return KERN_SUCCESS;
}


/*
 *	Retrieve a special port.
 *
 *	For retrievals from the local node, we look in the
 *	special ports table.
 *
 *	For retrievals from a remote node, we create a proxy
 *	for the remote special port (if we don't already have
 *	one).  A proxy for a remote special port is itself
 *	ordinary (not special); special ports can only be
 *	registered through set_special_port.
 *
 *	For IDs assigned by the kernel, we construct the
 *	proxy by building a UID based on the special ID.
 *	There is no need to contact the remote node, as it
 *	may be assumed to have the desired kernel special port.
 *	If not, the error will be discovered when an attempt
 *	is made to send a message on the port.
 *
 *	For IDs managed by the user, we contact the remote
 *	node to	discover the UID of the port associated with
 *	the requested special port ID.
 *
 *	N.B.  A proxy for a remote special port will be	destroyed
 *	when all its references are released.  Such a port will be
 *	re-created the next time it is requested.
 */
ipc_port_t
get_special_port(
	node_name	node,
	unsigned long	id,
	dipc_return_t	*dr)
{
	ipc_port_t	port;
	kkt_return_t	kktr;
	uid_t		uid;
	boolean_t	nms;

	assert(norma_special_ports_initialized == TRUE);
	assert(id >= 0 && id < MAX_SPECIAL_ID);

	dstat(c_ns_user_get_calls++);

	*dr = DIPC_SUCCESS;

	if (!dipc_node_is_valid(node))
		return IP_NULL;

	/*
	 *	Local node:  fetch port from special port table
	 *	and return a send right to the caller.  There is
	 *	a race here with port destruction:  briefly, the
	 *	port may be dead but still in the table.
	 */
	if (node == dipc_node_self()) {
		mutex_lock(&host_special_port_lock);
		port = host_special_port(id);
		assert(port == IP_NULL || port->ip_special == TRUE);
		if (port != IP_NULL) {
			ip_lock(port);
			if (!IP_VALID(port))
				port = IP_DEAD;
			ip_unlock(port);
			if (port != IP_DEAD)
				port = ipc_port_make_send(port);
		}
		mutex_unlock(&host_special_port_lock);
		return port;
	}

	if (id >= MAX_SPECIAL_KERNEL_ID) {
		/*
		 *	Remote node:  ask the remote node for the uid
		 *	of its registered special port.  There could
		 *	be an optimization here:  we could remember
		 *	the association between (node, special port id)
		 *	and port in a local cache to avoid doing a
		 *	remote RPC for every get_special_port
		 *	operation.  But then we'd have to worry about
		 *	cache invalidation, recycling entries, etc.
		 *	The original NORMA_IPC implementation avoided
		 *	this problem because it inserted special port
		 *	detection logic in the message reception path,
		 *	which we believe is precisely the wrong path
		 *	to slow down.
		 */
		dstat(c_ns_user_get_remote++);
		kktr = drpc_get_special_port(node, id, dr, &uid, &nms);
		if (kktr != KKT_SUCCESS) {
			*dr = DIPC_TRANSPORT_ERROR;
			return IP_NULL;
		}
		if (*dr != DIPC_SUCCESS) {
			/*
			 *	Port probably wasn't registered
			 *	on the other side.
			 */
			printf("get_special_port:  no port for node %d id %d\n",
			     node, id);
			return IP_NULL;
		}
	} else {
		/*
		 *	For kernel special ports only, we can construct
		 *	the port's UID on the fly.  The UID of such a
		 *	port is always (node_name, special-port-id).
		 *	Because kernel special ports never have NMS
		 *	detection enabled, there's absolutely no need
		 *	to do an RPC to the remote node.
		 */
		DIPC_UID_MAKE(&uid, node, (port_id) id);
		nms = FALSE;
	}

	/*
	 *	Given the UID, pretend we are migrating a send
	 *	right to this node.  For a user special port,
	 *	we are simply matching the dipc_port_copyin
	 *	done on the remote node.  For a kernel special
	 *	port, there was no matching dipc_port_copyin,
	 *	but that's OK because kernel special ports don't
	 *	have NMS detection enabled.
	 */
	*dr = dipc_port_copyout(&uid, MACH_MSG_TYPE_PORT_SEND,
			       node, FALSE, nms, &port);
	if (*dr != DIPC_SUCCESS)
		return IP_NULL;

	return port;
}


/*
 *	Called during port destruction.  The port is
 *	believed to have no more users but is known
 *	to be a special port.
 *
 *	It is currently illegal to destroy a local
 *	kernel special port.  They are expected to
 *	persist for all time.
 */
void
dipc_special_port_destroy(
	ipc_port_t	port)
{
	node_name	origin;
	port_id		ident;
	unsigned long	id;

	assert(norma_special_ports_initialized == TRUE);

	dstat(c_ns_special_port_destroy_calls++);
	mutex_lock(&host_special_port_lock);
	ip_lock(port);

	/*
	 *	Catch an attempt to turn off special port
	 *	handling for a port that isn't special.
	 */
	if (!DIPC_IS_DIPC_PORT(port) || port->ip_special == FALSE) {
		ip_unlock(port);
		mutex_unlock(&host_special_port_lock);
		dstat(c_ns_special_port_destroy_bogons++);
		return;
	}

	origin = port->ip_dipc->dip_uid.origin;
	ident = port->ip_dipc->dip_uid.identifier;

	/*
	 *	Catch attempts to destroy kernel special ports.
	 *	If the special port is for a remote kernel, the
	 *	only information for it is in the port name
	 *	table -- it is the responsibility of the caller
	 *	to remove the port from the port name table.
	 *	The only thing we must do is mark the port
	 *	as not being special any more.
	 *
	 *	It is a fatal error to destroy a local kernel
	 *	special port.  This should never happen.
	 */
	if (ident < MAX_SPECIAL_KERNEL_ID) {
		assert(origin == dipc_node_self());
		panic("dipc_special_port_destroy:  kernel port id %d\n",
		      ident);
	}

	/*
	 *	Search the special port table for the port.
	 *	There must be an entry in the table because
	 *	the port is marked special.  When we find
	 *	the port in the table, clear it out of the
	 *	table, mark the port as !special, and release it.
	 */
	for (id = MAX_SPECIAL_KERNEL_ID; id < MAX_SPECIAL_ID; id++) {
		if (host_special_port(id) == port) {
			printf("dipc_special_port_destroy:  id #%d died\n", id);
			port->ip_special = FALSE;
			host_special_port(id) = IP_NULL;
			ip_unlock(port);
			mutex_unlock(&host_special_port_lock);
			return;
		}
	}
	mutex_unlock(&host_special_port_lock);
	panic("dipc_special_port_destroy:  port 0x%x (%d,%d) isn't in table!",
	      port, origin, ident);
}
#if	1
void
special_port_destroy(
	ipc_port_t	port);
void
special_port_destroy(
	ipc_port_t	port)
{
	dipc_special_port_destroy(port);
}
#endif


/*
 *	Handle a remote request for a special port based
 *	on its special port ID.  Return the UID of that
 *	special port.  Obviously, this action is racy:
 *	the special port may be destroyed later.  The
 *	remote node will discover such errors after it
 *	constructs a proxy for the special port and then
 *	tries to use that proxy.
 *
 *	In effect, we are "migrating" a send right to
 *	this port.  However, we aren't using the normal,
 *	message-based mechanisms to do this.  Therefore,
 *	we must do a dipc copyin operation ourselves, which
 *	will convert the send right acquired in
 *	user_get_special_port to a transit, which implicitly
 *	accompanies the UID over the wire.  The receiver must
 *	do the counterpart DIPC copyout operation.
 *
 *	XXX If the client crashes after sending us this RPC
 *	but before it gets our reply, we are left with a port
 *	that has one too many transits outstanding.  If the
 *	port also has NMS detection, it will never correctly
 *	detect no more senders. XXX
 */
dipc_return_t
dproc_get_special_port(
	unsigned long	special_id,
	uid_t		*uidp,
	boolean_t	*nms)
{
	ipc_port_t	port;
	dipc_return_t	dr;

	assert(norma_special_ports_initialized == TRUE);
	port = get_special_port(dipc_node_self(), special_id, &dr);
	if (port == IP_NULL)
		DIPC_UID_NULL(uidp);
	else {
		dr = dipc_port_copyin(port, MACH_MSG_TYPE_PORT_SEND,
				      uidp);
		assert(dr == DIPC_SUCCESS || dr == DIPC_PORT_DEAD);
		*nms = IP_NMS(port);
	}
	return DIPC_SUCCESS;
}


/*
 *	Retrieve a special port representing a remote
 *	node's NORMA_DEVICE_PORT.
 */
ipc_port_t
dipc_device_port(
	node_name	node)
{
	dipc_return_t	dr;

	return get_special_port(node, (unsigned long)NORMA_DEVICE_PORT, &dr);
}


/*
 *	Retrieve a special port representing a remote
 *	node's NORMA_HOST_PORT.
 */
ipc_port_t
dipc_host_port(
	node_name	node)
{
	dipc_return_t	dr;

	return get_special_port(node, (unsigned long) NORMA_HOST_PORT, &dr);
}


/*
 *	Retrieve a special port representing a remote
 *	node's NORMA_HOST_PRIV_PORT.
 */
ipc_port_t
dipc_host_priv_port(
	node_name	node)
{
	dipc_return_t	dr;

	return get_special_port(node, (unsigned long)NORMA_HOST_PRIV_PORT, &dr);
}


/*
 *	User interface for setting a special port.
 *
 *	Only permits the user to set a user-owned special port
 *	ID, rejecting a kernel-owned special port ID.
 *
 *	A special kernel port cannot be set up using this
 *	routine; use kernel_set_special_port() instead.
 */
kern_return_t
norma_set_special_port(
	host_t		host,
	int		id,
	ipc_port_t	port)
{
	ipc_port_t old;

	assert(norma_special_ports_initialized == TRUE);

	/*
	 *	Disallow an attempt to set a special
	 *	port without appropriate privilege.
	 */
	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	if (id < MAX_SPECIAL_KERNEL_ID || id >= MAX_SPECIAL_ID)
		return KERN_INVALID_VALUE;

	return set_special_port(host, id, port);
}


/*
 *	User interface for retrieving a special port.
 *
 *	When all processing is local, this call does not block.
 *	If processing goes remote to discover a remote UID,
 *	this call blocks but not indefinitely.  If the remote
 *	node does not exist, has panic'ed, or is booting but
 *	hasn't yet turned on DIPC, then we expect the transport
 *	to return an error.
 *
 *	This routine always returns SUCCESS, even if there's
 *	no resulting port.
 *
 *	Note that there is nothing to prevent a user special
 *	port from disappearing after it has been discovered by
 *	the caller; thus, using a special port can always result
 *	in a "port not valid" error.
 */

kern_return_t
norma_get_special_port(
	host_t		host,
	int		node,
	int		id,
	ipc_port_t	*portp)
{
	node_name	target_node;
	dipc_return_t	dr;

	assert(norma_special_ports_initialized == TRUE);

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	target_node = (node_name) node;
	if (id < 0 || id >= MAX_SPECIAL_ID || !dipc_node_is_valid(target_node))
		return KERN_INVALID_ARGUMENT;

	*portp = get_special_port(target_node, id, &dr);
	if (dr == DIPC_TRANSPORT_ERROR)
		return KERN_NODE_DOWN;
	else
		return KERN_SUCCESS;
}


/*
 *	Initialize special ports module.
 */
void
norma_special_ports_initialization()
{
	unsigned long	id;

	/*
	 *	initializED must be FALSE at this point,
	 *	but set initializING to be TRUE as a
	 *	side-effect of the third assertion.
	 */
	assert(norma_special_ports_initialized == FALSE);
	assert(norma_special_ports_initializing == FALSE);
	assert(norma_special_ports_initializing = TRUE); /* intended to be = */

	/*
	 *	Initialize special port table.
	 */
	mutex_init(&host_special_port_lock, ETAP_DIPC_SPECIAL_PORT);
	for (id = 0; id < MAX_SPECIAL_ID; ++id)
		host_special_port(id) = IP_NULL;

	/*
	 *	Register master device, host, and host_priv ports.
	 */
	kernel_set_special_port(NORMA_DEVICE_PORT, master_device_port);
	kernel_set_special_port(NORMA_HOST_PORT, realhost.host_self);
	kernel_set_special_port(NORMA_HOST_PRIV_PORT, realhost.host_priv_self);

#if	MACH_ASSERT
	norma_special_ports_initializing = FALSE;
	norma_special_ports_initialized = TRUE;
#endif
}


#if	MACH_KDB
#include <ddb/db_output.h>
#define	printf	kdbprintf

/*
 *	Dump the special ports table.
 */
void
db_dipc_special_ports(void);
void
db_dipc_special_ports(void)
{
	unsigned long	id;
	extern int	indent;

	iprintf("DIPC special ports table contents:\n");
	indent += 2;
	for (id = 0; id < MAX_SPECIAL_ID; id++)
		if (host_special_port(id) != IP_NULL)
			iprintf("id %3d:\tport 0x%x\n",
			       id, host_special_port(id));
	indent -= 2;
}
#endif	/* MACH_KDB */
