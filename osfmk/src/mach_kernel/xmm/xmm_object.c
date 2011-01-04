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
/* CMU_HIST */
/*
 * 
 * Revision 2.1.2.4  92/06/24  18:02:59  jeffreyh
 * 	Avoid creating svm and split layers for internal objects.
 * 	Also don't use port xmm_object's for internal objects.
 * 	[92/06/08            dlb]
 * 
 * Revision 2.1.2.3  92/05/28  18:22:09  jeffreyh
 * 	Add new type argument to remote_host_priv call
 * 
 * Revision 2.1.2.2  92/04/08  15:46:44  jeffreyh
 * 	Just access ip_norma_dest_node field of memory object instead
 * 	of calling norma_port_location_hint (latter now consumes a send right).
 * 	[92/04/03            dlb]
 * 
 * Revision 2.1.2.1  92/02/21  11:27:52  jsb
 * 	Use xmm_object_destroy in xmm_object_notify to release mobj and
 * 	deallocate xmm object port. Added hack to get rid of send-once
 * 	right to xmm object port before we deallocate the port, since
 * 	ipc_port_release_sonce won't do so after we deallocate it.
 * 	[92/02/20  14:00:36  jsb]
 * 
 * 	Fixed reference counting on xmm objs. A reference is now held by
 * 	xmm object, which is copied along with send right to xmm object.
 * 	[92/02/18  17:15:33  jsb]
 * 
 * 	Lots of changes. First reasonably working version.
 * 	[92/02/16  15:58:03  jsb]
 * 
 * 	Added missing line to xmm_memory_manager_export.
 * 	[92/02/12  05:58:07  jsb]
 * 
 * 	Added xmm_object_allocate routine to replace replicated xmm object
 * 	creation and initialization logic.
 * 	Added xmm_object_by_memory_object_release which disassociates
 * 	xmm object from memory object, possibly resulting in the deallocation
 * 	of mobj associated with xmm object (via no-senders).
 * 	Moved all responsibility for tearing down stack in case of xmm
 * 	object creation race to no-senders notification handler.
 * 	[92/02/11  18:38:36  jsb]
 * 
 * 	Updated explanatory text. Fixed send right management. Added
 * 	xmm_memory_manager_export routine for xmm-internal memory managers.
 * 	[92/02/11  11:24:42  jsb]
 * 
 * 	Added xmm_object_notify.
 * 	[92/02/10  17:27:44  jsb]
 * 
 * 	First checkin.
 * 	[92/02/10  17:05:02  jsb]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
 */
/*
 *	File:	norma/xmm_object.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Routines to manage xmm object to memory object association.
 */

#include <dipc.h>

#include <xmm/xmm_internal_server.h>
#include <xmm/xmm_internal.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <vm/memory_object.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <kern/host.h>
#include <kern/ipc_kobject.h>
#include <kern/misc_protos.h>
#include <xmm/xmm_obj.h>
#include <xmm/svm_temporary.h>
#if	DIPC
#include <dipc/dipc_port.h>
#include <dipc/special_ports.h>
#endif	/* DIPC */

/*
 * Forward.
 */
void		xmm_object_set(
			ipc_port_t	memory_object,
			ipc_port_t	xmm_object,
			boolean_t	make_copy);

ipc_port_t	xmm_object_copy(
			ipc_port_t	memory_object);

void		xmm_object_wait(
			ipc_port_t	memory_object);

void		xmm_object_release_local(
			ipc_port_t	memory_object);

void		xmm_object_terminate_local(
			ipc_port_t	memory_object);

ipc_port_t	xmm_object_allocate(
			xmm_obj_t	mobj);

void		xmm_object_destroy(
			ipc_port_t	xmm_object,
			xmm_obj_t	mobj);

void		xmm_object_nuke(
			ipc_port_t	xmm_object);

ipc_port_t	xmm_object_by_memory_object_remote(
			ipc_port_t	memory_object);

void		xmm_object_release_remote(
			ipc_port_t	memory_object);

void		xmm_object_terminate_remote(
			ipc_port_t	memory_object);

