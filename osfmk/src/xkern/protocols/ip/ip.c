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
 * ip.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * ip.c,v
 * Revision 1.102.1.5.1.2  1994/09/07  04:18:31  menze
 * OSF modifications
 *
 * Revision 1.102.1.5  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.102.1.4  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.102.1.3  1994/07/22  20:08:13  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.102.1.2  1994/04/23  00:41:41  menze
 * Added registration for allocator call-backs
 * Fragments are built with the same allocator as their message
 *
 * Revision 1.102.1.1  1994/04/14  21:42:11  menze
 * Uses allocator-based message interface
 *
 * Revision 1.102  1994/02/05  00:04:22  menze
 *   [ 1994/01/20          menze ]
 *   Length comparison and trace statement fix in ipPush
 *
 * Revision 1.101  1994/01/20  16:34:37  menze
 *   [ 1994/01/13          menze ]
 *   Now uses library routines for rom options
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/gc.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/ip/ip_i.h>
#include <xkern/include/prot/arp.h>
#include <xkern/protocols/ip/route.h>

static void		callRedirect( Event, void * );
static XObj		createLocalSessn( XObj, XObj, XObj,
					  ActiveId *, IPhost *, Path );
static void		destroyForwardSessn( XObj s );
static void		destroyNormalSessn( XObj s );
static void		destroySessn( XObj, Map );
static XObj		forwardSessn( XObj, ActiveId *, FwdId *, Path );
static XObj		fwdBcastSessn( XObj, XObj, ActiveId *, FwdId *, Path );
static XObjInitFunc	fwdSessnInit;
static IPhost *		getHost( Part );
static long		ipGetRelProtNum( XObj, XObj, char * );
static int 		get_ident( XObj );
static xkern_return_t	ipCloseSessn( XObj );
static XObj		ipCreateSessn( XObj, XObj, XObj, XObjInitFunc,
				       IPhost *, Path );
static int 		ipHandleRedirect( XObj );
static XObj		ipOpen( XObj, XObj, XObj, Part, Path );
static xkern_return_t	ipOpenDisable( XObj, XObj, XObj, Part );
static xkern_return_t	ipOpenDisableAll( XObj, XObj );
static xkern_return_t	ipOpenEnable( XObj, XObj, XObj, Part );
static xmsg_handle_t	ipPush( XObj, Msg );
static XObj		localPassiveSessn( XObj, ActiveId *, IPhost *, Path );
static XObjInitFunc	localSessnInit;
static int		routeChangeFilter( void *, void *, void * );
static void             ipPsFree( PState *ps );

#if XK_DEBUG
int     traceipp;
#endif /* XK_DEBUG */

IPhost	ipSiteGateway;


#define SESSN_COLLECT_INTERVAL	20 * 1000 * 1000	/* 20 seconds */
#define IP_MAX_PROT	0xff


static long
ipGetRelProtNum( hlp, llp, s )
    XObj	hlp, llp;
    char	*s;
{
    long	n;

    n = relProtNum(hlp, llp);
    if ( n == -1 ) {
	xTrace3(ipp, TR_SOFT_ERROR,
	       "%s: couldn't get prot num of %s relative to %s",
	       s, hlp->name, llp->name);
	return -1;
    }
    if ( n < 0 || n > 0xff ) {
	xTrace4(ipp, TR_SOFT_ERROR,
	       "%s: prot num of %s relative to %s (%d) is out of range",
	       s, hlp->name, llp->name, n);
	return -1;
    }
    return n;
}


static xkern_return_t
ipOpenDisableAll( self, hlpRcv )
    XObj	self, hlpRcv;
{
    PState	*ps = (PState *)self->state;
    
    xTrace0(ipp, TR_MAJOR_EVENTS, "ipOpenDisableAll");
    defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
    defaultOpenDisableAll(ps->passiveSpecMap, hlpRcv, 0);
    return XK_SUCCESS;
}


static void
ipPsFree( PState *ps )
{
    if ( ps->activeMap ) mapClose(ps->activeMap);
    if ( ps->fwdMap ) mapClose(ps->fwdMap);
    if ( ps->passiveMap ) mapClose(ps->passiveMap);
    if ( ps->passiveSpecMap ) mapClose(ps->passiveSpecMap);
    if ( ps->fragMap ) mapClose(ps->fragMap);
    pathFree(ps);
}

