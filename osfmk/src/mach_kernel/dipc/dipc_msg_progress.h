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
 *	File:	dipc/dipc_msg_progress.h
 *	Author:	Steve Sears
 *	Date:	1994
 *
 *	Necessary data structures to transmit/receive
 *	a kmsg.
 */

#ifndef	_DIPC_DIPC_MSG_PROGRESS_H
#define	_DIPC_DIPC_MSG_PROGRESS_H

#include <dipc_timer.h>

#include <kern/macro_help.h>
#include <mach/kern_return.h>
#include <mach/kkt_request.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <ipc/ipc_kmsg.h>
#include <kern/lock.h>

/*
 * maximum number of request blocks in a chain, max number of chains.
 */
extern unsigned int	dipc_chain_size;
#define	CHAINSIZE	dipc_chain_size
#define	MAXCHAINS	2

/*
 * A dipc_copy_list is used to link together all of the copy
 * objects that represent OOL regions to be sent.
 */
typedef struct dipc_copy_list	*dipc_copy_list_t;

struct dipc_copy_list {
	dipc_copy_list_t	next;
	vm_map_copy_t		copy;
	unsigned int		flags;
};

#define	DIPC_COPY_LIST_NULL	((dipc_copy_list_t) 0)

/* dipc_copy_list flags */
#define	DCL_OVERWRITE		1	/* overwrite optimization for copy_t */

/*
 * A prep_pages_queue is a ring buffer of vm_page_ts.  When
 * a request is to be sent, the vm_page_t is extracted and 
 * pinned before sending.  A non-full queue will be replenished
 * from thread context by the sending thread; the interrupt
 * routine will awaken it.
 *
 * The fill_lock prevents more than one thread from trying to fill
 * the queue at once; it is a simple lock that is only accessed with
 * simple_lock_try to allow a single thread to insert pages into the
 * queue, but other threads can simultaneously extract pages.
 *
 * The prep_pages_queue has a variable length, so that it may be
 * tuned at boot-time.  This means that structures containing a
 * prep_pages_queue must themselves have a variable length!  The
 * code that sizes these structures can be written fairly portably,
 * as long as the last element of the prep_pages_queue is the one
 * of variable size, and it is always named "page", and always
 * has a declared size of 1.  Then when sizing a prep_pages_queue,
 * add in sizeof(page[0])*NELEM-1 (the -1 subtracts off the space
 * already allocated for page[0]).
 */
#define	PREP_MAX	dipc_prep_pages_max
#define	PREP_LOW_WATER	dipc_prep_pages_low_water
extern unsigned int	dipc_prep_pages_max;
extern unsigned int	dipc_prep_pages_low_water;

struct prep_pages_queue {
	int		ins;
	int		ext;
	int		size;
	int		low_water;
	boolean_t	pslock;
	decl_simple_lock_data(,fill_lock)
	decl_simple_lock_data(,lock)
	vm_page_t	page[1];	/* dynamically sized */
};

#define	fill_lock_lock(q)	simple_lock_try(&(q)->fill_lock)
#define	fill_lock_unlock(q)	simple_unlock(&(q)->fill_lock)

typedef struct prep_pages_queue	*prep_pages_queue_t;

#define	prep_queue_full(p)	(p->ins == p->ext - 1 || 		\
				 (p->ext == 0 && p->ins == p->size - 1))
#define	prep_queue_empty(p)	(p->ins == p->ext)
#define	prep_queue_force_empty(p)	(p->ins = p->ext = 0)
#define	prep_insert_page(q, p)						\
MACRO_BEGIN								\
	q->page[q->ins++] = p;						\
	if (q->ins >= q->size)						\
		q->ins = 0;						\
MACRO_END
#define	prep_next_page(q)	((q->ins != q->ext) ?			\
				 q->page[q->ext] : VM_PAGE_NULL)

#define	prep_next_page_v(q, l, m)					\
MACRO_BEGIN								\
	int ptr;							\
	prep_queue_length(q, ptr);					\
	if (ptr < l)							\
		m = VM_PAGE_NULL;					\
	ptr = q->ext + l;						\
	if (ptr >= q->size)						\
		ptr -= q->size;						\
	m = q->page[ptr];						\
