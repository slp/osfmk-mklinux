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
 * idmap.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * idmap.c,v
 * Revision 1.40.3.3.1.2  1994/09/07  04:18:14  menze
 * OSF modifications
 *
 * Revision 1.40.3.3  1994/09/02  21:43:08  menze
 * mapBind and mapVarBind return xkern_return_t
 * Added mapReserve
 *
 * Revision 1.40.3.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.40.1.2  1994/08/10  20:52:10  menze
 * msgForEach didn't support maps with variable-length keys
 *
 * Revision 1.40.1.1  1994/08/04  17:42:36  menze
 * mapCreate(): return 0 for failure instead of -1
 *
 * Revision 1.40  1994/08/04  17:38:16  menze
 *   [ 1994/04/14          menze ]
 *   Added mapVarUnbind
 *
 * Revision 1.39  1994/02/05  00:07:33  menze
 *   [ 1994/01/28          menze ]
 *   assert.h renamed to xk_assert.h
 *
 * Revision 1.38  1993/12/11  00:23:33  menze
 * fixed #endif comments
 *
 * Revision 1.37  1993/12/07  02:25:12  menze
 * generichash was sometimes looking at too many bytes past the end of
 * the key.
 *
 */

#include <xkern/include/domain.h>
#include <xkern/include/upi.h>
#include <xkern/framework/idmap/idmap_internal.h>
#include <xkern/include/assert.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/xk_path.h>

static struct {
    XMapResolveFunc	*resolve;
    XMapBindFunc	*bind;
    XMapUnbindFunc	*unbind;
    XMapRemoveFunc	*remove;
} map_functions[MAX_MAP_KEY_SIZE];

#if XK_DEBUG  
int	traceidmap;
#endif /* XK_DEBUG */

#define MAX_MAP_SIZE 1024


static int	generichash( char *,  int ,  int );
static void	removeList( MapElement * );

static XMapResolveFunc	mgenericresolve;
static XMapBindFunc	mgenericbind;
static XMapUnbindFunc  	mgenericunbind;
static XMapRemoveFunc  	mgenericremove;
static XMapResolveFunc 	mgenericerror;


/*
 * Create and return a new map containing a table with nEntries entries 
 * mapping keys of size keySize to integers
 */

Map
mapCreate(
    int 	nEntries,
    int 	keySize,
    Path	path)
{
    register Map m;
    register int pow2 = 0;
    int entries = nEntries;
    
    xTrace1(idmap, TR_FULL_TRACE, "mapCreate called, %d entries", nEntries);
    xAssert(nEntries);
    while (nEntries) { nEntries >>= 1; pow2++; }
    xAssert(pow2<8*sizeof(int)-1);
    if (entries == 1<<(pow2-1)) nEntries = entries;
    else nEntries = 1<<pow2;
    xAssert(nEntries<=MAX_MAP_SIZE);

    xTrace1(idmap, TR_EVENTS, "mapCreate: actual map size %d\n", nEntries);
    if (keySize > MAX_MAP_KEY_SIZE) {
	return 0;
    }
    if ( (m = (Map)pathAlloc(path, sizeof(*m))) == 0 ) {
	return 0;
    }
    bzero((char *)m, sizeof(struct map));
    m->tableSize = nEntries;
    m->keySize = keySize;
    m->alloc = pathGetAlloc(path, MD_ALLOC);
    m->table = (MapElement **)allocGet(m->alloc, nEntries * sizeof(MapElement *));
    if ( ! m->table ) {
	allocFree(m);
	return 0;
    }
    bzero((char *)m->table, nEntries * sizeof(MapElement *));
    if (keySize > 0) {
      if (map_functions[keySize].resolve != 0) {
	m->resolve = map_functions[keySize].resolve;
	m->bind    = map_functions[keySize].bind   ;
	m->unbind  = map_functions[keySize].unbind ;
	m->remove  = map_functions[keySize].remove;
      } else {
	m->resolve = mgenericresolve;
	m->bind    = mgenericbind   ;
	m->unbind  = mgenericunbind ;
	m->remove  = mgenericremove;
      }    
    }
    else {  /* this is a map with variable length external id's */
      m->resolve = mgenericerror;
      m->bind =  (XMapBindFunc *)mgenericerror;
      m->unbind = mgenericunbind;
      m->remove = mgenericremove;
    }
    return m;
  }


static xkern_return_t mgenericerror( table, extptr, intptr )
     Map table;
     void *extptr;
     void **intptr;
{
  xTrace0(idmap, TR_ERRORS, "variable length map accessed via mapBind or "
	  "mapResolve; use mapVarBind or mapVarResolve");
  Kabort("wrong map function");
  return XK_FAILURE;
}


