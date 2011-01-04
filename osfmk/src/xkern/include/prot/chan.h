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
 * chan.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * chan.h,v
 * Revision 1.6.2.2.1.2  1994/09/07  04:18:10  menze
 * OSF modifications
 *
 * Revision 1.6.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 */

#ifndef chan_h
#define chan_h

enum {
    CHAN_ABORT_CALL = CHAN_CTL*MAXOPS,
    CHAN_SET_TIMEOUT,
    CHAN_GET_TIMEOUT,
    CHAN_SET_MAX_TIMEOUT,
    CHAN_GET_MAX_TIMEOUT,
    CHAN_RETRANSMIT,
    CHAN_SET_ASYNC,
    CHAN_GET_ASYNC
};

typedef struct {
    XObj	transport;
    int		ticket;
    int		reliable;
    int		expensive;
    unsigned int timeout;
} chan_info_t;

xkern_return_t	chan_init( XObj );

#endif /* chan_h */
