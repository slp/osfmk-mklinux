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
 * idmap_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * idmap_internal.h,v
 * Revision 1.8.1.3.1.2  1994/09/07  04:18:15  menze
 * OSF modifications
 *
 * Revision 1.8.1.3  1994/09/02  21:43:33  menze
 * Added mapReserve
 *
 * Revision 1.8.1.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.8  1994/08/04  17:55:09  menze
 *   [ 1994/04/14          menze ]
 *   Added mapVarUnbind
 *
 */

#define	GETMAPELEM(map, elem) { 					\
    if ((elem = (map)->freelist) == 0) { 				\
	elem = (MapElement*)allocGet(map->alloc, ROUND4 		\
	    (ROUND4(sizeof(MapElement))+(map)->keySize)); 		\
        if ( elem ) { 							\
            elem->externalid =						\
	       (void *)(ROUND4((unsigned int)elem+sizeof(MapElement)));	\
 	} 								\
    } else { 								\
        (map)->freelist = (elem)->next; 				\
    } 									\
}

#define	GETVARMAPELEM(map, elem, size) { 				\
    if ((elem = (map)->freelist) == 0) { 				\
        elem = (MapElement*)allocGet(map->alloc, ROUND4 		\
		(ROUND4(sizeof(MapElement))+size)); 			\
        if ( elem ) { 							\
            elem->externalid =						\
	       (void *)(ROUND4((unsigned int)elem+sizeof(MapElement))); \
	} 								\
    } else { 								\
        (map)->freelist = (elem)->next; 				\
    } 									\
}

static xkern_return_t
reserveMapElem( Map m )
{
    MapElement	*elem;

    elem = (MapElement*)allocGet(m->alloc,
				 ROUND4(ROUND4(sizeof(MapElement)) +
					m->keySize));
    if ( ! elem ) { 
	return XK_FAILURE;
    }
    elem->externalid = (void *)(ROUND4((u_int)elem + sizeof(MapElement)));
    elem->next = m->freelist;
    m->freelist = elem;
    return XK_SUCCESS;
}


static void
releaseMapElem( Map m )
{
    MapElement	*elem;

    if ( ! (elem = m->freelist) ) {
	return;
    }
    m->freelist = elem->next;
    allocFree(elem);
}


#define	FREEVARMAPELEM(map, elem)        allocFree(elem)

#define	FREEIT(map, elem) {			\
    (elem)->next = (map)->freelist;		\
    (map)->freelist = (elem);			\
}
