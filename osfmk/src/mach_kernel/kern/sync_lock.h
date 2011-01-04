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
 *	File:	kern/sync_lock.h
 *	Author:	Joseph CaraDonna
 *
 *	Contains RT distributed lock synchronization service definitions.
 */

#ifndef _KERN_SYNC_LOCK_H_
#define _KERN_SYNC_LOCK_H_

#include <kern/macro_help.h>
#include <kern/thread.h>

typedef struct ulock {
	queue_chain_t	thread_link; /* ulocks owned by a thread  	    */
	queue_chain_t	held_link;   /* ulocks held in the lock set	    */
	queue_chain_t	handoff_link;/* ulocks w/ active handoff operations */
 
	decl_mutex_data(,lock) 	     /* ulock lock			    */

	struct lock_set *lock_set;   /* the retaining lock set		    */	
	thread_act_t	owner;	     /* thread activation that owns the lock*/
	boolean_t	stable;	     /* stable state		            */	
	int		state;	     /* current lock state		    */
	thread_t	ho_thread;   /* handoff thread			    */

	int		blocked_count;	  /* # of threads blocked on ulock  */
	queue_head_t	blocked_threads;  /* queue of blocked threads	    */
} Ulock;

typedef struct ulock *ulock_t;

typedef struct lock_set {
	queue_chain_t	task_link;   /* chain of lock sets owned by a task  */
	decl_mutex_data(,lock)	     /* lock set lock			    */
	task_t		owner;	     /* task that owns the lock set	    */
	ipc_port_t	port;	     /* lock set port			    */
	int		ref_count;   /* reference count			    */

	boolean_t	active;	     /* active status			    */
	int		policy;	     /* wakeup policy for blocked threads   */
	int		n_ulocks;    /* number of ulocks in the lock set    */

	queue_head_t	held_ulocks;	/* list of currently held ulocks    */
	queue_head_t	handoff_ulocks;	/* list of ulocks currently causing
					 * a thread to block due to a
					 * hand-off operation.
					 */

	struct ulock	ulock_list[1];	/* ulock group list place holder    */
} Lock_Set;

typedef struct lock_set *lock_set_t;


#define LOCK_SET_NULL	((lock_set_t) 0)
#define ULOCK_NULL	((ulock_t) 0)

#define ULOCK_FREE	0
#define ULOCK_HELD	1

#define LOCK_OPERATION_ABORTED(th)   ((th)->wait_link.prev != (queue_entry_t) 0)
#define LOCK_OPERATION_COMPLETE(th)  ((th)->wait_link.prev  = (queue_entry_t) 0)

/*
 *  Data structure internal lock macros
 */

#define lock_set_lock_init(ls)		mutex_init(&(ls)->lock, \
						   ETAP_THREAD_LOCK_SET)
#define lock_set_lock(ls)		mutex_lock(&(ls)->lock)
#define lock_set_unlock(ls)		mutex_unlock(&(ls)->lock)

#define ulock_lock_init(ul)		mutex_init(&(ul)->lock, \
						   ETAP_THREAD_ULOCK)
#define ulock_lock(ul)			mutex_lock(&(ul)->lock)
#define ulock_unlock(ul)		mutex_unlock(&(ul)->lock)


/*
 *	Forward Declarations
 */

extern	kern_return_t	lock_set_create	     	(task_t task,
						 lock_set_t *new_lock_set,
						 int n_ulocks,
						 int policy);

extern	kern_return_t	lock_set_destroy     	(task_t task,
						 lock_set_t lock_set);

extern	kern_return_t	lock_acquire   	     	(lock_set_t lock_set,
						 int lock_id);

extern	kern_return_t	lock_release	     	(lock_set_t lock_set,
						 int lock_id);

extern	kern_return_t	lock_try	     	(lock_set_t lock_set,
						 int lock_id);

extern	kern_return_t	lock_make_stable	(lock_set_t lock_set,
						 int lock_id);	       

extern  kern_return_t	lock_make_unstable	(ulock_t ulock,
						 thread_act_t thr_act);

extern	kern_return_t	lock_release_internal	(ulock_t ulock,
						 thread_act_t thr_act);

extern	kern_return_t	lock_handoff		(lock_set_t lock_set,
						 int lock_id);

extern	kern_return_t	lock_handoff_accept	(lock_set_t lock_set,
						 int lock_id);

extern	void		lock_set_reference	(lock_set_t lock_set);
extern	void		lock_set_dereference	(lock_set_t lock_set);

#endif /* _KERN_SYNC_LOCK_H_ */
