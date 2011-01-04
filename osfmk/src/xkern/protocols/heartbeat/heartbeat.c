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
 * A simple heartbeat protocol. Its message is
 * captured by the census taker protocol, which
 * is istantiated in one or more nodes within 
 * the computing domain ("master node"). The 
 * address of the census taker we start with 
 * comes from BOOTP. During the lifetime of 
 * the cluster, it gets overwritten via xControl.
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/heartbeat.h>
#include <xkern/protocols/heartbeat/heartbeat_i.h>

#if XK_DEBUG
int traceheartbeatp;
#endif /* XK_DEBUG */

static XObjRomOpt	heartbeatOpts[] =
{
    { "rate_msecs", 3, LoadRate }
};

static xkern_return_t
getSessnFuncs(
	      XObj	s)
{
    s->pop = heartbeatPop;
    s->push = heartbeatPush;
    s->close = heartbeatClose;
    s->control = heartbeatSessControl;
    return XK_SUCCESS;
}

static xkern_return_t
heartbeatClose(
	       XObj	self)
{
    XObj	myProtl = xMyProtl(self);
    XObj	lls = xGetDown(self, 0);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "heartbeat Close");

    if ( mapRemoveBinding(ps->activeMap, self->binding) == XK_FAILURE ) {
	xAssert(0);
	return XK_FAILURE;
    }
    xAssert(ss->ev != (Event)0);
    evCancel(ss->ev);
    evDetach(ss->ev);
    xAssert(xIsSession(lls));
    xClose(lls);
    xDestroy(self);
    return XK_SUCCESS;
}

static XObj
heartbeatCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path)
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    XObj        sess;
    
    semWait(&ps->serviceSem);
    if ( mapResolve(ps->activeMap, key, &sess) == XK_FAILURE ) {
	sess = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 1, &key->lls, path);
	if (sess == ERR_XOBJ)
	    return ERR_XOBJ;
	if ( mapBind(ps->activeMap, key, sess, &sess->binding) != XK_SUCCESS ) {
	    xDestroy(sess);
	    return ERR_XOBJ;
	}
    }
    semSignal(&ps->serviceSem);
    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	xDestroy(sess);
	return ERR_XOBJ;
    }
    sess->state = ss;
    ss->hlpNum = key->hlpNum;
    ss->rate_msecs = ps->def_rate_msecs;
    ss->GenNumber = 0;
    if (!(ss->ev = evAlloc(self->path))) {
	xTraceP0(self, TR_ERRORS, "Event allocation error");
	pathFree(ss);
	xDestroy(sess);
	return ERR_XOBJ;
    }
    if (msgConstructContig(&ss->msg_save, self->path, 0, 0) != XK_SUCCESS) {
	xTraceP0(self, TR_ERRORS, "Message allocation error in heartbeatCreateSess");
	evDetach(ss->ev);
	pathFree(ss);
	xDestroy(sess);
	return ERR_XOBJ;
    } 
    
    return sess;
}

xkern_return_t
heartbeatDemux(
		XObj 	self,
		XObj	lls,
		Msg 	msg)
{
    PState	 *ps = (PState *)self->state;
    XObj         sess;
    heartbeatXchangeHdr hdr;
    Enable	 *en;
    ActiveKey    key;

    xTraceP0(self, TR_FULL_TRACE, "demux called");

    if ( msgPop(msg, heartbeatHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "heartbeat demux -- msg pop failed");
	return XK_SUCCESS;
    }
    key.lls = lls;
    key.hlpNum = hdr.Prot;
    if ( mapResolve(ps->activeMap, (char *)&key, &sess) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.Prot, &en) == XK_FAILURE ) {
	    xTraceP1(self, TR_SOFT_ERRORS,
		    "demux -- no protl for Prot %d ", hdr.Prot);
	    return XK_FAILURE;
	} else {
	    xTraceP1(self, TR_DETAILED,
		    "demux -- found enable for Prot %d ", hdr.Prot);
	}
	/*
	 * Create a new session 
	 */
	sess = heartbeatCreateSessn(self, en->hlpRcv, en->hlpType, &key, msgGetPath(msg));
	if ( sess == ERR_XOBJ ) {
	    return XK_FAILURE;
	}
	xDuplicate(lls);
	xOpenDone(en->hlpRcv, sess, self, msgGetPath(msg));
    } else {
	xAssert(sess->rcnt > 0);
    }
    return xPop(sess, lls, msg, &hdr);
}

