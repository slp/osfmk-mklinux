/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *	File:	queue.h
 *
 *	Type definitions for generic queues.
 */

#ifndef	_OSFMACH3_QUEUE_H_
#define _OSFMACH3_QUEUE_H_

#include <osfmach3/macro_help.h>

/*
 *	Queue of abstract objects.  Queue is maintained
 *	within that object.
 *
 *	Supports fast removal from within the queue.
 *
 *	How to declare a queue of elements of type "foo_t":
 *		In the "*foo_t" type, you must have a field of
 *		type "queue_chain_t" to hold together this queue.
 *		There may be more than one chain through a
 *		"foo_t", for use by different queues.
 *
 *		Declare the queue as a "queue_t" type.
 *
 *		Elements of the queue (of type "foo_t", that is)
 *		are referred to by reference, and cast to type
 *		"queue_entry_t" within this module.
 */

/*
 *	A generic doubly-linked list (queue).
 */

struct queue_entry {
	struct queue_entry	*next;		/* next element */
	struct queue_entry	*prev;		/* previous element */
};

typedef struct queue_entry	*queue_t;
typedef	struct queue_entry	queue_head_t;
typedef	struct queue_entry	queue_chain_t;
typedef	struct queue_entry	*queue_entry_t;

/*
 *	enqueue puts "elt" on the "queue".
 *	dequeue returns the first element in the "queue".
 */

#define enqueue(queue,elt)	enqueue_tail(queue, elt)
#define dequeue(queue)		dequeue_head(queue)

#if	!defined(__GNUC__) || _NO_INLINE_QUEUE
extern void		enqueue_head();
extern void		enqueue_tail();
extern queue_entry_t	dequeue_head();
extern queue_entry_t	dequeue_tail();
#endif
#if 0
#if	!defined(__GNUC__) || _NO_INLINE_QUEUE
extern queue_entry_t	remque();
extern void		insque();
#endif
#endif

/*
 *	Macro:		queue_init
 *	Function:
 *		Initialize the given queue.
 *	Header:
 *		void queue_init(q)
 *			queue_t		q;	// MODIFIED
 */
#define queue_init(q)	((q)->next = (q)->prev = q)

/*
 *	Macro:		queue_first
 *	Function:
 *		Returns the first entry in the queue,
 *	Header:
 *		queue_entry_t queue_first(q)
 *			queue_t	q;		// IN 
 */
#define queue_first(q)	((q)->next)

/*
 *	Macro:		queue_next
 *	Header:
 *		queue_entry_t queue_next(qc)
 *			queue_t qc;
 */
#define queue_next(qc)	((qc)->next)

/*
 *	Macro:		queue_end
 *	Header:
 *		boolean_t queue_end(q, qe)
 *			queue_t q;
 *			queue_entry_t qe;
 */
#define queue_end(q, qe)	((q) == (qe))

#define queue_empty(q)		queue_end((q), queue_first(q))

/*
 *	Macro:		queue_enter
 *	Header:
 *		void queue_enter(q, elt, type, field)
 *			queue_t q;
 *			<type> elt;
 *			<type> is what's in our queue
 *			<field> is the chain field in (*<type>)
 */
#define queue_enter(head, elt, type, field)			\
MACRO_BEGIN							\
	if (queue_empty((head))) {				\
		(head)->next = (queue_entry_t) elt;		\
		(head)->prev = (queue_entry_t) elt;		\
		(elt)->field.next = head;			\
		(elt)->field.prev = head;			\
	}							\
	else {							\
		register queue_entry_t prev;			\
								\
		prev = (head)->prev;				\
		(elt)->field.prev = prev;			\
		(elt)->field.next = head;			\
		(head)->prev = (queue_entry_t)(elt);		\
		((type)prev)->field.next = (queue_entry_t)(elt);\
	}							\
MACRO_END

/*
 *	Macro:		queue_insert_after
 *	Function:
 *		Insert <elt> after <cur> in <head> queue.
 */
#define queue_insert_after(head, cur, elt, type, field)		\
MACRO_BEGIN							\
	register queue_entry_t next;				\
								\
	next = (cur)->field.next;				\
	if ((head) == next)					\
		(head)->prev = (queue_entry_t)(elt);		\
	else							\
		((type)next)->field.prev = (queue_entry_t)(elt);\
	(elt)->field.next = next;				\
	(elt)->field.prev = (queue_entry_t)(cur);		\
	(cur)->field.next = (queue_entry_t)(elt);		\
MACRO_END

/*
 *	Macro:		queue_insert_before
 *	Function:
 *		Insert <elt> before <cur> in <head> queue.
 */
#define queue_insert_before(head, cur, elt, type, field)	\
MACRO_BEGIN							\
	register queue_entry_t prev;				\
								\
	prev = (cur)->field.prev;				\
	if ((head) == prev)					\
		(head)->next = (queue_entry_t)(elt);		\
	else							\
		((type)prev)->field.next = (queue_entry_t)(elt);\
	(elt)->field.next = (queue_entry_t)(cur);		\
	(elt)->field.prev = prev;				\
	(cur)->field.prev = (queue_entry_t)(elt);		\
