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
 * File : bootp.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains BOOTP descriptions (RFC1533) used for Network bootstrap.
 */

#ifndef __BOOTP_H__
#define __BOOTP_H__

#define	BOOTP_DEFAULT "<bootp_default>"	/* BOOTP generic default file name */
#define	BOOTP_RETRANSMIT	937	/* BOOPT first retransmit (937 ms) */

struct frame_bootp {
	u8bits	bootp_op;		/* Operation */
	u8bits	bootp_htype;		/* Hardware address type */
	u8bits	bootp_hlen;		/* Hardware address length */
	u8bits	bootp_hops;		/* Cross gateway booting */
	u32bits	bootp_xid;		/* Transaction id */
	u16bits	bootp_secs;		/* seconds elapsed since boot */
	u16bits	bootp_filler;		/* unused */
	u32bits	bootp_ciaddr;		/* Client IP address */
	u32bits	bootp_yiaddr;		/* Your (client) IP address */
	u32bits	bootp_siaddr;		/* Server IP address */
	u32bits	bootp_giaddr;		/* Gateway IP address */
	u8bits	bootp_chaddr[16];	/* Client hardware address */
	char	bootp_sname[64];	/* Server host name */
	char	bootp_file[128];	/* Boot file name */
	u32bits	bootp_cookie;		/* vendor-specific magic cookie */
#define	MAX_VENDOR	124
	u8bits	bootp_vend[MAX_VENDOR];	/* vendor-specific area */
};

#define	BOOTP_OP_REQUEST	1		/* bootp_op: request */
#define	BOOTP_OP_REPLY		2		/* bootp_op: reply */
#define	BOOTP_HTYPE_ETHERNET	1		/* bootp_htype: ETHERNET */
#define	BOOTP_HLEN_ETHERNET	6		/* bootp_hlen: ETHERNET */
#define	BOOTP_COOKIE		0x63825363	/* bootp_cookie: value */