#define newMap(map, sz, keySz) 					\
	if ( ! ((map) = mapCreate((sz), (keySz), path)) ) {	\
	    ipPsFree(ps);						\
	    return XK_FAILURE;					\
	}		

/*
 * ip_init: main entry point to IP
 */
xkern_return_t
ip_init(self)
    XObj self;
{
    PState	*ps;
    Part_s	part;
    Event	ev1, ev2, ev3;
    Path	path = self->path;
    
    xTrace0(ipp, 1, "IP init");
    ipProcessRomFile(self);
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xError("No llp configured below IP");
	return XK_FAILURE;
    }
    /* initialize protocol-specific state */
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
    bzero((char *)ps, sizeof(PState));
    self->state = (char *) ps;
    ps->self = self;
    newMap(ps->activeMap, IP_ACTIVE_MAP_SZ, sizeof(ActiveId));
    newMap(ps->fwdMap, IP_FORWARD_MAP_SZ, sizeof(FwdId));
    newMap(ps->passiveMap, IP_PASSIVE_MAP_SZ, sizeof(PassiveId));
    newMap(ps->passiveSpecMap, IP_PASSIVE_SPEC_MAP_SZ, sizeof(PassiveSpecId));
    newMap(ps->fragMap, IP_FRAG_MAP_SZ, sizeof(FragId));
    xTrace1(ipp, 2, "IP has %d protocols below\n", self->numdown);
    /* 
     * Determine number of interfaces used by the lower protocol --
     * knowing this will simplify some of our routing decisions
     */
    if ( xControl(xGetDown(self, 0), VNET_GETNUMINTERFACES,
		  (char *)&ps->numIfc, sizeof(int)) <= 0 ) {
	xError("Couldn't do GETNUMINTERFACES control op");
	ps->numIfc = 1;
    } else {
	xTrace1(ipp, TR_MAJOR_EVENTS, "llp has %d interfaces", ps->numIfc);
    }
    /*
     * initialize route table and set up default route
     */
    if ( rt_init(self, &ipSiteGateway) ) {
	xTrace0(ipp, TR_MAJOR_EVENTS, "IP rt_init -- no default gateway");
    }
    /*
     * set up function pointers for IP protocol object
     */
    self->open = ipOpen;
    self->control = ipControlProtl;
    self->openenable = ipOpenEnable;
    self->opendisable = ipOpenDisable;
    self->demux = ipDemux;
    self->opendisableall = ipOpenDisableAll;
    if ( allocRegister(0, ipAllocCallBack, self) == XK_FAILURE ) {
	ipPsFree(ps);
	return XK_FAILURE;
    }
    /*
     * openenable physical network protocols
     */
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);	
    if ( xOpenEnable(self, self, xGetDown(self, 0), &part) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "ip_init : can't openenable net protocols");
    }
    ev1 = scheduleIpFragCollector(ps, path);
    ev2 = initSessionCollector(ps->activeMap, SESSN_COLLECT_INTERVAL,
			       destroyNormalSessn, path, "ip");
    ev3 = initSessionCollector(ps->fwdMap, SESSN_COLLECT_INTERVAL,
			       destroyForwardSessn, path, "ip forwarding");
    if ( ! ev1 || !ev2 || !ev3 ) {
	if ( ev1 ) { evCancel(ev1); evDetach(ev1); }
	if ( ev2 ) { evCancel(ev2); evDetach(ev2); }
	if ( ev3 ) { evCancel(ev3); evDetach(ev3); }
	ipPsFree(ps);
	return XK_FAILURE;
    }
    xTraceP0(self, TR_EVENTS, "IP init done");
    return XK_SUCCESS;
}
#undef newMap


static IPhost *
getHost( p )
    Part	p;
{
    IPhost	*h;

    if ( !p || (partLen(p) < 1) ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ipGetHost: participant list error");
	return 0;
    }
    h = (IPhost *)partPop(p[0]);
    if ( h == 0 ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ipGetHost: empty participant stack");
    }
    return h;
}


/*
 * ipOpen
 */
