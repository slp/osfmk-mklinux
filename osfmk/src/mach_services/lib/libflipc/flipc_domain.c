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
 * libflipc/flipc_domain.c
 *
 * Flipc domain related routines.  
 */

#include "flipc_ail.h"
#include <string.h>
#include <mach/mach_norma.h>
#include <mach/norma_special_ports.h>
#include <mach_error.h>

/*
 * Simple lock yield function initialization.
 */
FLIPC_thread_yield_function flipc_simple_lock_yield_fn =
(FLIPC_thread_yield_function) 0;

/*
 * Chain of open domains.
 */
domain_lookup_t domain_lookup_chain = DOMAIN_LOOKUP_NULL;

/*
 * Routines for modifying the domain lookup chain.  These routines
 * assume that they are the only routines modifying this structure.
 * The parent routine is responsible for guarding against
 * multi-threaded accesses (probably by taking
 * domain_op_lock, below).
 */

/* Find the correct lookup structure for a domain handle; returns
   null pointer if not found.  */
extern domain_lookup_t domain_handle_lookup(mach_port_t domain_handle)
{
    domain_lookup_t dptr;

    for (dptr = domain_lookup_chain; dptr; dptr = dptr->next)
	if (dptr->device_port == domain_handle)
	    break;

    return dptr;
}

/* Find the correct lookup structure for an arbitrary pointer.
   Returns a null pointer if not found.  */
extern domain_lookup_t domain_pointer_lookup(void *iptr)
{
    unsigned char *cptr = iptr;
    domain_lookup_t dptr;

    for (dptr = domain_lookup_chain; dptr; dptr = dptr->next)
	if (cptr >= dptr->comm_buffer_base
	    && cptr < dptr->comm_buffer_base + dptr->comm_buffer_length)
	    break;

    return dptr;
}  

/*
 * Allocate a new domain lookup structure given the passed parameters;
 * returns null if allocation failed, the structure allocated
 * otherwise.
 */
static domain_lookup_t
domain_lookup_allocate(mach_port_t dev_port,
		       unsigned char *cb_base,
		       unsigned long cb_length,
		       FLIPC_thread_yield_function yield_fn
#if REAL_TIME_PRIMITIVES
		       , lock_set_port_t alloc_lock
#endif
		       )
{
    domain_lookup_t result = (domain_lookup_t)
	malloc(sizeof(struct domain_lookup));

    if (!result) return result;

    result->device_port = dev_port;
    result->comm_buffer_base = cb_base;
    result->comm_buffer_length = cb_length;
    result->yield_fn = yield_fn;
#if REAL_TIME_PRIMITIVES
    result->allocations_lock = alloc_lock;
#endif

    result->next = domain_lookup_chain;
    domain_lookup_chain = result;

    return result;
}

/*
 * Deallocat a domain lookup structure, snipping it off the chain.
 * Return a pointer to the structure if deallocation was successful,
 * or null if the structure wasn't found.
 */
static domain_lookup_t domain_lookup_deallocate(mach_port_t dev_port)
{
    domain_lookup_t *last, ptr;

    for (last = &domain_lookup_chain, ptr = *last;
	 ptr;
	 last = &ptr->next, ptr = *last)
	if (ptr->device_port == dev_port) {
	    *last = ptr->next;
	    return ptr;
	}

    return DOMAIN_LOOKUP_NULL;
}

/*
 * domain_op_lock is a flipc simple lock taken while a domain
 * op (init, attach, detach, or query) is occuring; if it
 * is taken when a domain operation is initiated, that operation
 * will return with error code FLIPC_DOMAIN_OP_IN_PROGRESS.
 *
 * Note that this lock may be contested by threads in different
 * priority classes, so flipc_simple_lock_acquire should never
 * be called on it; only flipc_simple_lock_try.
 */
FLIPC_DECL_SIMPLE_LOCK(static,domain_op_lock);

/*
 * Helper routines for initialization and attaching.
 */

#define FLIPC_DEVICE_BASENAME "usermsg"

/*
 * Open the device.
 *
 * This goes through the hassle of opening a mach device from scratch.
 * It also confirms that we haven't previously opened the device.
 * It is called from both FLIPC_domain_init and FLIPC_domain_attach.
 *
 * It takes a domain index that indicates which domain's device to
 * open, and a pointer to a mach_port_t, which is filled in with the
 * device port if the device is successfully opened.
 *
 * There are no side effects except for the device being openned (all ports
 * used by this routine are deallocated before return). */
