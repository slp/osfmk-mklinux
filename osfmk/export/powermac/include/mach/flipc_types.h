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
 * mach/flipc_types.h
 *
 * Definitions of those flipc types that need to be visible to both the AIL
 * and kernel sides of flipc (which is just about everything).
 */

#ifndef _MACH_FLIPC_TYPES_H_
#define _MACH_FLIPC_TYPES_H_

#include <mach/port.h>

/*
 * Define a couple of generally useful types.
 */
#include <mach/mach_types.h>

#ifndef MACH_KERNEL
#define SEMAPHORE_NULL (semaphore_port_t)0
#endif /* !defined(MACH_KERNEL) */

/*
 * Basic flipc types; visible to both user and kernel segments of the
 * flipc implementation.
 */
/* Flipc addresses.  These name a node-endpoint combination for
   sending.  */
typedef unsigned int FLIPC_address_t;
#define FLIPC_ADDRESS_ERROR ((FLIPC_address_t) -1)

/* Flipc endpoints.  */
typedef void *FLIPC_endpoint_t;
#define FLIPC_ENDPOINT_NULL ((FLIPC_endpoint_t) 0)

/* Buffer pointers (returned by query functions).  Users are allowed to
   copy directly to/from this pointer; it points at their data.  */
typedef void *FLIPC_buffer_t;   
#define FLIPC_BUFFER_NULL ((FLIPC_buffer_t) 0)   

/* Endpoint group identifiers.  */
typedef void *FLIPC_epgroup_t;
#define FLIPC_EPGROUP_NULL ((FLIPC_epgroup_t) 0)
#define FLIPC_EPGROUP_ERROR ((FLIPC_epgroup_t) -1)

/* Domain index; argument to initialization and attach routines.  */
typedef unsigned int FLIPC_domain_index_t;

/* Domain handle (mach port).  */
typedef mach_port_t FLIPC_domain_t;

/* The different types an endpoint can be.  FLIPC_Inactive is used when
   the endpoint has not been configured and hence is on the freelist.  */
typedef enum {
    FLIPC_Inactive = -1,
    FLIPC_Send,
    FLIPC_Receive
} FLIPC_endpoint_type_t;

/* Structure for returning performance information about the flipc
   domain; a placeholder for future entries as needed.
   This information will only be valid if the kernel is configured to
   keep flipc performance information.  */
typedef struct FLIPC_domain_performance_info {
    unsigned long performance_valid;	/* Non zero if the other information
				   in this structure is valid.  */
    unsigned long messages_sent;		/* Since last init.  */
    unsigned long messages_received;	/* Since last init.  Includes overruns
				   (because they are marked in the
				   endpoint data structure).  Doesn't
				   include other drops (they are
				   marked in other places) */
} *FLIPC_domain_performance_info_t;

/* Flipc yield function.  */
typedef void (*FLIPC_thread_yield_function)(void);

/* Structure for returning information about the flipc domain.  */
typedef struct FLIPC_domain_info {
    int max_endpoints;
    int max_epgroups;
    int max_buffers;
    int max_buffers_per_endpoint;
    int msg_buffer_size;
    FLIPC_thread_yield_function yield_fn;
    int policy;			/* Allocations lock sched policy.
				   Unused if REAL_TIME_PRIMITIVES are
				   not being used.  */
    struct FLIPC_domain_performance_info performance;
    int error_log_size;		/* In bytes.  */
} *FLIPC_domain_info_t;

/* Structure for returning information about the error state of
   the flipc domain.  Note that this is variable sized; the size
   of the transport specific information is not known at compile
   time.  */
typedef struct FLIPC_domain_errors {
    int error_full_config_p;		/* 1 if disabled and badtype below are
					   valid; 0 if only msgdrop_inactive
					   is valid.  */ 
    int msgdrop_inactive;		/* Messages dropped because
					   of the domain being locally
					   inactive.  */
    int msgdrop_disabled;		/* Messages dropped because of a
					   disabled endpoint.  */
    int msgdrop_badtype;		/* Messages dropped because they
					   were sent to a send endpoint.  */

    int transport_error_size;	/* Size of the following array of
				   ints, in bytes.  */
    int transport_error_info[1];	/* Really of transport_error_size.  */
} *FLIPC_domain_errors_t;

/* Structure for returning information about endpoints.  */
typedef struct FLIPC_endpoint_info {
    FLIPC_endpoint_type_t type;
    unsigned int processed_buffers_dropped_p;
    unsigned long number_buffers;
    FLIPC_epgroup_t epgroup;
} *FLIPC_endpoint_info_t;

typedef struct FLIPC_epgroup_info {
    unsigned long msgs_per_wakeup;
} *FLIPC_epgroup_info_t;

#endif /* _MACH_FLIPC_TYPES_H_ */
