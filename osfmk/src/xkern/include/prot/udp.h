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
 * udp.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * udp.h,v
 * Revision 1.9.2.2.1.2  1994/09/07  04:18:11  menze
 * OSF modifications
 *
 * Revision 1.9.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 * Revision 1.9  1994/01/10  17:53:33  menze
 *   [ 1994/01/05          menze ]
 *   PROTNUM changed to PORTNUM
 *
 */

#ifndef udp_h
#define udp_h

#ifndef ip_h
#include <xkern/include/prot/ip.h>
#endif

#define UDP_ENABLE_CHECKSUM (UDP_CTL * MAXOPS + 0)
#define UDP_DISABLE_CHECKSUM (UDP_CTL * MAXOPS + 1)
#define UDP_GETFREEPORTNUM	(UDP_CTL * MAXOPS + 2)
#define UDP_RELEASEPORTNUM	(UDP_CTL * MAXOPS + 3)

xkern_return_t	udp_init( XObj );

#endif
