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

#include "flipc_ail.h"

/*
 * libflipc/flipc_epgroup.c
 *
 * Endpoint group related routines (other than allocation).
 */

/*
 * Subroutines called by other routines, either in this file or in
 * other files.
 */

/*
 * Remove an endpoint from an endpoint group.
 *
 * Only succeeds if they are both active and the endpoint is actually
 * on the endpoint group.  Used by both endpoint_join_epgroup and
 * epgroup_deallocate subroutines; needs to be specific about the
 * epgroup for the epgroup deallocate subroutines.
 *
 * Takes two argument; the endpoint and endpoint group.
 * Returns success or error code.
 *
 * Only side effect is to remove the endpoint from the endpoint group */
FLIPC_return_t flipc_epgroup_remove_endpoint(flipc_epgroup_t epgroup,
					     flipc_endpoint_t endpoint)
{

    /* Take endpoint group's simple lock.  */
    flipc_simple_lock_acquire(&epgroup->pail_lock);

    if (!epgroup->sail_enabled) {
	/* Well, we'll fail.  How badly?  */
	if (FLIPC_EPGROUP_PTR(endpoint->sail_epgroup)
	    == epgroup) {
	    /* Badly.  This isn't allowed, for an endpoint group to be
	       disabled and have an endpoint on it.  */
	    flipc_simple_lock_release(&epgroup->pail_lock);
	    fprintf_stderr("Data structure corruption on comm buffer.\n");
	    exit(-1);
	}

	/* Not too too badly.  Cleanup and report.  */
	flipc_simple_lock_release(&epgroup->pail_lock);
	return FLIPC_EPGROUP_INACTIVE;
    }

    /* Is the endpoint on the endpoint group?  */
    if (FLIPC_EPGROUP_PTR(endpoint->sail_epgroup)
	== epgroup) {
	/* Yep.  Take it off.  First edit the chain.  */
	volatile flipc_cb_ptr *last_cbptr;
	flipc_endpoint_t chain_endpoint;

	for (last_cbptr = &epgroup->pail_first_endpoint,
	     chain_endpoint = FLIPC_ENDPOINT_PTR(*last_cbptr);
	     chain_endpoint != endpoint;
	     last_cbptr = &chain_endpoint->pail_next_eg_endpoint,
	     chain_endpoint = FLIPC_ENDPOINT_PTR(*last_cbptr))
	    ;

	*last_cbptr = endpoint->pail_next_eg_endpoint;

	/* Now, modify the internal pointer.  This is a synchronization step
	   with the rest of the AIL, as we give up ownership of the
	   endpoint group related data structures in the endpoint at this
	   point.  */
	SYNCVAR_WRITE(endpoint->sail_epgroup = FLIPC_CBPTR_NULL);

	/* Increment epgroup's version counter.  */
	epgroup->pail_version++;

	/* All done; clean up and return.  */
	flipc_simple_lock_release(&epgroup->pail_lock);

	return FLIPC_SUCCESS;
    } else {
	/* Nope; can't complete the mission.  */
	flipc_simple_lock_release(&epgroup->pail_lock);

	return FLIPC_ENDPOINT_NOT_ON_EPGROUP;
    }
}

/*
 * Add an endpoint to an endpoint group.
 *
 * Only succeeds if the endpoint is not on an endpoint group.  Used by
 * the endpoint_join_epgroup subroutine; convenient to have the
 * functionality broken out.  Fails if the endpoint is already on an
 * endpoint group.
 *
 * Takes two arguments: the endpoint and endpoint group.
 * Returns success or an error code.
 *
 * No side effects other than the addition of the endpoint to the
 * endpoint group. 
 */
FLIPC_return_t flipc_epgroup_add_endpoint(flipc_epgroup_t epgroup,
					  flipc_endpoint_t endpoint)
{
    flipc_simple_lock_acquire_2(&epgroup->pail_lock,
				&endpoint->pail_lock);

    /* If either's inactive, return noting that.  */
    if (!EXTRACT_ENABLED(endpoint->sailda_dpb_or_enabled)) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_ENDPOINT_INACTIVE;
    }

    if (!epgroup->sail_enabled) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_EPGROUP_INACTIVE;
    }
    
    /* If the endpoint is already on an endpoint group, that's an
       error also.  */
    if (endpoint->sail_epgroup != FLIPC_CBPTR_NULL) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	flipc_simple_lock_release(&endpoint->pail_lock);
	return FLIPC_ENDPOINT_ON_EPGROUP;
    }

    /* Link it in.  */
    SYNCVAR_WRITE(endpoint->sail_epgroup = FLIPC_CBPTR(epgroup));

    endpoint->pail_next_eg_endpoint = epgroup->pail_first_endpoint;
    epgroup->pail_first_endpoint = FLIPC_CBPTR(endpoint);

    /* Increment version counter.  */
    epgroup->pail_version++;

    /* Release locks and return.  */
    flipc_simple_lock_release(&epgroup->pail_lock);
    flipc_simple_lock_release(&endpoint->pail_lock);

    return FLIPC_SUCCESS;
}

