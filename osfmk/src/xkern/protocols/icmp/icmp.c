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
 * icmp.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * icmp.c,v
 * Revision 1.43.1.3.1.2  1994/09/07  04:18:29  menze
 * OSF modifications
 *
 * Revision 1.43.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.43.1.2  1994/07/22  20:08:11  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.43.1.1  1994/04/14  21:39:37  menze
 * Uses allocator-based message interface
 *
 */

/*
 * x-kernel Internet Control Message Protocol
 *
 * Correctly handle ALL incoming ICMP packets.
 * Generate outgoing ICMP packets when appropriate.
 *
 * For our purposes, the important ICMP messages to handle are
 *	redirects, which modify the routing table
 *	info requests, which generate IP addresses at boot time
 *	source quenches, which allow rudimentary congestion control
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/icmp.h>
#include <xkern/protocols/icmp/icmp_internal.h>

#if XK_DEBUG
int traceicmpp;
#endif /* XK_DEBUG */

static int		controlProtl( XObj, int, char *, int );
static void		getProcProtl(XObj);
static xkern_return_t	getProcSessn(XObj);
static long 		icmpHdrLoad(void *hdr, char *netHdr, long len, void *);
static xkern_return_t	icmp_close( XObj );
static int		icmp_controlSessn( XObj, int, char *, int );
static xkern_return_t	icmp_demux( XObj, XObj, Msg );
static XObj		icmp_open( XObj, XObj, XObj, Part, Path );
static long 		icmpRedirectLoad(void *, char *, long, void *);
static void		updateCksum( short *, int, int );


xkern_return_t
icmp_init(self)
    XObj self;
{
    Part_s	part;
    Pstate	*pstate;
    Path	path = self->path;
    
    xTrace0(icmpp,3, "ICMP init");
    
    xAssert(xIsProtocol(self));
    getProcProtl(self);
    
    if ( ! (pstate = pathAlloc(path, sizeof(Pstate))) ) {
	return XK_FAILURE;
    }
    self->state = pstate;
    pstate->sessionsCreated = 0;
    if ( ! (pstate->waitMap = mapCreate(23, sizeof(mapId), path)) ) {
	pathFree(pstate);
	return XK_FAILURE;
    }
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    xOpenEnable(self, self, xGetDown(self, 0), &part);
    return XK_SUCCESS;
}


/*
 * icmp_open is here to provide user processes with a limited access to
 * icmp functions.  Returns a session upon which control ops may be executed.
 */
