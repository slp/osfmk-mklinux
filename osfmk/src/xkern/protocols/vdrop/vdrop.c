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
 * vdrop.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * 1.12
 * 1994/04/22 04:22:24
 */


#include <xkern/include/xkernel.h>
#include <xkern/include/prot/vdrop.h>
#include <xkern/protocols/vdrop/vdrop_i.h>


static void		protlFuncInit( XObj );
static xkern_return_t	sessnInit( XObj );
static XObj		vdropCreateSessn( XObj, XObj, XObj, XObj, Path );
static xkern_return_t	vdropOpenDone( XObj, XObj, XObj, XObj, Path );
static int		vdropControlProtl( XObj, int, char *, int );
static int		vdropControlSessn( XObj, int, char *, int );
static xkern_return_t	vdropClose( XObj );


int tracevdropp;


static xkern_return_t
readInt( 
	XObj	self,
	char	**str,
	int	nFields,
	int	line,
	void	*arg)
{
    *(int *)arg = atoi(str[2]);
    return XK_SUCCESS;
} /* readInt */


static XObjRomOpt vdropOpt[] = {
    { "interval", 3, readInt }
};


xkern_return_t
vdrop_init( self )
    XObj 	self;
{
    PState	*ps;
    Path	path = self->path;
    
    xTraceP0(self, TR_GROSS_EVENTS, "init");
    xAssert(xIsProtocol(self));
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xError("VDROP down vector is misconfigured");
	return XK_FAILURE;
    }
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    self->state = ps;
    protlFuncInit(self);
    ps->activeMap = mapCreate(VDROP_ACT_MAP_SZ, sizeof(XObj), path);
    ps->passiveMap = mapCreate(VDROP_PAS_MAP_SZ, sizeof(XObj), path);
    if ( ! ps->activeMap || ! ps->passiveMap ) {
	if ( ps->activeMap ) mapClose(ps->activeMap);
	if ( ps->passiveMap ) mapClose(ps->passiveMap);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static XObj
vdropOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p,
    Path	path)
{
    XObj	s, lls;
    PState	*ps = (PState *)self->state;
    
    xTraceP0(self, TR_MAJOR_EVENTS, "open");
    lls = xOpen(self, hlpType, xGetDown(self, 0), p, path);
    if ( lls == ERR_XOBJ ) {
	xTraceP0(self, TR_ERRORS, "open: could not open lower session");
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &lls, &s) == XK_SUCCESS ) {
	xTraceP0(self, TR_MORE_EVENTS, "open -- found an existing one");
	xClose(lls);
    } else {
	xTraceP0(self, TR_MAJOR_EVENTS, "open -- creating a new one");
	s = vdropCreateSessn(self, hlpRcv, hlpType, lls, path);
	/* 
	 * s may be ERR_XOBJ here ...
	 */
    }
    return s;
}


static XObj
vdropCreateSessn( self, hlpRcv, hlpType, lls, path )
    XObj 	self, hlpRcv, hlpType, lls;
    Path	path;
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    SState	*ss;
    XTime	t;
    
    
    if ( (s = xCreateSessn(sessnInit, hlpRcv, hlpType, self, 1, &lls, path))
			== ERR_XOBJ ) {
	xTraceP0(self, TR_ERRORS, "xCreateSessn fails");
	return ERR_XOBJ;
    }
    if ( mapBind(ps->activeMap, &lls, s, &s->binding) != XK_SUCCESS ) {
	xTraceP0(self, TR_SOFT_ERRORS, "mapBind fails in createSessn");
	xDestroy(s);
	xClose(lls);
	return ERR_XOBJ;
    }
    /*
     * The lower sessions' up field is made to point to this
     * vdrop session (not the protocol)
     */
    xSetUp(lls, s);
    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	vdropClose(s);
	return ERR_XOBJ;
    }
    s->state = ss;
    ss->interval = -1;
    findXObjRomOpts(self, vdropOpt, sizeof(vdropOpt)/sizeof(vdropOpt[0]),
		    &ss->interval);
    if (ss->interval <= 0) {
	xGetTime(&t);
	ss->interval = ((t.usec/100) % (VDROP_MAX_INTERVAL - 1) + 2);
    } /* if */
    xTraceP1(self, TR_MAJOR_EVENTS, "dropping once every %d pckts",
	     ss->interval);
    ss->count = 0;
    xTraceP1(self, TR_MAJOR_EVENTS, "open returns %x", s);
    return s;
}


