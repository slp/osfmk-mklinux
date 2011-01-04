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
/* 
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */

#ifndef	_KGDB_GDB_PACKET_H_
#define	_KGDB_GDB_PACKET_H_

#define CURRENT_KGDB_PROTOCOL_VERSSION 4

/*
 * This file defines the layout of requests and responses for the remote
 * microkernel and task debuggers.  In both cases, requests come only from
 * the remote debugger.  Responses are sent back only when a request is
 * received.
 */

struct gdb_request {
	unsigned int	xid;
	unsigned int	type;
	unsigned int	task_id;
	unsigned int	thread_id;
	unsigned int	request;
	unsigned int	address;
	unsigned int	size;
	unsigned char	data[1024];
};

typedef struct gdb_request *gdb_request_t;

struct gdb_response {
	unsigned int	xid;
	unsigned int	type;
	unsigned int	task_id;
	unsigned int	thread_id;
	unsigned int	return_code;
	unsigned int	pad;
	unsigned int	size;
	unsigned char	data[1024];
};

typedef struct gdb_response *gdb_response_t;

#include <machine_tgdb.h>

#endif	/* _KGDB_GDB_PACKET_H_ */
