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

#include <xkern/include/xkernel.h>
#include <xkern/protocols/panning/panning_i.h>
#include <xkern/include/prot/panning.h>
#include <xkern/include/prot/bootp.h>

#if XK_DEBUG
int	tracepanningp;
#endif /* XK_DEBUG */

static void
getProtlFuncs(
	      XObj	p)
{
    p->open = panningOpen;
    p->control = panningControlProtl;
    p->demux = panningDemux;
    p->openenable = panningOpenEnable;
    p->opendisable = panningOpenDisable;
    p->opendisableall = panningOpenDisableAll;
}

static xkern_return_t
getSessnFuncs(
	      XObj	s)
{
    s->control = panningControlSessn;
    s->close = panningClose;
    s->push = panningPush;
    s->pop  = panningPop;
    return XK_SUCCESS;
}

static xkern_return_t
panningClose(
	     XObj	self)
{
    XObj	myProtl = xMyProtl(self);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;
    XObj	lls;

    xTraceP0(self, TR_MAJOR_EVENTS, "panning Close");
    lls = xGetDown(self, 0);

    if ( mapRemoveBinding(ps->activeMap, self->binding) == XK_FAILURE ) {
	xAssert(0);
	return XK_FAILURE;
    }
    xAssert(xIsSession(lls));
    xClose(lls);
    xDestroy(self);
    return XK_SUCCESS;
}

static int
panningControlProtl(
		    XObj	self,
		    int		op,
		    char 	*buf,
		    int		len)
{
    PState	*ps = (PState *)self->state;

    switch ( op ) {
    case GETMAXPACKET:
    case GETOPTPACKET:
	if (xControl(xGetDown(self, PANNING_TRANSPORT), op, buf, len) < (int)sizeof(int)) {
	    return -1;
	}
	*(int *)buf -= sizeof(panningHdr);
	return (sizeof(int));

    default:
	return xControl(xGetDown(self, PANNING_TRANSPORT), op, buf, len);
    }
}

static int
panningControlSessn(
		    XObj	self,
		    int		op,
		    char 	*buf,
		    int		len)
{
    SState	*ss = (SState *)self->state;
    PState      *ps = (PState *)xMyProtl(self)->state;

    switch ( op ) {

    case SETINCARNATION:
	checkLen(len, sizeof(u_bit32_t));
	ps->myGroupIncarnation = *(u_bit32_t *)buf;
	return 0;

    case CHECKINCARNATION:
	checkLen(len, sizeof(boolean_t));
	if ( *(boolean_t *)buf) {
	    ss->s_check_incarnation = TRUE;
	} else {
	    ss->s_check_incarnation = FALSE;
	}
	return 0;

    case GETMAXPACKET:
    case GETOPTPACKET:
	if (xControl(xGetDown(self, PANNING_TRANSPORT), op, buf, len) < (int)sizeof(int)) {
	    return -1;
	}
	*(int *)buf -= sizeof(panningHdr);
	return (sizeof(int));

    default:
	return xControl(xGetDown(self, PANNING_TRANSPORT), op, buf, len);
    }
}

static xkern_return_t
panningDemux(
	     XObj	self,
	     XObj	lls,
	     Msg	m)
{
    PState	*ps = (PState *)self->state;
    panningHdr	hdr;
    XObj	s;
    Enable	*e;
    ActiveKey	key;

    xTraceP0(self, TR_EVENTS, "panning demux");
    if ( msgPop(m, panningHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "panning demux -- msg pop failed");
	return XK_SUCCESS;
    }
    key.lls = lls;
    key.hlpNum = hdr.h_hlpNum;
    if ( mapResolve(ps->activeMap, (char *)&key, &s) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.h_hlpNum, &e) == XK_FAILURE ) {
	    xTraceP1(self, TR_SOFT_ERRORS,
		    "panning demux -- no protl for hlpNum %d ", hdr.h_hlpNum);
	    return XK_FAILURE;
	} else {
	    xTraceP1(self, TR_DETAILED,
		    "panning demux -- found enable for hlpNum %d ", hdr.h_hlpNum);
	}
	s = panningCreateSessn(self, e->hlpRcv, e->hlpType, &key, msgGetPath(m));
	if ( s == ERR_XOBJ ) {
	    return XK_FAILURE;
	}
	xDuplicate(lls);
	xOpenDone(e->hlpRcv, s, self, msgGetPath(m));
    } else {
	xAssert(s->rcnt > 0);
    }
    return xPop(s, lls, m, &hdr);
}

