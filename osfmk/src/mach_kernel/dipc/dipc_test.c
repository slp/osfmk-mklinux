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
 *	dipc_test.c	in-kernel unit tests for DIPC
 */

#include <dipc_timer.h>
#include <kernel_test.h>
#include <mach_kdb.h>
#include <flipc.h>

#if	KERNEL_TEST

#include <ddb/kern_test_defs.h>
#include <kern/assert.h>
#include <mach/kkt_request.h>
#include <mach/norma_special_ports.h>
#include <dipc/special_ports.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_mqueue.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <dipc/port_table.h>
#include <dipc/dipc_rpc.h>
#include <kern/lock.h>
#include <dipc/dipc_port.h>
#include <kern/zalloc.h>
#include <kern/misc_protos.h>
#include <kern/ipc_sched.h>
#include <vm/vm_kern.h>
#if	DIPC_TIMER
#include <dipc/dipc_timer.h>
#endif	/* DIPC_TIMER */
#include <ddb/tr.h>

#if	!DIPC_TIMER
#define	timer_set(a,b)
#define	timer_start(a)
#define	timer_stop(a,b)
#endif	/* DIPC_TIMER */

#if	!MACH_ASSERT
/*
 *	Even when assertions are disabled for
 *	the rest of the world, turn them on
 *	for this code, which is so heavily
 *	assertion-dependent.
 *
 *	This can be a bit of a problem for any
 *	code that believe that structures have
 *	additional debugging fields.
 *
 *	This is NOT MP-safe.
 */
#undef	assert
#define assert(ex)							\
MACRO_BEGIN								\
	if (!(ex))							\
		dipc_test_assert(__FILE__, __LINE__);			\
MACRO_END
void	dipc_test_assert(
		char	*file,
		int	line);

#define NO_MACH_ASSERT 1

#else

#define NO_MACH_ASSERT 0

#endif	/* !MACH_ASSERT */

ipc_space_t	ipc_space_dipc_test;
ipc_port_t	dipc_test_port;
ipc_port_t	dipc_test_port_proxy;
ipc_port_t	dipc_inactive_port;
ipc_port_t	handy_port = IP_NULL;

/*
 *	Following define really should be exported by kkt.
 */

#define	DIPC_TEST_SIZE	(2 * 1024 * 1024)
#define	DIPC_TEST_SLOP	8192
char	dipc_test_region_memory[DIPC_TEST_SIZE + DIPC_TEST_SLOP];

vm_page_t	dipc_test_pages = VM_PAGE_NULL;
vm_offset_t	dipc_test_region = (vm_offset_t) 0;
vm_size_t	dipc_test_size = DIPC_TEST_SIZE;

void		*dtest_port_origin, *dtest_port_ident;
void		do_verify_ool(
			mach_msg_header_t	*msg_recv,
			mach_msg_header_t	*msg_sent);
void		ool_receiver_thread(void);
void		dtest_kkt_test_notify(
			int		error,
			handle_t	handle,
			request_block_t req,
			boolean_t	thread_context);
ipc_port_t
dtest_port_alloc_sright(
	boolean_t	nms_detection,
	boolean_t	send_right);
void dtest_port_destroy(ipc_port_t port);
mach_msg_return_t
dipc_msg_send_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_size_t		send_size,
	mach_msg_option_t	option,
	mach_msg_timeout_t	timeout);
void dtest_uid_routines(void);
void dtest_special_ports(void);
void dtest_alloc_routines(void);
void dtest_rpc(void);
void dtest_mqdeliver_to_msgqueue(void);
void dtest_enqueue_to_msgqueue(void);
void do_deliver(
	void 		*ep0,
	void 		*ep1,
	vm_size_t	size,
	unsigned int	intr_control,
	kern_return_t	cr1,
	kern_return_t	cr2);
void
dtest_connect_reply(
	handle_t	handle,
	kern_return_t	ret1,
	kern_return_t	ret2);
void dtest_deliver_to_msgqueue(void);
void dtest_pull_ool_data(
			 ipc_kmsg_t	kmsg);
void dtest_send_to_msgqueue(void);
void dtest_send_ool_to_msgqueue(void);
void dipc_test_intr(void);
void dtest_port_copyinout(void);
void dtest_copyin(void);
void dtest_copyout(void);
void dtest_send_test_recv_msg(
			      ipc_port_t port,
			      int msgnum);
void dmt_do_test(
		 dipc_return_t	(*test)(node_name node),
		 char		*description,
		 node_name	node);
void dipc_multinode_test_thread(void);
void dtest_send_error(
		      int		error,
		      handle_t	handle,
		      request_block_t req);
void dmt_start(node_name node);
void
dtest_send_test_msg(
	mach_msg_header_t	*h,
	int			msgnum,
	mach_msg_option_t	option);
void dtest_send_test(void);
void dipc_test_thread(void);
void dipc_test_init(boolean_t startup);

extern void	kkt_test_driver(
			node_name	node);
extern void	transport_send_test(
			vm_size_t	send_size,
			vm_offset_t	send_data,
			node_name	node);
extern void	transport_test_setup(void);

typedef void (*kkt_test_client)(void);
dipc_return_t	dmt_begin_sender(node_name);
dipc_return_t	dmt_end_sender(node_name);
void		dmt_begin_receiver(void);
void		dmt_end_receiver(void);
int		dipc_test_obtain_buffer(char *);
int		dipc_test_verify_buffer(char *);
int		dipc_test_release_buffer(char *);

#if	FLIPC
extern void	flipcme_low_level_test(
			node_name	node,
			vm_size_t	test_size,
			unsigned int	*result);
#endif	/* FLIPC */

decl_simple_lock_data(,dipc_test_lock_data)

#define dipc_test_lock()	simple_lock(&dipc_test_lock_data);
#define dipc_test_unlock()	simple_unlock(&dipc_test_lock_data);

/*
 *	The following are used to serialize and synchronize
 *	the kernel tests. 
 */

extern	boolean_t	kernel_test_thread_blocked;
extern	boolean_t	unit_test_thread_sync_done;
extern	boolean_t	unit_test_thread_blocked;
extern	boolean_t	unit_test_done;
decl_simple_lock_data(extern, kernel_test_lock)

typedef struct transmit {
	unsigned int		node_token;	/* node or token */
	endpoint_t		endpoint; 	/* endpoint param */
	vm_size_t		size;		/* size param */
	unsigned int		buff[1];	/* data if valid */
} *transmit_t;

char		transmit_buf[400];
transmit_t	transp = (transmit_t)transmit_buf;
mach_msg_header_t	*trans_msghp;
kern_return_t	dtest_cr1, dtest_cr2;
vm_offset_t	save_free_elements;

#define	dtest_intr_control	kern_test_intr_control[DIPC_TEST]
#define	DTEST_DELIVER			1
#define	DTEST_DELIVER_INLINE		2
#define	DTEST_DELIVER_RESTORE_ZONE	3

/* external definitions */
extern zone_t		kkt_req_zone;
extern zone_t		fast_kmsg_zone;
extern boolean_t	dipc_kkt_transport_threads;
extern zone_t		meta_kmsg_zone;
extern unsigned int	dipc_fast_kmsg_size;
decl_simple_lock_data(extern,dipc_cleanup_lock_data)

#define	dipc_cleanup_lock()	simple_lock(&dipc_cleanup_lock_data);
#define	dipc_cleanup_unlock()	simple_unlock(&dipc_cleanup_lock_data);

extern struct ipc_kmsg_queue	dipc_kmsg_destroy_queue;
extern vm_map_t			kernel_map;

#define HANDLE_DIPC_TEST	((handle_t)(-1))


/*
 *	Allocate a receive right with one send or send-
 *	once right made from it.  The caller optionally
 *	requests NMS detection on the port.
 *
 *	The port probably should be freed using
 *	dtest_port_destroy, unless you've done something
 *	funny to it that will lead to port death
 *	notifications, etc.  In that case, you should
 *	clean up the port state enough so that you *can*
 * 	call dtest_port_destroy.
 *
 *	N.B.  DIPC will treat this port as a principal
 *	unless you make a special effort otherwise.
 *	Proxies are created via DIPC_UID_MAKE +
 *	dipc_port_copyout.  Don't try to free a
 *	principal directly via ipc_port_release or
 *	ip_release+ip_check_unlock -- we try hard to
 *	catch that case.
 */
#define	SEND_RIGHT	TRUE
#define	SO_RIGHT	FALSE
ipc_port_t
dtest_port_alloc_sright(
	boolean_t	nms_detection,
	boolean_t	send_right)
{
	ipc_port_t	port;

	port = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(port != IP_NULL);
	assert(!DIPC_IS_PROXY(port));
	if (nms_detection == TRUE)
		IP_SET_NMS(port);
	if (send_right == TRUE)
		(void) ipc_port_make_send(port);
	else
		(void) ipc_port_make_sonce(port);
	assert(port->ip_references == 2);
	return port;
}


/*
 *	Dispose of a receive right.  This was a port
 *	created by dipc_port_alloc_send or similar,
 *	rather than by arriving from off-node.
 */
void
dtest_port_destroy(
	ipc_port_t	port)
{
	ip_lock(port);
	assert(IP_VALID(port));
	assert(!DIPC_IS_DIPC_PORT(port) || port->ip_proxy == FALSE);
	assert(port->ip_references > 0);
	ipc_port_clear_receiver(port);
	ipc_port_destroy(port);
}


/*
 * dipc_msg_send_from_kernel
 *
 * This routine is mach_msg_send_from_kernel except that it allows
 * the sender to supply option and timeout to ipc_mqueue_send.
 */
mach_msg_return_t
dipc_msg_send_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_size_t		send_size,
	mach_msg_option_t	option,
	mach_msg_timeout_t	timeout)
{
	ipc_kmsg_t		kmsg;
	mach_msg_return_t	mr;

	if (!MACH_PORT_VALID(msg->msgh_remote_port))
		return MACH_SEND_INVALID_DEST;

	mr = ipc_kmsg_get_from_kernel(msg, send_size, &kmsg);
	if (mr != MACH_MSG_SUCCESS) {
		printf("dipc_msg_send_from_kernel: kmsg_get returned 0x%x\n",
			mr);
		assert(FALSE);
	}
	assert(kmsg != IKM_NULL);
	assert(!KMSG_IS_DIPC_FORMAT(kmsg));

	ipc_kmsg_copyin_from_kernel(kmsg);
	return ipc_mqueue_send(kmsg, option, timeout);
}

/*
 *	Test port name table routines.
 */

void
dtest_uid_routines(void)
{
	ipc_port_t	port, port2;
	dipc_port_t	dip, dip2;
	dipc_return_t	dr;
	extern zone_t	dipc_port_zone;

	/*
	 *	Test assignment of a fresh uid.
	 */

	port = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(port != IP_NULL);
	dip = (dipc_port_t) zalloc(dipc_port_zone);
	assert(dip != DIP_NULL);
	bzero((char *)dip, sizeof(struct dipc_port));

	/*
	 *	The dip will be attached to the port as
	 *	a side-effect of this call.
	 */
	dr = dipc_uid_allocate(port, dip);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_UID_VALID(&dip->dip_uid));
	assert(DIPC_IS_DIPC_PORT(port));


#if	!NO_MACH_ASSERT

	/* 
	 *	We can  not do this test w/o MACH_ASSERT being
	 *	globally true since the uid code will not bother
	 *	checking for duplicate UIDs
	 */

	/*
	 *	Test insertion of a duplicate UID.
	 */

	port2 = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(port2 != IP_NULL);
	dip2 = (dipc_port_t) zalloc(dipc_port_zone);
	assert(dip2 != DIP_NULL);
	bzero((char *)dip2, sizeof(struct dipc_port));

	port2->ip_dipc = dip2;

	DIPC_UID_MAKE(&port2->ip_uid, NODE_NAME_NULL, PORT_ID_NULL);

	dr = dipc_uid_install(port2, &port->ip_uid);
	assert(dr == DIPC_DUPLICATE);

	port2->ip_dipc = DIP_NULL;

	zfree(dipc_port_zone, (vm_offset_t)dip2);

	ip_lock(port2);
	assert(port2->ip_references == 1);
	ip_release(port2);
	ip_check_unlock(port2);
	
#endif	/*!NO_MACH_ASSERT*/

	/*
	 *	Look up an existing UID.
	 */

	dr = dipc_uid_lookup(&port->ip_uid, &port2);
	assert(dr == DIPC_SUCCESS);
	assert(port2 == port);
	assert(port->ip_references == 1);
	assert(port->ip_ref_count == 1);

	/*
	 *	Convert a DIPC-level port reference
	 *	into a real port reference.
	 */

	dr = dipc_uid_port_reference(port2);
	assert(dr == DIPC_SUCCESS);
	assert(port2 == port);
	assert(port->ip_references == 2);
	assert(port->ip_dipc->dip_ref_count == 0);

	/*
	 *	Remove a port from the name table.
	 */

	dr = dipc_uid_remove(port);
	assert(dr == DIPC_SUCCESS);

	/*
	 *	Search for a non-existent port in the
	 *	name table.
	 */

	dr = dipc_uid_lookup(&port->ip_uid, &port2);
	assert(dr == DIPC_NO_UID);

	ip_lock(port);
	assert(port->ip_references == 2);
	port->ip_dipc = DIP_NULL;
	zfree(dipc_port_zone, (vm_offset_t)dip);

	ip_release(port);
	ip_release(port);
	ip_check_unlock(port);

	printf("dtest_uid_routines: done\n");
}


/*
 *	Test special ports functionality.
 *
 *	Kernel special ports, even proxies for remote
 *	kernel special ports, are handled with purely
 *	local-node operations.  However, user special
 *	ports must go remote.
 *
 *	Requirements:
 *		- Port name table routines must work.
 *
 *	dtest_special_ports() tests only that special
 *	ports functionality that can be executed on
 *	one node.  Remote node testing is done elsewhere.
 */

/*
 *	A special port ID guaranteed to be invalid.
 */
#define	BOGUS_SPECIAL_PORT_ID	12345

/*
 *	A special port ID guaranteed to be a valid
 *	user-settable ID -- but that doesn't conflict
 *	with any currently known user-settable IDs.
 */
#define	GOOD_USER_PORT_ID	(NORMA_TEST_PORT - 1)