MACRO_END

/*
 *	Compute the number of prep'd pages remaining in
 *	the queue.  Should be called while holding the
 *	prep queue lock.
 */
#define prep_queue_length(p,l)                                          \
MACRO_BEGIN                                                             \
        if (prep_queue_empty(p))                                        \
                l = 0;                                                  \
	else {								\
		l = p->ins - p->ext;					\
		if (l < 0)						\
			l += p->size;					\
	}								\
MACRO_END

/*
 * page error information.
 */

typedef struct page_error {
	unsigned int		entry;		/* ordinal error entry */
	vm_offset_t		offset;		/* page offset */
	kern_return_t		error;		/* error type */
} page_error;

typedef struct page_err_header	page_err_header_t;

struct page_err_header {
	page_err_header_t	*next;		/* link multiple pages */
	int			count;		/* count of errors */
};

/*
 *	XXX should be sized from PAGE_SIZE... there's a
 *	problem here, can't allocate arrays from a variable.
 */
#define	PAGE_ERR_MAX	((8192 - sizeof(struct page_err_header)) / sizeof(struct page_error))

struct page_err_page {
	page_err_header_t	header;
	page_error		error_vec[PAGE_ERR_MAX];
};


struct ool_marker {
	dipc_copy_list_t	copy_list;
	vm_map_entry_t		entry;
	vm_offset_t		offset;
	vm_size_t		size;
	vm_offset_t		vaddr;
	int			entry_count;	/* ordinal entry */
};

/*
 * msg_progress state definitions
 */
#define	MP_S_START	0	/* initialized state */
#define	MP_S_READY	1	/* ready for transmission */
#define	MP_S_OOL_DONE	2	/* entire ool component of msg done */
#define	MP_S_ERROR_DONE	3	/* page error info has been sent/received */
#define	MP_S_MSG_DONE	4	/* entire message done */

/*
 * msg_progress flags definitions
 */
#define	MP_F_RECV	 0x001	/* on if receiving */
#define	MP_F_INIT	 0x002	/* msg_progress has been initialized */
#define	MP_F_THR_AVAIL	 0x004	/* snd/rcv thread available for thr-context */
#define	MP_F_PREP_QUEUE	 0x008	/* mp is enqueued on dipc_msg_prep_queue */
#define	MP_F_QUEUEING	 0x010	/* set so we can drop mp->lock */
#define	MP_F_IDLE	 0x020	/* one or both chains idle */
#define MP_F_PREP_DONE	 0x040	/* entire ool component has been prepped */
#define MP_F_KKT_ERROR	 0x080	/* transport error has occurred during xmit */
#define MP_F_SENDING_THR 0x100	/* sending thr available to free kmsg */
#define MP_F_OOL_IN_PROG 0x200	/* ool engine still running */

typedef struct msg_progress	*msg_progress_t;
extern unsigned int		msg_progress_size; /* size of mp structure */

struct msg_progress {
	msg_progress_t		prep_next;	/* for asynch prep queue */
	ipc_kmsg_t		kmsg;
	decl_simple_lock_data(,lock)
	unsigned short		state;
	unsigned short		flags;
	int			chain_count;
	int			idle_count;
	request_block_t		idle_chain[MAXCHAINS];
	unsigned int		msg_size;	/* OOL data size */
	struct page_err_page	*page_err_info;
	unsigned int		error_count;	/* count of error pages */
	dipc_copy_list_t	copy_list;
	struct ool_marker	prep;
	struct ool_marker	pin;
#if	DIPC_TIMER
	unsigned int		timer_key;
#endif	/* DIPC_TIMER */
	struct prep_pages_queue	prep_queue; /* variable-sized:  must be last! */
};

#define	MSG_PROGRESS_NULL	((msg_progress_t) 0)

extern msg_progress_t		msg_progress_allocate(
					boolean_t		wait);

extern void			msg_progress_deallocate(
					msg_progress_t		mp);

extern	dipc_copy_list_t	dipc_copy_list_allocate(
					boolean_t		wait);

extern	void			dipc_copy_list_deallocate(
					dipc_copy_list_t	dp);


#endif	/* _DIPC_DIPC_MSG_PROGRESS_H */
