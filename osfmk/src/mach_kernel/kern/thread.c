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
 * Revision 2.22.6.5  92/09/15  17:23:04  jeffreyh
 * 	Deadlock fix: Acquire pset lock before task lock in
 * 		thread_deallocate().
 * 	[92/09/03            dlb]
 * 	Cleanup for profiling
 * 	[92/07/24            bernadat]
 * 
 * 	Fixed rcs history
 * 	[92/08/12            jeffreyh]
 * 
 * 	Add thread_abort_safely and modifications to thread_halt to allow
 * 	a caller to only halt a thread in a restartable manner.
 * 	[92/08/06            dlb]
 * 
 * Revision 2.22.6.4  92/04/08  15:44:48  jeffreyh
 * 	Rework thread_halt_self to check contents of ast field in halt
 * 	(but not terminate) case, and to check ast field under lock.
 * 	[92/04/08            dlb]
 * 
 * Revision 2.22.6.3  92/03/28  10:10:42  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:17:58  jeffreyh]
 * 
 * Revision 2.24  92/02/19  16:07:03  elf
 * 	Change calls to compute_priority.
 * 	[92/01/19            rwd]
 * 
 * Revision 2.23  92/01/03  20:18:56  dbg
 * 	Make thread_wire really reserve a kernel stack.
 * 	[91/12/18            dbg]
 * 
 * Revision 2.22.6.2  92/03/03  16:20:44  jeffreyh
 * 	Fixed an assert.
 * 	[92/03/03  10:10:13  jeffreyh]
 * 
 * 	Remove continuation from AST_TERMINATE case of thread_halt_self 
 * 	so that "zombie walks" is reachable.
 * 	[92/02/25            dlb]
 * 
 * Revision 2.22.6.1  92/02/18  19:11:56  jeffreyh
 * 	Fixed Profiling code. [emcmanus@gr.osf.org]
 * 
 * Revision 2.22  91/12/13  14:54:53  jsb
 * 	Removed thread_resume_from_kernel.
 * 	[91/12/12  17:40:12  af]
 * 
 * Revision 2.21  91/08/28  11:14:43  jsb
 * 	Fixed thread_halt, mach_msg_interrupt interaction.
 * 	Added checks for thread_exception_return, thread_bootstrap_return.
 * 	[91/08/03            rpd]
 * 
 * Revision 2.20.3.1  91/09/26  04:48:40  bernadat
 * 	Thread profiling fields initialization
 * 	(Bernard Tabib & Andrei Danes @ gr.osf.org)
 * 	[91/09/16            bernadat]
 * 
 * Revision 2.20  91/07/31  17:49:10  dbg
 * 	Call pcb_module_init from thread_init.
 * 	[91/07/26            dbg]
 * 
 * 	When halting a thread: if it is waiting at a continuation with a
 * 	known cleanup routine, call the cleanup routine instead of
 * 	resuming the thread.
 * 	[91/06/21            dbg]
 * 
 * 	Revise scheduling state machine.
 * 	[91/05/22            dbg]
 * 
 * 	Add thread_wire.
 * 	[91/05/14            dbg]
 * 
 * Revision 2.19  91/06/25  10:29:48  rpd
 * 	Picked up dlb's thread_doassign no-op fix.
 * 	[91/06/23            rpd]
 * 
 * Revision 2.18  91/05/18  14:34:09  rpd
 * 	Fixed stack_alloc to use kmem_alloc_aligned.
 * 	[91/05/14            rpd]
 * 	Added argument to kernel_thread.
 * 	[91/04/03            rpd]
 * 
 * 	Changed thread_deallocate to reset timer and depress_timer.
 * 	[91/03/31            rpd]
 * 
 * 	Replaced stack_free_reserved and swap_privilege with stack_privilege.
 * 	[91/03/30            rpd]
 * 
 * Revision 2.17  91/05/14  16:48:35  mrt
 * 	Correcting copyright
 * 
 * Revision 2.16  91/05/08  12:49:14  dbg
 * 	Add volatile declarations.
 * 	[91/04/26  14:44:13  dbg]
 * 
 * Revision 2.15  91/03/16  14:52:40  rpd
 * 	Fixed the initialization of stack_lock_data.
 * 	[91/03/11            rpd]
 * 	Updated for new kmem_alloc interface.
 * 	[91/03/03            rpd]
 * 	Removed ith_saved.
 * 	[91/02/16            rpd]
 * 
 * 	Added stack_alloc_max.
 * 	[91/02/10            rpd]
 * 	Added active_stacks.
 * 	[91/01/28            rpd]
 * 	Can't use thread_dowait on the current thread now.
 * 	Added reaper_thread_continue.
 * 	Added stack_free_reserved.
 * 	[91/01/20            rpd]
 * 
 * 	Removed thread_swappable.
 * 	Allow swapped threads on the run queues.
 * 	Changed the AST interface.
 * 	[91/01/17            rpd]
 * 
 * Revision 2.14  91/02/05  17:30:14  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:19:30  mrt]
 * 
 * Revision 2.13  91/01/08  15:17:57  rpd
 * 	Added KEEP_STACKS support.
 * 	[91/01/06            rpd]
 * 	Added consider_thread_collect, thread_collect_scan.
 * 	[91/01/03            rpd]
 * 
 * 	Added locking to the stack package.  Added stack_collect.
 * 	[90/12/31            rpd]
 * 	Changed thread_dowait to discard the stacks of suspended threads.
 * 	[90/12/22            rpd]
 * 	Added continuation argument to thread_block.
 * 	[90/12/08            rpd]
 * 
 * 	Changed thread_create to make new threads be swapped.
 * 	Changed kernel_thread to swapin the threads.
 * 	[90/11/20            rpd]
 * 
 * 	Removed stack_free/stack_alloc/etc.
 * 	[90/11/12            rpd]
 * 
 * 	Changed thread_create to let pcb_init handle all pcb initialization.
 * 	[90/11/11            rpd]
 * 
 * Revision 2.12  90/12/05  23:29:09  af
 * 
 * 
 * Revision 2.11  90/12/05  20:42:19  af
 * 	Added (temporarily, until we define the right new primitive with
 * 	the RTMach guys) thread_resume_from_kernel() for internal kernel
 * 	use only.
 * 
 * 	Revision 2.7.1.1  90/09/25  13:06:04  dlb
 * 	Inline thread_hold in thread_suspend and thread_release in
 * 	thread_resume (while still holding the thread lock in both).
 * 	This eliminates a long-standing MP bug.
 * 	[90/09/20            dlb]
 * 
 * Revision 2.10  90/11/05  14:31:46  rpd
 * 	Unified untimeout and untimeout_try.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.9  90/10/25  14:45:37  rwd
 * 	Added host_stack_usage and processor_set_stack_usage.
 * 	[90/10/22            rpd]
 * 
 * 	Removed pausing code in stack_alloc/stack_free.
 * 	It was broken and it isn't needed.
 * 	Added code to check how much stack is actually used.
 * 	[90/10/21            rpd]
 * 
 * Revision 2.8  90/10/12  12:34:37  rpd
 * 	Fixed HW_FOOTPRINT code in thread_create.
 * 	Fixed a couple bugs in thread_policy.
 * 	[90/10/09            rpd]
 * 
 * Revision 2.7  90/08/27  22:04:03  dbg
 * 	Fixed thread_info to return the correct count.
 * 	[90/08/23            rpd]
 * 
 * Revision 2.6  90/08/27  11:52:25  dbg
 * 	Remove unneeded mode parameter to thread_start.
 * 	Remove u_zone, thread_deallocate_interrupt.
 * 	[90/07/17            dbg]
 * 
 * Revision 2.5  90/08/07  17:59:04  rpd
 * 	Removed tmp_address, tmp_object fields.
 * 	Picked up depression abort functionality in thread_abort.
 * 	Picked up initialization of max_priority in kernel_thread.
 * 	Picked up revised priority computation in thread_create.
 * 	Removed in-transit check from thread_max_priority.
 * 	Picked up interprocessor-interrupt optimization in thread_dowait.
 * 	[90/08/07            rpd]
 * 
 * Revision 2.4  90/06/02  14:56:54  rpd
 * 	Converted to new IPC and scheduling technology.
 * 	[90/03/26  22:24:06  rpd]
 * 
 * Revision 2.3  90/02/22  20:04:12  dbg
 * 	Initialize per-thread global VM variables.
 * 	Clean up global variables in thread_deallocate().
 * 		[89/04/29	mwyoung]
 * 
 * Revision 2.2  89/09/08  11:26:53  dbg
 * 	Allocate thread kernel stack with kmem_alloc_wired to get
 * 	separate vm_object for it.
 * 	[89/08/18            dbg]
 * 
 * 19-Aug-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed all non-MACH code.
 *
 * 11-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	Rewrote exit logic to use new ast mechanism.  Includes locking
 *	bug fix to thread_terminate().
 *
 *  9-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	first_quantum replaces preempt_pri.
 *
 * Revision 2.3  88/08/06  18:26:41  rpd
 * Changed to use ipc_thread_lock/ipc_thread_unlock macros.
 * Eliminated use of kern/mach_ipc_defs.h.
 * Added definitions of all_threads, all_threads_lock.
 * 
 *  4-May-88  David Golub (dbg) and David Black (dlb) at CMU
 *	Remove vax-specific code.  Add register declarations.
 *	MACH_TIME_NEW now standard.  Moved thread_read_times to timer.c.
 *	SIMPLE_CLOCK: clock drift compensation in cpu_usage calculation.
 *	Initialize new fields in thread_create().  Implemented cpu usage
 *	calculation in thread_info().  Added argument to thread_setrun.
 *	Initialization changes for MACH_TIME_NEW.
 *
 * 13-Apr-88  David Black (dlb) at Carnegie-Mellon University
 *	Rewrite kernel stack retry code to eliminate races and handle
 *	termination correctly.
 *
 * 19-Feb-88  David Kirschen (kirschen) at Encore Computer Corporation
 *      Retry if kernel stacks exhausted on thread_create
 *
 * 12-Feb-88  David Black (dlb) at Carnegie-Mellon University
 *	Fix MACH_TIME_NEW code.
 *
 *  1-Feb-88  David Golub (dbg) at Carnegie-Mellon University
 *	In thread_halt: mark the victim thread suspended/runnable so that it
 *	will notify the caller when it hits the next interruptible wait.
 *	The victim may not immediately proceed to a clean point once it
 *	is awakened.
 *
 * 21-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Use new swapping state machine.  Moved thread_swappable to
 *	thread_swap.c.
 *
 * 17-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Added new thread interfaces: thread_suspend, thread_resume,
 *	thread_get_state, thread_set_state, thread_abort,
 *	thread_info.  Old interfaces remain (temporarily) for binary
 *	compatibility, prefixed with 'xxx_'.
 *
 * 29-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Delinted.
 *
 * 15-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Made thread_reference and thread_deallocate check for null
 *	threads.  Call pcb_terminate when a thread is deallocated.
 *	Call pcb_init with thread pointer instead of pcb pointer.
 *	Add missing case to thread_dowait.
 *
 *  9-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Rewrote thread termination to have a terminating thread clean up
 *	after itself.
 *
 *  9-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Moved reaper invocation to thread_terminate from
 *	thread_deallocate.  [XXX temporary pending rewrite.]
 *
 *  8-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Added call to ipc_thread_disable.
 *
 *  4-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Set ipc_kernel in kernel_thread().
 *
 *  3-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Rewrote thread_create().  thread_terminate() must throw away
 *	an extra reference if called on the current thread [ref is
 *	held by caller who will not be returned to.]  Locking bug fix
 *	to thread_status.
 *
 * 19-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Eliminated TT conditionals.
 *
 * 30-Oct-87  David Golub (dbg) at Carnegie-Mellon University
 *	Fix race condition in thread_deallocate for thread terminating
 *	itself.
 *
 * 23-Oct-87  David Golub (dbg) at Carnegie-Mellon University
 *	Correctly set thread_statistics fields.
 *
 * 13-Oct-87  David Black (dlb) at Carnegie-Mellon University
 *	Use counts for suspend and resume primitives.
 *
 *  5-Oct-87  David Golub (dbg) at Carnegie-Mellon University
 *	MACH_TT: Completely replaced scheduling state machine.
 *
 * 30-Sep-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added initialization of thread->flags in thread_create().
 *	Added thread_swappable().
 *	De-linted.
 *
 * 30-Sep-87  David Black (dlb) at Carnegie-Mellon University
 *	Rewrote thread_dowait to more effectively stop threads.
 *
 * 11-Sep-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Initialize thread fields and unix_lock.
 *
 *  9-Sep-87  David Black (dlb) at Carnegie-Mellon University
 *	Changed thread_dowait to count a thread as stopped if it is
 *	sleeping and will stop immediately when woken up.  [i.e. is
 *	sleeping interruptibly].  Corresponding change to
 *	thread_terminate().
 *
 *  4-Aug-87  David Golub (dbg) at Carnegie-Mellon University
 *	Moved ipc_thread_terminate to thread_terminate (from
 *	thread_deallocate), to shut out other threads that are
 *	manipulating the thread via its thread_port.
 *
 * 29-Jul-87  David Golub (dbg) at Carnegie-Mellon University
 *	Make sure all deallocation calls are outside of locks - they may
 *	block.  Moved task_deallocate from thread_deallocate to
 *	thread_destroy, since thread may blow up if task disappears while
 *	thread is running.
 *
 * 26-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Added update_priority() call to thread_release() for any thread
 *	actually released.
 *
 * 23-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Initialize thread priorities in thread_create() and kernel_thread().
 *
 * 10-Jun-87  Karl Hauth (hauth) at Carnegie-Mellon University
 *	Added code to fill in the thread_statistics structure.
 *
 *  1-Jun-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added thread_statistics stub.
 *
 * 21-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Clear the thread u-area upon creation of a thread to keep
 *	consistent.
 *
 *  4-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Call uarea_init to initialize u-area stuff.
 *
 * 29-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Moved call to ipc_thread_terminate into the MACH_TT only branch
 *	to prevent problems with non-TT systems.
 *
 * 28-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Support the thread status information as a MiG refarray.
 *	[NOTE: turned off since MiG is still too braindamaged.]
 *
 * 23-Apr-87  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Moved ipc_thread_terminate to thread_deallocate from
 *	thread_destroy to eliminate having the reaper call it after
 *	the task has been freed.
 *
 * 18-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added reaper thread for deallocating threads that cannot
 *	deallocate themselves (some time ago).
 *
 * 17-Mar-87  David Golub (dbg) at Carnegie-Mellon University
 *	De-linted.
 *
 * 14-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Panic if no space left in the kernel map for stacks.
 *
 *  6-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add kernel_thread routine which starts up kernel threads.
 *
 *  4-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Make thread_terminate work.
 *
 *  2-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	New kernel stack allocation mechanism.
 *
 * 27-Feb-87  David L. Black (dlb) at Carnegie-Mellon University
 *	MACH_TIME_NEW: Added timer inits to thread_create().
 *
 * 24-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Rewrote thread_suspend/thread_hold and added thread_wait for new
 *	user synchronization paradigm.
 *
 * 24-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Reorded locking protocol in thread_deallocate for the
 *	all_threads_lock (this allows one to reference a thread then
 *	release the all_threads_lock when scanning the thread list).
 *
 * 31-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged in my changes for real thread implementation.
 *
 * 30-Sep-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Make floating u-area work, maintain list of threads per task.
 *
 *  1-Aug-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added initialization for Mach-style IPC.
 *
 *  7-Jul-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added thread_in_use_chain to keep track of threads which
 *	have been created but not yet destroyed.
 *
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Initialize thread state field to THREAD_WAITING.  Some general
 *	cleanup.
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
 *	File:	kern/thread.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *	Date:	1986
 *
 *	Thread/thread_shuttle management primitives implementation.
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
 */