void
dtest_special_ports(void)
{
	ipc_port_t		device_port, host_port, host_priv_port;
	ipc_port_t		peek_port, remote_dev_port, user_port;
	ipc_port_t		test_user_port, bogon_port, user_port2;
	node_name		mynode = dipc_node_self();
	kern_return_t		kr;
	uid_t			uid;
	ipc_object_refs_t	iprefs;
	mach_port_rights_t	ipsrights;
	extern ipc_port_t	get_special_port(node_name,
						 unsigned long,
						 dipc_return_t *);
	dipc_return_t		dr;
#if	MACH_ASSERT
	extern boolean_t	norma_special_ports_initialized;
#endif	/* MACH_ASSERT */
#if	DIPC_DO_STATS
	unsigned int		dipc_remote_entries;
	extern unsigned int	c_dipc_uid_table_entries;
#endif	/* DIPC_DO_STATS */


	/*
	 *	After special ports have been initialized,
	 *	we have executed three kernel_set_special_port
	 *	calls, one each for NORMA_DEVICE_PORT,
	 *	NORMA_HOST_PORT	and NORMA_HOST_PRIV_PORT.
	 *	So kernel_set_special_port is known to work.
	 *	We won't bother testing the routine for
	 *	handling bogus special port IDs because the
	 *	user is a trusted kernel programmer.
	 */

#if	MACH_ASSERT
	/*
	 *	I can explain...  when !MACH_ASSERT but
	 *	KERNEL_TEST, we have the special definition
	 *	for assert() (see above).  But of course
	 *	there's no norma_special_ports_initialized
	 *	variable.
	 */
	assert(norma_special_ports_initialized == TRUE);
#endif

	/*
	 *	Now, we should be able to look up the ports
	 *	created at initialization time.
	 */
	device_port = get_special_port(mynode, NORMA_DEVICE_PORT, &dr);
	assert(device_port != IP_NULL);
	assert(dr == DIPC_SUCCESS);
	/*
	 *	There should be at least two references on the port,
	 *	one when the port was created and one acquired
	 *	by calling get_special_port, which also created
	 *	a send right.
	 */
	assert(device_port->ip_references > 1);
	assert(device_port->ip_srights > 0);
	ipc_port_release_send(device_port);

	host_port = get_special_port(mynode, NORMA_HOST_PORT, &dr);
	assert(host_port != IP_NULL);
	assert(dr == DIPC_SUCCESS);
	/*
	 *	Apparently, the host port has already acquired a
	 *	bunch of send rights and references at this point,
	 *	so it's unreasonable to predict what these values
	 *	will be.
	 */
	assert(host_port->ip_references >= 2);
	assert(host_port->ip_srights >= 1);
	ipc_port_release_send(host_port);

	host_priv_port = get_special_port(mynode, NORMA_HOST_PRIV_PORT, &dr);
	assert(dr == DIPC_SUCCESS);
	assert(host_priv_port != IP_NULL);
	assert(host_priv_port->ip_references > 1);
	assert(host_priv_port->ip_srights > 0);
	ipc_port_release_send(host_priv_port);

	/*
	 *	Test "retrieving" remote kernel special ports.
	 *	Really, if the special port isn't already in the port
	 *	name table, a port will be created and installed
	 *	on-the-fly.  No interaction with the remote node
	 *	is required.  We can use NODE_NAME_LOCAL to test
	 *	this case.  First check to make sure that the
	 *	port we want to create doesn't already exist.
	 */
	
	DIPC_UID_MAKE(&uid, NODE_NAME_LOCAL, NORMA_DEVICE_PORT);
	assert(dipc_uid_lookup(&uid, &peek_port) == DIPC_NO_UID);

#if	DIPC_DO_STATS
	dipc_remote_entries = c_dipc_uid_table_entries;
#endif	/* DIPC_DO_STATS */
	remote_dev_port = get_special_port(NODE_NAME_LOCAL, NORMA_DEVICE_PORT,
									&dr);
	assert(dr == DIPC_SUCCESS);
	assert(remote_dev_port != IP_NULL);
	assert(DIPC_UID_EQUAL(&remote_dev_port->ip_uid, &uid));
	assert(remote_dev_port->ip_references == 1);
	assert(remote_dev_port->ip_srights == 1);
#if	DIPC_DO_STATS
	/*
	 *	Verify that a special port goes away
	 *	when the last reference to it is destroyed.
	 */
	assert(dipc_remote_entries == c_dipc_uid_table_entries - 1);
#endif	/* DIPC_DO_STATS */
	ipc_port_release_send(remote_dev_port);
#if	DIPC_DO_STATS
	/*
	 *	Verify that a special port goes away
	 *	when the last reference to it is destroyed.
	 */
	assert(dipc_remote_entries == c_dipc_uid_table_entries);
#endif	/* DIPC_DO_STATS */

	/*
	 *	These calls should all fail.
	 */
	assert(norma_get_special_port(HOST_NULL, mynode,
		BOGUS_SPECIAL_PORT_ID, &bogon_port) == KERN_INVALID_HOST);
	assert(norma_get_special_port((host_t) 1, mynode, -1, &bogon_port) ==
	       KERN_INVALID_ARGUMENT);
	assert(norma_get_special_port((host_t) 1, mynode,
		BOGUS_SPECIAL_PORT_ID, &bogon_port) == KERN_INVALID_ARGUMENT);
	assert(norma_get_special_port((host_t) 1, mynode, 0,
				      &bogon_port) == KERN_SUCCESS &&
				      bogon_port == IP_NULL);

	/*
	 *	Install and retrieve a local user special port.
	 */
	user_port = ipc_port_alloc_special(ipc_space_dipc_test);
	handy_port = user_port;		/* to poke at in the debugger */
	assert(user_port != IP_NULL);

	ip_lock(user_port);
	iprefs = user_port->ip_references;
	ipsrights = user_port->ip_srights;
	ip_unlock(user_port);
	kr = norma_set_special_port((host_t) 1, GOOD_USER_PORT_ID, user_port);
	assert(kr == KERN_SUCCESS);
	ip_lock(user_port);
	assert(iprefs == user_port->ip_references);
	assert(ipsrights == user_port->ip_srights);
	ip_unlock(user_port);

	kr = norma_get_special_port((host_t) 1, mynode, GOOD_USER_PORT_ID,
				    &test_user_port);
	assert(kr == KERN_SUCCESS);
	assert(user_port == test_user_port);
	/*
	 *	The port acquired a reference when it was created.
	 *	Entering it into the special ports table does not
	 *	add a reference (otherwise, changes would be
	 *	required to the port destruction logic to check
	 *	when only one reference was outstanding OR a
	 *	special port, once checked in, couldn't be
	 *	destroyed until it was checked out.
	 *
	 *	Retrieving the special port made a send right,
	 *	which increased the ip_refcount and ip_srights
	 *	(which formerly was zero).
	 */
	assert(user_port->ip_references == 2);
	assert(user_port->ip_srights == 1);

	/*
	 *	Test the norma_set_special_port feature that
	 *	allows atomically swapping a new special port
	 *	for an old one.
	 */
	printf("\tExpect to see a warning about replacing a special port.\n");
	user_port2 = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(user_port2 != IP_NULL);
	kr = norma_set_special_port((host_t) 1, GOOD_USER_PORT_ID, user_port2);
	assert(kr == KERN_SUCCESS);
	ip_lock(user_port);
	assert(user_port->ip_references == 2);
	assert(user_port->ip_srights == 1);
	assert(user_port->ip_special == FALSE);
	ip_unlock(user_port);
	ip_lock(user_port2);
	assert(user_port2->ip_references == 1);
	assert(user_port2->ip_special == TRUE);
	ip_unlock(user_port2);

	printf("\tExpect to see a warning about replacing a special port.\n");
	kr = norma_set_special_port((host_t) 1, GOOD_USER_PORT_ID, IP_NULL);
	assert(kr == KERN_SUCCESS);

	/*
	 *	Destroy outstanding ports.
	 */
	ipc_port_release(user_port);
	dtest_port_destroy(user_port);
	printf("\tExpect to see a warning about destroying a special port.\n");
	dtest_port_destroy(user_port2);

	printf("dtest_special_ports: done\n");
}


/*
 * Test the basic DIPC alloc and free routines.
 * Must be run at startup time.
 */
void
dtest_alloc_routines(void)
{
	request_block_t	req1, req2;
	ipc_kmsg_t	kmsg, kmsg2;
	meta_kmsg_t	mkmsg;
	spl_t	s;

	/* test dipc_get_kkt_req/dipc_free_kkt_req */

	assert(kkt_req_zone->count == 0);
	req1 = dipc_get_kkt_req(TRUE);
	assert(req1 != NULL_REQUEST);
	assert(kkt_req_zone->count == 1);
	/* this assumes the zone was crammed with at least two reqeusts */
	req2 = dipc_get_kkt_req(TRUE);
	assert(req2 != NULL_REQUEST);
	assert(kkt_req_zone->count == 2);
	dipc_free_kkt_req(req2);
	assert(kkt_req_zone->count == 1);
	dipc_free_kkt_req(req1);
	assert(kkt_req_zone->count == 0);

	/* test dipc_meta_kmsg_alloc/dipc_meta_kmsg_free */
	assert(meta_kmsg_zone->count == 0);
	mkmsg = dipc_meta_kmsg_alloc(TRUE);
	assert(mkmsg != MKM_NULL);
	assert(KMSG_IS_META(mkmsg));
	assert(meta_kmsg_zone->count == 1);
	dipc_meta_kmsg_free(mkmsg);
	assert(meta_kmsg_zone->count == 0);

	/*
	 * test dipc_kmsg_alloc/dipc_kmsg_free
	 *	-- but only the "interrupt level" path for now XXX
	 */
	if (!dipc_kkt_transport_threads) {
		assert(fast_kmsg_zone->count == 0);
		kmsg = dipc_kmsg_alloc(dipc_fast_kmsg_size,
				       dipc_kkt_transport_threads);
		assert(kmsg != IKM_NULL);
		assert(!KMSG_IS_META(kmsg));
		assert(fast_kmsg_zone->count == 1);
		dipc_kmsg_free(kmsg);
		assert(fast_kmsg_zone->count == 0);
	
		/*
		 * test dipc_kmsg_destroy_delayed and the dipc_cleanup_thread 
		 */
		assert(fast_kmsg_zone->count == 0);
		kmsg = dipc_kmsg_alloc(dipc_fast_kmsg_size,
				       dipc_kkt_transport_threads);
		assert(kmsg != IKM_NULL);
		assert(!KMSG_IS_META(kmsg));
		assert(fast_kmsg_zone->count == 1);

		/*
		 * null out important fields in the kmsg so that it doesn't get
		 * "cleaned" by ipc_kmsg_destroy.
		 */
		kmsg->ikm_header.msgh_remote_port = MACH_PORT_NULL;
		kmsg->ikm_header.msgh_local_port = MACH_PORT_NULL;
		kmsg->ikm_header.msgh_size = 0;
		dipc_kmsg_destroy_delayed(kmsg);
		s = splkkt();
		dipc_cleanup_lock();
		while ((kmsg2 = ipc_kmsg_queue_first(&dipc_kmsg_destroy_queue)) != IKM_NULL){
			assert(kmsg == kmsg2);
			assert(fast_kmsg_zone->count == 1);
			assert_wait((event_t)
				    (((int)&dipc_kmsg_destroy_queue) + 1),
				    TRUE);
			dipc_cleanup_unlock();
			splx(s);
			thread_set_timeout(hz);	/* pause for a second */
			thread_block((void (*)(void)) 0);
			reset_timeout_check(&current_thread()->timer);
			s = splkkt();
			dipc_cleanup_lock();
		}
		assert(fast_kmsg_zone->count == 0);
		assert(ipc_kmsg_queue_first(&dipc_kmsg_destroy_queue) ==
		       IKM_NULL);
		dipc_cleanup_unlock();
		splx(s);
	}

	printf("dtest_alloc_routines: done\n");
}

/*
 * RPC test routines
 */
dipc_return_t
dproc_test_intr_rpc(
	unsigned int	arg_in)
{
	if (arg_in == 11559)
		return DIPC_SUCCESS;
	else
		return DIPC_INVALID_TEST;
}

dipc_return_t
dproc_test_thread_rpc(
	unsigned int	arg_in,
	unsigned int	*arg_out)
{
	if (arg_in == 9158)
		*arg_out = 92989;
	else
		*arg_out = 112891;
	return DIPC_SUCCESS;
}

/*
 * Test the DIPC RPC service, using special test rpc routines in loopback
 * mode.
 */
void
dtest_rpc(void)
{
	dipc_return_t	dr;
	kkt_return_t	kktr;
	unsigned int	arg_out;

	kktr = drpc_test_intr_rpc(NODE_NAME_LOCAL, 11559, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);

	kktr = drpc_test_intr_rpc(NODE_NAME_LOCAL, 109, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_INVALID_TEST);

	kktr = drpc_test_thread_rpc(NODE_NAME_LOCAL, 9158, &dr, &arg_out);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	assert(arg_out == 92989);

	kktr = drpc_test_thread_rpc(NODE_NAME_LOCAL, 11559, &dr, &arg_out);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	assert(arg_out == 112891);

	printf("dtest_rpc: done\n");
}

/*
 * Test the message path from ipc_mqueue_deliver to ipc_mqueue_receive.
 * Must be run at startup time.
 */
void
dtest_mqdeliver_to_msgqueue(void)
{
	meta_kmsg_t	mkmsg;
	ipc_kmsg_t	kmsg, rcvmsg;
	ipc_mqueue_t	mq;
	ipc_kmsg_queue_t	kmsgq;
	mach_port_seqno_t	seqno1, seqno2;
	TR_DECL("dtest_mqdeliver_to_msgqueue");

	tr1("enter");
	ip_lock(dipc_test_port);
	mq = &dipc_test_port->ip_messages;
	kmsgq = &mq->imq_messages;
	ip_unlock(dipc_test_port);

	/* deliver a meta_kmsg */
	mkmsg = dipc_meta_kmsg_alloc(TRUE);
	assert(mkmsg != MKM_NULL);
	mkmsg->mkm_handle = HANDLE_DIPC_TEST;
	mkmsg->mkm_size = dipc_fast_kmsg_size + 20 - MACH_MSG_TRAILER_FORMAT_0_SIZE;
	mkmsg->mkm_remote_port = dipc_test_port;
	ipc_port_reference(dipc_test_port);
	imq_lock(mq);
	assert(ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	ip_lock(dipc_test_port);    /* lock consumed by ipc_mqueue_deliver */
	ipc_mqueue_deliver(dipc_test_port, (ipc_kmsg_t)mkmsg, TRUE);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	/* deliver a kmsg */
	kmsg = dipc_kmsg_alloc(dipc_fast_kmsg_size, dipc_kkt_transport_threads);
	assert(kmsg != IKM_NULL);
	kmsg->ikm_handle = HANDLE_DIPC_TEST;
	kmsg->ikm_header.msgh_remote_port = (mach_port_t)dipc_test_port;
	kmsg->ikm_header.msgh_local_port = MACH_PORT_NULL;
	kmsg->ikm_header.msgh_size = 0;
	kmsg->ikm_header.msgh_id = 987654321;
	ipc_port_reference(dipc_test_port);

	ip_lock(dipc_test_port);    /* lock consumed by ipc_mqueue_deliver */
	ipc_mqueue_deliver(dipc_test_port, kmsg, TRUE);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	/*
	 * get the first message.  This was a meta_kmsg but should be
	 * a real kmsg when we get it back.  make sure the meta_kmsg got
	 * freed.
	 */
	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno1);
	assert(rcvmsg != IKM_NULL);
	assert(!KMSG_IS_META(rcvmsg));
	assert(meta_kmsg_zone->count == 0);
	assert(rcvmsg->ikm_size == ikm_plus_overhead(dipc_fast_kmsg_size + 20));
	assert(rcvmsg->ikm_handle == HANDLE_DIPC_TEST);
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
								dipc_test_port);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	ipc_port_release(dipc_test_port);
	ikm_free(rcvmsg);

	/*
	 * get the second message, which was a kmsg all along.   Make
	 * sure it gets put back in the fast_kmsg_zone.
	 */
	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno2);
	assert(rcvmsg != IKM_NULL);
	assert(seqno2 == seqno1 + 1);
	assert(!KMSG_IS_META(rcvmsg));
	assert(rcvmsg == kmsg);
	assert(rcvmsg->ikm_handle == HANDLE_DIPC_TEST);
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
								dipc_test_port);
	assert(rcvmsg->ikm_header.msgh_local_port == MACH_PORT_NULL);
	assert(rcvmsg->ikm_header.msgh_size == 0);
	assert(rcvmsg->ikm_header.msgh_id == 987654321);
	imq_lock(mq);
	assert(ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	ipc_port_release(dipc_test_port);
	ikm_free(rcvmsg);

	assert(dipc_test_port->ip_references == 2);
	assert(dipc_kkt_transport_threads || fast_kmsg_zone->count == 0);
	printf("dtest_mqdeliver_to_msgqueue: done\n");
	tr1("exit");
}

/*
 * Test the message path from dipc_enqueue_message to ipc_mqueue_receive.
 * Must be run at startup time.
 */
