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
 * tcp_x.c,v
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp_x.c,v
 * Revision 1.54.1.4.1.2  1994/09/07  04:18:36  menze
 * OSF modifications
 *
 * Revision 1.54.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.54.1.3  1994/09/01  20:07:41  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.54.1.2  1994/07/22  20:08:18  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.54.1.1  1994/04/20  22:05:11  hkaram
 * Uses allocator-based message interface
 *
 * Revision 1.54  1994/01/08  21:23:50  menze
 *   [ 1994/01/05          menze ]
 *   PROTNUM changed to PORTNUM
 *
 * Revision 1.53  1993/12/16  01:45:15  menze
 * Strict ANSI compilers weren't combining multiple tentative definitions
 * of external variables into a single definition at link time.
 *
 * Revision 1.52  1993/12/13  23:26:33  menze
 * Modifications from UMass:
 *
 *   [ 93/08/20          nahum ]
 *   Fixed #undef problem.
 *
 * Revision 1.51  1993/12/11  00:25:08  menze
 * fixed #endif comments
 *
 * Revision 1.50  1993/12/07  18:15:56  menze
 * Improved validity checks on incoming participants
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/tcp/tcp_internal.h>
#include <xkern/protocols/tcp/tcp_fsm.h>
#include <xkern/protocols/tcp/tcp_seq.h>
#include <xkern/protocols/tcp/tcp_timer.h>
#include <xkern/protocols/tcp/tcp_var.h>
#include <xkern/protocols/tcp/tcpip.h>


char *prurequests[] = {
	"ATTACH",	"DETACH",	"BIND",		"LISTEN",
	"CONNECT",	"ACCEPT",	"DISCONNECT",	"SHUTDOWN",
	"RCVD",		"SEND",		"ABORT",	"CONTROL",
	"SENSE",	"RCVOOB",	"SENDOOB",	"SOCKADDR",
	"PEERADDR",	"CONNECT2",	"FASTTIMO",	"SLOWTIMO",
	"PROTORCV",	"PROTOSEND",
};


tcp_seq	tcp_iss;		/* tcp initial send seq # */


static IPhost   myHost;

static xkern_return_t	tcpSessnInit( XObj );
static XObj		tcp_establishopen( XObj, XObj, XObj, IPhost *, IPhost *,
					   int, int, Path );
static void		tcp_dumpstats( struct tcpstat * );
static xkern_return_t	tcpClose( XObj );
static int		tcpControlProtl( XObj, int, char *, int );
static int		tcpControlSessn( XObj, int, char *, int );
static XObj		tcpOpen( XObj, XObj, XObj, Part, Path );
static xkern_return_t	tcpOpenEnable( XObj, XObj, XObj, Part );
static xkern_return_t	tcpOpenDisable( XObj, XObj, XObj, Part );
static xmsg_handle_t	tcpPush( XObj, Msg );

/* 
 * Check for a valid participant list
 */
#define partCheck(p, name, max, retval)					\
	{								\
	  if ( ! (p) || partLen(p) < 1 || partLen(p) > (max)) { 	\
		xTrace1(tcpp, TR_ERRORS,				\
			"%s -- bad participants",			\
			(name));  					\
		return (retval);					\
	  }								\
	}


/*************************************************************************/

static void
psFree( PSTATE *ps )
{
    if ( ps->passiveMap ) mapClose(ps->passiveMap);
    if ( ps->activeMap ) mapClose(ps->activeMap);
    if ( ps->tcpstat ) pathFree(ps->tcpstat);
    if ( ps->tcb ) pathFree(ps->tcb);
    if ( ps->portstate ) tcpPortMapClose(ps->portstate);
    pathFree(ps);
}

