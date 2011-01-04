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
 * blast.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * blast.c,v
 * Revision 1.42.1.4.1.2  1994/09/07  04:18:27  menze
 * OSF modifications
 *
 * Revision 1.42.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.42.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.42.1.2  1994/07/22  20:08:05  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.42.1.1  1994/04/23  00:12:53  menze
 * Added registration for allocator call-backs
 *
 * Revision 1.42  1993/12/13  21:11:29  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

/*
 * Initialization and connection establishment / teardown routines
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/blast.h>
#include <xkern/protocols/blast/blast_internal.h>


#define checkPart(pxx, sxx, retVal) 					\
    if ( ! (pxx) || partLen(pxx) < 1 ) {				\
        xTrace1(blastp, TR_SOFT_ERROR, "%s -- bad participants", sxx); 	\
	return retVal;							\
    }

static xkern_return_t	blastClose( XObj );
static long		getRelProtNum( XObj, XObj, char * );
static void		protGetProc( XObj );
static xkern_return_t	sessnGetProc( XObj );

#if XK_DEBUG
int 		traceblastp;
#endif /* XK_DEBUG */

BlastMask	blastFullMask[BLAST_MAX_FRAGS + 1];


static void
psFree( PState *ps )
{
    if ( ps->active_map ) mapClose(ps->active_map);
    if ( ps->passive_map ) mapClose(ps->passive_map);
    if ( ps->send_map ) mapClose(ps->send_map);
    if ( ps->mstateStack ) stackDestroy(ps->mstateStack);
    pathFree(ps);
}


xkern_return_t
blast_init(self)
    XObj self;
{
    Part_s 	part;
    PState 	*pstate;
    int 	i;
    XObj	llp;
    Path	path = self->path;
    
    xTrace0(blastp, TR_GROSS_EVENTS, "BLAST init");
    xAssert(xIsProtocol(self));
    
    if ((llp = xGetDown(self,0)) == ERR_XOBJ) {
	xTrace0(blastp, TR_ERRORS, "blast couldn't access lower protocol");
	return XK_FAILURE;
    }
    protGetProc(self);
    if ( ! (self->state = pstate = pathAlloc(path, sizeof(PState))) ) {
	return XK_FAILURE;
    }
    bzero((char *)pstate, sizeof(PState));
    pstate->active_map = mapCreate(BLAST_ACTIVE_MAP_SZ, sizeof(ActiveID),
				   path);
    pstate->passive_map = mapCreate(BLAST_PASSIVE_MAP_SZ, sizeof(PassiveID),
				    path);
    pstate->send_map = mapCreate(BLAST_SEND_MAP_SZ, sizeof(BlastSeq), path);
    pstate->mstateStack = stackCreate(BLAST_MSTATE_STACK_SZ, path);
    if ( ! pstate->active_map || ! pstate->passive_map || ! pstate->send_map ||
	 ! pstate->mstateStack ) {
	xTraceP0(self, TR_ERRORS, "allocation error in init");
	psFree(pstate);
	return XK_FAILURE;
    }
    semInit(&pstate->outstanding_messages, OUTSTANDING_MESSAGES);
    semInit(&pstate->createSem, 1);
    pstate->max_outstanding_messages = OUTSTANDING_MESSAGES;
    for (i=1; i <= BLAST_MAX_FRAGS; i++) {
	BLAST_FULL_MASK(blastFullMask[i], i);
    }
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    xOpenEnable(self, self, llp, &part);
    allocRegister(0, blastAllocCallBack, self);
    return XK_SUCCESS;
}


static long
getRelProtNum( hlp, llp, s )
    XObj	hlp, llp;
    char	*s;
{
    long	n;

    n = relProtNum(hlp, llp);
    if ( n == -1 ) {
	xTrace3(blastp, TR_SOFT_ERROR,
	       "%s: couldn't get prot num of %s relative to %s",
	       s, hlp->name, llp->name);
	return -1;
    }
    return n;
}


