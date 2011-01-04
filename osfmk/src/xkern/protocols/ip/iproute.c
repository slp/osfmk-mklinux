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
 * iproute.c,v
 * 
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * iproute.c,v
 * Revision 1.26.2.2.1.2  1994/09/07  04:18:32  menze
 * OSF modifications
 *
 * Revision 1.26.2.2  1994/09/01  20:05:09  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.26  1994/04/20  22:51:29  davidm
 * (rt_init): Added initialization of "tbl->defrt."
 *
 * Revision 1.25  1993/12/16  01:30:22  menze
 * Fixed function parameters to compile with strict ANSI restrictions
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/ip/ip_i.h>
#include <xkern/include/prot/arp.h>
#include <xkern/protocols/ip/route.h>
#include <xkern/protocols/ip/route_i.h>

static route *	rt_alloc( RouteTable * );
static void	rt_free( RouteTable *, route * );
static int	rt_hash(IPhost *);
static route *	rt_new(RouteTable *, IPhost *, IPhost *, IPhost *, int, int);
static void	rt_timer( Event, void * );

static IPhost	ipNull = { 0, 0, 0, 0 };


xkern_return_t
rt_init( XObj self, IPhost *defGw )
{
    PState	*ps = (PState *)self->state;
    RouteTable	*tbl = &ps->rtTbl;
    Path	path = self->path;
    Event	ev;
    
    xTrace0(ipp, TR_GROSS_EVENTS, "IP rt_init()");
    tbl->valid = TRUE;
    tbl->defrt = 0;
    tbl->arr = (route **)pathAlloc(path, ROUTETABLESIZE * sizeof(route *));
    tbl->path = path;
    if ( ! tbl->arr ) {
	xTraceP0(self, TR_ERRORS, "allocation failure");
	return XK_FAILURE;
    }
    bzero((char *)tbl->arr, ROUTETABLESIZE * sizeof(route *));
    tbl->bpoolsize = BPSIZE;
    if ( ! (ev = evAlloc(path)) ) {
	pathFree(tbl->arr);
	xTraceP0(self, TR_ERRORS, "allocation failure");
	return XK_FAILURE;
    }
    if ( IP_EQUAL(*defGw, ipNull) ) {
	xTrace0(ipp, TR_GROSS_EVENTS,
		"IP routing -- default routing disabled");
    } else {
	if ( rt_add_def(ps, defGw) ) {
	    return XK_FAILURE;
	}
    }
    evSchedule(ev, rt_timer, tbl, RTTABLEUPDATE * 1000);
    xTrace0(ipp, TR_GROSS_EVENTS, "IP rt_init() done");
    return XK_SUCCESS;
}


static route *
rt_alloc( tbl )
    RouteTable	*tbl;
{
    route *r;

    if ( tbl->bpoolsize == 0 ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ip rt_alloc ... route table is full");
	return 0;
    }
    if ( ! (r = pathAlloc(tbl->path, sizeof(route))) ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ip rt_alloc ... allocation failure");
	return 0;
    }
    tbl->bpoolsize--;
    return r;
}


static route *
rt_new(tbl, net, mask, gw, metric, ttl)
    RouteTable	*tbl;
    IPhost *net;
    IPhost *mask;
    IPhost *gw;
    int metric;
    int ttl;
{
    route *ptr;
    
    ptr = rt_alloc(tbl);
    if ( ptr ) {
	ptr->net = *net;
	ptr->mask = *mask;
	ptr->gw = *gw;
	ptr->metric = metric;
	ptr->ttl = ttl;
	ptr->next = NULL;
    }
    return ptr;
}