xkern_return_t
tcp_init(self)
    XObj	self;
{
  Part_s	p;
  PSTATE	*pstate;
  Path		path = self->path;
  Event		ev1 = 0, ev2 = 0;

  xTrace0(tcpp, TR_GROSS_EVENTS, "TCP init");
  xTrace1(tcpp, TR_GROSS_EVENTS, "BYTE_ORDER = %d", BYTE_ORDER);

  xAssert(xIsProtocol(self));

  if ( ! (pstate = pathAlloc(path, sizeof(PSTATE))) ) {
      xTraceP0(self, TR_ERRORS, "allocation failure");
      return XK_FAILURE;
  }
  bzero((char *)pstate,sizeof(PSTATE));
  self->state = (char *)pstate;

  self->control = tcpControlProtl;
  self->open = tcpOpen;
  self->openenable = tcpOpenEnable;
  self->opendisable = tcpOpenDisable;
  self->demux = tcp_input;

  if ( tcpPortMapInit(&pstate->portstate, path) != XK_SUCCESS ) {
      psFree(pstate);
      return XK_FAILURE;
  }
  if ( ! (pstate->passiveMap = mapCreate(40, sizeof(PassiveId), path)) ||
       ! (pstate->activeMap = mapCreate(100, sizeof(ActiveId), path)) ||
       ! (pstate->tcpstat = pathAlloc(path, sizeof(struct tcpstat))) ||
       ! (pstate->tcb = pathAlloc(path, sizeof(struct inpcb))) ) { 
      xTraceP0(self, TR_ERRORS, "allocation error");
      psFree(pstate);
      return XK_FAILURE;
  }
  pstate->tcb->inp_next = pstate->tcb->inp_prev = pstate->tcb;
  tcp_iss = 1;
  /* Turn on IP pseudoheader length-fixup kludge in lower protocols. */
  xControl(xGetDown(self, 0),IP_PSEUDOHDR,(char *)0,0);
  if ( ! (ev1 = evAlloc(path)) || ! (ev2 = evAlloc(path)) ) {
      if ( ev1 ) { evDetach(ev1); }
      if ( ev2 ) { evDetach(ev2); }
      psFree(pstate);
      return XK_FAILURE;
  }
  evDetach(evSchedule(ev1, tcp_fasttimo, self, TCP_FAST_INTERVAL));
  evDetach(evSchedule(ev2, tcp_slowtimo, self, TCP_SLOW_INTERVAL));
  partInit(&p, 1);
  partPush(p, ANY_HOST, 0);
  xOpenEnable(self, self, xGetDown(self, 0), &p);
  xControl(xGetDown(self, 0), GETMYHOST, (char *)&myHost, sizeof(myHost));
  return XK_SUCCESS;
}

/*************************************************************************/
static struct tcpstate *
new_tcpstate( Path p )
{
    struct tcpstate *tcpstate;
    if ( ! (tcpstate = pathAlloc(p, sizeof(struct tcpstate))) ) {
	return 0;
    }
    bzero((char *)tcpstate, sizeof(struct tcpstate));
    if ( ! (tcpstate->snd = pathAlloc(p, sizeof(struct sb))) ) {
	pathFree(tcpstate);
	return 0;
    }
    tcpSemInit(&tcpstate->waiting, 0);
    tcpSemInit(&tcpstate->lock, 1);
    tcpstate->closed = 0;
    /* initialize buffer size to TCPRCVWIN to get things started: */
    tcpstate->rcv_space = TCPRCVWIN;
    tcpstate->rcv_hiwat = TCPRCVWIN;
    return tcpstate;
}

/*************************************************************************/
void delete_tcpstate(tcpstate)
struct tcpstate *tcpstate;
{
    tcpVAll(&tcpstate->lock);
    pathFree(tcpstate->snd);
    pathFree(tcpstate);
}

/*************************************************************************/
static XObj
tcp_establishopen(self, hlpRcv, hlpType, raddr, laddr, rport, lport, path)
    XObj self, hlpRcv, hlpType;
    IPhost *raddr, *laddr;
    int rport, lport;
    Path	path;
{
  Part_s        p[2];
  XObj		new;
  struct tcpstate *tcpst;
  struct inpcb *inp;
  ActiveId      ex_id;
  XObj		lls;
  PSTATE	*pstate;

  xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp_establish: on %X", hlpRcv);
  xAssert(xIsProtocol(hlpRcv));

  pstate = (PSTATE *)self->state;
  ex_id.localport = lport;
  ex_id.remoteport = rport;
  ex_id.remoteaddr = *raddr;
  if ( mapResolve(pstate->activeMap, &ex_id, &new) == XK_SUCCESS ) {
    xTrace3(tcpp, TR_GROSS_EVENTS, "tcp_establish: (%d->%X.%d) already open",
	    ex_id.localport, ex_id.remoteaddr, ex_id.remoteport);
    return 0;
  }

  partInit(p, laddr ? 2 : 1);
  partPush(p[0], raddr, sizeof(IPhost));
  if ( laddr ) {
      partPush(p[1], laddr, sizeof(IPhost));
  }
  xTrace0(tcpp, TR_EVENTS, "tcpOpen: opening IP");
  if ( (lls = xOpen(self, self, xGetDown(self,0), p, path)) == ERR_SESSN ) {
    xTrace0(tcpp, TR_ERRORS, "tcpOpen: cannot open IP session");
    return 0;
  }

  if ( ! (tcpst = new_tcpstate(path)) ) {
      xTraceP0(self, TR_SOFT_ERRORS, "tcpstate allocation error");
      xClose(lls);
      return 0;
  }
  new = xCreateSessn(tcpSessnInit, hlpRcv, hlpType, self, 1, &lls, path);
  if ( new == ERR_XOBJ ) {
      xClose(lls);
      delete_tcpstate(tcpst);
      return 0;
  }
  xTrace3(tcpp, TR_GROSS_EVENTS, "Mapping %d->%X.%d", lport, raddr, rport);
  if ( mapBind(pstate->activeMap, &ex_id, new, &new->binding) != XK_SUCCESS ) {
      xTraceP3(self, TR_ERRORS, "tcp_establish: Bind of %d->%X.%d failed", 
	       lport, raddr, rport);
      xClose(lls);
      xDestroy(new);
      delete_tcpstate(tcpst);
      return 0;
  }

  tcpst->hlpType = hlpType;
  new->state = (char *)tcpst;
  xTrace0(tcpp, TR_EVENTS, "tcpOpen: attaching...");
  if ( tcp_attach(new) ) {
      mapRemoveBinding(pstate->activeMap, new->binding);
      xClose(lls);
      delete_tcpstate(tcpst);
      new->state = 0;
      xDestroy(new);
      return 0;
  }

  inp = sotoinpcb(new);
  tcpst->inpcb = inp;
  tcpst->tcpcb = (struct tcpcb *)inp->inp_ppcb;
  *(IPhost *)&inp->inp_laddr = myHost;
  inp->inp_lport = ex_id.localport;
  *(IPhost *)&inp->inp_raddr = ex_id.remoteaddr;
  inp->inp_rport = ex_id.remoteport;

  /*  xControl(xGetDown(new,0),IP_PSEUDOHDR,(char *)0,0);  moved to tcp_init --rcs */
  return new;
}


