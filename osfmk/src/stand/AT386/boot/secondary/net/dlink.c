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
 */
/*
 * MkLinux
 */

/*
 * File : dlink.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains all data link definitions used for Network bootstrap.
 */


#include <secondary/net.h>
#include <secondary/net_debug.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>

struct dlink_st dlink;	/* local data link informations */

u8bits ether_broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int
ether_output(void *addr,
	     unsigned len,
	     u8bits dst[6],
	     u16bits type)
{
	struct frame_ether *ep;
	unsigned i;

#if	defined(ETHERDEBUG)
	NPRINT(("%s (0x%x, 0x%x) [%x:%x:%x:%x:%x:%x 0x%x]\n",
		       "Start ether_output", addr, len, dst[0], dst[1],
		       dst[2], dst[3], dst[4], dst[5], type));
#endif
	ep = (struct frame_ether *)addr;
	for (i = 0; i < 6; i++)
		ep->ether_dst[i] = dst[i];
	for (i = 0; i < 6; i++)
		ep->ether_src[i] = dlink.dl_laddr[i];
	ep->ether_type = htons(type);

#if	defined(NETDEBUG) && defined(ETHERDEBUG)
	if (netdebug) {
		dump_packet("Etherdump",addr,42);
	}
#endif
	return ((*dlink.dl_output)(addr, len + dlink.dl_hlen));
}

void
ether_input(void *addr,
	    unsigned len)
{
	struct frame_ether *ep;

#if	defined(ETHERDEBUG)
	NPRINT(("Start ether_input(0x%x, 0x%x)\n", addr, len));
#endif
	ep = (struct frame_ether *)addr;
	if (len < sizeof (struct frame_ether))
		return;
	ep->ether_type = ntohs(ep->ether_type);
	switch (ep->ether_type) {
	case ETHERNET_TYPE_IP:
		udpip_input(&((char *)addr)[ETHERNET_HLEN],
			    len - ETHERNET_HLEN);
		break;
	case ETHERNET_TYPE_ARP:
		arp_input(&((char *)addr)[ETHERNET_HLEN], len - ETHERNET_HLEN);
		break;
	default:
#if	defined(ETHERDEBUG)
		NPRINT(("Unknown Ethernet Type : %x\n", ep->ether_type));
#endif
		break;
	}
}
