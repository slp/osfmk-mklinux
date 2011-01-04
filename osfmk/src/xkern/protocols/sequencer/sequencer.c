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
#include <xkern/include/prot/sequencer.h>
#include <xkern/include/prot/membership.h>
#include <xkern/include/prot/bootp.h>
#include <xkern/protocols/sequencer/sequencer_i.h>

#if XK_DEBUG
int tracesequencerp;
#endif /* XK_DEBUG */

#define SEQUENCER_MCAST 1
#define SEQUENCER_BCAST 0
#define SEQUENCER_UCAST 0

static IPhost   ipNull = { 0, 0, 0, 0 };

static XObjRomOpt	sequencerOpts[] =
{
     { "initial_master", 3, LoadMaster },
     { "mcast_address", 3, LoadMcastAddress },
     { "max_groups", 3, LoadMaxGroups },
     { "max_history_entries", 3, LoadMaxHistoryEntries },
     { "retrans_max", 3, LoadRetransMax },
     { "retrans_msecs", 3, LoadRetransMsecs }
};


static MState *
getms(
      Map    map,
      IPhost node)
{
    xkern_return_t       xkr;
    node_element         *el;

    xAssert(map);
    xkr = mapResolve(map, (void *) &node, (void *) &el);
    if (xkr != XK_SUCCESS)
	return (MState *)0;

    return (MState *)&el->opaque[0];
}

static int
getnewmaster_entries(
		     void	*key,
		     void	*val,
		     void	*arg)
{
    IPhost         *node  = (IPhost *)key;
    node_element   *element = (node_element *)val;
    Election       *elect = (Election *)arg;
    IPhost         *newnode = &elect->new;

    if (!IP_EQUAL(*node, elect->old) && *(u_bit32_t *)node > *(u_bit32_t *)newnode) {
	*newnode = *node;
    }
    return MFE_CONTINUE;
}

static IPhost
getnewmaster(
	     Map    map,
	     IPhost oldmaster)
{
    Election  elect;

    xAssert(map);
    xTrace1(sequencerp, TR_EVENTS, "old master is (%s)", ipHostStr(&oldmaster));

    elect.old = oldmaster;
    elect.new = ipNull;
    mapForEach(map, getnewmaster_entries, &elect);

    xTrace1(sequencerp, TR_EVENTS, "new master is (%s)", ipHostStr(&elect.new));
    xAssert(!IP_EQUAL(elect.new, oldmaster) && !IP_EQUAL(elect.new, ipNull));

    return elect.new;
}
	     
static xkern_return_t
getSessnFuncs(
	      XObj	s)
{
    s->pop = sequencerPop;
    s->push = sequencerPush;
    s->close = sequencerClose;
    s->control = sequencerSessControl;
    return XK_SUCCESS;
}

static xkern_return_t   
LoadMaster(
	      XObj	self,
	      char	**str,
	      int	nFields,
	      int	line,
	      void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    if ( str2ipHost(&ps->master, str[2]) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    xTraceP1(self, TR_EVENTS, "initial master is (%s)", ipHostStr(&ps->master));

    return XK_SUCCESS;
}

static xkern_return_t   
LoadMcastAddress(
		 XObj	self,
		 char	**str,
		 int	nFields,
		 int	line,
		 void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    if ( str2ipHost(&ps->mcaddr, str[2]) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    xTraceP1(self, TR_EVENTS, "initial master is (%s)", ipHostStr(&ps->mcaddr));

    return XK_SUCCESS;
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
LoadMaxHistoryEntries(
		      XObj	self,
		      char	**str,
		      int	nFields,
		      int	line,
		      void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "max history entries (%s) specified", str[2]);

    ps->max_history_entries = atoi(str[2]);
    return XK_SUCCESS;
}

static xkern_return_t   
LoadRetransMax(
	       XObj	self,
	       char	**str,
	       int	nFields,
	       int	line,
	       void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "max retransmissions (%s) specified", str[2]);

    ps->retrans_max = atoi(str[2]);
    return XK_SUCCESS;
}

static xkern_return_t   
LoadRetransMsecs(
		 XObj	self,
		 char	**str,
		 int	nFields,
		 int	line,
		 void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "retransmission interval (%s) specified", str[2]);

    ps->retrans_msecs = atoi(str[2]);
    return XK_SUCCESS;
}

static void
sequencerAlert(
	       Event	ev,
	       void 	*arg)
{
    XObj         self = (XObj)arg;
    PState       *ps = (PState *)self->state;
    
    if (IP_EQUAL(ps->master, ipNull)) {
	/*
	 * Then let's pretend to be the MASTER!
	 */
	ps->master = ps->thishost;
	xTraceP0(self, TR_FULL_TRACE, "No life signal. We are the master");
	semSignal(&ps->serviceSem);
    }
}

static int
sequencerAsynNotif(
		   void	*key,
		   void	*val,
		   void	*arg)
{
    XObj                sess = (XObj)val;
    SState              *ss = (SState *)sess->state;
    GState              *gg = (GState *)arg;
    GHistory            *his = ss->his;
    sequencerXchangeHdr xhdr;

    if (gg->this_gid != ALL_NODES_GROUP) {
	/* MORE WORK */
	return MFE_CONTINUE;
    }
    if (his) {
	xTraceP0(sess, TR_FULL_TRACE, "signal error and clear out our own temporary history entry");
	xhdr.type = SEQ_DATA;
	xhdr.xkr  = XK_FAILURE;
	xhdr.seq  = INVALID_SEQUENCE_NUMBER;
	msgSetAttr(&his->i_message, 0, &xhdr, sizeof(xhdr));
	xDemux(sess, &his->i_message);
	msgDestroy(&his->i_message);
	evDetach(his->i_ev);
	pathFree(his);
	ss->his = (GHistory *)0;
	/* we can accept new requests, now */
	semSignal(&ss->sessionSem);
    }
    if (ss->asynnot && ss->asynmsg) {
	xTraceP1(sess, TR_EVENTS, "pop a notification for XObj %x", sess);
	xhdr.type = SEQ_NOTIFICATION;
	xhdr.xkr  = XK_SUCCESS;
	xhdr.seq  = INVALID_SEQUENCE_NUMBER;
	msgSetAttr(ss->asynmsg, 0, &xhdr, sizeof(xhdr));

	xDemux(sess, ss->asynmsg);
	msgDestroy(ss->asynmsg);
	pathFree(ss->asynmsg);
	ss->asynmsg = (Msg) 0;
    }
    return MFE_CONTINUE;
}

static xkern_return_t
sequencerClose(
	       XObj	self)
{
    XObj	myProtl = xMyProtl(self);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "sequencer Close");

    if ( mapRemoveBinding(ps->activeMap, self->binding) == XK_FAILURE ) {
	xAssert(0);
	return XK_FAILURE;
    }
    xDestroy(self);
    return XK_SUCCESS;
}

