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
 * chan_internal.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * chan_internal.h,v
 * Revision 1.41.1.4.1.2  1994/09/07  04:18:43  menze
 * OSF modifications
 *
 * Revision 1.41.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.41.1.3  1994/07/22  20:08:09  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.41.1.2  1994/06/30  01:35:52  menze
 * Control messages are built using the allocator of the request
 * message, not the allocator attached to the session.
 *
 * Revision 1.41.1.1  1994/04/14  21:37:17  menze
 * Uses allocator-based message interface
 *
 */
 
#ifndef chan_internal_h
#define chan_internal_h

#include <xkern/include/prot/chan.h>

#define CHAN_STACK_LEN	        100
#define CHANHLEN  		sizeof(CHAN_HDR) 
#define CHANEXTIDLEN 		sizeof(EXT_ID);
/* 
 * START_SEQ must be > 0
 */
#define START_SEQ 		1
 
#define BOOTID	1
#define CHAN_ALLOW_USER_ABORT 1

/* 
 * Flags
 */
#define	FROM_CLIENT	0x01
#define	USER_MSG	0x02	/* valid user data */
#define ACK_REQUESTED	0x04
#define NEGATIVE_ACK	0x08


/* 
 * Protocol down vector indices
 */
#define CHAN_BIDCTL_I	1


typedef enum {
    /*--- Server states */
    SVC_EXECUTE = 1,	/* Sent msg up, waiting for answer */
    SVC_WAIT,		/* Waiting for an ACK of the reply */
    SVC_IDLE,
    /*--- Client states */
    CLNT_CALLING,
    CLNT_WAIT,
    CLNT_FREE,
    /*--- Client or server */
    DISABLED
} FsmState;


/*--- Message sequence possible status */
typedef enum {
  	old, current, new
} SEQ_STAT;

typedef u_int	SeqNum;
typedef u_short	Channel;

/*--- Channel Header structure */ 
typedef struct  {
    Channel	chan;
    u_char	unused;
    u_char 	flags;
    u_int	prot_id;
    SeqNum	seq;
    u_int	len;
} CHAN_HDR;


typedef struct {
    Msg_s	m;
    char 	valid;
} ChanMsg;


/*--- Channel State structure */
typedef
struct {
	u_char		isAsync;
	u_char		fsmState;
        int 		wait;
	int		waitParam;	/* user configurable wait value */
	int		maxWait;	/* user configurable wait value */
        Event 		event;
	CHAN_HDR 	hdr;
	ChanMsg		saved_msg;
	/* 
	 * client only
	 */
	xkern_return_t	replyValue;
        Msg 		answer;
        xkSemaphore 	reply_sem;
	chan_info_t	info;
	IPhost		peer;
} CHAN_STATE, SState;

#define CHAN_IDLE_CLIENT_MAP_SZ		31
#define	CHAN_IDLE_SERVER_MAP_SZ		31
#define CHAN_ACTIVE_CLIENT_MAP_SZ	101
#define CHAN_ACTIVE_SERVER_MAP_SZ	101
#define CHAN_HLP_MAP_SZ		1

#define FIRST_CHAN	1

typedef struct {
    Map			idleMap;
    Map			keyMap;
    Map 		hostMap;
    XObjInitFunc	*initFunc;
} SessnInfo;

/*--- Protocol state structure */
typedef struct {
    SessnInfo	svrInfo;
    SessnInfo	cliInfo;
    Map 	passiveMap;
    Map		newChanMap;
    xkSemaphore	newSessnLock;
} PSTATE, PState;

/*--- Ids used to demux to active sessions */
typedef struct {
    int		chan;
    XObj	lls;
    u_int 	prot_id;
} ActiveID;


/*--- prot_id serves as passive key to demux to passive sessions */
typedef unsigned int PassiveID;