static XObj
panningCreateSessn(
		   XObj	self,
		   XObj	hlpRcv,
		   XObj	hlpType,
		   ActiveKey	*key,
		   Path	path)
{
    PState	*ps = (PState *)self->state;
    XObj	s;

    /* 
     * We might block (but only briefly), so we'll only allow one
     * thread to create a session at a time to avoid map binding
     * conflicts. 
     */
    semWait(&ps->sessnCreationSem);
    /* 
     * Check again now that we have the lock
     */
    if ( mapResolve(ps->activeMap, key, &s) == XK_FAILURE ) {
	s = panningNewSessn(self, hlpRcv, hlpType, key, path);
    }
    semSignal(&ps->sessnCreationSem);
    return s;
}

static void
panningHdrStore(
		void	*h,
		char 	*dst,
		long	len,
		void	*arg)
{
    panningHdr	hdr;

    xAssert( len == sizeof(panningHdr) );
    hdr.h_hlpNum = htonl(((panningHdr *)h)->h_hlpNum);
    hdr.h_cluster = htonl(((panningHdr *)h)->h_cluster);
    hdr.h_gincarnation = htonl(((panningHdr *)h)->h_gincarnation);
    bcopy((char *)&hdr, dst, len);
}

static long
panningHdrLoad(
	       void	*hdr,
	       char 	*src,
	       long	len,
	       void	*arg)
{
    xAssert( len == sizeof(panningHdr) );
    bcopy(src, hdr, len);
    ((panningHdr *)hdr)->h_hlpNum = ntohl(((panningHdr *)hdr)->h_hlpNum);
    ((panningHdr *)hdr)->h_cluster = ntohl(((panningHdr *)hdr)->h_cluster);
    ((panningHdr *)hdr)->h_gincarnation = ntohl(((panningHdr *)hdr)->h_gincarnation);
    return len;
}

static XObj
panningNewSessn(
		XObj	self,
		XObj	hlpRcv,
		XObj	hlpType,
		ActiveKey	*key,
		Path	path)	    
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    SState	*ss;
    
    s = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 1, &key->lls, path);
    if ( s == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return ERR_XOBJ;
    }

    ss->s_hdr.h_hlpNum = key->hlpNum;
    ss->s_hdr.h_cluster = ps->myCluster;
    ss->s_check_incarnation = FALSE;
    s->state = ss;

    if ( mapBind(ps->activeMap, key, s, &s->binding) != XK_SUCCESS ) {
	xDestroy(s);
	return ERR_XOBJ;
    }

    return s;
}

static XObj
panningOpen(
	    XObj	self,
	    XObj	hlpRcv,
	    XObj	hlpType,
	    Part	p,
	    Path	path)
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    ActiveKey	key;

    xTraceP0(self, TR_MAJOR_EVENTS, "panningOpen");
    if ( (key.lls = xOpen(self, self, xGetDown(self, PANNING_TRANSPORT), p, path))
		== ERR_XOBJ ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "panning could not open lls");
	return ERR_XOBJ;
    }
    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTraceP0(self, TR_ERRORS, "panning open couldn't get hlpNum");
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &key, &s) == XK_SUCCESS ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "panning open found existing sessn");
	xClose(key.lls);
	return s;
    }
    s = panningCreateSessn(self, hlpRcv, hlpType, &key, path);
    if ( s == ERR_XOBJ ) {
	xClose(key.lls);
	return ERR_XOBJ;
    }
    xTraceP1(self, TR_DETAILED, "panningOpen returning %x", s);
    return s;
}

static xkern_return_t
panningOpenEnable(
		  XObj	self,
		  XObj	hlpRcv,
		  XObj	hlpType,
		  Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    hlpNum = relProtNum(hlpType, self);
    xTraceP1(self, TR_MAJOR_EVENTS, "panning openEnable binding key %d",
	    hlpNum);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}