xkern_return_t
mapVarUnbind( table, ext, len )
    Map table;
    register void *ext;
    int len;
{
    Bind     elem_posn, *prev_elem;
    register char *o_ext;
    int		ind;

    ind = generichash((char *)ext, table->tableSize, len);
    prev_elem = &table->table[ind];
    elem_posn = *prev_elem;
    table->cache = 0;
    while (elem_posn != 0) {
	o_ext = (char *)elem_posn->externalid;
	if (! bcmp(o_ext, ext, len) ) {
	    *prev_elem = elem_posn->next;
	    FREEVARMAPELEM(table, elem_posn);
	    return XK_SUCCESS;
	} 
	prev_elem = &(elem_posn->next);
	elem_posn = elem_posn->next;
    }
    return XK_FAILURE;
}


xkern_return_t
mapVarBind ( table, ext, len, intern, bp )
    Map table;
    register void *ext;
    int len;
    void *intern;
    Bind *bp;
{
    Bind     elem_posn, new_elem, prev_elem, *table_posn;
    register char *o_ext;
    int		ind;
    
    ind = generichash((char *)ext, table->tableSize, len);
    table_posn = &table->table[ind];

    elem_posn = *table_posn;
    prev_elem = elem_posn;
    while (elem_posn != 0) {
	o_ext = elem_posn->externalid;
	if (!bcmp(o_ext, (char *)ext, len)) {
	    if ( bp ) {
		*bp = elem_posn;
	    }
	    if (elem_posn->internalid == intern) {
		return XK_SUCCESS;
	    } else {
		return XK_ALREADY_BOUND;
	    }
	} else {
	    prev_elem = elem_posn;
	    elem_posn = elem_posn->next;
	}
    }
    
    /* Note: Memory allocated here that is not immediately freed  cjt */
    /*
     * Elements must be inserted at the end of the list rather than at the
     * beginning for the semantics mapForEach to work properly.
     */
    GETVARMAPELEM(table, new_elem, len);
    if ( new_elem == 0 ) {
	return XK_NO_MEMORY;
    }
    bcopy((char *)ext, (char *)new_elem->externalid, len);
    new_elem->internalid = intern;
    new_elem->next = 0;
    new_elem->elmlen = len;
    table->cache = new_elem;
    if ( prev_elem == 0 ) {
	*table_posn = new_elem;
    } else {
	prev_elem->next = new_elem;
    }
    if ( bp ) {
	*bp = new_elem;
    }
    return XK_SUCCESS;
}


xkern_return_t
mapVarResolve ( table, ext, len, resPtr )
    Map table;
    register void *ext;
    int len;
    register void **resPtr;
{
    register Bind elem_posn;
    register char *o_ext;
    
    if ((elem_posn = table->cache) && elem_posn->elmlen == len) {
	o_ext = elem_posn->externalid;
	if (!bcmp(o_ext, (char *)ext, len)) {
	    if ( resPtr ) {
		*resPtr = elem_posn->internalid;
	    }
	    return XK_SUCCESS;
	}
    }
    elem_posn = table->table[generichash(ext, table->tableSize, len)];
    while (elem_posn != 0) {
	o_ext = elem_posn->externalid;
	if (elem_posn->elmlen == len &&
	    !bcmp(o_ext, (char *)ext, len)) {
	    table->cache = elem_posn;
	    if ( resPtr ) {
		*resPtr = elem_posn->internalid;
	    }
	    return XK_SUCCESS;
	} else {
	    elem_posn = elem_posn->next;
	}
    }
    return XK_FAILURE;
}

static void
removeList( e )
    MapElement	*e;
{
    MapElement	*next;

    while ( e != 0 ) {
	xTrace1(idmap, TR_FULL_TRACE, "mapClose removing element %x", (int)e);
	next = e->next;
	allocFree(e);
	e = next;
    }
}


void
mapClose(m)
    Map m;
{
    int		i;

    xTrace1(idmap, TR_MAJOR_EVENTS, "mapClose of map %x", m);
    /* 
     * Free any elements still bound into the map
     */
    for ( i=0; i < m->tableSize; i++ ) {
	removeList(m->table[i]);
    }
    /* 
     * Remove freelist
     */
    removeList(m->freelist);
    allocFree(m->table);
    allocFree(m);
}


