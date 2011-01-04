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
 * Revision 2.3.2.4  92/06/24  18:03:17  jeffreyh
 * 	Changed routines to reflect new M_ interface.
 * 	[92/06/18            sjs]
 * 
 * 	Add release logic to lock_completed.
 * 	[92/06/09            dlb]
 * 
 * Revision 2.3.2.3  92/03/28  10:13:24  jeffreyh
 * 	Changed data_write to data_write_return and deleted data_return
 * 	 method.
 * 	[92/03/20            sjs]
 * 
 * Revision 2.3.2.2  92/02/21  11:28:13  jsb
 * 	Changed reply->mobj to reply->kobj. Discard reply in lock_completed.
 * 	[92/02/16  18:24:34  jsb]
 * 
 * 	Explicitly provide name parameter to xmm_decl macro.
 * 	Changed debugging printf name and declaration.
 * 	[92/02/16  14:25:05  jsb]
 * 
 * 	Use new xmm_decl, and new memory_object_name and deallocation protocol.
 * 	[92/02/09  14:17:57  jsb]
 * 
 * Revision 2.3.2.1  92/01/21  21:54:54  jsb
 * 	De-linted. Supports new (dlb) memory object routines.
 * 	Supports arbitrary reply ports to lock_request, etc.
 * 	Converted mach_port_t (and port_t) to ipc_port_t.
 * 	[92/01/20  17:30:57  jsb]
 * 
 * Revision 2.3  91/07/01  08:26:34  jsb
 * 	Use zone for requests.
 * 	[91/06/29  15:34:20  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:40  jsb
 * 	First checkin.
 * 	[91/06/17  11:04:50  jsb]
 * 
 */
/* CMU_ENDHIST */
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
 */
/*
 *	File:	norma/xmm_split.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm layer to split multi-page lock_requests.
 */

#include <mach/vm_param.h>
#include <xmm/xmm_obj.h>
#include <xmm/xmm_methods.h>

typedef struct request *request_t;

#define	REQUEST_NULL	((request_t) 0)

struct mobj {
	struct xmm_obj	obj;
	request_t	r_head;
	request_t	r_tail;
	boolean_t	terminated;
};

struct request {
	vm_offset_t	r_offset;
	vm_size_t	r_length;
	vm_offset_t	r_start;
	vm_size_t	r_count;
	vm_size_t	r_resid;
	xmm_reply_data_t r_reply_data;
	request_t	r_next;
};

#undef  KOBJ
#define KOBJ    ((struct mobj *) kobj)

#define	m_split_init			m_interpose_init
#define	m_split_deallocate		m_interpose_deallocate
#define	m_split_copy			m_interpose_copy
#define	m_split_data_request		m_interpose_data_request
#define	m_split_data_unlock		m_interpose_data_unlock
#define	m_split_data_return		m_interpose_data_return
#define	m_split_supply_completed	m_interpose_supply_completed
#define	m_split_change_completed	m_interpose_change_completed
#define	m_split_synchronize		m_interpose_synchronize
#define	m_split_freeze			m_interpose_freeze
#define	m_split_share			m_interpose_share
#define	m_split_declare_page		m_interpose_declare_page
#define	m_split_declare_pages		m_interpose_declare_pages
#define	m_split_caching			m_interpose_caching
#define	m_split_uncaching		m_interpose_uncaching

#define	k_split_data_unavailable	k_interpose_data_unavailable
#define	k_split_get_attributes		k_interpose_get_attributes
#define	k_split_data_error		k_interpose_data_error
#define	k_split_set_ready		k_interpose_set_ready
#define	k_split_destroy			k_interpose_destroy
#define	k_split_data_supply		k_interpose_data_supply
#define	k_split_create_copy		k_interpose_create_copy
#define	k_split_uncaching_permitted	k_interpose_uncaching_permitted
#define	k_split_synchronize_completed	k_interpose_synchronize_completed

xmm_decl(split, "split", sizeof(struct mobj));

zone_t		xmm_split_request_zone;

/*
 * Forward.
 */
void	xmm_split_terminate(
		xmm_obj_t	mobj);

void
xmm_split_create(
	xmm_obj_t	old_mobj,
	xmm_obj_t	*new_mobj)
{
	xmm_obj_t mobj;

	xmm_obj_allocate(&split_class, old_mobj, &mobj);
	MOBJ->r_head = REQUEST_NULL;
	MOBJ->r_tail = REQUEST_NULL;
	MOBJ->terminated = FALSE;
	*new_mobj = mobj;
}

/*
 * Deallocate any pending request.
 */
void
xmm_split_terminate(
	xmm_obj_t	mobj)
{
	request_t r, rp;

	xmm_obj_lock(mobj);
	assert(MOBJ->terminated);
	if (MOBJ->r_head != REQUEST_NULL) {
		rp = MOBJ->r_head;
		MOBJ->r_head = REQUEST_NULL;
		xmm_obj_unlock(mobj);

		/*
		 * Look for request structures, and zfree() them.
		 */
		while (r = rp) {
			rp = r->r_next;
			zfree(xmm_split_request_zone, (vm_offset_t) r);
		}
	} else
		xmm_obj_unlock(mobj);
}

kern_return_t
k_split_lock_request(
	xmm_obj_t	kobj,
	vm_offset_t	offset,
	vm_size_t	length,
	boolean_t	should_clean,
	boolean_t	should_flush,
	vm_prot_t	lock_value,
	xmm_reply_t	reply)
{
	request_t r;
	xmm_reply_data_t split_reply_data;

#ifdef	lint
	(void) K_LOCK_REQUEST(kobj, offset, length, should_clean, should_flush,
			      lock_value, reply);
#endif	/* lint */

	xmm_obj_lock(kobj);
	if (KOBJ->terminated) {
		xmm_obj_unlock(kobj);
		return KERN_SUCCESS;
	}
	xmm_obj_unlock(kobj);

	/*
	 * If we don't have to split the request,
	 * then just pass it through.
	 */
	if (length == 0) {
		return M_LOCK_COMPLETED(kobj, offset, length, reply);
	}
	if (length <= PAGE_SIZE) {
		return K_LOCK_REQUEST(kobj, offset, length, should_clean,
				      should_flush, lock_value, reply);
	}
	if (reply != XMM_REPLY_NULL) {
		r = (request_t) zalloc(xmm_split_request_zone);
		r->r_offset = offset;
		r->r_length = length;
		r->r_start = (offset + PAGE_SIZE - 1) / PAGE_SIZE;
		r->r_count = (length + PAGE_SIZE - 1) / PAGE_SIZE;
		r->r_resid = r->r_count;
		r->r_reply_data = *reply;
		r->r_next = REQUEST_NULL;
		xmm_obj_lock(kobj);
		if (KOBJ->r_head) {
			KOBJ->r_tail->r_next = r;
		} else {
			KOBJ->r_head = r;
		}
		KOBJ->r_tail = r;
		xmm_obj_unlock(kobj);
		xmm_reply_set(&split_reply_data, XMM_SPLIT_REPLY, r);
		reply = &split_reply_data;
	}
	/*
	 *	Do multi-page lock request.  Overload the length parameter
	 *	to signal to the next layer when we start and stop the
	 *	request; the SVM layer will initiate locking to prevent
	 *	the object stack from terminating during the lock_request.
	 */
	(void) K_LOCK_REQUEST(kobj, offset, PAGE_SIZE + LOCK_REQ_START,
			      should_clean, should_flush, lock_value, reply);
	offset += PAGE_SIZE;
	length -= PAGE_SIZE;
	while (length > PAGE_SIZE) {
		xmm_obj_lock(kobj);
		if (KOBJ->terminated) {
			xmm_obj_unlock(kobj);
			return KERN_SUCCESS;
		}
		xmm_obj_unlock(kobj);
		(void) K_LOCK_REQUEST(kobj, offset, PAGE_SIZE, should_clean,
				      should_flush, lock_value, reply);
		offset += PAGE_SIZE;
		length -= PAGE_SIZE;
	}
	if (length > 0) {
		xmm_obj_lock(kobj);
		if (KOBJ->terminated) {
			xmm_obj_unlock(kobj);
			return KERN_SUCCESS;
		}
		xmm_obj_unlock(kobj);
		(void) K_LOCK_REQUEST(kobj, offset, length + LOCK_REQ_STOP,
				      should_clean, should_flush, 
				      lock_value, reply);
	} else
		assert(0);  /* shouldn't get here - will mess up start/stop */
	/* XXX check return values above??? */
	return KERN_SUCCESS;
}

kern_return_t
k_split_release_all(
	xmm_obj_t	mobj)
{
#ifdef	lint
	(void) K_RELEASE_ALL(mobj);
#endif	/* lint */
	assert(mobj->class == &split_class);

	/*
	 * Look for request structures and dequeue them.
	 */
	xmm_split_terminate(mobj);
	return K_RELEASE_ALL(mobj);
}

/*ARGSUSED*/
kern_return_t
m_split_lock_completed(
	xmm_obj_t	mobj,
	vm_offset_t	offset,
	vm_size_t	length,
	xmm_reply_t	reply)
{
	request_t r, *rp, r_prev = REQUEST_NULL, real_r;

	assert(mobj->class == &split_class);
	assert(!MOBJ->terminated);

#ifdef	lint
	(void) M_LOCK_COMPLETED(mobj, offset, length, reply);
#endif	/* lint */
	/*
	 * If reply is not a private xmm_split reply, then
	 * just pass it through.
	 */
	if (reply->type != XMM_SPLIT_REPLY) {
		return M_LOCK_COMPLETED(mobj, offset, length, reply);
	}

	/*
	 * Retrieve request pointer from reply.
	 */
	real_r = (request_t) reply->req;
	assert (real_r != REQUEST_NULL);

	/*
	 * Look for request structure to dequeue it.
	 */
	xmm_obj_lock(mobj);
	for (rp = &MOBJ->r_head; r = *rp; rp = &r->r_next) {
		if (r == real_r) {
			break;
		}
		r_prev = r;
	}

	assert(r != REQUEST_NULL);

	/*
	 * If this is the reply that we are waiting for, then
	 * send the real unsplit reply. Otherwise, just return.
	 */
	if (--r->r_resid > 0) {
		xmm_obj_unlock(mobj);
		return KERN_SUCCESS;
	}
	if (r == MOBJ->r_tail) {
		MOBJ->r_tail = r_prev;
	}
	*rp = r->r_next;
	xmm_obj_unlock(mobj);
	(void) M_LOCK_COMPLETED(mobj, r->r_offset, r->r_length, 
				&r->r_reply_data);
	zfree(xmm_split_request_zone, (vm_offset_t) r);
	return KERN_SUCCESS;
}

kern_return_t
m_split_terminate(
	xmm_obj_t	mobj,
	boolean_t	release)
{
	request_t r, rp;
	kern_return_t kr;

	assert(mobj->class == &split_class);
	assert(release == MOBJ->terminated);

#ifdef	lint
	(void) M_TERMINATE(mobj, release);
#endif	/* lint */
	if (!release) {
		xmm_obj_lock(mobj);
		MOBJ->terminated = TRUE;
		xmm_obj_unlock(mobj);
	}

	kr = M_TERMINATE(mobj, release);

	if (release && kr == KERN_SUCCESS)
		xmm_split_terminate(mobj);
	return kr;
}

void
xmm_split_init(void)
{
	xmm_split_request_zone = zinit(sizeof(struct request), 512*1024,
				       sizeof(struct request),
				       "xmm.split.request");
}

#if	MACH_KDB
void
m_split_debug(
	xmm_obj_t	mobj,
	int		flags)
{
	xmm_obj_print((db_addr_t)mobj);
	M_DEBUG(mobj, flags);
}
#endif	/* MACH_KDB */
