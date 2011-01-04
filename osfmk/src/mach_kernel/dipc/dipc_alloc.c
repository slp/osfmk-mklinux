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
 *	File:	dipc/dipc_alloc.c
 *	Author:	Michelle Dominijanni
 *	Date:	1994
 *
 *	Allocation and deallocation routines for
 *	DIPC resources such as kmsgs and meta-kmsgs.
 */

/*
 * DIPC alloc/free routines.
 */
#include <mach_kdb.h>
#include <norma_scsi.h>
#include <dipc_xkern.h>

#include <kern/assert.h>
#include <mach/kkt_request.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <dipc/dipc_counters.h>
#include <kern/lock.h>
#include <kern/zalloc.h>
#include <vm/vm_kern.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <ddb/tr.h>

/*
 *	Maximum number of elements the system will use
 *	for meta_kmsg structures.  This value controls
 *	the number of messages that can simultaneously
 *	be queued on ports waiting for reception.  When
 *	the system runs out of these, inbound message
 *	traffic stops.
 */
#define	META_KMSG_ZONE_MAX_ELEM	1000
int meta_kmsg_zone_max_elem = META_KMSG_ZONE_MAX_ELEM;

/*
 *	Maximum number of elements the system will use
 *	to receive kmsgs directly at interrupt level.
 *	This optimization permits an incoming message
 *	to be received off the wire, directly into a
 *	kmsg buffer suitable to be enqueued directly
 *	on a port (in thread context) or even dropped
 *	directly into a waiting receiver (from interrupt
 *	context).
 */
#define	DIPC_FAST_KMSG_ZONE_MAX_ELEM	1000
int dipc_fast_kmsg_zone_max_elem = DIPC_FAST_KMSG_ZONE_MAX_ELEM;

/*
 *	Maximum size of a kmsg that can be received
 *	in the fastest DIPC delivery path.
 *
 *	For transports using interrupt-level delivery,
 *	dipc_fast_kmsg_size is the size of the largest message
 *	that can fit in an entry allocated from the
 *	fast_kmsg_zone.  For transports using thread-context
 *	delivery, dipc_fast_kmsg_size is the size of the largest
 *	message the transport is willing to have kalloc'd in
 *	the delivery path.
 *
 *	This variable is set to a default value here but may
 *	be overridden by kkt_bootstrap_init before the rest
 *	of DIPC initializes itself.  This value is extremely
 *	transport specific and should not be altered lightly.
 */
#if	NORMA_SCSI
#define	INTR_KMSG_SIZE		290
#else	/* NORMA_SCSI */
#define	INTR_KMSG_SIZE		192
#endif	/* NORMA_SCSI */
unsigned int dipc_fast_kmsg_size = INTR_KMSG_SIZE;

/*
 *	Maximum number of elements the system will use
 *	for sending and receiving transport (KKT) requests.
 *	Given that the zone is expandable, this value isn't
 *	very important.  More attention should be paid to
 *	the *minimum* value.
 */
#define	KKT_REQ_ZONE_MAX_ELEM	1000
int kkt_req_zone_max_elem = KKT_REQ_ZONE_MAX_ELEM;

/*
 *	Minimum number of pre-allocated elements the
 *	system will use for sending and receiving transport
 *	(KKT) requests.  Additional elements will be
 *	allocated on demand, although in that case the
 *	caller may be blocked while memory allocation occurs.
 */
#define	KKT_REQ_ZONE_MIN_ELEM	1000
int kkt_req_zone_min_elem = KKT_REQ_ZONE_MIN_ELEM;

/*
 *
 */
#define	COPY_LIST_ZONE_MAX_ELEM	50
int copy_list_zone_max_elem = COPY_LIST_ZONE_MAX_ELEM;

/*
 *
 */
#define	COPY_LIST_ZONE_MIN_ELEM	50
int copy_list_zone_min_elem = COPY_LIST_ZONE_MIN_ELEM;

/*
 *
 */
