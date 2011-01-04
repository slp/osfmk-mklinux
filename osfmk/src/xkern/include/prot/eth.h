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
 * eth.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * eth.h,v
 * Revision 1.16.2.2.1.2  1994/09/07  04:18:08  menze
 * OSF modifications
 *
 * Revision 1.16.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 * Revision 1.16  1994/02/05  00:06:05  menze
 *   [ 1994/01/18          menze ]
 *   No longer includes process.h
 *
 */


#ifndef eth_h
#define eth_h

#include <xkern/include/domain.h>
#include <xkern/include/prot/eth_host.h>


/* Header definitions */

#define ETHHOSTLEN sizeof(ETHhost)

#define ETH_SETPROMISCUOUS	(ETH_CTL*MAXOPS + 0)
#define ETH_REGISTER_ARP	(ETH_CTL*MAXOPS + 1)
#define ETH_DUMP_STATS		(ETH_CTL*MAXOPS + 2)
#define ETH_SET_PRIORITY	(ETH_CTL*MAXOPS + 3)

#define ETH_AD_SZ		sizeof(ETHhost)

#define ETH_ADS_EQUAL(A,B)	((A).high==(B).high	\
			       &&(A).mid ==(B).mid	\
			       &&(A).low ==(B).low)

/* is it a multi cast address? */
#define ETH_ADS_MCAST(A)	((A).high & 0x0100)

#define ZERO_ETH_AD(ad)		{ ad.high = ad.mid = ad.low=0; }

#define	BCAST_ETH_AD	{ 0xffff, 0xffff, 0xffff }

#define	ETH_BCAST_HOST	{ 0xffff, 0xffff, 0xffff }

#define	MAX_ETH_DATA_SZ		1500

#define MAX_IEEE802_3_DATA_SZ	1500

xkern_return_t	eth_init( XObj );

#endif /* eth_h */