static int
vdropControlSessn( s, opcode, buf, len )
    XObj 	s;
    int 	opcode, len;
    char 	*buf;
{
    SState	*ss = (SState *)s->state;

    xTraceP0(s, TR_EVENTS, "controlsessn");
    switch ( opcode ) {

      case VDROP_SETINTERVAL:
	ss->interval = *(int *)buf;
	xTraceP1(s, TR_EVENTS,
		"controlsessn resets drop interval to %d",
		ss->interval);
	return 0;

      case VDROP_GETINTERVAL:
	*(int *)buf = ss->interval;
	return sizeof(int);

      default:
	/*
	 * All other opcodes are forwarded to the lower session.  
	 */
	return xControl(xGetDown(s, 0), opcode, buf, len);
    }
}


static int
vdropControlProtl( self, opcode, buf, len )
    XObj 	self;
    int 	opcode, len;
    char 	*buf;
{
    /*
     * All opcodes are forwarded to the lower protocol
     */
    return xControl(xGetDown(self, 0), opcode, buf, len);
}


static xkern_return_t
vdropOpenEnable(
    XObj 	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP0(self, TR_MAJOR_EVENTS, "open enable");
    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    self->down, p);
}


static xkern_return_t
vdropOpenDisable(
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
vdropClose( s )
    XObj 	s;
{
    PState		*ps;
    xkern_return_t	res;
    XObj		lls;
    
    xTraceP1(s, TR_MAJOR_EVENTS, "close of session %lx", (u_long)s);
    xAssert(s->rcnt == 0);
    ps = (PState *)xMyProtl(s)->state;
    res = mapRemoveBinding(ps->activeMap, s->binding);
    xAssert( res != XK_FAILURE );
    lls = xGetDown(s, 0);
    xAssert(xIsSession(lls));
    xSetUp(lls, xMyProtl(s));
    xClose(lls);
    xDestroy(s);
    return XK_SUCCESS;
}


static xmsg_handle_t
vdropPush(
    XObj	s,
    Msg 	msg)
{
    xTraceP0(s, TR_EVENTS, "push");
    return xPush(xGetDown(s, 0), msg);
}


static xkern_return_t
vdropOpenDone( self, lls, llp, hlpType, path )
    XObj	self, lls, llp, hlpType;
    Path	path;
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    Enable	*e;
    
    xTraceP0(self, TR_MAJOR_EVENTS, "In openDone");
    if ( self == hlpType ) {
	xTraceP0(self, TR_ERRORS, "self == hlpType in openDone");
	return XK_FAILURE;
    }
    /*
     * check for openEnable
     */
    
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTraceP0(self, TR_ERRORS,
		"openDone: Couldn't find hlp for incoming session");
	return XK_FAILURE;
    }
    xDuplicate(lls);
    if ( (s = vdropCreateSessn(self, e->hlpRcv, e->hlpType, lls, path))
							== ERR_XOBJ ) {
	return XK_FAILURE;
    }
    xTraceP0(self, TR_EVENTS,
	    "passively opened session successfully created");
    return xOpenDone(e->hlpRcv, s, self, path);
}


static xkern_return_t
vdropProtlDemux(
    XObj	self,
    XObj	lls,
    Msg		m)
{
    xTraceP0(self, TR_ERRORS, "protlDemux called!!");
    return XK_SUCCESS;
}


/* 
 * vdropPop and vdropSessnDemux must be used (i.e., they can't be
 * bypassed) for the UPI reference count mechanism to work properly. 
 */
static xkern_return_t
vdropPop(
    XObj 	s,
    XObj	lls,
    Msg 	msg,
    void	*h)
{
    SState	*ss = (SState *)s->state;

    xTraceP0(s, TR_EVENTS, "pop");
    if ((ss->interval) && ( ++ss->count >= ss->interval )) {
	xTraceP0(s, TR_MAJOR_EVENTS, "pop is dropping msg");
	ss->count = 0;
	return XK_SUCCESS;
    } else {
	return xDemux(s, msg);
    }
}


static xkern_return_t
vdropSessnDemux(
    XObj 	self,
    XObj	lls,
    Msg 	msg)
{
    xTraceP0(self, TR_EVENTS, "session demux");
    return xPop(self, lls, msg, 0);
}


static xkern_return_t
sessnInit( s )
    XObj 	s;
{
    xAssert(xIsSession(s));
    
    s->push = vdropPush;
    s->pop = vdropPop;
    s->close = vdropClose;
    s->control = vdropControlSessn;
    /* 
     * VDROP sessions will look like a protocol to lower sessions, so we
     * need a demux function
     */
    s->demux = vdropSessnDemux;
    return XK_SUCCESS;
}


static void
protlFuncInit(p)
    XObj p;
{
    xAssert(xIsProtocol(p));

    p->control = vdropControlProtl;
    p->open = vdropOpen;
    p->openenable = vdropOpenEnable;
    p->opendisable = vdropOpenDisable;
    p->demux = vdropProtlDemux;
    p->opendone = vdropOpenDone;
}
