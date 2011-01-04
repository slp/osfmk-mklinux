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
 * libflipc/flipc_ail.h
 *
 * Header file used internally by the flipc applications interface layer.
 */

#ifndef _FLIPC_FLIPC_AIL_H_
#define _FLIPC_FLIPC_AIL_H_

#include <flipc_config.h>
#include <device/device.h>
#include <flipc.h>
#include <mach.h>
#include <mach/bootstrap.h>
#include <mach/flipc_locks.h>
#include <mach/flipc_cb.h>
#include <mach/flipc_device.h>
#include <stdio.h>
#include <stdlib.h>


/*
 * Define some extra return values for internal use.
 */
#define FLIPC_ENDPOINT_NOT_ON_EPGROUP	(FLIPC_LOWEST_RETURN-1)

/*
 * Lookup structure for domain handle-->data mapping.  Just a simple
 * linked list that will be searched through linearly for a
 * particular domain.  There are much better algorithms to implement, *if*
 * it becomes common to open multiple domains.
 */
typedef struct domain_lookup {
    mach_port_t device_port;	/* Key.  */
    struct domain_lookup *next;	/* Link to next.  */

    unsigned char *comm_buffer_base;
    unsigned long comm_buffer_length;
    FLIPC_thread_yield_function yield_fn;
#if REAL_TIME_PRIMITIVES
    lock_set_port_t allocations_lock;
#endif
} *domain_lookup_t;
#define DOMAIN_LOOKUP_NULL ((domain_lookup_t) 0)

domain_lookup_t domain_lookup_chain; /* List of open domains.  */

/* 
 * Externally visible (outside flipc_domain.c) routines which work
 * with above.
 */
domain_lookup_t domain_handle_lookup(mach_port_t domain_handle);
extern domain_lookup_t domain_pointer_lookup(void *iptr);

/*
 * Global definition of simple lock yield function, for locks.
 */
void (*flipc_simple_lock_yield_fn)();

/*
 * Prototypes for helper routines used through ail.
 */

FLIPC_return_t flipc_epgroup_remove_endpoint(flipc_epgroup_t epgroup,
					     flipc_endpoint_t endpoint);

FLIPC_return_t flipc_epgroup_add_endpoint(flipc_epgroup_t epgroup,
					  flipc_endpoint_t endpoint);

/*
 * Prototypes for message engine kick if we have a kernel message engine.
 */
#ifdef KERNEL_MESSAGE_ENGINE
/* Conditionalized on test variable to allow stacking of sends.  */
void flipc_kick_message_engine(void *ptr);

/* The real thing.  */
void flipc_really_kick_message_engine(void *ptr);
#endif

#endif /* _FLIPC_FLIPC_AIL_H_ */
