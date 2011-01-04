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
 * MkLinux
 */
/* This file was formerly named kern/act.c; history is below:
 *
 * Revision 1.1.2.8  1994/02/26  17:38:42  bolinger
 * 	Merge up to colo_shared.
 * 	[1994/02/26  17:31:59  bolinger]
 *
 * 	Move profiling data from thread to activation.
 * 	[1994/02/25  23:44:17  bolinger]
 *
 * Revision 1.1.2.7  1994/02/25  23:15:11  dwm
 * 	To cope with usage in MIG stubs, act_reference and
 * 	act_deallocate must guard against null.
 * 	[1994/02/25  23:15:04  dwm]
 * 
 * Revision 1.1.2.6  1994/02/09  00:42:59  dwm
 * 	Remove various unused future hooks to conserve wired memory.
 * 	[1994/02/09  00:35:22  dwm]
 * 
 * Revision 1.1.2.5  1994/01/26  15:47:27  bolinger
 * 	Merge up to colo_shared.
 * 	[1994/01/25  22:58:09  bolinger]
 * 
 * 	Keep prototypes current.
 * 	[1994/01/25  18:52:06  bolinger]
 * 
 * Revision 1.1.2.4  1994/01/25  18:37:39  dwm
 * 	Move ast here from shuttle.
 * 	[1994/01/25  18:34:29  dwm]
 * 
 * Revision 1.1.2.3  1994/01/17  18:09:08  dwm
 * 	add dump_act() proto - debugging.
 * 	[1994/01/17  16:07:03  dwm]
 * 
 * Revision 1.1.2.2  1994/01/14  18:27:34  dwm
 * 	Coloc: comment out unused act_yank
 * 	[1994/01/14  17:59:39  dwm]
 * 
 * Revision 1.1.2.1  1994/01/12  17:53:13  dwm
 * 	Coloc: initial restructuring to follow Utah model.
 * 	This file defines the activation structure.
 * 	[1994/01/12  17:15:19  dwm]
 */
/*
 * Copyright (c) 1993 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *      Author: Bryan Ford, University of Utah CSL
 *
 *      File:   thread_act.h
 *
 *      thread activation definitions
 */
#ifndef	_KERN_THREAD_ACT_H_
#define _KERN_THREAD_ACT_H_

#include <mach_assert.h>
#include <xkmachkernel.h>
#include <thread_swapper.h>

#include <cputypes.h>
#include <mach/rpc.h>
#include <mach/vm_param.h>
#include <mach/thread_info.h>
#include <mach/exception.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <kern/etap_macros.h>
#include <machine/thread_act.h>
#include <kern/thread_pool.h>

/* Here is a description of the states an thread_activation may be in.
 *
 * An activation always has a valid task pointer, and it is always constant.
 * The activation is only linked onto the task's activation list until
 * the activation is terminated.
 *
 * An activation is in use or not, depending on whether its thread
 * pointer is nonzero.  If it is not in use, it is just sitting idly
 * waiting to be used by a thread.  The thread holds a reference on
 * the activation while using it.
 *
 * An activation lives on an thread_pool if its pool_port pointer is nonzero.
 * When in use, it can still live on an thread_pool, but it is not actually
 * linked onto the thread_pool's list of available activations.  In this case,
 * the act will return to its thread_pool as soon as it becomes unused.
 *
 * An activation is active until thread_terminate is called on it;
 * then it is inactive, waiting for all references to be dropped.
 * Future control operations on the terminated activation will fail,
 * with the exception that act_yank still works if the activation is
 * still on an RPC chain.  A terminated activation always has null
 * thread and pool_port pointers.
 *
 * An activation is suspended when suspend_count > 0.
 * A suspended activation can live on an thread_pool, but it is not
 * actually linked onto the thread_pool while suspended.
 *
 * Locking note:  access to data relevant to scheduling state (user_stop_count,
 * suspend_count, handlers, special_handler) is controlled by the combination
 * of locks acquired by act_lock_thread().  That is, not only must act_lock()
 * be held, but RPC through the activation must be frozen (so that the
 * thread pointer doesn't change).  If a shuttle is associated with the
 * activation, then its thread_lock() must also be acquired to change these
 * data.  Regardless of whether a shuttle is present, the data must be
 * altered at splsched().
 */

typedef struct ReturnHandler {
	struct ReturnHandler *next;
	void (*handler)(struct ReturnHandler *rh,
				struct thread_activation *thr_act);
} ReturnHandler;

