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
#include <xkern/include/prot/heartbeat.h>
#include <xkern/include/prot/membership.h>
#include <xkern/include/prot/sequencer.h>
#include <xkern/include/prot/censustaker.h>
#include <xkern/protocols/censustaker/census_i.h>

#if XK_DEBUG
int tracecensustakerp;
#endif /* XK_DEBUG */

static IPhost   ipNull = { 0, 0, 0, 0 };

static XObjRomOpt	censustakerOpts[] =
{
    { "number_of_nodes", 3, LoadMaxNodes },
    { "tolerance", 3, LoadTolerance }
};

xkern_return_t
censustakerControl(
		   XObj	self,
		   int	op,
		   char *buf,
		   int	len)
{
    xTraceP0(self, TR_FULL_TRACE, "control called");

    switch (op) {
    
    default:
	return xControl(xGetDown(self, HEARTBEAT), op, buf, len);
    }
}

xkern_return_t
censustakerDemux(
		 XObj 	self,
		 XObj	lls,
		 Msg 	msg)
{
    PState	 *ps = (PState *)self->state;
    boolean_t    iammaster = IP_EQUAL(ps->master, ps->thishost);

    xTraceP0(self, TR_FULL_TRACE, "demux called");

    if (lls == ps->lls[MEMBERSHIP]) {
	membershipXchangeHdr *xhdr = msgGetAttr(msg, 0);
	xkern_return_t xkr;

	xTraceP0(self, TR_FULL_TRACE, "demux called from a membership session");
	xAssert(xhdr->group == ALL_NODES_GROUP);
	if (xhdr->type == MASTERCHANGE) {
	    xAssert(!iammaster);
	    censustakerTrigger(self);
	    return XK_SUCCESS;
	} 
	if (iammaster) {
	    xAssert(ps->unstable);
	    xAssert(ps->logical_clock == xhdr->token);

	    if (xhdr->xkr == XK_SUCCESS) {
		/*
		 * The membership header reports us a success story.
		 * As a result of the atomic broadcast, membership is
		 * stable and we can update the census taker's tables.
		 */
		xTraceP0(self, TR_FULL_TRACE, "SUCCESS");
		ps->force_check = FALSE;
		censustakerUpdateState(self);
	    } else {
		/*
		 * We're unable to make this message stable. Membership
		 * must have changed in the meanwhile. Do not alter the
		 * census taker's tables and wait for the next timeout
		 * to gain another view of the membership.
		 */
		ps->force_check = TRUE;
	    }
	    
	    ps->unstable = FALSE;
	    ps->logical_clock++;
	    evSchedule(ps->ev, censustakerTimeout, self, ps->tolerance * 1000);
	} /* iammaster */
    } else {
	heartbeatXchangeHdr *xhdr;
	NodeState           *ns;
    
	xTraceP0(self, TR_ERRORS, "demux called from a heartbeat session");

	if (iammaster) {
	    /*
	     * We can go further only iff we do not have any 
	     * outstanding communication with the membership 
	     * protocol.
	     */
	    if (!ps->unstable) {
		/*
		 * We are uninterested to the message content.
		 * We care instead about the message attributes,
		 * compliments of the heartbeat protocol.
		 */
		xhdr = (heartbeatXchangeHdr *)msgGetAttr(msg, 0);
		if (mapResolve(ps->address_map, (void *) &xhdr->Sender, (void **) &ns) == XK_SUCCESS) {
		    xTraceP1(self, TR_FULL_TRACE, "node %s is already known", 
			     ipHostStr(&xhdr->Sender));
		    /*
		     * Skip duplicate packets
		     */
		    xAssert(!dlist_empty(&ps->token_list));
		    xAssert(IP_EQUAL(xhdr->Sender, ns->address));
		    if (xhdr->GenNumber != ns->last_update) {
			/*
			 * We update this NodeState entry and
			 * we put it back at the end of the queue.
			 */
			ns->last_update = xhdr->GenNumber;
			ns->timestamp = ps->logical_clock;
			dlist_remove(ns);
			dlist_insert(ns, ps->token_list.prev);
			xTraceP1(self, TR_FULL_TRACE, 
				 "node %s entry is now at the end of queue", ipHostStr(&xhdr->Sender));
		    }
		} else {
		    xTraceP1(self, TR_FULL_TRACE, "node %s is unknown", 
			     ipHostStr(&xhdr->Sender));
		    ns = (NodeState *)pathAlloc(self->path, sizeof(NodeState));
		    if (!ns) {
			xTraceP1(self, TR_FULL_TRACE, "fail to create entry for node %s", 
				 ipHostStr(&xhdr->Sender));
		    } else {
			Bind bd;
			
			ns->address = xhdr->Sender;
			ns->last_update = xhdr->GenNumber;
			ns->is_new = TRUE;
			ns->timestamp = ps->logical_clock;
			dlist_insert(ns, ps->token_list.prev);
			ps->force_check = TRUE;
			(void)mapBind(ps->address_map, (void *)&ns->address, (void *)ns, &bd); 
		    }
		}
	    }
	} else {  /* iammaster */
	    xTraceP0(self, TR_ERRORS, "demux called from a heartbeat session: !master -> noop");
	}
    }
    return XK_SUCCESS;
}

