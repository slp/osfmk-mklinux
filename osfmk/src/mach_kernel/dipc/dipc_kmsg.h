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
 *	File:	dipc/dipc_kmsg.h
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Definitions for distributed kernel messages.
 */

#ifndef	_DIPC_DIPC_KMSG_H_
#define	_DIPC_DIPC_KMSG_H_

#include <mach_kdb.h>

#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <vm/vm_map.h>

/*
 *	The meta_kmsg is a placeholder for a remote kmsg.
 *	A message queue may contain an arbitrary mixture of
 *	local (real) kmsgs and remote (meta) kmsgs.  The meta_kmsg
 *	contains all the information necessary to retrieve the
 *	remote kmsg.  However, the meta_kmsg is significantly
 *	smaller than a real kmsg, as no message body follows
 *	the meta_kmsg.
 *
 *	The meta_kmsg definition must closely match that of
 *	the original kmsg in ipc/ipc_kmsg.h.  A meta_kmsg is
 *	distinguished from a kmsg by examining the
 *	MACH_MSGH_BITS_META_KMSG field in the msgh_bits field,
 *	which is a common field between	both structures.
 *
 *	The meta_kmsg obviously requires the next and previous
 *	pointers that link all kmsgs together and the ikm_size.
 *	While typed IPC code remains in the kernel, the meta_kmsg
 *	must waste the same amount of space wasted by a regular
 *	kmsg on the ikm_version field.
 *
 *	The meta_kmsg also must have the same msgh_bits used by
 *	the kmsg; in particular, this field contains the required
 *	bit (MACH_MSGH_BITS_META_KMSG) allowing the kernel to
 *	distinguish between meta_kmsg and kmsg.
 *
 *	However, unlike an ordinary kmsg, the meta_kmsg does *not*
 *	include a full-blown mach_msg_header_t.  Instead, the
 *	meta_kmsg must have the bitflag field of the Mach message
 *	(which also contains bitfields controlled by the kernel,
 *	not by the user) and the destination port to link the meta_kmsg
 *	to its (local) destination port.  The destination port
 *	must be known when manipulating meta_kmsgs in a port set;
 *	the destination port also stores the port's UID.  Unfortunately,
 *	there is no way to eliminate the mkm_size field, which must
 *	overlay the ikm_header.msgh_size, and is essentially unused
 *	in the meta_kmsg.
 *
 *	The handle_t linking the meta_kmsg to the sender's kmsg is
 *	contained in both the meta_kmsg and ksmg proper.
 *
 *	When the remote kmsg is retrieved and replaces the meta_kmsg,
 *	all information required for the kmsg (such as the destination
 *	port) is extracted from the meta_kmsg and stored in the kmsg.
 *
 *	When retrieving out-of-line ports or memory, a msg_state
 *	structure is attached to the real kmsg to describe the
 *	transmission/reception.  Because the message has been removed
 *	from the mqueue by this time, we can use one of the message
 *	queue links to store the msg_progress pointer.
 */

typedef struct meta_kmsg {
	struct ipc_kmsg	*mkm_next, *mkm_prev;
	vm_size_t	mkm_size;
	unsigned int	mkm_fill;	/* overlays ikm_header.ikm_private */
	handle_t	mkm_handle;	/* overlays ikm_header.ikm_handle */
	mach_msg_bits_t	mkm_msgh_bits;	/* first field in ikm_header */
	mach_msg_size_t	mkm_msgh_size;	/* overlays ikm_header.msgh_size */
	ipc_port_t	mkm_remote_port; /* overlays ikm_hdr...remote_port */
} *meta_kmsg_t;


/*
 *	Meta-kmsg shortcuts.
 */

#define MKM(mkm)		((meta_kmsg_t)mkm)

#define	MKM_NULL		((meta_kmsg_t) 0)

#define	ikm_msg_state		((msg_state_t) ikm_prev)

#define	_IKMBITS(km)		((km)->ikm_header.msgh_bits)

/*
 *	Determine whether the entity is a kmsg or a meta-kmsg.
 *	The definitions for the two structures are common at the
 *	beginning, so it is valid to cast either into a pointer
 *	to a kmsg to find the bit field.
 */
#define	KMSG_IS_META(xmsg)	(_IKMBITS((ipc_kmsg_t)(xmsg)) \
				 & MACH_MSGH_BITS_META_KMSG)

/*
 *	Shorthand checks for interesting kmsg cases.
 */
