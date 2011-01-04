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
 * udp.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * udp.c,v
 * Revision 1.85.1.4.1.2  1994/09/07  04:18:39  menze
 * OSF modifications
 *
 * Revision 1.85.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.85.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.85.1.2  1994/07/22  20:08:21  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.85.1.1  1994/04/12  20:56:20  hkaram
 * Uses new message interface
 *
 * Revision 1.85  1994/01/08  21:26:18  menze
 *   [ 1994/01/05          menze ]
 *   PROTNUM changed to PORTNUM
 *
 * Revision 1.84  1993/12/13  23:32:10  menze
 * Modifications from UMass:
 *
 *   [ 93/11/04          nahum ]
 *   Some UDP cleanup to appease GCC -wall
 *
 * Revision 1.83  1993/12/11  00:25:16  menze
 * fixed #endif comments
 *
 * Revision 1.82  1993/12/07  17:38:02  menze
 * Improved validity checks on incoming participants
 *
 * Revision 1.81  1993/12/07  00:59:00  menze
 * Now uses IP_GETPSEUDOHDR to determine protocol number relative to IP.
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/udp.h>
#include <xkern/protocols/udp/udp_internal.h>
#include <xkern/include/gc.h>

#if XK_DEBUG
int	traceudpp;
#endif /* XK_DEBUG */

/* #define UDP_USE_GC */


/* 
 * Check port range
 */
#define portCheck(_port, name, retval) 					\
	{								\
    	  if ( (_port) < 0 || (_port) > MAX_PORT ) {			\
		xTrace2(udpp, TR_SOFT_ERRORS,				\
		   "%s port %d out of range", (name), (_port));  	\
		return (retval);					\
	  }								\
        }								


static void		callUnlockPort( void *, Enable * );
static void		dispPseudoHdr( IPpseudoHdr * );
static XObjInitFunc	getproc_sessn;
static void		getproc_protl(XObj);
static xkern_return_t	udpCloseProtl( XObj );
static xkern_return_t	udpCloseSessn( XObj );
static int		udpControlProtl( XObj, int, char *, int ) ;
static int		udpControlSessn( XObj, int, char *, int ) ;
static XObj		udpCreateSessn( XObj, XObj, XObj, ActiveId *, Path );
static long		udpHdrLoad( void *, char *, long, void * );
static void		udpHdrStore( void *, char *, long, void * );

#ifdef UDP_USE_GC
static void		udpDestroySessn( XObj );
#endif

#define		SESSN_COLLECT_INTERVAL	30 * 1000 * 1000    /* 30 seconds */
#define		ACTIVE_MAP_SIZE 101
#define		PASSIVE_MAP_SIZE 23

static void
dispPseudoHdr(h)
    IPpseudoHdr *h;
{
    xTrace2(udpp, TR_ALWAYS, "   IP pseudo header: src: %s  dst: %s",
	    ipHostStr(&h->src), ipHostStr(&h->dst));
    xTrace3(udpp, TR_ALWAYS, "      z:  %d  p: %d len: %d",
	    h->zero, h->prot, ntohs(h->len));
}


/*
 * udpHdrStore -- write header to potentially unaligned msg buffer.
 * Note:  *hdr will be modified
 */
static void
udpHdrStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    SSTATE *sstate;
    
    xAssert(len == sizeof(HDR));
    xAssert(arg);
    
    ((HDR *)hdr)->ulen = htons(((HDR *)hdr)->ulen);
    ((HDR *)hdr)->sport = htons(((HDR *)hdr)->sport);
    ((HDR *)hdr)->dport = htons(((HDR *)hdr)->dport);
    ((HDR *)hdr)->sum = 0;
    bcopy((char *)hdr, dst, sizeof(HDR));
    sstate = (SSTATE *)((storeInfo *)arg)->s->state;
    if (sstate->useCkSum) {
	u_short sum = 0;
	
	xTrace0(udpp, TR_FUNCTIONAL_TRACE, "Using UDP checksum");
	sstate->pHdr.len = ((HDR *)hdr)->ulen;	/* already in net byte order */
	xIfTrace(udpp, TR_FUNCTIONAL_TRACE) {
	    dispPseudoHdr(&sstate->pHdr);
	}
	sum = inCkSum(((storeInfo *)arg)->m, (u_short *)&sstate->pHdr,
		       sizeof(IPpseudoHdr));
	bcopy((char *)&sum, (char *)&((HDR *)dst)->sum, sizeof(u_short));
	xAssert(! inCkSum(((storeInfo *)arg)->m, (u_short *)&sstate->pHdr,
			  sizeof(IPpseudoHdr)));
    }
}


