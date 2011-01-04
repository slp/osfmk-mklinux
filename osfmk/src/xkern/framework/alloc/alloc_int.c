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
 * alloc_int.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * alloc_int.c,v
 * Revision 1.2.1.3.1.2  1994/09/07  04:18:22  menze
 * OSF modifications
 *
 * Revision 1.2.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.2.1.2  1994/09/01  04:24:15  menze
 * Meta-data allocations now use Allocators and Paths
 * Better default block sizes
 *
 * Revision 1.2.1.1  1994/07/21  23:51:10  menze
 * Slight changes in interior-allocator interface as a result of the
 * extraction of the message-oriented exterior-allocator interface
 *
 * Revision 1.2  1994/05/17  18:24:59  menze
 * Call-backs are working
 * Interior alloc interface is now more block-oriented
 *
 * Revision 1.1  1994/03/22  00:25:24  menze
 * Initial revision
 *
 */

#include <xkern/include/xk_path.h>
#include <xkern/include/xk_alloc.h>
#include <xkern/include/intern_alloc.h>
#include <xkern/include/xtime.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>
#include <xkern/include/dlist.h>

/* 
 * Methods for a most simple internal allocator
 */

/* #define XK_ALLOC_TRACE (XK_DEBUG && XKMACHKERNEL) */
#define XK_ALLOC_TRACE XK_DEBUG

/* 
 * 'free' keeps track of memory that is neither reserved nor being
 * used by clients which have exceeded their allocation.  It may be
 * used for overflowing clients or for new reservations (subject to
 * reserveLimit.) 
 *
 * 'reserved' is the total amount of memory that has been
 * reserved.  reserved <= reserveLimit.
 *
 * If free falls below callBackThreshold, call-backs are started.
 */
typedef struct {
    int		free;
    int		reserved;
    int		reserveLimit;
    int		callBackThreshold;
    int		max;
    XTime	last;	/* last time call-backs were initiated */
    Map		clientMap;
    int		blocks[ALLOC_MAX_BLOCKS];
#if XK_ALLOC_TRACE
    struct dlist	sizeList;
#endif
} PoolState;

typedef struct {
    int min;
    int current;
} ClientState;


#define MY_DEFAULT_MAX	(10 * 1000 * 1000)

static u_int
defaultBlockSizes[ ALLOC_MAX_BLOCKS ] =
{
    200, 1750, 4050, 0
};


/* 
 * Very minimal client checking ... may need to be expanded, since
 * it's an error to call lower allocator operations without first
 * connecting. 
 */
#define CHECK_CLIENT_VALIDITY( _client, _action )	\
  do { if ( _client->lstate == 0 ) { _action ; } } while (0)


#define MIN_CALL_BACK_INTERVAL	15	/* seconds */



#if XK_ALLOC_TRACE

#define SIZE_LIST_GRAN	10

typedef struct {
    struct dlist	dl;
    u_int		alloc;
    u_int		free;
    u_int		high;
    u_int		fail;
    u_int		base;
} SizeInfo;



static void
recordAlloc( Allocator self, void *ptr, u_int size )
{
    PoolState	*ps = self->state;
    SizeInfo	*si, *siPred;
    u_int	base;
    static int	recording = 0;
    
    /* 
     * Avoid recursion 
     */
    if ( recording ) {
	return;
    }
    recording = 1;
    base = (size / SIZE_LIST_GRAN) * SIZE_LIST_GRAN;
    if ( ! dlist_empty(&ps->sizeList) ) {
	dlist_iterate(&ps->sizeList, si, SizeInfo *) {
	    if ( base == si->base ) {
		break;
	    }
	    if ( base < si->base ){
		siPred = (SizeInfo *)si->dl.prev;
		si = 0;
		break;
	    }
	    if ( si == (SizeInfo *)ps->sizeList.prev ) {
		siPred = si;
		si = 0;
		break;
	    }
	}
    } else {
	si = 0;
	siPred = (SizeInfo *)&ps->sizeList;
    }
    if ( si == 0 ) {
	/* 
	 * Need a new element
	 */
	if ( ! (si = allocGet(xkSystemAllocator, sizeof(SizeInfo))) ) {
	    recording = 0;
	    return;
	}
	bzero((char *)si, sizeof(SizeInfo));
	si->base = base;
	dlist_insert(si, siPred);
    }
    if ( ptr ) {
	if ( ++si->alloc - si->free > si->high ) {
	    si->high = si->alloc - si->free;
	}
    } else {
	si->fail++;
    }
    recording = 0;
}


