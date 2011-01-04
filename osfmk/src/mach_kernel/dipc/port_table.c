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
 *	File:	dipc/port_table.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Functions to manage the distributed port name table.
 */


#include <kernel_test.h>
#include <mach_kdb.h>

#include <kern/assert.h>
#include <kern/lock.h>
#include <dipc/dipc_funcs.h>
#include <dipc/port_table.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_counters.h>
#include <kern/lock.h>
#include <mach/kkt_request.h>
#include <mach/norma_special_ports.h>



/*
 *	Given an ipc_port_t, the attached dipc_port_t contains
 *	the port's network-wide name, or UID.  However, looking
 *	up a port based on its UID is less simple.  A hash table
 *	chains together all ports based on their UIDs.  The hash
 *	table itself, the dipc_port_name_table, is nothing more
 *	than a set of buckets pointing at ports.  The dipc_port_t
 *	contains the following interesting fields:
 *
 *		dip_uid		Port's network-wide name
 *		dip_hash_next	Next port in the hash bucket
 *		dip_ref_count	Number of interrupt-level refs to port
 *
 *	The UID is created in thread context, under the ipc_port_t's
 *	ip_lock.  The UID remains valid until the port is destroyed.
 *
 *	Because port locking demands a thread context, locating
 *	a port from interrupt level requires some care.  At interrupt
 *	level, it is not possible to lock the port's lock and increase
 *	the port's reference count.  Hence, we use the dip_ref_count
 *	to signify uses of the port from interrupt level.  A port may
 *	only be destroyed when its ipc_object's reference count *and*
 *	its dip_ref_count have both fallen to zero.  Furthermore, a port
 *	may only be destroyed in thread context.
 *
 *	Because a port is removed from the dipc_port_name_table
 *	before it is destroyed, we can be sure that there is no way
 *	to find a port based on its UID while racing with port
 *	destruction.
 *
 *	Once in thread context,	it is necessary to "rationalize"
 *	the port reference count by increasing it to account for the
 *	reference acquired in interrupt context, then decreasing the
 *	dip_ref_count.
 *
 *	Use and modification of dip_hash_next and dip_ref_count is
 *	governed by the dpn_table_lock.
 *
 *	The port lock takes precedence over dpn_table_lock.
 */


/*
 *	UID allocation note.  The first few unique identifiers,
 *	0..MAX_SPECIAL_ID, are reserved in the UID name space
 *	for kernel and user special ports.  (These are ports
 *	essential to low-level system operation that can't
 *	easily be named through other port-based operations.)
 */

/*
 *	The port name table consists of a series of hash buckets,
 *	each containing a pointer to the first port with a UID
 *	that hashes to the bucket.  Subsequent hash collisions hang
 *	off the dipc_port_t's dip_hash_next field.
 *
 *	The table is dynamically allocated at initialization time.
 *
 *	Because the table may be manipulated as the result of an
 *	upcall from the transport, the table, its contents (i.e.,
 *	dip_hash_next and dip_ref_count) and the table's lock
 *	must always be manipulated at splkkt.
 */

#define	DIPC_PORT_NAME_TABLE_BUCKETS	1024
#define	DIPC_HASH_MASK			(DIPC_PORT_NAME_TABLE_BUCKETS - 1)
#define	DIPC_UID_HASH(uidp)		(((uidp)->origin + (uidp)->identifier) \
					 & dipc_hash_mask)

unsigned int	dipc_port_name_table_buckets = DIPC_PORT_NAME_TABLE_BUCKETS;
unsigned int	dipc_hash_mask = DIPC_HASH_MASK;
ipc_port_t	*dipc_port_name_table;
ipc_port_t	*dipc_port_name_table_end;

/*
 *	We enter new ports at the beginning of a hash table
 *	bucket, which undoubtedly discriminates against older
 *	and possibly more-used ports.
 */
#define	PORT_NAME_BUCKET_SELECT(bucket, uidp)				\
	bucket = dipc_port_name_table + DIPC_UID_HASH(uidp);

#define	PORT_NAME_TABLE_ENTER(port, dip, bucket)			\
MACRO_BEGIN								\
	PORT_NAME_BUCKET_SELECT(bucket, &(dip)->dip_uid);		\
	(dip)->dip_hash_next = *bucket;					\
	*bucket = port;							\
	dstat(++c_dipc_uid_table_entries);				\
