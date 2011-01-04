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
 * select_common.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * select_common.c,v
 * Revision 1.12.1.3.1.2  1994/09/07  04:18:45  menze
 * OSF modifications
 *
 * Revision 1.12.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.12.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.12.1.1  1994/07/26  23:54:51  menze
 * Uses Path-based message interface and UPI
 *
 */


/* 
 * select_common.c contains code common to both the 'select' and the
 * 'mselect' (multi-select) protocols.
 */


#include <xkern/include/xkernel.h>
#include <xkern/protocols/select/select_i.h>

int traceselectp;

static XObjInitFunc	getSessnFuncs;
static long		hdrLoad( void *, char *, long, void * );
static void		hdrStore( void *, char *, long, void * );
static void		phdr( SelHdr * );
static xkern_return_t	selectCall( XObj, Msg, Msg );
static xmsg_handle_t	selectPush( XObj, Msg );
static xkern_return_t	selectClose( XObj );
static XObj		selectCreateSessn( XObj, XObj, XObj, ActiveKey *,
					   Path );
static xkern_return_t	selectPop( XObj, XObj, Msg, void * );
static xkern_return_t	selectCallPop( XObj, XObj, Msg, void *, Msg );

xkern_return_t
selectCommonInit(
    XObj	self)
{
    Part_s 	part;
    PState 	*ps;
    Path	path = self->path;
    
    xTrace0(selectp, TR_GROSS_EVENTS, "SELECT common init");
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	return XK_FAILURE;
    }
    self->state = ps;
    if ( ! (ps->passiveMap = mapCreate(100, sizeof(PassiveKey), path)) ) {
	pathFree(ps);
	return XK_FAILURE;
    }
    if ( ! (ps->activeMap = mapCreate(100, sizeof(ActiveKey), path)) ) {
	mapClose(ps->passiveMap);
	pathFree(ps);
	return XK_FAILURE;
    }
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, 0), &part) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS,"init: could not openenable lower protocol");
    }
    return XK_SUCCESS;
}


static long
hdrLoad(
    void	*hdr,
    char 	*src,
    long  	len,
    void	*arg)
{
    xAssert(len == sizeof(SelHdr));
    bcopy( src, hdr, len );
    ((SelHdr *)hdr)->id = ntohl(((SelHdr *)hdr)->id);
    ((SelHdr *)hdr)->status = ntohl(((SelHdr *)hdr)->status);
    return len;
}


static void
hdrStore(
    void	*hdr,
    char 	*dst,
    long  	len,
    void	*arg)
{
    SelHdr	h;

    xAssert( len == sizeof(SelHdr) );
    h.id = htonl(((SelHdr *)hdr)->id);
    h.status = htonl(((SelHdr *)hdr)->status);
    bcopy( (char *)&h, dst, len );
}


XObj
selectCommonOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part    	p,
    long	hlpNum,
    Path	path)
{
    XObj   	s;
    PState 	*pstate = (PState *)self->state;
    ActiveKey	key;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT common open");
    key.id = hlpNum;
    key.lls = xOpen(self, self, xGetDown(self, 0), p, path);
    if ( key.lls == ERR_XOBJ ) {
	xTrace0(selectp, TR_SOFT_ERRORS, "selectOpen: could not open lls");
	return ERR_XOBJ;
    }
    if ( mapResolve(pstate->activeMap, &key, &s) == XK_FAILURE ) {
	/*
	 * session did not exist -- get an uninitialized one
	 */
	xTrace0(selectp, TR_MAJOR_EVENTS,
		"selectOpen -- no active session existed.  Creating new one.");
	s = selectCreateSessn(self, hlpRcv, hlpType, &key, path);
    } else {
	xTrace1(selectp, TR_MAJOR_EVENTS, "Active sessn %x already exists", s);
	/*
	 * Undo the lls open
	 */
	xClose(key.lls);  
    }
    return s;
}