static void
recordFree( Allocator self, u_int size )
{
    PoolState	*ps = self->state;
    SizeInfo	*si;
    u_int	base;

    base = (size / SIZE_LIST_GRAN) * SIZE_LIST_GRAN;
    dlist_iterate(&ps->sizeList, si, SizeInfo *) {
	if ( base == si->base ) {
	    si->free++;
	    return;
	}
    }
}


static void
dumpSizeStats( PoolState *ps )
{
    SizeInfo	*si;

    dlist_iterate(&ps->sizeList, si, SizeInfo *) {
	xTrace5(alloc, TR_ALWAYS,
		"\tsize: %7d  alloc: %5d  current: %5d  high: %5d  failed: %3d",
		si->base + SIZE_LIST_GRAN, si->alloc, 
		si->alloc - si->free, si->high, si->fail);
    }
}

#endif /* XK_ALLOC_TRACE */


static int
dumpClientStats( void *key, void *value, void *arg )
{
    Allocator	client = *(Allocator *)key;
    ClientState	*cs = client->lstate;

    xTrace3(alloc, TR_ALWAYS, "\t\tname: %-15s  min: %8d  current: %8d",
	    client->name, cs->min, cs->current);
    *(int *)arg = 1;
    return MFE_CONTINUE;
}


static void
dumpStats( Allocator self )
{
    PoolState	*ps = self->state;
    int		anyClients = 0;

    xTrace1(alloc, TR_ALWAYS, "\nStatistics for interior pool allocator %s:",
	    self->name);
    if ( self->lower ) {
	xTrace1(alloc, TR_ALWAYS, "   (client of %s)", self->lower->name);
    }
    xTrace2(alloc, TR_ALWAYS, "\t max: %11d        reserved: %11d",
	    ps->max, ps->reserved);
    xTrace2(alloc, TR_ALWAYS, "\tfree: %11d    reserveLimit: %11d",
	    ps->free, ps->reserveLimit);
    xTrace1(alloc, TR_ALWAYS, "\tcallBackThreshold: %11d",
	    ps->callBackThreshold);
    xTrace0(alloc, TR_ALWAYS, "    Clients:");
    mapForEach(ps->clientMap, dumpClientStats, &anyClients);
    if ( ! anyClients ) {
	xTrace0(alloc, TR_ALWAYS, "\tnone");
    }
    xTrace0(alloc, TR_ALWAYS, "");
#if XK_ALLOC_TRACE
    if ( tracealloc >= TR_EVENTS ) {
	xTrace0(alloc, TR_ALWAYS, "    Sizes:");
	dumpSizeStats(ps);
	xTrace0(alloc, TR_ALWAYS, "");
    }
#endif
}



#if XK_DEBUG

static int
sumFreeSpaceUsed( void *key, void *value, void *arg )
{
    Allocator	client = *(Allocator *)key;
    ClientState	*cs = client->lstate;

    *(int *)arg += MAX(cs->current, cs->min);
    return MFE_CONTINUE;
}