static long
heartbeatHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg)
{
    xAssert( len == sizeof(heartbeatXchangeHdr) );
    bcopy(src, hdr, len);
    ((heartbeatXchangeHdr *)hdr)->Prot = ntohl(((heartbeatXchangeHdr *)hdr)->Prot);
    ((heartbeatXchangeHdr *)hdr)->GenNumber = ntohl(((heartbeatXchangeHdr *)hdr)->GenNumber);
    return len;
}

static void
heartbeatHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg)
{
    xAssert(len = sizeof(heartbeatXchangeHdr));
    ((heartbeatXchangeHdr *)hdr)->Prot = htonl(((heartbeatXchangeHdr *)hdr)->Prot);
    ((heartbeatXchangeHdr *)hdr)->GenNumber = htonl(((heartbeatXchangeHdr *)hdr)->GenNumber);
    bcopy((char *)hdr, dst, len);
}

static XObj
heartbeatOpen(
	      XObj	self,
	      XObj	hlpRcv,
	      XObj	hlpType,
	      Part	p,
	      Path	path)
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    ActiveKey	key;
    XObj        sess;

    xTraceP0(self, TR_MAJOR_EVENTS, "open called");

    if ( (key.lls = xOpen(self, self, xGetDown(self, 0), p, path))
		== ERR_XOBJ ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "heartbeat could not open lls");
	return ERR_XOBJ;
    }
    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTraceP0(self, TR_ERRORS, "heartbeat open couldn't get hlpNum");
	xClose(key.lls);
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &key, &sess) == XK_SUCCESS ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "heartbeat open found existing sessn");
	xClose(key.lls);
	return sess;
    }
    sess = heartbeatCreateSessn(self, hlpRcv, hlpType, &key, path);
    if ( sess == ERR_XOBJ ) {
	xClose(key.lls);
	return ERR_XOBJ;
    }
    xTraceP1(self, TR_DETAILED, "heartbeatOpen returning %x", sess);

    return sess;
}

static xkern_return_t
heartbeatOpenDisable(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTraceP0(self, TR_MAJOR_EVENTS, "openDisable called");

    hlpNum = relProtNum(hlpType, self);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}

static xkern_return_t
heartbeatOpenDisableAll(
			XObj	self,
			XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_MAJOR_EVENTS, "openDisableAll called");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

xkern_return_t
heartbeatOpenDone(
		   XObj self,
		   XObj	lls,
		   XObj	llp,
		   XObj	hlpType,
		   Path	path)
{
    xTraceP0(self, TR_FULL_TRACE, "opendone called");

    return XK_SUCCESS;
}

static xkern_return_t
heartbeatOpenEnable(
		    XObj	self,
		    XObj	hlpRcv,
		    XObj	hlpType,
		    Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTraceP0(self, TR_FULL_TRACE, "openenable called");
    
    hlpNum = relProtNum(hlpType, self);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}

static xkern_return_t
heartbeatPop(
	     XObj	self,
	     XObj	lls,
	     Msg	m,
	     void	*inHdr)
{
    xTraceP0(self, TR_FULL_TRACE, "pop called");
    
    /*
     * We pass along the heartbeat header as a message attribute.
     */
    msgSetAttr(m, 0, inHdr, sizeof(heartbeatXchangeHdr));
    return xDemux(self, m);
}

static int
heartbeatProtControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "protocol control called");

    switch (op) {

    default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}

