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
 * include/flipc_ui.h
 *
 * This file contains all of the user-visible components of the flipc
 * interface (function interfaces & user-visible types).
 */

#ifndef _FLIPC_FLIPC_UI_H_
#define _FLIPC_FLIPC_UI_H_

#include <mach/flipc_types.h>
#include <flipc_return.h>

/* 
 * User callable routines
 */


/* --- Domain setup/teardown --- */

/* Domain initialize.  Initializes a flipc domain on a node for
 * communication and attach to it.  This function requires kernel
 * level synchronization, and thus should not be considered fast.
 * Fails if:
 *	domain already initialized (by some other process)
 *	domain already attached (by us)
 *	domain not available to this application
 *	Total size of requested structures too large for domain
 *	A null yield function was provided.
 * 	Invalid (eg. negative) arguments were provided.
 *	One of the domain initialization operations was in progress at
 *		the time of the call.
 *	The kernel had an unexpected port translation problem.
 *
 * Args: Domain index
 *	 domain_info struct (domain_params) zeroed except for:
 *	 	max number of endpoints,
 *       	max number of endpoint groups
 *       	max number of buffers
 *       	max number of buffers per endpoint
 *	 	Allocations lock scheduling policy 
 *       	Entry point for yielding control of CPU if the AIL encounters
 *              	significant spinning on a simple lock.]
 *       Pointer to domain handle arg; written if the call suceeds.
 *
 * Returns: Error code upon failure; 0 upon sucess. */

FLIPC_return_t FLIPC_domain_init(FLIPC_domain_index_t domain_index,
				 FLIPC_domain_info_t domain_params,
				 FLIPC_domain_t *domain_handle);

/* Domain attach.  Attach to a previously locally initialized domain
 * (presumably by some other process).  This function requires kernel
 * level synchronization, and thus should not be considered fast.
 * Fails if:
 *	domain already attached (by us)
 *	domain not available to this application
 *	A null yield function was provided.
 *	One of the domain initialization operations was in progress at
 *		the time of the call.
 *	The kernel had an unexpected port translation problem.
 * 	Domain not previously initialized
 * 
 * Args: Domain index.
 *	 domain_info struct (domain_params) zeroed except for:
 *	 	Entry point for yielding control of CPU if the AIL encounters
 *              	significant spinning on a simple lock.
 *       Pointer to domain handle arg; written if the call suceeds.
 * 
 * Returns: Error code upon failure; 0 upon success. */

FLIPC_return_t FLIPC_domain_attach(FLIPC_domain_index_t domain_index,
				   FLIPC_domain_info_t domain_params,
				   FLIPC_domain_t *domain_handle);
   
/* Domain detach.  Releases attach to current domain, and if last task
 * to release domain on this node, causes cleanup.  This function
 * requires kernel level synchronization, and thus should not be
 * considered fast.  Fails if:
 *	Argument isn't a valid domain handle.
 *	An invalid yield function has been specified.
 *	One of the domain initialization operations was in progress at
 *		the time of the call.
 * 
 * Args: Domain handle.
 * 
 * Returns: Error code upon failure; 0 upon sucess. */

FLIPC_return_t FLIPC_domain_detach(FLIPC_domain_t domain_handle);   

/* Domain query.  Provides a list of the attributes set for this
 * domain.  Fails if:
 *	Argument isn't a valid domain handle.
 *	One of the domain initialization operations was in progress at
 *		the time of the call.
 * 
 * Args: Domain handle.
 *       Pointers to output locations for a flipc domain info struct
 *		(see flipc_types.h).
 *
 * Returns: Error code if invalid argument; 0 if success. */

FLIPC_return_t FLIPC_domain_query(FLIPC_domain_t domain_handle,
				  FLIPC_domain_info_t info);

/* Domain error query.  Provides an error log for errors that have
 * been detected on this node by this domain.  This log is implementation
 * dependent; see flipc_types.h for some details.
 *
 * This routine must be passed a pointer to a structure with enough
 * space to hold all of the returned information; the amount of space
 * necessary to hold the information may be found in the return values
 * from the domain query function above.
 *
 * Fails if:
 *	Argument isn't a valid domain handle.
 *	One of the domain initialization operations was in progress at
 *		the time of the call.
 *
 * Returns: Error code if invalid argument; 0 if success.  */

FLIPC_return_t FLIPC_domain_error_query(FLIPC_domain_t domain_handle,
					FLIPC_domain_errors_t error_log);

/* --- Memory allocation/deallocation --- */

