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
 * flipc/flipc_kfr.c
 *
 * Contains the kernel flipc routines (routines that are not
 * device driver entry points or part of the message engine).  
 */

/* Include file note; the include file <mach/flipc_cb.h> is included
   later in the file, with only those kfr routines that need to access
   the CB following it.  */
#include <flipc/flipc.h>
#include <flipc/flipc_usermsg.h>
#include <kern/sync_sema.h>
#include <kern/ipc_sync.h>

/* Maximum size the transport will ship for a single message.  */
int flipckfr_data_buffer_size = 0;

/* Size of log transport will use for keeping track of its
   errors, in bytes.  */
int flipckfr_transport_log_size = 0;

/* Has the buffer been initialized?  */
int flipckfr_buffer_initialized_p = 0;

/* 
 * For serializing initialization related accesses.
 */
struct flipc_gateway_lock init_gateway_lock;

#if REAL_TIME_PRIMITIVES
/*
 * Lock passed back to the user for allocations; initialized in
 * flipc_system_init.
 */
lock_set_t flipckfr_alloc_lock = LOCK_SET_NULL;
#endif /* REAL_TIME_PRIMITIVES */

/*
 * The array of semaphores that are associated with the various
 * endpoint groups, along with its size and a simple lock to protect
 * it.  
 *
 * The simple lock must be held in all accesses to this structure, with
 * a single exception.  The wakeup code (which may be called in
 * interrupt context) is called from the send/receive side processing on
 * the ME, and hence will only be called while the comm buffer is active.
 * This structure will always be initialized and valid for reading while
 * the comm buffer is active.
 *
 * This set of assumptions should be revisited when we adapt this
 * code to the paragon.
 */
static semaphore_t *flipckfr_epgroup_semaphore_array = (semaphore_t *)0;
static int flipckfr_epgroup_semaphore_array_size = 0;
decl_simple_lock_data(static,flipckfr_epgroup_semaphore_array_lock)

/*
 * Initialize flipc.
 */
void flipc_system_init(void)
{
    FLIPC_INIT_GATELOCK(init_gateway_lock);
    simple_lock_init(&flipckfr_epgroup_semaphore_array_lock, ETAP_MISC_FLIPC);

    flipcme_system_init(&flipckfr_data_buffer_size,
			&flipckfr_transport_log_size);
}

/*
 * This routine is called to wakeup a semaphore associated with an
 * endpoint group.  It should get some meat in it when we move to a
 * real-time base.
 * I do not need to take the flipckfr_epgroup_semaphore_array_lock here
 * because:
 *	1) I can trust that the array and size variables will stay valid
 *	   as long as the communications buffer is up, and this routine
 *	   cannot be called if the comm buffer is down, 
 *	2) I read an atomic value from the semaphore array itself, and I
 *	   don't care which value I get.
 *	3) It's semaphore_signal's problem what to do with a semaphore
 *	   reference that dies while we're using it (check this with
 *	   someone). 
 */
void flipckfr_wakeup_epgroup(int epgroup_idx)
{
    semaphore_t semaphore;

    /* Should be able to trust caller, but ... */
    if (epgroup_idx >= flipckfr_epgroup_semaphore_array_size
	|| epgroup_idx < 0)
	return;

    semaphore = flipckfr_epgroup_semaphore_array[epgroup_idx];

    if (semaphore == SEMAPHORE_NULL) return;

    /* The semaphore might have gone away by now, but presumably
       semaphore_signal can deal with that.  */
    semaphore_signal(semaphore);

    return;
}

/*
 * Is the buffer initialized?
 */
int flipckfr_cb_init_p(void)
{
    int result;

    FLIPC_ACQUIRE_GATELOCK(init_gateway_lock);
    result = flipckfr_buffer_initialized_p;
    FLIPC_RELEASE_GATELOCK(init_gateway_lock);
    return result;
}

