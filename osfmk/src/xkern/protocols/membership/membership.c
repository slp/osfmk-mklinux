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
#include <xkern/include/prot/membership.h>
#include <xkern/include/prot/sequencer.h>
#include <xkern/include/prot/censustaker.h>
#include <xkern/protocols/membership/membership_i.h>

#if XK_DEBUG
int tracemembershipp;
#endif /* XK_DEBUG */

static XObjRomOpt	membershipOpts[] =
{
    { "max_groups", 3, LoadMaxGroups },
    { "max_nodes", 3, LoadMaxNodes },
};

static xkern_return_t
getSessnFuncs(
	      XObj	s)
{
    s->pop = membershipPop;
    s->push = membershipPush;
    s->close = membershipClose;
    s->control = membershipSessControl;
    return XK_SUCCESS;
}

static xkern_return_t
membershipClose(
	       XObj	self)
{
    XObj	myProtl = xMyProtl(self);
    XObj	lls = xGetDown(self, 0);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "membership Close");

    if ( mapRemoveBinding(ps->activeMap, self->binding) == XK_FAILURE ) {
	xAssert(0);
	return XK_FAILURE;
    }
    xAssert(xIsSession(lls));
    xClose(lls);
    xDestroy(self);
    return XK_SUCCESS;
}

static GState *
membershipCreateGState(
		       XObj     self,
		       group_id gid)
{
    PState	 *ps = (PState *)self->state;
    GState       *gg;

    if ( ! (gg = (GState *)pathAlloc(self->path, sizeof(GState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error for GState");
	return (GState *)0;
    }
    memset(gg, 0, sizeof(GState));
    gg->this_gid = gid;
    if (gg->this_gid == ALL_NODES_GROUP) {
	gg->node_map = ps->node2groups;
    } else {
	gg->node_map = mapCreate(ps->max_nodes, sizeof(IPhost), self->path);
	if (!gg->node_map) {
	    xTraceP0(self, TR_ERRORS, "allocation error for GState");
	    return (GState *)0;
	}
    }
    return gg;
}

static XObj
membershipCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path)
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    GState      *gg;
    XObj        sess;
    group_id    gid;
    Bind        bd;
    
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
	xTraceP0(self, TR_ERRORS, "allocation error for SState");
	xDestroy(sess);
	return ERR_XOBJ;
    }
    sess->state = ss;
    memset(ss, 0, sizeof(SState));
    ss->timestamp = ++ps->timestamp;
    ss->hlpNum = key->hlpNum;
    /*
     * Determine the group id associated to this session.
     */
    if (xControl(key->lls, GETGROUPID, (char *)&gid, sizeof(group_id)) < sizeof(group_id)) {
	xTraceP0(self, TR_ERRORS, "fail to retrieve group id from sequencer");
	xDestroy(sess);
	return ERR_XOBJ;
    }
    /*
     * Connect with the group's state (shared among sessions)
     */
    if ( mapResolve(ps->groupMap, &gid, &gg) == XK_FAILURE ) {
	gg = membershipCreateGState(self, gid);
	if (gg == (GState *)0) {
	    xTraceP0(self, TR_ERRORS, "GState allocation error");
	    xDestroy(sess);
	    return ERR_XOBJ;
	}
	/*
	 * Let the sequencer know that we do have a group map
	 */
	if (xControl(key->lls, SEQUENCER_SETGROUPMAP, (char *)&gg->node_map, sizeof(Map)) < 0) {
	    xTraceP0(self, TR_ERRORS, "xControl returns an error: I cannot set the population Map");
	    xDestroy(sess);
	    pathFree(gg);
	    return ERR_XOBJ;
	}
	(void)mapBind(ps->groupMap, (void *)&gid, (void *)gg, &bd); 
    } else {
	xAssert(gg->this_gid == gid);
    }

    ss->shared = gg;
    return sess;
}