/* Allocate endpoint.  Allocates an endpoint for communication.  This
 * function requires kernel level synchronization, and thus should not
 * be considered fast.  Endpoints must be removed from any
 * endpoint group they are on before freeing.  Fails if:
 *	There are no endpoints
 * 	There are insufficient buffers available
 *      Request to allocate non-zero buffers on a send endpoint with 
 *		drop buffers after processing requested.
 *	More buffers requested than the max specified for the domain
 *      	per endpoint
 *	The domain handle isn't valid.
 *	The endpoint is on an endpoint group.
 * 
 * Args: Domain handle.
 *       Message buffers to allocate with this endpoint.
 *       Endpoint send or receive?
 *       Leave messages on endpoint after processing?
 *       Pointer to endpoint pointer; filled in if call suceeds
 * 
 * Returns: Negative error code if failure, 0 on success.  */

FLIPC_return_t FLIPC_endpoint_allocate(FLIPC_domain_t domain_handle,
				       int number_of_buffers,
				       FLIPC_endpoint_type_t type,
				       unsigned long drop_processed_message_p,
				       FLIPC_endpoint_t *endpoint);

/* Allocate endpoint group.  Allocates an endpoint group for grouping
 * of endpoints, and attaches a provided semaphore to it if that is
 * desired.  Note that semaphores must be attached to the epgroup at
 * creation time, if you want a semaphore associated with this endpoint
 * group. This function requires kernel level synchronization,
 * and thus should not be considered fast.  Fails if:
 *	There are no endpoint groups available
 *	The domain handle isn't valid.
 * 
 * Args: Domain handle
 *	 Semaphore (may be SEMAPHORE_NULL if no semaphore is to
 *	 	be associated with the endpoint group).
 *       Pointer to endpoint group pointer; filled in if call suceeds.
 * 
 * Returns Negative error code if failure, 0 for success.  */

FLIPC_return_t FLIPC_epgroup_allocate(FLIPC_domain_t domain_handle,
				      semaphore_port_t semaphore,
				      FLIPC_epgroup_t *epgroup);

/* Deallocate endpoint.  Releases an endpoint back to the system,
 * along with all buffer resources currently associated with this
 * endpoint.  Note that an endpoint should only be deallocated by the
 * same priority class of thread that is using it, otherwise deadlock
 * may result as deallocation requires locking the endpoint.  Fails
 * if:
 *	Invalid arguments (endpoint group not in valid domain)
 *      Endpoint inactive
 * 
 * Args: Endpoint pointer.
 * 
 * Returns a negative error code upon failure, 0 upon success.
 */
   
FLIPC_return_t FLIPC_endpoint_deallocate(FLIPC_endpoint_t endpoint);

/* Deallocate endpoint group.  This call deallocates an endpoint
 * group, and all endpoints associated with it.  This function
 * requires kernel level synchronization, and thus should not be
 * considered fast.  Fails if:
 *	Invalid arguments (endpoint group not in valid domain)
 *	Endpoint group inactive.
 * 
 * Args: Endpoint group identifier.
 * 
 * Returns a negative error code upon failure, 0 upon success.
*/

FLIPC_return_t FLIPC_epgroup_deallocate(FLIPC_epgroup_t epgroup);

/* --- Endpoint group functions --- */

/* Endpoint group set messages wakeup.  Set the number of messages (n)
 * required before a wakeup will be initiated on this endpoint group (this
 * defaults to one at endpoint group allocation).  A single wakeup
 * will be issued for each (n) messages received on this endpoint
 * group.  Fails if:
 *      Endpoint group inactive.
 * 
 * Args: Endpoint group, number of messages.
 * 
 * Returns: Negative error code if failure, 0 if
 * success. 
 */

FLIPC_return_t FLIPC_epgroup_set_wakeup_count(FLIPC_epgroup_t epgroup,
					      int messages_per_wakeup);
   
/* Endpoint group get semaphore.  Returns the semaphore associated
 * with an endpoint group (null if none).  This call may require
 * kernel involvement, and hence should not be considered fast.  Fails
 * if:
 *      Invalid arguments are passed (bad epgroup pointer)
 *	The kernel had unexpected trouble translating the semaphore.
 *	The epgroup is in an unattached domain.
 * 
 * Args: Endpoint group
 *       Pointer to semaphore object; filled in if the call suceeds.
 * 
 * Returns: Negative error code upon failure; 0 upon sucess.
 */

FLIPC_return_t FLIPC_epgroup_semaphore(FLIPC_epgroup_t epgroup,
				       semaphore_port_t *semaphore);

/*
 * Endpoint group query.  This calls returns information
 * about the endpoint group in an FLIPC_epgroup_info structure.
 *
 * Args: Endpoint group, pointer to structure to be filled on return.
 *
 * Returns: FILPC_SUCCESS if successful, FLIPC_EPGROUP_INACTIVE if the
 * endpoint is inactive.
 */