static void
checkConsistency( Allocator self )
{
    PoolState	*ps = self->state;
    int		sum = 0;

    mapForEach(ps->clientMap, sumFreeSpaceUsed, &sum);
    if ( ps->free + sum != ps->max ) {
	xTrace0(alloc, TR_ERRORS, "\ninternal allocator accounting inconsistency");
	xTrace3(alloc, TR_ERRORS, "free: %d, client usage sum: %d, max: %d",
		ps->free, sum, ps->max);
	dumpStats(self);
	xAssert(0);
    }
}    
#endif


static u_int
getMax( Allocator self, Allocator client )
{
    PoolState	*ps = self->state;
    ClientState	*cs = client->lstate;

    CHECK_CLIENT_VALIDITY(client, return 0);
    return MAX(cs->min - cs->current, 0) + ps->free;
}


/* 
 * Since we're on the bottom of the allocator stack, no one should be
 * making transfer requests to us.
 */
static xkern_return_t
xferRequest( Allocator self, void *ptr, u_int size )
{
    xError("simpleInternal xferRequest called");
    return XK_FAILURE;
}    

static xkern_return_t
xferIncoming( Allocator self, void *ptr, u_int size )
{
    xError("simpleInternal xferIncoming called");
    return XK_FAILURE;
}    


static xkern_return_t
xferHandler( Allocator self, Allocator from, Allocator to,
			    void *buf, u_int size )
{
    /* 
     * XXX
     */
    return XK_FAILURE;
}


static void
decPoolFree( Allocator self, PoolState *ps, int N )
{
    XTime	now, diff;

    ps->free -= N;
    if ( ps->free < ps->callBackThreshold ) {
	xGetTime(&now);
	xSubTime(&diff, now, ps->last);
	if ( diff.sec >= MIN_CALL_BACK_INTERVAL ) {
	    ps->last = now;
	    allocStartCallBacks(self);
	}
    }
}


static xkern_return_t
adjustMinBlocks( Allocator self, Allocator client,
			        u_int *size, int nBlocks )
{
    ClientState	*cs = client->lstate;
    PoolState	*ps = self->state;
    int		nBytes;
    
    xTrace3(alloc, TR_DETAILED,
	    "internal_adjustMin (client: %s, %u bytes, %d blocks)",
	    client->name, *size, nBlocks);
    CHECK_CLIENT_VALIDITY(client, return XK_FAILURE);
    nBytes = ((int) *size) * nBlocks;
    if ( nBytes <= 0 ) {
	/* 
	 * Returning an existing reservation
	 */
	nBytes = -nBytes;
	if ( cs->min < nBytes ) {
	    xTrace0(alloc, TR_SOFT_ERRORS,
		    "internal_adjustMin: return request exceeds reservations");
	    return XK_FAILURE;
	}
	if ( cs->min > cs->current ) {
	    /* 
	     * Free increases by the amount of this reservation that
	     * isn't being used.
	     */
	    ps->free += MIN(nBytes, cs->min - cs->current);
	}
	ps->reserved -= nBytes;
	cs->min -= nBytes;
	return XK_SUCCESS;
    }
    /* 
     * Requesting an increase in reservation
     */
    if ( nBytes > (ps->reserveLimit - ps->reserved) ) {
	return XK_FAILURE;
    }
    /* 
     * By policy,
     * we ought to be able to reserve this much memory, but there
     * might be squatters in the free space.
     */
    if ( nBytes > ps->free ) {
	/* 
	 * Should we perform call-backs here?
	 */
	return XK_FAILURE;
    }
    /* 
     * If the client is already exceeding its reservation, at least
     * part of the reservation is credited to that difference, and
     * doesn't cut into the pool's free space.
     */
    decPoolFree(self, ps, MAX(nBytes - MAX(cs->current - cs->min, 0), 0));
    ps->reserved += nBytes;
    cs->min += nBytes;
    xIfTrace(alloc, TR_FULL_TRACE) dumpStats(self);
#if XK_DEBUG    
    checkConsistency(self);
#endif
    return XK_SUCCESS;
}