GState *
sequencerCreateGState(
		      XObj     self,
		      group_id gid)
{
    PState	 *ps = (PState *)self->state;
    GState       *gg;
    Bind         bd;
    Part_s       part[1];
    unsigned int unused;
    char         *buf;
    GHistory     *his;

    if (!(gg = (GState *)pathAlloc(self->path, sizeof(GState)))) {
	xTraceP0(self, TR_ERRORS, "allocation error for GState");
	return (GState *)0;
    }
    memset((char *)gg, 0, sizeof(GState));
    gg->this_gid = gid;
    gg->address = sequencerGroupAddress(self, gid);
    gg->group_incarnation = INCARNATION_WILD_CARD; /* anything is good */
    gg->state = MASTER_OR_SLAVE_IDLE;
    gg->delay = DEFAULT_DELAY;
    xAssert(!IP_EQUAL(gg->address, ipNull));

    if (!(gg->gc_ev = evAlloc(self->path))) {
	xTraceP0(self, TR_ERRORS, "garbage collection Event allocation error");
	pathFree(gg);
	return (GState *)0;
    }

    xAssert(ps->max_history_entries);
    /*
     * We may add something smarter here to
     * scale group_history_entries as a 
     * F(ps->max_history_entries).
     */
    gg->group_history_entries = ps->max_history_entries;
    if ( ! (gg->cbuffer = (GHistory *)pathAlloc(self->path, 
						gg->group_history_entries * sizeof(GHistory))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error for history circular buffer");
	evDetach(gg->gc_ev);
	pathFree(gg);
	return (GState *)0;
    }
    /* 
     * Anything is initialized to 0 (i_mode == EMPTY)
     */
    memset((char *)gg->cbuffer, 0, gg->group_history_entries * sizeof(GHistory));
    for (his = gg->cbuffer; his < (gg->cbuffer + gg->group_history_entries); his++) {
	if (!(his->i_ev = evAlloc(self->path))) {
	    xTraceP0(self, TR_ERRORS, "Retry Event allocation error");
	    evDetach(gg->gc_ev);
	    pathFree(gg->cbuffer);
	    pathFree(gg);
	    return (GState *)0;
	}
    }
    
    partInit(&part[0], 1); 
    partPush(*part, &gg->address, sizeof(IPhost));
    
    gg->group_lls = xOpen(self, self, xGetDown(self, SEQ_TRANSPORT), part, self->path);
    if (gg->group_lls == ERR_XOBJ) {
	xTraceP0(self, TR_ERRORS, "cannot open group session");
	evDetach(gg->gc_ev);
	pathFree(gg->cbuffer);
	pathFree(gg);
	return (GState *)0;
    }

    if (!IP_EQUAL(ps->master, ps->thishost)) {
	partInit(&part[0], 1);
	partPush(*part, &ps->master, sizeof(IPhost));
	gg->master_lls = xOpen(self, self, xGetDown(self, SEQ_TRANSPORT), part, self->path);
	if (gg->master_lls == ERR_XOBJ) {
	    xTraceP0(self, TR_ERRORS, "cannot open master-host session");
	    evDetach(gg->gc_ev);
	    xClose(gg->group_lls);
	    pathFree(gg->cbuffer);
	    pathFree(gg);
	    return (GState *)0;
	}
	gg->watchdog = SLAVE_DEFAULT_WATCHDOG;
    } else {
	gg->master_lls = ERR_XOBJ;
	gg->watchdog = MASTER_DEFAULT_WATCHDOG;
    }
    
    /*
     * In the specific case of ALL_NODES_GROUP, 
     * I resort to the ps->node2groups map. Note
     * that it may not be set as well (membership
     * will fix things properly).
     */
    if (gid == ALL_NODES_GROUP) {
	if (!(gg->live_ev = evAlloc(self->path))) {
	    xTraceP0(self, TR_ERRORS, "sequencer master liveness allocation error");
	    evDetach(gg->gc_ev);
	    xClose(gg->group_lls);
	    if (gg->master_lls != ERR_XOBJ)
		xClose(gg->master_lls);
	    pathFree(gg->cbuffer);
	    pathFree(gg);
	    return (GState *)0;
	}
	gg->node_map = ps->node2groups;
    }
    
    evSchedule(gg->gc_ev, sequencerSweepHistory, gg, SEQUENCER_SWEEP_HISTORY * 1000);
    return gg;
}

static XObj
sequencerCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path)
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    XObj        sess;
    GState      *gg;
    Bind        bd;
    
    semWait(&ps->serviceSem);
    if ( mapResolve(ps->activeMap, key, &sess) == XK_FAILURE ) {
	sess = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 0, (XObj *)0, path);
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
    memset(ss, 0, sizeof(SState));
    ss->hlpNum = key->hlpNum;
    semInit(&ss->sessionSem, 1);
    
    /*
     * Connect with the group's state (shared among sessions)
     */
    if ( mapResolve(ps->groupMap, &key->group, &gg) == XK_FAILURE ) {
	gg = sequencerCreateGState(self, key->group);
	if (gg == (GState *)0) {
	    xTraceP0(self, TR_ERRORS, "GState allocation error");
	    xDestroy(sess);
	    return ERR_XOBJ;
	}
	(void)mapBind(ps->groupMap, (void *)&key->group, (void *)gg, &bd); 
	if (key->group == ALL_NODES_GROUP) {
	    xDuplicate(sess);
	    evSchedule(gg->live_ev, sequencerMasterAlive, sess, gg->watchdog * 1000);
	}
    } else {
	xAssert(gg->this_gid == key->group);
	xAssert(!IP_EQUAL(gg->address, ipNull));
	xAssert(gg->group_lls && gg->group_lls != ERR_XOBJ);
	xAssert((gg->master_lls != ERR_XOBJ && !IP_EQUAL(ps->master, ps->thishost)) || 
		(gg->master_lls == ERR_XOBJ && IP_EQUAL(ps->master, ps->thishost)));
    }

    /*
     * Connect the two other states (GState and MState) to SState.
     */
    xAssert(gg);
    ss->shared = gg;

    return sess;
}

static xkern_return_t
sequencerDemux(
		XObj 	self,
		XObj	lls,
		Msg 	msg)
{
    PState	        *ps = (PState *)self->state;
    XObj                sess;
    sequencerHdr        hdr;
    Enable	        *en;
    ActiveKey           key;

    xTraceP0(self, TR_FULL_TRACE, "demux called");

    if ( msgPop(msg, sequencerHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "sequencer demux -- msg pop failed");
	return XK_SUCCESS;
    }
    key.hlpNum = hdr.h_hlpNum;
    key.group = hdr.h_gid;
    if ( mapResolve(ps->activeMap, (char *)&key, &sess) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.h_hlpNum, &en) == XK_FAILURE ) {
	    xTraceP1(self, TR_SOFT_ERRORS,
		    "demux -- no protl for Prot %d ", hdr.h_hlpNum);
	    return XK_FAILURE;
	} else {
	    xTraceP1(self, TR_DETAILED,
		    "demux -- found enable for Prot %d ", hdr.h_hlpNum);
	}
	/*
	 * Create a new session 
	 */
	sess = sequencerCreateSessn(self, en->hlpRcv, en->hlpType, &key, msgGetPath(msg));
	if ( sess == ERR_XOBJ ) {
	    return XK_FAILURE;
	}
	/*
	 * We hold a reference to the newly create session. 
	 * It won't disappear on us (timeouts and the like) 
	 */	 
	xDuplicate(sess);
	xOpenDone(en->hlpRcv, sess, self, msgGetPath(msg));
    } else {
	xAssert(sess->rcnt > 0);
    }
    return xPop(sess, lls, msg, &hdr);
}