static XObj
icmp_open(self, hlpRcv, hlpType, p, path)
    XObj self, hlpRcv, hlpType;
    Part p;
    Path path;
{
    Sessn 	s;
    Sessn 	down_s;
    Pstate 	*pstate;
    Sstate 	*sstate;
    
    xTrace0(icmpp,3,"ICMP open");
    pstate = (Pstate *)self->state;
    down_s = xOpen(self, self, xGetDown(self, 0), p, path);
    s = xCreateSessn(getProcSessn, hlpRcv, hlpType, self, 1, &down_s, path);
    if ( s == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    if ( ! (sstate = pathAlloc(path, sizeof(Sstate))) ) {
	icmp_close(s);
	return ERR_XOBJ;
    }
    s->state = sstate;
    bzero(s->state, sizeof(Sstate));
    if ( ! (sstate->timeoutEvent = evAlloc(path)) ) {
	icmp_close(s);
	return ERR_XOBJ;
    }
    sstate->sessId = ++(pstate->sessionsCreated);
    sstate->seqNum = 0;
    semInit(&sstate->replySem, 0);
    xTrace1(icmpp,3,"ICMP open returns %x",s);
    return s;
}


/*
 * This implementation does no caching of sessions -- close destroys
 * the session
 */
static xkern_return_t
icmp_close(s)
    XObj s;
{
    int 	i;
    Sstate	*ss = s->state;
    
    xAssert(s->rcnt <= 0);
    xAssert(xIsSession(s));
    if ( ss->timeoutEvent ) {
	evDetach(ss->timeoutEvent);
    }
    for (i=0; i < s->numdown; i++) {
	xClose(xGetDown(s, i));
    }
    xDestroy(s);
    return XK_SUCCESS;
}


/*
 * updateCksum: increase the 1's complement short pointed to by 'cksum'
 * by the difference of 'n1' and 'n2'
 */
static void
updateCksum(cksum, n1, n2)
    short int *cksum;
    int n1;
    int n2;
{
  u_short sum[2];

  xTrace3(icmpp, 7, "updateCksum: original sum == %x, n1=%x, n2=%x",
	  *cksum, n1, n2);
  sum[0] = *cksum;
  /*
   * sum[1] is the 1's complement representation of the difference
   */
  sum[1] = htons ( (n1 >= n2) ? (n1 - n2) : ~(n2 - n1) );
  *cksum = ocsum( sum, 2 );
  xTrace1(icmpp, 7, "updateCksum: new sum == %x", *cksum);
}


static xkern_return_t
icmp_demux(self, s, msg)
    XObj self;
    XObj s;
    Msg msg;
{
  ICMPHeader	hdr;
  loadInfo	loadI;

  /* msg_peek(msg, 0, sizeof(ICMPHeader), (char *)&hdr); */
  xAssert(xIsProtocol(self));
  xAssert(xIsSession(s));

  loadI.msg = msg;
  if ( msgPop(msg, icmpHdrLoad, (char *)&hdr, sizeof(ICMPHeader), &loadI)
      	== XK_FAILURE ) {
      xTraceP0(self, TR_ERRORS, "msgPop fails");
      return XK_FAILURE;
  }
  xTrace1(icmpp, 3, "Received an ICMP message with type %d", hdr.icmp_type);
  if ( loadI.sum ) {
    xTrace0(icmpp, 4, "icmp_demux: invalid checksum -- dropping message");
    return XK_SUCCESS;
  }
  switch(hdr.icmp_type)
    {
    case ICMP_ECHO_REQ:
    case ICMP_INFO_REQ:
      {
	u_short origType;

	xTrace1(icmpp, 3, "icmp_demux: echo/info request with msg length %d",
		msgLen(msg) - sizeof(ICMPEcho));
	origType = hdr.icmp_type;
	hdr.icmp_type =
	  ( origType == ICMP_ECHO_REQ ) ? ICMP_ECHO_REP : ICMP_INFO_REP;
	/*
	 * We can just do an incremental update of the msg checksum rather
	 * than recomputing it.
	 */
	updateCksum(&hdr.icmp_cksum, origType << 8, hdr.icmp_type << 8);
	if ( msgPush(msg, icmpHdrStore, &hdr, sizeof(ICMPHeader), NULL) == XK_SUCCESS ) {
	    xAssert(! inCkSum(msg, 0, 0));
	    xTrace0(icmpp, 3, "icmp_demux: sending reply");
	    xPush(s, msg);
	} else {
	    xTraceP0(self, TR_ERRORS, "msgPush fails");
	}
      }
      break;

    case ICMP_REDIRECT:
      {
	IPhost addrs[2];
	ICMPRedirect rd;
	
	if ( msgPop(msg, icmpRedirectLoad, &rd, sizeof(ICMPRedirect), NULL)
	    	== XK_FAILURE ) {
	    xTraceP0(self, TR_ERRORS, "msgPop fails");
	    break;
	}
	addrs[0] = rd.icmp_badmsg.icmp_dest.dest;
	addrs[1] = rd.icmp_gw;
	(void)xControl(xGetDown(self, 0), IP_REDIRECT, (char *)addrs,
		       2*sizeof(IPhost));
	
      }
      break;

    case ICMP_ECHO_REP:
      icmpPopEchoRep(self, msg);
      break;

    case ICMP_DEST_UNRCH:
    case ICMP_SRC_QUENCH:
    case ICMP_TIMEOUT:
    case ICMP_SYNTAX:
    case ICMP_TSTAMP_REQ:
    case ICMP_TSTAMP_REP:
    case ICMP_INFO_REP:
    case ICMP_AMASK_REQ:
    case ICMP_AMASK_REP:
      xTrace1(icmpp,3,"I can't handle ICMP packet type %d!\n", hdr.icmp_type);
      break;
      
    default:
      xTrace1(icmpp,3,"ICMP drops nonexistent ICMP message type %d!",
	      hdr.icmp_type);
    }
  return XK_SUCCESS;
}


static int
icmp_controlSessn(s, opCode, buf, len)
    XObj s;
    int opCode;
    char *buf;
    int len;
{
  xAssert(xIsSession(s));
  switch (opCode)
    {
    case ICMP_ECHO_CTL:
      return icmpSendEchoReq(s, *(int *)buf);
    case GETMAXPACKET:
    case GETOPTPACKET:
      /* 
       * These operations are probably only relevant for Echo's and
       * information requests, so we'll subtract that amount from the
       * lls's result.
       */
      if ( xControl(xGetDown(s, 0), opCode, buf, len) <= sizeof(int) ) {
	  return -1;
      }
      *(int *)buf -= sizeof(ICMPHeader) + sizeof(ICMPEcho);
    case GETMYPROTO:
    case GETPEERPROTO:
      return 0;
    default:
      /*
       * Unknown or unimplemented control operation -- send to down session
       */
      return xControl(xGetDown(s, 0), opCode, buf, len);
    }
}


/* 
 * controlProtl -- ICMP doesn't implement any control operations,
 * just forwards them to the lower protocol.
 */
static int
controlProtl(s, opCode, buf, len)
    XObj s;
    int opCode;
    char *buf;
    int len;
{
    return xControl(xGetDown(s, 0), opCode, buf, len);
}


/*
 * icmpHdrLoad -- loads hdr from msg.  Checksum is computed over the
 * entire message and written in the checksum field of the load struct
 * (passed through 'arg')
 */
static long
icmpHdrLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    u_short *sum = &((loadInfo *)arg)->sum;
    Msg msg = ((loadInfo *)arg)->msg;
    
    xAssert(len == sizeof(ICMPHeader));
    *sum = inCkSum(msg, 0, 0);
    bcopy(src, (char *)hdr, sizeof(ICMPHeader));
    return sizeof(ICMPHeader);
}  


