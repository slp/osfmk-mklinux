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
 * eth.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * eth.c,v
 * Revision 1.58.1.9.1.2  1994/09/07  04:18:41  menze
 * OSF modifications
 *
 * Revision 1.58.1.9  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.58.1.8  1994/09/01  18:58:05  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 * VCI_ETH_NET_TYPE stored rather than recalculated
 *
 * Revision 1.58.1.7  1994/08/11  16:45:06  menze
 * Removed sscanf call
 *
 * Revision 1.58.1.6  1994/07/22  20:08:11  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.58.1.5  1994/04/23  00:24:35  menze
 * Correction to MAXPACKET for VCI header
 *
 * Revision 1.58.1.4  1994/04/21  17:24:44  menze
 * xMyAllocator became xMyAlloc
 *
 * Revision 1.58.1.3  1994/04/14  21:38:35  menze
 * Uses allocator-based message interface
 *
 * Revision 1.58.1.2  1994/04/05  22:20:03  menze
 * Bug fixes in VCI code
 *
 * Revision 1.58.1.1  1994/03/31  17:15:32  menze
 * Added VCI support
 *
 * Revision 1.58  1993/12/13  22:46:46  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

/*
 * The xkernel ethernet driver is structured in two layers.
 *
 * The ethernet protocol layer (this file) is independent of any
 * particular ethernet controller hardware.
 * It comprises the usual xkernel protocol functions,
 * e.g. eth_open, eth_push, etc.).
 * It knows about ethernet addresses and "types,"
 * but nothing about any particular ethernet controller.
 *
 * The device driver, which exports an xkernel interface, sits below
 * this protocol
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/romopt.h>
#include <xkern/include/prot/eth.h>
#include <xkern/protocols/eth/eth_i.h>

typedef struct {
    ETHhdr	hdr;
    ETHhost	rcvHost;
    int		hlp_num;	/* HLP number in host byte order */
} SState;

typedef struct {
    ETHhost	remote;
    ETHhost	local;
    ETHtype	type;
} ActiveId;

typedef struct {
    XObj    	prmSessn;
    ETHhost 	myHost;
    Map		actMap;
    Map 	pasMap;
    int		mtu;
} PState;

typedef ETHtype  PassiveId;

typedef struct {
    Msg_s	msg;
    XObj	self;
    XObj	llp;
    ETHhdr	hdr;
} RetBlock;

#define	ETH_ACTIVE_MAP_SZ	257
#define	ETH_PASSIVE_MAP_SZ	13

ETHtype	vciEthNetType;

#if XK_DEBUG

int 	traceethp;
int 	tracevci;
static ETHhost	ethBcastHost = BCAST_ETH_AD;

#endif /* XK_DEBUG */


static void		demuxStub( Event, void * );
static int		dispActiveMap( void *, void *, void * );
static int		dispPassiveMap( void *, void *, void * );
static XObj		ethCreateSessn( XObj, XObj, XObj, ActiveId *, Path );
static xkern_return_t	ethSessnInit( XObj );
static int		ethControlProtl( XObj, int, char *, int );
static xkern_return_t	ethClose( XObj );
static int		ethControlSessn( XObj, int, char *, int );
static xkern_return_t	ethDemux( XObj, XObj, Msg );
static XObj		ethOpen( XObj, XObj, XObj, Part, Path );
static xkern_return_t	ethOpenEnable( XObj, XObj, XObj, Part );
static xkern_return_t	ethOpenDisable( XObj, XObj, XObj, Part );
static xkern_return_t	ethOpenDisableAll( XObj, XObj );
static long		ethGetRelProtNum( XObj, XObj, char * );
static xkern_return_t	readMtu( XObj, char **, int, int, void * );
static void 		ethPsFree( PState	*ps );
static long	 	vciHdrLoad( void *hdr, char *src, long len, void *arg );
static xmsg_handle_t 	ethLoopPush( XObj s, Msg m );
static void	 	vciHdrStore( void *hdr, char *des, long len, void *arg );
static xmsg_handle_t 	ethPush( XObj	s, Msg 	msg );
static xkern_return_t   ethPop( XObj	s, XObj	llp, Msg m, void *h );


