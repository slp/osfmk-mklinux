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
 * libflipc/flipc_alloc.c
 *
 * Routines for allocation and deallocation of flipc structures.
 */

#include "flipc_ail.h"
#if REAL_TIME_PRIMITIVES
#include <mach/sync.h>
#endif

/*
 * Routines to take and release the allocations lock.
 *
 * These either ask the kernel flipc routines to do it for us (if we
 * don't have real time primitives), or they call the real time
 * primitives on the allocations lock structure we got during startup.
 *
 * They take a single argument, the domain lookup structure for this
 * domain, so that they can either: a) use the device port to get at
 * the device routines, or b) find the allocations lock in the
 * structure directly.
 *
 * No return values, no side effects.
 */
#if REAL_TIME_PRIMITIVES
static void flipc_acquire_allocations_lock(domain_lookup_t dl)
{
    kern_return_t kr;
    
    kr = lock_acquire(dl->allocations_lock, 0);
    if (kr == KERN_LOCK_UNSTABLE) {
	/* This is not good.  Specifically, it means that some task underwent
	   unexpected termination while holding the allocations lock.  This
	   may imply communciation buffer data structure corruption.
	   In response, this task will exit without doing anything, which
	   will cause a domino effect of all of the tasks on this node
	   exiting as they attempt to acquire the allocations lock, which
	   will result in the buffer being cleaned up.  */
	fprintf_stderr("Allocations lock found unstable.\n");
	fprintf_stderr("Possible buffer corruption; exiting.\n");
	exit(-1);
    }
}

