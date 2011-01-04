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
 * Revision 2.10  91/08/28  11:14:16  jsb
 * 	Renamed AST_CLPORT to AST_NETIPC.
 * 	[91/08/14  21:39:25  jsb]
 * 
 * Revision 2.9  91/06/17  15:46:48  jsb
 * 	Renamed NORMA conditionals.
 * 	[91/06/17  10:48:46  jsb]
 * 
 * Revision 2.8  91/06/06  17:06:43  jsb
 * 	Added AST_CLPORT.
 * 	[91/05/13  17:34:31  jsb]
 * 
 * Revision 2.7  91/05/14  16:39:48  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/05/08  12:47:06  dbg
 * 	Add volatile declarations where needed.
 * 	[91/04/18            dbg]
 * 
 * 	Add missing argument to ast_on in assign/shutdown case.
 * 	[91/03/21            dbg]
 * 
 * Revision 2.5  91/03/16  14:49:23  rpd
 * 	Cleanup.
 * 	[91/02/13            rpd]
 * 	Changed the AST interface.
 * 	[91/01/17            rpd]
 * 
 * Revision 2.4  91/02/05  17:25:33  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:11:01  mrt]
 * 
 * Revision 2.3  90/06/02  14:53:30  rpd
 * 	Updated with new processor/processor-set technology.
 * 	[90/03/26  22:02:00  rpd]
 * 
 * Revision 2.2  90/02/22  20:02:37  dbg
 * 	Remove lint.
 * 	[90/01/29            dbg]
 * 
 * Revision 2.1  89/08/03  15:42:10  rwd
 * Created.
 * 
 *  2-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Moved swtch to this file.
 *
 * 23-Nov-88  David Black (dlb) at Carnegie-Mellon University
 *	Hack up swtch() again.  Drop priority just low enough to run
 *	something else if it's runnable.  Do missing priority updates.
 *	Make sure to lock thread and double check whether update is needed.
 *	Yet more cruft until I can get around to doing it right.
 *
 *  6-Sep-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed all non-MACH code.
 *
 * 11-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	csw_check is now the csw_needed macro in sched.h.  Rewrite
 *	ast_check for new ast mechanism.
 *
 *  9-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	Rewrote swtch to check runq counts directly.
 *
 *  9-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	Delete runrun.  Rewrite csw_check so it can become a macro.
 *
 *  4-May-88  David Black (dlb) at Carnegie-Mellon University
 *	Moved cpu not running check to ast_check().
 *	New preempt priority logic.
 *	Increment runrun if ast is for context switch.
 *	Give absolute priority to local run queues.
 *
 * 20-Apr-88  David Black (dlb) at Carnegie-Mellon University
 *	New signal check logic.
 *
 * 18-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Flushed conditionals, reset history.
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
 *	This file contains routines to check whether an ast is needed.
 *
 *	ast_check() - check whether ast is needed for interrupt or context
 *	switch.  Usually called by clock interrupt handler.
 *
 */

#include <cputypes.h>
#include <cpus.h>
#include <platforms.h>
#include <fast_idle.h>
#include <dipc.h>
#if	PARAGON860
#include <mcmsg.h>
#include <mcmsg_eng.h>
#endif	/* PARAGON860 */
#include <task_swapper.h>

#include <kern/ast.h>
#include <kern/counters.h>
#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/processor.h>
#include <kern/spl.h>
#include <mach/policy.h>
#include <device/net_io.h>
#if	DIPC
#include <dipc/dipc_funcs.h>
#endif	/* DIPC */
#if	TASK_SWAPPER
#include <kern/task_swap.h>
#endif	/* TASK_SWAPPER */

#ifdef hp_pa
volatile ast_t need_ast[NCPUS] = {0};
#else
volatile ast_t need_ast[NCPUS];
#endif

void
ast_init(void)
{
#ifndef	MACHINE_AST
	register int i;

	for (i=0; i<NCPUS; i++) {
		need_ast[i] = AST_NONE;
	}
#endif	/* MACHINE_AST */
}