static XObjRomOpt ethOpt[] = {
    { "mtu", 3, readMtu }
};


static long
ethGetRelProtNum( hlp, llp, s )
    XObj	hlp, llp;
    char	*s;
{
    long	n;

    n = relProtNum(hlp, llp);
    if ( n == -1 ) {
	xTrace3(ethp, TR_ERRORS,
		"eth %s could not get prot num of %s relative to %s",
		s, hlp->name, llp->name);
    }
    if ( n < 0 || n > 0xffff ) {
	return -1;
    }
    return n;
}


static xkern_return_t
readMtu( self, str, nFields, line, arg )
    XObj	self;
    int		line, nFields;
    char	**str;
    void	*arg;
{
    int	mtu;

    mtu = atoi(str[2]);  
    if ( mtu <= 0 ) return XK_FAILURE;
    ((PState *)self->state)->mtu = mtu;
    return XK_SUCCESS;
}


static void
ethPsFree( PState	*ps )
{
    if ( ps->actMap ) mapClose(ps->actMap);
    if ( ps->pasMap ) mapClose(ps->pasMap);
    pathFree(ps);
}


xkern_return_t
eth_init( self )
     XObj self;
{
    PState	*ps;
    XObj	llp;
    Path	path = self->path;

    xTrace0(ethp, TR_EVENTS, "eth_init");
#ifdef XK_VCI
    if ( ! vciEthNetType ) {
	vciEthNetType = htons(VCI_ETH_TYPE);
    }
#endif
    if ( ! xIsProtocol(llp = xGetDown(self, 0)) ) {
	xError("eth can not get driver protocol object");
	return XK_FAILURE;
    }
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_FAILURE;
    }
    self->state = ps;
    ps->actMap = mapCreate(ETH_ACTIVE_MAP_SZ, sizeof(ActiveId), path);
    ps->pasMap = mapCreate(ETH_PASSIVE_MAP_SZ, sizeof(PassiveId), path);
    if ( ! ps->actMap || ! ps->pasMap ) {
	ethPsFree(ps);
	return XK_FAILURE;
    }
    ps->prmSessn = 0;
    ps->mtu = MAX_ETH_DATA_SZ;
    if ( xControl(llp, GETMAXPACKET, (char *)&ps->mtu, sizeof(ps->mtu))
	< sizeof(ps->mtu) )
    {
	xError("eth_init: can't get mtu of driver, using default");
    } /* if */
    findXObjRomOpts(self, ethOpt, sizeof(ethOpt)/sizeof(XObjRomOpt), 0);
    xTrace1(ethp, TR_MAJOR_EVENTS, "eth using mtu %d", ps->mtu);
    if ( xControl(llp, GETMYHOST, (char *)&ps->myHost, sizeof(ETHhost))
			< (int)sizeof(ETHhost) ) {
	xError("eth_init: can't get my own host");
	ethPsFree(ps);
	return XK_FAILURE;
    }
    self->control = ethControlProtl;
    self->open = ethOpen;
    self->openenable = ethOpenEnable;
    self->opendisable = ethOpenDisable;
    self->demux = ethDemux;
    self->opendisableall = ethOpenDisableAll;
    if ( xOpenEnable(self, self, llp, 0) == XK_FAILURE ) {
	xError("eth can not openenable driver protocol");
	ethPsFree(ps);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static XObj
ethOpen( 
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	part,
    Path	path )
{
    PState	*ps = (PState *)self->state;
    ActiveId  	key;
    XObj 	ethSessn;
    ETHhost	*remoteHost;
    long	protNum;
    
    if ( part == 0 || partLen(part) < 1 ) {
	xTrace0(ethp, TR_SOFT_ERRORS, "ethOpen -- bad participants");
	return ERR_XOBJ;
    }
    remoteHost = (ETHhost *)partPop(*part);
    xAssert(remoteHost);
    key.remote = *remoteHost;
    key.local = ps->myHost;
    if ( (protNum = ethGetRelProtNum(hlpType, self, "open")) == -1 ) {
	return ERR_XOBJ;
    }
    key.type = protNum;
    xTrace2(ethp, TR_MAJOR_EVENTS, "eth_open: destination address = %s:%4x",
	    ethHostStr(&key.remote), key.type);
    key.type = htons(key.type);
    if ( mapResolve(ps->actMap, &key, &ethSessn) == XK_FAILURE ) {
	ethSessn = ethCreateSessn(self, hlpRcv, hlpType, &key, path);
    }
    xTrace1(ethp, TR_MAJOR_EVENTS, "eth_open: returning %X", ethSessn);
    return (ethSessn);

}


static XObj
ethCreateSessn( self, hlpRcv, hlpType, key, path )
    XObj	self, hlpRcv, hlpType;
    ActiveId	*key;
    Path	path;
{
    XObj	s;
    XObj	llp = xGetDown(self, 0);
    SState	*ss;
    PState	*ps = (PState *)self->state;

    if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return ERR_XOBJ;
    }
    s = xCreateSessn(ethSessnInit, hlpRcv, hlpType, self, 1, &llp, path);
    if ( s == ERR_XOBJ ) {
	pathFree(ss);
	return ERR_XOBJ;
    }
    s->state = ss;
    xTraceP2(self, TR_MORE_EVENTS,
	     "Creating session with key \n\t<host == %s, type == 0x%x>",
	     ethHostStr(&key->remote), ntohs(key->type));
#ifdef notdef 
    /* XXX to enable kernel loopback */
    if ( ETH_ADS_EQUAL(key->host, ps->myHost) ) {
	xTrace0(ethp, TR_MAJOR_EVENTS,
		"ethCreateSessn -- creating loopback session");
	s->push = ethLoopPush;
    }
#endif
    if ( mapBind(ps->actMap, key, s, &s->binding) != XK_SUCCESS ) {
	xTraceP0(self, TR_ERRORS, "error binding in createSessn");
	xDestroy(s);
	return ERR_XOBJ;
    }
    ss->hdr.dst = key->remote;
    ss->hdr.type = key->type;
    ss->hdr.src = ps->myHost;
    ss->rcvHost = key->local;
    ss->hlp_num = ntohs(key->type);
    return s;
}


