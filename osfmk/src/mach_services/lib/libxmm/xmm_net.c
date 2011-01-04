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
 * Copyright (c) 1991 Carnegie Mellon University
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
 *	File:	xmm_net.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class for exporting objects across a network.
 */

#include <mach.h>
#include "xmm_obj.h"
#include "net.h"

#define	XReqBigRepl	100
#define	XReqSmallRepl	101
#define	XNoReqRepl	102
#define	XReqVmRepl	103

int Xflag = 0;

extern kern_return_t	tcp_init();
extern kern_return_t	udp_init();

struct net_proto net_protos[] = {
	{ tcp_init,	0,	{0,0,0,0},	"tcp"},
	{ udp_init,	0,	{0,0,0,0},	"udp"},
};

net_proto_t default_proto;	/* XXX */

struct mobj {
	struct xmm_obj	obj;
	mach_port_t	net_port;
	struct net_addr	addr;
	unsigned long	id;
	xmm_obj_t	next;
};

#undef	KOBJ
#define	KOBJ	((struct mobj *) kobj)

kern_return_t m_net_init();
kern_return_t m_net_terminate();
kern_return_t m_net_copy();
kern_return_t m_net_data_request();
kern_return_t m_net_data_unlock();
kern_return_t m_net_data_write();
kern_return_t m_net_lock_completed();
kern_return_t m_net_supply_completed();
kern_return_t m_net_data_return();

kern_return_t k_net_data_provided();
kern_return_t k_net_data_unavailable();
kern_return_t k_net_get_attributes();
kern_return_t k_net_lock_request();
kern_return_t k_net_data_error();
kern_return_t k_net_set_attributes();
kern_return_t k_net_destroy();
kern_return_t k_net_data_supply();

