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
 * Revision 1.3.2.2  1993/06/09  03:05:37  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  20:47:38  gm]
 *
 * Revision 1.3  1993/04/19  17:39:19  devrcs
 * 	Added trailer support
 * 	[1993/04/07  13:13:10  travos]
 * 
 * 	Merge untyped ipc:
 * 	MACH_IPC_TYPED support.
 * 	[1993/02/22  00:45:45  rod]
 * 
 * Revision 1.2  1992/05/12  14:42:45  devrcs
 * 	Created for OSF/1 MK
 * 	[1992/05/04  06:59:31  condict]
 * 
 * Revision 2.6  92/01/27  16:43:28  rpd
 * 	Added MACH_SEND_TIMED_OUT case.
 * 	[92/01/26            rpd]
 * 
 * Revision 2.5  92/01/23  15:22:28  rpd
 * 	Fixed to handle MACH_RCV_TOO_LARGE from receives.
 * 	Fixed to supply MACH_SEND_TIMEOUT when necessary.
 * 	[92/01/20            rpd]
 * 
 * Revision 2.4  91/05/14  17:53:22  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/14  14:17:47  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:44:20  mrt]
 * 
 * Revision 2.2  90/08/06  17:23:58  rpd
 * 	Created.
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

extern int	traceproxy;
#define INTER_ALLOC_DELAY	( 5 * 1000 ) 

#ifdef XK_PROXY_MSG_HACK

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <xkern/include/xkernel.h>
#include <xkern/protocols/proxy/proxy_util.h>


/*
 *	Routine:	mach_msg_server
 *	Purpose:
 *		A simple generic server function.
 *
 * Allocation portions hacked for the x-kernel
 */