/*
 * The structure here is:
 *
 *	memory object		-> xmm object		[ send right ]
 *	xmm object		-> top of mobj stack	[ xmm obj ref ]
 *	bottom of mobj stack	-> memory object	[ send right ]
 *
 * The xmm object and mobj stack are colocated. They are originally created
 * on the same node as the memory object, so that we can atomically set
 * memory_object->ip_norma_xmm_object on principal port for memory object,
 * which is the central synchronizing point for creating and finding an
 * mobj stack for a memory object.
 * 
 * After the stack is created, the memory object may migrated away from
 * the stack. The port migration mechanism is responsible for maintaining
 * the association between the memory object port and the xmm object port.
 * (In the current implementation, this means moving the send right to
 * xmm object and setting it as ip_norma_xmm_object in the new principal
 * for memory object.)
 *
 * This doesn't seem right... of course the real basic problem is
 * we are designing around a couple unclean things:
 *
 *	1. pointers from ports to objects
 *	2. transparent port interposition
 *
 * So I guess it's natural that if an object moves, and we've associated
 * data with that object, and that object doesn't know about it, then
 * we have to migrate that data ourselves. I guess a netmsgserver could
 * export a kobject (port data) interface, in which case it would know
 * to migrate the data when the port was migrated.
 *
 * Right now the policy is to create the layer at the current home of the
 * memory object. We could create it at the home of the first mapper.
 * This might make sense if we expect to often not need to talk to the
 * pager from the svm layer, for example in shadowing cases.
 * We might even want to migrate the layer. There's a lot of flexibility
 * here now that memory object has a port pointing to the svm layer.
 *
 * We could get rid of ip_norma_xmm_object and replace it with a hash table
 * (and a set of routines to manipulate it). The advantage would be the
 * space savings of eliminating the field, which for most ports will be
 * unused. Note that port migration must still migrate xmm object association.
 */

/*
 * EXECUTIVE SUMMARY:
 * 
 * In the steady state, there should only be one xmm object send right,
 * held on the node which holds the receive right to the memory object.
 * 
 * FULL-LENGTH, UNCUT VERSION:
 * 
 * The xmm_object module's purpose in life is to support the primitive
 * xmm_object_by_memory_object. There are three interesting nodes that
 * can be involved when this call is invoked:
 * 
 * 1. The calling node (can be anywhere)
 * 2. The svm node (the node holding the svm stack for memory object)
 * 3. The pager node (the node holding the receive right for memory object,
 *    and which does internode reference counting on the svm stack;
 *    different from svm node if the pager port migrates after the svm
 *    stack is created)
 * 
 * The original xmm_object_by_memory_object call is forwarded via the
 * rpc forms as necessary to the pager node: eventually,
 * xmm_object_by_memory_object will be called on the pager node.
 * If we arrive at the pager node and there is no svm stack
 * (equivalently: the memory object port has ip_norma_xmm_object null), then we
 * create a stack and check to see whether we are still the pager node;
 * if we are not, then we destroy the stack and either forward to the new
 * pager node or find the winning local stack (xmm object port).
 * 
 * Eventually, xmm_object_by_memory_object will be called on the pager
 * node and it will have found or created an xmm object port. At this
 * point, ignoring other xmm_object_by_memory_object calls in progress,
 * there should be one send right for the xmm object port, attached to
 * the memory object, plus one send right created by either
 * xmm_object_copy or xmm_object_set. It is this second send right which
 * is returned by xmm_object_by_memory_object all the way back to the
 * original caller. The rpc form uses move_send so that we don't have to
 * perform any reference counting dependent on whether the caller of
 * xmm_object_by_memory_object was local or remote.
 * 
 * So finally the caller of xmm_object_by_memory_object will hold a send
 * right to the xmm object. This send right, as discussed in the comment
 * for xmm_object_by_memory_object, is to be consumed by the caller,
 * either by passing it in proxy_init in the remote case (i.e. calling
 * node is not svm node), or by deallocation in the local case. This code
 * lives in xmm_memory_object_init. And thus we return to steady state,
 * where the xmm_object has exactly one send right, held by the memory
 * object whose ip_norma_xmm_object field points to it.
 * 
 * The one steady state send right to xmm object held by the pager node
 * is released when xmm_object_terminate_local is called, which in turn
 * is called only when the xmm object reference count
 * (ip_norma_xmm_object_refs) drops to zero on the memory object node.
 * Decrements to this reference count will be sent from the svm node via
 * xmm_object_release, and increments will be send from all over via
 * xmm_object_by_memory_object. So on the decrement that actually kills
 * the xmm object, we will have m_ksvm_terminate on the svm node calling
 * xmm_object_release which will end up on the pager node, then we will
 * do the decrement there, see that the ref count is now zero, and send
 * an xmm_object_terminate which will end up back on the svm node.
 */

