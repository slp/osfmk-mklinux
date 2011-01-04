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
 * ip_rom.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.4
 * Date: 1993/02/09 03:29:01
 */

/* 
 * ROM file processing
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/ip/ip_i.h>

static xkern_return_t   gwOpt( XObj, char **, int, int, void * );

static XObjRomOpt	rOpts[] = {
    { "gateway", 3, gwOpt }
};

static xkern_return_t
gwOpt( self, str, nFields, line, arg )
    XObj	self;
    char	**str;
    int		nFields, line;
    void	*arg;
{
    IPhost	iph;

    if ( str2ipHost(&iph, str[2]) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    ipSiteGateway = iph;
    xTraceP1(self, TR_EVENTS, "loaded default GW %s from rom file",
	    ipHostStr(&ipSiteGateway));
    return XK_SUCCESS;
}


void
ipProcessRomFile( self )
    XObj	self;
{
    findXObjRomOpts(self, rOpts, sizeof(rOpts)/sizeof(XObjRomOpt), 0);
}




