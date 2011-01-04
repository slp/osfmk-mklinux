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
 * sessn_gc.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * sessn_gc.c,v
 * Revision 1.13.1.2.1.2  1994/09/07  04:18:18  menze
 * OSF modifications
 *
 * Revision 1.13.1.2  1994/09/01  04:09:07  menze
 * initSessionCollector returns the event
 * Meta-data allocations now use Allocators and Paths
 *
 */

/*
 * This garbage collector collects idle sessions.  See gc.h for the
 * interface.
 */

#include <xkern/include/domain.h>
#include <xkern/include/xkernel.h>
#include <xkern/include/gc.h>

#if XK_DEBUG
int	tracesessngc;
#endif /* XK_DEBUG */

typedef struct {
    Map		map;
    u_int	interval;
    void	(*destroy)(XObj s);
    char *	msg;
} CollectInfo;

static int	markIdle( void *, void *, void * );
static void	sessnCollect( Event, void * );


static int
markIdle(
    void *key,
    void *value,
    void *arg)
{
    XObj	s = (XObj)value;
    CollectInfo	*c = (CollectInfo *)arg;

    if ( s->rcnt == 0 ) {
	if ( s->idle ) {
	    xTrace2(sessngc, 5, "%s sessn GC closing %x", c->msg, s);
	    c->destroy( s );
	} else {
	    xTrace2(sessngc, 5, "%s sessn GC marking %x idle", c->msg, s);
	    s->idle = TRUE;
	}
    } else {
	xTrace3(sessngc, 7, "%s session GC: %x rcnt %d is not idle",
		c->msg, s, s->rcnt);
    }
    return MFE_CONTINUE;
}


static void
sessnCollect(
    Event	ev,
    void 	*arg)
{
    CollectInfo	*c = (CollectInfo *)arg;

    xTrace1(sessngc, 3, "session garbage collector (%s)", c->msg);
    mapForEach(c->map, markIdle, c);
    evSchedule( ev, sessnCollect, c, c->interval );
    xTrace1(sessngc, 5, "%s sessn GC exits", c->msg);
}


Event
initSessionCollector(
    Map		m,
    int		interval,
    void	(*destructor)(XObj s),
    Path	p,
    char	*msg)
{
    CollectInfo *c;
    Event	ev;

    xTrace2(sessngc, 3,
	    "session garbage collector initialized for map %x (%s)",
	    m, msg ? msg : "");
    if ( ! (c = pathAlloc(p, sizeof(CollectInfo))) ) {
	return 0;
    }
    if ( ! (ev = evAlloc(p)) ) {
	pathFree(c);
	return 0;
    }
    c->map = m;
    c->interval = interval;
    c->destroy = destructor;
    c->msg = msg ? msg : "";
    evSchedule( ev, sessnCollect, c, c->interval );
    return ev;
}