xkern_return_t
mapReserve( Map m, int n )
{
    int	i;

    if ( m->keySize <= 0 ) {
	return XK_FAILURE;
    }
    if ( n >= 0 ) {
	for ( i=0; i < n; i++ ) {
	    if ( reserveMapElem(m) == XK_FAILURE ) {
		for ( ; i ; i-- ) {
		    releaseMapElem(m);
		}
		return XK_FAILURE;
	    }
	}
	m->reserved += n;
	return XK_SUCCESS;
    }
    xAssert( n < 0 );
    if ( - n > m->reserved ) {
	return XK_FAILURE;
    }
    m->reserved += n;
    while ( n ) {
	releaseMapElem(m);
	n++;
    }
    return XK_SUCCESS;
}


void
mapForEach(m, f, arg)
    Map m;
    MapForEachFun f;
    void *arg;
{
    char	key[MAX_MAP_KEY_SIZE];
    MapElement	*elem, *prevElem;
    int 	i;
    int		userResult = 0;

    xTrace0(idmap, TR_FULL_TRACE, "mapForEach called");
    for ( i = 0; i < m->tableSize; i++ ) {
	prevElem = 0;
	do {
	    /* 
	     * Set elem to first item in the bucket
	     */
	    elem = m->table[i];
	    if ( prevElem != 0 ) {
		/*
		 * Set elem to the next element after the old elem in the
		 * same hash bucket.  If the previous element has been
		 * removed we will skip the rest of this bucket.  If
		 * the previous element has been removed and
		 * reinserted (with a possibly different key/value),
		 * we will skip it and everything before it in the
		 * list. 
		 */
		for ( ; elem != 0 ; elem = elem->next ) {
		    if ( prevElem == elem ) {
			elem = elem->next;
			break;
		    }
		}
		if ( userResult & MFE_REMOVE ) {
		    m->remove(m, prevElem);
		}
		prevElem = 0;
	    }
	    if ( elem != 0 ) {
		bcopy((char *)elem->externalid, key,
		      m->keySize > 0 ? m->keySize : elem->elmlen);
		userResult = f(key, elem->internalid, arg);
		if ( ! (userResult & MFE_CONTINUE) ) {
		    return;
		}
		prevElem = elem;
	    }
	} while ( elem != 0 );
    } 
}


static void
dumpKey( int size, u_char *p )
{
    int i;

    xTrace0(idmap, TR_DETAILED, "external key: ");
    for ( i=0; i < size; i++ ) {
	printf("%x ", p[i]);
    }
    printf("\n");
}




#ifdef XK_USE_INLINE
#  define INLINE	__inline__
#else
#  define INLINE
#endif


static INLINE int	hash2( void *, int );
static INLINE int	hash4( void *, int );
static INLINE int	hash6( void *, int );
static INLINE int	hash8( void *, int );
static INLINE int	hash10( void *, int );
static INLINE int	hash12( void *, int );
static INLINE int	hash16( void *, int );


#define GENCOMPBYTES(s1, s2) (!bcmp(s1, s2, table->keySize))
#define GENCOPYBYTES(s1, s2) bcopy(s2, s1, table->keySize)

#define KEYSIZE 2
#define BINDNAME m2bind
#define REMOVENAME m2remove
#define RESOLVENAME m2resolve
#define UNBINDNAME m2unbind
static INLINE int
hash2(K, tblSize)
    void	*K;
    int		tblSize;
{
    u_int	h = 0;
    u_char      h0, h1;

    h0 = *(u_char *)K;
    h1 = *((u_char *)K+1);
    h = (((h0<<2) + h1) ^ ((h1<<2) + h0)) % tblSize;
    xTrace1(idmap, TR_FULL_TRACE, "2 byte key optimized hash returns %d",
	    h);
    return h;
}
#define HASH(K, tblSize, keySize)        hash2(K, tblSize)
#define COMPBYTES(s1, s2) (((char *)(s1))[0] == ((char *)(s2))[0] && 	\
			   ((char *)(s1))[1] == ((char *)(s2))[1])
#define COPYBYTES(s1, s2) {((char *)(s1))[0] = ((char *)(s2))[0];	\
			   ((char *)(s1))[1] = ((char *)(s2))[1];}


#include <xkern/framework/idmap/idmap_templ.c>