static XObj
ipOpen(self, hlpRcv, hlpType, p, path)
    XObj self, hlpRcv, hlpType;
    Part p;
    Path path;
{
    XObj	ip_s;
    IPhost      *remoteHost;
    IPhost      *localHost = 0;
    ActiveId    activeid;
    long	hlpNum;
    
    xTrace0(ipp, 3, "IP open");
    if ( (remoteHost = getHost(p)) == 0 ) {
	return ERR_XOBJ;
    }
    if ( (hlpNum = ipGetRelProtNum(hlpType, self, "open")) == -1 ) {
	return ERR_XOBJ;
    }
    if ( partLen(p) > 1 ) {
	/* 
	 * Local participant has been explicitly specified
	 */
	localHost = (IPhost *)partPop(p[1]);
	if ( localHost == (IPhost *)ANY_HOST ) {
	    localHost = 0;
	}
    }
    xTrace2(ipp, 5, "IP sends to %s, %d", ipHostStr(remoteHost), hlpNum);
    
    /*
     * key on hlp prot number, destination addr, and local addr (if given)
     */
    activeid.protNum = hlpNum;
    activeid.remote = *remoteHost;
    if ( localHost ) {
	activeid.local = *localHost;
    }
    ip_s = createLocalSessn( self, hlpRcv, hlpType, &activeid, localHost, path );
    if ( ip_s != ERR_XOBJ ) {
	ip_s->idle = FALSE;
    }
    xTrace1(ipp, 3, "IP open returns %x", ip_s);
    return ip_s;
}


/* 
 * Create an IP session which sends to remote host key->dest.  The
 * 'rem' and 'prot' fields of 'key' will be used as passed in.
 *
 * 'localHost' specifies the host to be used in the header for
 * outgoing packets.  If localHost is null, an appropriate localHost will
 * be selected and used as the 'local' field of 'key'.  If localHost
 * is non-null, the 'local' field of 'key' will not be modified.
 */
static XObj
createLocalSessn( self, hlpRcv, hlpType, key, localHost, path )
    XObj	self, hlpRcv, hlpType;
    ActiveId 	*key;
    IPhost 	*localHost;
    Path	path;
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    IPheader	*iph;
    IPhost	host;
    XObj	s;
    xkern_return_t xkr;
    
    s = ipCreateSessn(self, hlpRcv, hlpType, localSessnInit, &key->remote, path);
    if ( s == ERR_XOBJ ) {
	return s;
    }
    /*
     * Determine my host address
     */
    if ( localHost ) {
	if ( ! ipIsMyAddr(self, localHost) ) {
	    xTrace1(ipp, TR_SOFT_ERROR, "%s is not a local IP host",
		    ipHostStr(localHost));
	    return ERR_XOBJ;
	}
    } else {
	if ( xControl(xGetDown(s, 0), GETMYHOST, (char *)&host,
		      sizeof(host)) < (int)sizeof(host) ) {
	    xTrace0(ipp, TR_SOFT_ERROR,
		    "IP open could not get interface info for remote host");
	    destroyNormalSessn(s);
	    return ERR_XOBJ;
	}
	localHost = &host;
	key->local = *localHost;
    }
    if ( (xkr = mapBind(ps->activeMap, key, s, &s->binding)) != XK_SUCCESS ) {
	if ( xkr ==  XK_ALREADY_BOUND ) {
	    xTraceP0(self, TR_MAJOR_EVENTS, "open -- session already existed");
	    destroyNormalSessn(s);
	    s = ERR_XOBJ;
	    xkr = mapResolve(ps->activeMap, key, &s);
	    xAssert( xkr == XK_SUCCESS );
	    return s;
	}
	destroyNormalSessn(s);
	return ERR_XOBJ;
    }
    ss = (SState *)s->state;
    iph = &ss->hdr;
    iph->source = *localHost;
    /*
     * fill in session template header
     */
    iph->vers_hlen = IPVERS;
    iph->vers_hlen |= 5;	/* default hdr length */
    iph->type = 0;
    iph->time = IPDEFAULTDGTTL;
    iph->prot = key->protNum;
    xTrace1(ipp, 4, "IP open: my ip address is %s",
	    ipHostStr(&iph->source));
    return s;
}


static XObj
ipCreateSessn( self, hlpRcv, hlpType, f, dst, path )
    XObj 	self, hlpRcv, hlpType;
    XObjInitFunc	f;
    IPhost	*dst;
    Path	path;
{
    XObj	s;
    SState	*ss;

    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	return ERR_XOBJ;
    }
    if ( ! (ss->ev = evAlloc(path)) ) {
	pathFree(ss);
	return ERR_XOBJ;
    }
    s = xCreateSessn(f, hlpRcv, hlpType, self, 0, 0, path);
    if ( s == ERR_XOBJ ) {
	evDetach(ss->ev);
	pathFree(ss);
	return ERR_XOBJ;
    }
    s->state = ss;
    bzero((char *)ss, sizeof(SState));
    ss->hdr.dest = *dst;
    if ( ipHandleRedirect(s) ) {
	xTrace0(ipp, 3, "IP open fails");
	destroyNormalSessn(s);
	return ERR_XOBJ;
    }
    return s;
}


