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
 * msg.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * msg.h,v
 * Revision 1.28.3.3.1.2  1994/09/07  04:18:02  menze
 * OSF modifications
 *
 * Revision 1.28.3.3  1994/09/01  04:14:06  menze
 * Subsystem initialization functions can fail
 * *AdjustMin* becomes *Reserve*
 *
 * Revision 1.28.3.2  1994/07/21  23:12:08  menze
 * Constructors use Paths rather than Allocators
 *
 * Message reservation functions (msgAdjustMin*) are now part of the
 * message tool
 *
 * Revision 1.28.3.1  1994/07/21  23:07:30  menze
 *     [ 1994/04/12         hkaram ]
 *     Resource allocation modifications
 *
 */

#ifndef msg_h
#define msg_h

#include <xkern/include/xk_alloc.h>
#include <xkern/include/msg_s.h>
#include <xkern/include/platform.h>

/* constructor... make user data buffer part of the Msg. */
/* The freefunc will be called when the Msg is destroyed. */
typedef void (*MsgCIPFreeFunc)(void *, int, void *);


/* 
 * types of user functions called in ForEach()
 */
typedef boolean_t (*XCharFun)(char *ptr, long len, void *arg);
/*
 * types of load and store functions for headers
 */

/* function to convert the header to network byte order and store it	
 * in a potentially unaligned buffer. 					
 * 'len' is the number of bytes to be read and 'arg' is an arbitrary
 * argument passed through msgPush
 */
typedef void (*MStoreFun)(void *hdr, char *des, long len, void *arg);

/* function to load the header from a potentially unaligned buffer, 	
 * and convert it to machine byte order.
 * 'len' is the number of bytes to be read and 'arg' is an arbitrary
 * argument passed through msgPop
 */
typedef long (*MLoadFun)(void *hdr, char *src, long len, void *arg);

/* Msg operations */

/* initializer; must be called before any Msg instances are created 	*/
/* returns XK_SUCCESS after successful intialization			*/
xkern_return_t msgInit(void);

/************* constructors/destructors **************/

/* construct a Msg from another Msg */
void msgConstructCopy( Msg this, Msg another );

/* 
 * Construct a Msg that has a single contiguous buffer for both stack
 * and data.  
 */
xkern_return_t msgConstructContig( Msg		this, 
				   Path 	path, 
				   u_int 	dataLength,
				   u_int 	stackLength );
/* 
 * Construct a message that has (potentially) many discontiguous
 * buffers, although an initial stack of stackLength will be
 * contiguous.  
 */
xkern_return_t msgConstructUltimate( Msg	this, 	
				     Path	path, 
				     u_int	dataLength,
				     u_int 	stackLength );

/* 
 * Construct a Msg from a user-supplied buffer, which should be of
 * size stackLength + dataLength.  Both stack and data will use the
 * user-supplied buffer.
 */
xkern_return_t	msgConstructInPlace( Msg 	this, 
				     Path	path, 
				     u_int 	dataLength,
				     u_int 	stackLength,
				     char 	*buf, 
				     MsgCIPFreeFunc f, 
				     void 	*arg );

/* destructor */
void msgDestroy(Msg this);


/* 
 * Reserve space in allocator 'alloc' for 'nMsgs' calls to
 * msgConstructContig with these 'dataSize' and 'stackSize' arguments. 
 */
xkern_return_t msgReserveContig( Path path, int nMsgs,
				 u_int dataSize, u_int stackSize );

/* 
 * Reserve space in allocator 'alloc' for 'nMsgs' calls to
 * msgConstructUltimate with these 'dataSize' and 'stackSize' arguments. 
 */
xkern_return_t msgReserveUltimate( Path path, int nMsgs,
				   u_int dataSize, u_int stackSize );

/* 
 * Reserve space in allocator 'alloc' for 'nMsgs' calls to
 * msgConstructInPlace.
 */