static FLIPC_return_t flipc_device_open(FLIPC_domain_index_t domain_index,
					mach_port_t *device_port_ptr)
{
    mach_port_t my_task = mach_task_self(),  privileged_host_port;
    mach_port_t	device_port;
    extern mach_port_t bootstrap_port;	/* from libmach */
    extern mach_port_t mach_host_priv_self(); /* from libmach */
    kern_return_t kr;
    int node;
    char device_name[sizeof(FLIPC_DEVICE_BASENAME)+3];
    security_token_t	security_token;
    char
	*device_name_ptr = device_name,
	*device_basename_ptr = FLIPC_DEVICE_BASENAME;

    /* Range check the domain index.  */
    if (domain_index >= 100)
	return FLIPC_INVALID_ARGUMENTS;

    /* Figure out the device name.  This could be easily done use sprintf,
       but we cannot rely on sprintf being reentrant.  */
    while (*device_basename_ptr)
	*device_name_ptr++ = *device_basename_ptr++;

    /* Have just copied everything but the trailing null, and are
       pointing at the next entry in device_name_ptr.  Put in the
       final touches.  */

    *device_name_ptr++ = (domain_index / 10) + '0';
    *device_name_ptr++ = (domain_index % 10) + '0';
    *device_name_ptr++ = '\0';
	
    /* No need to check length; we know exactly how many bytes
       we've copied.   */

    /* Open the device.  This is a real pain, and for now requires
       root priveleges.  */
    privileged_host_port = mach_host_priv_self();
    if (privileged_host_port == MACH_PORT_NULL) {
	return FLIPC_DOMAIN_NOT_AVAILABLE;
    }

    /* Don't use the device server port directly; we want to make sure
       we have the right device for the node we're on.  */

    kr = norma_port_location_hint(my_task, my_task, &node);
    if (kr != KERN_SUCCESS) {
	mach_error("norma_node_self fails", kr);
	return FLIPC_DOMAIN_NOT_AVAILABLE;
    }

    kr = norma_get_device_port(privileged_host_port, node, &device_port);
    if (kr != KERN_SUCCESS) {
	mach_error("norma_get_device_port fails",kr);
	mach_port_deallocate(my_task, privileged_host_port);
	return FLIPC_DOMAIN_NOT_AVAILABLE;
    }

    kr = device_open(device_port,
		     (mach_port_t)0,
		     D_READ|D_WRITE,
		     security_token,
		     device_name, 
		     device_port_ptr);

    if (kr != KERN_SUCCESS) {
	mach_port_deallocate(my_task, privileged_host_port);
	mach_port_deallocate(my_task, device_port);
	return FLIPC_DOMAIN_NOT_AVAILABLE;
    }

    /*
     * Confirm that we haven't opened the device before.
     */
    if (domain_handle_lookup(*device_port_ptr)) {
	    device_close(*device_port_ptr);
	    mach_port_deallocate(my_task, privileged_host_port);
	    mach_port_deallocate(my_task, device_port);
	    return FLIPC_DOMAIN_ATTACHED;
    }

    /* Clean up.  */
    mach_port_deallocate(my_task, privileged_host_port);
    mach_port_deallocate(my_task, device_port);

    /* We got it; return success.  */
    return FLIPC_SUCCESS;
}

/*
 * Attach to the communications buffer.
 *
 * Takes two args: the device port (gotten by the above routine),
 * and a yield function.  This maps the communications buffer in and sets
 * up local flipc data structures for support.
 *
 * It is called by both the FLIPC_domain_init and FLIPC_domain_attach
 * routines.
 *
 * Side effects:
 *	comm buffer mapped into memory
 *	domain lookup struct created and linked into chain.
 * 	global thread yield function set
 *	flipc_cb_base and flipc_cb_length set.
 */