typedef struct thread_activation {

	/*** task linkage ***/

	/* Links for task's circular list of activations.  The activation
	 * is only on the task's activation list while active.  Must be
	 * first.
	 */
	queue_chain_t	thr_acts;

	/* Indicators for whether this activation is in the midst of
	 * resuming or has already been resumed in a kernel-loaded
	 * task -- these flags are basically for quick access to
	 * this information.
	 */
	boolean_t	kernel_loaded;	/* running in kernel-loaded task */
	boolean_t	kernel_loading;	/* about to run kernel-loaded */

	/*** Machine-dependent state ***/
	struct MachineThrAct	mact;

	/*** Consistency ***/
	decl_mutex_data(,lock)
	decl_simple_lock_data(,sched_lock)
	int		ref_count;

	/* Reference to the task this activation is in.
	 * Constant for the life of the activation
	 */
	struct task	*task;
	vm_map_t	map;		/* cached current map */

	/*** thread_pool-related stuff ***/
	/* Port containing the thread_pool this activation normally lives
	 * on, zero if none.  The port (really the thread_pool) holds a
	 * reference to the activation as long as this is nonzero (even when
	 * the activation isn't actually on the thread_pool's list).
	 */
	struct ipc_port	*pool_port;

	/* Link on the thread_pool's list of activations.
	 * The activation is only actually on the thread_pool's list
	 * (and hence this is valid) when not in use (thread == 0) and
	 * not suspended (suspend_count == 0).
	 */
	struct thread_activation *thread_pool_next;

	/* User stack location and size for initialization on migrating RPCs */
	vm_offset_t	user_stack;
	vm_size_t	user_stack_size;

	/* RPC state */
        union {
                struct {
                        rpc_subsystem_t         r_subsystem;
                        mach_rpc_id_t           r_routine_num;
                        mach_rpc_signature_t    r_sig_ptr;
                        mach_rpc_size_t         r_sig_size;
                        vm_offset_t             r_new_argv;
                        vm_offset_t            *r_arg_buf;
                        vm_offset_t             r_arg_buf_data[RPC_KBUF_SIZE];
                        rpc_copy_state_t        r_state;
                        rpc_copy_state_data_t   r_state_data[RPC_DESC_COUNT];
                        unsigned int            r_port_flags;
                } regular;
                struct {
                        ipc_port_t              r_port;
                        ipc_port_t              r_exc_port;
			int			r_exc_flavor;
			mach_msg_type_number_t	r_ostate_cnt;
			exception_data_type_t	r_code[EXCEPTION_CODE_MAX];
#if                     ETAP_EVENT_MONITOR
                        exception_type_t        r_exception;
#endif
                } exception;
        } rpc_state;

	/*** Thread linkage ***/
	/* Shuttle using this activation, zero if not in use.  The shuttle
	 * holds a reference on the activation while this is nonzero.
	 */
	struct thread_shuttle	*thread;

	/* The rest in this section is only valid when thread is nonzero.  */

	/* Next higher and next lower activation on the thread's activation
	 * stack.  For a topmost activation or the null_act, higher is
	 * undefined.  The bottommost activation is always the null_act.
	 */
	struct thread_activation *higher, *lower;

	/* Alert bits pending at this activation; some of them may have
	 * propagated from lower activations.
	 */
	unsigned	alerts;

	/* Mask of alert bits to be allowed to pass through from lower levels.
	 */
	unsigned	alert_mask;

	/* Saved policy and priority of shuttle if changed to migrate into
	 * higher-priority or more real-time task.  Only valid if
	 * saved_sched_stamp is nonzero and equal to the sched_change_stamp
	 * in the thread_shuttle.  (Otherwise, the policy or priority has
	 * been explicitly changed in the meantime, and the saved values
	 * are invalid.)
	 */
	policy_t	saved_policy;
	integer_t	saved_base_priority;
	unsigned int	saved_sched_change_stamp;

	/*** Control information ***/

	/* Number of outstanding suspensions on this activation.  */
	int		suspend_count;

	/* User-visible scheduling state */
	int		user_stop_count;	/* outstanding stops */

	/* ast is needed - see ast.h */
	int		ast;

#if	THREAD_SWAPPER
	/* task swapper */
      	int		swap_state;	/* swap state (or unswappable flag)*/
	queue_chain_t	swap_queue;	/* links on swap queues */
#if	MACH_ASSERT
	boolean_t	kernel_stack_swapped_in;
					/* debug for thread swapping */
#if	THREAD_SWAP_UNWIRE_USER_STACK
	boolean_t	user_stack_swapped_in;
					/* debug for thread swapping */
#endif	/* THREAD_SWAP_UNWIRE_USER_STACK */
#endif	/* MACH_ASSERT */
#endif	/* THREAD_SWAPPER */

	/* This is normally true, but is set to false when the
	 * activation is terminated.
	 */
	int		active;

	/* Chain of return handlers to be called before the thread is
	 * allowed to return to this invocation
	 */
	ReturnHandler	*handlers;

	/* A special ReturnHandler attached to the above chain to
	 * handle suspension and such
	 */
	ReturnHandler	special_handler;

	/* Special ports attached to this activation */
	struct ipc_port *ith_self;	/* not a right, doesn't hold ref */
	struct ipc_port *ith_sself;	/* a send right */
	struct exception_action exc_actions[EXC_TYPES_COUNT];

	/* A list of ulocks (a lock set element) currently held by the thread
	 */
	queue_head_t	held_ulocks;

#if	XKMACHKERNEL
	void		*xk_resources;
#endif	/* XKMACHKERNEL */

#if	MACH_PROF
	/* Profiling data structures */
	boolean_t	act_profiled;	/* is activation being profiled? */
	boolean_t	act_profiled_own;
					/* is activation being profiled 
					 * on its own ? */
	struct prof_data *profil_buffer;/* prof struct if either is so */
#endif	/* MACH_PROF */

} Thread_Activation;