static xkern_return_t
accountDebit( ClientState *cs, Allocator self, u_int N )
{
    PoolState	*ps = self->state;
    int		diff;

    if ( (int)N <= cs->min - cs->current ) {
	/* 
	 * It all comes from pre-reserved space.  Doesn't affect any
	 * statistics for the pool.
	 */
	xTrace0(alloc, TR_DETAILED,
		"accountDebit request satisfied by client reservation");
	cs->current += N;
	return XK_SUCCESS;
    }
    diff = N - MAX(cs->min - cs->current, 0);
    xAssert(diff >= 0);
    xTrace1(alloc, TR_DETAILED,
	    "debit -- reservations insufficient, needs %d bytes of free space",
	    diff);
    if ( ps->free >= diff ) {
	cs->current += N;
	decPoolFree(self, ps, diff);
#if XK_DEBUG    
	checkConsistency(self);
#endif
	return XK_SUCCESS;
    } else {
	return XK_FAILURE;
    }
}


static void
accountCredit( ClientState *cs, Allocator self, u_int N )
{
    PoolState *ps = self->state;

    if ( cs->current > cs->min ) {
	ps->free += MIN(cs->current - cs->min, N);
    }
    cs->current = MAX(cs->current - N, 0);
#if XK_DEBUG    
    checkConsistency(self);
#endif
}


static void *
get( Allocator self, Allocator client, u_int *reqSize )
{
    ClientState	*cs = client->lstate;
    void	*b;
    
    xTrace2(alloc, TR_DETAILED, "internal get (client: %s, %d)",
	    client->name, *reqSize);
    CHECK_CLIENT_VALIDITY(client, return 0);
    if ( accountDebit(cs, self, *reqSize) == XK_FAILURE ) {
	return 0;
    }
    if ( self->lower ) {
	/* 
	 * We ignore any excess memory we might get from the platform
	 * allocator to make the accounting simple.  For a
	 * block-allocator or guaranteed-allocator client, this is
	 * pretty unlikely anyway.  
	 */
	u_int	tmpSize = *reqSize;
	b = allocInteriorGet(self->lower, self, &tmpSize);
    } else {
#if ! XKMACHKERNEL
	b = malloc(*reqSize);
#else
	panic("x-kernel internal allocator -- no lower allocator");
#endif
    }
    if ( b == 0 ) {
	accountCredit(cs, self, *reqSize);
    }
    xIfTrace(alloc, TR_FULL_TRACE) dumpStats(self);
    xTrace1(alloc, TR_DETAILED, "internal get returns %x", b);
#if XK_ALLOC_TRACE
    recordAlloc(self, b, *reqSize);
#endif
    return b;
}


static void
put( Allocator self, Allocator client, void *ptr, u_int len )
{
    xTrace3(alloc, TR_DETAILED, "internal put (client: %s, %x, %d)",
	    client->name, ptr, len);
    CHECK_CLIENT_VALIDITY(client, return);
    accountCredit((ClientState *)client->lstate, self, len);
    if ( self->lower ) {
	allocInteriorPut(self->lower, self, (char *)ptr, len);
    } else {
#if ! XKMACHKERNEL
	free(ptr);
#else
	panic("x-kernel internal allocator -- no lower allocator");
#endif
    }
#if XK_ALLOC_TRACE
    recordFree(self, len);
#endif
    xIfTrace(alloc, TR_FULL_TRACE) dumpStats(self);
}