void
dtest_enqueue_to_msgqueue(void)
{
	ipc_kmsg_t	kmsg, kmsg2, rcvmsg, kmsg_tmp;
	meta_kmsg_t	mkmsg;
	dipc_return_t	dr;
	boolean_t	free_handle, kern_port;
	ipc_mqueue_t	mq;
	ipc_kmsg_queue_t	kmsgq;
	mach_port_seqno_t	seqno;
	mach_port_msgcount_t	saved_qlimit;
	TR_DECL("dtest_enqueue_to_msgqueue");

	tr1("enter");
	ip_lock(dipc_test_port);
	mq = &dipc_test_port->ip_messages;
	kmsgq = &mq->imq_messages;
	saved_qlimit = dipc_test_port->ip_qlimit;
	dipc_test_port->ip_qlimit = 3;
	ip_unlock(dipc_test_port);

	/* enqueue a kmsg (!COMPLEX_OOL) */
	kmsg = dipc_kmsg_alloc(dipc_fast_kmsg_size, dipc_kkt_transport_threads);
	assert(kmsg != IKM_NULL);
	kmsg->ikm_handle = HANDLE_DIPC_TEST;
	kmsg->ikm_header.msgh_remote_port = (mach_port_t)dipc_test_port;
	kmsg->ikm_header.msgh_local_port = MACH_PORT_NULL;
	kmsg->ikm_header.msgh_size = 0;
	kmsg->ikm_header.msgh_id = 987654321;
	kmsg->ikm_header.msgh_bits = 0;
	KMSG_MARK_RECEIVING(kmsg);
	KMSG_MARK_HANDLE(kmsg);
	ipc_port_reference(dipc_test_port);
	imq_lock(mq);
	assert(ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	dr = dipc_enqueue_message((meta_kmsg_t)kmsg, &free_handle, &kern_port);
	assert(dr == DIPC_SUCCESS);
	assert(free_handle == TRUE);
	KMSG_CLEAR_HANDLE(kmsg);	/* pretend to free handle */
	assert(kmsg->ikm_handle == (handle_t)NODE_NAME_LOCAL);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	/* enqueue a meta_kmsg */
	mkmsg = dipc_meta_kmsg_alloc(TRUE);
	assert(mkmsg != MKM_NULL);
	mkmsg->mkm_handle = HANDLE_DIPC_TEST;
	mkmsg->mkm_size = dipc_fast_kmsg_size + 20 - MACH_MSG_TRAILER_FORMAT_0_SIZE;
	mkmsg->mkm_remote_port = dipc_test_port;
	kmsg_tmp = (ipc_kmsg_t) mkmsg;
	KMSG_MARK_RECEIVING(kmsg_tmp);
	KMSG_MARK_HANDLE(kmsg_tmp);
	ipc_port_reference(dipc_test_port);

	dr = dipc_enqueue_message(mkmsg, &free_handle, &kern_port);
	assert(dr == DIPC_SUCCESS);
	assert(free_handle == FALSE);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	/* enqueue a kmsg with COMPLEX_OOL set */
	kmsg2 = dipc_kmsg_alloc(dipc_fast_kmsg_size,dipc_kkt_transport_threads);
	assert(kmsg2 != IKM_NULL);
	kmsg2->ikm_handle = HANDLE_DIPC_TEST;
	kmsg2->ikm_header.msgh_remote_port = (mach_port_t)dipc_test_port;
	kmsg2->ikm_header.msgh_local_port = MACH_PORT_NULL;
	kmsg2->ikm_header.msgh_size = 0;
	kmsg2->ikm_header.msgh_id = 123456789;
	kmsg2->ikm_header.msgh_bits = MACH_MSGH_BITS_COMPLEX_OOL;
	KMSG_MARK_RECEIVING(kmsg2);
	KMSG_MARK_HANDLE(kmsg2);
	ipc_port_reference(dipc_test_port);

	dr = dipc_enqueue_message((meta_kmsg_t)kmsg2, &free_handle, &kern_port);
	assert(dr == DIPC_SUCCESS);
	assert(free_handle == FALSE);
	assert(kmsg2->ikm_handle == HANDLE_DIPC_TEST);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

#if 0
	/*
	 * try to enqueue when the message queue is full.  The return
	 * code should be DIPC_QUEUE_FULL, the message should have been
	 * freed and the port reference count decremented.
	 */
	assert(meta_kmsg_zone->count == 1);
	mkmsg = dipc_meta_kmsg_alloc(TRUE);
	assert(mkmsg != MKM_NULL);
	assert(meta_kmsg_zone->count == 2);
	mkmsg->mkm_handle = HANDLE_DIPC_TEST;
	mkmsg->mkm_size = dipc_fast_kmsg_size + 40;
	mkmsg->mkm_remote_port = dipc_test_port;
	kmsg_tmp = (ipc_kmsg_t) mkmsg;
	KMSG_MARK_RECEIVING(kmsg_tmp);
	KMSG_MARK_HANDLE(kmsg_tmp);
	ipc_port_reference(dipc_test_port);

	dr = dipc_enqueue_message(mkmsg, &free_handle, &kern_port);
	assert(dr == DIPC_QUEUE_FULL);
	assert(free_handle == TRUE);
	assert(meta_kmsg_zone->count == 1);
#endif

	/* receive the first message, which was an inline-only kmsg */
	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno);
	assert(rcvmsg != IKM_NULL);
	assert(!KMSG_IS_META(rcvmsg));
	assert(rcvmsg == kmsg);
	assert(rcvmsg->ikm_handle == (handle_t)NODE_NAME_LOCAL);
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
								dipc_test_port);
	assert(rcvmsg->ikm_header.msgh_local_port == MACH_PORT_NULL);
	assert(rcvmsg->ikm_header.msgh_size == 0);
	assert(rcvmsg->ikm_header.msgh_id == 987654321);
	assert(KMSG_RECEIVING(rcvmsg));
	assert(!KMSG_HAS_HANDLE(rcvmsg));
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	ipc_port_release(dipc_test_port);
	ikm_free(rcvmsg);
	assert(dipc_kkt_transport_threads || fast_kmsg_zone->count == 1);

	/*
	 * receive the second message, which was a meta_kmsg but should be
	 * a real kmsg when we get it back.
	 */
	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno);
	assert(rcvmsg != IKM_NULL);
	assert(!KMSG_IS_META(rcvmsg));
	assert(meta_kmsg_zone->count == 0);
	assert(rcvmsg->ikm_size == ikm_plus_overhead(dipc_fast_kmsg_size + 20));
	assert(rcvmsg->ikm_handle == HANDLE_DIPC_TEST);
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
								dipc_test_port);
	assert(KMSG_RECEIVING(rcvmsg));
	assert(KMSG_HAS_HANDLE(rcvmsg));
	ikm_free(rcvmsg);
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	ipc_port_release(dipc_test_port);

	/* receive the third message, which was a COMPLEX_OOL kmsg */
	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno);
	assert(rcvmsg != IKM_NULL);
	assert(!KMSG_IS_META(rcvmsg));
	assert(rcvmsg == kmsg2);
	assert(rcvmsg->ikm_handle == HANDLE_DIPC_TEST);
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
								dipc_test_port);
	assert(rcvmsg->ikm_header.msgh_local_port == MACH_PORT_NULL);
	assert(rcvmsg->ikm_header.msgh_size == 0);
	assert(rcvmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL);
	assert(rcvmsg->ikm_header.msgh_id == 123456789);
	assert(KMSG_RECEIVING(rcvmsg));
	assert(KMSG_HAS_HANDLE(rcvmsg));
	imq_lock(mq);
	assert(ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	ipc_port_release(dipc_test_port);
	ikm_free(rcvmsg);
	assert(dipc_kkt_transport_threads || fast_kmsg_zone->count == 0);

	/* check final conditions */
	ip_lock(dipc_test_port);
	assert(dipc_test_port->ip_references == 2);
	dipc_test_port->ip_qlimit = saved_qlimit;
	ip_unlock(dipc_test_port);
	printf("dtest_enqueue_to_msgqueue: done\n");
	tr1("exit");
}

#define MSG1_SIZE	48
#define MSG2_SIZE	dipc_fast_kmsg_size
#define MSG3_SIZE	52
#define MSG4_SIZE	56
#define MSG5_SIZE	60

void
do_deliver(
	void 		*ep0,
	void 		*ep1,
	vm_size_t	size,
	unsigned int	intr_control,
	kern_return_t	cr1,			/* expected return code */
	kern_return_t	cr2)			/* expected return code */
{
	spl_t		s;

	transp->endpoint.ep[0] = ep0;
	transp->endpoint.ep[1] = ep1;
	transp->size = size - MACH_MSG_TRAILER_FORMAT_0_SIZE;
	trans_msghp->msgh_size = transp->size;
	trans_msghp->msgh_bits = MACH_MSGH_BITS_COMPLEX_OOL;
	dtest_cr1 = -1;
	dtest_intr_control = intr_control;	/* cause interrupt */
	s = splkkt();
	dipc_test_lock();
	while (dtest_cr1 == -1) {
		assert_wait((event_t)&transp, FALSE);
		dipc_test_unlock();
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splkkt();
		dipc_test_lock();
	}
	dipc_test_unlock();
	splx(s);
	if (cr1 != dtest_cr1) {
		printf("do_deliver:  cr1 0x%x dtest_cr1 0x%x\n", cr1,dtest_cr1);
		assert(cr1 == dtest_cr1);
	}
	if (cr2 != dtest_cr2) {
		printf("do_deliver:  cr2 0x%x dtest_cr2 0x%x\n", cr2,dtest_cr2);
		assert(cr2 == dtest_cr2);
	}
}

void
dtest_connect_reply(
	handle_t	handle,
	kern_return_t	ret1,
	kern_return_t	ret2)
{
	spl_t		s;

	assert(handle == HANDLE_DIPC_TEST);
	s = splkkt();
	dipc_test_lock();
	dtest_cr1 = ret1;
	dtest_cr2 = ret2;
	dipc_test_unlock();
	splx(s);
	thread_wakeup((event_t)&transp);
}

static void
do_receive(
	ipc_mqueue_t	mq,
	vm_size_t	size)
{
	ipc_kmsg_t	rcvmsg;
	mach_port_seqno_t	seqno;

	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno);
	assert(rcvmsg != IKM_NULL);
	assert(!KMSG_IS_META(rcvmsg));
#if	0				/* alanl XXX */
	assert(dipc_kkt_transport_threads==TRUE ||rcvmsg->ikm_size==IKM_SIZE_INTR_KMSG);
	assert(dipc_kkt_transport_threads==FALSE||rcvmsg->ikm_size==dipc_fast_kmsg_size);
#endif
	assert(rcvmsg->ikm_handle == HANDLE_DIPC_TEST);
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
								dipc_test_port);
	ipc_port_release(dipc_test_port);
	ikm_free(rcvmsg);
}

/*
 * Test the message path from dipc_deliver to ipc_mqueue_receive.
 * Must be run at startup time.
 */
void
dtest_deliver_to_msgqueue(void)
{
	ipc_mqueue_t	mq;
	ipc_kmsg_queue_t	kmsgq;
	mach_port_msgcount_t	saved_qlimit;
	TR_DECL("dtest_deliver_to_msgqueue");

#if	1
	printf("dtest_deliver_to_msgqueue:  this test is way, way broken\n");
#else	/* 1 */
	tr1("enter");
	ip_lock(dipc_test_port);
	mq = &dipc_test_port->ip_messages;
	kmsgq = &mq->imq_messages;
	saved_qlimit = dipc_test_port->ip_qlimit;
	dipc_test_port->ip_qlimit = 5;
	ip_unlock(dipc_test_port);

	/* check initial conditions */
	imq_lock(mq);
	assert(ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	assert(dipc_kkt_transport_threads || fast_kmsg_zone->count == 0);
	assert(meta_kmsg_zone->count == 0);

	/* set up for an interrupt for each test message deliver */

	trans_msghp = (mach_msg_header_t *)&transp->buff;
	trans_msghp->msgh_bits = 0;
	trans_msghp->msgh_remote_port = MACH_PORT_NULL;
	trans_msghp->msgh_local_port = MACH_PORT_NULL;

	/*
	 * (1) kmsg w/ inline data, fits in INTR_KMSG
	 * Queued: intr_kmsg	Received: intr_kmsg
	 */
	do_deliver(dtest_port_origin, dtest_port_ident, MSG1_SIZE,
		DTEST_DELIVER_INLINE, DIPC_DATA_SENT, DIPC_SUCCESS);

	/*
	 * (2) kmsg w/ inline data, does not fit in INTR_KMSG
	 * Queued: meta_kmsg	Received: kmsg
	 */
	do_deliver(dtest_port_origin, dtest_port_ident, MSG2_SIZE,
		DTEST_DELIVER_INLINE, DIPC_DATA_DROPPED, DIPC_SUCCESS);

	/*
	 * (3) kmsg w/o inline data
	 * Queued: meta_kmsg	Received: kmsg
	 */
	do_deliver(dtest_port_origin, dtest_port_ident, MSG3_SIZE,
		DTEST_DELIVER, DIPC_SUCCESS, DIPC_SUCCESS);

	/*
	 * (4) kmsg w/ inline data, fits in INTR_KMSG, but no intr_kmsgs
	 * available
	 * Queued: meta_kmsg	Received: kmsg
	 */
	/* make the zone look like it's empty */
	if (dipc_kkt_transport_threads) {
		save_free_elements = fast_kmsg_zone->free_elements;
		fast_kmsg_zone->free_elements = 0;
	}
	do_deliver(dtest_port_origin, dtest_port_ident, MSG4_SIZE,
		DTEST_DELIVER_INLINE, DIPC_DATA_DROPPED, DIPC_SUCCESS);

	/* fix the zone up agagin */
	if (dipc_kkt_transport_threads)
	    fast_kmsg_zone->free_elements = save_free_elements;


	/*
	 * (5) kmsg w/inline data, fits in INTR_KMSG, no intr_kmsgs available
	 * at intr time but one can be obtained by thread
	 * XXX this will only work right on a uniprocessor...
	 * Queued: intr_kmsg	Received: intr_kmsg
	 */
	/* make the zone look like it's empty */
	if (dipc_kkt_transport_threads) {
		save_free_elements = fast_kmsg_zone->free_elements;
		fast_kmsg_zone->free_elements = 0;
	}
	/* this time free_elements will be restored by the interrupt routine. */

	do_deliver(dtest_port_origin, dtest_port_ident, MSG5_SIZE,
		DTEST_DELIVER_RESTORE_ZONE, DIPC_DATA_SENT, DIPC_SUCCESS);

#if 0
	/*
	 * (6) kmsg w/o inline data, but QUEUE_FULL
	 * Queued: nothing	Received: nothing
	 * DIPC_QUEUE_FULL, message destroyed
	 */
	do_deliver(dtest_port_origin, dtest_port_ident, MSG3_SIZE,
		DTEST_DELIVER, DIPC_QUEUE_FULL, DIPC_SUCCESS);
	assert(dipc_test_port->ip_qlimit == 5);
	transp->endpoint.ep[0] = (void *)dipc_test_port->ip_uid.origin;
	transp->endpoint.ep[1] = (void *)dipc_test_port->ip_uid.identifier;
	transp->size = MSG3_SIZE;
	trans_msghp->msgh_size = MSG3_SIZE -
						MACH_MSG_TRAILER_FORMAT_0_SIZE;
	dtest_intr_control = DTEST_DELIVER;		/* cause interrupt */
#endif

	/*
	 * (7) kmsg w/o inline data, but no meta_kmsgs
	 * DIPC_RESOURCE_SHORTAGE, message destroyed
	 */
	/* make the zone look like it's empty */
	save_free_elements = meta_kmsg_zone->free_elements;
	meta_kmsg_zone->free_elements = 0;
	do_deliver(dtest_port_origin, dtest_port_ident, MSG3_SIZE,
		DTEST_DELIVER, DIPC_RESOURCE_SHORTAGE, DIPC_SUCCESS);

	/* fix the zone up agagin */
	meta_kmsg_zone->free_elements = save_free_elements;

	/*
	 * (8) kmsg w/ port NOT in uid table
	 * DIPC_NO_UID, message destroyed
	 */
	dtest_intr_control = DTEST_DELIVER;		/* cause interrupt */
	do_deliver((void *)dipc_node_self(), (void *)0xfeedface, MSG3_SIZE,
		DTEST_DELIVER, DIPC_NO_UID, DIPC_SUCCESS);

	/*
	 * (9) kmsg w/ port in uid table but not active
	 * DIPC_PORT_DEAD, message destroyed
	 */
	do_deliver((void *)dipc_inactive_port->ip_uid.origin,
		(void *)dipc_inactive_port->ip_uid.identifier,
		MSG3_SIZE, DTEST_DELIVER, DIPC_PORT_DEAD,
		DIPC_SUCCESS);

	/*
	 * there should now be 7 refs on the port: the original 2,
	 * and one for each of the 5 messages on the queue.
	 */
	ip_lock(dipc_test_port);
	assert(dipc_test_port->ip_references == 7 ||
	       (dipc_test_port->ip_references + 
	       dipc_test_port->ip_ref_count== 7));
	ip_unlock(dipc_test_port);

	/*
	 * receive the messages on the queue.  Messages 1 through 5 above
	 * should be there.
	 */
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive(mq, dipc_fast_kmsg_size);

	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive(mq, ikm_plus_overhead(MSG2_SIZE));

	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive(mq, ikm_plus_overhead(MSG3_SIZE));

	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive(mq, ikm_plus_overhead(MSG4_SIZE));

	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive(mq, dipc_fast_kmsg_size);

	/* check final conditions */
	imq_lock(mq);
	assert(ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);

	assert(dipc_kkt_transport_threads || fast_kmsg_zone->count == 0);
	assert(meta_kmsg_zone->count == 0);
	ip_lock(dipc_test_port);
	assert(dipc_test_port->ip_references == 2);
	dipc_test_port->ip_qlimit = saved_qlimit;
	ip_unlock(dipc_test_port);

	printf("dtest_deliver_to_msgqueue: done\n");
	tr1("exit");
#endif	/* 1 */
}


static void
do_receive_and_verify(
	ipc_mqueue_t		mq,
	vm_size_t		size,
	mach_msg_header_t	*msg_sent)
{
	ipc_kmsg_t		rcvmsg;
	mach_port_seqno_t	seqno;
	char			*sent_data, *rcv_data;
	int			i;
	void			dtest_pull_ool_data(ipc_kmsg_t);
	TR_DECL("do_receive_and_verify");

	imq_lock(mq);		/* lock consumed by ipc_mqueue_receive */
	ipc_mqueue_receive(mq, MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE, &rcvmsg,
		&seqno);
	assert(rcvmsg != IKM_NULL);
	assert(!KMSG_IS_META(rcvmsg));
	assert((ipc_port_t)rcvmsg->ikm_header.msgh_remote_port ==
							dipc_test_port);
	if (rcvmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL) {
		assert(rcvmsg->ikm_handle != HANDLE_NULL);
		dtest_pull_ool_data(rcvmsg);
		do_verify_ool(&rcvmsg->ikm_header, msg_sent);
	} else {
		tr5("rcvmsg 0x%x msg_sent 0x%x mq 0x%x &imq_msg 0x%x",
		    rcvmsg, msg_sent, mq, &mq->imq_messages);
		assert(rcvmsg->ikm_handle != (handle_t) NODE_NAME_NULL);
		assert(msg_sent->msgh_size == rcvmsg->ikm_header.msgh_size);
		assert(msg_sent->msgh_id == rcvmsg->ikm_header.msgh_id);
		sent_data = (char *)msg_sent;
		rcv_data = (char *)&rcvmsg->ikm_header;
		for (i = (sizeof(mach_msg_header_t));
		     i < rcvmsg->ikm_header.msgh_size;
		     i++) {
			if (sent_data[i] != rcv_data[i]) {
				printf("data mismatch: sent kmsg 0x%x, rcv kmsg 0x%x, i=%d\n",
				       msg_sent, rcvmsg, i);
				assert(FALSE);
			}
		}
	}

	ipc_port_release(dipc_test_port);
	ikm_free(rcvmsg);
}

/*
 * Verify the sent OOL data - this only works in loopback mode
 */
void
do_verify_ool(
	mach_msg_header_t	*msg_recv,
	mach_msg_header_t	*msg_sent)
{
	mach_msg_body_t		*rbody, *sbody;
	mach_msg_descriptor_t	*rdes, *sdes;
	vm_map_copy_t		rcopy, scopy;

	char *sent_data, *rcv_data;
	int i;

	rbody = (mach_msg_body_t *)(msg_recv + 1);
	sbody = (mach_msg_body_t *)(msg_sent + 1);
	assert(rbody->msgh_descriptor_count == sbody->msgh_descriptor_count);
	rdes = (mach_msg_descriptor_t *)(rbody + 1);
	sdes = (mach_msg_descriptor_t *)(sbody + 1);

	/*
	 * verify the OOL bits
	 */
	for(i=0;i<rbody->msgh_descriptor_count;i++) {
		switch (rdes->type.type & ~DIPC_TYPE_BITS) {
		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR:
			assert((sdes->type.type & ~DIPC_TYPE_BITS) == 
			       MACH_MSG_OOL_DESCRIPTOR);
			assert(rdes->out_of_line.size == 
			       sdes->out_of_line.size);
			assert(rdes->out_of_line.copy ==
			       sdes->out_of_line.copy);
			rcopy = (vm_map_copy_t)rdes->out_of_line.address;
			scopy = (vm_map_copy_t)sdes->out_of_line.address;
#if	0
			/*
			 *	Can't do this!  The sent message is
			 *	likely deallocated at this point,
			 *	along with its copy objects and possibly
			 *	the data involved as well.  Think again...
			 */
			if (rcopy->type != scopy->type) {
				printf("rcopy type %d != scopy type %d\n",
				       rcopy->type, scopy->type);
				printf("rcv_msg = 0x%x snd_msg = 0x%x\n",
				       msg_recv, msg_sent);
				assert(FALSE);
			}
#endif	/* 0 */
			assert(rcopy->size == scopy->size);
			/*
			 * need to check the OOL data here
			 */
			break;
		    default:
			break;
		}
	}
}


void
dtest_pull_ool_data(
	ipc_kmsg_t	kmsg)
{
	msg_progress_t mp;
	int i;
	dipc_return_t			dr;
	mach_msg_ool_descriptor_t *dsc;
	vm_map_copy_t vmc;
	dipc_copy_list_t		dclp, *dclpp;
	/* 
	 * This is an easier way to view the kmsg
	 */
	struct foo {
		mach_msg_header_t h;
		mach_msg_body_t b;
		mach_msg_descriptor_t d[1];
	} *msg = (struct foo *)&kmsg->ikm_header;
	vm_object_t obj = VM_OBJECT_NULL;
	vm_offset_t offset;

	mp = msg_progress_allocate(TRUE);
	mp->kmsg = kmsg;
	dclpp = &mp->copy_list;
	KMSG_MSG_PROG_RECV_SET(kmsg, mp);

	for(i=0;i<msg->b.msgh_descriptor_count;i++) {
		switch (msg->d[i].type.type & ~DIPC_TYPE_BITS) {
		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR:

			/*
			 * This is not all that hard either.  For
			 * now we will use entry list copy_ts
			 * and a single object for all ool regions.
			 * In the future, we should use KERNEL_BUFFER
			 * copy_ts for small ool regions.  We will
			 * have to keep track of the total kalloc
			 * consumption for this message when we
			 * do that
			 */

			dsc = &msg->d[i].out_of_line;
			if (obj == VM_OBJECT_NULL) {
				/*
				 * We only need one of these
				 */
				obj = vm_object_allocate(
						round_page(dsc->size));
				assert(obj != VM_OBJECT_NULL);
				offset = 0;
			} else {
				obj->size += round_page(dsc->size);
			}
			vmc = (vm_map_copy_t)
				vm_map_entry_list_from_object(
					obj, offset, dsc->size,
					current_task()->map->hdr.entries_pageable);
			assert(vmc != VM_MAP_COPY_NULL);

			offset += round_page(dsc->size);

			/*
			 * Note that we have not supported matching
			 * send side and receive side alignment of
			 * ool data
			 */

			dclp = *dclpp = dipc_copy_list_allocate(TRUE);
			assert(dclp != (dipc_copy_list_t)0);
			dclpp = &dclp->next;

			dclp->next = DIPC_COPY_LIST_NULL;
			dsc->type &= ~DIPC_TYPE_BITS;
			dsc->address = (void *)vmc;
			dclp->copy = vmc;
			break;
		    default:
			/* This test is not interested in other types */
			break;
		}
	}
	mp->msg_size = offset;
	dr = dipc_receive_ool(mp);
	assert(dr == DIPC_SUCCESS);
	msg_progress_deallocate(mp);
	kmsg->ikm_header.msgh_bits &= ~MACH_MSGH_DIPC_BITS;
}


/*
 * Test the message path from dipc_mqueue_send to ipc_mqueue_receive using the
 * loopback node.
 * Must be run at startup time.
 */
void
dtest_send_to_msgqueue(void)
{
	char		   msgbuf[400];
	mach_msg_header_t  *msghp = (mach_msg_header_t *)msgbuf;
	int		   *datap = (int *)(msgbuf + sizeof(mach_msg_header_t));
	ipc_kmsg_t	   kmsg1;
	mach_msg_return_t  mr;
	ipc_mqueue_t	   mq = &dipc_test_port->ip_messages;
	ipc_kmsg_queue_t   kmsgq = &mq->imq_messages;
	TR_DECL("dtest_send_to_msgqueue");

	tr1("enter");
	/* common to all messages */
	msghp->msgh_remote_port = (mach_port_t)dipc_test_port_proxy;
	msghp->msgh_local_port = MACH_PORT_NULL;

	/* send small inline-only message */
	msghp->msgh_bits = MACH_MSGH_BITS_DIPC_FORMAT |
	    MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0);
	msghp->msgh_id = 0x1122;
	msghp->msgh_size = sizeof(mach_msg_header_t) + 4;
	*datap = 0x11111111;
	mr = ipc_kmsg_get_from_kernel(msghp, msghp->msgh_size, &kmsg1);
	assert(mr == MACH_MSG_SUCCESS);
	assert(kmsg1 != IKM_NULL);
	KMSG_MSG_PROG_SEND_SET(kmsg1, MSG_PROGRESS_NULL);

	ip_lock(dipc_test_port_proxy);
	/*
	 *	Message post-processing disposes of a reference
	 *	on the destination port; this reference normally
	 *	would have been acquired by dipc_kmsg_copyin.
	 */
	ip_reference(dipc_test_port_proxy);
	mr = dipc_mqueue_send(kmsg1, 0, 0);
	assert(mr == MACH_MSG_SUCCESS);

	/* receive messages */
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive_and_verify(mq,
		msghp->msgh_size + IKM_OVERHEAD + MAX_TRAILER_SIZE, msghp);

	printf("dtest_send_to_msgqueue: done\n");
	tr1("exit");
}