static xkern_return_t
localSessnInit(self)
     XObj self;
{
    self->push = ipPush;
    self->pop = ipStdPop;
    self->control = ipControlSessn;
    self->close = ipCloseSessn;
    return XK_SUCCESS;
}


static xkern_return_t
fwdSessnInit(self)
     XObj self;
{
    self->pop = ipForwardPop;
    return XK_SUCCESS;
}


/*
 * ipOpenEnable
 */
static xkern_return_t
ipOpenEnable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part p;
{
    PState 	*pstate = (PState *)self->state;
    IPhost	*localHost;
    long	protNum;
    
    xTrace0(ipp, TR_MAJOR_EVENTS, "IP open enable");
    if ( (localHost = getHost(p)) == 0 ) {
	return XK_FAILURE;
    }
    if ( (protNum = ipGetRelProtNum(hlpType, self, "ipOpenEnable")) == -1 ) {
	return XK_FAILURE;
    }
    if ( localHost == (IPhost *)ANY_HOST ) {
	xTrace1(ipp, TR_MAJOR_EVENTS, "ipOpenEnable binding protocol %d",
		protNum);
	return defaultOpenEnable(pstate->passiveMap, hlpRcv, hlpType,
				 &protNum);
    } else {
	PassiveSpecId	key;

	if ( netMaskIsMulticast(localHost) ) {
	    /*
	     * Have we joined this multicast group?
	     */
	    xTrace1(ipp, TR_MAJOR_EVENTS,
		    "ipOpenEnable -- looking for %s multicast group",
		    ipHostStr(localHost));
	    /* Is this too picky? */
	    if (ipGroupQuery(localHost, xGetDown(self, 0)) == 0)
		return XK_FAILURE;
	} else if ( ! ipIsMyAddr(self, localHost) ) {
	    xTrace1(ipp, TR_MAJOR_EVENTS,
		    "ipOpenEnable -- %s is not one of my hosts",
		    ipHostStr(localHost));
	    return XK_FAILURE;
	}
	key.host = *localHost;
	key.prot = protNum;
	xTrace2(ipp, TR_MAJOR_EVENTS,
		"ipOpenEnable binding protocol %d, host %s",
		key.prot, ipHostStr(&key.host));
	return defaultOpenEnable(pstate->passiveSpecMap, hlpRcv, hlpType,
				 &key);
    }
}


/*
 * ipOpenDisable
 */
static xkern_return_t
ipOpenDisable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part p;
{
    PState      *pstate = (PState *)self->state;
    IPhost	*localHost;
    long	protNum;
    
    xTrace0(ipp, 3, "IP open disable");
    xAssert(self->state);
    xAssert(p);

    if ( (localHost = getHost(p)) == 0 ) {
	return XK_FAILURE;
    }
    if ( (protNum = ipGetRelProtNum(hlpType, self, "ipOpenDisable")) == -1 ) {
	return XK_FAILURE;
    }
    if ( localHost == (IPhost *)ANY_HOST ) {
	xTrace1(ipp, TR_MAJOR_EVENTS,
		"ipOpenDisable unbinding protocol %d", protNum);
	return defaultOpenDisable(pstate->passiveMap, hlpRcv, hlpType,
				  &protNum);
    } else {
	PassiveSpecId	key;

	key.host = *localHost;
	key.prot = protNum;
	xTrace2(ipp, TR_MAJOR_EVENTS,
		"ipOpenDisable unbinding protocol %d, host %s",
		key.prot, ipHostStr(&key.host));
	return defaultOpenDisable(pstate->passiveSpecMap, hlpRcv, hlpType,
				  &key);
    }
}


/*
 * ipCloseSessn
 */
static xkern_return_t
ipCloseSessn(s)
    XObj s;
{
  xTrace1(ipp, 3, "IP close of session %x (does nothing)", s);
  xAssert(xIsSession(s));
  xAssert( s->rcnt == 0 );
  return XK_SUCCESS;
}


static void
destroyForwardSessn(s)
    XObj s;
{
    PState 	*ps = (PState *)(xMyProtl(s))->state;

    destroySessn(s, ps->fwdMap);
}    
  