static xkern_return_t
sequencerDemuxBootstrap(
			XObj 	self,
			XObj	lls,
			Msg 	msg)
{
    PState	        *ps = (PState *)self->state;
    XObj                sess;
    sequencerHdr        hdr;
    Enable	        *en;
    ActiveKey           key;

    xTraceP0(self, TR_FULL_TRACE, "demuxBootstrap called");

    if ( msgPop(msg, sequencerHdrLoad, (void *)&hdr, sizeof(hdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "sequencer demuxBootstrap -- msg pop failed");
	return XK_SUCCESS;
    }
    switch (hdr.h_type) {
    case SEQUENCER_BROADCAST:
    case SEQUENCER_ACCEPT:
    case SEQUENCER_SYNC_REPLY:
    case SEQUENCER_MASTER_ALIVE:
	if (IP_EQUAL(ps->master, ipNull)) {
	    xTraceP1(self, TR_FULL_TRACE, "sequencer demuxBootstrap -- found master to be %s", 
		     ipHostStr(&hdr.h_name));
	    ps->master = hdr.h_name;
	    semSignal(&ps->serviceSem);
	}
    default:
	xTraceP1(self, TR_ERRORS, "sequencer demuxBootstrap -- ignoring message type %d",
		 hdr.h_type);
    }
    return XK_SUCCESS;
}

static void
sequencerError(
	       XObj      self,
	       GHistory  *his,
	       boolean_t istimeout)
{
    XObj                 thisp = xMyProtl(self);
    PState               *ps = (PState *)thisp->state;
    SState	         *ss = (SState *)self->state;
    GState               *gg = ss->shared;
    ActiveKey            key;
    sequencerXchangeHdr  xhdr;

    xTraceP0(self, TR_FULL_TRACE, "error handler for !master");
    
    xAssert(!IP_EQUAL(ps->master, ps->thishost));

    if (gg->this_gid != ALL_NODES_GROUP) {
	/* MORE WORK */
	return;
    }
    /* 
     * We must revert the master!
     */
    ps->master = getnewmaster(gg->node_map, ps->master);
    if (IP_EQUAL(ps->master, ps->thishost)) {
	xTraceP0(self, TR_FULL_TRACE, "we are the new master");
	xClose(gg->master_lls);
	gg->master_lls = ERR_XOBJ;
	gg->watchdog = MASTER_DEFAULT_WATCHDOG;
	/* MORE WORK */
    } else {
	Part_s       part[1];

	if (gg->master_lls != ERR_XOBJ)
	    xClose(gg->master_lls);
	partInit(&part[0], 1);
	partPush(*part, &ps->master, sizeof(IPhost));
	gg->master_lls = xOpen(thisp, thisp,  xGetDown(thisp, SEQ_TRANSPORT), part, thisp->path);
	xAssert(gg->master_lls != ERR_XOBJ);
	gg->watchdog = SLAVE_DEFAULT_WATCHDOG;
	/* MORE WORK */
    }

    /*
     * Clear up state and send a notification upstream 
     * to the sessions that have requested it.
     */
    mapForEach(ps->activeMap, sequencerAsynNotif, gg);
    gg->state = MASTER_OR_SLAVE_IDLE;
}

/*
 * The master sequencer has stumbled on a seqno that did 
 * not go through. 
 */
static void
sequencerErrorMaster(
		     XObj      self,
		     GHistory  *his,
		     boolean_t istimeout)
{
    PState              *ps = (PState *)xMyProtl(self)->state;
    SState	        *ss = (SState *)self->state;
    GState              *gg = ss->shared;
    sequencerXchangeHdr xhdr;

    xTraceP0(self, TR_FULL_TRACE, "error handler for master");
    
    xAssert(IP_EQUAL(ps->master, ps->thishost));
    /*
     * While we're cleaning up the history, we prevent any
     * new traffic from accumulating.
     */
    xAssert(his >= &gg->cbuffer[gg->consumer % gg->group_history_entries]);
    xAssert(his < &gg->cbuffer[gg->producer % gg->group_history_entries]);

    if (his->i_mode == UNSTABLE) {
	if (IP_EQUAL(his->i_hdr.h_name_initiator, ps->thishost)) {
	    /*
	     * Message originated here.
	     */
	    xTraceP0(self, TR_FULL_TRACE, "return message to the sender with error code");
	    /*
	     * Report the error. For now, just a return code.
	     * In the future, we want to have a list of group
	     * members that did not reply (as a hint to the
	     * sequencer clients).
	     * MORE WORK
	     */
	    xhdr.type = SEQ_DATA;
	    xhdr.xkr = XK_FAILURE;
	    xhdr.seq = INVALID_SEQUENCE_NUMBER;
	    msgSetAttr(&his->i_message, 0, &xhdr, sizeof(xhdr));
	    /*
	     * Note that I do not bump up the group number.
	     * The next packet in our log will be played with
	     * the same seqno.
	     */
	    gg->state = MASTER_OR_SLAVE_IDLE;
	    xDemux(self, &his->i_message);
	    msgDestroy(&his->i_message);
	} else {
	    /*
	     * Message originated elsewhere.
	     */
	    xTraceP0(self, TR_FULL_TRACE, "dropping message silently");
	}
    }
    /* 
     * Ready to do business again on the new incarnation 
     */
}

static void
sequencerEventReturn(
		     Event     ev,
		     void      *arg)
{
    XObj                self = (XObj)arg;
    PState              *ps = (PState *)xMyProtl(self)->state;
    SState	        *ss = (SState *)self->state;
    GState              *gg = ss->shared;
    GHistory            *his;
    sequencerXchangeHdr xhdr;
    
    xTraceP0(self, TR_FULL_TRACE, "a message is being shorcircuited back to sender");

    his = ss->his;
    his->i_mode = STABLE;
    his->i_refcount = 0;
    xhdr.type = SEQ_DATA;
    xhdr.xkr = XK_SUCCESS;
    xhdr.seq = his->i_hdr.h_seqno;
    msgSetAttr(&his->i_message, 0, &xhdr, sizeof(xhdr));
    xAssert(xhdr.seq == gg->group_seqno);
    gg->group_seqno = xhdr.seq + 1;
    xDemux(self, &his->i_message);
    /*
     * The message will be destroied when 
     * sequencerPruneHistory fires off.
     */
}

static IPhost
sequencerGroupAddress(
		      XObj     self,
		      group_id gid)
{
    PState	 *ps = (PState *)self->state;
    /*
     * What's the IP number associated to this group?
     * Eventually, our groups will be registered at 
     * DNS, with a class D address that will allow
     * us to use IP multicast. 
     * Note that any node must join the ALL_NODES_GROUP
     * multicast address, at boot time.
     */
    /*
     * MORE WORK
     */
#if   SEQUENCER_MCAST
    return ps->mcaddr;
#elif SEQUENCER_BCAST
    return IP_LOCAL_BCAST;
#elif SEQUENCER_UCAST
    IPhost libra6 = {130, 105, 3, 61}; /* XXX */

    return libra6;
#else  /* SEQUENCER_{M,B,U}CAST */
#error Undefined SEQUENCER_TYPE_OF_CAST
#endif /* SEQUENCER_{M,B,U}CAST */
}

static long
sequencerHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg)
{
    xAssert( len == sizeof(sequencerHdr) );
    bcopy(src, hdr, len);
    return len;
}

static void
sequencerHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg)
{
    xAssert(len == sizeof(sequencerHdr));
    bcopy((char *)hdr, dst, len);
}

static void
sequencerMasterAlive(
		     Event	ev,
		     void 	*arg)
{
    XObj             self = (XObj)arg;
    PState           *ps = (PState *)xMyProtl(self)->state;
    SState	     *ss = (SState *)self->state;
    GState           *gg = ss->shared;
    static u_bit32_t last_seqno;
    static u_long    last_idle_ticks;

    if (evIsCancelled(ev)) {
	xTraceP0(self, TR_FULL_TRACE, "ALIVE timeout cancelled: no-op");
	return;
    } 

    xTraceP0(self, TR_FULL_TRACE, "ALIVE timeout");

    if (last_seqno == gg->group_seqno && gg->state == MASTER_OR_SLAVE_IDLE) {
	/*
	 * Nothing has happened in a while
	 */
	if (IP_EQUAL(ps->thishost, ps->master)) {
	    if (sequencerOutputControl(self, SEQUENCER_MASTER_ALIVE, 
					  (u_bit32_t)-1, TRUE) != XMSG_NULL_HANDLE) {
		xTraceP0(self, TR_FULL_TRACE, "failure to diffuse a MasterAlive. Next time 'round");
	    }
	} else {
	    if (last_idle_ticks == gg->idle_ticks) {
		sequencerError(self, &gg->cbuffer[(gg->producer - 1) % gg->group_history_entries], FALSE);
	    } else
		last_idle_ticks = gg->idle_ticks;
	}
    } else
	last_seqno = gg->group_seqno;

    evSchedule(ev, sequencerMasterAlive, self, gg->watchdog * 1000);
}

static XObj
sequencerOpen(
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
    group_id    *pgid;

    xTraceP0(self, TR_MAJOR_EVENTS, "open called");

    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTraceP0(self, TR_ERRORS, "sequencer open couldn't get hlpNum");
	return ERR_XOBJ;
    }
    if ( part == 0 || partLen(part) < 1 ) {
	xTraceP0(self, TR_ERRORS, "sequencer Open -- bad participants");
	return ERR_XOBJ;
    }
    pgid = (group_id *)partPop(*part);
    key.group = *pgid;

    if ( mapResolve(ps->activeMap, &key, &sess) == XK_SUCCESS ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "sequencer open found existing sessn");
	return sess;
    }
    sess = sequencerCreateSessn(self, hlpRcv, hlpType, &key, path);
    if ( sess == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    xTraceP1(self, TR_DETAILED, "sequencerOpen returning %x", sess);

    return sess;
}

