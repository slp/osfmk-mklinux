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
 * Revision 2.13.1.1  92/06/22  11:49:47  rwd
 * 	Add cthread_status struct
 * 	[92/02/10            rwd]
 * 
 * 	Changes for single internal lock version of cproc.c
 * 	[92/01/09            rwd]
 * 
 * Revision 2.13  92/03/06  14:09:24  rpd
 * 	Added yield, defined using thread_switch.
 * 	[92/03/06            rpd]
 * 
 * Revision 2.12  92/03/01  00:40:23  rpd
 * 	Removed exit declaration.  It conflicted with the real thing.
 * 	[92/02/29            rpd]
 * 
 * Revision 2.11  91/08/28  11:19:23  jsb
 * 	Fixed MACH_CALL to allow multi-line expressions.
 * 	[91/08/23            rpd]
 * 
 * Revision 2.10  91/07/31  18:33:33  dbg
 * 	Protect against redefinition of ASSERT.
 * 	[91/07/30  17:33:21  dbg]
 * 
 * Revision 2.9  91/05/14  17:56:24  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/02/14  14:19:42  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:41:02  mrt]
 * 
 * Revision 2.7  90/11/05  14:36:55  rpd
 * 	Added spin_lock_t.
 * 	[90/10/31            rwd]
 * 
 * Revision 2.6  90/09/09  14:34:51  rpd
 * 	Remove special field.
 * 	[90/08/24            rwd]
 * 
 * Revision 2.5  90/06/02  15:13:44  rpd
 * 	Converted to new IPC.
 * 	[90/03/20  20:52:47  rpd]
 * 
 * Revision 2.4  90/03/14  21:12:11  rwd
 * 	Added waiting_for field for debugging deadlocks.
 * 	[90/03/01            rwd]
 * 	Added list field to keep a master list of all cprocs.
 * 	[90/03/01            rwd]
 * 
 * Revision 2.3  90/01/19  14:37:08  rwd
 * 	Keep track of real thread for use in thread_* substitutes.
 * 	Add CPROC_ARUN for about to run and CPROC_HOLD to avoid holding
 * 	spin_locks over system calls.
 * 	[90/01/03            rwd]
 * 	Add busy field to be used by cthread_msg calls to make sure we
 * 	have the right number of blocked kernel threads.
 * 	[89/12/21            rwd]
 * 
 * Revision 2.2  89/12/08  19:53:28  rwd
 * 	Added CPROC_CONDWAIT state
 * 	[89/11/28            rwd]
 * 	Added on_special field.
 * 	[89/11/26            rwd]
 * 	Removed MSGOPT conditionals
 * 	[89/11/25            rwd]
 * 	Removed old debugging code.  Add wired port/flag.  Add state
 * 	for small state machine.
 * 	[89/10/30            rwd]
 * 	Added CPDEBUG code
 * 	[89/10/26            rwd]
 * 	Change TRACE to {x;} else.
 * 	[89/10/24            rwd]
 * 	Rewrote to work for limited number of kernel threads.  This is
 * 	basically a merge of coroutine and thread.  Added
 * 	cthread_receivce call for use by servers.
 * 	[89/10/23            rwd]
 * 
 */
/*
 * cthread_internals.h
 *
 *
 * Private definitions for the C Threads implementation.
 *
 * The cproc structure is used for different implementations
 * of the basic schedulable units that execute cthreads.
 *
 */
#include "cthread_options.h"
#include <mach.h>
#include <mach/thread_switch.h>
#include <mach/machine/vm_param.h>

#ifdef SWITCH_OPTION_IDLE
#define CTHREADS_VERSION 3
#else /*SWITCH_OPTION_IDLE*/
#define CTHREADS_VERSION 2
#endif /*SWITCH_OPTION_IDLE*/

struct cthread_dlq_entry {
	struct cthread_dlq_entry	*next;		/* next element */
	struct cthread_dlq_entry	*prev;		/* previous element */
};
typedef	struct cthread_dlq_entry *cthread_dlq_entry_t;
#define CTHREAD_DLQ_INITIALIZER {(cthread_dlq_entry_t)0, (cthread_dlq_entry_t)0}

/*
 * Build time options.
 */
#if defined(GPROF) || defined(DEBUG)
#define staticf
#endif

#if !defined(GPROF) && defined(DEBUG)
#define	private
#endif

#if !defined(staticf)
#define staticf static
#endif

#if !defined(private)
#define	private static
#endif

/*
 * Low-level thread implementation.
 */