void
censustakerMasterChange(
			XObj   self,
			IPhost master)
{
    PState	*ps = (PState *)self->state;
    Part_s      part[1];
    NodeState   *ns;
    int         unused;

    if (!IP_EQUAL(master, ps->master)) {
	/*
	 * Close the session in use with the old master
	 */
	if (ps->lls[HEARTBEAT] != ERR_XOBJ) {
	    xClose(ps->lls[HEARTBEAT]); 
	    ps->lls[HEARTBEAT] = ERR_XOBJ;
	}
	    
	xTraceP1(self, TR_ERRORS, "the censustaker for this node is: %s",
		 ipHostStr(&master));
	partInit(&part[0], 1);
	partPush(*part, &master, sizeof(IPhost));
	ps->lls[HEARTBEAT] = xOpen(self, self, xGetDown(self, HEARTBEAT), part, self->path);
	if (ps->lls[HEARTBEAT] != ERR_XOBJ) {
	    if (xControl(ps->lls[HEARTBEAT], HEARTBEAT_START_PULSE, (char *)&unused, sizeof(unused)) < 0) {
		xAssert(0);
		xTraceP0(self, TR_ERRORS,  "censustaker can't activate heartbeat");
	    }
	} else {
	    xTraceP0(self, TR_ERRORS,  "censustaker can't open heartbeat protocol");
	}
	
	if (IP_EQUAL(master, ps->thishost)) {
	    boolean_t  yes = TRUE;
	    /*
	     * We are now the master!
	     */
	    if (xControl(ps->lls[HEARTBEAT], CHECKINCARNATION, (char *)&yes, sizeof(yes)) < 0) {
		xAssert(0);
		xTraceP0(self, TR_ERRORS,  "censustaker can't tell to filter out bad incarnations");
	    }
	    (void)evSchedule(ps->ev, censustakerTimeout, self, ps->tolerance * 1000);
	}
	if (IP_EQUAL(ps->master, ps->thishost)) {
	    /*
	     * We used to be a master and now we are not a master any longer
	     */
	    evCancel(ps->ev);
	    evDetach(ps->ev);
	}
	ps->master = master;
	ps->force_check = TRUE;
    }
}

xkern_return_t
censustakerOpenDone(
		    XObj self,
		    XObj lls,
		    XObj llp,
		    XObj hlpType,
		    Path path)
{
    PState	*ps = (PState *)self->state;
    boolean_t   yes = TRUE;

    xTraceP0(self, TR_FULL_TRACE, "opendone called");
    
    if (xControl(lls, CHECKINCARNATION, (char *)&yes, sizeof(yes)) < 0) {
	xAssert(0);
	xTraceP0(self, TR_ERRORS,  "censustaker can't tell to filter out bad incarnations");
    }
    xDuplicate(lls);
    return XK_SUCCESS;
}

static void
censustakerTimeout(
		   Event	ev,
		   void 	*arg)
{
    XObj         self = (XObj)arg;
    PState       *ps = (PState *)self->state;
    NodeState    *ns;
    boolean_t    stop_clocking = FALSE;

    xTraceP0(self, TR_FULL_TRACE, "censustaker: timeout");

    if (evIsCancelled(ev)) {
	xTraceP0(self, TR_FULL_TRACE, "timeout cancelled: no-op");
	return;
    } 
    
    ns = (NodeState *)dlist_first(&ps->token_list);
    if (!ps->unstable && (ps->force_check || 
	ns && ns->timestamp < ps->logical_clock)) {
	
	/*
	 * If we did not notify the membership protocol already and
	 * if there are new entries OR if there is at least one
	 * node which did not give any sign of life, we realize
	 * that we need to install a new view of the nodes present.
	 */
	ps->unstable = TRUE;
	stop_clocking = TRUE;
	xTraceP0(self, TR_FULL_TRACE, "at least one new node or at least one dead node");

	/*
	 * If there is a new node, we go for the absolute type of message.
	 */
	censustakerSendUpdate(self, ps->force_check);
    } else {
	xTraceP0(self, TR_FULL_TRACE, "censustaker: no event which is worth mentioning");
    }

    if (!stop_clocking) {
	ps->logical_clock++;
	evSchedule(ev, censustakerTimeout, arg, ps->tolerance * 1000);
    }
}

