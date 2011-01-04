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
 * chan.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * chan.c,v
 * Revision 1.59.1.7.1.2  1994/09/07  04:18:42  menze
 * OSF modifications
 *
 * Revision 1.59.1.7  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.59.1.6  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.59.1.5  1994/08/10  21:17:38  menze
 * Server times out on failed push instead of destroying reply
 *
 * Revision 1.59.1.4  1994/07/22  20:08:08  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.59.1.3  1994/06/30  01:35:11  menze
 * Control messages are built using the allocator of the request
 * message, not the allocator attached to the session.
 *
 * Revision 1.59.1.2  1994/04/23  00:22:54  menze
 * timeout code was resending to the wrong session and dereferencing
 * null pointers.
 *
 * Revision 1.59.1.1  1994/04/14  21:36:26  menze
 * Uses allocator-based message interface
 *
 * Revision 1.59  1993/12/13  22:37:47  menze
 * Modifications from UMass:
 *
 *   [ 93/09/21          nahum ]
 *   Got rid of casts in mapUnbind, mapResolve calls
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/chan/chan_internal.h>
#ifdef BOOTID
#include <xkern/include/prot/bidctl.h>
#endif /* BOOTID */


/* If NO_KILLTICKET is defined, CHAN will not inform the lower protocol when it
 * can free a message.  The lower protocol will then free messages based on
 * its timeouts.
 */
/* #define NO_KILLTICKET */

#define TIMEOUT_TRACE	TR_SOFT_ERRORS

/*
 * Global data
 */

#if XK_DEBUG
int 		tracechanp=1;
#endif /* XK_DEBUG */

#ifdef BOOTID
typedef struct {
    XObj	self;
    IPhost	peer;
} DisableStubInfo;
#endif /* BOOTID */

static xkern_return_t	chanBidctlUnregister( XObj, IPhost * );
static int		chanControlProtl( XObj, int, char *, int );
static long		chanHdrLoad( void *, char *, long, void * );
static void		disableStub( Event, void * );
static int		getIdleSeqNum( Map, ActiveID * );
static void 		getProcProtl( XObj );
static XObj		getSpecIdleSessn( XObj, Enable *, ActiveID *,
					  Map, Map, Path );

#define HDR ((CHAN_HDR *)hdr)

/*
 * chanHdrStore - Used when calling msgPush
 */
void
chanHdrStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    CHAN_HDR h;
    
    xAssert(len == CHANHLEN);  
    h.chan = htons(HDR->chan);
    h.flags = HDR->flags;
    h.prot_id = htonl(HDR->prot_id);
    h.seq = htonl(HDR->seq);
    h.len = htonl(HDR->len);
    bcopy((char *)(&h), dst, CHANHLEN);
}


/*
 * chanHdrLoad - Used when calling msgPop
 */
static long
chanHdrLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    xAssert(len == sizeof(CHAN_HDR));  
    bcopy(src, (char *)hdr, CHANHLEN);
    HDR->chan = ntohs(HDR->chan);
    HDR->prot_id = ntohl(HDR->prot_id);
    HDR->seq = ntohl(HDR->seq);
    HDR->len = ntohl(HDR->len);
    return CHANHLEN;
}

#undef HDR

  
static void
psFree( PSTATE *ps )
{
    if ( ps->svrInfo.idleMap ) mapClose(ps->svrInfo.idleMap);
    if ( ps->svrInfo.keyMap ) mapClose(ps->svrInfo.keyMap);
    if ( ps->svrInfo.hostMap ) mapClose(ps->svrInfo.hostMap);
    if ( ps->cliInfo.idleMap ) mapClose(ps->cliInfo.idleMap);
    if ( ps->cliInfo.keyMap ) mapClose(ps->cliInfo.keyMap);
    if ( ps->cliInfo.hostMap ) mapClose(ps->cliInfo.hostMap);
    if ( ps->passiveMap ) mapClose(ps->passiveMap);
    if ( ps->newChanMap ) mapClose(ps->newChanMap);
    pathFree(ps);
}


#define newMap(map, sz, keySz) 					\
	if ( ! ((map) = mapCreate((sz), (keySz), path)) ) {	\
	    psFree(ps);						\
	    return XK_FAILURE;					\
	}		
/*
 * chan_init
 */
xkern_return_t
chan_init(self)
    XObj self;
{
    Part_s	part;
    PSTATE 	*ps;
    Path	path = self->path;
    
    xTrace0(chanp, TR_GROSS_EVENTS, "CHAN init");
    
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTrace0(chanp, TR_ERRORS,
		"CHAN could not get get transport protl -- not initializing");
	return XK_FAILURE;
    }