#define	MSG_PROGRESS_ZONE_MAX_ELEM	50
int msg_progress_zone_max_elem = MSG_PROGRESS_ZONE_MAX_ELEM;

/*
 *
 */
#define	MSG_PROGRESS_ZONE_MIN_ELEM	50
int msg_progress_zone_min_elem = MSG_PROGRESS_ZONE_MIN_ELEM;

decl_simple_lock_data(,dipc_cleanup_lock_data)

#define	dipc_cleanup_lock()	simple_lock(&dipc_cleanup_lock_data)
#define	dipc_cleanup_unlock()	simple_unlock(&dipc_cleanup_lock_data)

struct ipc_kmsg_queue	dipc_kmsg_destroy_queue;
struct ipc_kmsg_queue	dipc_kmsg_release_queue;

dstat_decl(unsigned int	c_dkdq_length = 0;)
dstat_decl(unsigned int	c_dkdq_enqueued = 0;)
dstat_decl(unsigned int	c_dkdq_handled = 0;)
dstat_decl(unsigned int	c_dkdq_max_length = 0;)

dstat_decl(unsigned int	c_dkrq_length = 0;)
dstat_decl(unsigned int	c_dkrq_enqueued = 0;)
dstat_decl(unsigned int	c_dkrq_handled = 0;)
dstat_decl(unsigned int	c_dkrq_max_length = 0;)

zone_t	kkt_req_zone;
zone_t	fast_kmsg_zone;
zone_t	meta_kmsg_zone;
zone_t  copy_list_zone;
zone_t	msg_progress_zone;

/*
 *	Size of a struct msg_progress -- this size changes
 *	based on the size of the prep_pages_queue, which
 *	is tunable at boot-time.
 */
unsigned int	msg_progress_size;

void	dipc_cleanup_thread(void);

#if	PORTABLE_SPLKKT_HELD
/*
 *	Spl value for splkkt.
 */
int	portable_splkkt_level = 0;
#endif	/* PORTABLE_SPLKKT_HELD */


/*
 * Initialize the zones used by the allocation routines.
 * Called from dipc_bootstrap().
 */
