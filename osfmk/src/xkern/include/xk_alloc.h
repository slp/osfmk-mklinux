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
 * xk_alloc.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * xk_alloc.h,v
 * Revision 1.2.1.3.1.2  1994/09/07  04:18:03  menze
 * OSF modifications
 *
 * Revision 1.2.1.3  1994/09/01  04:14:45  menze
 * Subsystem initialization functions can fail
 * allocAdjustMin* becomes allocReserve*
 * Added allocGetwithSize and modified allocGet
 * Added PLATFORM_SPECIFIC allocator type
 *
 * Revision 1.2.1.2  1994/07/27  19:42:06  menze
 * Allocators no longer have ID numbers
 *
 * Revision 1.2.1.1  1994/07/21  23:00:20  menze
 * Allocators present only a "raw memory" interface.  All Msg and MNode
 * references have been removed.
 *
 * Revision 1.2  1994/05/17  18:32:41  menze
 * Interior alloc interface is now more block-oriented
 * new allocator type: block allocator
 * various interface changes
 *
 * Revision 1.1  1994/03/22  00:24:43  menze
 * Initial revision
 */

#ifndef xk_alloc_h
#define xk_alloc_h


#include <xkern/include/upi.h>
#include <xkern/include/platform.h>
#include <xkern/include/msg_s.h>
#include <xkern/include/xtype.h>
#include <xkern/include/assert.h>

/* 
 * There are two types of allocators, exterior allocators and interior
 * allocators.  Exterior allocators are the ones seen by protocols and
 * device drivers, whereas interior allocators communicate only with
 * other allocators (and possibly with a host-system allocation module.)  
 */


/* 
 * Allocators which do blocking (i.e., round up allocation requests to
 * convenient sizes) are constrained to (ALLOC_MAX_BLOCKS - 1)
 * different sizes.
 */
#define ALLOC_MAX_BLOCKS	(10 + 1)	/* Includes zero termination */

typedef struct {
    void	*private;
    u_int	len;
    Allocator	alloc;
    char	*buf[1];
} AllocHdrStd;

#define ALLOC_HDR_SIZE_STD	offsetof(AllocHdrStd, buf)


/* 
 * Type definitions for allocator methods
 */
typedef	u_int
( * AllocGetMaxFunc ) ( Allocator );

typedef	u_int
( * AllocInteriorGetMaxFunc ) ( Allocator, Allocator );

typedef	xkern_return_t
( * AllocXferRequestFunc ) ( Allocator, void *, u_int );

typedef	xkern_return_t
( * AllocXferIncomingFunc ) ( Allocator, void *, u_int );

typedef	xkern_return_t
( * AllocAdjustMinFunc ) ( Allocator, int, u_int * );

typedef	xkern_return_t
( * AllocAdjustMinPartialFunc ) ( Allocator, int, u_int *, u_int,  u_int * );

typedef void *
( * AllocGetFunc ) ( Allocator, u_int * );

typedef void *
( * AllocGetPartialFunc ) ( Allocator, u_int *, u_int );

typedef void
( * AllocPutFunc ) ( Allocator, void * );

typedef	xkern_return_t
( * AllocXferFunc ) ( Allocator, Allocator, Allocator, void *, u_int );

typedef	xkern_return_t
( * AllocInteriorAdjustMinFunc ) ( Allocator, Allocator, u_int *, int );

typedef	void *
( * AllocInteriorGetFunc ) ( Allocator, Allocator, u_int * );

typedef	void
( * AllocInteriorPutFunc ) ( Allocator, Allocator, void *, u_int );

typedef	xkern_return_t
( * AllocConnectFunc ) ( Allocator, Allocator );

typedef	xkern_return_t
( * AllocDisconnectFunc ) ( Allocator, Allocator );

typedef xkern_return_t
( * AllocMaxChangesFunc ) ( Allocator, u_int );

typedef void
( * AllocStatFunc ) ( Allocator );

typedef xkern_return_t
( * AllocBlockSizesFunc ) ( Allocator, u_int buf[ALLOC_MAX_BLOCKS] );  