#include <cpus.h>
#include <mach_rt.h>
#include <hw_footprint.h>
#include <mach_host.h>
#include <simple_clock.h>
#include <mach_debug.h>
#include <mach_prof.h>
#include <dipc.h>
#include <stack_usage.h>

#include <mach/boolean.h>
#include <mach/policy.h>
#include <mach/thread_info.h>
#include <mach/thread_special_ports.h>
#include <mach/thread_status.h>
#include <mach/time_value.h>
#include <mach/vm_param.h>
#include <kern/ast.h>
#include <kern/counters.h>
#include <kern/etap_macros.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/mach_param.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/thread_act.h>
#include <kern/thread_swap.h>
#include <kern/host.h>
#include <kern/zalloc.h>
#include <vm/vm_kern.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/mach_msg.h>
#include <machine/thread.h>		/* for MACHINE_STACK */
#include <kern/profile.h>
#include <kern/assert.h>

/*
 * Exported interfaces
 */

#include <mach/mach_server.h>
#include <mach/mach_host_server.h>
#if	MACH_DEBUG
#include <mach_debug/mach_debug_server.h>
#endif	/* MACH_DEBUG */

/*
 * Per-Cpu stashed global state
 */
vm_offset_t	active_stacks[NCPUS];	/* per-cpu active stacks	*/
vm_offset_t	kernel_stack[NCPUS];	/* top of active stacks		*/
thread_act_t	active_kloaded[NCPUS];	/*  + act if kernel loaded	*/

struct zone *thread_shuttle_zone;

queue_head_t		reaper_queue;
decl_simple_lock_data(,reaper_lock)

extern int		tick;

extern void		pcb_module_init(void);

/* private */
static struct thread_shuttle	thr_sh_template;

#if	MACH_DEBUG
#if	STACK_USAGE
static void		stack_init(vm_offset_t stack, unsigned int bytes);
void		stack_finalize(vm_offset_t stack);
vm_size_t	stack_usage(vm_offset_t stack);
#else	/*STACK_USAGE*/
#define stack_init(stack, size)
#define stack_finalize(stack)
#define stack_usage(stack) (vm_size_t)0
#endif	/*STACK_USAGE*/

#ifdef	MACHINE_STACK
extern
#endif
    void	stack_statistics(
			unsigned int	*totalp,
			vm_size_t	*maxusagep);

#define	STACK_MARKER	0xdeadbeef
#if	STACK_USAGE
boolean_t		stack_check_usage = TRUE;
#else	/* STACK_USAGE */
boolean_t		stack_check_usage = FALSE;
#endif	/* STACK_USAGE */
decl_simple_lock_data(,stack_usage_lock)
vm_size_t		stack_max_usage = 0;
vm_size_t		stack_max_use = KERNEL_STACK_SIZE - 64;
#endif	/* MACH_DEBUG */

/* Forwards */
kern_return_t	thread_policy_common(
			thread_t	thread,
			int		policy,
			int		data,
			processor_set_t	pset);
void		thread_collect_scan(void);
void		thread_stats(void);
kern_return_t
		thread_create_in(
			thread_act_t thr_act,
			void (*start_pos)(void),
			thread_t *child_thread);

extern void             Load_context(
                                thread_t                thread);


/*
 *	Machine-dependent code must define:
 *		thread_machine_init
 *		thread_machine_terminate
 *		thread_machine_collect
 *
 *	The thread->pcb field is reserved for machine-dependent code.
 */

#ifdef	MACHINE_STACK
/*
 *	Machine-dependent code must define:
 *		stack_alloc_try
 *		stack_alloc
 *		stack_free
 *		stack_collect
 *	and if MACH_DEBUG:
 *		stack_statistics
 */
#else	/* MACHINE_STACK */
/*
 *	We allocate stacks from generic kernel VM.
 *	Machine-dependent code must define:
 *		machine_kernel_stack_init
 *
 *	The stack_free_list can only be accessed at splsched,
 *	because stack_alloc_try/thread_invoke operate at splsched.
 */

decl_simple_lock_data(,stack_lock_data)         /* splsched only */
#define stack_lock()	simple_lock(&stack_lock_data)
#define stack_unlock()	simple_unlock(&stack_lock_data)

vm_offset_t stack_free_list;		/* splsched only */
unsigned int stack_free_count = 0;	/* splsched only */
unsigned int stack_free_limit = 1;	/* patchable */

unsigned int stack_alloc_hits = 0;	/* debugging */
unsigned int stack_alloc_misses = 0;	/* debugging */
unsigned int stack_alloc_max = 0;	/* debugging */

/*
 *	The next field is at the base of the stack,
 *	so the low end is left unsullied.
 */

#define stack_next(stack) (*((vm_offset_t *)((stack) + KERNEL_STACK_SIZE) - 1))

/*
 *	stack_alloc:
 *
 *	Allocate a kernel stack for an activation.
 *	May block.
 */