void
dipc_alloc_init(void)
{
	vm_size_t	meta_kmsg_zone_max;
	vm_size_t	fast_kmsg_zone_max;
	vm_size_t	kkt_req_zone_max;
	vm_size_t	kkt_req_zone_min;
	vm_size_t	copy_list_zone_max;
	vm_size_t	copy_list_zone_min;
	vm_size_t	msg_progress_zone_max;
	vm_size_t	msg_progress_zone_min;
	msg_progress_t	mp;
	int		nalloc;
	extern boolean_t	dipc_kkt_transport_threads;
#if	PORTABLE_SPLKKT_HELD
	int		cur_ipl;
#endif	/* PORTABLE_SPLKKT_HELD */

#if	PORTABLE_SPLKKT_HELD
	cur_ipl = splkkt();
	portable_splkkt_level = splhigh();
	splx(cur_ipl);
#endif	/* PORTABLE_SPLKKT_HELD */

	/*
	 * the meta_kmsg_zone and the fast_kmsg_zone are both non-expandable
	 * and non-pageable since both are used from interrupt level.  Allocate
	 * the maximum number of elements now and cram it into each zone.
	 */
	meta_kmsg_zone_max = (vm_size_t) meta_kmsg_zone_max_elem *
		sizeof(struct meta_kmsg);
	meta_kmsg_zone = zinit(sizeof(struct meta_kmsg), meta_kmsg_zone_max,
				meta_kmsg_zone_max, "meta kmsgs");
	zone_change(meta_kmsg_zone, Z_EXHAUST, TRUE);
	zone_change(meta_kmsg_zone, Z_COLLECT, FALSE);
	zone_change(meta_kmsg_zone, Z_EXPAND, FALSE);
#if !DIPC_XKERN
	zone_enable_spl(meta_kmsg_zone, splkkt);
#endif /* !DIPC_XKERN */
	nalloc = zfill(meta_kmsg_zone, meta_kmsg_zone_max_elem);
	assert(nalloc >= meta_kmsg_zone_max_elem);

	if (dipc_kkt_transport_threads) {
		fast_kmsg_zone = ZONE_NULL;
	} else {
		fast_kmsg_zone_max = (vm_size_t) dipc_fast_kmsg_zone_max_elem *
		    dipc_fast_kmsg_size;
		fast_kmsg_zone = zinit(dipc_fast_kmsg_size, fast_kmsg_zone_max,
				       fast_kmsg_zone_max, "intr kmsgs");
		zone_change(fast_kmsg_zone, Z_EXHAUST, TRUE);
		zone_change(fast_kmsg_zone, Z_COLLECT, FALSE);
		zone_change(fast_kmsg_zone, Z_EXPAND, FALSE);
#if !DIPC_XKERN
		zone_enable_spl(fast_kmsg_zone, splkkt);
#endif /* !DIPC_XKERN */
		nalloc = zfill(fast_kmsg_zone, dipc_fast_kmsg_zone_max_elem);
		assert(nalloc >= dipc_fast_kmsg_zone_max_elem);
	}

	/*
	 * The kkt_req_zone is expandable and collectable.  Allocate the
	 * minimum number of elements and cram them into the zone.  These
	 * pages won't be garbage collected.
	 */
	kkt_req_zone_max = (vm_size_t) kkt_req_zone_max_elem *
		sizeof(struct request_block);

	kkt_req_zone_min = (vm_size_t) kkt_req_zone_min_elem *
		sizeof(struct request_block);
	kkt_req_zone = zinit(sizeof(struct request_block), kkt_req_zone_max,
		sizeof(struct request_block), "kkt request blocks");
	nalloc = zfill(kkt_req_zone, kkt_req_zone_min_elem);
	assert(nalloc >= kkt_req_zone_min_elem);
#if !DIPC_XKERN
	zone_enable_spl(kkt_req_zone, splkkt);
#endif /* !DIPC_XKERN */

	/*
	 * The copy_list_zone is expandable and collectable
	 */

	copy_list_zone_min = (vm_size_t) copy_list_zone_min_elem *
		sizeof(struct dipc_copy_list);
	copy_list_zone = zinit(sizeof(struct dipc_copy_list), 
			       copy_list_zone_max,
			       sizeof(struct dipc_copy_list), 
			       "copy_list structs");
	nalloc = zfill(copy_list_zone, copy_list_zone_min_elem);
	assert(nalloc >= copy_list_zone_min_elem);
#if !DIPC_XKERN
	zone_enable_spl(copy_list_zone, splkkt);
#endif /* !DIPC_XKERN */

	/*
	 * Compute size of a msg_progress structure.  The sizeof(*mp)
	 * already includes space for mp->page[0], which is why we
	 * subtract one from PREP_MAX.
	 */
	mp = MSG_PROGRESS_NULL;		/* don't actually USE this pointer! */
	msg_progress_size = sizeof(*mp) + (sizeof(mp->prep_queue.page[0])
					   * (PREP_MAX-1));

	/*
	 * The msg_progress_zone is expandable and collectable
	 */

	msg_progress_zone_min = (vm_size_t) msg_progress_zone_min_elem *
		msg_progress_size;
	msg_progress_zone = zinit(msg_progress_size,
			       msg_progress_zone_max,
			       msg_progress_size, 
			       "msg_progress structs");
	nalloc = zfill(msg_progress_zone, msg_progress_zone_min_elem);
	assert(nalloc >= msg_progress_zone_min_elem);

	simple_lock_init(&dipc_cleanup_lock_data, ETAP_DIPC_CLEANUP);
	ipc_kmsg_queue_init(&dipc_kmsg_destroy_queue);
	ipc_kmsg_queue_init(&dipc_kmsg_release_queue);

	(void) kernel_thread(kernel_task, dipc_cleanup_thread, (char *) 0);
}

vm_offset_t
dipc_alloc(void		*opaque,
	   vm_size_t	size)
{
	return (kalloc(size));
}

