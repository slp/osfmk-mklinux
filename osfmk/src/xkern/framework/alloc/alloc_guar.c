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
 * alloc_guar.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * alloc_guar.c,v
 * Revision 1.2.1.3.1.2  1994/09/07  04:18:21  menze
 * OSF modifications
 *
 * Revision 1.2.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.2.1.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.2.1.1  1994/07/21  23:47:10  menze
 * Allocators present only a "raw memory" interface.  All Msg and MNode
 * references have been removed.
 *
 * Uses a memory header format common to most allocators
 *
 * Revision 1.2  1994/05/17  18:21:59  menze
 * Call-backs are working
 * Interior alloc interface is now more block-oriented
 * Allocators no longer have knowledge of MNode internals
 *
 * Revision 1.1  1994/04/07  22:47:47  menze
 * Initial revision
 *
 */


/* 
 * These are methods for a guaranteed block-allocator.  This allocator
 * returns memory in specified block sizes only, and also
 * pre-allocates at reservation time.
 */

#include <xkern/include/xk_path.h>
#include <xkern/include/xk_alloc.h>
#include <xkern/include/intern_alloc.h>
#include <xkern/include/xtime.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>
#include <xkern/include/list.h>



#define HDR		AllocHdrStd
#define HDR_TYPE	ALLOC_HDR_TYPE_STD
#define HDR_SIZE	ALLOC_HDR_SIZE_STD


/* 
 * The stored block sizes reflect useful user data size (i.e., does
 * not include the Block overhead).
 */
typedef struct {
    u_int		size;
    u_int		excessCount;
    struct list_head	free;
} BlockList;


/* 
 * Estimate number of distinct block sizes in the best-fit list for
 * large contiguous buffers.  
 */
#define LARGE_BLOCK_MAP_SIZE	20

typedef struct {
    /* 
     * The 'blocks' list is used for best-fit
     */
    BlockList	blocks[ALLOC_MAX_BLOCKS];
    /* 
     * largeBlockMap is used to keep track of blocks which exceed the
     * maximum best-fit size.
     */
    Map		largeBlockMap;
    u_int	max;
} MyState;
#define	maxBlock(_state) ((_state)->blocks[(_state)->max])


static u_int
getMax( Allocator self )
{
    return allocInteriorGetMax(self->lower, self);
}


static void
dispList( BlockList *l )
{
    list_entry_t	e;
    int			i = 0;

    for ( e = l->free.head; e != 0; e = e->next ) {
	i++;
    }
    xTrace3(alloc, TR_EVENTS, "list (size %d) has %d elements, excessCount %d",
	    l->size, i, l->excessCount);
}


static __inline__ HDR *
getFromFreeList( BlockList *l )
{
    list_entry_t	e;

    e = delist_head_strong(&l->free);
    xIfTrace(alloc, TR_FULL_TRACE) {
	dispList(l);
    }
    if ( e ) {
	return (HDR *)((char *)e - offsetof(HDR, private));
    } else {
	return 0;
    }
}