xkern_return_t msgReserveInPlace( Path path, int nMsgs );

/* 
 * Reserve space in allocator for nNodes message nodes.  This
 * shouldn't normally be necessary, as the message tool implicitly
 * reserves space for message nodes in the other msgReserve
 * operations.  
 */
xkern_return_t msgReserveNode( Path path, int nNodes );

/*
 * Adjusts the value of 'min' for the allocators on this path by an
 * appropriate amount to buffer 'nMsgs' incoming messages of size
 * 'size'.  The messag tool will communicate with the device drivers
 * to determine exactly what this means in terms of reservations.  A
 * negative value of 'nMsgs' will release a previous reservation.
 * This operation will fail if there are insufficient resources to
 * satisfy an increase in 'min', if an attempt is made to release a
 * reservation which was not actually made, or if no input pools have
 * been established for this path via pathEstablishPool.
 *
 * The 'dev' string can be used to limit this adjustment to a single
 * device driver.  A value of 0 will adjust all devices for which this
 * path has established input pools.
 */
xkern_return_t	msgReserveInput( Path, int nMsgs, u_int size, char *dev );


/************* regular protocols call these **************/
/* assignment */
void msgAssign(Msg this, Msg m);

/* return the current length of the message */
#define msgLen(_mesg) ((_mesg)->tailPtr - (_mesg)->stackHeadPtr)

/* truncate this Msg to length newLength */
void msgTruncate(Msg this, long newLength);

/* remove a chunk of length len from the head of this Msg */
/* and assign it to head; like old break */
void msgChopOff(Msg this, Msg head, long len);

/* assign to this Msg the concatenation of Msg1 and Msg2 */
xkern_return_t msgJoin(Msg this, Msg Msg1, Msg Msg2);

/* push a header onto this Msg */
xkern_return_t msgPush(Msg this, MStoreFun store, void *hdr,
		       long hdrLen, void *arg);

/* pop a header from this Msg 
 * 'arg' is an arbitrary argument which will be passed to 'store'
 * returns TRUE after successful pop
 */
xkern_return_t msgPop(Msg this, MLoadFun load, void *hdr,
		      long hdrLen, void *arg);

/* add a trailer to a message
 * 'arg' is an arbitrary argument which will be passed to 'store'.
 * if a new stack must allocated, it will of size newlength.
 */
xkern_return_t msgAppend(Msg this, MStoreFun store, void *hdr,
			 long hdrLen, void *arg, long newlength);

/* pop and discard an object of length len */
/* returns TRUE after successful pop */
boolean_t msgPopDiscard(Msg this, long len);

/* perform housecleaning to free unnecessary resources allocated to this msg */
void msgCleanUp(Msg this);

/* Copy fragments of the message into a contiguous buffer.  Buffer must be */
/* long enough */
void msg2buf(Msg this, char *buf);

/*
 * Get a pointer to where data is
 */
boolean_t msgDataPtr( char *ptr, long len, void *arg );


/*
 * associate an attribute with a message
 */
xkern_return_t	msgSetAttr(Msg this, int name, void *attr, int len);


/*
 * retrieve an attribute associated with a message
 */
void *		msgGetAttr(Msg this, int name);

#define msgGetPath(m) ((m)->path)

/************* get to the data in the message **************/
/* for every contig in this Msg, invoke the function f with */
/* arguments const char *buf (=address of contig), long len */
/* (=length of contig), and void *arg (=user-supplied argument), */  
/* while f returns TRUE */
void msgForEach(Msg this, XCharFun f, void *arg);

/* same as above, but: for the data fragments imported with */
/* msgConstructInPlace call fUsr rather than f1             */
void msgForEachAlternate(Msg this, XCharFun f1, XCharFun fUsr, void *arg);

/************* debugging **************/
/* 
 * Display some textual representation of the message
 */
void msgShow(Msg this);

/* 
 * Display statistics about message usage
 */
void msgStats(void);

#endif /* msg_h */