vm_offset_t
stack_alloc(
	void)
{
	vm_offset_t stack;
	spl_t	s;

	/*
	 *	We first try the free list.  It is probably empty,
	 *	or stack_alloc_try would have succeeded, but possibly
	 *	a stack was freed before the swapin thread got to us.
	 */

	s = splsched();
	stack_lock();
	stack = stack_free_list;
	if (stack != 0) {
		stack_free_list = stack_next(stack);
		stack_free_count--;
	}
	stack_unlock();
	splx(s);

	if (stack == 0) {
		/*
		 *	Kernel stacks should be naturally aligned,
		 *	so that it is easy to find the starting/ending
		 *	addresses of a stack given an address in the middle.
		 */

		if (kmem_alloc_aligned(kernel_map, &stack,
				round_page(KERNEL_STACK_SIZE)) != KERN_SUCCESS)
			panic("stack_alloc");

#if	MACH_DEBUG
		stack_init(stack, round_page(KERNEL_STACK_SIZE));
#endif	/* MACH_DEBUG */

		/*
		 * If using fractional pages, free the remainder(s)
		 */
		if (KERNEL_STACK_SIZE < round_page(KERNEL_STACK_SIZE)) {
		    vm_offset_t ptr  = stack + KERNEL_STACK_SIZE;
		    vm_offset_t endp = stack + round_page(KERNEL_STACK_SIZE);
		    while (ptr < endp) {
#if	MACH_DEBUG
			    /*
			     * We need to initialize just the end of the 
			     * region.
			     */
			    stack_init(ptr, (unsigned int) (endp - ptr));
#endif
			    stack_free(ptr);
			    ptr += KERNEL_STACK_SIZE;
		    }
		}
	}

	return (stack);
}

/*
 *	stack_free:
 *
 *	Free a kernel stack.
 *	Called at splsched.
 */

void
stack_free(
	vm_offset_t stack)
{
	stack_lock();
	stack_next(stack) = stack_free_list;
	stack_free_list = stack;
	if (++stack_free_count > stack_alloc_max)
		stack_alloc_max = stack_free_count;
	stack_unlock();
}

/*
 *	stack_collect:
 *
 *	Free excess kernel stacks.
 *	May block.
 */

void
stack_collect(void)
{
	register vm_offset_t stack;
	spl_t	s;

	/* If using fractional pages, Cannot just call kmem_free(),
	 * and we're too lazy to coalesce small chunks.
	 */
	if (KERNEL_STACK_SIZE < round_page(KERNEL_STACK_SIZE))
		return;

	s = splsched();
	stack_lock();
	while (stack_free_count > stack_free_limit) {
		stack = stack_free_list;
		stack_free_list = stack_next(stack);
		stack_free_count--;
		stack_unlock();
		splx(s);

#if	MACH_DEBUG
		stack_finalize(stack);
#endif	/* MACH_DEBUG */
		kmem_free(kernel_map, stack, KERNEL_STACK_SIZE);

		s = splsched();
		stack_lock();
	}
	stack_unlock();
	splx(s);
}


#if	MACH_DEBUG
/*
 *	stack_statistics:
 *
 *	Return statistics on cached kernel stacks.
 *	*maxusagep must be initialized by the caller.
 */

void
stack_statistics(
	unsigned int	*totalp,
	vm_size_t	*maxusagep)
{
	spl_t	s;

	s = splsched();
	stack_lock();

#if	STACK_USAGE
	if (stack_check_usage) {
		vm_offset_t stack;

		/*
		 *	This is pretty expensive to do at splsched,
		 *	but it only happens when someone makes
		 *	a debugging call, so it should be OK.
		 */

		for (stack = stack_free_list; stack != 0;
		     stack = stack_next(stack)) {
			vm_size_t usage = stack_usage(stack);

			if (usage > *maxusagep)
				*maxusagep = usage;
		}
	}
#endif	/* STACK_USAGE */

	*totalp = stack_free_count;
	stack_unlock();
	splx(s);
}
#endif	/* MACH_DEBUG */

#endif	/* MACHINE_STACK */


/*
 *	stack_privilege:
 *
 *	stack_alloc_try on this thread must always succeed.
 */

void
stack_privilege(
	register thread_t thread)
{
	/*
	 *	This implementation only works for the current thread.
	 */

	if (thread != current_thread())
		panic("stack_privilege");

	if (thread->stack_privilege == 0)
		thread->stack_privilege = current_stack();
}

void
thread_init(
	 void)
{
	thread_shuttle_zone = zinit(
			sizeof(struct thread_shuttle),
			THREAD_MAX * sizeof(struct thread_shuttle),
			THREAD_CHUNK * sizeof(struct thread_shuttle),
			"threads");

	/*
	 *	Fill in a template thread_shuttle for fast initialization.
	 *	[Fields that must be (or are typically) reset at
	 *	time of creation are so noted.]
	 */

	/* thr_sh_template.links (none) */
	thr_sh_template.runq = RUN_QUEUE_NULL;


	/* thr_sh_template.task (later) */
	/* thr_sh_template.thread_list (later) */
	/* thr_sh_template.pset_threads (later) */

	/* one ref for being alive, one to return to the creator */
	thr_sh_template.ref_count = 2;

	thr_sh_template.wait_event = NO_EVENT;
	thr_sh_template.wait_result = KERN_SUCCESS;
	thr_sh_template.wake_active = FALSE;
	/*thr_sh_template.state = TH_SUSP;*/
	thr_sh_template.state = 0;
	thr_sh_template.continuation = (void (*)(void))thread_bootstrap_return;
	thr_sh_template.top_act = THR_ACT_NULL;

/*	thr_sh_template.priority (later) */
	thr_sh_template.max_priority = BASEPRI_USER;
/*	thr_sh_template.sched_pri (later - compute_priority) */
	thr_sh_template.sched_data = 0;
	thr_sh_template.policy = POLICY_TIMESHARE;
	thr_sh_template.depress_priority = -1;
	thr_sh_template.cpu_usage = 0;
	thr_sh_template.sched_usage = 0;
	/* thr_sh_template.sched_stamp (later) */
	thr_sh_template.sched_change_stamp = 1;

	thr_sh_template.vm_privilege = FALSE;

	/* thr_sh_template.<IPC structures> (later) */

	timer_init(&(thr_sh_template.user_timer));
	timer_init(&(thr_sh_template.system_timer));
	thr_sh_template.user_timer_save.low = 0;
	thr_sh_template.user_timer_save.high = 0;
	thr_sh_template.system_timer_save.low = 0;
	thr_sh_template.system_timer_save.high = 0;
	thr_sh_template.cpu_delta = 0;
	thr_sh_template.sched_delta = 0;

	thr_sh_template.active = FALSE; /* reset */

	/* thr_sh_template.processor_set (later) */
#if	NCPUS > 1
	thr_sh_template.bound_processor = PROCESSOR_NULL;
#endif	/*NCPUS > 1*/
#if	MACH_HOST
	thr_sh_template.may_assign = TRUE;
	thr_sh_template.assign_active = FALSE;
#endif	/* MACH_HOST */

#if	NCPUS > 1
	/* thr_sh_template.last_processor  (later) */
#endif	/* NCPUS > 1 */

	/*
	 *	Initialize other data structures used in
	 *	this module.
	 */

	queue_init(&reaper_queue);
	simple_lock_init(&reaper_lock, ETAP_THREAD_REAPER);

#ifndef MACHINE_STACK
	simple_lock_init(&stack_lock_data, ETAP_THREAD_STACK);
#endif  /* MACHINE_STACK */

#if	MACH_DEBUG
	simple_lock_init(&stack_usage_lock, ETAP_THREAD_STACK_USAGE);
#endif	/* MACH_DEBUG */

#if	MACH_LDEBUG
	thr_sh_template.kthread = FALSE;
	thr_sh_template.mutex_count = 0;
#endif	/* MACH_LDEBUG */

	/*
	 *	Initialize any machine-dependent
	 *	per-thread structures necessary.
	 */
	thread_machine_init();
}

void
thread_terminate_self(void)
{
	spl_t		s;
	thread_t	thread;
	thread_act_t	thr_act, prev_act;
	task_t		task;
	etap_data_t	probe_data;

#if	MACH_ASSERT
	if (watchacts & WA_EXIT)
		printf("thread_terminate_self() thr_act=%x(%d) thr=%x(%d)\n",
			    current_act(), current_act()->ref_count,
			    current_thread(), current_thread()->ref_count);
#endif	/* MACH_ASSERT */

	/*	
	 *	Check for rpc chain. If so, switch to the previous 
	 *	activation, set error code, switch stacks and jump
 	 *	to mach_rpc_return_error.
	 */
	thread = current_thread();

	thr_act = thread->top_act;

	ETAP_DATA_LOAD(probe_data[0], thr_act->task);
	ETAP_DATA_LOAD(probe_data[1], thr_act);
	ETAP_DATA_LOAD(probe_data[2], thread->priority);
	ETAP_PROBE_DATA(ETAP_P_THREAD_LIFE,
			EVENT_END,
			thread,
			&probe_data,
			ETAP_DATA_ENTRY*3);

	thread = act_lock_thread(thr_act);
	if (thr_act->lower) {
		act_unlock(thr_act);
		act_switch_swapcheck(thread, (ipc_port_t)0);
		act_lock(thr_act);	/* hierarchy violation XXX */
		(void) switch_act(THR_ACT_NULL);
		assert(thr_act->ref_count == 1);	/* XXX */
		/* act_deallocate(thr_act);		   XXX */
		prev_act = thread->top_act;
		MACH_RPC_RET(prev_act) = KERN_RPC_SERVER_TERMINATED;
		machine_kernel_stack_init(thread, 
			(void (*)(void)) mach_rpc_return_error);
		Load_context(thread);
		/* NOTREACHED */
	}
	act_unlock_thread(thr_act);

	s = splsched();
	thread_lock(thread);
	thr_act = thread->top_act;
	thread->active = FALSE;
	thread_unlock(thread);
	splx(s);

	/* flush any lazy HW state while in own context */
	thread_machine_flush(thr_act);

	/* Reap times from dying threads */
	ipc_thr_act_disable(thr_act);
	task = thr_act->task;

	/*
  	 * the test for task_active seems unnecessary because
   	 * the thread holds a reference to the task (so it
	 * can't be deleted out from under it).
	 */
	if( task && task->active) {
		time_value_t user_time, system_time;

		thread_read_times(thread, &user_time, &system_time);

#if	THREAD_SWAPPER
		thread_swap_disable(thr_act);
#endif	/* THREAD_SWAPPER */

		task_lock(task);

		if( task->active) {
			/*
			 *	Accumulate times for dead threads into task.
		 	 */
			time_value_add(&task->total_user_time, &user_time);
			time_value_add(&task->total_system_time, &system_time);
		}

		/* Make act inactive iff it was born as a base activation */
		if( thr_act->active && (thr_act->pool_port == IP_NULL))
			act_disable_task_locked( thr_act );
		task_unlock( task );
	}

	thread_deallocate(thread); /* take caller's ref; 1 left for reaper */

	ipc_thread_terminate(thread);

	s = splsched();
	simple_lock(&reaper_lock);
	thread_lock(thread);

	enqueue_tail(&reaper_queue, (queue_entry_t) thread);

	thread->state |= TH_HALTED;
	assert((thread->state & TH_UNINT) == 0);
#ifdef MACH_RT
	/*
	 * Since thread has been put in the reaper_queue, it must no longer
	 * be preempted (otherwise, it could be put back in a run queue).
	 */
	thread->preempt = TH_NOT_PREEMPTABLE;
#endif /* MACH_RT */
	thread_unlock(thread);
	simple_unlock(&reaper_lock);
	/* splx(s); */

	assert_wait( NO_EVENT, FALSE);
	thread_wakeup((event_t)&reaper_queue);
	ETAP_SET_REASON(thread, BLOCKED_ON_TERMINATION);
	thread_block((void (*)(void)) 0);
	panic("the zombie walks!");
	/*NOTREACHED*/
}