#ifdef BOOTID
    if ( ! xIsProtocol(xGetDown(self, CHAN_BIDCTL_I)) ) {
	xTrace0(chanp, TR_ERRORS,
		"CHAN could not get get bidctl protl -- not initializing");
	return XK_FAILURE;
    }
#endif /* BOOTID */
    if ( ! (self->state = ps = pathAlloc(path, sizeof(PSTATE))) ) {
	xTraceP0(self, TR_ERRORS, "allocation errors in init");
	return XK_FAILURE;
    }
    bzero((char *)ps, sizeof(PSTATE));
    newMap(ps->svrInfo.idleMap, CHAN_IDLE_SERVER_MAP_SZ, sizeof(IPhost));
    newMap(ps->svrInfo.keyMap, CHAN_ACTIVE_SERVER_MAP_SZ, sizeof(ActiveID));
    newMap(ps->svrInfo.hostMap, CHAN_ACTIVE_SERVER_MAP_SZ, sizeof(IPhost));
    newMap(ps->cliInfo.idleMap, CHAN_IDLE_CLIENT_MAP_SZ, sizeof(IPhost));
    newMap(ps->cliInfo.keyMap, CHAN_ACTIVE_CLIENT_MAP_SZ, sizeof(ActiveID));
    newMap(ps->cliInfo.hostMap, CHAN_ACTIVE_CLIENT_MAP_SZ, sizeof(IPhost));
    newMap(ps->passiveMap, CHAN_HLP_MAP_SZ, sizeof(PassiveID));
    newMap(ps->newChanMap, CHAN_HLP_MAP_SZ, sizeof(long));
    ps->svrInfo.initFunc = chanGetProcServer;
    ps->cliInfo.initFunc = chanGetProcClient;
    getProcProtl(self);
    semInit(&ps->newSessnLock, 1);
    
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, 0), &part) == XK_FAILURE ) {
	xTrace0(chanp, TR_ERRORS,
		"chan_init: can't openenable transport protocol");
	psFree(ps);
	return XK_FAILURE;
    }
    xTrace0(chanp, TR_GROSS_EVENTS, "CHAN init done");
    return XK_SUCCESS;
}
#undef newMap


/* 
 * Adds an entry to the '{ peer -> { hlp -> newChannel } }' map for
 * the given peer if it doesn't already exist.  If it doesn't exist,
 * CHAN's interest in this host is communicated to BIDCTL.
 */
Map
chanGetChanMap( XObj self, IPhost *h, Path path )
{
    Map		hlpMap;
    PState	*ps = (PState *)self->state;

    if ( mapResolve(ps->newChanMap, h, &hlpMap) == XK_FAILURE ) {
	xTrace1(chanp, TR_EVENTS, "creating new 'newChanMap' for host %s",
		ipHostStr(h));
	if ( ! (hlpMap = mapCreate(CHAN_HLP_MAP_SZ, sizeof(IPhost), path)) ) {
	    return 0;
	}
#ifdef BOOTID
	if ( chanBidctlRegister(self, h) == XK_FAILURE ) {
	    mapClose(hlpMap);
	    return 0;
	}
#endif /* BOOTID */
	if ( mapBind(ps->newChanMap, h, hlpMap, 0) != XK_SUCCESS ) {
	    mapClose(hlpMap);
#ifdef BOOTID
	    chanBidctlUnregister(self, h);
#endif /* BOOTID */
	    return 0;
	}
    }
    return hlpMap;
}


/* 
 * chanCreateSessn -- create a new channel session using the given
 * XObjects and the information in 'key'.  The session is bound into
 * 'keyMap' with 'key' and is also bound into the mapchain rooted at
 * 'hostMap.'
 */
