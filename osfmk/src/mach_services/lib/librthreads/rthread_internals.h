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
 * rthread_internals.h
 *
 * Private definitions for the Real-Time Threads implementation.
 */

#include <options.h>
#include <queue.h>
#include <mach.h>
#include <mach/thread_switch.h>


#define RTHREADS_VERSION 1


/*
 * Low-level Rthread implementation.
 */
struct rthread {
	struct rthread  *next;
	int 		status;
	rthread_fn_t	func;
	void 		*arg;
	int 		result;
	const char 	*name;
	void 		*data;
	policy_t	policy;
	int		priority;
	int		quantum;
	int 		context;
	struct rthread 	*list;		/* for master rthread list  */
	mach_port_t 	wired;		/* kernel thread port       */
	mach_port_t 	reply_port;	/* for mig_get_reply_port() */
	vm_offset_t	stack_base;
	vm_size_t	stack_size;
	int		err_no;
};

/*
 * Thread status bits.
 */
#define	T_MAIN		0x1
#define T_ACT		0x2
#define T_CHILD		0x4

/*
 * Rthread control block definition
 */
typedef struct rthread_control_struct {
	lock_set_port_t	locks;		   /* rthread control block locks    */
	int 		version;	   /* rthread version		     */
	int 	  	active_rthreads;   /* number of active rthreads      */
	int 	  	alloc_rthreads;    /* number of allocated rthreads   */
	int		activation_count;  /* number of created activations  */
	rthread_t  	rthread_list;	   /* list of all rthreads           */
	vm_size_t  	stack_size;	   /* size of user stacks 	     */
	vm_offset_t	stack_mask;	   /* mask to begining               */
	policy_t	default_policy;	   /* default child policy	     */
	int		base_priority;
	int		max_priority;
	int		default_quantum;
	boolean_t	main_waiting;      /* T_MAIN wait flag  	     */
	mach_port_t 	main_wait;	   /* semaphore T_MAIN blocks on if  */
	                                   /* waiting for its children to    */
					   /* complete.			     */
	struct rthread_queue  free_list;   /* exited rthread list: cached    */
					   /* for reuse 		     */
} *rthread_control_t;

extern struct rthread_control_struct rthread_control;

/*
 * Rthread control block lock definitions
 */

#define RTHREAD_CONTROL_LOCK			0
#define RTHREAD_CONTROL_POLICY_LOCK		1
#define RTHREAD_CONTROL_THREAD_LOCK		2
#define RTHREAD_CONTROL_TRACE_LOCK		3
#define RTHREAD_CONTROL_STACK_POOL_LOCK		4
#define RTHREAD_CONTROL_LOCK_GRANULARITY	5

/*
 * Rthread control block locking MACROS
 */

#ifdef	LOCKS

#define rthread_control_create_locks()  			\
	MACH_CALL(						\
	lock_set_create(mach_task_self(),			\
			&rthread_control.locks,			\
			RTHREAD_CONTROL_LOCK_GRANULARITY,	\
			SYNC_POLICY_FIXED_PRIORITY),r)

#define rthread_control_destroy_locks()				\
	MACH_CALL(						\
	lock_set_destroy(mach_task_self(),			\
			 rthread_control.locks),r)

#define rthread_control_lock()		\
	MACH_CALL(			\
	lock_acquire(rthread_control.locks, RTHREAD_CONTROL_LOCK),r)

#define rthread_control_unlock()	\
	MACH_CALL(			\
	lock_release(rthread_control.locks, RTHREAD_CONTROL_LOCK),r)

#define rthread_control_policy_lock()	\
	MACH_CALL(			\
	lock_acquire(rthread_control.locks, RTHREAD_CONTROL_POLICY_LOCK),r)

#define rthread_control_policy_unlock()	\
	MACH_CALL(			\
	lock_release(rthread_control.locks, RTHREAD_CONTROL_POLICY_LOCK),r)

#define rthread_lock()			\
	MACH_CALL(			\
	lock_acquire(rthread_control.locks, RTHREAD_CONTROL_THREAD_LOCK),r)

#define rthread_unlock()		\
	MACH_CALL(			\
	lock_release(rthread_control.locks, RTHREAD_CONTROL_THREAD_LOCK),r)

