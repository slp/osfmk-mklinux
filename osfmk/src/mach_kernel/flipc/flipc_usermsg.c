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
 * flipc/flipc_usermsg.c
 *
 * The device driver entry points for flipc.
 */

/*
 * One of the major things this file does is translate port names
 * (user space), into actual ports for the kernel.  Normally this is
 * taken care of by MiG, but since we are using the device driver
 * interface we don't have that option.  So instead we take the
 * integer port names passed to us by the user and translate them in
 * the context of the current task.  This is almost fine: the fly in
 * the ointment is that if we are handling device calls from another
 * node, the current task may not be the task who initiated the call.
 * There's no way around this given the current device interface.
 * However, it would make no sense to do FLIPC related device calls
 * remotely, as the whole point of flipc is doing communications
 * remotely through a locally available communications buffer.  So for
 * our purposes, it shouldn't matter that the translation isn't quite
 * perfect.
 */

#include <flipc/flipc.h>
#include <flipc/flipc_usermsg.h>
#include <mach/flipc_cb.h>
#include <kern/sync_sema.h>
#include <kern/ipc_sync.h>
#if REAL_TIME_PRIMITIVES
#include <kern/sync_lock.h>
#endif

/* 
 * This is exactly the file for inline functions, so if I can have them,
 * I will.
 */
#ifdef __GNUC__
#define INLINE inline
#else /* __GNUC__ */
#define INLINE
#endif /* __GNUC__ */

#include <machine/flipc_page.h>

/* 
 * Variables pointing to the comm buffer itself.  These are initialized
 * through the contiguous physical memory allocation structure described in
 * vm/vm_page.h and defined in vm/vm_resident.c.
 */
u_char *flipc_cb_base = (u_char *) 0;
unsigned long flipc_cb_length = COMM_BUFFER_SIZE;

/*
 * Routine called to open the device.  Not much for it to do.
 */
io_return_t usermsg_open(dev_t dev,
			 dev_mode_t flag,
			 io_req_t ior)
{
    if (dev > 0)
	return D_NO_SUCH_DEVICE;
    else
	return D_SUCCESS;
}

/*
 * Routine called to figure out how to map the device into
 * user space.  Returns the physical page in the CB if the request
 * is valid, or an error.  The request is not valid if the address is
 * out of bounds.
 *
 * It would be good to make it invalid if the CB isn't initialized, but that
 * would not have consistent semantics (depending on whether something
 * forced a remapping of the page or not), so for now we leave it
 * valid in that case.
 */
vm_offset_t
usermsg_map(dev_t dev,
	    vm_offset_t off,
	    vm_prot_t prot)
{
    if (off >= flipc_cb_length)
	return -1;

    return (FLIPC_BTOP(pmap_extract(pmap_kernel(),
				    (vm_offset_t) flipc_cb_base + off)));
}

void
usermsg_close(dev_t dev)
{
    /* This routine is only called if there are no open references to this
       port, and is called with the device marked as closing (so no new
       references will be created while we close).  So it is safe to go
       ahead and tear down the communications buffer.  */

    flipckfr_teardown();
}

/*
 * Following are the get_status and set_status routines for the
 * usermsg device driver.  These implement various out-of-band
 * communications between the user and the kernel routines that manage
 * flipc.
 *
 * See flipc_device.h for a list of device operations, along with the
 * arguments they require.
 */

