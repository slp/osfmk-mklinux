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
 * sb.c,v
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
 * sb.c,v
 * Revision 1.5.1.3.1.2  1994/09/07  04:18:32  menze
 * OSF modifications
 *
 * Revision 1.5.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.5.1.2  1994/07/22  20:08:15  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.5.1.1  1994/04/20  22:00:13  hkaram
 * Uses allocator-based message interface
 *
 * Revision 1.5  1993/12/07  18:13:16  menze
 * Added a prototype
 *
 */

/*
 * sb = send buffer ??
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/tcp/insque.h>
#include <xkern/protocols/tcp/sb.h>

extern int tracetcpp;
struct sb_i *sbifreelist = 0;


static void	print_sb( struct sb *, char * );


static void
print_sb(sb, m)
    struct sb *sb;
    char *m;
{
    struct sb_i *s;
    printf("%s sbLen == %d:", m, sb->len);
    for (s = sb->next; s != (struct sb_i *)sb; s = s->next) {
	printf("(%d)", msgLen(&s->m));
    }
    printf("\n");
}


xkern_return_t
sbappend(sb, m, sessn)
    struct sb *sb;
    Msg m;
    XObj sessn;
{
    struct sb_i *nsb;
    xTrace3(tcpp, 4, "sbappend on %X olen %d msglen %d",
	    sb, sb->len, msgLen(m));
    if (msgLen(m) == 0) {
	return XK_SUCCESS;
    }
    sbinew(nsb, sessn->path);
    if ( ! nsb ) {
	return XK_NO_MEMORY;
    }
    msgConstructCopy(&nsb->m, m);
    if (sb->len==0) {
	sb->next = nsb;
	nsb->prev = (struct sb_i *)sb;
    }
    else {
	nsb->prev = sb->prev;
	nsb->prev->next = nsb;
    }
    nsb->next = (struct sb_i *)sb;
    sb->prev = nsb;
    
    sb->len += msgLen(m);
    /*  insque(nsb, sb->next); */
    xIfTrace(tcpp, 4) print_sb(sb, "append");
    return XK_SUCCESS;
}


/*
 * collect 'len' bytes at offset 'off' from send buffer 'sb' and put
 * them in msg 'm'.  'm' is assumed to be uninitialized.
 */
xkern_return_t
sbcollect(sb, m, off, len, delete, sessn)
    XObj sessn;
    struct sb *sb;
    Msg m;
    int off, len, delete;
{
    struct sb_i *s, *next;
    xkern_return_t	xkr;
    
    xTrace5(tcpp, 4, "sbcollect on %X olen %d len %d off %d %s",
	    sb, sb->len, len, off, delete ? "delete" : "");
    xAssert(!delete || off == 0);
    if (len == 0) {
	xkr = msgConstructContig(m, xMyPath(sessn), 0, 0);
	xAssert( xkr == XK_SUCCESS );
	return xkr;
    }
    xIfTrace(tcpp, 5) print_sb(sb, "collect");
    for (s = sb->next;
	 s != (struct sb_i *)sb && off >= msgLen(&s->m);
	 s = s->next) 
      off -= msgLen(&s->m);
    xAssert(s != (struct sb_i *)sb);
    if (off > 0 && off < msgLen(&s->m)) {
	struct sb_i *ns;
	sbinew(ns, sessn->path);
	if ( ! ns ) {
	    msgDestroy(m);
	    return XK_NO_MEMORY;
	}
	xkr = msgConstructContig(&ns->m, xMyPath(sessn), 0, 0);
	xAssert( xkr == XK_SUCCESS );
	xTrace2(tcpp, 5, "sbcollect: split0 msg size %d at %d",
		msgLen(&s->m), off);
	msgChopOff(&s->m, &ns->m, off);
	insque(ns, s);
    }
    if (msgLen(&s->m) > len) {
	struct sb_i *ns;
	sbinew(ns, sessn->path);
	if ( ! ns ) {
	    msgDestroy(m);
	    return XK_NO_MEMORY;
	}
	xkr = msgConstructContig(&ns->m, xMyPath(sessn), 0, 0);
	xAssert( xkr == XK_SUCCESS );
	xTrace2(tcpp, 5, "sbcollect: split1 msg size %d at %d",
		msgLen(&s->m), len);
	msgChopOff(&s->m, &ns->m, len);
	insque(ns, s);
	s = ns;
    }
    /*
     * We now have s pointing to the first msg, collect the rest
     */
    xTrace1(tcpp, 5, "sbcollect: first piece has size %d", msgLen(&s->m));
    /* msg_save(new, s->m); */
    msgConstructCopy(m, &s->m);
    len -= msgLen(m);
    s = s->next;
    while (len > 0) {
	xAssert(s != (struct sb_i *) sb);
	next = s->next;
	if (msgLen(&s->m) > len) {
	    sbinew(next, sessn->path);
	    if ( ! next ) {
		msgDestroy(m);
		return XK_NO_MEMORY;
	    }
	    msgConstructCopy(&next->m, &s->m);
	    xTrace2(tcpp, 5, "sbcollect: split2 msg size %d at %d",
		    msgLen(&s->m), len);
	    msgChopOff(&next->m, &s->m, len);
	    insque(next, s->next);
	}
	/* msg_save(s->m, s->m); */
	if ( msgJoin(m, m, &s->m) == XK_FAILURE ) {
	    xTraceP0(sessn, TR_ERRORS, "msgJoin fails");
	    msgDestroy(m);
	    return XK_FAILURE;
	}
	len -= msgLen(&s->m);
	s = next;
    }
    return XK_SUCCESS;
}


void
sbflush(sb)
    struct sb *sb;
{
    struct sb_i *s = sb->next, *next;
    
    xTrace2(tcpp, 4, "sbflush on %X olen %d", sb, sb->len);
    while (s != (struct sb_i *)sb) {
	next = s->next;
	xTrace1(tcpp, 5, "sbflush: freeing msg len %d", msgLen(&s->m));
	msgDestroy(&s->m);
	sbifree(s);
	s = next;
    }
    sb->len = 0;
}


void
sbdrop(sb, len, sessn)
    struct sb *sb;
    int len;
    XObj sessn;
{
    struct sb_i *s, *next;
    
    s = sb->next;
    xTrace3(tcpp, 4, "sbdrop on %X olen %d len %d", sb, sb->len, len);
    xIfTrace(tcpp, 5) print_sb(sb, "drop before");
    while (s != (struct sb_i *)sb && len > 0) {
	if (len < msgLen(&s->m)) {
	    msgPopDiscard(&s->m, len);
	    break;
	}
	len -= msgLen(&s->m);
	sb->len -= msgLen(&s->m);
	next = s->next;
	remque(s);
	xTrace1(tcpp, 5, "sbdrop: freeing msg len %d", msgLen(&s->m));
	msgDestroy(&s->m);
	sbifree(s);
	s = next;
    }
    xIfTrace(tcpp, 5) print_sb(sb, "drop after");
}


void
sbdelete(sb)
    struct sb *sb;
{
    xTrace1(tcpp, 3, "sbdelete on %X", sb);
    sbflush(sb);
    allocFree((char *)sb);
}