MACRO_END


/*
 *	A single global lock is used to protect the entire port
 *	name table.  If necessary, this lock could be split into
 *	per-hash-bucket locks.  This lock must be manipulated at
 *	splkkt.
 *
 *	This lock also guards the global port identifier counter
 *	used to create new UIDs.
 */
decl_simple_lock_data(,dpn_table_lock)

#define	DIPC_PORT_NAME_TABLE_LOCK	simple_lock(&dpn_table_lock)
#define	DIPC_PORT_NAME_TABLE_UNLOCK	simple_unlock(&dpn_table_lock)
#define	DIPC_PORT_NAME_TABLE_LOCKED	simple_lock_held(&dpn_table_lock)


#if	MACH_ASSERT
/*
 *	Verify that the processor is in interrupt or in
 *	thread context.  We always use the "...thread..."
 *	check when we *expect* the cpu to be in thread
 *	context, and the "...interrupt..." check when we
 *	expect the cpu to be in interrupt context.
 *
 *	For now, these definitions merely document the
 *	expectations of the programmer.  Eventually, they
 *	might be implemented with the necessary hardware
 *	hooks to check the cpu state at run-time.
 */
#define	cpu_in_thread_context()		TRUE
#define	cpu_in_interrupt_context()	TRUE
#endif	/* MACH_ASSERT */



/*
 *	The dipc_uid_identifier is used to assign new, unique
 *	identifiers when creating new UIDs.  The counter is
 *	assumed never to rollover.  If one believes otherwise,
 *	enable ROLLOVER_PARANOIA.
 *
 *	An identifier value of zero is reserved, and means
 *	that the UID is invalid/unassigned.
 *
 *	Use of the dipc_uid_identifier is serialized by the
 *	dpn_table_lock.
 */
port_id		dipc_uid_identifier = 0;
#define	ROLLOVER_PARANOIA	MACH_ASSERT

#if	ROLLOVER_PARANOIA
unsigned int	c_dipc_uid_allocate_collisions = 0;
#endif	/* ROLLOVER_PARANOIA */

dipc_return_t	dipc_uid_peek(uid_t		*uid,
			      ipc_port_t	*port);


#if	MACH_ASSERT
/*
 *	Simple check to guarantee that functions aren't used
 *	before the subsystem has been initialized.
 */
boolean_t	dipc_port_name_table_initialized = FALSE;
#endif	/* MACH_ASSERT */


dstat_decl(unsigned int c_dipc_uid_table_entries = 0;)
dstat_decl(unsigned int	c_dipc_uid_lookup_calls = 0;)
dstat_decl(unsigned int c_dipc_uid_lookup_fast_calls = 0;)
dstat_decl(unsigned int	c_dipc_uid_lookup_searches = 0;)
dstat_decl(unsigned int	c_dipc_uid_lookup_hits = 0;)
dstat_decl(unsigned int	c_dipc_uid_remove_calls = 0;)
dstat_decl(unsigned int	c_dipc_uid_remove_searches = 0;)
dstat_decl(unsigned int	c_dipc_uid_remove_hits = 0;)


/*
 *	Assign a UID to a port and install the port in the
 *	port name table.  The port and dipc_port are separate
 *	at this point due to races in entering ports into
 *	DIPC (see dipc_port.c:dipc_port_init()).  On success,
 *	we will attach the dipc_port to the port, here.
 *
 *	The port may be locked but need not be; this routine
 *	does not block.  However, if the port is unlocked at
 *	this point, presumably the caller has special knowledge.
 */

