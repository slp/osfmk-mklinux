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
 * libflipc/flipc_endpoint.c
 *
 * This file contains all of the endpoint related functions.
 */

#include "flipc_ail.h"
/* For definitions of acquire and release.  */
#include <flipc_macros.h>

/*
 * Join an endpoint to an endpoint group.
 *
 * This function is somewhat unusual in that locks on structures are
 * generally not taken; all of the work is done in subroutines.
 */

FLIPC_return_t FLIPC_endpoint_join_epgroup(FLIPC_endpoint_t endpoint_arg,
					   FLIPC_epgroup_t epgroup_arg)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    flipc_epgroup_t epgroup = epgroup_arg;
    flipc_cb_ptr endpoint_epgroup_cbptr;
    flipc_epgroup_t endpoint_epgroup;
    FLIPC_return_t fr;

    /* Allowed as an atomic read without the lock; we will get a valid
       value.  */
    endpoint_epgroup_cbptr = endpoint->sail_epgroup;

    /* Convert it to a pointer.  */
    endpoint_epgroup =
	((endpoint_epgroup_cbptr == FLIPC_CBPTR_NULL)
	 ? (flipc_epgroup_t) 0 : FLIPC_EPGROUP_PTR(endpoint_epgroup_cbptr));

    while (endpoint_epgroup != epgroup) {
	/* Get the old endpoint group out.  */
	while (endpoint_epgroup) {
	    fr = flipc_epgroup_remove_endpoint(endpoint_epgroup, endpoint);
	    if (fr == FLIPC_EPGROUP_INACTIVE) {
		/* Possible badness here.  The epgroup might be
		   inactive, with the endpoint on it.  This is a
		   "can't happen", but check it out, just in case.  */
		flipc_simple_lock_acquire(&endpoint_epgroup->pail_lock);
		if (!endpoint_epgroup->sail_enabled
		    && (endpoint->sail_epgroup
			== FLIPC_CBPTR(endpoint_epgroup))) {
		    /* Confirmed badness.  */
		    flipc_simple_lock_release(&endpoint_epgroup->pail_lock);
		    fprintf_stderr("Data structure corruption in "
				   "communications buffer.\n");
		    exit(-1);
		}
		flipc_simple_lock_release(&endpoint_epgroup->pail_lock);
	    }
	    if (fr == FLIPC_ENDPOINT_INACTIVE)
		/* Ah, well, we can't do anything if the endpoint is
                   inactive.  */
		return fr;

	    /* Other failure modes are ok.  */

	    endpoint_epgroup_cbptr = endpoint->sail_epgroup;
	    endpoint_epgroup =
		((endpoint_epgroup_cbptr == FLIPC_CBPTR_NULL)
		 ? (flipc_epgroup_t) 0
		 : FLIPC_EPGROUP_PTR(endpoint_epgroup_cbptr));
	}

	/* If we want to put the endpoint onto a new endpoint group, try
	   and do that.  */
	if (epgroup) {
	    fr = flipc_epgroup_add_endpoint(epgroup, endpoint);
	    switch(fr) {
	      case FLIPC_SUCCESS:
	      case FLIPC_ENDPOINT_INACTIVE:
	      case FLIPC_EPGROUP_INACTIVE:
		return fr;
	      case FLIPC_ENDPOINT_NOT_ON_EPGROUP:
		continue;
	    }
	}
    }
    /* No matter what, if we fell out of the loop the endpoint is on the
       endpoint group.  */
    return FLIPC_SUCCESS;
}

/*
 * Given an endpoint, give back an address.
 */
FLIPC_return_t FLIPC_endpoint_address(FLIPC_endpoint_t endpoint_arg,
				      FLIPC_address_t *address)   
{
    flipc_endpoint_t endpoint = endpoint_arg;
    domain_lookup_t dl = domain_pointer_lookup(endpoint);
    flipc_comm_buffer_ctl_t cb_ctl =
	(flipc_comm_buffer_ctl_t) dl->comm_buffer_base;
    /* Can be read atomically; no need to lock.  */
    FLIPC_endpoint_type_t endpoint_type = endpoint->constda_type;
    int endpoint_idx =
	endpoint - FLIPC_ENDPOINT_PTR(cb_ctl->endpoint.start);

    if (endpoint_idx >= cb_ctl->endpoint.number
	|| endpoint_idx < 0)
	return FLIPC_INVALID_ARGUMENTS;

    if (endpoint_type != FLIPC_Receive)
	return FLIPC_INVALID_ARGUMENTS;

    *address = FLIPC_CREATE_ADDRESS(cb_ctl->local_node_address,
				    endpoint_idx);

    return FLIPC_SUCCESS;
}

/*
 * Return the overrun count for an endpoint, optionally zeroing it.
 */