#define THR_ACT_NULL ((thread_act_t)0)

/* RPC state fields */
#define r_subsystem     rpc_state.regular.r_subsystem
#define r_routine_num   rpc_state.regular.r_routine_num
#define r_sig_ptr       rpc_state.regular.r_sig_ptr
#define r_sig_size      rpc_state.regular.r_sig_size
#define r_new_argv      rpc_state.regular.r_new_argv
#define r_arg_buf       rpc_state.regular.r_arg_buf
#define r_arg_buf_data  rpc_state.regular.r_arg_buf_data
#define r_state         rpc_state.regular.r_state
#define r_state_data    rpc_state.regular.r_state_data
#define r_port_flags    rpc_state.regular.r_port_flags
#define r_port          rpc_state.exception.r_port
#define r_exc_port      rpc_state.exception.r_exc_port
#define r_exc_flavor	rpc_state.exception.r_exc_flavor
#define r_ostate_cnt	rpc_state.exception.r_ostate_cnt
#define r_code          rpc_state.exception.r_code
#define r_exception     rpc_state.exception.r_exception

/* Alert bits */
#define SERVER_TERMINATED	0x01
#define ORPHANED		0x02
#define CLIENT_TERMINATED	0x04

#if THREAD_SWAPPER
/*
 * Encapsulate the actions needed to ensure that next lower act on
 * RPC chain is swapped in.  Used at base spl; assumes rpc_lock()
 * of thread is held; if port is non-null, assumes its ip_lock()
 * is also held.
 */
#define act_switch_swapcheck(thread, port)			\
MACRO_BEGIN							\
	thread_act_t __act__ = thread->top_act;			\
								\
	while (__act__->lower) {				\
		thread_act_t __l__ = __act__->lower;		\
								\
		if (__l__->swap_state == TH_SW_IN ||		\
			__l__->swap_state == TH_SW_UNSWAPPABLE)	\
			break;					\
		/*						\
		 * XXX - Do we need to reference __l__?  	\
		 */						\
		if (port)					\
			ip_unlock(port);			\
		if (!thread_swapin_blocking(__l__))		\
			panic("act_switch_swapcheck: !active");	\
		if (port)					\
			ip_lock(port);				\
		if (__act__->lower == __l__)			\
			break;					\
	}							\
MACRO_END

#else	/* !THREAD_SWAPPER */

#define act_switch_swapcheck(thread, port)

#endif	/* !THREAD_SWAPPER */


/* Exported to world */

extern kern_return_t	act_alert(thread_act_t, unsigned);
extern kern_return_t	act_alert_mask(thread_act_t, unsigned );
extern kern_return_t	act_create(struct task *, vm_offset_t, vm_size_t,
				   thread_act_t *);
extern kern_return_t	act_get_state(thread_act_t, int, thread_state_t,
				mach_msg_type_number_t *);
extern kern_return_t	act_set_state(thread_act_t, int, thread_state_t,
				mach_msg_type_number_t);
extern kern_return_t	act_disable_task_locked(thread_act_t);
extern kern_return_t	thread_abort(thread_act_t);
extern kern_return_t	thread_abort_safely(thread_act_t);
extern void		thread_release(thread_act_t);
extern kern_return_t	thread_resume(thread_act_t);
extern kern_return_t	thread_suspend(thread_act_t);
extern kern_return_t	thread_terminate(thread_act_t);