static int
extractPart(
    Part	p,
    long	*port,
    IPhost	**host,
    char	*s)
{
    long	*portPtr;
    IPhost	*hostPtr;

    xAssert(port);
    if ( ! p || partStackTopByteLen(*p) < sizeof(long) ||
	 (portPtr = (long *)partPop(*p)) == 0 ) {
	xTrace1(tcpp, TR_SOFT_ERROR, "Bad participant in %s -- no port", s);
	return -1;
    }
    *port = *portPtr;
    if ( *port > MAX_PORT ) {
	xTrace2(tcpp, TR_SOFT_ERROR,
		"Bad participant in %s -- port %d out of range", s, *port);
	return -1;
    }
    if ( host ) {
	if ( partStackTopByteLen(*p) < sizeof(IPhost) ||
	     (hostPtr = (IPhost *)partPop(*p)) == 0 ) {
	    xTrace1(tcpp, TR_SOFT_ERROR,
		    "Bad participant in %s -- no host", s);
	    return -1;
	}
	*host = hostPtr;
    }
    return 0;
}


/*************************************************************************/
static XObj
tcpOpen(self, hlpRcv, hlpType, p, path)
    XObj        self, hlpRcv, hlpType;
    Part        p;
    Path	path;
{
    XObj	s;
    long	remotePort, localPort = ANY_PORT;
    IPhost	*remoteHost, *localHost = 0;
    
    xTrace0(tcpp, TR_GROSS_EVENTS, "TCP open");
    partCheck(p, "tcpOpen", 2, ERR_XOBJ);
    if ( extractPart(p, &remotePort, &remoteHost, "tcpOpen (remote)") ) {
	return ERR_XOBJ;
    }
    if ( partLen(p) > 1 ) {
	if ( extractPart(p+1, &localPort, &localHost, "tcpOpen (local)") ) {
	    return ERR_XOBJ;
	}
	if ( localHost == (IPhost *)ANY_HOST ) {
	    localHost = 0;
	}
    }
    if ( localPort == ANY_PORT ) {
	/* 
	 * We need to find a free local port
	 */
	long	freePort;
	if ( tcpGetFreePort(((PSTATE *)(self->state))->portstate, &freePort) ) {
	    xError("tcpOpen -- no free ports!");
	    return ERR_XOBJ;
	}
	localPort = freePort;
    } else {
	/* 
	 * A specific local port was requested
	 */
	if ( tcpDuplicatePort(((PSTATE *)(self->state))->portstate, localPort))
	  {
	    xTrace1(tcpp, TR_MAJOR_EVENTS,
		    "tcpOpen: requested port %d is already in use",
		    localPort);
	    return ERR_XOBJ;
	  }
      }
    s = tcp_establishopen(self, hlpRcv, hlpType, remoteHost, localHost,
			  remotePort, localPort, path);
    if (!s) {
	tcpReleasePort(((PSTATE *)(self->state))->portstate, localPort);
	s = ERR_SESSN;
    } else {
	xTrace0(tcpp, TR_EVENTS, "tcpOpen: connecting...");
	tcp_usrreq(s, PRU_CONNECT, 0, 0);
	
	xTrace0(tcpp, TR_EVENTS, "tcpOpen: waiting...");
	tcpSemWait(&(sototcpst(s)->waiting));
	if (sototcpcb(s)->t_state != TCPS_ESTABLISHED) {
	    xTrace3(tcpp, TR_EVENTS, "tcpOpen: open (%d->%X.%d) connect failed",
		    localPort,  ipHostStr(remoteHost), remotePort);
	    tcp_destroy(sototcpcb(s));
	    tcpReleasePort(((PSTATE *)(self->state))->portstate, localPort);
	    s = ERR_SESSN;
	}
    }
    xTrace1(tcpp, TR_GROSS_EVENTS, "Return from tcpOpen, session = %x", s);
    return s;
}

