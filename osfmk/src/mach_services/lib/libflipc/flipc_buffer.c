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
 * libflipc/flipc_buffer.c
 *
 * This file is for buffer related functions.  Remember that for these
 * functions, the user passes us a pointer to her data area, and we need
 * to offset from it.
 */

#include "flipc_ail.h"

/* Several routines that would be in this file are defined as macros,
   cryptically, in flipc_macros.h.  */

/*
 * Get the destination of a buffer.
 */
FLIPC_address_t FLIPC_buffer_destination(FLIPC_buffer_t buffer)
{
    flipc_data_buffer_t db =
	(flipc_data_buffer_t) ((char *) buffer - sizeof(struct flipc_data_buffer));

    if (db->shrd_state == flipc_Free)
	return FLIPC_ADDRESS_ERROR;
    else
	return db->u.destination;
}

/*
 * Set a buffers destination to null.
 */
FLIPC_return_t FLIPC_buffer_set_destination_null(FLIPC_buffer_t buffer_arg)
{
    flipc_data_buffer_t db = FLIPC_INT_BUFFER(buffer_arg);
    domain_lookup_t dl = domain_pointer_lookup(db);
    flipc_comm_buffer_ctl_t cb_ctl =
	(flipc_comm_buffer_ctl_t) dl->comm_buffer_base;

#ifndef NO_ERROR_CHECKING
    if (db->shrd_state != flipc_Ready)
	return FLIPC_BUFFER_UNOWNED;
#endif

    db->u.destination = cb_ctl->null_destination;
    return FLIPC_SUCCESS;	
}



