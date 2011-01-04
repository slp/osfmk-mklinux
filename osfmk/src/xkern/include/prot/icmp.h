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
 * 
 */
/*
 * MkLinux
 */
/*
 * icmp.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * icmp.h,v
 * Revision 1.7.2.2.1.2  1994/09/07  04:18:10  menze
 * OSF modifications
 *
 * Revision 1.7.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 */

#ifndef icmp_h
#define icmp_h

#ifndef ip_h
#include <xkern/include/prot/ip.h>
#endif

#define	ICMP_ECHO_REP	0
#define ICMP_DEST_UNRCH	3
#define ICMP_SRC_QUENCH	4
#define ICMP_REDIRECT	5
#define	ICMP_ECHO_REQ	8
#define ICMP_TIMEOUT	11
#define ICMP_SYNTAX	12
#define ICMP_TSTAMP_REQ	13
#define ICMP_TSTAMP_REP	14
#define ICMP_INFO_REQ	15
#define ICMP_INFO_REP	16
#define ICMP_AMASK_REQ	17
#define ICMP_AMASK_REP	18


#define ICMP_NET_UNRCH	0
#define ICMP_HOST_UNRCH	1
#define ICMP_PROT_UNRCH	2
#define ICMP_PORT_UNRCH	3
#define ICMP_CANT_FRAG	4
#define ICMP_SRC_R_FAIL	5

#define ICMP_CANT_HOP	0
#define ICMP_CANT_REASS	1


#define ICMP_NET_REDIRECT	0
#define ICMP_HOST_REDIRECT	1
#define ICMP_TOSNET_REDIRECT	2
#define ICMP_TOSHOST_REDIRECT	3

#define ICMP_ECHO_CTL		(ICMP_CTL * MAXOPS + 0)

XObjInitFunc	icmp_init;

#endif /* icmp_h */
