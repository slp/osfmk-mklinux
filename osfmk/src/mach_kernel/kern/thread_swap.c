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
/* CMU_HIST */
/*
 * Revision 2.8  91/07/31  17:50:04  dbg
 * 	Revise scheduling state machine.
 * 	[91/07/30  17:07:03  dbg]
 * 
 * Revision 2.7  91/05/14  16:49:08  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/03/16  14:53:09  rpd
 * 	Removed thread_swapout.
 * 	[91/02/24            rpd]
 * 	Added swapin_thread_continue.
 * 	Simplified the state machine.  Now it uses only
 * 	TH_SW_IN, TH_SW_OUT, TH_SW_COMING_IN.
 * 	[91/01/20            rpd]
 * 
 * 	Simplified thread_swapin.
 * 	[91/01/17            rpd]
 * 
 * Revision 2.5  91/02/05  17:30:40  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:20:14  mrt]
 * 
 * Revision 2.4  91/01/08  15:18:20  rpd
 * 	Added continuation argument to thread_block.
 * 	[90/12/08            rpd]
 * 
 * 	Removed swapout_thread, swapout_threads,
 * 	swapout_scan, thread_swapout.
 * 	[90/11/11            rpd]
 * 
 * Revision 2.3  90/06/02  14:57:18  rpd
 * 	In thread_swapout, free the thread's cached message buffer.
 * 	[90/04/23            rpd]
 * 	Converted to new processor set technology.
 * 	[90/03/26  22:26:32  rpd]
 * 
 * Revision 2.2  89/12/08  19:52:35  rwd
 * 	Added call to zone_gc()
 * 	[89/11/21            rwd]
 * 
 * Revision 2.1  89/08/03  15:48:24  rwd
 * Created.
 * 
 * Revision 2.4  88/10/27  10:50:40  rpd
 * 	Changed includes to the new style.  Removed extraneous semis
 * 	from the swapper_lock/swapper_unlock macros.
 * 	[88/10/26  14:49:09  rpd]
 * 
 * 15-Jun-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Fix improper handling of swapper_lock() in swapin_thread().
 *	Problem discovery and elegant recoding due to Richard Draves.
 *
 *  4-May-88  David Golub (dbg) at Carnegie-Mellon University
 *	Remove vax-specific code.
 *
 *  1-Mar-88  David Black (dlb) at Carnegie-Mellon University
 *	Logic change due to replacement of wait_time field in thread
 *	with sched_stamp.  Extra argument to thread_setrun().
 *
 * 25-Jan-88  Richard Sanzi (sanzi) at Carnegie-Mellon University
 *	Notify pcb module that pcb is about to be unwired by calling
 *	pcb_synch(thread).
 *	
 * 21-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Fix lots more race conditions (thread_swapin called during
 *	swapout, thread_swapin called twice) by adding a swapper state
 *	machine to each thread.  Moved thread_swappable here from
 *	kern/thread.c.
 *
 * 12-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Fix race condition in thread_swapout: mark thread as swapped
 *	before swapping out its stack, so that an intervening wakeup
 *	will put it on the swapin list.
 *
 *  5-Oct-87  David Golub (dbg) at Carnegie-Mellon University
 *	Changed to new scheduling state machine.
 *
 * 15-Sep-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 *  5-Sep-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added check for THREAD_SWAPPABLE in swapout_scan().
 *
 * 14-Jul-87  David Golub (dbg) at Carnegie-Mellon University
 *	Truncate the starting address and round up the size given to
 *	vm_map_pageable, when wiring/unwiring kernel stacks.
 *	KERNEL_STACK_SIZE is not necessarily a multiple of page_size; if
 *	it isn't, forgetting to round the address and size to page
 *	boundaries results in panic.  Kmem_alloc and kmem_free, used in
 *	thread.c to allocate and free kernel stacks, already round to
 *	page boundaries.
 *
 * 26-Jun-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Add thread_swapout_allowed flag to make it easy to turn
 *	off swapping when debugging.
 *
 *  4-Jun-87  David Golub (dbg) at Carnegie-Mellon University
 *	Pass correct number of parameters to lock_init - initialize
 *	swap_lock as sleepable instead of calling lock_sleepable
 *	separately.
 *
 *  1-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Include vm_param.h to pick up KERNEL_STACK_SIZE definition.
 *
 * 20-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Lower ipl before calling thread_swapout().
 *
 * 19-Mar-87  David Golub (dbg) at Carnegie-Mellon University
 *	Fix one race condition in this (not so buggy) version - since
 *	thread_swapin can be called from interrupts, must raise IPL when
 *	locking swapper_lock.
 *
 * 09-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created, based somewhat loosely on the earlier (which was a highly
 *	buggy, race condition filled version).
 *
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
 */