dipc_return_t
dipc_uid_allocate(
	ipc_port_t	port,
	dipc_port_t	dip)
{
	ipc_port_t	*bucket;
	spl_t		s;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(!DIPC_IS_DIPC_PORT(port));
	assert(cpu_in_thread_context() == TRUE);

	s = splkkt();
	DIPC_PORT_NAME_TABLE_LOCK;

	/*
	 *	Assign UID.
	 */
	DIPC_UID_MAKE(&dip->dip_uid, dipc_node_self(), ++dipc_uid_identifier);

#if	ROLLOVER_PARANOIA
	/*
	 *	Guarantee no collisions ever on UID assignment.
	 *	Only fails if all possible identifiers are
	 *	currently in use, an unlikely possibility indeed.
	 *	This code should not be enabled in normal use
	 *	because it will make UID allocation much more
	 *	expensive.
	 */
	for (;;) {
		ipc_port_t	collision_port;

		if (dipc_uid_peek(&dip->dip_uid, &collision_port)==DIPC_NO_UID)
			break;
		++c_dipc_uid_allocate_collisions;
		dip->dip_uid.identifier = ++dipc_uid_identifier;
	}
#endif	/* ROLLOVER_PARANOIA */

	/*
	 *	Enter port into the port name table and attach
	 *	the dipc_port at the same time.  From the
	 *	standpoint of other routines probing the name
	 *	table, this port appears in it atomically.
	 *	If the caller has been careful, anyone else
	 *	using the port at the same time will see it
	 *	see it atomically transition into DIPC.
	 */
	PORT_NAME_TABLE_ENTER(port, dip, bucket);
	port->ip_dipc = dip;

	DIPC_PORT_NAME_TABLE_UNLOCK;
	splx(s);
	return DIPC_SUCCESS;
}


/*
 *	Add a port to the name table with an already-existing UID.
 *
 *	The port may be locked but need not be.
 *
 *	This call does not block.
 */
dipc_return_t
dipc_uid_install(
	ipc_port_t	port,
	uid_t		*uidp)
{
	ipc_port_t	*bucket;
	ipc_port_t	collision_port;
	spl_t		s;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(DIPC_IS_DIPC_PORT(port));
	assert(port->ip_uid.origin == NODE_NAME_NULL);
	assert(port->ip_uid.identifier == PORT_ID_NULL);
	assert(cpu_in_thread_context() == TRUE);

	s = splkkt();
	DIPC_PORT_NAME_TABLE_LOCK;

	/*
	 *	Catch races assigning a UID to a port,
	 *	or an attempt to re-assign a UID to a port
	 *	that hasn't been recycled.  Unfortunately,
	 *	there's no good way for us to distinguish
	 *	between the first (legitimate) case) and
	 *	the second (erroneous) case.
	 */
	if (port->ip_uid.identifier != PORT_ID_NULL) {
		DIPC_PORT_NAME_TABLE_UNLOCK;
		splx(s);
		return DIPC_DUPLICATE;
	}

	/*
	 *	An identifier hasn't already been assigned
	 *	to this port.  The port therefore must be
	 *	a principal, and must never have had a UID
	 *	assigned during its lifetime.
	 *
	 *	Furthermore, the proposed UID must have a
	 *	valid origin and a valid identifier.
	 */
	assert(port->ip_hash_next == IP_NULL);
	assert(port->ip_ref_count == 0);
	assert(port->ip_uid.origin == NODE_NAME_NULL);
	assert(port->ip_uid.identifier == PORT_ID_NULL);
	assert(DIPC_UID_VALID(uidp));


	/*
	 *	Check for a collision -- this happens as the result
	 *	of a dipc_uid_lookup/dipc_uid_install race.  In the
	 *	absence of guaranteed external synchronization, there
	 *	is no other way to detect and avoid this race.
	 */

	if (dipc_uid_peek(uidp, &collision_port) == DIPC_SUCCESS) {
		DIPC_PORT_NAME_TABLE_UNLOCK;
		splx(s);
		return DIPC_DUPLICATE;
	}

	/*
	 *	Assign UID.
	 */
	port->ip_uid = *uidp;

	/*
	 *	Enter port into port name table.
	 */
	PORT_NAME_TABLE_ENTER(port, port->ip_dipc, bucket);

	DIPC_PORT_NAME_TABLE_UNLOCK;
	splx(s);
	return DIPC_SUCCESS;
}


/*
 *	Look for a UID in the table and return the
 *	associated port without incrementing its
 *	reference count.
 *
 *	Must be called while holding the port name
 *	table lock.  Currently thought only to be
 *	called in thread context; and not intended
 *	for export outside the port table routines.
 *
 *	Caller must also have acquired splkkt.
 */