/*
 * Create a new thread in the specified activation (i.e. "populate" the
 * activation).  The activation can be either user or kernel, but it must
 * be brand-new: no thread, no pool_port, nobody else knows about it.
 * Doesn't start the thread running; use thread_setrun to start it.
 */
kern_return_t
thread_create_in(thread_act_t thr_act, void (*start_pos)(void),
						thread_t *child_thread)
{
	task_t			parent_task = thr_act->task;
	thread_t		new_thread;
	processor_set_t		pset;
	int			rc, sc, max;
	spl_t			s;

	/*
	 *	Allocate a thread and initialize static fields
	 */

	new_thread = (thread_t) zalloc(thread_shuttle_zone);
	if (new_thread == THREAD_NULL)
		return(KERN_RESOURCE_SHORTAGE);

	*new_thread = thr_sh_template;
	thread_lock_init(new_thread);
	rpc_lock_init(new_thread);
	wake_lock_init(new_thread);
	new_thread->sleep_stamp = sched_tick;

	/*
	 * No need to lock thr_act, since it can't be known to anyone --
	 * we set its suspend_count to one more than the task suspend_count
	 * by calling thread_hold.
	 */
	thr_act->user_stop_count = 1;
	for (sc = thr_act->task->suspend_count + 1; sc; --sc)
		thread_hold(thr_act);

#if	MACH_ASSERT
	if (watchacts & WA_THR)
		printf("thread_create_in(thr_act=%x,st=%x,thr@%x=%x)\n",
			thr_act, start_pos, child_thread, new_thread);
#endif	/* MACH_ASSERT */

	simple_lock_init(&new_thread->lock, ETAP_THREAD_NEW);

	/*
	 * Initialize system-dependent part.
	 */
	rc = thread_machine_create(new_thread, thr_act, start_pos);
	if (rc != KERN_SUCCESS) {
		zfree(thread_shuttle_zone, (vm_offset_t) new_thread);
		return rc;
	}

	/* Attach the thread to the activation.  */
	assert(!thr_act->thread);
	assert(!thr_act->pool_port);
	/* Synchronize with act_lock_thread() et al. */
	act_lock(thr_act);
	act_locked_act_reference(thr_act);	/* Thread holds a ref to the thr_act */
	act_attach(thr_act, new_thread, 0);
	act_unlock(thr_act);

	/*
	 *	Initialize runtime-dependent fields
	 */
	new_thread->sched_stamp = sched_tick;
	thread_timeout_setup(new_thread);

	machine_kernel_stack_init(new_thread, (void (*)(void))thread_continue);
	ipc_thread_init(new_thread);
	thread_start(new_thread, start_pos);

	pset = parent_task->processor_set;
	if (!pset->active) {
		pset = &default_pset;
	}
	pset_lock(pset);

	task_lock(parent_task);

	/* Inherit scheduling policy and attributes from parent task */
	new_thread->priority = parent_task->priority;
	new_thread->max_priority = parent_task->max_priority;
	new_thread->sched_data = parent_task->sched_data;
	new_thread->unconsumed_quantum = parent_task->sched_data;
	new_thread->policy = parent_task->policy;

	/*
	 * No check is needed against the pset limit.  If the task could raise
	 * its priority above the limit, then it must have had permisssion.
	 */

	/*
	 *	Don't need to lock thread here because it can't
	 *	possibly execute and no one else knows about it.
	 */
	compute_priority(new_thread, TRUE);

#if	ETAP_EVENT_MONITOR
	new_thread->etap_reason = 0;
	new_thread->etap_trace  = FALSE;
#endif	/* ETAP_EVENT_MONITOR */

	new_thread->active = TRUE;

	pset_add_thread(pset, new_thread);
	/* XXXX if (pset->empty) { suspend new thread? } */

#if	HW_FOOTPRINT
	/*
	 *	Need to set last_processor, idle processor would be best, but
	 *	that requires extra locking nonsense.  Go for tail of
	 *	processors queue to avoid master.
	 */
	if (!(pset->empty)) {
		new_thread->last_processor = 
			(processor_t)queue_first(&pset->processors);
	}
	else {
		/*
		 *	Thread created in empty processor set.  Pick
		 *	master processor as an acceptable legal value.
		 */
		new_thread->last_processor = master_processor;
	}
#else
	/*
	 *	Don't need to initialize because the context switch
	 *	code will set it before it can be used.
	 */
#endif	/* HW_FOOTPRINT */
	
	if (!parent_task->active) {
		task_unlock(parent_task);
		pset_unlock(pset);
		thread_deallocate(new_thread);
		/* Drop ref we'd have given caller */
		thread_deallocate(new_thread);
		return(KERN_FAILURE);
	}

	task_unlock(parent_task);
	pset_unlock(pset);

	*child_thread = new_thread;
	return(KERN_SUCCESS);
}

kern_return_t
thread_create(task_t task, thread_act_t *new_act)
{
	thread_t thread;
	thread_act_t thr_act;
	int rc;
	spl_t s;
	extern void thread_bootstrap_return(void);

#if	MACH_ASSERT
	if (watchacts & WA_THR)
		printf("thread_create(task=%x,&thr_act=%x)\n",task, new_act);
#endif	/* MACH_ASSERT */

	if ((rc = act_create(task, 0, 0, &thr_act)) != KERN_SUCCESS) {
		return(rc);
	}

	if ((rc = thread_create_in(thr_act, thread_bootstrap_return, &thread))
							    != KERN_SUCCESS) {
		thread_terminate(thr_act);
		act_deallocate(thr_act);
		return(rc);
	}

	if (task->kernel_loaded)
		thread_user_to_kernel(thread);

	/* Start the thread running (it will immediately suspend itself).  */
	s = splsched();
	thread_ast_set(thr_act, AST_APC);
	thread_lock(thread);
	thread->state |= TH_RUN;
#if     (THREAD_SWAPPER && ((NCPUS > 1) || MACH_RT))
	if (thread->state & TH_SWAPPED_OUT)
		thread_swapin(thread->top_act, FALSE);
	else
#endif  /* (THREAD_SWAPPER && ((NCPUS > 1) || MACH_RT)) */
		thread_setrun(thread, FALSE, TAIL_Q);
	thread_unlock(thread);
	splx(s);
	
	/*****
	act_lock_thread(thr_act);
	thread_dowait( thr_act, FALSE);
	act_unlock_thread(thr_act);
	*****/

	/* Return the activation reference act_create gave us.  */
	*new_act = thr_act;
	return(KERN_SUCCESS);
}

/*
 *	Create a kernel thread in the specified task,
 *	but don't start it running.
 *	Caller must thread_setrun the thread to start it.
 */
kern_return_t
thread_create_at(task_t parent_task, thread_t *child_thread,
					void (*start_pos)(void))
{
	thread_act_t thr_act;
	int rc;

#if	MACH_ASSERT
	if (watchacts & WA_THR)
		printf("thread_create_at(task=%x,&thread=%x,start=%x)\n",
				parent_task,child_thread,start_pos);
#endif	/* MACH_ASSERT */

	if ((rc = act_create_kernel(parent_task, (vm_offset_t) 0, 
				    (vm_size_t) 0, &thr_act)) != KERN_SUCCESS)
		return rc;
	if ((rc = thread_create_in(thr_act, start_pos, child_thread))
							!= KERN_SUCCESS) {
		thread_terminate(thr_act);
		return rc;
	}
	return rc;
}

/*
 * Update thread that belongs to a task created via kernel_task_create().
 */
void
thread_user_to_kernel(
	thread_t	thread)
{
	/*
	 * Used to set special swap_func here...
	 */
}

kern_return_t
thread_create_running(
        register task_t         parent_task,
        int                     flavor,
        thread_state_t          new_state,
        mach_msg_type_number_t  new_state_count,
        thread_act_t		*child_act)		/* OUT */
{
        register kern_return_t  ret;

        ret = thread_create(parent_task, child_act);
        if (ret != KERN_SUCCESS)
                return(ret);

        ret = act_set_state(*child_act, flavor, new_state,
                               new_state_count);
        if (ret != KERN_SUCCESS) {
                (void) thread_terminate(*child_act);
                return(ret);
        }

        ret = thread_resume(*child_act);
        if (ret != KERN_SUCCESS) {
                (void) thread_terminate(*child_act);
                return(ret);
        }

        return(ret);
}


/*
 *	kernel_thread:
 *
 *	Create and start running a kernel thread in the specified task.
 */
thread_t
kernel_thread(task_t task, void (*start)(void), void *arg)
{
	kern_return_t	kr;
	thread_t	thread;
	spl_t		s;
	thread_act_t	thr_act;

#if	MACH_ASSERT
	if (watchacts & WA_THR)
		printf("kernel_thread(tsk=%x,start=%x)\n",task,start);
#endif	/* MACH_ASSERT */

	kr = thread_create_at(task, &thread, start);
	if (kr != KERN_SUCCESS) {
		printf("kernel_thread: thread_create %x\n",kr);
		panic("thread_create failure");
	}

#if     NCPUS > 1 && defined(PARAGON860)
        /* force ALL kernel threads to the master cpu */
        thread_bind(thread, cpu_to_processor(master_cpu));
#endif

#if	MACH_ASSERT
	if (watchacts & WA_THR)
		printf("\tkernel_thread  thread = %x\n",thread);
#endif	/* MACH_ASSERT */

	thread_swappable(thread->top_act, FALSE);

	s = splsched();
	thread_lock(thread);

	thr_act = thread->top_act;
	thread->ith_other = arg;
#if	MACH_LDEBUG
	thread->kthread = TRUE;
#endif	/* MACH_LDEBUG */

	thread->max_priority = BASEPRI_SYSTEM;
	thread->priority = BASEPRI_SYSTEM;
	thread->sched_pri = BASEPRI_SYSTEM;

	thread->state |= TH_RUN;
	thread_setrun(thread, TRUE, TAIL_Q);
	thread_unlock(thread);
	splx(s);

	act_deallocate(thr_act);
	thread_resume(thr_act);

	return(thread);
}