#if	MACH_IPC_TYPED
mach_msg_return_t
xk_mach_msg_server(demux, max_size, rcv_name)
    boolean_t (*demux)();
    mach_msg_size_t max_size;
    mach_port_t rcv_name;
{
    register mig_reply_header_t *bufRequest, *bufReply, *bufTemp;
    register mach_msg_return_t mr;
    ProxyThreadInfo	*info;
    Allocator		alloc;

    info = (ProxyThreadInfo *)cthread_data(cthread_self());
    xAssert(info);
    alloc = pathGetAlloc(info->path, MSG_ALLOC);
    xk_master_lock();
    bufRequest = (mig_reply_header_t *) allocGet(alloc, max_size);
    bufReply = (mig_reply_header_t *) allocGet(alloc, max_size);
    xk_master_unlock();
    if (! bufRequest || ! bufReply) {
	return KERN_RESOURCE_SHORTAGE;
    }

    for (;;) {
      get_request:
	mr = mach_msg(&bufRequest->Head, MACH_RCV_MSG,
		      0, max_size, rcv_name,
		      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	while (mr == MACH_MSG_SUCCESS) {
	    /* we have a request message */

	    (void) (*demux)(&bufRequest->Head, &bufReply->Head);


	    if (bufReply->RetCode != KERN_SUCCESS) {
		if (bufReply->RetCode == MIG_NO_REPLY)
		    goto get_request;

#if 0
		/* 
		 * x-kernel hack ... the demux routine is responsible for
		 * deallocating the request buffer.  It may, in fact,
		 * already be deallocated.  We must be careful not to
		 * reference through it.
		 */

		/* don't destroy the reply port right,
		   so we can send an error message */
		bufRequest->Head.msgh_remote_port = MACH_PORT_NULL;
		mach_msg_destroy(&bufRequest->Head);
#endif
	    }

	    if (bufReply->Head.msgh_remote_port == MACH_PORT_NULL) {
		/* no reply port, so destroy the reply */
		if (bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)
		    mach_msg_destroy(&bufReply->Head);

		goto get_request;
	    }

	    /* send reply and get next request */

	    bufRequest = bufReply;
	    while ( ! (bufReply = allocGet(alloc, max_size)) ) {
		xTrace0(proxy, TR_SOFT_ERRORS,
			"proxy input thread can't alloc buffer");
		Delay(INTER_ALLOC_DELAY);
	    }

	    /*
	     *	We don't want to block indefinitely because the client
	     *	isn't receiving messages from the reply port.
	     *	If we have a send-once right for the reply port, then
	     *	this isn't a concern because the send won't block.
	     *	If we have a send right, we need to use MACH_SEND_TIMEOUT.
	     *	To avoid falling off the kernel's fast RPC path unnecessarily,
	     *	we only supply MACH_SEND_TIMEOUT when absolutely necessary.
	     */

	    mr = mach_msg(&bufRequest->Head,
			  (MACH_MSGH_BITS_REMOTE(bufRequest->Head.msgh_bits) ==
						MACH_MSG_TYPE_MOVE_SEND_ONCE) ?
			  MACH_SEND_MSG|MACH_RCV_MSG :
			  MACH_SEND_MSG|MACH_SEND_TIMEOUT|MACH_RCV_MSG,
			  bufRequest->Head.msgh_size, max_size, rcv_name,
			  0, MACH_PORT_NULL);
	}

	/* a message error occurred */

	switch (mr) {
	  case MACH_SEND_INVALID_DEST:
	  case MACH_SEND_TIMED_OUT:
	    /* the reply can't be delivered, so destroy it */
	    mach_msg_destroy(&bufRequest->Head);
	    break;

	  case MACH_RCV_TOO_LARGE:
	    /* the kernel destroyed the request */
	    break;

	  default:
	    /* should only happen if the server is buggy */
	    xk_master_lock();
	    allocFree((char *) bufRequest);
	    allocFree((char *) bufReply);
	    xk_master_unlock();
	    return mr;
	}
    }
}
#else	/* !MACH_IPC_TYPED */
mach_msg_return_t
xk_mach_msg_server(demux, max_size, rcv_name, options)
    boolean_t (*demux)();
    mach_msg_size_t max_size;
    mach_port_t rcv_name;
    mach_msg_options_t	options;
{
    register mig_reply_error_t *bufRequest, *bufReply;
    register mach_msg_return_t mr;
    ProxyThreadInfo	*info;
    Allocator		alloc;

    info = (ProxyThreadInfo *)cthread_data(cthread_self());
    xAssert(info);
    alloc = pathGetAlloc(info->path, MSG_ALLOC);
    xk_master_lock();
    bufRequest = allocGet(alloc, max_size + MAX_TRAILER_SIZE);
    bufReply = allocGet(alloc, max_size + MAX_TRAILER_SIZE);
    xk_master_unlock();
    if (! bufRequest || ! bufReply) {
	return KERN_RESOURCE_SHORTAGE;
    }

    for (;;) {
      get_request:
	mr = mach_msg_overwrite(&bufRequest->Head, MACH_RCV_MSG|options,
		      0, max_size, rcv_name,
		      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		      (mach_msg_header_t *) 0, 0);
	while (mr == MACH_MSG_SUCCESS) {
	    /* we have a request message */

	    (void) (*demux)(&bufRequest->Head, &bufReply->Head);

#if 0
	    /* 
	     * x-kernel hack ... the demux routine is responsible for
	     * deallocating the request buffer.  It may, in fact,
	     * already be deallocated.  We must be careful not to
	     * reference through it.
	     */
	    if (!(bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) &&
		bufReply->RetCode != KERN_SUCCESS) {
		    if (bufReply->RetCode == MIG_NO_REPLY)
			goto get_request;

		    /* don't destroy the reply port right,
			so we can send an error message */
		    bufRequest->Head.msgh_remote_port = MACH_PORT_NULL;
		    mach_msg_destroy(&bufRequest->Head);
	    }
#endif

	    if (bufReply->Head.msgh_remote_port == MACH_PORT_NULL) {
		/* no reply port, so destroy the reply */
		if (bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)
		    mach_msg_destroy(&bufReply->Head);

		goto get_request;
	    }

	    /* send reply and get next request */

	    bufRequest = bufReply;
	    xk_master_lock();
	    while ( ! (bufReply = allocGet(alloc, max_size)) ) {
		xTrace1(proxy, TR_SOFT_ERRORS,
			"proxy input thread %d can't alloc buffer",
			cthread_self());
		Delay(INTER_ALLOC_DELAY);
	    }
	    xk_master_unlock();

	    /*
	     *	We don't want to block indefinitely because the client
	     *	isn't receiving messages from the reply port.
	     *	If we have a send-once right for the reply port, then
	     *	this isn't a concern because the send won't block.
	     *	If we have a send right, we need to use MACH_SEND_TIMEOUT.
	     *	To avoid falling off the kernel's fast RPC path unnecessarily,
	     *	we only supply MACH_SEND_TIMEOUT when absolutely necessary.
	     */

	    mr = mach_msg_overwrite(&bufRequest->Head,
			  (MACH_MSGH_BITS_REMOTE(bufRequest->Head.msgh_bits) ==
						MACH_MSG_TYPE_MOVE_SEND_ONCE) ?
			  MACH_SEND_MSG|MACH_RCV_MSG|options :
			  MACH_SEND_MSG|MACH_SEND_TIMEOUT|MACH_RCV_MSG|options,
			  bufRequest->Head.msgh_size, max_size, rcv_name,
			  0, MACH_PORT_NULL, (mach_msg_header_t *) 0, 0);
	}

	/* a message error occurred */

	switch (mr) {
	  case MACH_SEND_INVALID_DEST:
	  case MACH_SEND_TIMED_OUT:
	    /* the reply can't be delivered, so destroy it */
	    mach_msg_destroy(&bufRequest->Head);
	    break;

	  case MACH_RCV_TOO_LARGE:
	    /* the kernel destroyed the request */
	    break;

	  default:
	    /* should only happen if the server is buggy */
	    xk_master_lock();
	    allocFree((char *) bufRequest);
	    allocFree((char *) bufReply);
	    xk_master_unlock();
	    return mr;
	}
    }
}
#endif	/* MACH_IPC_TYPED */

#endif /* XK_PROXY_MSG_HACK */