void
xmm_object_set(
	ipc_port_t	memory_object,
	ipc_port_t	xmm_object,
	boolean_t	make_copy)
{
	assert(! IP_IS_REMOTE(xmm_object));
	assert(ip_kotype(xmm_object) == IKOT_XMM_OBJECT);
	assert(memory_object->ip_norma_xmm_object == IP_NULL);
	memory_object->ip_norma_xmm_object = ipc_port_make_send(xmm_object);
	if (make_copy) {
		memory_object->ip_norma_xmm_object_refs = 1;
		ipc_port_copy_send(xmm_object);
	} else {
		memory_object->ip_norma_xmm_object_refs = 0;
	}
}

ipc_port_t
xmm_object_copy(
	ipc_port_t	memory_object)
{
	register ipc_port_t xmm_object;

	assert(! IP_WAS_REMOTE(memory_object));
	xmm_object = memory_object->ip_norma_xmm_object;
	if (xmm_object == IP_NULL) {
		return IP_NULL;
	}
	assert(!((int)xmm_object & 1));
	assert(!IP_IS_REMOTE(xmm_object));
	assert(ip_kotype(xmm_object) == IKOT_XMM_OBJECT);
	assert(memory_object->ip_norma_xmm_object_refs > 0);

	memory_object->ip_norma_xmm_object_refs++;
	return ipc_port_copy_send(xmm_object);
}

/*
 * If xmm_object is being terminated, wait for it.
 */
void
xmm_object_wait(
	ipc_port_t	memory_object)
{
	while (! IP_WAS_REMOTE(memory_object) &&
	       ((int)memory_object->ip_norma_xmm_object | 1) ==
	       ((int)memory_object | 1)) {
		/*
		 * Use the low bit to flag someone is waiting
		 */
		memory_object->ip_norma_xmm_object = (ipc_port_t)
			((int)memory_object->ip_norma_xmm_object | 1);

		assert_wait((event_t) &memory_object->ip_norma_xmm_object, FALSE);
		ip_unlock(memory_object);
		thread_block((void (*)(void)) 0);
		ip_lock(memory_object);
	}
}

void
xmm_object_nuke(
	ipc_port_t	xmm_object)
{
	xmm_obj_t mobj;

	/*
	 * Get and disassociate mobj from xmm object port.
	 */
	assert(!IP_IS_REMOTE(xmm_object));
	assert(ip_kotype(xmm_object) == IKOT_XMM_OBJECT);
	mobj = (xmm_obj_t) xmm_object->ip_kobject;
	ipc_kobject_set(xmm_object, IKO_NULL, IKOT_NONE);
	/*
	 * Destroy xmm object port and mobj.
	 */
	xmm_object_destroy(xmm_object, mobj);
}

/*
 * Release a reference to memory object's xmm object.
 * If there are references left, then return.
 * Otherwise:
 * Disable current xmm stack associated with memory object.
 * Nuke the xmm_object, and place memory object in a transitional
 * state (by setting memory_object->ip_norma_xmm_object to memory_object).
 * This transitional state is exited by a call to xmm_object_terminate,
 * which is called when the stack has terminated the object.
 * Attempts to create a new stack must wait for this state to be exited.
 */
void
xmm_object_release_local(
	ipc_port_t	memory_object)
{
	ipc_port_t xmm_object;

	assert(! IP_WAS_REMOTE(memory_object));
	assert(memory_object->ip_norma_xmm_object_refs > 0);
	if (--memory_object->ip_norma_xmm_object_refs == 0) {
		xmm_object = memory_object->ip_norma_xmm_object;
		assert(xmm_object != memory_object);
		memory_object->ip_norma_xmm_object = memory_object;
		ip_unlock(memory_object);
		assert(!IP_IS_REMOTE(xmm_object));
		xmm_object_nuke(xmm_object);
	} else
		ip_unlock(memory_object);
}

void
xmm_object_terminate_local(
	ipc_port_t	memory_object)
{
	boolean_t wanted;

	wanted = ((int)memory_object->ip_norma_xmm_object & 1) &&
		 (((int)memory_object->ip_norma_xmm_object & ~1) == 
							 (int)memory_object);
	memory_object->ip_norma_xmm_object = IP_NULL;
	assert(memory_object->ip_srights > 0);
	ip_unlock(memory_object);
	ipc_port_release_send(memory_object);
	if (wanted)
		thread_wakeup((event_t) &memory_object->ip_norma_xmm_object);
}

