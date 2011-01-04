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
 * idmap.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * idmap.h,v
 * Revision 1.24.1.3.1.2  1994/09/07  04:18:00  menze
 * OSF modifications
 *
 * Revision 1.24.1.3  1994/09/02  21:43:08  menze
 * mapBind and mapVarBind return xkern_return_t
 * Added mapReserve
 *
 * Revision 1.24.1.2  1994/09/01  04:10:38  menze
 * Subsystem initialization functions can fail
 * mapCreate takes an Allocator
 *
 * Revision 1.24  1994/08/04  17:49:17  menze
 *   [ 1994/04/14          menze ]
 *   Added mapVarUnbind
 *
 */

#ifndef idmap_h
#define idmap_h

#include <xkern/include/xtype.h>


#define MAX_MAP_KEY_SIZE	100


typedef struct mapelement {
    struct mapelement	*next;
    int			elmlen;
    void		*internalid;
    void		*externalid;
} MapElement, *Bind;


typedef xkern_return_t	(XMapResolveFunc)( Map, void *, void ** );
typedef xkern_return_t	(XMapBindFunc)( Map, void *, void *, Bind * );
typedef xkern_return_t	(XMapRemoveFunc)( Map, Bind );
typedef xkern_return_t	(XMapUnbindFunc)( Map, void * );


typedef struct map {
    int			tableSize, keySize, reserved;
    MapElement		*cache;
    MapElement		*freelist;
    MapElement		**table;
    XMapResolveFunc	*resolve;
    XMapBindFunc	*bind;
    XMapRemoveFunc	*remove;
    XMapUnbindFunc	*unbind;
    Allocator 		alloc;
} Map_s;


#define mapResolve(_map, _key, _valPtr)		\
	(_map)->resolve(_map, (void *)_key, (void **)_valPtr)

#define mapBind(_map, _key, _value, _bp)	\
  	((_map)->bind(_map, (void *)_key, (void *)(_value), (_bp)))

#define mapRemoveBinding(_map, _binding)	\
	(_map)->remove((_map), (_binding))

#define mapUnbind(_map, _key)			\
	(_map)->unbind((_map), (void *)(_key))

#define mapKeySize(_map) ((_map)->keySize)

/* 
 * msgForEach return value flags
 */
#define MFE_CONTINUE	1
#define MFE_REMOVE	2

typedef	int	(*MapForEachFun)( void *key, void *value, void *arg );

extern void 		mapClose( Map );
extern Map 		mapCreate( int tableSize, int keySize, Path );
extern void		mapForEach( Map, MapForEachFun, void * );
extern xkern_return_t 	mapVarResolve(Map, void *, int, void **);
extern xkern_return_t	mapVarBind(Map, void *, int, void *, Bind *);
extern xkern_return_t	mapVarUnbind(Map, void *, int );
extern xkern_return_t	mapReserve(Map, int);

/* 
 * map_init is not part of the public map interface.  It is to be
 * called once at xkernel initialization time.
 */
extern xkern_return_t	map_init( void );

#endif /* idmap_h */