/*************************************************************************/
static xkern_return_t
tcpOpenEnable(self, hlpRcv, hlpType, p)
    XObj	self, hlpRcv, hlpType;
    Part	p;
{
    PassiveId	key;
    long	protNum;
    PSTATE	*pstate;
    Enable	*e;
    
    partCheck(p, "tcpOpenEnable", 1, XK_FAILURE);
    if ( extractPart(p, &protNum, 0, "tcpOpenEnable") ) {
	return XK_FAILURE;
    }
    key = protNum;
    pstate = (PSTATE *)self->state;
    xTrace2(tcpp, TR_MAJOR_EVENTS, "tcpOpenEnable mapping %d->%x", key,
	    hlpRcv);
    if ( mapResolve(pstate->passiveMap, &key, &e) == XK_SUCCESS ) {
	if ( e->hlpRcv == hlpRcv && e->hlpType == hlpType ) {
	    e->rcnt++;
	    return XK_SUCCESS;
	}
	return XK_FAILURE;
    }
    if ( tcpDuplicatePort(pstate->portstate, key) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    if ( ! (e = (Enable *)pathAlloc(self->path, sizeof(Enable))) ) {
	xTraceP0(self, TR_SOFT_ERRORS, "openEnable allocation failure");
	tcpReleasePort(pstate->portstate, key);
	return XK_FAILURE;
    }
    e->hlpRcv = hlpRcv;
    e->hlpType = hlpType;
    e->rcnt = 1;
    if ( mapBind(pstate->passiveMap, &key, e, &e->binding) != XK_SUCCESS ) {
	pathFree((char *)e);
	tcpReleasePort(pstate->portstate, key);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

/*************************************************************************/
static xkern_return_t
tcpOpenDisable(self, hlpRcv, hlpType, p)
    XObj	self, hlpRcv, hlpType;
    Part        p;
{
    PassiveId	key;
    long	protNum;
    Enable	*e;
    PSTATE	*pstate;

    pstate = (PSTATE *)self->state;
    partCheck(p, "tcpOpenDisable", 1, XK_FAILURE);
    if ( extractPart(p, &protNum, 0, "tcpOpenEnable") ) {
	return XK_FAILURE;
    }
    key = protNum;
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp_disable removing %d", key);
    if ( mapResolve(pstate->passiveMap, &key, &e) == XK_FAILURE ||
		e->hlpRcv != hlpRcv || e->hlpType != hlpType ) {
	return XK_FAILURE;
    }
    if (--(e->rcnt) == 0) {
	mapRemoveBinding(pstate->passiveMap, e->binding);
	pathFree(e);
	tcpReleasePort(pstate->portstate, key);
    }
    return XK_SUCCESS;
}

/*************************************************************************/
/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
static xkern_return_t
tcpClose(so)
    XObj	so;
{
    register struct tcpcb *tp;
    register struct tcpstate *tcpst;
    
    xTrace1(tcpp, TR_FUNCTIONAL_TRACE, "tcpClose(so=%08x)", so);

    tcpst = sototcpst(so);
    tcpst->closed |= 1;
    
    tp = sototcpcb(so);
    tp = tcp_usrclosed(tp);
    if (tp) tcp_output(tp);

    xTrace2(tcpp, TR_MAJOR_EVENTS, "tcpClose: tp=%08x, tcpst->closed=%x",
	    tp, tcpst->closed);
    
    if (tcpst->closed == 1) {
	/*
	 * This was a user close before the network close
	 */
	xTrace0(tcpp, TR_EVENTS, "tcpClose: waiting...");
	tcpSemWait(&tcpst->waiting);
	xTrace0(tcpp, TR_EVENTS, "tcpClose: done");
    }
    return XK_SUCCESS;
}


/*************************************************************************/
static xmsg_handle_t
tcpPush(so, msg)
     XObj so;
     Msg msg;
{
    int error;
    int space;
    Msg_s pushMsg;
    register struct tcpstate *tcpst = sototcpst(so);
    register struct tcpcb *tp = sototcpcb(so);

    xTrace2(tcpp, TR_MAJOR_EVENTS, "tcpPush: session %X, msg %d bytes",
	    so, msgLen(msg));
    if (so->rcnt < 1) {
	xTrace1(tcpp, TR_GROSS_EVENTS, "tcpPush: session ref count = %d",
		so->rcnt);
	return XMSG_ERR_HANDLE;
    }
    if (tcpst->closed) {
	xTrace1(tcpp, TR_SOFT_ERROR, "tcpPush: session already closed %d",
		tcpst->closed);
	return XMSG_ERR_HANDLE;
    }

#undef CHUNKSIZE /* (sbhiwat(tcpst->snd) / 2) */
#define CHUNKSIZE (tp->t_maxseg)

    if ((tcpst->flags & TCPST_NBIO) &&
	(sbspace(tcpst->snd) < MIN(msgLen(msg), CHUNKSIZE)))
    {
	return XMSG_ERR_WOULDBLOCK;
    } /* if */

    msgConstructContig(&pushMsg, xMyPath(so), 0, 0);
    while ( msgLen(msg) != 0 ) {
	xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
		"tcpPush: msgLen = <%d>", msgLen(msg));
	msgChopOff(msg, &pushMsg, CHUNKSIZE);
	xTrace2(tcpp, TR_FUNCTIONAL_TRACE,
		"tcpPush: after break pushMsg = <%d> msg = <%d>",
		msgLen(&pushMsg), msgLen(msg));
	while ((space = sbspace(tcpst->snd)) < msgLen(&pushMsg)) {
	    xTrace1(tcpp, TR_GROSS_EVENTS,
		    "tcpPush: waiting for space (%d available)", space);
	    xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
		    "tcpPush: msgLen == %d before blocking",
		    msgLen(msg));
	    tcpSemWait(&tcpst->waiting);
	    if (tcpst->closed) {
		xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
			"tcpPush: session already closed %d", tcpst->closed);
		msgDestroy(&pushMsg);
		return XMSG_ERR_HANDLE;
	    }
	}
	while ( sbappend(tcpst->snd, &pushMsg, so) != XK_SUCCESS ) {
	    if ( tcpst->flags & TCPST_NBIO ) {
		msgDestroy(&pushMsg);
		return XMSG_ERR_WOULDBLOCK;
	    }
	    xTraceP0(so, TR_SOFT_ERRORS, "couldn't allocate sb, waiting");
	    tcpst->flags |= TCPST_MEMWAIT;
	    tcpSemWait(&tcpst->waiting);
	    tcpst->flags &= ~TCPST_MEMWAIT;
	    if ( tcpst->closed ) {
		msgDestroy(&pushMsg);
		return XMSG_ERR_HANDLE;
	    }
	}
	xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
		"tcppush about to append to sb.  orig msgLen == %d",
		msgLen(msg));
	if ( tcpst->flags & TCPST_PUSH_ALWAYS ) {
	    tp->t_force = 1;
	    error = tcp_output(sototcpcb(so));
	    tp->t_force = 0;
	} else {
	    error = tcp_output(sototcpcb(so));
	}
	if (error) {
	    xTraceP1(so, TR_SOFT_ERRORS, "push failed with code %d", error);
	}
    }
    msgDestroy(&pushMsg);
    return XMSG_NULL_HANDLE;
}