XObj 
chanCreateSessn( self, hlpRcv, hlpType, key, info, newChan, path )
    XObj 	self, hlpRcv, hlpType;
    ActiveID	*key;
    Path	path;
    SessnInfo	*info;
    boolean_t	newChan;
{
    XObj   	s;
    CHAN_STATE 	*ss;
    CHAN_HDR 	*hdr;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN createSessn ......................");
    xTraceP1(self, TR_MAJOR_EVENTS, "createSession: path %d", path->id);
    xIfTrace(chanp, TR_MAJOR_EVENTS) chanDispKey(key);
    if ( ! (ss = pathAlloc(path, sizeof(CHAN_STATE))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return ERR_XOBJ;
    }
    bzero((char *)ss, sizeof(CHAN_STATE));
    /*
     * Fill in  state
     */
    msg_clear(ss->saved_msg);
    semInit(&ss->reply_sem, 0);
    if ( xControl(key->lls, GETPEERHOST, (char *)&ss->peer, sizeof(IPhost))
				< (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS,
		"chan_open: can't do GETPEERHOST on lower session");
	pathFree(ss);
	return ERR_XOBJ;
    }
    if ( chanGetChanMap(self, &ss->peer, path) == 0 ) {
	pathFree(ss);
	return ERR_XOBJ;
    }
    /*
     * Fill in header
     */
    hdr = &ss->hdr;
    hdr->chan = key->chan;
    hdr->prot_id = key->prot_id;
    hdr->flags = USER_MSG;
    /*
     * Create session and bind to address
     */
    xDuplicate(key->lls);
    s = xCreateSessn(info->initFunc, hlpRcv, hlpType, self, 1, &key->lls, path);
    if ( s == ERR_XOBJ ) {
	xClose(key->lls);
	pathFree(ss);
	return ERR_XOBJ;
    }
    s->state = (char *) ss;
    if ( ! (ss->event = evAlloc(path)) ) {
	chanDestroy(s);
	return ERR_XOBJ;
    }
    if ( mapBind(info->keyMap, key, s, &s->binding) != XK_SUCCESS ) {
	xTraceP0(self, TR_SOFT_ERRORS, "createSessn: could not bind session");
	chanDestroy(s);
	return ERR_XOBJ;
    }
    if ( newChan ) {
	/* 
	 * Make sure there's enough room to bind this channel into the
	 * idle map when it is closed.  
	 */
	if ( chanMapChainReserve(info->idleMap, &ss->peer, key->prot_id,
				 path, 1) == XK_FAILURE ) {
	    chanDestroy(s);
	    return ERR_XOBJ;
	}
    }
    if ( chanMapChainAddObject(s, info->hostMap, &ss->peer,
			       key->prot_id, key->chan, path) == XK_FAILURE ) {
	chanMapChainReserve(info->idleMap, &ss->peer, key->prot_id, path, -1);
	mapUnbind(info->keyMap, key);
	chanDestroy(s);
	return ERR_XOBJ;
    }
    xTrace1(chanp, TR_MAJOR_EVENTS, "chanCreateSessn returns %x", s);
    return s;
}



void
chanDestroy( s )
    XObj 	s;
{
    CHAN_STATE	*sstate;
    PSTATE 	*pstate;
    XObj	lls;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN Destroy ........................");
    xTrace1(chanp, TR_MAJOR_EVENTS, "Of session %x", s);
    xAssert( xIsSession(s) );
    xAssert(s->rcnt == 0);
    xAssert(s->binding == 0);
    pstate  = (PSTATE *)s->myprotl->state;
    sstate  = (CHAN_STATE *)s->state;
    /*
     * Free chan state
     */
    if (sstate) {
	msg_flush(sstate->saved_msg); 
	lls = xGetDown(s, 0);
	if ( lls != ERR_XOBJ ) {
	    xClose(lls);
	}
	if ( sstate->event ) {
	    evDetach(sstate->event);
	}
    }
    xDestroy(s);
    return;
}


/*
 * chanDemux
 */
static xkern_return_t
chanDemux(
    XObj self,
    XObj lls,
    Msg msg )
{
    CHAN_HDR 	hdr;
    XObj   	s;
    ActiveID 	actKey;
    PassiveID 	pasKey;
    PSTATE 	*ps = (PSTATE *)self->state;
    Enable	*e;
    Map		map;
    
    xTrace0(chanp, TR_EVENTS, "CHAN demux .............................");

    if ( msgPop(msg, chanHdrLoad, (void *)&hdr, CHANHLEN, 0) == XK_FAILURE ) {
	xError("chanDemux: msgPop returned false");
	return XK_FAILURE;
    }
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(&hdr);
    } 
    /*
     * Check for active channel
     */
    actKey.chan		= hdr.chan;
    actKey.prot_id 	= hdr.prot_id;
    actKey.lls 	= lls;
    xIfTrace(chanp, TR_DETAILED) {
	chanDispKey( &actKey );
    }
    map = (hdr.flags & FROM_CLIENT) ? ps->svrInfo.keyMap : ps->cliInfo.keyMap;
    if ( mapResolve(map, &actKey, &s) == XK_SUCCESS ) {
	/*
	 * Pop to active channel
	 */
	xTrace1(chanp, TR_MORE_EVENTS, "chanDemux: existing channel %s", s);
	return xPop(s, lls, msg, &hdr);
    } 
    /* 
     * Look for an idle/dormant session
     */
    if ( ! (hdr.flags & FROM_CLIENT) ) {
	int	seq;

	seq = getIdleSeqNum(ps->cliInfo.idleMap, &actKey);
	if ( seq == -1 ) {
	    /* 
	     * We never heard of this channel -- we'll drop the msg
	     */
	    xTrace0(chanp, TR_SOFT_ERRORS,
		    "spurious msg for non-existent channel");
	} else {
	    chanClientIdleRespond(&hdr, actKey.lls, seq, msgGetPath(msg));
	}
    } else {
	/*
	 * Find openenable
	 */
	pasKey = actKey.prot_id;
	if ( mapResolve(ps->passiveMap, &pasKey, &e) == XK_FAILURE ) {
	    xTrace1(chanp, TR_EVENTS,
		    "chanDemux -- no openenable for hlp %d", hdr.prot_id);
	} else {
	    /* 
	     * Try to create a session
	     */
	    semWait(&ps->newSessnLock);
	    /* 
	     * Look again ...
	     */
	    if ( mapResolve(map, &actKey, &s) == XK_FAILURE ) {
		s = getSpecIdleSessn(self, e, &actKey, 
				     ps->svrInfo.idleMap, ps->svrInfo.keyMap,
				     msgGetPath(msg));
		if ( s == ERR_XOBJ ) {
		    s = chanSvcOpen(self, e->hlpRcv, e->hlpType, &actKey, 0, TRUE,
				    msgGetPath(msg));
		}
	    } else {
		/*
		 * Pop to active channel
		 */
		xTrace1(chanp, TR_MORE_EVENTS, "chanDemux: new channel %s created by someone else", s);
	    } 
	    semSignal(&ps->newSessnLock);
	    if ( s != ERR_XOBJ ) {
		xPop(s, lls, msg, &hdr);
	    } else {
		xTrace0(chanp, TR_SOFT_ERRORS,
			"chanDemux: can't create session ");
	    }
	}
    }
    return XK_SUCCESS;
}