/* 
 * Methods supplied by both exterior and interior allocators
 */
typedef struct {
    AllocXferRequestFunc	xferRequest;
    AllocXferIncomingFunc	xferIncoming;
    AllocStatFunc		stats;
} SharedMethods;

typedef struct {
    AllocGetMaxFunc		getMax;
    AllocAdjustMinFunc		min;
    AllocAdjustMinPartialFunc	minPartial;
    AllocPutFunc		put;
    AllocGetFunc		get;
    AllocGetPartialFunc		getPartial;
} ExteriorMethods;

typedef struct {
    AllocXferFunc		handleXfer;
    AllocInteriorAdjustMinFunc	minBlocks;
    AllocInteriorGetFunc	get;
    AllocInteriorPutFunc	put;
    AllocInteriorGetMaxFunc	getMax;
    AllocConnectFunc		connect;
    AllocDisconnectFunc		disconnect;
    AllocMaxChangesFunc		maxChanges;
    AllocBlockSizesFunc		blockSizes;
} InteriorMethods;

typedef struct {
    SharedMethods s;
    union {
	ExteriorMethods	e;
	InteriorMethods	i;
    } u;
} AllocMethods;

typedef enum { ALLOC_EXTERIOR, ALLOC_INTERIOR } AllocClass;

typedef enum {
    ALLOC_HDR_TYPE_STD
} AllocHdrType;

#define	MAX_ALLOC_NAME_LEN	80

typedef struct allocator {
    AllocHdrType	hdrType;
    char		name[MAX_ALLOC_NAME_LEN];
    AllocClass		class;
    u_int		largeBlockSize;	/* exterior only */
    struct allocator	*lower;
    void		*state;		/* Local state info */
    void		*lstate;	/* maintained by self->lower */
    AllocMethods	*methods;
} Allocator_s;



extern	Allocator	xkBootStrapAllocator;
extern	Allocator	xkDefaultAllocator;
extern	Allocator	xkDefaultMsgAllocator;
extern	Allocator	xkSystemAllocator;


#define xSysAlloc(_size) \
    (allocGet(xkSystemAllocator, (_size)))


/* 
 * --------------------------------------------------------------------
 * Allocator interface presented to protocols
 *
 * In these descriptions, 'min' refers to the amount of storage
 * reserved by an external allocator on behalf of its users.
 * --------------------------------------------------------------------
 */

/* 
 * Adjusts the value of 'min' for this allocator by an appropriate
 * amount to satisfy 'nBuffs' calls to allocGet with parameter '*size'.
 * On successful return, *size is set to the actual size of the
 * reserved buffer (which will be >= the original value of '*size'.)
 * A negative value of 'nBuffs' will release a previous reservation.
 * This operation will fail if there are insufficient resources to
 * satisfy an increase in 'min' or if an attempt is made to release a
 * reservation which was not actually made.
 */
xkern_return_t	allocReserve( Allocator, int nBuffs, u_int *size );

/* 
 * Adjusts the value of 'min' for this allocator by an appropriate
 * amount to satisfy 'nBuffs' iterations over allocGetPartial with
 * parameter 'size'.  If the user needs to prepend (or append) a
 * header to each block returned by allocGetPartial, that amount is
 * specified as 'blockOverhead' (and should probably be used as
 * minSize in the allocGetPartial calls.)  On successful return,
 * '*numBlocks' is set to the number of iterations on allocGetPartial
 * that will be necessary to completely satisfy each allocation, and
 * '*size' is set to the total length of all buffers, including
 * 'blockOverhead'.  A negative value of 'nBuffs' will release a
 * previous reservation.  This operation will fail if there are
 * insufficient resources to satisfy an increase in 'min' or if an
 * attempt is made to release a reservation which was not actually
 * made.
 */
xkern_return_t	allocReservePartial( Allocator, int nBuffs, u_int *size,
				     u_int blockOverhead,
				     u_int *numBlocks );