static FLIPC_return_t flipc_cb_attach(mach_port_t device_port,
				      FLIPC_thread_yield_function yield_fn)
{
    kern_return_t kr;
    mach_port_t memobj_port;
    mach_port_t my_task = mach_task_self();
    vm_address_t comm_buffer_base = (vm_address_t) 0;
    flipc_comm_buffer_ctl_t cb_ctl;

    /* Get the device mapping port.  */
    kr = device_map(device_port,
		    VM_PROT_READ|VM_PROT_WRITE,
		    0,
		    COMM_BUFFER_SIZE,
		    &memobj_port,
		    0);
    if (kr != KERN_SUCCESS) {
	/* I'm not really sure what this means, so I'll return a
	   confusing error code.  */
	return FLIPC_KERNEL_TRANSLATION_ERR;
    }

    /* Map in the communications buffer.  */
    kr = vm_map(my_task,
		&comm_buffer_base,
		COMM_BUFFER_SIZE,
		0,		/* Mask. */
		TRUE,		/* Map anywhere? */
		memobj_port,
		0,		/* Offset in object.  */
		FALSE,		/* Copy?  */
		VM_PROT_READ|VM_PROT_WRITE, /* Cur protection.  */
		VM_PROT_READ|VM_PROT_WRITE, /* Max protection.  */
		VM_INHERIT_NONE);
    if (kr != KERN_SUCCESS) {
	/* I'm not really sure what this means, so I'll return a
	   confusing error code.  */
	mach_port_deallocate(my_task, memobj_port);
	return FLIPC_KERNEL_TRANSLATION_ERR;
    }

    /* Confirm that the AIL and kernel have matching configurations
       options.  */
    cb_ctl = (flipc_comm_buffer_ctl_t) comm_buffer_base;
    if ((cb_ctl->kernel_configuration.real_time_primitives
	 != REAL_TIME_PRIMITIVES)
	|| (cb_ctl->kernel_configuration.message_engine_in_kernel
	    != KERNEL_MESSAGE_ENGINE)
	/* The kernel says to bus lock and we say not to.  */
	|| (!cb_ctl->kernel_configuration.no_bus_locking
	    && !FLIPC_BUSLOCK)) {
	/* There's some important incompatibility between the way the
	   kernel things the world works and the way we think that the
	   world works.  */
	mach_port_deallocate(my_task, memobj_port);
	vm_deallocate(my_task,
		      (vm_address_t) comm_buffer_base,
		      (vm_size_t) COMM_BUFFER_SIZE);
	return FLIPC_KERNEL_INCOMPAT;
    }
    
#if REAL_TIME_PRIMITIVES
    {
	int dev_status[1];
	unsigned int dev_status_count = 1;

	kr = device_get_status(device_port,
			       FLIPC_DEVICE_FLAVOR(usermsg_Return_Allocations_Lock,
						   0),
			       dev_status,
			       &dev_status_count);
	if (kr != KERN_SUCCESS) {
	    /* I believe this means that I'm incompatible with the kernel in
	       some way.  I'm going to come up with the best error message I
	       can and abort.  */

	    switch (kr) {
	      case D_INVALID_OPERATION:
		fprintf_stderr("Flipc Library is configured for real-time support,\n");
		fprintf_stderr("but underlying kernel is not. \n");
		exit(-1);
	      case D_DEVICE_DOWN:
		fprintf_stderr("Communications buffer dissapeared underneath us.\n");
		exit(-1);
	    }
	    /* NOTREACHED */
	}

	if (!domain_lookup_allocate(device_port,
				    (unsigned char*) comm_buffer_base,
				    COMM_BUFFER_SIZE,
				    yield_fn,
				    (lock_set_port_t) dev_status[0])) {
	    fprintf_stderr("Could not allocate internal flipc memory.\n");
	    exit(-1);
	}
    }
#else
    if (!domain_lookup_allocate(device_port,
				(unsigned char*) comm_buffer_base,
				COMM_BUFFER_SIZE,
				yield_fn)) {
	fprintf_stderr("Could not allocate internal flipc memory.\n");
	exit(-1);
    }
#endif

    /* We are here assuming single domains, so we set some global variables.  */
    flipc_cb_base = (unsigned char *) comm_buffer_base;
    flipc_cb_length = COMM_BUFFER_SIZE;
    if (flipc_simple_lock_yield_fn) {
	if(flipc_simple_lock_yield_fn != yield_fn) {
	    fprintf_stderr("Conflict between old and new values of "
			   "yield function.\n");
	    exit(-1);
	}
    } else
	flipc_simple_lock_yield_fn = yield_fn;

    /* Cleanup.  */
    mach_port_deallocate(my_task, memobj_port);

    /* Success!  */
    return FLIPC_SUCCESS;
}  

/*
 * Initialize the domain.
 */