static XObj
blastOpen(
    XObj self,
    XObj hlpRcv,
    XObj hlpType,
    Part p,
    Path path )
{
    XObj	s;
    XObj	lls;
    ActiveID 	active_id;
    PState 	*pstate;
    
    xTrace0(blastp, TR_MAJOR_EVENTS, "BLAST open");
    checkPart(p, "blastOpen", ERR_XOBJ);
    pstate = (PState *)self->state;
    semWait(&pstate->createSem);
    if ( (active_id.prot = getRelProtNum(hlpType, self, "blastOpen")) == -1 ) {
	return ERR_XOBJ;
    }
    if ( (lls = xOpen(self, self, xGetDown(self, 0), p, path)) == ERR_XOBJ ) {
	xTrace0(blastp, TR_MAJOR_EVENTS, "blast open: could not open lls");
	s = ERR_XOBJ;
    } else {
	xTrace0(blastp, TR_MAJOR_EVENTS, "blast_open successfully opened lls");
	active_id.lls = lls;
	xIfTrace(blastp, TR_MORE_EVENTS) {
	    blastShowActiveKey(&active_id, "blast_open");
	}
	/*
	 * is there an existing session?
	 */
	if ( mapResolve(pstate->active_map, &active_id, &s) == XK_FAILURE ) {
	    xTrace0(blastp, TR_MAJOR_EVENTS,
		    "blast_open creating new session");
	    s = blastCreateSessn(self, hlpRcv, hlpType, &active_id, path);
	    if ( s == ERR_XOBJ ) {
		xClose(lls);
	    }
	} else {
	    xTrace0(blastp, TR_MAJOR_EVENTS, "blast_open: session exists");
	    xClose(lls);
	}
    }
    semSignal(&pstate->createSem);
    return s;
}    


static void
ssFree( SState *ss )
{
    if ( ss->rec_map ) {
	mapClose(ss->rec_map);
    }
    pathFree(ss);
}


XObj
blastCreateSessn( self, hlpRcv, hlpType, key, path )
    XObj 	self, hlpRcv, hlpType;
    ActiveID 	*key;
    Path	path;
{
    PState	*pstate;
    SState 	*state;
    BLAST_HDR	*hdr;
    XObj	s;

    xTraceP1(self, TR_ALWAYS, "createSession: path %d", path->id);
    pstate = (PState *)self->state;
    if ( ! (state = pathAlloc(path, sizeof(SState))) ) {
	return ERR_XOBJ;
    }
    bzero((char *)state, sizeof(SState));
    state->prot_id = key->prot;
    state->rec_map = mapCreate(BLAST_REC_MAP_SZ, sizeof(BlastSeq), path);
    if ( ! state->rec_map ) {
	ssFree(state);
	return ERR_XOBJ;
    }
    /*
     * fill in header for messages that don't require fragmentation
     */
    hdr = &state->short_hdr;
    hdr->op = BLAST_SEND;
    hdr->prot_id = key->prot;
    hdr->seq = 0; /* protocol guarantees that 0 can never be a real seq */
    BLAST_MASK_CLEAR(hdr->mask);
    hdr->num_frag = 0;
    /*
     * Determine the maximum size of datagrams which this session
     * supports.  
     */
    if (xControl(key->lls, GETOPTPACKET, (char *)&state->fragmentSize,
		 sizeof(state->fragmentSize)) < 0) {
	xTrace0(blastp, TR_ERRORS,
		"Blast could not get opt packet size from lls");
	if (xControl(key->lls, GETMAXPACKET, (char *)&state->fragmentSize,
		     sizeof(state->fragmentSize)) < 0) {
	    xTrace0(blastp, TR_ERRORS,
		    "Blast could not get max packet size from lls");
	    ssFree(state);
	    return ERR_XOBJ;
	}
    }
    xTrace1(blastp, TR_MAJOR_EVENTS,
	    "fragment size (from llp): %d", state->fragmentSize);
    xTrace1(blastp, TR_MAJOR_EVENTS, "blast hdr len: %d", BLASTHLEN);
    state->fragmentSize -= BLASTHLEN;
    xTrace2(blastp, TR_MAJOR_EVENTS,
	    "Blast fragmenting into packets of size %d, max dgm: %d",
	    state->fragmentSize, state->fragmentSize * BLAST_MAX_FRAGS);
    state->per_packet = PER_PACKET;
    /*
     * create session and bind to address
     */
    s = xCreateSessn(sessnGetProc, hlpRcv, hlpType, self, 1, &key->lls, path);
    if ( s == ERR_XOBJ ) {
	ssFree(state);
	return ERR_XOBJ;
    }
    s->state = state;
    state->self = s;
    if ( mapBind(pstate->active_map, key, s, &s->binding) != XK_SUCCESS ) {
	xDestroy(s);
	return ERR_XOBJ;
    }
    xTraceP1(self, TR_MAJOR_EVENTS, "open returns %lx", (long)s);
    return s;
}


