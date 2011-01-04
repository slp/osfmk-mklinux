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

#ifndef heartbeat_h
#define heartbeat_h

xkern_return_t heartbeat_init( XObj );

typedef struct {
    u_bit32_t	  Prot;
    u_bit32_t     GenNumber;
    IPhost        Sender;
} heartbeatXchangeHdr;

/*
 * Control Operations
 */
#define HEARTBEAT_START_PULSE    ( HEARTBEAT_CTL * MAXOPS + 0 )
#define HEARTBEAT_STOP_PULSE     ( HEARTBEAT_CTL * MAXOPS + 1 )

#endif /* ! heartbeat_h */