static xkern_return_t
ethOpenEnable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	part )
{
    PState	*ps = (PState *)self->state;
    PassiveId	key;
    long	protNum;
    
    if ( (protNum = ethGetRelProtNum(hlpType, self, "openEnable")) == -1 ) {
	return XK_FAILURE;
    }
    xTrace2(ethp, TR_GROSS_EVENTS, "eth_openenable: hlp=%x, protlNum=%x",
	    hlpRcv, protNum);
    key = protNum;
    key = htons(key);
    return defaultOpenEnable(ps->pasMap, hlpRcv, hlpType, (void *)&key);
} 


static xkern_return_t
ethOpenDisable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	part )
{
    PState	*ps = (PState *)self->state;
    long	protNum;
    PassiveId	key;
    
    if ( (protNum = ethGetRelProtNum(hlpType, self, "opendisable")) == -1 ) {
	return XK_FAILURE;
    }
    xTrace2(ethp, TR_GROSS_EVENTS, "eth_openenable: hlp=%x, protlNum=%x",
	    hlpRcv, protNum);
    key = protNum;
    key = htons(key);
    return defaultOpenDisable(ps->pasMap, hlpRcv, hlpType, (void *)&key);
}


static int
dispActiveMap( key, val, arg )
    void	*key, *val, *arg;
{
    XObj	s = (XObj)val;
    xPrintXObj(s);
    return MFE_CONTINUE;
}


static int
dispPassiveMap( key, val, arg )
    void	*key, *val, *arg;
{
#if XK_DEBUG
    Enable	*e = (Enable *)val;
#endif /* XK_DEBUG */   
    xTrace2(ethp, TR_ALWAYS, "Enable object, hlpRcv == %s, hlpType = %s",
	    e->hlpRcv->fullName, e->hlpType->fullName);
    return MFE_CONTINUE;
}


