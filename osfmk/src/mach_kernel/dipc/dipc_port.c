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
 *	File:	dipc/dipc_port.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Functions to manipulate DIPC ports.
 */

#include <mach_assert.h>
#include <mach_kdb.h>
#include <kernel_test.h>
#include <dipc_timer.h>

#include <ipc/ipc_space.h>
#include <ipc/ipc_pset.h>
#include <kern/ipc_sched.h>
#include <ipc/ipc_notify.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_funcs.h>
#include <dipc/port_table.h>
#include <dipc/dipc_counters.h>
#include <mach/kkt_request.h>
#include <dipc/dipc_rpc.h>
#include <kern/lock.h>
#include <dipc/special_ports.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_timer.h>
#include <kern/zalloc.h>
#include <mach/boolean.h>
#include <mach/message.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <ddb/tr.h>
#include <kern/misc_protos.h>
#if	MACH_ASSERT
#include <kern/cpu_number.h>
#endif	/* MACH_ASSERT */
#include <string.h>
#include <machine/kkt_map.h>

/*
 *	Sensitive transit-yielding code requires
 *	special-casing for DIPC loopback mode.
 */
#define	LOOPBACK_SUPPORT	(MACH_ASSERT || KERNEL_TEST)

/*
 *	This is the number of transits that will be handed out by
 *	a principal when it receives a request for more transits
 *	from a proxy.  If we find that clients consume these
 *	transits and have to keep coming back for more, it might
 *	make sense to either have an adaptive algorithm for how
 *	many to hand out, increasing potentially exponentially with
 *	each request on a port, or to just bump up this constant.
 */
#define	DIPC_TRANSIT_UNIT	100

/*
 *	The DIPC port extension zone.
 */
#define	DIPC_PORT_ZONE_MAX	5000
zone_t	dipc_port_zone;
int	dipc_port_zone_max = DIPC_PORT_ZONE_MAX;

/*
 *	When entering a port for the first time into DIPC,
 *	we check for several interesting cases for resetting
 *	the port queue limit.  For devices, we may want to
 *	adjust the queue limit because DIPC respects queue
 *	limits while the local case does not.  The same rationale
 *	applies to other, kernel-owned ports.  (We distinguish
 *	the device and general, kernel case because we may want
 *	much higher concurrency on devices.)  For now, we don't
 *	automatically override the user queue limit.
 */
mach_port_msgcount_t	dipc_dev_qlim_default = MACH_PORT_QLIMIT_DEFAULT;
mach_port_msgcount_t	dipc_kport_qlim_default = MACH_PORT_QLIMIT_DEFAULT;


#if	MACH_ASSERT
/*
 *	Verify subsystem initialization.
 */
unsigned int	dipc_initialized = 0;
#endif


dstat_decl(unsigned int	c_dipc_port_init_calls = 0;)
dstat_decl(unsigned int	c_dipc_port_init_rejects = 0;)
dstat_decl(unsigned int	c_dipc_port_init_races = 0;)
dstat_decl(unsigned int	c_dipc_port_free_calls = 0;)
dstat_decl(unsigned int	c_dipc_port_free_in_use = 0;)
dstat_decl(unsigned int c_dipc_kmsg_converts = 0;)
dstat_decl(unsigned int c_dipc_recv_copyin = 0;)
dstat_decl(unsigned int c_dipc_port_copyout_creates = 0;)
dstat_decl(unsigned int c_dipc_port_copyout_recvs = 0);
dstat_decl(unsigned int c_dipc_needed_transits = 0);
dstat_decl(unsigned int c_dipc_port_copyout_races = 0);

/*
 *	Track recursion when converting messages
 *	to network format while migrating receive rights.
 *	This case arises when a migrating message itself
 *	contains a receive right with queued messages.
 *	This case is unbounded, hence our concern.
 */
dcntr_decl(unsigned int c_dipc_recursion_level[NCPUS];)
dcntr_decl(unsigned int c_dipc_max_recursion_level[NCPUS];)
#define	xxx_recursion_level	c_dipc_recursion_level[cpu_number()]
#define	xxx_max_recursion_level	c_dipc_max_recursion_level[cpu_number()]

dcntr_decl(unsigned int c_dipc_dn_inits = 0;)
dcntr_decl(unsigned int c_dipc_recv_migrations = 0;)

/*
 *	Forward declarations.
 */
dipc_return_t
dipc_port_wait_for_transits(
	ipc_port_t	port);


dipc_return_t
dipc_port_dn_register(
	ipc_port_t	port,
	node_name	node);

kkt_return_t
drpc_begin_recv_migrate(
	node_name	node,
	uid_t		*uidp,
	node_name	mnode,
	dipc_return_t	*dr);

dipc_return_t
dipc_port_yield_transits(
	ipc_port_t	port);

dipc_return_t
dipc_port_kmsgs_convert(
	ipc_port_t	port);

void
dipc_migration_thread(void);

void
dipc_migration_rpc_deliver(
	channel_t	channel,
	rpc_buf_t	rpc_buf,
	boolean_t	thread_context);

void
dipc_migration_request(
	rpc_buf_t	rpc_buf,
	boolean_t	do_everything);

void
dipc_migration_thread_init(void);

/*
 *	Initialize a port for DIPC operation.  The caller
 *	indicates whether a UID should be assigned at
 *	this time.
 *
 *	The DIPC port extension is initialized to the
 *	following defaults:
 *		- port is a principal
 *		- port is not special
 *		- port is not a forwarding proxy
 *		- port has no queue limit or resource
 *		shortage problems
 *
 *	Port initialization is racy.  Threads on separate
 *	processors on an SMP node may be racing to enter a port
 *	into DIPC.  For example, this can happen when both
 *	threads are trying to migrate different send rights to
 *	the same port.  We want to guarantee that, whenever the
 *	port's ip_dipc pointer is non-NULL, it always points to
 *	a completely valid dipc_port.  We achieve this guarantee
 *	by entirely initializing the dipc_port before attaching
 *	it to the ipc_port.
 *
 *	Special port handling is another interesting case.  A
 *	local thread can be entering a port into DIPC while
 *	a DIPC thread carries out a lookup operation for a
 *	special port.  This problem is solved by forcing a
 *	special port into DIPC at the time it is registered.
 *
 *	However, if the caller chooses assign_uid==FALSE, then
 *	the port is only partly initialized on return -- it has
 *	no UID -- and the caller has responsibility for handling
 *	any races.
 */
dipc_return_t
dipc_port_init(
	ipc_port_t	port,
	boolean_t	assign_uid)
{
	dipc_port_t		dip;
	dipc_return_t		dr;
	boolean_t		reset_qlim;
	mach_port_msgcount_t	new_qlim;

	dstat(c_dipc_port_init_calls++);
	if (DIPC_IS_DIPC_PORT(port)) {
		dstat(c_dipc_port_init_rejects++);
		return DIPC_DUPLICATE;
	}

	dip = (dipc_port_t) zalloc(dipc_port_zone);
	assert(dip != DIP_NULL);

	/*
	 *	Set proxy, special, forward and other flags
	 *	to FALSE by setting migrate_state to 0.
	 */
	dip->dip_flag_bits = 0;

	dip->dip_qf_count = 0;

	dip->dip_dest_node = NODE_NAME_NULL;
	dip->dip_transit = dip->dip_remote_sorights = 0;
	dip->dip_hash_next = IP_NULL;
	dip->dip_ref_count = 0;
	dip->dip_callback_map = NODE_MAP_NULL;
	dip->dip_restart_list = IP_NULL;
	dip->dip_node_wait = 0;
	dip->dip_node_wait_list = IP_NULL;

#if	DIPC_DO_STATS
	dip->dip_queue_full = 0;
	dip->dip_queue_max = 0;
	dip->dip_enqueued = 0;
	dip->dip_thread_hit = 0;
#endif	/* DIPC_DO_STATS */

#if	MACH_ASSERT
	dip->dip_inited = (long) port;
	dip->dip_spares[0] =
		dip->dip_spares[1] =
			dip->dip_spares[2] = 0;
#endif	/* MACH_ASSERT */

	reset_qlim = FALSE;
	switch (ip_kotype(port)) {
	    case IKOT_NONE:
		/*
		 *	User port, ignore.
		 */
		break;
	    case IKOT_DEVICE:
	    case IKOT_MASTER_DEVICE:
		/*
		 *	Device.
		 */
		reset_qlim = TRUE;
		new_qlim = dipc_dev_qlim_default;
		break;
	    default:
		/*
		 *	Other kernel port.
		 */
		reset_qlim = TRUE;
		new_qlim = dipc_kport_qlim_default;
		break;
	}

	/*
	 *	Attempt to atomically enter the port
	 *	into the UID table, then assign the
	 *	dipc_port to the port.  By assigning
	 *	ip_dipc *last*, anyone else looking
	 *	at the port will think it's local
	 *	until the last possible instant, at
	 *	which point it will atomically
	 *	transition to an initialized DIPC port.
	 *
	 *	If assign_uid==FALSE, the caller had
	 *	better know what's going on.
	 */
	 
	ip_lock(port);
	if (DIPC_IS_DIPC_PORT(port)) {
		/*
		 *	Lost race... someone else managed to slip
		 *	this port into DIPC in the meantime.
		 *	We've "lost" a UID but there's no good
		 *	way at this point to reclaim one.
		 */
		ip_unlock(port);
		zfree(dipc_port_zone, (vm_offset_t)dip);
		dstat(c_dipc_port_init_races++);
		return DIPC_DUPLICATE;
	}
	if (assign_uid == TRUE) {
		/*
		 *	The following call will attach the dipc port
		 *	extension to the port, while atomically
		 *	assigning the UID.
		 */
		dr = dipc_uid_allocate(port, dip);
		assert(dr == DIPC_SUCCESS);
	} else {
		DIPC_UID_MAKE(&dip->dip_uid, NODE_NAME_NULL, PORT_ID_NULL);
		port->ip_dipc = dip;
	}
	/*
	 *	Port can't be migrating yet, so there's no
	 *	worry even if there are already messages
	 *	or blocked senders on this port.  We never
	 *	decrease the qlimit.
	 */
	if (reset_qlim == TRUE && new_qlim > port->ip_qlimit)
		ipc_port_set_qlimit(port, new_qlim);
	ip_unlock(port);

	assert(DIPC_IS_DIPC_PORT(port));
	return DIPC_SUCCESS;
}