static XObj
selectCreateSessn(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    ActiveKey	*key,
    Path	path)
{
    XObj 	s;
    SState	*state;
    PState	*pstate = (PState *)self->state;
    SelHdr 	*hdr;
    
    xTrace0(selectp, TR_FUNCTIONAL_TRACE, "selectCreateSessn");
    if ( ! (state = pathAlloc(path, sizeof(SState))) ) {
	return ERR_XOBJ;
    }
    bzero((char *)state, sizeof(SState));
    hdr = &state->hdr;
    hdr->status = SEL_OK;
    hdr->id = key->id;
    s = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 1, &key->lls, path);
    if ( s == ERR_XOBJ ) {
	pathFree(state);
	return ERR_XOBJ;
    }
    if ( mapBind(pstate->activeMap, key, s, &s->binding) != XK_SUCCESS ) {
	xTraceP0(self, TR_SOFT_ERRORS, "createSessn -- bind failed!");
	xClose(key->lls);
	xDestroy(s);
	return ERR_XOBJ;
    }
    s->state = (void *)state;
    return s;
}


xkern_return_t
selectCommonOpenEnable(
    XObj   	self,
    XObj   	hlpRcv,
    XObj   	hlpType,
    Part	p,
    long	hlpNum)
{
    PassiveKey	key;
    PState 	*pstate = (PState *)self->state;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT common open enable");
    key = hlpNum;
    return defaultOpenEnable(pstate->passiveMap, hlpRcv, hlpType, &key);
}


xkern_return_t
selectCommonOpenDisable(
    XObj   	self,
    XObj   	hlpRcv,
    XObj   	hlpType,
    Part    	p,
    long	hlpNum)
{
    PassiveKey	key;
    PState 	*pstate = (PState *)self->state;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT open disable");
    key = hlpNum;
    return defaultOpenDisable(pstate->passiveMap, hlpRcv, hlpType, &key);
}


static xkern_return_t
selectClose(
    XObj	s)
{
    PState		*ps = (PState *)xMyProtl(s)->state;
    xkern_return_t	res;

    xTrace1(selectp, TR_EVENTS, "select_close of session %x", s);
    xAssert(xIsSession(s));
    xAssert(s->rcnt <= 0);
    xClose(xGetDown(s, 0));
    res = mapRemoveBinding(ps->activeMap, s->binding);
    xAssert( res == XK_SUCCESS );
    xDestroy(s);
    return XK_SUCCESS;
}