void
dipc_free(void		*opaque,
	  vm_offset_t	data,
	  vm_size_t	size)
{
	kfree(data, size);
}

msg_progress_t
msg_progress_allocate(
		      boolean_t		wait)
{
	msg_progress_t	mp;
	int		i;

	if (wait == TRUE)
		mp = (msg_progress_t)zalloc(msg_progress_zone);
	else {
		panic("no way Jose");
		mp = (msg_progress_t)zget(msg_progress_zone);
	}
	if (mp) {
		/*
		 *	Initialize everything by hand rather than
		 *	with bzero, to avoid wasting time zeroing
		 *	large numbers of vm_page_t entries in the
		 *	prep queue.
		 *
		 *	Not entirely sure which fields are assumed
		 *	by caller to be initialized, so zero everything.
		 */
		mp->prep_next = MSG_PROGRESS_NULL;
		mp->kmsg = IKM_NULL;
		mp->flags = 0;
		mp->chain_count = 0;
		mp->idle_count = 0;
		for (i = 0; i < MAXCHAINS; ++i)
			mp->idle_chain[i] = NULL_REQUEST;
		mp->msg_size = 0;
		mp->page_err_info = (struct page_err_page *) 0;
		mp->error_count = 0;
		mp->copy_list = DIPC_COPY_LIST_NULL;

		mp->prep.copy_list = DIPC_COPY_LIST_NULL;
		mp->prep.entry = VM_MAP_ENTRY_NULL;
		mp->prep.offset = 0;
		mp->prep.size = 0;
		mp->prep.vaddr = 0;
		mp->prep.entry_count = 0;

		mp->pin.copy_list = DIPC_COPY_LIST_NULL;
		mp->pin.entry = VM_MAP_ENTRY_NULL;
		mp->pin.offset = 0;
		mp->pin.size = 0;
		mp->pin.vaddr = 0;
		mp->pin.entry_count = 0;

		mp->prep_queue.ins = 0;
		mp->prep_queue.ext = 0;
		mp->prep_queue.pslock = 0; /* XXX */

		mp->prep_queue.size = PREP_MAX;
		mp->prep_queue.low_water = PREP_LOW_WATER;
		mp->state = MP_S_START;
		simple_lock_init(&mp->lock, ETAP_DIPC_MSG_PROG);
		simple_lock_init(&mp->prep_queue.fill_lock,ETAP_DIPC_PREP_FILL);
		simple_lock_init(&mp->prep_queue.lock, ETAP_DIPC_PREP_QUEUE);
		/* don't bother initializing contents of prep_queue array */
	}
	return (mp);
}

void
msg_progress_deallocate(
			msg_progress_t		mp)
{
	dipc_copy_list_t ncl, cl = mp->copy_list;

	/*
	 *	These assertions courtesy of people who don't yet
	 *	believe in page errors.
	 */
	assert(mp->state == MP_S_MSG_DONE);
	assert(mp->error_count == 0);
	assert(!mp->page_err_info);

	while (cl != DIPC_COPY_LIST_NULL) {
		ncl = cl;
		cl = cl->next;
		dipc_copy_list_deallocate(ncl);
	}
	zfree(msg_progress_zone, (vm_offset_t)mp);
}

dipc_copy_list_t
dipc_copy_list_allocate(
			boolean_t		wait)
{
	dipc_copy_list_t cl;

	if (wait == TRUE)
	    cl = (dipc_copy_list_t)zalloc(copy_list_zone);
	else {
		panic("no way Jose");
		cl = (dipc_copy_list_t)zget(copy_list_zone);
	}
	cl->flags = 0;
	return (cl);
}

void
dipc_copy_list_deallocate(
			  dipc_copy_list_t	dp)
{
	zfree(copy_list_zone, (vm_offset_t)dp);
}


/*
 * allocate a request block from the request block zone.
 * caller must initialize the request.
 */
request_block_t
dipc_get_kkt_req(
	boolean_t wait)
{
	request_block_t req;

	if (wait == TRUE)
		req = (request_block_t)zalloc(kkt_req_zone);
	else
		req = (request_block_t)zget(kkt_req_zone);
	return req;
}