/*
 *	Destroy the dipc_port extension.
 *
 *	Caller believes no other thread knows about
 *	this port, but the caller has no way to prevent
 *	DIPC interrupt-level races.
 *
 *	Returns DIPC_SUCCESS if able to free up the
 *	dipc_port, DIPC_IN_USE if the dipc_port is
 *	still being used from interrupt level.  In
 *	the latter case, the caller should refrain
 *	from deallocating the port.
 *
 *	The caller supplies a locked port and is
 *	returned an unlocked port.
 */
dipc_return_t
dipc_port_free(
	ipc_port_t	port)
{
	dipc_return_t dr;

	assert(DIPC_IS_DIPC_PORT(port));

	dstat(c_dipc_port_free_calls++);

	/*
	 *	Remove the port's UID from the name table.
	 *	For principals, this could be done in
	 *	ipc_port_destroy (where it is redundant with
	 *	making the port dead) but for proxies this
	 *	work must be done here, anyway.
	 *
	 *	There is a race with an interrupt-level entity
	 *	using the port at the same time:  in that case,
	 *	just give up.  The other entity will convert
	 *	its DIPC reference to a port reference, then
	 *	decrement the port reference and try to kill
	 *	the port all over again.
	 */
	dr = dipc_uid_remove(port);
	if (dr == DIPC_IN_USE) {
		assert(port->ip_references > 0 || port->ip_ref_count > 0);
		ip_unlock_absolute(port);
		dstat(++c_dipc_port_free_in_use);
		return dr;
	}
	assert(dr == DIPC_SUCCESS);

	/*
	 *	At this point, the port has been removed
	 *	from the port name table and so is no
	 *	longer accessible via interrupt-level
	 *	DIPC operations.  Moreover, we know that
	 *	no other interrupt-level operations are
	 *	in progress, as dipc_uid_remove succeeded.
	 *
	 *	Any remaining references represent operations
	 *	that began at interrupt level but have already
	 *	passed into thread context.  Those references
	 *	will be released soon, and eventually cause
	 *	another trip through io_free.
	 */
	if (port->ip_references != 0) {
		assert(port->ip_references > 0 || port->ip_ref_count > 0);
		ip_unlock_absolute(port);
		dstat(++c_dipc_port_free_in_use);
		return DIPC_IN_USE;
	}

	/*
	 *	No longer require the port lock:  all races have
	 *	been resolved.  There is no way for DIPC interrupt-
	 *	level operations to find the port, and no other
	 *	thread knows about the port.
	 */
	port->ip_object.io_bits &= ~IO_BITS_ACTIVE;
	assert(port->ip_references == 0 && port->ip_ref_count == 0);
	ip_unlock_absolute(port);

	/*
	 *	
	 *	Clean the port:  yield transits if necessary.
	 */
	if (IP_NMS(port) && DIPC_IS_PROXY(port)) {
		dr = dipc_port_yield_transits(port);
		assert(dr == DIPC_SUCCESS);
	}

	/*
	 *	Special ports shouldn't be special
	 *	by this time.
	 */
	assert(port->ip_special == FALSE);

	/*
	 *	Reclaim dipc_port.
	 */
	zfree(dipc_port_zone, (vm_offset_t)port->ip_dipc);

	/*
	 *	Make port state consistent.
	 */
	DIPC_INIT_POINTER(port);
	return DIPC_SUCCESS;
}


/*
 *	Tell caller whether port is a proxy or a principal.
 */
boolean_t
dipc_is_remote(
	ipc_port_t	port)
{
	return DIPC_IS_PROXY(port);
}


#define USER_SEND_HANDLES	100
int	user_send_handles = USER_SEND_HANDLES;
#define USER_RECV_HANDLES	300
int	user_recv_handles = USER_RECV_HANDLES;

/*
 * Routine:	dipc_port_kmsgs_convert
 *
 * Convert all of the kmsgs in a ports queue to network format
 *
 * Preconditions:
 *	Port is not locked
 *	Port has been marked for conversion
 *
 * Comments:
 *	The local IPC path guarantees that the receive right
 *	is no longer accessible for receiving messages.  Thus,
 *	we don't have to worry about messages being dequeued.
 *	Therefore, this routine may assume that the queue next
 *	pointers are always valid.
 *
 *	New messages may be enqueued, but they will be added
 *	at the end of the queue and will be converted to
 *	DIPC_FORMAT by the enqueuing thread.  Therefore, there
 *	is no way for this routine to lose a race that would
 *	leave a local format message on the queue.
 *
 * Warning:
 *	XXX This routine can be recursively invoked by dipc_kmsg_copyin
 *	when encountering a queued message with another migrating
 *	receive right.  This recursion is unbounded and one day
 *	must be fixed. XXX
 */

dipc_return_t
dipc_port_kmsgs_convert(
	ipc_port_t	port)
{
	ipc_kmsg_t		kmsg;
	ipc_kmsg_queue_t	queue;
	ipc_mqueue_t		mqueue = &port->ip_messages;
	dipc_return_t		dr;

	/*
	 * Run the kmsg queue and convert all messages to network
	 * format.
	 */
	dcntr((xxx_recursion_level++ >= xxx_max_recursion_level) ?
	      xxx_max_recursion_level = xxx_recursion_level : 0);
	assert(port->ip_pset == IPS_NULL);
	assert(port->ip_dipc->dip_ms_convert == TRUE);
	imq_lock(mqueue);
	queue = &mqueue->imq_messages;
	for(kmsg = ipc_kmsg_queue_first(queue);
	    kmsg != IKM_NULL;
	    kmsg = ipc_kmsg_queue_next(queue, kmsg)) {
		imq_unlock(mqueue);
		/*
		 *	We could have an entire inline kmsg with OOL
		 *	components that haven't yet been moved over.
		 *	This is a tough thing to migrate:  ideally,
		 *	we leave the OOL components on the other node.
		 *	But then the next guy might need TWO handles:
		 *	one to retrieve the inline kmsg from us and
		 *	another for retrieving the OOL components from
		 *	the original node.  Simplify life:  bring over
		 *	the entire kmsg to this node, THEN migrate it.
		 */
		if (KMSG_IS_DIPC_FORMAT(kmsg) && KMSG_RECEIVING(kmsg) &&
		    KMSG_HAS_HANDLE(kmsg)) {
			assert(kmsg->ikm_header.msgh_bits &
			       MACH_MSGH_BITS_COMPLEX_OOL);
			dr = dipc_kmsg_copyout(kmsg, current_task()->map,
					       MACH_MSG_BODY_NULL);
			assert(dr == DIPC_SUCCESS);
		}
		if (!KMSG_IS_DIPC_FORMAT(kmsg)) {
			dstat(c_dipc_kmsg_converts++);
			dr = dipc_kmsg_copyin(kmsg);
			assert(dr == DIPC_SUCCESS);
		}
		imq_lock(mqueue);
	}
	imq_unlock(mqueue);
	dcntr(xxx_recursion_level--);
	return (DIPC_SUCCESS);
}


/*
 * Routine:	dipc_port_copyin
 *
 * Given a local port return a DIPC uid for the port.  If the port
 * has never been exported, initialize it for remote access.
 *
 * Preconditions:
 *	Port is unlocked if the port exists when this call is made,
 *	the port will continue to exist unless this routine releases
 *	the last reference which represents the right being copied in.
 *	This may be done by the caller holding a reference, or by the
 *	caller being able to reason about the state of this port
 *
 * Postconditions:
 *	Port is unlocked.  We consume the reference which the right 
 *	represented since we are taking that right off node.  This could
 *	cause the port to disapear.  If the right is a RECEIVE_RIGHT
 *	then an additional reference is held until dead name notification time
 */