/*
 * icmpHdrStore -- stores hdr onto msg.
 *
 * If arg is NULL, checksum is assumed to be already computed.
 *
 * Otherwise arg should point to the message and the checksum is computed
 * over the entire message.  In this case the checksum field of *hdr will
 * be modified.
 */
void
icmpHdrStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    xAssert(len == sizeof(ICMPHeader));
    if (arg) {
	((ICMPHeader *)hdr)->icmp_cksum = 0;
	bcopy((char *)hdr, dst, sizeof(ICMPHeader));
	((ICMPHeader *)hdr)->icmp_cksum = inCkSum((Msg)arg, 0, 0);
	bcopy((char *)hdr, dst, sizeof(ICMPHeader));
	xAssert(! inCkSum((Msg)arg, 0, 0));
    } else {
	bcopy((char *)hdr, dst, sizeof(ICMPHeader));
    }
}  


long
icmpEchoLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    xAssert(len == sizeof(ICMPEcho));
    bcopy(src, (char *)hdr, sizeof(ICMPEcho));
    ((ICMPEcho *)hdr)->icmp_id = ntohs(((ICMPEcho *)hdr)->icmp_id);
    ((ICMPEcho *)hdr)->icmp_seqnum = ntohs(((ICMPEcho *)hdr)->icmp_seqnum);
    return sizeof(ICMPEcho);
}


/*
 * Note: the fields of *hdr will be modified
 */
void
icmpEchoStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    xAssert(len == sizeof(ICMPEcho));
    ((ICMPEcho *)hdr)->icmp_id = ntohs(((ICMPEcho *)hdr)->icmp_id);
    ((ICMPEcho *)hdr)->icmp_seqnum = ntohs(((ICMPEcho *)hdr)->icmp_seqnum);
    bcopy((char *)hdr, dst, sizeof(ICMPEcho));
}


static long
icmpRedirectLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    xAssert(len == sizeof(ICMPRedirect));
    bcopy(src, (char *)hdr, sizeof(ICMPRedirect));
    return sizeof(ICMPRedirect);
}


static xkern_return_t
getProcSessn(s)
    XObj s;
{
  s->control = icmp_controlSessn;
  s->close = icmp_close;
  return XK_SUCCESS;
}


static void
getProcProtl(s)
    XObj s;
{
  s->open = icmp_open;
  s->demux = icmp_demux;
  s->control = controlProtl;
}