FLIPC_return_t FLIPC_domain_init(FLIPC_domain_index_t domain_index,
				 FLIPC_domain_info_t domain_params,
				 FLIPC_domain_t *domain_handle)
{
    FLIPC_return_t fr;
    int dev_status[5];
    mach_port_t msg_device_port;
    kern_return_t kr;
    flipc_comm_buffer_ctl_t cb_ctl;
    
    /* Is the yield function good?  */
    if (!domain_params->yield_fn)
	return FLIPC_DOMAIN_NO_THREAD_YIELD;

    /* Is everything zero that should be?  */
    if (domain_params->msg_buffer_size != 0
	|| domain_params->error_log_size != 0
	|| domain_params->performance.messages_sent != 0
	|| domain_params->performance.messages_received != 0
	|| domain_params->performance.performance_valid != 0)
	return FLIPC_INVALID_ARGUMENTS;

    /* Protect against user races.  */
    if (!flipc_simple_lock_try(&domain_op_lock))
	return FLIPC_DOMAIN_OP_IN_PROGRESS;

    /* Open the device.  */
    fr = flipc_device_open(domain_index, &msg_device_port);
    if (fr != FLIPC_SUCCESS) {
	flipc_simple_lock_release(&domain_op_lock);
	return fr;
    }
    
    /* Try to initialize it.  */
    dev_status[0] = domain_params->max_endpoints;
    dev_status[1] = domain_params->max_epgroups;
    dev_status[2] = domain_params->max_buffers;
    dev_status[3] = domain_params->max_buffers_per_endpoint;
#if REAL_TIME_PRIMITIVES
    dev_status[4] = domain_params->policy;
#else
    /* Filler, since I want to change the calling conventions now.  */
    dev_status[4] = 0;
#endif

    kr = device_set_status(msg_device_port,
			   FLIPC_DEVICE_FLAVOR(usermsg_Init_Buffer, 0),
			   dev_status,
			   5);
    if (kr != KERN_SUCCESS) {
	device_close(msg_device_port);
	flipc_simple_lock_release(&domain_op_lock);

	switch (kr) {
	  case D_ALREADY_OPEN:
	    return FLIPC_DOMAIN_INITIALIZED;
	  case D_INVALID_SIZE:
	    return FLIPC_DOMAIN_RESOURCE_SHORTAGE;
	  case D_INVALID_OPERATION:
	    return FLIPC_INVALID_ARGUMENTS;
	  default:
	    return FLIPC_KERNEL_TRANSLATION_ERR;
	}
    }

    /* The buffer's been initialized; now attach to it.  */
    fr = flipc_cb_attach(msg_device_port, domain_params->yield_fn);
    if (fr != FLIPC_SUCCESS) {
	if (fr == FLIPC_KERNEL_INCOMPAT) {
	    /* This error code's ok; we know what to do with it.  */
	    device_close(msg_device_port);
	    flipc_simple_lock_release(&domain_op_lock);
	    return fr;
	}
	/* Shouldn't happen; we give up violently.  */
	fprintf_stderr("Flipc failed to attach communications buffer after opening.\n");
	exit(-1);
    }

    /* Domain op no longer in progress.  */
    flipc_simple_lock_release(&domain_op_lock);

    /* Return domain handle.  */
    *domain_handle = msg_device_port;

    /* Comm buffer should now be mapped in.  */
    cb_ctl = (flipc_comm_buffer_ctl_t) flipc_cb_base;

    /* Fill in values that weren't provided by the user.  */
    domain_params->msg_buffer_size =
	cb_ctl->data_buffer_size - sizeof(struct flipc_data_buffer);
    domain_params->error_log_size =
	(sizeof(struct FLIPC_domain_errors)
	 + cb_ctl->sme_error_log.transport_error_size - sizeof(int));
    domain_params->performance = cb_ctl->performance;

    /* Success!  */
    return FLIPC_SUCCESS;
}

/*
 * Attach to a previous initialized domain.
 */