#define KEYSIZE 4
#define BINDNAME m4bind
#define REMOVENAME m4remove
#define RESOLVENAME m4resolve
#define UNBINDNAME m4unbind
static INLINE int
hash4(K, tblSize)
    void	*K;
    int		tblSize;
{
    register u_char	h0=*(u_char *)K,
                        h1=*((u_char *)K+1),
                        h2=*((u_char *)K+2),
			h3=*((u_char *)K+3);
    u_int	h;
#if	XK_DEBUG
    unsigned long k = h0|(h1<<8)|(h2<<16)|(h3<<24);
#endif

    h = ((h0+(h2<<2)) ^ (h1+(h3<<1)) ^ (h2+(h0<<1)) ^ (h3+h1<<2)) % tblSize;

    xTrace4(idmap, TR_DETAILED, "4 byte key optimized hash of %x (at %x) returns %d in table size %d",
	    k, (K), h, tblSize);
    return h;
  }

#define HASH(K, tblSize, keySize)        hash4(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
   ( *(int *)(s1) == (*(int *)(s2))) :			\
   GENCOMPBYTES((s1), (s2)) )

#define COPYBYTES(s1, s2) {				\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {		\
    	*(int *)(s1) = (*(int *)(s2)); 			\
  } else {						\
	GENCOPYBYTES(s1,s2);				\
  }							\
}

#include <xkern/framework/idmap/idmap_templ.c>

#define KEYSIZE 6
#define BINDNAME m6bind
#define REMOVENAME m6remove
#define RESOLVENAME m6resolve
#define UNBINDNAME m6unbind
static INLINE int
hash6(K, tblSize)
    void	*K;
    int		tblSize;
{
    u_int	h = 0;
    register    u_char	*k = (u_char *)K;

    h = hash4(k,tblSize) ^ hash2(k+4,tblSize);
    
    xTrace1(idmap, TR_FULL_TRACE, "6 byte key optimized hash returns %d",
		h);
    return h;
}

#define HASH(K, tblSize, keySize) 	hash6(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
   ( (*(int *)(s1)) == (*(int *)(s2)) &&		\
     (*((short *)(s1)+2)) == (*((short *)(s2)+2)) ) :	\
   GENCOMPBYTES((s1), (s2)) )

#define COPYBYTES(s1, s2) {				\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {		\
    	*(int *)(s1) = (*(int *)(s2)); 			\
	*((short *)(s1)+2) = *((short *)(s2)+2); 	\
  } else {						\
	GENCOPYBYTES(s1,s2);				\
  }							\
}


#include <xkern/framework/idmap/idmap_templ.c>

#define KEYSIZE 8
#define BINDNAME m8bind
#define REMOVENAME m8remove
#define RESOLVENAME m8resolve
#define UNBINDNAME m8unbind
static INLINE int
hash8( K, tblSize )
    void	*K;
    int		tblSize;
{
    u_int	h;
    register    u_char	*k = (u_char *)K;

    h = hash4(k,tblSize) ^ hash4(k+4,tblSize);

    xTrace1(idmap, TR_FULL_TRACE, "8 byte key optimized hash returns %d",
		h);
    return h;
}
#define HASH(K, tblSize, keySize)	hash8(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	 (*((int *)(s1)+1)) == (*((int *)(s2)+1))) :	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}


#include <xkern/framework/idmap/idmap_templ.c>

#define KEYSIZE 10
#define BINDNAME m10bind
#define REMOVENAME m10remove
#define RESOLVENAME m10resolve
#define UNBINDNAME m10unbind
static INLINE int
hash10(K, tblSize)
    void	*K;
    int		tblSize;
{
    u_int	h;
    register    u_char	*k = (u_char *)K;

    h = hash4(k,tblSize) ^ hash4(k+4,tblSize) ^ hash2(k+8,tblSize);
    
    xTrace1(idmap, TR_FULL_TRACE, "10 byte key optimized hash returns %d",
	    h);
    return h;
}
#define HASH(K, tblSize, keySize) 	hash10(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
   ( (*(int *)(s1)) == (*(int *)(s2)) &&		\
     (*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
     (*((short *)(s1)+4)) == (*((short *)(s2)+4)) ) :	\
   GENCOMPBYTES((s1), (s2)) )

#define COPYBYTES(s1, s2) {				\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {		\
    	*(int *)(s1) = (*(int *)(s2)); 			\
	*((int *)(s1)+1) = *((int *)(s2)+1); 		\
	*((short *)(s1)+4) = *((short *)(s2)+4); 	\
  } else {						\
	GENCOPYBYTES(s1,s2);				\
  }							\
}

#include <xkern/framework/idmap/idmap_templ.c>



