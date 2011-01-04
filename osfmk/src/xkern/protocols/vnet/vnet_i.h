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
 * veth_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:42:58 $
 */


#ifndef vnet_h
#include <xkern/include/prot/vnet.h>
#endif

typedef struct {
    IPhost	host;
    IPhost	mask;
    XObj	llp;
    XObj	arp;
    int		subnet;	 /* does this interface use subnetting? */
} Interface;

#define VNET_MAX_INTERFACES 	10
#define VNET_MAX_PARTS	 	5

typedef struct {
    Map		activeMap;
    Map 	passiveMap;
    Map		bcastMap;
    Interface	ifc[VNET_MAX_INTERFACES];
    XObj	llpList[VNET_MAX_INTERFACES+1];
    u_int	numIfc;
} PState;

typedef struct {
    struct {
	u_int isBcast 		: 1;
	u_int rewriteHosts 	: 1;
    } flags;
    struct {
	Interface	*ifc;
	int		active;
    } ifcs[VNET_MAX_INTERFACES];
    	/* 
     	 * There may be more than one interface for a broadcast session
     	 */
    Map			map;	/* Which map am I bound in */
} SState;

typedef	XObj	PassiveKey;
typedef XObj	ActiveKey;

#define VNET_BCAST_MAP_SZ	11
#define VNET_ACTIVE_MAP_SZ	23
#define VNET_PASSIVE_MAP_SZ	11