static void
censustakerTrigger(
		   XObj self)
{
    IPhost       master;
    
    xTraceP0(self, TR_FULL_TRACE, "censustaker is being triggered!");
    /*
     * Get the master from lower layers!
     */
    if (xControl(xGetDown(self, MEMBERSHIP), GETMASTER, 
		     (char *)&master, sizeof(master)) == sizeof(master)) {
	xAssert(!IP_EQUAL(master, ipNull));
	censustakerMasterChange(self, master);
    }
}

void
censustakerSendUpdate(
		      XObj      self,
		      boolean_t full_format)
{
    PState       *ps = (PState *)self->state;
    NodeState    *ns;
    Msg_s  message;
    membershipXchangeHdr xhdr;
    IPhost *addresses;
    xkern_return_t xkr;
    u_long count;

    xTraceP0(self, TR_ERRORS, "about to send a message");

    while (msgConstructContig(&message, 
			      self->path, 
			      ps->max_nodes * sizeof(IPhost), 
			      CENSUSTAKER_STACK_SIZE) != XK_SUCCESS) {
	xTraceP0(self, TR_ERRORS, "could not create a message");
	Delay(CENSUSTAKER_DELAY);
    }
	
    msgForEach(&message, msgDataPtr, (void *)&addresses);
    memset((char *)&xhdr, 0, sizeof(membershipXchangeHdr));

    if (full_format) {
	xTraceP0(self, TR_ERRORS, "the message contains absolute membership information");
	xhdr.update_count.absolute = 0;
	dlist_iterate(&ps->token_list, ns, NodeState *) {
	    if (ns->timestamp < ps->logical_clock) 
		continue;
	    /*
	     * Compose the message to be proposed to the membership
	     * protocol. -> xPush(MEMBERSHIP)
	     * include logical_clock in the message, to recognize reply!
	     */
	    xhdr.update_count.absolute++;
	    *addresses++ = ns->address;
	}
	msgTruncate(&message, xhdr.update_count.absolute * sizeof(IPhost));
	/*
	 * fill up the header
	 */
	xhdr.type  = FULL;
	count = xhdr.update_count.absolute;
    } else {
	xTraceP0(self, TR_ERRORS, "the message contains relative membership information");
	dlist_iterate(&ps->token_list, ns, NodeState *) {
	    if (ns->is_new) {
		xhdr.update_count.rel.additions++;
		*addresses++ = ns->address;
	    }
	}
	dlist_iterate(&ps->token_list, ns, NodeState *) {
	    if (ns->timestamp == ps->logical_clock) 
		break;
	    xhdr.update_count.rel.deletions++;
	    *addresses++ = ns->address;
	}
	msgTruncate(&message, 
		    (xhdr.update_count.rel.additions + xhdr.update_count.rel.deletions) * sizeof(IPhost));
	xhdr.type  = DELTA;
	count = xhdr.update_count.rel.additions + xhdr.update_count.rel.deletions;
    }
    
    if (count) {
	xTraceP0(self, TR_ERRORS, "message ready for liftoff");
	xhdr.token = ps->logical_clock;
	msgSetAttr(&message, 0, &xhdr, sizeof(xhdr));
	xPush(ps->lls[MEMBERSHIP], &message);
    } else {
	xTraceP0(self, TR_ERRORS, "message is empty: no-op");
	ps->unstable = FALSE;
	ps->logical_clock++;
	evSchedule(ps->ev, censustakerTimeout, self, ps->tolerance * 1000);
    }
    msgDestroy(&message);
}