dipc_return_t
dipc_uid_peek(
	uid_t		*uidp,
	ipc_port_t	*port)
{
	ipc_port_t	*bucket;
	ipc_port_t	p;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(cpu_in_thread_context() == TRUE);
	DIPC_PORT_NAME_TABLE_LOCKED;
	assert(DIPC_UID_VALID(uidp));

	PORT_NAME_BUCKET_SELECT(bucket, uidp);
	for (p = *bucket; p != IP_NULL; p = p->ip_hash_next)
		if (DIPC_UID_EQUAL(&p->ip_uid, uidp)) {
			*port = p;
			return DIPC_SUCCESS;
		}

	return DIPC_NO_UID;
}


/*
 *	Look for a UID in the table, returning its
 *	associated port after incrementing the port's
 *	DIPC reference count.  The DIPC reference count
 *	exists to permit lookups from interrupt context,
 *	when it isn't possible to directly increment
 *	the port's reference count (contained in the
 *	embedded io object).
 *
 *	We expect lookups to take place on UIDs that
 *	"look reasonable", e.g., specify nodes within
 *	the domain that have valid node numbers.  It
 *	shouldn't be possible to construct a UID anywhere
 *	in the system that doesn't refer to a valid node
 *	(note that valid doesn't imply alive).
 */
dipc_return_t
dipc_uid_lookup(
	uid_t		*uidp,
	ipc_port_t	*port)
{
	ipc_port_t	*bucket;
	ipc_port_t	p;
	spl_t		s;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(DIPC_UID_VALID(uidp));

	/*
	 *	Search the table for the proposed UID.
	 *	If we find a match, increment the port's
	 *	DIPC reference count to prevent the port
	 *	from being deallocated before we have a
	 *	chance to fix up the real reference count.
	 */
	dstat(++c_dipc_uid_lookup_calls);
	s = splkkt();
	DIPC_PORT_NAME_TABLE_LOCK;
	PORT_NAME_BUCKET_SELECT(bucket, uidp);
	for (p = *bucket; p != IP_NULL; p = p->ip_hash_next) {
		dstat(++c_dipc_uid_lookup_searches);
		assert(DIPC_IS_DIPC_PORT(p));
		if (DIPC_UID_EQUAL(&p->ip_uid, uidp)) {
			dstat(++c_dipc_uid_lookup_hits);
			p->ip_ref_count++;
			*port = p;
			DIPC_PORT_NAME_TABLE_UNLOCK;
			splx(s);
			return DIPC_SUCCESS;
		}
	}
	DIPC_PORT_NAME_TABLE_UNLOCK;
	splx(s);
	return DIPC_NO_UID;
}


/*
 *	Look for a UID in the table, returning its
 *	associated port after incrementing the port's
 *	DIPC reference count.  The DIPC reference count
 *	exists to permit lookups from interrupt context,
 *	when it isn't possible to directly increment
 *	the port's reference count (contained in the
 *	embedded io object).
 *
 *	We expect lookups to take place on UIDs that
 *	"look reasonable", e.g., specify nodes within
 *	the domain that have valid node numbers.  It
 *	shouldn't be possible to construct a UID anywhere
 *	in the system that doesn't refer to a valid node
 *	(note that valid doesn't imply alive).
 *
 *	The caller must acquire splkkt before invoking
 *	this routine.
 */
dipc_return_t
dipc_uid_lookup_fast(
	uid_t		*uidp,
	ipc_port_t	*port)
{
	ipc_port_t	*bucket;
	ipc_port_t	p;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(DIPC_UID_VALID(uidp));

	/*
	 *	Search the table for the proposed UID.
	 *	If we find a match, increment the port's
	 *	DIPC reference count to prevent the port
	 *	from being deallocated before we have a
	 *	chance to fix up the real reference count.
	 */
	dstat(++c_dipc_uid_lookup_fast_calls);
	DIPC_PORT_NAME_TABLE_LOCK;
	PORT_NAME_BUCKET_SELECT(bucket, uidp);
	for (p = *bucket; p != IP_NULL; p = p->ip_hash_next) {
		dstat(++c_dipc_uid_lookup_searches);
		assert(DIPC_IS_DIPC_PORT(p));
		if (DIPC_UID_EQUAL(&p->ip_uid, uidp)) {
			dstat(++c_dipc_uid_lookup_hits);
			p->ip_ref_count++;
			*port = p;
			DIPC_PORT_NAME_TABLE_UNLOCK;
			return DIPC_SUCCESS;
		}
	}
	DIPC_PORT_NAME_TABLE_UNLOCK;
	return DIPC_NO_UID;
}


