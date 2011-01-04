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
 */
/*
 * MkLinux
 */
/*     
 * alloc_block.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * 
 * alloc_block.c,v
 * Revision 1.2.1.2.1.2  1994/09/07  04:18:23  menze
 * OSF modifications
 *
 * Revision 1.2.1.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.2.1.1  1994/07/21  23:43:58  menze
 * Allocators present only a "raw memory" interface.  All Msg and MNode
 * references have been removed.
 *
 * Uses a memory header format common to most allocators
 *
 * Revision 1.2  1994/05/17  18:16:23  menze
 * Call-backs are working
 * Interior alloc interface is now more block-oriented
 *
 */


/* 
 * These are methods for a block-allocator.  This allocator allocates
 * and returns memory in specified block sizes only.  
 */

#include <xkern/include/xk_alloc.h>
#include <xkern/include/intern_alloc.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>


/* 
 * The stored block sizes reflect useful user data size (i.e., does
 * not include the mnode length).
 */
typedef struct {
    u_int	blocks[ALLOC_MAX_BLOCKS];
} MyState;


#define HDR		AllocHdrStd
#define HDR_TYPE	ALLOC_HDR_TYPE_STD
#define HDR_SIZE	ALLOC_HDR_SIZE_STD


static u_int
getMax( Allocator self )
{
    return allocInteriorGetMax(self->lower, self);
}


/* 
 * Approve all requests unconditionally
 */
static xkern_return_t
xferRequest( Allocator self, void *buf, u_int size )
{
    return XK_SUCCESS;
}


/* 
 * Accept all requests unconditionally
 */
static xkern_return_t
xferIncoming( Allocator self, void *buf, u_int size )
{
    return XK_SUCCESS;
}


static __inline__ u_int
findBlockSize( u_int req, MyState *s )
{
    int	i;

    for ( i=0; s->blocks[i] != 0; i++ ) {
	if ( req <= s->blocks[i] ) {
	    return s->blocks[i];
	}
    }
    return req;
}


/* 
 * In both minContig (and, by extension, minUltimate), allocate space
 * for one PNode per MNode.  This should reserve enough PNode space
 * for all but the most pathological of messages. 
 */

static xkern_return_t
minContig( Allocator self, int nMsgs, u_int *reqSize )
{
    MyState	*state = self->state;

    xTrace3(alloc, TR_FULL_TRACE,
	    "block alloc [%s] minContig, %d bytes, %d msgs",
	    self->name, *reqSize, nMsgs);
    *reqSize = findBlockSize(*reqSize, state) + HDR_SIZE;
    if ( allocInteriorAdjustMin(self->lower, self, reqSize, nMsgs) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    *reqSize -= HDR_SIZE;
    return XK_SUCCESS;
}



static void *
get( Allocator self, u_int *length )
{
    MyState	*state = self->state;
    HDR		*h;

    xTrace2(alloc, TR_FULL_TRACE, "block alloc [%s] getMNode, %d bytes",
	    self->name, *length);
    *length = findBlockSize(*length, state) + HDR_SIZE;
    if ( (h = allocInteriorGet(self->lower, self, length)) == 0 ) {
	return 0;
    }
    h->len = *length;
    h->alloc = self;
    *length -= HDR_SIZE;
    return h->buf;
}


static void *
getPartial( Allocator self, u_int *length, u_int minLength )
{
    MyState	*state = self->state;
    u_int	allocLen;
    HDR		*h;

    if ( minLength > self->largeBlockSize ) {
	return 0;
    }
    if ( *length > self->largeBlockSize ) {
	allocLen = self->largeBlockSize;
    } else {
	allocLen = findBlockSize(*length, state);
    }
    allocLen += HDR_SIZE;
    xTrace3(alloc, TR_FULL_TRACE,
	    "block alloc [%s] getMNodePartial, %d bytes, allocating %d",
	    self->name, *length, allocLen);
    h = allocInteriorGet(self->lower, self, &allocLen);
    if ( h == 0 ) {
	return 0;
    }
    h->len = allocLen;
    h->alloc = self;
    *length = allocLen - HDR_SIZE;
    return h->buf;
}



static void
put( Allocator self, void *buf )
{
    HDR *h = (HDR *)((char *)buf - HDR_SIZE);

    xAssert(h->alloc == self);
    xTrace3(alloc, TR_FULL_TRACE, "block alloc [%s] free (%x, %d)",
	    self->name, buf, h->len);
    allocInteriorPut(self->lower, self, h, h->len);
}


static void
startCallBacks( Allocator lower, void *arg )
{
    allocStartCallBacks((Allocator)arg);
}


/* 
 * A single method vector for all instances of this allocator.
 */
static	AllocMethods	blockExt_methods;


xkern_return_t
blockAlloc_init( Allocator self, u_int blocks[ALLOC_MAX_BLOCKS] )
{
    MyState	*state;
    int		i, j;

    xTrace0(alloc, TR_FULL_TRACE, "block alloc init");
    if ( blockExt_methods.u.e.get == 0 ) {
	blockExt_methods.s.xferRequest = xferRequest;
	blockExt_methods.s.xferIncoming = xferIncoming;
	blockExt_methods.u.e.getMax = getMax;
	blockExt_methods.u.e.minPartial = defaultMinPartial;
	blockExt_methods.u.e.min = minContig;
	blockExt_methods.u.e.get = get;
	blockExt_methods.u.e.getPartial = getPartial;
	blockExt_methods.u.e.put = put;
    }
    if ( ! self->lower ) {
	return XK_FAILURE;
    }
    self->class = ALLOC_EXTERIOR;
    self->methods = &blockExt_methods;
    state = self->state = xSysAlloc(sizeof(MyState));
    bzero((char *)state, sizeof(MyState));

    xTrace1(alloc, TR_EVENTS, "block allocator %s using blocks:", self->name);
    if ( blocks[0] == 0 ) {
	xTrace1(alloc, TR_ERRORS, "block allocator %s -- no blocking info",
		self->name);
	return XK_FAILURE;
    }
    for ( i=0, j=0; blocks[i] != 0; i++ ) {
	if ( blocks[i] <= HDR_SIZE ) {
	    xTrace2(alloc, TR_EVENTS,
		    "block allocator %s rejects block size %d, too small",
		    self->name, blocks[i]);
	    continue;
	}
	state->blocks[j] = blocks[i] - HDR_SIZE;
	xTrace1(alloc, TR_EVENTS, "    %d", state->blocks[j]);
	j++;
    }
    if ( j == 0 ) return XK_FAILURE;
    self->largeBlockSize = state->blocks[j-1];
    allocRegister(self->lower, startCallBacks, self);
    return XK_SUCCESS;
}

