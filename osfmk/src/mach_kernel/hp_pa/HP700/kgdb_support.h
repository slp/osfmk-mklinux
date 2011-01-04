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
/*	$CHeader: kgdb_support.h 1.5 93/12/26 22:15:43 $ */

/* 
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */

#ifndef _PARISC_KGDB_SUPPORT_H_
#define _PARISC_KGDB_SUPPORT_H_

/*
 *	kgdb_support.h
 *
 *	The module kgdb_support contains the device dependent support routines
 *	for the kgdb debugger.
 */

/*
 * From kgdb_support.c
 */

/*
 *	Initialize the network connection
 */
void
kgdb_net_init(void);

void
tgdb_net_init(void);

/*
 *	Handle a network interrupt
 */
void
kgdb_net_intr(void);

/*
 *	Transmit a network packet
 */
boolean_t
kgdb_packet_input(void *buffer, unsigned int size);

void
kgdb_send_packet(void *buffer, unsigned int size);

boolean_t
tgdb_packet_input(void *buffer, unsigned int size);

void
tgdb_send_packet(void *buffer, unsigned int size);


#define	EHLEN		14
#define	ETHERTYPE_IP	0x0800
#define	IPVERSION	4
#define IP_OVERHEAD	20
#define UDP_PROTOCOL	17
#define UDP_OVERHEAD	8
#define	NETIPC_UDPPORT	0xffff

extern struct node_addr *node_self_addr;

typedef struct node_addr		*node_addr_t;
typedef struct netipc_ether_header	*netipc_ether_header_t;
typedef struct netipc_udpip_header	*netipc_udpip_header_t;

struct node_addr {
	unsigned short	node_ether_addr[3];	/* typedef this? */
	unsigned long	node_ip_addr;
};

struct netipc_ether_header {
	u_short	e_dest[3];
	u_short	e_src[3];
  	short	e_ptype;
};

struct netipc_udpip_header {
	u_char	ip_vhl;			/* version & header length */
	u_char	ip_type_of_service;	/* type of service */
	short	ip_total_length;	/* total length */
	u_short	ip_id;			/* identification */
	short	ip_fragment_offset;	/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
	u_char	ip_time_to_live;	/* time to live */
	u_char	ip_protocol;		/* protocol */
	u_short	ip_checksum;		/* checksum */
	u_long	ip_src;			/* source ip address */
	u_long	ip_dst; 		/* dest ip address */

	u_short	udp_source_port;	/* source port */
	u_short	udp_dest_port;		/* destination port */
	short	udp_length;		/* udp length */
	u_short	udp_checksum;		/* udp checksum */
};

#define	ETHERMTU	1500

#endif /* _PARISC_KGDB_SUPPORT_H_ */


