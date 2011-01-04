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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 *	File:	hash.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm hash routine routines.
 */

/* XXX ? */
#define	void	int

#include "xmm_hash.h"

char *malloc();

/* XXX should be insert and delete, not queue and dequeue!!! */

typedef struct xmm_hashelt	*xmm_hashelt_t;

struct xmm_hashelt {
	unsigned long	key;
	void *		elt;
	xmm_hashelt_t	next;
};

struct xmm_hash {
	unsigned long	mask;
	xmm_hashelt_t	*h;
};

xmm_hash_t
xmm_hash_allocate(size)
	unsigned long size;
{
	xmm_hash_t hash;

	hash = (xmm_hash_t) malloc(sizeof(*hash)
				   + size * sizeof(struct xmm_hashelt));
	hash->h = (xmm_hashelt_t *) (hash + 1);
	hash->mask = size - 1;
	return hash;
}

void
xmm_hash_deallocate(hash)
	xmm_hash_t hash;
{
	int i;
	xmm_hashelt_t he, *hep;

	for (i = 0; i <= hash->mask; i++) {
		for (hep = &hash->h[i]; he = *hep; ) {
			hep = &he->next;
			free((char *) he);
		}
	}
	free((char *) hash);
}

void
xmm_hash_enqueue(hash, elt, key)
	xmm_hash_t hash;
	void *elt;
	unsigned long key;
{
	xmm_hashelt_t he, *hep;

	hep = &hash->h[key & hash->mask];
	he = (xmm_hashelt_t) malloc(sizeof(*he));
	he->key = key;
	he->elt = elt;
	he->next = *hep;
	*hep = he;
}

void *
xmm_hash_dequeue(hash, key)
	xmm_hash_t hash;
	unsigned long key;
{
	xmm_hashelt_t he, *hep;
	void *elt;

	for (hep = &hash->h[key & hash->mask]; he = *hep; hep = &he->next) {
		if (he->key == key) {
			elt = he->elt;
			*hep = he->next;
			free((char *) he);
			return elt;
		}
	}
	return (void *) 0;
}

void *
xmm_hash_lookup(hash, key)
	xmm_hash_t hash;
	unsigned long key;
{
	xmm_hashelt_t he, *hep;

	for (hep = &hash->h[key & hash->mask]; he = *hep; hep = &he->next) {
		if (he->key == key) {
			return he->elt;
		}
	}
	return (void *) 0;
}
