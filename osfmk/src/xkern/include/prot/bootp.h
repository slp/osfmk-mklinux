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

#ifndef bootp_h
#define bootp_h
   
xkern_return_t	bootp_init( XObj );

/*
 * Control Operations
 */
#define BOOTP_GET_CLUSTERID    ( BOOTP_CTL * MAXOPS + 0 )
#define BOOTP_GET_IPADDR       ( BOOTP_CTL * MAXOPS + 1 )
#define BOOTP_GET_GWIPADDR     ( BOOTP_CTL * MAXOPS + 2 )
#define BOOTP_GET_NODENAME     ( BOOTP_CTL * MAXOPS + 3 )
#define BOOTP_GET_TIMEMASTER   ( BOOTP_CTL * MAXOPS + 4 )
#define BOOTP_GET_MCADDR       ( BOOTP_CTL * MAXOPS + 5 )

#endif  /* ! bootp_h */