#define	KMSG_IS_DIPC_FORMAT(km)	(_IKMBITS(km) & MACH_MSGH_BITS_DIPC_FORMAT)
#define	KMSG_IS_MIGRATING(km)	(_IKMBITS(km) & MACH_MSGH_BITS_MIGRATING)
#define	KMSG_HAS_WAITING(km)	(_IKMBITS(km) & MACH_MSGH_BITS_SENDER_WAITING)
#define	KMSG_PLACEHOLDER(km)	(_IKMBITS(km) & MACH_MSGH_BITS_PLACEHOLDER)
#define	KMSG_IN_DIPC(km)	(_IKMBITS(km) & (MACH_MSGH_BITS_META_KMSG |   \
						 MACH_MSGH_BITS_PLACEHOLDER | \
						 MACH_MSGH_BITS_DIPC_FORMAT))
#define	KMSG_COMPLEX_OOL(km)	(_IKMBITS(kmsg) & MACH_MSGH_BITS_COMPLEX_OOL)


/*
 *	Determine send- or recv-side state of a kmsg.
 */
#define	KMSG_RECEIVING(km)	(_IKMBITS(km) & MACH_MSGH_BITS_RECEIVING)
#define	KMSG_MARK_RECEIVING(km)	(_IKMBITS(km) |= MACH_MSGH_BITS_RECEIVING)
#define	KMSG_CLEAR_RECEIVING(km)(_IKMBITS(km) &= ~MACH_MSGH_BITS_RECEIVING)

/*
 *	Determine whether a kmsg has (or should have) a handle
 *	on the receive-side of the world.  This assertion is
 *	meaningless on the send-side, as the handle will be
 *	assigned dynamically during message transmission.
 */
#define	KMSG_HAS_HANDLE(km)	(_IKMBITS(km) & MACH_MSGH_BITS_HAS_HANDLE)
#define	KMSG_MARK_HANDLE(km)	(_IKMBITS(km) |= MACH_MSGH_BITS_HAS_HANDLE)
#define	KMSG_CLEAR_HANDLE(km)	(_IKMBITS(km) &= ~MACH_MSGH_BITS_HAS_HANDLE)


/*
 *	Drop a handle belonging to a kmsg.
 */
#define	KMSG_DROP_HANDLE(kmsg, handle)					\
MACRO_BEGIN								\
	kkt_return_t	drop_kktr;					\
									\
	assert(KMSG_HAS_HANDLE(kmsg));					\
	drop_kktr = KKT_HANDLE_FREE(handle);				\
	assert(drop_kktr == KKT_SUCCESS);				\
	KMSG_CLEAR_HANDLE(kmsg);					\
	(kmsg)->ikm_handle = HANDLE_NULL;				\
MACRO_END


/*
 * This assumes you have a valid handle
 */

#define	KMSG_DIPC_TRAILER(km)	((mach_msg_dipc_trailer_t *)		   \
				 ((unsigned int)(&(km->ikm_header)) +	   \
				  round_msg((unsigned int)		   \
					    (km->ikm_header.msgh_size))))

/*
 * The msg_progress pointer is stored in different places for send and
 * receive kmsgs.  msgh_seqno in the trailer is unused on the send side; 
 * on the receive side, by the time copyout is called, the message has
 * been dequeued and so ikm_prev is no longer needed.
 */
#define KMSG_MSG_PROG_SEND(km)	((msg_progress_t)			   \
				 KMSG_DIPC_TRAILER(km)->msgh_seqno)

#define KMSG_MSG_PROG_SEND_SET(km, val)	\
    KMSG_DIPC_TRAILER(km)->msgh_seqno = (mach_port_seqno_t)(val)

#define KMSG_MSG_PROG_RECV(km)	((msg_progress_t) (km)->ikm_prev)

#define KMSG_MSG_PROG_RECV_SET(km, val)	\
    (km)->ikm_prev = (struct ipc_kmsg *)(val)


/*
 *	DIPC provides an optimized delivery path for small
 *	messages -- when conditions are favorable, DIPC
 *	can deliver these messages with less effort.
 *
 *	This variable is the size of the largest inline
 *	message, in bytes that can benefit from this
 *	optimization.  Note that this value may be
 *	lowered at boot-time depending on what KKT
 *	reports for its largest buffer size.
 */
#define	DIPC_FAST_KMSG_SIZE_MAX	256


extern	dipc_return_t	dipc_kmsg_copyin(
				ipc_kmsg_t		kmsg);

extern	dipc_return_t	dipc_kmsg_copyout(
				ipc_kmsg_t		kmsg,
				vm_map_t		map,
				mach_msg_body_t		*slist);

extern	void		dipc_kmsg_release(
				ipc_kmsg_t		kmsg);

extern	void		dipc_kmsg_release_delayed(
				ipc_kmsg_t		kmsg);

extern	boolean_t	dipc_kmsg_clean(
				ipc_kmsg_t		kmsg);

#if	MACH_KDB
extern	void		dipc_descriptor_print(
				mach_msg_descriptor_t	*descrip);

extern void		dipc_kmsg_print(
				ipc_kmsg_t		kmsg);
#endif	/* MACH_KDB */


#endif	/* _DIPC_DIPC_KMSG_H_ */