/*
 * Retrieve the semaphore associated with an endpoint group.
 */
FLIPC_return_t FLIPC_epgroup_semaphore(FLIPC_epgroup_t epgroup_arg,
				       semaphore_port_t *semaphore)
{
    flipc_epgroup_t epgroup = epgroup_arg;
    kern_return_t kr;
    domain_lookup_t dl = domain_pointer_lookup(epgroup);
    flipc_comm_buffer_ctl_t cb_ctl;
    int dev_status[1];
    unsigned int dev_status_count;
    
    if (!dl)
	/* No domain pointer means that we were passed a bad
	   epgroup_arg.  */
	return FLIPC_INVALID_ARGUMENTS;

    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

    /* No point in checking if structure's enabled, as it might be
       disabled before we get out of the kernel.  */

    /* Get the semaphore.  */
    dev_status_count = 1;
    kr = device_get_status(dl->device_port,
			   FLIPC_DEVICE_FLAVOR(usermsg_Get_Epgroup_Semaphore,
					       (epgroup - FLIPC_EPGROUP_PTR(cb_ctl->epgroup.start))),
			   dev_status,
			   &dev_status_count);
    if (kr != KERN_SUCCESS) {
	switch (kr) {
	  case D_INVALID_OPERATION:
	    return FLIPC_INVALID_ARGUMENTS;
	  case D_DEVICE_DOWN:
	    return FLIPC_DOMAIN_NOT_INITIALIZED;
	  default:
	    return FLIPC_KERNEL_TRANSLATION_ERR;
	}
    }

    *semaphore = (semaphore_port_t) dev_status[0];

    return FLIPC_SUCCESS;
}

/*
 * Set the wakeup count for an endpoint group.
 */
FLIPC_return_t FLIPC_epgroup_set_wakeup_count(FLIPC_epgroup_t epgroup_arg,
					      int messages_per_wakeup)
{
    flipc_epgroup_t epgroup = epgroup_arg;

    /* Take the lock; release and return if inactive.  */
    flipc_simple_lock_acquire(&epgroup->pail_lock);
    if (!epgroup->sail_enabled) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	return FLIPC_EPGROUP_INACTIVE;
    }

    /* Set the count.  */
    epgroup->sail_msgs_per_wakeup = messages_per_wakeup;
    flipc_simple_lock_release(&epgroup->pail_lock);

    return FLIPC_SUCCESS;
}

/*
 * Helper routine for following two routines.
 * Finds a buffer, if one is available for acquisition, on the
 * endpoints on epgroup.  If it finds a buffer, it returns it.
 * If it finds a buffer, the endpoint the buffer is on is returned in
 * return_endpoint and both endpoint and epgroup are returned locked.
 */