static xkern_return_t
selectCall(
    XObj	self,
    Msg     	msg,
    Msg     	rMsg)
{
    SState	*state = (SState *)self->state;
    SelHdr 	rHdr;
    
    xTrace0(selectp, TR_EVENTS, "in selectCall");
    xIfTrace(selectp, TR_DETAILED) {
	phdr(&state->hdr);
    }
    msgPush(msg, hdrStore, &state->hdr, sizeof(SelHdr), 0);
    if ( xCall(xGetDown(self, 0), msg, rMsg) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    if ( msgPop(rMsg, hdrLoad, &rHdr, sizeof(SelHdr), 0) == FALSE ) {
	xTrace0(selectp, TR_ERRORS, "selectCall -- msgPop failed");
	return XK_FAILURE;
    }
    if ( rHdr.status == SEL_FAIL ) {
	xTrace0(selectp, TR_ERRORS, "selectCall -- reply indicates failure ");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xmsg_handle_t
selectPush(
    XObj	self,
    Msg     	msg)
{
    SState	*state = (SState *)self->state;
    
    xTrace0(selectp, TR_EVENTS, "in selectPush");
    xIfTrace(selectp, TR_DETAILED) {
	phdr(&state->hdr);
    }
    msgPush(msg, hdrStore, &state->hdr, sizeof(SelHdr), 0);
    return xPush(xGetDown(self, 0), msg);
}


xkern_return_t
selectCallDemux(
    XObj	self,
    XObj	lls,
    Msg		msg,
    Msg		rmsg)
{
    SelHdr 	hdr;
    PassiveKey 	pKey;
    ActiveKey	aKey;
    XObj   	s;
    Enable	*e;
    PState 	*pstate = (PState *)self->state;
    
    xTrace0(selectp, TR_EVENTS , "selectDemux");
    if ( msgPop(msg, hdrLoad, &hdr, sizeof(hdr), 0) == FALSE ) {
	xTrace0(selectp, TR_SOFT_ERRORS, "selectDemux -- msgPop failed!");
	return XK_FAILURE;
    }
    xIfTrace(selectp, TR_DETAILED) {
	phdr(&hdr);
    }
    aKey.id = hdr.id;
    aKey.lls = lls;
    if ( mapResolve(pstate->activeMap, &aKey, &s) == XK_FAILURE ) {
	/*
	 * no active session exists -- check for openenable
	 */
	pKey = aKey.id;
	xTrace1(selectp, TR_EVENTS, "Checking for enable using id: %d", pKey);
	if ( mapResolve(pstate->passiveMap, &pKey, &e) == XK_FAILURE ) {
	    xTrace0(selectp, TR_EVENTS, "selectDemux: no openenable done");
	} else {
	    Path	path = msgGetPath(msg);

	    xTrace0(selectp, TR_MAJOR_EVENTS,
		    "select_demux creates new server session");
	    s = selectCreateSessn(self, e->hlpRcv, e->hlpType, &aKey, path);

	    if ( s != ERR_XOBJ ) {
		xDuplicate(lls);
		xOpenDone(e->hlpRcv, s, self, path);
	    }
	}
    } else {
	xTrace0(selectp, TR_MORE_EVENTS, "selectDemux found existing sessn");
    }
    if ( rmsg ) {
	/* 
	 * This really is a call demux
	 */
	if ( s != ERR_XOBJ && xCallPop(s, lls, msg, 0, rmsg) == XK_SUCCESS ) {
	    hdr.status = SEL_OK;
	} else {
	    xTrace0(selectp, TR_EVENTS,
		    "selectDemux -- sending failure reply");
	    hdr.status = SEL_FAIL;
	}
	msgPush(rmsg, hdrStore, &hdr, sizeof(hdr), 0);
    } else {
	/* 
	 * This is really a demux, not a calldemux
	 */
	xPop(s, lls, msg, 0);
    }
    return XK_SUCCESS;
}


xkern_return_t
selectDemux(
    XObj	s,
    XObj	lls,
    Msg		msg)
{
    xTrace0(selectp, TR_EVENTS, "SELECT demux");
    return selectCallDemux(s, lls, msg, 0);
}


static xkern_return_t
selectCallPop(
    XObj	s,
    XObj	lls,
    Msg     	msg,
    void	*hdr,
    Msg     	rMsg)
{
    xTrace0(selectp, TR_EVENTS, "SELECT callpop");
    return xCallDemux(s, msg, rMsg);
}


static xkern_return_t
selectPop(
    XObj	s,
    XObj	lls,
    Msg     	msg,
    void	*hdr)
{
    xTrace0(selectp, TR_EVENTS, "SELECT_pop");
    return xDemux(s, msg);
}


int
selectControlProtl(
    XObj	self,
    int 	op,
    char	*buf,
    int 	len)
{
    return xControl(xGetDown(self, 0), op, buf, len);
}
  

int
selectCommonControlSessn(
    XObj	self,
    int 	op,
    char	*buf,
    int 	len)
{
    switch ( op ) {
      case GETMAXPACKET:
      case GETOPTPACKET:
	if ( xControl(xGetDown(self, 0), op, buf, len) < sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(SelHdr);
	return (sizeof(int));
	
      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}
  

static void
phdr(
    SelHdr	*h)
{
    xTrace2(selectp, TR_ALWAYS, "SELECT header: id == %d  status == %s",
	    h->id,
	    (h->status == SEL_OK) ? "SEL_OK" :
	    (h->status == SEL_FAIL) ? "SEL_FAIL" :
	    "UNKNOWN");
}


static xkern_return_t
getSessnFuncs(
    XObj	s)
{
    s->call = selectCall;
    s->push = selectPush;
    s->callpop = selectCallPop;
    s->pop = selectPop;
    s->control = selectCommonControlSessn;
    s->close = selectClose;
    return XK_SUCCESS;
}