/*
 * udpHdrLoad -- load header from potentially unaligned msg buffer.
 * Result of checksum calculation will be in hdr->sum.
 */
static long
udpHdrLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
  xAssert(len == sizeof(HDR));
  /*
   * 0 in the checksum field indicates checksum disabled
   */
  bcopy(src, (char *)hdr, sizeof(HDR));
  if (((HDR *)hdr)->sum) {
    IPpseudoHdr *pHdr = (IPpseudoHdr *)msgGetAttr((Msg)arg, 0);

    xTrace0(udpp, TR_FUNCTIONAL_TRACE, "UDP header checksum was used");
    xAssert(pHdr);
    xIfTrace(udpp, TR_FUNCTIONAL_TRACE) {
      dispPseudoHdr(pHdr);
    }
    ((HDR *)hdr)->sum = inCkSum((Msg)arg, (u_short *)pHdr, sizeof(*pHdr));
  } else {
    xTrace0(udpp, TR_FUNCTIONAL_TRACE, "No UDP header checksum was used");
  }
  ((HDR *)hdr)->ulen = ntohs(((HDR *)hdr)->ulen);
  ((HDR *)hdr)->sport = ntohs(((HDR *)hdr)->sport);
  ((HDR *)hdr)->dport = ntohs(((HDR *)hdr)->dport);
  return sizeof(HDR);
}



static void 
psFree( PSTATE *ps )
{
    if ( ps->activemap ) mapClose(ps->activemap);
    if ( ps->passivemap ) mapClose(ps->passivemap);
    if ( ps->portstate ) udpPortMapClose(ps->portstate);
    pathFree(ps);
}


/*
 * udp_init
 */
xkern_return_t
udp_init(self)
    XObj self;
{
    Part_s	part;
    PSTATE	*ps;
    XObj	llp;
    Path	path = self->path;
    
    xTrace0(udpp, TR_GROSS_EVENTS, "UDP init");
    xAssert(xIsProtocol(self));
    
    if ( ! xIsProtocol(llp = xGetDown(self, 0)) ) {
	xTraceP0(self, TR_ERRORS, "could not get lower protocol");
	return XK_FAILURE;
    }
    getproc_protl(self);
    if ( ! (ps = pathAlloc(path, sizeof(PSTATE))) ) {
	xTraceP0(self, TR_ERRORS, "allocation error");
	return XK_NO_MEMORY;
    }
    bzero((char *)ps, sizeof(PSTATE));
    self->state = ps;
    if ( udpPortMapInit(&ps->portstate, path) != XK_SUCCESS ) {
	psFree(ps);
	return XK_FAILURE;
    }
    ps->activemap = mapCreate(ACTIVE_MAP_SIZE, sizeof(ActiveId), path);
    ps->passivemap = mapCreate(PASSIVE_MAP_SIZE, sizeof(PassiveId), path);
    if ( ! ps->activemap || ! ps->passivemap ) {
	psFree(ps);
	return XK_NO_MEMORY;
    }
#ifdef UDP_USE_GC
    if ( ! initSessionCollector(pstate->activemap, SESSN_COLLECT_INTERVAL,
				udpDestroySessn, "udp") ) {
	psFree(ps);
	return XK_NO_MEMORY;
    }
#endif
    /* Notify any protocols between UDP & IP to turn on length-fixup for the
       IP pseudoheader. */

    xControl(llp,IP_PSEUDOHDR,(char *)0,0);
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if ( xOpenEnable(self, self, llp, &part) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS,
		 "udp_init: can't openenable transport protocol");
    }
    xTrace0(udpp, TR_GROSS_EVENTS, "UDP init done");
    return XK_SUCCESS;
}