static xkern_return_t
ethOpenDisableAll( 
    XObj	self,
    XObj	hlpRcv )
{
    xkern_return_t	xkr;
    PState		*ps = (PState *)self->state;

    xTrace0(ethp, TR_MAJOR_EVENTS, "eth openDisableAll called");

    xTrace0(ethp, TR_ALWAYS, "before passive map contents:");
    mapForEach(ps->pasMap, dispPassiveMap, 0);
    xkr = defaultOpenDisableAll(((PState *)self->state)->pasMap, hlpRcv, 0);
    xTrace0(ethp, TR_ALWAYS, "after passive map contents:");
    mapForEach(ps->pasMap, dispPassiveMap, 0);
    xTrace0(ethp, TR_ALWAYS, "active map contents:");
    mapForEach(ps->actMap, dispActiveMap, 0);
    return XK_SUCCESS;
}


#ifdef XK_VCI

static long
vciHdrLoad( void *hdr, char *src, long len, void *arg )
{
    VCIhdr	vciHdr;
    
    bcopy(src, (char *)&vciHdr, VCI_HDR_LEN);
    ((ETHhdr *)arg)->type = vciHdr.ethType;	/* leave in net byte order */
    return VCI_HDR_LEN;
}

#endif


static xkern_return_t
ethDemux(
    XObj	self,
    XObj	llp,
    Msg		msg )
{
    PState	*ps = (PState *)self->state;
    ActiveId 	actKey;
    PassiveId 	pasKey;
    XObj 	s = 0;
    Enable 	*e;
    ETHhdr	*hdr = msgGetAttr(msg, 0);
    unsigned short htype;
    
    xTrace0(ethp, TR_EVENTS, "eth_demux");
    xTrace1(ethp, TR_FUNCTIONAL_TRACE, "eth type: %x", hdr->type);
    xTrace2(ethp, TR_FUNCTIONAL_TRACE, "src: %s  dst: %s",
	    ethHostStr(&hdr->src), ethHostStr(&hdr->dst));
    xIfTrace(ethp, TR_DETAILED) msgShow(msg);
    xAssert(hdr);
    if ( ps->prmSessn ) {
	Msg_s	pMsg;
	
	xTrace0(ethp, TR_EVENTS,
		"eth_demux: passing msg to promiscuous session");
	msgConstructCopy(&pMsg, msg);
	xDemux(ps->prmSessn, &pMsg);
	msgDestroy(&pMsg);
    }
#if XK_DEBUG
    /*
     * verify that msg is for this host
     */
    if ( ! (ETH_ADS_EQUAL(hdr->dst, ps->myHost) ||
	    ETH_ADS_EQUAL(hdr->dst, ethBcastHost) ||
	    ETH_ADS_MCAST(hdr->dst))) {
	xError("eth_demux: msg is not for this host");
	return XK_FAILURE;
    }
#endif
    htype = ntohs(hdr->type);
#ifdef XK_VCI
    if ( hdr->type == vciEthNetType ) {
	if ( msgPop(msg, vciHdrLoad, 0, VCI_HDR_LEN, hdr) == XK_FAILURE ) {
	    msgDestroy(msg);
	    return XK_FAILURE;
	}
        htype = ntohs(hdr->type);
	xTraceP1(self, TR_EVENTS, "demux: real eth type: 0x%x", htype);
    }
#endif  /* XK_VCI */  
    bzero((char *)&actKey, sizeof(actKey));
    actKey.remote = hdr->src;
    actKey.local = hdr->dst;
    if (htype <= MAX_IEEE802_3_DATA_SZ) {
	/* it's an IEEE 802.3 packet---deliver it to protocol 0 */
	actKey.type = 0;
    } else {
	actKey.type = hdr->type;
    } /* if */
    if ( mapResolve(ps->actMap, &actKey, &s) == XK_FAILURE ) {
	pasKey = actKey.type;
	if ( mapResolve(ps->pasMap, &pasKey, &e) == XK_SUCCESS ) {
	    xTrace1(ethp, TR_EVENTS,
		    "eth_demux: openenable exists for msg type %x",
		    ntohs(pasKey));
	    xAssert( pasKey == 0 ||
		     ntohs(hdr->type) == relProtNum(e->hlpType, self) );
	    s = ethCreateSessn(self, e->hlpRcv, e->hlpType, &actKey,
			       msgGetPath(msg));
	    if ( s != ERR_XOBJ ) {
		xOpenDone(e->hlpRcv, s, self, msgGetPath(msg));
		xTrace0(ethp, TR_EVENTS,
			"eth_demux: sending message to new session");
	    }
	} else {
	    xTrace1(ethp, TR_EVENTS,
		    "eth_demux: openenable does not exist for msg type %x",
		    ntohs(pasKey));
	}
    }
    if ( xIsSession(s) ) {
	xPop(s, llp, msg, 0);
    }
    msgDestroy(msg);
    return XK_SUCCESS;
}



