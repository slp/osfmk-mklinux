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
#include <xkern/protocols/template/template_internal.h>

#if XK_DEBUG
int tracetemplatep;
#endif /* XK_DEBUG */

static XObjRomOpt	templateOpts[] =
{
    { "rate_msecs", 3, LoadRate }
};

static xkern_return_t
getSessnFuncs(
	      XObj	s)
{
    s->pop = templatePop;
    s->push = templatePush;
    s->close = templateClose;
    s->control = templateSessControl;
    return XK_SUCCESS;
}

static xkern_return_t
templateClose(
	       XObj	self)
{
    XObj	myProtl = xMyProtl(self);
    XObj	lls = xGetDown(self, 0);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;

    xTrace0(templatep, TR_FULL_TRACE, "template Close");

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
templateCreateSessn(
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
	xTrace0(templatep, TR_ERRORS, "Event allocation error");
	pathFree(ss);
	xDestroy(sess);
	return ERR_XOBJ;
    }
    if (msgConstructContig(&ss->msg_save, self->path, 0, 0) != XK_SUCCESS) {
	xTrace0(templatep, TR_ERRORS, "Message allocation error in templateCreateSess");
	evDetach(ss->ev);
	pathFree(ss);
	xDestroy(sess);
	return ERR_XOBJ;
    } 
    
    return sess;
}

xkern_return_t
templateDemux(
		XObj 	self,
		XObj	lls,
		Msg 	msg)
{
    PState	 *ps = (PState *)self->state;
    XObj         sess;
    templateHdr hdr;
    Enable	 *en;
    ActiveKey    key;

    xTrace0(templatep, TR_FULL_TRACE, "demux called");

    if ( msgPop(msg, templateHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTrace0(templatep, TR_ERRORS, "template demux -- msg pop failed");
	return XK_SUCCESS;
    }
    key.lls = lls;
    key.hlpNum = hdr.Prot;
    if ( mapResolve(ps->activeMap, (char *)&key, &sess) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.Prot, &en) == XK_FAILURE ) {
	    xTrace1(templatep, TR_SOFT_ERRORS,
		    "demux -- no protl for Prot %d ", hdr.Prot);
	    return XK_FAILURE;
	} else {
	    xTrace1(templatep, TR_DETAILED,
		    "demux -- found enable for Prot %d ", hdr.Prot);
	}
	/*
	 * Create a new session 
	 */
	sess = templateCreateSessn(self, en->hlpRcv, en->hlpType, &key, msgGetPath(msg));
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
templateHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg)
{
    xAssert( len == sizeof(templateHdr) );
    bcopy(src, hdr, len);
    ((templateHdr *)hdr)->Prot = ntohl(((templateHdr *)hdr)->Prot);
    ((templateHdr *)hdr)->GenNumber = ntohl(((templateHdr *)hdr)->GenNumber);
    return len;
}

static void
templateHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg)
{
    xAssert(len = sizeof(templateHdr));
    ((templateHdr *)hdr)->Prot = htonl(((templateHdr *)hdr)->Prot);
    ((templateHdr *)hdr)->GenNumber = htonl(((templateHdr *)hdr)->GenNumber);
    bcopy((char *)hdr, dst, len);
}

static XObj
templateOpen(
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

    xTrace0(templatep, TR_MAJOR_EVENTS, "open called");

    if ( (key.lls = xOpen(self, self, xGetDown(self, 0), p, path))
		== ERR_XOBJ ) {
	xTrace0(templatep, TR_MAJOR_EVENTS, "template could not open lls");
	return ERR_XOBJ;
    }
    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTrace0(templatep, TR_ERRORS, "template open couldn't get hlpNum");
	xClose(key.lls);
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &key, &sess) == XK_SUCCESS ) {
	xTrace0(templatep, TR_MAJOR_EVENTS, "template open found existing sessn");
	xClose(key.lls);
	return sess;
    }
    sess = templateCreateSessn(self, hlpRcv, hlpType, &key, path);
    if ( sess == ERR_XOBJ ) {
	xClose(key.lls);
	return ERR_XOBJ;
    }
    xTrace1(templatep, TR_DETAILED, "templateOpen returning %x", sess);

    ss = sess->state;
    (void)evSchedule(ss->ev, templatePulse, sess, ss->rate_msecs * 1000);
    return sess;
}

static xkern_return_t
templateOpenDisable(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTrace0(templatep, TR_MAJOR_EVENTS, "openDisable called");

    hlpNum = relProtNum(hlpType, self);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}

