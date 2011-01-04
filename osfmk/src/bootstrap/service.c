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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 *	Program:	Service server
 *
 *	Purpose:
 *		Create ports for globally interesting services,
 *		and hand the receive rights to those ports (i.e.
 *		the ability to serve them) to whoever asks.
 *
 *	Why we need it:
 *		We need to get the service ports into the
 *		very top of the task inheritance structure,
 *		but the currently available system startup
 *		mechanism doesn't allow the actual servers
 *		to be started up from within the initial task
 *		itself.  We start up as soon as we can, and
 *		force the service ports back up the task tree,
 *		and let servers come along later to handle them.
 *
 *		In the event that a server dies, a new instantiation
 *		can grab the same service port.
 */

/*
 * We must define MACH_INIT_SLOTS and include <mach_init.h> first,
 * to make sure SERVICE_SLOT gets defined.
 */
#define MACH_INIT_SLOTS		1
#include <mach_init.h>

#include <mach.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <mach/notify_server.h>
#include <mach/mig_errors.h>
#include <servers/service_server.h>

#include "bootstrap.h"

#ifdef	DEBUG
boolean_t debug;
#endif

mach_port_t bootstrap_service_port;
mach_port_t trusted_notify_port;
mach_port_t untrusted_notify_port;

#define MAXSIZE		64	/* max size for requests & replies */

typedef struct service_record {
    mach_port_t service;
    boolean_t taken;
} service_record_t;

service_record_t *services;
unsigned int services_count;

void
service_init(void)
{
    mach_port_t *ports;
    mach_msg_type_number_t portsCnt;
    unsigned int i;
    kern_return_t kr;

    /*
     *	Create the notification ports.
     */

    kr = mach_port_allocate(mach_task_self(),
			    MACH_PORT_RIGHT_RECEIVE, &trusted_notify_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_allocate: %d\n",
	      program_name, kr);
    }

    kr = mach_port_allocate(mach_task_self(),
			    MACH_PORT_RIGHT_RECEIVE, &untrusted_notify_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_allocate: %d\n",
		program_name, kr);
    }

#ifdef	DEBUG
    if (debug) {
	BOOTSTRAP_IO_LOCK();
	printf("Trusted notify = %x, untrusted notify = %x.\n",
	       trusted_notify_port, untrusted_notify_port);
	BOOTSTRAP_IO_UNLOCK();
    }
#endif

    /*
     *	See what ports we have.  We are interested
     *	in the count, not the ports themselves.
     *	However, we save the memory for use below.
     */

    kr = mach_ports_lookup(mach_task_self(), &ports, &portsCnt);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_ports_lookup: %d\n",
		program_name, kr);
    }

    services_count = portsCnt;

#ifdef	DEBUG
    if (debug) {
	BOOTSTRAP_IO_LOCK();
	printf("Got %u ports from lookup.\n", services_count);
	BOOTSTRAP_IO_UNLOCK();
    }
#endif

    if (services_count <= SERVICE_SLOT) {
	panic("%s: no room for SERVICE_SLOT???", program_name);
    }

    /* deallocate any send rights acquired from mach_ports_lookup */

    for (i = 0; i < services_count; i++)
	if (MACH_PORT_VALID(ports[i])) {
	    kr = mach_port_deallocate(mach_task_self(), ports[i]);
	    if (kr != KERN_SUCCESS) {
		panic("%s: mach_port_deallocate: %d\n",
			program_name, kr);
	    }
	}

    /*
     *	Create ports for the various services.
     *	We will have receive rights for these ports.
     */

    services = (service_record_t *) malloc(services_count * sizeof *services);
    if (services == NULL) {
	panic("%s: can't allocate services array", program_name);
    }

    for (i = 0; i < services_count; i++) {
	mach_port_t service;
	mach_port_t previous;

	/* create the service port */

	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_RECEIVE, &service);
	if (kr != KERN_SUCCESS) {
	    panic("%s: can't allocate service port %d: %d\n",
		    program_name, i, kr);
	}

	/* acquire a send right for the service port */

	kr = mach_port_insert_right(mach_task_self(), service,
				    service, MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
	    panic("%s: mach_port_insert_right: %d\n",
		    program_name, kr);
	}

#ifdef	DEBUG
	if (debug) {
	    BOOTSTRAP_IO_LOCK();
	    printf("Slot %d: service = %x.\n", i, service);
	    BOOTSTRAP_IO_UNLOCK();
	}