static xkern_return_t
sequencerOpenDisable(
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
sequencerOpenDisableAll(
			XObj	self,
			XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_MAJOR_EVENTS, "openDisableAll called");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

static xkern_return_t
sequencerOpenDone(
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
sequencerOpenEnable(
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

/*
 * Two distinct behaviors within the same procedure:
 *     1. If we are not the master, message must be set to be a valid
 *        message. This message gets sent to the master.
 *     2. If we are the master, and the message is set to be a valid
 *        message, we can either a) send the message or b) queue it 
 *        or c) queue it and send another message from the waiting list.
 *        If the message is not set to be a valid message, then we can
 *        only do d), send another message from the waiting list or
 *        e) do nothing.
 */
static xmsg_handle_t
sequencerOutput(
		XObj	self,
		Msg	message,
		IPhost  *node)
{
    SState	         *ss = (SState *)self->state;
    PState	         *ps = (PState *)xMyProtl(self)->state;
    GState               *gg = ss->shared;
    MState               *myms;
    Msg_s                lmessage;
    sequencerHdr         *hdr;
    xkern_return_t       xkr;
    XObj                 lls;
    GHistory             *his;
    EvFunc               func;

    if (!gg->node_map) {
	xTraceP0(self, TR_FULL_TRACE, "Push fails as the group map isn't known");
	return XMSG_ERR_HANDLE;
    }
    myms = getms(gg->node_map, ps->thishost);
    xAssert(myms);
    if (!IP_EQUAL(ps->thishost, ps->master)) {
	xAssert(message);
	/*
	 * We must send to the sequencer!
	 * Don't accept other messages in the
	 * pipe, as we wanna be FIFO and 
	 * therefore we want to make sure
	 * that the sequencer gets two packets
	 * from us in the same order.
	 */
	semWait(&ss->sessionSem);
	/* 
         * Note: a new broadcast request from slave
	 * does not generate a state transition
	 */
	lls = gg->master_lls;
	/*
	 * Non-master nodes use a temporary history entry
	 * 'til the master had a chance to bid a seqno from
	 * its own history circular buffer.
	 */
	if (!(his = pathAlloc(self->path, sizeof(GHistory)))) {
	    xTraceP0(self, TR_ERRORS, "history allocation error");
	    return XMSG_ERR_HANDLE;
	}
	if (!(his->i_ev = evAlloc(self->path))) {
	    xTraceP0(self, TR_ERRORS, "history event allocation error");
	    pathFree(his->i_ev);
	    return XMSG_ERR_HANDLE;
	}
	hdr = &his->i_hdr;
	msgConstructCopy(&his->i_message, message);
	hdr->h_type = BROADCAST_REQUEST;
	hdr->h_messageid = myms->m_messageid + 1;
	hdr->h_name_initiator = *node;    
	func = sequencerTimeout;
    } else {
	if (message) {
	    /* 
	     * Given that we're the master, we can be confident that
	     * gg->num_members reflects the actual population of the 
	     * group (we are still running funneled).
	     */
	    if (gg->num_members == 1) {
		Event   ev;
		/*
		 * Trivial case of atomic broadcast
		 */
		xTraceP0(self, TR_FULL_TRACE, "trivial: we're the master and group size is 1");
		
		his = &gg->cbuffer[gg->producer++ % gg->group_history_entries];
		/*
		 * Memorize this history entry so that the asynchronous
		 * sequencerEventReturn be able to find it.
		 */
		ss->his = his;
		hdr = &his->i_hdr;
		hdr->h_seqno = gg->group_seqno;      
		his->i_retrial = 0;
		his->i_refcount = 1;
		his->i_mode = UNSTABLE;
		msgConstructCopy(&his->i_message, message);

		if (!(ev = evAlloc(self->path))) {
		    xTraceP0(self, TR_ERRORS, "Event allocation error");
		    return XMSG_ERR_HANDLE;
		}
		evDetach(evSchedule(ev, sequencerEventReturn, self, 0));
		return XMSG_NULL_HANDLE;
	    }
	    /*
	     * Pick the first available entry in the history circular buffer.
	     * If history is full ... MORE WORK
	     */
	    his = &gg->cbuffer[gg->producer++ % gg->group_history_entries];
	    hdr = &his->i_hdr;
	    msgConstructCopy(&his->i_message, message);
	    hdr->h_name_initiator = *node;
	    
	    xTraceP1(self, TR_FULL_TRACE, "master: new message gets slot %d", gg->producer - 1);
	    
	    if (gg->state == MASTER_NEW_BROAD_SENT) {
		/*
		 * We cannot send the message right away.
		 * It will be sent when the seqno becomes current.
		 */
		xTraceP1(self, TR_FULL_TRACE, "master: reserved slot %d but we don't send it", gg->producer - 1);
		his->i_mode = WAITING;
		if (!gg->waitroom)
		    gg->waitroom = gg->producer - 1;
		
		return XMSG_NULL_HANDLE;
	    }
	    /*
	     * We are entitled to send something. Pick up the seqno that is current
	     */
	    if (gg->waitroom) {
		xAssert(gg->waitroom < gg->producer);
		xTraceP1(self, TR_FULL_TRACE, "master: slot becomes current from wait list %d", gg->waitroom);
		his->i_mode = WAITING;
		his = &gg->cbuffer[gg->waitroom % gg->group_history_entries];
		msgConstructCopy(&lmessage, &his->i_message);
		message = &lmessage;
		xAssert(his->i_mode == WAITING);
		his->i_mode = UNSTABLE;
		hdr = &his->i_hdr;
		/*
		 * There must be at least one more entry WAITING
		 * (the one that we were shepherding through)
		 */
		gg->waitroom++;
	    }
	} else {
	    if (gg->state == MASTER_NEW_BROAD_SENT) {
		xTraceP0(self, TR_FULL_TRACE, "master: sequenceroutput is a no-op");
		return XMSG_NULL_HANDLE;
	    }
	    if (!gg->waitroom) {
		xTraceP0(self, TR_FULL_TRACE, "master: sequenceroutput is a no-op. Switch to idle");
		gg->state = MASTER_OR_SLAVE_IDLE;
		return XMSG_NULL_HANDLE;
	    }
	    /*
	     * We have something to send from the wait room only.
	     * The waitroom may get empty.
	     */
	    xAssert(gg->waitroom < gg->producer);
	    xTraceP1(self, TR_FULL_TRACE, "master2: seqno becomes current from wait list %d", gg->waitroom);
	    his = &gg->cbuffer[gg->waitroom % gg->group_history_entries];
	    msgConstructCopy(&lmessage, &his->i_message);
	    message = &lmessage;
	    xAssert(his->i_mode == WAITING);
	    his->i_mode = UNSTABLE;
	    hdr = &his->i_hdr;
	    gg->waitroom++;
	    if (gg->cbuffer[gg->waitroom % gg->group_history_entries].i_mode != WAITING) 
		gg->waitroom = 0;
	}
	hdr->h_type = SEQUENCER_BROADCAST;
	/* refcount is driven by expect counters */
	his->i_refcount = gg->num_members - 1; 
	/* ackcount is driven by ACK packets */
	his->i_ackcount = his->i_refcount;
	/*
	 * Anytime the sequencer broadcasts, it receives its own packet back.
	 * To avoid this from happening, I make the new packet to look like
	 * a duplicate one.
	 */
	hdr->h_messageid = ++myms->m_messageid;
	hdr->h_seqno = gg->group_seqno;      
	func = sequencerTimeoutMaster;
	/*
	 * Change of state for the master
	 */
	gg->state = MASTER_NEW_BROAD_SENT;
	lls = gg->group_lls;

	xTraceP1(self, TR_FULL_TRACE, "master: about to send seqno %d", hdr->h_seqno);
    }

    /*
     * Fill up the rest of the header
     */
    hdr->h_hlpNum = ss->hlpNum;
    hdr->h_gid = gg->this_gid;
    hdr->h_gincarnation = gg->group_incarnation;
    hdr->h_name = ps->thishost;        /* MORE WORK: I might have two different addresses in two gids */
    hdr->h_master = ps->master;
    hdr->h_headcount = gg->num_members;
    hdr->h_expect = myms->m_expect;

    /* 
     * set the history entry and arm its retransmission counter 
     */
    his->i_retrial = ps->retrans_max;
    his->i_mode = UNSTABLE;

    if ( msgPush(message, sequencerHdrStore, hdr, sizeof(sequencerHdr), 0) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "msg push fails");
	return XMSG_ERR_HANDLE;
    }

    xTraceP0(self, TR_FULL_TRACE, "push takes place");
    xPush(lls, message);

    /*
     * set alarm
     */
    evSchedule(his->i_ev, func, self, gg->delay * 1000);
    ss->his = his;

    return XMSG_NULL_HANDLE;
}

static xmsg_handle_t
sequencerOutputControl(
		       XObj      self,
		       GOper     op,
		       seqno     number,
		       boolean_t toslave)
{
    SState	         *ss = (SState *)self->state;
    PState	         *ps = (PState *)xMyProtl(self)->state;
    GState               *gg = ss->shared;
    MState               *myms;
    sequencerHdr         hdr;
    xkern_return_t       xkr;
    Msg_s                message;

    xTraceP3(self, TR_ERRORS, "output control packet XObj %x Oper %d seno %d",
	     self, op, number);
    
    if (!gg->node_map) {
	xTraceP0(self, TR_ERRORS, "output control packet fails as we do not have a group map");
	return XMSG_ERR_HANDLE;
    }
    if (!(myms = getms(gg->node_map, ps->thishost))) {
	xTraceP0(self, TR_ERRORS, "output control packet no MState (yet?). Return");
	return XMSG_ERR_HANDLE;
    }

    xkr = msgConstructContig(&message, self->path, 0, SEQUENCER_STACK_SIZE);
    if (xkr != XK_SUCCESS) {
	xTraceP0(self, TR_ERRORS, "msgConstructContig in output control packet fails");
	return XMSG_ERR_HANDLE;
    }
    /*
     * Fill up the header
     */
    hdr.h_hlpNum = ss->hlpNum;
    hdr.h_gid = gg->this_gid;
    hdr.h_gincarnation = gg->group_incarnation;
    hdr.h_name = ps->thishost; 
    hdr.h_name_initiator = ps->thishost;
    hdr.h_master = ps->master;
    hdr.h_headcount = gg->num_members;
    hdr.h_type = op;
    hdr.h_seqno = number;

    xTraceP0(self, TR_FULL_TRACE, "master: about to output control packet");

    hdr.h_messageid = ++myms->m_messageid;  /* master does not want to see its own packets */
    hdr.h_expect = myms->m_expect;

    if (msgPush(&message, sequencerHdrStore, &hdr, sizeof(hdr), 0) == XK_FAILURE) {
	xTraceP0(self, TR_ERRORS, "msg push fails");
	return XMSG_ERR_HANDLE;
    }

    xTraceP0(self, TR_FULL_TRACE, "push takes place");
    xkr = xPush(toslave? gg->group_lls : gg->master_lls, &message);
    msgDestroy(&message);

    return XMSG_NULL_HANDLE; 
}

static xkern_return_t
sequencerPop(
	     XObj	self,
	     XObj	lls,
	     Msg	message,
	     void	*inHdr)
{
    PState              *ps = (PState *)xMyProtl(self)->state;
    SState	        *ss = (SState *)self->state;
    GState              *gg = ss->shared;
    GHistory            *his;
    MState              *ms, *myms;
    sequencerHdr        *hdr = (sequencerHdr *)inHdr;
    sequencerXchangeHdr xhdr;
    boolean_t           iammaster;
    char                *buf;

    xTraceP0(self, TR_FULL_TRACE, "pop called");

    if (IP_EQUAL(hdr->h_name, ps->thishost)) {
	/*
	 * We don't want to process our own packets
	 */
	xTraceP0(self, TR_FULL_TRACE, "throw away our own packet");
	return XK_SUCCESS;
    }
    if (!gg->node_map) {
	/*
	 * Packet recv'd while we don't have stable membership information:
	 * let's see if we can do something about it.
	 */
	xTraceP0(self, TR_FULL_TRACE, "we don't have a group map information");
	return XK_SUCCESS;
    }
    myms = getms(gg->node_map, ps->thishost);
    xAssert(myms);

    /*
     * Is there another master within our cluster?
     */
    if (!IP_EQUAL(hdr->h_master, ps->master) && 
	getms(gg->node_map, hdr->h_master) == (MState *)0) {
	/*
	 * Yes. Then we give way iff the new master has a larger
	 * number of adepts. If the number is even, we elect the
	 * master with higher IP number.
	 */
	if (hdr->h_headcount > gg->num_members ||
	    (hdr->h_headcount == gg->num_members &&
	     *(u_bit32_t *)&hdr->h_master > *(u_bit32_t *)&ps->master)) {
	    xTraceP3(self, TR_FULL_TRACE, "Give way to the master %s with %d many nodes (vs. %d nodes)",
		     ipHostStr(&hdr->h_master), hdr->h_headcount, gg->num_members);
	    Kabort("Please reboot and perform an amnesia join. We are out.");
	    /* NOTREACHED */
	}
    }
    
    ms = getms(gg->node_map, hdr->h_name); 
    /*
     * Is this a duplicate packet?
     */
    if (ms == (MState *)0) {
	/*
	 * Packet from non-member node. Dump it.
	 */
	xTraceP0(self, TR_FULL_TRACE, "pop called with a message from unknown node: no-op");
	return XK_SUCCESS;
    }
    if (hdr->h_messageid <= ms->m_messageid) {
	/*
	 * Duplicate packet
	 */
	xTraceP0(self, TR_FULL_TRACE, "pop called with a dup: no-op");
	return XK_SUCCESS;
    }
    ms->m_messageid = hdr->h_messageid;
    xAssert(hdr->h_expect >= ms->m_expect);

    iammaster = IP_EQUAL(ps->master, ps->thishost);
    if (iammaster && 
	hdr->h_gincarnation != gg->group_incarnation  &&
	hdr->h_gincarnation != INCARNATION_WILD_CARD  &&
	hdr->h_type != SEQUENCER_SYNC_REQUEST) {
	/* 
	 * Unacceptable value for incarnation.
	 * If I'm master, I can only take the exact one 
	 * or the WILD card (new node). 
	 */
	xTraceP2(self, TR_FULL_TRACE, "Master MUST throw away unacceptable group incarnation (%d) vs (%d)",
		hdr->h_gincarnation, gg->group_incarnation);
	return XK_SUCCESS;
    }
    if (!iammaster &&
	gg->group_incarnation != INCARNATION_WILD_CARD &&
	hdr->h_gincarnation != gg->group_incarnation &&
	hdr->h_type != SEQUENCER_SYNC_REPLY) {
	/* 
	 * Unacceptable value for incarnation.
	 * If I'm NOT master, I can only take the exact one 
	 * or the WILD card (new node). 
	 */
	xTraceP2(self, TR_FULL_TRACE, "Non-Master MUST throw away unacceptable group incarnation (%d) vs (%d)",
		 hdr->h_gincarnation, gg->group_incarnation);
	return XK_SUCCESS;
    }
    if (ms->m_expect < hdr->h_expect &&
	(iammaster || IP_EQUAL(hdr->h_name, ps->master))) {
	/*
	 * This message carries an implicit ack that the peer node
	 * has successfully delivered some messages. Thus, we can
	 * move ahead and release the proper reference counts in
	 * the circular buffer. If we are the master, we release
	 * a reference. If we are not the master but we've received
	 * a packet from the master, we zap the history entry.
	 */
	sequencerPruneHistory(gg, ms->m_expect, hdr->h_expect, TRUE, FALSE);
	ms->m_expect = hdr->h_expect;
    }
    xTraceP2(self, TR_FULL_TRACE, "received packet type %d seqno %d", 
	     hdr->h_type, hdr->h_seqno);

    /*
     * End of housekeeping. We can now look at what we've got.
     */
    switch (hdr->h_type) {
    case BROADCAST_REQUEST:
	xTraceP0(self, TR_FULL_TRACE, "received a BROADCAST_REQUEST");
	xAssert(iammaster);
	/*
	 * Store it in the history buffer and possibly diffuse it to the 
	 * other nodes.  Note that the hdr->h_name (original initiator) 
	 * will tag the packet (h_name_initiator).
	 */
	while (sequencerOutput(self, message, &hdr->h_name) != XMSG_NULL_HANDLE) {
	    xTraceP0(self, TR_FULL_TRACE, "failure to diffuse a broadcast. Retry");
	    Delay(gg->delay);
	}
	/*
	 * No state change 
	 */
	break;
    case SEQUENCER_BROADCAST:
	/*
	 * We cannot be the master as the master's output is not
	 * received back by the master itself.
	 */
	xAssert(!iammaster);
	if (!ss->asynmsg) {	    
	    /*
	     * A very unfrequent case: we've lost the message we cache
	     * session-wise for upstream asynchronous notifications.
	     */
	    xTraceP0(self, TR_ERRORS, "record a message for async notifications");
	    while (!(ss->asynmsg = pathAlloc(self->path, sizeof(Msg_s))))
		Delay(gg->delay);
	    msgConstructCopy(ss->asynmsg, message);
	}
	if (gg->state == SLAVE_NEW_BROAD_RCVD || gg->state == SLAVE_ACK_SENT) {
	    xTraceP0(self, TR_FULL_TRACE, "recvd BROADCAST from sequencer; bad state");
	    break;
	}
	/*
	 * Memorize this entry into the history buffer: it's either
	 * a new one, or the same one (if unstable). If it is
	 * the broadcast we initiated, get rid off of the temporary
	 * history entry (ss->his).
	 */
	xAssert(gg->producer == hdr->h_seqno || gg->producer == hdr->h_seqno + 1 ||
		gg->producer == 0);
	gg->state = SLAVE_NEW_BROAD_RCVD;
	his = &gg->cbuffer[hdr->h_seqno % gg->group_history_entries];
	if (his->i_mode == UNSTABLE) {
	    /* 
	     * This history entry did not make it to the stable state.
	     * Rather, the sequencer is bidding the same seqno with 
	     * another message (as a consequence of a sequencer error).
	     * We must clear state before reusing the history entry.
	     */
	    msgDestroy(&his->i_message);
	}
	gg->producer = hdr->h_seqno + 1;
	his->i_refcount = 1;   /* only one reference, the sequencer! */
	his->i_retrial = 0;
	his->i_mode = UNSTABLE;
	msgConstructCopy(&his->i_message, message);
	msgForEach(message, msgDataPtr, (void *)&his->i_hdr);
	if (IP_EQUAL(hdr->h_name_initiator, ps->thishost) && ss->his) {
	    /* we're the ones who started all */
	    xTraceP0(self, TR_FULL_TRACE, "clearing out our own temporary history entry");
	    msgDestroy(&ss->his->i_message);
	    evCancel(ss->his->i_ev);
	    evDetach(ss->his->i_ev);
	    pathFree(ss->his);
	    ss->his = (GHistory *)0;
	    /* we can accept new requests, now */
	    semSignal(&ss->sessionSem);
	}
	/* move on the local message counter */
	myms->m_messageid++;
	/*
	 * Send an ACK!
	 */
	while (sequencerOutputControl(self, ACK_TO_SEQUENCER, 
				      hdr->h_seqno, FALSE) != XMSG_NULL_HANDLE) {
	    xTraceP0(self, TR_ERRORS, "fails to send a ack");
	    Delay(gg->delay);
	}
	gg->state = SLAVE_ACK_SENT;
	evSchedule(his->i_ev, sequencerSendSync, self, gg->watchdog * 1000);
	xAssert(gg->cbuffer[(hdr->h_seqno - 1) % gg->group_history_entries].i_mode == EMPTY || 
		gg->cbuffer[(hdr->h_seqno - 1) % gg->group_history_entries].i_mode == STABLE);
	break;
    case ACK_TO_SEQUENCER:
	xAssert(iammaster);
	/*
	 * Receive ACK and (possibly) send an ACCEPT.
	 * The messageid story prevents us from getting
	 * duplicate ACKs from fellow nodes.
	 */
	if (gg->state != MASTER_NEW_BROAD_SENT) {
	    xTraceP1(self, TR_FULL_TRACE, "recvd ACK to sequencer; bad state (%d)", gg->state);
	    break;
	}
	xTraceP1(self, TR_FULL_TRACE, "received ack for (%d)", hdr->h_seqno);
	his = &gg->cbuffer[hdr->h_seqno % gg->group_history_entries];
	xAssert(his->i_mode == UNSTABLE);
	if (--his->i_ackcount == 0) {
	    sequencerXchangeHdr xhdr;

	    xTraceP1(self, TR_FULL_TRACE, "the last ack made (%d) stable", hdr->h_seqno);
	    gg->state = MASTER_ACKS_RCVD_OK;
	    his->i_mode = STABLE;
	    evCancel(his->i_ev);
	    /*
	     * Send accept (same seqno, different messageid and possibly expect)
	     */
	    while (sequencerOutputControl(self, SEQUENCER_ACCEPT, 
					  hdr->h_seqno, TRUE) != XMSG_NULL_HANDLE) {
		xTraceP0(self, TR_ERRORS, "fails to send an accept. Retry");
		Delay(gg->delay);
	    }
	    /*
	     * Pop our copy of the message for our clients.
	     * We don't need to clean out any state (ss->his)
	     * as we're the master.
	     */
	    gg->state = MASTER_ACCEPT_SENT;
	    xhdr.type = SEQ_DATA;
	    xhdr.xkr = XK_SUCCESS;
	    xhdr.seq = hdr->h_seqno;
	    msgSetAttr(&his->i_message, 0, &xhdr, sizeof(xhdr));
	    /*
	     * Note that the upcall may have the effect of bumping the
	     * group_incarnation number (iff the above session is a
	     * membership session).
	     */
	    xAssert(xhdr.seq == gg->group_seqno);
	    gg->group_seqno = xhdr.seq + 1;
	    xDemux(self, &his->i_message);
	    /*
	     * The next sequencerOutput has the effect of restarting
	     * the transmission, in case new broadcast requests were
	     * queued up in the waiting room.
	     */
	    while (sequencerOutput(self, (Msg) 0, &ipNull) != XMSG_NULL_HANDLE) {
		xTraceP0(self, TR_FULL_TRACE, "failure to diffuse a broadcast. Retry");
		Delay(gg->delay);
	    }
	    xAssert(gg->state == MASTER_NEW_BROAD_SENT ||
		    gg->state == MASTER_OR_SLAVE_IDLE);
	}
	break;
    case SEQUENCER_ACCEPT:
	xAssert(!iammaster);
	/* move on the local message counter */
	if (gg->state != SLAVE_ACK_SENT) {
	    xTraceP1(self, TR_FULL_TRACE, "recvd accept from sequencer; bad state (%d)", gg->state);
	    break;
	}
	myms->m_messageid++;
	his = &gg->cbuffer[hdr->h_seqno % gg->group_history_entries];
	evCancel(his->i_ev);
	if (his->i_mode == UNSTABLE) {
	    sequencerXchangeHdr xhdr;

	    xTraceP1(self, TR_FULL_TRACE, "making (%d) stable with accept", hdr->h_seqno);
	    xAssert(gg->cbuffer[(hdr->h_seqno - 1) % gg->group_history_entries].i_mode == STABLE ||
		    gg->cbuffer[(hdr->h_seqno - 1) % gg->group_history_entries].i_mode == EMPTY);
	    his->i_mode = STABLE;
	    /*
	     * don't prune it yet (wait for the expect counter to tell us
	     * that the sequencer has learned that all members have delivered
	     * the message)!
	     */
	    gg->state = SLAVE_ACCEPT_RCVD_DELIVER;
	    xTraceP1(self, TR_FULL_TRACE, "returning (%d) stable", hdr->h_seqno);
	    xhdr.type = SEQ_DATA;
	    xhdr.xkr = XK_SUCCESS;
	    xhdr.seq = hdr->h_seqno;
	    msgSetAttr(&his->i_message, 0, &xhdr, sizeof(xhdr));
	    xAssert(xhdr.seq == gg->group_seqno || !gg->group_seqno);
	    gg->group_seqno = xhdr.seq + 1;
	    xDemux(self, &his->i_message);
	    gg->state = MASTER_OR_SLAVE_IDLE;
	}
	break;
    case SEQUENCER_SYNC_REQUEST:
	xAssert(iammaster);
	while (sequencerOutputControl(self, SEQUENCER_SYNC_REPLY, 
				      gg->group_seqno, TRUE) != XMSG_NULL_HANDLE) {
	    xTraceP0(self, TR_FULL_TRACE, "failure to diffuse a SyncReply. Retry");
	    Delay(gg->delay);
	}
	break;
    case SEQUENCER_SYNC_REPLY:
	xAssert(!iammaster);
	xAssert(gg->state == SLAVE_ACK_SENT || gg->state == MASTER_OR_SLAVE_IDLE);
	his = &gg->cbuffer[(gg->producer - 1) % gg->group_history_entries];
	if (gg->state == SLAVE_ACK_SENT) {
	    if (hdr->h_seqno == (gg->producer - 1)) {
		/*
		 * The sequencer did not receive as many acks it needed to.
		 * No accept was missed. We're still waiting. The master
		 * sequencer will call the error first.
		 */
		break;
	    }
	    if (hdr->h_seqno != gg->producer) {
		/* 
		 * We've missed something!
		 */
		evCancel(his->i_ev);
		sequencerError(self,  &gg->cbuffer[hdr->h_seqno % gg->group_history_entries], FALSE);
		break;
	    }
	    if (his->i_mode == UNSTABLE) {
		sequencerXchangeHdr xhdr;
		
		xTraceP1(self, TR_FULL_TRACE, "making (%d) stable", gg->producer - 1);
		evCancel(his->i_ev);
		his->i_mode = STABLE;
		/*
		 * don't prune it yet (wait for the expect counter to tell us
		 * that the sequencer has learned that all members have delivered
		 * the message)!
		 */
		xTraceP1(self, TR_FULL_TRACE, "returning (%d) stable", gg->producer - 1);
		xhdr.type = SEQ_DATA;
		xhdr.xkr = XK_SUCCESS;
		xhdr.seq = hdr->h_seqno - 1;
		msgSetAttr(&his->i_message, 0, &xhdr, sizeof(xhdr));
		xAssert(xhdr.seq == gg->group_seqno || !gg->group_seqno);
		gg->group_seqno = xhdr.seq + 1;
		xDemux(self, &his->i_message);
	    }
	} else {
	    /*
	     * The reply has raced with the accept. 
	     * Cancel the alarm. We're cool.
	     */
	    evCancel(his->i_ev);
	}
	break;
    case SEQUENCER_MASTER_ALIVE:
	gg->idle_ticks++;
	break;
    default:
	xAssert(0);
    }

    return XK_SUCCESS;
}

static int
sequencerProtControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    PState	*ps = (PState *)self->state;

    xTraceP0(self, TR_FULL_TRACE, "control called");

    switch (op) {
    
    case GETMASTER:
	checkLen(len, sizeof(IPhost));
	/*
	 * Convention: the protocol version of this xControl
	 * concerns the ALL_NODES_GROUP group.
	 */
	*(IPhost *)buf = ps->master; 
	return sizeof(IPhost);

    case GETMAXPACKET:
    case GETOPTPACKET:
	if (xControl(xGetDown(self, SEQ_TRANSPORT), op, buf, len) < (int)sizeof(int)) {
	    return -1;
	}
	*(int *)buf -= sizeof(sequencerHdr);
	return (sizeof(int));

    case SEQUENCER_SETGROUPMAP:
	checkLen(len, sizeof(Map));
	/*
	 * Plug in the group map information
	 */
	ps->node2groups = *(Map *)buf;
	return 0;

    default:
	return xControl(xGetDown(self, SEQ_TRANSPORT), op, buf, len);
    }
}

static xmsg_handle_t
sequencerPush(
	      XObj	self,
	      Msg	message)
{
    PState              *ps = (PState *)xMyProtl(self)->state;

    return sequencerOutput(self, message, &ps->thishost);
}


static void
sequencerPruneHistory(
		      GState       *gg,
		      u_bit32_t    from,
		      u_bit32_t    to,
		      boolean_t    release_ref,
		      boolean_t    unconditioned)
{
    GHistory            *his;
    u_long              count;

    xTrace2(sequencerp, TR_FULL_TRACE, "[sequencer]: pruning history buffer from %d to %d", from, to);
    
    xAssert(to <= gg->producer);
    xAssert(gg->consumer <= from);
    for (count = from; count < to; count++) {
	his = &gg->cbuffer[count % gg->group_history_entries];
	if (his->i_mode == EMPTY)
	    continue;
	if (release_ref) {
	    xAssert(his->i_mode == STABLE);
	    --his->i_refcount;
	    xTrace0(sequencerp, TR_FULL_TRACE, "[sequencer]: pruning history buffer: release one reference");
	 }
	xTrace3(sequencerp, TR_FULL_TRACE, 
		"[sequencer]: pruning history buffer: entry %d refcount %d %sconditioned", 
		count, his->i_refcount, (unconditioned)? "UN" : "");
	if (his->i_refcount == 0 || unconditioned) {
	    /*
	     * We can zap this entry from the circular buffer!
	     * (if we are the master, refcount was initialized 
	     * to all members -1; if we are not, refcount was
	     * initialized to 1 - the master only).
	     */
	    msgDestroy(&his->i_message);
	    his->i_mode = EMPTY;
	    gg->consumer = count + 1;
	    xTrace1(sequencerp, TR_FULL_TRACE, 
		     "[sequencer]: pruning history buffer: advance consumer to %d", gg->consumer);
	}
    }
}

static void
sequencerSendSync(
		  Event	ev,
		  void 	*arg)
{
    XObj           self = (XObj)arg;
    SState	   *ss = (SState *)self->state;
    GState         *gg = ss->shared;

    if (evIsCancelled(ev)) {
	xTraceP0(self, TR_FULL_TRACE, "timeout cancelled: no-op");
	return;
    } 
    while (sequencerOutputControl(self, SEQUENCER_SYNC_REQUEST,
				  (u_bit32_t)-1, FALSE) != XMSG_NULL_HANDLE) {
	xTraceP0(self, TR_FULL_TRACE, "failure to diffuse a broadcast. Retry");
	Delay(gg->delay);
    }
    evSchedule(ev, sequencerSendSync, self, gg->watchdog * 1000);
}

static int
sequencerSessControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len)
{
    SState	*ss = (SState *)self->state;
    PState      *ps = (PState *)xMyProtl(self)->state;

    xTraceP0(self, TR_FULL_TRACE, "control called");

    switch (op) {

    case GETMASTER:
	checkLen(len, sizeof(IPhost));
	/* 
	 * MORE WORK: different groups (sessions) deserve different
	 * masters. Ergo, bring the ps->master info into the session
	 * state.
	 */
	*(IPhost *)buf = ps->master; 
	return sizeof(IPhost);

    case GETGROUPID:
	checkLen(len, sizeof(group_id));
	/*
	 * This is the group id associated to this sequencer session
	 */
	*(group_id *)buf = ss->shared->this_gid;
	return sizeof(group_id);

    case SETINCARNATION:
	{
	    GState         *gg = ss->shared;
	    GHistory       *his;

	    checkLen(len, sizeof(u_bit32_t));
	    xAssert(gg);

	    gg->group_incarnation = *(u_bit32_t *)buf;
	    /*
	     * Don't throw away the last entry of the history, as somebody may
	     * have missed the accept and, thus, the change in population.
	     * MORE WORK
	     */
	    xAssert(gg->producer > 0);
	    sequencerPruneHistory(gg, gg->consumer, gg->producer - 1, FALSE, TRUE);
	    if (gg->this_gid == ALL_NODES_GROUP) {
		/*
		 * Pass it down to the panning protocol
		 */
		return xControl(gg->group_lls, op, buf, len);
	    }
	    return 0;
	}

    case GETMAXPACKET:
    case GETOPTPACKET:
	if (xControl(xGetDown(self, SEQ_TRANSPORT), op, buf, len) < (int)sizeof(int)) {
	    return -1;
	}
	*(int *)buf -= sizeof(sequencerHdr);
	return (sizeof(int));

     case SEQUENCER_SETPOPULATION:
	{
	    GState         *gg = ss->shared;

	    checkLen(len, sizeof(u_long));
	    /*
	     * Tell sequencer that population has changed and
	     * that a new view has been installed.
	     */
	    gg->num_members = *(u_long *)buf;
	    return 0;
	}

    case SEQUENCER_SETNOTIFICATION:
	checkLen(len, sizeof(boolean_t));
	if ( *(boolean_t *)buf) {
	    ss->asynnot = TRUE;
	} else {
	    ss->asynnot = FALSE;
	}
	return 0;
	
    case SEQUENCER_SETGROUPMAP:
	{
	    GState         *gg = ss->shared;
	    
	    checkLen(len, sizeof(Map));
	    /*
	     * Plug in the group map information
	     */
	    xAssert(gg);
	    gg->node_map = *(Map *)buf;
	    return 0;
	}

   default:
	return xControl(xGetDown(self, SEQ_TRANSPORT), op, buf, len);
    }
}


