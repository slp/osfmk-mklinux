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
 * xk_path.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * xk_path.h,v
 * Revision 1.1.2.2.1.2  1994/09/07  04:18:01  menze
 * OSF modifications
 *
 * Revision 1.1.2.2  1994/09/01  04:14:06  menze
 * Subsystem initialization functions can fail
 * *AdjustMin* becomes *Reserve*
 *
 * Revision 1.1.2.1  1994/07/27  19:48:00  menze
 * Definition of PATH_ANY_ID wildcard
 *
 * Revision 1.1  1994/07/21  23:26:36  menze
 * Initial revision
 *
 */


#ifndef xk_path_h
#define xk_path_h

#include <xkern/include/platform.h>
#include <xkern/include/xtype.h>
#include <xkern/include/idmap.h>
#include <xkern/include/xk_thread.h>

/* 
 * Each protocol ought to be able to carve-out part of the PathAllocId
 * space (much as they do in control operation space) if protocols
 * need to add additional allocators to a path.  We just need the
 * common allocators for right now, though.
 */
typedef int	PathAllocId;

enum sysAllocTypes {
    MSG_ALLOC = 0,	/* Message allocator */
    MD_ALLOC,		/* Meta-data allocator */
    LAST_SYS_ALLOC
};

typedef struct path {
    xk_u_int32	id;
    Allocator	sysAlloc[LAST_SYS_ALLOC];
    Map		allocMap;	/* For protocol-specific allocators */
} Path_s;


extern	Path	xkDefaultPath;
extern	Path	xkSystemPath;


#define PATH_ANY_ID	0xffffffff

/* 
 * Create a path with the indicated name and ID (VCI).  If PATH_ANY_ID
 * is used, the system will select an unused ID.  The path will
 * initially have all allocators set to xkDefaultAlloc.  Returns 0 if
 * there were problems.
 */
Path		pathCreate( xk_u_int32 id, char *name );


/* 
 * Set the 'id' allocator for path 'p' to 'a'. 
 */
xkern_return_t	pathSetAlloc( Path p, PathAllocId id, Allocator a );


/* 
 * Return the 'id' allocator for path 'p'.  Returns 0 if no such
 * allocator has been set.
 * 
 * Allocator	pathGetAlloc( Path p, PathAllocId p );
 */
#define pathGetAlloc(path, id) ((id) < LAST_SYS_ALLOC ?			\
					(path)->sysAlloc[id] :		\
					pathGetAlloc_lookup(path, id))

Allocator	pathGetAlloc_lookup( Path, PathAllocId );


/* 
 * xk_u_int32	pathGetId( Path );
 */
#define	pathGetId(p) ((p)->id)


Path		getPathByName( char * );


Path		getPathById( xk_u_int32 );


/* 
 * Cause an input pool to be established for the indicated path.  The
 * user may specify the number of input buffers and threads to be
 * associated with that pool, or allow the driver to select default
 * values by specifying 0.  The 'dev' string may be used to create an
 * input pool for a specific device driver; a 0 value will cause
 * pools to be created for all device drivers.
 */
xkern_return_t	pathEstablishPool(
				  Path,
				  u_int bufs,
				  u_int threads,
				  XkThreadPolicy,
				  char *dev);


/* 
 * This is the type of call-back function the path module uses to
 * tell drivers about input requirements.
 */
typedef	xkern_return_t	(* DevInputFunc )( XObj self, Path,
					  u_int msgSize, int numMsgs );

/* 
 * This is the type of call-back function the path module uses to
 * tell drivers about path requirements.
 */
typedef	xkern_return_t	(* DevPoolFunc )(
					 XObj self,
					 Path,
					 u_int bufs,
					 u_int threads,
					 XkThreadPolicy);

/* 
 * Used by a device driver to register itself with the path module.
 * The path module will use the specified functions to inform the
 * driver about user-specified pool and input-message requirements.
 * Even in the absence of any user-specified information, the path
 * module will call back the device with pool information for the
 * xkDefaultPool. 
 */
void		pathRegisterDriver( XObj, DevPoolFunc, DevInputFunc );


/* 
 * Allocate a contiguous buffer of size 'size' bytes.  A null pointer
 * is returned in case of failure.  This data is allocated from the
 * path's meta-data allocator.
 *
 * void *
 * pathAlloc( Path path, u_int size )
 */
#define pathAlloc( _path, _size ) \
    (allocGet((_path)->sysAlloc[MD_ALLOC], (_size)))


/*
 * Reserve space for 'nBuffs' allocations of 'size' bytes.  The memory
 * should subsequently be allocated with pathAlloc.
 */
xkern_return_t
pathReserve( Path p, int nBuffs, u_int size );
	    

/*
 * Free memory previously allocated with pathAlloc
 * 
 * void
 * pathFree( void *ptr );
 */
#define pathFree( ptr )	\
	allocFree( ptr )


/* 
 * ----------------------------------------------------------------
 * Private interfaces used by other x-kernel subsystems.
 */

/* 
 * Initialize the path subsystem. 
 */
xkern_return_t	pathInit( void );

/* 
 * Called early ininitialization, before pathInit, to provide a
 * bootstrap path for other system allocations.
 */
xkern_return_t	pathBootStrap( void );

/*
 * Does most of the actual work of msgAdjustMinInput.  
 */
xkern_return_t	pathReserveInput( Path, int nMsgs, u_int size, char *dev );


#endif /* ! xk_path_h */
