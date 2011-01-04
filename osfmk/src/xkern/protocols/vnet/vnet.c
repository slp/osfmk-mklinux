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
 * vnet.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * vnet.c,v
 * Revision 1.17.2.4.1.2  1994/09/07  04:18:47  menze
 * OSF modifications
 *
 * Revision 1.17.2.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.17.2.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.17.2.2  1994/07/22  20:08:22  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.17.2.1  1994/04/14  21:47:01  menze
 * Uses allocator-based message interface
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/vnet/vnet_i.h>
#include <xkern/include/prot/arp.h>
#include <xkern/include/prot/ip.h>
    
/* 
 * Check for a valid participant list
 */
#define partCheck(pxx, name, retval)					\
	{								\
	  if ( ! (pxx) || partLen(pxx) < 1 ||				\
	       			(partLen(pxx) > VNET_MAX_PARTS) ) { 	\
		xTrace1(vnetp, TR_ERRORS,				\
			"VNET %s -- bad participants",			\
			(name));  					\
		return (retval);					\
	  }								\
	}

#if XK_DEBUG
int tracevnetp;
#endif /* XK_DEBUG */

static int              bcastRewriteParts( SState *, char * );
static VnetAddrClass    getAddrClass( PState *, IPhost *, Interface ** );
static Interface *      host2ifc( PState *, IPhost * );
static int              isMyAddr( PState *, IPhost * );
static Interface *      lls2ifc( XObj, XObj );
static XObj             openBcastSessn( XObj, IPhost *, XObj, XObj, Path );
static int              resolve( Interface *, IPhost *, ETHhost * );
static int              rewriteParts( SState *, char *, int );
static XObjInitFunc     vnetSessnInit;
static xkern_return_t   vnetClose( XObj );
static int              vnetControlProtl( XObj, int, char *, int );
static int              vnetControlSessn( XObj, int, char *, int );
static XObj             vnetCreateSessn( XObj, XObj, XObj, int, XObj *,
                                         Interface **, Map, void *, Path );
static xmsg_handle_t    vnetMultiPush( XObj, Msg );
static xkern_return_t   vnetOpenDone( XObj, XObj, XObj, XObj, Path );
static XObj             vnetOpen( XObj  self, XObj      hlpRcv, XObj    hlpType,
                                  Part  p, Path path);
static xkern_return_t   vnetProtlDemux( XObj self, XObj lls, Msg m );
static xkern_return_t   vnetPop( XObj   self, XObj lls, Msg msg, void   *h);
static xkern_return_t   vnetSessnDemux( XObj    self, XObj lls, Msg msg);
static xmsg_handle_t    vnetPush( XObj  self, Msg m);
static xkern_return_t   vnetOpenEnable( XObj    self, XObj      hlpRcv, XObj
                                        hlpType, Part   p);
static xkern_return_t   vnetOpenDisable( XObj   self, XObj      hlpRcv, XObj
                                         hlpType, Part  p);
static xkern_return_t   vnetOpenDisableAll( XObj   self, XObj      hlpRcv);

/* 
 * openBcastSessn -- return a session which will send broadcasts over
 * all interfaces which match the host 'h'.  'match' here means being
 * the broadcast address for the interface's network (ignoring
 * subnet mask.)  The 'local' IP broadcast address 255.255.255.255
 * 'matches' all interfaces.
 */