static xkern_return_t
connect( Allocator self, Allocator client )
{
    PoolState	*ps = self->state;

    xTrace1(alloc, TR_DETAILED, "simpleInternalConnect from client %s",
	    client->name);
    if ( client->lstate ) {
	xTrace0(alloc, TR_SOFT_ERRORS,
		"Warning, simpleInternal_connect overriding existing client lstate");
    }
    client->lstate = xSysAlloc(sizeof(ClientState));
    if ( ! client->lstate ) {
	return XK_FAILURE;
    }
    bzero(client->lstate, sizeof(ClientState));
    if ( mapBind(ps->clientMap, &client, 0, 0) != XK_SUCCESS ) {
	allocFree(client->lstate);
	client->lstate = 0;
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xkern_return_t
disconnect( Allocator self, Allocator client )
{
    PoolState	*ps = self->state;

    if ( client->lstate ) {
	allocFree(client->lstate);
	client->lstate = 0;
    }
    mapUnbind(ps->clientMap, &client);
    return XK_SUCCESS;
}


static xkern_return_t
maxChanges( Allocator self, u_int max )
{
    PoolState	*ps = self->state;
    int		oldMax;

    oldMax = ps->max;
    ps->max = max;
    ps->free += (ps->max - oldMax);
/*
 * kernel cannot do this float conversion
    ps->reserveLimit = 0.75 * ps->max;
    ps->callBackThreshold = 0.10 * ps->max;
 */
    ps->reserveLimit = (ps->max * 9 ) / 10;
    ps->callBackThreshold = ps->max / 10;

    return XK_SUCCESS;
}


/* 
 * This allocator doesn't round up to block sizes.  Default is to just
 * return a size to use when fragmenting large x-kernel messages,
 * although we will propagate user-configured block sizes up to our
 * clients. 
 */
static xkern_return_t
blockSizes( Allocator self, u_int buf[ALLOC_MAX_BLOCKS] )
{
    PoolState	*ps = self->state;

    bcopy((char *)ps->blocks, (char *)buf, ALLOC_MAX_BLOCKS * sizeof(int));
    return XK_SUCCESS;
}


static AllocMethods sInt_methods;


xkern_return_t
simpleInternalAlloc_init( Allocator self, u_int blocks[ALLOC_MAX_BLOCKS] )
{
    PoolState	*ps;
    int		i;

    if ( sInt_methods.u.i.get == 0 ) {
	sInt_methods.s.xferRequest = xferRequest;
	sInt_methods.s.xferIncoming = xferIncoming;
	sInt_methods.s.stats = dumpStats;
	sInt_methods.u.i.getMax = getMax;
	sInt_methods.u.i.handleXfer = xferHandler;
	sInt_methods.u.i.minBlocks = adjustMinBlocks;
	sInt_methods.u.i.get = get;
	sInt_methods.u.i.put = put;
	sInt_methods.u.i.connect = connect;
	sInt_methods.u.i.disconnect = disconnect;
	sInt_methods.u.i.maxChanges = maxChanges;
	sInt_methods.u.i.blockSizes = blockSizes;
    }
    self->class = ALLOC_INTERIOR;
    self->methods = &sInt_methods;
    if ( (ps = xSysAlloc(sizeof(PoolState))) == 0 ) {
	return XK_FAILURE;
    }
    self->state = ps;
    bzero((char *)ps, sizeof(PoolState));
    ps->clientMap = mapCreate(1, sizeof(Allocator), xkSystemPath);
    if ( ! ps->clientMap ) {
	allocFree(ps);
	return XK_FAILURE;
    }
#if XK_ALLOC_TRACE
    dlist_init(&ps->sizeList);
#endif
    {
	u_int max = 0;

	if ( self->lower ) {
	    max = allocInteriorGetMax(self->lower, self);
	}
	if ( max == 0 ) {
	    max = MY_DEFAULT_MAX;
	}
	maxChanges(self, max);
    }
    xTrace1(alloc, TR_EVENTS, "interior allocator %s using blocks:", self->name);
    bcopy((blocks[0] == 0 ? (char *)defaultBlockSizes : (char *)blocks),
	  (char *)ps->blocks, ALLOC_MAX_BLOCKS * sizeof(u_int));
    for ( i=0; ps->blocks[i] != 0; i++ ) {
	xTrace1(alloc, TR_EVENTS, "    %d", ps->blocks[i]);
    }
    return XK_SUCCESS;
}