static void
heartbeatPulse(
	       Event	ev,
	       void 	*arg)
{
    XObj         self = (XObj)arg;
    PState       *ps = (PState *)xMyProtl(self)->state;
    SState       *ss = (SState *)self->state;
    Msg_s        message;
    heartbeatXchangeHdr hdr;

    xTraceP0(self, TR_FULL_TRACE, "Sending a heartbeat (timeout)");

    if (evIsCancelled(ev)) {
	xTraceP0(self, TR_FULL_TRACE, "timeout cancelled: no-op");
	return;
    } 
    msgConstructCopy(&message, &ss->msg_save);
    hdr.Prot = ss->hlpNum;
    hdr.GenNumber = ++ss->GenNumber;
    hdr.Sender = ps->thishost;
    if ( msgPush(&message, heartbeatHdrStore, &hdr, sizeof(heartbeatXchangeHdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "msg push fails");
    }
    
    if (xPush(xGetDown(self, 0), &message) < 0) {
	xTraceP0(self, TR_ERRORS, "Failure in sending a heartbeat (timeout)");
    }
    msgDestroy(&message);

    evSchedule(ev, heartbeatPulse, self, ss->rate_msecs * 1000);
}

static xmsg_handle_t
heartbeatPush(
	      XObj	self,
	      Msg	m)
{
    /*
     * Nothing
     */
    return XMSG_ERR_HANDLE;
}

static int
heartbeatSessControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    SState	*ss = (SState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "sessn control called");

    switch (op) {
    case HEARTBEAT_START_PULSE:
	(void)evSchedule(ss->ev, heartbeatPulse, self, ss->rate_msecs * 1000);
	break;
    case HEARTBEAT_STOP_PULSE:
	xAssert(ss->ev != (Event)0);
	evCancel(ss->ev);
	evDetach(ss->ev);
	break;
    default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}

static xkern_return_t   
LoadRate(
	 XObj	self,
	 char	**str,
	 int	nFields,
	 int	line,
	 void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "rate (%s) specified", str[2]);

    ps->def_rate_msecs = atoi(str[2]);
    return XK_SUCCESS;
}

xkern_return_t
heartbeat_init(
	       XObj self)
{
    XObj	llp;
    PState      *ps;
    Part_s      part[2];
    u_int       unused = 0;
    
    xTraceP0(self, TR_FULL_TRACE, "init called");

    llp = xGetDown(self, 0);
    if (!xIsProtocol(llp)) {
	xTraceP0(self, TR_ERRORS, "no lower protocol");
	return XK_FAILURE;
    }

    self->open = heartbeatOpen;
    self->demux = heartbeatDemux;
    self->opendone = heartbeatOpenDone;
    self->control = heartbeatProtControl;
    self->openenable = heartbeatOpenEnable;
    self->opendisable = heartbeatOpenDisable;
    self->opendisableall = heartbeatOpenDisableAll;
    self->hlpType = self;

    if (!(self->state = ps = pathAlloc(self->path, sizeof(PState)))) {
	xTraceP0(self, TR_ERRORS, "state allocation error in init");
	return XK_FAILURE;
    }

    /*
     * setting up the PState
     */
    semInit(&ps->serviceSem, 1);
    ps->def_rate_msecs = DEFAULT_PULSE_RATE;
    findXObjRomOpts(self, heartbeatOpts, sizeof(heartbeatOpts)/sizeof(XObjRomOpt), 0);

    ps->activeMap = mapCreate(HEARTBEAT_ACTIVE_MAP_SIZE, sizeof(ActiveKey),
			       self->path);
    ps->passiveMap = mapCreate(HEARTBEAT_PASSIVE_MAP_SIZE, sizeof(PassiveKey),
			       self->path);
    if ( ! ps->activeMap || ! ps->passiveMap ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	pathFree(ps);
	return XK_FAILURE;
    }
    
    if (xControl(xGetDown(self, 0), GETMYHOST, (char *)&ps->thishost, sizeof(IPhost)) 
	                                         < (int)sizeof(IPhost))	{
	xTraceP0(self, TR_ERRORS, "couldn't get local address");
	pathFree(ps);
	return XK_FAILURE;
    }
	    
#ifdef TWO_PARTICIPANTS
    partInit(&part[0], 2);
    partPush(*part, &unused, sizeof(unused));
    partPush(*part, ANY_HOST, 0);
#else  /* TWO_PARTICIPANTS */
    partInit(&part[0], 1);
    partPush(*part, ANY_HOST, 0);
#endif /* TWO_PARTICIPANTS */
    if (xOpenEnable(self, self, llp, part)) {
	xTraceP0(self, TR_ERRORS, "openenable failed in init");
	pathFree(ps);
	return XK_FAILURE;
    }

    return XK_SUCCESS;
}