xmm_decl(import_mclass,
	/* m_init		*/	m_net_init,
	/* m_terminate		*/	m_net_terminate,
	/* m_copy		*/	m_net_copy,
	/* m_data_request	*/	m_net_data_request,
	/* m_data_unlock	*/	m_net_data_unlock,
	/* m_data_write		*/	m_net_data_write,
	/* m_lock_completed	*/	m_net_lock_completed,
	/* m_supply_completed	*/	m_net_supply_completed,
	/* m_data_return	*/	m_net_data_return,

	/* k_data_provided	*/	k_invalid_data_provided,
	/* k_data_unavailable	*/	k_invalid_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_invalid_data_error,
	/* k_set_attributes	*/	k_invalid_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"import.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(import_kclass,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_net_data_provided,
	/* k_data_unavailable	*/	k_net_data_unavailable,
	/* k_get_attributes	*/	k_net_get_attributes,
	/* k_lock_request	*/	k_net_lock_request,
	/* k_data_error		*/	k_net_data_error,
	/* k_set_attributes	*/	k_net_set_attributes,
	/* k_destroy		*/	k_net_destroy,
	/* k_data_supply	*/	k_net_data_supply,

	/* name			*/	"import.k",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(export_mclass,
	/* m_init		*/	m_interpose_init,
	/* m_terminate		*/	m_interpose_terminate,
	/* m_copy		*/	m_interpose_copy,
	/* m_data_request	*/	m_interpose_data_request,
	/* m_data_unlock	*/	m_interpose_data_unlock,
	/* m_data_write		*/	m_interpose_data_write,
	/* m_lock_completed	*/	m_interpose_lock_completed,
	/* m_supply_completed	*/	m_interpose_supply_completed,
	/* m_data_return	*/	m_interpose_data_return,

	/* k_data_provided	*/	k_interpose_data_provided,
	/* k_data_unavailable	*/	k_interpose_data_unavailable,
	/* k_get_attributes	*/	k_interpose_get_attributes,
	/* k_lock_request	*/	k_interpose_lock_request,
	/* k_data_error		*/	k_interpose_data_error,
	/* k_set_attributes	*/	k_interpose_set_attributes,
	/* k_destroy		*/	k_interpose_destroy,
	/* k_data_supply	*/	k_interpose_data_supply,

	/* name			*/	"export.m",
	/* size			*/	sizeof(struct mobj)
);

xmm_decl(export_kclass,
	/* m_init		*/	m_interpose_init,
	/* m_terminate		*/	m_interpose_terminate,
	/* m_copy		*/	m_interpose_copy,
	/* m_data_request	*/	m_interpose_data_request,
	/* m_data_unlock	*/	m_interpose_data_unlock,
	/* m_data_write		*/	m_interpose_data_write,
	/* m_lock_completed	*/	m_interpose_lock_completed,
	/* m_supply_completed	*/	m_interpose_supply_completed,
	/* m_data_return	*/	m_interpose_data_return,

	/* k_data_provided	*/	k_interpose_data_provided,
	/* k_data_unavailable	*/	k_interpose_data_unavailable,
	/* k_get_attributes	*/	k_interpose_get_attributes,
	/* k_lock_request	*/	k_interpose_lock_request,
	/* k_data_error		*/	k_interpose_data_error,
	/* k_set_attributes	*/	k_interpose_set_attributes,
	/* k_destroy		*/	k_interpose_destroy,
	/* k_data_supply	*/	k_interpose_data_supply,

	/* name			*/	"export.k",
	/* size			*/	sizeof(struct mobj)
);

net_init()
{
	extern int verbose;
	int i;
	net_proto_t np;

	for (i = 0; i < sizeof(net_protos)/sizeof(*net_protos); i++) {
		np = &net_protos[i];
		if (verbose) {
			printf("Initializing %s...\n", np->server_name);
		}
		(*np->server_init)(np);
	}
	default_proto = &net_protos[0];	/* 0->tcp, 1->udp */
}

struct net_addr xmm_net_null_addr = { 0 };

/*
 * XXX get rid of this list; will have to extend hash interface
 */

unsigned long wid_counter = 1;
xmm_obj_t xmm_net_obj_list;

xmm_obj_t
xmm_net_lookup_by_port(net_port)
	mach_port_t net_port;
{
	xmm_obj_t mobj;

	for (mobj = xmm_net_obj_list; mobj; mobj = MOBJ->next) {
		if (MOBJ->net_port == net_port) {
			return mobj;
		}
	}
	return XMM_OBJ_NULL;
}

xmm_obj_t
kobj_lookup_by_addr(waddr, wid, is_destination)
	net_addr_t waddr;
	unsigned long wid;
	boolean_t is_destination;
{
	xmm_obj_t kobj;

	if (NET_ADDR_NULL(waddr)) {
		return XMM_OBJ_NULL;
	}
	for (kobj = xmm_net_obj_list; kobj; kobj = KOBJ->next) {
		if (NET_ADDR_EQ(&KOBJ->addr, waddr) && KOBJ->id == wid) {
			return kobj;
		}
	}
	if (is_destination) {
		return XMM_OBJ_NULL;
	}
	/*
	 * kobj not here already; thus must create one as proxy.
	 */
	xmm_obj_allocate(&import_kclass, XMM_OBJ_NULL, &kobj);
	KOBJ->id = wid;
	KOBJ->addr = *waddr;
	KOBJ->next = xmm_net_obj_list;
	xmm_net_obj_list = kobj;
	return kobj;
}

xmm_obj_t
mobj_lookup_by_addr(waddr, wid, is_destination)
	net_addr_t waddr;
	unsigned long wid;
	boolean_t is_destination;
{
	xmm_obj_t mobj;

	if (NET_ADDR_NULL(waddr)) {
		return XMM_OBJ_NULL;
	}
	for (mobj = xmm_net_obj_list; mobj; mobj = MOBJ->next) {
		if (NET_ADDR_EQ(&MOBJ->addr, waddr) && MOBJ->id == wid) {
			return mobj;
		}
	}
	if (is_destination) {
		return XMM_OBJ_NULL;
	}
	/*
	 * mobj not here already; thus must create one as proxy.
	 * This can only happen as a reply_to port.
	 */
	xmm_obj_allocate(&import_mclass, XMM_OBJ_NULL, &mobj);
	MOBJ->id = wid;
	MOBJ->addr = *waddr;
	MOBJ->next = xmm_net_obj_list;
	xmm_net_obj_list = mobj;
	return mobj;
}

kern_return_t
xmm_net_export(old_mobj, net_port)
	xmm_obj_t old_mobj;
	mach_port_t *net_port;
{
	kern_return_t kr;
	xmm_obj_t mobj;

	kr = mach_port_get(net_port);
	if (kr) {
		panic("xmm_net_foo mach_port_get");
	}
	xmm_obj_allocate(&export_mclass, old_mobj, &mobj);
	MOBJ->net_port = *net_port;
	MOBJ->id = wid_counter++;
	MOBJ->addr = default_proto->server_addr;
	MOBJ->next = xmm_net_obj_list;
	xmm_net_obj_list = mobj;
	return KERN_SUCCESS;
}

/* used internally, not by users */
_xmm_net_lookup(net_port, waddr0, waddr1, waddr2, waddr3, wid)
	mach_port_t net_port;
	unsigned long *waddr0;
	unsigned long *waddr1;
	unsigned long *waddr2;
	unsigned long *waddr3;
	unsigned long *wid;
{
	xmm_obj_t mobj;

	mobj = xmm_net_lookup_by_port(net_port);
	if (mobj == XMM_OBJ_NULL) {
		printf("xmm_net_export: bad net object\n");
		return KERN_FAILURE;
	}
	*waddr0 = (unsigned long) ntohl(MOBJ->addr.a[0]);
	*waddr1 = (unsigned long) ntohl(MOBJ->addr.a[1]);
	*waddr2 = (unsigned long) ntohl(MOBJ->addr.a[2]);
	*waddr3 = (unsigned long) ntohl(MOBJ->addr.a[3]);
	*wid = MOBJ->id;
	return KERN_SUCCESS;
}

xmm_net_import(net_port, new_mobj)
	mach_port_t net_port;
	xmm_obj_t *new_mobj;
{
	unsigned long waddr0;
	unsigned long waddr1;
	unsigned long waddr2;
	unsigned long waddr3;
	unsigned long wid;
	struct net_addr waddr;
	kern_return_t kr;
	xmm_obj_t mobj;

	mobj = xmm_net_lookup_by_port(net_port);
	if (mobj != XMM_OBJ_NULL) {
		*new_mobj = mobj;
		return KERN_SUCCESS;
	}
	/* should be done asynchronously */
	xmm_net_lookup(net_port, &waddr0, &waddr1, &waddr2, &waddr3, &wid);

	waddr.a[0] = (unsigned long) htonl(waddr0);
	waddr.a[1] = (unsigned long) htonl(waddr1);
	waddr.a[2] = (unsigned long) htonl(waddr2);
	waddr.a[3] = (unsigned long) htonl(waddr3);
	xmm_obj_allocate(&import_mclass, XMM_OBJ_NULL, &mobj);
	MOBJ->net_port = net_port;
	MOBJ->id = wid;
	MOBJ->addr = waddr;
	MOBJ->next = xmm_net_obj_list;
	xmm_net_obj_list = mobj;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

/* XXX reply_to not handled correctly */
net_send_k(kobj, mobj, msg, data, length)
	xmm_obj_t kobj;
	xmm_obj_t mobj;
	net_msg_t msg;
	vm_offset_t data;
	vm_size_t length;
{
	net_addr_t source_addr;
	int source_id;
	kern_return_t kr;

	if (kobj == XMM_OBJ_NULL) {
		printf("net_send_k: null kobj\n");
		return KERN_FAILURE;
	}
	if (NET_ADDR_EQ(&KOBJ->addr, &default_proto->server_addr)) {
		printf("net_send_k: stupid kobj\n");
		return KERN_FAILURE;
	}
	if (mobj) {
		source_addr = &MOBJ->addr;
		source_id = MOBJ->id;
	} else {
		source_addr = &xmm_net_null_addr;
		source_id = 0;
	}
	msg->m_call = FALSE;
	kr = (*default_proto->server_send)(&KOBJ->addr, KOBJ->id,
					   source_addr, source_id,
					   msg, data, length);
	if (kr) {
		mach_error("## net_send_k", kr);
	}
	return KERN_SUCCESS;
}

net_send_m(mobj, kobj, msg, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	net_msg_t msg;
	vm_offset_t data;
	vm_size_t length;
{
	net_addr_t source_addr;
	int source_id;
	kern_return_t kr;

	if (NET_ADDR_EQ(&MOBJ->addr, &default_proto->server_addr)) {
		printf("net_send_m: stupid mobj\n");
		return KERN_FAILURE;
	}
	source_addr = &KOBJ->addr;
	source_id = KOBJ->id;
	msg->m_call = TRUE;
	kr = (*default_proto->server_send)(&MOBJ->addr, MOBJ->id,
					   source_addr, source_id,
					   msg, data, length);
	if (kr) {
		mach_error("## net_send_m", kr);
	}
	return KERN_SUCCESS;
}

m_net_init(mobj, k_kobj, memory_object_name, page_size)
	xmm_obj_t mobj;
	xmm_obj_t k_kobj;
	mach_port_t memory_object_name;
	vm_size_t page_size;
{
	struct net_msg msg;
	xmm_obj_t kobj;

#if     lint
	memory_object_name++;
#endif  /* lint */
	/* XXX we do not yet handle memory_object_name */

	xmm_obj_allocate(&export_kclass, XMM_OBJ_NULL, &kobj);
	k_kobj->m_kobj = kobj;
	kobj->k_kobj = k_kobj;

	KOBJ->id = wid_counter++;
	KOBJ->addr = default_proto->server_addr;
	KOBJ->next = xmm_net_obj_list;
	xmm_net_obj_list = kobj;

	msg.id = M_INIT_ID;
	msg.arg1 = htonl((unsigned long) page_size);
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_terminate(mobj, kobj, memory_object_name)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	mach_port_t memory_object_name;
{
	struct net_msg msg;

#if     lint
	memory_object_name++;
#endif  /* lint */
	/* XXX we do not yet handle memory_object_name */
	msg.id = M_TERMINATE_ID;
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_copy(mobj, kobj, offset, length, new_mobj)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	xmm_obj_t new_mobj;
{
	struct net_msg msg;

#ifdef	lint
	new_mobj++;
#endif  /* lint */
	/* XXX we do not yet handle new_mobj */
	msg.id = M_COPY_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	struct net_msg msg;

	if (Xflag == XNoReqRepl) {
		vm_offset_t data;
		
		vm_allocate_dirty(&data);
		K_DATA_PROVIDED(kobj, offset, data, length, VM_PROT_NONE);
		return KERN_SUCCESS;
	}
	msg.id = M_DATA_REQUEST_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(desired_access);
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_data_unlock(mobj, kobj, offset, length, desired_access)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{
	struct net_msg msg;

	msg.id = M_DATA_UNLOCK_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(desired_access);
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	struct net_msg msg;

	msg.id = M_DATA_WRITE_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	return net_send_m(mobj, kobj, &msg, data, length);
}

m_net_lock_completed(mobj, kobj, offset, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	struct net_msg msg;

	msg.id = M_LOCK_COMPLETED_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_supply_completed(mobj, kobj, offset, length, result, error_offset)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{
	struct net_msg msg;

	msg.id = M_SUPPLY_COMPLETED_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(result);
	msg.arg4 = htonl(error_offset);
	return net_send_m(mobj, kobj, &msg, 0, 0);
}

m_net_data_return(mobj, kobj, offset, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
{
	struct net_msg msg;

	msg.id = M_DATA_REQUEST_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	return net_send_m(mobj, kobj, &msg, data, length);
}

m_net_get_attributes_reply(kobj)
	xmm_obj_t kobj;
{
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;

	K_GET_ATTRIBUTES(kobj, &object_ready, &may_cache, &copy_strategy);
	/* XXX not yet implmented */
}

k_net_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	struct net_msg msg;

	msg.id = K_DATA_PROVIDED_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(lock_value);
	return net_send_k(kobj, XMM_OBJ_NULL, &msg, data, length);
}

k_net_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	struct net_msg msg;

	msg.id = K_DATA_UNAVAILABLE_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	return net_send_k(kobj, XMM_OBJ_NULL, &msg, 0, 0);
}

k_net_get_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t *object_ready;
	boolean_t *may_cache;
	memory_object_copy_strategy_t *copy_strategy;
{
	struct net_msg msg;

#if     lint
	object_ready++;
	may_cache++;
	copy_strategy++;
#endif  /* lint */
	msg.id = K_GET_ATTRIBUTES_ID;
	return net_send_k(kobj, XMM_OBJ_NULL, &msg, 0, 0);
	/* XXX should block for reply? */
}

k_net_lock_request(kobj, offset, length, should_clean, should_flush,
		      lock_value, robj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	boolean_t should_clean;
	boolean_t should_flush;
	vm_prot_t lock_value;
	xmm_obj_t robj;
{
	struct net_msg msg;

	msg.id = K_LOCK_REQUEST_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(should_clean);
	msg.arg4 = htonl(should_flush);
	msg.arg5 = htonl(lock_value);
	return net_send_k(kobj, robj, &msg, 0, 0);
}

k_net_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	struct net_msg msg;

	msg.id = K_DATA_ERROR_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(error_value);
	return net_send_k(kobj, XMM_OBJ_NULL, &msg, 0, 0);
}

k_net_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	struct net_msg msg;

	msg.id = K_SET_ATTRIBUTES_ID;
	msg.arg1 = htonl(object_ready);
	msg.arg2 = htonl(may_cache);
	msg.arg3 = htonl(copy_strategy);
	return net_send_k(kobj, XMM_OBJ_NULL, &msg, 0, 0);
}

k_net_destroy(kobj, reason)
	xmm_obj_t kobj;
	kern_return_t reason;
{
	struct net_msg msg;

	msg.id = K_DESTROY_ID;
	msg.arg1 = htonl(reason);
	return net_send_k(kobj, XMM_OBJ_NULL, &msg, 0, 0);
}

k_net_data_supply(kobj, offset, data, length, dealloc_data, lock_value,
		   precious, mobj)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	boolean_t dealloc_data;
	vm_prot_t lock_value;
	boolean_t precious;
	xmm_obj_t mobj;
{
	struct net_msg msg;

	msg.id = K_DATA_SUPPLY_ID;
	msg.arg1 = htonl(offset);
	msg.arg2 = htonl(length);
	msg.arg3 = htonl(dealloc_data);
	msg.arg4 = htonl(lock_value);
	msg.arg5 = htonl(precious);
	return net_send_k(kobj, mobj, &msg, data, length);
}

#undef	O1
#undef	O2
#undef	O3
#undef	O4
#undef	O5
#define	O1	((vm_offset_t) ntohl(msg->arg1))
#define	O2	((vm_offset_t) ntohl(msg->arg2))
#define	O3	((vm_offset_t) ntohl(msg->arg3))
#define	O4	((vm_offset_t) ntohl(msg->arg4))
#define	O5	((vm_offset_t) ntohl(msg->arg5))

#undef	A1
#undef	A2
#undef	A3
#undef	A4
#undef	A5
#define	A1	ntohl(msg->arg1)
#define	A2	ntohl(msg->arg2)
#define	A3	ntohl(msg->arg3)
#define	A4	ntohl(msg->arg4)
#define	A5	ntohl(msg->arg5)

#undef	L1
#undef	L2
#undef	L3
#undef	L4
#undef	L5
#define	L1	((vm_size_t) ntohl(msg->arg1))
#define	L2	((vm_size_t) ntohl(msg->arg2))
#define	L3	((vm_size_t) ntohl(msg->arg3))
#define	L4	((vm_size_t) ntohl(msg->arg4))
#define	L5	((vm_size_t) ntohl(msg->arg5))

#undef	B1
#undef	B2
#undef	B3
#undef	B4
#undef	B5
#define	B1	((boolean_t) ntohl(msg->arg1))
#define	B2	((boolean_t) ntohl(msg->arg2))
#define	B3	((boolean_t) ntohl(msg->arg3))
#define	B4	((boolean_t) ntohl(msg->arg4))
#define	B5	((boolean_t) ntohl(msg->arg5))

#undef	E1
#undef	E2
#undef	E3
#undef	E4
#undef	E5
#define	E1	((kern_return_t) ntohl(msg->arg1))
#define	E2	((kern_return_t) ntohl(msg->arg2))
#define	E3	((kern_return_t) ntohl(msg->arg3))
#define	E4	((kern_return_t) ntohl(msg->arg4))
#define	E5	((kern_return_t) ntohl(msg->arg5))

#undef	P1
#undef	P2
#undef	P3
#undef	P4
#undef	P5
#define	P1	((vm_prot_t) ntohl(msg->arg1))
#define	P2	((vm_prot_t) ntohl(msg->arg2))
#define	P3	((vm_prot_t) ntohl(msg->arg3))
#define	P4	((vm_prot_t) ntohl(msg->arg4))
#define	P5	((vm_prot_t) ntohl(msg->arg5))

net_dispatch(mobj, kobj, msg, data, length)
	xmm_obj_t mobj;
	xmm_obj_t kobj;
	net_msg_t msg;
	vm_offset_t data;
	vm_size_t length;
{
#if     lint
	length++;
#endif  /* lint */
	switch ((int) msg->id) {
		case M_INIT_ID:
		M_INIT(mobj, kobj, MACH_PORT_NULL/*XXX*/, L1);
		break;

		case M_TERMINATE_ID:
		M_TERMINATE(mobj, kobj, MACH_PORT_NULL/*XXX*/);
		break;

		case M_COPY_ID:
		M_COPY(mobj, kobj, O1, L2, XMM_OBJ_NULL);
		break;

		case M_DATA_REQUEST_ID:
		if (Xflag == XReqBigRepl) {
			char xxx[8192];
			(void) k_net_data_provided(kobj, O1,
						   (vm_offset_t) xxx, L2,
						   VM_PROT_NONE);
			break;
		}
		if (Xflag == XReqSmallRepl) {
			(void) k_net_data_provided(kobj, O1,
						   (vm_offset_t) 0, L2,
						   VM_PROT_NONE);
			break;
		}
		if (Xflag == XReqVmRepl) {
			vm_offset_t data;
			vm_allocate_dirty(&data);
			(void) k_net_data_provided(kobj, O1, data, L2,
						   VM_PROT_NONE);
			break;
		}
		M_DATA_REQUEST(mobj, kobj, O1, L2, P3);
		break;

		case M_DATA_UNLOCK_ID:
		M_DATA_UNLOCK(mobj, kobj, O1, L2, P3);
		break;

		case M_DATA_WRITE_ID:
		M_DATA_WRITE(mobj, kobj, O1, data, L2);
		break;

		case M_LOCK_COMPLETED_ID:
		M_LOCK_COMPLETED(mobj, kobj, O1, L2);
		break;

		case M_SUPPLY_COMPLETED_ID:
		M_SUPPLY_COMPLETED(mobj, kobj, O1, L2, E3, O4);
		break;

		case M_DATA_RETURN_ID:
		M_DATA_RETURN(mobj, kobj, O1, data, L2);
		break;

		case K_DATA_PROVIDED_ID:
		if (Xflag == XReqSmallRepl) {
			if (data == 0) {
				vm_allocate_dirty(&data);
			}
		}
		K_DATA_PROVIDED(kobj, O1, data, L2, P3);
		break;
		
		case K_DATA_UNAVAILABLE_ID:
		K_DATA_UNAVAILABLE(kobj, O1, L2);
		break;
		
		case K_GET_ATTRIBUTES_ID:
		m_net_get_attributes_reply(kobj);
		break;
		
		case K_LOCK_REQUEST_ID:
		K_LOCK_REQUEST(kobj, O1, L2, B3, B4, P5, mobj);
		break;
		
		case K_DATA_ERROR_ID:
		K_DATA_ERROR(kobj, O1, L2, E3);
		break;
		
		case K_SET_ATTRIBUTES_ID:
		K_SET_ATTRIBUTES(kobj, B1, B2, A3);
		break;
		
		case K_DESTROY_ID:
		K_DESTROY(kobj, E1);
		break;
		
		case K_DATA_SUPPLY_ID:
		K_DATA_SUPPLY(kobj, O1, data, L2, B3, P4, B5, mobj);
		break;
		
		default:
		printf("net_dispatch: bad netmsg id %lu\n", msg->id);
	}
}