/*
 *
 *	File:	kern/thread_swap.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1987
 *
 *	Mach thread swapper:
 *		Find idle threads to swap, freeing up kernel stack resources
 *		at the expense of allowing them to execute.
 *
 *		Swap in threads that need to be run.  This is done here
 *		by the swapper thread since it cannot be done (in general)
 *		when the kernel tries to place a thread on a run queue.
 *
 *	Note: The act of swapping a thread in Mach does not mean that
 *	its memory gets forcibly swapped to secondary storage.  The memory
 *	for the task corresponding to a swapped thread is paged out
 *	through the normal paging mechanism.
 *
 */

#include <mach_rt.h>

#include <kern/thread.h>
#include <kern/lock.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <mach/vm_param.h>
#include <kern/sched_prim.h>
#include <kern/processor.h>
#include <kern/thread_swap.h>
#include <kern/spl.h>		/* for splsched */
#include <kern/misc_protos.h>
#include <mach/policy.h>

queue_head_t		swapin_queue;
decl_simple_lock_data(,swapper_lock)

#define swapper_lock()		simple_lock(&swapper_lock)
#define swapper_unlock()	simple_unlock(&swapper_lock)

#define THREAD_SW_DEBUG 1
int thread_swap_debug = 0;

/*
 * MAXSLP and MAX_SWAP_RATE can be overridden in vm_param.h, or by poking
 * the global variable (lower-case) at any time.
 *
 * MAXSLP is the amount of time a thread will be allowed to sleep
 * without being a candidate for idle thread swapping.
 *
 * MAX_SWAP_RATE is the number of scheduler ticks (see sched_prim.c),
 * between periodic scans of the idle thread list. When the pageout
 * daemon detects a severe page shortage, this rate is ignored. A 
 * scheduler tick is usually 1 second.
 */
#ifndef MAXSLP
#define MAXSLP	10
#endif

int	maxslp = MAXSLP;

#ifndef MAX_SWAP_RATE
#define MAX_SWAP_RATE	60
#endif

int	max_swap_rate = MAX_SWAP_RATE;

/* should we unwire the kernel stack when a thread is swapped out ? */
boolean_t thread_swap_unwire_stack = TRUE;

#if	THREAD_SWAP_UNWIRE_USER_STACK
boolean_t thread_swap_unwire_user_stack = TRUE;
#else	/* THREAD_SWAP_UNWIRE_USER_STACK */
#define thread_swap_unwire_user_stack	FALSE
#endif	/* THREAD_SWAP_UNWIRE_USER_STACK */

/*
 *	thread_swapper_init: [exported]
 *
 *	Initialize the swapper module.
 */
void thread_swapper_init(void)
{
	queue_init(&swapin_queue);
	simple_lock_init(&swapper_lock, ETAP_THREAD_SWAPPER);
}

/*
 *	thread_swapin: [exported]
 *
 *	Place the specified thread in the list of threads to swapin.  If
 *	thr_act is associated with a thread, the thread is locked; if not,
 *	the thr_act itself (i.e., its act_lock()) is locked.  Since
 *	swapper_lock() must be taken at splsched(), we go to splsched()
 *	ourself to cover the latter case.  The make_unswappable argument
 *	is used to ask swapin_thread to make thread unswappable.
 *
 *	The naked references to thr_act->thread here are safe purely based
 *	on the assumption that only an activation itself can remove itself
 *	from an RPC chain.  Since its thread is "swapped out", the activation
 *	cannot be running.  Similarly, the reference to thread->top_act
 *	in calls to this function from scheduler code is safe purely because
 *	an RPC chain can change only under its own steam (i.e., if the
 *	associated thread is running).
 *
 *	XXX - Shuttle cloning would break the above assumptions.
 */