static void flipc_release_allocations_lock(domain_lookup_t dl)
{
    lock_release(dl->allocations_lock, 0);
}
#else	/* REAL_TIME_PRIMITIVES */
static void flipc_acquire_allocations_lock(domain_lookup_t dl)
{
    flipc_comm_buffer_ctl_t cb_ctl =
	(flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

    while (!flipc_simple_lock_try(&cb_ctl->pail_alloc_lock))
	(dl->yield_fn)();
}

static void flipc_release_allocations_lock(domain_lookup_t dl)
{
    flipc_comm_buffer_ctl_t cb_ctl =
	(flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

    flipc_simple_lock_release(&cb_ctl->pail_alloc_lock);
}
#endif	/* REAL_TIME_PRIMITIVES */

/*
 * Allocate an endpoint
 */
FLIPC_return_t FLIPC_endpoint_allocate(FLIPC_domain_t domain_handle,
				       int number_of_buffers,
				       FLIPC_endpoint_type_t type,
				       unsigned long drop_processed_message_p,
				       FLIPC_endpoint_t *return_endpoint)
{
    domain_lookup_t dl = domain_handle_lookup(domain_handle);
    flipc_comm_buffer_ctl_t cb_ctl;
    flipc_endpoint_t endpoint;
    flipc_cb_ptr buffer_cbptr;
    flipc_cb_ptr *bufferlist_ptr;
    int i;
    flipc_buffer_state_t buffer_state;

    /* First level error checks.  Do we have a domain?  */
    if (!dl)
	return FLIPC_DOMAIN_NOT_ATTACHED;

    /* Did the user ask for anything overtly silly?  */
    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;
    if (number_of_buffers >
	(cb_ctl->bufferlist.number / cb_ctl->endpoint.number) - 1)
	return FLIPC_ALLOCATE_BUFFERLIST_SHORTAGE;

    if (number_of_buffers && (type == FLIPC_Send) && drop_processed_message_p)
	return FLIPC_ALLOCATE_BUFFERDROP_CONFLICT;

    /* Take the allocations lock.  */
    flipc_acquire_allocations_lock(dl);

    /* Second stage error checks, having to do with the free list.  */

    /* Are there any endpoints available?  */
    if (cb_ctl->endpoint.free == FLIPC_CBPTR_NULL) {
	flipc_release_allocations_lock(dl);
	return FLIPC_ALLOCATE_ENDPOINT_SHORTAGE;
    }

    /* Are there enough buffers available?  */
    for (i = number_of_buffers, buffer_cbptr = cb_ctl->data_buffer.free;
	 i && buffer_cbptr != FLIPC_CBPTR_NULL;
	 i--, buffer_cbptr = FLIPC_DATA_BUFFER_PTR(buffer_cbptr)->u.free)
	;
    if (i) {
	flipc_release_allocations_lock(dl);
	return FLIPC_ALLOCATE_BUFFER_SHORTAGE;
    }

    /* Take the endpoint off the freelist.  */
    endpoint = FLIPC_ENDPOINT_PTR(cb_ctl->endpoint.free);
    cb_ctl->endpoint.free = endpoint->sail_next_send_endpoint;

    /* Initialize fields of the endpoint.  */
    endpoint->constda_type = type;
    endpoint->constdm_type = type;
    endpoint->sme_overruns = 0;
    endpoint->pail_overruns_seen = 0;

    endpoint->pail_next_eg_endpoint = FLIPC_CBPTR_NULL;
    endpoint->sail_epgroup = FLIPC_CBPTR_NULL;
    
    /* If this is a send endpoint, point it's next send endpoint at the
       send endpoint list.  The send endpoint list will get modified in
       the synchronization step below.  */
    if (type == FLIPC_Send)
	endpoint->sail_next_send_endpoint = cb_ctl->sail_send_endpoint_list;
    else
	endpoint->sail_next_send_endpoint = FLIPC_CBPTR_NULL;

    /* Get the buffers and put them where you want them.  */
    bufferlist_ptr = FLIPC_BUFFERLIST_PTR(endpoint->constda_my_buffer_list);
    buffer_state = (type == FLIPC_Send) ? flipc_Completed : flipc_Processing;
    for (i = 0, buffer_cbptr = cb_ctl->data_buffer.free;
	 i < number_of_buffers;
	 i++) {
	bufferlist_ptr[i] = buffer_cbptr;
	FLIPC_DATA_BUFFER_PTR(buffer_cbptr)->shrd_state = buffer_state;
	buffer_cbptr = FLIPC_DATA_BUFFER_PTR(buffer_cbptr)->u.free;
	FLIPC_DATA_BUFFER_PTR(bufferlist_ptr[i])->u.destination =
	    cb_ctl->null_destination;
    }
    cb_ctl->data_buffer.free = buffer_cbptr;
    /* Fill in the rest of the bufferlist with some valid value.  */
    while (bufferlist_ptr + i
	   != FLIPC_BUFFERLIST_PTR(endpoint->constda_next_buffer_list))
	bufferlist_ptr[i++] = buffer_cbptr;

    /* Set the movable bufferlist pointers.  Note that the release pointer
       can't wrap, because if it did that would mean that the bufferlist was
       one more than full.  */
    endpoint->pme_release_ptr = 
	endpoint->sail_release_ptr =
	    FLIPC_CBPTR(&bufferlist_ptr[number_of_buffers]);
    if (drop_processed_message_p)
	endpoint->shrd_acquire_ptr = FLIPC_CBPTR_NULL;
    else
	endpoint->shrd_acquire_ptr = endpoint->constda_my_buffer_list;
    endpoint->pail_process_ptr = 
	endpoint->sme_process_ptr = (type == FLIPC_Send
				     ? endpoint->sail_release_ptr
				     : endpoint->constda_my_buffer_list);

    /* Setup the AIL copy of the dpb_or_enabled entry.  */
    endpoint->sailda_dpb_or_enabled =
	DOE_CONSTRUCT(drop_processed_message_p, 1);

    /* Synchronization steps.  We officially give up ownership of the
       endpoint here, though for send endpoints nothing can happen until
       we return the pointer to the user and he starts doing things
       with it.  */
    
    WRITE_FENCE();

    if (type == FLIPC_Send)
	SYNCVAR_WRITE(cb_ctl->sail_send_endpoint_list = FLIPC_CBPTR(endpoint));
    
    /* We synchronize with the message engine here.  Other parts of the AIL
       can't be doing anything with this endpoint as we haven't returned
       a pointer to it yet.  */
    SYNCVAR_WRITE(endpoint->saildm_dpb_or_enabled =
		  DOE_CONSTRUCT(drop_processed_message_p, 1));
    
    
    /* Release the allocations lock.  */
    flipc_release_allocations_lock(dl);

    /* Return the endpoint.  */
    *return_endpoint = endpoint;

    return FLIPC_SUCCESS;
}

/*
 * Allocate an endpoint group.
 */
FLIPC_return_t FLIPC_epgroup_allocate(FLIPC_domain_t domain_handle,
				      semaphore_port_t semaphore,
				      FLIPC_epgroup_t *return_epgroup)
{
    domain_lookup_t dl = domain_handle_lookup(domain_handle);
    flipc_comm_buffer_ctl_t cb_ctl;
    flipc_epgroup_t epgroup;

    /* Do we have a domain?  */
    if (!dl)
	return FLIPC_DOMAIN_NOT_ATTACHED;

    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

    /* Take the allocations lock.  */
    flipc_acquire_allocations_lock(dl);
    
    /* Are there any epgroups available?  */
    if (cb_ctl->epgroup.free == FLIPC_CBPTR_NULL) {
	flipc_release_allocations_lock(dl);
	return FLIPC_ALLOCATE_EPGROUP_SHORTAGE;
    }

    /* Grab the endpoint group.  */
    epgroup = FLIPC_EPGROUP_PTR(cb_ctl->epgroup.free);
    cb_ctl->epgroup.free = epgroup->pail_free;

    /* Associate the semaphore.  */
    if (semaphore != SEMAPHORE_NULL) {
	int dev_status[2];
	kern_return_t kr;

	dev_status[0] = epgroup - FLIPC_EPGROUP_PTR(cb_ctl->epgroup.start);
	dev_status[1] = semaphore;
	kr = device_set_status(dl->device_port,
			       FLIPC_DEVICE_FLAVOR(usermsg_Epgroup_Associate_Semaphore,
						   0),
			       dev_status,
			       2);
	if (kr != KERN_SUCCESS) {
	    /* We've checked to make sure that we're attached to the
	       domain, and we've pulled the epgroup from the free list
	       so it had better be good.  If we did get an error,
	       something's badly wrong.  */
	    fprintf_stderr("Unexpected internal error in flipc.\n");
	    exit(-1);
	}
    }

    /* Init all the fields.  */
    epgroup->const_semaphore_associated =
	(semaphore == SEMAPHORE_NULL ? 0 : 1);
    epgroup->sail_wakeup_req = 0;
    epgroup->pme_wakeup_del = 0;
    epgroup->sail_msgs_per_wakeup = 1;
    epgroup->pme_msgs_since_wakeup = 0;
    epgroup->pail_first_endpoint = FLIPC_CBPTR_NULL;
    epgroup->pail_free = FLIPC_CBPTR_NULL;

    /* Set the structure enabled.  In theory this is a synchronizing step,
       but neither the ME nor the rest of the AIL could know about this
       guy yet, so it could probably not be a synchronizing step.  But
       officially, this is where we give up ownership of the epgroup.  */

    WRITE_FENCE();

    SYNCVAR_WRITE(epgroup->sail_enabled = 1);

    flipc_release_allocations_lock(dl);

    *return_epgroup = epgroup;

    return FLIPC_SUCCESS;
}

/* 
 * Deallocate an endpoint without taking the allocations lock.
 *
 * Takes one argument, the endpoint to deallocate.  
 *
 * This is called by both endpoint deallocate and endpoint group
 * deallocate.  It exists so that endpoint group deallocate only has
 * to take the allocations lock once no matter how many structures it
 * is deallocating.
 *
 * No side effects other than deallocation of the endpoint.  Returns a
 * success or failure return code.
 */
FLIPC_return_t flipc_free_endpoint(flipc_endpoint_t endpoint)
{
    flipc_cb_ptr epgroup_cbptr;
    domain_lookup_t dl = domain_pointer_lookup(endpoint);
    flipc_comm_buffer_ctl_t cb_ctl;
    flipc_cb_ptr acquire_cbptr, release_cbptr;
    unsigned long dpb_or_enabled;

    /* Basic error check.  */
    if (!dl)
	return FLIPC_DOMAIN_NOT_ATTACHED;

    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

    /* Take the endpoint simple lock.  */
    flipc_simple_lock_acquire(&endpoint->pail_lock);

    /* Save this value; we'll be resetting the value in the endpoint.  */
    dpb_or_enabled = endpoint->sailda_dpb_or_enabled;
    /* If we are disabled, we return.  */
    if (!EXTRACT_ENABLED(dpb_or_enabled)) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_ENDPOINT_INACTIVE;
    }

    /* If we are on an endpoint group, we return.  */
    epgroup_cbptr = endpoint->sail_epgroup;
    if (epgroup_cbptr != FLIPC_CBPTR_NULL) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_ENDPOINT_ON_EPGROUP;
    }

    /* At this point we are a) locked, and b) not on any endpoint group.  */

    /* Make sure the message engine leaves us alone as well.  */     
    SYNCVAR_WRITE(endpoint->saildm_dpb_or_enabled = 0);

    /* Disable the endpoint on the AIL side (safe to do since we hold
       the lock.  */
    endpoint->sailda_dpb_or_enabled = 0;

    /* We're safe to release the simple lock here, since it's an AIL side
       concept, and disabling the structure gives us permanent ownership
       as far as the AIL is concerned.  */
    flipc_simple_lock_release(&endpoint->pail_lock);
    
    if (endpoint->constda_type == FLIPC_Send) {
	flipc_endpoint_t send_endpoint;
	volatile flipc_cb_ptr *last_cbptr;
	unsigned long qsync_orig, psync_orig;

	/* Find where it is on the send endpoint list.  */
	for (last_cbptr = &cb_ctl->sail_send_endpoint_list,
	     send_endpoint = FLIPC_ENDPOINT_PTR(*last_cbptr);
	     send_endpoint != endpoint;
	     last_cbptr = &send_endpoint->sail_next_send_endpoint, 
	     send_endpoint = FLIPC_ENDPOINT_PTR(*last_cbptr))
	    ;

	/* Remove it from the list.  */
	SYNCVAR_WRITE(*last_cbptr = endpoint->sail_next_send_endpoint);

	/* Sync with the send side ME.  */
	qsync_orig = cb_ctl->sail_request_sync;
	psync_orig = cb_ctl->sme_respond_sync;

	SYNCVAR_WRITE((cb_ctl->sail_request_sync = !qsync_orig, cb_ctl->send_ready++));

#ifdef KERNEL_MESSAGE_ENGINE
	/* Bypass possible disable of the message engine kick;
	   if we don't get the message engine running here, we hang.  */
	flipc_really_kick_message_engine(endpoint);
#endif

	while (cb_ctl->sme_respond_sync == psync_orig)
	    ;

	/* Safe to null out it's endpoint now.  */
	endpoint->sail_next_send_endpoint = FLIPC_CBPTR_NULL;
    } else {
	/* Make sure that any receive in progress is completed.  */
	while (cb_ctl->sme_receive_in_progress)
	    ;
	/* Wasn't that so much easier than the send side?  */
    }

    WRITE_FENCE();		/* May be gratiuitous.  */

    /* At this point no other entity can play with the endpoint.  */

    /* Read the bufferlist pointers.  For purposes of putting all of
       the buffers back on the free list, if drop processed buffers is
       set, the process pointer *is* the acquire pointer.  */
    acquire_cbptr = (EXTRACT_DPB(dpb_or_enabled)
		     ? endpoint->sme_process_ptr
		     : endpoint->shrd_acquire_ptr);
    release_cbptr = endpoint->sail_release_ptr;

    while (release_cbptr != acquire_cbptr) {
	/* Put the buffer pointed to by the acquire pointer on the
	   freelist.  */
	flipc_cb_ptr db_cbptr = *FLIPC_BUFFERLIST_PTR(acquire_cbptr);
	flipc_data_buffer_t db = FLIPC_DATA_BUFFER_PTR(db_cbptr);

	db->shrd_state = flipc_Free;
	db->u.free = cb_ctl->data_buffer.free;
	cb_ctl->data_buffer.free = db_cbptr;

	acquire_cbptr = NEXT_BUFFERLIST_PTR_AIL(acquire_cbptr, endpoint);
    }

    /* Reset the buffer pointers.  */
    endpoint->shrd_acquire_ptr = endpoint->constda_my_buffer_list;
    endpoint->sail_release_ptr = endpoint->constda_my_buffer_list;
    endpoint->sme_process_ptr = endpoint->constda_my_buffer_list;

    /* Reset the endpoint type.  */
    endpoint->constda_type = FLIPC_Inactive;
    endpoint->constdm_type = FLIPC_Inactive;

    /* Link the endpoint on to the free list.  Here's where we give up
       ownership.  */
    endpoint->sail_next_send_endpoint = cb_ctl->endpoint.free;
    cb_ctl->endpoint.free = FLIPC_CBPTR(endpoint);

    return FLIPC_SUCCESS;
}

