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
 * bidctl_timer.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * bidctl_timer.c,v
 * Revision 1.11.1.2.1.2  1994/09/07  04:18:49  menze
 * OSF modifications
 *
 * Revision 1.11.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */

/* 
 * Timer routines.  Active sessions (rcnt > 0) exchange keepalives and
 * inactive cached sessions eventually time out and are removed from
 * the map. 
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/bidctl/bidctl_i.h>

static void	idleCollect( BidctlState *, Map );
static int	clockTick( void *, void *, void * );
static void	bidctlKeepAlive( BidctlState * );

static void
bidctlKeepAlive(
    BidctlState	*bs)
{
    if ( bs->timer && (bs->timer % BIDCTL_KEEPALIVE == 0)
	 && bs->fsmState == BIDCTL_NORMAL_S ) {
	xTrace1(bidctlp, TR_EVENTS,
		"Sending keepalive to %s", ipHostStr(&bs->peerHost));
	bidctlOutput(bs, BIDCTL_NO_QUERY, 0, 0);
    }
}


static void
idleCollect(
    BidctlState	*bs,
    Map		m)
{
    xTrace1(bidctlp, TR_DETAILED, "bidctl idleCollect runs for peer %s",
	    ipHostStr(&bs->peerHost));
    if ( bs->timer >= BIDCTL_IDLE_LIMIT ) {
	xTrace1(bidctlp, TR_EVENTS,
		"Removing state for %s", ipHostStr(&bs->peerHost));
	/* 
	 * It's OK for bidctlDestroy to fail (if it couldn't nuke the event,
	 * for example.)  We'll just pick it up on the next clock iteration. 
	 */
	bidctlDestroy(bs);
    }
}


static int
clockTick(
    void	*key,
    void	*val,
    void	*arg)
{
    BidctlState	*bs = (BidctlState *)val;
    Map		m = (Map)arg;

    bs->timer++;
    xTrace3(bidctlp, TR_DETAILED,
	    "bidctl clockTick runs for peer %s, rcnt %d, timer %d",
	    ipHostStr(&bs->peerHost), bs->rcnt, bs->timer);
    if ( bs->rcnt < 1 ) {
	idleCollect(bs, m);
    } else {
	bidctlKeepAlive(bs);
    }
    return MFE_CONTINUE;
}


/* 
 * bidctlTimer -- scan through the list of bidctlStates:
 *
 *	send keepalive messages to peers of idle states
 */
void
bidctlTimer(
    Event	ev,
    void	*arg)
{
    PState	*ps = (PState *)arg;

    xTrace0(bidctlp, TR_EVENTS, "bidctl timer runs");
    mapForEach(ps->bsMap, clockTick, ps->bsMap);
    evSchedule(ev, bidctlTimer, arg, BIDCTL_TIMER_INTERVAL);
}