static xkern_return_t
blastOpenEnable(
    XObj self,
    XObj hlpRcv,
    XObj hlpType,
    Part p )
{
    PassiveID 	key;
    PState 	*pstate;
    
    xTrace0(blastp, TR_MAJOR_EVENTS, "BLAST open enable");
    checkPart(p, "blastOpenEnable", XK_FAILURE);
    pstate = (PState *)self->state;
    if ( (key = getRelProtNum(hlpType, self, "BLAST openEnable")) == -1 ) {
	return XK_FAILURE;
    }
    xTrace1(blastp, TR_MAJOR_EVENTS,
	    "blast_openenable: ext_id.prot_id = %d", key);
    return defaultOpenEnable(pstate->passive_map, hlpRcv, hlpType, &key);
}


static xkern_return_t
blastOpenDisable(
    XObj self,
    XObj hlpRcv,
    XObj hlpType,
    Part p )
{
    PassiveID   key;
    PState      *pstate;
    
    xTrace0(blastp, TR_MAJOR_EVENTS, "BLAST open disable");
    checkPart(p, "blastOpenDisable", XK_FAILURE);
    pstate = (PState *)self->state;
    if ( (key = getRelProtNum(hlpType, self, "BLAST openDisable")) == -1 ) {
	return XK_FAILURE;
    }
    return defaultOpenDisable(pstate->passive_map, hlpRcv, hlpType, &key);
}


/* 
 * Decrement the internal reference count and call blastClose if appropriate
 */
void
blastDecIrc( s )
    XObj	s;
{
    SState	*ss = (SState *)s->state;

    xAssert(ss->ircnt > 0);
    if ( --ss->ircnt == 0 && s->rcnt == 0 ) {
	blastClose(s);
    }
}


/*
 * blastClose: blast does no caching of sessions -- when a session's
 * reference count drops to zero it is destroyed.
 */
static xkern_return_t
blastClose(s)
    XObj s;
{
    SState *ss;
    PState	*ps;
    XObj	lls;
    
    xTrace1(blastp, TR_MAJOR_EVENTS, "blast_close of session %x", s);

    if (!s) return(XK_SUCCESS); 
    ss = (SState *)s->state;
    ps = (PState *)s->myprotl->state;

    xAssert(s->rcnt == 0);
    xTrace1(blastp, TR_EVENTS, "blast_close ircnt == %d", ss->ircnt);
    if ( ss->ircnt ) {
	return XK_SUCCESS;
    }
    if (s->binding) {
	mapRemoveBinding(ps->active_map,s->binding);
    }

    ssFree(ss);
    s->state = 0;

    if ((lls = xGetDown(s, 0)) != ERR_XOBJ) {
	xClose(lls);
    }
    xDestroy(s);
    return XK_SUCCESS;
}

    	
MSG_STATE *
blastNewMstate(s)
    XObj s;
{
    PState	*ps;
    MSG_STATE	*mstate;
    
    ps = (PState *)xMyProtl(s)->state;
    mstate = (MSG_STATE *)stackPop(ps->mstateStack);
    if ( mstate == 0 ) {
	Path	p = s->path;

	if ( ! (mstate = pathAlloc(p, sizeof(MSG_STATE))) ) {
	    return 0;
	}
	bzero((char *)mstate, sizeof(MSG_STATE));
	if ( ! (mstate->event = evAlloc(p)) ) {
	    pathFree(mstate);
	    return 0;
	}
	xTrace0(blastp, TR_MORE_EVENTS, "blast_pop: new_state created ");
    } else {
	Event ev;

	ev = mstate->event;
	bzero((char *)mstate, sizeof(MSG_STATE));
	mstate->event = ev;
    }
    mstate->rcnt = 1;
    /* 
     * Add a reference count for this message state
     */
    ((SState *)s->state)->ircnt++;
    xTrace1(blastp, TR_MORE_EVENTS, "blast receive returns %x", mstate);
    return mstate;
}


static xkern_return_t
sessnGetProc(s)
    XObj s;
{
    s->push = blastPush;
    s->control = blastControlSessn;
    s->close = blastClose;
    s->pop = blastPop;
    return XK_SUCCESS;
}


static void
protGetProc(p)
    XObj p;
{
    p->control = blastControlProtl;
    p->open = blastOpen;
    p->openenable = blastOpenEnable;
    p->demux = blastDemux;
    p->opendisable = blastOpenDisable;
}


void
blastAllocCallBack( Allocator a, void *arg )
{
    XObj	self = arg;
    
    xTraceP0(self, TR_EVENTS, "blast call-backs initiated");
    mapForEach(((PState *)self->state)->active_map, blastRecvCallBack, a);
    blastSendCallBack(self, a);
    xTraceP0(self, TR_EVENTS, "blast call-backs finished");
}
