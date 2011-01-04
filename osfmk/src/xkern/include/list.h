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
 * list.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:24:25 $
 */


/*
 *	A generic singly-linked list
 */

#ifndef list_h
#define list_h

struct list_entry {
	struct list_entry	*next;		/* next element */
};

struct list_head {
	struct list_entry	*head;		/* first element */
	struct list_entry	*tail;		/* last  element */
};

typedef struct list_head	*list_t;
typedef	struct list_entry	*list_entry_t;

/*
 *	enlist puts "elt" on the "list".
 *	delist returns the first element in the "list".
 */

#define enlist(list,elt)	enlist_tail(list, elt)
#define	delist(list)		delist_head(list)

list_entry_t	delist_head( list_t );
list_entry_t	delist_head_strong( list_t );
list_entry_t	delist_tail( list_t );
void 		enlist_head( list_t, list_entry_t );
void 		enlist_tail( list_t, list_entry_t );
void		list_init( list_t );

#endif /* list_h */