unsigned int c_weird_pset_ref_exit = 0;	/* pset code raced us */

void
thread_deallocate(
	register thread_t	thread)
{
	spl_t		s;
	register task_t	task;
	register processor_set_t	pset;

	if (thread == THREAD_NULL)
		return;

	/*
	 *	First, check for new count > 0 (the common case).
	 *	Only the thread needs to be locked.
	 */
	s = splsched();
	thread_lock(thread);
	if (--thread->ref_count > 0) {
		thread_unlock(thread);
		splx(s);
		return;
	}

	/*
	 *	Count is zero.  However, the processor set's
	 *	thread list has an implicit reference to
	 *	the thread, and may make new ones.  Its lock also
	 *	dominate the thread lock.  To check for this, we
	 *	temporarily restore the one thread reference, unlock
	 *	the thread, and then lock the pset in the proper order.
	 */
	assert(thread->ref_count == 0); /* Else this is an extra dealloc! */
	thread->ref_count++;
	thread_unlock(thread);
	splx(s);

#if	MACH_HOST
	thread_freeze(thread);
#endif	/* MACH_HOST */

	pset = thread->processor_set;
	pset_lock(pset);

	s = splsched();
	thread_lock(thread);

	if (thread->ref_count == 2 &&
	    thread->timer.set == TELT_SET &&
	    thread->timer.fcn == (timeout_fcn_t) thread_timeout) {
		/*
		 * Only 2 references left: ours and the timeout's.
		 * Cancel the timeout and proceed with the thread termination.
		 */
		reset_timeout_check(&thread->timer);
	}

	if (--thread->ref_count > 0) {
#if	MACH_HOST
		boolean_t need_wakeup = FALSE;
		/*
		 *	processor_set made extra reference.
		 */
		/* Inline the unfreeze */
		thread->may_assign = TRUE;
		if (thread->assign_active) {
			need_wakeup = TRUE;
			thread->assign_active = FALSE;
		}
#endif	/* MACH_HOST */
		thread_unlock(thread);
		splx(s);
		pset_unlock(pset);
#if	MACH_HOST
		if (need_wakeup)
			thread_wakeup((event_t)&thread->assign_active);
#endif	/* MACH_HOST */
		c_weird_pset_ref_exit++;
		return;
	}
#if	MACH_HOST
	assert(thread->assign_active == FALSE);
#endif	/* MACH_HOST */

	/*
	 *	Thread has no references - we can remove it.
	 */

	/*
	 *	A quick sanity check
	 */
	if (thread == current_thread())
	    panic("thread deallocating itself");

	/*
	 *	Remove pending timeouts.
	 */
	reset_timeout_check(&thread->timer);

	reset_timeout_check(&thread->depress_timer);
	thread->depress_priority = -1;

	pset_remove_thread(pset, thread);

	thread_unlock(thread);		/* no more references - safe */
	splx(s);
	pset_unlock(pset);

	pset_deallocate(thread->processor_set);

	/* frees kernel stack & other MD resources */
	thread_machine_destroy(thread);

	zfree(thread_shuttle_zone, (vm_offset_t) thread);
}

void
thread_reference(
	register thread_t	thread)
{
	spl_t	s;

	if (thread == THREAD_NULL)
		return;

	s = splsched();
	thread_lock(thread);
	thread->ref_count++;
	thread_unlock(thread);
	splx(s);
}

/*
 * Called with "appropriate" thread-related locks held on
 * thread and its top_act for synchrony with RPC (see
 * act_lock_thread()).
 */
kern_return_t
Thread_info(
	register thread_t	thread,
	register thread_act_t   thr_act,
	thread_flavor_t		flavor,
	thread_info_t		thread_info_out,	/* ptr to OUT array */
	mach_msg_type_number_t	*thread_info_count)	/*IN/OUT*/
{
	spl_t	s;
	int	state, flags;

	assert(thread && thr_act && thread == thr_act->thread);

	if (thread == THREAD_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (flavor == THREAD_BASIC_INFO) {
	    register thread_basic_info_t	basic_info;

	    if (*thread_info_count < THREAD_BASIC_INFO_COUNT) {
		return(KERN_INVALID_ARGUMENT);
	    }

	    basic_info = (thread_basic_info_t) thread_info_out;

	    s = splsched();
	    thread_lock(thread);

	    /*
	     *	Update lazy-evaluated scheduler info because someone wants it.
	     */
	    if ((thread->state & TH_RUN) == 0 &&
		thread->sched_stamp != sched_tick)
		    update_priority(thread);

	    /* fill in info */

	    thread_read_times(thread,
			&basic_info->user_time,
			&basic_info->system_time);
	    basic_info->policy	= thread->policy;

	    /*
	     *	To calculate cpu_usage, first correct for timer rate,
	     *	then for 5/8 ageing.  The correction factor [3/5] is
	     *	(1/(5/8) - 1).
	     */
	    basic_info->cpu_usage = thread->cpu_usage /
					(TIMER_RATE/TH_USAGE_SCALE);
	    basic_info->cpu_usage = (basic_info->cpu_usage * 3) / 5;
#if	SIMPLE_CLOCK
	    /*
	     *	Clock drift compensation.
	     */
	    basic_info->cpu_usage =
		(basic_info->cpu_usage * 1000000)/sched_usec;
#endif	/* SIMPLE_CLOCK */

	    flags = 0;
	    if (thread->state & TH_SWAPPED_OUT)
		flags = TH_FLAGS_SWAPPED;
	    else if (thread->state & TH_IDLE)
		flags = TH_FLAGS_IDLE;

	    state = 0;		/* ? */
	    if (thread->state & TH_HALTED)
		state = TH_STATE_HALTED;
	    else
	    if (thread->state & TH_RUN)
		state = TH_STATE_RUNNING;
	    else
	    if (thread->state & TH_UNINT)
		state = TH_STATE_UNINTERRUPTIBLE;
	    else
	    if (thread->state & TH_SUSP)
		state = TH_STATE_STOPPED;
	    else
	    if (thread->state & TH_WAIT)
		state = TH_STATE_WAITING;

	    basic_info->run_state = state;
	    basic_info->flags = flags;
	    basic_info->suspend_count = thr_act->user_stop_count;
	    if (state == TH_STATE_RUNNING)
		basic_info->sleep_time = 0;
	    else
		basic_info->sleep_time = sched_tick - thread->sched_stamp;

	    thread_unlock(thread);
	    splx(s);

	    *thread_info_count = THREAD_BASIC_INFO_COUNT;
	    return(KERN_SUCCESS);
	}
	else if (flavor == THREAD_SCHED_TIMESHARE_INFO) {
            register policy_timeshare_info_t        ts_info;

            if (*thread_info_count < POLICY_TIMESHARE_INFO_COUNT) {
                return(KERN_INVALID_ARGUMENT);
            }

            ts_info = (policy_timeshare_info_t) thread_info_out;

	    s = splsched();
            thread_lock(thread);

	    if (thread->policy != POLICY_TIMESHARE) {
	    	thread_unlock(thread);
		splx(s);
		return(KERN_INVALID_POLICY);
	    }

            ts_info->base_priority = thread->priority;
            ts_info->max_priority = thread->max_priority;
            ts_info->cur_priority = thread->sched_pri;

            ts_info->depressed = (thread->depress_priority >= 0);
            ts_info->depress_priority = thread->depress_priority;

            thread_unlock(thread);
	    splx(s);

            *thread_info_count = POLICY_TIMESHARE_INFO_COUNT;
            return(KERN_SUCCESS);	
	}
	else if (flavor == THREAD_SCHED_FIFO_INFO) {
            register policy_fifo_info_t        fifo_info;

            if (*thread_info_count < POLICY_FIFO_INFO_COUNT) {
                return(KERN_INVALID_ARGUMENT);
            }

            fifo_info = (policy_fifo_info_t) thread_info_out;

	    s = splsched();
            thread_lock(thread);

	    if (thread->policy != POLICY_FIFO) {
	    	thread_unlock(thread);
		splx(s);
		return(KERN_INVALID_POLICY);
	    }

            fifo_info->base_priority = thread->priority;
            fifo_info->max_priority = thread->max_priority;

            fifo_info->depressed = (thread->depress_priority >= 0);
            fifo_info->depress_priority = thread->depress_priority;

            thread_unlock(thread);
	    splx(s);

            *thread_info_count = POLICY_FIFO_INFO_COUNT;
            return(KERN_SUCCESS);	
	}
	else if (flavor == THREAD_SCHED_RR_INFO) {
            register policy_rr_info_t        rr_info;

            if (*thread_info_count < POLICY_RR_INFO_COUNT) {
                return(KERN_INVALID_ARGUMENT);
            }

            rr_info = (policy_rr_info_t) thread_info_out;

	    s = splsched();
            thread_lock(thread);

	    if (thread->policy != POLICY_RR) {
	    	thread_unlock(thread);
		splx(s);
		return(KERN_INVALID_POLICY);
	    }

            rr_info->base_priority = thread->priority;
            rr_info->max_priority = thread->max_priority;

	    rr_info->quantum = (thread->sched_data * tick)/1000;

            rr_info->depressed = (thread->depress_priority >= 0);
            rr_info->depress_priority = thread->depress_priority;

            thread_unlock(thread);
	    splx(s);

            *thread_info_count = POLICY_RR_INFO_COUNT;
            return(KERN_SUCCESS);	
	}

	return(KERN_INVALID_ARGUMENT);
}

/*
 *	reaper_thread:
 *
 *	This kernel thread runs forever looking for threads to destroy
 *	(when they request that they be destroyed, of course).
 *
 *	The reaper thread will disappear in the next revision of thread
 *	control when it's function will be moved into thread_dispatch.
 */
void
reaper_thread(void)
{
	for (;;) {
		register thread_t	thread;
		thread_act_t		thr_act;
		struct ipc_port		*pool_port;
		spl_t			s;

		s = splsched();
		simple_lock(&reaper_lock);

		while ((thread = (thread_t) dequeue_head(&reaper_queue))
							!= THREAD_NULL) {
			register int	thread_running;

			simple_unlock(&reaper_lock);
			splx(s);

			/*
			 * wait for run bit to clear
			 */
			(void) thread_wait(thread);

			thr_act = thread_lock_act(thread);
			assert(thr_act && thr_act->thread == thread);

#if	MACH_ASSERT
			if (watchacts & (WA_EXIT|WA_THR))
			    printf("Reaper: thr=0x%x(%d) thr_act=%x(%d)\n",
					thread, thread->ref_count,
					thread->top_act ? thread->top_act : 0,
					thread->top_act ?
					thread->top_act->ref_count : 0
					);
#endif	/* MACH_ASSERT */

			act_locked_act_reference(thr_act);
			pool_port = thr_act->pool_port;

			/*
			 * Replace `act_unlock_thread()' with individual
			 * calls.  (`act_detach()' can change fields used
			 * to determine which locks are held, confusing
			 * `act_unlock_thread()'.)
			 */
			rpc_unlock(thread);
			if (pool_port != IP_NULL) ip_unlock(pool_port);
			act_unlock(thr_act);

			/* Remove the reference held by a rooted thread */
			if (pool_port == IP_NULL) act_deallocate(thr_act);

			/* Remove the reference held by the thread: */
			act_deallocate(thr_act);

			s = splsched();
			simple_lock(&reaper_lock);
		}

		assert_wait((event_t) &reaper_queue, FALSE);
		simple_unlock(&reaper_lock);
		splx(s);
		counter(c_reaper_thread_block++);
		thread_block((void (*)(void)) 0);
	}
}

#if	MACH_HOST
/*
 *	thread_assign:
 *
 *	Change processor set assignment.
 *	Caller must hold an extra reference to the thread (if this is
 *	called directly from the ipc interface, this is an operation
 *	in progress reference).  Caller must hold no locks -- this may block.
 */

kern_return_t
thread_assign(
	thread_act_t	thr_act,
	processor_set_t	new_pset)
{
	thread_t	thread;

	if (thr_act == THR_ACT_NULL || new_pset == PROCESSOR_SET_NULL)
		return(KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	if (thread == THREAD_NULL) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_ARGUMENT);
	}

	thread_freeze(thread);
	thread_doassign(thread, new_pset, TRUE);
	act_unlock_thread(thr_act);
	return(KERN_SUCCESS);
}