static xkern_return_t
templateOpenDisableAll(
			XObj	self,
			XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTrace0(templatep, TR_MAJOR_EVENTS, "openDisableAll called");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

xkern_return_t
templateOpenDone(
		   XObj self,
		   XObj	lls,
		   XObj	llp,
		   XObj	hlpType,
		   Path	path)
{
    xTrace0(templatep, TR_FULL_TRACE, "opendone called");

    return XK_SUCCESS;
}

static xkern_return_t
templateOpenEnable(
		    XObj	self,
		    XObj	hlpRcv,
		    XObj	hlpType,
		    Part	p)
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTrace0(templatep, TR_FULL_TRACE, "openenable called");
    
    hlpNum = relProtNum(hlpType, self);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}

static xkern_return_t
templatePop(
	     XObj	self,
	     XObj	lls,
	     Msg	m,
	     void	*inHdr)
{
    xTrace0(templatep, TR_FULL_TRACE, "pop called");
    
    /*
     * We pass along the template header as a message attribute.
     */
    msgSetAttr(m, 0, inHdr, sizeof(templateHdr));
    return xDemux(self, m);
}

static int
templateProtControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    PState	*ps = (PState *)self->state;

    xTrace0(templatep, TR_FULL_TRACE, "control called");

    switch (op) {

    default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}

static void
templatePulse(
	       Event	ev,
	       void 	*arg)
{
    XObj         self = (XObj)arg;
    PState       *ps = (PState *)xMyProtl(self)->state;
    SState       *ss = (SState *)self->state;
    Msg_s        message;
    templateHdr hdr;

    xTrace0(templatep, TR_FULL_TRACE, "Sending a template (timeout)");

    if (evIsCancelled(ev)) {
	xTrace0(templatep, TR_FULL_TRACE, "timeout cancelled: no-op");
	return;
    } 
    msgConstructCopy(&message, &ss->msg_save);
    hdr.Prot = ss->hlpNum;
    hdr.GenNumber = ++ss->GenNumber;
    hdr.Sender = ps->thishost;
    if ( msgPush(&message, templateHdrStore, &hdr, sizeof(templateHdr), 0) == XK_FAILURE ) {
	xTrace0(templatep, TR_ERRORS, "msg push fails");
    }
    
    if (xPush(xGetDown(self, 0), &message) < 0) {
	xTrace0(templatep, TR_ERRORS, "Failure in sending a template (timeout)");
    }
    msgDestroy(&message);

    evSchedule(ev, templatePulse, self, ss->rate_msecs * 1000);
}

static xmsg_handle_t
templatePush(
	      XObj	self,
	      Msg	m)
{
    /*
     * Nothing
     */
    return XMSG_ERR_HANDLE;
}

static int
templateSessControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    SState	*ss = (SState *)self->state;

    xTrace0(templatep, TR_FULL_TRACE, "control called");

    switch (op) {

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
    
    xTrace1(templatep, TR_EVENTS, "rate (%s) specified", str[2]);

    ps->def_rate_msecs = atoi(str[2]);
    return XK_SUCCESS;
}

xkern_return_t
template_init(
	       XObj self)
{
    XObj	llp;
    PState      *ps;
    Part_s      part[2];
    u_int       unused = 0;
    
    xTrace0(templatep, TR_FULL_TRACE, "init called");

    llp = xGetDown(self, 0);
    if (!xIsProtocol(llp)) {
	xTrace0(templatep, TR_ERRORS, "no lower protocol");
	return XK_FAILURE;
    }

    self->open = templateOpen;
    self->demux = templateDemux;
    self->opendone = templateOpenDone;
    self->control = templateProtControl;
    self->openenable = templateOpenEnable;
    self->opendisable = templateOpenDisable;
    self->opendisableall = templateOpenDisableAll;
    self->hlpType = self;

    if (!(self->state = ps = pathAlloc(self->path, sizeof(PState)))) {
	xTrace0(templatep, TR_ERRORS, "state allocation error in init");
	return XK_FAILURE;
    }

    /*
     * setting up the PState
     */
    semInit(&ps->serviceSem, 1);
    ps->def_rate_msecs = DEFAULT_PULSE_RATE;
    findXObjRomOpts(self, templateOpts, sizeof(templateOpts)/sizeof(XObjRomOpt), 0);

    ps->activeMap = mapCreate(TEMPLATE_ACTIVE_MAP_SIZE, sizeof(ActiveKey),
			       self->path);
    ps->passiveMap = mapCreate(TEMPLATE_PASSIVE_MAP_SIZE, sizeof(PassiveKey),
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
	xTrace0(templatep, TR_ERRORS, "openenable failed in init");
	pathFree(ps);
	return XK_FAILURE;
    }

    return XK_SUCCESS;
}