/*
 * Only called internally. Allocate an xmm_object port.
 * The xmm_object takes the mobj reference.
 */
ipc_port_t
xmm_object_allocate(
	xmm_obj_t	mobj)
{
	ipc_port_t xmm_object;

	/*
	 * Create an xmm object port.
	 */
	xmm_object = ipc_port_alloc_kernel();
	if (xmm_object == IP_NULL) {
		return IP_NULL;
	}
	IP_CLEAR_NMS(xmm_object);

	/*
	 * Associate the xmm obj with the xmm object port.
	 * We keep the xmm obj reference returned by xmm_svm_create.
	 */
	ipc_kobject_set(xmm_object, (ipc_kobject_t) mobj, IKOT_XMM_OBJECT);

	/*
	 * Return the port.
	 */
	return xmm_object;
}

/*
 * Called when we lose a race to associate a newly created xmm object
 * with a memory object. Also called by xmm_object_nuke.
 */
void
xmm_object_destroy(
	ipc_port_t	xmm_object,
	xmm_obj_t	mobj)
{
	/*
	 * Destroy xmm object port.
	 */
	ipc_port_release_send(xmm_object);
	ipc_port_dealloc_kernel(xmm_object);

	/*
	 * Lose reference to mobj, and explicitly destroy it.
	 */
	xmm_obj_lock(mobj);
	xmm_obj_release_quick(mobj);
	xmm_svm_destroy(mobj);	/* consumes lock */
}

/*
 * Called with memory_object locked. Unlocks memory_object.
 */
ipc_port_t
xmm_object_by_memory_object_remote(
	ipc_port_t	memory_object)
{
	kern_return_t kr;
	ipc_port_t xmm_object;
#if	DIPC
	node_name	node;
#endif	/* DIPC */

	assert(IP_WAS_REMOTE(memory_object));
#if	DIPC
	node = DIPC_ORIGIN_NODE(memory_object);
	ip_unlock(memory_object);
	kr = r_xmm_remote_object_by_memory_object(dipc_host_priv_port(node),
						  memory_object,
						  &xmm_object);
#else	/* DIPC */
	panic("xmm_object_by_memory_object_remote:  no transport");
#endif	/* DIPC */
	assert(kr == KERN_SUCCESS);
	return xmm_object;
}

/*
 * XXX who cares about send rights anymore XXX
 * Return send right for xmm object corresponding to memory object.
 * This is to be consumed when using xmm object to set up init,
 * either via move_send dest in proxy_init, or explicit deallocation
 * in local case.
 * Also returns one xmm_object ref, to be given to svm layer and
 * released there upon termination via xmm_object_release.
 * Also returns one xmm obj ref, to be consumed by xmm_obj_allocate
 * in either _proxy_init or xmm_memory_object_init.
 *
 * Create xmm object if necessary.
 * Memory object holds a send right to xmm object as well, which is released
 * when xmm object refs drop to 0. No-senders then triggers XXXXXX
 * svm deallocation.
 */