/*
 *	thread_freeze:
 *
 *	Freeze thread's assignment.  Prelude to assigning thread.
 *	Only one freeze may be held per thread.  
 */
void
thread_freeze(
	thread_t	thread)
{
	spl_t	s;

	/*
	 *	Freeze the assignment, deferring to a prior freeze.
	 */

	s = splsched();
	thread_lock(thread);
	while (thread->may_assign == FALSE) {
		thread->assign_active = TRUE;
		thread_sleep_simple_lock((event_t) &thread->assign_active,
					simple_lock_addr(thread->lock), TRUE);
		thread_lock(thread);
	}
	thread->may_assign = FALSE;
	thread_unlock(thread);
	splx(s);

}

/*
 *	thread_unfreeze: release freeze on thread's assignment.
 */
void
thread_unfreeze(
	thread_t	thread)
{
	spl_t	s;

	s = splsched();
	thread_lock(thread);
	thread->may_assign = TRUE;
	if (thread->assign_active) {
		thread->assign_active = FALSE;
		thread_unlock(thread);
		splx(s);
		thread_wakeup((event_t)&thread->assign_active);
		return;
	}
	thread_unlock(thread);
	splx(s);
}

/*
 *	thread_doassign:
 *
 *	Actually do thread assignment.  thread_will_assign must have been
 *	called on the thread.  release_freeze argument indicates whether
 *	to release freeze on thread.
 *
 *	Called with "appropriate" thread-related locks held on thread (see
 *	act_lock_thread()).  Returns with thread unlocked.
 */

void
thread_doassign(
	register thread_t		thread,
	register processor_set_t	new_pset,
	boolean_t			release_freeze)
{
	register boolean_t		old_empty, new_empty;
	register processor_set_t	pset;
	boolean_t			recompute_pri = FALSE;
	int				max_priority;
	spl_t				s;
	thread_act_t			thr_act = thread->top_act;
	
	/*
	 *	Check for silly no-op.
	 */
	pset = thread->processor_set;
	if (pset == new_pset) {
		if (release_freeze)
			thread_unfreeze(thread);
		return;
	}
	/*
	 *	Suspend the thread and stop it if it's not the current thread.
	 */
	thread_hold(thr_act);
	act_locked_act_reference(thr_act);
	act_unlock_thread(thr_act);
	if (thread != current_thread()) {
		if (thread_stop_wait(thread) == FALSE ){
			(void)act_lock_thread(thr_act);
			thread_release(thr_act);
			act_locked_act_deallocate(thr_act);
			act_unlock_thread(thr_act);
			if (release_freeze )
				thread_unfreeze(thread);
			return;
		}
	}
	/*
	 *	Had to release thread-related locks before acquiring pset
	 *	locks.
	 */

	/*
	 *	Lock both psets now, use ordering to avoid deadlocks.
	 */
Restart:
	if (pset < new_pset) {
	    pset_lock(pset);
	    pset_lock(new_pset);
	} else {
	    pset_lock(new_pset);
	    pset_lock(pset);
	}

	/*
	 *	Check if new_pset is ok to assign to.  If not, reassign
	 *	to default_pset.
	 */
	if (!new_pset->active) {
	    pset_unlock(pset);
	    pset_unlock(new_pset);
	    new_pset = &default_pset;
	    goto Restart;
	}

	/*
	 *	Grab the thread lock and move the thread.
	 *	Then drop the lock on the old pset and the thread's
	 *	reference to it.
	 */

	s = splsched();
	thread_lock(thread);

	thread_change_psets(thread, pset, new_pset);

	old_empty = pset->empty;
	new_empty = new_pset->empty;

	pset_unlock(pset);
	pset_deallocate(pset);

        /*
	 *	Reset policy and priorities if needed. 
 	 *
	 *	There are three rules for threads under assignment:
	 *
         *      (1) If the new pset has the old policy enabled, keep the
         *          old policy. Otherwise, use the default policy for the pset.
         *      (2) The new limits will be the pset limits for the new policy.
         *      (3) The new base will be the same as the old base unless either
         *              (a) the new policy changed to the pset default policy;
         *                  in this case, the new base is the default policy
         *                  base,
         *          or
         *              (b) the new limits are different from the old limits;
         *                  in this case, the new base is the new limits.
         */
	max_priority = pset_max_priority(new_pset, thread->policy);
	if ((thread->policy & new_pset->policies) == 0) {
		thread->policy = new_pset->policy_default;
		thread->sched_data = pset_sched_data(new_pset, thread->policy);
		thread->unconsumed_quantum = thread->sched_data;
		thread->priority = pset_base_priority(new_pset, thread->policy);
		max_priority = pset_max_priority(new_pset, thread->policy);
		recompute_pri = TRUE;
	} 
	else if (thread->max_priority != max_priority) {
		thread->priority = max_priority;
                recompute_pri = TRUE;
	}

	thread->max_priority = max_priority;
	if ((thread->depress_priority >= 0) &&
            (thread->depress_priority < thread->max_priority)) {
                        thread->depress_priority = thread->max_priority;
	}

	pset_unlock(new_pset);

	if (recompute_pri)
		compute_priority(thread, TRUE);

	if (release_freeze) {
		boolean_t need_wakeup = FALSE;
		thread->may_assign = TRUE;
		if (thread->assign_active) {
			thread->assign_active = FALSE;
			need_wakeup = TRUE;
		}
		thread_unlock(thread);
		splx(s);
		if (need_wakeup)
			thread_wakeup((event_t)&thread->assign_active);
	} else {
		thread_unlock(thread);
		splx(s);
	}
	if (thread != current_thread())
		thread_unstop(thread);
	/*
	 *	Figure out hold status of thread.  Threads assigned to empty
	 *	psets must be held.  Therefore:
	 *		If old pset was empty release its hold.
	 *		Release our hold from above unless new pset is empty.
	 */

	(void)act_lock_thread(thr_act);
	if (old_empty)
		thread_release(thr_act);
	if (!new_empty)
		thread_release(thr_act);
	act_locked_act_deallocate(thr_act);
	act_unlock_thread(thr_act);

	/*
	 *	If current_thread is assigned, context switch to force
	 *	assignment to happen.  This also causes hold to take
	 *	effect if the new pset is empty.
	 */
	if (thread == current_thread()) {
		s = splsched();
		mp_disable_preemption();
		ast_on(cpu_number(), AST_BLOCK);
		mp_enable_preemption();
		splx(s);
	}
}

#else	/* MACH_HOST */

kern_return_t
thread_assign(
	thread_act_t	thr_act,
	processor_set_t	new_pset)
{
#ifdef	lint
	thread++; new_pset++;
#endif	/* lint */
	return(KERN_FAILURE);
}
#endif	/* MACH_HOST */

/*
 *	thread_assign_default:
 *
 *	Special version of thread_assign for assigning threads to default
 *	processor set.
 */
kern_return_t
thread_assign_default(
	thread_act_t	thr_act)
{
	return (thread_assign(thr_act, &default_pset));
}

/*
 *	thread_get_assignment
 *
 *	Return current assignment for this thread.
 */	    
kern_return_t
thread_get_assignment(
	thread_act_t	thr_act,
	processor_set_t	*pset)
{
	thread_t	thread;

	if (thr_act == THR_ACT_NULL)
		return(KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	if (thread == THREAD_NULL) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_ARGUMENT);
	}
	*pset = thread->processor_set;
	act_unlock_thread(thr_act);
	pset_reference(*pset);
	return(KERN_SUCCESS);
}

/*
 *	thread_priority:
 *
 *	Set priority (and possibly max priority) for thread.
 */
kern_return_t
thread_priority(
	thread_act_t	thr_act,
	int		priority,
	boolean_t	set_max)
{
    kern_return_t	kr;
    thread_t		thread;

    if (thr_act == THR_ACT_NULL)
	return (KERN_INVALID_ARGUMENT);
    thread = act_lock_thread(thr_act);
    if ((thread = thr_act->thread) == THREAD_NULL || invalid_pri(priority)) {
	act_unlock_thread(thr_act);
	return (KERN_INVALID_ARGUMENT);
    }
    kr = thread_priority_locked(thread, priority, set_max);
    act_unlock_thread(thr_act);
    return (kr);
}

/*
 *	thread_priority_locked:
 *
 *	Kernel-internal work function for thread_priority().  Called
 *	with thread "properly locked" to ensure synchrony with RPC
 *	(see act_lock_thread()).
 */