/*
 *	Adjust the port's reference counts.  The dip_ref_count
 *	is incremented to prevent the port from disappearing,
 *	but as soon as possible (i.e., as soon as we have a
 *	thread context) the reference counts should be adjusted
 *	to remove the DIPC reference and create a real reference.
 *
 *	Must be called from thread context.
 */
dipc_return_t
dipc_uid_port_reference(
	ipc_port_t	port)
{
	spl_t		s;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(DIPC_IS_DIPC_PORT(port));
	assert(cpu_in_thread_context() == TRUE);

	ip_lock(port);

	s = splkkt();
	DIPC_PORT_NAME_TABLE_LOCK;

	/*
	 *	The port's dip_ref_count must be positive
	 *	and its real reference count must be non-negative.
	 */
	assert(port->ip_dipc->dip_ref_count > 0);
	assert(port->ip_references >= 0);

	/*
	 *	Increment port reference count, under lock,
	 *	to account for DIPC reference.
	 */
	assert(port->ip_references > 0 ||
	       (DIPC_IS_DIPC_PORT(port) && port->ip_ref_count > 0));
	ip_reference(port);
	assert(port->ip_references > 0);

	/*
	 *	Remove DIPC reference.
	 */
	port->ip_dipc->dip_ref_count--;
	assert(port->ip_dipc->dip_ref_count >= 0);

	DIPC_PORT_NAME_TABLE_UNLOCK;
	splx(s);
	ip_unlock(port);
	return DIPC_SUCCESS;
}


/*
 *	Remove a port from the port name table.
 *
 *	The caller is expected to make this call as part of
 *	the port destruction sequence.  Note, however, that
 *	the port itself and the ip_dipc port extension may not
 *	be destroyed until the port reference count *AND* the
 *	dip_ref_count have both fallen to zero.
 *
 *	This routine detects races with interrupt-level
 *	DIPC operations.
 *
 *	It is permissible to call dipc_uid_remove on a port
 *	that has already been removed from the port table.
 *
 *	This call may only take place in thread context.
 */
dipc_return_t
dipc_uid_remove(
	ipc_port_t	port)
{
	ipc_port_t	*bucket, *last, p;
	spl_t		s;

	assert(dipc_port_name_table_initialized == TRUE);
	assert(DIPC_IS_DIPC_PORT(port));
	assert(cpu_in_thread_context() == TRUE);
	assert(port->ip_uid.identifier != PORT_ID_NULL);

	/*
	 *	Because the port name table is maintained
	 *	as a set of hash buckets, each with a
	 *	singly-linked list of ports, it is necessary
	 *	to walk the bucket looking for the port
	 *	prior to the one to be removed.  Once found,
	 *	that port must be updated to point to the
	 *	port pointed to by the removed port.  (Try
	 *	saying *that* sentence three times quickly!)
	 */
	dstat(++c_dipc_uid_remove_calls);
	s = splkkt();
	DIPC_PORT_NAME_TABLE_LOCK;
	if (port->ip_ref_count > 0) {
		/*
		 *	Race:  an interrupt-level entity is
		 *	using this port at the same time
		 *	that we are.  Tell the caller to
		 *	try again later.
		 */
		DIPC_PORT_NAME_TABLE_UNLOCK;
		splx(s);
		return DIPC_IN_USE;
	}
	PORT_NAME_BUCKET_SELECT(bucket, &port->ip_uid);
	last = bucket;
	for (p = *bucket; p != IP_NULL; p = p->ip_hash_next) {
		assert(DIPC_IS_DIPC_PORT(p));
		dstat(++c_dipc_uid_remove_searches);
		if (DIPC_UID_EQUAL(&p->ip_uid, &port->ip_uid)) {
			dstat(--c_dipc_uid_table_entries);
			dstat(++c_dipc_uid_remove_hits);
			*last = p->ip_hash_next;
			p->ip_hash_next = IP_NULL;
			DIPC_PORT_NAME_TABLE_UNLOCK;
			splx(s);
			return DIPC_SUCCESS;
		}
		last = &p->ip_hash_next;
	}
	DIPC_PORT_NAME_TABLE_UNLOCK;
	splx(s);
	return DIPC_SUCCESS;
}