static FLIPC_buffer_t flipc_epgroup_find_buffer(flipc_epgroup_t epgroup,
						flipc_endpoint_t
						*return_endpoint)
{
    flipc_cb_ptr current_cbptr;
    unsigned long version_count;

    /* Make sure we're enabled to be doing this sort of thing.  */
    flipc_simple_lock_acquire(&epgroup->pail_lock);
    if (!epgroup->sail_enabled) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	return FLIPC_BUFFER_NULL;
    }
    
    /* I have the endpoint group simple lock.  */
    do {

	/* For when we find a buffer, but lose the version update race.  */
      try_again:		

	/* Save current version information.  */
	version_count = epgroup->pail_version;

	/* Search the epgroup's endpoints.  */
	current_cbptr = epgroup->pail_first_endpoint;
	
	while (current_cbptr != FLIPC_CBPTR_NULL) {
	    flipc_endpoint_t endpoint = FLIPC_ENDPOINT_PTR(current_cbptr);
	    flipc_cb_ptr acquire_cbptr, tmp_cbptr;
	    
	    /* Release the epgroup lock.  */
	    flipc_simple_lock_release(&epgroup->pail_lock);

	    /* Get both locks.  */
	    flipc_simple_lock_acquire_2(&epgroup->pail_lock,
					&endpoint->pail_lock);
	    
	    /* Are they both active?  */
	    if (!epgroup->sail_enabled) {
		flipc_simple_lock_release(&epgroup->pail_lock);
		flipc_simple_lock_release(&endpoint->pail_lock);
		return FLIPC_BUFFER_NULL;
	    }
	    if (!EXTRACT_ENABLED(endpoint->sailda_dpb_or_enabled)) {
		/* We lost a race, and an endpoint was removed
		   from the epgroup and deallocated on us.  Let go
		   of the endpoint lock, and start over.  */
		flipc_simple_lock_release(&endpoint->pail_lock);
		goto try_again;
	    }

	    /* Is there a buffer on this endpoint? */
	    if (!EXTRACT_DPB(endpoint->sailda_dpb_or_enabled)
		&& ENDPOINT_ACQUIRE_OK(acquire_cbptr, tmp_cbptr, endpoint)) {
		flipc_data_buffer_t db =
		    FLIPC_DATA_BUFFER_PTR(*FLIPC_BUFFERLIST_PTR(acquire_cbptr));
		/* We have a buffer, and everything is locked.
		   We're golden, unless the version count has
		   changed while we've been working.  */
		if (version_count == epgroup->pail_version) {
		    *return_endpoint = endpoint;
		    return db;
		}
		/* Release the endpoint lock, and bail out to the next
		   time around the loop.  */
		flipc_simple_lock_release(&endpoint->pail_lock);
		/* Only endpoint group lock held here.  */
		goto try_again;
	    }
	    
	    /* There wasn't a buffer on that endpoint; move on.  */
	    current_cbptr = endpoint->pail_next_eg_endpoint;
	    
	    flipc_simple_lock_release(&endpoint->pail_lock);
	    /* Note that we leave the epgroup simple lock held.  */
	}

    } while (version_count != epgroup->pail_version);

    /* Now we release the epgroup simple lock.  */
    flipc_simple_lock_release(&epgroup->pail_lock);
    
    /* Nothing to get.  */
    return FLIPC_BUFFER_NULL;
}


/*
 * Return a buffer, if any available, on the endpoints on epgroup.
 */
FLIPC_buffer_t FLIPC_epgroup_get_message(FLIPC_epgroup_t epgroup_arg,
                                         int request_wakeup,
					 FLIPC_endpoint_t *endpoint_ptr_arg,
                                         int *wakeup_requested)
{
    flipc_epgroup_t epgroup = epgroup_arg;
    flipc_endpoint_t endpoint;
    flipc_data_buffer_t data_buffer;

    /* If the user has requested a wakeup, check to see if there's a
       sempahore associated with the epgroup and let him know if he
       messed up.  */
    if (request_wakeup) {
	flipc_simple_lock_acquire(&epgroup->pail_lock);
	if (!epgroup->const_semaphore_associated) {
	    flipc_simple_lock_release(&epgroup->pail_lock);
	    return FLIPC_BUFFER_NULL;
	}
	flipc_simple_lock_release(&epgroup->pail_lock);
	/* Set wakeup_requested to zero (if the pointer is valid);
	   either this will be accurate, or we will later reset it
	   to 1.  */
	if (wakeup_requested)
	    *wakeup_requested = 0;
    }
    
    data_buffer = flipc_epgroup_find_buffer(epgroup, &endpoint);

    if (data_buffer) {
	/* We've got one!  Note that successfully finding a buffer
	   for acquisition on an endpoint implies that the endpoint
	   is not configured to drop buffers after processing.  */
	flipc_cb_ptr acquire_ptr = endpoint->shrd_acquire_ptr;

	/* Increment the acquire pointer.  */
	SYNCVAR_WRITE(endpoint->shrd_acquire_ptr =
		      NEXT_BUFFERLIST_PTR_AIL(acquire_ptr, endpoint));

	/* Release the locks.  */
	flipc_simple_lock_release(&endpoint->pail_lock);
	flipc_simple_lock_release(&epgroup->pail_lock);

	/* Change the buffer state.  */
	data_buffer->shrd_state = flipc_Ready;

	/* Return */
	*endpoint_ptr_arg = endpoint;
	return (FLIPC_buffer_t) ((char *) data_buffer
				 + sizeof(struct flipc_data_buffer));
    }

    if (!request_wakeup)
	return FLIPC_BUFFER_NULL;

    flipc_simple_lock_acquire(&epgroup->pail_lock);
    if (!epgroup->const_semaphore_associated) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	/* The only way for this bit to change state from the beginning of
	   the function was for the epgroup to be deallocated and
	   allocated again.  Thus returning this error code is valid.  */
	return FLIPC_BUFFER_NULL;
    }
    SYNCVAR_WRITE(epgroup->sail_wakeup_req++);
    WRITE_FENCE();
    flipc_simple_lock_release(&epgroup->pail_lock);

    /* Won't be here if request_wakeup was zero, thus this variable
       only gets set if request_wakeup is set and wakeup_requested
       (pointer to out variable) is set.  */
    if (wakeup_requested)
	*wakeup_requested = 1;

    /* Check again, in case we lost the race.  */

    data_buffer = flipc_epgroup_find_buffer(epgroup, &endpoint);

    if (data_buffer) {
	flipc_cb_ptr acquire_ptr = endpoint->shrd_acquire_ptr;

	/* Increment the acquire pointer.  */
	SYNCVAR_WRITE(endpoint->shrd_acquire_ptr =
		      NEXT_BUFFERLIST_PTR_AIL(acquire_ptr, endpoint));

	/* Release the locks.  */
	flipc_simple_lock_release(&endpoint->pail_lock);
	flipc_simple_lock_release(&epgroup->pail_lock);

	/* Change the buffer state.  */
	data_buffer->shrd_state = flipc_Ready;

	/* Return */
	*endpoint_ptr_arg = endpoint;
	return (FLIPC_buffer_t) ((char *) data_buffer
				 + sizeof(struct flipc_data_buffer));
    }

    return FLIPC_BUFFER_NULL;
}

