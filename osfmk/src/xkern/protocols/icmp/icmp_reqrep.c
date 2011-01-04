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
 * icmp_reqrep.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * icmp_reqrep.c,v
 * Revision 1.17.1.4.1.2  1994/09/07  04:18:28  menze
 * OSF modifications
 *
 * Revision 1.17.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.17.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.17.1.2  1994/07/22  20:08:12  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.17.1.1  1994/04/14  21:41:30  menze
 * Uses allocator-based message interface
 *
 * Revision 1.17  1993/12/13  22:48:49  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

/*
 * coordination of ICMP requests and replies
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/icmp.h>
#include <xkern/protocols/icmp/icmp_internal.h>

static void 	echoReqTimeout( Event, void * );
static void	signalWaiter( Sstate *st, int res );

int
icmpSendEchoReq(s, msgLength)
    XObj s;
    int msgLength;
{
  Msg_s msg;
  ICMPEcho echoHdr;
  ICMPHeader stdHdr;
  Pstate *pstate = (Pstate *)s->myprotl->state;
  Sstate *sstate = (Sstate *)s->state;
  mapId key;
  xkern_return_t  xkr;

  if ( msgConstructContig(&msg, xMyPath(s), msgLength, ICMP_STACK_LEN)
      		== XK_FAILURE ) {
      xTraceP0(s, TR_SOFT_ERRORS, "Couldn't alloc request msg");
      return -1;
  }
  xAssert(sstate->timeoutEvent);
  echoHdr.icmp_id = sstate->sessId;
  echoHdr.icmp_seqnum = sstate->seqNum;
  xkr = msgPush(&msg, icmpEchoStore, &echoHdr, sizeof(ICMPEcho), NULL);
  if ( xkr == XK_FAILURE ) {
      msgDestroy(&msg);
      return -1;
  }
  stdHdr.icmp_type = ICMP_ECHO_REQ;
  stdHdr.icmp_code = 0;
  xkr = msgPush(&msg, icmpHdrStore, &stdHdr, sizeof(ICMPHeader), &msg);
  if ( xkr == XK_FAILURE ) {
      msgDestroy(&msg);
      return -1;
  }
  key.id = echoHdr.icmp_id;
  key.seq = echoHdr.icmp_seqnum;
  if ( mapBind(pstate->waitMap, &key, s, &sstate->bind) != XK_SUCCESS ) {
      msgDestroy(&msg);
      return -1;
  }
  xPush(xGetDown(s, 0), &msg);
  evSchedule(sstate->timeoutEvent, echoReqTimeout, s, REQ_TIMEOUT * 1000);
  msgDestroy(&msg);
  semWait(&sstate->replySem);
  return sstate->result;
}


void
icmpPopEchoRep(self, msg)
    XObj self;
    Msg msg;
{
  Pstate *pstate;
  Sstate *sstate;
  ICMPEcho echoHdr;
  Sessn s;
  mapId key;
  
  xAssert(xIsProtocol(self));
  xTrace0(icmpp, 3, "ICMP echo reply received");
  if ( msgPop(msg, icmpEchoLoad, &echoHdr, sizeof(ICMPEcho), NULL)
	      	== XK_FAILURE ) {
      xTraceP0(self, TR_ERRORS, "echo reply msgPop failed");
      return;
  }
  pstate = (Pstate *)self->state;
  key.id = echoHdr.icmp_id;
  key.seq = echoHdr.icmp_seqnum;
  xTrace3(icmpp, 4, "id = %d, seq = %d, data len = %d",
	  key.id, key.seq, msgLen(msg));
  if ( mapResolve(pstate->waitMap, &key, &s) == XK_FAILURE ) {
    xTrace1(icmpp, 3, "ICMP echo reply received for nonexistent session %x",
	    echoHdr.icmp_id);
    return;
  }
  sstate = (Sstate *)s->state;
  evCancel(sstate->timeoutEvent);
  mapRemoveBinding(pstate->waitMap, sstate->bind);
  signalWaiter(sstate, 0);
}


static void
echoReqTimeout(ev, arg)
    Event	ev;
    void 	*arg;
{
  XObj	s = (XObj)arg;    
  Sstate *sstate;
  Pstate *pstate;

  xTrace1(icmpp, 3, "ICMP Request timeout for session %x", s);
  xAssert(xIsSession(s));
  sstate = (Sstate *)s->state;
  pstate = (Pstate *)s->myprotl->state;
  mapRemoveBinding(pstate->waitMap, sstate->bind);
  signalWaiter(sstate, -1);
}


static void
signalWaiter(st, res)
    Sstate *st;
    int res;
{
  st->result = res;
  st->seqNum++;
  semSignal(&st->replySem);
}
