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
 * bidctl.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * bidctl.h,v
 * Revision 1.7.2.2.1.2  1994/09/07  04:18:09  menze
 * OSF modifications
 *
 * Revision 1.7.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 */

/* 
 * Public non-standard interface to bidctl
 */


#ifndef bidctl_h
#define bidctl_h
   
xkern_return_t	bidctl_init( XObj );

typedef	u_int	BootId;
typedef u_bit32_t ClusterId;

/* 
 * Protocols which previously registered are informed of reboots using
 * a BidctlRebootMsg as the buffer in a BIDCTL_PEER_REBOOTED control call.
 * These protocols will also be informed by BIDCTL when an initial
 * BootId is learned via a BIDCTL_FIRST_CONTACT control call.
 */

typedef struct {
    IPhost	h;
    BootId	id;
} BidctlBootMsg;
   
#define BIDCTL_PEER_REBOOTED		( BIDCTL_CTL * MAXOPS + 0 )
#define BIDCTL_GET_LOCAL_BID		( BIDCTL_CTL * MAXOPS + 1 )
#define BIDCTL_GET_PEER_BID		( BIDCTL_CTL * MAXOPS + 2 )
#define BIDCTL_GET_PEER_BID_BLOCKING	( BIDCTL_CTL * MAXOPS + 4 )
#define BIDCTL_FIRST_CONTACT		( BIDCTL_CTL * MAXOPS + 3 )
#define BIDCTL_GET_CLUSTERID		( BIDCTL_CTL * MAXOPS + 5 )

/* 
 * Temporary session control ops for testing
 */

#define BIDCTL_TEST_SEND_BAD_BID	( BIDCTL_CTL * MAXOPS + 10 )
#define BIDCTL_TEST_SEND_QUERY		( BIDCTL_CTL * MAXOPS + 11 )
#define BIDCTL_DUMP_STATE		( BIDCTL_CTL * MAXOPS + 12 )
#define BIDCTL_TEST_SEND_BCAST		( BIDCTL_CTL * MAXOPS + 13 )

#endif  /* ! bidctl_h */
