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
 * File : arp.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains ARP definitions used for Network bootstrap.
 */

#include <secondary/net.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>

#define	ARP_BUFLEN(len)	 (sizeof (struct frame_arp) + 2 * ((len) + sizeof (u32bits)))

/* ARP buffer */
u8bits arp_buffer[DLINK_MAXHLEN + ARP_BUFLEN(DLINK_MAXLEN) < DLINK_BUFMIN ?
		  DLINK_BUFMIN : DLINK_MAXHLEN + ARP_BUFLEN(DLINK_MAXLEN)];
u32bits arp_waiting;

void
arp_init()
{
	arp_waiting = 0;
}

static void
arp_output()
{
	struct frame_arp *ap;
	u8bits *p;
	unsigned i;

#ifdef	ARPDEBUG
	PPRINT(("Start arp_output()\n"));
#endif
	ap = (struct frame_arp *)&arp_buffer[dlink.dl_hlen];
	ap->arp_hrd = htons(dlink.dl_type);
	ap->arp_pro = htons(ETHERNET_TYPE_IP);
	ap->arp_hln = dlink.dl_len;
	ap->arp_pln = sizeof (u32bits);
	ap->arp_op = htons(ARPOP_REQUEST);
	p = (u8bits *)(ap + 1);
	for (i = 0; i < dlink.dl_len; i++)
		*p++ = dlink.dl_laddr[i];
	*p++ = (udpip_laddr >> 24) & 0xFF;
	*p++ = (udpip_laddr >> 16) & 0xFF;
	*p++ = (udpip_laddr >> 8) & 0xFF;
	*p++ = udpip_laddr & 0xFF;
	for (i = 0; i < dlink.dl_len; i++)
		*p++ = '\0';
	*p++ = (arp_waiting >> 24) & 0xFF;
	*p++ = (arp_waiting >> 16) & 0xFF;
	*p++ = (arp_waiting >> 8) & 0xFF;
	*p++ = arp_waiting & 0xFF;

	switch (dlink.dl_type) {
	case ARPHRD_ETHERNET:
		ether_output(arp_buffer, ARP_BUFLEN(ap->arp_hln), ether_broadcast, ETHERNET_TYPE_ARP);
		break;
	default:
		printf("Unknown dlink type 0x%x\n", dlink.dl_type);
		break;
	}
}

void
arp_input(void *addr,
	  unsigned len)
{
	struct frame_arp *ap;
	struct frame_arp *ap1;
	unsigned i;
	u8bits *p;
	u32bits ip;

#ifdef	ARPDEBUG
	PPRINT(("Start arp_input(0x%x, 0x%x)\n", addr, len));
#endif

	ap = (struct frame_arp *)addr;
	if (ntohs(ap->arp_hrd) != dlink.dl_type ||
	    ntohs(ap->arp_pro) != ETHERNET_TYPE_IP ||
	    ap->arp_hln != dlink.dl_len ||
	    ap->arp_pln != sizeof (u32bits) ||
	    len < sizeof (struct frame_arp) + 2 * (ap->arp_hln + sizeof (u32bits))) {
#ifdef	ARPDEBUG
		PPRINT(("arp_input: Bad type\n"));
#endif
		return;
	}
	ap->arp_op = ntohs(ap->arp_op);
	switch (ap->arp_op) {
	case ARPOP_REQUEST:
		p = (u8bits *)(ap + 1) + 2 * ap->arp_hln + ap->arp_pln;
		ip = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		if (udpip_laddr != ip) {
#ifdef	ARPDEBUG
			PPRINT(("arp_input: Not for us\n"));
#endif
			return;
		}

		ap1 = (struct frame_arp *)&arp_buffer[dlink.dl_hlen];
		ap1->arp_hrd = htons(dlink.dl_type);
		ap1->arp_pro = htons(ETHERNET_TYPE_IP);
		ap1->arp_hln = dlink.dl_len;
		ap1->arp_pln = sizeof (u32bits);
		ap1->arp_op = htons(ARPOP_REPLY);
		p = (u8bits *)(ap1 + 1);
		for (i = 0; i < dlink.dl_len; i++)
			*p++ = dlink.dl_laddr[i];
		*p++ = (udpip_laddr >> 24) & 0xFF;
		*p++ = (udpip_laddr >> 16) & 0xFF;
		*p++ = (udpip_laddr >> 8) & 0xFF;
		*p++ = udpip_laddr & 0xFF;
		for (i = 0; i < dlink.dl_len + sizeof (u32bits); i++)
			*p++ = ((u8bits *)(ap + 1))[i];
		len = sizeof (struct frame_arp) + 2 * (dlink.dl_len + 4);
		switch (dlink.dl_type) {
		case ARPHRD_ETHERNET:
			(void)ether_output(arp_buffer, ARP_BUFLEN(ap1->arp_hln),
					   (u8bits *)(ap + 1), ETHERNET_TYPE_ARP);
			break;
		default:
			printf("Unknown dlink type 0x%x\n", dlink.dl_type);
			break;
		}
		return;

	case ARPOP_REPLY:
		if (arp_waiting != 0) {
			p = (u8bits *)(ap + 1) + ap->arp_hln;
			ip = 0;
			for (i = 0; i < sizeof (u32bits); i++)
				ip = (ip << 8) | *p++;
			if (arp_waiting == ip) {
				arp_waiting = 0;
				p = (u8bits *)(ap + 1);
				for (i = 0; i < ap->arp_hln; i++) {
					dlink.dl_raddr[i] = *p++;
				}
				break;
			}
		}
#ifdef	ARPDEBUG
		PPRINT(("arp_input: Bad sender (%x)\n", ap + 1));
#endif
		break;

	default:
#ifdef	ARPDEBUG
		PPRINT(("arp_input: Bad operation (%x)\n", ap->arp_op));
#endif
		break;
	}
}

int
arp_request(u32bits dst)
{
	unsigned waiting;

#ifdef	ARPDEBUG
	PPRINT(("Start arp_request(0x%x)\n", dst));
#endif
	arp_waiting = dst;
	(void)arp_output();
	settimeout(ARP_RETRANSMIT);
	waiting = ARP_RETRANSMIT;

	while (arp_waiting != 0 && waiting <= 60000) {
		if (isatimeout()) {
#ifdef	ARPDEBUG
			PPRINT(("ARP request sent\n"));
#endif
			arp_output();
			settimeout(ARP_RETRANSMIT);
			waiting += ARP_RETRANSMIT;
		}
		if (is_intr_char())
			return (NETBOOT_ABORT);
		(*dlink.dl_input)(arp_buffer, sizeof(arp_buffer));
	}
	return (arp_waiting == 0 ? NETBOOT_SUCCESS : NETBOOT_ERROR);
}