/*
 *	Test the ool message path from dipc_mqueue_send to 
 *	ipc_mqueue_receive using the loopback node.
 *	Must be run at startup time.
 */

int ool_send_pages_msgqueue;	/* Initialized to CHAINSIZE */

void
dtest_send_ool_to_msgqueue(void)
{
#if	!NO_MACH_ASSERT
	ipc_kmsg_t	   kmsg1;
	unsigned int	   *send_page;
	vm_map_copy_t	   copy;
	dipc_copy_list_t   dcopy;
	msg_progress_t	   mp;
	mach_msg_return_t  mr;
	ipc_mqueue_t	   mq = &dipc_test_port->ip_messages;
	ipc_kmsg_queue_t   kmsgq = &mq->imq_messages;
	mach_msg_format_0_trailer_t	*trailer;
	struct {
		mach_msg_header_t	head;
		mach_msg_body_t		msgh_body;
		mach_msg_ool_descriptor_t data;
	} Send_msg;
	TR_DECL("dtest_send_ool_to_msgqueue");

	tr1("enter");
	/* common to all messages */
	Send_msg.head.msgh_remote_port = (mach_port_t)dipc_test_port_proxy;
	Send_msg.head.msgh_local_port = MACH_PORT_NULL;

	/* send ool message */
	Send_msg.head.msgh_bits = MACH_MSGH_BITS_DIPC_FORMAT |
		MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0) |
		MACH_MSGH_BITS_COMPLEX_OOL | MACH_MSGH_BITS_COMPLEX;
	Send_msg.head.msgh_id = 0x1122;
	Send_msg.head.msgh_size = sizeof(Send_msg);
	Send_msg.msgh_body.msgh_descriptor_count = 1;

	/*
	 * Get an ENTRY_LIST flavor copy object.  The memory
	 * allocated here will need to be released!
	 */
#define	SEND_SIZE	(PAGE_SIZE * ool_send_pages_msgqueue)
	kmem_alloc_pageable(kernel_map, (vm_offset_t *)&send_page,
			    SEND_SIZE);
	*send_page = 0x11112222, *(send_page+1) = 33334444;
	vm_map_copyin(kernel_map, (vm_offset_t) send_page,
		      (vm_size_t) SEND_SIZE, FALSE, &copy);
        Send_msg.data.address = (void *)(copy);
        Send_msg.data.size = SEND_SIZE;
        Send_msg.data.deallocate = FALSE;
        Send_msg.data.copy = MACH_MSG_VIRTUAL_COPY;
        Send_msg.data.type = MACH_MSG_OOL_DESCRIPTOR;


	mr = ipc_kmsg_get_from_kernel(&Send_msg.head,
				      Send_msg.head.msgh_size,
				      &kmsg1);
	assert(mr == MACH_MSG_SUCCESS);
	assert(kmsg1 != IKM_NULL);

	/*
	 * allocate & setup the msg_progress structure
	 */
	mp = msg_progress_allocate(TRUE);
	assert(mp);
	mp->kmsg = kmsg1;
	dcopy = dipc_copy_list_allocate(TRUE);
	assert(dcopy);
	dcopy->next = DIPC_COPY_LIST_NULL;
	dcopy->copy = copy;
	mp->copy_list = dcopy;
	mp->msg_size = SEND_SIZE;
	trailer = KMSG_DIPC_TRAILER(kmsg1);
	trailer->msgh_seqno = (mach_port_seqno_t)mp;

	ip_lock(dipc_test_port_proxy);
	/*
	 *	Message post-processing disposes of a reference
	 *	on the destination port; this reference normally
	 *	would have been acquired by dipc_kmsg_copyin.
	 */
	ip_reference(dipc_test_port_proxy);
	mr = dipc_mqueue_send(kmsg1, 0, 0);
	assert(mr == MACH_MSG_SUCCESS);

	/* 
	 * The message is now sent, so get it back
	 */
	imq_lock(mq);
	assert(!ipc_kmsg_queue_empty(kmsgq));
	imq_unlock(mq);
	do_receive_and_verify(mq,
		Send_msg.head.msgh_size + IKM_OVERHEAD + MAX_TRAILER_SIZE,
		&Send_msg.head);

	printf("dtest_send_ool_to_msgqueue: done\n");
#undef	SEND_SIZE
	tr1("exit");
#endif	/* !NO_MACH_ASSERT */
}

void
dipc_test_intr(void)
{
	unsigned int control = dtest_intr_control;

	dtest_intr_control = 0;

#if	1
	panic("dipc_test_intr");
#else
	switch (control) {
	case DTEST_DELIVER:
		dipc_deliver(CHAN_USER_KMSG, HANDLE_DIPC_TEST, transp,
			     FALSE, FALSE);
		break;

	case DTEST_DELIVER_INLINE:
		dipc_deliver(CHAN_USER_KMSG, HANDLE_DIPC_TEST, transp,
			     TRUE, FALSE);
		break;

	case DTEST_DELIVER_RESTORE_ZONE:
		/* fix the zone up again */
		if (dipc_kkt_transport_threads)
		    fast_kmsg_zone->free_elements = save_free_elements;

		dipc_deliver(CHAN_USER_KMSG, HANDLE_DIPC_TEST, transp,
			     TRUE, FALSE);
		break;
	}
#endif
}

void
dtest_port_copyinout(void)
{
	ipc_port_t port;
	dipc_return_t dr;
	uid_t uid, uid2;

	/* Create send right */

	DIPC_UID_MAKE(&uid, NODE_NAME_LOCAL, 666);
	dr = dipc_port_copyout(&uid,
			       MACH_MSG_TYPE_PORT_SEND, 
			       dipc_node_self(),
			       FALSE, /*destination*/
			       TRUE,  /*NMS*/
			       &port);
	assert(dr == DIPC_SUCCESS);
	ip_lock(port);
	ip_reference(port);
	assert(DIPC_IS_DIPC_PORT(port));
	assert(DIPC_IS_PROXY(port));
	assert(port->ip_srights == 1);
	assert(!port->ip_special);
	assert(!port->ip_forward);
	assert(DIPC_UID_EQUAL(&port->ip_uid, &uid));
	assert(port->ip_dest_node == NODE_NAME_LOCAL);
	assert(port->ip_transit == 1);
	assert(port->ip_sorights == 0);
	assert(port->ip_references == 2);

	ip_unlock(port);

	/* Consume the sright */

	dr = dipc_port_copyin(port, 
			      MACH_MSG_TYPE_PORT_SEND,
			      &uid2);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_UID_EQUAL(&uid, &uid2));
	assert(port->ip_srights == 0);
	assert(port->ip_transit == 0);

	ip_lock(port);
	ip_release(port);
	assert(port->ip_references == 0);
	ip_check_unlock(port);
	assert(dipc_uid_lookup(&uid, &port) == DIPC_NO_UID);

	/* Create send once right */

	DIPC_UID_MAKE(&uid, NODE_NAME_LOCAL, 666);
	dr = dipc_port_copyout(&uid,
			       MACH_MSG_TYPE_PORT_SEND_ONCE,
			       dipc_node_self(),
			       FALSE, /*destination*/
			       TRUE,  /*NMS*/
			       &port);
	assert(dr == DIPC_SUCCESS);
	ip_lock(port);
	ip_reference(port);
	assert(DIPC_IS_DIPC_PORT(port));
	assert(DIPC_IS_PROXY(port));
	assert(port->ip_sorights == 1);
	assert(!port->ip_special);
	assert(!port->ip_forward);
	assert(DIPC_UID_EQUAL(&port->ip_uid, &uid));
	assert(port->ip_dest_node == uid.origin);
	assert(port->ip_transit == 0);
	assert(port->ip_srights == 0);
	assert(port->ip_references == 2);
	ip_unlock(port);

	/* Consume the soright */

	dr = dipc_port_copyin(port, 
			      MACH_MSG_TYPE_PORT_SEND_ONCE,
			      &uid2);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_UID_EQUAL(&uid, &uid2));
	assert(port->ip_sorights == 0);

	ip_lock(port);
	ip_release(port);
	assert(port->ip_references == 0);
	ip_check_unlock(port);
	assert(dipc_uid_lookup(&uid, &port) == DIPC_NO_UID);
	
	printf("dtest_port_copyinout: done\n");
}

void
dtest_copyin(void)
{
	struct {
		mach_msg_header_t	h;
		mach_msg_body_t		b;
		mach_msg_descriptor_t	d1;
		mach_msg_descriptor_t	d2;
		mach_msg_descriptor_t	d3;
	} msg;
	mach_msg_header_t *h = &msg.h;
	mach_msg_body_t *b = &msg.b;
	mach_msg_port_descriptor_t *d1 = &msg.d1.port;
	mach_msg_ool_descriptor_t *d2 = &msg.d2.out_of_line;
	mach_msg_ool_ports_descriptor_t *d3 = &msg.d3.ool_ports;
	dipc_return_t dr;
	uid_t uid;
	vm_map_copy_t vmc;
	kern_return_t kr;
	ipc_port_t *ool_ports;


	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;
	ipc_port_t local, remote, inlinep, outline;

	local = dtest_port_alloc_sright(TRUE, SO_RIGHT);
	h->msgh_local_port = (mach_port_t)local;

	inlinep = dtest_port_alloc_sright(TRUE, SEND_RIGHT);
	d1->name = (mach_port_t)inlinep;
	d1->disposition = MACH_MSG_TYPE_PORT_SEND;
	d1->type = MACH_MSG_PORT_DESCRIPTOR;
	
	DIPC_UID_MAKE(&uid, NODE_NAME_LOCAL, 666);
	dr = dipc_port_copyout(&uid,
			       MACH_MSG_TYPE_PORT_SEND_ONCE,
			       dipc_node_self(),
			       FALSE, /*destination*/
			       TRUE,  /*NMS*/
			       &remote);
	assert(dr == DIPC_SUCCESS);
	assert(remote != IP_NULL);
	ip_reference(remote);		/* XXX */
	assert(remote->ip_references == 2);
	assert(DIPC_IS_PROXY(remote));
	h->msgh_remote_port = (mach_port_t)remote;

	vmc = (vm_map_copy_t) kalloc(8192 + sizeof(struct vm_map_copy));
	assert(vmc != VM_MAP_COPY_NULL);
	vmc->type = VM_MAP_COPY_KERNEL_BUFFER;
	vmc->offset = 0;
	vmc->size = 8192;
	vmc->cpy_kdata = (vm_offset_t) (vmc+1);
	vmc->cpy_kalloc_size = 8192 + sizeof(struct vm_map_copy);

	d2->address = (void *)vmc;
	d2->size = 8192;
	d2->deallocate = FALSE;
	d2->copy = MACH_MSG_KALLOC_COPY_T;
	d2->type = MACH_MSG_OOL_DESCRIPTOR;

	outline = dtest_port_alloc_sright(TRUE, SEND_RIGHT);
	ool_ports = (ipc_port_t *)kalloc(sizeof(ipc_port_t));
	assert(ool_ports != (ipc_port_t *)0);
	*ool_ports  = outline;
	d3->address = (void *)ool_ports;
	d3->count = 1;
	d3->deallocate = FALSE;
	d3->disposition = MACH_MSG_TYPE_PORT_SEND;
	d3->type = MACH_MSG_OOL_PORTS_DESCRIPTOR;

	h->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE,
				      MACH_MSG_TYPE_PORT_SEND_ONCE) |
		       MACH_MSGH_BITS_COMPLEX;

	h->msgh_size = sizeof(msg);
	b->msgh_descriptor_count = 3;

	mr = ipc_kmsg_get_from_kernel(h,h->msgh_size,&kmsg);
	assert(mr == MACH_MSG_SUCCESS);
	assert(kmsg != IKM_NULL);

	dr = dipc_kmsg_copyin(kmsg);
	assert(dr == DIPC_SUCCESS);
	assert(KMSG_IS_DIPC_FORMAT(kmsg));

	assert(DIPC_UID_EQUAL(&local->ip_uid, 
			      (uid_t *)(&kmsg->ikm_header.msgh_local_port)));

	assert(MACH_MSGH_BITS_LOCAL_NMS & kmsg->ikm_header.msgh_bits);

	assert(DIPC_IS_DIPC_PORT(local));
	assert(local->ip_sorights == 0);
	assert(local->ip_remote_sorights == 1);
	assert(local->ip_references == 1);

	assert(DIPC_IS_DIPC_PORT(inlinep));
	assert(inlinep->ip_srights == 0);
	assert(inlinep->ip_transit == -1);
	assert(inlinep->ip_references == 1);

	assert(DIPC_IS_DIPC_PORT(outline));
	assert(outline->ip_srights == 0);
	assert(outline->ip_transit == -1);
	assert(outline->ip_references == 1);

	assert(DIPC_IS_DIPC_PORT(remote));
	assert(remote->ip_sorights == 0);
	/*
	 *	dipc_kmsg_copyin takes TWO extra references
	 *	on the destination port, so the two
	 *	references on remote going into dipc_kmsg_copyin
	 *	became four, then dropped back to three when
	 *	dipc_kmsg_copyin called dipc_port_copyin on
	 *	remote.
	 */
	assert(remote->ip_references == 3);

	/*
	 *	We are testing the copyin path with ool data,
	 *	causing lower-level code to allocate a msg_progress
	 *	structure.  We don't actually MOVE the data, though,
	 *	so the msg_progress is never deallocated.  We manually
	 *	reset its state to MSG_DONE to convince the deallocation
	 *	code not to complain.
	 */
	KMSG_MSG_PROG_SEND(kmsg)->state = MP_S_MSG_DONE;

	/*
	 *	Dispose of the kmsg:  release extra reference
	 *	on remote, destroy copy objects for the ool
	 *	memory region and the ool port region.
	 */
	dipc_kmsg_release(kmsg);

	assert(DIPC_IS_DIPC_PORT(local));
	assert(local->ip_sorights == 0);
	assert(local->ip_remote_sorights == 1);
	assert(local->ip_references == 1);
	dtest_port_destroy(local);

	assert(DIPC_IS_DIPC_PORT(inlinep));
	assert(inlinep->ip_srights == 0);
	assert(inlinep->ip_transit == -1);
	assert(inlinep->ip_references == 1);
	dtest_port_destroy(inlinep);

	assert(DIPC_IS_DIPC_PORT(outline));
	assert(outline->ip_srights == 0);
	assert(outline->ip_transit == -1);
	assert(outline->ip_references == 1);
	dtest_port_destroy(outline);

	assert(DIPC_IS_DIPC_PORT(remote));
	assert(remote->ip_sorights == 0);
	assert(remote->ip_references == 2);
	ipc_port_release(remote);
	ipc_port_release(remote);

	printf("dtest_copyin: done\n");
}