MACRO_END

/*
 *	Macro:		queue_field [internal use only]
 *	Function:
 *		Find the queue_chain_t (or queue_t) for the
 *		given element (thing) in the given queue (head)
 */
#define queue_field(head, thing, type, field)			\
		(((head) == (thing)) ? (head) : &((type)(thing))->field)

/*
 *	Macro:		queue_remove
 *	Header:
 *		void queue_remove(q, qe, type, field)
 *			arguments as in queue_enter
 */
#define queue_remove(head, elt, type, field)			\
MACRO_BEGIN							\
	register queue_entry_t	next, prev;			\
								\
	next = (elt)->field.next;				\
	prev = (elt)->field.prev;				\
								\
	queue_field((head), next, type, field)->prev = prev;	\
	queue_field((head), prev, type, field)->next = next;	\
MACRO_END

/*
 *	Macro:		queue_assign
 */
#define queue_assign(to, from, type, field)			\
MACRO_BEGIN							\
	((type)((from)->prev))->field.next = (to);		\
	((type)((from)->next))->field.prev = (to);		\
	*to = *from;						\
MACRO_END

#define queue_remove_first(h, e, t, f)				\
MACRO_BEGIN							\
	e = (t) queue_first((h));				\
	queue_remove((h), (e), t, f);				\
MACRO_END

#define queue_remove_last(h, e, t, f)				\
MACRO_BEGIN							\
	e = (t) queue_last((h));				\
	queue_remove((h), (e), t, f);				\
MACRO_END

/*
 *	Macro:		queue_enter_first
 *	Header:
 *		void queue_enter_first(q, elt, type, field)
 *			queue_t q;
 *			<type> elt;
 *			<type> is what's in our queue
 *			<field> is the chain field in (*<type>)
 */
#define queue_enter_first(head, elt, type, field)		\
MACRO_BEGIN							\
	if (queue_empty((head))) {				\
		(head)->next = (queue_entry_t) elt;		\
		(head)->prev = (queue_entry_t) elt;		\
		(elt)->field.next = head;			\
		(elt)->field.prev = head;			\
	}							\
	else {							\
		register queue_entry_t next;			\
								\
		next = (head)->next;				\
		(elt)->field.prev = head;			\
		(elt)->field.next = next;			\
		(head)->next = (queue_entry_t)(elt);		\
		((type)next)->field.prev = (queue_entry_t)(elt);\
	}							\
MACRO_END

/*
 *	Macro:		queue_last
 *	Function:
 *		Returns the last entry in the queue,
 *	Header:
 *		queue_entry_t queue_last(q)
 *			queue_t	q;		// IN
 */
#define queue_last(q)	((q)->prev)

/*
 *	Macro:		queue_prev
 *	Header:
 *		queue_entry_t queue_prev(qc)
 *			queue_t qc;
 */
#define queue_prev(qc)	((qc)->prev)

#if	__GNUC__ && !_NO_INLINE_QUEUE
/* Define fast inline functions for queues */
/*
 *	Insert element at head of queue.
 */
extern __inline__ void enqueue_head(
	register queue_t	que,
	register queue_entry_t	elt)
{
	elt->next = que->next;
	elt->prev = que;
	elt->next->prev = elt;
	que->next = elt;
}

/*
 *	Insert element at tail of queue.
 */
extern __inline__ void enqueue_tail(
	register queue_t	que,
	register queue_entry_t	elt)
{
	elt->next = que;
	elt->prev = que->prev;
	elt->prev->next = elt;
	que->prev = elt;
}

/*
 *	Remove and return element at head of queue.
 */
extern __inline__ queue_entry_t dequeue_head(
	register queue_t	que)
{
	register queue_entry_t	elt;

	if (que->next == que)
		return((queue_entry_t)0);

	elt = que->next;
	elt->next->prev = que;
	que->next = elt->next;
	return(elt);
}

/*
 *	Remove and return element at tail of queue.
 */
extern __inline__ queue_entry_t dequeue_tail(
	register queue_t	que)
{
	register queue_entry_t	elt;

	if (que->prev == que)
		return((queue_entry_t)0);

	elt = que->prev;
	elt->prev->next = que;
	que->prev = elt->prev;
	return(elt);
}

#if 0
/*
 *	Remove arbitrary element from queue.
 *	Does not check whether element is on queue - the world
 *	will go haywire if it isn't.
 */

extern __inline__ void insque(
	register struct queue_entry *entry,
	register struct queue_entry *pred)
{
	entry->next = pred->next;
	entry->prev = pred;
	(pred->next)->prev = entry;
	pred->next = entry;
}

extern __inline__ queue_entry_t remque(
	register queue_entry_t elt)
{
	(elt->next)->prev = elt->prev;
	(elt->prev)->next = elt->next;
	return(elt);
}
#endif

#endif	/* __GNUC__ && !_NO_INLINE_QUEUE */

#endif	/* _OSFMACH3_QUEUE_H_ */