dipc_return_t
dipc_port_copyin(
	ipc_port_t		port,
	mach_msg_type_name_t	type,
	uid_t			*uid)
{
	dipc_return_t		dr;
	TR_DECL("dipc_port_copyin");

	tr_start();
	tr3("port=0x%x type=0x%x", port, type);

	/*
	 * We must have a DIPC port with a uid in all cases
	 */

	assert(port != IP_NULL);

	if (port == IP_DEAD || !ip_active(port)) {
		DIPC_UID_DEAD(uid);
		tr1("exit dead");
		tr_stop();
		return (DIPC_PORT_DEAD);
	}

	if (!DIPC_IS_DIPC_PORT(port)) {
		dr = dipc_port_init(port, TRUE);
		assert((dr==DIPC_DUPLICATE)||(dr==DIPC_SUCCESS));
	}

      Restart:

	tr4("...port 0x%x is_dipc %d ref %d", port, DIPC_IS_DIPC_PORT(port),
	    port->ip_references);
	ip_lock(port);
    
	assert(DIPC_IS_DIPC_PORT(port));
	assert(port->ip_dipc != (struct dipc_port *) 0xdeadbeef);
	assert(DIPC_UID_VALID(&port->ip_dipc->dip_uid));

	tr3("...uid 0x%x,0x%x",port->ip_uid.origin, port->ip_uid.identifier);
	if (!ip_active(port)) {
		DIPC_UID_DEAD(uid);
		ip_unlock(port);
		tr1("exit dead 2");
		tr_stop();
		return (DIPC_PORT_DEAD);
	}

	/* wait for migration to finish */
	while (port->ip_ms_block) {
		assert(type != MACH_MSG_TYPE_PORT_RECEIVE);
		assert_wait((event_t)port, FALSE);
		ip_unlock(port);
		thread_block(0);
		ip_lock(port);
	}

	switch (type) {

	    case MACH_MSG_TYPE_PORT_RECEIVE: {

		assert(!DIPC_IS_PROXY(port));

		dstat(c_dipc_recv_copyin++);

		/*
		 * Switch to first migration state.
		 * We allow further messages to be
		 * queued, but they must be in network
		 * format
		 */
		port->ip_dipc->dip_ms_convert = TRUE;

		/*
		 * We don't consume the receive rights reference here.
		 * We consume it at dead name notification time
		 */

		ip_unlock(port);
		dipc_port_kmsgs_convert(port);
		*uid = port->ip_uid;
		break;
	    }

	    case MACH_MSG_TYPE_PORT_SEND: {

		assert(port->ip_srights > 0);

		if (IP_NMS(port)) {
			if (DIPC_IS_PROXY(port)) {
				assert(port->ip_transit > 0);
				/* 
				 * We need to keep at least one local
				 * transit if there are going to be
				 * any srights left
				 */
				while((port->ip_srights > 1) &&
				      port->ip_transit == 1) {
					dr = dipc_port_wait_for_transits(port);
					if (dr == DIPC_PORT_MIGRATED)
					    goto Restart;
					if (dr != DIPC_SUCCESS) {
					    if (dr == DIPC_PORT_DEAD)
						DIPC_UID_DEAD(uid);
					    tr2("exit dr=0x%x", dr);
					    tr_stop();
					    return (dr);
					}
				}
			}

			/*
			 * Either decrease proxy transit count
			 * or increase principals remote
			 * transit count
			 */
			port->ip_transit--;
		}
		port->ip_srights--;		/* Consume local sright */

		/*
		 *	Must copy port's UID information now, as
		 *	releasing a reference may deallocate the
		 *	port if it is a proxy.
		 */
		*uid = port->ip_uid;

		assert(port->ip_references > 1 || DIPC_IS_PROXY(port));
		ip_release(port);		/* This is the srights ref */
		tr3("...release send right, port 0x%x, ref now %d",
		    port, port->ip_references);
		ip_check_unlock(port);
		break;
	    }

	    case MACH_MSG_TYPE_PORT_SEND_ONCE: {

		assert(port->ip_sorights > 0);

		/*
		 * Principal only:  track remote sorights.
		 * Proxies can't make sorights.
		 */
		if (!DIPC_IS_PROXY(port)) {
			port->ip_remote_sorights++;
		}
		/*
		 * Decrease local soright count
		 */
		port->ip_sorights--;

		/*
		 *	Must copy port's UID information now, as
		 *	releasing a reference may deallocate the
		 *	port if it is a proxy.
		 */
		*uid = port->ip_uid;

		assert(port->ip_references > 1 || DIPC_IS_PROXY(port));
		ip_release(port);		/* This is the sorights ref */
		tr3("...release send-once right, port 0x%x, ref now %d",
		    port, port->ip_references);
		ip_check_unlock(port);
		break;
	    }

	    default:
		panic("dipc_port_copyin, port 0x%x, unknown type", port);
		ip_unlock(port);
		break;
	}

	tr1("exit");
	tr_stop();
	return (DIPC_SUCCESS);
}


/*
 * Routine:	dipc_port_enqueued
 *
 * This is called when a message has been successfully 
 * enqueued remotely.  Currently this simply releases an
 * extra reference on the remote port (acquired in
 * dipc_kmsg_copyin) and optionally reclaims its transit.
 *
 * Preconditions:
 *	Port is referenced and locked
 * Postconditions:
 *	Port is unreferenced and locked
 */

dipc_return_t
dipc_port_dest_done(
	ipc_port_t		port,
	mach_port_type_t	type)

{
	if (!IP_NMS(port)) {
		ip_release(port);
		return(DIPC_SUCCESS);
	}

	assert(DIPC_IS_DIPC_PORT(port));
	assert(DIPC_UID_VALID(&port->ip_dipc->dip_uid));
	assert(DIPC_IS_PROXY(port));

	switch (type) {
	    case MACH_MSG_TYPE_PORT_SEND:
		port->ip_transit++;		/* Reclaim transit */
		break;

	    case MACH_MSG_TYPE_PORT_SEND_ONCE:
		/* There is nothing to do for send once rights */
		break;

	    default:
		assert(FALSE);
		break;
	}

	/*
	 *	Release one of the references acquired in
	 *	dipc_kmsg_copyin.  The port might when it is unlocked.
	 */
	ip_release(port);
	return (DIPC_SUCCESS);
}


/*
 * Routine:	dipc_port_copyout
 *
 * Take uid and convert to local port.  If the port does not yet exist
 * on this node, create a port structure, and insert the uid in the local
 * uid table.
 *
 * Preconditions:
 *	Port is unlocked
 *
 * Postconditions:
 *	A reference is taken to represent the right locally
 */
