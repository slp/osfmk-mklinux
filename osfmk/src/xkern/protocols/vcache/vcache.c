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
 * vcache.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * vcache.c,v
 * Revision 1.22.1.3.1.2  1994/09/07  04:18:48  menze
 * OSF modifications
 *
 * Revision 1.22.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.22.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.22.1.1  1994/07/27  00:17:56  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.22  1994/07/27  00:16:49  menze
 *   [ 1994/07/08          menze ]
 *   Store participants for actively-opened sessions, restoring the
 *   sessions if subsequent active opens use the same participants.
 *   This provides a client/server distinction, so symmetric/asymmetric
 *   options are no longer needed
 *
 * Revision 1.21  1994/01/27  17:10:41  menze
 *   [ 1994/01/13          menze ]
 *   Now uses library routines for rom options
 *
 */

/* 
 * VCACHE -- a session caching protocol.
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/gc.h>
#include <xkern/include/prot/vcache.h>
#include <xkern/protocols/vcache/vcache_i.h>
#include <xkern/include/prot/bidctl.h>

static void		addActiveSessn( PState *, XObj );
static xkern_return_t	addIdleSessn( PState *, XObj );
static xkern_return_t   intervalOpt( XObj, char **, int, int, void * );
static void		protlFuncInit( XObj );
static void		remActiveSessn( PState *, XObj );
static void		remIdleSessn( PState *, XObj );
static xkern_return_t	restoreIdleServer( XObj );
static XObjInitFunc	sessnInit;
static xkern_return_t	vcacheClose( XObj );
static xkern_return_t	vcacheCloseDone( XObj );
static int		vcacheControlProtl( XObj, int, char *, int );
static int		vcacheControlSessn( XObj, int, char *, int );
static void		vcacheDestroy( XObj );
static void		vcacheDestroyIdle( XObj );
static XObj		vcacheCreateSessn( XObj, XObj, XObj, XObj, Path );
static xkern_return_t	vcacheOpenDone( XObj, XObj, XObj, XObj, Path );


static XObjRomOpt	rOpts[] = {
    { "interval", 3, intervalOpt }
};

#if XK_DEBUG
int tracevcachep;
#endif /* XK_DEBUG */

#if XKSTATS
XObj   tracevcacheobj;
void   vcacheCheckMaps( void );
int    vcacheCountMaps( void *, void *, void * );
#endif /* XKSTATS */

static void 
vcachepsFree( PState *ps )
{
    if ( ps->activeMap ) mapClose(ps->activeMap);
    if ( ps->passiveMap ) mapClose(ps->passiveMap);
    if ( ps->idleMap ) mapClose(ps->idleMap);
    if ( ps->idlePartMap ) mapClose(ps->idlePartMap);
    pathFree(ps);
}


xkern_return_t
vcache_init( self )
    XObj 	self;
{
    PState	*ps;
    int		collectInterval;
    Path	path = self->path;
    
    xTrace0(vcachep, TR_GROSS_EVENTS, "VCACHE init");
    xAssert(xIsProtocol(self));
#if XKSTATS
    tracevcacheobj = self;
#endif /* XKSTATS */
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTraceP0(self, TR_ERRORS, "protocol down vector is misconfigured");
	return XK_FAILURE;
    }
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    self->state = ps;
    ps->activeMap = mapCreate(VCACHE_ALL_SESSN_MAP_SZ, sizeof(XObj), path);
    ps->idleMap = mapCreate(VCACHE_ALL_SESSN_MAP_SZ, sizeof(XObj), path);
    ps->idlePartMap = mapCreate(VCACHE_HLP_MAP_SZ, -1, path);
    ps->passiveMap = mapCreate(VCACHE_HLP_MAP_SZ, sizeof(XObj), path);
    if ( ! ps->activeMap || ! ps->idleMap || ! ps->idlePartMap ||
	 ! ps->passiveMap ) {
	vcachepsFree(ps);
	return XK_NO_MEMORY;
    }
    collectInterval = VCACHE_COLLECT_INTERVAL;
    findXObjRomOpts(self, rOpts, sizeof(rOpts)/sizeof(XObjRomOpt),
		    &collectInterval);
    xTrace1(vcachep, TR_EVENTS, "VCACHE uses collect interval of %d seconds",
	    collectInterval );
    protlFuncInit(self);
    if ( ! (initSessionCollector(ps->idleMap, collectInterval * 1000 * 1000, 
				 vcacheDestroyIdle, path, "vcache")) ) {
	vcachepsFree(ps);
	return XK_NO_MEMORY;
    }
    return XK_SUCCESS;
}


