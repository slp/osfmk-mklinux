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
 * select.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * select.c,v
 * Revision 1.13.1.2.1.2  1994/09/07  04:18:46  menze
 * OSF modifications
 *
 * Revision 1.13.1.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 * Revision 1.13.1.1  1994/07/26  23:32:44  menze
 * Uses Path-based message interface and UPI
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/select.h>
#include <xkern/protocols/select/select_i.h>


/* 
 * select -- The select protocol determines the upper
 * protocol ID at open/openenable time by calling relProtNum.
 */

static void		getProtFuncs( XObj );
static XObj		selectOpen( XObj, XObj, XObj, Part, Path );
static xkern_return_t	selectOpenDisable( XObj, XObj, XObj, Part );
static xkern_return_t 	selectOpenEnable( XObj, XObj, XObj, Part );


xkern_return_t
select_init( self )
    XObj self;
{
    xTrace0(selectp, TR_GROSS_EVENTS, "SELECT init");
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTraceP0(self, TR_ERRORS,
		"could not find down protocol -- not initializing");
	return XK_FAILURE;
    }
    getProtFuncs(self);
    return selectCommonInit(self);
}


static XObj
selectOpen( self, hlpRcv, hlpType, p, path )
    XObj	self, hlpRcv, hlpType;
    Part    	p;
    Path	path;
{
    long	hlpNum;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT open");
    if ( (hlpNum = relProtNum(hlpType, self)) == -1 ) {
	return ERR_XOBJ;
    }
    return selectCommonOpen(self, hlpRcv, hlpType, p, hlpNum, path);
}


static xkern_return_t
selectOpenEnable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT openEnable");
    if ( (hlpNum = relProtNum(hlpType, self)) == -1 ) {
	return XK_FAILURE;
    }
    return selectCommonOpenEnable(self, hlpRcv, hlpType, p, hlpNum);
}


static xkern_return_t
selectOpenDisable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT openDisable");
    if ( (hlpNum = relProtNum(hlpType, self)) == -1 ) {
	return XK_FAILURE;
    }
    return selectCommonOpenDisable(self, hlpRcv, hlpType, p, hlpNum);
}



static void
getProtFuncs( p )
    XObj 	p;
{
    p->control = selectControlProtl;
    p->open = selectOpen;
    p->openenable = selectOpenEnable;
    p->opendisable = selectOpenDisable;
    p->calldemux = selectCallDemux;
    p->demux = selectDemux;
}