xkern_return_t
membershipDemux(
		XObj 	self,
		XObj	lls,
		Msg 	msg)
{
    PState	         *ps = (PState *)self->state;
    XObj                 sess;
    membershipXchangeHdr hdr;
    Enable	         *en;
    ActiveKey            key;

    xTraceP0(self, TR_FULL_TRACE, "demux called");

    if ( msgPop(msg, membershipHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "membership demux -- msg pop failed");
	return XK_SUCCESS;
    }
    key.lls = lls;
    key.hlpNum = hdr.hlpNum;
    if ( mapResolve(ps->activeMap, (char *)&key, &sess) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.hlpNum, &en) == XK_FAILURE ) {
	    xTraceP1(self, TR_SOFT_ERRORS,
		    "demux -- no protl for group %d ", hdr.group);
	    return XK_FAILURE;
	} else {
	    xTraceP1(self, TR_DETAILED,
		    "demux -- found enable for group %d ", hdr.group);
	}
	/*
	 * Create a new session 
	 */
	sess = membershipCreateSessn(self, en->hlpRcv, en->hlpType, &key, msgGetPath(msg));
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
membershipHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg)
{
    xAssert( len == sizeof(membershipXchangeHdr) );
    bcopy(src, hdr, len);
    return len;
}

static void
membershipHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg)
{
    xAssert(len = sizeof(membershipXchangeHdr));
    bcopy((char *)hdr, dst, len);
}

static XObj
membershipOpen(
	      XObj	self,
	      XObj	hlpRcv,
	      XObj	hlpType,
	      Part	part,
	      Path	path)
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    ActiveKey	key;
    XObj        sess;
    boolean_t   buf;

    xTraceP0(self, TR_MAJOR_EVENTS, "open called");

    if ( (key.lls = xOpen(self, self, xGetDown(self, 0), part, path))
		== ERR_XOBJ ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "membership could not open lls");
	return ERR_XOBJ;
    }
    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTraceP0(self, TR_ERRORS, "heartbeat open couldn't get hlpNum");
	xClose(key.lls);
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &key, &sess) == XK_SUCCESS ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "membership open found existing sessn");
	xClose(key.lls);
	return sess;
    }
    buf = TRUE;
    if ( xControl(key.lls, SEQUENCER_SETNOTIFICATION, (char *)&buf, sizeof(buf)) < 0 ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "cannot set the request for asynchronous notifications");
	xClose(key.lls);
	return ERR_XOBJ;
    }
    sess = membershipCreateSessn(self, hlpRcv, hlpType, &key, path);
    if ( sess == ERR_XOBJ ) {
	xClose(key.lls);
	return ERR_XOBJ;
    }
    xTraceP1(self, TR_DETAILED, "membershipOpen returning %x", sess);

    return sess;
}

