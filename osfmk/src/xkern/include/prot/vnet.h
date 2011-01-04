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
 * vnet.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * vnet.h,v
 * Revision 1.6.2.2.1.2  1994/09/07  04:18:06  menze
 * OSF modifications
 *
 * Revision 1.6.2.2  1994/09/01  04:05:26  menze
 * XObj initialization functions return xkern_return_t
 *
 */

#ifndef vnet_h
#define vnet_h


typedef enum {
    LOCAL_ADDR_C = 1,	/* An address for the local host */
    REMOTE_HOST_ADDR_C,	/* Remote host directly reachable on a local net */
    REMOTE_NET_ADDR_C,  /* Remote host on a remote network */
    BCAST_LOCAL_ADDR_C, /* 255.255.255.255 -- all local nets broadcast */
    BCAST_NET_ADDR_C,	/* Broadcast address for a single network	*/
    BCAST_SUBNET_ADDR_C,/* Broadcast for a network in presence of subnets */
    MULTI_ADDR_C	/* Multicast (class D) */
} VnetAddrClass;

typedef union {
    VnetAddrClass	class;
    IPhost		host;
} VnetClassBuf;


#define VNET_HOSTONLOCALNET	(VNET_CTL*MAXOPS + 0)
/* 
 * Input: IPhost.  Determines if the host is directly reachable on a
 * local interface.  When run on a protocol, all interfaces are
 * considered.  When run on a session, only those interfaces associated
 * with the session are considered.  Returns sizeof(IPhost) if the
 * host is reachable, 0 if it is not.
 */

#define VNET_ISMYADDR		(VNET_CTL*MAXOPS + 1)
/* 
 * Input: IPhost.  Is this ip host one of mine? 
 * (i.e., should packets sent to this address be delivered locally
 * (including broadcast))
 */

#define VNET_GETADDRCLASS	(VNET_CTL*MAXOPS + 2)
/* 
 * Input/output: VnetClassBuf.  Determines the VnetAddrClass of the given
 * IP host. 
 */

#define VNET_DISABLEINTERFACE	(VNET_CTL*MAXOPS + 3)
/* 
 * Input: void *.  Pushes through this session will not use the
 * specified interface until a corresponding VNET_ENABLEINTERFACE is
 * performed.  Probably only useful for broadcast sessions.
 */

#define VNET_ENABLEINTERFACE	(VNET_CTL*MAXOPS + 4)
/* 
 * Input: void *.  Undoes the effect of a VNET_DISABLEINTERFACE
 */

#define VNET_GETINTERFACEID	(VNET_CTL*MAXOPS + 5)
/* 
 * Output: void *.  Returns an identifier for this interface which can
 * be used as the argument on VNET_{DISABLE,ENABLE}INTERFACE calls.
 */

#define VNET_GETNUMINTERFACES	(VNET_CTL*MAXOPS + 6)
/* 
 * Output: int.  Returns number of interfaces used by the protocol
 * instantiation.
 */

#define VNET_REWRITEHOSTS	(VNET_CTL*MAXOPS + 7)
/* 
 * Input: int.  By default, VNET sessions implement GETPARTICIPANTS by
 * invoking it on their lower sessions and reverse-resolving the hosts
 * on the top of the participant stacks.  This operation allows an
 * upper protocol (e.g., IP) to indicate that it is going to rewrite the
 * top-component of the participant stack on return, so VNET can avoid
 * the bother of reverse-resolution (which can be considerable.)
 * int == 0 turns off the host rewriting behavior, int != 0 turns it on
 * (default).
 */

xkern_return_t	vnet_init( XObj );

#endif /* ! vnet_h */
