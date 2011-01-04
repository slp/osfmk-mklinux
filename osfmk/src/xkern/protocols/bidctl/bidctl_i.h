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
 * bidctl_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.11.1 $
 * $Date: 1995/02/23 15:29:01 $
 */

#ifndef bidctl_i_h
#define bidctl_i_h

/* 
 * Declarations private to the BIDCTL protocol (private even from BID).
 */


#include <xkern/include/prot/bidctl.h>

/* 
 * Message flags
 */

#define	BIDCTL_QUERY_F		0x1
#define	BIDCTL_RESPONSE_F	0x2
#define BIDCTL_BCAST_F		0x4


typedef enum {
    BIDCTL_NO_QUERY = 0, BIDCTL_NEW_QUERY, BIDCTL_REPEAT_QUERY
} BidctlQuery;


/* 
 * Session finite state machine states
 */
typedef enum {
    BIDCTL_INIT_S,
    BIDCTL_NORMAL_S,
    BIDCTL_QUERY_S,
    BIDCTL_RESET_S,
    BIDCTL_OPEN_S,
    BIDCTL_OPENDONE_S
} FsmState;


/* 
 * A BidCtlState is shared among sessions with the same remote host.
 * It may also exist in the protocol idMap without being used by any
 * session. 
 */
typedef struct {
    IPhost	peerHost;
    FsmState	fsmState;
    BootId	peerBid;
    u_int	timer;	
    u_int	rcnt;
    BootId	reqTag;		/* Unique stamp we used in our request msg */
    Bind	bind;
    XObj	lls;
    XObj	myProtl;
    u_int	retries;
    Event	ev;
    Map		hlpMap;
    xkSemaphore	waitSem;
    int		waitSemCount;
    int		timeout;
} BidctlState;


typedef struct {
    u_short	flags;
    u_short	sum;
    BootId	srcBid;		/* sender boot ID   */
    BootId	dstBid;		/* receiver boot ID */
    BootId	reqTag;		/* receiver boot ID */
    BootId	rplTag;		/* receiver boot ID */
} BidctlHdr;


typedef struct {
    BootId	myBid;
    ClusterId   myCluster;      
    Map		bsMap;		/* remote host -> BootidState	    */
} PState;


extern int	tracebidctlp;


/* 
 * Configuration constants
 */
#define BIDCTL_BSMAP_SIZE	101
#define BIDCTL_HLPMAP_SIZE	131
#define BIDCTL_REG_MAP_SIZE	31

#define BIDCTL_OPEN_TIMEOUT	5 * 1000 * 1000		/* 5 seconds */
#define BIDCTL_OPEN_TIMEOUT_MULT	2
#define BIDCTL_OPEN_TIMEOUT_MAX 60 * 1000 * 1000

#define BIDCTL_QUERY_TIMEOUT	5 * 1000 * 1000		/* 5 seconds */
#define BIDCTL_QUERY_TIMEOUT_MULT	2
#define BIDCTL_QUERY_TIMEOUT_MAX 60 * 1000 * 1000

#define BIDCTL_TIMER_INTERVAL	60 * 1000 * 1000
#define BIDCTL_KEEPALIVE	5			/* timer intervals */
#define BIDCTL_IDLE_LIMIT	10 			/* timer intervals */
/* 
 * BIDCTL_BCAST_QUERY_DELAY is low for testing purposes.  We probably
 * want a larger value.
 */
#define BIDCTL_BCAST_QUERY_DELAY	3 * 1000	/* 3 seconds */

#define BIDCTL_STACK_LEN	100

void	bidctlBcastBoot( XObj );
xkern_return_t	bidctlDestroy( BidctlState * );
void	bidctlDispState( BidctlState * );
char *	bidctlFsmStr( FsmState );
void	bidctlHdrDump( BidctlHdr *h, char *, IPhost * );
void	bidctlHdrStore( void *, char *, long, void * );
BootId	bidctlNewId( void );
void	bidctlOutput( BidctlState *, BidctlQuery sendQuery, int sendResponse,
		      BidctlHdr * );
void	bidctlReleaseWaiters( BidctlState * );
BootId	bidctlReqTag( void );
void	bidctlSemWait( BidctlState * );
void	bidctlStartQuery( BidctlState *, int );
void	bidctlTimer( Event, void * );
void	bidctlTimeout( Event, void * );
void	bidctlTransition( BidctlState *, BidctlHdr * );

#endif  /* ! bootid_i_h */
