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
 * multi_select.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * multi_select.c,v
 * Revision 1.6.1.2.1.2  1994/09/07  04:18:46  menze
 * OSF modifications
 *
 * Revision 1.6.1.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 * Revision 1.6.1.1  1994/07/26  22:34:23  menze
 * Uses Path-based message interface and UPI
 *
 */

/* 
 * multi_select -- The multi-select protocol determines the upper
 * protocol ID at open/openenable time by pulling the ID number off
 * the participant stack.
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/mselect.h>
#include <xkern/protocols/select/select_i.h>

static int		extractId( Part, long * );
static void		getProtFuncs( XObj );
static int		mselectControlSessn( XObj, int, char *, int );
static XObj		mselectOpen( XObj, XObj, XObj, Part, Path );
static xkern_return_t	mselectOpenDisable( XObj, XObj, XObj, Part );
static xkern_return_t 	mselectOpenEnable( XObj, XObj, XObj, Part );


xkern_return_t
mselect_init( self )
    XObj self;
{
    xTrace0(selectp, TR_GROSS_EVENTS, "MSELECT init");
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTraceP0(self, TR_ERRORS,
		"could not find down protocol -- not initializing");
	return XK_FAILURE;
    }
    getProtFuncs(self);
    return selectCommonInit(self);
}


static int
extractId( p, id )
    Part	p;
    long	*id;
{
    long	*n;

    if ( ! p || partLen(p) < 1 ) {
	xTrace0(selectp, TR_SOFT_ERROR,
		"select extractId -- bad participant");
	return -1;
    }
    n = (long *)partPop(*p);
    if ( n == 0 || n == (long *)-1 ) {
	xTrace0(selectp, TR_SOFT_ERROR,
		"select extractId -- bad participant stack");
	return -1;
    }
    xTrace1(selectp, TR_MORE_EVENTS, "select extractID got id == %d", *n);
    *id = *n;
    return 0;
}


static XObj
mselectOpen( self, hlpRcv, hlpType, p, path )
    XObj	self, hlpRcv, hlpType;
    Part    	p;
    Path	path;
{
    long	hlpNum;
    XObj	s;

    xTrace0(selectp, TR_MAJOR_EVENTS, "MSELECT open");
    if ( extractId(p, &hlpNum) ) {
	return ERR_XOBJ;
    }
    s = selectCommonOpen(self, hlpRcv, hlpType, p, hlpNum, path);
    if ( xIsSession(s) ) {
	s->control = mselectControlSessn;
    }
    return s;
}


static int
mselectControlSessn(self, op, buf, len)
    XObj	self;
    int 	op, len;
    char	*buf;
{

    switch ( op ) {
      case GETPARTICIPANTS:
	{
	    int		retLen;
	    SState	*state = (SState *)self->state;
	    Part_s	p[2];

	    retLen = xControl(xGetDown(self, 0), op, buf, len);
	    if ( retLen <= 0 ) {
		return -1;
	    }
	    partInternalize(p, buf);
	    partPush(*p, &state->hdr.id, sizeof(long));
	    return (partExternalize(p, buf, &len) == XK_FAILURE) ? -1 : len;
	}

      default:
	return selectCommonControlSessn(self, op, buf, len);
    }
}
  


static xkern_return_t
mselectOpenEnable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "MSELECT openEnable");
    if ( extractId(p, &hlpNum) ) {
	return XK_FAILURE;
    }
    if ( selectCommonOpenEnable(self, hlpRcv, hlpType, p, hlpNum)
		== XK_SUCCESS ) {
	return xOpenEnable(self, self, xGetDown(self, 0), p);
    } else {
	return XK_FAILURE;
    }
}


static xkern_return_t
mselectOpenDisable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "MSELECT openDisable");
    if ( extractId(p, &hlpNum) ) {
	return XK_FAILURE;
    }
    if ( selectCommonOpenDisable(self, hlpRcv, hlpType, p, hlpNum)
		== XK_SUCCESS ) {
	return xOpenDisable(self, self, xGetDown(self, 0), p);
    } else {
	return XK_FAILURE;
    }
}


static void
getProtFuncs( p )
    XObj 	p;
{
    p->control = selectControlProtl;
    p->open = mselectOpen;
    p->openenable = mselectOpenEnable;
    p->opendisable = mselectOpenDisable;
    p->calldemux = selectCallDemux;
    p->demux = selectDemux;
}