void
censustakerUpdateState(
		       XObj self)
{
    PState       *ps = (PState *)self->state;
    NodeState      *ns;
    xkern_return_t xkr;

    xTraceP0(self, TR_ERRORS, "update state");
    dlist_iterate(&ps->token_list, ns, NodeState *) {

	if (ns->is_new)
	    ns->is_new = FALSE;
	if (ns->timestamp == ps->logical_clock) 
	    continue;
	/*
	 * Remove this entry: node is really dead
	 * (I can tell from the timestamp).
	 */
	xAssert(ns->timestamp < ps->logical_clock);
	dlist_remove(ns);
	xkr = mapUnbind(ps->address_map, (void *)&ns->address);
	xAssert(xkr == XK_SUCCESS);
	pathFree(ns);
    }
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

static xkern_return_t   
LoadTolerance(
	      XObj	self,
	      char	**str,
	      int	nFields,
	      int	line,
	      void	*arg)
{
    PState	*ps = (PState *)self->state;
    
    xTraceP1(self, TR_EVENTS, "tolerance (%s) specified", str[2]);

    ps->tolerance = atoi(str[2]);
    return XK_SUCCESS;
}

xkern_return_t
censustaker_init(
		 XObj self)
{
    PState      *ps;
    u_int       unused = 0;
    Event       ev;
    IPhost      master;
    group_id    gid;
    Part_s      part[1];

    xTraceP0(self, TR_FULL_TRACE, "init called");

    if (!xIsProtocol(xGetDown(self, HEARTBEAT)) || 
	!xIsProtocol(xGetDown(self, MEMBERSHIP))) {
	xTraceP0(self, TR_ERRORS, "no lower protocol");
	return XK_FAILURE;
    }

    self->demux = censustakerDemux;
    self->control = censustakerControl;
    self->opendone = censustakerOpenDone;
    self->hlpType = self;

    if (!(self->state = ps = pathAlloc(self->path, sizeof(PState)))) {
	xTraceP0(self, TR_ERRORS, "state allocation error in init");
	return XK_FAILURE;
    }

    /*
     * setting up the PState
     */
    ps->master = ipNull;
    ps->max_nodes = DEFAULT_MAX_NODES;
    ps->tolerance = DEFAULT_MAX_TOLERANCE;
    ps->lls[HEARTBEAT] = ps->lls[MEMBERSHIP] = ERR_XOBJ;
    dlist_init(&ps->token_list);
    findXObjRomOpts(self, censustakerOpts, sizeof(censustakerOpts)/sizeof(XObjRomOpt), 0);

    ps->address_map = mapCreate(ps->max_nodes, sizeof(IPhost), self->path);
    if (!ps->address_map) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	pathFree(ps);
	return XK_FAILURE;
    }
    ps->force_check = FALSE;
    ps->unstable    = FALSE;
    
   /*
     * Let the heartbeat and the membership protocols know that we are alive
     */
    if (xOpenEnable(self, self, xGetDown(self, HEARTBEAT), (Part) 0)) {
	xTraceP0(self, TR_ERRORS, "heartbeat openenable failed in init");
	pathFree(ps);
	return XK_FAILURE;
    }
    if (xOpenEnable(self, self, xGetDown(self, MEMBERSHIP), (Part) 0)) {
	xTraceP0(self, TR_ERRORS, "membership openenable failed in init");
	pathFree(ps);
	return XK_FAILURE;
    }
    if (!(ps->ev = evAlloc(self->path))) {
	xTraceP0(self, TR_ERRORS, "Event allocation error in init");
	pathFree(ps);
	return XK_FAILURE;
    }
    
    if (xControl(xGetDown(self, HEARTBEAT), GETMYHOST, (char *)&ps->thishost, sizeof(IPhost)) 
	                                         < (int)sizeof(IPhost))	{
	xTraceP0(self, TR_ERRORS, "couldn't get local address");
	evDetach(ev);
	pathFree(ps);
	return XK_FAILURE;
    }

    /*
     * Actively open the membership protocol with a
     * single participant, the group indicator
     */
    gid = ALL_NODES_GROUP;
    partInit(&part[0], 1);
    partPush(*part, &gid, sizeof(gid));
    if (!(ps->lls[MEMBERSHIP] = xOpen(self, self, xGetDown(self, MEMBERSHIP), part, self->path))) {
	xTraceP0(self, TR_ERRORS, "couldn't open MEMBERSHIP");
	evDetach(ev);
	pathFree(ps);
	return XK_FAILURE;
    }

    /*
     * Censustaker liftoff
     */
    censustakerTrigger(self);

    return XK_SUCCESS;
}