/*************************************************************************/
static int
tcpControlSessn(s, opcode, buf, len)
    Sessn           s;
    char           *buf;
    int             opcode;
    int             len;
{
  struct tcpstate *ts;
  struct tcpcb   *tp;
  u_short size;

  ts = (struct tcpstate *) s->state;
  tp = ts->tcpcb;
  switch (opcode) {

   case GETMYPROTO:
     checkLen(len, sizeof(long));
     *(long *)buf = tp->t_template->ti_sport;
     return sizeof(long);

   case GETPEERPROTO:
     checkLen(len, sizeof(long));
     *(long *)buf = tp->t_template->ti_dport;
     return sizeof(long);

   case TCP_PUSH:
     tp->t_force = 1;
     tcp_output(tp);
     tp->t_force = 0;
     return 0;

   case TCP_GETSTATEINFO:
     *(int *) buf = tp->t_state;
     return (sizeof(int));

   case GETOPTPACKET:
   case GETMAXPACKET:
     if ( xControl(xGetDown(s, 0), opcode, buf, len) < sizeof(int) ) {
	 return -1;
     }
     *(int *)buf -= sizeof(struct tcphdr);
     return sizeof(int);
     
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
		 localPort = tp->t_template->ti_sport;
		 partPush(p[1], &localPort, sizeof(long));
	     }
	     remotePort = tp->t_template->ti_dport;
	     partPush(p[0], &remotePort, sizeof(long));
	     xkr = partExternalize(p, buf, &len);
	     if ( xkr == XK_FAILURE ) {
		 xTrace0(tcpp, TR_SOFT_ERRORS,
			 "TCP GETPARTS -- buffer too small");
		 retLen = -1;
	     } else {
		 retLen = len;
	     }
	 } else {
	     xTrace1(tcpp, TR_SOFT_ERRORS,
		 "TCP -- Bad number of participants (%d) returned in GETPARTS",
		     partExtLen(buf));
	     retLen = -1;
	 }
	 return retLen;
     }

   case SETNONBLOCKINGIO:
     xControl(xGetDown(s, 0), opcode, buf, len);
     if (*(int*)buf) {
	 ts->flags |= TCPST_NBIO;
     } else {
	 ts->flags &= ~TCPST_NBIO;
     } /* if */
     return 0;

   case TCP_SETPUSHALWAYS:
     checkLen(len, sizeof(int));
     if (*(int*)buf) {
	 ts->flags |= TCPST_PUSH_ALWAYS;
     } else {
	 ts->flags &= ~TCPST_PUSH_ALWAYS;
     } /* if */
     return 0;

   case TCP_SETRCVACKALWAYS:
     checkLen(len, sizeof(int));
     if (*(int*)buf) {
	 ts->flags |= TCPST_RCV_ACK_ALWAYS;
     } else {
	 ts->flags &= ~TCPST_RCV_ACK_ALWAYS;
     } /* if */
     return 0;

   case TCP_SETRCVBUFSPACE:
     checkLen(len, sizeof(u_short));
     size = *(u_short *)buf;
     ts->rcv_space = size;
     /* after receiving message possibly send window update: */
     tcp_output(tp);
     return 0;

   case TCP_GETSNDBUFSPACE:
     checkLen(len, sizeof(u_short));
     *(u_short*)buf = sbspace(ts->snd);
     return sizeof(u_short);

   case TCP_SETRCVBUFSIZE:
     checkLen(len, sizeof(u_short));
     size = *(u_short *)buf;
     ts->rcv_hiwat = size;
     return 0;

   case TCP_SETSNDBUFSIZE:
     checkLen(len, sizeof(u_short));
     sbhiwat(ts->snd) = *(u_short *)buf;
     return 0;

   case TCP_SETOOBINLINE:
     checkLen(len, sizeof(int));
     if (*(int*)buf) {
	 ts->flags |= TCPST_OOBINLINE;
     } else {
	 ts->flags &= ~TCPST_OOBINLINE;
     } /* if */
     return 0;

   case TCP_GETOOBDATA:
     {     
	 char peek;

	 checkLen(len, sizeof(char));

	 peek = *(char*)buf;
	 if ((tp->t_oobflags & TCPOOB_HADDATA) ||
	     !(tp->t_oobflags & TCPOOB_HAVEDATA))
	 {
	     return 0;
	 } /* if */
	 *(char*)buf = tp->t_iobc;
	 if (!peek) {
	     tp->t_oobflags ^= TCPOOB_HAVEDATA | TCPOOB_HADDATA;
	 } /* if */
	 return 1;
     }

   case TCP_OOBPUSH:
     {
	 checkLen(len, sizeof(Msg));

	 return tcp_usrreq(s, PRU_SENDOOB, (Msg)buf, 0);
     }

   default:
     return xControl(xGetDown(s, 0), opcode, buf, len);
  }
}

