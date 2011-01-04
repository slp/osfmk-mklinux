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
 *	File:	kern/sync_sema.h
 *	Author:	Joseph CaraDonna
 *
 *	Contains RT distributed semaphore synchronization service definitions.
 */

#ifndef _KERN_SYNC_SEMA_H_
#define _KERN_SYNC_SEMA_H_

#include <kern/thread.h>

typedef struct semaphore {
	queue_chain_t	task_link;   /* chain of semaphores owned by a task */
	decl_simple_lock_data(,lock) /* semaphore lock			    */	
	task_t		owner;	     /* task that owns semaphore            */
	ipc_port_t	port;	     /* semaphore port			    */
	int		ref_count;   /* reference count			    */
	int		count;	     /* current count value	            */
	int		policy;	     /* wakeup policy for blocked threads   */
	boolean_t	active;      /* active status			    */
	queue_head_t	blocked_threads;   /* queue of blocked threads	    */
} Semaphore;

typedef struct semaphore *semaphore_t;

#define SEMAPHORE_NULL	((semaphore_t) 0)

#define SEMAPHORE_OPERATION_ABORTED(th)	\
	((th)->wait_link.prev != (queue_entry_t) 0) 

#define SEMAPHORE_OPERATION_COMPLETE(th) \
	((th)->wait_link.prev = (queue_entry_t) 0) 

/*
 *  Data structure internal lock macros
 */

#define semaphore_lock_init(s)	simple_lock_init(&(s)->lock, ETAP_THREAD_SEMA)
#define semaphore_lock(s)	simple_lock(&(s)->lock)
#define semaphore_unlock(s)	simple_unlock(&(s)->lock)

/*
 *	Forward Declarations
 */

extern	kern_return_t	semaphore_create	(task_t task,
						 semaphore_t *new_semaphore,
						 int policy,
						 int value);

extern	kern_return_t	semaphore_destroy	(task_t task,
						 semaphore_t semaphore);

extern	void		semaphore_reference	(semaphore_t semaphore);
extern	void		semaphore_dereference	(semaphore_t semaphore);

extern	kern_return_t	semaphore_signal     	(semaphore_t semaphore);
extern	kern_return_t	semaphore_signal_all 	(semaphore_t semaphore);
extern	kern_return_t	semaphore_signal_thread	(semaphore_t semaphore,
						 thread_act_t thread_act);
extern	kern_return_t	semaphore_wait       	(semaphore_t semaphore);
extern	kern_return_t	semaphore_timedwait    	(semaphore_t semaphore, 
						 tvalspec_t wait_time);

#endif /* _KERN_SYNC_SEMA_H_ */
