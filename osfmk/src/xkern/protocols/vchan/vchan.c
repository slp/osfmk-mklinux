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
 * vchan.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * vchan.c,v
 * Revision 1.12.1.3.1.2  1994/09/07  04:18:46  menze
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
 * Revision 1.12.1.1  1994/07/27  00:30:35  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.12  1993/12/13  23:41:12  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 *   [ 93/09/21          nahum ]
 *   Got rid of cast in mapResolve call
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip_host.h>
#include <xkern/include/prot/vchan.h>
#include <xkern/protocols/vchan/vchan_i.h>


int             tracevchanp;

static	IPhost	defaultHost = { 0, 0, 0, 0 };

static void 		addSessn( XObj, XObj );
static int 		decreaseConcurrency( XObj, int );
static void 		dispStack( SessnStack * );
static void		getProcProtl( XObj );
static XObjInitFunc 	getProcSessn;
static XObj 		getSessn( XObj );
static int 		increaseConcurrency( XObj, int );
static xkern_return_t 	vchanCloseSessn( XObj );
static int		vchanControlProtl( XObj, int, char *, int );
static int		vchanControlSessn( XObj, int, char *, int );

/*
 * init -- Initialize protocol state
 */
xkern_return_t
vchan_init(self)		
    XObj            self;
{
    PState      *pstate;
    Path	path = self->path;
    
    xTrace0(vchanp, TR_GROSS_EVENTS, "VCHAN init");
    xAssert(xIsProtocol(self));
    
    if ( ! (pstate = pathAlloc(path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }	
    self->state = pstate;
    pstate->activeMap = mapCreate(ACTIVE_MAP_SIZE, sizeof(ActiveKey), path);
    if ( ! pstate->activeMap ) {
	pathFree(pstate);
	return XK_NO_MEMORY;
    }
    getProcProtl(self);
    return XK_SUCCESS;
}


static int
buildKey(
    ActiveKey	*key,
    Part_s	*p,
    XObj	hlp)
{
    IPhost	*h;

    if ( ! p || partLen(p) < 1 ) {
	xTrace0(vchanp, TR_SOFT_ERRORS, "VCHAN -- bad participant in open");
	return -1;
    }
    /* 
     * Find remote host
     */
    if ( (h = (IPhost *)partPop(p[0])) == 0 ) {
	xTrace0(vchanp, TR_SOFT_ERRORS, "vchanOpen -- no remote host");
	return -1;
    }
    xTrace1(vchanp, TR_MORE_EVENTS, "vchanOpen -- remote host == %s",
	    ipHostStr(h));
    key->rHost = *h;
    partPush(p[0], h, sizeof(IPhost));
    /* 
     * Find local host (if specified)
     */
    h = 0;
    if ( partLen(p) < 2 ||
	 (h = (IPhost *)partPop(p[1])) == (IPhost *)ANY_HOST ) {
	xTrace0(vchanp, TR_MORE_EVENTS,
		"vchanOpen -- no local host, using default");
	key->lHost = defaultHost;
    } else {
	if ( h == 0 ) {
	    xTrace0(vchanp, TR_SOFT_ERRORS,
		    "vchanOpen -- local part, but no local host");
	    return -1;
	}
	xTrace1(vchanp, TR_MORE_EVENTS,
		"vchanOpen -- local host is %s", ipHostStr(h));
	key->lHost = *h;
    }
    if ( h ) {
	partPush(p[1], h, sizeof(IPhost));
    }
    key->hlpType = hlp;
    return 0;
}



/*
 * open -- create a vchan session (on the client side)
 */
static XObj
vchanOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p,
    Path	path)
{
    ActiveKey 	key;  
    XObj 	s, lls;
    PState 	*pstate = (PState *)self->state;
    SState 	*sstate;
    SessnStack	*stack;
    int		i, pLen;
    
    xTrace0(vchanp, TR_MAJOR_EVENTS, "VCHAN open");
    if ( buildKey(&key, p, hlpType) ) {
	return ERR_XOBJ;
    }
    /*
     * Check for already existing session
     */
    if ( mapResolve(pstate->activeMap, &key, &s) == XK_SUCCESS ) {
	return s;
    }
    xTrace0(vchanp, TR_MAJOR_EVENTS, "vchan creating new session");
    /* 
     * The hlp here doesn't really mean anything since the VCHAN
     * session will never be used in a server context
     */
    if ( (s = xCreateSessn(getProcSessn, hlpRcv, hlpType, self, 0, 0, path))
			== ERR_XOBJ ) {
	xTrace0(vchanp, TR_ERRORS,
		"vchan_open could not create vchan session");
	return ERR_XOBJ;
    }
    xTrace1(vchanp, TR_MORE_EVENTS, "Created new vchan session %x", s);
    /*
     * create state for new vchan session
     */
    if ( ! (sstate = pathAlloc(path, sizeof(SState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	vchanCloseSessn(s);
	return ERR_XOBJ;
    }
    bzero((char *)sstate, sizeof(SState));
    s->state = sstate;
    sstate->hlpType = hlpType;
    stack = &sstate->stack;
    stack->s = pathAlloc(path, sizeof(XObj) * sizeof(DEFAULT_NUM_SESSNS));
    if ( ! stack->s ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	vchanCloseSessn(s);
	return ERR_XOBJ;
    }
    stack->size = DEFAULT_NUM_SESSNS;
    stack->top = stack->bottom = 0;
    semInit(&stack->available, 0);
    /* 
     * Open initial lower sessions with which to initialize the stack
     */
    pLen = partLen(p) * sizeof(Part_s);
    for ( i=0; i < DEFAULT_NUM_SESSNS; i++ ) {
	Part_s	tmpPart[2];
	bcopy((char *)p, (char *)tmpPart, pLen);
	lls = xOpen(self, hlpType, xGetDown(self, 0), tmpPart, path);
	if ( lls == ERR_XOBJ ) {
	    xTrace0(vchanp, TR_ERRORS, "vchanOpen could not open lls");
	    stack->size = i;
	    vchanCloseSessn(s);
	    return ERR_XOBJ;
	}
	addSessn(s, lls);
    }
    /*
     * Install new session in the active map
     */
    xTrace0(vchanp, TR_EVENTS, "binding new session");
    if ( mapBind(pstate->activeMap, &key, s, &s->binding) != XK_SUCCESS ) {
	xTrace0(vchanp, TR_SOFT_ERRORS, "VCHAN could not bind new session");
	vchanCloseSessn(s);
	return ERR_XOBJ;
    }
    xTrace1(vchanp, TR_MAJOR_EVENTS, "VCHAN open returns %x", s);
    return s;
}


static int
vchanControlSessn( s, opcode, buf, len )
    XObj            s;
    int             opcode, len;
    char           *buf;
{
    xAssert(xIsSession(s));

    switch (opcode) {
	
      case VCHAN_INCCONCURRENCY:
	checkLen(len, sizeof(int));
	return increaseConcurrency(s, *(int *)buf);
	
      case VCHAN_DECCONCURRENCY:
	checkLen(len, sizeof(int));
	return decreaseConcurrency(s, *(int *)buf);
	
      default:
	return xControl(((SState *)s->state)->stack.s[0], opcode, buf, len);
    }
}


static int
vchanControlProtl(self, opcode, buf, len)		
    XObj        self;
    int         opcode, len;
    char        *buf;
{
    return xControl(xGetDown(self, 0), opcode, buf, len);
}


/*
 * addSessn -- insert lls in the stack of session s
 */
static void
addSessn( s, lls )
    XObj	s, lls;
{
    SessnStack	*stack = &((SState *)s->state)->stack;
    
    xTrace1(vchanp, TR_MORE_EVENTS,
	    "vchan adds channel session %x to map", lls);
    xAssert(stack->top < stack->size);
    stack->s[stack->top++] = lls;
    semSignal(&stack->available);
}


/*
 * getSessn -- Return a channel session from the stack
 */
static XObj
getSessn( s )
    XObj 	s;
{
    SessnStack	*stack = &((SState *)s->state)->stack;
    XObj	lls;

    semWait(&stack->available);
    xAssert(stack->top > 0);
    xAssert(stack->top > stack->bottom);
    lls = stack->s[--stack->top];
    xTrace2(vchanp, TR_MORE_EVENTS,
	    "vchan getSessn returning sessn %x (# %d)", lls, stack->top);
    return lls;
}


/*
 * increaseConcurrency -- Increase the number of "active" channel
 * sessions in the stack by 'n'.  This may involve opening additional
 * channel sessions.
 */
static int
increaseConcurrency( s, n ) 
    XObj	s;
    int		n;
{
    XObj 	*newArr;
    SState	*state = (SState *)s->state;
    SessnStack	*stack = &state->stack;
    int 	i;
    XObj	lls, llp;
    Part_s	part[2];
    char	extBuf[1000];
    
    xTrace3(vchanp, TR_EVENTS,
	    "VCHAN session %x increasing concurrency (%d) by %d",
	    s, stack->size - stack->bottom, n);
    /*
     * If bottom > 0, we can reuse the 'trapped' channels, rather than
     * opening new ones
     */
    while ( stack->bottom > 0 && n > 0 ) {
	xTrace0(vchanp, TR_MORE_EVENTS, "Freeing trapped channel");
	stack->bottom--;
	n--;
	semSignal(&stack->available);
    }
    if ( n == 0 ) {
	xIfTrace(vchanp, TR_DETAILED) {
	    dispStack(stack);
	}
	return 0;
    }
    /* 
     * New channels have to be created.  Query an existing lower
     * session for the participants necessary to open the new sessions.
     */
    lls = *stack->s;
    xAssert(xIsSession(lls));
    if ( xControl(lls, GETPARTICIPANTS, (char *)extBuf, sizeof(extBuf)) < 0 ) {
	xTrace0(vchanp, TR_ERRORS,
		"vchan addSessn -- GETPARTICIPANTS call failed");
	return -1;
    }
    /*
     * Create a new array of channel sessions and copy the XObj's from
     * the old array to the new array
     */
    newArr = (XObj *)pathAlloc(s->path, (stack->size + n) * sizeof(XObj));
    if ( ! newArr ) {
	return -1;
    }
    xAssert( stack->s );
    bcopy((char *)stack->s, (char *)newArr, stack->top * sizeof(XObj));
    pathFree((char *)stack->s);
    stack->s = newArr;
    /*
     * Create and insert new channel sessions
     */
    xTrace0(vchanp, TR_MORE_EVENTS, "Opening new channel sessions");
    llp = xGetDown(xMyProtl(s), 0);
    for ( i=0; i < n; i++ ) {
	partInternalize(part, extBuf);
	lls = xOpen(xMyProtl(s), state->hlpType, llp, part, xMyPath(s));
	xTrace1(vchanp, TR_MORE_EVENTS, "got channel session %x", lls);
	if ( lls == ERR_XOBJ ) {
	    xTrace0(vchanp, TR_ERRORS,
		    "vchan could not open low-level session");
	    return -1;
	}
	stack->size += 1;
	addSessn(s, lls);
    }
    xIfTrace(vchanp, TR_DETAILED) {
	dispStack(stack);
    }
    xTrace0(vchanp, TR_EVENTS, "concurrency increase completed");
    return 0;
}


/*
 * decrease_concurrency -- reduce the number of "active" channels by n.
 * We don't really close channels, but we fake it by raising the stack
 * bottom.
 */
static int
decreaseConcurrency( s, n )
    XObj	s;
    int		n;
{
    SessnStack	*stack = &((SState *)s->state)->stack;
    
    xTrace2(vchanp, TR_EVENTS,
	    "vchan: decreasing concurrency (currently %d) by %d",
	    stack->top - stack->bottom, n);
    if ( n > (stack->size - stack->bottom) ) {
	/*
	 * We can't decrease the concurrency by the requested value.
	 * Give the caller a chance to reconsider rather than removing
	 * all of the available channels.
	 */
	return -1;
    }
    while ( n > 0 ) {
	semWait(&stack->available);
	stack->bottom++;
	n--;
    }
    xIfTrace(vchanp, TR_DETAILED) {
	dispStack(stack);
    }
    return 0;
}


static void
dispStack(s)
    SessnStack	*s;
{
    int i;
    
    xTrace4(vchanp, TR_ALWAYS,
	    "session stack %x:  size: %d, top: %d, bottom: %d\n", s,
	    s->size, s->top, s->bottom);
    xTrace0(vchanp, TR_ALWAYS, "sessions: ");
    for ( i=0; i < s->size; i++ ) {
	xTrace1(vchanp, TR_ALWAYS, "  %x", s->s[i]);
    }
}     


static xkern_return_t
vchanOpenEnable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    xTrace0(vchanp, TR_MAJOR_EVENTS, "VCHAN open enable");
    
    /*
     * openenable the protocol below me as the above protocol
     */
    return xOpenEnable(hlpRcv, hlpType, xGetDown(self,0), p);
}


static xkern_return_t
vchanOpenDisable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    xTrace0(vchanp, TR_MAJOR_EVENTS, "VCHAN openDisable");
    
    return xOpenDisable(hlpRcv, hlpType, xGetDown(self,0), p);
}


static xkern_return_t
vchanCloseSessn( s )
    XObj            s;
{
    PState	*ps = xMyProtl(s)->state;
    SessnStack	*stack = &((SState *)s->state)->stack;
    int		i;
    
    xAssert(xIsSession(s));
    xTrace1(vchanp, TR_MAJOR_EVENTS, "VCHAN close of session %x", s);
    /*
     * Free all of the channel sessions in the stack
     */
    for ( i=0; i < stack->size; i++ ) {
	xClose(stack->s[i]);
    }
    /*
     * Free the rest of the state associated with this session
     */
    if ( stack->s ) {
	pathFree(stack->s);
    }
    if ( s->binding ) {
	mapRemoveBinding(ps->activeMap, s->binding);
    }
    xDestroy(s);
    return XK_SUCCESS;
}


static xkern_return_t
vchanCall(
    XObj        s,
    Msg		msg,
    Msg		rmsg)
{
    XObj	lls;
    xkern_return_t	result;
    
    xTrace0(vchanp, TR_EVENTS, "in vchanCall");
    xAssert(xIsSession(s));
    lls = getSessn(s);
    result = xCall(lls, msg, rmsg);
    addSessn(s, lls);
    return result;
}


static xkern_return_t
getProcSessn( s )
    XObj	s;
{
    s->call = vchanCall;
    s->control = vchanControlSessn;
    s->close = vchanCloseSessn;
    return XK_SUCCESS;
}


static void
getProcProtl( p )
    XObj	p;
{
    p->control = vchanControlProtl;
    p->open = vchanOpen;
    p->openenable = vchanOpenEnable;
    p->opendisable = vchanOpenDisable;
}