FLIPC_return_t FLIPC_epgroup_query(FLIPC_epgroup_t epgroup,
				   FLIPC_epgroup_info_t epgroup_info);

/* Endpoint group get message.  If there is a message available on any
 * of the endpoints in this endpoint group, return it.  Optionally
 * request a wakeup on the endpoint group's semaphore if there is no
 * message.  The routine will return a value saying whether or not a
 * wakeup was requested if a non-null pointer is passed it.; a wakeup
 * may have been requested even if a buffer was returned (because of
 * an unavoidable race condition).  If a non-NULL buffer is returned,
 * the variable pointed to by 'endpoint' will be set to the endpoint
 * on which the buffer was found.
 *
 * Note that if a wakeup was requested and the epgroup does not have a
 * semaphore associated, an null message pointer will be returned
 * without checking for messages.
 * 
 * Args: Endpoint group identifier
 *       wakeup request flag
 *	 Pointer to endpoint variable.
 *       Pointer to variable in which the routine may not whether or
 *           not it requested a wakeup.  This variable is set if a
 *           wakeup was actually requested; the time when it conveys
 *           useful information is when a wakeup was requested *and*
 *           (because of a race) a message was retrieved.
 * 
 * Returns: Message if available, null message pointer if not.
 * If a message is returned, *endpoint is set to the endpoint on which the
 * message was found.  */

FLIPC_buffer_t FLIPC_epgroup_get_message(FLIPC_epgroup_t epgroup,
                                         int request_wakeup,
					 FLIPC_endpoint_t *endpoint,
                                         int *wakeup_requested);

/* Endpoint group message available.  Query if there is a message
 * available on an endpoint group.  Note that the result of this
 * function should be taken as a hint; since no lock is held upon
 * return from the function, the message available status may change
 * by the next time the endpoint group is accessed.
 * 
 * Args: Endpoint group identifier.
 * 
 * Returns: True (1) if there is a message available, False (0) if
 * not.
 */   

int FLIPC_epgroup_message_available(FLIPC_epgroup_t epgroup);   

/* Endpoint group endpoints.  This function fills in an array with a
 * list of all of the endpoints on an endpoint group (that will fit in
 * the array).  If the endpoints on the endpoint group will not fit
 * into the space provided, the call returns an error (but still fills in
 * the array it was passed).  The array size variable is altered to indicate
 * the number of endpoints copied, or, if there wasn't enough room
 * to copy all of the endpoints, it is altered to indicate the size
 * the array must have.  This call fails if: 
 *      Endpoint group inactive.
 *	Not enough space was provided to copy in all of the endpoints.
 * 
 * Args: Endpoint group identifier.
 *       Pointer to size of the array passed (in/out; modified to
 *      	reflect number of pointers copied or size needed).
 *       Pointer to array of pointers to endpoints; filled in on
 *              success or array-too-small failure.  This array will
 *              be null terminated. 
 *              
 * Returns: Negative return code on error, 0 on success.
 */

FLIPC_return_t FLIPC_epgroup_endpoints(FLIPC_epgroup_t epgroup,
				       int *endpoint_array_size,
				       FLIPC_endpoint_t *endpoint_array);

/* --- Endpoint functions --- */

/* Endpoint associate with endpoint group.  Adds an endpoint to an endpoint
 * group.  Specifying a null endpoint group removes the endpoint from
 * any endpoint group.  Note that this call is not atomic; an intermediate
 * state in which the endpoint belongs to no endpoint group may be observed.
 * Fails if:
 *	Endpoint is inactive
 *	Endpoint group is inactive.
 *
 * Args: Endpoint, endpoint group.
 * Returns: Negative error code if failure, 0 if
 * sucess. 
 */   

FLIPC_return_t FLIPC_endpoint_join_epgroup(FLIPC_endpoint_t endpoint,
					   FLIPC_epgroup_t epgroup);
   
/* Endpoint buffer available.  Queries if there's a message buffer
 * available for acquiring on an endpoint.  Note that the result of this
 * function should be taken as a hint; since no lock is held upon
 * return from the function, the message available status may change
 * by the next time the endpoint is accessed.
 * Also note that this functionality may be implemented through a macro,
 * so the argument should not have side effects.
 * 
 * Args: Endpoint identifier.
 * 
 * Returns: True (1) if there is a buffer available on the endpoint,
 * False (0) if there is not.
 */

int FLIPC_endpoint_buffer_available(FLIPC_endpoint_t endpoint);      

/* Endpoint query.  Gets information about an endpoint.
 *
 * Args: 	Endpoint
 *		Pointer to flipc endpoint info structure, which is filled 
 * 	in on successful return.
 *
 * Returns FLIPC_SUCCESS upon success.  FLIPC_ENDPOINT_INACTIVE if the
 * endpoint is not active.
 */
