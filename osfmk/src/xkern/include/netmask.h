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
 * netmask.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * netmask.h,v
 * Revision 1.5.1.2.1.2  1994/09/07  04:18:01  menze
 * OSF modifications
 *
 * Revision 1.5.1.2  1994/09/01  04:12:48  menze
 * Subsystem initialization functions can fail
 *
 */

#ifndef netmask_h
#define netmask_h

/* 
 * Public interface to the 'netmask' module
 */


/* 
 * netMaskAdd -- add 'mask' to the table as the netmask for 'net'
 */
xkern_return_t	netMaskAdd(IPhost *net, IPhost *mask);


/* 
 * netMaskInit -- initialize the netmask table.  Must be called before
 * any other netmask routines.
 */
xkern_return_t	netMaskInit(void);

/* 
 * netMaskFind -- Set 'mask' to the netmask for 'host'.  If an
 * appropriate netmask is not in the table, a default netmask based on
 * the address class is used.
 */
void	netMaskFind(IPhost *mask, IPhost *host);


/* 
 * netMaskDefault -- Set 'mask' to the default netmask for 'host'.
 * This differs from the result of netMaskFind only if subnets are
 * being used.
 */
void	netMaskDefault(IPhost *mask, IPhost *host);


/* 
 * netMaskSubnetsEqual -- returns non-zero if the two hosts are on the
 * same subnet
 */
int	netMaskSubnetsEqual(IPhost *, IPhost *);


/* 
 * netMaskNetsEqual -- returns non-zero if the two hosts are on the
 * same net
 */
int	netMaskNetsEqual(IPhost *, IPhost *);


/* 
 * netMaskIsBroadcast -- returns non-zero if the host is a broadcast address. 
 * address for its subnet.  A broadcast address is defined as:
 *
 *	1) having all ones or all zeroes in its host component 
 *	2) having all ones or all zeroes in the host component using the
 *	   default network mask for that address (this catches
 *	   'full network broadcast addresses' in the presence of subnets.)
 *  or	3) having ones in the entire address
 */
int	netMaskIsBroadcast(IPhost *);

/*
 * netMaskIsMulticast -- returns non-zero if the host is a multicast address.
 *	All multicast addresses are class D.
 */
int	netMaskIsMulticast(IPhost *);

extern IPhost	IP_LOCAL_BCAST;

#endif /* netmask_h */