static xkern_return_t
intervalOpt( self, str, nFields, line, arg )
    XObj	self;
    char	**str;
    int		nFields, line;
    void	*arg;
{
    int	val;

    val = atoi(str[2]);
    if ( val < 0 ) return XK_FAILURE;
    *(int *)arg = val;
    xTraceP1(self, TR_EVENTS, "collect interval %d (romfile)",
	     *(int *)arg);
    return XK_SUCCESS;
}



static void
recordParticipants( XObj self, void *buf, int len, boolean_t mustclone )
{
    SState	*ss = (SState *)self->state;

    if ( ss->partBuf ) {
	pathFree(ss->partBuf);
    }
    if ( mustclone ) {
	if ( (ss->partBuf = pathAlloc(self->path, len)) == 0 ) 
	    /*
	     * Too bad. I can't save myself the work for the next time around.
	     */
	    return;
	memcpy((char *)ss->partBuf, (const char *)buf, len);
	ss->partBufLen = len;
	return;
    }
    ss->partBuf = buf;
    ss->partBufLen = len;
}


static xkern_return_t
buildPartKey( Part p, void **buf, int *len, Path path )
{
    static int partBufSize = PART_BUF_SZ;

    while (1) {
	if ( (*buf = pathAlloc(path, partBufSize)) == 0 ) {
	    return XK_NO_MEMORY;
	}
	*len = partBufSize;
	if ( partExternalize( p, *buf, len) == XK_SUCCESS ) {
	    return XK_SUCCESS;
	}
	partBufSize *= 2;
    }
}



static XObj
vcacheOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p,
    Path	path)
{
    XObj	s, lls;
    PState	*ps = (PState *)self->state;
    SState	*ss;
    void	*partBuf;
    int		partBufLen;
    dlist_t	list;
    char        partCache[PART_CACHE_SIZE];
    boolean_t   partCacheHit = TRUE;  
    
    xTrace0(vcachep, TR_MAJOR_EVENTS, "VCACHE open");

    /*
     * I call partExternalize first. Should I fail
     * (partCache[] isn't big enough), I'll fall back to
     * the slow path (buildPartKey will work with
     * any configuration of participant stacks).
     */
    partBufLen = PART_CACHE_SIZE;
    partBuf = (void *)&partCache[0];
    if ( partExternalize(p, partBuf, &partBufLen) != XK_SUCCESS ) {
	if ( buildPartKey(p, &partBuf, &partBufLen, path) != XK_SUCCESS ) {
	    return ERR_XOBJ;
	} else
	    partCacheHit = FALSE;
    } 
	
    /* 
     * See if there's anything in the idle maps that was previously
     * xOpen'ed with these participants.  
     */
    if ( (mapVarResolve(ps->idlePartMap, partBuf, partBufLen, (void **)&list)
	  	== XK_SUCCESS)
	 && (ss = (SState *)dlist_first(list)) != 0 ) {
	s = ss->self;
	xTraceP1(self, TR_EVENTS,
		 "Found idle session (%x) w/ matching participants", (long)s);
	xSetUp(s, hlpRcv);
	remIdleSessn(ps, s);
	addActiveSessn(ps, s);
	if (!partCacheHit)
	    pathFree(partBuf);
	return s;
    }
    lls = xOpen(self, hlpType, xGetDown(self, 0), p, path);
    if ( lls == ERR_XOBJ ) {
	xTrace0(vcachep, TR_ERRORS, "vcacheOpen: could not open lower sessn");
	if (!partCacheHit)
	    pathFree(partBuf);
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &lls, &s) == XK_SUCCESS ) {
	xTraceP1(self, TR_EVENTS, "returning existing active session (%x)",
		 (long)s);
	xClose(lls);
	if (!partCacheHit)
	    pathFree(partBuf);
	return s;
    }
    if ( mapResolve(ps->idleMap, &lls, &s) == XK_SUCCESS ) {
	xTraceP1(self, TR_EVENTS, "restoring idle session (%x)", (long)s);
	xClose(lls);
	xSetUp(s, hlpRcv);
	remIdleSessn(ps, s);
	addActiveSessn(ps, s);
	recordParticipants(s, partBuf, partBufLen, partCacheHit);
	if (!partCacheHit)
	    pathFree(partBuf);
	return s;
    }
    s = vcacheCreateSessn(self, hlpRcv, hlpType, lls, path);
    if ( s == ERR_XOBJ ) {
	xClose(lls);
	if (!partCacheHit)
	    pathFree(partBuf);
	return ERR_XOBJ;
    }
    recordParticipants(s, partBuf, partBufLen, partCacheHit);
    addActiveSessn(ps, s);
    xTraceP1(self, TR_EVENTS, "returning new session (%x)", (long)s);
    if (!partCacheHit)
	pathFree(partBuf);
    return s;
}


