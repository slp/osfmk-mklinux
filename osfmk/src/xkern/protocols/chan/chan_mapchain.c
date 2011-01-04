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
 * chan_mapchain.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * chan_mapchain.c,v
 * Revision 1.8.2.3.1.2  1994/09/07  04:18:42  menze
 * OSF modifications
 *
 * Revision 1.8.2.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.8.2.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.8  1993/12/13  22:42:34  menze
 * Modifications from UMass:
 *
 *   [ 93/09/21          nahum ]
 *   Got rid of casts in mapUnbind, mapResolve calls
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/chan/chan_internal.h>

static int		applyToChanMap( void *, void *, void * );
static int		applyToHlpMap( void *, void *, void * );


typedef struct {
    int			count;
    MapChainForEachFun	f;
} ChanMapS;


/* 
 * The number of objects at the end of the map chain (i.e., the number
 * of objects to which the functions is applied) is returned.
 */
int
chanMapChainApply( map, peer, f )
    Map			map;
    IPhost		*peer;
    MapChainForEachFun	f;
{
    Map		hlpMap;
    ChanMapS	mapStr;

    mapStr.count = 0;
    mapStr.f = f;
    if ( mapResolve(map, peer, &hlpMap) == XK_FAILURE ) {
	xTrace1(chanp, TR_MAJOR_EVENTS,
		"No map for peer %s exists", ipHostStr(peer));
    } else {
	xTrace1(chanp, TR_MAJOR_EVENTS, "Applying function for peer %s",
		ipHostStr(peer));
	mapForEach(hlpMap, applyToHlpMap, &mapStr);
	xTrace1(chanp, TR_MAJOR_EVENTS,
		"chanMapChainApply -- %d idle objects were processed",
		mapStr.count);
    }
    return mapStr.count;
}


/* 
 * This function must not block
 */
static int
applyToChanMap( key, val, arg )
    void	*key, *val, *arg;
{
    /* 
     * key == chan, val == XObj, arg == mapStr
     */
    ChanMapS	*mapStr = (ChanMapS *)arg;
    int		retVal;

    xAssert(key);
    xAssert(val);
    xAssert(arg);
    xTrace2(chanp, TR_MORE_EVENTS,
	    "applyToChanMap: key(chan) == %d, val(seq) == %d",
	    *(int *)key, val);
    retVal = mapStr->f(key, val);
    mapStr->count++;
    return retVal;
}


/* 
 * This function must not block
 */
static int
applyToHlpMap( key, val, arg )
    void	*key, *val, *arg;
{
    ChanMapS	*mapStr = (ChanMapS *)arg;
    /* 
     * key == hlpNum, val == chanMap
     */
    xAssert(key);
    xAssert(val);
    xAssert(arg);
    xTrace2(chanp, TR_MORE_EVENTS,
	    "clearHlpMap: key(hlpNum) == %d, val(chanMap) == %x",
	    *(int *)key, val);
    mapForEach((Map)val, applyToChanMap, (void *)mapStr);
    return MFE_CONTINUE;
}


    
static Map
establishChain(
    Map		m,
    IPhost 	*peer,
    long 	prot,
    Path 	path)
{
    Map		peerMap, hlpMap;
#ifdef XK_DEBUG    
    Map		mapCheck;
#endif

    if ( mapResolve(m, peer, &peerMap) == XK_FAILURE ) {
	if ( ! (peerMap = mapCreate(1, sizeof(long), path)) ) {
	    return 0;
	}
	if ( mapBind(m, peer, peerMap, 0) != XK_SUCCESS ) {
	    mapClose(peerMap);
	    return 0;
	}
	xAssert( mapResolve(m, peer, &mapCheck) == XK_SUCCESS && 
		 mapCheck == peerMap);
    }
    if ( mapResolve(peerMap, &prot, &hlpMap) == XK_FAILURE ) {
	if ( ! (hlpMap = mapCreate(1, sizeof(Channel), path)) ) {
	    return 0;
	}
	if ( mapBind(peerMap, &prot, hlpMap, 0) != XK_SUCCESS ) {
	    mapClose(hlpMap);
	    return 0;
	}
	xAssert(mapResolve(peerMap, &prot, &mapCheck) == XK_SUCCESS && 
		mapCheck == hlpMap);
    }
    return hlpMap;
}


/* 
 * Binds 'obj' in the idleMap series starting at m.  The map series is
 * keyed by { peer --> { hlp --> { chan --> obj } } }.  Maps are
 * created as necessary.
 */
xkern_return_t
chanMapChainAddObject( 
    void	*obj,
    Map		m,
    IPhost	*peer,
    long	prot,
    int		c,
    Path	path)
{
    Map		hlpMap;
    Channel	chan;

    if ( ! (hlpMap = establishChain(m, peer, prot, path)) ) {
	return XK_FAILURE;
    }
    chan = c;
    return mapBind(hlpMap, &chan, obj, 0) == XK_SUCCESS ?
      		XK_SUCCESS : XK_FAILURE;
}


Map
chanMapChainFollow( m, h, prot )
    Map		m;
    IPhost	*h;
    long	prot;
{
    Map		peerMap, hlpMap;

    if ( mapResolve(m, h, &peerMap) == XK_FAILURE ||
    	 mapResolve(peerMap, &prot, &hlpMap) == XK_FAILURE ) {
	return 0;
    }
    return hlpMap;
}
    

xkern_return_t
chanMapChainReserve(
    Map		m,
    IPhost 	*h,
    long	prot,
    Path	path,
    int 	num)
{
    if ( num > 0 ) {
	return establishChain(m, h, prot, path) ? XK_SUCCESS : XK_FAILURE;
    }
    return XK_FAILURE;
    /* 
     * XXX -- Still need to reserve a binding here in the map, when
     * maps support such reservations.  
     */
}
