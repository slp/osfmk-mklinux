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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 * queue.h
 */

#ifndef _RTHREAD_QUEUE_H_
#define _RTHREAD_QUEUE_H_

/*
 * Queues.
 */

typedef struct rthread_queue {
	struct rthread_queue_item *head;
	struct rthread_queue_item *tail;
} *rthread_queue_t;

typedef struct rthread_queue_item {
	struct rthread_queue_item *next;
} *rthread_queue_item_t;

#define	NO_QUEUE_ITEM	((rthread_queue_item_t) 0)

#define	QUEUE_INITIALIZER	{ NO_QUEUE_ITEM, NO_QUEUE_ITEM }


/*
 *  Rthread queue operation MACROS
 */

#define	rthread_queue_init(q)	((q)->head = (q)->tail = 0)

#define	rthread_queue_enq(q, x) 					\
	do {								\
		(x)->next = 0; 						\
		if ((q)->tail == 0) 					\
			(q)->head = (rthread_queue_item_t) (x); 	\
		else 							\
			(q)->tail->next = (rthread_queue_item_t) (x); 	\
		(q)->tail = (rthread_queue_item_t) (x); 		\
	} while (0)

#define	rthread_queue_preq(q, x) 					\
	do { 								\
		if ((q)->tail == 0) 					\
			(q)->tail = (rthread_queue_item_t) (x); 	\
		((rthread_queue_item_t) (x))->next = (q)->head; 	\
		(q)->head = (rthread_queue_item_t) (x); 		\
	} while (0)

#define	rthread_queue_head(q, t)	((t) ((q)->head))

#define	rthread_queue_deq(q, t, x) 					\
    do { 								\
	if (((x) = (t) ((q)->head)) != 0 && 				\
	    ((q)->head = (rthread_queue_item_t) ((x)->next)) == 0) 	\
		(q)->tail = 0; 						\
    } while (0)


#endif /* _RTHREAD_QUEUE_H_ */