dipc_return_t
dipc_port_copyout(
	uid_t			*uidp,
	mach_port_type_t	type,
	node_name		rnode,
	boolean_t		destination,
	boolean_t		nms,
	ipc_port_t		*return_port)
{
	ipc_port_t		port, ipdest;
	dipc_return_t		dr;
	TR_DECL("dipc_port_copyout");

	tr_start();
	tr5("uid=0x%x,%x type=0x%x rnode=0x%x", uidp->origin, uidp->identifier,
	   type, rnode);

	assert(rnode != NODE_NAME_NULL);

	if (DIPC_IS_UID_NULL(uidp)) {
	    *return_port = IP_NULL;
	    tr_stop();
	    return (DIPC_SUCCESS);
	}

	if (DIPC_IS_UID_DEAD(uidp)) {
	    *return_port = IP_DEAD;
	    tr_stop();
	    return (DIPC_PORT_DEAD);
	}

	/*
	 * Check to see if we have seen this port before and still have
	 * a local port for it.  If not, then allocate a port in the ipc
	 * space ipc_space_remote and initialize it with the specified uid
	 */
    Retry:
	if (dipc_uid_lookup(uidp, &port) == DIPC_NO_UID) {
		mach_port_t name;
		kern_return_t kr;
	
		assert(!destination);
	
		dstat(c_dipc_port_copyout_creates++);

		port = ipc_port_alloc_special(ipc_space_remote);
		assert(port != IP_NULL);

		/*
		 *	It's OK to defer assigning the UID
		 *	because no one else yet knows about
		 *	this port.
		 */
		dr = dipc_port_init(port, FALSE);
		assert(dr == DIPC_SUCCESS);
	
		/*
		 * Additional state to initialize; no one
		 * else knows yet about this port so it's
		 * not necessary to lock it.
		 */
		if (nms)
			IP_SET_NMS(port);
		else
			IP_CLEAR_NMS(port);

		/*
		 *	Even a migrating receive right begins
		 *	its life on a new node as a proxy.
		 */
		port->ip_proxy = TRUE;
		port->ip_dipc->dip_ms_convert = TRUE;

		/*
		 * If the right is a receive or send once, then the likely
		 * location of the principal is the sending node (actually
		 * it IS the sending node for receive rights).  If it is
		 * a send right, then the best guess we can make is the
		 * node of origin in the UID.
		 */
		switch (type) {
		    case MACH_MSG_TYPE_PORT_SEND:
			port->ip_dest_node = uidp->origin;
			assert(dipc_node_is_valid(port->ip_dest_node));
			break;

		    case MACH_MSG_TYPE_PORT_SEND_ONCE:
			if (rnode == dipc_node_self())
				port->ip_dest_node = uidp->origin;
			else
				port->ip_dest_node = rnode;
			assert(dipc_node_is_valid(port->ip_dest_node));
			assert(port->ip_dest_node != dipc_node_self());
			break;

		    case MACH_MSG_TYPE_PORT_RECEIVE:
			port->ip_source_node = rnode;
			/*
			 *	There is a small window here between
			 *	the time we create the new port and
			 *	the time we set ms_convert and ms_block,
			 *	below.  This window does not matter:
			 *	no one knows about this port yet.  All
			 *	senders will be trying to reach the
			 *	receive right on the other node, which
			 *	will block them.
			 */
			assert(dipc_node_is_valid(port->ip_source_node));
			break;

		    default:
			assert(FALSE);	/* impossible! */
			break;
		}

		dr = dipc_uid_install(port, uidp);   /* Install in uid table */
		if (dr != DIPC_SUCCESS) {
			assert(dr == DIPC_DUPLICATE);
			tr2("port 0x%x DUPLICATE UID", port);
			dstat(++c_dipc_port_copyout_races);
			/*
			 *	It's more work than it's worth to fix
			 *	the port up at this point so that it can
			 *	be destroyed through normal means.
			 */
			zfree(dipc_port_zone, (vm_offset_t) port->ip_dipc);
			DIPC_INIT_POINTER(port);
			io_free(IOT_PORT, (ipc_object_t) port);
			goto Retry;
		}
		ip_lock(port);
	} else {
		dr = dipc_uid_port_reference(port);
		assert(dr == DIPC_SUCCESS);
		ip_lock(port);
	}

	assert(!DIPC_IS_PROXY(port) || !destination);

	switch (type) {

	    case MACH_MSG_TYPE_PORT_RECEIVE: {
		ipc_kmsg_t		kmsg;
		ipc_mqueue_t		mqueue = &port->ip_messages;
		ipc_kmsg_queue_t	queue;
		struct ipc_kmsg_queue	save_queue;
		kkt_return_t		kktr;

		assert(!destination);

		dstat(c_dipc_port_copyout_recvs++);

		/*
		 *	It is necessary to convert the proxy into
		 *	a principal now.  This is so we can receive
		 *	any messages that were enqueued on the
		 *	migrating port -- these messages will start
		 *	flowing after we send the begin_recv_migrate
		 *	RPC, below.
		 *
		 *	Some information must be set now:  the fact
		 *	that the port is a principal.
		 *
		 *	Some information could be set almost any time:
		 *	the ip_receiver_name and ip_destination, which
		 *	must match what local IPC expects.  It doesn't
		 *	hurt to set this up now.
		 *
		 *	Some information won't be set until the
		 *	entire state of the port has been sent over:
		 *	this information includes the sequence number,
		 *	queue limit, and transit information.
		 */

		port->ip_proxy = FALSE;
		port->ip_forward = FALSE;
		port->ip_source_node = rnode;
		assert(dipc_node_is_valid(port->ip_source_node));

		/*
		 * Change state so messages are no longer converted
		 * and senders block until receive right arrives
		 */
		port->ip_dipc->dip_ms_convert = FALSE;
		port->ip_dipc->dip_ms_block = TRUE;

		/*
		 *	Fix up port state so the IPC system will
		 *	think the port is OK.  We must put the port
		 *	in limbo (receiver_name and destination
		 *	both NULL).  A side-effect of blowing away
		 *	the destination is losing the relationship
		 *	between the port and the space in which it
		 *	was originally allocated.  This is OK.
		 *
		 *	It's not clear that we have to do the
		 *	ipc_port_clear_receiver; however, the proxy
		 *	might have blocked senders while the receive
		 *	right is migrating over.  Check this.  XXX
		 *
		 *	If the message containing the migrating receive
		 *	right was copied in on this node,
		 *	ipc_port_check_circularity left a pointer to
		 *	the destination port in ip_destination, along
		 *	with an extra reference on that port.  We need
		 *	to drop that reference, so save the port and
		 *	drop the reference when we detect this situation
		 *	below (rnode == dipc_node_self()).
		 */
		ipdest = port->ip_destination;

		ipc_port_clear_receiver(port);
		port->ip_receiver_name = MACH_PORT_NULL;
		port->ip_destination = IP_NULL;

		/*
		 *	This flag must be set BEFORE sending the
		 *	begin_recv_migrate RPC, otherwise if there
		 *	are no messages to send it's possible for
		 *	the receiver to send back its reply before
		 *	we get a chance to block, below.
		 */
		port->ip_migrate_wait = TRUE;

		ip_unlock(port);

		/*
		 *	Detect occasional case of receive right
		 *	that never finished migrating off node.
		 *	This can happen when a port is put into
		 *	a message to be sent off-node but the
		 *	message winds up being received locally.
		 */
		if (rnode == dipc_node_self()) {
			if (IP_VALID(ipdest))
				ipc_port_release(ipdest);
			ip_lock(port);
			port->ip_migrate_wait = FALSE;
			goto messages_already_here;
		}

		/*
		 * We need to remove any kmsgs from the proxy's queue
		 * until after we receive all of the kmsgs from the
		 * principal's queue to maintain message ordering.
		 */

		ipc_kmsg_queue_init(&save_queue);
		imq_lock(mqueue);
		queue = &mqueue->imq_messages;
		while((kmsg = ipc_kmsg_dequeue(queue)) != IKM_NULL)
			ipc_kmsg_enqueue(&save_queue,kmsg);
		imq_unlock(mqueue);

		/*
		 * Tell principal to begin migration of port
		 */

		kktr = drpc_begin_recv_migrate(rnode, uidp,
					       dipc_node_self(), &dr);
		assert(kktr == KKT_SUCCESS);
		assert(dr == DIPC_SUCCESS);

		/*
		 * We now block waiting for the port to become a principal.
		 * The conversion will take place as part of the migrate_state
		 * RPC handling.
		 */
		ip_lock(port);
		while (port->ip_migrate_wait) {
			assert_wait((event_t)&port->ip_dipc->dip_flag_bits,
				    FALSE);
			ip_unlock(port);
			thread_block(0);
			ip_lock(port);
		}

		/*
		 * We now restore the messages that were resident on the proxy
		 * before the right migrated
		 */

		imq_lock(mqueue);
		while((kmsg = ipc_kmsg_dequeue(&save_queue)) != IKM_NULL)
			ipc_kmsg_enqueue(queue,kmsg);
		imq_unlock(mqueue);

	    messages_already_here:
		/*
		 * We can now allow messages to be enqueued
		 */

		port->ip_dipc->dip_ms_block = FALSE;

		ip_unlock(port);

		/*
		 * We now can wakeup any local threads which blocked during
		 * the migration
		 */
		thread_wakeup((event_t)port);

		break;
	    }

	    case MACH_MSG_TYPE_PORT_SEND: {
		/*
		 * Increase local send right count;
		 * if NMS is enabled, account for
		 * a transit as well
		 */
		port->ip_srights++;
		if (IP_NMS(port) && !destination) {
			port->ip_transit++;
		}
		ip_unlock(port);
		break;
	    }

	    case MACH_MSG_TYPE_PORT_SEND_ONCE: {
		/*
		 * Increase local sendonce right count;
		 * if the port is a principal, then
		 * there is one less soright outstanding
		 */
		port->ip_sorights++;
		if (!DIPC_IS_PROXY(port))
			port->ip_remote_sorights--;
		ip_unlock(port);
		break;
	    }

	    default:
		assert(FALSE);
		break;
	}

	*return_port = port;
	tr2("exit: port=0x%x", port);
	tr_stop();
	return(DIPC_SUCCESS);
}


/*
 * Routine:	dipc_port_update
 *
 * This routine will get up to date location and aliveness information
 * about the port by contacting the principal
 *
 * The information could be out of date by the time this routine returns;
 * however, the port's UID stays intact as long as the port doesn't disappear.
 */

dipc_return_t
dipc_port_update(
	ipc_port_t	port)
{
	node_name	node;
	dipc_return_t	dr;
	kkt_return_t	kktr;

	ip_lock(port);

	assert(DIPC_IS_DIPC_PORT(port));
	assert(DIPC_UID_VALID(&port->ip_uid));
	
	node = port->ip_dest_node;
	assert(dipc_node_is_valid(node));
	dr = (DIPC_IS_PROXY(port)?DIPC_PORT_PROXY:DIPC_PORT_PRINCIPAL);
	ip_unlock(port);
	
	/*
	 * Chase down the principal even if it means running the forward
	 * proxy gambit
	 */
	
	while (dr != DIPC_PORT_DEAD && dr != DIPC_PORT_PRINCIPAL) {
	        kktr = drpc_port_probe(node, &port->ip_uid, &dr, &node);
		assert(kktr == KKT_SUCCESS);
		switch (dr) {
		    case DIPC_PORT_PROXY_FORWARD:
			ip_lock(port);
			port->ip_dest_node = node;
			assert(dipc_node_is_valid(port->ip_dest_node));
			ip_unlock(port);
			break;

		    case DIPC_PORT_PROXY:
			assert(FALSE);
			break;

		    case DIPC_PORT_PRINCIPAL:
			break;

		    case DIPC_PORT_DEAD:
			ip_lock(port);		/* To be consumed */
			ip_reference(port);	/* To be consumed */
			ipc_port_destroy(port);
			break;

		    default:
			assert(FALSE);
		}
	}
	return (DIPC_SUCCESS);
}


/*
 * Routine:	dproc_port_probe
 *
 * Return this node's current information about the state and
 * principal location for this port
 */

dipc_return_t
dproc_port_probe(
	uid_t		*uidp,
	node_name	*new_node)
{
	dipc_return_t	dr;
	ipc_port_t	port;
	
	dr = dipc_uid_lookup(uidp, &port);
	if (dr != DIPC_SUCCESS)
	    return (DIPC_PORT_DEAD);
	
	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);
	
	ip_lock(port);
	ip_release(port);
	if (!ip_active(port)) {
		dr = DIPC_PORT_DEAD;
	} else if (!DIPC_IS_PROXY(port)) {
		dr = DIPC_PORT_PRINCIPAL;
	} else if (port->ip_forward) {
		dr = DIPC_PORT_PROXY_FORWARD;
		*new_node = port->ip_dest_node;
		assert(dipc_node_is_valid(port->ip_dest_node));
	} else {
		assert(DIPC_IS_PROXY(port));
		*new_node = port->ip_dest_node;
		assert(dipc_node_is_valid(port->ip_dest_node));
		dr = DIPC_PORT_PROXY;
	}
	ip_unlock(port);
	return (dr);
}

/*
 * Routine:	dipc_port_wait_for_transits
 *
 * Get more transits from principal
 *
 * Preconditions:
 *	Port is locked
 *
 * Postconditions:
 *	Port is locked on DIPC_SUCCESS
 */
dipc_return_t
dipc_port_wait_for_transits(
	ipc_port_t	port)
{
	kkt_return_t	kktr;
	dipc_return_t	dr;
	node_name	node, new_node;
	long		transits = 0;


	assert(DIPC_IS_DIPC_PORT(port));
	assert(DIPC_UID_VALID(&port->ip_dipc->dip_uid));
	assert(DIPC_IS_PROXY(port));

	dstat(c_dipc_needed_transits++);

	node = port->ip_dest_node;
	assert(dipc_node_is_valid(port->ip_dest_node));

	ip_unlock(port);

	/*
	 * We may need to chase forward proxies to get transits
	 */

	dr = DIPC_PORT_MIGRATED;
	while (dr == DIPC_PORT_MIGRATED) {
		kktr = drpc_acquire_transits(node, &port->ip_uid, &dr,
					     &transits, &new_node);
		assert(kktr == KKT_SUCCESS);
		if (dr == DIPC_PORT_MIGRATED) {
			ip_lock(port);
			port->ip_dest_node = node = new_node;
			assert(dipc_node_is_valid(port->ip_dest_node));
			if (node == dipc_node_self())
				break;
			ip_unlock(port);
		}
	}

	if (dr == DIPC_SUCCESS) {
		ip_lock(port);
		port->ip_transit += transits;
		return (DIPC_SUCCESS);
	} else if (dr == DIPC_PORT_DEAD) {
		ip_lock(port);
		ip_reference(port); /* To be consumed */
		ipc_port_destroy(port);
	} else if (dr ==  DIPC_PORT_MIGRATED) {
		ip_unlock(port);
	} else
		assert(FALSE);

	return (dr);
}