FLIPC_return_t FLIPC_endpoint_query(FLIPC_endpoint_t endpoint,
				    FLIPC_endpoint_info_t endpoint_info);

/* Endpoint buffer release.  This function releases a buffer
 * to an endpoint.  This corresponds to sending, if the endpoint is a
 * send endpoint, and to allocating a buffer for incoming messages if
 * the buffer is a receiving endpoint.  Two versions of this interface,
 * one that protects against multi-threaded accesses to the endpoint, and
 * one that does not.  Fails if:
 *	The endpoint is full
 *      The endpoint is inactive.
 * 
 * Args: Endpoint identifier
 *       Buffer
 * 
 * Returns: Negative error code upon failure, 0 upon sucess.
 */

FLIPC_return_t FLIPC_endpoint_buffer_release_unlocked(
			FLIPC_endpoint_t endpoint,
			FLIPC_buffer_t buffer);

FLIPC_return_t FLIPC_endpoint_buffer_release(
			FLIPC_endpoint_t endpoint,
			FLIPC_buffer_t buffer);

/* Endpoint buffer acquire.  This function allocates a buffer from an
 * endpoint.  This corresponds to reclaiming a sent buffer if the
 * endpoint is a send endpoint, and to receiving a message if the
 * endpoint is a receiving endpoint.  Data may be copied directly to
 * memory at the pointer that is returned.  Two versions of this
 * interface, one that protects against multi-threaded access to the
 * endpoint, and one that does not.  Fails if:
 *      The endpoint has no buffers available
 *      The endpoint is inactive.	
 * 
 * Args: Endpoint identifier
 * 
 * Returns: buffer pointer upon sucess, null buffer upon failure. */

FLIPC_buffer_t FLIPC_endpoint_buffer_acquire_unlocked(
			FLIPC_endpoint_t endpoint);

FLIPC_buffer_t FLIPC_endpoint_buffer_acquire(
			FLIPC_endpoint_t endpoint);

/* Endpoint address.  Gets a FLIPC_address_t corresponding to the
 * endpoint.  Fails if:
 * 	An invalid endpoint is passed.
 * 
 * Args: Endpoint identifier.
 *       Pointer to address; filled in upon success.
 * 
 * Returns: 0 on success, negative return code on failure.
 */

FLIPC_return_t FLIPC_endpoint_address(FLIPC_endpoint_t endpoint,
				      FLIPC_address_t *address);

/* Endpoint overrun count.  Returns the number of messages that have
 * been dropped on a receive endpoint due to no buffers in which to
 * place them.  Optionally allows this count to be zeroed.
 * 
 * If the endpoint is unallocated, -1 will be returned and the count will
 * not be changed. 
 */

int FLIPC_endpoint_overrun_count(FLIPC_endpoint_t endpoint,
				 int zero_overrun_count);

/* --- Buffer functions --- */

/* Buffer processed.  Queries whether a buffer has been sucessfully
 * processed by the message engine.
 * 
 * Args: Buffer pointer.
 * 
 * Returns: True (1) if the buffer has been processed, False (0) if
 * not.
 */

int FLIPC_buffer_processed(FLIPC_buffer_t buffer);   

/* Buffer set destination.  Set the destination field in a buffer.
 * Fails if:
 * 	The destination is invalid.
 *	The buffer is unallocated.
 *      
 * Note that only some destination checks are possible; a destination
 * may be accepted by this routine but still not be valid.
 * 
 * Args: Buffer pointer.
 *       Flipc destination.
 * 
 * Returns: Negative error code upon failure, 0 upon sucess.
 */

FLIPC_return_t FLIPC_buffer_set_destination(FLIPC_buffer_t buffer,
					    FLIPC_address_t destination);

/* Buffer set destination to a safe null destination.  Set the
 * destination field in a buffer to a null destination (the message
 * will be dropped when transmitted).
 * Fails if:
 *	The buffer is unallocated.
 *      
 * Args: Buffer pointer.
 * 
 * Returns: Negative error code upon failure, 0 upon sucess.
 */

FLIPC_return_t FLIPC_buffer_set_destination_null(FLIPC_buffer_t buffer);

/* Buffer get destination.  Get the destination field of a buffer.
 * If the buffer is unallocated, FLIPC_ADDRESS_ERROR will be
 * returned.
 * 
 * Args: Buffer pointer.
 * 
 * Returns: Flipc destination.
 */

FLIPC_address_t FLIPC_buffer_destination(FLIPC_buffer_t buffer);
    
#endif /* _FLIPC_FLIPC_UI_H_ */
