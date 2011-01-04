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
 * upi_defaults.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * upi_defaults.c,v
 * Revision 1.13.1.3.1.2  1994/09/07  04:18:19  menze
 * OSF modifications
 *
 * Revision 1.13.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.13.1.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.13  1993/12/13  20:27:49  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

#include <xkern/include/domain.h>
#include <xkern/include/xkernel.h>

#if XK_DEBUG
extern int	traceprotocol;
int		traceprotdef;
#endif /* XK_DEBUG */


#define savePart(pxx, spxx)	\
    bcopy((char *)(pxx), (char *)(spxx), sizeof(Part_s) * partLen(p))

#define restorePart(pxx, spxx)	\
    bcopy((char *)(spxx), (char *)(pxx), sizeof(Part_s) * partLen(p))

/* 
 * Check for a valid participant list
 */
#define partCheck(pxx, name, retval)					\
	{								\
	  if ( ! (pxx) || partLen(pxx) < 1 ) {				\
		xTrace1(protdef, TR_ERRORS,				\
			"xDefault %s -- bad participants",		\
			(name));  					\
		return (retval);					\
	  }								\
	}


static int		findHlpEnable( void *, void *, void * );
static xkern_return_t	llpOpenDisable(XObj, XObj, XObj, Part_s *, XObj *, int);

xkern_return_t
defaultOpenEnable( map, hlpRcv, hlpType, key )
    Map		map;
    XObj   	hlpRcv, hlpType;
    void	*key;
{
    Enable	*e;
    
    xAssert(map);
    xAssert(key);
    xAssert(xIsXObj(hlpRcv));
    xAssert(xIsXObj(hlpType));
    if ( mapResolve(map, key, &e) == XK_FAILURE ) {
	xTrace0(protdef, TR_MORE_EVENTS,
		"openenable -- creating new enable obj");
	if ( (e = allocGet(map->alloc, sizeof(Enable))) == 0 ) {
	    return XK_FAILURE;
	}
	e->rcnt = 1;
	e->hlpRcv = hlpRcv;
	e->hlpType = hlpType;
	if ( mapBind(map, key, e, &e->binding) != XK_SUCCESS ) {
	    xTrace0(protdef, TR_SOFT_ERRORS,
		    "openenable -- binding failed!");
	    allocFree(e);
	    return XK_FAILURE;
	}
    } else {
	xTrace0(protdef, TR_MORE_EVENTS, "openenable -- obj exists");
	if ( e->hlpRcv != hlpRcv || e->hlpType != hlpType ) {
	    xTrace0(protdef, TR_SOFT_ERRORS, "openenable -- hlp mismatch");
	    xTrace4(protdef, TR_SOFT_ERRORS, 
		    "(existing rcv: %x  type %x, paramater rcv: %x  type %x",
		    e->hlpRcv, e->hlpType, hlpRcv, hlpType);
	    return XK_FAILURE;
	}
	e->rcnt++;
	xTrace1(protdef, TR_MORE_EVENTS,
		"openenable -- increasing enable obj ref count to %d",
		e->rcnt);
    }
    return XK_SUCCESS;
}


xkern_return_t
defaultOpenDisable( map, hlpRcv, hlpType, key )
    Map		map;
    XObj   	hlpRcv, hlpType;
    void	*key;
{
    Enable	*e;
    
    xAssert(map);
    xAssert(key);
    xAssert(xIsXObj(hlpRcv));
    xAssert(xIsXObj(hlpType));
    if ( mapResolve(map, key, &e) == XK_FAILURE ) {
	xTrace0(protdef, TR_SOFT_ERRORS,
		"opendisable -- no enable obj found");
	return XK_FAILURE;
    }
    if (  e->hlpRcv != hlpRcv || e->hlpType != hlpType ) {
	    xTrace0(protdef, TR_SOFT_ERRORS, "opendisable -- hlp mismatch");
	    xTrace4(protdef, TR_SOFT_ERRORS, 
		    "(existing rcv: %x  type %x, paramater rcv: %x  type %x",
		    e->hlpRcv, e->hlpType, hlpRcv, hlpType);
	return XK_FAILURE;
    }
    e->rcnt--;
    xTrace1(protdef, TR_MORE_EVENTS,
	    "opendisable -- reducing enable obj ref count to %d",
		e->rcnt);
    if ( e->rcnt == 0 ) {
	xTrace0(protdef, TR_MORE_EVENTS,
		"opendisable -- removing enable object");
	mapRemoveBinding(map, e->binding);
	allocFree(e);
    }
    return XK_SUCCESS;
}


typedef struct {
    Enable	*e;
    char	key[MAX_MAP_KEY_SIZE];
    int		keySize;
    XObj	hlpRcv;
} FindStuff;


static int
findHlpEnable( key, val, arg )
    void	*key, *val, *arg;
{
    FindStuff	*fs = (FindStuff *)arg;
    Enable	*e = (Enable *)val;

    if ( e->hlpRcv == fs->hlpRcv ) {
	fs->e = e;
	bcopy(key, fs->key, fs->keySize);
	return FALSE;
    } else {
	return TRUE;
    }
}


