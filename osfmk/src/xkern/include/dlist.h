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
 * dlist.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision 1.1  1994/07/08  05:02:46  menze
 * Initial revision
 *
 */

/* 
 * A doubly-linked list type
 */


typedef struct dlist {
    struct dlist *next;
    struct dlist *prev;
} *dlist_t, *dlist_elem_t;

#define dlist_empty(dl) ((dl)->next == (dl))

#define dlist_init(dl) do {	\
  (dl)->next = (dl)->prev = (dl);	\
} while (0)

/* 
 * insert a after pred
 */
#define dlist_insert(a, pred) do {			\
  ((dlist_elem_t)(a))->next = ((dlist_elem_t)(pred))->next; \
  ((dlist_elem_t)(a))->prev = (dlist_elem_t)(pred); 	\
  ((dlist_elem_t)(pred))->next = (dlist_elem_t)(a); 	\
  ((dlist_elem_t)(a))->next->prev = (dlist_elem_t)(a); 	\
} while (0)

#define dlist_remove(a) do {					\
  ((dlist_elem_t)(a))->prev->next = ((dlist_elem_t)(a))->next; 	\
  ((dlist_elem_t)(a))->next->prev = ((dlist_elem_t)(a))->prev; 	\
} while (0)

#define dlist_first(dl) (dlist_empty(dl) ? 0 : (dl)->next)

#define dlist_iterate(dl, var, type)				\
	for ( (var) = (type)(dl)->next;				\
	      (var) != (type)(dl);				\
	      (var) = (type)((dlist_elem_t)(var))->next )

