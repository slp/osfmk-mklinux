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
 * ip_gc.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * ip_gc.c,v
 * Revision 1.27.1.3.1.2  1994/09/07  04:18:30  menze
 * OSF modifications
 *
 * Revision 1.27.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.27.1.2  1994/07/22  20:08:14  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.27.1.1  1994/04/23  00:42:46  menze
 * Added allocator call-backs
 *
 */

/*
 * When fragments of IP messages are lost, the resources dedicated to
 * the reassembly of those messages remain allocated.  This code scans
 * throught the fragment map looking for such fragments and deallocates
 * their resources.
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/ip/ip_i.h>

/*
 * Trace levels
 */
#define GC1 TR_EVENTS
#define GC2 TR_MORE_EVENTS


static char *	fragIdStr( FragId *, char * );
static void	ipFragCollect( Event, void * );
static int	markFrag( void *, void *, void * );
static int      findAllocMatches( void *key, void *val, void *arg );


Event
scheduleIpFragCollector( PState *ps, Path p )
{
    Event	ev;

    if ( ! (ev = evAlloc(p)) ) {
	return 0;
    }
    evSchedule(ev, ipFragCollect, ps, IP_GC_INTERVAL);
    return ev;
}


#if XK_DEBUG

static char *
fragIdStr(key, buf)
    FragId *key;
    char *buf;
{
    sprintf(buf, "s: %s  d: %s  p: %d  seq: %d",
	    ipHostStr(&key->source), ipHostStr(&key->dest),
	    key->prot, key->seqid);
    return buf;
}

#endif /* XK_DEBUG */   



static int
markFrag(key, value, arg)
    void *key;
    void *value;
    void *arg;
{
    FragList		*list = (FragList *)value;
    PState		*pstate = (PState *)arg;
    xkern_return_t	xkr;
#if XK_DEBUG
    char	buf[80];
#endif /* XK_DEBUG */   

    if ( list->gcMark ) {
	xTrace1(ipp, GC2, "IP GC removing %s", fragIdStr(key, buf));
	xkr = mapRemoveBinding(pstate->fragMap, list->binding);
	if ( xkr != XK_SUCCESS ) {
	    xTrace0(ipp, TR_ERRORS, "IP GC couldn't unbind!");
	}
	ipFreeFragList(list);
    } else {
	xTrace1(ipp, GC2, "IP GC marking  %s", fragIdStr(key, buf));
	list->gcMark = TRUE;
    }
    return MFE_CONTINUE;
}


static void
ipFragCollect(ev, arg)
    Event	ev;
    void 	*arg;
{
    PState	*pstate = (PState *)arg;

    xTrace0(ipp, GC1, "IP fragment garbage collector");
    mapForEach(pstate->fragMap, markFrag, pstate);
    evSchedule(ev, ipFragCollect, pstate, IP_GC_INTERVAL);
    xTrace0(ipp, GC2, "IP GC exits");
}




static int
findAllocMatches( void *key, void *val, void *arg )
{
    FragList	*list = val;
    FragId	*id = key;
    FragInfo	*fi;

    for( fi = list->head.next; fi != 0; fi = fi->next ) {
	if ( fi->type == RFRAG ) {
	    if ( pathGetAlloc(msgGetPath(&fi->u.frag), MSG_ALLOC)
			== (Allocator)arg ) {
		xTrace1(ipp, TR_MORE_EVENTS,
			"ip alloc call-back: removing frag (id %d)", id->seqid);
		ipFreeFragList(list);
		return MFE_CONTINUE | MFE_REMOVE;
	    } else {
		xTrace1(ipp, TR_MORE_EVENTS,
			"ip alloc call-back: allocator mismatch (id %d)",
			id->seqid);
	    }
	    break;
	}
    }
    return MFE_CONTINUE;
}


void
ipAllocCallBack( Allocator a, void *arg )
{
    XObj	self = arg;

    xTraceP0(self, TR_EVENTS, "allocator call-back runs");
    mapForEach(((PState *)self->state)->fragMap, findAllocMatches, a);
}