static xkern_return_t
panningOpenDisable(
		   XObj	self,
		   XObj	hlpRcv,
		   XObj	hlpType,
		   Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTraceP0(self, TR_MAJOR_EVENTS, "panning openDisable");
    hlpNum = relProtNum(hlpType, self);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}

static xkern_return_t
panningOpenDisableAll(
		      XObj	self,
		      XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_MAJOR_EVENTS, "panning openDisableAll");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

static xkern_return_t
panningPop(
	   XObj	self,
	   XObj	lls,
	   Msg	m,
	   void	*inHdr)
{
    SState	*ss = (SState *)self->state;
    PState      *ps = (PState *)xMyProtl(self)->state;
    panningHdr	*hdr = (panningHdr *)inHdr;
    
    xAssert(hdr);
    if ( hdr->h_cluster != ps->myCluster ) {
	xTraceP2(self, TR_MAJOR_EVENTS,
		"panningPop: mismatch of my clusterid (%x (hdr) vs. %x (state))",
		hdr->h_cluster, ps->myCluster);
	return XK_SUCCESS;
    }
    /*
     * I pass up the exact incarnation and the WILD CARD
     * regardless of the per-session choice of 
     * ss->s_check_incarnation. If s_check_incarnation
     * is TRUE, I don't pass anything else.
     */
    if (hdr->h_gincarnation != ps->myGroupIncarnation &&
	hdr->h_gincarnation != PANNING_WILD_CARD &&
	ss->s_check_incarnation) {
	xTraceP2(self, TR_MAJOR_EVENTS,
		"panningPop: mismatch of incarnation (%x (hdr) vs. %x (state))",
		hdr->h_gincarnation, ps->myGroupIncarnation);
	return XK_SUCCESS;
    }
    return xDemux(self, m);
}

static xmsg_handle_t
panningPush(
	    XObj	self,
	    Msg		m)
{
    SState	*ss = (SState *)self->state;
    PState      *ps = (PState *)xMyProtl(self)->state;
    panningHdr  hdr;

    xTraceP0(self, TR_EVENTS, "panningPush");

    hdr = ss->s_hdr;
    /*
     * Refresh the group incarnation as the header
     * cached in session state may have a stale
     * version.
     */
    hdr.h_gincarnation = ps->myGroupIncarnation;
    if ( msgPush(m, panningHdrStore, &hdr, sizeof(panningHdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "msg push fails");
	return XMSG_ERR_HANDLE;
    }
    return xPush(xGetDown(self, PANNING_TRANSPORT), m);
}

xkern_return_t
panning_init(
	     XObj	self)
{
    Part_s	p;
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_GROSS_EVENTS, "panning Init");
    if ( ! (ps = pathAlloc(self->path, sizeof(PState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
    self->state = ps;
    ps->myGroupIncarnation = PANNING_WILD_CARD;
    if ( ! xIsProtocol(xGetDown(self, PANNING_TRANSPORT)) ) {
	xTraceP0(self, TR_ERRORS,
		"panning could not get transport protocol -- aborting init");
	return XK_FAILURE;
    }
    if ( ! xIsProtocol(xGetDown(self, PANNING_BOOTP)) ) {
	xTraceP0(self, TR_ERRORS,
		"panning could not get to BOOTP protocol -- aborting init");
	return XK_FAILURE;
    }

    if ( xControl(xGetDown(self, PANNING_BOOTP), BOOTP_GET_CLUSTERID,
		  (char *)&ps->myCluster, sizeof(u_bit32_t)) < (int)sizeof(u_bit32_t) ) {
	xTraceP0(self, TR_ERRORS, "panning could not get my clusterid -- aborting init");
	return XK_FAILURE;
    }
    ps->activeMap = mapCreate(PANNING_ACTIVE_MAP_SIZE, sizeof(ActiveKey),
			      self->path);
    ps->passiveMap = mapCreate(PANNING_PASSIVE_MAP_SIZE, sizeof(PassiveKey),
			       self->path);
    if ( ! ps->activeMap || ! ps->passiveMap ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
    semInit(&ps->sessnCreationSem, 1);
    getProtlFuncs(self);
    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, PANNING_TRANSPORT), &p)	== XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "openEnable failed in panning init");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}






