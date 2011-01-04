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

#ifndef eth_support_h
#define eth_support_h

#include <xkern/include/prot/eth_host.h>
#include <kern/queue.h>
#include <kern/lock.h>
#include <xkern/include/msg.h>
#include <xkern/include/xk_thread.h>
#include <device/net_status.h>
#include	<device/device_types.h>
#include	<device/io_req.h>

typedef struct {
    queue_chain_t       link;
    char                hdr[NET_HDW_HDR_MAX];
    Msg_s               upmsg;
    char                *data;  	
    XObj		driverProtl;	/* The protocol currently using this buffer */
    unsigned short	len;
} InputBuffer;

#define IN_BUF_NULL	((InputBuffer *) 0)

typedef struct {
    queue_head_t	xkBufferPool;
    queue_head_t	xkIncomingData;
    u_int               threads_out;    /* x-kernel threads out of x-kernel */
    u_int               threads_max;
    char                *name;
    Path           	path;
    u_int               allocSize;
    XkThreadPolicy_s	policy;
    decl_simple_lock_data(,iplock)
} InputPool;

#define IN_POOL_NULL	((InputPool *) 0)

xkern_return_t allocateResources(XObj self);

extern InputPool       *defaultPool;
extern Map             poolMap;

extern queue_head_t ior_xkernel_qhead;
extern int     xk_ior_dropped;
extern int     xk_ior_queued;

#define MAX_IO_XK_BUFFERS 64
#define IO_IS_XK_MSG    (IO_SPARE_START)
#define IO_IS_XK_DIRTY  (IO_SPARE_START*2)

/*
 * An input thread can process MAX_SERVICES 
 * incoming PDUs before giving up the processor 
 * sua sponte.
 */
#define MAX_SERVICES 10 

#endif /* eth_support_h */