void thread_swapin(
	thread_act_t	thr_act,
	boolean_t	make_unswappable)
{
	spl_t		s;

	switch (thr_act->swap_state & TH_SW_STATE) {
	    case TH_SW_OUT:
		/*
		 *	Swapped out - queue for swapin thread
		 */
		thr_act->swap_state = TH_SW_COMING_IN;
		s = splsched();
		swapper_lock();
		queue_enter(&swapin_queue, thr_act, thread_act_t, swap_queue);
		swapper_unlock();
		thread_wakeup((event_t) &swapin_queue);
		splx(s);
		break;

	    case TH_SW_GOING_OUT:
		/*
		 *	Being swapped out - wait until swapped out,
		 *	then queue for swapin thread (in thread_swapout).
		 */
		thr_act->swap_state = TH_SW_WANT_IN;
		break;

	    case TH_SW_WANT_IN:
	    case TH_SW_COMING_IN:
		/*
		 *	Already queued for swapin thread, or being
		 *	swapped in
		 */
		break;

	    default:
		/*
		 *	Swapped in or unswappable
		 */
		panic("thread_swapin");
	}

	/*
	 *	Set make unswappable flag if asked to.  swapin thread
	 *	will make thread unswappable.
	 */
	if (make_unswappable)
		thr_act->swap_state |= TH_SW_MAKE_UNSWAPPABLE;
}

/*
 *	thread_swapin_blocking: [exported]
 *
 *	Mimic thread_swapin(), but block if swapin of our target thread
 *	has already been initiated, and call thread_doswapin() directly
 *	(instead of via swapin thread) if we initiate it ourself.
 *
 *	Locking: if thr_act is on an RPC chain, the rpc_lock() for the
 *	attached shuttle is held; otherwise both the act_lock() for
 *	thr_act plus the ip_lock() for its thread_pool port are held.
 *	NB:  if thr_act is attached to a thread, thread_lock() controls
 *	access to thr_act's swap_state.
 */
boolean_t thread_swapin_blocking(
	thread_act_t	thr_act)
{
	spl_t		s;
	thread_t	thread, othread;

#define lock_swap_state(thr_act)		\
MACRO_BEGIN					\
	if (thr_act->thread) {			\
		s = splsched();			\
		thread_lock(thr_act->thread);	\
	}					\
MACRO_END

#define unlock_swap_state(thr_act)		\
MACRO_BEGIN					\
	if (thr_act->thread) {			\
		thread_unlock(thr_act->thread);	\
		splx(s);			\
	}					\
MACRO_END

	for (thread = thr_act->thread; ; ) {
		lock_swap_state(thr_act);
		switch (thr_act->swap_state & TH_SW_STATE) {
		case TH_SW_OUT:
			/*
			 * Swapped all the way out -- start it back in.
			 */
			thr_act->swap_state = TH_SW_COMING_IN;
			unlock_swap_state(thr_act);
			if (thread)
				rpc_unlock(thread);
			else {
				ip_unlock(thr_act->pool_port);
				act_unlock(thr_act);
			}
			thread_doswapin(thr_act);
		relock:
			othread = thread;
			thread = act_lock_thread(thr_act);
			/*
			 * We may resume with a shuttle attached
			 * after blocking with no shuttle attached,
			 * but should never move between shuttles.
			 */
			if (othread && thread != othread)
				panic("thread_swapin_blocking: act moved");
			if (othread) {
				/*
				 * XXX - don't care about this ip_lock (taken
				 * by act_lock_thread() if pool_port non-null)
				 */
				if (thr_act->pool_port)
				    	ip_unlock(thr_act->pool_port);
				act_unlock(thr_act);
			}
			else if (thread) {
				/*
				 * XXX - don't care about this rpc_lock (taken
				 * by act_lock_thread() if thread non-null)
				 */
				rpc_unlock(thr_act->thread);
			}
			break;

		case TH_SW_GOING_OUT:
			/*
			 * Not out yet -- have to wait till it is, then queue
			 * it for swapin.
			 */
			thr_act->swap_state = TH_SW_WANT_IN;

			/* fall-through */

		case TH_SW_WANT_IN:
		case TH_SW_COMING_IN:
			/*
			 * On its way back -- wait for it to get here.
			 */
			assert_wait((event_t)&thr_act->swap_state, FALSE);
			unlock_swap_state(thr_act);
			if (thread)
				rpc_unlock(thread);
			else {
				ip_unlock(thr_act->pool_port);
				act_unlock(thr_act);
			}
			thread_block((void (*)(void))0);
			goto relock;
			break;

		default:
			/*
			 * Swapped in or not swappable -- finally!
			 */
			unlock_swap_state(thr_act);
			return (TRUE);
		}
		if (!thr_act->active)
			break;	/* out of loop -- don'tcha love C? */
	}
	return (FALSE);
}