/*************************************************************************/
static int
tcpControlProtl(self, opcode, buf, len)
    XObj	    self;
    int             opcode, len;
    char           *buf;
{
    long port;

    switch (opcode) {

      case TCP_DUMPSTATEINFO:
	{
	    PSTATE	*ps = (PSTATE *)self->state;

	    tcp_dumpstats(ps->tcpstat);
	    return 0;
	}

      case TCP_GETFREEPORTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	if (tcpGetFreePort(((PSTATE *)(self->state))->portstate, &port)) {
	    return -1;
	} /* if */
	*(long *)buf = port;
	return 0;

      case TCP_RELEASEPORTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	tcpReleasePort(((PSTATE *)(self->state))->portstate, port);
	return 0;

      default:
	return xControl(xGetDown(self,0), opcode, buf, len);
    }
}

/*************************************************************************/
static xkern_return_t
tcpSessnInit(s)
    XObj s;
{
    xAssert(xIsSession(s));
    s->push = tcpPush;
    s->close = tcpClose;
    s->control = tcpControlSessn;
    s->demux = tcp_input;
    s->pop = tcpPop;
    return XK_SUCCESS;
}

/*************************************************************************/
void
sorwakeup(so)
XObj so;
{
  xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp: sorwakeup on %X", so);
}
    
/*************************************************************************/
void
sowwakeup(so)
XObj so;
{
  xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp: sowwakeup on %X", so);
  tcpSemSignal(&sototcpst(so)->waiting);
}

/*************************************************************************/
/*ARGSUSED*/
int
soreserve(so, send, recv)
XObj so;
int send, recv;
{
  struct tcpstate *tcpst = sototcpst(so);
  sbinit(tcpst->snd);
  return 0;
}

/*************************************************************************/
void
socantsendmore(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  socantsendmore on %X", so);
}

/*************************************************************************/
void
soisdisconnected(so)
    XObj so;
{
    struct tcpstate *ts;

    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisdisconnected on %X", so);
    ts = sototcpst(so);
    if (ts->closed) {
	/* It was already closed, either by the network or the user */
    } else {
	/* deliver close done after we're done handling current message: */
	xCloseDone(so);
    } /* if */
    tcpVAll(&sototcpst(so)->waiting);
} /* soisdisconnected */

/*************************************************************************/
void
soabort(so)
XObj so;
{
}

/*************************************************************************/
void
socantrcvmore(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  socantrcvmore on %X", so);
    
    sototcpst(so)->closed |= 2;
    if (sototcpst(so)->closed & 1) {
	/* do nothing, the user already knows that it is closed */
    } else {
	xCloseDone(so);
    }
}