int FLIPC_endpoint_overrun_count(FLIPC_endpoint_t endpoint_arg,
				 int zero_overrun_count)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    int tmp, result;

    flipc_simple_lock_acquire(&endpoint->pail_lock);

    if (!EXTRACT_ENABLED(endpoint->sailda_dpb_or_enabled)) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return -1;
    }

    /* Find out how many overruns have occurred since we last reset.  */
    tmp = endpoint->sme_overruns;
    result = tmp - endpoint->pail_overruns_seen;

    /* And if we want to reset again, increment our count.  */
    if (zero_overrun_count)
	endpoint->pail_overruns_seen = tmp;

    flipc_simple_lock_release(&endpoint->pail_lock);
    return result;
}

/*
 * Return the various pieces of information that the user might want
 * about an endpoint.
 */

FLIPC_return_t FLIPC_endpoint_query(FLIPC_endpoint_t endpoint_arg,
				    FLIPC_endpoint_info_t endpoint_info)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    unsigned long dpb_or_enabled;
    flipc_cb_ptr acquire_cbptr, release_cbptr;

    /* I mostly need the lock for number buffers, but it just seems safe
       if I'm getting this much information to do it this way.  */
    flipc_simple_lock_acquire(&endpoint->pail_lock);

    dpb_or_enabled = endpoint->sailda_dpb_or_enabled;

    if (!EXTRACT_ENABLED(dpb_or_enabled)) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_ENDPOINT_INACTIVE;
    }

    endpoint_info->type = endpoint->constda_type;

    endpoint_info->processed_buffers_dropped_p = EXTRACT_DPB(dpb_or_enabled);

    endpoint_info->epgroup =
	(endpoint->sail_epgroup == FLIPC_CBPTR_NULL
	 ? FLIPC_EPGROUP_NULL
	 : FLIPC_EPGROUP_PTR(endpoint->sail_epgroup));

    /* If drop processed buffers is set, the process pointer is the
       effective acquire pointer.  We presume that this isn't a
       critical section call (too general), and get the real value of
       the process pointer.  */
    acquire_cbptr = (EXTRACT_DPB(dpb_or_enabled)
		     ? endpoint->sme_process_ptr
		     : endpoint->shrd_acquire_ptr);
    release_cbptr = endpoint->sail_release_ptr;
    endpoint_info->number_buffers =
	BUFFERS_ON_ENDPOINT_AIL(acquire_cbptr, release_cbptr, endpoint);

    flipc_simple_lock_release(&endpoint->pail_lock);

    return FLIPC_SUCCESS;
}

/*
 * Note that having FLIPC_TIMING_TRACE defined requires time_trace.h
 * to exist somewhere in the include path, which it doesn't in a normal
 * sandbox.  Thus this flag should only be used if you have made arrangements
 * for that file to be somewhere on the include path.
 */
#ifdef FLIPC_TIMING_TRACE
#define TIMING
#include <time_trace.h>
#else
/* Null definitions for timing samples.  */
#define TIMER_SAMPLE_LOG(note)
#endif

#define	HANDCODED	__i860__

#if	!HANDCODED
/*
 * FLIPC_endpoint_buffer_release_unlocked
 *
 * Single thread only implementation.
 *
 * Release a buffer to an endpoint for its use.  If a send endpoint,
 * this queues the buffer for sending.  If a receive endpoint, this
 * allows the buffer to be used for receiving.
 */
FLIPC_return_t 
FLIPC_endpoint_buffer_release_unlocked(FLIPC_endpoint_t endpoint_arg,
				       FLIPC_buffer_t buffer_arg)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    flipc_data_buffer_t db =
	(flipc_data_buffer_t) (((char *) (buffer_arg))
			       - sizeof(struct flipc_data_buffer));
    flipc_cb_ptr tmp_cbptr, release_cbptr;
    FLIPC_endpoint_type_t type;
    flipc_comm_buffer_ctl_t cb_ctl = (flipc_comm_buffer_ctl_t)flipc_cb_base;

    TIMER_SAMPLE_LOG("FE_buffer_release: entry");

#if !NO_ERROR_CHECKING
    /* If it's disabled, return.  */
    if (!EXTRACT_ENABLED(endpoint->sailda_dpb_or_enabled)) {
	return FLIPC_ENDPOINT_INACTIVE;
    }
    /* If the buffer is bad, return.  */
    if (buffer_arg == FLIPC_BUFFER_NULL) {
	return FLIPC_INVALID_ARGUMENTS;
    }
