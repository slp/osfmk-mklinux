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
 * route.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * route.h,v
 * Revision 1.13.1.2.1.2  1994/09/07  04:18:30  menze
 * OSF modifications
 *
 * Revision 1.13.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */

/*
 * Interface to the routing subsystem
 */

#ifndef route_h
#define route_h

#define IPROUTE_TTL_INFINITE	0xffff
#define RTDEFAULTTTL    60*24	  /* default ttl for a route - 1 day */

typedef struct route {
	IPhost	net; 		/* net for this route */
	IPhost  mask;		/* mask for this net */
 	IPhost  gw;		/* gateway for hop */
	int 	metric; 	/* distance metric */
	Sessn 	netSessn; 	/* cached network session */
	u_short key;		/* sort key */
	u_short ttl;		/* time to live */
	struct route *next;	/* next for this hash */
} route;


typedef struct {
	u_char  valid;		/* is table valid */
	route	*defrt;		/* default route */
	u_short bpoolsize;	/* number of unallocated structures remaining */
	route	**arr;
	Path	path;
} RouteTable;

#include <xkern/protocols/ip/ip_i.h>

/* 
 * Initialize the routing tables and set the default router to be the
 * given IP host.  If the IP host is all zeroes or if it is not
 * directly reachable, there will be no default router and rt_init
 * will return XK_FAILURE.
 */
xkern_return_t 	rt_init( XObj, IPhost * );
xkern_return_t 	rt_add( PState *, IPhost *, IPhost *, IPhost *, int, int );
xkern_return_t	rt_add_def( PState *, IPhost * );
xkern_return_t	rt_get( RouteTable *, IPhost *, route * );
void		rt_delete( RouteTable *, IPhost *, IPhost * );

#endif /* ! route_h */

