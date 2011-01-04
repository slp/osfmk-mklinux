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
 * in_hacks.c
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
 * in_hacks.c,v
 * Revision 1.5.2.2.1.2  1994/09/07  04:18:35  menze
 * OSF modifications
 *
 * Revision 1.5.2.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/tcp/tcp_internal.h>

#define CKSUM_TRACE 8


int
in_pcballoc(so)
	XObj so;
{
	register struct inpcb *inp;
	struct inpcb *head;
	PSTATE	*ps;

	ps = (PSTATE *)xMyProtl(so)->state;
	head = ps->tcb;
	if ( ! (inp = pathAlloc(so->path, sizeof *inp)) ) {
	    return -1;
	}
	inp->inp_head = head;
	inp->inp_session = so;
	insque(inp, head);
	sotoinpcb(so) = inp;
	return (0);
}


/*ARGSUSED*/
void
in_pcbdisconnect(inp)
    struct inpcb *inp;
{
    Kabort("in_pcbdisconnect");
}


void
in_pcbdetach(inp)
    struct inpcb *inp;
{
    remque(inp);
    allocFree(inp);
}


boolean_t
in_broadcast(addr)
    struct in_addr addr;
{
    return (ntohl(addr.s_addr) & 0xff) == 0 ||
      	   (ntohl(addr.s_addr) & 0xff) == 0xff;
}


