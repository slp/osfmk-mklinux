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
 * File : icmp.h
 *
 * Author : Yves Paindaveine
 *
 * This file contains ICMP descriptions used for Network bootstrap.
 *
 * references to be found in RFC792 - RFC896 - RFC1016 - RFC950 - RFC956-958
 */

#ifndef __ICMP_H__
#define __ICMP_H__

#define IPPROTO_ICMP	1

struct frame_icmp {
	u8bits	icmp_type;		/* Type */
	u8bits	icmp_code;		/* Code */
	u16bits	icmp_checksum;		/* Checksum */
	u16bits	icmp_id;		/* identifier */
	u16bits	icmp_seqno;		/* Sequence number */
#define MAX_ICMP_DATA	10
	u8bits	icmp_data[MAX_ICMP_DATA];/* Additional data */
};

#define	ICMP_TYPE_ECHO_REPLY		 0
#define	ICMP_TYPE_DST_UNREACHABLE	 3
#define		ICMP_CODE_NET_UNREACHABLE	 0
#define		ICMP_CODE_PORT_UNREACHABLE	 3
#define	ICMP_TYPE_SOURCE_QUENCH		 4
#define	ICMP_TYPE_ECHO_REQUEST		 8
#define	ICMP_TYPE_TIME_EXCEEDED		11

extern int icmp_input(struct udpip_input *);

#endif	/* __ICMP_H__ */