#endif

	/* request a port-destroyed notification */

	kr = mach_port_request_notification(mach_task_self(), service,
			MACH_NOTIFY_PORT_DESTROYED, 0,
			untrusted_notify_port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
			&previous);
	if ((kr != KERN_SUCCESS) || (previous != MACH_PORT_NULL)) {
	    panic("%s: mach_port_request_notification: %d\n",
		    program_name, kr);
	}

	/* request a dead-name notification */

	kr = mach_port_request_notification(mach_task_self(), service,
			MACH_NOTIFY_DEAD_NAME, 0,
			trusted_notify_port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
			&previous);
	if ((kr != KERN_SUCCESS) || (previous != MACH_PORT_NULL)) {
	    panic("%s: mach_port_request_notification: %d\n",
		    program_name, kr);
	}

	ports[i] = service;
	services[i].service = service;
	services[i].taken = FALSE;
    }

    /*
     *	Remember our service port, and mark it as taken.
     */

    bootstrap_service_port = services[SERVICE_SLOT].service;
    services[SERVICE_SLOT].taken = TRUE;

    /*
     *	Check the ports into our task.
     */

    kr = mach_ports_register(mach_task_self(), ports, services_count);
    if (kr != KERN_SUCCESS) {
	panic("%s: can't register ports", program_name);
    }

    kr = vm_deallocate(mach_task_self(), (vm_address_t) ports,
		       (vm_size_t) (services_count*sizeof(mach_port_t)));
    if (kr != KERN_SUCCESS) {
	panic("%s: vm_deallocate: %d\n",
		program_name, kr);
    }

    /*
     *	Put service ports into the bootstrap port set.
     */

    kr = mach_port_move_member(mach_task_self(),
			       trusted_notify_port, bootstrap_set);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_move_member: %d\n",
		program_name, kr);
    }

    kr = mach_port_move_member(mach_task_self(),
			       untrusted_notify_port, bootstrap_set);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_move_member: %d\n",
		program_name, kr);
    }

    for (i = 0; i < services_count; i++) {
	kr = mach_port_move_member(mach_task_self(),
				   services[i].service, bootstrap_set);
	if (kr != KERN_SUCCESS) {
	    panic("%s: mach_port_move_member: %d\n",
		    program_name, kr);
	}
    }
}

boolean_t
service_demux(mach_msg_header_t *request, mach_msg_header_t *reply)
{
#ifdef	DEBUG
    if (debug) {
	BOOTSTRAP_IO_LOCK();
	printf("Received request, port %x id %d\n",
	       request->msgh_local_port, request->msgh_id);
	BOOTSTRAP_IO_UNLOCK();
    }
#endif

    if (request->msgh_local_port == bootstrap_service_port)
	return service_server(request, reply);
    else if ((request->msgh_local_port == trusted_notify_port) ||
	     (request->msgh_local_port == untrusted_notify_port) ||
	     (request->msgh_local_port == bootstrap_notification_port))
	return notify_server(request, reply);
    else {
	/*
	 *	This must be a request directed to a port, probably
	 *	the name server port, which hasn't been taken yet.
	 *	We destroy the request without replying.
	 */

	mach_msg_destroy(request);
	((mig_reply_error_t *) reply)->RetCode = MIG_NO_REPLY;

	return TRUE;
    }
}

kern_return_t
do_mach_notify_port_deleted(mach_port_t notify, mach_port_t name)
{
    if (notify != untrusted_notify_port) {
	panic("%s: do_mach_notify_port_deleted\n", program_name);
    }

    return KERN_FAILURE;
}