static xkern_return_t
ethClose( s )
    XObj	s;
{
    PState	*ps = (PState *)xMyProtl(s)->state;

    xTrace1(ethp, TR_MAJOR_EVENTS, "eth closing session %x", s);
    xAssert( xIsSession( s ) );
    xAssert( s->rcnt <= 0 );
    mapRemoveBinding( ps->actMap, s->binding );
    xDestroy( s );
    return XK_SUCCESS;
}


static void
demuxStub(ev, arg)
    Event	ev;
    void 	*arg;
{
    RetBlock	*b = (RetBlock *)arg;

    ethDemux(b->self, b->llp, &b->msg);
    pathFree(arg);
}


static xmsg_handle_t
ethLoopPush(
    XObj	s,
    Msg		m )
{
    RetBlock	*b;
    Event	ev;
    Path	path = s->path;

    if ( ! (b = pathAlloc(path, sizeof(RetBlock))) ) {
	xTraceP0(s, TR_SOFT_ERRORS, "loop push -- allocation error");
	return XMSG_ERR_HANDLE;
    }
    if ( ! (ev = evAlloc(path))) {
	pathFree(b);
	xTraceP0(s, TR_SOFT_ERRORS, "loop push -- allocation error");
	return XMSG_ERR_HANDLE;
    }
    msgConstructCopy(&b->msg, m);
    b->hdr = ((SState *)s->state)->hdr;
    if (((SState*)s->state)->hlp_num <= MAX_IEEE802_3_DATA_SZ) {
	/* it's an 802.3 type packet: set type field to size of packet */
	b->hdr.type = htons(msgLen(m));
    } /* if */
    msgSetAttr(&b->msg, 0, (void *)&b->hdr, sizeof(b->hdr));
    b->self = s->myprotl;
    b->llp = xGetDown(s->myprotl, 0);
    evDetach( evSchedule(ev, demuxStub, b, 0) );
    return XMSG_NULL_HANDLE;
}


#ifdef XK_VCI

static void
vciHdrStore( void *hdr, char *des, long len, void *arg )
{
    ((VCIhdr *)hdr)->vci = htons(((VCIhdr *)hdr)->vci);
    bcopy(hdr, des, len);
}

#endif