void
dtest_copyout(void)
{
	struct {
		mach_msg_header_t	h;
		mach_msg_body_t		b;
		mach_msg_descriptor_t	d1;
	} msg;
	mach_msg_header_t *h = &msg.h;
	mach_msg_body_t *b = &msg.b;
	mach_msg_port_descriptor_t *d1 = &msg.d1.port;
	dipc_return_t dr;
	uid_t uid;
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;
	ipc_port_t local, remote, inlinep;

	local = dtest_port_alloc_sright(TRUE, SO_RIGHT);
	h->msgh_local_port = (mach_port_t)local;

	inlinep = dtest_port_alloc_sright(TRUE, SEND_RIGHT);
	d1->name = (mach_port_t)inlinep;
	d1->disposition = MACH_MSG_TYPE_PORT_SEND;
	d1->type = MACH_MSG_PORT_DESCRIPTOR;
	
	DIPC_UID_MAKE(&uid, NODE_NAME_LOCAL, 666);
	dr = dipc_port_copyout(&uid,
			       MACH_MSG_TYPE_PORT_SEND_ONCE,
			       dipc_node_self(),
			       FALSE, /*destination*/
			       TRUE,  /*NMS*/
			       &remote);
	assert(dr == DIPC_SUCCESS);
	assert(remote != IP_NULL);
	assert(DIPC_IS_PROXY(remote));
	ip_reference(remote);
	assert(remote->ip_references == 2);
	h->msgh_remote_port = (mach_port_t)remote;

	h->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE,
				      MACH_MSG_TYPE_PORT_SEND_ONCE) |
		       MACH_MSGH_BITS_COMPLEX;

	h->msgh_size = sizeof(msg);
	b->msgh_descriptor_count = 1;

	mr = ipc_kmsg_get_from_kernel(h,h->msgh_size,&kmsg);
	assert(mr == MACH_MSG_SUCCESS);
	assert(kmsg != IKM_NULL);

	dr = dipc_kmsg_copyin(kmsg);
	assert(dr == DIPC_SUCCESS);
	assert(KMSG_IS_DIPC_FORMAT(kmsg));

	assert(DIPC_UID_EQUAL(&local->ip_uid, 
			      (uid_t *)(&kmsg->ikm_header.msgh_local_port)));

	assert(MACH_MSGH_BITS_LOCAL_NMS & kmsg->ikm_header.msgh_bits);

	assert(remote->ip_sorights == 0);
	assert(remote->ip_references == 3 ||
	       (remote->ip_references + 
		remote->ip_ref_count== 3));

	/*
	 *	Now pretend remote is a local receive right
	 *	with one send right outstanding.
	 */
	remote->ip_remote_sorights = 1;
	remote->ip_proxy = FALSE;

	assert(DIPC_IS_DIPC_PORT(local));
	assert(local->ip_sorights == 0);
	assert(local->ip_remote_sorights == 1);
	assert(local->ip_references == 1);

	assert(DIPC_IS_DIPC_PORT(inlinep));
	assert(inlinep->ip_srights == 0);
	assert(inlinep->ip_transit == -1);
	assert(inlinep->ip_references == 1);

	/*
	 *	Now pretend inlinep is a local receive right
	 *	with one transit outstanding.
	 */
	inlinep->ip_transit = -1;
	inlinep->ip_proxy = FALSE;

	kmsg->ikm_header.msgh_remote_port = (mach_port_t)remote;
	kmsg->ikm_handle = dipc_node_self();
	kmsg->ikm_next = kmsg;
	kmsg->ikm_prev = kmsg;

	dr = dipc_kmsg_copyout(kmsg, current_task()->map, MACH_MSG_BODY_NULL);
	assert(dr == DIPC_SUCCESS);

	assert(kmsg->ikm_header.msgh_remote_port == (mach_port_t)remote);
	assert(kmsg->ikm_header.msgh_local_port == (mach_port_t)local);

	assert(!KMSG_IS_DIPC_FORMAT(kmsg));

	/*
	 *	Tear down what we have built up.
	 *	Unfortunately, we can't just use
	 *	dipc_kmsg_release -- the kmsg is
	 *	no longer in DIPC FORMAT -- or
	 *	even ipc_kmsg_destroy -- the
	 *	kernel will want to send notifications
	 *	for the destroyed send-once rights.
	 */

	ip_lock(remote);
	assert(remote->ip_sorights == 1);
	assert(remote->ip_remote_sorights == 0);
	assert(remote->ip_references == 4);
	remote->ip_sorights--;
	ip_release(remote);
	ip_release(remote);
	ip_release(remote);
	ip_check_unlock(remote);
	dtest_port_destroy(remote);

	ip_lock(local);
	assert(local->ip_sorights == 1);
	assert(local->ip_remote_sorights == 0);
	assert(local->ip_references == 2);
	local->ip_sorights--;
	ip_release(local);
	ip_check_unlock(local);
	dtest_port_destroy(local);

	ip_lock(inlinep);
	assert(inlinep->ip_srights == 1);
	assert(inlinep->ip_transit == 0);
	assert(inlinep->ip_references == 2);
	inlinep->ip_srights--;
	ip_release(inlinep);
	ip_check_unlock(inlinep);
	dtest_port_destroy(inlinep);

	ipc_kmsg_free(kmsg);

	printf("dtest_copyout: done\n");
}

ipc_port_t	dtest_send_port;
node_name	dtest_send_rcv_node;
thread_t	dtest_send_thread;

/* receives a message and consumes it and the remote_port send right */
void
dtest_send_test_recv_msg(
	ipc_port_t port,
	int msgnum)
{
	ipc_kmsg_t	kmsg;
	int		*dp;
	dipc_return_t	dr;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	imq_lock(&port->ip_messages);
	assert(port->ip_msgcount + 1 == port->ip_references + port->ip_ref_count);
	mr = ipc_mqueue_receive(&port->ip_messages,
		MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
		MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE,
		&kmsg, &seqno);
	assert(mr == MACH_MSG_SUCCESS);
	assert((int)seqno == msgnum - 1);
	assert(kmsg != IKM_NULL);
	assert(kmsg->ikm_header.msgh_remote_port == (mach_port_t)port);
	assert(kmsg->ikm_header.msgh_id == (10 + msgnum));
	assert(kmsg->ikm_header.msgh_size == (sizeof (mach_msg_header_t) + 8));
	dp = (int *)((unsigned int)&kmsg->ikm_header +
		(unsigned int)sizeof(mach_msg_header_t));
	assert(dp[0] == (msgnum * 100));
	assert(dp[1] == ((msgnum * 100) + 1));
			
	/* clean up remote_port rights and message */
	dr = dipc_kmsg_copyout(kmsg, current_task()->map, MACH_MSG_BODY_NULL);
	assert(dr == DIPC_SUCCESS);
	ipc_kmsg_destroy(kmsg);
}



/*
 *	Multinode tests.
 *
 *	The default is to execute all available multinode tests;
 *	however, it still requires an explicit command to initiate
 *	these tests at all.  This is because we need to know the
 *	name of the victim node.....
 *
 *	Important note:  for many tests, it is critical that DMT_BEGIN
 *	be executed to allocate test buffers on sender and receiver.
 *	DMT_END is then important if you want to clean up these buffers,
 *	which may be very large.
 */

#define	DMT_BEGIN		0x0001
#define DMT_TRANSPORT		0x0002
#define DMT_KKT			0x0004
#define DMT_MSG_SEND		0x0008
#define	DMT_SPECIAL_PORTS	0x0010
#define DMT_RECV_MIGRATE	0x0020
#define DMT_QUEUE_FULL		0x0040
#define DMT_RES_SHORT		0x0080
#define	DMT_MSG_SEND_OOL	0x0100
#define	DMT_MSG_TEST_GEN	0x0200
#define	DMT_FLIPC_TEST		0x0400
#define	DMT_END			0x1000
#define	DMT_ALL			0x1fff

node_name			dmt_node;
unsigned int			dmt_tests = DMT_ALL;

dipc_return_t			dmt_msg_send(
					node_name	node);
dipc_return_t			dmt_special_ports(
					node_name	node);
dipc_return_t			dmt_recv_migrate(
					node_name	node);
dipc_return_t			dmt_queue_full(
					node_name	node);
dipc_return_t			dmt_res_short(
					node_name	node);
dipc_return_t			dmt_ool_send(
					node_name	node);
dipc_return_t			dmt_msg_test_gen(
					node_name	node);
dipc_return_t			dmt_kkt_test(
					node_name	node);
dipc_return_t			dmt_transport(
					node_name	node);

extern	void			kern_msg_test(
					node_name	node);
#if	FLIPC
dipc_return_t			dmt_flipc(
					node_name	node);
#endif	/* FLIPC */


void
dmt_do_test(
	dipc_return_t	(*test)(node_name node),
	char		*description,
	node_name	node)
{
	dipc_return_t	dr;
	TR_DECL("dmt_do_test");

	printf("\t%s...\n", description);
	tr2("Start %s", description);

	dr = (*test)(node);

	tr2("End %s", description);

	if (dr == DIPC_SUCCESS)
		return;

	if (dr == DIPC_INVALID_TEST) {
		printf("\t\tUNIMPLEMENTED.\n");
		return;
	}

	printf("\t\tFAILED with error %d.\n", dr);
}


/*
 *	The dipc multinode tests are now invoked automatically
 *	at boot time--by default. They can also be invoked
 *	manually.
 */
void
dipc_multinode_test_thread(void)
{
	unsigned int	i;
	spl_t		s;

	for (;;) {


		/*
		 *	Wait for the wake-up from dmt_start(). For
		 *	the case where we are running the multinode
		 *	test automatically at boot time, we need to
		 *	first resume the thread that is currently
		 *	blocked in dipc_test_init() having waited to
		 *	make sure we get here before going on.....
		 */

		s = splsched();
		simple_lock(&kernel_test_lock);
		unit_test_thread_sync_done = TRUE;
		assert_wait((event_t)dipc_multinode_test_thread, FALSE);
		if(kernel_test_thread_blocked)
			thread_wakeup((event_t)&kernel_test_thread);
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);

		printf("** DIPC Multinode Tests start:\n");
		for (i = 1; i <= DMT_ALL; i <<= 1) {
			switch (i & dmt_tests) {
			    case DMT_BEGIN:
				dmt_do_test(dmt_begin_sender,
					    "Notify Remote Node (Begin)",
					    dmt_node);
				break;
			    case DMT_END:
				dmt_do_test(dmt_end_sender,
					    "Notify Remote Node (End)",
					    dmt_node);
				break;
			    case DMT_MSG_SEND:
				dmt_do_test(dmt_msg_send,
					    "Send a remote simple message",
					    dmt_node);
				break;

			    case DMT_SPECIAL_PORTS:
				dmt_do_test(dmt_special_ports,
					    "Remote DIPC special ports",
					    dmt_node);
				break;

			    case DMT_RECV_MIGRATE:
				dmt_do_test(dmt_recv_migrate,
					    "DIPC-level recv-right migration",
					    dmt_node);
				break;

			    case DMT_QUEUE_FULL:
				dmt_do_test(dmt_queue_full,
					    "Remote queue full handling",
					    dmt_node);
				break;

			    case DMT_RES_SHORT:
				dmt_do_test(dmt_res_short,
					    "Remote resource shortages",
					    dmt_node);
				break;

			    case DMT_MSG_SEND_OOL:
				dmt_do_test(dmt_ool_send,
					    "DIPC ool data engine",
					    dmt_node);
				break;

			    case DMT_MSG_TEST_GEN:
				dmt_do_test(dmt_msg_test_gen,
					    "DIPC-level message tests",
					    dmt_node);
				break;

			    case DMT_KKT:
				dmt_do_test(dmt_kkt_test,
					    "DIPC KKT test",
					    dmt_node);
				break;

			    case DMT_TRANSPORT:
				dmt_do_test(dmt_transport,
					    "DIPC transport test",
					    dmt_node);
				break;

#if	FLIPC
			    case DMT_FLIPC_TEST:
				dmt_do_test(dmt_flipc,
					    "FLIPC low level test",
					    dmt_node);
				break;
#endif	/* FLIPC */
			}
		}
		printf("** DIPC Multinode Tests conclude\n");
	}
}


#if	FLIPC
/*
 * Invoke flipc low level test (transport only).
 *
 * The following definitions are intended to work across a broad
 * range of architectures.  Currently, FLIPC does not send anything
 * larger than 128 bytes, but the test allocates a 256 byte region
 * to give a broader picture of the transport capabilities.  We send
 * decrementally sized payloads, decrementing by CACHE_LINE which is
 * 32 bytes on current architectures.
 */
#define	CACHE_LINE	32
#define	FLIPC_MAX_PACKET	256

dipc_return_t
dmt_flipc(
	node_name	node)
{
	unsigned int results[FLIPC_MAX_PACKET/CACHE_LINE+1];
	int i;
	int test_size;

	/*
	 *  FLIPC is optimized to send small packets.  Start with the
	 *  largest buffer size and decrease the amount sent by a
	 *  cache-line each time.  A zero length send implies only
	 *  control information gets sent (the payload is 0 length; it
	 *  does not imply no message).
	 */
	test_size = FLIPC_MAX_PACKET;
	for (i = 0; test_size >= 0; i++) {
		/*
		 * most flipc interfaces are asynchronous, so
		 * synchronization must occur in the low_level_test
		 * routine.
		 */
		flipcme_low_level_test(node,
				       (vm_size_t)test_size,
				       &results[i]);
		test_size -= CACHE_LINE;
	}

	/*
	 * Test complete; print out the results
	 */
	printf("Flipc round trip test results:\n\tSize\tusecs\n");
	test_size = FLIPC_MAX_PACKET;
	for (i = 0; test_size >= 0; i++) {
		printf ("\t%d\t%d\n", test_size, results[i]);
		test_size -= CACHE_LINE;
	}

	return DIPC_SUCCESS;
}

#endif	/* FLIPC */

/*
 * send a small inline-only message between nodes,
 * and then receive it on the remote node
 */