#define trace_lock()			\
	MACH_CALL(			\
	lock_acquire(rthread_control.locks, RTHREAD_CONTROL_TRACE_LOCK),r)

#define trace_try_lock()		\
	lock_try(rthread_control.locks, RTHREAD_CONTROL_TRACE_LOCK)

#define trace_unlock()			\
	MACH_CALL(			\
	lock_release(rthread_control.locks, RTHREAD_CONTROL_TRACE_LOCK),r)

#define stack_pool_lock()		\
	MACH_CALL(			\
	lock_acquire(rthread_control.locks, RTHREAD_CONTROL_STACK_POOL_LOCK),r)

#define stack_pool_unlock()		\
	MACH_CALL(			\
	lock_release(rthread_control.locks, RTHREAD_CONTROL_STACK_POOL_LOCK),r)

#else	/* LOCKS */
#define rthread_control_create_locks()
#define rthread_control_destroy_locks()
#define rthread_control_lock()
#define rthread_control_unlock()
#define rthread_control_policy_lock()
#define rthread_control_policy_unlock()
#define rthread_lock()
#define rthread_unlock()
#define trace_lock()
#define trace_try_lock()
#define trace_unlock()
#endif 	/* LOCKS */


#ifdef	STACK_GROWTH_UP
#define	_rthread_ptr(sp) (*(rthread_t *) ((sp) & rthread_control.stack_mask))
#else	/*STACK_GROWTH_UP*/
#define	_rthread_ptr(sp) 						\
	(* (rthread_t *) ( ((sp) | rthread_control.stack_mask) + 1 	\
			      - sizeof(rthread_t *)) )
#endif	/*STACK_GROWTH_UP*/

#define	_rthread_self()		((rthread_t) _rthread_ptr(rthread_sp()))

#define RTHREAD_FILTER(a, b, c, d, e, f) rthread_filter(  		\
 		       (int *)(&(a)->context), (int)(b), (void *)(c), 	\
		       (void *)(d), (void *)(e), (void *)(f))

/*
 * Macro for MACH kernel calls.
 */
#ifdef CHECK_STATUS
#define	MACH_CALL(expr, ret)						\
	if (((ret) = (expr)) != KERN_SUCCESS)				\
		mach_error("Rthreads kernel return error:", ret)
#else /*CHECK_STATUS*/
#define MACH_CALL(expr, ret) (ret) = (expr)
#endif /*CHECK_STATUS*/

#ifdef GPROF
#define private
#define staticf
#else /*GPROF*/
#define private static
#define staticf static
#endif

#if	defined(__i386)
#define IN_KERNEL(addr)	(VM_MAX_ADDRESS < (unsigned)(addr))
#define IN_KERNEL_STACK_SIZE	(32 * 1024)
#elif	defined(i860)
#define IN_KERNEL(addr)	(VM_MAX_ADDRESS < (unsigned)(addr))
#define IN_KERNEL_STACK_SIZE	(32 * 1024)
#elif	defined(hp_pa)
extern	boolean_t IN_KERNEL(void *addr);
#define IN_KERNEL_STACK_SIZE	(32 * 1024)
#elif	defined(ppc)
extern boolean_t IN_KERNEL(void *addr);
#define IN_KERNEL_STACK_SIZE	(32 * 1024)
#else
#error Need an IN_KERNEL definition for current target.
#endif

boolean_t in_kernel;

/*
 * Internal rthread function prototypes.
 */
extern void		*rthreads_malloc (unsigned int);
extern void		*rthreads_realloc(void *old_base,
					  unsigned int new_size);
extern void 		rthreads_free(void *);
extern void 		exit(int ret);
extern void 		alloc_stack(rthread_t);
extern void 		stack_init(rthread_t, vm_offset_t *newstack);
extern void 		mig_init(rthread_t);
extern void 		rthread_setup(rthread_t, thread_port_t, rthread_fn_t);
extern void		rthread_policy_info(rthread_t, boolean_t);
extern kern_return_t	rthread_set_default_policy_real(policy_t, boolean_t);
extern void		rthread_set_default_attibutes(rthread_t);
