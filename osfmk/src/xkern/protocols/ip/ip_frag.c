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
 * ip_frag.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * ip_frag.c,v
 * Revision 1.8.1.4.1.2  1994/09/07  04:18:31  menze
 * OSF modifications
 *
 * Revision 1.8.1.4  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.8.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.8.1.2  1994/07/22  20:08:13  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.8.1.1  1994/04/14  21:42:52  menze
 * Uses allocator-based message interface
 *
 * Revision 1.8  1993/12/13  23:13:51  menze
 * Modifications from UMass:
 *
 *   [ 93/11/12          yates ]
 *   Changed casting of Map manager calls so that the header file does it all.
 *
 */

/*
 * REASSEMBLY ROUTINES
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/ip/ip_i.h>


static void		displayFragList( FragList * );
static xkern_return_t 	hole_create(FragInfo *, FragInfo *, u_int, u_int, Path);


xkern_return_t
ipReassemble(s, down_s, dg, hdr)
    XObj s;
    XObj down_s;
    Msg dg;
    IPheader *hdr;
{
    PState 	*ps = xMyProtl(s)->state;
    FragId 	fragid;
    FragList	*list;
    FragInfo 	*fi, *prev;
    Hole 	*hole;
    u_short 	offset, len;
    xkern_return_t xkr;
    
    offset = FRAGOFFSET(hdr->frag)*8;
    len = hdr->dlen - GET_HLEN(hdr) * 4;
    xTraceP3(s, TR_EVENTS, "IP reassemble, seq=%d, off=%d, len=%d",
	    hdr->ident, offset, len);
    
    fragid.source = hdr->source;
    fragid.dest = hdr->dest;	/* might be multiple IP addresses for me! */
    fragid.prot = hdr->prot;
    fragid.pad  = 0;
    fragid.seqid = hdr->ident;
    
    if ( mapResolve(ps->fragMap, &fragid, &list) == XK_FAILURE ) {
	Allocator	mda = pathGetAlloc(s->path, MD_ALLOC);

	xTraceP0(s, TR_MORE_EVENTS, "reassemble, allocating new Fraglist");
	if ( ! (list = allocGet(mda, sizeof(FragList))) ) {
	    xTraceP0(s, TR_SOFT_ERRORS, "allocation failure");
	    return XK_FAILURE;
	}
	/* 
	 * Initialize the list with a single hole spanning the whole datagram 
	 */
	list->nholes = 1;
	list->head.next = fi = allocGet(mda, sizeof(FragInfo));
	if ( fi == 0 ) {
	    xTraceP0(s, TR_SOFT_ERRORS, "allocation failure");
	    allocFree(list);
	    return XK_FAILURE;
	}
	fi->next = 0;
	fi->type = RHOLE;
	fi->u.hole.first = 0;
	fi->u.hole.last = INFINITE_OFFSET;
	if ( mapBind(ps->fragMap, &fragid, list, &list->binding )
	    	!= XK_SUCCESS ) {
	    xTraceP0(s, TR_SOFT_ERRORS, "bind failure");
	    allocFree(fi);
	    allocFree(list);
	    return XK_FAILURE;
	}
    } else {
	xTraceP1(s, TR_MORE_EVENTS,"reassemble - found fraglist == %x", list);
    }
    list->gcMark = FALSE;
    
    xIfTrace(ipp, TR_DETAILED) {
	xTraceP0(s, TR_DETAILED, "frag table before adding");
	displayFragList(list);
    }
    prev = &list->head;
    for ( fi = prev->next; fi != 0; prev = fi, fi = fi->next ) {
	if ( fi->type == RFRAG ) {
	    continue;
	}
	hole = &fi->u.hole;
	if ( (offset < hole->last) && ((offset + len) > hole->first) ) {
	    xTraceP0(s, TR_MORE_EVENTS, "reassemble, found hole for datagram");
	    xTraceP2(s, TR_DETAILED, "hole->first: %d  hole->last: %d",
		    hole->first, hole->last);
	    /*
	     * check to see if frag overlaps previously received
	     * frags.  If it does, we discard parts of the new message.
	     */
	    if ( offset < hole->first ) {
		xTraceP0(s, TR_MORE_EVENTS, "Truncating message from left");
		msgPopDiscard(dg, hole->first - offset);
		offset = hole->first;
	    }
	    if ( (offset + len) > hole->last ) {
		xTraceP0(s, TR_MORE_EVENTS, "Truncating message from right");
		msgTruncate(dg, hole->last - offset); 
		len = hole->last - offset;
	    }
	    /* now check to see if new hole(s) need to be made */
	    if ( ((offset + len) < hole->last) &&
		 (hdr->frag & MOREFRAGMENTS) ) {
		/* This hole is not created if this is the last fragment */
		xTraceP0(s, TR_DETAILED, "Creating new hole above");
		if ( hole_create(prev, fi, (offset+len), hole->last, s->path)
		    	== XK_FAILURE ) {
		    return XK_FAILURE;
		}
		list->nholes++;
	    }
	    if ( offset > hole->first ) {
		xTraceP0(s, TR_DETAILED, "Creating new hole below");
		if ( hole_create(fi, fi->next, hole->first, (offset), s->path)
		    	== XK_FAILURE ) {
		    return XK_FAILURE;
		}
		list->nholes++;
	    }
	    /*
	     * change this FragInfo structure to be an RFRAG
	     */
	    list->nholes--;
	    fi->type = RFRAG;
	    msgConstructCopy(&fi->u.frag, dg); 
	    break;
	} /* if found a hole */
    } /* for loop */
    xIfTrace(ipp, TR_DETAILED) {
	xTraceP0(s, TR_DETAILED, "frag table after adding");
	displayFragList(list);
    }
    /*
     * check to see if we're done
     */
    if ( list->nholes == 0 ) {
	Msg_s 		fullMsg;
	xkern_return_t	joinRes = XK_SUCCESS;
	
	xTraceP0(s, TR_MORE_EVENTS, "reassemble: now have a full datagram");
	msgConstructContig(&fullMsg, xMyPath(s), 0, 0);
	for( fi = list->head.next; fi != 0; fi = fi->next ) {
	    xAssert( fi->type == RFRAG );
	    joinRes |= msgJoin(&fullMsg, &fi->u.frag, &fullMsg);
	}
	xkr = mapRemoveBinding(ps->fragMap, list->binding);
	xAssert( xkr == XK_SUCCESS );
	ipFreeFragList(list);
	if ( joinRes == XK_SUCCESS ) {
	    xTraceP1(s, TR_EVENTS,
		     "IP reassemble popping up message of length %d",
		     msgLen(&fullMsg));
	    xkr = ipMsgComplete(s, down_s, &fullMsg, hdr);
	} else {
	    xkr = XK_FAILURE;
	}
	msgDestroy(&fullMsg);
    } else {
	xkr = XK_SUCCESS;
    }
    return xkr;
}