static void
destroySessn(s, map)
    XObj 	s;
    Map		map;
{
    int		i;
    XObj	lls;
    SState	*ss = (SState *)s->state;

    xTrace1(ipp, 3, "IP DestroySessn %x", s);
    xAssert(xIsSession(s));
    if ( s->binding ) {
	xAssert(map);
	mapRemoveBinding(map, s->binding);
    }
    for (i=0; i < s->numdown; i++ ) {
	lls = xGetDown(s, i);
	if ( xIsSession(lls) ) {
	    xClose(lls);
	}
    }
    if ( ss->ev ) {
	evDetach(ss->ev);
    }
    xDestroy(s);
}    
  

static void
destroyNormalSessn(s)
    XObj s;
{
    PState 	*ps = (PState *)(xMyProtl(s))->state;

    destroySessn(s, ps->activeMap);
}    
  


/*
 * ipPush
 */
static xmsg_handle_t
ipPush(s, msg)
    XObj s;
    Msg msg;
{
    SState	*sstate;
    IPheader	hdr;
    int		dlen;
    
    xAssert(xIsSession(s));
    sstate = (SState *) s->state;
    
    hdr = sstate->hdr;
    hdr.ident = get_ident(s);
    dlen = msgLen(msg) + (GET_HLEN(&hdr) * 4);
    if ( dlen > IPMAXPACKET ) {
	xTrace2(ipp, TR_SOFT_ERRORS, "ipPush: msgLen(%d) > MAXPACKET(%d)",
		dlen, IPMAXPACKET);
	return XMSG_ERR_HANDLE;
    }
    hdr.dlen = dlen;
    return ipSend(s, xGetDown(s, 0), msg, &hdr);
}


/*
 * Send the msg over the ip session's down session, fragmenting if necessary.
 * All header fields not directly related to fragmentation should already
 * be filled in.  We only reference the 'mtu' field of s->state (this
 * could be a forwarding session with a vestigial header in s->state,
 * so we use the header passed in as a parameter.)
 */
xmsg_handle_t
ipSend(s, lls, msg, hdr)
    XObj	s, lls;
    Msg 	msg;
    IPheader 	*hdr;
{
    int 	hdrLen;
    int		len;
    SState	*sstate;

    sstate = (SState *)s->state;
    len = msgLen(msg);
    hdrLen = GET_HLEN(hdr);
    if ( len + hdrLen * 4 <= sstate->mtu ) {
	/*
	 * No fragmentation
	 */
	xTrace0(ipp,5,"IP send : message requires no fragmentation");
	if ( msgPush(msg, ipHdrStore, hdr, hdrLen * 4, NULL) == XK_FAILURE ) {
	    xTraceP0(s, TR_ERRORS, "msgPush fails");
	    return XMSG_ERR_HANDLE;
	}
	xIfTrace(ipp,5) {
	    xTrace0(ipp,5,"IP send unfragmented datagram header: \n");
	    ipDumpHdr(hdr);
	}
	return xPush(lls, msg);
    } else {
	/*
	 * Fragmentation required
	 */
	int 	fragblks;
	int	fragsize;
	Msg_s	fragmsg;
	int	offset;
	int	fraglen;
	xmsg_handle_t handle = XMSG_NULL_HANDLE;
	
	if ( hdr->frag & DONTFRAGMENT ) {
	    xTrace0(ipp,5,
		    "IP send: fragmentation needed, but NOFRAG bit set");
	    return XMSG_NULL_HANDLE;  /* drop it */
	}
	fragblks = (sstate->mtu - (hdrLen * 4)) / 8;
	fragsize = fragblks * 8;
	xTrace0(ipp,5,"IP send : datagram requires fragmentation");
	xIfTrace(ipp,5) {
	    xTrace0(ipp,5,"IP original datagram header :");
	    ipDumpHdr(hdr);
	}
	/*
	 * fragmsg = msg;
	 */
	xAssert(xIsXObj(lls));
	msgConstructContig(&fragmsg, msgGetPath(msg), 0, 0);
	for( offset = 0; len > 0; len -= fragsize, offset += fragblks) {
	    IPheader  	hdrToPush;
	    
	    hdrToPush = *hdr;
	    fraglen = len > fragsize ? fragsize : len;
	    msgChopOff(msg, &fragmsg, fraglen);
	    /*
	     * eventually going to need to selectively copy options
	     */
	    hdrToPush.frag += offset;
	    if ( fraglen != len ) {
		/*
		 * more fragments
		 */
		hdrToPush.frag |= MOREFRAGMENTS;
	    }
	    hdrToPush.dlen = hdrLen * 4 + fraglen;
	    xIfTrace(ipp,5) {
		xTrace0(ipp,5,"IP datagram fragment header: \n");
		ipDumpHdr(&hdrToPush);
	    }
	    if ( msgPush(&fragmsg, ipHdrStore, &hdrToPush, hdrLen * 4, NULL)
			== XK_FAILURE ) {
		break;
	    }
	    if ( (handle =  xPush(lls, &fragmsg)) == XMSG_ERR_HANDLE ) {
		break;
	    }
	}
	msgDestroy(&fragmsg);
	return ( handle == XMSG_ERR_HANDLE ) ? handle : XMSG_NULL_HANDLE;
    }
}


