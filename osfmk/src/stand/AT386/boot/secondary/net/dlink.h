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
 * File : dlink.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains all data link descriptions used for Network bootstrap.
 */

#ifndef __DLINK_H__
#define __DLINK_H__

#define	DLINK_BUFMIN	64			/* Data Link input minimum length */
#define	DLINK_MAXHLEN	14			/* Data Link header maximum length */
#define	DLINK_MAXLEN	6			/* Data Link address maximum length */

struct dlink_st {
	u8bits	dl_hlen;			/* Data link header length */
	u8bits	dl_type;			/* Data link address type */
	u8bits	dl_len;				/* Data link address length */
	u8bits	dl_laddr[DLINK_MAXLEN];		/* Data link local address pointer */
	u8bits	dl_raddr[DLINK_MAXLEN];		/* Data link remote address pointer */
	int	(*dl_output)(void *, unsigned);	/* Output procedure */
	void	(*dl_input)(void *, unsigned);	/* Input procedure */
};

extern struct dlink_st dlink;
extern u8bits ether_broadcast[];

struct frame_ether {
	u8bits	ether_dst[6];	/* destination address */
	u8bits	ether_src[6];	/* source address */
	u16bits	ether_type;	/* Ethernet type */
};

#define	ETHERNET_HLEN		14	/* Ethernet Header length */

#define	ETHERNET_TYPE_IP	0x800	/* Internet Protocol */
#define	ETHERNET_TYPE_ARP	0x806	/* Address Resolution Protocol */

extern int ether_output(void *, unsigned, u8bits [6], u16bits);
extern void ether_input(void *, unsigned);

#endif	/* __DLINK_H__ */
