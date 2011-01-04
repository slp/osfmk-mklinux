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
 *	File:	dipc/dipc_error.h
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Definitions of error values passed within DIPC
 *	and back to callers of DIPC.  The dipc_return_t
 *	type is actually defined in dipc_types.h.
 */

#ifndef	_DIPC_DIPC_ERROR_H_
#define	_DIPC_DIPC_ERROR_H_

#include <mach/error.h>


#define	DIPCSUB			7
#define	DIPCERR(errno)		(err_dipc | err_sub(DIPCSUB) | errno)

#define	DIPC_SUCCESS		ERR_SUCCESS /* operation successful */
#define	DIPC_DUPLICATE		DIPCERR(1)  /* Item already in use */
#define	DIPC_NO_UID		DIPCERR(2)  /* UID sought can't be found */
#define	DIPC_NO_MEMORY		DIPCERR(3)  /* ran out of memory */
#define	DIPC_RESOURCE_SHORTAGE	DIPCERR(4)  /* resource shortage on rcv node */
#define	DIPC_PORT_MIGRATED	DIPCERR(5)  /* rcv right no longer on node */
#define	DIPC_PORT_DEAD		DIPCERR(6)  /* port dead */
#define	DIPC_QUEUE_FULL		DIPCERR(7)  /* msg queue full on rcv node */
#define	DIPC_RETRY_SEND		DIPCERR(8)  /* retry send */
#define	DIPC_WAKEUP_LOCAL	DIPCERR(9)  /* wake up local blocked sender */
#define	DIPC_DATA_SENT		DIPCERR(10) /* SEND_CONNECT data accepted */
#define	DIPC_DATA_DROPPED	DIPCERR(11) /* SEND_CONNECT data rejected */
#define	DIPC_KERNEL_PORT	DIPCERR(12) /* dest port is kernel port */
#define	DIPC_NO_MSGS		DIPCERR(13) /* no msgs on unblocking port */

#define	DIPC_PORT_PRINCIPAL	DIPCERR(14) /* port is principal */
#define	DIPC_PORT_PROXY		DIPCERR(15) /* port is proxy */
#define	DIPC_PORT_PROXY_FORWARD	DIPCERR(16) /* port is forwarding proxy */

#define DIPC_INVALID_TEST	DIPCERR(17) /* Invalid multinode test */

#define	DIPC_IN_USE		DIPCERR(18) /* Port surprisingly alive */
#define	DIPC_TRANSPORT_ERROR	DIPCERR(19) /* KKT error */
#define	DIPC_MSG_DESTROYED	DIPCERR(20) /* message destroyed by IPC/DIPC */

#endif	/* _DIPC_DIPC_ERROR_H_ */
