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
 * tcp.h
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * tcp.h,v
 * Revision 1.8.2.2.1.2  1994/09/07  04:18:10  menze
 * OSF modifications
 *
 * Revision 1.8.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 * Revision 1.8  1994/01/27  21:51:36  menze
 *   [ 1994/01/05          menze ]
 *   PROTNUM changed to PORTNUM
 *
 */

#ifndef tcp_h
#define tcp_h


#define TCP_PUSH		(TCP_CTL*MAXOPS + 0)
#define TCP_GETSTATEINFO	(TCP_CTL*MAXOPS + 1)
#define TCP_DUMPSTATEINFO	(TCP_CTL*MAXOPS + 2)
#define TCP_GETFREEPORTNUM	(TCP_CTL*MAXOPS + 3)
#define TCP_RELEASEPORTNUM	(TCP_CTL*MAXOPS + 4)
#define TCP_SETRCVBUFSPACE	(TCP_CTL*MAXOPS + 5)	/* set rx buf space */
#define TCP_GETSNDBUFSPACE	(TCP_CTL*MAXOPS + 6)	/* get tx buf space */
#define TCP_SETRCVBUFSIZE	(TCP_CTL*MAXOPS + 7)	/* set rx buf size */
#define TCP_SETSNDBUFSIZE	(TCP_CTL*MAXOPS + 8)	/* set tx buf size */
#define TCP_SETOOBINLINE	(TCP_CTL*MAXOPS + 9)	/* set oob inlining */
#define TCP_GETOOBDATA		(TCP_CTL*MAXOPS + 10)	/* read the oob data */
#define TCP_OOBPUSH		(TCP_CTL*MAXOPS + 11)	/* send oob message */
#define TCP_OOBMODE		(TCP_CTL*MAXOPS + 12)	/* this is an upcall */
#define TCP_SETPUSHALWAYS	(TCP_CTL*MAXOPS + 13)	/* always push */
#define TCP_SETRCVACKALWAYS	(TCP_CTL*MAXOPS + 14)
			    /* Implicitly does a SETRCVBUFSPACE on each xPop */

xkern_return_t	tcp_init( XObj );


#endif