static XObj
openBcastSessn(
    XObj	self,
    IPhost	*h,
    XObj	hlpRcv,
    XObj	hlpType,
    Path	path)
{
    PState	*ps = (PState *)self->state;
    int		i, n;
    Interface	*ifc[VNET_MAX_INTERFACES];
    XObj	lls[VNET_MAX_INTERFACES];
    XObj	s;
    ETHhost	physHost;
    Part_s	p;

    if ( mapResolve(ps->bcastMap, h, &s) == XK_SUCCESS ) {
	return s;
    }
    if ( IP_EQUAL(*h, IP_LOCAL_BCAST) ) {
	for( n=0; n < ps->numIfc; n++ ) {
	    ifc[n] = &ps->ifc[n];
	}
    } else {
	for ( i=0, n=0; i < ps->numIfc; i++ ) {
	    if ( netMaskNetsEqual(h, &ps->ifc[i].host) ) {
		ifc[n++] = &ps->ifc[i];
	    }
	}
    }
    if ( n == 0 ) {
	return ERR_XOBJ;
    }
    for ( i=0; i < n; i++ ) {
	if ( resolve(ifc[i], &IP_LOCAL_BCAST, &physHost) == 0 ) {
	    while ( --i >= 0 ) {
		xClose(lls[i]);
	    }
	    return ERR_XOBJ;
	}
	partInit(&p, 1);
	partPush(p, &physHost, sizeof(ETHhost));
	lls[i] = xOpen(self, hlpType, ifc[i]->llp, &p, path);
	if ( lls[i] == ERR_XOBJ ) {
	    while ( --i >= 0 ) {
		xClose(lls[i]);
	    }
	    return ERR_XOBJ;
	}
    }
    s = vnetCreateSessn(self, hlpRcv, hlpType, n, lls, ifc, ps->bcastMap,
			&h, path);
    if ( s != ERR_XOBJ ) {
	s->push = vnetMultiPush;
	((SState *)s->state)->flags.isBcast = 1;
	return s;
    }
    return ERR_XOBJ;
}


static int
resolve(
    Interface	*ifc,
    IPhost	*ip,
    ETHhost	*phys)
{
    ArpBinding	b;

    b.ip = *ip;
    if ( xControl(ifc->arp, RESOLVE, (char *)&b, sizeof(ArpBinding))
 		< (int)sizeof(ArpBinding) ) {
	return 0;
    }
    *phys = b.hw;
    return 1;
}

static XObj
vnetOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p,
    Path	path)
{
    PState	*ps = (PState *)self->state;
    IPhost 	*remIpHost;
    XObj	lls, s;
    Interface	*ifc;
    ETHhost	remPhysHost;
    Part_s	part;
    
    xTrace0(vnetp, TR_MAJOR_EVENTS, "VNET open");
    partCheck(p, "vnetOpen", ERR_XOBJ);
    remIpHost = (IPhost *)partPop(*p);
    if ( remIpHost == 0 || remIpHost == (IPhost *)ANY_HOST ) {
	xTrace0(vnetp, TR_SOFT_ERRORS, "VNET open -- bad host");
	return ERR_XOBJ;
    }
    xTrace1(vnetp, TR_MAJOR_EVENTS, "remote IP host is %s",
	    ipHostStr(remIpHost));

    switch( getAddrClass(ps, remIpHost, &ifc) ) {
      case REMOTE_NET_ADDR_C:
	xTrace0(vnetp, TR_MAJOR_EVENTS, "vnetOpen: could not find interface");
	return ERR_XOBJ;

      case BCAST_SUBNET_ADDR_C:
      case BCAST_LOCAL_ADDR_C:
	return openBcastSessn(self, remIpHost, hlpRcv, hlpType, path);

      case BCAST_NET_ADDR_C:
      case LOCAL_ADDR_C:
      case REMOTE_HOST_ADDR_C:
      case MULTI_ADDR_C:
	break;

      default:
	xError("vnetOpen: unknown return type from getAddrClass");
	return ERR_XOBJ;
    }
    /* 
     * Unicast session
     */
    if ( resolve(ifc, remIpHost, &remPhysHost) == 0 ) {
	xTrace0(vnetp, TR_MAJOR_EVENTS, "vnetOpen: could not resolve addr");
	return ERR_XOBJ;
    }
    /* 
     * Resolved the address, we should be able to open a
     * local session  
     */
    xTrace1(vnetp, TR_EVENTS, "vnetOpen - resolution succeeded, addr = %s",
	    ethHostStr(&remPhysHost));
    partInit(&part, 1);
    partPush(part, &remPhysHost, sizeof(ETHhost));
    lls = xOpen(self, hlpType, ifc->llp, &part, path);
    if ( lls == ERR_XOBJ ) {
	xTrace0(vnetp, TR_MAJOR_EVENTS, "vnetOpen -- llp open failed.");
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &lls, &s) == XK_FAILURE ) {
	s = vnetCreateSessn(self, hlpRcv, hlpType, 1, &lls, &ifc,
			    ps->activeMap, (char *)&lls, path);
    } else {
	xTrace0(vnetp, TR_MAJOR_EVENTS, "vnetOpen -- session already exists");
	xClose(lls);
    }
    return s;
}