/* hole_create :
 *   insert a new hole frag after the given list with the given 
 *   first and last hole values
 */
static xkern_return_t
hole_create(prev, next, first, last, path)
    FragInfo *prev, *next;
    u_int first;
    u_int last;
    Path path;
{
    FragInfo 	*fi;
    
    xTrace2(ipp, TR_MORE_EVENTS,
	    "IP hole_create : creating new hole from %d to %d",
	    first, last);
    if ( ! (fi = pathAlloc(path, sizeof(FragInfo))) ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ip frag allocation failure");
	return XK_FAILURE;
    }
    fi->type = RHOLE;
    fi->u.hole.first = first;
    fi->u.hole.last = last;
    fi->next = next;
    prev->next = fi;
    return XK_SUCCESS;
}


void
ipFreeFragList( l )
    FragList *l;
{
    FragInfo *fi, *next;
    
    for( fi = l->head.next; fi != 0; fi = next ) {
	next = fi->next;
	if (fi->type == RFRAG) {
	    msgDestroy(&fi->u.frag);
	}
	allocFree(fi);
    }
    allocFree(l);
}



static void
displayFragList(l)
    FragList *l;
{
    FragInfo 	*fi;

    xTrace2(ipp, TR_ALWAYS, "Table has %d hole%c", l->nholes,
	    l->nholes == 1 ? ' ' : 's');
    for ( fi = l->head.next; fi != 0; fi = fi->next ) {
	if ( fi->type == RHOLE ) {
	    xTrace2(ipp, TR_ALWAYS, "hole  first == %d  last == %d",
		    fi->u.hole.first, fi->u.hole.last);
	} else {
	    xTrace1(ipp, TR_ALWAYS, "frag, len %d", msgLen(&fi->u.frag));
	}
    } 
}