int FLIPC_domain_attach(FLIPC_domain_index_t domain_index,
			FLIPC_domain_info_t domain_params,
			FLIPC_domain_t *domain_handle)
{
    FLIPC_return_t fr;
    int dev_status[1];
    unsigned int dev_status_count;
    kern_return_t kr;
    mach_port_t msg_device_port;
    flipc_comm_buffer_ctl_t cb_ctl;

    /* Is everything zero that should be?  */
    if (domain_params->msg_buffer_size != 0
	|| domain_params->error_log_size != 0
	|| domain_params->performance.messages_sent != 0
	|| domain_params->performance.messages_received != 0
	|| domain_params->performance.performance_valid != 0
	|| domain_params->max_endpoints != 0
	|| domain_params->max_epgroups != 0
	|| domain_params->max_buffers != 0
	|| domain_params->max_buffers_per_endpoint != 0
	|| domain_params->policy != 0)
	return FLIPC_INVALID_ARGUMENTS;

    /* Is the yield function good?  */
    if (!domain_params->yield_fn)
	return FLIPC_DOMAIN_NO_THREAD_YIELD;

    /* Protect against user races.  */
    if (!flipc_simple_lock_try(&domain_op_lock))
	return FLIPC_DOMAIN_OP_IN_PROGRESS;

    /* Open the device.  */
    fr = flipc_device_open(domain_index, &msg_device_port);
    if (fr != FLIPC_SUCCESS) {
	flipc_simple_lock_release(&domain_op_lock);
	return fr;
    }
    
    /* We haven't been here before, have we?  This relies on device open
       returning the same port name for multiple opens of the device port.  */
    if (domain_handle_lookup(msg_device_port)) {
	/* Yes, we have.  Close down.  */
	device_close(msg_device_port);
	flipc_simple_lock_release(&domain_op_lock);
	return FLIPC_DOMAIN_ATTACHED;
    }

    /* Confirm that it is already initialized.  */
    dev_status_count = 1;
    kr = device_get_status(msg_device_port,
			   FLIPC_DEVICE_FLAVOR(usermsg_Get_Initialized_Status,
					       0),
			   dev_status,
			   &dev_status_count);
    if (kr != KERN_SUCCESS) {
	fprintf_stderr("Kernel refused initialization status request.\n");
	exit(-1);
    }

    if (!dev_status[0]) {
	device_close(msg_device_port);
	flipc_simple_lock_release(&domain_op_lock);

	return FLIPC_DOMAIN_NOT_INITIALIZED;
    }

    fr = flipc_cb_attach(msg_device_port, domain_params->yield_fn);
    if (fr != FLIPC_SUCCESS) {
	fprintf_stderr("Flipc failed to attach communications buffer after opening.\n");
	exit(-1);
    }

    /* Domain op no longer in progress.  */
    flipc_simple_lock_release(&domain_op_lock);

    /* Return domain handle.  */
    *domain_handle = msg_device_port;

    /* Comm buffer should now be mapped in.  */
    cb_ctl = (flipc_comm_buffer_ctl_t) flipc_cb_base;

    /* Fill in values unset by user on entry.  */
    domain_params->msg_buffer_size =
	cb_ctl->data_buffer_size - sizeof(struct flipc_data_buffer);
    domain_params->error_log_size =
	(sizeof(struct FLIPC_domain_errors)
	 + cb_ctl->sme_error_log.transport_error_size - sizeof(int));     
    domain_params->max_endpoints = cb_ctl->endpoint.number;	
    domain_params->max_epgroups = cb_ctl->epgroup.number;
    domain_params->max_buffers = cb_ctl->data_buffer.number;
    domain_params->max_buffers_per_endpoint =
	(cb_ctl->bufferlist.number / cb_ctl->endpoint.number) - 1;
#if REAL_TIME_PRIMITIVES
    domain_params->policy = cb_ctl->allocations_lock_policy;
#else
    domain_params->policy = 0;
#endif
    domain_params->performance = cb_ctl->performance;

    /* Success!  */
    return FLIPC_SUCCESS;
}

FLIPC_return_t FLIPC_domain_detach(FLIPC_domain_t domain_handle)
{
    domain_lookup_t dl;
    mach_port_t my_task;
    kern_return_t kr;

    /* Protect against user races.  */
    if (!flipc_simple_lock_try(&domain_op_lock))
	return FLIPC_DOMAIN_OP_IN_PROGRESS;

    /* Find the domain information.  */
    dl = domain_lookup_deallocate(domain_handle);
    if (!dl) {
	flipc_simple_lock_release(&domain_op_lock);
	return FLIPC_DOMAIN_NOT_ATTACHED;
    }

    /* Unmap the memory.  */
    my_task = mach_task_self();
    kr = vm_deallocate(my_task,
		       (vm_address_t) dl->comm_buffer_base,
		       (vm_size_t) dl->comm_buffer_length);
    if (kr != KERN_SUCCESS) {
	if (kr == KERN_INVALID_ADDRESS)
	    fprintf_stderr("Internal data structures corrupted.\n");
	else
	    fprintf_stderr("Kernel won't unmap comm buffer.\n");
	exit(-1);
    }

    /* Cleanup.  */
    device_close(domain_handle);
#if REAL_TIME_PRIMITIVES
    mach_port_deallocate(my_task, dl->allocations_lock);
#endif
    free(dl);

    /* Release lock and go home.  */
    flipc_simple_lock_release(&domain_op_lock);

    return FLIPC_SUCCESS;
}