kern_return_t
do_mach_notify_port_destroyed(mach_port_t notify,
			      mach_port_t port)
{
    mach_port_t previous;
    unsigned int i;
    kern_return_t kr;

    if (notify != untrusted_notify_port) {
	panic("%s: do_mach_notify_port_destroyed\n", program_name);
    }

    /*
     *	We must be cautious in processing this notification.
     *	However, because the message was type-checked
     *	we must have receive rights for the port.  So if it
     *  matches one of our service ports, we can be confident.
     */

    if (!MACH_PORT_VALID(port))
	return KERN_FAILURE;

    for (i = 0; i < services_count; i++)
	if (services[i].service == port) {
#ifdef	DEBUG
	    if (debug) {
		BOOTSTRAP_IO_LOCK();
		printf("Port %x (slot %d) is returned.\n", port, i);
		BOOTSTRAP_IO_UNLOCK();
	    }
#endif

	    if (!services[i].taken) {
		panic("%s: port %x not taken!\n",
			program_name, port);
	    }
	    services[i].taken = FALSE;

	    /* start receiving from the port again */

	    kr = mach_port_move_member(mach_task_self(), port,
				       bootstrap_set);
	    if (kr != KERN_SUCCESS) {
		panic("%s: mach_port_move_member: %d\n",
			program_name, kr);
	    }

	    /* re-request a port-destroyed notification */

	    kr = mach_port_request_notification(mach_task_self(), port,
			MACH_NOTIFY_PORT_DESTROYED, 0,
			untrusted_notify_port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
			&previous);
	    if (kr != KERN_SUCCESS) {
		panic("%s: mach_port_request_notification: %d\n",
			program_name, kr);
	    }

	    if (MACH_PORT_VALID(previous)) {
		kr = mach_port_deallocate(mach_task_self(), previous);
		if (kr != KERN_SUCCESS) {
		    panic("%s: mach_port_deallocate: %d\n",
			    program_name, kr);
		}
	    }

	    /* we assume responsibility for the receive right */

	    return KERN_SUCCESS;
	}

    return KERN_FAILURE;
}

kern_return_t
do_mach_notify_no_senders(mach_port_t notify, mach_port_mscount_t mscount)
{
    if (notify != untrusted_notify_port) {
	panic("%s: do_mach_notify_no_senders\n", program_name);
    }

    return KERN_FAILURE;
}

kern_return_t
do_mach_notify_send_once(mach_port_t notify)
{
    if (notify != untrusted_notify_port) {
	panic("%s: do_mach_notify_send_once\n", program_name);
    }

    return KERN_SUCCESS;
}

kern_return_t
do_mach_notify_dead_name(mach_port_t notify, mach_port_t name)
{
    unsigned int i;
    kern_return_t kr;

    if (notify == bootstrap_notification_port)
	return bootstrap_notify_dead_name(name);

    if (notify != trusted_notify_port)
	return KERN_FAILURE;

    /*
     *	We can trust this notification.
     */

    for (i = 0; i < services_count; i++)
	if (services[i].service == name)
	    break;

#ifdef	DEBUG
    if (debug) {
	BOOTSTRAP_IO_LOCK();
	printf("Port %x (slot %d) died.\n", name, i);
	BOOTSTRAP_IO_UNLOCK();
    }
#endif

    /* clean up the dead name */

    kr = mach_port_mod_refs(mach_task_self(), name,
			    MACH_PORT_RIGHT_DEAD_NAME, -2);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_mod_refs: %d\n",
		program_name, kr);
    }

    services[i].service = MACH_PORT_DEAD;
    services[i].taken = FALSE;

    return KERN_SUCCESS;
}

/*
 *	Routine:	service_checkin
 *	Function:
 *		This is the routine the MiG interface calls...
 *		it merely returns the requested port (as granted).
 */
kern_return_t
do_service_checkin(mach_port_t request,
		   mach_port_t requested,
		   mach_port_t *granted)
{
    unsigned int i;
    kern_return_t kr;

    if (!MACH_PORT_VALID(requested))
	return KERN_FAILURE;

    for (i = 0; i < services_count; i++)
	if ((services[i].service == requested) && !services[i].taken) {
#ifdef	DEBUG
	    if (debug) {
		BOOTSTRAP_IO_LOCK();
		printf("Port %x (slot %d) is taken.\n", requested, i);
		BOOTSTRAP_IO_UNLOCK();
	    }
#endif

	    services[i].taken = TRUE;

	    /* deallocate the send right we acquired in the request */

	    kr = mach_port_deallocate(mach_task_self(), requested);
	    if (kr != KERN_SUCCESS) {
		panic("%s: mach_port_deallocate: %d\n",
			program_name, kr);
	    }

	    *granted = requested;
	    return KERN_SUCCESS;
	}

#ifdef	DEBUG
    if (debug) {
	BOOTSTRAP_IO_LOCK();
	printf("Request for port %x refused.\n", requested);
	BOOTSTRAP_IO_UNLOCK();
    }
#endif

    /* mach_msg_server will deallocate requested for us */
    return KERN_FAILURE;
}