dipc_return_t
dmt_msg_send(
	node_name	node)
{
	ipc_kmsg_t		kmsg;
	struct {
		mach_msg_header_t	h;
		int			d[4];
	} msg;
	mach_msg_header_t	*h = &msg.h;
	ipc_port_t		port;
	uid_t			uid;
	dipc_return_t		dr;
	mach_msg_return_t	mr;
	kkt_return_t		kktr;

	/*
	 * make up a uid, and tell the remote node to make
	 * a port and receive right with it.
	 */
	DIPC_UID_MAKE(&uid, node, 1234);
	dr = dipc_uid_lookup(&uid, &port);
	assert(dr == DIPC_NO_UID);
	kktr = drpc_test_sync(node,
			      DMT_MSG_SEND, &uid, 1, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);

	/* create a send right on this node */
	dr = dipc_port_copyout(&uid, MACH_MSG_TYPE_PORT_SEND,
			       node, FALSE, FALSE, &port);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_IS_PROXY(port));

	/* create and send the message */
	bzero((char *)&msg, sizeof(msg));
	h->msgh_remote_port = (mach_port_t)port;
	h->msgh_local_port = MACH_PORT_NULL;
#define	OLD_STUFF 1
#if OLD_STUFF
	h->msgh_size = sizeof(msg);
#else
#define	BIG_MSG	(1*1024*1024)
	h->msgh_size = BIG_MSG;
#endif
	h->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND,
						      0);
	h->msgh_id = 9999;
	msg.d[0] = 0x12121212;
	msg.d[1] = 0x34343434;
	msg.d[2] = 0x56565656;
	msg.d[3] = 0x78787878;
	mr = ipc_kmsg_get_from_kernel(h, h->msgh_size, &kmsg);
	assert(mr == MACH_MSG_SUCCESS);
	assert(kmsg != IKM_NULL);

	/* simulate a COPY_SEND */
	ip_lock(port);
	ip_reference(port);
	port->ip_srights++;
	ip_unlock(port);

	dr = dipc_kmsg_copyin(kmsg);
	assert(dr == DIPC_SUCCESS);
	assert(KMSG_IS_DIPC_FORMAT(kmsg));

	ip_lock(port);

	mr = dipc_mqueue_send(kmsg, 0, 0);
	assert(mr == MACH_MSG_SUCCESS);

	/* tell remote node to receive it and clean up */
	kktr = drpc_test_sync(node,
			      DMT_MSG_SEND, &uid, 2, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	
	/* clean up */
	ip_lock(port);
	assert(port->ip_references == 1);
	ipc_port_destroy(port);

	return dr;
}


/*
 *	Test remote special ports functionality.
 *
 *	Verify operation of remote user special ports.
 *	Kernel special ports require no remote interaction.
 *
 *	Requirements:
 *		- Port name table routines must work.
 *		- Must be able to communicate with
 *		  a second node (implies cross-node
 *		  DIPC RPCs, hence KKT, must work).
 *		- A cooperating test-mode kernel on
 *		  the remote node.
 */

dipc_return_t
dmt_special_ports(
	node_name	test_node)
{
	ipc_port_t		device_port, host_port, host_priv_port;
	ipc_port_t		peek_port, remote_dev_port, user_port;
	ipc_port_t		test_user_port, bogon_port, user_port2;
	node_name		mynode = dipc_node_self();
	kern_return_t		kr;
	uid_t			uid;
	ipc_object_refs_t	iprefs;
	mach_port_rights_t	ipsrights;

#if	MACH_ASSERT
	extern boolean_t	norma_special_ports_initialized;
#endif	/* MACH_ASSERT */
#if	DIPC_DO_STATS
	unsigned int		dipc_remote_entries;
	extern unsigned int	c_dipc_uid_table_entries;
#endif	/* DIPC_DO_STATS */


#if	MACH_ASSERT
	/*
	 *	I can explain...  when !MACH_ASSERT but
	 *	KERNEL_TEST, we have the special definition
	 *	for assert() (see above).  But of course
	 *	there's no norma_special_ports_initialized
	 *	variable.
	 */
	assert(norma_special_ports_initialized == TRUE);
#endif

	return DIPC_INVALID_TEST;
}


/*
 * Migrate a receive right away from this node and then
 * destroy the receive right.  This will test receive
 * right migration and dead name notifications.
 */

ipc_port_t	dtest_multinode_port;	/* global to be examined in debugger */

dipc_return_t
dmt_recv_migrate(
	node_name	node)
{
	ipc_port_t	port;
	uid_t		uid;
	dipc_return_t	dr;
	kkt_return_t	kktr;

	dtest_multinode_port = port = 
		ipc_port_alloc_special(ipc_space_dipc_test);
	assert(port != IP_NULL);
	assert(port->ip_references == 1);
	dr = dipc_port_init(port, TRUE);
	assert(dr == DIPC_SUCCESS);
	port->ip_proxy = FALSE;
	IP_SET_NMS(port);
	port->ip_dest_node = dipc_node_self();
			
	kktr = drpc_test_sync(node,
			      DMT_RECV_MIGRATE, 
			      &port->ip_uid, 1, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
			
	assert(port->ip_transit == 0);
	port->ip_transit = -5;
			
	dr = dipc_port_copyin(port,
			      MACH_MSG_TYPE_PORT_RECEIVE,
			      &uid);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_UID_EQUAL(&uid, &port->ip_uid));
	assert(port->ip_dipc->dip_ms_convert == TRUE);
	assert(port->ip_references == 1);
	port->ip_destination = IP_NULL;
			
	kktr = drpc_test_sync(node,
			      DMT_RECV_MIGRATE, 
			      &uid, 2, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
			
	assert(DIPC_IS_PROXY(port));
	assert(port->ip_references == 1);
	assert(port->ip_dipc->dip_ms_convert == TRUE);
	assert(port->ip_dipc->dip_ms_block == FALSE);
	assert(port->ip_transit == 0);
			
	ip_reference(port);
			
	kktr = drpc_test_sync(node,
			      DMT_RECV_MIGRATE, 
			      &uid, 3, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);

	assert(port->ip_references == 1);
	ip_lock(port);
	ip_release(port);
	ip_check_unlock(port);
			
	return dr;
}


/*
 * Test QUEUE_FULL processing, both blocking the
 * sending thread and enqueueing on the proxy.
 */
uid_t		dtest_send_uid;
int		dtest_send_state = 0;

dipc_return_t
dmt_queue_full(
	node_name	node)
{
	ipc_port_t	port;
	uid_t		uid;
	dipc_return_t	dr;
	kkt_return_t	kktr;
	spl_t		s;

	/* create a receiving port */
	port = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(port != IP_NULL);
	assert(IP_NMS(port));

	port = ipc_port_make_send(port);
	assert(port->ip_references == 2);
	assert(port->ip_srights == 1);

	dr = dipc_port_copyin(port, MACH_MSG_TYPE_PORT_SEND,
			      &uid);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_UID_EQUAL(&port->ip_uid, &uid));
	assert(port->ip_srights == 0);
	assert(port->ip_references == 1);
	assert(port->ip_transit == -1);

	ip_lock(port);
	/* set queue limit to 1 */
	ipc_port_set_qlimit(port, 1);
	ip_unlock(port);

	/*
	 * tell remote node to create the proxy and start
	 * up the sending thread
	 */
	kktr = drpc_test_sync(node,
			      DMT_QUEUE_FULL, &uid, 1, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);

	/* wait for port->ip_callback_map to be set */
	while (port->ip_callback_map == NODE_MAP_NULL) {
		assert_wait((event_t)&port->ip_callback_map, TRUE);
		thread_set_timeout(hz);	/* pause for a sec */
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
	}

	/*
	 * sender should now be blocked for QUEUE_FULL with
	 * one message in the queue.
	 * Verify that the node is in the node map by
	 * trying to add it again.
	 */
	ip_lock(port);
	assert(port->ip_references + port->ip_ref_count == 2);
	assert(port->ip_msgcount == 1);
	kktr = KKT_ADD_NODE(port->ip_callback_map, node);
	assert(kktr == KKT_NODE_PRESENT);
	ip_unlock(port);
	
	/* check state on sending node */
	kktr = drpc_test_sync(node,
			      DMT_QUEUE_FULL, &uid, 2, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	
	assert(port->ip_references + port->ip_ref_count == 2);
	/* receive first message */
	dtest_send_test_recv_msg(port, 1);
	
	/* receive second message */
	dtest_send_test_recv_msg(port, 2);
	
	/* check port state */
	ip_lock(port);
	assert(port->ip_references == 1);
	assert(port->ip_msgcount == 0);
	assert(port->ip_callback_map == NODE_MAP_NULL);
	ip_unlock(port);
	
	dtest_send_uid = uid;
	dtest_send_state = 0;
	
	/* tell sending node to go again */
	kktr = drpc_test_sync(node,
			      DMT_QUEUE_FULL, &uid, 3, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	
	/*
	 * wait until the sending node wakes us up with
	 * drpc_test_sync(... 4 ...)
	 */
	s = splkkt();
	dipc_test_lock();
	while (dtest_send_state != 4) {
		assert_wait((event_t)&dtest_send_state, FALSE);
		dipc_test_unlock();
		splx(s);
		thread_block((void (*)(void)) 0);
		dipc_test_lock();
	}
	dtest_send_state = 0;
	dipc_test_unlock();
	splx(s);
	
	/*
	 * verify that there is a message in the queue, and a
	 * callback request
	 */
	ip_lock(port);
	assert(port->ip_references + port->ip_ref_count == 2);
	assert(port->ip_msgcount == 1);
	assert(port->ip_callback_map != NODE_MAP_NULL);
	kktr = KKT_ADD_NODE(port->ip_callback_map, node);
	assert(kktr == KKT_NODE_PRESENT);
	ip_unlock(port);
	
	/* pause for a second */
	thread_set_timeout(hz);
	thread_block((void (*)(void)) 0);
	reset_timeout_check(&current_thread()->timer);
	
	/* check state on sending node */
	kktr = drpc_test_sync(node, DMT_QUEUE_FULL, &uid, 5, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	
	/* receive messages 3-6 */
	dtest_send_test_recv_msg(port, 3);
	dtest_send_test_recv_msg(port, 4);
	dtest_send_test_recv_msg(port, 5);
	dtest_send_test_recv_msg(port, 6);

	/* check port state */
	ip_lock(port);
	assert(port->ip_references == 1);
	assert(port->ip_msgcount == 0);
	assert(port->ip_callback_map == NODE_MAP_NULL);
	ip_unlock(port);
	
	/* tell sending node to check state and clean up */
	kktr = drpc_test_sync(node, DMT_QUEUE_FULL, &uid, 6, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	
	/* clean up */
	DIPC_UID_NULL(&dtest_send_uid);
	ip_lock(port);
	assert(port->ip_srights == 0);
	assert(port->ip_references == 1);
	ipc_port_clear_receiver(port);
	ipc_port_destroy(port);		/* consumes lock */
	
	return dr;
}


/*
 *	Test resource shortage.
 */
dipc_return_t
dmt_res_short(
	node_name	node)
{
	return DIPC_INVALID_TEST;
}


/*
 * Send a large OOL message between nodes, verify it on the remote node
 */
int ool_send_pages = 200;
dipc_return_t
dmt_ool_send(
	node_name	node)
{
	kkt_return_t	kktr;
	dipc_return_t	dr;
	ipc_port_t	port;
	uid_t		uid;
	ipc_kmsg_t	kmsg;
	unsigned int	*send_region, *sr;
	vm_map_copy_t	copy;
	mach_msg_return_t mr;
	mach_msg_format_0_trailer_t	*trailer;
	int		i;
	struct {
		mach_msg_header_t	head;
		mach_msg_body_t		body;
		mach_msg_ool_descriptor_t data;
	} msg;
	mach_msg_header_t *h = &msg.head;

	/*
	 * make up a uid, and tell the remote node to make
	 * a port and receive right with it.
	 */
	DIPC_UID_MAKE(&uid, node, 1235);
	dr = dipc_uid_lookup(&uid, &port);
	assert(dr == DIPC_NO_UID);
	/*
	 * This RPC sets up a receiving port - use the MSG_SEND
	 * RPC which is identical, rather than creating a special
	 * OOL routine.
	 */
	kktr = drpc_test_sync(node, DMT_MSG_SEND, &uid, 1, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);

	/* create a send right on this node */
	dr = dipc_port_copyout(&uid, MACH_MSG_TYPE_PORT_SEND,
			       node, FALSE, FALSE, &port);
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_IS_PROXY(port));

	/* create and send the message */
	bzero((char*)&msg, sizeof(msg));
	h->msgh_remote_port = (mach_port_t)port;
	h->msgh_local_port = MACH_PORT_NULL;
	h->msgh_size = sizeof(msg);
	h->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0) |
		    MACH_MSGH_BITS_COMPLEX_OOL | MACH_MSGH_BITS_COMPLEX;
	h->msgh_id = 9998;
	msg.body.msgh_descriptor_count = 1;
	/*
	 * Get an ENTRY_LIST flavor copy object.  The memory
	 * allocated here will need to be released!
	 */
#define	SEND_SIZE	(PAGE_SIZE * ool_send_pages)
	printf("\t\tSending %d ool pages in a single message...\n",
	       ool_send_pages);
	kmem_alloc_pageable(kernel_map,
			    (vm_offset_t *)&send_region, SEND_SIZE);
	/*
	 * Fault in the pages
	 */
	for (sr = send_region, i = 0; i < ool_send_pages; i++) {
		*sr = i;
		sr += PAGE_SIZE / sizeof(int);
	}
	vm_map_copyin(kernel_map, (vm_offset_t) send_region,
		      (vm_size_t) SEND_SIZE, FALSE, &copy);
        msg.data.address = (void *)(copy);
        msg.data.size = SEND_SIZE;
        msg.data.deallocate =  FALSE;
        msg.data.copy = MACH_MSG_VIRTUAL_COPY;
        msg.data.type = MACH_MSG_OOL_DESCRIPTOR;

	mr = ipc_kmsg_get_from_kernel(h, h->msgh_size, &kmsg);
	assert(mr == MACH_MSG_SUCCESS);
	assert(kmsg != IKM_NULL);

	/* simulate a COPY_SEND */
	ip_lock(port);
	ip_reference(port);
	port->ip_srights++;
	ip_unlock(port);

	dr = dipc_kmsg_copyin(kmsg);
	assert(dr == DIPC_SUCCESS);
	assert(KMSG_IS_DIPC_FORMAT(kmsg));

	ip_lock(port);

	mr = dipc_mqueue_send(kmsg, 0, 0);
	assert(mr == MACH_MSG_SUCCESS);

	/* tell remote node to receive it and clean up */
	kktr = drpc_test_sync(node, DMT_MSG_SEND_OOL, &uid, 3, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);
	
	/* clean up */
	ip_lock(port);
	ip_release(port);    /* Release self ref, leaving message ref */
	assert(port->ip_references == 1);
	ip_unlock(port);
	/*
	 * The final ref on the port should go away when the
	 * kmsg is released, which happens asynchronously.
	 */

	return dr;
}


/*
 *	Use the message test generator to send a medley of messages between
 *	nodes. 
 */
dipc_return_t
dmt_msg_test_gen(
	node_name	node)
{
	spl_t	s;

	assert(dipc_node_is_valid(node));

	kern_msg_test(node);

	/* 
	 *	Block this thread until the dipc-level message test just
	 *	started above is finished. The wake-up will come from
	 *	msg_test_sender().
	 */

	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_done){
		assert_wait((event_t)&kern_msg_test, FALSE);
		unit_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		unit_test_thread_blocked = FALSE;
	}
	unit_test_done = FALSE;  /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);
	return(DIPC_SUCCESS);
}

void
dtest_send_error(
	int		error,
	handle_t	handle,
	request_block_t req)
{
	panic("dtest_kkt_test: error!\n");
}

void
dtest_kkt_test_notify(
	int		error,
	handle_t	handle,
	request_block_t req,
	boolean_t	thread_context)
{
	boolean_t	*wait;
	extern unsigned int trbb_cur_timer; /* oh, so tacky! XXX */

	if (trbb_cur_timer)
		timer_stop(trbb_cur_timer, req->data_length);
	timer_stop((unsigned int)req, req->data_length);
	wait = (boolean_t *)req->args[0];
	*wait = FALSE;
	thread_wakeup_one((event_t)wait);
}


unsigned int	dipc_test_obtain_buffer_warning = 0;

int
dipc_test_obtain_buffer(
	char	*caller_name)
{
	kern_return_t	kr;

	if (dipc_test_region != (vm_offset_t) 0) {
		if (dipc_test_region % page_size != 0) {
			if (dipc_test_obtain_buffer_warning++ == 0)
				printf("%s:  test region 0x%x mis-aligned",
				       caller_name, dipc_test_region);
			return 0;
		}
		return 1;
	}
	
	dipc_test_region = round_page(dipc_test_region_memory);
	return 1;
}


int
dipc_test_verify_buffer(
	char	*caller_name)
{
	if (dipc_test_region == (vm_offset_t) 0) {
		panic("dipc_test_verify_buffer:  no region!");
		return 0;
	}

	if (dipc_test_region % page_size != 0) {
		panic("dipc_test_verify_buffer:  region 0x%x unaligned\n",
		      dipc_test_region);
		return 0;
	}
	return 1;
}


unsigned int	dipc_test_release_buffer_warning = 0;

int
dipc_test_release_buffer(
	char	*caller_name)
{
	if (dipc_test_region == (vm_offset_t) 0)
		return 1;

	dipc_test_region = (vm_offset_t) 0;
	return 1;
}


void
dmt_begin_receiver()
{
	if (dipc_test_obtain_buffer("dmt_start_receiver"))
		return;
	printf("dmt_start_receiver:  no memory was allocated.\n");
	printf("\tFURTHER KKT/DIPC TESTS ARE LIKELY TO HANG.\n");
}


void
dmt_end_receiver()
{
	(void) dipc_test_release_buffer("dmt_end_receiver");
}