void
ast_taken(
	boolean_t		preemption,
	ast_t			mask,
	spl_t			old_spl
#if	FAST_IDLE
        ,int			thread_type
#endif	/* FAST_IDLE */
)
{
	register thread_t	self = current_thread();
	register processor_t	mypr;
	register ast_t		reasons;
	register int		mycpu;
	thread_act_t		act = self->top_act;

	/*
	 *	Interrupts are still disabled.
	 *	We must clear need_ast and then enable interrupts.
	 */

extern void	log_thread_action(thread_t, char *);

#if 0
	log_thread_action (current_thread(), "ast_taken");
#endif

	mp_disable_preemption();
	mycpu = cpu_number();
	reasons = need_ast[mycpu] & mask;
	need_ast[mycpu] &= ~reasons;
	mp_enable_preemption();

	splx(old_spl);

	/*
	 *	These actions must not block.
	 */

#if	MCMSG
	if (reasons & AST_MCMSG)
		mcmsg_ast();
#endif	/* MCMSG */

	if (reasons & AST_NETWORK)
		net_ast();

#if	MCMSG_ENG
	if (reasons & AST_RPCREQ)
		rpc_engine_request_intr();

	if (reasons & AST_RPCREPLY)
		rpc_engine_reply_intr();

	if (reasons & AST_RPCDEPART)
		rpc_engine_depart_intr();

	if (reasons & AST_RDMASEND)
		rdma_engine_send_intr();

	if (reasons & AST_RDMARECV)
		rdma_engine_recv_intr();

	if (reasons & AST_RDMATXF)
		rdma_engine_send_fault_intr();

	if (reasons & AST_RDMARXF)
		rdma_engine_recv_fault_intr();
#endif	/* MCMSG_ENG */

#if	PARAGON860 && MCMSG_ENG
	if (reasons & AST_SCAN_INPUT)
		scan_input_ast();
#endif	/* PARAGON860 */

#if	DIPC
	if (reasons & AST_DIPC)
		dipc_ast();
#endif	/* DIPC */

	/*
	 *	Make darn sure that we don't call thread_halt_self
	 *	or thread_block from the idle thread.
	 */

	/* XXX - this isn't currently right for the HALT case... */

	mp_disable_preemption();
	mypr = current_processor();
	if (self == mypr->idle_thread) {
#if	NCPUS == 1
	    if (reasons & AST_URGENT) {
		if (!preemption)
		    panic("ast_taken: AST_URGENT for idle_thr w/o preemption");
	    }
#endif
	    mp_enable_preemption();
	    return;
	}
	mp_enable_preemption();

#if	FAST_IDLE
	if (thread_type != NO_IDLE_THREAD)
		return;
#endif	/* FAST_IDLE */

#if	TASK_SWAPPER
	/* must be before AST_APC */
	if (reasons & AST_SWAPOUT) {
		spl_t s;
		swapout_ast();
		s = splsched();
		mp_disable_preemption();
		mycpu = cpu_number();
		if (need_ast[mycpu] & AST_APC) {
			/* generated in swapout_ast() to get suspended */
			reasons |= AST_APC;		/* process now ... */
			need_ast[mycpu] &= ~AST_APC;	/* ... and not later */
		}
		mp_enable_preemption();
		splx(s);
	}
#endif	/* TASK_SWAPPER */

	/* migration APC hook */
	if (reasons & AST_APC) {
		act_execute_returnhandlers();
		return;	/* auto-retry will catch anything new */
	}

	/* 
	 *	thread_block needs to know if the thread's quantum 
	 *	expired so the thread can be put on the tail of
	 *	run queue. One of the previous actions might well
	 *	have woken a high-priority thread, so we also use
	 *	csw_needed check.
	 */
	{   void (*safept)(void) = (void (*)(void))SAFE_EXCEPTION_RETURN;

	    if (reasons &= AST_PREEMPT) {
		    if (preemption)
			    safept = (void (*)(void)) 0;
	    } else {
		    mp_disable_preemption();
		    mypr = current_processor();
		    if (csw_needed(self, mypr)) {
			    reasons = (mypr->first_quantum
				       ? AST_BLOCK
				       : AST_QUANTUM);
		    }
		    mp_enable_preemption();
	    }
	    if (reasons) {
		    counter(c_ast_taken_block++);
		    thread_block_reason(safept, reasons);
	    }
	}
}

void
ast_check(void)
{
	register int		mycpu;
	register processor_t	myprocessor;
	register thread_t	thread = current_thread();
	spl_t			s = splsched();

	mp_disable_preemption();
	mycpu = cpu_number();

	/*
	 *	Check processor state for ast conditions.
	 */
	myprocessor = cpu_to_processor(mycpu);
	switch(myprocessor->state) {
	    case PROCESSOR_OFF_LINE:
	    case PROCESSOR_IDLE:
	    case PROCESSOR_DISPATCHING:
		/*
		 *	No ast.
		 */
	    	break;

#if	NCPUS > 1
	    case PROCESSOR_ASSIGN:
	    case PROCESSOR_SHUTDOWN:
	        /*
		 * 	Need ast to force action thread onto processor.
		 *
		 * XXX  Should check if action thread is already there.
		 */
		ast_on(mycpu, AST_BLOCK);
		break;
#endif	/* NCPUS > 1 */

	    case PROCESSOR_RUNNING:
	    case PROCESSOR_VIDLE:

		/*
		 *	Propagate thread ast to processor.  If we already
		 *	need an ast, don't look for more reasons.
		 */
		ast_propagate(current_act(), mycpu);
		if (ast_needed(mycpu))
			break;

		/*
		 *	Context switch check.
		 */
		if (csw_needed(thread, myprocessor)) {
			ast_on(mycpu, (myprocessor->first_quantum ?
			       AST_BLOCK : AST_QUANTUM));
		}
		break;

	    default:
	        panic("ast_check: Bad processor state");
	}
	mp_enable_preemption();
	splx(s);
}