static void
sequencerSweepHistory(
		      Event	ev,
		      void 	*arg)
{
    GState         *gg = (GState *)arg;
    
    xTrace0(sequencerp, TR_FULL_TRACE, "[sequencer]: sweep history");

    xAssert(gg->consumer <= gg->producer);

    if (gg->consumer != gg->producer) {
	sequencerPruneHistory(gg, gg->consumer, gg->producer, FALSE, FALSE);
    }
    
    evSchedule(gg->gc_ev, sequencerSweepHistory, gg, SEQUENCER_SWEEP_HISTORY * 1000);
}

static void
sequencerTimeout(
		 Event	ev,
		 void 	*arg)
{
    XObj           self = (XObj)arg;
    PState         *ps = (PState *)xMyProtl(self)->state;
    SState	   *ss = (SState *)self->state;
    GState         *gg = ss->shared;
    GHistory       *his = ss->his;
    
    if (evIsCancelled(ev)) {
	xTraceP0(self, TR_FULL_TRACE, "timeout !master cancelled: no-op");
	return;
    } 
    xTraceP0(self, TR_FULL_TRACE, "timeout !master");
   
    xAssert(!IP_EQUAL(ps->thishost, ps->master));
    xAssert(!his || his->i_mode == UNSTABLE || his->i_mode == STABLE);

    if (his && his->i_mode == UNSTABLE) {
	if (his->i_retrial-- > 0) {
	    Msg_s          message;
	    xkern_return_t xkr;

	    msgConstructCopy(&message, &his->i_message);
	    xkr = msgPush(&message, sequencerHdrStore, &his->i_hdr, sizeof(sequencerHdr), 0);
	    if (xkr == XK_SUCCESS) {
		xPush(gg->master_lls, &message);
	    } else {
		xTraceP0(self, TR_ERRORS, "msg push fails, nothing done");
	    }
	    msgDestroy(&message);
	    
	    evSchedule(his->i_ev, sequencerTimeout, self, gg->delay * 1000);
	    xTraceP0(self, TR_FULL_TRACE, "restarted timeout !master");
	} else {
	    /*
	     * I could not reach the master. I suspect a master failure
	     */
	    sequencerError(self, his, TRUE);
	}
    }
}

