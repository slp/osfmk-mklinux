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
 * arp_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * arp_i.h,v
 * Revision 1.15.2.2.1.2  1994/09/07  04:18:25  menze
 * OSF modifications
 *
 * Revision 1.15.2.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.15.2.1  1994/04/14  00:26:14  menze
 * Uses allocator-based message interface
 *
 */

#ifndef arp_i_h
#define arp_i_h

#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/prot/arp.h>

#define	ARP_HRD	  1
#define	ARP_PROT  0x0800	/* doing IP addresses only */
#define	ARP_HLEN  28		/* the body is null */
#define ARP_TIME  2000		/* 2 seconds */
#define ARP_RTRY  2		/* retries for arp request */
#define ARP_RRTRY 5		/* retries for rarp request */
#define INIT_RARP_DELAY	 5000	/* msec to delay between failed self rarps */

#define	ARP_REQ   1
#define	ARP_RPLY  2
#define	ARP_RREQ  3
#define	ARP_RRPLY 4

#define ARP_MAXOP 4

#define ARP_STACK_LEN	100

typedef enum { ARP_ARP, ARP_RARP } ArpType;
typedef enum { ARP_FREE, ARP_ALLOC, ARP_RSLVD } ArpStatus;

typedef struct {
    short  arp_hrd;
    short  arp_prot;
    char   arp_hlen;
    char   arp_plen;
    short  arp_op;
    ETHhost  arp_sha;
    IPhost arp_spa;
    ETHhost  arp_tha;
    IPhost arp_tpa;
} ArpHdr;


/*
 * An arpWait represents an outstanding request
 */
typedef struct {
    ArpStatus	*status;
    int 	tries;
    Event	event;
    xkSemaphore 	s;
    int 	numBlocked;
    XObj	self;		/* ARP protocol */
    ArpHdr	reqMsg;		/* ARP requests only */
    XObj	lls;
} ArpWait;


typedef struct {
    XObj		rarp;
    struct arpent	**tbl;
    XObj		arpSessn;
    ArpHdr		hdr;
} PState;

void		arpPlatformInit( XObj );
xkern_return_t	arpSendRequest( ArpWait * );
void		newArpWait( ArpWait *, XObj, IPhost *, ArpStatus * );
xkern_return_t	newRarpWait( ArpWait *, XObj, ETHhost *, ArpStatus * );

extern int	tracearpp;

#endif /* arp_i_h */
