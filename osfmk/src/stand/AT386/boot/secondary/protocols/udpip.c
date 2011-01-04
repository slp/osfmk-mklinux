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
 * File : udpip.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains UDP/IP functions used for Network bootstrap.
 */


#include <secondary/net.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>
#include <secondary/protocols/icmp.h>
#include <secondary/protocols/tftp.h>
#include <secondary/protocols/bootp.h>	/* bootp_input */

static u16bits ip_id;		/* IP id counter */

u32bits	udpip_laddr;		/* Local IP address */
u32bits	udpip_raddr;		/* Remote IP address */
u8bits	udpip_buffer[IP_MSS];	/* Output buffer */
u16bits udpip_tftp;		/* tftp server port */
u16bits	udpip_port;		/* first udp port available */

void
udpip_init()
{
	udpip_laddr = 0;
	udpip_tftp = 0;
	udpip_port = IPPORT_START;
	tftp_port = 0;
}

void
udpip_abort(u16bits port)
{
	if (udpip_tftp != 0 && port == udpip_tftp)
		udpip_tftp = 0;
}

static u16bits
udpip_cksum(u16bits *addr,
	    u32bits count)
{
        u32bits sum = 0;

        while (count > 1) {
                sum += *addr++;
                count -= 2;
        }
        if (count > 0)
                sum += *(u8bits *)addr;
        while (sum >> 16)
                sum = (sum & 0xFFFF) + (sum >> 16);
        return (~sum);
}

int
udpip_output_with_protocol(struct udpip_output *udpip_output, int proto)
{
	struct frame_ip *ip;
	struct frame_udp *up;
	struct pseudo_ip *pp;
	unsigned len;

	ip = (struct frame_ip *)&udpip_output->udpip_buffer[dlink.dl_hlen];
	up = (struct frame_udp *)&udpip_output->udpip_buffer[dlink.dl_hlen +
							     sizeof (struct frame_ip)];
	pp = (struct pseudo_ip *)&udpip_output->udpip_buffer[dlink.dl_hlen +
							     sizeof (struct frame_ip) -
							     sizeof (struct pseudo_ip)];
	pp->ps_src = htonl(udpip_output->udpip_src);
	pp->ps_dst = htonl(udpip_output->udpip_dst);
	pp->ps_zero = 0;
	pp->ps_protocol = proto;
	pp->ps_len = htons(udpip_output->udpip_len + sizeof (struct frame_udp));
	up->udp_sport = htons(udpip_output->udpip_sport);
	up->udp_dport = htons(udpip_output->udpip_dport);
	up->udp_len = htons(udpip_output->udpip_len + sizeof (struct frame_udp));
	up->udp_cksum = 0;
	up->udp_cksum = udpip_cksum((u16bits *)pp, udpip_output->udpip_len +
				    sizeof (struct frame_udp) +
				    sizeof (struct pseudo_ip));
	if (up->udp_cksum == 0)
		up->udp_cksum = 0xFFFF;

	len = udpip_output->udpip_len + sizeof (struct frame_ip) +
		sizeof (struct frame_udp);
	ip->ip_version = IP_VERSION;
	ip->ip_hlen = sizeof (struct frame_ip) >> 2;
	ip->ip_tos = IP_TOS;
	ip->ip_len = htons(len);
	ip->ip_id = htons(ip_id++);
	ip->ip_offset = 0;
	ip->ip_ttl = 0xFF;
	ip->ip_protocol = proto;
	ip->ip_cksum = 0;
	ip->ip_src = htonl(udpip_output->udpip_src);
	ip->ip_dst = htonl(udpip_output->udpip_dst);
	ip->ip_cksum = udpip_cksum((u16bits *)ip, sizeof (struct frame_ip));

#ifdef	UDPIPDEBUG
	PPRINT(("udpip_output(src %x dst %x)\n",
			udpip_output->udpip_src,udpip_output->udpip_dst));
	printf("link 0x%x link type %d\n",&dlink,dlink.dl_type);
#endif
	switch (dlink.dl_type) {
	    case ARPHRD_ETHERNET:
		return (ether_output(udpip_output->udpip_buffer,
				     len,
				     udpip_output->udpip_dst == 0xFFFFFFFF ?
				     ether_broadcast : dlink.dl_raddr,
				     ETHERNET_TYPE_IP));
		/*NOTREACHED*/
	    default:
		printf("Unknown link type: %d\n",dlink.dl_type);
	}
	return (1);
}

