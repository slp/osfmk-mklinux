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
 * netmask.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * netmask.c,v
 * Revision 1.11.1.3.1.2  1994/09/07  04:18:16  menze
 * OSF modifications
 *
 * Revision 1.11.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.11.1.2  1994/09/01  18:49:59  menze
 * Meta-data allocations now use Allocators and Paths
 * Subsystem initialization functions can fail
 *
 * Revision 1.11  1994/01/27  21:54:21  menze
 *   [ 1994/01/13          menze ]
 *   Now uses library routines for rom options
 *
 */

/* 
 * Maintains a table of network masks.  
 */

#include <xkern/include/domain.h>
#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip_host.h>
#include <xkern/include/netmask.h>
#include <xkern/include/romopt.h>

int	tracenetmask;

IPhost	IP_LOCAL_BCAST = { 255, 255, 255, 255 };

#define CLASSA(ad) (~((ad).a) & 0x80)
#define CLASSB(ad) (((ad).a & 0x80) && (~((ad).a) & 0x40))
#define CLASSC(ad) (((ad).a & 0x80) && ((ad).a & 0x40) && (~((ad).a) & 0x20))
#define CLASSD(ad) (((ad).a & 0x80) && ((ad).a & 0x40) && \
				((ad).a & 0x20) && (~((ad).a) & 0x10))


static IPhost	classDmask = { 0xff,0xff,0xff,0xff };
static IPhost	classCmask = { 0xff,0xff,0xff,0 };
static IPhost	classBmask = { 0xff,0xff,0,0 };
static IPhost	classAmask = { 0xff,0,0,0 };
static IPhost	ipNull = { 0, 0, 0, 0 };

#define MAX_NET_MASKS 50
#define IPNETMAPSIZE  17  /* should 2 * max number of masks in use */

static	Map	netMaskMap;

static int		bcastHost( IPhost *, IPhost * );
static xkern_return_t	netMaskOpt( char **, int, int, void * );

static RomOpt romOptions[] = {
    { "", 3, netMaskOpt }
};


xkern_return_t
netMaskAdd(
	IPhost *iph,
	IPhost *newMask)
{
    IPhost		net, defMask, *mask;

    xAssert(netMaskMap);
    netMaskDefault(&defMask, iph);
    IP_AND(net, *iph, defMask);
    xTrace2(netmask, TR_EVENTS, "netmask adding mask %s for net %s",
	    ipHostStr(newMask), ipHostStr(&net));
    if (mapResolve(netMaskMap, &net, &mask) == XK_SUCCESS) {
	xTrace1(netmask, TR_EVENTS,
		"netmask add is overriding previous mask %s",
		ipHostStr(mask));
	allocFree(mask);
	mapUnbind(netMaskMap, &net);
    }
    if ( ! (mask = xSysAlloc(sizeof(IPhost))) ) {
	return XK_FAILURE;
    }
    *mask = *newMask;
    if ( mapBind(netMaskMap, &net, mask, 0) != XK_SUCCESS ) {
	allocFree(mask);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xkern_return_t
netMaskOpt( str, nFields, ln, arg )
    char	**str;
    int		nFields;
    int		ln;
    void	*arg;
{
    IPhost	net, mask;

    if ( str2ipHost(&net, str[1]) == XK_FAILURE || 
	 str2ipHost(&mask, str[2]) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    if ( netMaskAdd(&net, &mask) ) {
	sprintf(errBuf,
		"netmask: error adding initial mask %s -> %s to map",
		ipHostStr(&net), ipHostStr(&mask));
	xError(errBuf);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


xkern_return_t
netMaskInit()
{
    netMaskMap = mapCreate(IPNETMAPSIZE, sizeof(IPhost), xkSystemPath);
    if ( netMaskMap == 0 ) {
	xTrace0(netmask, TR_ERRORS, "map creation error");
	return XK_FAILURE;
    }
    findRomOpts("netmask", romOptions, sizeof(romOptions)/sizeof(RomOpt), 0);
    return XK_SUCCESS;
}


void
netMaskDefault( mask, host )
    IPhost	*mask, *host;
{
    /* 
     * Use default network mask
     */
    if (CLASSD(*host)) {
    	*mask = classDmask;
    } else if (CLASSC(*host)) {
	*mask = classCmask;
    } else if (CLASSB(*host)) {
	*mask = classBmask;
    } else { /* CLASS A */
	*mask = classAmask;
    }
}


void
netMaskFind(
	IPhost	*mask, 
	IPhost  *host)
{
    IPhost	*mp;
    IPhost	defMask, net;
    
    xAssert(netMaskMap);
    netMaskDefault(&defMask, host);
    IP_AND(net, *host, defMask);
    if (mapResolve(netMaskMap, &net, &mp) == XK_FAILURE) {
	*mask = defMask;
    } else {	
	*mask = *mp;
    }
}


static int
bcastHost(
	IPhost	*h, 
	IPhost  *m)
{
    IPhost	mComp;
    IPhost	res;

    IP_COMP(mComp, *m);
    /* 
     * Look for a host component that is all zeroes
     */
    IP_AND(res, *h, mComp);
    if (IP_EQUAL(res, ipNull)) {
	return 1;
    }
    /* 
     * Look for a host component that is all ones
     */
    IP_OR(res, *h, *m);
    if (IP_EQUAL(res, IP_LOCAL_BCAST)) {
	return 1;
    }
    return 0;
}

int
netMaskIsMulticast(
	IPhost	*h)
{
	return CLASSD(*h);
}

int
netMaskIsBroadcast(
	IPhost	*h)
{
    IPhost	m;

    netMaskFind(&m, h);
    if (bcastHost(h, &m)) {
	return 1;
    }
    /* 
     * See if this is a broadcast address using the default mask
     * (i.e., the original mask was for a subnet)
     */
    netMaskDefault(&m, h);
    if (bcastHost(h, &m)) {
	return 1;
    }
    /* 
     * See if the original address is all ones
     */
    if (IP_EQUAL(*h, IP_LOCAL_BCAST)) {
	return 1;
    }
    /* 
     * Address is not broadcast
     */
    return 0;
}


/* 
 * Are h1 and h2 on the same subnet?
 */
int
netMaskSubnetsEqual(
	IPhost	*h1, 
	IPhost  *h2)
{
    IPhost	m1, m2;

    netMaskFind(&m1, h1);
    netMaskFind(&m2, h2);
    if (!IP_EQUAL(m1, m2)) {
	return 0; 
    }
    IP_AND(m1, *h1, m1);
    IP_AND(m2, *h2, m2);
    return IP_EQUAL(m1, m2);
}


/* 
 * Are h1 and h2 on the same net?
 */
int
netMaskNetsEqual(
	IPhost	*h1, 
	IPhost  *h2)
{
    IPhost	m, net1, net2;

    netMaskDefault(&m, h1);
    IP_AND(net1, *h1, m);
    IP_AND(net2, *h2, m);
    return IP_EQUAL(net1, net2);
}
