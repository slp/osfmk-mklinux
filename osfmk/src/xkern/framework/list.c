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
 * list.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:22:03 $
 */

#include <xkern/include/domain.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/trace.h>
#include <xkern/include/list.h>
int tracelist;

/*
 *	Initialize a list
 *       
 */
void 
list_init(
	register struct list_head *list)
{
	xTrace1(list,TR_EVENTS,"list_init: %x",list);
	list->head = list->tail = 0;
}

/*
 *	Insert element at tail of list.
 *      It is safe for this to be interrupted by a head consumer.
 */
void 
enlist_tail(
	register struct list_head *list,
	register list_entry_t	elt)
{
	xTrace4(list,TR_EVENTS,
		"enlist_tail: list %x  head %x  tail %x  elt %x",
		list,list->head,list->tail,elt);
        elt->next = 0;
	if (list->tail) 
	    list->tail->next = elt;
	list->tail = elt;
	if (!list->head) 
	    list->head = elt;
}

/*
 *	Remove and return element at head of list.
 *        It is safe for this to interrupt a tail supplier.
 */
list_entry_t 
delist_head(
	register struct list_head *list)
{
	register list_entry_t	elt;

	xTrace3(list,TR_EVENTS,"delist_head: list %x  head %x  tail %x",
		list,list->head,list->tail);
	/* consumer may not take from a zero or one element list */
	if (!list->head || list->tail == list->head)
	    return((list_entry_t)0);

	elt = list->head;
	list->head = elt->next;
	elt->next = 0;
	xTrace3(list,TR_EVENTS,"delist_head2: list %x  head %x  tail %x",
		list,list->head,list->tail);
	return(elt);
}

/*
 *	Remove and return element at head of list.
 *        This will remove the last element in the list.
 *        It is not safe unless it has sync'ed with the supplier
 */
list_entry_t 
delist_head_strong(
	register list_t	list)
{
	register list_entry_t	elt;

	xTrace3(list,TR_EVENTS,
		"delist_head_strong: list %x  head %x  tail %x",
		list,list->head,list->tail);
	if (!list->head) 	
	    return((list_entry_t)0);

	elt = list->head;
	list->head = elt->next;
	if (!list->head) 
	    list->tail = 0;
	xTrace3(list,TR_EVENTS,
		"delist_head_strong2: list %x  head %x  tail %x",
		list,list->head,list->tail);
	return(elt);
}

/*
 *	Insert element at head of list.
 */
void 
enlist_head(
	register list_t	list,
	register list_entry_t	elt)
{
	xTrace4(list,TR_EVENTS,
		"enlist_head: list %x  head %x  tail %x  elt %x",
		list,list->head,list->tail,elt);
	elt->next = list->head;
	list->head = elt;
}

/*
 *	Remove and return element at tail of list.
 *      Can't be done with singly-linked list.
 */
list_entry_t 
delist_tail(
	register list_t	list)
{
	return((list_entry_t)0);
}