kern_return_t
thread_priority_locked(
	thread_t	thread,
	int		priority,
	boolean_t	set_max)
{
    spl_t		s;
    kern_return_t	ret = KERN_SUCCESS;
    etap_data_t         probe_data;

    s = splsched();
    thread_lock(thread);

    /*
     *	Check for violation of max priority
     */
    if (priority < thread->max_priority) {
	ret = KERN_FAILURE;
    }
    else {

	ETAP_DATA_LOAD(probe_data[0], thread);
	ETAP_DATA_LOAD(probe_data[1], priority);
	ETAP_DATA_LOAD(probe_data[2], thread->priority);

	/*
	 *	Set priorities.  If a depression is in progress,
	 *	change the priority to restore.
	 */
	if (thread->depress_priority >= 0) {
	    thread->depress_priority = priority;
	}
	else {
	    thread->priority = priority;
	    compute_priority(thread, TRUE);

	    /* if the current thread has changed its priority let the
	     * ast code decide whether a different thread should run.
	     */
	    if ( thread == current_thread() ) {
		mp_disable_preemption();
		ast_on( cpu_number(), AST_BLOCK );
		mp_enable_preemption();
	    }
	}
	/* Timestamp the change so any saved priorities due to priority
	 * inheritance will be out of date.
	 */
	thread->sched_change_stamp++;

	if (set_max)
	    thread->max_priority = priority;
    }
    thread_unlock(thread);
    splx(s);

    ETAP_PROBE_DATA(ETAP_P_PRIORITY,
		    0,
		    current_thread(),
		    &probe_data,
		    ETAP_DATA_ENTRY*3);

    return(ret);
}

/*
 *	thread_set_own_priority:
 *
 *	Internal use only; sets the priority of the calling thread.
 *	Will adjust max_priority if necessary.
 */
void
thread_set_own_priority(
	int	priority)
{
    spl_t	s;
    thread_t	thread = current_thread();

    s = splsched();
    thread_lock(thread);

    if (priority < thread->max_priority)
	thread->max_priority = priority;
    thread->priority = priority;
    compute_priority(thread, TRUE);

    thread_unlock(thread);
    splx(s);
}

/*
 *	thread_max_priority:
 *
 *	Reset the max priority for a thread.
 */
kern_return_t
thread_max_priority(
	thread_act_t	thr_act,
	processor_set_t	pset,
	int		max_priority)
{
    spl_t               s;
    kern_return_t       ret = KERN_SUCCESS;
    thread_t            thread;

    if (thr_act == THR_ACT_NULL)
	return(KERN_INVALID_ARGUMENT);
    thread = act_lock_thread(thr_act);
    if (thread == THREAD_NULL ||
	(pset == PROCESSOR_SET_NULL) || invalid_pri(max_priority)) {
	act_unlock_thread(thr_act);
	return(KERN_INVALID_ARGUMENT);
    }
    ret = thread_max_priority_locked(thread, pset, max_priority);
    act_unlock_thread(thr_act);
    return (ret);
}

/*
 *	thread_max_priority_locked:
 *
 *	Work function for thread_max_priority.
 *
 *	Called with all RPC-related locks held for thread (see
 *	act_lock_thread()).
 */
kern_return_t
thread_max_priority_locked(
	thread_t	thread,
	processor_set_t	pset,
	int		max_priority)
{
    spl_t		s;
    kern_return_t	ret = KERN_SUCCESS;

    s = splsched();
    thread_lock(thread);

#if	MACH_HOST
    /*
     *	Check for wrong processor set.
     */
    if (pset != thread->processor_set) {
	ret = KERN_FAILURE;
    }
    else {
#endif	/* MACH_HOST */
	thread->max_priority = max_priority;

	/*
	 *	Reset priority if it violates new max priority
	 */
	if (max_priority > thread->priority) {
	    thread->priority = max_priority;

	    compute_priority(thread, TRUE);
	}
	else {
	    if (thread->depress_priority >= 0 &&
		max_priority > thread->depress_priority)
		    thread->depress_priority = max_priority;
	    }
#if	MACH_HOST
    }
#endif	/* MACH_HOST */

    thread_unlock(thread);
    splx(s);

    return(ret);
}

/*
 *	thread_policy_common:
 *
 *	Set scheduling policy for thread. If pset == PROCESSOR_SET_NULL,
 * 	policy will be checked to make sure it is enabled.
 */
kern_return_t
thread_policy_common(
	thread_t	thread,
	int		policy,
	int		data,
	processor_set_t	pset)
{
	register kern_return_t	ret = KERN_SUCCESS;
	register int temp;
	spl_t	s;

	if ((thread == THREAD_NULL) || invalid_policy(policy))
		return(KERN_INVALID_ARGUMENT);

	s = splsched();
	thread_lock(thread);

	/*
	 *	Check if changing policy.
	 */
	if (policy == thread->policy) {
	    /*
	     *	Just changing data.  This is meaningless for
	     *	timeshareing, quantum for fixed priority (but
	     *	has no effect until current quantum runs out).
	     */
	    if (policy == POLICY_FIFO || policy == POLICY_RR) {
		temp = data * 1000;
		if (temp % tick)
		    temp += tick;
		thread->sched_data = temp/tick;
		thread->unconsumed_quantum = temp/tick;
	    }
	}
	else {
	    /*
	     *	Changing policy.  Check if new policy is allowed.
	     */
	    if ( (pset == PROCESSOR_SET_NULL) && 
		((thread->processor_set->policies & policy) == 0) ) {
		ret = KERN_FAILURE;
	    } else {
		if (pset != thread->processor_set) {
		    ret = KERN_FAILURE;
		} else {
		    /*
		     *	Changing policy.  Save data and calculate new
		     *	priority.
		     */
		    thread->policy = policy;
		    if (policy == POLICY_FIFO || policy == POLICY_RR) {
			temp = data * 1000;
			if (temp % tick)
			    temp += tick;
			thread->sched_data = temp/tick;
			thread->unconsumed_quantum = temp/tick;
		    }
		    compute_priority(thread, TRUE);
		}
	    }
	}
	thread_unlock(thread);
	splx(s);

	return(ret);
}


/*
 *	thread_set_policy
 *
 *	Set scheduling policy and parameters, both base and limit, for 
 *	the given thread. Policy can be any policy implemented by the
 *	processor set, whether enabled or not. 
 */
kern_return_t
thread_set_policy(
	thread_act_t			thr_act,
	processor_set_t			pset,
	policy_t			policy,
	policy_base_t			base,
	mach_msg_type_number_t		base_count,
	policy_limit_t			limit,
	mach_msg_type_number_t		limit_count)
{
	int 				max, bas, dat;
	kern_return_t			ret = KERN_SUCCESS;
	thread_t			thread;

	if (thr_act == THR_ACT_NULL || pset == PROCESSOR_SET_NULL)
		return (KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	if (thread ==THREAD_NULL) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_ARGUMENT);
	}
	if (pset != thread->processor_set) {
		act_unlock_thread(thr_act);
		return(KERN_FAILURE);
	}

	switch (policy) {
	case POLICY_RR:
        {
                policy_rr_base_t rr_base = (policy_rr_base_t) base;
                policy_rr_limit_t rr_limit = (policy_rr_limit_t) limit;

                if (base_count != POLICY_RR_BASE_COUNT ||
                    limit_count != POLICY_RR_LIMIT_COUNT) {
                        ret = KERN_INVALID_ARGUMENT;
                        break;
                }
		dat = rr_base->quantum;
		bas = rr_base->base_priority;
		max = rr_limit->max_priority;
		if (invalid_pri(bas) || invalid_pri(max)) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}
		break;
	}

	case POLICY_FIFO:
	{
                policy_fifo_base_t fifo_base = (policy_fifo_base_t) base;
                policy_fifo_limit_t fifo_limit = (policy_fifo_limit_t) limit;

                if (base_count != POLICY_FIFO_BASE_COUNT ||
                    limit_count != POLICY_FIFO_LIMIT_COUNT) {
                        ret = KERN_INVALID_ARGUMENT;
                        break;
                }
		dat = 0;
		bas = fifo_base->base_priority;
		max = fifo_limit->max_priority;
		if (invalid_pri(bas) || invalid_pri(max)) {
			ret = KERN_INVALID_ARGUMENT;
                        break;
                }
                break;
	}

	case POLICY_TIMESHARE:
        {
                policy_timeshare_base_t ts_base =
                                        (policy_timeshare_base_t) base;
                policy_timeshare_limit_t ts_limit =
                                        (policy_timeshare_limit_t) limit;

                if (base_count != POLICY_TIMESHARE_BASE_COUNT ||
                    limit_count != POLICY_TIMESHARE_LIMIT_COUNT) {
                        ret = KERN_INVALID_ARGUMENT;
                        break;
                }
		dat = 0;
		bas = ts_base->base_priority;
		max = ts_limit->max_priority;
		if (invalid_pri(bas) || invalid_pri(max)) {
			ret = KERN_INVALID_ARGUMENT;
                        break;
                }
                break;
	}

	default:
		ret = KERN_INVALID_POLICY;
	}

	if (ret != KERN_SUCCESS) {
		act_unlock_thread(thr_act);
		return(ret);
	}

	/*	
	 *	We've already checked the inputs for thread_max_priority,
	 *	so it's safe to ignore the return value.
	 */
	(void) thread_max_priority_locked(thread, pset, max);	
	ret = thread_priority_locked(thread, bas, FALSE);
	if (ret == KERN_SUCCESS)
		ret = thread_policy_common(thread, policy, dat, pset);
	act_unlock_thread(thr_act);
	return(ret);
}


/*
 * 	thread_policy
 *
 *	Set scheduling policy and parameters, both base and limit, for
 *	the given thread. Policy must be a policy which is enabled for the
 *	processor set. Change contained threads if requested. 
 */