/*
 * Routine:	dproc_acquire_transits
 *
 * Give transits from the principal to the requesting proxy.  If this
 * is not the principal, give updated location information
 */

dipc_return_t
dproc_acquire_transits(
	uid_t		*uidp,
	long		*transits,
	node_name	*new_node)
{
	ipc_port_t	port;
	dipc_return_t	dr;

	dr = dipc_uid_lookup(uidp, &port);
	if (dr != DIPC_SUCCESS)
		return (DIPC_PORT_DEAD);

	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);

	ip_lock(port);
	ip_release(port);
	if (!ip_active(port)) {
		dr = DIPC_PORT_DEAD;
	} else if (DIPC_IS_PROXY(port)) {
		*new_node = port->ip_dest_node;
		assert(dipc_node_is_valid(port->ip_dest_node));
		dr = DIPC_PORT_MIGRATED;
	} else {
		port->ip_transit -= DIPC_TRANSIT_UNIT;
		*transits = DIPC_TRANSIT_UNIT;
		dr = DIPC_SUCCESS;
	}
	ip_check_unlock(port);

	return (dr);
}


/*
 * Routine:	dipc_port_yield_transits
 *
 * Yield any leftover transits from a proxy port.
 *
 * The port is dead:  there is absolutely no other
 * way to find it.  Port locking is unnecessary.
 */

dipc_return_t
dipc_port_yield_transits(
	ipc_port_t	port)
{
	kkt_return_t	kktr;
	node_name	node;
	uid_t		uid;
	long		transits;

	assert(DIPC_IS_DIPC_PORT(port));
	assert(DIPC_UID_VALID(&port->ip_dipc->dip_uid));
	assert(DIPC_IS_PROXY(port));
	assert(!ip_active(port));
	assert(port->ip_references == 0 && port->ip_ref_count == 0);
	assert(IP_NMS(port));

	node = port->ip_dest_node;
	assert(dipc_node_is_valid(port->ip_dest_node));
	transits = port->ip_transit;

#if	LOOPBACK_SUPPORT
	/*
	 *	Yielding transits back to ourself is a
	 *	losing proposition.
	 */
	if (node == NODE_NAME_LOCAL)
		return (DIPC_SUCCESS);
#endif	/* LOOPBACK_SUPPORT */

	/*
	 *	It's necessary to clobber the transit
	 *	count in case there is a race with an
	 *	interrupt-level reference to the port,
	 *	so that the port isn't completely torn
	 *	down now but is the subject of an
	 *	io_free/dipc_port_free sequence again,
	 *	later.
	 */
	port->ip_transit = 0;

	if (transits != 0) {
		dipc_return_t dr;

		do {
			kktr = drpc_yield_transits(node, &port->ip_uid,
						   transits, &dr, &node);
			assert(kktr == KKT_SUCCESS);
		} while (dr == DIPC_PORT_MIGRATED);
	}

	return (DIPC_SUCCESS);
}


/*
 * Routine:	dproc_yield_transits
 *
 * Server side of proxy yielding leftover transits
 *
 */

dipc_return_t
dproc_yield_transits(
	uid_t		*uidp,
	long		transits,
	node_name	*new_node)
{
	ipc_port_t	port;
	dipc_return_t	dr;
	node_name	node;
	kkt_return_t	kktr;

	dr = dipc_uid_lookup(uidp, &port);
	if (dr != DIPC_SUCCESS)
		return (DIPC_PORT_DEAD);

	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);

	ip_lock(port);
	ip_release(port);
	if (!DIPC_IS_PROXY(port) || port->ip_transit < 0) {
		port->ip_transit += transits;
		if (port->ip_transit == 0 &&
		    port->ip_srights == 0 &&
		    port->ip_nsrequest != IP_NULL) {
			ipc_port_t nsrequest = port->ip_nsrequest;
			mach_port_mscount_t mscount = port->ip_mscount;
			port->ip_nsrequest = IP_NULL;
			ip_check_unlock(port);
			ipc_notify_no_senders(nsrequest, mscount);
		} else
		    ip_check_unlock(port);
		return (DIPC_SUCCESS);
	} else {
		*new_node = port->ip_dest_node;
		assert(dipc_node_is_valid(port->ip_dest_node));
		ip_check_unlock(port);
		return (DIPC_PORT_MIGRATED);
	}
}




/*
 * Routine:	dproc_migrate_state
 *
 * Accept migration state for a migrating receive right.  This
 * routine then finishes up the migration process and awakens the
 * blocked thread which is copying out this right.
 *
 * The port remains blocked (ip_ms_block==TRUE) until the thread
 * copying out this right has a chance to restore any saved
 * messages.
 */

dipc_return_t
dproc_migrate_state(
	uid_t			*uidp,
	mach_port_seqno_t	seqno,
	mach_port_msgcount_t	qlimit,
	long			transits,
	long			remote_sorights)
{
	ipc_port_t		port;
	dipc_return_t		dr;
	TR_DECL("dproc_migrate_state");

	tr_start();
	tr3("enter: uid=0x%x,%x", uidp->origin, uidp->identifier);

	/*
	 * Get local port structure.  It must be here because we have
	 * a blocked thread with a reference to it.
	 */
	dr = dipc_uid_lookup(uidp, &port);
	assert(dr==DIPC_SUCCESS);

	/*
	 * We convert to real reference, lock the port and release it
	 */
	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);
	ip_lock(port);
	ip_release(port);

	/*
	 *	When the receive right dies, we must remember to
	 *	notify the last node with a forwarding proxy.
	 *	It, in turn, will notify the next node in the
	 *	forwarding chain, and so on.
	 */
	dr = dipc_port_dn_register(port, port->ip_source_node);
	assert(dr == DIPC_SUCCESS);

	port->ip_seqno = seqno;
	port->ip_qlimit = qlimit;
	port->ip_transit += transits;	/* Consolidate transit count */
	port->ip_remote_sorights = remote_sorights - port->ip_sorights;
	port->ip_source_node = NODE_NAME_NULL;
	port->ip_migrate_wait = FALSE;

	ip_check_unlock(port);

	/*
	 * Wake up the thread in dipc_port_copyout.  Leave the
	 * port blocked (ip_ms_block==TRUE) until it has a
	 * chance to do its thing.
	 */
	thread_wakeup((event_t)&port->ip_dipc->dip_flag_bits);
	tr1("exit");
	tr_stop();
	return (DIPC_SUCCESS);
}


#define DIPC_NUM_MIGRATION_THREADS 3
#define DIPC_MIGRATION_SEND_HANDLES 5
#define DIPC_MIGRATION_RECV_HANDLES 5
#define DIPC_MIGRATION_RPC_SIZE 12

int dipc_num_migration_threads = DIPC_NUM_MIGRATION_THREADS;
int dipc_migration_send_handles = DIPC_MIGRATION_SEND_HANDLES;
int dipc_migration_recv_handles = DIPC_MIGRATION_RECV_HANDLES;
int dipc_migration_rpc_size = DIPC_MIGRATION_RPC_SIZE;

rpc_buf_t dipc_migration_list = (rpc_buf_t)0;
decl_simple_lock_data(,dipc_migration_lock)

#define dipcm_lock() simple_lock(&dipc_migration_lock);
#define dipcm_unlock() simple_unlock(&dipc_migration_lock);

dstat_decl(unsigned int c_dml_length = 0;)
dstat_decl(unsigned int c_dml_enqueued = 0;)
dstat_decl(unsigned int c_dml_handled = 0;)
dstat_decl(unsigned int c_dml_max_length = 0;)

union drpc_brmu {
	struct {
		uid_t uid;
		node_name node;
	} in;
	dipc_return_t dr;
};

/*
 * Routine:	drpc_begin_recv_migrate
 *
 * Make remote rpc to begin receive migration
 */

kkt_return_t
drpc_begin_recv_migrate(
	node_name	node,
	uid_t		*uidp,
	node_name	mnode,
	dipc_return_t	*dr)
{
	kern_return_t	kktr, error;
	handle_t	handle;
	union drpc_brmu *buf;
	TR_DECL("drpc_begin_recv_migrate");

	tr_start();
	tr5("enter: node=0x%x uid=0x%x,%x mnode=0x%x", node,
	    uidp->origin, uidp->identifier, mnode);

	assert(node != NODE_NAME_NULL);

	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_DIPC_MIGRATION, &handle,
				    dipc_migration_rpc_size);
	if (kktr != KKT_SUCCESS) {
		tr2("KKT_RPC_HANDLE_ALLOC **FAILS** with kktr 0x%x", kktr);
		tr_stop();
		return (kktr);
	}

	kktr = KKT_RPC_HANDLE_BUF(handle, (void **)&buf);
	assert(kktr == KKT_SUCCESS);

	buf->in.uid = *uidp;
	buf->in.node = mnode;

	error = KKT_RPC(node, handle);

	*dr = buf->dr;

	kktr = KKT_RPC_HANDLE_FREE(handle);

	tr1("exit");
	tr_stop();
	return ((error == KKT_SUCCESS)?kktr:error);
}