#undef	lock_swap_state
#undef	unlock_swap_state

/*
 *	thread_doswapin: [exported]
 *
 *	Swapin the specified thread, if it should be runnable, then put
 *	it on a run queue.  No locks should be held on entry, as it is
 *	likely that this routine will sleep (waiting for page faults).
 */
void thread_doswapin(
	thread_act_t	thr_act)
{
	register vm_offset_t	addr;
	register int		s;
	kern_return_t		kr;
	thread_t		thread;

	/*
	 *	Wire down the kernel stack.
	 */

	if (thread_swap_unwire_stack && (thread = thr_act->thread)) {
		assert(KERNEL_STACK_SIZE % page_size == 0);
		addr = thread->kernel_stack;
		/*
		 * This pagein can never fail
		 */
#if	THREAD_SW_DEBUG
		if (thread_swap_debug) {
			printf("thread_doswapin(%x): wiring stack %x\n",
			       thr_act, trunc_page(addr));
		}
#endif	/* THREAD_SW_DEBUG */
#if	MACH_ASSERT
		assert(thr_act->kernel_stack_swapped_in == FALSE);
		thr_act->kernel_stack_swapped_in = TRUE;
#endif	/* MACH_ASSERT */
		for (;;) {
			kr = vm_map_wire(kernel_map, trunc_page(addr),
					 round_page(addr + KERNEL_STACK_SIZE),
					 VM_PROT_READ|VM_PROT_WRITE,
					 FALSE);
			if (kr == KERN_SUCCESS) {
				break;
			}
			if (kr != KERN_MEMORY_ERROR) {
				printf("vm_map_wire returned 0x%x\n", kr);
				panic("thread_doswapin");
			}
			thread_block((void (*)(void)) 0);
		}
		vm_map_simplify(kernel_map, addr);
	}

#if	THREAD_SWAP_UNWIRE_USER_STACK
	if (thread_swap_unwire_user_stack && thr_act->user_stack) {
		vm_offset_t	user_stack;
		long		user_stack_size;	/* signed ! */
		vm_offset_t	start, end;
		
		user_stack = thr_act->user_stack;
		user_stack_size = (long) thr_act->user_stack_size;
#if 0
		assert(thr_act->user_stack_size % page_size == 0);
#endif
#if	THREAD_SW_DEBUG
		if (thread_swap_debug) {
			printf("thread_doswapin(%x): wiring user stack %x\n",
			       thr_act, trunc_page(user_stack));
		}
#endif	/* THREAD_SW_DEBUG */
#if	MACH_ASSERT
		assert(thr_act->user_stack_swapped_in == FALSE);
		thr_act->user_stack_swapped_in = TRUE;
#endif	/* MACH_ASSERT */
		assert(thr_act->map != VM_MAP_NULL);
		if (user_stack_size > 0) {
			/* stack grows up */
			start = trunc_page(user_stack);
			end = round_page(user_stack + user_stack_size - 1);
		} else {
			/* stack grows down */
			start = trunc_page(user_stack + user_stack_size + 1);
			end = round_page(user_stack);
		}
		for (;;) {
			kr = vm_map_wire(thr_act->map, start, end,
					 VM_PROT_READ|VM_PROT_WRITE,
					 TRUE);
			if (kr == KERN_SUCCESS) {
				break;
			}
			if (kr != KERN_MEMORY_ERROR) {
				printf("vm_map_wire(user) returned 0x%x\n", kr);
				panic("thread_doswapin 2");
			}
			thread_block((void (*)(void)) 0);
		}
		vm_map_simplify(thr_act->map, start);
	}
#endif	/* THREAD_SWAP_UNWIRE_USER_STACK */

	/*
	 *	Make unswappable and wake up waiting thread(s) if needed.
	 *	Place on run queue if appropriate.  
	 *
	 *	This function only operates on threads that had a state
	 *	of TH_SW_OUT at one time, in which case, there needs
	 *	to be a reference on the task's resident thread count.
	 */

	/*
	 * Increment the resident thread count for the task.
	 * This has no direct effect here, but serves as a counter
	 * for the pmap_collect call in thread_swapout.
	 */
	if (thr_act->task != TASK_NULL) {
		mutex_lock(&thr_act->task->act_list_lock);
		++thr_act->task->res_act_count;
		mutex_unlock(&thr_act->task->act_list_lock);
	}

	thread = act_lock_thread(thr_act);
	if (thread != THREAD_NULL) {
		s = splsched();
		thread_lock(thread);
	}
	if (thr_act->swap_state & TH_SW_MAKE_UNSWAPPABLE) {
		thr_act->swap_state = TH_SW_UNSWAPPABLE;
	} else {
		thr_act->swap_state = TH_SW_IN;
	}

	if (thread != THREAD_NULL) {
		if (thread->top_act == thr_act) {
			thread->state &= ~TH_SWAPPED_OUT;
			if (thread->state & TH_RUN)
				thread_setrun(thread, TRUE, TAIL_Q); /* XXX or HEAD_Q */
		}
		thread_unlock(thread);
		(void) splx(s);
	}
	thread_wakeup((event_t)&thr_act->swap_state);
	act_unlock_thread(thr_act);
}