static XObj
vnetCreateSessn(
    XObj 	self,
    XObj 	hlpRcv,
    XObj 	hlpType,
    int		numDown,
    XObj 	*lls,
    Interface	**ifc,
    Map		map,
    void	*key,
    Path	path)
{
    XObj 	s;
    SState	*ss;
    int		i;

    s = xCreateSessn(vnetSessnInit, hlpRcv, hlpType, self, numDown, lls, path);
    if ( s == ERR_XOBJ ) {
	xTrace0(vnetp, TR_ERRORS, "xCreateSessn failed in vnetCreateSessn");
	for ( i=0; i < numDown; i++ ) {
	    xClose(lls[i]);
	}
	return ERR_XOBJ;
    }
    if ( mapBind(map, (char *)key, s, &s->binding) != XK_SUCCESS ) {
	xTrace0(vnetp, TR_SOFT_ERRORS, "mapBind failed in vnetCreateSessn");
	vnetClose(s);
	return ERR_XOBJ;
    }
    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	xTrace0(vnetp, TR_ERRORS, "allocation failure");
	vnetClose(s);
	return ERR_XOBJ;
    }
    bzero((char *)ss, sizeof(SState));
    s->state = ss;
    ss->flags.rewriteHosts = 1;
    ss->map = map;
    for ( i=0; i < numDown; i ++ ) {
	ss->ifcs[i].ifc = ifc[i];
	ss->ifcs[i].active = TRUE;
	/* 
	 * Cause the lls upper protocol pointer to point to this
	 * *session*, not to this protocol.
	 */
	lls[i]->up = s;
    }
    return s;
}


static xkern_return_t
vnetClose(
    XObj	s)
{
    XObj	lls;
    int		i;
    SState	*ss = (SState *)s->state;

    xAssert(xIsSession(s));
    for ( i=0; i < s->numdown; i++ ) {
	lls = xGetDown(s, i);
	/* 
	 * Cause the lower session's up pointer, previously pointing to
	 * this session, to point to the protocol
	 */
	lls->up = s->myprotl;
	xClose(lls);
    }
    if ( s->binding ) {
	mapRemoveBinding(ss->map, s->binding);
    }
    xDestroy(s);
    return XK_SUCCESS;
}


/* 
 * Figure out which of my lower protocols owns the lower session
 */
static Interface *
lls2ifc(
    XObj	self,
    XObj	lls)
{
    PState	*ps = (PState *)self->state;
    int		i;

    for ( i=0; i < ps->numIfc; i++ ) {
	if ( xMyProtl(lls) == ps->ifc[i].llp ) {
	    return &ps->ifc[i];
	}
    }
    xError("Impossible lower level session in VNET openDone");
    return 0;
}