dipc_return_t
dmt_begin_sender(
	node_name	node)
{
	kkt_return_t	kktr;
	dipc_return_t	dr;
	uid_t		uid;

	/*
	 *	Allocate global send-side buffer for future tests.
	 */
	if (!dipc_test_obtain_buffer("dmt_begin_sender"))
		return DIPC_NO_MEMORY;

	kktr = drpc_test_sync(node, DMT_BEGIN, &uid, 1, &dr);
	return dr;
}


dipc_return_t
dmt_end_sender(
	node_name	node)
{
	kkt_return_t	kktr;
	dipc_return_t	dr;
	uid_t		uid;
	spl_t		s;

	(void) dipc_test_release_buffer("dmt_end_sender");

	kktr = drpc_test_sync(node, DMT_END, &uid, 1, &dr);

	/* 
	 *	We are done so we wake up the thread suspended in
	 *	dipc_test_init(). 
	 */
	s = splsched();
	simple_lock(&kernel_test_lock);
	unit_test_thread_sync_done = TRUE;
	if(kernel_test_thread_blocked)
		thread_wakeup((event_t)&kernel_test_thread);
	simple_unlock(&kernel_test_lock);
	splx(s);

	return dr;
}


#if	DIPC_TIMER
char	*kkt_btest_name = "KKT Send bulk";
char	*kkt_test_max_name = "KKT Send max";
#endif	/* DIPC_TIMER */

#define KKT_TEST_ITERATIONS 100
unsigned int kkt_test_iterations = KKT_TEST_ITERATIONS;

#define	K	* 1024
#define	M	* 1024 * 1024

unsigned int	transport_send_size_table[] = {
	4 K,
	8 K,
	16 K,
	32 K,
	64 K,
	128 K,
	256 K,
	512 K,
	1 M,
	2 M,
#if	PARAGON860
	3 M,
#endif	/* PARAGON860 */
	4 M,
#if	PARAGON860
	5 M,
	6 M,
	7 M,
#endif	/* PARAGON860 */
	8 M,
#if	!PARAGON860
	16 M,
#endif	/* !PARAGON860 */
	0
};


dipc_return_t	dmt_do_kkt_test(node_name, unsigned int);

dipc_return_t
dmt_kkt_test(
	node_name	node)
{
	unsigned int	*sizep;

	printf("\t\t%d iterations\n", kkt_test_iterations);

	if (!dipc_test_verify_buffer("dmt_kkt_test"))
		return DIPC_NO_MEMORY;

	for (sizep = transport_send_size_table; *sizep != 0; ++sizep)
		if (*sizep <= dipc_test_size)
			dmt_do_kkt_test(node, *sizep);

	/*
	 * Invoke the driver in kkt_driver.c
	 */
	kkt_test_driver(node);

	return (DIPC_SUCCESS);
}


dipc_return_t
dmt_do_kkt_test(
	node_name	node,
	unsigned int	size)
{
	handle_t	handle;
	kkt_return_t	kr;
	request_block_t	req, treq, nreq[4];
	boolean_t	wait;
	endpoint_t	endp;
	kern_return_t	ret[2];
	int		iter = kkt_test_iterations, i;
	vm_offset_t	curp;
	int		s;

	if (!dipc_test_verify_buffer("dmt_do_kkt_test"))
		return DIPC_NO_MEMORY;

	req = dipc_get_kkt_req(TRUE);

	timer_set((unsigned int)&wait, kkt_btest_name);
	timer_set((unsigned int)req, kkt_test_max_name);

	while (iter--) {
		timer_start((unsigned int)&wait);
		kr = KKT_HANDLE_ALLOC(CHAN_TEST_KMSG,
				      &handle,
				      dtest_send_error,
				      TRUE,
				      FALSE);
		assert(kr == KKT_SUCCESS);

		req->data.address = dipc_test_region;
		req->data_length = size;
		req->callback = dtest_kkt_test_notify;
		req->request_type = REQUEST_SEND;
		wait = TRUE;
		req->args[0] = (unsigned int)&wait;
#define	TEST_BIGBUF	0xedde0001	/* TEST_BIGBUF in kkt_test.c */
		endp.ep[0] = (void *) TEST_BIGBUF;

		timer_start((unsigned int)req);

		kr = KKT_SEND_CONNECT(handle,
				      node,
				      &endp,
				      req,
				      FALSE,
				      &ret[0]);
		assert(kr == KKT_SUCCESS);

		while (wait) {
			assert_wait((event_t) &wait, TRUE);
			thread_block((void (*)) 0);
		}
		KKT_HANDLE_FREE(handle);

		timer_stop((unsigned int)&wait, size);
	}
	dipc_free_kkt_req(req);

	return (DIPC_SUCCESS);
}


dipc_return_t
dmt_transport(
	node_name	node)
{
	vm_offset_t	data;
	int		i;
	unsigned int	*sizep;

	if (!dipc_test_verify_buffer("dmt_transport"))
		return DIPC_NO_MEMORY;

	for (sizep = transport_send_size_table; *sizep != 0; ++sizep) {
		if (*sizep > dipc_test_size)
			continue;
		for (i = 0; i < 100; i++) {
			data = dipc_test_region;
			transport_send_test(*sizep, data, node);
		}
	}

	return (DIPC_SUCCESS);
}

/*
 *	Kick the multinode tests into life.
 *
 *	If you desire a particular subset of the
 *	tests, you must first set dmt_tests to the
 *	desired bitmask.  The default is to execute
 *	all available tests.
 */
void
dmt_start(
	node_name	node)
{
	if (node == dipc_node_self()) {
		printf("Cannot run tests in loopback mode\n");
		return;
	}
	dmt_node = node;
	thread_wakeup_one((event_t)dipc_multinode_test_thread);
}


node_name		ool_node_val;
unsigned int		ool_test_val;
uid_t			ool_uid_val;

dipc_return_t
dproc_test_sync(
	        node_name		node,
		unsigned int		test,
		uid_t			*uidp,
		unsigned int		opaque)
{
	dipc_return_t		dr;
	ipc_port_t		port;
	uid_t			uid2;
	ipc_kmsg_t		kmsg;
	mach_msg_return_t	mr;
	mach_port_seqno_t	seqno;
	spl_t			s;
	int			*dp;
	int			i;
	struct {
		mach_msg_header_t	h;
		int			d[4];
	} *msg;

	switch (test) {
	      case DMT_BEGIN:
		dmt_begin_receiver();
		dr = DIPC_SUCCESS;
		break;

	      case DMT_END:
		dmt_end_receiver();
		dr = DIPC_SUCCESS;
		break;

	      case DMT_RECV_MIGRATE:
		switch (opaque) {
		      case 1:
			dr = dipc_uid_lookup(uidp, &port);
			assert(dr == DIPC_NO_UID);
			dr = DIPC_SUCCESS;
			break;
		      case 2:
			dr = dipc_port_copyout(uidp, 
					       MACH_MSG_TYPE_PORT_RECEIVE,
					       node,
					       FALSE,
					       TRUE,
					       &port);
			assert(dr == DIPC_SUCCESS);
			assert(!DIPC_IS_PROXY(port));
			assert(port->ip_dipc->dip_ms_convert == FALSE);
			assert(port->ip_dipc->dip_ms_block == FALSE);
			assert(port->ip_references == 1);
			assert(port->ip_dnrequests != IPR_NULL);
			assert(port->ip_transit == -5);
			dr = DIPC_SUCCESS;
			break;
		      case 3:
			dr = dipc_uid_lookup(uidp, &port);
			assert(dr == DIPC_SUCCESS);
			dr = dipc_uid_port_reference(port);
			assert(dr == DIPC_SUCCESS);
			ip_lock(port);
			ipc_port_destroy(port);
			ip_lock(port);
			assert(port->ip_references == 1);
			assert(!ip_active(port));
			ip_release(port);
			ip_check_unlock(port);
			dr = DIPC_SUCCESS;
			break;
		      default:
			dr = DIPC_INVALID_TEST;
		}
		break;

	      case DMT_MSG_SEND:
		switch (opaque) {
		      case 1:
			/* create a receiving port */
			dr = dipc_uid_lookup(uidp, &port);
			assert(dr == DIPC_NO_UID);
			port = ipc_port_alloc_special(ipc_space_dipc_test);
			assert(port != IP_NULL);
			dr = dipc_port_init(port, FALSE);
			assert(dr == DIPC_SUCCESS);
			dr = dipc_uid_install(port, uidp);
			assert(dr == DIPC_SUCCESS);

			ip_lock(port);
			ip_reference(port);
			assert(port->ip_srights == 0);
			assert(port->ip_references == 2);
			port->ip_srights = 1;
			ip_unlock(port);
			dr = dipc_port_copyin(port, MACH_MSG_TYPE_PORT_SEND,
				&uid2);
			assert(dr == DIPC_SUCCESS);
			assert(DIPC_UID_EQUAL(uidp, &uid2));
			ip_lock(port);
			assert(port->ip_srights == 0);
			assert(port->ip_references == 1);
			ip_unlock(port);

			dr = DIPC_SUCCESS;
			break;

		      case 2:
			/* receive the message */

			dr = dipc_uid_lookup(uidp, &port);
			assert(dr == DIPC_SUCCESS);
			dr = dipc_uid_port_reference(port);
			assert(dr == DIPC_SUCCESS);
			ip_lock(port);
			assert(port->ip_references == 3 ||
			       (port->ip_references + 
				port->ip_ref_count== 3));
			assert(port->ip_msgcount == 1);
			ip_release(port);    /* unneeded ref acquired above */
			ip_unlock(port);
			imq_lock(&port->ip_messages);
			mr = ipc_mqueue_receive(&port->ip_messages,
				MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
				MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE,
				&kmsg, &seqno);
			assert(mr == MACH_MSG_SUCCESS);
			assert(kmsg != IKM_NULL);
			assert(kmsg->ikm_header.msgh_remote_port ==
							(mach_port_t)port);
			assert(kmsg->ikm_header.msgh_id == 9999);
#if OLD_STUFF
			assert(kmsg->ikm_header.msgh_size == sizeof(*msg));
#else
			assert(kmsg->ikm_header.msgh_size == BIG_MSG);
#endif
			dp = (int *)((unsigned int)&kmsg->ikm_header +
				(unsigned int)sizeof(mach_msg_header_t));
			assert(dp[0] == 0x12121212);
			assert(dp[1] == 0x34343434);
			assert(dp[2] == 0x56565656);
			assert(dp[3] == 0x78787878);
			
			/* clean up */
			ipc_kmsg_free(kmsg);
			ip_lock(port);
			ip_release(port);
			assert(port->ip_references == 1);
			ipc_port_clear_receiver(port);
			ipc_port_destroy(port);		/* consumes lock */

			dr = DIPC_SUCCESS;
			break;
		}
		break;

	      case DMT_QUEUE_FULL:
		switch (opaque) {
		      case 1:
			/* set up sending side, wake up sending thread */

			dr = dipc_uid_lookup(uidp, &port);
			assert(dr == DIPC_NO_UID);

			/* create a send right on this node */
			dr = dipc_port_copyout(uidp, MACH_MSG_TYPE_PORT_SEND,
				node, FALSE, TRUE, &port);
			assert(dr == DIPC_SUCCESS);
			assert(DIPC_IS_PROXY(port));
			assert(port->ip_references == 1);
			assert(port->ip_srights == 1);
			assert(port->ip_transit == 1);
			assert(port->ip_receiver == ipc_space_remote);
			assert(port->ip_dipc->dip_ms_convert == TRUE);

			/* set things up for sending thread */
			dtest_send_uid = *uidp;
			dtest_send_port = port;
			dtest_send_rcv_node = node;
			dtest_send_state = opaque;

			/* wake up the dipc_test_thread to be the sender */
			thread_wakeup_one((event_t)dipc_test_thread);

			dr = DIPC_SUCCESS;
			break;

		      case 2:
		      case 5:
		      
			/* verify that sender is blocked */

			dr = dipc_uid_lookup(uidp, &port);
			assert(dr == DIPC_SUCCESS);
			dr = dipc_uid_port_reference(port);
			assert(dr == DIPC_SUCCESS);
			assert(port == dtest_send_port);

			ip_lock(port);
			ip_release(port);/* drop extra ref from lookup */
			assert(port->ip_qf_count == 1);
			assert(!port->ip_resending);
			assert(!port->ip_rs_block);
			assert(port->ip_restart_list == IP_NULL);
			assert(ipc_thread_queue_first(&port->ip_blocked) ==
				dtest_send_thread);
			if (opaque == 2)
				assert(port->ip_msgcount == 0);
			else if (opaque == 5)
				assert(port->ip_msgcount == 2);
			ip_unlock(port);

			dtest_send_state = opaque;
			dr = DIPC_SUCCESS;
			break;

		      case 3:
		      case 4:
			/* wake up sending (case 3) or receiving (4) thread */

			assert(DIPC_UID_EQUAL(uidp, &dtest_send_uid));
			s = splkkt();
			dipc_test_lock();
			dtest_send_state = opaque;
			dipc_test_unlock();
			splx(s);
			thread_wakeup((event_t)&dtest_send_state);

			dr = DIPC_SUCCESS;
			break;

		      case 6:
			/* check port state and clean up */

			assert(DIPC_UID_EQUAL(uidp, &dtest_send_uid));
			port = dtest_send_port;

			ip_lock(port);
			assert(port->ip_qf_count == 0);
			assert(!port->ip_resending);
			assert(!port->ip_rs_block);
			assert(port->ip_restart_list == IP_NULL);
			assert(ipc_thread_queue_empty(&port->ip_blocked));
			assert(port->ip_msgcount == 0);

			/*
			 * Use the port->ip_references count to determine
			 * whether the kmsg's have been released since the
			 * messages have already been received on the 
			 * receiving node. Loop for 10 one-second timeouts
			 * before giving up on waiting for kmsg release.
			 */
			for(i = 0 ; i < 10; i++ ){
				if(port->ip_references == 1)
					break;
				else{
					ip_unlock(port);
					thread_will_wait_with_timeout(
 					   current_thread(),1000 /* msecs */);
					thread_block((void (*)(void)) 0);
					ip_lock(port);
				}
			}
			assert(port->ip_references == 1);
			assert(port->ip_srights == 1);
			ip_unlock(port);
			ipc_port_release_send(port);

			dtest_send_state = 0;
			DIPC_UID_NULL(&dtest_send_uid);
			dtest_send_port = IP_NULL;
			dtest_send_rcv_node = NODE_NAME_NULL;

			dr = DIPC_SUCCESS;
			break;
		}
		break;

	      case DMT_RES_SHORT:
		switch (opaque) {
		      case 1:
			dr = DIPC_SUCCESS;
			break;
		}
		break;

	      case DMT_MSG_SEND_OOL:
	        ool_node_val = node;
		ool_test_val = test;
		ool_uid_val = *uidp;
		dr = DIPC_SUCCESS;
		thread_wakeup_one((event_t)ool_receiver_thread);
		break;

	      default:
		dr = DIPC_INVALID_TEST;
	}

	return (dr);
}

void
dtest_send_test_msg(
	mach_msg_header_t	*h,
	int			msgnum,
	mach_msg_option_t	option)
{
	int *dp;
	mach_msg_return_t mr;

	h->msgh_id = 10 + msgnum;
	dp = (int *)((unsigned int)h + (unsigned int)sizeof(mach_msg_header_t));
	dp[0] = (msgnum * 100);
	dp[1] = (msgnum * 100) + 1;

	mr = dipc_msg_send_from_kernel(h, h->msgh_size, option, 0);
	assert(mr == MACH_MSG_SUCCESS);
}


void
dtest_send_test(void)
{
	dipc_return_t	dr;
	kkt_return_t	kktr;
	spl_t		s;
	struct {
		mach_msg_header_t	h;
		int			d[2];
	} msg;
	mach_msg_header_t *h = &msg.h;
	mach_msg_return_t mr;

	dtest_send_thread = current_thread();
	assert(dtest_send_port != IP_NULL);

	/* initialize message */
	bzero((char *)&msg, sizeof(msg));
	h->msgh_remote_port = (mach_port_t)dtest_send_port;
	h->msgh_local_port = MACH_PORT_NULL;
	h->msgh_size = sizeof(msg);
	h->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);

	/* create and send a message to fill the queue */
	dtest_send_test_msg(h, 1, 0);

	/*
	 * create and send another message.  The send should block for
	 * QUEUE_FULL, which will be verified elsewhere.
	 */
	dtest_send_test_msg(h, 2, 0);

	/* check state of port -- nothing should be blocked */
	ip_lock(dtest_send_port);
	assert(dtest_send_port->ip_qf_count == 0);
	assert(!dtest_send_port->ip_resending);
	assert(!dtest_send_port->ip_rs_block);
	assert(dtest_send_port->ip_restart_list == IP_NULL);
	assert(ipc_thread_queue_empty(&dtest_send_port->ip_blocked));
	assert(dtest_send_port->ip_msgcount == 0);
	ip_unlock(dtest_send_port);

	/* wait until receiving side says to go again */
	s = splkkt();
	dipc_test_lock();
	while (dtest_send_state != 3) {
		assert_wait((event_t)&dtest_send_state, FALSE);
		dipc_test_unlock();
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splkkt();
		dipc_test_lock();
	}
	dipc_test_unlock();
	splx(s);

	/* send message 3 to fill up queue */
	dtest_send_test_msg(h, 3, 0);

	/*
	 * send messages 4 and 5 with MACH_SEND_ALWAYS and make sure they're
	 * enqueued here on the proxy
	 */
	dtest_send_test_msg(h, 4, MACH_SEND_ALWAYS);
	assert(dtest_send_port->ip_qf_count == 1);
	assert(!dtest_send_port->ip_resending);
	assert(!dtest_send_port->ip_rs_block);
	assert(dtest_send_port->ip_restart_list == IP_NULL);
	assert(ipc_thread_queue_empty(&dtest_send_port->ip_blocked));
	assert(dtest_send_port->ip_msgcount == 1);

	dtest_send_test_msg(h, 5, MACH_SEND_ALWAYS);
	assert(dtest_send_port->ip_qf_count == 1);
	assert(!dtest_send_port->ip_resending);
	assert(!dtest_send_port->ip_rs_block);
	assert(dtest_send_port->ip_restart_list == IP_NULL);
	assert(ipc_thread_queue_empty(&dtest_send_port->ip_blocked));
	assert(dtest_send_port->ip_msgcount == 2);

	/*
	 * wake up receiver
	 */
	kktr = drpc_test_sync(dtest_send_rcv_node, DMT_QUEUE_FULL,
						&dtest_send_uid, 4, &dr);
	assert(kktr == KKT_SUCCESS);
	assert(dr == DIPC_SUCCESS);


	/*
	 * send message 6 without SEND_ALWAYS; it should block (verified
	 * elsewhere)
	 */
	dtest_send_test_msg(h, 6, 0);

	/* check state of port -- nothing should be blocked */
	ip_lock(dtest_send_port);
	assert(dtest_send_port->ip_qf_count == 0);
	assert(!dtest_send_port->ip_resending);
	assert(!dtest_send_port->ip_rs_block);
	assert(dtest_send_port->ip_restart_list == IP_NULL);
	assert(ipc_thread_queue_empty(&dtest_send_port->ip_blocked));
	assert(dtest_send_port->ip_msgcount == 0);
	ip_unlock(dtest_send_port);

	/* all done */
	dtest_send_thread = THREAD_NULL;
}