/*
 * Get domain attributes.
 */
FLIPC_return_t FLIPC_domain_query(FLIPC_domain_t domain_handle,
				  FLIPC_domain_info_t info)
{
    domain_lookup_t dl;
    flipc_comm_buffer_ctl_t cb_ctl;

    /* Protect against user races.  */
    if (!flipc_simple_lock_try(&domain_op_lock))
	return FLIPC_DOMAIN_OP_IN_PROGRESS;

    /* Find the domain information.  */
    dl = domain_handle_lookup(domain_handle);
    if (!dl) {
	flipc_simple_lock_release(&domain_op_lock);
	return FLIPC_DOMAIN_NOT_ATTACHED;
    }

    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;
    
    /* Fill in the structure.  */
    info->max_endpoints = cb_ctl->endpoint.number;
    info->max_epgroups = cb_ctl->epgroup.number;
    info->max_buffers = cb_ctl->data_buffer.number;
    info->max_buffers_per_endpoint =
	(cb_ctl->bufferlist.number / cb_ctl->endpoint.number) - 1;
    info->msg_buffer_size =
	cb_ctl->data_buffer_size - sizeof(struct flipc_data_buffer);
    info->yield_fn = dl->yield_fn;
    info->performance = cb_ctl->performance;
    info->error_log_size =
	(sizeof(struct FLIPC_domain_errors)
	 + cb_ctl->sme_error_log.transport_error_size - sizeof(int));     
#if REAL_TIME_PRIMITIVES
    info->policy = cb_ctl->allocations_lock_policy;
#else
    info->policy = 0;
#endif    

    /* Release the domain_op_lock.  */
    flipc_simple_lock_release(&domain_op_lock);

    return FLIPC_SUCCESS;
}

FLIPC_return_t FLIPC_domain_error_query(FLIPC_domain_t domain_handle,
					FLIPC_domain_errors_t error_log)
{
    domain_lookup_t dl;
    flipc_comm_buffer_ctl_t cb_ctl;

    /* Protect against user races.  */
    if (!flipc_simple_lock_try(&domain_op_lock))
	return FLIPC_DOMAIN_OP_IN_PROGRESS;

    /* Find the domain information.  */
    dl = domain_handle_lookup(domain_handle);
    if (!dl) {
	flipc_simple_lock_release(&domain_op_lock);
	return FLIPC_DOMAIN_NOT_ATTACHED;
    }

    cb_ctl = (flipc_comm_buffer_ctl_t) dl->comm_buffer_base;
    
    /* Fill in the structure.  */
    bcopy((const char *) &cb_ctl->sme_error_log, (char *) error_log,
	  sizeof(struct FLIPC_domain_errors) - sizeof(int)
	  + cb_ctl->sme_error_log.transport_error_size);

    flipc_simple_lock_release(&domain_op_lock);

    return FLIPC_SUCCESS;
}


#ifdef KERNEL_MESSAGE_ENGINE
int disable_me_kick = 0;

/*
 * Routine to get the message engine going in a device driver
 * implementation.
 */
void flipc_really_kick_message_engine(void *ptr)
{
    domain_lookup_t dl = domain_pointer_lookup(ptr);			
    kern_return_t kr = 							
	device_set_status(dl->device_port,				
			  FLIPC_DEVICE_FLAVOR(usermsg_Process_Work, 0),	
			  0, 0);						
    
    if (kr != KERN_SUCCESS) {						
	fprintf_stderr("Kernel refused to kick message engine.\n");	
	exit(-1);
    }
}

void flipc_kick_message_engine(void *ptr)
{
    if (!disable_me_kick)
	flipc_really_kick_message_engine(ptr);
}
#endif	/* KERNEL_MESSAGE_ENGINE */