#define	BOOTP_TAG_PAD		  0	/* Pad */
#define	BOOTP_TAG_SUBNET	  1	/* Subnet mask */
#define	BOOTP_TAG_TIMEOFFSET	  2	/* Time offset */	
#define	BOOTP_TAG_GATEWAY	  3	/* Gateways */
#define	BOOTP_TAG_TIME		  4	/* Time servers */
#define	BOOTP_TAG_IEN116	  5	/* IEN116 Name servers */
#define	BOOTP_TAG_DOMAIN	  6	/* Domain name servers */
#define	BOOTP_TAG_LOG		  7	/* MIT-LCS log servers */
#define	BOOTP_TAG_QUOTE		  8	/* Quote of the day servers */
#define	BOOTP_TAG_LPR		  9	/* 4BSD LPR servers */
#define	BOOTP_TAG_IMPRESS	 10	/* Impress net image servers */
#define	BOOTP_TAG_RLP		 11	/* RLP servers */
#define	BOOTP_TAG_HOSTNAME	 12	/* Client host name */
#define	BOOTP_TAG_FILESIZE	 13	/* Boot file size */
#define	BOOTP_TAG_DUMPFILE	 14	/* Merit Dump File */
#define	BOOTP_TAG_DOMAINNAME	 15	/* Domain Name */
#define	BOOTP_TAG_SWAP		 16	/* Swap server */
#define	BOOTP_TAG_ROOTPATH	 17	/* Root path */
#define	BOOTP_TAG_EXTENSIONS	 18	/* Extensions path */
#define	BOOTP_TAG_IPFORWARD	 19	/* IP forwarding */
#define	BOOTP_TAG_SRCROUTING	 20	/* Non-local source routing */
#define	BOOTP_TAG_FILTER	 21	/* Policy filter */
#define	BOOTP_TAG_MAXREASS	 22	/* Max datagram reassembly */
#define	BOOTP_TAG_IPTTL		 23	/* Default IP time-to-live */
#define	BOOTP_TAG_MTUTIMEOUT	 24	/* Path MTU aging timeout */
#define	BOOTP_TAG_MTUTABLE	 25	/* Path MTU Table */
#define	BOOTP_TAG_MTU		 26	/* Interface MTU */
#define	BOOTP_TAG_LOCALSUBNET	 27	/* All subnets are local */
#define	BOOTP_TAG_BROADCAST	 28	/* Broadcast address */
#define	BOOTP_TAG_MASKDISCOVERY	 29	/* Perform mask discovery */
#define	BOOTP_TAG_MASKSUPPLIER	 30	/* Mask supplier */
#define	BOOTP_TAG_RTEDISCOVERY	 31	/* Perform router discovery */
#define	BOOTP_TAG_RTESENDADDR	 32	/* Router solicitation addr */
#define	BOOTP_TAG_STATICRTE	 33	/* Static route option */
#define	BOOTP_TAG_TRAILER	 34	/* Trailer encapsulation */
#define	BOOTP_TAG_ARPTIMEOUT	 35	/* ARP cache timeout */
#define	BOOTP_TAG_ETHERNET	 36	/* Ethernet encapsulation */
#define	BOOTP_TAG_TCPTTL	 37	/* TCP default time-to-live */
#define	BOOTP_TAG_TCPKEEPALIVEI	 38	/* TCP keepalive interval */
#define	BOOTP_TAG_TCPKEEPALIVEG	 39	/* TCP keepalive garbage */
#define	BOOTP_TAG_NISDOMAIN	 40	/* NIS domain */
#define	BOOTP_TAG_NIS		 41	/* NIS servers */
#define	BOOTP_TAG_NTP		 42	/* NTP servers */
#define	BOOTP_TAG_VENDOR	 43	/* Vendor specific information */
#define	BOOTP_TAG_NETBIOS_NAME	 44	/* NetBIOS name servers */
#define	BOOTP_TAG_NETBIOS_DATA	 45	/* NetBIOS datagram servers */
#define	BOOTP_TAG_NETBIOS_NODE	 46	/* NetBIOS node type */
#define	BOOTP_TAG_NETBIOS_SCOPE	 47	/* NetBIOS scope */
#define	BOOTP_TAG_X_FONT	 48	/* X window font servers */
#define	BOOTP_TAG_X_DM		 49	/* X window display managers */
#define	BOOTP_TAG_DHPC_IP	 50	/* DHPC requested IP address */
#define	BOOTP_TAG_DHPC_LEASE	 51	/* DHPC IP address lease time */
#define	BOOTP_TAG_DHPC_OVERLOAD	 52	/* DHPC fields overload */
#define	BOOTP_TAG_DHPC_MSGTYPE	 53	/* DHPC message type */
#define	BOOTP_TAG_DHPC_SERVERID	 54	/* DHPC server identifier */
#define	BOOTP_TAG_DHPC_PARAM	 55	/* DHPC parameter request list */
#define	BOOTP_TAG_DHPC_MESSAGE	 56	/* DHPC error message */
#define	BOOTP_TAG_DHPC_MSGSIZE	 57	/* DHPC maximum message size */
#define	BOOTP_TAG_DHPC_RENEWAL	 58	/* DHPC renewal time */
#define	BOOTP_TAG_DHPC_REBIND	 59	/* DHPC rebinding time */
#define	BOOTP_TAG_DHPC_CLASSID	 60	/* DHPC class-identifier */
#define	BOOTP_TAG_DHPC_CLIENTID	 61	/* DHPC client-identifier */
#define	BOOTP_TAG_64	 	 64
#define	BOOTP_TAG_NODE_NUMBER	200	/* user defined tag */
#define	BOOTP_TAG_CLUSTER_NAME	201	/* user defined tag */
#define	BOOTP_TAG_CLUSTER_ID	202	/* user defined tag */
#define	BOOTP_TAG_END		255	/* End field */

extern int bootp_input(struct udpip_input *);
#endif	/* __BOOTP_H__ */