/*
 *	thread_swapout: [exported]
 *
 *	Swap out the specified thread (unwire its kernel stack).
 *	The thread must already be marked as 'swapping out'.
 *
 *	Decrement the resident thread count for the task, and if
 *	it goes to zero, call pmap_collect on the task's pmap.
 *	
 *	The thread has an extra reference (obtained by the caller)
 *	to keep it from being deallocated during swapout.
 */
void thread_swapout(
	thread_act_t	thr_act)
{
	register vm_offset_t	addr;
	register boolean_t	make_unswappable;
	boolean_t		collect;
	int			s;
	kern_return_t 		kr;
	thread_t		thread;

	/*
	 *	Thread is marked as swapped before we swap it out; if
	 *	it is awakened while we are swapping it out, it will
	 *	be put on the swapin list.
	 */

	/*
	 * Notify the pcb module that it must update any
 	 * hardware state associated with this thread.
	 */
#if	0	/* XXX */
	pcb_synch(thread);
#endif

	/*
	 *	Unwire the kernel stack.
	 * 	No need to lock the act before acquiring thread to
	 * 	get its kernel_stack address, since the thread is blocked
	 *	somewhere and can't restart without being swapped back in.
	 */

	if (thread_swap_unwire_stack && (thread = thr_act->thread)) {
		assert(KERNEL_STACK_SIZE % page_size == 0);
		addr = thread->kernel_stack;
#if	THREAD_SW_DEBUG
		if (thread_swap_debug) {
			printf("thread_swapout(%x):unwiring stack %x\n",
			       thread, trunc_page(addr));
		}
#endif	/* THREAD_SW_DEBUG */
#if	MACH_ASSERT
		assert(thr_act->kernel_stack_swapped_in == TRUE);
		thr_act->kernel_stack_swapped_in = FALSE;
#endif	/* MACH_ASSERT */
		kr = vm_map_unwire(kernel_map, trunc_page(addr),
				   round_page(addr + 
					      KERNEL_STACK_SIZE),
				   FALSE);
		if (kr != KERN_SUCCESS) {
			printf("vm_map_unwire returned %x\n", kr);
			panic("thread_swapout");
		}
		vm_map_simplify(kernel_map, addr);
	}

#if	THREAD_SWAP_UNWIRE_USER_STACK
	if (thread_swap_unwire_user_stack && thr_act->user_stack) {
		vm_offset_t	user_stack;
		long		user_stack_size;	/* signed ! */
		vm_offset_t	start, end;

		user_stack = thr_act->user_stack;
		user_stack_size = thr_act->user_stack_size;
#if 0
		assert(user_stack_size % page_size == 0);
#endif
#if	THREAD_SW_DEBUG
		if (thread_swap_debug) {
			printf("thread_swapout(%x):unwiring user stack %x\n",
			       thr_act, trunc_page(user_stack));
		}
#endif	/* THREAD_SW_DEBUG */
#if	MACH_ASSERT
		assert(thr_act->user_stack_swapped_in == TRUE);
		thr_act->user_stack_swapped_in = FALSE;
#endif	/* MACH_ASSERT */
		assert(thr_act->map != VM_MAP_NULL);
		if (user_stack_size > 0) {
			/* stack grows up */
			start = trunc_page(user_stack);
			end = round_page(user_stack + user_stack_size - 1);
		} else {
			/* stack grows down */
			start = trunc_page(user_stack + user_stack_size + 1);
			end = round_page(user_stack);
		}
		kr = vm_map_unwire(thr_act->map, start, end, TRUE);
		if (kr != KERN_SUCCESS) {
			printf("vm_map_unwire(user_stack) returned %x\n", kr);
			panic("thread_swapout 2");
		}
		vm_map_simplify(thr_act->map, start);
	}
#endif	/* THREAD_SWAP_UNWIRE_USER_STACK */

	/*
	 * Arrange to call pmap_collect if this is the last resident
	 * thread for its task.  The task and its map aren't going
	 * anywhere because of the extra reference on this thread.
	 */
	mutex_lock(&thr_act->task->act_list_lock);
	collect = (--thr_act->task->res_act_count == 0);
	mutex_unlock(&thr_act->task->act_list_lock);

	thread = act_lock_thread(thr_act);
	s = splsched();
	if (thread)
		thread_lock(thread);
	switch (thr_act->swap_state & TH_SW_STATE) {
	    case TH_SW_GOING_OUT:
		thr_act->swap_state = TH_SW_OUT;
		break;

	    case TH_SW_WANT_IN:
		/* didn't get it out fast enough */
		make_unswappable = thr_act->swap_state & TH_SW_MAKE_UNSWAPPABLE;
		thr_act->swap_state = TH_SW_OUT;
		thread_swapin(thr_act, make_unswappable);
		collect = FALSE;	/* don't pmap_collect */
		break;

	    default:
		panic("thread_swapout");
	}
	if (thread)
		thread_unlock(thread);
	splx(s);
	act_unlock_thread(thr_act);
	if (collect) {
		/* task and map can't disappear yet */
		assert((thr_act->task != TASK_NULL) &&
			(thr_act->task->map != VM_MAP_NULL));
		pmap_collect(vm_map_pmap(thr_act->task->map));
	}
}