xkern_return_t
rt_add_def( ps, gw )
    PState *ps;
    IPhost *gw;
{
    RouteTable	*tbl = &ps->rtTbl;

    xTrace1(ipp, TR_MAJOR_EVENTS,
	    "IP default route changes.  New GW: %s", ipHostStr(gw));
    if ( ! IP_EQUAL(*gw, ipNull) && ! ipHostOnLocalNet(ps, gw) ) {
	xTrace1(ipp, TR_SOFT_ERRORS,
		"ip: rt_add_def couldn't find interface for gw %s",
		ipHostStr(gw));
	return XK_FAILURE;
    }
    if ( tbl->defrt ) {
	rt_free(tbl, tbl->defrt);
    }
    if ( ! IP_EQUAL(*gw, ipNull) ) {
	tbl->defrt = rt_new( tbl, &ipNull, &ipNull, gw, 1,  0 );
	if ( tbl->defrt == 0 ) {
	    return XK_FAILURE;
	}
	/*
	 * Re-open the connection for every remote host not connected to the
	 * local net.  This is certainly overkill, but:
	 * 		-- it is not incorrect
	 *		-- the default route shouldn't change often
	 *		-- keeping track of which hosts use the default route
	 *		   is a pain.
	 */
	ipRouteChanged(ps, tbl->defrt, ipRemoteNet);
    }
    return XK_SUCCESS;
}
  

xkern_return_t
rt_add( pstate, net, mask, gw, metric, ttl )
    PState *pstate;
    IPhost *net;
    IPhost *mask;
    IPhost *gw;
    int metric;
    int ttl;
{
    route 	*ptr, *srt, *prev;
    u_char  	isdup;
    int  	j;
    u_long 	hashvalue;
    RouteTable	*tbl = &pstate->rtTbl;
    
    ptr = rt_new(tbl, net, mask, gw, metric, ttl);
    if ( ptr == 0 ) {
	return XK_FAILURE;
    }
    
    /* compute sort key - number of set bits in mask 
       so that route are sorted : host, subnet, net */
    for (j = 0; j < 8; j++)
      ptr->key += ((mask->a >> j) & 1) + 
	((mask->b >> j) & 1) +
	  ((mask->c >> j) & 1) +
	    ((mask->d >> j) & 1) ;
    
    prev = NULL;
    hashvalue = rt_hash(net);
    xTrace1(ipp, TR_MORE_EVENTS, "IP rt_add : hash value is %d", hashvalue);
    isdup = FALSE;
    for ( srt = tbl->arr[hashvalue]; srt; srt = srt->next ) {
	if ( ptr->key > srt->key )
	  break;
	if ( IP_EQUAL(srt->net, ptr->net) && 
	    IP_EQUAL(srt->mask, ptr->mask) ) {
	    isdup = TRUE;
	    break;
	}
	prev = srt;
    }
    if ( isdup ) {
	route *tmptr;
	if ( IP_EQUAL(srt->gw, ptr->gw) ) {
	    /* update existing route */
	    xTrace0(ipp, TR_MORE_EVENTS, "IP rt_add: updating existing route");
	    srt->metric = metric;
	    srt->ttl = ttl;
	    rt_free(tbl, ptr);
	    return XK_SUCCESS;
	}
	/* otherwise someone else has a route there */
	/*
	 * negative metric indicates unconditional override
	 */
	if ( ptr->metric > 0 && srt->metric <= ptr->metric ) {
	    /* it's no better, just drop it */
	    xTrace0(ipp, TR_MORE_EVENTS,
		    "IP rt_add : dropping duplicate route with greater metric");
	    rt_free(tbl, ptr);
	    return XK_SUCCESS;
	}
	xTrace0(ipp, TR_MORE_EVENTS,
		"IP rt_add: new duplicate route better, deleting old");
	tmptr = srt;
	srt = srt->next;
	rt_free(tbl, tmptr);
    } else {
	xTrace0(ipp, TR_MORE_EVENTS, "IP rt_add: adding fresh route");
    }
    ipRouteChanged(pstate, ptr, ipSameNet);
    ptr->next = srt;
    if ( prev ) {
	prev->next = ptr;
    } else {
	tbl->arr[hashvalue] = ptr;
    }
    return XK_SUCCESS; 
} /* rt_add */