#endif

    TIMER_SAMPLE_LOG("FE_buffer_release: post extract enabled");

    /* Figure out what macro (ie. limitting circular buffer pointer) we
       should use and then if there's room to release a buffer.  Some
       complex logic in these macros; among other things they set
       release_cbptr.  */
    if (EXTRACT_DPB(endpoint->sailda_dpb_or_enabled)) {
	if (!DPB_ENDPOINT_RELEASE_OK(release_cbptr, tmp_cbptr, endpoint)) {
		return FLIPC_ENDPOINT_FULL;
	}
    } else {
	if (!NODPB_ENDPOINT_RELEASE_OK(release_cbptr, tmp_cbptr, endpoint)) {
	    return FLIPC_ENDPOINT_FULL;
	}
    }

    TIMER_SAMPLE_LOG("FE_buffer_release: post space check");

    /* Release the buffer.  */
    *FLIPC_BUFFERLIST_PTR(release_cbptr) = FLIPC_CBPTR(db);

    db->shrd_state = flipc_Processing;

    WRITE_FENCE();

    SYNCVAR_WRITE(endpoint->sail_release_ptr =
		  NEXT_BUFFERLIST_PTR_AIL(release_cbptr, endpoint));

    TIMER_SAMPLE_LOG("FE_buffer_release: post buffer release");

    /* Get the type to possibly kick the message engine.  */
    type = endpoint->constda_type;

    if (type == FLIPC_Send) {
	cb_ctl->send_ready++; 
#if KERNEL_MESSAGE_ENGINE
	flipc_kick_message_engine(endpoint);
#endif
    }

    TIMER_SAMPLE_LOG("FE_buffer_release: post lock release (& me kick)");

    return FLIPC_SUCCESS;
}

#endif	/* !HANDCODED */


/*
 * FLIPC_endpoint_buffer_release
 *
 * Multi-thread implementation.
 *
 * Release a buffer to an endpoint for its use.  If a send endpoint,
 * this queues the buffer for sending.  If a receive endpoint, this
 * allows the buffer to be used for receiving.
 */
FLIPC_return_t FLIPC_endpoint_buffer_release(
	FLIPC_endpoint_t endpoint_arg,
	FLIPC_buffer_t buffer_arg)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    flipc_data_buffer_t db =
	(flipc_data_buffer_t) (((char *) (buffer_arg))
			       - sizeof(struct flipc_data_buffer));
    flipc_cb_ptr tmp_cbptr, release_cbptr;
    FLIPC_endpoint_type_t type;
    flipc_comm_buffer_ctl_t cb_ctl = (flipc_comm_buffer_ctl_t)flipc_cb_base;

    TIMER_SAMPLE_LOG("FE_buffer_release: entry");

    /* Take the structure lock.  */
    flipc_simple_lock_acquire(&endpoint->pail_lock);

    TIMER_SAMPLE_LOG("FE_buffer_release: post lock acquire");

#if !NO_ERROR_CHECKING
    /* If it's disabled, return.  */
    if (!EXTRACT_ENABLED(endpoint->sailda_dpb_or_enabled)) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_ENDPOINT_INACTIVE;
    }
    /* If the buffer is bad, return.  */
    if (buffer_arg == FLIPC_BUFFER_NULL) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_INVALID_ARGUMENTS;
    }
#endif

    TIMER_SAMPLE_LOG("FE_buffer_release: post extract enabled");

    /* Figure out what macro (ie. limitting circular buffer pointer) we
       should use and then if there's room to release a buffer.  Some
       complex logic in these macros; among other things they set
       release_cbptr.  */
    if (EXTRACT_DPB(endpoint->sailda_dpb_or_enabled)) {
	if (!DPB_ENDPOINT_RELEASE_OK(release_cbptr, tmp_cbptr, endpoint)) {
	    flipc_simple_lock_release(&endpoint->pail_lock);
		return FLIPC_ENDPOINT_FULL;
	}
    } else {
	if (!NODPB_ENDPOINT_RELEASE_OK(release_cbptr, tmp_cbptr, endpoint)) {
	    flipc_simple_lock_release(&endpoint->pail_lock);
	    return FLIPC_ENDPOINT_FULL;
	}
    }

    TIMER_SAMPLE_LOG("FE_buffer_release: post space check");

    /* Release the buffer.  */
    *FLIPC_BUFFERLIST_PTR(release_cbptr) = FLIPC_CBPTR(db);

    db->shrd_state = flipc_Processing;

    WRITE_FENCE();

    SYNCVAR_WRITE(endpoint->sail_release_ptr =
		  NEXT_BUFFERLIST_PTR_AIL(release_cbptr, endpoint));

    TIMER_SAMPLE_LOG("FE_buffer_release: post buffer release");

    /* Get the type to possibly kick the message engine.  */
    type = endpoint->constda_type;

    /* Release the lock.  */
    flipc_simple_lock_release(&endpoint->pail_lock);

    if (type == FLIPC_Send) {
	cb_ctl->send_ready++; 
#if KERNEL_MESSAGE_ENGINE
	flipc_kick_message_engine(endpoint);
#endif
    }

    TIMER_SAMPLE_LOG("FE_buffer_release: post lock release (& me kick)");

    return FLIPC_SUCCESS;
}


