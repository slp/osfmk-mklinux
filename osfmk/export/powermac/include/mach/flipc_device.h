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
 * mach/flipc_device.h
 *
 * Declarations related to the device driver interface to FLIPC.
 */

#ifndef _MACH_FLIPC_DEVICE_H_
#define _MACH_FLIPC_DEVICE_H_

/*
 * Definitions of constants both the ME and AIL need to know for
 * communications through the device driver interface.  These are the
 * possible values for the top 16 bits of the flavor parameter; the
 * bottom 16 bits are extra information that may be needed (eg. to
 * parameterize a request for semaphore in the get status routine).
 */
typedef enum {			/* Arguments.  */
    /* Get status flavors.  */	
    usermsg_Get_Initialized_Status = 1, /* (int *init_p) */
    usermsg_Get_Epgroup_Semaphore, /* (mach_port_t *semaphore) */
    usermsg_Return_Allocations_Lock, /* (void) */

    /* Set status flavors.  */
    usermsg_Init_Buffer,		/* (int max_endpoints,
					   int max_epgroups,
					   int max_buffers,
					   int max_buffers_per_endpoint,
					   int allocations_lock_policy) */
    usermsg_Process_Work,		/* (void) */
    usermsg_Acquire_Allocations_Lock, /* (void) */
    usermsg_Release_Allocations_Lock, /* (void) */
    usermsg_Epgroup_Associate_Semaphore /* (int epgroup_idx, mach_port_t port) */
} usermsg_devop_t;

#define FLIPC_DEVICE_FLAVOR(devop, param)  (((devop)<<16)|(param))

#endif /* _MACH_FLIPC_DEVICE_H_ */
