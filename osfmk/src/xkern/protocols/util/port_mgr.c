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
 * port_mgr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * port_mgr.c,v
 * Revision 1.22.2.3.1.2  1994/09/07  04:18:40  menze
 * OSF modifications
 *
 * Revision 1.22.2.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.22.2.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.22  1994/01/08  21:29:39  menze
 *   [ 1994/01/03          menze ]
 *   Uses the protocol-specific trace variable to avoid warnings when > 1
 *   protocols using the port_mgr are linked together
 *
 */

/* 
 * Management of ports
 *
 * This file is designed to be included by another source file which
 * defines these macros:
 *
 *	PORT -- the port type 
 *	MAX_PORT -- maximum allowed port
 *	FIRST_USER_PORT -- the first port which may be handed out through
 *		'getFreePort'
 *	NAME -- token to prepend to the routine names
 *	PROT_NAME -- string of protocol name (for debugging)
 *	TRACE_VAR -- trace variable to use in tracing
 * 
 * NOTE -- this code assumes a port is no larger than an int.
 * 
 */

#include <xkern/include/xkernel.h>


extern int PASTE(trace,TRACE_VAR) ;

#define DUMP(_ps)    xIfTrace(TRACE_VAR, TR_DETAILED) { displayMap(_ps); }


typedef struct {
    int		rcnt;
    long	port;
} PortDesc;


typedef struct {
    Map		portMap;
    unsigned long nextPort;
    Path	path;
} port_state;


static int		displayElem( void *, void *, void * );
static void		displayMap( port_state * );
static xkern_return_t	portBind( port_state *, long, int );
static void		portUnbind( port_state *, PortDesc * );


xkern_return_t
PASTE(NAME,PortMapInit)
( void *pst, Path path )
{
    port_state **ps = (port_state **)pst;
    
    if (!(*ps))  {
	if ( ! (*ps = pathAlloc(path, sizeof (port_state))) ) {
	    return XK_NO_MEMORY;
	}
	bzero((char *)*ps, sizeof(port_state));
	(*ps)->portMap = mapCreate(PORT_MAP_SIZE, sizeof(long), path);
	if ( ! (*ps)->portMap ) {
	    pathFree(*ps);
	    *ps = 0;
	    return XK_NO_MEMORY;
	}
	(*ps)->nextPort = FIRST_USER_PORT;
	(*ps)->path = path;
    }
    return XK_SUCCESS;
}


void
PASTE(NAME,PortMapClose)
  (pst) void *pst;
{
  port_state **ps = (port_state **)pst;

  if (!*ps)  return;
  if ( (*ps)->portMap ) {
     mapClose((*ps)->portMap);
  }
  pathFree((char *)*ps);
}


struct dmargs {
    int	i;
    char		msgBuf[200];
  };

static int
displayElem( key, value, idv )
    void *key;
    void *value;
    void *idv;
{
    PortDesc	*pd = (PortDesc *)value;
    struct dmargs *idx = (struct dmargs *)idv;

    xAssert(pd);
    sprintf(idx->msgBuf, 
	    "Element %d:	  port = %d  rcnt = %d",
	   ++idx->i, pd->port, pd->rcnt);
    xError(idx->msgBuf);
    return MFE_CONTINUE;
}


static void
displayMap(ps)
    port_state *ps;
{
  struct dmargs args;

    args.i = 0;
    sprintf(args.msgBuf, "dump of %s port map:", PROT_NAME);
    xError(args.msgBuf);
    mapForEach(ps->portMap, displayElem, &args);
}



/* 
 * Binds 'port' into the map with the indicated reference count.
 * Returns 0 on a successful bind, 1 if the port could not be bound
 * (indicating that it was already bound.)
 */
static xkern_return_t
portBind( ps, port, rcnt )
    port_state *ps;
    long port;
    int rcnt;
{
    PortDesc	*pd;

    if ( ! (pd = pathAlloc(ps->path, sizeof(PortDesc))) ) {
	return XK_NO_MEMORY;
    }
    pd->rcnt = rcnt;
    pd->port = port;
    if ( mapBind(ps->portMap, &port, pd, 0) != XK_SUCCESS ) {
	pathFree((char *)pd);
	return XK_FAILURE;
    } 
    return XK_SUCCESS;
}


static void
portUnbind( ps, pd )
    port_state *ps;
    PortDesc *pd;
{
    xAssert( pd && pd != (PortDesc *) -1 );
    mapUnbind(ps->portMap, &pd->port);
    pathFree(pd);
}


xkern_return_t
PASTE(NAME,GetFreePort)
  ( pst, port )
    void *pst;
    long *port;
{
    port_state *ps = (port_state *)pst;
    unsigned long firstPort;
    xkern_return_t xkr;

    xAssert(ps->portMap);
    firstPort = ps->nextPort;
    do {
	*port = ps->nextPort;
	if (ps->nextPort >= MAX_PORT) {
	    ps->nextPort = FIRST_USER_PORT;
	} else {
	    ps->nextPort++;
	} /* if */
	xkr = portBind(ps, *port, 1);
	switch( xkr ) {
	  case XK_SUCCESS:
	    /* 
	     * Found a free port
	     */
	    DUMP(ps);
	    return 0;

	  case XK_ALREADY_BOUND:
	    break;

	  default:
	    return xkr;
	}
    } while ( ps->nextPort != firstPort );
    return XK_FAILURE;
}


xkern_return_t
PASTE(NAME,DuplicatePort)
  ( pst, port )
    void *pst;
    long port;
{
    PortDesc		*pd;
    xkern_return_t	res;
    port_state  	*ps = (port_state *)pst;

    xAssert(ps->portMap);
    if ( port > MAX_PORT ) {
	res = XK_FAILURE;
    } else {
	if ( mapResolve(ps->portMap, &port, &pd) == XK_FAILURE ) {
	    res = portBind(ps, port, 1);
	} else {
	    pd->rcnt++;
	    res = XK_SUCCESS;
	}
    }
    DUMP(ps);
    return res;
}


void
PASTE(NAME,ReleasePort)
  ( pst, port )
    void *pst;
    long port;
{
    PortDesc	*pd;
    port_state  *ps = (port_state *)pst;

    xAssert(ps->portMap);
    if ( mapResolve(ps->portMap, &port, &pd) == XK_SUCCESS ) {
	if ( pd->rcnt > 0 ) {
	    if ( --pd->rcnt == 0 ) {
		portUnbind(ps, pd);
	    }
	}
    }
    DUMP(ps);
}