/*
 * return a request to the request block zone.
 */
void
dipc_free_kkt_req(
	request_block_t req)
{
	unsigned int	s;

	zfree(kkt_req_zone, (vm_offset_t)req);
}

request_block_t
dipc_get_kkt_req_chain(
	int		req_count,	/* count of requests */
	unsigned int	req_type)	/* READ or WRITE */
{
	request_block_t	req, base;

	/*
	 * get the first request block and initialized it - use it
	 * as a template for the rest.
	 */
	req = dipc_get_kkt_req(TRUE);
	bzero((char *)req, sizeof(struct request_block));
	req->data_length = PAGE_SIZE;
	req->request_type = req_type;
	base = req;

	for (req_count--; req_count > 0; req_count--) {
		req->next = dipc_get_kkt_req(TRUE);
		req = req->next;
		*req = *base;
	}
	req->next = NULL_REQUEST;

	return base;
}

void
dipc_free_kkt_req_chain(
	request_block_t	req)
{
	request_block_t	nreq;

	unsigned int    s;
	s = splkkt();
	while (req) {
		nreq = req->next;
		zfree(kkt_req_zone, (vm_offset_t)req);
		req = nreq;
	}
	splx(s);
}

/*
 * Called from interrupt context to queue up a kmsg to be destroyed
 * by the dipc_cleanup thread.
 */
void
dipc_kmsg_destroy_delayed(
	ipc_kmsg_t	kmsg)
{
	spl_t	s;

	s = splkkt();
	dipc_cleanup_lock();
	ipc_kmsg_enqueue_macro(&dipc_kmsg_destroy_queue, kmsg);
	dstat(++c_dkdq_length);
	dstat(++c_dkdq_enqueued);
	dipc_cleanup_unlock();
	splx(s);
	thread_wakeup((event_t)dipc_cleanup_thread);
}

/*
 * Called from interrupt context to queue up a kmsg to be released
 * by the dipc_cleanup thread.
 */
void
dipc_kmsg_release_delayed(
	ipc_kmsg_t	kmsg)
{
	spl_t	s;

	TR_DECL("d_kmsg_reldel");

	tr2("kmsg 0x%x", kmsg);
	s = splkkt();
	dipc_cleanup_lock();
	ipc_kmsg_enqueue_macro(&dipc_kmsg_release_queue, kmsg);
	dstat(++c_dkrq_length);
	dstat(++c_dkrq_enqueued);
	assert(kmsg->ikm_next != 0);
	assert(kmsg->ikm_next != IKM_BOGUS);
	assert(kmsg->ikm_prev != 0);
	assert(kmsg->ikm_prev != IKM_BOGUS);
	dipc_cleanup_unlock();
	splx(s);
	thread_wakeup((event_t)dipc_cleanup_thread);
}


void
dipc_cleanup_thread(void)
{
	ipc_kmsg_t	kmsg;
	spl_t	s;
	TR_DECL("dipc_cleanup_thread");

	for(;;) {
		tr_start();
		tr3("enter: release: 0x%x destroy: 0x%x",
		    *((int *)&dipc_kmsg_release_queue),
		    *((int *)&dipc_kmsg_destroy_queue));
		s = splkkt();
		dipc_cleanup_lock();
		kmsg = ipc_kmsg_dequeue(&dipc_kmsg_release_queue);
		while (kmsg != IKM_NULL) {
			dstat_max(c_dkrq_max_length, c_dkrq_length);
			dstat(--c_dkrq_length);
			dstat(++c_dkrq_handled);
			dipc_cleanup_unlock();
			splx(s);
			dipc_kmsg_release(kmsg);
			s = splkkt();
			dipc_cleanup_lock();
			kmsg = ipc_kmsg_dequeue(&dipc_kmsg_release_queue);
		}
		kmsg = ipc_kmsg_dequeue(&dipc_kmsg_destroy_queue);
		while (kmsg != IKM_NULL) {
			dstat_max(c_dkdq_max_length, c_dkdq_length);
			dstat(--c_dkdq_length);
			dstat(++c_dkdq_handled);
			dipc_cleanup_unlock();
			splx(s);
			ipc_kmsg_destroy(kmsg);
			s = splkkt();
			dipc_cleanup_lock();
			kmsg = ipc_kmsg_dequeue(&dipc_kmsg_destroy_queue);
		}
		assert_wait((event_t)dipc_cleanup_thread, FALSE);
		dipc_cleanup_unlock();
		splx(s);
		tr1("exit");
		tr_stop();
		thread_block((void (*)(void)) 0);
	}
}