static xkern_return_t
vnetOpenDone(
    XObj	self,
    XObj	lls,
    XObj	llp,
    XObj	hlpType,
    Path	path)
{
    XObj	s;
    PassiveKey	key;
    PState	*ps = (PState *)self->state;
    Enable	*e;
    Interface	*ifc;
    
    xTrace0(vnetp, 3, "In VNET OpenDone");
    key = hlpType;
    if ( mapResolve(ps->passiveMap, &key, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTrace0(vnetp, TR_ERRORS,
		"vnetOpenDone: Couldn't find hlp for new session");
	return XK_FAILURE;
    }
    if ( (ifc = lls2ifc(self, lls)) == 0 ) {
	return XK_FAILURE;
    }
    s = vnetCreateSessn(self, e->hlpRcv, e->hlpType, 1, &lls, &ifc,
			ps->activeMap, &lls, path);
    if ( s == ERR_XOBJ ) {
	return XK_FAILURE;
    }
    xTrace0(vnetp, TR_EVENTS,
	    "vnet Passively opened session successfully created");
    xDuplicate(lls);
    return xOpenDone(e->hlpRcv, s, self, path);
}


/* 
 * This shouldn't ever get called.  When a VNET session is created,
 * the up pointer of the lls is set to point to a VNET session. 
 */
static xkern_return_t
vnetProtlDemux(
    XObj	self,
    XObj	lls,
    Msg		m)
{
    xTrace0(vnetp, TR_ERRORS, "vnetProtlDemux called");
    return XK_SUCCESS;
}


static xkern_return_t
vnetPop(
    XObj	self,
    XObj	lls,
    Msg		msg,
    void	*h)
{
    SState	*ss = (SState *)self->state;

    xTrace0(vnetp, TR_EVENTS, "vnet Pop");
    xAssert(! ss->flags.isBcast);
    return xDemux(self, msg);
}


static xkern_return_t
vnetSessnDemux(
    XObj	self,
    XObj	lls,
    Msg		msg)
{
    xTrace0(vnetp, TR_EVENTS, "vnet Session Demux");
    return xPop(self, lls, msg, 0);
}


static xmsg_handle_t
vnetPush(
    XObj	self,
    Msg		m)
{
    xTrace0(vnetp, TR_EVENTS, "vnet push");
    return xPush(xGetDown(self, 0), m);
}


static xmsg_handle_t
vnetMultiPush(
    XObj	self,
    Msg		m)
{
    xmsg_handle_t	res = XMSG_NULL_HANDLE;
    int			i;
    Msg_s		msgCopy;
    SState		*ss = (SState *)self->state;

    xTrace0(vnetp, TR_EVENTS, "vnet push");
    msgConstructContig(&msgCopy, xMyPath(self), 0, 0);
    for ( i=0; i < self->numdown; i++ ) {
	if ( ss->ifcs[i].active ) {
	    msgAssign(&msgCopy, m);
	    if ( xPush(xGetDown(self, i), &msgCopy) == XMSG_ERR_HANDLE ) {
		res = XMSG_ERR_HANDLE;
	    }
	} else {
	    xTrace1(vnetp, TR_EVENTS, "vnet push, ifc %d inactive", i);
	}
    }
    msgDestroy(&msgCopy);
    return res;
}


static xkern_return_t
vnetOpenEnable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    PState	*ps = (PState *)self->state;

    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    ps->llpList, p);
}


static xkern_return_t
vnetOpenDisable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    PState	*ps = (PState *)self->state;

    return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
				     ps->llpList, p);
}

static xkern_return_t
vnetOpenDisableAll(
		   XObj	self,
		   XObj	hlpRcv)
{
    PState	*ps = (PState *)self->state;

    xTrace0(vnetp, TR_MAJOR_EVENTS, "vnetOpenDisableAll");
    return defaultVirtualOpenDisableAll(self, ps->passiveMap, hlpRcv, &ps->llpList[0]);
}

static int
bcastRewriteParts(
    SState	*ss,
    char	*buf)
{
    xAssert(0);
    /* 
     * Postpone until generic hosts are available
     */
    return 0;
}


static int
rewriteParts(
    SState	*ss,
    char	*buf,
    int		bufLen)
{
    Part_s	p[2];
    ArpBinding	ab[2];
    ETHhost	*eh;
    int	i	;

    /* 
     * Reverse-resolve the hosts
     */
    xTrace0(vnetp, TR_MORE_EVENTS, "vnet GETPARTICIPANTS RevResolving addrs");
    partInternalize(p, buf);
    for ( i=0; i < partLen(p); i++ ) {
	if ( (eh = (ETHhost *)partPop(p[i])) == 0 ) {
	    return -1;
	}
	ab[i].hw = *eh;
	if ( xControl(ss->ifcs[0].ifc->arp, RRESOLVE, (char *)&ab[i],
		      sizeof(ArpBinding)) < 0 ) {
	    xTrace1(vnetp, TR_ERRORS,
		    "VNET control could not rresolve eth host %s",
		    ethHostStr(&ab[i].hw));
	    return -1;
	}
	xTrace2(vnetp, TR_EVENTS,
		"VNET control rev-resolves %s as %s",
		ethHostStr(&ab[i].hw), ipHostStr(&ab[i].ip));
	partPush(p[i], &ab[i].ip, sizeof(IPhost));
    }
    return (partExternalize(p, buf, &bufLen) == XK_FAILURE) ? -1 : bufLen;
}    