/*************************************************************************/
void
soisconnected(so)
    XObj so;
{
    struct tcpstate	*state = sototcpst(so);
    TcpSem 		*s = &state->waiting;

    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisconnected on %X", so);
    if (s->waitCount) {
	xTrace1(tcpp, TR_EVENTS, "tcp:  waking up opener on %X", so);
	tcpSemSignal(s);
    } else {
	/*
	 * I think that this is the time for a open done
	 */
	xOpenDone(xGetUp(so), so, xMyProtl(so), xMyPath(so));
    }
}

/*************************************************************************/
void
soisconnecting(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisconnecting on %X", so);
}

/*************************************************************************/
void
soisdisconnecting(so)
XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisdisconnecting on %X", so);
}

/*************************************************************************/
/* 
 * sonewconn is called by the input routine to establish a passive open
 */
XObj
sonewconn(self, so, hlpType, src, dst, sport, dport, path)
    XObj self, so, hlpType;
    IPhost *src, *dst;
    int sport, dport;
    Path	path;
{
  XObj new;
  xAssert(xIsProtocol(so));
  xTrace1(tcpp, TR_MAJOR_EVENTS, "sonewconn on %X", so);
  new = tcp_establishopen(self, so, hlpType, src, dst, sport, dport, path);
  if ( new ) {
      tcpDuplicatePort(((PSTATE *)self->state)->portstate, dport);
  }
  return new;
}


/*************************************************************************/
/* 
 * sohasoutofband is called by the input routine to signal the presence
 * of urgent (out-of-band) data
 */
void
sohasoutofband(so, oobmark)
     XObj so;
     u_int oobmark;
{
    void *buf[2];

    buf[0] = so;
    buf[1] = (void*) oobmark;
    xControl(xGetUp(so), TCP_OOBMODE, (char*) buf, sizeof(buf));
} /* sohasoutofband */

/*************************************************************************/

void
tcpSemWait( ts )
    TcpSem	*ts;
{
    ts->waitCount++;
    semWait(&ts->s);
    ts->waitCount--;
}


void
tcpSemSignal( ts )
    TcpSem	*ts;
{
    semSignal(&ts->s);
}


void
tcpVAll( ts )
    TcpSem	*ts;
{
    int i, n;

    n=ts->waitCount;
    for ( i=0; i < n; i++ ) {
	semSignal(&ts->s);
    }
}


void
tcpSemInit( ts, n )
    TcpSem	*ts;
    int		n;
{
    semInit(&ts->s, n);
    ts->waitCount = 0;
}

/*************************************************************************/
static void
tcp_dumpstats( tcpstat )
    struct tcpstat	*tcpstat;
{
  printf("tcps_badsum %d\n", tcpstat->tcps_badsum);
  printf("tcps_badoff %d\n", tcpstat->tcps_badoff);
  printf("tcps_hdrops %d\n", tcpstat->tcps_hdrops);
  printf("tcps_badsegs %d\n", tcpstat->tcps_badsegs);
  printf("tcps_unack %d\n", tcpstat->tcps_unack);
  printf("connections initiated %d\n", tcpstat->tcps_connattempt);
  printf("connections accepted %d\n", tcpstat->tcps_accepts);
  printf("connections established %d\n", tcpstat->tcps_connects);
  printf("connections dropped %d\n", tcpstat->tcps_drops);
  printf("embryonic connections dropped %d\n", tcpstat->tcps_conndrops);
  printf("conn. closed (includes drops) %d\n", tcpstat->tcps_closed);
  printf("segs where we tried to get rtt %d\n", tcpstat->tcps_segstimed);
  printf("times we succeeded %d\n", tcpstat->tcps_rttupdated);
  printf("delayed acks sent %d\n", tcpstat->tcps_delack);
  printf("conn. dropped in rxmt timeout %d\n", tcpstat->tcps_timeoutdrop);
  printf("retransmit timeouts %d\n", tcpstat->tcps_rexmttimeo);
  printf("persist timeouts %d\n", tcpstat->tcps_persisttimeo);
  printf("keepalive timeouts %d\n", tcpstat->tcps_keeptimeo);
  printf("keepalive probes sent %d\n", tcpstat->tcps_keepprobe);
  printf("connections dropped in keepalive %d\n", tcpstat->tcps_keepdrops);
  printf("total packets sent %d\n", tcpstat->tcps_sndtotal);
  printf("data packets sent %d\n", tcpstat->tcps_sndpack);
  printf("data bytes sent %d\n", tcpstat->tcps_sndbyte);
  printf("data packets retransmitted %d\n", tcpstat->tcps_sndrexmitpack);
  printf("data bytes retransmitted %d\n", tcpstat->tcps_sndrexmitbyte);
  printf("ack-only packets sent %d\n", tcpstat->tcps_sndacks);
  printf("window probes sent %d\n", tcpstat->tcps_sndprobe);
  printf("packets sent with URG only %d\n", tcpstat->tcps_sndurg);
  printf("window update-only packets sent %d\n", tcpstat->tcps_sndwinup);
  printf("control (SYN|FIN|RST) packets sent %d\n", tcpstat->tcps_sndctrl);
  printf("total packets received %d\n", tcpstat->tcps_rcvtotal);
  printf("packets received in sequence %d\n", tcpstat->tcps_rcvpack);
  printf("bytes received in sequence %d\n", tcpstat->tcps_rcvbyte);
  printf("packets received with ccksum errs %d\n", tcpstat->tcps_rcvbadsum);
  printf("packets received with bad offset %d\n", tcpstat->tcps_rcvbadoff);
  printf("packets received too short %d\n", tcpstat->tcps_rcvshort);
  printf("duplicate-only packets received %d\n", tcpstat->tcps_rcvduppack);
  printf("duplicate-only bytes received %d\n", tcpstat->tcps_rcvdupbyte);
  printf("packets with some duplicate data %d\n", tcpstat->tcps_rcvpartduppack);
  printf("dup. bytes in part-dup. packets %d\n", tcpstat->tcps_rcvpartdupbyte);
  printf("out-of-order packets received %d\n", tcpstat->tcps_rcvoopack);
  printf("out-of-order bytes received %d\n", tcpstat->tcps_rcvoobyte);
  printf("packets with data after window %d\n", tcpstat->tcps_rcvpackafterwin);
  printf("bytes rcvd after window %d\n", tcpstat->tcps_rcvbyteafterwin);
  printf("packets rcvd after \"close\" %d\n", tcpstat->tcps_rcvafterclose);
  printf("rcvd window probe packets %d\n", tcpstat->tcps_rcvwinprobe);
  printf("rcvd duplicate acks %d\n", tcpstat->tcps_rcvdupack);
  printf("rcvd acks for unsent data %d\n", tcpstat->tcps_rcvacktoomuch);
  printf("rcvd ack packets %d\n", tcpstat->tcps_rcvackpack);
  printf("bytes acked by rcvd acks %d\n", tcpstat->tcps_rcvackbyte);
  printf("rcvd window update packets %d\n", tcpstat->tcps_rcvwinupd);