/* 
 * getPorts -- extract ports from the participant, checking for validity.
 * If lPort is 0, no attempt to read a local port is made.
 * Returns 0 if the port extraction succeeded, -1 if there were problems. 
 */
static int
getPorts(
    PSTATE	*pstate,
    Part_s	*p,
    UDPport	*rPort,
    UDPport	*lPort,
    char	*str)
{
    long	*port;
    long	freePort;

    xAssert(rPort);
    if ( ! p || partLen(p) < 1
	     || partStackTopByteLen(p[0]) < sizeof(long)
	     || partLen(p) > 1 && (partStackTopByteLen(p[1]) < sizeof(long))
       ) {
	xTrace1(udpp, TR_SOFT_ERRORS, "%s -- bad participants", str);
	return -1;
    }
    if ( (port = (long *)partPop(p[0])) == 0 ) {
	xTrace1(udpp, TR_SOFT_ERRORS, "%s -- no remote port", str);
	return -1;
    }
    if (*port != ANY_PORT) {
	portCheck(*port, str, -1);
    } /* if */
    *rPort = *port;
    if ( lPort ) {
 	if ( (partLen(p) < 2)  ||
 	    (((port = (long *)partPop(p[1])) != 0) && (*port == ANY_PORT)) ) {
	    /* 
	     * No port specified -- find a free one
	     */
	    if ( udpGetFreePort(pstate->portstate, &freePort) ) {
		sprintf(errBuf, "%s -- no free ports!", str);
		xError(errBuf);
		return -1;
	    }
	    *lPort = freePort;
	} else {
	    /* 
	     * A specific local port was requested
	     */
	    if ( port == 0 ) {
		xTrace1(udpp, TR_SOFT_ERRORS,
			"%s -- local participant, but no local port", str);
		return -1;
	    }
	    portCheck(*port, str, -1);
	    *lPort = *port;
	    udpDuplicatePort(pstate->portstate, *lPort);
	}
    }
    return 0;
}


/*
 * udpOpen
 */
static XObj
udpOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p,
    Path	path)
{
    XObj	udp_s;
    XObj	lls;
    ActiveId 	key;
    PSTATE	*pstate;
    
    xTrace0(udpp, TR_MAJOR_EVENTS, "UDP open");
    pstate = (PSTATE *)self->state;
    if ( getPorts(pstate, p, &key.remoteport, &key.localport, "udpOpen") ) {
	return ERR_XOBJ;
    }
    xTrace2(udpp, TR_MAJOR_EVENTS, "UDP open: from port %d to port %d",
	    key.localport, key.remoteport);
    udp_s = ERR_XOBJ;	
    lls = xOpen(self, self, xGetDown(self, 0), p, path);
    if ( lls != ERR_XOBJ ) {
	key.lls = lls;
	if ( mapResolve(pstate->activemap, &key, &udp_s) == XK_FAILURE ) {
	    xTrace0(udpp, TR_MAJOR_EVENTS, "udpOpen creates new session");
	    udp_s = udpCreateSessn(self, hlpRcv, hlpType, &key, path);
	    if ( udp_s != ERR_XOBJ ) {
		/* 
		 * A successful open!
		 */
		xTrace1(udpp, TR_MAJOR_EVENTS, "UDP open returns %x", udp_s);
		return udp_s;
	    } 
	} else {
	    /*
	     * We don't allow multiple opens of the same UDP session.  If
	     * the refcount is zero, the session is just being idle
	     * awaiting garbage collection
	     */
	    if ( udp_s->rcnt > 0 ) {
		xTrace0(udpp, TR_MAJOR_EVENTS, "udpOpen ERROR -- found existing session!");
		udp_s = ERR_XOBJ;
	    } else {
		udp_s->idle = FALSE;
	    }
	}	
	xClose(lls);
    }
    return udp_s;
}