static xmsg_handle_t
ethPush(
    XObj	s,
    Msg 	msg )
{
    ETHhdr hdr;

    xTraceP1(s, TR_EVENTS, "push (%lx)", (u_long)s);
    hdr = ((SState *)s->state)->hdr;
    if (((SState*)s->state)->hlp_num <= MAX_IEEE802_3_DATA_SZ) {
	/* it's an 802.3 type packet: set type field to size of packet */
	hdr.type = htons(msgLen(msg));
    } /* if */
#ifdef XK_VCI
    {
	VCIhdr  vciHdr;
	VciType	vci;

	if ( (vci = pathGetId(msgGetPath(msg))) != 0 ) {
	    vciHdr.ethType = hdr.type;
	    vciHdr.vci = vci;
	    hdr.type = vciEthNetType;
	    if ( msgPush(msg, vciHdrStore, &vciHdr, VCI_HDR_LEN, 0) == XK_FAILURE ) {
		return XMSG_ERR_HANDLE;
	    }
	}
	xTraceP1(s, TR_MORE_EVENTS, "outgoing msg uses VCI %d", vci);
	xIfTrace(vci, TR_EVENTS) {
	    printf("out: %d ", vci);
	    if ( vci == 0 ) {
		printf("(%x) ", (u_short)htons(vciHdr.ethType));
	    }
	}
    }
#endif
    msgSetAttr(msg, 0, &hdr, sizeof(ETHhdr));
    if ( xPush(xGetDown(s, 0), msg) == XMSG_ERR_HANDLE ) {
	xTraceP0(s, TR_SOFT_ERRORS, "write to driver failed");
    }
    return XMSG_NULL_HANDLE;
}


static xkern_return_t
ethPop(
    XObj	s,
    XObj	llp,
    Msg 	m,
    void	*h )
{
    return xDemux(s, m);
} 


static int
ethControlSessn(s, op, buf, len)
    XObj	s;
    int 	op, len;
    char 	*buf;
{
    SState	*ss = (SState *)s->state;
    
    xAssert(xIsSession(s));
    switch (op) {

      case GETMYHOST:
      case GETMAXPACKET:
      case GETOPTPACKET:
	return ethControlProtl(xMyProtl(s), op, buf, len);
	
      case GETPEERHOST:
	checkLen(len, sizeof(ETHhost));
	bcopy((char *) &ss->hdr.dst, buf, sizeof(ETHhost));
	return (sizeof(ETHhost));
	
      case GETRCVHOST:
	checkLen(len, sizeof(ETHhost));
	bcopy((char *) &ss->rcvHost, buf, sizeof(ETHhost));
	return (sizeof(ETHhost));

      case GETMYHOSTCOUNT:
      case GETPEERHOSTCOUNT:
	checkLen(len, sizeof(int));
	*(int *)buf = 1;
	return sizeof(int);
	
      case GETMYPROTO:
      case GETPEERPROTO:
	checkLen(len, sizeof(long));
	*(long *) buf = ss->hdr.type;
	return sizeof(long);
	
      case GETPARTICIPANTS:
	{
	    Part_s	p[2];

	    partInit(p, 2);
	    /* 
	     * Remote host
	     */
	    partPush(p[0], &ss->hdr.dst, sizeof(ETHhost));
	    partPush(p[1], &ss->hdr.src, sizeof(ETHhost));
	    return (partExternalize(p, buf, &len) == XK_FAILURE) ? -1 : len;
	}

      case ETH_SETPROMISCUOUS:
	{
	    PState	*ps = (PState *)xMyProtl(s)->state;
	    checkLen(len, sizeof(int));
	    ps->prmSessn = s;
	    /* 
	     * Tell the device driver to go into promiscuous mode
	     */
	    return xControl(xGetDown(s, 0), op, buf, len);
	}

      default:
	return -1;
    }
}


static int
ethControlProtl( self, op, buf, len )
    XObj	self;
    int 	op, len;
    char 	*buf;
{
    PState	*ps = (PState *)self->state;

    xAssert(xIsProtocol(self));
  
    switch (op) {

      case GETMAXPACKET:
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	*(int *) buf = ps->mtu;
#ifdef XK_VCI
	*(int *) buf -= VCI_HDR_LEN;
#endif
	return (sizeof(int));
	
      case GETMYHOST:
	checkLen(len, sizeof(ETHhost));
	bcopy((char *) &ps->myHost, buf, sizeof(ETHhost));
	return (sizeof(ETHhost));

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
  }
  
}


static xkern_return_t
ethSessnInit(s)
    XObj s;
{
  s->push = ethPush;
  s->pop = ethPop;
  s->close = ethClose;
  s->control = ethControlSessn;
  return XK_SUCCESS;
}