int
udpip_input(void *addr,
	    unsigned len)
{
	struct frame_ip *ip;
	struct frame_udp *up;
	struct pseudo_ip *pp;
	struct udpip_input udpip_input;
	u16bits cksum;
	unsigned i;

#if	defined(PROTODEBUG) && defined (UDPIPDEBUG)
	if (protodebug) {
		dump_packet("udpip dump: ", addr, 128);
	}
#endif
	ip = (struct frame_ip *)addr;
	if (ip->ip_version != IP_VERSION ||
	    (ip->ip_hlen << 2) > len - dlink.dl_hlen ||
	    (ip->ip_protocol != IPPROTO_UDP 
	     && ip->ip_protocol != IPPROTO_ICMP
	     ) || udpip_cksum((u16bits *)ip, sizeof (struct frame_ip))) {
#ifdef	UDPIPDEBUG
		PPRINT(("udpip_input: bad IP header %d\n",ip->ip_protocol));
#endif
		return (0);
	}

	ip->ip_len = ntohs(ip->ip_len);
	if (ip->ip_len > len)
		return (0);
	if (ip->ip_len < len)
		udpip_input.udpip_len = ip->ip_len;
	else
		udpip_input.udpip_len = len;
	ip->ip_offset = ntohs(ip->ip_offset);
	if ((ip->ip_offset & IP_MF) || (ip->ip_offset & ~IP_FLAGS)) {
		PPRINT(("udpip_input: IP fragmented\n"));
		return (0);
	}
	udpip_input.udpip_dst = ntohl(ip->ip_dst);
	if (udpip_laddr != 0 && udpip_input.udpip_dst != udpip_laddr) {
#ifdef	UDPIPDEBUG
		PPRINT(("udpip_input: not for us (%x dst %x)\n",
			       udpip_laddr,udpip_input.udpip_dst));
#endif
		return (0);
	}
	udpip_input.udpip_src = ntohl(ip->ip_src);
	if (ip->ip_protocol == IPPROTO_ICMP) {
		udpip_input.udpip_addr = ((char *)addr) + (ip->ip_hlen << 2);
		return (icmp_input(&udpip_input));
	}
	udpip_input.udpip_addr = ((char *)addr) + (ip->ip_hlen << 2) +
		sizeof (struct frame_udp);
	up = (struct frame_udp *)&((char *)addr)[ip->ip_hlen << 2];
	pp = (struct pseudo_ip *)&((char *)addr)[(ip->ip_hlen << 2) -
						 sizeof (struct pseudo_ip)];
	if (up->udp_cksum != 0) {
		pp->ps_src = ip->ip_src;
		pp->ps_dst = ip->ip_dst;
		pp->ps_zero = 0;
		pp->ps_protocol = IPPROTO_UDP;
		pp->ps_len = up->udp_len;
		if (cksum = udpip_cksum((u16bits *)pp, ntohs(up->udp_len) +
				sizeof (struct pseudo_ip))) {
			PPRINT(("udpip_input: bad udp checksum (%x)\n",
				       cksum));
			return (0);
		}
	}
	up->udp_dport = ntohs(up->udp_dport);
	up->udp_sport = ntohs(up->udp_sport);
	up->udp_len = ntohs(up->udp_len);
	if (up->udp_len - sizeof (struct frame_udp) < udpip_input.udpip_len)
		udpip_input.udpip_len = up->udp_len - sizeof (struct frame_udp);
	if (up->udp_dport == IPPORT_BOOTPC &&
	    up->udp_sport == IPPORT_BOOTPS)
		return (bootp_input(&udpip_input));

	if (tftp_port != 0 && tftp_port == up->udp_dport) {
		if (udpip_tftp == 0) {
			udpip_tftp = up->udp_sport;
			if (!tftp_input(&udpip_input))
				udpip_tftp = 0;
			return (1);
		}
		if (up->udp_sport == udpip_tftp)
			return (tftp_input(&udpip_input));
	}
#if 0
	printf("udpip_input: %s (sport %d, dport %d sp %d dp %d)\n",
	       "no destination", up->udp_sport, up->udp_dport,
	       tftp_port, udpip_tftp);
#endif
#if defined(PROTODEBUG) && defined (UDPIPDEBUG)
	PPRINT(("udpip_input: %s (sport = %d, dport = %d)\n",
		       "no destination", up->udp_sport, up->udp_dport));
#endif
	return (0);
}