/*
 * Is a message available on an endpoint group?
 */
int FLIPC_epgroup_message_available(FLIPC_epgroup_t epgroup_arg)
{
    flipc_epgroup_t epgroup = epgroup_arg;
    flipc_endpoint_t endpoint;

    if (flipc_epgroup_find_buffer(epgroup, &endpoint)) {
	flipc_simple_lock_release(&endpoint->pail_lock);
	flipc_simple_lock_release(&epgroup->pail_lock);
	return 1;
    }
    return 0;
}

/*
 * Return the endpoints that are on an endpoint group.
 */
FLIPC_return_t FLIPC_epgroup_endpoints(FLIPC_epgroup_t epgroup_arg,
				       int *endpoint_array_size,
				       FLIPC_endpoint_t *endpoint_array)
{
    flipc_epgroup_t epgroup = epgroup_arg;
    int endpoint_cnt;
    flipc_cb_ptr endpoint_cbptr;
    flipc_endpoint_t endpoint;

    flipc_simple_lock_acquire(&epgroup->pail_lock);
    if (!epgroup->sail_enabled) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	return FLIPC_EPGROUP_INACTIVE;
    }

    for (endpoint_cnt = 0, endpoint_cbptr = epgroup->pail_first_endpoint;
	 (endpoint_cbptr != FLIPC_CBPTR_NULL
	  && endpoint_cnt < *endpoint_array_size);
	 endpoint_cnt++, endpoint_cbptr = endpoint->pail_next_eg_endpoint) {
	endpoint = FLIPC_ENDPOINT_PTR(endpoint_cbptr);
	endpoint_array[endpoint_cnt] = endpoint;
    }

    if (endpoint_cbptr != FLIPC_CBPTR_NULL) {
	/* There were more to copy.  How many more?  */
	while (endpoint_cbptr != FLIPC_CBPTR_NULL) {
	    endpoint_cnt++;
	    endpoint = FLIPC_ENDPOINT_PTR(endpoint_cbptr);
	    endpoint_cbptr = endpoint->pail_next_eg_endpoint;
	}
	/* Tell the user.  */
	*endpoint_array_size = endpoint_cnt;

	/* Release the lock.  */
	flipc_simple_lock_release(&epgroup->pail_lock);

	/* And return.  */
	return FLIPC_EPGROUP_ENDPOINTS_MISSING;
    }

    /* We have copied all the endpoints.  */
    *endpoint_array_size = endpoint_cnt;

    flipc_simple_lock_release(&epgroup->pail_lock);

    return FLIPC_SUCCESS;
}

FLIPC_return_t FLIPC_epgroup_query(FLIPC_epgroup_t epgroup_arg,
				   FLIPC_epgroup_info_t epgroup_info)
{
    flipc_epgroup_t epgroup = epgroup_arg;

    flipc_simple_lock_acquire(&epgroup->pail_lock);

    if (!epgroup->sail_enabled) {
	flipc_simple_lock_release(&epgroup->pail_lock);
	return FLIPC_EPGROUP_INACTIVE;
    }

    epgroup_info->msgs_per_wakeup = epgroup->sail_msgs_per_wakeup;

    flipc_simple_lock_release(&epgroup->pail_lock);

    return FLIPC_SUCCESS;
}