Enable *
ipFindEnable( self, hlpNum, localHost )
    XObj	self;
    int		hlpNum;
    IPhost	*localHost;
{
    PState		*ps = (PState *)self->state;
    Enable		*e = ERR_ENABLE;
    PassiveId		key = hlpNum;
    PassiveSpecId	specKey;

    if ( mapResolve(ps->passiveMap, &key, &e) == XK_SUCCESS ) {
	xTrace1(ipp, TR_MAJOR_EVENTS,
		"Found an enable object for prot %d", key);
    } else {
	specKey.prot = key;
	specKey.host = *localHost;
	if ( mapResolve(ps->passiveSpecMap, &specKey, &e) == XK_SUCCESS ) {
	    xTrace2(ipp, TR_MAJOR_EVENTS,
		    "Found an enable object for prot %d host %s",
		    specKey.prot, ipHostStr(&specKey.host));
	}
    }
    return e;
}


static XObj
localPassiveSessn( self, actKey, localHost, path )
    XObj 	self;
    ActiveId 	*actKey;
    IPhost	*localHost;
    Path	path;
{
    Enable		*e;

    e = ipFindEnable(self, actKey->protNum, localHost);
    if ( e == ERR_ENABLE ) {
	return ERR_XOBJ;
    }
    return createLocalSessn(self, e->hlpRcv, e->hlpType, actKey, localHost, path); 
    /* 
     * openDone will get called in validateOpenEnable
     */
}


static XObj
fwdBcastSessn( self, llsIn, actKey, fwdKey, path )
    XObj 	self, llsIn;
    ActiveId 	*actKey;
    FwdId	*fwdKey;
    Path	path;
{
    XObj	s;
    Part_s	p;
    XObj	lls;
    IPhost	localHost;

    xTrace0(ipp, TR_MAJOR_EVENTS, "creating forward broadcast session");
    if ( xControl(llsIn, GETMYHOST, (char *)&localHost, sizeof(IPhost)) < 0 ) {
	return ERR_XOBJ;
    }
    if ( (s = localPassiveSessn(self, actKey, &localHost, path)) == ERR_XOBJ ) {
	/* 
	 * There must not have been an openenable for this msg type --
	 * this will just be a forwarding session
	 */
	if ( (s = forwardSessn(self, actKey, fwdKey, path)) == ERR_XOBJ ) {
	    return ERR_XOBJ;
	}
	xSetDown(s, 1, xGetDown(s, 0));
	xSetDown(s, 0, 0);
    } else {
	/* 
	 * This will be a local session with an extra down session for
	 * the forwarding of broadcasts
	 */
	partInit(&p, 1);
	partPush(p, &actKey->local, sizeof(IPhost));
	lls = xOpen(self, self, xGetDown(self, 0), &p, path);
	if ( lls == ERR_XOBJ ) {
	    xTrace0(ipp, TR_ERRORS, "ipFwdBcastSessn couldn't open lls");
	    return ERR_XOBJ;
	}
	xSetDown(s, 1, lls);
    }
    s->pop = ipFwdBcastPop;
    return s;
}