static void
sequencerTimeoutMaster(
		       Event	ev,
		       void 	*arg)
{
    XObj         self = (XObj)arg;
    PState       *ps = (PState *)xMyProtl(self)->state;
    SState	 *ss = (SState *)self->state;
    GState       *gg = ss->shared;
    GHistory     *his = ss->his;

    if (evIsCancelled(ev)) {
	xTraceP0(self, TR_FULL_TRACE, "timeout cancelled: no-op");
	return;
    } 
    xTraceP0(self, TR_FULL_TRACE, "timeout");

    xAssert(IP_EQUAL(ps->thishost, ps->master));
    xAssert(!his || his->i_mode == UNSTABLE || his->i_mode == STABLE);

    if (his && his->i_mode == UNSTABLE) {
	if (his->i_retrial > 0) {
	    Msg_s          message;
	    xkern_return_t xkr;
	    
	    xTraceP0(self, TR_FULL_TRACE, "timeout: must push again");
	    
	    his->i_retrial--;
	    msgConstructCopy(&message, &his->i_message);
	    xkr = msgPush(&message, sequencerHdrStore, &his->i_hdr, sizeof(sequencerHdr), 0);
	    if (xkr == XK_SUCCESS) {
		xPush(gg->group_lls, &message);
	    } else {
		xTraceP0(self, TR_ERRORS, "msg push fails, nothing done");
	    }
	    
	    msgDestroy(&message);
	    evSchedule(his->i_ev, sequencerTimeoutMaster, self, gg->delay * 1000);
	    xTraceP0(self, TR_FULL_TRACE, "restarted timeout master");
	} else {
	    sequencerErrorMaster(self, his, TRUE);
	}
    } else {
	xTraceP0(self, TR_ERRORS, "msg got stable, nothing done");
    }
}