xkern_return_t
rt_get( tbl, dest, req )
    RouteTable	*tbl;
    IPhost 	*dest;
    route	*req;
{
    route *ptr;
    u_long hashvalue;
    u_long sum;
    IPhost fdest;
    
    hashvalue = rt_hash(dest);
    xTrace1(ipp, TR_MORE_EVENTS, "IP rt_get: hash value is %d",hashvalue);
    for( ptr = tbl->arr[hashvalue]; ptr; ptr = ptr->next ) {
	if ( ptr->ttl == 0 )
	  continue;
	sum =  *(long *)dest & *(long *)&(ptr->mask);
	fdest = *(IPhost *) &sum;
	if ( IP_EQUAL(fdest,ptr->net) )
	  break;
    }
    if ( ptr == 0 ) {
	ptr = tbl->defrt;
    }
    if ( ptr ) {
	*req = *ptr;
	xTrace3(ipp, TR_MORE_EVENTS,
		"IP rt_get : Mapped host %s to net %s, gw %s",
		ipHostStr(dest), ipHostStr(&ptr->net), ipHostStr(&ptr->gw));
	return XK_SUCCESS;
    } else {
	xTrace1(ipp, TR_SOFT_ERRORS,
		"IP rt_get: Could not find route for host %s!",
		ipHostStr(dest));
	return XK_FAILURE;
    }
}


void
rt_delete( tbl, net, mask )
    RouteTable	*tbl;
    IPhost 	*net, *mask;
{
    route *ptr, *prev;
    u_long  hashvalue; 
    
    hashvalue = rt_hash(net);
    prev = NULL;
    for ( ptr = tbl->arr[hashvalue]; ptr; ptr = ptr->next ) {
	if ( IP_EQUAL(*net, ptr->net) &&
	    IP_EQUAL(*mask, ptr->mask) )
	  break;
	prev = ptr;
    }
    if ( ptr == NULL ) {
	return;
    }
    if ( prev )
      prev->next = ptr->next;
    else
      tbl->arr[hashvalue] = ptr->next;
    rt_free(tbl, ptr);
    return;
}


/* hash value is sum of net portions of IP address */
static int
rt_hash( net )
     IPhost *net;
{
    IPhost	mask;
    u_long 	hashvalue;
    
    netMaskFind(&mask, net);
    IP_AND(mask, mask, *net);
    hashvalue = mask.a + mask.b + mask.c + mask.d;
    return (hashvalue % ROUTETABLESIZE);
}


static void
rt_free(tbl, rt)
    RouteTable	*tbl;
    route 	*rt;
{
    tbl->bpoolsize++;
    pathFree(rt);
}


static void
rt_timer(ev, arg)
    Event	ev;
    void 	*arg;
{
    RouteTable	*tbl = (RouteTable *)arg;
    route 	*ptr, *prev;
    int 	i;
    
    xTrace0(ipp, TR_EVENTS, "IP rt_timer called");
    for ( i = 0; i < ROUTETABLESIZE; i++) {
	if ( tbl->arr[i] == 0 ) {
	    continue;
	}
	prev = NULL;
	for ( ptr = tbl->arr[i]; ptr; ) {
	    if ( ptr->ttl != IPROUTE_TTL_INFINITE ) {
		ptr->ttl -= RTTABLEDELTA;
	    }
	    if ( ptr->ttl == 0 ) {
		if ( prev ) {
		    prev->next = ptr->next;
		    rt_free(tbl, ptr);
		    ptr = prev->next;
		} else {
		    tbl->arr[i] = ptr->next;
		    rt_free(tbl, ptr);
		    ptr = tbl->arr[i];
		}
		continue;
	    }
	    prev = ptr;
	    ptr = ptr->next;
	}
    }
    /*
     * Reschedule this event
     */
    evSchedule( ev, rt_timer, tbl, RTTABLEUPDATE * 1000 );
}