/*
 *	swapin_thread: [exported]
 *
 *	This procedure executes as a kernel thread.  Threads that need to
 *	be swapped in are swapped in by this thread.
 */
void swapin_thread(void)
{
	thread_swappable(current_act(), FALSE);
	stack_privilege(current_thread());

	while (TRUE) {
		register thread_act_t	thr_act;
		register int		s;

		s = splsched();
		swapper_lock();

		while (! queue_empty(&swapin_queue)) {
			queue_remove_first(&swapin_queue, thr_act, thread_act_t,
					   swap_queue);
			swapper_unlock();
			splx(s);

			thread_doswapin(thr_act);

			s = splsched();
			swapper_lock();
		}

		assert_wait((event_t) &swapin_queue, FALSE);
		swapper_unlock();
		splx(s);
		thread_block((void (*)(void)) 0);
	}
}

boolean_t	thread_swapout_allowed = TRUE;

int	last_swap_tick = 0;

/*
 *	swapout_threads: [exported]
 *
 *	This procedure is called periodically by the pageout daemon.  It
 *	determines if it should scan for threads to swap and starts that
 *	scan if appropriate.
 *	The pageout daemon sets the now flag if all the page queues are
 *	empty and it wants to start the swapper right away.
 */
void swapout_threads(
	boolean_t	now)
{
	if (thread_swapout_allowed &&
	    (now || (sched_tick > (last_swap_tick + max_swap_rate)))) {
		last_swap_tick = sched_tick;
		thread_wakeup((event_t) &last_swap_tick); /* poke swapper */
		thread_block((void (*)(void)) 0);/* let it run if it wants to */
	}
}

boolean_t thread_swapout_empty_acts = FALSE;

void swapout_scan(void); /* forward */

/*
 *	swapout_scan:
 *
 *	Scan the list of all threads looking for threads to swap.
 */
