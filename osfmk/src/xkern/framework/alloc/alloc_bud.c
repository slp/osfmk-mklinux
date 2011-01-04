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
 * alloc_bud.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * alloc_bud.c,v
 * Revision 1.2.1.2.1.2  1994/09/07  04:18:21  menze
 * OSF modifications
 *
 * Revision 1.2.1.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.2.1.1  1994/07/21  23:45:58  menze
 * Allocators present only a "raw memory" interface.  All Msg and MNode
 * references have been removed.
 *
 * Uses a memory header format common to most allocators
 *
 */

/* 
 * This is the "Budget Allocator", an exterior allocator which does
 * not round-up allocations and does not pre-allocate.  
 */

#include <xkern/include/xk_alloc.h>
#include <xkern/include/intern_alloc.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>

/* 
 * Block size for breaking up allocPartial requests.  This is usually
 * determined by querying the lower allocator -- this default is used
 * only in extraordinary circumstances.
 */
#define DEFAULT_BLOCK_SIZE	4000

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



static xkern_return_t
adjustMin( Allocator self, int nBufs, u_int *length )
{
    xTrace3(alloc, TR_FULL_TRACE,
	    "budget alloc [%s] adjustMin, %d bytes, %d msgs",
	    self->name, *length, nBufs);
    *length += HDR_SIZE;
    if ( allocInteriorAdjustMin(self->lower, self, length, nBufs) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    *length -= HDR_SIZE;
    return XK_SUCCESS;
}


static void *
get( Allocator self, u_int *length )
{
    HDR		*h;

    *length += HDR_SIZE;
    xTrace2(alloc, TR_FULL_TRACE, "budget alloc [%s] get, allocating %d",
	    self->name, *length);
    if ( ! (h = allocInteriorGet(self->lower, self, length)) ) {
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
    xTrace2(alloc, TR_FULL_TRACE, "budget alloc [%s] getPartial (%d)",
	    self->name, *length);
    if ( minLength > self->largeBlockSize ) {
	return 0;
    }
    if ( *length > self->largeBlockSize ) {
	*length = self->largeBlockSize;
    }
    return get(self, length);
}


static void
put( Allocator self, void *b )
{
    HDR	*h = (HDR *)((char *)b - offsetof(HDR, buf));

    xTrace3(alloc, TR_FULL_TRACE, "budget alloc [%s] put (%x, %d)",
	    self->name, b, h->len);
    xAssert(h->alloc == self);
    allocInteriorPut(self->lower, self, h, h->len);
}


/* 
 * A single method vector for all instances of this allocator.
 */
static	AllocMethods	bExt_methods;


xkern_return_t
budgetAlloc_init( Allocator self, u_int blocks[ALLOC_MAX_BLOCKS] )
{
    xTrace0(alloc, TR_FULL_TRACE, "budget alloc init");
    if ( bExt_methods.u.e.min == 0 ) {
	bExt_methods.s.xferRequest = xferRequest;
	bExt_methods.s.xferIncoming = xferIncoming;
	bExt_methods.u.e.getMax = getMax;
	bExt_methods.u.e.min = adjustMin;
	bExt_methods.u.e.minPartial = defaultMinPartial;
	bExt_methods.u.e.get = get;
	bExt_methods.u.e.getPartial = getPartial;
	bExt_methods.u.e.put = put;
    }
    if ( ! self->lower ) {
	return XK_FAILURE;
    }
    self->class = ALLOC_EXTERIOR;
    self->methods = &bExt_methods;
    self->hdrType = HDR_TYPE;
    {
	int	i;

	if ( blocks[0] == 0 ) {
	    xTrace1(alloc, TR_ERRORS, "budget allocator %s -- no blocking info",
		    self->name);
	    return XK_FAILURE;
	}
	for ( i=0; blocks[i]; i++ )
	  ;
	self->largeBlockSize = (blocks[i-1] <= HDR_SIZE) ?
	  	DEFAULT_BLOCK_SIZE : blocks[i-1] - HDR_SIZE;
	xTrace2(alloc, TR_MORE_EVENTS, "budget alloc %s using largeBlockSize %d",
		self->name, self->largeBlockSize);
    }
    return XK_SUCCESS;
}