xkern_return_t
sequencer_init(
	       XObj self)
{
    XObj	llp;
    PState      *ps;
    Part_s      part[1];
    u_int       unused = 0;
    IPhost      temp;
    
    xTraceP0(self, TR_FULL_TRACE, "init called");

    llp = xGetDown(self, SEQ_TRANSPORT);
    if (!xIsProtocol(llp)) {
	xTraceP0(self, TR_ERRORS, "no lower protocol");
	return XK_FAILURE;
    }

    self->open = sequencerOpen;
    self->demux = sequencerDemuxBootstrap;
    self->opendone = sequencerOpenDone;
    self->control = sequencerProtControl;
    self->openenable = sequencerOpenEnable;
    self->opendisable = sequencerOpenDisable;
    self->opendisableall = sequencerOpenDisableAll;
    self->hlpType = self;

    if (!(self->state = ps = pathAlloc(self->path, sizeof(PState)))) {
	xTraceP0(self, TR_ERRORS, "state allocation error in init");
	return XK_FAILURE;
    }

    /*
     * setting up the PState
     */
    ps->master = ipNull;
    ps->retrans_max = DEFAULT_RETRANS_MAX;
    ps->retrans_msecs = DEFAULT_RETRANS_MSECS;
    ps->max_groups = DEFAULT_MAX_GROUPS;
    ps->max_history_entries = DEFAULT_MAX_HISTORY_ENTRIES;
    findXObjRomOpts(self, sequencerOpts, sizeof(sequencerOpts)/sizeof(XObjRomOpt), 0);

    ps->activeMap = mapCreate(SEQUENCER_ACTIVE_MAP_SIZE, sizeof(ActiveKey),
			       self->path);
    ps->passiveMap = mapCreate(SEQUENCER_PASSIVE_MAP_SIZE, sizeof(PassiveKey),
			       self->path);
    ps->groupMap = mapCreate(ps->max_groups, sizeof(group_id), self->path);

    if ( ! ps->activeMap || ! ps->passiveMap || ! ps->groupMap ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	pathFree(ps);
	return XK_FAILURE;
    }
    
    if (xControl(xGetDown(self, SEQ_TRANSPORT), GETMYHOST, (char *)&ps->thishost, sizeof(IPhost)) 
	                                         < (int)sizeof(IPhost))	{
	xTraceP0(self, TR_ERRORS, "couldn't get local address");
	pathFree(ps);
	return XK_FAILURE;
    }
	    
    partInit(&part[0], 1);
    partPush(*part, ANY_HOST, 0);
    if (xOpenEnable(self, self, llp, part)) {
	xTraceP0(self, TR_ERRORS, "openenable failed in init");
	pathFree(ps);
	return XK_FAILURE;
    }

#if SEQUENCER_MCAST
    /*
     * Regardless of master/slave, join the multicast group.
     */
    if (IP_EQUAL(ps->mcaddr, ipNull)) {
	if (xControl(xGetDown(self, SEQ_BOOTP), BOOTP_GET_MCADDR, (char *)&ps->mcaddr, sizeof(IPhost)) 
	                                         != (int)sizeof(IPhost))	{
	    xTraceP0(self, TR_ERRORS, "cannot figure out a multicast address");
	    pathFree(ps);
	    return XK_FAILURE;
	}
	xTraceP1(self, TR_ERRORS, "multicast address is being defined by BOOTP as %s",
		 ipHostStr(&ps->mcaddr));
    }
    xAssert(!IP_EQUAL(ps->mcaddr, ipNull));
    if (xControl(xGetDown(self, SEQ_TRANSPORT), ADDMULTI, 
		 (char *)&ps->mcaddr, sizeof (ps->mcaddr)) < 0) {
	xTraceP1(self, TR_ERRORS, "cannot join mcast group %s", ipHostStr(&ps->mcaddr));
	pathFree(ps);
	return XK_FAILURE;
    }
    xTraceP1(self, TR_ERRORS, "joined mcast group %s", ipHostStr(&ps->mcaddr));
#endif /* SEQUENCER_MCAST */

    if (IP_EQUAL(ps->master, ipNull)) {
	semInit(&ps->serviceSem, 0);
	evDetach(evSchedule(evAlloc(self->path), sequencerAlert, self, SLAVE_DEFAULT_WATCHDOG * 1000));
	xTraceP0(self, TR_FULL_TRACE, "waiting for a life signal from a master ...");
	semWait(&ps->serviceSem);
	xTraceP0(self, TR_FULL_TRACE, "... end of wait");
    }

    xAssert(!IP_EQUAL(ps->master, ipNull));
    semInit(&ps->serviceSem, 1);
    self->demux = sequencerDemux;
    return XK_SUCCESS;
}