void swapout_scan(void)
{
	register int		s;
	register thread_t	thread, prev_thread;
	processor_set_t		pset, prev_pset;
	thread_act_t		thr_act, prev_act;
	task_t			task, prev_task;

	zone_gc();

	prev_thread = THREAD_NULL;
	prev_act = THR_ACT_NULL;
	prev_task = TASK_NULL;
	prev_pset = PROCESSOR_SET_NULL;
	/*
	 * Ugh.  If an activation is terminated while being swapped
	 * out, we can no longer count on consistency of the lists
	 * we're traversing, so we have to start over.
	 *
	 * Note: we deliberately leave the "prev" variables alone if
	 * we restart -- we will remove a ref to them as needed in
	 * the normal operation of the loop.
	 */
restart:
	mutex_lock(&all_psets_lock);
	pset = (processor_set_t) queue_first(&all_psets);
	while (!queue_end(&all_psets, (queue_entry_t) pset)) {
		pset_lock(pset);
		task = (task_t) queue_first(&pset->tasks);
		while (!queue_end(&pset->tasks, (queue_entry_t) task)) {
			task_lock(task);
			thr_act = (thread_act_t) queue_first(&task->thr_acts);
			while (!queue_end(&task->thr_acts,
					  (queue_entry_t) thr_act)) {
				boolean_t swap_it;
				thread_act_t act;

				thread = act_lock_thread(thr_act);
				s = splsched();
				if (thread != THREAD_NULL) {
					thread_lock(thread);
					swap_it = TRUE;
					for (act = thread->top_act; act; act = act->lower)
						if (act->swap_state == TH_SW_UNSWAPPABLE) {
							swap_it = FALSE;
							break;
						}
					if (!swap_it)
						;
					else if (
#if	MACH_RT
					    /* don't swap real-time threads */
					    (thread->policy &
					     POLICYCLASS_FIXEDPRI) != 0 || 
#endif	/* MACH_RT */
					    (thread->state &
					     (TH_RUN|TH_SWAPPED_OUT)) != 0 ||
					    thr_act->swap_state != TH_SW_IN ||
					    (thread->state & TH_UNINT) != 0 ||
					    (sched_tick-thread->sleep_stamp <=
					     maxslp) ||
					    !thr_act->active) {
						swap_it = FALSE;
					}
				} else {
					if (! thread_swapout_empty_acts) {
						splx(s);
						act_unlock_thread(thr_act);
						thr_act = (thread_act_t)
							queue_next(&thr_act->thr_acts);
						continue;
					}
					if (thr_act->swap_state == TH_SW_IN &&
					    thr_act->active &&
					    thr_act->thread_pool_next) {
						swap_it = TRUE;
					} else {
						swap_it = FALSE;
					}
				}
				if (swap_it) {
					thr_act->swap_state = TH_SW_GOING_OUT;
					if (thread != THREAD_NULL) {
						if (thread->top_act ==
							thr_act) {
							thread->state |=
								TH_SWAPPED_OUT;
						}
						thread->ref_count++;
						thread_unlock(thread);
					}
					act_locked_act_reference(thr_act);
					(void) splx(s);
					act_unlock_thread(thr_act);
					task->ref_count++;
					task_unlock(task);
					pset->ref_count++;
					pset_unlock(pset);
					mutex_unlock(&all_psets_lock);

					thread_swapout(thr_act); /* swap it */

					if (prev_thread != THREAD_NULL) {
						thread_deallocate(prev_thread);
					}
					if (prev_act != THR_ACT_NULL) {
						act_deallocate(prev_act);
					}
					if (prev_task != TASK_NULL) {
						task_deallocate(prev_task);
					}
					if (prev_pset != PROCESSOR_SET_NULL)
						pset_deallocate(prev_pset);

					prev_thread = thread;
					prev_act = thr_act;
					prev_task = task;
					prev_pset = pset;
					mutex_lock(&all_psets_lock);
					pset_lock(pset);
					task_lock(task);
					thread = act_lock_thread(thr_act);
					/*
					 * See if current act was terminated; if so,
					 * can't continue with current traversal --
					 * we don't have a valid "next" pointer, and
					 * there's no certain way to acquire one.
					 *
					 * Also check for task having moved between
					 * psets; don't want to continue current
					 * traversal if it has.  (The traversal would
					 * be valid in this case, just not complete.)
					 */
					if (!thr_act->active || task->processor_set != pset) {
						act_unlock_thread(thr_act);
						task_unlock(task);
						pset_unlock(pset);
						mutex_unlock(&all_psets_lock);
						goto restart;
					}
				
					s = splsched();
					assert(!prev_thread || thread == prev_thread);
				} else {
					if (thread != THREAD_NULL) 
						thread_unlock(thread);
				}
				splx(s);
				act_unlock_thread(thr_act);
				thr_act = (thread_act_t)
					queue_next(&thr_act->thr_acts);
			}
			task_unlock(task);
			task = (task_t) queue_next(&task->pset_tasks);
		}
		pset_unlock(pset);
		pset = (processor_set_t) queue_next(&pset->all_psets);
	}
	mutex_unlock(&all_psets_lock);

	if (prev_thread != THREAD_NULL) {
		thread_deallocate(prev_thread);
	}
	if (prev_act != THR_ACT_NULL) {
		act_deallocate(prev_act);
	}
	if (prev_task != TASK_NULL) {
		task_deallocate(prev_task);
	}
	if (prev_pset != PROCESSOR_SET_NULL)
		pset_deallocate(prev_pset);
}

