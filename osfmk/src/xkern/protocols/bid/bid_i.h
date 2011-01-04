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
 * bid_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.11.1 $
 * $Date: 1995/02/23 15:28:47 $
 */

/* 
 * Declarations private to the BID protocol
 */


#ifndef bid_i_h
#define bid_i_h

#include <xkern/include/prot/bidctl.h>

#define BID_CHECK_CLUSTERID 1

typedef struct {
    BootId	srcBid;		/* sender boot ID   */
    BootId	dstBid;		/* receiver boot ID */
#if BID_CHECK_CLUSTERID
    ClusterId   srcCluster;     /* sender cluster ID */
#endif /* BID_CHECK_CLUSTERID */
    long	hlpNum;
} BidHdr;


typedef struct {
    BootId	myBid;
#if BID_CHECK_CLUSTERID
    ClusterId   myCluster;
#endif /* BID_CHECK_CLUSTERID */
    Map		activeMap;	/* lls -> bootid sessions 	*/
    Map		passiveMap;	/* hlpNum -> hlps 	 	*/
    xkSemaphore	sessnCreationSem;
} PState;



typedef struct {
    BidHdr	hdr;
    IPhost	peer;
} SState;



typedef struct {
    XObj	lls;
    long	hlpNum;
} ActiveKey;

typedef long	PassiveKey;



/* 
 * Configuration constants
 */
#define BID_ACTIVE_MAP_SIZE	101
#define BID_PASSIVE_MAP_SIZE	31

#define BID_STACK_LEN	100

/* 
 * Protocol down vector indices
 */
#define BID_XPORT_I	0
#define BID_CTL_I	1

#endif  /* ! bootid_i_h */