static XObj
udpCreateSessn(self, hlpRcv, hlpType, key, path)
    XObj self, hlpRcv, hlpType;
    ActiveId *key;
    Path	path;
{
    XObj	s;
    SSTATE	*sstate;
    PSTATE	*pstate;
    HDR		*udph;

    pstate = (PSTATE *)self->state;
    s = xCreateSessn(getproc_sessn, hlpRcv, hlpType, self, 1, &key->lls, path);
    if ( s == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    if ( mapBind(pstate->activemap, key, s, &s->binding) != XK_SUCCESS ) {
	xTraceP0(self, TR_SOFT_ERRORS, "bind error");
	xDestroy(s);
	return ERR_XOBJ;
    }
    if ( ! (sstate = pathAlloc(path, sizeof(SSTATE))) ) {
	xTraceP0(self, TR_SOFT_ERRORS, "session allocation error");
	xDestroy(s);
	return ERR_XOBJ;
    }
    if ( xControl(key->lls, IP_GETPSEUDOHDR, (char *)&sstate->pHdr,
		  sizeof(IPpseudoHdr)) == -1 ) {
	xTrace0(udpp, TR_MAJOR_EVENTS,
		"UDP create sessn could not get pseudo-hdr from lls");
    }
    sstate->useCkSum = USE_CHECKSUM_DEF;
    s->state = (char *)sstate;
    udph = &(sstate->hdr);
    udph->sport = key->localport;
    udph->dport = key->remoteport;
    udph->ulen = 0;
    udph->sum = 0;
    /* Notify any protocols between UDP & IP to turn on length-fixup for the
       IP pseudoheader. */
    /* xControl(xGetDown(s,0),IP_PSEUDOHDR,(char *)0,0); redundant for now,
       since we're doing this at the protocol level instead of session level
       for UDP.  Might change back to sessions if UDP were to use opendone. */

    return s;
}


/*
 * udpControlSessn
 */
static int
udpControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
  SSTATE        *sstate;
  PSTATE        *pstate;
  HDR           *hdr;
  
  xAssert(xIsSession(s));
  
  sstate = (SSTATE *) s->state;
  pstate = (PSTATE *) s->myprotl->state;
  
  hdr = &(sstate->hdr);
  switch (opcode) {
    
  case UDP_DISABLE_CHECKSUM:
    sstate->useCkSum = 0;
    return 0;

  case UDP_ENABLE_CHECKSUM:
    sstate->useCkSum = 1;
    return 0;

  case GETMYPROTO:
    checkLen(len, sizeof(long));
    *(long *)buf = sstate->hdr.sport;
    return sizeof(long);

  case GETPEERPROTO:
    checkLen(len, sizeof(long));
    *(long *)buf = sstate->hdr.dport;
    return sizeof(long);

   case GETPARTICIPANTS:
     {
	 Part_s		p[2];
	 int 		retLen;
	 long		localPort, remotePort;
	 int		numParts;
	 xkern_return_t	xkr;
	 
	 retLen = xControl(xGetDown(s, 0), opcode, buf, len);
	 if ( retLen < 0 ) {
	     return retLen;
	 }
	 numParts = partExtLen(buf);
	 if ( numParts > 0 && numParts <= 2 ) {
	     partInternalize(p, buf);
	     if ( numParts == 2 ) {
		 localPort = sstate->hdr.sport;
		 partPush(p[1], &localPort, sizeof(long));
	     }
	     remotePort = sstate->hdr.dport;
	     partPush(p[0], &remotePort, sizeof(long));
	     xkr = partExternalize(p, buf, &len);
	     if ( xkr == XK_FAILURE ) {
		 xTrace0(udpp, TR_SOFT_ERRORS,
			 "UDP GETPARTS -- buffer too small");
		 retLen = -1;
	     } else {
		 retLen = len;
	     }
	 } else {
	     xTrace1(udpp, TR_SOFT_ERRORS,
		 "UDP -- Bad number of participants (%d) returned in GETPARTS",
		     partExtLen(buf));
	     retLen = -1;
	 }
	 return retLen;
     }

  case GETMAXPACKET:
  case GETOPTPACKET:
    {
	checkLen(len, sizeof(int));
	if ( xControl(xGetDown(s, 0), opcode, buf, len) < sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(HDR);
	return sizeof(int);
    }
    
  default:
    return xControl(xGetDown(s, 0), opcode, buf, len);;
  }
}


/*
 * udpControlProtl
 */
static int
udpControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    long port;

    xAssert(xIsProtocol(self));
    
    switch (opcode) {
	
      case GETMAXPACKET:
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	if ( xControl(xGetDown(self, 0), opcode, buf, len) < sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(HDR);
	return sizeof(int);
	
      case UDP_GETFREEPORTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	if (udpGetFreePort(((PSTATE *)(self->state))->portstate, &port)) {
	    return -1;
	} /* if */
	*(long *)buf = port;
	return 0;

      case UDP_RELEASEPORTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	udpReleasePort(((PSTATE *)(self->state))->portstate, port);
	return 0;

      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
	
    }
}


/*
; * udpOpenEnable
 */
static xkern_return_t
udpOpenEnable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p)
{
    PSTATE 	*pstate;
    Enable 	*e;
    PassiveId	key;
    
    xTrace0(udpp, TR_MAJOR_EVENTS, "UDP open enable");
    pstate = (PSTATE *)self->state;
    if ( getPorts(pstate, p, &key, 0, "udpOpenEnable") ) {
	return XK_FAILURE;
    }
    xTrace1(udpp, TR_MAJOR_EVENTS, "Port number %d", key);
    if ( mapResolve(pstate->passivemap, &key, &e) != XK_FAILURE ) {
	if ( e->hlpRcv == hlpRcv ) {
	    e->rcnt++;
	    return XK_SUCCESS;
	}
	return XK_FAILURE;
    }
    if ( udpDuplicatePort(pstate->portstate, key) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    if ( ! (e = (Enable *)pathAlloc(self->path, sizeof(Enable))) ) {
	xTraceP0(self, TR_SOFT_ERRORS, "openEnable allocation failure");
	udpReleasePort(pstate->portstate, key);
	return XK_FAILURE;
    }
    e->hlpRcv = hlpRcv;
    e->hlpType = hlpType;
    e->rcnt = 1;
    if ( mapBind(pstate->passivemap, &key, e, &e->binding) != XK_SUCCESS ) {
	xTraceP0(self, TR_SOFT_ERRORS, "bind error");
	pathFree((char *)e);
	udpReleasePort(pstate->portstate, key);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


#ifdef UDP_USE_GC

/*
 * udpCloseSessn
 */
static xkern_return_t
udpCloseSessn(s)
    XObj s;
{
    SSTATE	*sstate;

    sstate = (SSTATE *)s->state;

    xAssert(xIsSession(s));
    xTrace1(udpp, TR_MAJOR_EVENTS, "UDP close of session %x", s);
    xAssert(s->rcnt == 0);
    udpReleasePort(((PSTATE *)(xMyProtl(s)->state))->portstate,
		   sstate->hdr.sport);
    return XK_SUCCESS;
}


/* 
 * This function is udpDestroySessn if we are using garbage
 * collection, udpCloseSessn if we are not.
 */
static void
udpDestroySessn( s )

#else

static xkern_return_t
udpCloseSessn( s )

#endif    
    XObj	s;
{
    PSTATE	*pstate;
    SSTATE	*sstate;
    
    xAssert(xIsSession(s));
    xTrace1(udpp, TR_MAJOR_EVENTS, "UDP destroy session %x", s);
    xAssert(s->rcnt == 0);
    pstate = (PSTATE *)s->myprotl->state;
    sstate = (SSTATE *)s->state;
    mapRemoveBinding(pstate->activemap, s->binding);
    xClose(xGetDown(s, 0));
#ifndef UDP_USE_GC
    udpReleasePort(pstate->portstate, sstate->hdr.sport);
#endif
    xDestroy(s);
#ifndef UDP_USE_GC
    return XK_SUCCESS;
#endif
}


/*
 * udpCloseProtl
 */
static xkern_return_t
udpCloseProtl(self)
    XObj self;
{
  PSTATE        *pstate;
  
  xAssert(xIsProtocol(self));
  
  if (((self->rcnt)) <= 0) {
      pstate = (PSTATE *) self->state;
      mapClose(pstate->activemap);
      mapClose(pstate->passivemap);
      udpPortMapClose(pstate->portstate);
  }
  return XK_SUCCESS;
}


/*
 * udpPush
 */
/*ARGSUSED*/
static xmsg_handle_t
udpPush(
    XObj s,
    Msg msg)
{
  SSTATE        *sstate;
  HDR           hdr;
  storeInfo	info;
  
  xTrace0(udpp, TR_EVENTS, "in udp push");
  xAssert(xIsSession(s));
  sstate = (SSTATE *) s->state;
  hdr = sstate->hdr;
  hdr.ulen = msgLen(msg) + HLEN;
  info.s = s;
  info.m = msg;
  xTrace2(udpp, TR_EVENTS, "sending msg len %d from port %d",
	 msgLen(msg), hdr.sport);
  xTrace5(udpp, TR_EVENTS, "  to port %d @ %d.%d.%d.%d", hdr.dport,
	 sstate->pHdr.dst.a, sstate->pHdr.dst.b, sstate->pHdr.dst.c,
	 sstate->pHdr.dst.d);
  if (msgPush(msg, udpHdrStore, &hdr, HLEN, &info) == XK_FAILURE) {
    xTrace0(udpp, TR_ERRORS, "udpPush could not push header");
    return XMSG_ERR_HANDLE;
  }
  return xPush(xGetDown(s, 0), msg);
}


/*
 * udpDemux
 */
static xkern_return_t
udpDemux(
    XObj	self,
    XObj	lls,
    Msg 	dg)
{
    HDR   	h;
    XObj   	s;
    ActiveId  	activeid;
    PassiveId	passiveid;
    PSTATE      *pstate;
    Enable	*e;
    
    pstate = (PSTATE *)self->state;
    xTrace0(udpp, TR_EVENTS, "UDP Demux");
    if (msgPop(dg, udpHdrLoad, &h, HLEN, dg) == XK_FAILURE) {
      xTrace0(udpp, TR_ERRORS, "udpDemux could not pop header");
      return XK_SUCCESS;
    }

    xTrace1(udpp, TR_FUNCTIONAL_TRACE, "Sending host: %s",
	    ipHostStr(&((IPpseudoHdr *)msgGetAttr(dg, 0))->src));
    xTrace1(udpp, TR_FUNCTIONAL_TRACE, "Destination host: %s",
	    ipHostStr(&((IPpseudoHdr *)msgGetAttr(dg, 0))->dst));
    xTrace2(udpp, TR_EVENTS, "sport = %d, dport = %d", h.sport, h.dport);

    if (h.sum) {
	xTrace1(udpp, TR_MAJOR_EVENTS, "udpDemux: bad hdr checksum (%x)! dropping msg",
		h.sum);
	return XK_SUCCESS;
    }
    if ((h.ulen - HLEN) < msgLen(dg)) {
	msgTruncate(dg, (int) h.ulen);
    }
    xTrace2(udpp, TR_FUNCTIONAL_TRACE, " h->ulen = %d, msg_len = %d", h.ulen,
	    msgLen(dg));
    activeid.localport = h.dport;
    activeid.remoteport = h.sport;
    activeid.lls = lls;
    if ( mapResolve(pstate->activemap, &activeid, &s) == XK_FAILURE ) {
	passiveid = h.dport;
	if ( mapResolve(pstate->passivemap, &passiveid, &e) == XK_FAILURE ) {
	    xTrace0(udpp, TR_MAJOR_EVENTS, "udpDemux dropping the message");
	    return XK_SUCCESS;
	}
	xTrace1(udpp, TR_MAJOR_EVENTS, "Found an open enable for prot %d",
		e->hlpRcv);
	s = udpCreateSessn(self, e->hlpRcv, e->hlpType, &activeid,
			   msgGetPath(dg));
	if (s == ERR_XOBJ) {
	    xTrace0(udpp, TR_ERRORS, "udpDemux could not create session");
	    return XK_SUCCESS;
	}
	xDuplicate(lls);
	udpDuplicatePort(pstate->portstate, activeid.localport);
#ifndef UDP_USE_GC
	xOpenDone(e->hlpRcv, s, self, msgGetPath(dg));
#endif /* ! UDP_USE_GC */
    } else {
	xTrace1(udpp, TR_EVENTS, "Popping to existing session %x", s);
    }
#ifdef UDP_USE_GC
    /* 
     * Since UDP sessions don't go away when the external ref count is
     * zero, we need to check for openEnables when rcnt == 0.
     */
    if ( s->rcnt == 0 ) {
	passiveid = h.dport;
	if ( mapResolve(pstate->passivemap, &passiveid, &e) == XK_FAILURE ) {
	    xTrace0(udpp, TR_MAJOR_EVENTS, "udpDemux dropping the message");
	    return XK_SUCCESS;
	}
	xOpenDone(e->hlpRcv, s, self);
    }
#endif  /* UDP_USE_GC */
    xAssert(xIsSession(s));
    return xPop(s, lls, dg, 0);
}


/*
 * udpPop
 */
/*ARGSUSED*/
static xkern_return_t
udpPop(
    XObj	s,
    XObj 	ds,
    Msg 	dg,
    void 	*arg)
{
  xTrace1(udpp, TR_EVENTS, "UDP pop, length = %d", msgLen(dg));
  xAssert(xIsSession(s));
  return xDemux(s, dg);
}


static xkern_return_t
udpOpenDisable(
    XObj 	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p)
{
    PassiveId	key;
    PSTATE   	*pstate;
    Enable	*e;
    
    xTrace0(udpp, TR_MAJOR_EVENTS, "UDP open disable");
    pstate = (PSTATE *)self->state;
    if ( getPorts(pstate, p, &key, 0, "udpOpenDisable") ) {
	return XK_FAILURE;
    }
    xTrace1(udpp, TR_MAJOR_EVENTS, "port %d", key);
    if ( mapResolve(pstate->passivemap, &key, &e) == XK_FAILURE ||
	e->hlpRcv == hlpRcv && e->hlpType == hlpType ) {
	if (--(e->rcnt) == 0) {
	    mapRemoveBinding(pstate->passivemap, e->binding);
	    pathFree((char *)e);
	    udpReleasePort(pstate->portstate, key);
	}
	return XK_SUCCESS;
    } else {
	return XK_FAILURE;
    }
}

static void *portstatekludge; /* no, it's not instantiated separately
				 for each instance, but because it
				 won't be used with blocking, we escape
				 disaster */

static void
callUnlockPort( key, e )
    void	*key;
    Enable	*e;
{
    xTrace1(udpp, TR_FUNCTIONAL_TRACE,
	    "UDP callUnlockPort called with key %d", (int)*(PassiveId *)key);
    udpReleasePort(portstatekludge, *(PassiveId *)key);
}


static xkern_return_t
udpOpenDisableAll(
    XObj	self,
    XObj	hlpRcv)
{
    xTrace0(udpp, TR_MAJOR_EVENTS, "udpOpenDisableAll");
    portstatekludge = (void *)(((PSTATE *)(self->state))->portstate);
    return defaultOpenDisableAll(((PSTATE *)self->state)->passivemap,
				 hlpRcv, callUnlockPort);
}


static void
getproc_protl(s)
    XObj s;
{
  xAssert(xIsProtocol(s));
  s->close = udpCloseProtl;
  s->control = udpControlProtl;
  s->open = udpOpen;
  s->openenable = udpOpenEnable;
  s->opendisable = udpOpenDisable;
  s->demux = udpDemux;
  s->opendisableall = udpOpenDisableAll;
}


static xkern_return_t
getproc_sessn(s)
    XObj s;
{
  xAssert(xIsSession(s));
  s->push = udpPush;
  s->pop = udpPop;
  s->control = udpControlSessn;
  s->close = udpCloseSessn;
  return XK_SUCCESS;
}