xkern_return_t
defaultOpenDisableAll( map, hlpRcv, f )
    Map			map;
    XObj		hlpRcv;
    DisableAllFunc	f;
{
    FindStuff		fs;
    xkern_return_t	xkr;

    xTrace0(protdef, TR_FUNCTIONAL_TRACE, "openDisableAll");
    fs.hlpRcv = hlpRcv;
    fs.keySize = mapKeySize(map);
    for ( ;; ) {
	fs.e = 0;
	/* 
	 * Restrictions against modifying a map during mapForEach make this awkward ... 
	 */
	mapForEach(map, findHlpEnable, &fs);
	if ( fs.e == 0 ) break;
	xTrace2(protdef, TR_ALWAYS,
		"defaultOpenDisableAll removes binding (rcv %s, type %s)",
		fs.e->hlpRcv->fullName, fs.e->hlpType->fullName);
	if ( f ) {
	    f(fs.key, fs.e);
	}
	xkr = mapRemoveBinding(map, fs.e->binding);
	xAssert(xkr == XK_SUCCESS);
	allocFree((char *)fs.e);
    }
    return XK_SUCCESS;
}


xkern_return_t
defaultVirtualOpenEnable( self, map, hlpRcv, hlpType, llp, p )
    Map		map;
    XObj	self, hlpRcv, hlpType, *llp;
    Part	p;
{
    Part_s		*sp;
    int			i;
    xkern_return_t	xkr;
    
    xAssert(map);
    xAssert(xIsXObj(self));
    xAssert(xIsXObj(hlpRcv));
    xAssert(xIsXObj(hlpType));
    xTrace0(protdef, TR_MAJOR_EVENTS, "default virtual open enable");
    partCheck(p, "openEnable", XK_FAILURE);
    if ( defaultOpenEnable(map, hlpRcv, hlpType,
			   (void *)&hlpType) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    if ( ! llp[0] ) {
	return XK_SUCCESS;
    }
    if ( llp[1] ) {
	xkr = XK_SUCCESS;
	sp = (Part_s *)pathAlloc(xMyPath(self), (sizeof(Part_s) * partLen(p)));
	if ( sp == 0 ) {
	    return XK_FAILURE;
	}
	savePart(p, sp);
	for ( i=0; llp[i]; i++ ) {
	    restorePart(p, sp);
	    if ( ! xIsProtocol(llp[i]) || 
		xOpenEnable(self, hlpType, llp[i], p) == XK_FAILURE ) {
		xTrace1(protdef, TR_SOFT_ERRORS,
			"could not openEnable llp %s", llp[i]->name);
		llpOpenDisable(self, hlpRcv, hlpType, sp, llp, i);
		xkr = XK_FAILURE;
		break;
	    }
	}
	allocFree(sp);
	return xkr;
    } else {
	return xOpenEnable(self, hlpType, llp[0], p);
    }
}


static xkern_return_t
llpOpenDisable( self, hlpRcv, hlpType, p, llp, i )
    XObj	self, hlpRcv, hlpType;
    XObj	*llp;
    Part	p;
    int		i;
{
    Part_s		*sp;
    xkern_return_t	rv = XK_SUCCESS;

    sp = (Part_s *)pathAlloc(self->path, sizeof(Part_s) * partLen(p));
    if ( sp == 0 ) {
	return XK_FAILURE;
    }
    savePart(p, sp);
    for ( i--; i >= 0; i-- ) {
	restorePart(p, sp);
	if ( ! xIsProtocol(llp[i]) || 
		xOpenDisable(hlpRcv, hlpType, llp[i], p) == XK_FAILURE ) {
	    xTrace1(protdef, TR_SOFT_ERRORS,
		    "llpOpenDisable could not openDisable llp %s",
		    llp[i]->name);
	    rv = XK_FAILURE;
	}
    }
    allocFree((char *)sp);
    return rv;
}


xkern_return_t
defaultVirtualOpenDisable( self, map, hlpRcv, hlpType, llp, p )
    Map		map;
    XObj 	self, hlpRcv, hlpType, *llp;
    Part 	p;
{
    int	i;

    xAssert(map);
    xAssert(xIsXObj(self));
    xAssert(xIsXObj(hlpRcv));
    xAssert(xIsXObj(hlpType));
    xTrace0(protdef, TR_MAJOR_EVENTS, "default virtual open disable");
    if ( defaultOpenDisable(map, hlpRcv, hlpType,
			    (void *)&hlpType) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    for ( i=0; llp[i]; i++ );
    return llpOpenDisable(self, hlpRcv, hlpType, p, llp, i);
}

xkern_return_t
defaultVirtualOpenDisableAll( self, map, hlpRcv, llp )
    Map		map;
    XObj 	self, hlpRcv, *llp;
{
    int ii;

    xAssert(map);
    xAssert(xIsXObj(self));
    xAssert(xIsXObj(hlpRcv));
    xTrace0(protdef, TR_MAJOR_EVENTS, "default virtual open disable all");

    if (defaultOpenDisableAll(map, hlpRcv, 0) == XK_FAILURE)
	return XK_FAILURE;

    for ( ii = 0; llp[ii]; ii++ );
    for ( ii--; ii >= 0; ii-- ) {
	xOpenDisableAll(llp[ii], hlpRcv);
    }
    return XK_SUCCESS;
}