/*
 * When called from thread context, try to allocate a kmsg in the
 * usual way; in this case, the routine may block.
 *
 * When called from interrupt context, try to allocate a kmsg from
 * the interrupt kmsg zone; in this case, the routine does not block.
 * Note that in this case the size argument is ignored -- the caller
 * had better be prepared to deal with an interrupt-level kmsg!
 *
 * The kmsg will be marked to indicate how it must be freed; if the
 * size is set to IKM_SIZE_INTR_KMSG, it must be returned to the
 * special interrupt kmsg zone, otherwise freed in the normal way.
 */
ipc_kmsg_t
dipc_kmsg_alloc(
	vm_size_t	size,
	boolean_t	thread_context)
{
	ipc_kmsg_t	kmsg;

	if (thread_context == TRUE) {
		kmsg = (ipc_kmsg_t)kalloc(ikm_plus_overhead(size));
		size = ikm_plus_overhead(size);
	} else {
		kmsg = (ipc_kmsg_t)zget(fast_kmsg_zone);
		size = IKM_SIZE_INTR_KMSG;
	}
	if (kmsg == IKM_NULL)
		return IKM_NULL;
	ikm_init_special(kmsg, size);
	assert(thread_context == TRUE || kmsg->ikm_size == IKM_SIZE_INTR_KMSG);
	assert(thread_context == FALSE || kmsg->ikm_size == size);
	return kmsg;
}


/*
 * For an interrupt-level kmsg, this routine guarantees
 * not to block.
 *
 * For a kmsg allocated from thread context, this routine
 * blocks only insofar as ikm_free ever blocks.
 */
void
dipc_kmsg_free(
	ipc_kmsg_t kmsg)
{
	unsigned int	s;

	if (kmsg->ikm_size == IKM_SIZE_INTR_KMSG)
		zfree(fast_kmsg_zone, (vm_offset_t)kmsg);
	else
		ikm_free(kmsg);
}

/*
 * allocate a meta kmsg from the zone.
 * Caller is responsible for initialization other than the META_KMSG bit.
 */
meta_kmsg_t
dipc_meta_kmsg_alloc(
	boolean_t	can_block)
{
	meta_kmsg_t	mkmsg;
	unsigned int	s;

	if (can_block)
		mkmsg = (meta_kmsg_t)zalloc(meta_kmsg_zone);
	else
		mkmsg = (meta_kmsg_t)zget(meta_kmsg_zone);
	if (mkmsg == MKM_NULL)
		return MKM_NULL;
	mkmsg->mkm_msgh_bits = MACH_MSGH_BITS_META_KMSG;
	return mkmsg;
}

void
dipc_meta_kmsg_free(
	meta_kmsg_t mkmsg)
{
	unsigned int	s;

	assert(KMSG_IS_META(mkmsg));
	zfree(meta_kmsg_zone, (vm_offset_t)mkmsg);
}

/*
 * Return number of free meta kmsgs in zone.
 */
int
dipc_meta_kmsg_free_count(void)
{
	return zone_free_count(meta_kmsg_zone);
}


#if	PORTABLE_SPLKKT_HELD
int
portable_splkkt_held()
{
	int	s;

	s = splhigh();
	splx(s);
	return s >= portable_splkkt_level;
}
#endif	/* PORTABLE_SPLKKT_HELD */
