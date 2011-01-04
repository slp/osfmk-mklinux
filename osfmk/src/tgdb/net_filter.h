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

#include <machine/endian.h>

struct netipc_ether_header {
	unsigned short	e_dest[3];
	unsigned short	e_src[3];
  	short	e_ptype;
};

struct netipc_udpip_header {
	unsigned char	ip_vhl;			/* version & header length */
	unsigned char	ip_type_of_service;	/* type of service */
	short	ip_total_length;	/* total length */
	unsigned short	ip_id;			/* identification */
	short	ip_fragment_offset;	/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
	unsigned char	ip_time_to_live;	/* time to live */
	unsigned char	ip_protocol;		/* protocol */
	unsigned short	ip_checksum;		/* checksum */
	unsigned long	ip_src;			/* source ip address */
	unsigned long	ip_dst; 		/* dest ip address */

	unsigned short	udp_source_port;	/* source port */
	unsigned short	udp_dest_port;		/* destination port */
	short	udp_length;		/* udp length */
	unsigned short	udp_checksum;		/* udp checksum */
};

#define	ETHERMTU	1500
#define	EHLEN		14
#define	ETHERTYPE_IP	0x0800
#define	IPVERSION	4
#define IP_OVERHEAD	20
#define UDP_PROTOCOL	17
#define UDP_OVERHEAD	8
#define	NETIPC_UDPPORT	0xffff

#define GDB_PORT	468