static XObj
forwardSessn( self, actKey, fwdKey, path )
    XObj	self;
    ActiveId	*actKey;
    FwdId	*fwdKey;
    Path	path;
{
    PState	*ps = (PState *)self->state;
    XObj	s;

    xTraceP2(self, TR_MAJOR_EVENTS,
	    "creating forwarding session to net %s (host %s)",
	    ipHostStr(fwdKey), ipHostStr(&actKey->local));
    s = ipCreateSessn(self, xNullProtl, xNullProtl, fwdSessnInit,
		      &actKey->local, path);
    if ( s == ERR_XOBJ ) {
	return s;
    }
    if ( mapBind(ps->fwdMap, fwdKey, s, &s->binding) != XK_SUCCESS ) {
	destroySessn(s, 0);
	return ERR_XOBJ;
    }
    return s;
}


XObj
ipCreatePassiveSessn( self, lls, actKey, fwdKey, path )
    XObj 	self, lls;
    ActiveId	*actKey;
    FwdId	*fwdKey;
    Path	path;
{
    PState		*ps = (PState *)self->state;
    VnetClassBuf	buf;
    XObj		s = ERR_XOBJ;

    buf.host = actKey->local;
    if ( xControl(xGetDown(self, 0), VNET_GETADDRCLASS,
		  (char *)&buf, sizeof(buf)) < (int)sizeof(buf) ) {
	xTrace0(ipp, TR_ERRORS,
		"ipCreatePassiveSessn: GETADDRCLASS failed");
	return ERR_XOBJ;
    }
    switch( buf.class ) {
      case LOCAL_ADDR_C:
	/* 
	 * Normal session 
	 */
	s = localPassiveSessn(self, actKey, &actKey->local, path);
	break;

      case REMOTE_HOST_ADDR_C:
      case REMOTE_NET_ADDR_C:
	s = forwardSessn(self, actKey, fwdKey, path);
	break;
	    
      case BCAST_SUBNET_ADDR_C:
	if ( ps->numIfc > 1 ) {
	    /* 
	     * Painfully awkward forward/local consideration session
	     */
	    s = fwdBcastSessn(self, lls, actKey, fwdKey, path);
	    break;
	}
	/* 
	 * Else fallthrough
	 */

      case BCAST_LOCAL_ADDR_C:
      case BCAST_NET_ADDR_C:
	{
	    IPhost	localHost;

	    /* 
	     * Almost a normal session -- need to be careful about our
	     * source address 
	     */
	    if ( xControl(lls, GETMYHOST, (char *)&localHost, sizeof(IPhost))
		< 0 ) {
		return ERR_XOBJ;
	    }
	    s = localPassiveSessn(self, actKey, &localHost, path);
	}
	break;

      case MULTI_ADDR_C:
	/*
	 * Rewrite actKey to ignore source address.
	 * This puts a single mcast group in a single session.
	 */
/*	bzero((char *)&actKey->remote, (char *)&actKey->remotesizeof actKey->remote); */
	actKey->remote = actKey->local;
	s = localPassiveSessn(self, actKey, &actKey->local, path);
	break;

    }
    return s;
}


/*
 * ipHandleRedirect -- called when the ip session's lower session needs
 * to be (re)opened.  This could be when the ip session is first created
 * and the lower session is noneistent, or when a redirect is received
 * for this session's remote network.  The router is
 * consulted for the best interface.  The new session is assigned to
 * the first position in the ip session's down vector.  The old session,
 * if it existed, is freed.
 *
 * Note that the local IPhost field of the header doesn't change even
 * if the route changes.
 * 
 * preconditions: 
 * 	s->state should be allocated
 * 	s->state->hdr.dest should contain the ultimate remote address or net
 *
 * return values:
 *	0 if lower session was succesfully opened and assigned
 *	1 if lower session could not be opened -- old lower session is
 *		not affected
 */