/* 
 * chanCheckMsgLen -- checks the length field of the header with the
 * actual length of the message, truncating it if necessary.
 *
 * returns 0 if the message is long enough and
 * 	  -1 if the message is too short.
 */
int
chanCheckMsgLen( 
    u_int 	hdrLen,
    Msg 	m )
{
    u_int	dataLen;

    dataLen = msgLen(m);
    xTrace2(chanp, TR_DETAILED,
	    "chan checkLen: hdr->len = %d, msg_len = %d", hdrLen,
	    dataLen);
    if (hdrLen < dataLen) {
	xTrace0(chanp, TR_MORE_EVENTS, "chan_pop: truncating msg");
	msgTruncate(m, hdrLen);
    } else if (hdrLen > dataLen) {
	xTrace0(chanp, TR_SOFT_ERRORS, "chan_pop: message too short!");
	return -1;
    }
    return 0;
}


static int
chanControlProtl( self, op, buf, len )
    XObj 	self;
    int 	op, len;
    char 	*buf;
{
    PSTATE		*ps = (PSTATE *)self->state;
#ifdef BOOTID
    BidctlBootMsg	*msg;
#endif /* BOOTID */

    switch ( op ) {

#ifdef BOOTID
      /* 
       * The BIDCTL_* handlers must not block
       */
      case BIDCTL_FIRST_CONTACT:
	{
	    msg = (BidctlBootMsg *)buf;
	    chanClientPeerContact(ps, &msg->h);
	}
	return 0;

      case BIDCTL_PEER_REBOOTED:
	{
	    Map			hlpMap;
	    xkern_return_t	res;

	    msg = (BidctlBootMsg *)buf;
	    xTrace1(chanp, TR_MAJOR_EVENTS,
		    "chan receives notification that peer %s rebooted",
		    ipHostStr(&msg->h));
	    if ( mapResolve(ps->newChanMap, &msg->h, &hlpMap) == XK_FAILURE ) {
		/* 
		 * We shouldn't have been notified of this reboot
		 */
		xTrace1(chanp, TR_SOFT_ERRORS,
			"CHAN receives spurious notification of %s reboot",
			ipHostStr(&msg->h));
	    } else {
		chanClientPeerRebooted(ps, &msg->h);
		chanServerPeerRebooted(ps, &msg->h);
		mapClose(hlpMap);
		res = mapUnbind(ps->newChanMap, &msg->h);
		xAssert(res == XK_SUCCESS);
		{
		    /* 
		     * Schedule an openDisable to run on BIDCTL.  It's
		     * actually not critical that this run ... if we
		     * can't run it, we'll just get spurious
		     * notifications later. 
		     */
		    DisableStubInfo *dsInfo;
		    Event ev;

		    dsInfo = pathAlloc(self->path, sizeof(DisableStubInfo));
		    if ( ! dsInfo ) {
			return 0;
		    }
		    if ( ! (ev = evAlloc(self->path)) ) {
			pathFree(dsInfo);
			return 0;
		    }
		    dsInfo->peer = msg->h;
		    dsInfo->self = self;
		    evDetach(evSchedule(ev, disableStub, dsInfo, 0));
		}
	    }
	}
	return 0;
#endif /* BOOTID */

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}
  

static xkern_return_t
errCall( XObj s, Msg m, Msg rm )
{
    return XK_FAILURE;
}

static xmsg_handle_t
errPush( XObj s, Msg m )
{
    return XMSG_ERR_HANDLE;
}


/*
 * chanControlSessn
 */
int
chanControlSessn( self, opcode, buf, len )
    XObj 	self;
    int 	opcode, len;
    char 	*buf;
{
    CHAN_STATE *sstate;
    
    xTrace0(chanp, TR_EVENTS, "CHAN controlsessn ......................");
    xTrace1(chanp, TR_EVENTS, "Of session=%x", self); 
    
    sstate = (CHAN_STATE *)self->state;
    
    switch (opcode) {
	
      case GETMYPROTO:
      case GETPEERPROTO:
	checkLen(len, sizeof(long));
	*(long *)buf = sstate->hdr.prot_id;
	return sizeof(long);

      case GETMAXPACKET:
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	if ( xControl(xGetDown(self, 0), opcode, buf, len) <= 0 ) {
	    return -1;
	}
	*(int *)buf -= sizeof(CHAN_HDR);
	return sizeof(int);
	
      case GETPEERHOST:
	checkLen(len, sizeof(IPhost));
	*(IPhost *)buf = sstate->peer;
	return sizeof(IPhost);

      case CHAN_SET_TIMEOUT:
	checkLen(len, sizeof(int));
	sstate->waitParam = *(int *)buf;
	xTrace1(chanp, TR_EVENTS, "channel session timeout set to %d",
		sstate->waitParam);
	return 0;
	
      case CHAN_SET_MAX_TIMEOUT:
	checkLen(len, sizeof(int));
	sstate->maxWait = *(int *)buf;
	xTrace1(chanp, TR_EVENTS, "channel session max timeout set to %d",
		sstate->maxWait);
	return 0;
	
      case CHAN_GET_TIMEOUT:
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->waitParam;
	return sizeof(int);

      case CHAN_GET_MAX_TIMEOUT:
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->maxWait;
	return sizeof(int);

      case CHAN_SET_ASYNC:
	checkLen(len, sizeof(int));
	if ( sstate->fsmState != CLNT_FREE ) {
	    return -1;
	}
	if ( *(int *)buf && ! sstate->isAsync ) {
	    self->call = errCall;
	    self->push = chanPush;
	    sstate->isAsync = 1;
	} else if ( *(int *)buf == 0 && sstate->isAsync ) {
	    self->call = chanCall;
	    self->push = errPush;
	    sstate->isAsync = 0;
	}
	return 0;

      case CHAN_GET_ASYNC:
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->isAsync;
	return sizeof(int);

      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}


/*
 * killticket: get rid of any dangling fragments 
 */
void
killticket(s)
    XObj s;
{
    CHAN_STATE *ss=(CHAN_STATE *)s->state;

#ifndef NO_KILLTICKET
    if ( ss->info.transport) {
	xControl(ss->info.transport, FREERESOURCES, (char *)&ss->info.ticket, sizeof(ss->info.ticket));
    }
#endif
}


/* 
 * Release timeout resources currently held (timeout events, saved
 * messages, tickets for lower messages).  Works for both client and
 * server
 */
void
chanFreeResources(self)
    XObj self;
{
    CHAN_STATE	*state;

    state = (CHAN_STATE *)self->state;
    xAssert(state);
    killticket(self);
    msg_flush(state->saved_msg);
    evCancel(state->event);
}


/*
 * chanReply -- send a control reply (no user data) to lower session
 * 's' using the given header and flags.
 */
xmsg_handle_t
chanReply( s, hdr, flags, path )
    XObj 	s;
    CHAN_HDR 	*hdr;
    int 	flags;
    Path	path;
{
    CHAN_HDR 		hdr_copy;
    Msg_s 		msg;
    xmsg_handle_t 	rval;
    xkern_return_t	xkr;

    xAssert(xIsSession(s));
    hdr_copy 	 = *hdr;
    hdr_copy.flags = flags;
    hdr_copy.len 	 = 0;
    xTrace0(chanp, TR_EVENTS, "chan_pop: Sending reply");
    xIfTrace(chanp, TR_MORE_EVENTS) {
	pChanHdr(&hdr_copy);
    }
    if ( msgConstructContig(&msg, path, 0, CHAN_STACK_LEN) == XK_FAILURE ) {
	xTraceP0(s, TR_SOFT_ERRORS, "Couldn't allocate control message");
	return XMSG_ERR_HANDLE;
    }
    xkr = msgPush(&msg, chanHdrStore, &hdr_copy, CHANHLEN, NULL);
    xAssert( xkr == XK_SUCCESS );
    rval = xPush(s, &msg);
    msgDestroy(&msg);
    return rval;
}


/*
 * chanCheckSeq -- determine the relation of the new sequence number 
 * 'new_seq' to 'cur_seq'.  
 */
SEQ_STAT 
chanCheckSeq(cur_seq, new_seq)
    unsigned int cur_seq;
    unsigned int new_seq;
{
    if (cur_seq == new_seq) {
	xTrace0(chanp, TR_MORE_EVENTS, "chanPop: current sequence number");
	return(current);
    }
    if (cur_seq < new_seq) {
	xTrace0(chanp, TR_MORE_EVENTS, "chanPop: new sequence number");
	return(new);
    }
    xTrace0(chanp, TR_MORE_EVENTS, "chanPop: old sequence number");
    return(old);
}


/* 
 * chanGetProtNum -- determine the protocol number of 'hlp'
 * relative to this protocol
 */
long
chanGetProtNum( self, hlp )
    XObj	self, hlp;
{
    long n;

    if ( (n = relProtNum(hlp, self)) == -1 ) {
	xTrace1(chanp, TR_ERRORS,
		"chan could not get relative protocol number of %s",
		hlp->name);
    }
    return n;
}
  


/* 
 * getProcProtl
 */
static void 
getProcProtl(s)
    XObj s;
{
    xAssert(xIsProtocol(s));
    s->control 	= chanControlProtl;
    s->open   	= chanOpen;
    s->openenable = chanOpenEnable;
    s->opendisable = chanOpenDisable;
    s->opendisableall = chanOpenDisableAll;
    s->demux 	= chanDemux;
}



#ifdef BOOTID
/* 
 * Register this protocol's interest in 'peer' with BIDCTL
 */
xkern_return_t
chanBidctlRegister( self, peer )
    XObj	self;
    IPhost	*peer;
{
    XObj	llp;
    Part_s	p;
    
    xAssert(xIsProtocol(self));
    xTrace1(chanp, TR_MAJOR_EVENTS,
	    "chan registering interest in peer %s with bidctl",
	    ipHostStr(peer));
    llp = xGetDown(self, CHAN_BIDCTL_I);
    xAssert(xIsProtocol(llp));
    partInit(&p, 1);
    partPush(p, (void *)peer, sizeof(IPhost));
    return xOpenEnable(self, self, llp, &p);
}

/* 
 * Register this protocol's interest in 'peer' with BIDCTL
 */
static xkern_return_t
chanBidctlUnregister( self, peer )
    XObj	self;
    IPhost	*peer;
{
    XObj	llp;
    Part_s	p;
    
    xAssert(xIsProtocol(self));
    xTrace1(chanp, TR_MAJOR_EVENTS,
	    "chan unregistering interest in peer %s with bidctl",
	    ipHostStr(peer));
    llp = xGetDown(self, CHAN_BIDCTL_I);
    xAssert(xIsProtocol(llp));
    partInit(&p, 1);
    partPush(p, (void *)peer, sizeof(IPhost));
    return xOpenDisable(self, self, llp, &p);
}


static void
disableStub( ev, arg )
    Event	ev;
    void 	*arg;
{
    DisableStubInfo	*dsInfo = (DisableStubInfo *)arg;

    xAssert(dsInfo);
    xTrace1(chanp, TR_MAJOR_EVENTS, "chan disable stub runs for host %s",
	    ipHostStr(&dsInfo->peer));
    chanBidctlUnregister(dsInfo->self, &dsInfo->peer);
    pathFree((char *)dsInfo);
}
#endif /* BOOTID */


/* 
 * Remove the session 's' from the active maps 'keyMap' and 'hostMap'.
 * The session must actually be in the maps.
 */
void
chanRemoveActive( s, keyMap, hostMap )
    XObj	s;
    Map		keyMap, hostMap;
{
    xkern_return_t	res;
    Map			chanMap;
    SState		*ss = (SState *)s->state;
    XObj		oldSessn;

    /* 
     * Remove from active key map
     */
    xAssert(s->binding);
    res = mapRemoveBinding(keyMap, s->binding);
    xAssert( res == XK_SUCCESS );
    s->binding = 0;
    chanMap = chanMapChainFollow(hostMap, &ss->peer, ss->hdr.prot_id);
    xAssert( chanMap );
    xAssert( mapResolve(chanMap, &ss->hdr.chan, &oldSessn) == XK_SUCCESS
	     && oldSessn == s );
    res = mapUnbind(chanMap, &ss->hdr.chan);
    xAssert( res == XK_SUCCESS );
}


/* 
 * Removes the session from the 'fromMap' and binds it in the idleMap
 * series starting at 'toMap', using the peer, prot_id and channel
 * found in the session state.
 */
xkern_return_t
chanAddIdleSessn( s, info )
    XObj	s;
    SessnInfo	*info;
{
    CHAN_STATE		*ss = (CHAN_STATE *)s->state;
    xkern_return_t	xkr;

    /* 
     * We don't need to store the session if it's sequence number is
     * the default.
     */
    if ( ss->hdr.seq > START_SEQ ) {
	xkr = chanMapChainAddObject((void *)ss->hdr.seq, info->idleMap,
				    &ss->peer, ss->hdr.prot_id, ss->hdr.chan,
				    s->path);
	if ( xkr == XK_FAILURE ) {
	    /* 
	     * We've taken great pains to make sure this didn't happen
	     * (by reserving space in the idle maps for new channels).  
	     */
	    xTraceP0(s, TR_ERRORS, "addIdleSession allocation error");
	    return XK_FAILURE;
	}
    }
    chanRemoveActive(s, info->keyMap, info->hostMap);
    chanDestroy(s);
    return XK_SUCCESS;
}


/* 
 * Looks for a dormant session corresponding to the active key,
 * looking in the map chain starting at idleMap.  If
 * such a session is found, the corresponding sequence number is
 * returned.  The sequence number is not unbound from the map.
 * If no dormant session is found, -1 is returned.
 */
static int
getIdleSeqNum( idleMap, key )
    ActiveID	*key;
    Map		idleMap;
{
    Map		map;
    Channel	chan;
    int		seq;

    if ( ! (map = chanGetMap(idleMap, key->lls, key->prot_id)) ) {
	xTrace0(chanp, TR_EVENTS, "chanGetIdleSeqNum -- no map");
	return -1;
    }
    chan = key->chan;
    if ( mapResolve(map, &chan, &seq) == XK_FAILURE ) {
	seq = -1;
    }
    return seq;
}


/* 
 * Restores the appropriate dormant idle session if it exists.
 * The session is removed from the idle map and placed in the active map.
 */
static XObj
getSpecIdleSessn( self, e, key, idleMap, actMap, path )
    XObj	self;
    ActiveID	*key;
    Enable	*e;
    Map		idleMap, actMap;
    Path	path;
{
    Map			map;
    Channel		chan;
    xkern_return_t	res;
    int			seq;

    if ( ! (map = chanGetMap(idleMap, key->lls, key->prot_id)) ) {
	xTrace0(chanp, TR_EVENTS, "chanGetSpecIdleSessn -- no map");
	return ERR_XOBJ;
    }
    chan = key->chan;
    if ( mapResolve(map, &chan, &seq) == XK_SUCCESS ) {
	XObj	s;

	xTrace1(chanp, TR_EVENTS,
		"chanGetSpecIdleSessn -- found stored sequence #%d", seq);
	s = chanSvcOpen(self, e->hlpRcv, e->hlpType, key, seq, FALSE, path);
	if ( s == ERR_XOBJ ) {
	    xTrace1(chanp, TR_ERRORS,
		    "chan getSpecIdleSessn could not reopen chan %d",
		    key->chan);
	    return ERR_XOBJ;
	}
	/* 
	 * Remove this session from the idle map
	 */
	res = mapUnbind(map, &chan);
	xAssert(res == XK_SUCCESS);
	return s;
    } else {
	xTrace0(chanp, TR_EVENTS,
		"chanGetSpecIdleSessn -- found no appropriate session");
	return ERR_XOBJ;
    }
}


int
chanMapRemove( key, val )
    void	*key;
    void	*val;
{
    return MFE_CONTINUE | MFE_REMOVE;
}


/* 
 * Follows the map chain starting at 'm', looking for the map
 * corresponding to the given lls and hlp number.
 */
Map
chanGetMap( m, lls, prot )
    Map		m;
    XObj	lls;
    long	prot;
{
    IPhost	peerHost;

    if ( xControl(lls, GETPEERHOST, (char *)&peerHost,
		  sizeof(IPhost)) < (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS, "chanGetIdleMap could not get peer host");
	return 0;
    }
    return chanMapChainFollow(m, &peerHost, prot);
}


/* 
 * Send a message to the lower session of 's' using 'flags.'  The
 * saved user message will be sent if the 'forceUsrMsg' parameter is
 * non-zero or if the user message will fit in a single packet.
 *
 * The saved user message must be valid.
 *
 * Returns a handle on the outgoing message.
 */
xmsg_handle_t
chanResend( s, flags, forceUsrMsg )
    XObj	s;
    int		flags, forceUsrMsg;
{
    CHAN_HDR		hdr;
    SState		*ss = (SState *)s->state;
    Msg_s		packet;
    xmsg_handle_t	rval;
    Path		path;

    hdr = ss->hdr;
    hdr.flags = flags;
    xAssert( ! msg_isnull(ss->saved_msg));
    xAssert( (flags & ACK_REQUESTED) || forceUsrMsg );
    path = msgGetPath(&ss->saved_msg.m);

    if ( ss->info.reliable && ! forceUsrMsg ) {
	return chanReply(xGetDown(s, 0), &hdr, flags, path);
    }
    if ( ss->info.expensive ) {
	if (! forceUsrMsg) {
	    return chanReply(xGetDown(s, 0), &hdr, flags, path);
        }
	if (ss->info.transport) {
	    if (!xControl(ss->info.transport,CHAN_RETRANSMIT,
			  (char *)&ss->info.ticket, sizeof(xmsg_handle_t))) {
	        return chanReply(xGetDown(s, 0), &hdr, flags, path);
	    }
	}
    }
	        
    if ( ! forceUsrMsg ) {
	int	size;
    
	if ( xControl(s, GETOPTPACKET, (char *)&size, sizeof(int))
	    						< (int)sizeof(int) ) {
	    xTraceP0(s, TR_ERRORS, "resend couldn't GETOPTPKT");
	    size = 0;
	}
	if ( msgLen(&ss->saved_msg.m) > size ) {
	    return chanReply(xGetDown(s, 0), &hdr, flags, path);
	}
    }

    msgConstructCopy(&packet, &ss->saved_msg.m);
    hdr.flags |= USER_MSG;

    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(&hdr);
    } 
    if ( msgPush(&packet, chanHdrStore, &hdr, CHANHLEN, NULL) == XK_SUCCESS ) {
	/*
	 * Send message
	 */
	bzero((char *)&ss->info, sizeof(ss->info));
	msgSetAttr(&packet, 0, (void *)&ss->info, sizeof(ss->info));
	xAssert(xIsSession(xGetDown(s, 0)));
	rval = xPush(xGetDown(s, 0), &packet);
    } else {
	rval = XK_FAILURE;
    }
    msgDestroy(&packet);
    return rval;
}


/* 
 * Run on inactive channels, asks for ACK
 */
void
chanTimeout( ev, arg )
    Event	ev;
    void 	*arg;
{
    XObj		s = (XObj)arg;
    CHAN_STATE		*ss;
    int			flags;
    
    xAssert(xIsSession(s));
    ss = (CHAN_STATE *)s->state; 
    xTrace1(chanp, TIMEOUT_TRACE, "CHAN: %sTimeout: timeout!",
	    ss->hdr.flags & FROM_CLIENT ? "client" : "server");
    if ( ss->hdr.flags & FROM_CLIENT ) {
	flags = ACK_REQUESTED | FROM_CLIENT;
    } else {
	flags = ACK_REQUESTED;
    }
    /* 
     * Check to see if we've successfully sent the message once ...
     */
    switch ( ss->fsmState ) {
      case SVC_EXECUTE:
	 if ( chanResend(s, flags, 1) >= 0 ) {
	     ss->fsmState = SVC_WAIT;
	 }
	 break;
	 
       case CLNT_CALLING:
	 if ( chanResend(s, flags, 1) >= 0 ) {
	     ss->fsmState = CLNT_WAIT;
	 }
	 break;

       default:
	chanResend(s, flags, 0);
    }
    if ( evIsCancelled(ev) ) {
	xTrace0(chanp, TR_EVENTS, "chanTimeout cancelled");
	return;
    }
    ss->wait = MIN(2*ss->wait, ss->maxWait);
    xTrace1(chanp, TIMEOUT_TRACE, "new timeout value: %d", ss->wait);
    evSchedule(ev, chanTimeout, s, ss->wait);
}
