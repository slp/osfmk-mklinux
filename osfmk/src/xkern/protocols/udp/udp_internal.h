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
 * udp_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.9.1 $
 * $Date: 1995/02/23 15:41:10 $
 */

#include <xkern/protocols/udp/udp_port.h>

#define	HLEN	(sizeof(HDR))

typedef struct header {
    UDPport 	sport;	/* source port */
    UDPport 	dport;	/* destination port */
    u_short 	ulen;	/* udp length */
    u_short	sum;	/* udp checksum */
} HDR;

typedef struct pstate {
    Map   	activemap;
    Map		passivemap;
    void	*portstate;
} PSTATE;

typedef struct sstate {
    HDR         hdr;
    IPpseudoHdr	pHdr;
    u_char	useCkSum;
} SSTATE;

/*
 * The active map is keyed on the pair of ports and the lower level IP
 * session.
 */
typedef struct {
    UDPport   	localport;
    UDPport  	remoteport;
    Sessn	lls;
} ActiveId;

typedef struct {
    Msg		m;
    XObj 	s;
} storeInfo;

/*
 * The key for the passive map is just the local UDP port number.
 */
typedef UDPport PassiveId;

#define USE_CHECKSUM_DEF 0