io_return_t
usermsg_getstat(dev_t dev,
		dev_flavor_t flavor,
		dev_status_t data,
		mach_msg_type_number_t *count)
{
    usermsg_devop_t request = flavor >> 16;
    int param = flavor & 0xffff;

    switch(request) {
      case usermsg_Get_Initialized_Status: /* Is the buffer initialized?  */
	/* Do we have enough room?  */
	if (*count < 1)
	    return D_INVALID_OPERATION;

	/* Get the result.  */
	data[0] = flipckfr_cb_init_p();
	*count = 1;

	return D_SUCCESS;

      case usermsg_Get_Epgroup_Semaphore: /* What is this epgroup's
					     semaphore?  */
	{
	    ipc_port_t semaphore_port;
	    flipc_return_t fr;
	    
	    /* Do we have enough room?  */
	    if (*count < 1)
		return D_INVALID_OPERATION;

	    /* Get the semaphore port.  */
	    fr = flipckfr_get_semaphore_port(param, &semaphore_port);

	    if (fr == flipc_Bad_Epgroup)
		return D_INVALID_OPERATION;
	    
	    /* The routine should only return Bad_Epgroup and Not_Initialized,
	       but I want to catch anything other than success *somewhere*.  */
	    if (fr != flipc_Success)
		return D_DEVICE_DOWN;

	    /* Translate the port we have into the user's space.  */
	    data[0] = 
		ipc_port_copyout_send(semaphore_port, current_space());
	    *count = 1;
	    return D_SUCCESS;
	}

      case usermsg_Return_Allocations_Lock:	/* What is the allocations
						   lock?  */
#if !REAL_TIME_PRIMITIVES
	return D_INVALID_OPERATION;
#else
	{
	    lock_set_t alloc_lock;
	    ipc_port_t lock_port;
	    flipc_return_t fr;

	    if (*count < 1)
		return D_INVALID_OPERATION;

	    /* Get the lock.  */
	    fr = flipckfr_allocations_lock(&alloc_lock);
	    if (fr != flipc_Success)
		return D_DEVICE_DOWN;

	    /* Convert it to a port.  This should be a new send right
	       which I can use up by giving it to the user.  */
	    lock_port = convert_lock_set_to_port(alloc_lock);

	    /* Put the port in the users space.  */
	    data[0] =
		ipc_port_copyout_send(lock_port, current_space());

	    /* Set the count.  */
	    *count = 1;

	    /* Return.  */
	    return D_SUCCESS;
	}
#endif

      default:
	return D_INVALID_OPERATION;
    }
}

io_return_t
usermsg_setstat(dev_t dev,
		dev_flavor_t flavor,
		dev_status_t data,
		mach_msg_type_number_t count)
{
    usermsg_devop_t request = flavor >> 16;
    int param = flavor & 0xffff;
    flipc_return_t fr;
    
    switch (request) {
      case usermsg_Init_Buffer:	/* Initialize the buffer.  */
	/* Do we have enough data?  */
	if (count < 5)
	    return D_INVALID_OPERATION;

	fr = flipckfr_cb_init(data[0], data[1], data[2], data[3], data[4]);

	if (fr == flipc_Buffer_Initialized)
	    return D_ALREADY_OPEN;

	if (fr == flipc_No_Space)
	    return D_INVALID_SIZE;

	if (fr == flipc_Bad_Arguments)
	    return D_INVALID_OPERATION;

	if (fr != flipc_Success)
	    return D_DEVICE_DOWN;

	return D_SUCCESS;

      case usermsg_Process_Work:	/* Look for work (Get a job!).  */
#if	!PARAGON860 || FLIPC_KKT
	flipcme_duty_loop(0);
#endif	/* !PARAGON860 || FLIPC_KKT */
	return D_SUCCESS;

      case usermsg_Acquire_Allocations_Lock:
      case usermsg_Release_Allocations_Lock:
	return D_INVALID_OPERATION;

      case usermsg_Epgroup_Associate_Semaphore: /* Put a semaphore on an
						   endpoint group.  */
	/* Args are: epgroup_index, port.  */
	{
	    ipc_port_t semaphore;
	    kern_return_t kr;

	    /* Have we been given enough information?  */
	    if (count < 2)
		return D_INVALID_OPERATION;

	    if (data[1] != MACH_PORT_NULL) {
		/* First we have to figure out what the semaphore is.  */
		kr = ipc_object_copyin(current_space(),
				       (mach_port_t) data[1],
				       MACH_MSG_TYPE_COPY_SEND,
				       (ipc_object_t *) &semaphore);
		
		if (kr != KERN_SUCCESS) {
		    if (kr == KERN_INVALID_NAME)
			return D_INVALID_OPERATION;
		    else
			return kr;
		}
	    } else
		semaphore = IP_NULL;	/* It's a port, not a semaphore.  */

	    /* We should at this point have a naked send right.  Pass
	       it to the kfr.  */
	    fr = flipckfr_epgroup_set_semaphore(data[0], semaphore);

	    /* If that didn't work, I've got a send right I need to
	       release.  */
	    if (fr != flipc_Success) {
		ipc_port_release_send(semaphore);
		
		if (fr == flipc_Bad_Epgroup)
		    return D_INVALID_OPERATION;
		
		if (fr == flipc_Buffer_Not_Initialized)
		    return D_DEVICE_DOWN;
		
		return D_INVALID_OPERATION;
	    }

	    return D_SUCCESS;
	}
	
      default:
	return D_INVALID_OPERATION;
    }
}

