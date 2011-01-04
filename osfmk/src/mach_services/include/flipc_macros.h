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
 * include/flipc_macros.h
 *
 * Definitions for those routines which we choose to define as macros.
 * This file must be included after the user interface file.
 */

#ifndef _FLIPC_FLIPC_MACROS_H_
#define _FLIPC_FLIPC_MACROS_H_
/*
 * We unfortunately need the flipc internal include files.
 */
#include <mach/flipc_cb.h>
#include <flipc_config.h>
#include <mach/flipc_locks.h>

/*
 * Helper macros, to make the following easier to write.
 */
#define FLIPC_INT_BUFFER(user_buffer)				\
  ((flipc_data_buffer_t) (((char*) (user_buffer))		\
			  -sizeof(struct flipc_data_buffer)))

#define FLIPC_BUFFER_STATE(user_buffer)		\
  (FLIPC_INT_BUFFER(user_buffer)->shrd_state)

/*
 * Can be used in assignment.
 */
#define FLIPC_BUFFER_DEST(user_buffer)			\
  (FLIPC_INT_BUFFER(user_buffer)->u.destination)

/*
 * Note that this reads from a volatile memory location, and hence
 * shouldn't be used unless it's the only read from that memory
 * location in the real macro ("shouldn't" == "poorer performance").
 */
#define FLIPC_ENDPOINT_ENABLED_P(endpoint) 			\
  EXTRACT_ENABLED(((flipc_endpoint_t)(endpoint))->sailda_dpb_or_enabled)

/*
 * Note that the following should be used with care, as they intentionally
 * ignores the fact that it's reading from volatile pointers (and so
 * may allow the compiler to combine references that really shouldn't
 * be combined).  If you might be screwed by rearrangement of accesses
 * to the volatile bufferlist pointers in the endpoint structure, don't
 * use these macros.
 */

#define FLIPC_ACQUIRE_NONV(endpoint) \
     (*((flipc_cb_ptr *)&(((flipc_endpoint_t)(endpoint))->shrd_acquire_ptr)))
#define FLIPC_PROCESS_NONV(endpoint) \
     (*((flipc_cb_ptr *)&(((flipc_endpoint_t)(endpoint))->sme_process_ptr)))
#define FLIPC_RELEASE_NONV(endpoint) \
     (*((flipc_cb_ptr *)&(((flipc_endpoint_t)(endpoint))->sail_release_ptr)))

#define FLIPC_LOCK_ENDPOINT(endpoint)					\
  (flipc_simple_lock_acquire(&((flipc_endpoint_t)(endpoint))->pail_lock))

/*
 * End helper macros.
 */


/*
 * Has the buffer been processed?
 */
#define FLIPC_buffer_processed(buffer)			\
  FLIPC_BUFFER_PROCESSED_P(FLIPC_BUFFER_STATE(buffer))

/*
 * Set the destination of a buffer, and return whether it has an
 * available buffer or not.
 */
#define FLIPC_endpoint_buffer_available(endpoint)		\
  (!DISABLED_OR_DPB_P(((flipc_endpoint_t)(endpoint))->sailda_dpb_or_enabled) \
   && ((FLIPC_ACQUIRE_NONV(endpoint) !=				\
	((flipc_endpoint_t)(endpoint))->pail_process_ptr)	\
       || (FLIPC_ACQUIRE_NONV(endpoint) !=			\
	   (((flipc_endpoint_t)(endpoint))->pail_process_ptr =	\
	    FLIPC_PROCESS_NONV(endpoint)))))

#ifndef NO_ERROR_CHECKING
#define FLIPC_buffer_set_destination(buffer, dest)		\
  (FLIPC_BUFFER_STATE(buffer) != flipc_Ready			\
   ? FLIPC_BUFFER_UNOWNED					\
   : (FLIPC_BUFFER_DEST(buffer) = dest), FLIPC_SUCCESS)

#else 	/* NO_ERROR_CHECKING */

#define FLIPC_buffer_set_destination(buffer, dest)		\
  ((FLIPC_BUFFER_DEST(buffer) = dest), FLIPC_SUCCESS)

#endif	/* NO_ERROR_CHECKING */

#endif /* _FLIPC_FLIPC_MACROS_H_ */