  tcpstat->tcps_badsum = 0;
  tcpstat->tcps_badoff = 0;
  tcpstat->tcps_hdrops = 0;
  tcpstat->tcps_badsegs = 0;
  tcpstat->tcps_unack = 0;
  tcpstat->tcps_connattempt = 0;
  tcpstat->tcps_accepts = 0;
  tcpstat->tcps_connects = 0;
  tcpstat->tcps_drops = 0;
  tcpstat->tcps_conndrops = 0;
  tcpstat->tcps_closed = 0;
  tcpstat->tcps_segstimed = 0;
  tcpstat->tcps_rttupdated = 0;
  tcpstat->tcps_delack = 0;
  tcpstat->tcps_timeoutdrop = 0;
  tcpstat->tcps_rexmttimeo = 0;
  tcpstat->tcps_persisttimeo = 0;
  tcpstat->tcps_keeptimeo = 0;
  tcpstat->tcps_keepprobe = 0;
  tcpstat->tcps_keepdrops = 0;
  tcpstat->tcps_sndtotal = 0;
  tcpstat->tcps_sndpack = 0;
  tcpstat->tcps_sndbyte = 0;
  tcpstat->tcps_sndrexmitpack = 0;
  tcpstat->tcps_sndrexmitbyte = 0;
  tcpstat->tcps_sndacks = 0;
  tcpstat->tcps_sndprobe = 0;
  tcpstat->tcps_sndurg = 0;
  tcpstat->tcps_sndwinup = 0;
  tcpstat->tcps_sndctrl = 0;
  tcpstat->tcps_rcvtotal = 0;
  tcpstat->tcps_rcvpack = 0;
  tcpstat->tcps_rcvbyte = 0;
  tcpstat->tcps_rcvbadsum = 0;
  tcpstat->tcps_rcvbadoff = 0;
  tcpstat->tcps_rcvshort = 0;
  tcpstat->tcps_rcvduppack = 0;
  tcpstat->tcps_rcvdupbyte = 0;
  tcpstat->tcps_rcvpartduppack = 0;
  tcpstat->tcps_rcvpartdupbyte = 0;
  tcpstat->tcps_rcvoopack = 0;
  tcpstat->tcps_rcvoobyte = 0;
  tcpstat->tcps_rcvpackafterwin = 0;
  tcpstat->tcps_rcvbyteafterwin = 0;
  tcpstat->tcps_rcvafterclose = 0;
  tcpstat->tcps_rcvwinprobe = 0;
  tcpstat->tcps_rcvdupack = 0;
  tcpstat->tcps_rcvacktoomuch = 0;
  tcpstat->tcps_rcvackpack = 0;
  tcpstat->tcps_rcvackbyte = 0;
  tcpstat->tcps_rcvwinupd = 0;
}