/*
 * Should only be called when noone has the device open, and noone will be
 * allowed to open it until we're done.  This should be guaranteed by our
 * caller (usually the device close code).
 */
flipc_return_t flipckfr_teardown(void)
{
#if REAL_TIME_PRIMITIVES
    kern_return_t kr;
#endif
    int i;
    semaphore_t *old_epgroup_array;
    unsigned int old_epgroup_array_size;

    /* The above makes this paranoid, but do it anyway ... */
    FLIPC_ACQUIRE_GATELOCK(init_gateway_lock);

    if (!flipckfr_buffer_initialized_p) {
	FLIPC_RELEASE_GATELOCK(init_gateway_lock);
	return flipc_Buffer_Not_Initialized;
    }

    /* Would be nice if we could map all of the pages of the
       CB out of user space here.  I'm sure there's a way to do it;
       I just don't know what it is and I'm not going to worry
       about it now.  */

    /* Ask the ME to do its part of the teardown.  */
#if	PARAGON860 && !FLIPC_KKT
    flipcme_teardown();
#else	/* PARAGON860 && !FLIPC_KKT */
    flipcme_cb_teardown();
#endif	/* PARAGON860 && !FLIPC_KKT */

#if REAL_TIME_PRIMITIVES
    /* Get rid of the allocations lock.  */
    if (flipckfr_alloc_lock == LOCK_SET_NULL)
	panic("flipckfr_teardown: No allocations lock to destroy.\n");

    kr = lock_set_destroy(kernel_task, flipckfr_alloc_lock);
    if (kr != KERN_SUCCESS)
	panic("flipckfr_teardown: Could not destroy allocations lock.\n");
    flipckfr_alloc_lock = LOCK_SET_NULL;
#endif

    /* We are now uninitialized.  */
    flipckfr_buffer_initialized_p = 0;

    /* Scoot the semaphore array out from the variable while holding
       the lock.  */
    simple_lock(&flipckfr_epgroup_semaphore_array_lock);
    old_epgroup_array = flipckfr_epgroup_semaphore_array;
    old_epgroup_array_size = flipckfr_epgroup_semaphore_array_size;
    flipckfr_epgroup_semaphore_array = (semaphore_t *) 0;
    flipckfr_epgroup_semaphore_array_size = 0;
    simple_unlock(&flipckfr_epgroup_semaphore_array_lock);
    
    /* Now release the references at leisure.  */
    for (i = 0; i < old_epgroup_array_size; i++)
	if (old_epgroup_array[i] != SEMAPHORE_NULL)
	    semaphore_dereference(old_epgroup_array[i]);

    /* Deallocate the array's space.  */
    kfree((vm_offset_t) old_epgroup_array,
	  old_epgroup_array_size * sizeof(semaphore_t));
    flipckfr_epgroup_semaphore_array = (semaphore_t *) 0;
    flipckfr_epgroup_semaphore_array_size = 0;

    /* We're done.  */
    FLIPC_RELEASE_GATELOCK(init_gateway_lock);
    return flipc_Success;
}

/* 
 * Returns a naked send right to the semaphore associated with
 * a given epgroup.
 */
flipc_return_t flipckfr_get_semaphore_port(int epgroup_idx,
					   ipc_port_t *semaphore)
{
    semaphore_t actual_semaphore;
    
    simple_lock(&flipckfr_epgroup_semaphore_array_lock);

    if (flipckfr_epgroup_semaphore_array_size == 0) {
	simple_unlock(&flipckfr_epgroup_semaphore_array_lock);
	return flipc_Buffer_Not_Initialized;
    }

    if (epgroup_idx >= flipckfr_epgroup_semaphore_array_size
	|| epgroup_idx < 0) {
	simple_unlock(&flipckfr_epgroup_semaphore_array_lock);
	return flipc_Bad_Epgroup;
    }

    actual_semaphore = flipckfr_epgroup_semaphore_array[epgroup_idx];

    simple_unlock(&flipckfr_epgroup_semaphore_array_lock);

    *semaphore = convert_semaphore_to_port(actual_semaphore);

    return flipc_Success;
}