/*
 *	swapout_thread: [exported]
 *
 *	Executes as a separate kernel thread.  This thread is periodically
 *	woken up.  When this happens, it initiates the scan for threads
 *	to swap.
 */
void swapout_thread(void)
{
	thread_swappable(current_act(), FALSE);
	stack_privilege(current_thread());

	spllo();
	while (TRUE) {
		swapout_scan();

		assert_wait((event_t)&last_swap_tick, FALSE);
		thread_block((void (*) (void)) 0);
	}
}

/*
 *	Mark a thread as swappable or unswappable.  May be called at
 *	any time.  No longer assumes thread is swapped in.  Frees
 *	kernel stack backing store when a kernel thread is made
 *	unswappable.  Panics if a kernel thread is subsequently made
 *	swappable.
 */
void thread_swappable(
	thread_act_t	thr_act,
	boolean_t	is_swappable)
{
	int		s;
	thread_t	thread;

	thread = act_lock_thread(thr_act);
	if (thread) {
		s = splsched();
		thread_lock(thread);
	}
	if (is_swappable) {
	    if (thr_act->swap_state == TH_SW_UNSWAPPABLE) {
		if (thr_act->task == kernel_task) {
			panic("thread_swappable");
		}
		thr_act->swap_state = TH_SW_IN;
	    }
	}
	else {
	    switch(thr_act->swap_state) {
		case TH_SW_UNSWAPPABLE:
			/*
			 * Thread is already unswappable, won't need to
			 * free backing store.
			 */
			is_swappable = TRUE;
		    break;

		case TH_SW_IN:
		    thr_act->swap_state = TH_SW_UNSWAPPABLE;
		    break;

		default:
		    do {
			thread_swapin(thr_act, TRUE);
			assert_wait((event_t) &thr_act->swap_state, FALSE);
			if (thread) {
				thread_unlock(thread);
				splx(s);
			}
			act_unlock_thread(thr_act);
			thread_block((void (*)(void))0);
			thread = act_lock_thread(thr_act);
			if (thread) {
				s = splsched();
				thread_lock(thread);
			}
		    } while (thr_act->swap_state != TH_SW_UNSWAPPABLE);
		    break;
	    }
	}
	if (thread) {
		thread_unlock(thread);
		splx(s);
	}
	act_unlock_thread(thr_act);

	/* 
	 * Deallocate kernel stack backing store for any
	 * kernel thread made unswappable.
	 */
	if (!is_swappable && thr_act->task == kernel_task) {
#if 0
		stack_backing_free(thr_act->kernel_stack, KERNEL_STACK_SIZE);
#endif
	}
}

int thread_swap_disable_swapins = 0;

void
thread_swap_disable(
	thread_act_t	thr_act)
{
	spl_t		s;
	thread_t	thread;

	/*
	 * Note: we have to swapin the thread only to make sure
	 * that its stacks are wired when we free them.
	 */
	thread = act_lock_thread(thr_act);
	if (thread) {
		s = splsched();
		thread_lock(thread);
	}
	if ((thread_swap_unwire_stack || thread_swap_unwire_user_stack) &&
	    (thr_act->swap_state != TH_SW_UNSWAPPABLE)) {
		/*
		 * Make the activation unswappable or it might get swapped
		 * before we complete its termination.
		 */
		if (thr_act->swap_state == TH_SW_IN ||
		    thr_act->swap_state == (TH_SW_IN|TH_SW_TASK_SWAPPING)) {
			thr_act->swap_state = TH_SW_UNSWAPPABLE;
			if (thread) {
				thread_unlock(thread);
				splx(s);
			}
			act_unlock_thread(thr_act);
		} else {
			thr_act->swap_state |= TH_SW_MAKE_UNSWAPPABLE;
			if (thread) {
				thread_unlock(thread);
				splx(s);
			}
			act_unlock_thread(thr_act);
			thread_swap_disable_swapins++;
			thread_doswapin(thr_act);
		}

		assert(thr_act->swap_state == TH_SW_UNSWAPPABLE);
	} else {
		if (thread) {
			thread_unlock(thread);
			splx(s);
		}
		act_unlock_thread(thr_act);
	}
}