/*
 * Routine:	dipc_migration_thread()
 *
 * This routine is run by the dipc migration thread to handle 
 * migration of receive rights.  It is kicked off by the drpc_migrate_state
 * RPC which is itself handled at interrupt level.
 *
 */

#if	MACH_ASSERT
uid_t		dipc_migration_uid;
rpc_buf_t	dipc_migration_rpc_buf;
#endif	/*MACH_ASSERT*/


/*
 *	Caller supplies the rpc_buf containg the migration
 *	request and a flag indicating whether the entire
 *	migration should be performed now.
 *
 *	First this function looks up and then disables
 *	traffic on the port.  If the caller doesn't want
 *	to do everything now, the function then returns.
 *	Otherwise, push all messages and state to the
 *	new node.
 */
void
dipc_migration_request(
	rpc_buf_t	rpc_buf,
	boolean_t	do_everything)
{
	ipc_port_t		port, ipdest;
	node_name		mnode;
	ipc_kmsg_t		kmsg;
	ipc_mqueue_t		mqueue;
	ipc_kmsg_queue_t	queue;
	mach_port_msgcount_t	nmsgs;
	ipc_thread_t		sap;
	dipc_return_t		dr;
	kkt_return_t		kktr;
	union drpc_brmu		*buf;
	boolean_t		was_already_blocked;
	TR_DECL("dipc_migration_request");

	buf = (union drpc_brmu*)rpc_buf->buff;
	mnode = buf->in.node;

	dr = dipc_uid_lookup(&buf->in.uid, &port);
	assert(dr == DIPC_SUCCESS);
	tr_start();
	tr4("migrating port 0x%x, dr %d, to node %d", port, dr, mnode);
	dr = dipc_uid_port_reference(port);
	assert(dr == DIPC_SUCCESS);

	ip_lock(port);

	assert(port->ip_references >= 2);
	assert(do_everything == TRUE || port->ip_dipc->dip_ms_block == FALSE);

	/*
	 * In the case of a transport that supplies a thread context,
	 * we have already done some of this work at RPC time.  We must
	 * check for the case that we have already disabled traffic
	 * on this port and avoid duplicating important work.
	 */
	was_already_blocked = port->ip_dipc->dip_ms_block;
	if (was_already_blocked != TRUE) {
		/*
		 * We now block further message enqueing.  The port
		 * must turn into a forwarding proxy NOW because
		 * we may try to send messages over it (below).
		 *
		 * Since the port is now blocked, we can respond to the
		 * originating RPC.
		 */

		port->ip_dipc->dip_ms_block = TRUE;
		port->ip_proxy = TRUE;
		port->ip_forward = TRUE;
		port->ip_dest_node = mnode;
		assert(dipc_node_is_valid(port->ip_dest_node));

		/*
		 *	Release reference acquired by dipc_uid_lookup/
		 *	dipc_uid_port_reference.  Another reference
		 *	will be taken out when this routine is called
		 *	again with everything==TRUE.  There should be
		 *	at least two references on the port at this point,
		 *	one acquired above and one acquired when the port
		 *	was copied in from user land.
		 */
		assert(port->ip_references >= 2);
		if (do_everything == FALSE)
			ip_release(port);
		ip_unlock(port);

		buf->dr = DIPC_SUCCESS;
		kktr = KKT_RPC_REPLY(rpc_buf->handle);
		assert(kktr == KKT_SUCCESS);
	} else {
		if (do_everything == FALSE)
			ip_release(port);
	        ip_unlock(port);
	}

	if (do_everything == FALSE) {
		tr_stop();
		return;
	}

	assert(do_everything == TRUE && port->ip_references >= 2);

	/*
	 *	Check whether the supplied buffer was dynamically
	 *	allocated by the caller and must be freed.  This case
	 *	arises for transports that supply their own threads
	 *	during RPC upcalls.  This case is easy to detect:
	 *	the port will already have been marked as blocked
	 *	during the RPC itself; therefore, the buffer we
	 *	are operating on now was kalloc'd.
	 */
	if (was_already_blocked) {
		kfree((vm_offset_t)rpc_buf, sizeof(struct rpc_buf) + 
		      sizeof(union drpc_brmu));
	}

	/*
	 * We now must forward all the messages in the
	 * principal's queue to the new principal.
	 * Messages can be in one of three formats/states.
	 * If the message is in local format, we just use
	 * the existing dipc_mqueue_send to move the message.
	 * If it is a meta_kmsg, we use drpc_send_meta_kmsg
	 * which will move the meta_kmsg info over and
	 * translate the handle at the remote end.  The third
	 * state is an inline kmsg with a handle for ports
	 * or ool data.  In this case we tell the original
	 * sender to rewind its concept of what has been sent
	 * to not include the inline kmsg, and we create and
	 * send to the new principal a meta_kmsg representing
	 * this third-case kmsg.
	 */

	mqueue = &port->ip_messages;
	imq_lock(mqueue);
	queue = &mqueue->imq_messages;
	assert(ipc_thread_queue_empty(&mqueue->imq_threads));
	nmsgs = port->ip_msgcount;
	/*
	 *	We must always take either all the messages
	 *	present on the queue or MACH_PORT_QLIMIT_MAX
	 *	messages.  If we take fewer than the maximum
	 *	permitted queue limit's worth, we allow possible
	 *	out-of-order message delivery.  This case arises
	 *	when a remote sender successfully enqueues a
	 *	message on the principal but this message remains
	 *	behind on the forwarding proxy when the principal
	 *	migrates.  On its new node, the new principal does
	 *	not discriminate between traffic from the new
	 *	forwarding proxy and the original sender.  Thus,
	 *	the thread that enqueued the original message
	 *	could unblock and generate another, and there would
	 *	be no guarantee that the second message would be
	 *	delivered after the first.
	 *
	 *	Because the queue limit can never exceed QLIMIT_MAX,
	 *	there is absolutely no way for a remote sender to
	 *	enqueue a message that will be left behind when the
	 *	principal migrates.
	 */
	if (port->ip_msgcount > MACH_PORT_QLIMIT_MAX)
		nmsgs = MACH_PORT_QLIMIT_MAX;
	assert(nmsgs >= 0);
	while(nmsgs-- > 0 &&
	      (kmsg = ipc_kmsg_dequeue(queue)) != IKM_NULL) {
		port->ip_msgcount--;
		imq_unlock(mqueue);
		assert(KMSG_IS_META(kmsg) || KMSG_IS_DIPC_FORMAT(kmsg));
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_MIGRATING;
		tr3("forwarding %skmsg 0x%x",
		    (KMSG_IS_META(kmsg) ? "meta-" : ""), kmsg);
		if (KMSG_IS_META(kmsg)) {
			assert(KMSG_HAS_HANDLE(kmsg));
			kktr = drpc_send_meta_kmsg(mnode,
						   &port->ip_uid,
						   dipc_node_self(),
						   MKM(kmsg)->mkm_size, 
						   MKM(kmsg)->mkm_handle,
						   &dr);
			assert(kktr == KKT_SUCCESS);
			assert(dr == DIPC_SUCCESS);
			KMSG_DROP_HANDLE(kmsg, kmsg->ikm_handle);
			dipc_kmsg_free(kmsg);
		} else {
			/*
			 * dipc_mqueue_send requires a locked port
			 */
			mach_msg_return_t mr;
			
			ip_lock(port);
			mr = dipc_mqueue_send(kmsg,
					      MACH_SEND_MSG, 0);
			assert(dr == MACH_MSG_SUCCESS);
		}
		imq_lock(mqueue);
	}
	imq_unlock(mqueue);

	assert(port->ip_references >= 2);

	/*
	 *	Snapshot the remaining port state and
	 *	forward it to the new principal.
	 */

	(void) drpc_migrate_state(mnode,
				  &port->ip_uid,
				  port->ip_seqno,
				  port->ip_qlimit,
				  (IP_NMS(port)?
				   port->ip_transit - port->ip_srights:0),
				  port->ip_remote_sorights+port->ip_sorights,
				  &dr);

	/*
	 *	As soon as we sent the MIGRATE_STATE RPC,
	 *	life became very racy:  the port could
	 *	immediately try to migrate back here.  However,
	 *	because ms_block is still set, the reverse
	 *	port migration must wait.
	 *
	 *	Take this opportunity to fix up some remaining
	 *	details of the forwarding proxy's state, so
	 *	that it looks like a just-allocated proxy.
	 *	Then release ms_block.
	 *
	 *	When the message was copied in, ipc_port_check_circularity
	 *	left a pointer to the destination port in ip_destination
	 *	along with an extra reference on that port.  Now that we
	 *	know that the message was received remotely, we can drop
	 *	the extra reference and null out ip_destination.  (In the
	 *	local case, the copyout logic consumes the extra reference.)
	 *
	 *	Things we don't have to fix:
	 *		ip_sorights	value remains correct
	 */
	ip_lock(port);
	assert(port->ip_proxy == TRUE);
	assert(port->ip_forward == TRUE);
	assert(port->ip_ms_block == TRUE);
	assert(port->ip_dest_node == mnode);

	port->ip_transit = port->ip_srights;
	port->ip_seqno = 0;
	port->ip_remote_sorights = 0;
	port->ip_qlimit = MACH_PORT_QLIMIT_DEFAULT;
	ipdest = port->ip_destination;
	port->ip_destination = IP_NULL;

	port->ip_dipc->dip_ms_block = FALSE;

	/*
	 *	Now wake up all the poor saps who might
	 *	have been blocked.
	 *
	 *	Blocked case 1:  threads that encountered
	 *	this port while it was migrating are blocked
	 *	because of the dip_ms_block bit.
	 */
	thread_wakeup((event_t)port);

	/*
	 *	Blocked case 2:  local threads may be blocked
	 *	because they encountered the port's queue limit.
	 *	They will probably wind up sleeping again -- and
	 *	one or more of them will attempt a SEND_CONNECT
	 *	message.  This behavior is inefficient but not
	 *	currently thought to be a problem.
	 *
	 *	This wakeup loop must happen under port lock.
	 */
	while ((sap=ipc_thread_dequeue(&port->ip_blocked))!=ITH_NULL) {
		sap->ith_state = MACH_MSG_SUCCESS;
		thread_go(sap);
	}

	assert(port->ip_proxy == TRUE);
	assert(port->ip_forward == TRUE);
	assert(port->ip_ms_convert == TRUE);
	assert(port->ip_ms_block == FALSE);

	assert(port->ip_references >= 1);

	ip_unlock(port);

	assert(ipdest != port);
	if (IP_VALID(ipdest))
		ipc_port_release(ipdest);

	/*
	 *	Blocked case 3:  remote threads that encountered
	 *	a full queue must wake up and discover that the
	 *	port has migrated.  They will wind up sleeping
	 *	again.
	 *
	 *	For this case, we must drop the port lock.  Briefly,
	 *	the port state will be a tad inconsistent:  a
	 *	forwarding proxy with remote blocked senders.  We
	 *	can live with this.
	 */
	dr = dipc_wakeup_blocked_senders(port);	/* remote threads */
	assert(dr == DIPC_SUCCESS);

	/*
	 *	Release port reference acquired when translating
	 *	uid to port.
	 */
	ipc_port_release(port);

	tr_stop();
}