static xkern_return_t
membershipOpenDisable(
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
membershipOpenDisableAll(
			XObj	self,
			XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_MAJOR_EVENTS, "openDisableAll called");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

xkern_return_t
membershipOpenDone(
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
membershipOpenEnable(
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
membershipPop(
	     XObj	self,
	     XObj	lls,
	     Msg	message,
	     void	*hdr)
{
    PState               *ps = (PState *)xMyProtl(self)->state;
    SState	         *ss = (SState *)self->state;
    membershipXchangeHdr *mhdr = (membershipXchangeHdr *)hdr;
    sequencerXchangeHdr  *xhdr = msgGetAttr(message, 0);
    
    xTraceP0(self, TR_FULL_TRACE, "pop called");
    
    xAssert(xhdr);
    if (ss->shared->this_gid == ALL_NODES_GROUP) {
	switch (xhdr->type) {
	case SEQ_DATA:
	    if (ss->shared->initiator == TRUE) {
		/*
		 * This is a special case. Only one node can propose
		 * membership changes to the  ALL_NODES_GROUP (that is,
		 * the node with the census taker). This node (master!!) 
		 * must be waiting for the update to happen.
		 */
		ss->shared->initiator = FALSE;
		mhdr->xkr = xhdr->xkr;           /* carry the return code forward */
		xAssert(ss->shared->token_inprogress == mhdr->token);
	    } else {
		mhdr->xkr = XK_SUCCESS;
		/*
		 * scratch header to be modified while updating membership
		 */
		ss->shared->hdr_inprogress = *mhdr;
		
		xTraceP0(self, TR_FULL_TRACE, "pop: the message is stable");
		
		membershipUpdateState(self, message);
	    } 
	    if (mhdr->xkr == XK_SUCCESS) {
		xAssert(xhdr->seq != INVALID_SEQUENCE_NUMBER);
		if (xControl(lls, SETINCARNATION, (char *) &xhdr->seq, sizeof(seqno)) < 0) {
		    xTraceP1(self, TR_ERRORS, "fail to bump the incarnation into the sequencer (%d)", xhdr->seq);
		    xAssert(0);
		}
		xTraceP1(self, TR_ERRORS, "bumped the incarnation number into the sequencer (%d)", xhdr->seq);
	    }
	    break;
	case SEQ_NOTIFICATION:
	    xAssert(ss->shared->initiator == FALSE);
	    /*
	     * Update the fields of the membershipXchangeHdr
	     * (typically, we're using an old one to go upstream)
	     */
	    xAssert(mhdr->group == ALL_NODES_GROUP);
	    mhdr->type = MASTERCHANGE;
	    break;
	default:
	    xAssert(0);
	}
    } else {
	/*
	 * MORE WORK
	 */
	xAssert(0);
    }
    /*
     * Replace the sequencer's XchangeHdr with the membership XchangHdr
     */
    msgSetAttr(message, 0, mhdr, sizeof(membershipXchangeHdr));
    return xDemux(self, message);
}

static int
membershipProtControl(
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

static xmsg_handle_t
membershipPush(
	       XObj	self,
	       Msg	message)
{
    SState	         *ss = (SState *)self->state;
    PState               *ps = (PState *)xMyProtl(self)->state;
    membershipXchangeHdr *xhdr = msgGetAttr(message, 0);
    XObj                 lls = xGetDown(self, 0);
    IPhost               master;

    /*
     * message describes a population according to the Xchange
     * format described in include/prot/membership.h
     */
    xTraceP0(self, TR_FULL_TRACE, "push called");
    if (ss->shared->initiator) {
	xTraceP0(self, TR_ERRORS, "push returns an error as the session isn't stable");
	return XMSG_ERR_HANDLE;
    }
    /*
     * Is this the master node for this group?
     */
    if (xControl(lls, GETMASTER, (char *)&master, sizeof(master)) < sizeof(master)) {
	xTraceP0(self, TR_ERRORS, "xControl returns an error: I can't tell whether I'm master");
	return XMSG_ERR_HANDLE;
    }

    if (!IP_EQUAL(master, ps->thishost)) {
	/*
	 * Membership changes can only be initiated at the node which plays
	 * the role of master sequencer.
	 */
	xTraceP0(self, TR_ERRORS, "I can't change membership if I'm not the master");
	return XMSG_ERR_HANDLE;
    }
    ss->shared->initiator = TRUE;
    ss->shared->token_inprogress = xhdr->token;
    xhdr->group = ss->shared->this_gid;
    xhdr->hlpNum = ss->hlpNum;
    
    /*
     * Update the state of the group local to this node
     * (using the scratch header ss->shared->hdr_inprogress).
     * The sequencer needs it to understand whether the
     * message is delivered to the right members for that
     * group.
     */
    ss->shared->hdr_inprogress = *xhdr;
    membershipUpdateState(self, message);
    /*
     * We can now compress the XchangeHdr into the message
     */
    if ( msgPush(message, membershipHdrStore, xhdr, sizeof(*xhdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "msg push fails");
    }
    
    if ( xPush(lls, message) == XMSG_ERR_HANDLE ) {
	xTraceP0(self, TR_ERRORS, "push to down session failed");
    }

    return XMSG_NULL_HANDLE;
}

static int
membershipSessControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    SState	*ss = (SState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "control called");

    switch (op) {

    default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}

xkern_return_t
membership_init(
	       XObj self)
{
    XObj	llp;
    PState      *ps;
    IPhost      master;
    
    xTraceP0(self, TR_FULL_TRACE, "init called");

    llp = xGetDown(self, 0);
    if (!xIsProtocol(llp)) {
	xTraceP0(self, TR_ERRORS, "no lower protocol");
	return XK_FAILURE;
    }

    self->open = membershipOpen;
    self->demux = membershipDemux;
    self->opendone = membershipOpenDone;
    self->control = membershipProtControl;
    self->openenable = membershipOpenEnable;
    self->opendisable = membershipOpenDisable;
    self->opendisableall = membershipOpenDisableAll;
    self->hlpType = self;

    if (!(self->state = ps = pathAlloc(self->path, sizeof(PState)))) {
	xTraceP0(self, TR_ERRORS, "state allocation error in init");
	return XK_FAILURE;
    }

    /*
     * setting up the PState
     */
    ps->max_groups = DEFAULT_MAX_GROUPS;
    ps->max_nodes  = DEFAULT_MAX_NODES;
    ps->timestamp = 0;
    semInit(&ps->serviceSem, 1);
    findXObjRomOpts(self, membershipOpts, sizeof(membershipOpts)/sizeof(XObjRomOpt), 0);

    ps->activeMap = mapCreate(MEMBERSHIP_ACTIVE_MAP_SIZE, sizeof(ActiveKey),
			       self->path);
    ps->passiveMap = mapCreate(MEMBERSHIP_PASSIVE_MAP_SIZE, sizeof(PassiveKey),
			       self->path);
    ps->groupMap = mapCreate(ps->max_groups, sizeof(group_id), self->path);
    ps->node2groups = mapCreate(ps->max_nodes, sizeof(IPhost), self->path);

    if ( ! ps->activeMap || ! ps->passiveMap || ! ps->groupMap || ! ps->node2groups ) {
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
    /*
     * As part of the bootstrapping procedure, if I'm not running on
     * the master node I have to pre-populate the ps->node2groups map
     * with two node entries, the sequencer and mynode. These entries
     * may be modified when the first, real membership message get
     * installed and become stable. I have also to create and bind
     * a GState for ALL_NODES_GROUP.
     */
    if (xControl(xGetDown(self, 0), GETMASTER, (char *)&master, sizeof(master)) < sizeof(master)) {
	xTraceP0(self, TR_ERRORS, "xControl returns an error: I can't tell whether I'm master");
	pathFree(ps);
	return XK_FAILURE;
    }

    if (!IP_EQUAL(master, ps->thishost)) {
	node_element  *el1, *el2;
	Bind          bd;
	group_id      gid;
	GState        *gg;

	el1 = (node_element *)pathAlloc(self->path, sizeof(node_element));
	if (!el1) {
	    xTraceP0(self, TR_ERRORS, "fail to create node map bootstrap entries");
	    pathFree(ps);
	    return XK_FAILURE;
	}
	dlist_init(&el1->dl);
	el1->session = self;
	el1->timestamp = ps->timestamp;
	el1->touched = FALSE;           
	(void)mapBind(ps->node2groups, (void *)&ps->thishost, (void *)el1, &bd);

	el2 = (node_element *)pathAlloc(self->path, sizeof(node_element));
	if (!el2) {
	    xTraceP0(self, TR_ERRORS, "fail to create node map bootstrap entries");
	    pathFree(el1);
	    pathFree(ps);
	    return XK_FAILURE;
	}
	dlist_init(&el2->dl);
	el2->session = self;
	el2->timestamp = ps->timestamp;
	el2->touched = FALSE;           
	(void)mapBind(ps->node2groups, (void *)&master, (void *)el2, &bd);

	gid = ALL_NODES_GROUP;
	gg = membershipCreateGState(self, gid);
	if (gg == (GState *)0) {
	    xTraceP0(self, TR_ERRORS, "GState allocation error");
	    pathFree(ps);
	    return XK_FAILURE;
	}
	if (xControl(xGetDown(self, 0), SEQUENCER_SETGROUPMAP, (char *)&gg->node_map, sizeof(Map)) < 0) {
	    xTraceP0(self, TR_ERRORS, "xControl returns an error: I cannot set the population Map");
	    pathFree(ps);
	    return XK_FAILURE;
	}
	(void)mapBind(ps->groupMap, (void *)&gid, (void *)gg, &bd); 
    }   
    
    if (xOpenEnable(self, self, llp, (Part) 0)) {
	xTraceP0(self, TR_ERRORS, "openenable failed in init");
	pathFree(ps);
	return XK_FAILURE;
    }

    return XK_SUCCESS;
}

node_element *
membershipUpdateAddition(
			 XObj   self,
			 IPhost node)
{
    PState               *ps = (PState *)xMyProtl(self)->state;
    SState	         *ss = (SState *)self->state;
    node_element         *el;
    Bind                 bd;

    el = (node_element *)pathAlloc(self->path, sizeof(node_element));
    if (!el) {
	xTraceP1(self, TR_ERRORS, "fail to create entry for node %s", 
		ipHostStr(&node));
	return el;
    }
    memset((char *)el, 0, sizeof(node_element));
    dlist_init(&el->dl);
    el->session = self;
    el->timestamp = ss->timestamp;
    el->touched = FALSE;            /* it may be set later, if needed */
    (void)mapBind(ss->shared->node_map, (void *)&node, (void *)el, &bd);
    return el;
}

xkern_return_t
membershipUpdateDeletion(
			 XObj   self,
			 IPhost node)
{
    PState               *ps = (PState *)xMyProtl(self)->state;
    SState	         *ss = (SState *)self->state;
    node_element         *el, *nel;
    xkern_return_t       xkr;
	    
    xkr = mapResolve(ss->shared->node_map, (void *) &node, (void *) &el);
    xAssert(xkr == XK_SUCCESS);
    dlist_iterate(&el->dl, nel, node_element *) {
	/*
	 * We are tearing down a node which was part of other groups.
	 * Verify that the timestamp match with the current session's one.
	 * If it does, notify the session. Free the storage in any case.
	 */
	/*
	 * MORE WORK
	 */
	xAssert(0);
    }
    xkr = mapUnbind(ss->shared->node_map, (void *) &node);
    xAssert(xkr == XK_SUCCESS);

    pathFree(el);
    return XK_SUCCESS;
}

static boolean_t
membershipUpdateInternal(
			 char *buf,
			 long int len,
			 void *foo)
{
    XObj                 self = (XObj)foo;
    PState               *ps = (PState *)xMyProtl(self)->state;
    SState	         *ss = (SState *)self->state;
    membershipXchangeHdr *hdr = &ss->shared->hdr_inprogress;
    IPhost               *node = (IPhost *)buf;
    int                  i;
    node_element         *el;
    xkern_return_t       xkr;

    for (i = 0; i < len; i += sizeof(IPhost), node++) {
	if (hdr->type == FULL) {
	    xkr = mapResolve(ss->shared->node_map, (void *) node, (void *) &el);
	    if (xkr == XK_FAILURE) {
		el = membershipUpdateAddition(self, *node);
		xAssert(el);
		xTraceP1(self, TR_ERRORS, "adding an entry for node %s", ipHostStr(node));
	    } else {
		xAssert(el->touched == FALSE);
	    }
	    el->touched = TRUE;
	} else {
	    if (hdr->update_count.rel.additions) {
		el = membershipUpdateAddition(self, *node);
		xTraceP1(self, TR_ERRORS, "adding an entry for node %s", ipHostStr(node));
		hdr->update_count.rel.additions--;
	    } 
	    if (hdr->update_count.rel.deletions) {
		xkr = membershipUpdateDeletion(self, *node);
		xAssert(xkr == XK_SUCCESS);
		xTraceP1(self, TR_ERRORS, "deleting an entry for node %s", ipHostStr(node));
		hdr->update_count.rel.deletions--;
	    }
	}
    }

    return TRUE;
}

void
membershipUpdateState(
		      XObj self,
		      Msg  message)
{
    SState	         *ss = (SState *)self->state;
    XObj                 lls = xGetDown(self, 0);

    xTraceP0(self, TR_ERRORS, "updating the state");

    msgForEach(message, membershipUpdateInternal, self);
    ss->shared->gcount = 0;
    mapForEach(ss->shared->node_map, membershipZapEntries, self);

    xAssert(ss->shared->gcount >= 1);
    if (xControl(lls, SEQUENCER_SETPOPULATION, (char *) &ss->shared->gcount, sizeof(u_long)) < 0) {
	xTraceP0(self, TR_ERRORS, "fail to install the number of members into the sequencer");
	xAssert(0);
    }
    xTraceP1(self, TR_ERRORS, "installed the number of members into the sequencer (%d)", ss->shared->gcount);
}

static int
membershipZapEntries(
		     void	*key,
		     void	*val,
		     void	*arg)
{
    XObj                 self = (XObj)arg;
    SState	         *ss = (SState *)self->state;
    membershipXchangeHdr *hdr = &ss->shared->hdr_inprogress;
    IPhost               *node = (IPhost *)key;
    node_element         *el = (node_element *)val;
    int                  ret = MFE_CONTINUE;
    xkern_return_t       xkr;

    ss->shared->gcount++;
    if (hdr->type == FULL) {
	if (!el->touched) {
	    xkr = membershipUpdateDeletion(self, *node);
	    xAssert(xkr == XK_SUCCESS);
	    xTraceP1(self, TR_ERRORS, "deleting an entry for node %s", ipHostStr(node));
	    ret |= MFE_REMOVE;
	    ss->shared->gcount--;
	}
	el->touched = FALSE;
    }
    return ret;
}

static xkern_return_t   
LoadMaxGroups(
	      XObj	self,
	      char	**str,
	      int	nFields,
	      int	line,
	      void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "max groups (%s) specified", str[2]);

    ps->max_groups = atoi(str[2]);
    return XK_SUCCESS;
}

static xkern_return_t   
LoadMaxNodes(
	     XObj	self,
	     char	**str,
	     int	nFields,
	     int	line,
	     void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "max nodes (%s) specified", str[2]);

    ps->max_nodes = atoi(str[2]);
    return XK_SUCCESS;
}