static void
putOnFreeList( BlockList *l, HDR *h )
{
    enlist_head(&l->free, (list_entry_t)&h->private);
    xIfTrace(alloc, TR_FULL_TRACE) {
	dispList(l);
    }
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


typedef enum { BL_CREATE, BL_NO_CREATE } CreateFlag;


static BlockList *
newBlockList( u_int size, MyState *state )
{
    BlockList	*l;

    xTrace1(alloc, TR_EVENTS, "creating new block list for size %d", size);
    if ( (l = xSysAlloc(sizeof(BlockList))) == 0 ) {
	return 0;
    }
    l->size = size;
    l->excessCount = 0;
    list_init(&l->free);
    if ( mapBind(state->largeBlockMap, &size, l, 0) != XK_SUCCESS ) {
	allocFree(l);
	return 0;
    }
    return l;
}


static __inline__ BlockList *
findBlockList( u_int size, MyState *state, CreateFlag flag )
{
    int		i;
    BlockList	*l;
    
    for ( i=0; state->blocks[i].size != 0; i++ ) {
	if ( size <= state->blocks[i].size ) {
	    return &state->blocks[i];
	}
    }
    /* 
     * Requested size is too large for the standard blocks. 
     */
    if ( mapResolve(state->largeBlockMap, &size, &l) == XK_SUCCESS ) {
	return l;
    }
    if ( flag == BL_NO_CREATE ) {
	return 0;
    }
    return newBlockList(size, state);
}



/* 
 * Reserve space for 'nMsgs' contiguous blocks with user-data size of
 * 'reqSize', and pre-allocate them.  
 */
static xkern_return_t
minContig( Allocator self, int nMsgs, u_int *reqSize )
{
    MyState	*state = self->state;
    BlockList	*l;
    int		i;
    u_int	len;
    HDR		*h;

    xTrace3(alloc, TR_FULL_TRACE,
	    "guar alloc [%s] minContig, %d bytes, %d msgs",
	    self->name, *reqSize, nMsgs);
    if ( (l = findBlockList(*reqSize, state, BL_CREATE)) == 0 ) {
	return XK_FAILURE;
    }
    len = l->size + HDR_SIZE;
    if ( allocInteriorAdjustMin(self->lower, self, &len, nMsgs) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    if ( nMsgs > 0 ) {
	for ( i=0; i < nMsgs; i++ ) {
	    len = l->size + HDR_SIZE;
	    h = allocInteriorGet(self->lower, self, &len);
	    if ( h == 0 ) {
		while ( i-- ) {
		    h = getFromFreeList(l);
		    xAssert(h);
		    allocInteriorPut(self->lower, self, h, h->len);
		}
		len = l->size + HDR_SIZE;
		allocInteriorAdjustMin(self->lower, self, &len, - nMsgs);
		return XK_FAILURE;
	    }
	    h->len = len;
	    h->alloc = self;
	    putOnFreeList(l, h);
	}
    } else {
	for ( i=0; i < - nMsgs; i++ ) {
	    if ( (h = getFromFreeList(l)) == 0 ) {
		xTrace0(alloc, TR_EVENTS,
			"Couldn't remove return-reservation from free list");
		xTrace1(alloc, TR_EVENTS, "free list excess count increases by %d",
			- nMsgs - i);
		l->excessCount += ( - nMsgs - i );
		break;
	    } else {
		allocInteriorPut(self->lower, self, h, h->len);
	    }
	}
    }
    *reqSize = l->size;
    return XK_SUCCESS;
}


static void *
expandList( Allocator self, BlockList *l )
{
    u_int	len;
    HDR		*h;

    xTrace1(alloc, TR_EVENTS, "alloc [%s] -- nothing on the free list",
	    self->name);
    len = l->size + HDR_SIZE;
    if ( h = allocInteriorGet(self->lower, self, &len) ) {
	xTrace3(alloc, TR_SOFT_ERRORS,
		"alloc [%s] allocating excess block (%d bytes req, %d)",
		self->name, l->size + HDR_SIZE, len);
	l->excessCount++;
	h->len = len;
	h->alloc = self;
	return h;
    } else {
	xTrace2(alloc, TR_SOFT_ERRORS, "alloc [%s] -- Attempt to allocate"
		" excess block (%d bytes) failed.",
		self->name, l->size + HDR_SIZE);
	return 0;
    }
}


static __inline__ void *
getFromList( Allocator self, u_int *length, BlockList *l )
{
    u_int	len;
    HDR		*h;

    xTrace2(alloc, TR_FULL_TRACE, "guar alloc [%s] getFromList, %d bytes",
	    self->name, *length);
    if ( (h = getFromFreeList(l)) == 0 ) {
	if ( (h = expandList(self, l)) == 0 ) {
	    return 0;
	}
    } else {
	xTrace1(alloc, TR_MORE_EVENTS, "alloc [%s] -- returning from free list",
		self->name);
    }
    xTrace2(alloc, TR_MORE_EVENTS, "alloc [%s] returns %d bytes",
	    self->name, l->size);
    /* 
     * Even if the actual buffer length (which we record in h->len) is
     * larger, we want to report the length that was reserved.  Reporting
     * the larger length can mess with guarantees.
     */
    *length = l->size;
    /* 
     * Borrow private field to store list pointer so we don't have to
     * look it up when this block is returned. 
     */
    h->private = l;
    return ((char *)h) + HDR_SIZE;
}


static void *
get( Allocator self, u_int *length )
{
    BlockList *l;

    if ( (l = findBlockList(*length, self->state, BL_CREATE)) == 0 ) {
	return 0;
    }
    return getFromList(self, length, l);
}



static void *
getPartial( Allocator self, u_int *length, u_int minLength )
{
    MyState	*state = self->state;
    BlockList	*l;

    xTrace2(alloc, TR_FULL_TRACE, "guar alloc [%s] getPartial (%d)",
	    self->name, *length);
    if ( minLength > maxBlock(state).size ) {
	return 0;
    }
    if ( *length > maxBlock(state).size ) {
	l = &maxBlock(state);
    } else {
	if ( (l = findBlockList(*length, state, BL_NO_CREATE)) == 0 ) {
	    return 0;
	}
	xAssert(l);
    }
    return getFromList(self, length, l);
}


static void
put( Allocator self, void *buf )
{
    BlockList	*l;
    HDR		*h;

    xAssert(buf);
    h = (HDR *)((char *)buf - HDR_SIZE); 
    xAssert(h->alloc == self);
    l = h->private;
    xAssert( l->size <= h->len - HDR_SIZE );
    xTrace3(alloc, TR_FULL_TRACE, "guar alloc [%s] free (x%x, %d)",
	    self->name, buf, h->len);
    if ( l->excessCount ) {
	l->excessCount--;
	xTrace1(alloc, TR_EVENTS, "guarAllocFree -- releasing excess (new val %d)",
		l->excessCount);
	allocInteriorPut(self->lower, self, h, h->len);
    } else {
	xTrace0(alloc, TR_MORE_EVENTS, "guarAllocFree -- returning to free list");
	putOnFreeList(l, h);
    }
}


static int
mfeDispList( void *key, void *val, void *arg )
{
    dispList((BlockList *)val);
    return MFE_CONTINUE;
}


static void
dumpStats( Allocator a )
{
    MyState	*s = a->state;
    int		i;

    xTrace1(alloc, TR_ALWAYS, "Statistics for guar alloc [%s]", a->name);
    for ( i=0; s->blocks[i].size != 0; i++ ) {
	dispList(&s->blocks[i]);
    }
    mapForEach(s->largeBlockMap, mfeDispList, 0);
}


/* 
 * A single method vector for all instances of this allocator.
 */
static	AllocMethods	guarExt_methods;


xkern_return_t
guaranteedAlloc_init( Allocator self, u_int blocks[ALLOC_MAX_BLOCKS] )
{
    MyState	*state;

    xTrace0(alloc, TR_FULL_TRACE, "guar alloc init");
    if ( guarExt_methods.u.e.get == 0 ) {
	guarExt_methods.s.xferRequest = xferRequest;
	guarExt_methods.s.xferIncoming = xferIncoming;
	guarExt_methods.s.stats = dumpStats;
	guarExt_methods.u.e.getMax = getMax;
	guarExt_methods.u.e.minPartial = defaultMinPartial;
	guarExt_methods.u.e.min = minContig;
	guarExt_methods.u.e.get = get;
	guarExt_methods.u.e.getPartial = getPartial;
	guarExt_methods.u.e.put = put;
    }
    if ( ! self->lower ) {
	return XK_FAILURE;
    }
    self->class = ALLOC_EXTERIOR;
    self->methods = &guarExt_methods;
    state = self->state = xSysAlloc(sizeof(MyState));
    bzero((char *)state, sizeof(MyState));

    {
	int	j, i;
	
	if ( blocks[0] == 0 ) {
	    xTrace1(alloc, TR_ERRORS, "block allocator %s -- no blocking info",
		    self->name);
	    allocFree(state);
	    return XK_FAILURE;
	}
	xTrace1(alloc, TR_EVENTS, "guar allocator %s using blocks:", self->name);
	for ( i=0, j=0; blocks[i] != 0; i++ ) {
	    if ( blocks[i] <= HDR_SIZE ) {
		xTrace2(alloc, TR_EVENTS,
			"guar allocator %s rejects block size %d, too small",
			self->name, blocks[i]);
		continue;
	    }
	    state->blocks[j].size = blocks[i] - HDR_SIZE;
	    xTrace1(alloc, TR_EVENTS, "    %d", state->blocks[j].size);
	    list_init(&state->blocks[i].free);
	    j++;
	}
	if ( j == 0 ) {
	    allocFree(state);
	    return XK_FAILURE;
	}
	state->blocks[j].size = 0;
	state->max = j-1;
	self->largeBlockSize = maxBlock(state).size;
    }
    state->largeBlockMap = mapCreate(LARGE_BLOCK_MAP_SIZE, sizeof(u_int),
				     xkSystemPath);
    if ( ! state->largeBlockMap ) {
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

