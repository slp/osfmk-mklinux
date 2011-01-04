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
 * vmux.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * vmux.c,v
 * Revision 1.7.1.2.1.2  1994/09/07  04:18:47  menze
 * OSF modifications
 *
 * Revision 1.7.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.7.1.1  1994/07/22  19:59:00  menze
 * Uses Path-based message interface and UPI
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/vmux.h>
#include <xkern/protocols/vmux/vmux_i.h>

#define VMUX_MAX_PARTS		5
    
/* 
 * Check for a valid participant list
 */
#define partCheck(pxx, name, retval)					\
	{								\
	  if ( ! (pxx) || partLen(pxx) < 1 ||				\
	      			(partLen(pxx) > (VMUX_MAX_PARTS)) ) { 	\
		xTrace1(vmuxp, TR_ERRORS,				\
			"VMUX %s -- bad participants",			\
			(name));  					\
		return (retval);					\
	  }								\
	}

#define savePart(pxx, spxx)	\
    bcopy((char *)(pxx), (char *)(spxx), sizeof(Part_s) * partLen(p))

#define restorePart(pxx, spxx)	\
    bcopy((char *)(spxx), (char *)(pxx), sizeof(Part_s) * partLen(p))


#if XK_DEBUG
int tracevmuxp;
#endif /* XK_DEBUG */


static XObj
vmuxOpen(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p,
    Path	path)
{
    Part_s	sp[VMUX_MAX_PARTS];
    XObj	llp, lls;
    int		i;
    
    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open");
    partCheck(p, "vmuxOpen", ERR_XOBJ);
    savePart(p, sp);
    for ( i=0; i < self->numdown; i++ ) {
	restorePart(p, sp);
	xTrace1(vmuxp, TR_MAJOR_EVENTS, "VMUX open trying llp %d", i);
	llp = xGetDown(self, i);
	if ( (lls = xOpen(hlpRcv, hlpType, llp, p, path)) != ERR_XOBJ ) {
	    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open successful");
	    /* 
	     * This session is passing through the open of more than
	     * one protocol, so its reference count is being
	     * artificially increased.  We'll correct for this level
	     * before returning the session.
	     *
	     * Should we be scheduling an event to do an xClose on this
	     * session? 
	     */
	    lls->rcnt--;
	    return lls;
	}
    }
    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open fails");
    return ERR_XOBJ;
}


static xkern_return_t
vmuxOpenEnable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part	p)
{
    PState	*ps = (PState *)self->state;

    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    ps->llpList, p);
}


static xkern_return_t
vmuxOpenDisable(
    XObj	self,
    XObj	hlpRcv,
    XObj	hlpType,
    Part 	p)
{
    PState	*ps = (PState *)self->state;

    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open disable");
    return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
				     ps->llpList, p);
}


static xkern_return_t
vmuxOpenDone(
    XObj	self,
    XObj	lls,
    XObj	llp,
    XObj	hlpType,
    Path	path)
{
    PState	*ps = (PState *)self->state;
    Enable	*e;
    
    xTrace0(vmuxp, 3, "In VMUX OpenDone");
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTrace0(vmuxp, TR_ERRORS,
		"vmuxOpenDone: Couldn't find hlp for new session");
	return XK_FAILURE;
    }
    return xOpenDone(e->hlpRcv, lls, self, path);
}


static int
vmuxControlProtl(
    XObj	self,
    int 	opcode,
    char 	*buf,
    int		len)
{
    /*
     * All opcodes are forwarded to the last protocol
     */
    return xControl(xGetDown(self, self->numdown - 1), opcode, buf, len);
}


xkern_return_t
vmux_init( self )
    XObj self;
{
    int		i;
    PState	*ps;
    Path	path = self->path;

    xTrace0(vmuxp, TR_GROSS_EVENTS, "VMUX init");
    xAssert(xIsProtocol(self));
    if ( ! (ps = pathAlloc(path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    if ( ! (ps->passiveMap = mapCreate(VMUX_MAP_SZ, sizeof(XObj), path)) ) {
	pathFree(ps);
	return XK_NO_MEMORY;
    }
    self->state = ps;
    for ( i=0; i < self->numdown; i++ ) {
	XObj	llp;

	llp = xGetDown(self, i);
	if ( ! xIsProtocol(llp) ) {
	    xTrace1(vmuxp, TR_ERRORS, "VMUX can't find protocol %d", i);
	    xError("VMUX init errors -- not initializing");
	    mapClose(ps->passiveMap);
	    pathFree(ps);
	    return XK_FAILURE;
	} else {
	    xTrace2(vmuxp, TR_GROSS_EVENTS, "VMUX llp %d is %s", i, llp->name);
	}
	ps->llpList[i] = llp;
    }
    ps->llpList[i] = 0;
    self->control = vmuxControlProtl;
    self->open = vmuxOpen;
    self->openenable = vmuxOpenEnable;
    self->opendisable = vmuxOpenDisable;
    self->opendone = vmuxOpenDone;
    return XK_SUCCESS;
}