/*
 * Associate a semaphore with an endpoint group, possibly destroying
 * the send right to the semaphore previously there.  Passed a naked
 * send right and an index to associate it with.
 *
 * The semantics are "consume on success"; if I'm going to return
 * success, it's my responsibility to keep track of the port
 * reference.  If I'm returning failure, I can't deallocate the
 * port reference, and it's the responsibility of the routine that
 * called me to deal with the reference it passed.
 */
flipc_return_t flipckfr_epgroup_set_semaphore(int epgroup_idx,
					      ipc_port_t semaphore_port)
{
    semaphore_t old_semaphore;
    semaphore_t new_semaphore = convert_port_to_semaphore(semaphore_port);

    simple_lock(&flipckfr_epgroup_semaphore_array_lock);

    if (flipckfr_epgroup_semaphore_array_size == 0) {
	simple_unlock(&flipckfr_epgroup_semaphore_array_lock);
	return flipc_Buffer_Not_Initialized;
    }

    if (epgroup_idx >= flipckfr_epgroup_semaphore_array_size
	|| epgroup_idx < 0) {
	simple_unlock(&flipckfr_epgroup_semaphore_array_lock);
	return flipc_Bad_Epgroup;
    }

    /* If it's a real semaphore, take a reference on it, since we're
       going to be storing such a reference.  */
    if (new_semaphore != SEMAPHORE_NULL)
	semaphore_reference(new_semaphore);

    /* Transfer the old send right to a local reference, if we have an
       old send right.  */
    old_semaphore = flipckfr_epgroup_semaphore_array[epgroup_idx];

    /* Put the new semaphore send right in the array.  */
    flipckfr_epgroup_semaphore_array[epgroup_idx] = new_semaphore;

    simple_unlock(&flipckfr_epgroup_semaphore_array_lock);

    /* Release the old semaphore reference we had, if necessary.  */
    if (old_semaphore != SEMAPHORE_NULL)
	semaphore_dereference(old_semaphore);

    /* Release the send right on the port we were passed in (we no
       longer need to hold it since we now have a reference on the
       semaphore itself).  */
    if (semaphore_port != IP_NULL)
	ipc_port_release_send(semaphore_port);

    return flipc_Success;
}

#if REAL_TIME_PRIMITIVES
flipc_return_t flipckfr_allocations_lock(lock_set_t *lock)
{
    *lock = flipckfr_alloc_lock;
    return flipc_Success;
}
#endif

/*
 * Stuff in the kfr that touches the communciations buffer follows this
 * include.  Currently only the initialization routine.
 */
#include <mach/flipc_cb.h>
#include <mach/flipc_locks.h>

/*
 * Communications buffer initialization routine.  Called from main KFR
 * routines; it is the KFR's responsibility to make sure that this
 * routine is not called in inappropriate circumstances (eg. someone
 * using the CB, CB already initialized and not torndown, etc.
 */