/*
 *	Allocate and initialize port name table; initialize
 *	table lock; and (for debugging purposes only) remember
 *	that the initialization has been done.
 */
void
dipc_port_name_table_init()
{
	ipc_port_t	*dpip;

	assert(dipc_port_name_table_initialized == FALSE);

	/*
	 *	Create and initialize port name hash table & lock.
	 *	XXX
	 */
	dipc_port_name_table = (ipc_port_t *)
		kalloc((vm_size_t) (sizeof(*dpip) *
				    dipc_port_name_table_buckets));
	dipc_port_name_table_end = dipc_port_name_table +
		dipc_port_name_table_buckets;

	for (dpip=dipc_port_name_table; dpip<dipc_port_name_table_end; ++dpip)
		*dpip = IP_NULL;

	simple_lock_init(&dpn_table_lock, ETAP_DIPC_PORT_NAME);

	dipc_uid_identifier = MAX_SPECIAL_ID + 100; /* slop */

#if	MACH_ASSERT
	dipc_port_name_table_initialized = TRUE;
#endif
}


#if	MACH_KDB
#include <ddb/db_output.h>
#define	printf	kdbprintf
extern int	indent;


#define	PORT_TABLE_WALK(dpip, ip)					\
	for (dpip=dipc_port_name_table; dpip<dipc_port_name_table_end; ++dpip) \
		for (ip = *dpip; ip != IP_NULL; ip = ip->ip_hash_next)

void		dipc_port_display(
			ipc_port_t	ip);
ipc_port_t	db_dipc_port_find(
			uid_t	*uidp);
ipc_port_t	db_dipc_port_get(
			node_name	uid_node,
			port_id		uid_ident);
void		db_dipc_port_table(void);
void		db_dipc_port_table_proxies(void);
void		db_dipc_port_table_principals(void);
void		db_dipc_port_deads(void);
void		db_dipc_port_contender(
			ipc_port_t	ip);
void		db_dipc_port_contention(void);
void		db_dipc_port_table_statistics(void);
void		db_dipc_port_table_report(void);


void
dipc_port_display(
	ipc_port_t	ip)
{
	iprintf("0x%x ref %3d/%2d UID (",
		ip, ip->ip_references, ip->ip_ref_count);
	if (ip->ip_uid.origin == NODE_NAME_NULL)
		printf("%s", "NULL");
	else if (ip->ip_uid.origin == NODE_NAME_LOCAL)
		printf("%s", "LOCAL");
	else
		printf("%5x", ip->ip_uid.origin);
	assert(ip->ip_special == FALSE || ip->ip_forward == FALSE);
	printf(",%6x) %s %s\n",
	       ip->ip_uid.identifier,
	       ip->ip_proxy ? "prox" : "prin",
	       ip->ip_special ? "spec" : (ip->ip_forward ? "forw" : ""));
}


/*
 *	Given a pointer to a UID, return a port pointer.
 *	Be careful not to acquire or release locks, or
 *	increment any reference counts.  Debug use only.
 *	Stuff the UID anywhere, e.g., in parse_tr's buffer.
 */
ipc_port_t
db_dipc_port_find(
	uid_t	*uidp)
{
	dipc_return_t	dr;
	ipc_port_t	*bucket;
	ipc_port_t	p;

	PORT_NAME_BUCKET_SELECT(bucket, uidp);
	for (p = *bucket; p != IP_NULL; p = p->ip_hash_next)
		if (DIPC_UID_EQUAL(&p->ip_uid, uidp)) {
			printf("port 0x%x\n", p);
			return p;
		}
	printf("Couldn't find UID\n");
	return 0;
}


/*
 *	Hack:  assume that we know the format of the UID.
 */
ipc_port_t
db_dipc_port_get(
	node_name	uid_node,
	port_id		uid_ident)
{
	uid_t	uid;

	uid.origin = uid_node;
	uid.identifier = uid_ident;
printf("Searching for UID 0x%x/0x%x\n", uid_node, uid_ident);
	return db_dipc_port_find(&uid);
}


