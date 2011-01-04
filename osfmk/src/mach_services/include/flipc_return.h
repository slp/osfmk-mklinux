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
 * include/flipc_return.h
 *
 * Error return type and user visible error codes.
 */

#ifndef _FLIPC_FLIPC_RETURN_H_
#define _FLIPC_FILPC_RETURN_H_

typedef int FLIPC_return_t;

#define FLIPC_SUCCESS			0

#define FLIPC_DOMAIN_NOT_AVAILABLE	-1
				/* Domain does not exist or is not
				 * available to this application.  */

#define FLIPC_DOMAIN_OP_IN_PROGRESS	-2
				/* A domain operation (init, query,
				 * teardown) was called while another
				 * one was already in progress.  */

#define FLIPC_DOMAIN_INITIALIZED	-3
				/* Domain has already been
				 * initialized.
				 */

#define FLIPC_DOMAIN_NOT_INITIALIZED	-4
				/* Domain hasn't yet been initialized.
				 */

#define FLIPC_DOMAIN_RESOURCE_SHORTAGE	-5
				/* Not enough space for all requested
				 * structures on initialization.
				 */

#define FLIPC_DOMAIN_NOT_ATTACHED	-6
				/* The flipc domain hasn't been
				 * attached to by an attach or an
				 * initialize call, or a pointer to
				 * a flipc object is not in any
				 * known domain.  */

#define FLIPC_DOMAIN_ATTACHED		-7
				/* This flipc domain has already been
				 * attached to by an attach or an
				 * initialize call.  */

#define FLIPC_DOMAIN_NO_THREAD_YIELD	-8
				/* Initialization called w/o a yield fn.  */

#define FLIPC_ALLOCATE_ENDPOINT_SHORTAGE -9
				/* There are no available endpoints.  */

#define FLIPC_ALLOCATE_EPGROUP_SHORTAGE	-10
				/* There are no available epgroups.  */

#define FLIPC_ALLOCATE_BUFFER_SHORTAGE	-11
				/* There are not enough available
				 * buffers.  */

#define FLIPC_ALLOCATE_BUFFERLIST_SHORTAGE -12
				/* The number of buffers requested by an
				 * endpoint allocation exceeds the
				 * maximum specified in the domain
				 * initialization call.  */

#define FLIPC_ALLOCATE_BUFFERDROP_CONFLICT -13
				/* Buffers were requested on
				 * allocation of a send endpoint
				 * that has drop_processed_buffers
				 * set.  */

#define FLIPC_ENDPOINT_INACTIVE		-14
				/* An operation was attempted on an
				 * endpoint which was not active.  */

#define FLIPC_EPGROUP_INACTIVE		-15
				/* An operation was attempted on an
				 * epgroup which was not active.  */

#define FLIPC_ENDPOINT_FULL		-16
				/* An attempt was made to release
				 * a buffer onto a full endpoint.  */

#define FLIPC_ENDPOINT_EMPTY		-17
				/* An attempt was made to acquire a
				 * buffer from an empty endpoint.  */

#define FLIPC_KERNEL_TRANSLATION_ERR	-18
				/* The kernel could not translate a
				 * port reference.  This return code
				 * is used as a catch-all for
				 * unexpected kernel errors.  */

#define FLIPC_EPGROUP_ENDPOINTS_MISSING	-19
				/* A call to get the endpoints
				 * associated with an endpoint group
				 * did not provide enough space for
				 * all of the endpoints.  */

#define FLIPC_DESTINATION_INVALID	-20
				/* An attempt was made to set an
				 * invalid destination into a flipc
				 * buffer.  Note that the existence of
				 * this return code should not be
				 * taken to mean that all invalid
				 * destinations will be caught by
				 * flipc; it is implementation
				 * dependent which invalid
				 * destinations may be detected.  */

#define FLIPC_INVALID_ARGUMENTS		-21
				/* Invalid arguments of some type
				 * were passed to the flipc routine.  */

#define FLIPC_BUFFER_UNOWNED		-22
				/* The buffer was associated with an
				 * endpoint or had not been allocated when
				 * the operation was attempted.  */

#define FLIPC_KERNEL_INCOMPAT		-23
				/* The FLIPC library with which the
				 * application is linked and the
				 * kernel on this machine have
				 * incompatible configurations.  */

#define FLIPC_ENDPOINT_ON_EPGROUP	-24
				/* The endpoint is still on an endpoint
				 * group.  */

#define FLIPC_EPGROUP_NO_SEMAPHORE	-25
				/* There is no semaphore associated with
				 * this endpoint group.  */

#define FLIPC_ENDPOINT_DPB		-26
				/* An attempt was made to acquire a
				   buffer from an endpoint that has
				   drop processed buffers enabled.  */

#define FLIPC_LOWEST_RETURN		-26

#endif /* _FLIPC_FILPC_RETURN_H_ */