static XObj
vcacheCreateSessn( self, hlpRcv, hlpType, lls, path )
    XObj 	self, hlpRcv, hlpType, lls;
    Path	path;
{
    XObj	s;
    SState	*ss;
    IPhost	peer;
    PState	*ps = (PState *)self->state;
    
    if ( xControl(lls, GETPEERHOST, (char *)&peer, sizeof(IPhost))
				< (int)sizeof(IPhost) ) {
	xTrace0(vcachep, TR_ERRORS,
		"vcacheCreateSessn could not get peer host");
	return ERR_XOBJ;
    }
    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	return ERR_XOBJ;
    }
    if ( mapReserve(ps->activeMap, 1) != XK_SUCCESS ) {
	pathFree(ss);
	return ERR_XOBJ;
    }
    if ( (s = xCreateSessn(sessnInit, hlpRcv, hlpType, self, 1, &lls, path))
			== ERR_XOBJ ) {
	pathFree(ss);
	mapReserve(ps->activeMap, -1);
	xTrace0(vcachep, TR_ERRORS, "create sessn fails in vcacheOpen");
	return ERR_XOBJ;
    }
    bzero((char *)ss, sizeof(SState));
    s->state = (void *)ss;
    ss->self = s;
    ss->remote_host = peer;
    ss->partBuf = 0;
    ss->partBufLen = 0;
    /*
     * The lower sessions' up field is made to point to this
     * vcache session (not the protocol)
     */
    xSetUp(lls, s);
    return s;
}

static int
vcacheControlSessn( s, opcode, buf, len )
    XObj 	s;
    int 	opcode, len;
    char 	*buf;
{
    xTrace0(vcachep, TR_EVENTS, "VCACHE controlsessn");
    /*
     * All opcodes are forwarded to the lower session.  
     */
  if (opcode == GETPEERHOST && len >= sizeof(IPhost) && s->state)
    {
      bcopy((char *)&((SState *)s->state)->remote_host,
	    buf, sizeof(IPhost));
      return sizeof(IPhost);
    }
    return xControl(xGetDown(s, 0), opcode, buf, len);
}


static int
vcacheControlProtl( self, opcode, buf, len )
    XObj 	self;
    int 	opcode, len;
    char 	*buf;
{
  return xControl(xGetDown(self, 0), opcode, buf, len);
}