void
db_dipc_port_table(void)
{
	ipc_port_t	*dpip, ip;
	int		count = 0;

	iprintf("Port name table contents");
#if	DIPC_DO_STATS
	printf(" (%d entries)", c_dipc_uid_table_entries);
#endif	/* DIPC_DO_STATS */
	printf(":\n");

	indent += 2;
	PORT_TABLE_WALK(dpip, ip) {
		dipc_port_display(ip);
		++count;
	}
	iprintf("Found %d ports in table.\n", count);
	indent -= 2;
}


void
db_dipc_port_table_proxies(void)
{
	ipc_port_t	*dpip, ip;
	int		count = 0;

	iprintf("Port name table contents (proxies only):\n");

	indent += 2;
	PORT_TABLE_WALK(dpip, ip) {
		if (IP_IS_REMOTE(ip))
			dipc_port_display(ip);
		++count;
	}
	iprintf("Found %d proxies in table.\n", count);
	indent -= 2;
}


void
db_dipc_port_table_principals(void)
{
	ipc_port_t	*dpip, ip;
	int		count = 0;

	iprintf("Port name table contents (principals only):\n");

	indent += 2;
	PORT_TABLE_WALK(dpip, ip) {
		if (!IP_IS_REMOTE(ip))
			dipc_port_display(ip);
		++count;
	}
	iprintf("Found %d principals in table.\n", count);
	indent -= 2;
}


void
db_dipc_port_deads(void)
{
	ipc_port_t	*dpip, ip;
	unsigned int	dead_count;

	dead_count = 0;
	PORT_TABLE_WALK(dpip, ip) {
		if (!ip_active(ip))
			++dead_count;
	}
	iprintf("Dead ports in Port Name Table:  %d\n", dead_count);
	indent += 2;
	PORT_TABLE_WALK(dpip, ip) {
		if (!ip_active(ip))
			dipc_port_display(ip);
	}
	indent -= 2;
}


/*
 *	Walk the port table looking for high-contention ports;
 *	in this case, contention means a heavy message queue backlog.
 */
#define	TOP_PORT_CONTENDERS	10
ipc_port_t	contenders[TOP_PORT_CONTENDERS];
unsigned int	contend_count = 0;


/*
 *	For now, simply look for ports with the highest number
 *	of queue_full hits.  A better comparison might be the
 *	percentage of queue_full hits against deliveries, but
 *	this is adequate for now.
 */
#define	ip_queue_full	ip_dipc->dip_queue_full
void
db_dipc_port_contender(
	ipc_port_t	ip)
{
	unsigned int	i, lowest;
	ipc_port_t	ip2;
	boolean_t	insert = FALSE;

	lowest = TOP_PORT_CONTENDERS;
	for (i = 0; i < contend_count; ++i) {
		ip2 = contenders[i];
		if ((lowest == TOP_PORT_CONTENDERS)
		    || contenders[lowest]->ip_queue_full > ip2->ip_queue_full)
			lowest = i;
		/*
		 *	Even after we know that we should
		 *	insert the new element, continue
		 *	throughout the entire array to find
		 *	the lowest replacement candidate.
		 */
		if (ip->ip_queue_full > ip2->ip_queue_full)
			insert = TRUE;
	}
	if (insert == FALSE && contend_count >= TOP_PORT_CONTENDERS)
		return;

	if (contend_count < TOP_PORT_CONTENDERS - 1) {
		contenders[contend_count++] = ip;
		return;
	}

	assert(lowest < TOP_PORT_CONTENDERS);
	contenders[lowest] = ip;
}


void
db_dipc_port_contention(void)
{
	
	ipc_port_t	*dpip, ip;
	unsigned int	i;

#if	DIPC_DO_STATS
	iprintf("Ports with message queue backlogs (top %d):\n",
		TOP_PORT_CONTENDERS);
	contend_count = 0;

	PORT_TABLE_WALK(dpip, ip) {
		db_dipc_port_contender(ip);
	}

	indent += 2;
	for (i = 0; i < contend_count; ++i) {
		ip = contenders[i];
		iprintf("Port 0x%x:  qfull %8d qmax %3d enq %8d thread %8d\n",
			ip, ip->ip_queue_full,
			ip->ip_dipc->dip_queue_max, ip->ip_dipc->dip_enqueued,
			ip->ip_dipc->dip_thread_hit);
	}
	indent -= 2;
#endif	/* DIPC_DO_STATS */
}