/*--- Delays are specified in microseconds! */
#define SERVER_WAIT_DEFAULT	8 * 1000 * 1000		/* 8 seconds */
#define SERVER_MAX_WAIT_DEFAULT	60 * 1000 * 1000	/* 32 seconds */
#define CLIENT_WAIT_DEFAULT	5 * 1000 * 1000		/* 5 seconds */
#define CLIENT_MAX_WAIT_DEFAULT	60 * 1000 * 1000	/* 20 seconds */


/*--- We still need to figure out a good delay, may need to query
      bottom protocol for hints. */
#define CHAN_CLNT_DELAY(m, p) 	(p)
#define CHAN_SVC_DELAY(m, p)  	(p)

#define msg_valid(M)	((M).valid = 1)
#define msg_clear(M)	((M).valid = 0)
#define msg_isnull(M)	((M).valid == 0)
#define msg_flush(M) 	do { if ((M).valid) {  msgDestroy(&(M).m);  \
			    	 	    (M).valid=0; } } while(0)

#define MAX(x,y) 		((x)>(y) ? (x) : (y))
#define MIN(x,y) 		((x)<(y) ? (x) : (y))


typedef		int	(* MapChainForEachFun)(
					       void *, void *
					       );

typedef		XObj (ChanOpenFunc)( XObj, XObj, XObj, ActiveID *, int,
				     boolean_t, Path );
xkern_return_t	chanAddIdleSessn( XObj, SessnInfo * );
xkern_return_t	chanBidctlRegister( XObj, IPhost * );
xkern_return_t	chanCall( XObj, Msg, Msg );
int		chanCheckMsgLen( u_int, Msg );
SEQ_STAT 	chanCheckSeq( u_int, u_int );
void		chanClientIdleRespond( CHAN_HDR	*, XObj, u_int, Path );
void		chanClientPeerContact( PSTATE *, IPhost * );
void		chanClientPeerRebooted( PSTATE *, IPhost * );
int		chanControlSessn( XObj, int, char *, int );
XObj 		chanCreateSessn( XObj, XObj, XObj, ActiveID *,
				 SessnInfo *, boolean_t, Path );
void		chanDestroy( XObj );
void		chanDispKey( ActiveID * );
void		chanFreeResources( XObj );
Map		chanGetChanMap( XObj, IPhost *, Path );
Map 		chanGetMap( Map, XObj, long );
XObjInitFunc	chanGetProcClient;
XObjInitFunc	chanGetProcServer;
long		chanGetProtNum( XObj, XObj );
int		chanMapRemove( void *, void * );
xkern_return_t 	chanOpenEnable( XObj, XObj, XObj, Part );
xkern_return_t 	chanOpenDisable( XObj, XObj, XObj, Part );
xkern_return_t 	chanOpenDisableAll( XObj, XObj );
xmsg_handle_t	chanPush( XObj, Msg );
void		chanRemoveActive( XObj, Map keyMap, Map hostMap );
int		chanRemoveIdleSessns( Map, IPhost * );
xmsg_handle_t	chanReply( XObj, CHAN_HDR *, int, Path );
xmsg_handle_t	chanResend( XObj, int, int );
void		chanServerPeerRebooted( PSTATE *, IPhost * );
char * 		chanStateStr( int );
void		chanTimeout( Event, void * );
ChanOpenFunc	chanSvcOpenNew;
ChanOpenFunc	chanSvcOpen;
void		chanHdrStore( void *, char *, long, void * );
XObj		chanOpen( XObj, XObj, XObj, Part, Path );
char * 		chanStatusStr( SEQ_STAT );
void 		pChanHdr( CHAN_HDR * );
void            killticket( XObj );

/* 
 * chan_mapchain.c
 */
xkern_return_t	chanMapChainAddObject( void *, Map, IPhost *, long, int, 
				       Path );
int		chanMapChainApply( Map, IPhost *, MapChainForEachFun );
Map		chanMapChainFollow( Map, IPhost *, long );
xkern_return_t	chanMapChainReserve( Map, IPhost *, long, Path, int );

extern	int	tracechanp;

#endif
