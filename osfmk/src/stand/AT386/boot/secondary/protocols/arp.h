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
 * File : arp.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains ARP descriptions used for Network bootstrap.
 */

#ifndef __ARP_H__
#define __ARP_H__

#include <secondary/net.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>

struct frame_arp {
	u16bits	arp_hrd;		/* Hardware address space */
	u16bits	arp_pro;		/* Protocol address space */
	u8bits	arp_hln;		/* hardware address length */
	u8bits	arp_pln;		/* Protocol address length */
	u16bits	arp_op;			/* Operation code */
#ifdef	notdef
	u8bits	arp_sha[arp_hln];	/* Sender hardware address */
	u8bits	arp_spa[arp_pln];	/* Sender protocol address */
	u8bits	arp_tha[arp_hln];	/* Target hardware address */
	u8bits	arp_tpa[arp_pln];	/* Target protocol address */
#endif
};

#define	ARP_RETRANSMIT		4000	/* Retransmission timeout (4 secs) */

#define	ARPHRD_ETHERNET		1	/* Ethernet Address Space */

#define	ARPOP_REQUEST		1	/* ARP operation: Request */
#define	ARPOP_REPLY		2	/* ARP operation: Reply */

struct arp {
	u8bits	arp_hrd;		/* Hardware address type */
	u8bits	arp_haddr[DLINK_MAXLEN];/* Hardware address value */
	u8bits	arp_pro;		/* Protocol address type */
	u32bits	arp_paddr;		/* Protocol address (IP) */
};

extern void arp_init(void);
extern void arp_input(void *, unsigned);
extern int arp_request(u32bits);

#endif	/* __ARP_H__ */