void
dipc_migration_thread(void)
{
	thread_t	thread;
	rpc_buf_t		rpc_buf;
	spl_t			s;
	TR_DECL("dipc_migration_thread");

	/*
	 *	The migration thread is critical to the pageout
	 *	path; the node trying to pageout must migrate
	 *	a port.  Stack privilege is needed for a thread
	 *	required to run as part of the pageout path.
	 */
	thread = current_thread();
        stack_privilege(thread);

	/*
	 *	Ideally, this thread should only require stack
	 *	privilege.  However, the DIPC message transmission
	 *	path allocates various resources dynamically; this
	 *	allocation can block if memory is tight.  So we'll
	 *	let this thread dip into the reserved memory pool.
	 *	This behavior is NOT what we had designed for; we
	 *	are going to have to revisit the resource allocations
	 *	happening in dipc_mqueue_send.  XXX
	 */
        thread->vm_privilege = TRUE;


	for (;;) {
		s = splkkt();
		dipcm_lock();

		rpc_buf = dipc_migration_list;
		if (rpc_buf == (rpc_buf_t) 0) {
			assert_wait((event_t)dipc_migration_thread, FALSE);
			dipcm_unlock();
			splx(s);
			thread_block(0);
			continue;
		}

		assert(dipc_migration_list != (rpc_buf_t)0);
		dipc_migration_list = (rpc_buf_t)(rpc_buf->ulink);

		dcntr(c_dipc_recv_migrations++);
		dstat_max(c_dml_max_length, c_dml_length);
		dstat(--c_dml_length);
		dstat(++c_dml_handled);

		dipcm_unlock();
		splx(s);

		dipc_migration_request(rpc_buf, TRUE);
	}
}


/*
 * Routine:	dipc_migration_rpc_deliver
 *
 * Routine called upon delivery of a migration RPC
 *
 */
void
dipc_migration_rpc_deliver(
	channel_t	channel,
	rpc_buf_t	rpc_buf,
	boolean_t	thread_context)
{
	rpc_buf_t               new_rpc;
	spl_t			s;

	assert(channel == CHAN_DIPC_MIGRATION);
	if (thread_context == TRUE) {
		new_rpc = (rpc_buf_t)kalloc(sizeof(struct rpc_buf) + 
					    sizeof(union drpc_brmu));
		assert(new_rpc);
		memcpy((char *)new_rpc, (const char *)rpc_buf,
		       sizeof(struct rpc_buf) + sizeof(union drpc_brmu));
		dipc_migration_request(rpc_buf, FALSE);
		rpc_buf = new_rpc;
	}
	s = splkkt();
	dipcm_lock();
	rpc_buf->ulink = (unsigned int *) dipc_migration_list;
	dipc_migration_list = rpc_buf;
	dstat(++c_dml_length);
	dstat(++c_dml_enqueued);
	dipcm_unlock();
	splx(s);
	thread_wakeup_one((event_t)dipc_migration_thread);
}


/*
 * Routine:	dipc_migration_thread_init
 *
 * Intialize a pool of dipc migration threads
 */
void
dipc_migration_thread_init(void)
{
	int		i;
	kkt_return_t	kktr;

	simple_lock_init(&dipc_migration_lock, ETAP_DIPC_MIGRATE);

	for(i=0; i< dipc_num_migration_threads; i++)
		(void) kernel_thread(kernel_task,
				     dipc_migration_thread, (char *)0);

	kktr = KKT_RPC_INIT(CHAN_DIPC_MIGRATION, 
			    dipc_migration_send_handles,
			    dipc_migration_recv_handles,
			    dipc_migration_rpc_deliver,
			    (kkt_malloc_t)dipc_alloc,
			    (kkt_free_t)dipc_free,
			    (unsigned int) dipc_migration_rpc_size);

	assert(kktr == KKT_SUCCESS);
}


/*
 * Routine:	dproc_send_meta_kmsg
 *
 * Accept an incoming description of a meta_kmsg including a
 * needs-to-be-translated handle from the remote node
 */

dipc_return_t
dproc_send_meta_kmsg(
	uid_t		*uidp,
	node_name	rnode,
	vm_size_t	size,
	handle_t	handle)
{
	dipc_return_t		dr;
	kkt_return_t		kktr;
	handle_t		new_handle;
	meta_kmsg_t		mkm;
	ipc_port_t		port;
	mach_msg_return_t	mr;
	TR_DECL("dproc_send_meta_kmsg");

	tr_start();
	tr5("enter: uid=0x%x,%x rnode=0x%x size=0x5x", 
	    uidp->origin, uidp->identifier, rnode, size);
	kktr = dipc_send_handle_alloc(&new_handle);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_HANDLE_MIGRATE(rnode, handle, new_handle);
	assert(kktr == KKT_SUCCESS);

	mkm = dipc_meta_kmsg_alloc(TRUE);

	mkm->mkm_handle = new_handle;
	mkm->mkm_size = size;
	KMSG_MARK_RECEIVING((ipc_kmsg_t) mkm);
	KMSG_MARK_HANDLE((ipc_kmsg_t) mkm);

	/*
	 * This is currently only used during port migration so this
	 * MUST succeed
	 */

	dr = dipc_uid_lookup(uidp, &port);
	assert(dr == DIPC_SUCCESS);

	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);

	ip_lock(port);
	/* 
	 * We don't need the reference acquired during the
	 * UID lookup since a reference is already held
	 * during the entire migration process
	 */
	ip_release(port);
	mr = ipc_mqueue_deliver(port, (ipc_kmsg_t) mkm, TRUE);
	assert(mr == MACH_MSG_SUCCESS);

	tr1("exit");
	tr_stop();
	return (DIPC_SUCCESS);
}


/*
 * Routine:	dipc_port_dnrequest_init
 *
 * Setup the cross node dead_name notification.  This is called from
 * the local ipc code the first time a dead name notification is
 * registered.  Note that the port's UID doesn't change as long as
 * the port exists.
 *
 * Preconditions:
 *	Port is locked
 * Postconditions:
 *	Port is locked
 *	Port is referenced by remote request
 *
 */

dipc_return_t
dipc_port_dnrequest_init(
	ipc_port_t	port)
{
	kkt_return_t	kktr;
	dipc_return_t	dr;
	node_name	node, new_node;

	if (!DIPC_IS_DIPC_PORT(port))
	    return DIPC_SUCCESS;

	if (!DIPC_IS_DIPC_PORT(port) || !DIPC_IS_PROXY(port))
	    return DIPC_SUCCESS;

	dcntr(c_dipc_dn_inits++);
	ip_reference(port);

	node = port->ip_dest_node;
	assert(dipc_node_is_valid(port->ip_dest_node));

	ip_unlock(port);

	kktr = drpc_dn_register(node, &port->ip_uid, dipc_node_self(),
				&dr, &new_node);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	if (new_node != node && new_node != port->ip_dest_node) {
		ip_lock(port);
		if (new_node != port->ip_dest_node) {
			port->ip_dest_node = new_node;
			assert(dipc_node_is_valid(port->ip_dest_node));
		}
	} else
	    ip_lock(port);
	return (dr);
}


/*
 * Routine:	dipc_port_dnnotify
 *
 * Called by the local ipc code to notify a remote node of a dead name
 * request
 */

void
dipc_port_dnnotify(
	ipc_port_t	port,
	node_name	node)
{
	kkt_return_t	kktr;

	kktr = drpc_dn_notify(node, &port->ip_uid);
	assert (kktr == KKT_SUCCESS);
}


/*
 * Routine:	dproc_dn_register
 *
 * Register a remote node for dead name notification
 *
 */

dipc_return_t
dproc_dn_register(
	uid_t		*uidp,
	node_name	rnode,
	node_name	*new_node)
{
	dipc_return_t	dr;
	ipc_port_t	port;

	dr = dipc_uid_lookup(uidp, &port);
	if (dr != DIPC_SUCCESS)
		return (DIPC_PORT_DEAD);

	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);

	ip_lock(port);
    
	dr = dipc_port_dn_register(port, rnode);

	/*
	 *	Let the caller know where we think
	 *	the port lives.
	 */
	if (DIPC_IS_PROXY(port))
		*new_node = port->ip_dest_node;
	else
		*new_node = dipc_node_self();

	/*
	 *	Retain the extra port reference across
	 *	dipc_port_dn_register, because it might
	 *	call ipc_port_dngrow, which unlocks and
	 *	relocks the port.  (Unlocking could cause
	 *	the port to disappear, if we're racing
	 *	with some other activity and the ref count
	 *	falls to zero.)
	 */
	ip_release(port);

	ip_check_unlock(port);

	return (dr);
}