flipc_return_t flipckfr_cb_init(int max_endpoints,
				int max_epgroups,
				int max_buffers,
				int max_buffers_per_endpoint,
				int allocations_lock_policy)
{
#if REAL_TIME_PRIMITIVES
    kern_return_t kr;
#endif

    flipc_data_buffer_t data_buffer;
    flipc_cb_ptr freelist_cbptr;
    int i;
    struct flipc_comm_buffer_ctl cb_ctl;

    flipc_endpoint_t endpoint_array;
    flipc_epgroup_t epgroup_array;
    flipc_cb_ptr *bufferlist_array;
    flipc_data_buffer_t data_buffer_array;

    semaphore_t *new_semaphore_array;
    
    /* Confirm that we have at least half-way reasonable arguments (positive)
       arguments.  Excessively large args will be caught by the space check
       below.  */
    if (max_endpoints < 0 || max_epgroups < 0
	|| max_buffers < 0 || max_buffers_per_endpoint < 0)
	return flipc_Bad_Arguments;

    /* Lock out other initialization related processes.  */
    FLIPC_ACQUIRE_GATELOCK(init_gateway_lock);

    if (flipckfr_buffer_initialized_p) {
	FLIPC_RELEASE_GATELOCK(init_gateway_lock);
	return flipc_Buffer_Initialized;
    }

    /* If we made it past the above check, the ME should not
       consider the buffer to be active.  If it does, something's gone
       badly wrong (there's database corruption between us and the ME.  */
    if (flipcme_buffer_active())
	panic("flipckfr_cb_init: Called while buffer active.");

    /*
     * Will what the user's asked for fit?
     * (We use max_buffers_per_endpoint+1 because the full
     * condition on an endpoint leaves one buffer pointer unused).
     */
    /*
     * The first couple of lines are include the "-sizeof(int)" because
     * the flipc_comm_buffer_ctl structure is a variable sized structure
     * which includes an array of unspecified length for the transport
     * error log.  This array is declared as being of size one, so to find
     * the actual size of the flipc_comm_buffer_ctl struct, you must subtract
     * the size of a single integer and add in the transport error log size.
     */
    if ((sizeof(struct flipc_comm_buffer_ctl)
	 + flipckfr_transport_log_size - sizeof(int)       
	 + max_endpoints * sizeof(struct flipc_endpoint)
	 + max_epgroups * sizeof(struct flipc_epgroup)
	 + (max_buffers_per_endpoint+1)*max_endpoints*sizeof(flipc_cb_ptr)
	 + max_buffers*flipckfr_data_buffer_size)
	> flipc_cb_length) {
	FLIPC_RELEASE_GATELOCK(init_gateway_lock);
	return flipc_No_Space;
    }

    /* Initialize the allocations lock.  This is done here because if the
       user has specified a bad priority, we want to catch it early.  */

#if REAL_TIME_PRIMITIVES
    /* Set up the allocations lock.  */
    if (flipckfr_alloc_lock != LOCK_SET_NULL)
	panic("flipc_cb_init: found alloc lock previous initialized.");
    
    kr = lock_set_create(kernel_task, &flipckfr_alloc_lock, 1,
			 allocations_lock_policy);
    if (kr != KERN_SUCCESS) {
	if (kr == KERN_INVALID_ARGUMENT)
	    return flipc_Bad_Arguments;
	panic ("flipc_cb_init: Couldn't create allocations lock.");
    }
    cb_ctl.allocations_lock_policy = allocations_lock_policy;
#else
    /*
     * Initialize poor man's allocations lock.
     */
    flipc_simple_lock_init(&cb_ctl.pail_alloc_lock);
#endif  

    /* 
     * Set what will eventually be the AIL's copy of the data buffer size,
     * and the AIL's copy of the local node address.  (We copy our local
     * value into the CB with a bcopy later.
     */
    /* Initialize the config variables.  */
    cb_ctl.performance.performance_valid = FLIPC_PERF;
    cb_ctl.kernel_configuration.real_time_primitives = REAL_TIME_PRIMITIVES;
    cb_ctl.kernel_configuration.no_bus_locking = !FLIPC_BUSLOCK;
    cb_ctl.kernel_configuration.message_engine_in_kernel
	= KERNEL_MESSAGE_ENGINE;

    /* Data buffer size.  */
    cb_ctl.data_buffer_size = flipckfr_data_buffer_size;

    /* Setting the local node address is the ME's responsibility, as
       it gets the information from the transport.  */

    /*
     * Initialize synchronization variables and send endpoint list.
     */
    cb_ctl.sail_request_sync = 0;
    cb_ctl.sme_respond_sync = 0;
    cb_ctl.sme_receive_in_progress = 0;
    cb_ctl.sail_send_endpoint_list = FLIPC_CBPTR_NULL;  

    /*
     * Initialize performance info.
     */
    cb_ctl.performance.messages_sent = 0;
    cb_ctl.performance.messages_received = 0;

    /*
     * Space carving.
     */

    /* Endpoints.  */
    cb_ctl.endpoint.start =
	(sizeof(struct flipc_comm_buffer_ctl) - sizeof(int)
	 + flipckfr_transport_log_size);
    cb_ctl.endpoint.number = max_endpoints;
    endpoint_array = FLIPC_ENDPOINT_PTR(cb_ctl.endpoint.start);

    /* Endpoint groups.  */
    cb_ctl.epgroup.start =
	cb_ctl.endpoint.start + max_endpoints * sizeof(struct flipc_endpoint);
    cb_ctl.epgroup.number = max_epgroups;
    epgroup_array = FLIPC_EPGROUP_PTR(cb_ctl.epgroup.start);

    /* Bufferlist.  */
    cb_ctl.bufferlist.start =
	cb_ctl.epgroup.start
	    + max_epgroups * sizeof(struct flipc_epgroup);
    cb_ctl.bufferlist.number =
	(max_buffers_per_endpoint+1) * max_endpoints;
    bufferlist_array = FLIPC_BUFFERLIST_PTR(cb_ctl.bufferlist.start);

    /* Buffers; done from end.  */
    cb_ctl.data_buffer.number = max_buffers;
    cb_ctl.data_buffer.start = (flipc_cb_length
				- max_buffers * flipckfr_data_buffer_size);
    data_buffer_array = FLIPC_DATA_BUFFER_PTR(cb_ctl.data_buffer.start);
    
    /*
     * Initialize endpoints.
     */
    freelist_cbptr = FLIPC_CBPTR_NULL;
    for (i = 0; i < cb_ctl.endpoint.number; i++) {
	endpoint_array[i].constda_type = FLIPC_Inactive; 
	endpoint_array[i].constdm_type = FLIPC_Inactive; 
	endpoint_array[i].sailda_dpb_or_enabled = 0;
	endpoint_array[i].saildm_dpb_or_enabled = 0;

	flipc_simple_lock_init(&endpoint_array[i].pail_lock);

	endpoint_array[i].pail_next_eg_endpoint = FLIPC_CBPTR_NULL;
	endpoint_array[i].sail_epgroup = FLIPC_CBPTR_NULL;

	/* Free list pointer.  */
	endpoint_array[i].sail_next_send_endpoint = freelist_cbptr;
	freelist_cbptr = FLIPC_CBPTR(&endpoint_array[i]);

	/* Buffer list stuff.  */

	/* Set endpoint vars correctly.  */
	endpoint_array[i].constda_my_buffer_list =
	    FLIPC_CBPTR(&bufferlist_array[i*(max_buffers_per_endpoint+1)]);
	endpoint_array[i].constdm_my_buffer_list =
	    FLIPC_CBPTR(&bufferlist_array[i*(max_buffers_per_endpoint+1)]);
	endpoint_array[i].constda_next_buffer_list =
	    FLIPC_CBPTR(&bufferlist_array[(i+1)*(max_buffers_per_endpoint+1)]);
	endpoint_array[i].constdm_next_buffer_list =
	    FLIPC_CBPTR(&bufferlist_array[(i+1)*(max_buffers_per_endpoint+1)]);
	endpoint_array[i].pme_release_ptr =
	    endpoint_array[i].pail_process_ptr = 
		endpoint_array[i].sail_release_ptr = 
		    endpoint_array[i].shrd_acquire_ptr = 
			endpoint_array[i].sme_process_ptr =
			    endpoint_array[i].constdm_my_buffer_list;

	/* We don't need to initialize the buffer list itself, as anything
	   outside of the list for a particular endpoint is ignored, and
	   everything is currently outside the list.  */
    }
    /* Link all of them into the global freelist.  */
    cb_ctl.endpoint.free = freelist_cbptr;

    /* 
     * Initialize endpoint groups.
     */
    freelist_cbptr = FLIPC_CBPTR_NULL;
    for (i = 0; i < cb_ctl.epgroup.number; i++) {
	flipc_simple_lock_init(&epgroup_array[i].pail_lock);
	epgroup_array[i].sail_enabled = 0;
	epgroup_array[i].pail_version = 0;
	epgroup_array[i].pail_first_endpoint = FLIPC_CBPTR_NULL;
	epgroup_array[i].pail_free = freelist_cbptr;
	freelist_cbptr = FLIPC_CBPTR(&epgroup_array[i]);
    }
    cb_ctl.epgroup.free = freelist_cbptr;

    /*
     * Initialize data buffers.
     */
    freelist_cbptr = FLIPC_CBPTR_NULL;
    for (i = 0; i < cb_ctl.data_buffer.number; i++) {
	data_buffer = FLIPC_DATA_BUFFER_PTR(cb_ctl.data_buffer.start
					    + i * flipckfr_data_buffer_size);
	data_buffer->u.free = freelist_cbptr;
	data_buffer->shrd_state = flipc_Free;
	freelist_cbptr = FLIPC_CBPTR(data_buffer);
    }
    cb_ctl.data_buffer.free = freelist_cbptr;

    /*
     * Copy control structure over into the CB.
     */
    bcopy((char *) &cb_ctl, (char *) flipc_cb_base, sizeof(cb_ctl));

    /*
     * Setup the error log by zeroing all the counters and setting the
     * transport log size and counter validity flag.  The inactive
     * counter will be copied in during the ME specific
     * initialization.
     */
    bzero((char*) &((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log,
	  (sizeof(struct FLIPC_domain_errors) - sizeof(int)
	   + flipckfr_transport_log_size));

    ((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.transport_error_size
	= flipckfr_transport_log_size;
    ((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.error_full_config_p
	= FLIPC_UCHECK;

    /*
     * Allocate the EPGROUP semaphore array.
     */
    new_semaphore_array = 
	(semaphore_t *) kalloc(cb_ctl.epgroup.number * sizeof(semaphore_t));

    simple_lock(&flipckfr_epgroup_semaphore_array_lock);

    assert(flipckfr_epgroup_semaphore_array == (semaphore_t *) 0);

    flipckfr_epgroup_semaphore_array = new_semaphore_array;
    flipckfr_epgroup_semaphore_array_size = cb_ctl.epgroup.number;

    for (i = 0; i < cb_ctl.epgroup.number; i++)
	flipckfr_epgroup_semaphore_array[i] = SEMAPHORE_NULL;

    simple_unlock(&flipckfr_epgroup_semaphore_array_lock);

    /* Mark the buffer as initialized.  */
    flipckfr_buffer_initialized_p = 1;

    /*
     * Tell the ME to make the buffer active.  Tell it what you've done
     * while you're at it.
     */
#if	PARAGON860 && !FLIPC_KKT
    flipcme_buffer_setup(cb_ctl.endpoint.start, cb_ctl.endpoint.number,
			       cb_ctl.epgroup.start, cb_ctl.epgroup.number,
			       cb_ctl.bufferlist.start, cb_ctl.bufferlist.number,
			       cb_ctl.data_buffer.start, cb_ctl.data_buffer.number);

#else	/* PARAGON860 && !FLIPC_KKT */
    flipcme_make_buffer_active(cb_ctl.endpoint.start, cb_ctl.endpoint.number,
			       cb_ctl.epgroup.start, cb_ctl.epgroup.number,
			       cb_ctl.bufferlist.start, cb_ctl.bufferlist.number,
			       cb_ctl.data_buffer.start, cb_ctl.data_buffer.number);
#endif	/* PARAGON860 && !FLIPC_KKT */
    
    /* Release init lock before taking off.  */
    FLIPC_RELEASE_GATELOCK(init_gateway_lock);

    return flipc_Success;
}