static int
ipHandleRedirect(s)
    XObj s;
{
    XObj	ip = xMyProtl(s);
    XObj	llp = xGetDown(ip, 0);
    XObj 	lls, llsOld;
    SState	*ss = (SState *)s->state;
    PState	*ps = (PState *)ip->state;
    route	rt;
    Part_s	p;
    int		res;
    int		off = 0;
    
    /*
     * 'host' is the remote host to which this session sends packets,
     * not necessarily the final destination
     */
    xAssert(xIsSession(s));
    partInit(&p, 1);
    partPush(p, &ss->hdr.dest, sizeof(IPhost));
    if ( (lls = xOpen(ip, ip, llp, &p, xMyPath(s))) == ERR_XOBJ ) {
	xTrace0(ipp, TR_EVENTS,
		"ipHandleRedirect could not get direct lower session");
	if ( rt_get(&ps->rtTbl, &ss->hdr.dest, &rt) == XK_FAILURE ) {
	    xTrace0(ipp, TR_SOFT_ERRORS,
		    "ipHandleRedirect could not find route");
	    return 1;
	}
	partInit(&p, 1);
	partPush(p, &rt.gw, sizeof(IPhost));
	if ( (lls = xOpen(ip, ip, llp, &p, xMyPath(s))) == ERR_XOBJ ) {
	    xTrace0(ipp, TR_ERRORS,
		    "ipHandleRedirect could not get gateway lower session");
	    return 1;
	}
    }
    xTrace0(ipp, 5, "Successfully opened lls");
    res = xControl(lls, VNET_REWRITEHOSTS, (char *)&off, sizeof(off));
    if ( res < 0 ) {
	xTraceP0(s, TR_ERRORS, "VNET_REWRITEHOSTS control op fails");
	xClose(lls);
	return 1;
    }
    /*
     * Determine mtu for this interface
     */
    res = xControl(lls, GETMAXPACKET, (char *)&ss->mtu, sizeof(int));
    if (res < 0 || ss->mtu <= 0) {
	xTrace0(ipp, 3, "Could not determine interface mtu");
	ss->mtu = IPOPTPACKET;
    }
    if ( xIsSession(llsOld = xGetDown(s, 0)) ) {
	xClose(llsOld);
    }
    xSetDown(s, 0, lls);
    return 0;
}


/* 
 * Misc. routines 
 */
static int
get_ident( s )
    XObj	s;
{
    static int n = 1;
    return n++;
}


static void
callRedirect(ev, s)
    Event	ev;
    void	*s;
{
    xTrace1(ipp, 4, "ip: callRedirect runs with session %x", s);
    ipHandleRedirect((XObj) s);
    xClose((XObj)s);
    return;
}


typedef struct {
    int 	(* affected)(PState *, IPhost *, route *);
    route	*rt;
    PState	*pstate;
} RouteChangeInfo;

/* 
 * ipRouteChanged -- For each session in the active map, determine if a
 * change in the given route affects that session.  If it does, the
 * session is reconfigured appropriately.  This function does not block. 
 */
void
ipRouteChanged(pstate, rt, routeAffected)
    PState *pstate;
    route *rt;
    int (*routeAffected)(PState *, IPhost *, route *);
{
    RouteChangeInfo	rInfo;

    rInfo.affected = routeAffected;
    rInfo.rt = rt;
    rInfo.pstate = pstate;
    mapForEach(pstate->activeMap, routeChangeFilter, &rInfo);
    mapForEach(pstate->fwdMap, routeChangeFilter, &rInfo);
}
  

static int
routeChangeFilter(key, value, arg)
    void *key, *value, *arg;
{
    RouteChangeInfo	*rInfo = (RouteChangeInfo *)arg;
    XObj		s = (XObj)value;
    SState		*state;
    
    xAssert(xIsSession(s));
    state = (SState *)s->state;
    xTrace3(ipp, 4, "ipRouteChanged does net %s affect ses %x, dest %s?",
	    ipHostStr(&rInfo->rt->net), s, ipHostStr(&state->hdr.dest));
    if ( rInfo->affected(rInfo->pstate, &state->hdr.dest, rInfo->rt) ) {
	xTrace1(ipp, 4,
		"session %x affected -- reopening lower session", s);
	xDuplicate(s);
	evSchedule(state->ev, callRedirect, s, 0);
    } else {
	xTrace1(ipp, 4, "session %x unaffected by routing change", s);
    }
    return MFE_CONTINUE;
}



	/*
	 * Functions used as arguments to ipRouteChanged
	 */
/*
 * Return true if the remote host is not connected to the local net
 */
int
ipRemoteNet( ps, dest, rt )
    PState	*ps;
    IPhost 	*dest;
    route 	*rt;
{
    return ! ipHostOnLocalNet(ps, dest);
}


/*
 * Return true if the remote host is on the network described by the
 * route.
 */
int
ipSameNet(pstate, dest, rt)
    PState *pstate;
    IPhost *dest;
    route *rt;
{
    return ( (dest->a & rt->mask.a) == (rt->net.a & rt->mask.a) &&
	     (dest->b & rt->mask.b) == (rt->net.b & rt->mask.b) &&
	     (dest->c & rt->mask.c) == (rt->net.c & rt->mask.c) &&
	     (dest->d & rt->mask.d) == (rt->net.d & rt->mask.d) );
}