static xkern_return_t
vcacheOpenEnable(
    XObj 	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p)
{
    PState	*ps = (PState *)self->state;
    
    xTrace0(vcachep, TR_MAJOR_EVENTS, "VCACHE open enable");
    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    self->down, p);
}


static xkern_return_t
vcacheOpenDisable(
    XObj 	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p)
{
    PState	*ps = (PState *)self->state;

    return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
				     self->down, p);
}


static xkern_return_t
vcacheClose( s )
    XObj	s;
{
    PState 		*ps = (PState *)xMyProtl(s)->state;

    remActiveSessn(ps, s);
    xSetUp(s, 0);
    xTrace1(vcachep, TR_EVENTS, "VCACHE caching session %x", s);
    addIdleSessn(ps, s);
    return XK_SUCCESS;
}


static void
vcacheDestroyIdle( s )
    XObj	s;
{
    xAssert(s->rcnt == 0);
    remIdleSessn((PState *)xMyProtl(s)->state, s);
    vcacheDestroy(s);
}


static void
vcacheDestroy( s )
    XObj 	s;
{
    SState		*ss = (SState *)s->state;
    PState		*ps = (PState *)xMyProtl(s)->state;
    XObj		lls;
    dlist_t		dlist;
    xkern_return_t	xkr;
    
    xTrace1(vcachep, TR_MAJOR_EVENTS, "VCACHE destroy of session %x", s);
    lls = xGetDown(s, 0);
    xAssert(xIsSession(lls));
    if ( ss->partBuf ) {
	if ( mapVarResolve(ps->idlePartMap, ss->partBuf, ss->partBufLen,
			   (void **)&dlist) == XK_SUCCESS ) {
	    if ( dlist_empty(dlist) ) {
		xkr = mapVarUnbind(ps->idlePartMap, ss->partBuf, ss->partBufLen);
		xAssert( xkr == XK_SUCCESS );
		pathFree((char *)dlist);
	    }
	}
	pathFree(ss->partBuf);
    }
    mapReserve(ps->activeMap, -1);
    xSetUp(lls, xMyProtl(s));
    xClose(lls);
    xDestroy(s);
}


static xkern_return_t
vcacheCall(
    XObj	s,
    Msg 	msg,
    Msg		rmsg)
{
    xTrace0(vcachep, TR_EVENTS, "vcacheCall");
    return xCall(xGetDown(s, 0), msg, rmsg);
}


static xmsg_handle_t
vcachePush(
    XObj	s,
    Msg 	msg)
{
    xTrace0(vcachep, TR_EVENTS, "vcachePush");
    return xPush(xGetDown(s, 0), msg);
}


static xkern_return_t
vcacheOpenDone( self, lls, llp, hlpType, path )
    XObj	self, lls, llp, hlpType;
    Path	path;
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    Enable	*e;
    
    xTrace0(vcachep, TR_MAJOR_EVENTS, "In VCACHE openDone");
    if ( self == hlpType ) {
	xTrace0(vcachep, TR_ERRORS, "self == hlpType in vcacheOpenDone");
	return XK_FAILURE;
    }
    /*
     * check for openEnable
     */
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTrace0(vcachep, TR_ERRORS,
		"vcacheOpenDone: Couldn't find hlp for incoming session");
	return XK_FAILURE;
    }
    if ( (s = vcacheCreateSessn(self, e->hlpRcv, e->hlpType, lls, path))
							== ERR_XOBJ ) {
	return XK_FAILURE;
    }
    xDuplicate(lls);
    if ( addIdleSessn(ps, s) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    xTrace0(vcachep, TR_EVENTS,
	    "vcache Passively opened session successfully created");
    return xOpenDone(e->hlpRcv, s, self, path);
}


static xkern_return_t
vcacheProtlCallDemux(
    XObj	self,
    XObj	lls,
    Msg		m,
    Msg		rmsg)
{
    xTrace0(vcachep, TR_ERRORS, "vcacheProtlCallDemux called!!");
    return XK_SUCCESS;
}


