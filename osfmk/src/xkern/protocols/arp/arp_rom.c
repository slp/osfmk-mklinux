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
 * arp_rom.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.2
 * Date: 1993/11/15 20:51:01
 */

/*
 * initialize table from ROM entries
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/romopt.h>
#include <xkern/include/prot/arp.h>
#include <xkern/include/prot/sim.h>
#include <xkern/protocols/arp/arp_i.h>
#include <xkern/protocols/arp/arp_table.h>


static xkern_return_t loadEntry( XObj, char **, int, int, void * );

static XObjRomOpt	arpOpts[] =
{
    { "", 3, loadEntry }
};
 

static xkern_return_t
loadEntry(
    XObj	self,
    char	**str,
    int		nFields,
    int		line,
    void	*arg)
{
    ETHhost	ethHost;
    IPhost	ipHost;
    PState	*ps = (PState *)self->state;
    int		nextField;

    if ( str2ipHost(&ipHost, str[1]) == XK_FAILURE )
    	return XK_FAILURE;
    if ( str2ethHost(&ethHost, str[2]) == XK_SUCCESS ) {
	nextField = 3;
    } else {
	/* 
	 * Second field isn't an ethernet address.  See if it's
	 * one of the alternate ways of specifying a hardware address
	 * (there is currently only one alternate)
	 */
	{
	    /* 
	     * Look for SIM address, an IP-host/UDP-port pair
	     */
	    SimAddrBuf	buf;

	    if ( nFields < 4 ) return XK_FAILURE;
	    if ( str2ipHost(&buf.ipHost, str[2]) == XK_FAILURE ) {
		return XK_FAILURE;
	    }
	    if ( ! (buf.udpPort = atoi(str[3])) ) {
		return XK_FAILURE;
	    }
	    if ( xControl(xGetDown(self, 0), SIM_SOCK2ADDR,
			  (char *)&buf, sizeof(buf)) < 0 ) {
		xTraceP0(self, TR_ERRORS, "llp couldn't translate rom entry");
		return XK_FAILURE;
	    }
	    bcopy((char *)&buf.ethHost, (char *)&ethHost, sizeof(ETHhost));
	    nextField = 4;
	}
    }
    arpSaveBinding( ps->tbl, &ipHost, &ethHost );
    xTraceP1(self, TR_MAJOR_EVENTS, "loaded (%s) from rom file",
	    ipHostStr(&ipHost));
    xTraceP1(self, TR_MAJOR_EVENTS, "corresponding eth address: %s",
	    ethHostStr(&ethHost));

    if ( nFields > nextField ) {
	if ( ! strcmp(str[nextField], "lock") ) {
	    arpLock(ps->tbl, &ipHost);
	} else {
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


void
arpPlatformInit(
    XObj	self)
{
    /*
     * Check the rom file for arp initialization 
     */
    findXObjRomOpts(self, arpOpts, sizeof(arpOpts)/sizeof(XObjRomOpt), 0);
}
