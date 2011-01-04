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
 * File : udpip.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains UDP/IP descriptions used for Network bootstrap.
 */

#ifndef __UDPIP_H__
#define __UDPIP_H__

struct frame_ip {
#if	BYTE_ORDER == BIG_ENDIAN
	u8bits	ip_version:4,	/* Version */
		ip_hlen:4;	/* Header lenght */
#elif	BYTE_ORDER == LITTLE_ENDIAN
	u8bits	ip_hlen:4,	/* Header lenght */
		ip_version:4;	/* Version */
#else
	u8bits	ip_filler;	/* Byte order must be explicited */
#endif
	u8bits	ip_tos;		/* Type of service */
	u16bits	ip_len;		/* Total datagram length */
	u16bits	ip_id;		/* Datagram identification */
	u16bits	ip_offset;	/* Flags + Fragment offset */
	u8bits	ip_ttl;		/* Time to live */
	u8bits	ip_protocol;	/* Next level protocol */
	u16bits	ip_cksum;	/* Header checksum */
	u32bits	ip_src;		/* Source address */
	u32bits	ip_dst;		/* Destination address */
};

#define	IP_VERSION	4		/* Current IP version */
#define	IP_TOS		0		/* Normal TOS */
#define	IP_DF		0x4000		/* IP Flags: Don't Fragment */
#define	IP_MF		0x2000		/* IP Flags: More Fragments */
#define	IP_FLAGS	0xE000		/* IP Flags mask */
#define	IP_MSS		576		/* IP maximum segment size */

#define	IPPORT_BOOTPS	  67	/* BOOTP server port */
#define	IPPORT_BOOTPC	  68	/* BOOTP client port */
#define	IPPORT_TFTP	  69	/* TFTP port */
#define	IPPORT_START	1024	/* first free UDP client port */

#define	IPPROTO_UDP	17	/* UDP protocol number */

struct pseudo_ip {
	u32bits	ps_src;		/* Source address */
	u32bits	ps_dst;		/* Destination address */
	u8bits	ps_zero;	/* Zero padding */
	u8bits	ps_protocol;	/* Next level protocol */
	u16bits	ps_len;		/* Next level length */
};

struct frame_udp {
	u16bits	udp_sport;	/* Source port */
	u16bits udp_dport;	/* Destination port */
	u16bits udp_len;	/* Total length */
	u16bits udp_cksum;	/* Packet checksum */
};

struct udpip_output {
	u16bits	udpip_sport;	/* Source port */
	u16bits udpip_dport;	/* Destination port */
	u32bits	udpip_src;	/* Source address */
	u32bits	udpip_dst;	/* Destination address */
	u16bits	udpip_len;	/* data length */
	u8bits	*udpip_buffer;	/* buffer address */
};

struct udpip_input {
	void		*udpip_addr;	/* packet address */
	unsigned	udpip_len;	/* packet length */
	u32bits		udpip_src;	/* Source address */
	u32bits		udpip_dst;	/* Destination address */
};

extern u32bits	udpip_laddr;		/* Local IP addr */
extern u32bits	udpip_raddr;		/* Remote IP addr */
extern u8bits	udpip_buffer[IP_MSS];	/* Output buffer */
extern u16bits	udpip_tftp;		/* tftp server port */
extern u16bits	udpip_port;		/* first udp port available */

extern void udpip_init(void);
extern void udpip_abort(u16bits);
extern int udpip_output_with_protocol(struct udpip_output *, int);
extern int udpip_input(void *, unsigned);

#endif	/* __UDPIP_H__ */