/* Following are exported only to mach_host code */
extern kern_return_t	thread_dowait(thread_act_t, boolean_t);
extern void		thread_hold(thread_act_t);

#define	act_lock_init(thr_act)	mutex_init(&(thr_act)->lock, ETAP_THREAD_ACT)
#define	act_lock(thr_act)	mutex_lock(&(thr_act)->lock)
#define	act_lock_try(thr_act)	mutex_try(&(thr_act)->lock)
#define	act_unlock(thr_act)	mutex_unlock(&(thr_act)->lock)

/* Sanity check the ref count.  If it is 0, we may be doubly zfreeing.
 * If it is larger than max int, it has been corrupted, probably by being
 * modified into an address (this is architecture dependent, but it's
 * safe to assume there cannot really be max int references).
 */
#define ACT_MAX_REFERENCES					\
	(unsigned)(~0 ^ (1 << (sizeof(int)*BYTE_SIZE - 1)))

#define		act_reference(thr_act)				\
		MACRO_BEGIN					\
		    if (thr_act) {				\
			act_lock(thr_act);			\
			assert((thr_act)->ref_count < ACT_MAX_REFERENCES); \
			if ((thr_act)->ref_count <= 0) \
				panic("act_reference: already freed"); \
			(thr_act)->ref_count++;			\
			act_unlock(thr_act);			\
		    }						\
		MACRO_END

#define		act_locked_act_reference(thr_act)		\
		MACRO_BEGIN					\
		    if (thr_act) {				\
			assert((thr_act)->ref_count < ACT_MAX_REFERENCES); \
			if ((thr_act)->ref_count <= 0) \
				panic("a_l_act_reference: already freed"); \
			(thr_act)->ref_count++;			\
		    }						\
		MACRO_END

#define		act_deallocate(thr_act)				\
		MACRO_BEGIN					\
		    if (thr_act) {				\
			int new_value;				\
			act_lock(thr_act);			\
			assert((thr_act)->ref_count > 0 &&	\
			    (thr_act)->ref_count <= ACT_MAX_REFERENCES); \
			new_value = --(thr_act)->ref_count;	\
			if (new_value == 0){ act_free(thr_act); } \
			else act_unlock(thr_act);			\
		    }						\
		MACRO_END

#define		act_locked_act_deallocate(thr_act)		\
		MACRO_BEGIN					\
		    if (thr_act) {				\
			int new_value;				\
			assert((thr_act)->ref_count > 0 &&	\
			    (thr_act)->ref_count <= ACT_MAX_REFERENCES); \
			new_value = --(thr_act)->ref_count;	\
			if (new_value == 0) { 			\
			    panic("a_l_act_deallocate: would free act"); \
			}					\
		    }						\
		MACRO_END

extern void		act_init(void);
extern kern_return_t	act_create_kernel(task_t, vm_offset_t, vm_size_t,
					  thread_act_t *);
extern kern_return_t	act_set_thread_pool(thread_act_t, ipc_port_t);
extern kern_return_t	act_locked_act_set_thread_pool(thread_act_t, ipc_port_t);
extern kern_return_t	thread_get_special_port(thread_act_t, int,
					ipc_port_t *);
extern kern_return_t	thread_set_special_port(thread_act_t, int,
					ipc_port_t);
extern thread_t		act_lock_thread(thread_act_t);
extern void		act_unlock_thread(thread_act_t);
extern void		install_special_handler(thread_act_t);
extern thread_act_t	thread_lock_act(thread_t);
extern void		thread_unlock_act(thread_t);
extern void		act_attach(thread_act_t, thread_t, unsigned);
extern void		act_execute_returnhandlers(void);
extern void		act_detach(thread_act_t);
extern void		act_free(thread_act_t);

/* machine-dependent functions */
extern void		act_machine_return(kern_return_t);
extern void		act_machine_init(void);
extern kern_return_t	act_machine_create(struct task *, thread_act_t);
extern void		act_machine_destroy(thread_act_t);
extern kern_return_t	act_machine_set_state(thread_act_t,
					thread_flavor_t, thread_state_t,
					mach_msg_type_number_t );
extern kern_return_t	act_machine_get_state(thread_act_t,
					thread_flavor_t, thread_state_t,
					mach_msg_type_number_t *);
extern void		act_machine_switch_pcb(thread_act_t);
extern int		dump_act(thread_act_t);	/* debugging */

#endif /* _KERN_THREAD_ACT_H_ */