ipc_port_t
xmm_object_by_memory_object(
	ipc_port_t	memory_object)
{
	ipc_port_t xmm_object, old_xmm_object;
	xmm_obj_t mobj;

	/*
	 * We always create the svm stack at the current location of the
	 * memory object. We may have to chase it down if it's migrating.
	 *
	 * The memory_object principal node is the one true source
	 * of knowledge about whether an svm stack exists.
	 */
	ip_lock(memory_object);
	xmm_object_wait(memory_object);
	if (IP_WAS_REMOTE(memory_object)) {
	        /* the following call inherits the lock */
		return xmm_object_by_memory_object_remote(memory_object);
	}

	/*
	 * If there is already an xmm_object associated with this
	 * memory_object, return it, after taking a send-right reference
	 * which will be given (moved, if necessary) to the caller.
	 */
	xmm_object = xmm_object_copy(memory_object);
	if (xmm_object != IP_NULL) {
		ip_unlock(memory_object);
		assert(!IP_IS_REMOTE(xmm_object));
		return xmm_object;
	}

	/*
	 * Check kobject type, to foil attempts to map in inappropriate
	 * kernel objects (like task ports).
	 */
	if (ip_kotype(memory_object) != IKOT_NONE &&
	    ip_kotype(memory_object) != IKOT_PAGER) {
		ip_unlock(memory_object);
		return IP_NULL;
	}

	/*
	 * No xmm object is currently associcated with memory object.
	 * Unlock memory object port, and create an xmm obj stack.
	 * and a corresponding xmm obj stack.
	 *
	 * XXX
	 * Should deallocate things if this call fails part-way.
	 */
	ip_unlock(memory_object);
	xmm_user_create(memory_object, &mobj);
	xmm_svm_create(mobj, memory_object, &mobj);

	/*
	 * Create an xmm object and associate it with stack.
	 * It will have one send right.
	 */
	xmm_object = xmm_object_allocate(mobj);
	if (xmm_object == IP_NULL) {
		panic("xmm_mo_create: xmm_object_allocate");
		return IP_NULL;
	}

	ip_lock(memory_object);
	assert(!IP_WAS_REMOTE(memory_object));

	/*
	 * If we lost the race to create the stack, discard ours
	 * and use the one already created. Otherwise, associate
	 * our xmm object and stack with the memory object,
	 * by giving the memory object the send right to the xmm object.
	 */
	old_xmm_object = xmm_object_copy(memory_object);
	if (old_xmm_object != IP_NULL) {
		ip_unlock(memory_object);
		xmm_object_destroy(xmm_object, mobj);
		xmm_object = old_xmm_object;
	} else {
		xmm_object_set(memory_object, xmm_object, TRUE);
		ip_unlock(memory_object);
	}

	/*
	 * Return the xmm object send right.
	 */
	return xmm_object;
}

/*
 * Remote, protected cover routine for xmm_object_by_memory_object.
 * Requires host_priv.
 */