/*
 * Deallocate an endpoint.
 */
FLIPC_return_t FLIPC_endpoint_deallocate(FLIPC_endpoint_t endpoint)
{
    FLIPC_return_t result;
    domain_lookup_t dl = domain_pointer_lookup(endpoint);

    if (!dl)
	return FLIPC_DOMAIN_NOT_ATTACHED;

    /* Acquire the allocations lock, do the real work, release it and
       return.  */
    flipc_acquire_allocations_lock(dl);

    result = flipc_free_endpoint(endpoint);

    flipc_release_allocations_lock(dl);

    return result;
}

/*
 * Deallocate an endpoint group.  This is one of the fun ones.
 */
FLIPC_return_t FLIPC_epgroup_deallocate(FLIPC_epgroup_t epgroup_arg)
{
    flipc_comm_buffer_ctl_t cb_ctl;
    flipc_endpoint_t endpoint;
    flipc_epgroup_t epgroup = epgroup_arg;
    domain_lookup_t dl = domain_pointer_lookup(epgroup);
    unsigned long qsync_orig, psync_orig;

    /* Obvious error checks.  */
    if (!dl)
	return FLIPC_DOMAIN_NOT_ATTACHED;

    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

    flipc_acquire_allocations_lock(dl);

    flipc_simple_lock_acquire(&epgroup->pail_lock);

    /* Attempt to clear the endpoint group of endpoints.  */
    while (1) {
	/* Active check.  */
	if (!epgroup->sail_enabled) {
	    flipc_simple_lock_release(&epgroup->pail_lock);
	    flipc_release_allocations_lock(dl);
	    return FLIPC_EPGROUP_INACTIVE;
	}

	if (epgroup->pail_first_endpoint == FLIPC_CBPTR_NULL)
	    /* Yay!  No endpoints left on this endpoint group.  */
	    break;

	endpoint = FLIPC_ENDPOINT_PTR(epgroup->pail_first_endpoint);

	flipc_simple_lock_release(&epgroup->pail_lock);

	/* This may fail; the possible reasons for failure are:
	   epgroup_inactive: Will be caught above on the next pass.

	   endpoint_not_on_epgroup: We lost a race; trying again
	   is the way to go.

	   On success, we free the endpoint.  */
	if (flipc_epgroup_remove_endpoint(epgroup, endpoint)
	    == FLIPC_SUCCESS) {
	    FLIPC_return_t fr = flipc_free_endpoint(endpoint);

	    if (fr != FLIPC_SUCCESS	/* Grand.  */
		&& fr != FLIPC_ENDPOINT_ON_EPGROUP /* Not our epgroup; not
						      our problem.  */
		&& fr != FLIPC_ENDPOINT_INACTIVE) { /* Someone else got here
						       first.   */
		flipc_release_allocations_lock(dl);
		return fr;		/* Problem.  */
	    }
	}

	flipc_simple_lock_acquire(&epgroup->pail_lock);
    }

    /* We are locked with no endpoints on us at this point.  */

    /* Convince the ME to leave us alone.  */

    SYNCVAR_WRITE(epgroup->sail_enabled = 0);
    WRITE_FENCE();
    
    /* Sync with the send side ME.  */
    qsync_orig = cb_ctl->sail_request_sync;
    psync_orig = cb_ctl->sme_respond_sync;
    
    SYNCVAR_WRITE((cb_ctl->sail_request_sync = !qsync_orig, cb_ctl->send_ready++));

#ifdef KERNEL_MESSAGE_ENGINE
    /* Bypass possible disable of message engine kick; if we don't
       get the message engine running here, we hang.  */
    flipc_really_kick_message_engine(epgroup);
#endif

    while (cb_ctl->sme_respond_sync == psync_orig)
	;

    /* Sync with receive side of ME.  */
    while (cb_ctl->sme_receive_in_progress)
	;

    /* Release the endpoint group simple lock; it's ours until we put
       it on the freelist.  */
    flipc_simple_lock_release(&epgroup->pail_lock);

    /* Deassociate the semaphore, if any.  */
    if (epgroup->const_semaphore_associated) {
	int dev_status[2];
	kern_return_t kr;

	dev_status[0] = epgroup - FLIPC_EPGROUP_PTR(cb_ctl->epgroup.start);
	dev_status[1] = SEMAPHORE_NULL;
	kr = device_set_status(dl->device_port,
			       FLIPC_DEVICE_FLAVOR(usermsg_Epgroup_Associate_Semaphore,
						   0),
			       dev_status,
			       2);
	if (kr != KERN_SUCCESS) {
	    /* We've checked to make sure that we're attached to the
	       domain, and we've pulled the epgroup from the free list
	       so it had better be good.  If we did get an error,
	       something's badly wrong.  */
	    fprintf_stderr("Unexpected internal error in flipc.\n");
	    exit(-1);
	}
    }

    /* Put the epgroup on the freelist.  */
    epgroup->pail_free = cb_ctl->epgroup.free;
    cb_ctl->epgroup.free = FLIPC_CBPTR(epgroup);

    /* Release the allocations lock and return.  */
    flipc_release_allocations_lock(dl);

    return FLIPC_SUCCESS;
}