#if	!HANDCODED
/*
 * Single threaded buffer acquire operation.
 *
 * Acquire a buffer from an endpoint.  If this endpoint is a send endpoint,
 * this is an allocation operation.  If it is a receive endpoint, we are
 * getting a message that has been received.
 */
FLIPC_buffer_t FLIPC_endpoint_buffer_acquire_unlocked(
	FLIPC_endpoint_t endpoint_arg)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    flipc_cb_ptr acquire_cbptr, tmp_cbptr;
    flipc_data_buffer_t db;

    TIMER_SAMPLE_LOG("FE_buffer_acquire: entry");

    /* Return if the structure is disabled or has drop processed
       buffers set.  No point in ifdefing this out since we have to
       check DPB regardless, and the extra check of enabled costs us
       an xor.  */
    if (DISABLED_OR_DPB_P(endpoint->sailda_dpb_or_enabled)) {
	return FLIPC_BUFFER_NULL;
    } 

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post extract enabled");

    /* Is there a buffer?  Lot's of logic in this macro; among
       other things, it sets acquire_cbptr.  */
    if (!ENDPOINT_ACQUIRE_OK(acquire_cbptr, tmp_cbptr, endpoint)) {
	return FLIPC_BUFFER_NULL;
    }

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post buffer check");

    /* Yep. Get the data buffer.  */
    db = FLIPC_DATA_BUFFER_PTR(*FLIPC_BUFFERLIST_PTR(acquire_cbptr));

    /* Return it.  Note that the fact that there's a buffer in
       this position indicates that the endpoint was not configured
       to drop processed buffers.  */
    SYNCVAR_WRITE(endpoint->shrd_acquire_ptr =
		  NEXT_BUFFERLIST_PTR_AIL(acquire_cbptr, endpoint));

    WRITE_FENCE();

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post buffer acquire");

    /* Change buffer state.  */
    db->shrd_state = flipc_Ready;

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post lock release and state change");

    /* Return buffer (giving use pointer they expect).  */
    return
	(FLIPC_buffer_t) (((char *) db)
			  + sizeof(struct flipc_data_buffer));
}
#endif	/* !HANDCODED */

/*
 * FLIPC_endpoint_buffer_acquire
 *
 * Multi-thread safe buffer acquire operation.
 *
 * Acquire a buffer from an endpoint.  If this endpoint is a send endpoint,
 * this is an allocation operation.  If it is a receive endpoint, we are
 * getting a message that has been received.
 */
FLIPC_buffer_t FLIPC_endpoint_buffer_acquire(
	FLIPC_endpoint_t endpoint_arg)
{
    flipc_endpoint_t endpoint = endpoint_arg;
    flipc_cb_ptr acquire_cbptr, tmp_cbptr;
    flipc_data_buffer_t db;

    TIMER_SAMPLE_LOG("FE_buffer_acquire: entry");

    /* Take the structure lock.  */
    flipc_simple_lock_acquire(&endpoint->pail_lock);

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post lock acquire");

    /* Return if the structure is disabled or has drop processed
       buffers set.  No point in ifdefing this out since we have to
       check DPB regardless, and the extra check of enabled costs us
       an xor.  */
    if (DISABLED_OR_DPB_P(endpoint->sailda_dpb_or_enabled)) {
	flipc_simple_lock_release(&(endpoint)->pail_lock);
	return FLIPC_BUFFER_NULL;
    } 

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post extract enabled");

    /* Is there a buffer?  Lot's of logic in this macro; among
       other things, it sets acquire_cbptr.  */
    if (!ENDPOINT_ACQUIRE_OK(acquire_cbptr, tmp_cbptr, endpoint)) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_BUFFER_NULL;
    }

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post buffer check");

    /* Yep. Get the data buffer.  */
    db = FLIPC_DATA_BUFFER_PTR(*FLIPC_BUFFERLIST_PTR(acquire_cbptr));

    /* Return it.  Note that the fact that there's a buffer in
       this position indicates that the endpoint was not configured
       to drop processed buffers.  */
    SYNCVAR_WRITE(endpoint->shrd_acquire_ptr =
		  NEXT_BUFFERLIST_PTR_AIL(acquire_cbptr, endpoint));

    WRITE_FENCE();

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post buffer acquire");

    /* Let go of simple lock.  */
    flipc_simple_lock_release(&endpoint->pail_lock);

    /* Change buffer state.  */
    db->shrd_state = flipc_Ready;

    TIMER_SAMPLE_LOG("FE_buffer_acquire: post lock release and state change");

    /* Return buffer (giving use pointer they expect).  */
    return
	(FLIPC_buffer_t) (((char *) db)
			  + sizeof(struct flipc_data_buffer));
}