kern_return_t
xmm_remote_object_by_memory_object(
	host_t		host,
	ipc_port_t	memory_object,
	ipc_port_t	*xmm_object)
{
	/*
	 * Check host port validity.
	 */
	if (host == HOST_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	/*
	 * Obtain xmm_object, perhaps recursively.
	 */
	*xmm_object = xmm_object_by_memory_object(memory_object);

	/*
	 * Discard send right to memory_object given to us by our caller.
	 */
	ipc_port_release_send(memory_object);
	return KERN_SUCCESS;
}

/*
 * Called with memory_object locked. Unlocks memory_object.
 */
void
xmm_object_release_remote(
	ipc_port_t	memory_object)
{
#if	DIPC
	node_name	node;
#endif	/* DIPC */

	assert(IP_WAS_REMOTE(memory_object));
#if	DIPC
	node = DIPC_ORIGIN_NODE(memory_object);
	ip_unlock(memory_object);
	r_xmm_remote_object_release(dipc_host_priv_port(node),
				    memory_object);
#else	/* DIPC */
	panic("xmm_object_release_remote:  no transport");
#endif	/* DIPC */
}

/*
 * Called with memory_object locked. Unlocks memory_object.
 */
void
xmm_object_terminate_remote(
	ipc_port_t	memory_object)
{
#if	DIPC
	node_name	node;
#endif	/* DIPC */

	assert(IP_WAS_REMOTE(memory_object));
#if	DIPC
	node = DIPC_ORIGIN_NODE(memory_object);
	ip_unlock(memory_object);
	r_xmm_remote_object_terminate(dipc_host_priv_port(node),
				      memory_object);
#else	/* DIPC */
	panic("xmm_object_terminate_remote:  no transport");
#endif	/* DIPC */
}

/*
 * If there are no real references to xmm object, then break its
 * association with memory object.
 */
void
xmm_object_release(
	ipc_port_t	memory_object)
{
	register ipc_port_t xmm_object;

	ip_lock(memory_object);

	/*
	 * Use local or remote form as appropriate.  Call inherits
	 * the lock.
	 */
	if (IP_WAS_REMOTE(memory_object)) {
		xmm_object_release_remote(memory_object);
	} else {
		xmm_object_release_local(memory_object);
	}
}

kern_return_t
xmm_object_reference(
	ipc_port_t	memory_object)
{
	ipc_port_t	xmm_object;
#if	DIPC
	node_name	node;
#endif	/* DIPC */

	ip_lock(memory_object);

	/* We shouldn't ever be doing remote xmm_object refs */
	assert(!IP_WAS_REMOTE(memory_object));
	if (IP_WAS_REMOTE(memory_object)) {
#if	DIPC
		node = DIPC_ORIGIN_NODE(memory_object);
		ip_unlock(memory_object);
		return r_xmm_remote_object_reference(dipc_host_priv_port(node),
					      memory_object);
#else	/* DIPC */
		panic("xmm_object_reference:  no transport");
#endif	/* DIPC */
	}

	/*
	 * return error if XMM object is terminating or terminated.
	 */
	if (((int)memory_object->ip_norma_xmm_object | 1) ==
	    ((int)memory_object | 1) ||
	    (xmm_object = memory_object->ip_norma_xmm_object) == IP_NULL) {
		ip_unlock(memory_object);
		assert(!IP_IS_REMOTE(xmm_object));
		return KERN_ABORTED;
	}

	assert(!IP_IS_REMOTE(xmm_object));
	assert(ip_kotype(xmm_object) == IKOT_XMM_OBJECT);
	assert((memory_object->ip_norma_xmm_object_refs > 0) ||
		(ip_kotype(memory_object) == IKOT_NONE));
	memory_object->ip_norma_xmm_object_refs++;
	ip_unlock(memory_object);
	return KERN_SUCCESS;
}

void
xmm_object_terminate(
	ipc_port_t	memory_object)
{
	ip_lock(memory_object);

	/*
	 * Use local or remote form as appropriate.  Call inherits
	 * the lock.
	 */
	if (IP_WAS_REMOTE(memory_object)) {
		xmm_object_terminate_remote(memory_object);
	} else {
		xmm_object_terminate_local(memory_object);
	}
}

/*
 * Remote, protected cover routine for xmm_object_release.
 * Requires host_priv.
 */
kern_return_t
xmm_remote_object_release(
	host_t		host,
	ipc_port_t	memory_object)
{
	/*
	 * Check host port validity.
	 */
	if (host == HOST_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	assert(!IP_WAS_REMOTE(memory_object));

	/*
	 * Release xmm object.
	 */
	xmm_object_release_local(memory_object);

	/*
	 * Discard send right to memory_object given to us by our caller.
	 */
	ipc_port_release_send(memory_object);
	return KERN_SUCCESS;
}

/*
 * Remote, protected cover routine for xmm_object_reference.
 * Requires host_priv.
 */
kern_return_t
xmm_remote_object_reference(
	host_t		host,
	ipc_port_t	memory_object)
{
	kern_return_t kr;

	/*
	 * Check host port validity.
	 */
	if (host == HOST_NULL)
		return KERN_INVALID_ARGUMENT;

	assert(!IP_WAS_REMOTE(memory_object));

	/*
	 * Reference xmm object.
	 */
	kr = xmm_object_reference(memory_object);
	assert(kr == KERN_SUCCESS);

	/*
	 * Discard send right to memory_object given to us by our caller.
	 */
	ipc_port_release_send(memory_object);
	return kr;
}

/*
 * Remote, protected cover routine for xmm_object_terminate.
 * Requires host_priv.
 */
kern_return_t
xmm_remote_object_terminate(
	host_t		host,
	ipc_port_t	memory_object)
{
	/*
	 * Check host port validity.
	 */
	if (host == HOST_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	assert(!IP_WAS_REMOTE(memory_object));

	/*
	 * When this occurs, we may be leaking the
	 * receive right.
	 */
	assert(memory_object != IP_DEAD);  /* this shouldn't happen */
	if (memory_object == IP_DEAD) {
	    return KERN_SUCCESS;
	}

	/*
	 * Terminate xmm object.
	 */
	xmm_object_terminate_local(memory_object);

	return KERN_SUCCESS;
}

/*
 * Create an xmm object for an xmm-internal memory manager.
 */
kern_return_t
xmm_memory_manager_export(
	xmm_obj_t	mobj,
	ipc_port_t	memory_object)
{
	ipc_port_t xmm_object;

	/*
	 * Create an xmm object and associate it with stack.
	 * It will have one send right.
	 */
	xmm_object = xmm_object_allocate(mobj);
	if (xmm_object == IP_NULL) {
		panic("xmm_memory_manager_export: xmm_object_allocate");
	}

	/*
	 * Associate the xmm object with the memory object.
	 */
	xmm_object_set(memory_object, xmm_object, FALSE);
	return KERN_SUCCESS;
}