#ifndef ALLOC_DONT_USE_INLINE
/* 
 * Allocates a contiguous buffer of 'size' bytes.  A null pointer is
 * returned in case of failure.
 */
static __inline__ void *
allocGet( Allocator a, u_int size )
{
    xAssert( a->class == ALLOC_EXTERIOR );
    return a->methods->u.e.get(a, &size);
}



/* 
 * Allocates a contiguous buffer of size '*size' bytes.  If the
 * allocation succeeds, '*size' is set to the actual size of the
 * buffer, and the buffer pointer is returned.  A null pointer is
 * returned in case of failure.
 */
static __inline__ void *
allocGetWithSize( Allocator a, u_int *size )
{
    xAssert( a->class == ALLOC_EXTERIOR );
    return a->methods->u.e.get(a, size);
}


/* 
 * Allocates a contiguous buffer.  '*size' bytes are requested,
 * although the allocator may return any buffer larger than 'minSize', 
 * possibly only partially satisfying this request.  If the allocation
 * succeeds, '*size' is set to the actual size allocated, and a
 * pointer to the buffer is returned.  A null pointer is returned in
 * case of failure.
 */
static __inline__ void *
allocGetPartial( Allocator a, u_int *size, u_int minSize )
{
    xAssert( a->class == ALLOC_EXTERIOR );
    if ( *size < minSize ) {
	*size = minSize;
    }
    return a->methods->u.e.getPartial(a, size, minSize);
}


/* 
 * Free the buffer previously returned by this allocator.  The
 * allocator used must be using the ALLOC_HDR_TYPE_STD header.
 */
static __inline__ void
allocFree( void *buf )
{
    AllocHdrStd	*h;

    xAssert(buf);
    h = (AllocHdrStd *)(((char *)buf) - ALLOC_HDR_SIZE_STD);
    xAssert( h->alloc && h->alloc->class == ALLOC_EXTERIOR );
    h->alloc->methods->u.e.put(h->alloc, buf);
}


/* 
 * Free the buffer previously returned by an allocator.  
 */
static __inline__ void
allocPut( Allocator a, void *buf )
{
    xAssert ( a && a->class == ALLOC_EXTERIOR );
    a->methods->u.e.put(a, buf);
}

#endif /* ALLOC_DONT_USE_INLINE */

/* 
 * Returns the maximum size argument to allocGetPartial (or allocGet)
 * that could possibly succeed for this allocator. 
 */
u_int	allocGetMax( Allocator );


/* 
 * Call-back function used below.
 */
typedef  void	(* AllocNotifyFunc)( Allocator, void *object );

/* 
 * Register this object to be notified whenever the allocator system
 * feels particularly concerned about space.  
 */
xkern_return_t	allocRegister( Allocator, AllocNotifyFunc, void *object );

xkern_return_t	allocUnregister( Allocator, void *object );



/* 
 * Return the allocator object with this name, null if no such
 * allocator exists.
 */
Allocator	getAllocByName( char * );


/* 
 * Dump statistics for all allocators in the system.
 */
void		allocDumpStatsAll( void );

/* 
 * Dump statistics for this allocator.
 */
/* void allocDumpStats( Allocator ); */
#define allocDumpStats( _a ) do { 	\
    if ((_a)->methods->s.stats) 	\
      (_a)->methods->s.stats(_a);	\
  } while(0)




/* 
 * --------------------------------------------------------------------
 * Allocator interface functions exported to other xkernel modules
 * --------------------------------------------------------------------
 */

/* 
 * Called at system initialization time.  The allocator module must be
 * initialized before any drivers or protocols.
 */
xkern_return_t	allocInit( void );

xkern_return_t	allocBootStrap( void );

/* 
 * The exact semantics of allocXfer haven't been worked out yet.
 */
xkern_return_t	allocXfer( Allocator from, Allocator to, void *buf, u_int len );

/* 
 * Create a new allocator with the indicated name, type, and backing
 * allocator.
 */
Allocator	allocCreate( char *name, char *type, Allocator lower );


#endif /* ! xk_alloc_h */