static xkern_return_t
vcacheProtlDemux(
    XObj	self,
    XObj	lls,
    Msg		m)
{
    xTrace0(vcachep, TR_ERRORS, "vcacheProtlDemux called!!");
    return XK_SUCCESS;
}


static xkern_return_t
restoreIdleServer( s )
    XObj	s;
{
    Enable	*e;
    XObj	hlpType;
    PState	*ps;

    xTrace1(vcachep, TR_EVENTS, "VCACHE restoring session %x", s);
    ps = (PState *)xMyProtl(s)->state;
    hlpType = xHlpType(s);
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {	
	xTrace0(vcachep, TR_EVENTS, "vcachePop -- no openEnable!");	
	return XK_FAILURE;
    }								
    remIdleSessn(ps, s);
    addActiveSessn(ps, s);
    xOpenDone(e->hlpRcv, s, xMyProtl(s), xMyPath(s));
    return XK_SUCCESS;
}


/* 
 * vcachePop and vcacheSessnDemux must be used (i.e., they can't be
 * bypassed) for the UPI reference count mechanism to work properly. 
 */
static xkern_return_t
vcacheCallPop(
    XObj 	s,
    XObj	lls,
    Msg 	msg,
    void	*h,
    Msg		rmsg)
{
    xTrace0(vcachep, TR_EVENTS, "vcache callPop");
    if ( s->rcnt == 1 ) {
	if ( restoreIdleServer(s) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    return xCallDemux(s, msg, rmsg);
}


static xkern_return_t
vcacheSessnCallDemux(
    XObj 	self,
    XObj	lls,
    Msg 	msg,
    Msg		rmsg)
{
    xTrace0(vcachep, TR_EVENTS, "vcache Session callDemux");
    return xCallPop(self, lls, msg, 0, rmsg);
}


static xkern_return_t
vcachePop(
    XObj 	s,
    XObj	lls,
    Msg 	msg,
    void	*h)
{
    xTrace0(vcachep, TR_EVENTS, "vcache Pop");
    if ( s->rcnt == 1 ) {
	if ( restoreIdleServer(s) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    return xDemux(s, msg);
}


static xkern_return_t
vcacheSessnDemux(
    XObj 	self,
    XObj	lls,
    Msg 	msg)
{
    xTrace0(vcachep, TR_EVENTS, "vcache Session Demux");
    return xPop(self, lls, msg, 0);
}


static xkern_return_t
vcacheCloseDone( lls )
    XObj	lls;
{
    XObj	s;	/* vcache session */

    s = xGetUp(lls);
    xAssert(xIsSession(s));
    if ( xGetUp(s) ) {
	return xCloseDone(s);
    } else {
	return XK_SUCCESS;
    }
}


static xkern_return_t
sessnInit( s )
    XObj 	s;
{
    xAssert(xIsSession(s));
    
    s->call = vcacheCall;
    s->push = vcachePush;
    s->callpop = vcacheCallPop;
    s->pop = vcachePop;
    s->close = vcacheClose;
    s->control = vcacheControlSessn;
    /* 
     * VCACHE sessions will look like a protocol to lower sessions, so we
     * need a demux function
     */
    s->calldemux = vcacheSessnCallDemux;
    s->demux = vcacheSessnDemux;
    s->closedone = vcacheCloseDone;
    return XK_SUCCESS;
}


static void
protlFuncInit(p)
    XObj p;
{
    xAssert(xIsProtocol(p));

    p->control = vcacheControlProtl;
    p->open = vcacheOpen;
    p->openenable = vcacheOpenEnable;
    p->opendisable = vcacheOpenDisable;
    p->calldemux = vcacheProtlCallDemux;
    p->demux = vcacheProtlDemux;
    p->opendone = vcacheOpenDone;
}


/* 
 * Add the session s to the active map
 *
 *    session can be recovered from its lower sesson;
 *    remote host IP address is also recorded
 */
static void
addActiveSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    XObj lls;
    xkern_return_t xkr;

    xTraceP1(s, TR_EVENTS, "activating session %lx", (long)s);
    lls = xGetDown(s, 0);
    if ( ! xIsSession(lls) ) {
	xError("VCACHE addSession -- bogus lls!");
	xAssert(0);
	return;
    }
    xAssert( mapResolve(ps->activeMap, &lls, 0) == XK_FAILURE );
    xkr = mapBind(ps->activeMap, &lls, s, &s->binding);
    xAssert( xkr == XK_SUCCESS );
}



/* 
 * Add the session s to the idle map.  If the session can't be added
 * to the map it is immediately collected.  
 */
static xkern_return_t
addIdleSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    XObj	lls;
    SState	*ss = (SState *)s->state;

    lls = xGetDown(s, 0);
    if ( ! xIsSession(lls) ) {
	xError("VCACHE addIdleSession -- bogus lls!");
	vcacheDestroy(s);
	return XK_FAILURE;
    }
    /* 
     * Reset the session's garbage-collection 'idle' flag so it isn't
     * immediately collected. 
     */
    s->idle = FALSE;
    xAssert( mapResolve(ps->idleMap, &lls, 0) == XK_FAILURE );
    if ( mapBind(ps->idleMap, &lls, s, &s->binding) != XK_SUCCESS ) {
	vcacheDestroy(s);
	return XK_FAILURE;
    }
    if ( ss->partBuf ) {
	/* 
	 * Add this session to the idlePart map
	 */
	dlist_t	dlist;

	if ( mapVarResolve(ps->idlePartMap, ss->partBuf, ss->partBufLen,
			   (void **)&dlist) == XK_FAILURE ) {
	    if ( ! (dlist = pathAlloc(s->path, sizeof(struct dlist))) ) {
		mapRemoveBinding(ps->idleMap, s->binding);
		vcacheDestroy(s);
		return XK_FAILURE;
	    }
	    dlist_init(dlist);
	    if ( mapVarBind(ps->idlePartMap, ss->partBuf, ss->partBufLen,
			    dlist, 0) != XK_SUCCESS ) {
		pathFree(dlist);
		mapRemoveBinding(ps->idleMap, s->binding);
		vcacheDestroy(s);
		return XK_FAILURE;
	    }
	}
	dlist_insert(ss, dlist);
    }
    return XK_SUCCESS;
}


static void
remIdleSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    xkern_return_t	res;
    SState		*ss = (SState *)s->state;

    res = mapRemoveBinding(ps->idleMap, s->binding);
    xAssert( res == XK_SUCCESS );
    if ( ss->partBuf ) {
	dlist_remove(ss);
    }
}


static void
remActiveSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    xkern_return_t	res;

    res = mapRemoveBinding(ps->activeMap, s->binding);
    xAssert( res == XK_SUCCESS );
}

#if XKSTATS

int
vcacheCountMaps(
		void *key, 
		void *value,
		void *arg)
{
    int *count = (int *)arg;

    printf("%x key %x value\n", key, value);
    (*count)++;

    return MFE_CONTINUE;
}

void   
vcacheCheckMaps(void)
{
    PState	*ps = (PState *)tracevcacheobj->state;
    int         count = 0;
    
    if ( ps->activeMap ) 
	mapForEach(ps->activeMap, vcacheCountMaps, (void *)&count );
    printf("*** activeMap: %d entries\n", count);
    count = 0;
    if ( ps->passiveMap ) 
	mapForEach(ps->passiveMap, vcacheCountMaps, (void *)&count );
    printf("*** passiveMap: %d entries\n", count);
    count = 0;
    if ( ps->idleMap ) 
	mapForEach(ps->idleMap, vcacheCountMaps, (void *)&count );
    printf("*** idleMap: %d entries\n", count);
    count = 0;
    if ( ps->idlePartMap ) 
	mapForEach(ps->idlePartMap, vcacheCountMaps, (void *)&count );
    printf("*** idlePartMap: %d entries\n", count);
}

#endif /* XKSTATS */

