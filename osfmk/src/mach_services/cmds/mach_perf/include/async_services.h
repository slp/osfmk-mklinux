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
 */

/*
 * MkLinux
 */

#ifndef _ASYNC_SERVICES_H
#define _ASYNC_SERVICES_H

#define WAIT_REPLY(server_routine, reply_port, SIZE) {			\
	union {								\
		mach_msg_header_t	header;				\
		char			msg [SIZE+MAX_TRAILER_SIZE];	\
	} msg_in, msg_out;						\
									\
	MACH_CALL(mach_msg, (&(msg_in.header), MACH_RCV_MSG, 0,		\
			     sizeof msg_in, reply_port,			\
			     MACH_MSG_TIMEOUT_NONE,			\
			     MACH_PORT_NULL));				\
									\
	if (! server_routine(&(msg_in.header), &(msg_out.header)))	\
		test_error( # server_routine, "failed");		\
}

#endif	/* _ASYNC_SERVICES_H */