unsigned int	boot_node;
void
dipc_test_thread(void)
{
	spl_t		s;
	kern_return_t	kr;
	dipc_return_t	dr;
	uid_t		loopback_uid;

#if	PARAGON860
	{
		char		*s;
		unsigned int	firstnode;

		if ((s = (char *) getbootenv("BOOT_FIRST_NODE")) == 0)
			panic("dipc_test_thread");
		firstnode = atoi(s);
		boot_node = firstnode;
		if (dipc_node_self() != (node_name) firstnode) {
			printf("Non-boot-node skipping DIPC loopback tests.\n");
			goto out;
		}
	}
#else	/* PARAGON860 */
	boot_node = 0;
#endif	/* PARAGON860 */

	printf("** DIPC Loopback Tests start:\n");

	dtest_alloc_routines();

	/* Test uid code */
	dtest_uid_routines();

	/*
	 *	Local-case functionality of DIPC special ports.
	 */
	dtest_special_ports();
	/* test DIPC RPC service */
	dtest_rpc();

	/* allocate a test port and make a naked send right */
	dipc_test_port = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(dipc_test_port != IP_NULL);
	(void)ipc_port_make_send(dipc_test_port);

	/* set up the dipc part of the port and give it a uid */
	dr = dipc_port_init(dipc_test_port, TRUE);
	assert(dr == DIPC_SUCCESS);
	dtest_port_origin = (void *)dipc_test_port->ip_uid.origin;
	dtest_port_ident = (void *)dipc_test_port->ip_uid.identifier;

	/*
	 * allocate a loopback proxy port for dipc_test_port, make it a proxy,
	 * give it a corresponding uid with the loopback node, and install it.
	 * It's OK to defer assigning a UID until after calling
	 * dipc_port_init because this is special-case test code
	 * that is assumed to be non-racy.
	 */
	dipc_test_port_proxy = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(dipc_test_port_proxy != IP_NULL);
	(void)ipc_port_make_send(dipc_test_port_proxy);
	dr = dipc_port_init(dipc_test_port_proxy, FALSE);
	assert(dr == DIPC_SUCCESS);
	dipc_test_port_proxy->ip_proxy = TRUE;
	dipc_test_port_proxy->ip_dest_node = NODE_NAME_LOCAL;
	IP_CLEAR_NMS(dipc_test_port_proxy);
	DIPC_UID_MAKE(&loopback_uid, NODE_NAME_LOCAL,(port_id)dtest_port_ident);
	dr = dipc_uid_install(dipc_test_port_proxy, &loopback_uid);
	assert(dr == DIPC_SUCCESS);

	/* allocate another port, give it a uid, and make it inactive */
	dipc_inactive_port = ipc_port_alloc_special(ipc_space_dipc_test);
	assert(dipc_inactive_port != IP_NULL);
	(void)ipc_port_make_send(dipc_inactive_port);
	dr = dipc_port_init(dipc_inactive_port, TRUE);
	assert(dr == DIPC_SUCCESS);
	dipc_inactive_port->ip_object.io_bits &= ~IO_BITS_ACTIVE;

	dtest_mqdeliver_to_msgqueue();
	dtest_enqueue_to_msgqueue();
	dtest_deliver_to_msgqueue();
	dtest_send_to_msgqueue();
	dtest_send_ool_to_msgqueue();
	dtest_port_copyinout();
	dtest_copyin();
	dtest_copyout();

	kern_msg_test(dipc_node_self()); /* run an in-kernel ipc-level test */

	/* 
	 *	Serialize the tests by suspending this thread until
	 *	the ipc-level test started by kern_msg_test is done. The
	 *	wake-up call will come from msg_test_sender.
	 */

	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_done){
		assert_wait((event_t)&kern_msg_test, FALSE);
		unit_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		unit_test_thread_blocked = FALSE;
	}
	unit_test_done = FALSE;  /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);

	printf("** DIPC Loopback Tests conclude\n");

#if	PARAGON860
    out:
#endif	/* PARAGON860 */

	/*
	 *	Wake up the thread that was suspended in dipc_test_init.
	 */

	s = splsched();
	simple_lock(&kernel_test_lock);
	unit_test_thread_sync_done = TRUE;
	if(kernel_test_thread_blocked)
		thread_wakeup((event_t)&kernel_test_thread);
	simple_unlock(&kernel_test_lock);
	splx(s);

	for(;;) {
		if (dtest_send_state != 0) {
			dtest_send_test();
		}
		assert_wait((event_t)dipc_test_thread, FALSE);
		thread_block((void (*))0);
	}
}


void
ool_receiver_thread()
{
	dipc_return_t dr;
	ipc_port_t port;
	uid_t uid;
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;
	mach_port_seqno_t seqno;
	struct smsg {
		mach_msg_header_t	h;
		mach_msg_body_t		body;
		mach_msg_ool_descriptor_t data;
	} *msg;
	int *dp, i;
	mach_msg_body_t		*mbody;
	mach_msg_descriptor_t	*mdes;
	vm_map_copy_t		mcopy;

	for(;;) {
		if (ool_uid_val.identifier == 0) {
			assert_wait((event_t)ool_receiver_thread, FALSE);
			thread_block((void (*))0);
			continue;
		}
		
		uid = ool_uid_val;
		
		/* receive the message */
		
		dr = dipc_uid_lookup(&uid, &port);
		assert(dr == DIPC_SUCCESS);
		dr = dipc_uid_port_reference(port);
		assert(dr == DIPC_SUCCESS);
		ip_lock(port);
		assert(port->ip_references + port->ip_ref_count == 3);
		assert(port->ip_msgcount == 1);
		ip_release(port);    /* unneeded ref acquired above */
		ip_unlock(port);
		imq_lock(&port->ip_messages);
		mr = ipc_mqueue_receive(&port->ip_messages,
					MACH_MSG_OPTION_NONE, MACH_MSG_SIZE_MAX,
					MACH_MSG_TIMEOUT_NONE, FALSE, IMQ_NULL_CONTINUE,
					&kmsg, &seqno);
		assert(mr == MACH_MSG_SUCCESS);
		assert(kmsg != IKM_NULL);
		mr = dipc_kmsg_copyout(kmsg, current_task()->map, MACH_MSG_BODY_NULL);
		assert(dr == DIPC_SUCCESS);
		assert(kmsg->ikm_header.msgh_remote_port ==
		       (mach_port_t)port);
		assert(kmsg->ikm_header.msgh_id == 9998);
		assert(kmsg->ikm_header.msgh_size == sizeof(*msg));
		
		/*
		 * check the bits out; see if we got what we expected
		 */
		msg = (struct smsg *)&kmsg->ikm_header;
		mbody = (mach_msg_body_t *)(&msg->h + 1);
		mdes = (mach_msg_descriptor_t *)(mbody + 1);
		
		for(i=0;i<mbody->msgh_descriptor_count;i++) {
			switch (mdes->type.type & ~DIPC_TYPE_BITS) {
			    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
			    case MACH_MSG_OOL_DESCRIPTOR:
				assert((mdes->type.type & ~DIPC_TYPE_BITS) == 
				       MACH_MSG_OOL_DESCRIPTOR ||
				       (mdes->type.type & ~DIPC_TYPE_BITS) ==
				       MACH_MSG_OOL_VOLATILE_DESCRIPTOR);
				mcopy = (vm_map_copy_t)mdes->out_of_line.address;
				/*
				 * need to check the OOL data here
				 */
				break;
			    default:
				break;
			}
		}
		
		/* clean up */
		ipc_kmsg_free(kmsg);
		ip_lock(port);
		ip_release(port);	/* release message ref from copyout */
		assert(port->ip_references == 1);
		ipc_port_clear_receiver(port);
		ipc_port_destroy(port);		/* consumes lock */
		dr = DIPC_SUCCESS;
		
		assert_wait((event_t)ool_receiver_thread, FALSE);
		thread_block((void (*))0);
	}
}

#if	DIPC_TIMER
char	*kkt_recv_name = "KKT Recv";
char	*dtest_transport_recv = "Transport Recv";
#endif	/* DIPC_TIMER */

char *msge = "DIPC multinode tests will run automatically on node %d with node %d\n as the target. After node %d comes up, check that the tests completed.\n";

/*
 * The following external variables can be modified from the debugger,
 * during a boot with the "-h" option, to specify whether the dipc_multi_node
 * test is run automatically at boot-time and if so what nodes should
 * participate in that test.
 */
boolean_t	auto_dmt_start = TRUE;
unsigned int	sender_node = 1, target_node = 0;

void
dipc_test_init(boolean_t startup)
{
	int		*p, *pe;
	int		i;
	spl_t		s;
	unsigned int	this_node;
	char		*s1, *s2, *s3;
	unsigned int	numb_nodes;
	kern_return_t	kr;

	/* some of the DIPC tests must be called at startup time */
	assert(startup == TRUE);

	/*
	 *	Perform initialization before kicking off test
	 *	threads, to guarantee that the data structures
	 *	are, in fact, initialized *before* the test
	 *	threads can get to them.
	 */

	transport_test_setup();

	simple_lock_init(&dipc_test_lock_data, ETAP_DIPC_TEST_LOCK);

	/* allocate a special space */
	kr = ipc_space_create_special(&ipc_space_dipc_test);
	assert(kr == KERN_SUCCESS);

	ool_send_pages_msgqueue = CHAINSIZE;


	(void) kernel_thread(kernel_task, dipc_test_thread, (char *) 0);

	/* 
	 *	Serialize the tests by suspending this thread until
	 *	tests just started in dipc_test_thread() are done.
	 *	The wake-up will come from dipc_test_thread.
	 */
	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_thread_sync_done){
		assert_wait((event_t)&kernel_test_thread, FALSE);
		kernel_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		kernel_test_thread_blocked = FALSE;
	}
	unit_test_thread_sync_done = FALSE; /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);

	/*	
	 *	The next thread does not do much before it blocks. So we'll
	 *	leave it in an unserialized state for the moment. If it is an
	 * 	issue we will serialize this later....
	 */

	(void) kernel_thread(kernel_task, ool_receiver_thread, (char *) 0);

#if	PARAGON860
	/*
	 *	For the paragon, we use a bootmagic parameter to specify
	 *	whether to run the dipc multi-node tests automatically.
	 *	If the pertinent parameter: AUTO_DMT_START is set or if it
	 *	is not referenced, we run the test. If we do run the test,
	 *	we use the first (non-boot) io node as the target node and
	 *	the first compute node as the sender. This makes use of the
	 *	fact that when the paragon is booted, the io nodes come up
	 *	first followed by the compute nodes.
	 */

	if ((auto_dmt_start =  getbootint("AUTO_DMT_START", -1)) == 0)
		auto_dmt_start = FALSE;

	/*
	 *	Check that we can get the needed bootenv variables to set
	 *	things up to run the dipc multinode test automatically using
	 *	the nodes previously referred to.
	 */
	if (auto_dmt_start &&
	   (((s1 = (char *) getbootenv("BOOT_NUM_NODES")) == 0) ||
	   ((s2 = (char *) getbootenv("BOOT_COMPUTE_NODE_LIST")) == 0) ||
	   ((s3 = (char *) getbootenv("BOOT_IO_NODE_LIST")) == 0))){
	       printf("Can't get needed bootenv params: skipping auto test\n");
	       auto_dmt_start = FALSE;
	}
/*
	printf("BOOT_COMPUTE_NODE_LIST =%s\nBOOT_IO_NODE_LIST =%s\n", s2, s3);
*/
	if(auto_dmt_start){
		/*
		 *	Find the first (non-boot) io node--the target node.
		 */
		numb_nodes = atoi(s1);
		for( i = 0; i < numb_nodes; i++)
			if(boot_node == (node_name) i)
				continue;
			else if((char *) node_in_list(i, s3) != 0){
				target_node = i;
				break;
			}		

		/*
		 *	Find the first compute node--the sending node.
		 */
#if	FLIPC
		/*
		 *	For a flipc kernel, a node with a revision
		 *	B NIC chip is needed. Since there is always
		 *	the chance that the first sending node may not
		 *	be equipped with this chip, the sender node
		 *	number may have to be specified via bootmagic.
		 */
		if ((auto_dmt_start =  getbootint("DMT_SENDER_NODE", -1)) == 0)
			sender_node = 2; 
#else	/* FLIPC */

		for( i = 0; i < numb_nodes; i++)
			if((char *) node_in_list(i, s2) != 0){
				sender_node = i;
				break;
			}
#endif	/* FLIPC */
	}
	
#else	/* PARAGON860 */
	/*
	 *	For the x86, by default, we use node 0 as the target
	 *	and node 1 as sender. These defaults can be changed
         *	by bringing up the kernels with the "-h" option and
	 *	changing the defaults for auto_dmt_start and/or 
	 *	sender_node and/or target_node as appropriate. The
	 *	key thing is to remember to make the changes to all
	 *	the nodes in the cluster or at least to those nodes
	 *	where needed. 
	 */
#endif	/* PARAGON860 */

	this_node = dipc_node_self();

	if(auto_dmt_start && (this_node == boot_node))
		printf(msge, sender_node, target_node, sender_node);

	kernel_thread(kernel_task, dipc_multinode_test_thread, (char *) 0);

	/* 
	 *	Block this thread until the thread created which calls
	 *	dipc_multinode_test_thread() informs us that it is 
	 *	waiting for its own wakeup call.
	 */

	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_thread_sync_done){
		assert_wait((event_t)&kernel_test_thread, FALSE);
		kernel_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		kernel_test_thread_blocked = FALSE;
	}
	unit_test_thread_sync_done = FALSE; /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);

	if(auto_dmt_start && (this_node == (node_name) sender_node)){
		/*
		 *	Start the multinode tests with node target_node as
		 *	target.
		 */
		dmt_start((node_name)target_node);
		/* 
		 *	Serialize the tests by blocking this thread until the
		 *	multinode test started by dmt_start is finished. The
		 *	wake-up comes from dmt_end_sender.
		 */

		s = splsched();
		simple_lock(&kernel_test_lock);
		while(!unit_test_thread_sync_done){
			assert_wait((event_t)&kernel_test_thread, FALSE);
			kernel_test_thread_blocked = TRUE;
			simple_unlock(&kernel_test_lock);
			splx(s);
			thread_block((void (*)(void)) 0);
			s = splsched();
			simple_lock(&kernel_test_lock);
			kernel_test_thread_blocked = FALSE;
		}
		unit_test_thread_sync_done = FALSE; /* Reset for next use */
		simple_unlock(&kernel_test_lock);
		splx(s);
	}

	timer_set((unsigned int)kkt_recv_name, kkt_recv_name);
	timer_set((unsigned int)dtest_transport_recv, dtest_transport_recv);

}


#if	!MACH_ASSERT
unsigned int	dipc_test_assert_enabled = 1;

#if	MACH_KDB
#include <ddb/db_output.h>
extern int	db_breakpoints_inserted;
#endif	/* MACH_KDB */

void
dipc_test_assert(
	char	*file,
	int	line)
{
	if (! dipc_test_assert_enabled)
		return;

	printf("DIPC Test assertion failed:\n\tfile \"%s\", line %d\n",
	       file, line);
#if	MACH_KDB
	if (db_breakpoints_inserted)
#endif	/* MACH_KDB */
	Debugger("DIPC Test assertion failure");
}
#endif	/* !MACH_ASSERT */

#if	TRACE_BUFFER
void
db_show_tr_overhead(void);
void
db_show_tr_overhead(void)
{
	TR_DECL("db_show_tr_overhead");

	tr1("enter");
	tr1("exit");
}
#endif	/* TRACE_BUFFER */

#endif	/* KERNEL_TEST */
