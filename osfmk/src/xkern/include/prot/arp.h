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
 * arp.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * arp.h,v
 * Revision 1.12.3.2.1.2  1994/09/07  04:18:11  menze
 * OSF modifications
 *
 * Revision 1.12.3.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 */

#ifndef arp_h
#define arp_h

#ifndef upi_h
#include <xkern/include/upi.h>
#endif
#ifndef eth_h
#include <xkern/include/prot/eth.h>
#endif
#ifndef ip_h
#include <xkern/include/prot/ip.h>
#endif

xkern_return_t	arp_init( XObj );


#define ARP_INSTALL 		( ARP_CTL * MAXOPS + 0 )
#define ARP_GETMYBINDING	( ARP_CTL * MAXOPS + 1 )

typedef struct {
    ETHhost	hw;
    IPhost	ip;
} ArpBinding;


/* 
 * kludge to let the sunos simulator's ethernet driver simulate broadcast
 */

typedef int	(ArpForEachFunc)( ArpBinding *, void * );

typedef struct {
    void		*v;
    ArpForEachFunc	*f;
} ArpForEach;

#define ARP_FOR_EACH		( ARP_CTL * MAXOPS + 2 )

#define CONVERT_MULTI(ip, eth) { \
	(eth)->high = htons(0x0100); \
	(eth)->mid = htons(0x5e00 | ((ip)->b &  0x7f)); \
	(eth)->low = htons(((ip)->c << 8) | (ip)->d); \
}
	


#endif
