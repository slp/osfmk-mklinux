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
 * alloc_i.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * alloc_i.h,v
 * Revision 1.2.1.2  1994/09/01  04:25:35  menze
 * Added support for platform-specific allocators
 *
 * Revision 1.2.1.1  1994/07/21  23:48:30  menze
 * Allocators present only a "raw memory" interface.  All Msg and MNode
 * references have been removed.
 *
 * Most allocators use a common memory header format.
 *
 * Revision 1.2  1994/05/17  18:23:50  menze
 * Interior alloc interface is now more block-oriented
 * new allocator type: block allocator
 *
 * Revision 1.1  1994/04/05  22:15:11  menze
 * Initial revision
 *
 */


#ifndef offsetof
#  define offsetof(_type, _member) ((size_t)&((_type *)0)->_member)
#endif

#define MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b) )
#define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b) )

typedef	xkern_return_t (AllocInitFunc)( Allocator,
				        u_int blocks[ALLOC_MAX_BLOCKS] );

xkern_return_t	bootStrapAlloc_init( Allocator );
AllocInitFunc	simpleInternalAlloc_init;
AllocInitFunc	budgetAlloc_init;
AllocInitFunc	blockAlloc_init;
AllocInitFunc	guaranteedAlloc_init;

/* 
 * --------------------------------------------------------------------
 * Allocator interface functions internal to the allocator module
 * --------------------------------------------------------------------
 */

/* 
 * An allocator can use this method to iterate over its own minContig
 * method in order to satisfy a minUltimate request.
 */
xkern_return_t	defaultMinPartial( Allocator, int nMsgs, u_int *reqSize,
				   u_int hdrSize, u_int *numBlocks );

xkern_return_t	platformAllocators( void );

/* 
 * Defines a new type of allocator, and an initialization function to
 * be called when new instances of this allocator type are created.
 */
xkern_return_t	allocAddType( char *type, AllocInitFunc f );

/* 
 * Adjust 'min' for the upper allocator relative to the lower
 * allocator.  Size has requested length passed in, actual block
 * length returned.  N is the number of blocks.
 */
xkern_return_t	allocInteriorAdjustMin( Allocator lower, Allocator upper,
				        u_int *size, int N ); 

/* 
 * Returns the maximum amount of storage that allocator 'upper' could
 * possibly get back from 'lower'
 */
u_int	allocInteriorGetMax( Allocator lower, Allocator upper );


/* 
 * 'buf' is filled in with a list of block sizes that the lower
 * allocator uses for blocking.  Even interior allocators which don't
 * do blocking should return a single block size which the upper
 * allocators use to break up large Partial requests.  The lengths
 * represent the amount of data the upper allocator can actually use
 * (i.e., they don't include any lower allocator headers, if any.)
 *
 * The block array is terminated with a zero.
 */
xkern_return_t	allocBlockSizes( Allocator, u_int buf[ALLOC_MAX_BLOCKS] );


/* 
 * Tries to allocate a block of memory of at least size '*size'.  On
 * successful return, '*size' is set to the actual usable length of
 * the block and the buffer is returned.  Returns null in case of
 * failure. 
 */
static __inline__ void *
allocInteriorGet( Allocator a, Allocator upper, u_int *size )
{
    xAssert ( a && a->class == ALLOC_INTERIOR );
    return a->methods->u.i.get(a, upper, size);
}


/* 
 * Returns a block of memory previously allocated with allocInteriorGet.
 */
static __inline__ void
allocInteriorPut( Allocator a, Allocator upper, void *buf, u_int len )
{
    xAssert ( a && a->class == ALLOC_INTERIOR );
    a->methods->u.i.put(a, upper, buf, len);
}


/* 
 * Registers the 'upper' allocator with 'lower'.  This must be done
 * before 'lower' can perform any other operation on 'upper.'
 */
xkern_return_t	allocConnect( Allocator lower, Allocator upper );

/* 
 * Removes a connection previously established with allocConnect.
 */
xkern_return_t	allocDisconnect( Allocator lower, Allocator upper );

/* 
 * Starts call-backs for objects which have registered themselves as
 * interested in this allocator.  Typically invoked by an allocator on
 * behalf of itself.  If a previous series of call-backs is already
 * running for this allocator, this operation has no effect.
 */
void		allocStartCallBacks( Allocator );

/* 
 * Indicates that the allocator should use a new limiting max value.
 * Called on interior allocators only.
 */
xkern_return_t	allocMaxChanges( Allocator, u_int max );


/* 
 * The exact semantics of allocXfer haven't been worked out yet.
 */
/* xkern_return_t	allocXferRequest( Allocator self, void *buf, u_int len ); */
#define allocXferRequest( _a, _buf, _len ) \
  ((_a)->methods->s.xferRequest((_a), (_buf), (_len)))

/* xkern_return_t	allocXferIncoming( Allocator self, void *buf, u_int len ); */
#define allocXferIncoming( _a, _buf, _len ) \
  ((_a)->methods->s.xferIncoming((_a), (_buf), (_len)))


/* 
 * Platform-specific allocator type name.  If the platform-specific
 * allocators register a type of this name, it will be used by the
 * default interior allocator.
 */
#define ALLOC_PLATFORM_TYPE	"plat_alloc_default"