static int
vnetControlSessn(
    XObj	self,
    int		opcode,
    char	*buf,
    int		len)
{
    XObj	lls = xGetDown(self, 0);
    SState	*ss = (SState *)self->state;
    PState	*ps = (PState *)(xMyProtl(self)->state);

    xTrace0(vnetp, TR_FUNCTIONAL_TRACE, "vnetControlSessn");

    switch (opcode) {
      case GETMYHOST:
	{
	    int	i;
	    checkLen(len, self->numdown * sizeof(IPhost));
	    for ( i=0; i < self->numdown; i++ ) {
		((IPhost *)buf)[i] = ss->ifcs[i].ifc->host;
	    }
	    return self->numdown * sizeof(IPhost);
	}

      case GETPEERHOST:
	{
	    ArpBinding	b;

	    checkLen(len, sizeof(IPhost));
	    if ( xControl(xGetDown(self, 0), GETPEERHOST,
			  (char *)&b.hw, sizeof(b.hw)) < (int)sizeof(b.hw) ) {
		xTrace0(vnetp, TR_ERRORS, "vnet ctlSessn couldn't get peer");
		return -1;
	    }
	    if ( xControl(ss->ifcs[0].ifc->arp, RRESOLVE,
			  (char *)&b, sizeof(b)) < (int)sizeof(b) ) {
		xTrace1(vnetp, TR_ERRORS,
			"vnet rresolve of %s failed", ethHostStr(&b.hw));
		return -1;
	    }
	    *(IPhost *)buf = b.ip;
	    xTrace1(vnetp, TR_EVENTS, "vnet ctlSessn GETPEER returns %s",
		    ipHostStr((IPhost *)buf));
	    return sizeof(IPhost);
	}
	
      case GETPARTICIPANTS:
	{
	    int	newLen;

	    if ( (newLen = xControl(lls, opcode, buf, len)) > 0 ) {
		if ( ss->flags.rewriteHosts ) {
		    if ( ss->flags.isBcast ) {
			return bcastRewriteParts(ss, buf);
		    } else {
			return rewriteParts(ss, buf, len);
		    }
		}
		return newLen;
	    } else {
		xTrace0(vnetp, TR_SOFT_ERRORS,
			"vnet GETPARTICIPANTS call on lls failed");
		return -1;
	    }
	}
	
      case VNET_HOSTONLOCALNET:
	{
	    Interface	*ifc;
	    int		i;

	    checkLen(len, sizeof(IPhost));
	    if ( (ifc = host2ifc(ps, (IPhost *)buf)) == 0 ) {
		return 0;
	    }
	    for ( i=0; i < self->numdown; i++ ) {
		if ( ifc == ss->ifcs[i].ifc ) {
		    return sizeof(IPhost);
		}
	    }
	    return 0;
	}

      case VNET_ISMYADDR:		
      case VNET_GETADDRCLASS:
	return vnetControlProtl(xMyProtl(self), opcode, buf, len);

      case VNET_GETINTERFACEID:
	{
	    checkLen(len, sizeof(void *));
	    if ( self->numdown > 1 ) {
		/* 
		 * More than one interface -- ID for 'the interface'
		 * is ambiguous 
		 */
		return 0;
	    }
	    *(void **)buf = (void *)ss->ifcs[0].ifc;
	    return sizeof(void *);
	}

      case VNET_DISABLEINTERFACE:
      case VNET_ENABLEINTERFACE:
	{
	    int	i;

	    checkLen(len, sizeof(void *));
	    for ( i=0; i < self->numdown; i++ ) {
		if ( ss->ifcs[i].ifc == *(Interface **)buf ) {
		    ss->ifcs[i].active =
		      (opcode == VNET_DISABLEINTERFACE) ? FALSE : TRUE;
		    return sizeof(void *);
		}
	    }
	    return 0;
	}

      case VNET_REWRITEHOSTS:
	checkLen(len, sizeof(int));
	ss->flags.rewriteHosts = ! (*(int *)buf == 0);
	return 0;
	    
     default:
	/*
	 * All other opcodes are forwarded to the lower session
	 */
	xAssert(xIsSession(xGetDown(self, 0)));
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}


/*
 * isMyAddr:  is this IP address one of mine?  (i.e., should packets
 * sent to this address be delivered locally (including broadcast))
 */
static int
isMyAddr(
    PState	*ps,
    IPhost	*h)
{
    int i;
    
    for( i = 0; i < ps->numIfc; i++ ) {
	if ( IP_EQUAL(ps->ifc[i].host, *h) ) {
	    return 1;
	}
    }
    /* 
     * Look for broadcast and multicast addresses.
     */
    if ( netMaskIsBroadcast(h) || netMaskIsMulticast(h) ) {
	return 1;
    }
    return 0;
}


static int
vnetControlProtl(
    XObj	self,
    int		opcode,
    char	*buf,
    int		len)
{
    PState	*ps = (PState *)self->state;

    switch (opcode) {
	
      case GETMYHOST :
	{
	    int i, n;
	    
	    checkLen(len, sizeof(IPhost));
	    n = len / sizeof(IPhost);
	    if ( n > ps->numIfc ) {
		n = ps->numIfc;
	    }
	    for ( i=0; i < n; i++ ) {
		((IPhost *)buf)[i] = ps->ifc[i].host;
	    }
	    return n * sizeof(IPhost);
	}

      case GETMYHOSTCOUNT:
	{
	    checkLen(len, sizeof(int));
	    *(int *)buf = ps->numIfc;
	    return sizeof(int);
	}

      case VNET_GETNUMINTERFACES:
	checkLen(len, sizeof(int));
	*(int *)buf = ps->numIfc;
	return sizeof(int);

      case VNET_GETADDRCLASS:
	{
	    VnetClassBuf	*cb = (VnetClassBuf *)buf;
	    checkLen(len, sizeof(VnetClassBuf));
	    cb->class = getAddrClass(ps, &cb->host, 0);
	    return sizeof(VnetClassBuf);
	}

      case VNET_HOSTONLOCALNET:
	{
	    checkLen(len, sizeof(IPhost));
	    if ( host2ifc(ps, (IPhost *)buf) == 0 ) {
		return 0;
	    }
	    return sizeof(IPhost);
	}

      case VNET_ISMYADDR:		
	checkLen(len, sizeof(IPhost));
	return isMyAddr(ps, (IPhost *)buf) ? sizeof(IPhost) : 0;

      case ADDMULTI:
      case DELMULTI:
	{
		ETHhost multi;
		Interface *ifc;

		if ( getAddrClass(ps, (IPhost *)buf, &ifc) != MULTI_ADDR_C ) {
			xTrace1(vnetp, TR_SOFT_ERRORS,
				"vnet ADDMULTI getAddrClass(%s) on non-multi",
				ipHostStr((IPhost *)buf));
			return -1;
		}
    		if (ifc == 0) {
			xTrace1(vnetp, TR_SOFT_ERRORS,
				"vnet getAddrClass(%s) didn't find ifc",
				ipHostStr((IPhost *)buf));
    			return -1;
    		}

		if ( resolve(ifc, (IPhost *)buf, &multi) == 0) {
			xTrace1(vnetp, TR_SOFT_ERRORS,
				"vnet resolve(%s) failed",
				ipHostStr((IPhost *)buf));
			return -1;
		}
		return xControl(ifc->llp, opcode,
			(char *)&multi, sizeof(multi));
	}

     default:
	/*
	 * All other opcodes are forwarded to the remote-network protocol
	 */
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}


/* 
 * Can this host be reached on any of our local interfaces?
 */
static Interface *
host2ifc(
    PState	*ps,
    IPhost	*h)
{
    int 	i;
    Interface	*ifc;
    IPhost	h1Net, h2Net;

    xTrace1(vnetp, TR_MORE_EVENTS, "VNET looking up interface for host %s",
	    ipHostStr(h));
    for ( i = 0; i < ps->numIfc; i++ ) {
	ifc = &ps->ifc[i];
	IP_AND( h1Net, *h, ifc->mask);
	IP_AND( h2Net, ifc->host, ifc->mask);
	if ( IP_EQUAL(h1Net, h2Net) ) {
	    xTrace1(vnetp, TR_MORE_EVENTS, "VNET returning interface %d", i);
	    return ifc;
	}
    }
    xTrace0(vnetp, TR_MORE_EVENTS, "VNET found no appropriate interface");
    return NULL;
}


/* 
 * Determine the AddrType of the given host.  ifcPtr is only valid in the
 * cases of
 *	LOCAL_ADDR_C, BCAST_NET_ADDR_C, REMOTE_HOST_ADDR_C, MULTI_ADDR_C
 */
static VnetAddrClass
getAddrClass(
    PState	*ps,
    IPhost	*h,
    Interface	**ifcPtr)
{
    int		i;
    Interface	*ifc;

    if ( IP_EQUAL(*h, IP_LOCAL_BCAST) ) {
	return BCAST_LOCAL_ADDR_C;
    }
    if ( netMaskIsMulticast(h) ) {
    	/* HACK HACK HACK */
    	if ( ifcPtr )
    		*ifcPtr = &ps->ifc[0];
	return MULTI_ADDR_C;
    }
    for ( i=0; i < ps->numIfc; i++ ) {
	ifc = &ps->ifc[i];
	if ( ifcPtr ) {
	    *ifcPtr = ifc;
	}
	if ( IP_EQUAL(*h, ifc->host) ) {
	    return LOCAL_ADDR_C;
	}
	if ( netMaskSubnetsEqual(h, &ifc->host) ) {
	    if ( netMaskIsBroadcast(h) ) {
		return BCAST_NET_ADDR_C;
	    }
	    return REMOTE_HOST_ADDR_C;
	}
	if ( ifc->subnet && netMaskIsBroadcast(h) ) {
	    return BCAST_SUBNET_ADDR_C;
	}
    }
    return REMOTE_NET_ADDR_C;
}


xkern_return_t
vnet_init(
    XObj	self)
{
    PState	*ps;
    int		i;
    Path	path = self->path;

    xTrace0(vnetp, TR_GROSS_EVENTS, "VNET init");
    xAssert(xIsProtocol(self));
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    self->state = ps;
    ps->bcastMap = mapCreate(VNET_BCAST_MAP_SZ, sizeof(IPhost), path);
    ps->activeMap = mapCreate(VNET_ACTIVE_MAP_SZ, sizeof(ActiveKey), path);
    ps->passiveMap = mapCreate(VNET_PASSIVE_MAP_SZ, sizeof(PassiveKey), path);
    if ( ! ps->bcastMap || ! ps->activeMap || ! ps->passiveMap ) {
	if ( ps->bcastMap ) mapClose(ps->bcastMap);
	if ( ps->activeMap ) mapClose(ps->activeMap);
	if ( ps->passiveMap ) mapClose(ps->passiveMap);
	pathFree(ps);
	return XK_NO_MEMORY;
    }
    ps->numIfc = 0;
    for ( i = 0; i < self->numdown; i += 2 ) {
	ArpBinding	bind;
	Interface	*ifc;
	IPhost		defMask;
	
	if ( ps->numIfc > VNET_MAX_INTERFACES ) {
	    xError("VNET -- too many interfaces");
	    continue;
	}
	ifc = &ps->ifc[ps->numIfc];
	ifc->llp = xGetDown(self, i);
	if ( ! xIsProtocol(ifc->llp) ) {
	    xTrace1(vnetp, TR_ERRORS, "ip couldn't get llp %d", i+1);
	    continue;
	}
	ps->llpList[ps->numIfc] = ifc->llp;
	ifc->arp = xGetDown(self, i+1);
	if ( ! xIsProtocol(ifc->arp) ) {

	    xTrace1(vnetp, TR_ERRORS, "ip couldn't get llp %d", i);
	    continue;
	}
	if ( xControl(ifc->arp, ARP_GETMYBINDING, (char *)&bind,
		      sizeof(bind)) < (int)sizeof(bind)) {
	    xTrace1(vnetp, TR_ERRORS, 
		    "ip couldn't do GETMYBINDING on llp %d", i);
	    continue;
	}
	ifc->host = bind.ip;
	netMaskFind(&ifc->mask, &ifc->host);
	netMaskDefault(&defMask, &ifc->host);
	ifc->subnet = ! IP_EQUAL(defMask, ifc->mask);
	ps->numIfc++;
    }
    ps->llpList[ps->numIfc] = 0;
    self->control = vnetControlProtl;
    self->open = vnetOpen;
    self->openenable = vnetOpenEnable;
    self->opendisable = vnetOpenDisable;
    self->opendisableall = vnetOpenDisableAll;
    self->opendone = vnetOpenDone;
    self->demux = vnetProtlDemux;
    return XK_SUCCESS;
}


static xkern_return_t
vnetSessnInit(
    XObj	s)
{
    s->close = vnetClose;
    s->demux = vnetSessnDemux;
    s->pop = vnetPop;
    s->push = vnetPush;
    s->control = vnetControlSessn;
    return XK_SUCCESS;
}