unsigned int ikot_types[IKOT_MAX_TYPE];

void
db_dipc_port_table_statistics(void)
{
	ipc_port_t	*dpip, ip;
	int		i, total, average, len, longest;
	int		l1, l2, l3;
	int		proxies, principals, zero_refs, dead_count, type;
	extern char	*ikot_print_array[];

	total = proxies = principals = zero_refs = dead_count = 0;
	for (i = 0; i < IKOT_MAX_TYPE; ++i)
		ikot_types[i] = 0;

	PORT_TABLE_WALK(dpip, ip) {
		++total;
		if (ip->ip_references == 0)
			++zero_refs;
		if (!ip_active(ip))
			++dead_count;
		if (IP_IS_REMOTE(ip)) {
			++proxies;
			continue;
		}
		++principals;
		type = ip_kotype(ip);
		if (type >= 0 && type < IKOT_MAX_TYPE)
			ikot_types[type]++;
		else
			ikot_types[IKOT_UNKNOWN]++;
	}

	iprintf("Port Table Content Summary:\n");
	indent += 2;
	iprintf("Zero Refs:\t%d\n", zero_refs);
	iprintf("Dead:\t\t%d\n", dead_count);
	iprintf("Proxies:\t\t%d\n", proxies);
	iprintf("Principals:\t%d\n", principals);
	iprintf("By Port Type:\n");
	indent += 2;
	for (i = 0; i < IKOT_MAX_TYPE; ++i)
		iprintf("%2d:  %s count:  0x%x\n",
			i, ikot_print_array[i], ikot_types[i]);
	indent -= 2;

	average = total / dipc_port_name_table_buckets;
	if (average == 0)
		average = 1;

	longest = l1 = l2 = l3 = 0;
	for (dpip=dipc_port_name_table; dpip<dipc_port_name_table_end; ++dpip) {
		len = 0;
		for (ip = *dpip; ip != IP_NULL; ip = ip->ip_hash_next)
			len++;
		if (longest < len)
			longest = len;
		if (len > average) {
			l1++;
			if (len > 2 * average) {
				l2++;
				if (len > 3 * average) {
					l3++;
				}
			}
		}
	}

	iprintf("%d ports, %d buckets, average %d per bucket\n",
		total, dipc_port_name_table_buckets,
		total / dipc_port_name_table_buckets);
	iprintf("%d chains longer than %d, %d > %d, %d > %d; longest = %d\n",
		l1, average, l2, 2 * average, l3, 3 * average, longest);
	indent -= 2;
}


void
db_dipc_port_table_report(void)
{
#if	DIPC_DO_STATS
	iprintf("Port Table Statistics:\n");
	indent += 2;
	iprintf("Entries:\t%d\n", c_dipc_uid_table_entries);
	iprintf("Lookups:\tcalls %d fast %d searches %d hits %d\n",
		c_dipc_uid_lookup_calls, c_dipc_uid_lookup_fast_calls,
		c_dipc_uid_lookup_searches, c_dipc_uid_lookup_hits);
	iprintf("Removes:\tcalls %d searches %d hits %d\n",
		c_dipc_uid_remove_calls, c_dipc_uid_remove_searches,
		c_dipc_uid_remove_hits);
#if	ROLLOVER_PARANOIA
	if (c_dipc_uid_allocate_collisions > 0)
		iprintf("*** UID ALLOCATION COLLISIONS:  %d ***\n",
			c_dipc_uid_allocate_collisions);
	else
		iprintf("No UID allocation collisions.\n");
#endif	/* ROLLOVER_PARANOIA */
	db_dipc_port_table_statistics();
	db_dipc_port_contention();
	indent -= 2;

#else	/* DIPC_DO_STATS */
	iprintf("Port Table:  No statistics available.\n");
#endif	/* DIPC_DO_STATS */
}

#endif	/* MACH_KDB */