struct cthread {
    struct cthread 	*next;		/* Used to queue cthreads on lists. */
    int			status;		/* The thread's status (see below). */
    cthread_fn_t	func;		/* The thread's initial function. */
    void		*arg;		/* The thread's initial argument. */
    void		*result;	/* Return result from func() */
    const char		*name;		/* The thread's name. */
    void 		*data;		/* Thread data set by set_data(). */
					/* In the server this is used to */
					/* keep track of the cpu number. */
    struct cthread	*joiner;	/* Who are we joining with (from */
					/* cthread_join_real). */
    cthread_context_t	context;	/* Pointer to cthread's context. */
    struct cthread 	*list;		/* List of all cthreads. */
    mutex_t		cond_mutex;	/* Event the thread is waiting for. */
#ifdef	WAIT_DEBUG
	int waiting_for;
#endif	/* WAIT_DEBUG */
    volatile int	state;		/* The current state (see below). */
    volatile int	flags;		/* The thread's flags (see below). */
    volatile boolean_t	undepress;	/* Is the thread's priority */
					/* depressed */
    mach_port_t		wired;		/* is cthread wired to kernel thread */
    mach_port_t		reply_port;	/* for mig_get_reply_port() */
    struct cthread_dlq_entry dlq;
    vm_offset_t		stack_base; 	/* Base of the cthread's stack. */
    vm_size_t		stack_size;	/* Size of the cthread's stack. */
    int			err_no;		/* Thread-local errno. */
    struct cthread_status_struct *cthread_status;
};

/*
 * Thread status bits.
 */
#define	T_MAIN		0x1
#define	T_RETURNED	0x2
#define	T_DETACHED	0x4

/*
 * Thread state bits.
 */
#define CTHREAD_RUNNING		0x1
#define CTHREAD_RUNNABLE	0x2
#define CTHREAD_BLOCKED		0x4

/*
 * Thread flags.
 */
#define CTHREAD_WAITER		0x1
#define CTHREAD_EWIRED		0x2
#define CTHREAD_DEPRESS		0x4

/*
 * Cthread stack manipulation and self identification.
 *
 *	cthread_ptr - Calculates the cthread's first available stack address.
 *	_cthread_self - Returns the cthread ID.  This is synonymous with the
 *		base of the cthread's stack address.
 */
#ifdef	STACK_GROWTH_UP
#define	_cthread_ptr(sp) \
	(*(cthread_t *) ((vm_offset_t)(sp) & cthread_status.stack_mask))
#else	/*STACK_GROWTH_UP*/
#define	_cthread_ptr(sp) \
	(*(cthread_t *) \
	  (((vm_offset_t)(sp) | cthread_status.stack_mask) \
	   + 1 - sizeof(cthread_t *)))
#endif	/*STACK_GROWTH_UP*/

#define	_cthread_self()		((cthread_t) _cthread_ptr(cthread_sp()))

#define CTHREAD_FILTER(a, b, c, d, e, f) cthread_filter(  \
 		       (&(a)->context), b, (void *)(c), \
		       (void *)(d), (void *)(e), (void *)(f))

#ifndef THREAD_NULL
#define THREAD_NULL MACH_PORT_NULL
#endif /*THREAD_NULL*/

/*
 * Cthread locking
 *
 * Only one lock is used for all of the cthread's data.  The lock isn't held
 * long enough, nor is there enough contention to justify multiple locks.
 *
 * lock_spin_count - The number of iterations to spin waiting for a lock to
 *	become available.
 */

#define cthread_try_lock()	spin_try_lock(&cthread_status.run_lock)
    
#define cthread_lock() 					\
    do {						\
	if (!spin_try_lock(&cthread_status.run_lock))	\
	    spin_lock_solid(&cthread_status.run_lock);	\
    } while (0)

#define cthread_unlock()	spin_unlock(&cthread_status.run_lock)
#define cthread_lock_init()	spin_lock_init(&cthread_status.run_lock)

#define stats_lock(p)
#define stats_unlock(p)

/*
 * Cthread queue macros
 */
#define	cthread_queue_init(q)	((q)->head = (q)->tail = 0)

#define	cthread_queue_enq(q, x) \
	do { \
		(x)->next = 0; \
		if ((q)->tail == 0) \
			(q)->head = (cthread_queue_item_t) (x); \
		else \
			(q)->tail->next = (cthread_queue_item_t) (x); \
		(q)->tail = (cthread_queue_item_t) (x); \
	} while (0)

#define	cthread_queue_preq(q, x) \
	do { \
		if ((q)->tail == 0) \
			(q)->tail = (cthread_queue_item_t) (x); \
		((cthread_queue_item_t) (x))->next = (q)->head; \
		(q)->head = (cthread_queue_item_t) (x); \
	} while (0)

#define	cthread_queue_head(q, t)	((t) ((q)->head))

#define	cthread_queue_deq(q, t, x) \
    do { \
	if (((x) = (t) ((q)->head)) != 0 && \
	    ((q)->head = (cthread_queue_item_t) ((x)->next)) == 0) \
		(q)->tail = 0; \
    } while (0)

/*
 * Macro for MACH kernel calls.
 */
