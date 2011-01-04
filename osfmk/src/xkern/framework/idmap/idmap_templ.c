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
 * idmap_templ.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * idmap_templ.c,v
 * Revision 1.19.2.3.1.2  1994/09/07  04:18:14  menze
 * OSF modifications
 *
 * Revision 1.19.2.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.19.2.2  1994/09/01  18:55:46  menze
 * Meta-data allocations now use Allocators and Paths
 *
 * Revision 1.19  1993/12/13  20:37:03  menze
 * Modifications from UMass:
 *
 *   [ 93/10/21          nahum ]
 *   Expanded out test for zero to get GCC 2.4.5 to stop whining.
 *
 */

/*
 * We need the definitions of:
 *	KEYSIZE
 *	HASH
 *	COMPBYTES
 *	COPYBYTES
 */

static XMapBindFunc	BINDNAME;
static XMapResolveFunc	RESOLVENAME;
static XMapUnbindFunc	UNBINDNAME;
static XMapRemoveFunc	REMOVENAME;


static xkern_return_t
BINDNAME ( table, ext, intern, bp )
    Map table;
    register void *ext;
    void *intern;
    Bind *bp;
{
    Bind     elem_posn, new_elem, prev_elem, *table_posn;
    register char *o_ext;
    int		ind;
    
    ind = HASH((char *)ext, table->tableSize, table->keySize);
    table_posn = &table->table[ind];

    xTrace3(idmap, TR_DETAILED, "mapBind map %lx, bucket %d, intern %lx",
	    (u_long)table, ind, (u_long)intern);
    xIfTrace(idmap, TR_DETAILED) dumpKey(table->keySize, ext);
    elem_posn = *table_posn;
    prev_elem = elem_posn;
    while (elem_posn != 0) {
	o_ext = elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    if (elem_posn->internalid == intern) {
		if ( bp ) {
		    *bp = elem_posn;
		}
		xTrace0(idmap, TR_DETAILED, "redundant bind");
		return XK_SUCCESS;
	    } else {
		xTrace1(idmap, TR_DETAILED, 
			"bind error, key already bound to %lx",
			(u_long)elem_posn->internalid);
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
    xTrace0(idmap, TR_DETAILED, "New bind");
    GETMAPELEM(table, new_elem);
    if ( new_elem == 0 ) {
	return XK_NO_MEMORY;
    }
    COPYBYTES((char *)new_elem->externalid, (char *)ext);
    new_elem->internalid = intern;
    new_elem->next = 0;
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


static xkern_return_t
RESOLVENAME ( table, ext, resPtr )
    Map table;
    register void *ext;
    register void **resPtr;
{
    register Bind elem_posn;
    register char *o_ext;
    

   xTrace1(idmap, TR_DETAILED, "mapResolve map %lx", (u_long)table);
   xIfTrace(idmap, TR_DETAILED) dumpKey(table->keySize, ext);
   if ((elem_posn = table->cache) != (Bind) 0) {
	o_ext = elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    if ( resPtr ) {
		*resPtr = elem_posn->internalid;
	    }
	    xTrace0(idmap, TR_DETAILED, "Found key in cache");
	    return XK_SUCCESS;
	}
    }
    elem_posn = table->table[HASH(ext, table->tableSize, table->keySize)];
    while (elem_posn != 0) {
	o_ext = elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    table->cache = elem_posn;
	    if ( resPtr ) {
		*resPtr = elem_posn->internalid;
	    }
	    xTrace0(idmap, TR_DETAILED, "resolve succeeds");
	    return XK_SUCCESS;
	} else {
	    elem_posn = elem_posn->next;
	}
    }
    xTrace0(idmap, TR_DETAILED, "resolve fails");
    return XK_FAILURE;
}


/* 
 * UNBIND -- remove an entry based on the key->value pair
 */
static xkern_return_t
UNBINDNAME ( table, ext )
    Map table;
    register void *ext;
{
    Bind     elem_posn, *prev_elem;
    register char *o_ext;
    
    prev_elem = &table->table[HASH((char *)ext, table->tableSize,
				   table->keySize)];
    elem_posn = *prev_elem;
    table->cache = 0;
    while (elem_posn != 0) {
	o_ext = (char *)elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    *prev_elem = elem_posn->next;
	    FREEIT(table, elem_posn);
	    return XK_SUCCESS;
	} 
	prev_elem = &(elem_posn->next);
	elem_posn = elem_posn->next;
    }
    return XK_FAILURE;
}


/* 
 * REMOVE -- remove an entry based on the binding
 */
static xkern_return_t
REMOVENAME ( table, bind )
    Map table;
    Bind bind;
{
    Bind     elem_posn, *prev_elem;
    
    prev_elem = &table->table[HASH((char *)bind->externalid, table->tableSize,
				   table->keySize)];
    elem_posn = *prev_elem;
    table->cache = 0;
    while (elem_posn != 0) {
	if ( elem_posn == bind ) {
	    *prev_elem = elem_posn->next;
	    FREEIT(table, elem_posn);
	    return XK_SUCCESS;
	} 
	prev_elem = &(elem_posn->next);
	elem_posn = elem_posn->next;
    }
    return XK_FAILURE;
}



#undef BINDNAME
#undef REMOVENAME
#undef RESOLVENAME
#undef UNBINDNAME
#undef FIRSTNAME
#undef KEYSIZE
#undef HASH
#undef COMPBYTES
#undef COPYBYTES