kern_return_t
thread_policy(
	thread_act_t			thr_act,
        policy_t			policy,
        policy_base_t			base,
	mach_msg_type_number_t		count,
        boolean_t			set_limit)
{
	int				lc;
        processor_set_t			pset;
	policy_limit_t			limit;
        policy_rr_limit_data_t		rr_limit;
        policy_fifo_limit_data_t	fifo_limit;
        policy_timeshare_limit_data_t	ts_limit;
        kern_return_t			ret = KERN_SUCCESS;
	thread_t			thread;
	
	if (thr_act == THR_ACT_NULL)
		return (KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	pset = thread->processor_set;
	if (thread == THREAD_NULL || pset == PROCESSOR_SET_NULL) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_ARGUMENT);
	}

	if (invalid_policy(policy) || (pset->policies & policy) == 0) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_POLICY);
	}

	if (set_limit) {
		/*
	 	 * 	Set scheduling limits to base priority.
		 */
		switch (policy) {
		case POLICY_RR:
               {
                        policy_rr_base_t rr_base;

                        if (count != POLICY_RR_BASE_COUNT) {
                                ret = KERN_INVALID_ARGUMENT;
				break;
			}
                        lc = POLICY_RR_LIMIT_COUNT;
                        rr_base = (policy_rr_base_t) base;
                        rr_limit.max_priority = rr_base->base_priority;
                        limit = (policy_limit_t) &rr_limit;
			break;
		}

		case POLICY_FIFO:
                {
                       	policy_fifo_base_t fifo_base;

                       	if (count != POLICY_FIFO_BASE_COUNT) {
                                ret = KERN_INVALID_ARGUMENT;
				break;
			}
                       	lc = POLICY_FIFO_LIMIT_COUNT;
                        fifo_base = (policy_fifo_base_t) base;
                        fifo_limit.max_priority = fifo_base->base_priority;
                        limit = (policy_limit_t) &fifo_limit;
			break;
		}

		case POLICY_TIMESHARE:
                {
                        policy_timeshare_base_t ts_base;

                        if (count != POLICY_TIMESHARE_BASE_COUNT) {
                                ret = KERN_INVALID_ARGUMENT;
				break;
			}
                        lc = POLICY_TIMESHARE_LIMIT_COUNT;
                        ts_base = (policy_timeshare_base_t) base;
                        ts_limit.max_priority = ts_base->base_priority;
                        limit = (policy_limit_t) &ts_limit;
			break;
		}

		default:
			ret = KERN_INVALID_POLICY;
			break;
		}

	} else {
		/*
		 *	Use current scheduling limits. Ensure that the
		 *	new base priority will not exceed current limits.
		 */
                switch (policy) {
                case POLICY_RR:
                {
                        policy_rr_base_t rr_base;

                        if (count != POLICY_RR_BASE_COUNT) {
                                ret = KERN_INVALID_ARGUMENT;
				break;
			}
                        lc = POLICY_RR_LIMIT_COUNT;
                        rr_base = (policy_rr_base_t) base;
			if (rr_base->base_priority < thread->max_priority) {
				ret = KERN_POLICY_LIMIT;
				break;
			}
                        rr_limit.max_priority = thread->max_priority;
                        limit = (policy_limit_t) &rr_limit;
                        break;
		}

                case POLICY_FIFO:
                {
                        policy_fifo_base_t fifo_base;

                        if (count != POLICY_FIFO_BASE_COUNT) {
                                ret = KERN_INVALID_ARGUMENT;
				break;
			}
                        lc = POLICY_FIFO_LIMIT_COUNT;
                        fifo_base = (policy_fifo_base_t) base;
			if (fifo_base->base_priority < thread->max_priority) {
				ret = KERN_POLICY_LIMIT;
				break;
			}
                        fifo_limit.max_priority = thread->max_priority;
                        limit = (policy_limit_t) &fifo_limit;
                        break;
		}

                case POLICY_TIMESHARE:
                {
                        policy_timeshare_base_t ts_base;

                        if (count != POLICY_TIMESHARE_BASE_COUNT) {
                                ret = KERN_INVALID_ARGUMENT;
				break;
			}
                        lc = POLICY_TIMESHARE_LIMIT_COUNT;
                        ts_base = (policy_timeshare_base_t) base;
			if (ts_base->base_priority < thread->max_priority) {
				ret = KERN_POLICY_LIMIT;
				break;
			}
                        ts_limit.max_priority = thread->max_priority;
                        limit = (policy_limit_t) &ts_limit;
                        break;
		}

                default:
			ret = KERN_INVALID_POLICY;
			break;
                }

	}

	act_unlock_thread(thr_act);

	if (ret == KERN_SUCCESS)
	    ret = thread_set_policy(thr_act,pset,policy,base,count,limit,lc);

	return(ret);
}


/*
 *	thread_wire:
 *
 *	Specify that the target thread must always be able
 *	to run and to allocate memory.
 */
kern_return_t
thread_wire(
	host_t		host,
	thread_act_t	thr_act,
	boolean_t	wired)
{
	spl_t		s;
	thread_t	thread;
	extern void vm_page_free_reserve(int pages);

	if (thr_act == THR_ACT_NULL || host == HOST_NULL)
		return (KERN_INVALID_ARGUMENT);
	thread = act_lock_thread(thr_act);
	if (thread ==THREAD_NULL) {
		act_unlock_thread(thr_act);
		return(KERN_INVALID_ARGUMENT);
	}

	/*
	 * This implementation only works for the current thread.
	 * See stack_privilege.
	 */
	if (thr_act != current_act())	/* (thread != current_thread()) */
	    return KERN_INVALID_ARGUMENT;

	s = splsched();
	thread_lock(thread);

	if (wired) {
	    if (thread->vm_privilege == FALSE) 
		    vm_page_free_reserve(1);	/* XXX */
	    thread->vm_privilege = TRUE;
	} else {
	    if (thread->vm_privilege == TRUE) 
		    vm_page_free_reserve(-1);	/* XXX */
	    thread->vm_privilege = FALSE;
	}

	thread_unlock(thread);
	splx(s);
	act_unlock_thread(thr_act);

	/*
	 * Make the thread unswappable.
	 */
	thread_swappable(thr_act, FALSE);

	return KERN_SUCCESS;
}

/*
 *	thread_collect_scan:
 *
 *	Attempt to free resources owned by threads.
 */

void
thread_collect_scan(void)
{
	/* This code runs very quickly! */
}

boolean_t thread_collect_allowed = TRUE;
unsigned thread_collect_last_tick = 0;
unsigned thread_collect_max_rate = 0;		/* in ticks */

/*
 *	consider_thread_collect:
 *
 *	Called by the pageout daemon when the system needs more free pages.
 */

void
consider_thread_collect(void)
{
	/*
	 *	By default, don't attempt thread collection more frequently
	 *	than once a second.
	 */

	if (thread_collect_max_rate == 0)
		thread_collect_max_rate = hz;

	if (thread_collect_allowed &&
	    (sched_tick >
	     (thread_collect_last_tick + thread_collect_max_rate))) {
		thread_collect_last_tick = sched_tick;
		thread_collect_scan();
	}
}

#if	MACH_DEBUG
#if	STACK_USAGE

vm_size_t
stack_usage(
	register vm_offset_t stack)
{
	int i;

	for (i = 0; i < KERNEL_STACK_SIZE/sizeof(unsigned int); i++)
	    if (((unsigned int *)stack)[i] != STACK_MARKER)
		break;

	return KERNEL_STACK_SIZE - i * sizeof(unsigned int);
}

/*
 *	Machine-dependent code should call stack_init
 *	before doing its own initialization of the stack.
 */

static void
stack_init(
	   register vm_offset_t stack,
	   unsigned int bytes)
{
	if (stack_check_usage) {
	    int i;

	    for (i = 0; i < bytes / sizeof(unsigned int); i++)
		((unsigned int *)stack)[i] = STACK_MARKER;
	}
}

/*
 *	Machine-dependent code should call stack_finalize
 *	before releasing the stack memory.
 */

void
stack_finalize(
	register vm_offset_t stack)
{
	if (stack_check_usage) {
	    vm_size_t used = stack_usage(stack);

	    simple_lock(&stack_usage_lock);
	    if (used > stack_max_usage)
		stack_max_usage = used;
	    simple_unlock(&stack_usage_lock);
	    if (used > stack_max_use) {
		printf("stack usage = %x\n", used);
		panic("stack overflow");
	    }
	}
}

#endif	/*STACK_USAGE*/

kern_return_t
host_stack_usage(
	host_t		host,
	vm_size_t	*reservedp,
	unsigned int	*totalp,
	vm_size_t	*spacep,
	vm_size_t	*residentp,
	vm_size_t	*maxusagep,
	vm_offset_t	*maxstackp)
{
	unsigned int total;
	vm_size_t maxusage;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	simple_lock(&stack_usage_lock);
	maxusage = stack_max_usage;
	simple_unlock(&stack_usage_lock);

	stack_statistics(&total, &maxusage);

	*reservedp = 0;
	*totalp = total;
	*spacep = *residentp = total * round_page(KERNEL_STACK_SIZE);
	*maxusagep = maxusage;
	*maxstackp = 0;
	return KERN_SUCCESS;
}

/*
 * Return info on stack usage for threads in a specific processor set
 */
kern_return_t
processor_set_stack_usage(
	processor_set_t	pset,
	unsigned int	*totalp,
	vm_size_t	*spacep,
	vm_size_t	*residentp,
	vm_size_t	*maxusagep,
	vm_offset_t	*maxstackp)
{
	unsigned int total;
	vm_size_t maxusage;
	vm_offset_t maxstack;

	register thread_t *threads;
	register thread_t thread;

	unsigned int actual;	/* this many things */
	unsigned int i;

	vm_size_t size, size_needed;
	vm_offset_t addr;

	if (pset == PROCESSOR_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	size = 0; addr = 0;

	for (;;) {
		pset_lock(pset);
		if (!pset->active) {
			pset_unlock(pset);
			return KERN_INVALID_ARGUMENT;
		}

		actual = pset->thread_count;

		/* do we have the memory we need? */

		size_needed = actual * sizeof(thread_t);
		if (size_needed <= size)
			break;

		/* unlock the pset and allocate more memory */
		pset_unlock(pset);

		if (size != 0)
			kfree(addr, size);

		assert(size_needed > 0);
		size = size_needed;

		addr = kalloc(size);
		if (addr == 0)
			return KERN_RESOURCE_SHORTAGE;
	}

	/* OK, have memory and the processor_set is locked & active */

	threads = (thread_t *) addr;
	for (i = 0, thread = (thread_t) queue_first(&pset->threads);
	     i < actual;
	     i++,
	     thread = (thread_t) queue_next(&thread->pset_threads)) {
		thread_reference(thread);
		threads[i] = thread;
	}
	assert(queue_end(&pset->threads, (queue_entry_t) thread));

	/* can unlock processor set now that we have the thread refs */
	pset_unlock(pset);

	/* calculate maxusage and free thread references */

	total = 0;
	maxusage = 0;
	maxstack = 0;
	for (i = 0; i < actual; i++) {
		int cpu;
		thread_t thread = threads[i];
		vm_offset_t stack = 0;

		/*
		 *	thread->kernel_stack is only accurate if the
		 *	thread isn't swapped and is not executing.
		 *
		 *	Of course, we don't have the appropriate locks
		 *	for these shenanigans.
		 */

		stack = thread->kernel_stack;

		for (cpu = 0; cpu < NCPUS; cpu++)
			if (cpu_data[cpu].active_thread == thread) {
				stack = active_stacks[cpu];
				break;
			}

		if (stack != 0) {
			total++;

			if (stack_check_usage) {
				vm_size_t usage = stack_usage(stack);

				if (usage > maxusage) {
					maxusage = usage;
					maxstack = (vm_offset_t) thread;
				}
			}
		}

		thread_deallocate(thread);
	}

	if (size != 0)
		kfree(addr, size);

	*totalp = total;
	*residentp = *spacep = total * round_page(KERNEL_STACK_SIZE);
	*maxusagep = maxusage;
	*maxstackp = maxstack;
	return KERN_SUCCESS;
}

#endif	/* MACH_DEBUG */