#define KEYSIZE 12
#define BINDNAME m12bind
#define REMOVENAME m12remove
#define RESOLVENAME m12resolve
#define UNBINDNAME m12unbind
static INLINE int
hash12( K, tblSize )
    void	*K;
    int		tblSize;
{
    u_int	h;
    register    u_char	*k = (u_char *)K;

    h = hash4(k,tblSize) ^ hash4(k+4,tblSize) ^ hash4(k+8,tblSize);
    xTrace1(idmap, TR_FULL_TRACE, "12 byte key optimized hash returns %d",
	    h);
    return h;
}	
#define HASH(K, tblSize, keySize)	hash12(K, tblSize)


#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2))) : 	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include <xkern/framework/idmap/idmap_templ.c>


#define KEYSIZE 16
#define BINDNAME m16bind
#define REMOVENAME m16remove
#define RESOLVENAME m16resolve
#define UNBINDNAME m16unbind
static INLINE int
hash16( K, tblSize )
    void	*K;
    int		tblSize;
{
    u_int	h;
    register    u_char	*k = (u_char *)K;

    h = hash4(k,tblSize) ^ hash4(k+4,tblSize) ^ hash4(k+8,tblSize)
        ^ hash4(k+12,tblSize);
    xTrace1(idmap, TR_FULL_TRACE, "16 byte key optimized hash returns %d",
		h);
	return h;
}	
#define HASH(K, tblSize, keySize)	hash16(K, tblSize)


#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2)) && 	\
	(*((int *)(s1)+3)) == (*((int *)(s2)+3))) : 	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
	*((int *)(s1)+3) = *((int *)(s2)+3); 	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include <xkern/framework/idmap/idmap_templ.c>

static int
generichash(key, tableSize, keySize)
    char *key;
    int tableSize;
    int keySize;
{
    unsigned int	hash = 0;
    int	howmanylongs = keySize / 4;

    xTrace2(idmap, TR_FULL_TRACE,
	    "generic idmap hash -- tableSize %d, keySize %d",
	    tableSize, keySize);
    while (howmanylongs--) {
      hash ^= hash4(key, tableSize);
      key += 4;
    }
    if (keySize & 0x2) { hash ^= hash2(key, tableSize); key += 2;}
    if (keySize & 0x1)  hash ^=  *(u_char *)key;
    hash &= (tableSize -1);
    xTrace1(idmap, TR_FULL_TRACE, "generic hash returns %d", hash % tableSize);
    return hash % tableSize;
}

#define KEYSIZE generic
#define BINDNAME mgenericbind
#define REMOVENAME mgenericremove
#define RESOLVENAME mgenericresolve
#define UNBINDNAME mgenericunbind
#define HASH(K, tblSize, keySize) generichash(K, tblSize, keySize)
#define GENHASH(K, T, keySize) generichash(K, tblSize, keySize)
#define COMPBYTES(s1, s2) (!bcmp(s1, s2, table->keySize))
#define COPYBYTES(s1, s2) bcopy(s2, s1, table->keySize)

#include <xkern/framework/idmap/idmap_templ.c>

xkern_return_t
map_init()
{
#define KEYSIZE 2
#define BINDNAME m2bind
#define REMOVENAME m2remove
#define RESOLVENAME m2resolve
#define UNBINDNAME m2unbind
#include <xkern/framework/idmap/idmap_init.c>
#define KEYSIZE 4
#define BINDNAME m4bind
#define REMOVENAME m4remove
#define RESOLVENAME m4resolve
#define UNBINDNAME m4unbind
#include <xkern/framework/idmap/idmap_init.c>
#define KEYSIZE 6
#define BINDNAME m6bind
#define REMOVENAME m6remove
#define RESOLVENAME m6resolve
#define UNBINDNAME m6unbind
#include <xkern/framework/idmap/idmap_init.c>
#define KEYSIZE 8
#define BINDNAME m8bind
#define REMOVENAME m8remove
#define RESOLVENAME m8resolve
#define UNBINDNAME m8unbind
#include <xkern/framework/idmap/idmap_init.c>
#define KEYSIZE 10
#define BINDNAME m10bind
#define REMOVENAME m10remove
#define RESOLVENAME m10resolve
#define UNBINDNAME m10unbind
#include <xkern/framework/idmap/idmap_init.c>
#define KEYSIZE 12
#define BINDNAME m12bind
#define REMOVENAME m12remove
#define RESOLVENAME m12resolve
#define UNBINDNAME m12unbind
#include <xkern/framework/idmap/idmap_init.c>
#define KEYSIZE 16
#define BINDNAME m16bind
#define REMOVENAME m16remove
#define RESOLVENAME m16resolve
#define UNBINDNAME m16unbind
#include <xkern/framework/idmap/idmap_init.c>

    return XK_SUCCESS;
}