/*
 * Routine:	dipc_port_dn_register
 *
 * Register a remote node for a dead name notification
 *
 * Preconditions:
 * 	Port is locked
 * Postconditons:
 *	Port is locked
 */

dipc_return_t
dipc_port_dn_register(
	ipc_port_t	port,
	node_name	node)
{
	kern_return_t			kr;
	ipc_port_request_index_t	junk_request;

	kr = ipc_port_dnrequest(port, node, DIPC_FAKE_DNREQUEST,
				&junk_request);
	while (kr != KERN_SUCCESS) {
		kr = ipc_port_dngrow(port, ITS_SIZE_NONE);
		assert(kr == KERN_SUCCESS);
		ip_lock(port);
		kr = ipc_port_dnrequest(port, node, DIPC_FAKE_DNREQUEST,
					&junk_request);
	}

	return(DIPC_SUCCESS);
}


/*
 * Routine:	dproc_dn_notify
 *
 * This is a dead name notification from a remote principal.
 */

dipc_return_t
dproc_dn_notify(
	uid_t		*uidp)
{
	dipc_return_t	dr;
	ipc_port_t	port;
	ipc_port_request_t	dnrequests;
	TR_DECL("dproc_dn_notify");

	tr_start();

	/*
	 * The dnrequest has a reference so this MUST succeed
	 */

	dr = dipc_uid_lookup(uidp, &port);
	assert(dr==DIPC_SUCCESS);	

	dr = dipc_uid_port_reference(port);
	assert(dr==DIPC_SUCCESS);

	tr3("entry:  uid pointer 0x%x, port 0x%x", uidp, port);

	ip_lock(port);
	/*
	 * Release the reference acquired by the UID lookup.
	 * The dnrequest reference will be released after the
	 * notifications are done.
	 *
	 * Mark the port not active so no more requests and be made,
	 * and we might as well make note of the fact that the port
	 * is dead anyway.
	 */
	ip_release(port);
	
	port->ip_object.io_bits &= ~IO_BITS_ACTIVE;
	dnrequests = port->ip_dnrequests;
	port->ip_dnrequests = IPR_NULL;
	ip_unlock(port);

	/* generate the dead name notifications */
	if (dnrequests != IPR_NULL)
		ipc_port_dnnotify(port, dnrequests);
    
	/*
	 *	Release the reference owned by the dnrequest.
	 *	The port will usually be destroyed at this point,
	 *	except during a race with some other thread using
	 *	the port.
	 */
	ipc_port_release(port);

	tr_stop();
	return (DIPC_SUCCESS);
}


/*
 *	Subsystem initialization.
 */
void
dipc_bootstrap(void)
{
	kkt_return_t		kktr;
	kkt_properties_t	prop;
	extern int		dipc_num_deliver_threads;
	extern int		dipc_num_rpc_threads;
	extern boolean_t	dipc_kkt_transport_threads;

	assert(dipc_initialized++ == 0);

#if	DIPC_TIMER
	dipc_timer_init();
#endif	/* DIPC_TIMER */

	kkt_bootstrap_init();

	kktr = KKT_PROPERTIES(&prop);
	assert(kktr == KKT_SUCCESS);

	/*
	 *	Figure out how many of the various DIPC threads
	 *	to allocate based on the KKT upcall method.
	 */
	switch (prop.kkt_upcall_methods) {

	    case KKT_THREAD_UPCALLS:
		dipc_num_deliver_threads = 0;
		dipc_num_rpc_threads = 0;
		dipc_num_migration_threads = 1;
		dipc_kkt_transport_threads = TRUE;
		break;

	    case KKT_INTERRUPT_UPCALLS:
	    case KKT_MIXED_UPCALLS:
		/*
		 *	Use the compiled-in defaults.
		 */
		dipc_kkt_transport_threads = FALSE;
		break;
	}

	dipc_fast_kmsg_size = ikm_plus_overhead(prop.kkt_buffer_size) +
		MACH_MSG_TRAILER_FORMAT_0_SIZE;

	/*
	 *	Machine-specific initialization.  May override
	 *	the values set for the variables, above.
	 */
	dipc_machine_init();

	/*
	 *	Create the dipc port zone, which defaults to
	 *	expandable, !exhaustible, !sleepable, and
	 *	!collectable.  Make the zone !pageable, too.
	 */
	dipc_port_zone = zinit(sizeof(struct dipc_port),
			       dipc_port_zone_max * sizeof(struct dipc_port),
			       sizeof(struct dipc_port),
			       "dipc port extensions");
	dipc_port_name_table_init();

	norma_special_ports_initialization();

	kktr = KKT_CHANNEL_INIT(CHAN_USER_KMSG, 
				user_send_handles,
				user_recv_handles, 
				dipc_deliver,
				(kkt_malloc_t)dipc_alloc,
				(kkt_free_t)dipc_free);
	assert(kktr == KKT_SUCCESS);
	dipc_alloc_init();
	dipc_send_init();
	dipc_receive_init();
	dipc_kserver_init();
	dipc_rpc_init();
	KKT_MAP_INIT();

	dipc_migration_thread_init();
}


#if	MACH_KDB
#include <ddb/db_output.h>
#define	printf		kdbprintf
#define	BOOL(b)		((b) ? "" : "!")

void db_dipc_port_facts(void);

/*
 *	Pretty-print a port for kdb.
 */

void
dipc_port_print(
		ipc_port_t	port)
{
	extern int	indent;

	if (!DIPC_IS_DIPC_PORT(port))
		return;

	indent += 2;

	iprintf("%s", port->ip_proxy ? "proxy" : "principal");
	printf(", %sspecial", BOOL(port->ip_special));
	printf(", %sforward", BOOL(port->ip_forward));
	printf(", %smigrate_wait", BOOL(port->ip_migrate_wait));
	printf(", %sms_block", BOOL(port->ip_dipc->dip_ms_block));
	printf(", %sms_convert", BOOL(port->ip_dipc->dip_ms_convert));
	printf("\n");

	iprintf("%srs_block", BOOL(port->ip_rs_block));
	printf(", qf_count %d", port->ip_qf_count);
	printf(", %sresending", BOOL(port->ip_resending));
	printf(", %smap_alloc", BOOL(port->ip_map_alloc));
	printf(", %smap_wait", BOOL(port->ip_map_wait));
	printf("\n");


	iprintf("uid (origin=0x%x,ident=0x%x) dest/source_node=0x%x\n",
		port->ip_uid.origin,
		port->ip_uid.identifier,
		port->ip_dest_node);
	

	iprintf("transit=0x%x remote_sorights=0x%x\n",
		port->ip_transit,
		port->ip_remote_sorights);

	iprintf("hash_next=0x%x, ref_count=0x%x\n",
		port->ip_hash_next,
		port->ip_ref_count);

	iprintf("callback_map=0x%x, restart_list=0x%x\n",
		port->ip_callback_map,
		port->ip_restart_list);

	iprintf("node_wait=0x%x, node_wait_list=0x%x\n",
		port->ip_node_wait,
		port->ip_node_wait_list);

#if	DIPC_DO_STATS
	iprintf("queue_full %d max %d enqueued %d thread %d\n",
		port->ip_queue_full, port->ip_queue_max,
		port->ip_enqueued, port->ip_thread_hit);
#endif	/* DIPC_DO_STATS */

#if	MACH_ASSERT
	iprintf("dip_inited=0x%x dip_spares[0,1,2]=%x,%x,%x\n",
		port->ip_dipc->dip_inited, port->ip_dipc->dip_spares[0],
		port->ip_dipc->dip_spares[1], port->ip_dipc->dip_spares[2]);
#endif	/* MACH_ASSERT */

	indent -= 2;
}


void
db_dipc_port_facts(void)
{
	iprintf("General Port Operations:\n");
	indent += 2;
	iprintf("Port init:  calls %d rejects %d races %d\n",
		c_dipc_port_init_calls, c_dipc_port_init_rejects,
		c_dipc_port_init_races);
	iprintf("Port free:  calls %d in_use %d\n",
		c_dipc_port_free_calls, c_dipc_port_free_in_use);
	iprintf("Kmsg converts:  %d\n", c_dipc_kmsg_converts);
	iprintf("Port copyin:  recvs %d\n", c_dipc_recv_copyin);
	iprintf("Needed transits:  %d\n", c_dipc_needed_transits);
	iprintf("Port copyout:  recvs %d creates %d races %d\n",
		c_dipc_port_copyout_recvs, c_dipc_port_copyout_creates,
		c_dipc_port_copyout_races);
	indent -= 2;
}
#endif	/* MACH_KDB */


/*
 *	Return likely location to find port's receiver.
 *	XXX This routine does not belong here. XXX
 */
kern_return_t
norma_port_location_hint(
			 task_t task,
			 ipc_port_t port,
			 int *node);

kern_return_t
norma_port_location_hint(
			 task_t task,
			 ipc_port_t port,
			 int *node)
{
	ip_lock(port);
	if (!IP_VALID(port)) {
		ip_unlock(port);
		return KERN_INVALID_ARGUMENT;
	}
	if (IP_IS_REMOTE(port))
		*node = IP_PORT_NODE(port);
	else {
		*node = dipc_node_self();
	}
	ip_unlock(port);
	ipc_port_release_send(port);

	return KERN_SUCCESS;
}