#ifdef CHECK_STATUS
#define	MACH_CALL(expr, ret)						\
    do {								\
	if (((ret) = (expr)) != KERN_SUCCESS)				\
	    fprintf_stderr("error in %s at %d: %s\n", __FILE__,		\
			   __LINE__, mach_error_string(ret)); 				\
    } while (0)
#else /*CHECK_STATUS*/
#define MACH_CALL(expr, ret) (ret) = (expr)
#endif /*CHECK_STATUS*/

#define yield() \
	MACH_CALL(thread_switch(THREAD_NULL, SWITCH_OPTION_DEPRESS, 10), r)

/* 
 * cthread statistics
 */
#ifdef STATISTICS
extern struct cthread_statistics_struct cthread_stats;
#endif /*STATISTICS*/

extern void exit(int ret);

/*
 * Internal cthread function prototypes.
 */
extern void cthread_setup(cthread_t, thread_port_t, cthread_fn_t);

#if 0	/* These functions don't really exist as such. */
/* Cthread context functions (in machine/csw.s) */
extern void  cthread_context_prepare(cthread_context_t *, cthread_fn_t, void *);
extern void *cthread_context_user_build(cthread_context_t *, cthread_fn_t,
                                        void *, void *);
extern void  cthread_context_user_invoke(cthread_context_t *, void *);
extern void  cthread_context_internal_build(cthread_context_t *, cthread_fn_t,
                                        void *, spin_lock_t *, vm_offset_t *);
#endif	/* 0 */

/* Malloc functions (in malloc.c). */
#ifdef DEBUG
extern void cthread_print_malloc_free_list(void);
#endif

/* Support for wiring thread stacks.  */
#if	!defined(parisc) && !defined(ppc)
#define	WIRE_IN_KERNEL_STACKS
#endif

/* Stack support (in stack.c). */
extern void alloc_stack(cthread_t);
extern void stack_init(cthread_t, vm_offset_t *);
#ifdef	WIRE_IN_KERNEL_STACKS
extern void stack_wire(vm_address_t, vm_size_t);
#endif	/* WIRE_IN_KERNEL_STACKS */
extern void stack_fork_child(void);
extern void stack_wire(vm_address_t base, vm_size_t length);
extern vm_offset_t cthread_stack_base(cthread_t, vm_size_t);
                                           /*return start of stack*/

/* MIG support functions (in mig_support.c). */
extern void mig_init(cthread_t);

typedef struct cthread_status_struct {
    int version;
    struct cthread_queue cthreads;
    int cthread_cthreads;
    int alloc_cthreads;
    cthread_t cthread_list;	/* list of all cthreads */
    vm_size_t stack_size;	/* size of user stacks */
    vm_offset_t stack_mask;	/* mask to begining */
    int lock_spin_count;	/* how long to spin trying for spin lock w/o
				   thread_switch ing */
    int processors;		/* actual number of processors in PS */
    vm_size_t wait_stack_size;	/* stack size for idle threads */
    int max_kernel_threads;	/* max kernel threads */
    int kernel_threads;		/* current kernel threads */
    int waiter_spin_count;	/* amount of busy spin on idle thread */
    int mutex_spin_count;       /* amount of busy spin in mutex_lock_solid */
    spin_lock_t run_lock;	/* the spin lock for all context switches */
    struct cthread_queue run_queue;/* run queue */
    struct cthread_dlq_entry waiting_dlq;
    struct cthread_queue waiters;/* queue of cthreads to run as idle */
    cthread_t exit_thread;	/* for when T_MAIN exits */
} *cthread_status_t;

extern struct cthread_status_struct cthread_status;
extern void cthread_pstats(int file);

#ifdef STATISTICS
typedef struct cthread_statistics_struct {
    spin_lock_t lock; /* only used for some */
    int mutex_block;
    int mutex_miss; 
    int waiters;
    int unlock_new;
    int blocks;
    int notrigger;
    int wired;
    int wait_stacks;
    int rnone;
    int spin_count;
    int mutex_caught_spin;
    int idle_exit;
    int mutex_cwl;
    int idle_swtchs;
    int idle_lost;
    int umutex_enter;
    int umutex_noheld;
    int join_miss;
} *cthread_stats_t;

#endif /*STATISTICS*/

/* Support for wiring thread stacks.  */
#if	!defined(hp_pa) && !defined(ppc)
#define	WIRE_IN_KERNEL_STACKS
extern void stack_wire(vm_address_t base, vm_size_t length);
#endif

#if	defined(__i386) || defined (hp_pa) || defined (ppc)
extern	boolean_t IN_KERNEL(void *addr);
#define IN_KERNEL_STACK_SIZE	(32 * 1024)
#elif	defined(i860)
#define IN_KERNEL(addr)		(VM_MAX_ADDRESS < (unsigned)(addr))
#define IN_KERNEL_STACK_SIZE	(32 * 1024)
#else
#error Need an IN_KERNEL definition for current target.
#endif
boolean_t in_kernel;
