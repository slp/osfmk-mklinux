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
 * ip_util.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:33:01 $
 */

/* 
 * Internal IP utility functions
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/ip/ip_i.h>


int
ipHostOnLocalNet( ps, host )
    PState	*ps;
    IPhost	*host;
{
    int		res;
    XObj	llp;

    llp = xGetDown(ps->self, 0);
    xAssert(xIsProtocol(llp));
    res = xControl(llp, VNET_HOSTONLOCALNET, (char *)host, sizeof(IPhost));
    if ( res > 0 ) {
	return 1;
    }
    if ( res < 0 ) {
	xTrace0(ipp, TR_ERRORS, "ip could not do HOSTONLOCALNET call on llp");
    }
    return 0;
}



/*
 * ismy_addr:  is this IP address one which should reach me
 * (my address or broadcast)
 */
int
ipIsMyAddr( self, h )
    XObj	self;
    IPhost	*h;
{
    XObj	llp = xGetDown(self, 0);
    int		r;
    
    r = xControl(llp, VNET_ISMYADDR, (char *)h, sizeof(IPhost));
    if ( r > 0 ) {
	return 1;
    }
    if ( r < 0 ) {
	xError("ip couldn't do VNET_ISMYADDR on llp");
    }
    return 0;
}



