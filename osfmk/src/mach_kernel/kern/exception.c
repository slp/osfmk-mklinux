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
 * Revision 2.10.5.3  92/09/15  17:21:27  jeffreyh
 * 	On an iPSC860 drop into ddb instead of calling thread_kdb_return
 * 	until a real fix happens.
 * 	[92/09/15  15:23:53  jeffreyh]
 * 
 * Revision 2.10.5.2  92/03/28  10:10:01  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:18:37  jeffreyh]
 * 
 * Revision 2.11  92/03/03  00:44:45  rpd
 * 	Changed debug_user_with_kdb to FALSE.
 * 	[92/03/02            rpd]
 * 
 * Revision 2.10.5.1  92/02/21  11:23:44  jsb
 * 	Don't convert exception kmsg to network format.
 * 	[92/02/21  09:02:59  jsb]
 * 
 * Revision 2.10  91/12/14  14:18:23  jsb
 * 	Removed ipc_fields.h hack.
 * 
 * Revision 2.9  91/12/13  13:41:38  jsb
 * 	Added NORMA_IPC support.
 * 
 * Revision 2.8  91/08/28  11:14:26  jsb
 * 	Added seqno arguments to ipc_mqueue_receive
 * 	and exception_raise_continue_slow.
 * 	[91/08/10            rpd]
 * 
 * 	Changed msgh_kind to msgh_seqno.
 * 	[91/08/10            rpd]
 * 
 * 	Changed exception_no_server to print an informative message
 * 	before invoking kdb, so this doesn't look like a kernel bug.
 * 	[91/08/09            rpd]
 * 
 * Revision 2.7  91/06/25  10:28:06  rpd
 * 	Fixed ikm_cache critical sections to avoid blocking operations.
 * 	[91/05/23            rpd]
 * 
 * Revision 2.6  91/05/14  16:41:02  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/03/16  14:49:47  rpd
 * 	Fixed assertion typo.
 * 	[91/03/08            rpd]
 * 	Replaced ipc_thread_switch with thread_handoff.
 * 	Replaced ith_saved with ikm_cache.
 * 	[91/02/16            rpd]
 * 
 * Revision 2.4  91/02/05  17:26:05  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:11:53  mrt]
 * 
 * Revision 2.3  91/01/08  15:15:36  rpd
 * 	Added KEEP_STACKS support.
 * 	[91/01/08  14:11:40  rpd]
 * 
 * 	Replaced thread_doexception with new, optimized exception path.
 * 	[90/12/22            rpd]
 * 
 * Revision 2.2  90/06/02  14:53:44  rpd
 * 	Converted to new IPC.
 * 	[90/03/26  22:04:29  rpd]
 * 
 *
 * Condensed history:
 *	Changed thread_doexception to return boolean (dbg).
 *	Removed non-MACH code (dbg).
 *	Added thread_exception_abort (dlb).
 *	Use port_alloc, object_copyout (rpd).
 *	Removed exception-port routines (dbg).
 *	Check for thread halt condition if exception rpc fails (dlb).
 *	Switch to master before calling uprintf and exit (dbg).
 *	If rpc fails in thread_doexception, leave ports alone (dlb).
 *	Translate global port names to local port names (dlb).
 *	Rewrote for exc interface (dlb).
 *	Created (dlb).
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

#include <mach_kdb.h>

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/port.h>
#include <mach/mig_errors.h>
#include <mach/thread_status.h>
#include <mach/exception.h>
#include <mach/mach_server.h>
#include <ipc/port.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_pset.h>
#include <ipc/mach_msg.h>
#include <ipc/ipc_machdep.h>
#include <kern/etap_macros.h>
#include <kern/exception.h>
#include <kern/counters.h>
#include <kern/ipc_sched.h>
#include <kern/ipc_tt.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/host.h>
#include <kern/misc_protos.h>
#include <string.h>
#include <mach/exc_user.h>
#include <machine/machine_rpc.h>

#if	MACH_KDB
#include <ddb/db_trap.h>
#endif	/* MACH_KDB */

/*
 * Forward declarations
 */
void exception_try_task(
	exception_type_t	exception,
	exception_data_t	code,
	mach_msg_type_number_t	codeCnt);

void exception_no_server(void);

kern_return_t alert_exception_try_task(
	exception_type_t	exception,
	exception_data_t	code,
	int			codeCnt);

#if	MACH_KDB

#include <ddb/db_output.h>

#if iPSC386 || iPSC860
boolean_t debug_user_with_kdb = TRUE;
#else
boolean_t debug_user_with_kdb = FALSE;
#endif

#endif	/* MACH_KDB */

unsigned long c_thr_exc_raise = 0;
unsigned long c_thr_exc_raise_state = 0;
unsigned long c_thr_exc_raise_state_id = 0;
unsigned long c_tsk_exc_raise = 0;
unsigned long c_tsk_exc_raise_state = 0;
unsigned long c_tsk_exc_raise_state_id = 0;


#ifdef MACHINE_FAST_EXCEPTION	/* from <machine/thread.h> if at all */
/*
 * This is the fast, MD code, with lots of stuff in-lined.
 */

extern int switch_act_swapins;

#ifdef i386
/*
 * Temporary: controls the syscall copyin optimization.
 * If TRUE, the exception function will copy in the first n
 * words from the stack of the user thread and store it in
 * the saved state, so that the server doesn't have to do
 * this.
 */
boolean_t syscall_exc_copyin = TRUE;
#endif

/*
 *	Routine:	exception
 *	Purpose:
 *		The current thread caught an exception.
 *		We make an up-call to the thread's exception server.
 *	Conditions:
 *		Nothing locked and no resources held.
 *		Called from an exception context, so
 *		thread_exception_return and thread_kdb_return
 *		are possible.
 *	Returns:
 *		Doesn't return.
 */

void
exception(
	exception_type_t	exception,
	exception_data_t	code,
	mach_msg_type_number_t	codeCnt	)
{
	ipc_thread_t		self = current_thread();
	thread_act_t		a_self = self->top_act;
	thread_act_t		cln_act;
	ipc_port_t		exc_port;
	int			i;
	struct exception_action *excp = &a_self->exc_actions[exception];
	int			flavor;
	kern_return_t		kr;

	assert(exception != EXC_RPC_ALERT);

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	/*
	 *	Optimized version of retrieve_thread_exception.
	 */

	act_lock(a_self);
	assert(a_self->ith_self != IP_NULL);
	exc_port = excp->port;
	if (!IP_VALID(exc_port)) {
		act_unlock(a_self);
		exception_try_task(exception, code, codeCnt);
		/*NOTREACHED*/
		return;
	}
	flavor = excp->flavor;

#ifdef i386
	/* For this flavor, we must copy in the first few procedure call
	 * args from the user's stack, since that is part of the important
	 * state in a syscall exception (this is for performance -- we
	 * can do the copyin much faster than the server, even if it is
	 * kernel-loaded):
	 */
	if (flavor == i386_SAVED_STATE) {
		struct i386_saved_state *statep = (struct i386_saved_state *)
					act_machine_state_ptr(self->top_act);
		statep->argv_status = FALSE;
		if (syscall_exc_copyin && copyin((char *)statep->uesp,
			   (char *)statep->argv,
			   i386_SAVED_ARGV_COUNT * sizeof(int)) == 0) {
			/* Indicate success for the server: */
			statep->argv_status = TRUE;
		}
	}
#endif

	ETAP_EXCEPTION_PROBE(EVENT_BEGIN, self, exception, code);

	ip_lock(exc_port);
	act_unlock(a_self);
	if (!ip_active(exc_port)) {
		ip_unlock(exc_port);
		exception_try_task(exception, code, codeCnt);
		/*NOTREACHED*/
		return;
	}

	/*
	 * Hold a reference to the port over the exception_raise_* calls
	 * so it can't be destroyed.  This seems like overkill, but keeps
	 * the port from disappearing between now and when
	 * ipc_object_copyin_from_kernel is finally called.
	 */
	ip_reference(exc_port);	
	exc_port->ip_srights++;
	ip_unlock(exc_port);

	switch (excp->behavior) {
	case EXCEPTION_STATE: {
	    mach_msg_type_number_t state_cnt;
	    rpc_subsystem_t subsystem = ((ipc_port_t)exc_port)->ip_subsystem;

	    if (flavor == MACHINE_THREAD_STATE
			&& subsystem /* && subsystem->start == 2400 */
			&& is_fast_space(exc_port->ip_receiver)) {

		thread_state_t statep = act_machine_state_ptr(self->top_act);

		state_cnt = MACHINE_THREAD_STATE_COUNT;
		/*
		 * In-lined expansion of
		 * kr = exception_raise_state(exc_port, exception,
		 *		code, codeCnt,
		 *		&flavor, statep, state_cnt,
		 *		statep, &state_cnt);
		 * and machine_rpc_simple(port, 2402, 9, (9 E_R_S args))
		 * which comprises the bulk of E_R_S.
		 */
		{
		thread_act_t		old_act, new_act = THR_ACT_NULL;
		ipc_port_t		port = exc_port;
		mig_impl_routine_t	server_func;
		int *			new_sp;

		/*
		 * To save several compares in the course of this routine, we
		 * depend on the fact that the opening members of an ipc_pset
		 * and an ipc_port (ip{,s}_object, ip{,s}_act) are laid out
		 * the same way.  Hence we operate on a portset as if it were
		 * a port.
		 */
		if (port->ip_pset != IPS_NULL)
			port = (ipc_port_t)(port->ip_pset);
		ip_lock(port);
		ip_reference(port);

		/*
		 * If the port's target isn't the kernel, then
		 * obtain a thread into which to migrate the
		 * shuttle, and migrate it.
		 *
		 * In this in-lined routine, we're called only on the
		 * syscall exception path, we're running on a kernel
		 * stack but in the client thread.  So, we switch threads
		 * and switch stacks (to the new thread's stack).
		 */
		for (;;) {
#if 1
			/* InLine expansion of:
			 * new_act = thread_pool_get_act(port);
			 */
			{   thread_pool_t thread_pool = &port->ip_thread_pool;
			    while ((new_act = thread_pool->thr_acts)
				   == THR_ACT_NULL) {
				    if (!ip_active(port)) {
					    break;
				    }
				    thread_pool->waiting = 1;
				    assert_wait((event_t)thread_pool, FALSE);
				    ip_unlock(port);
				    thread_block((void (*)(void)) 0);
				    ip_lock(port);
			    }
			    if (new_act != THR_ACT_NULL) {
				    assert(new_act->thread == THREAD_NULL);
				    assert(new_act->suspend_count >= 0);
				    thread_pool->thr_acts =
					    new_act->thread_pool_next;
				    act_lock(new_act);
				    new_act->thread_pool_next = 0;
			    }
		    } /* end InLine expansion of thread_pool_get_act */
#else
			new_act = thread_pool_get_act(port);
#endif
#if	THREAD_SWAPPER
			if (new_act == THR_ACT_NULL ||
			    new_act->swap_state == TH_SW_IN ||
			    new_act->swap_state == TH_SW_UNSWAPPABLE) {
				/* got one ! */
				break;
			}
			act_locked_act_reference(new_act);
			thread_pool_put_act(new_act);
			switch_act_swapins++;	/* stats */
			(void)thread_swapin_blocking(new_act);
			act_locked_act_deallocate(new_act);
			act_unlock(new_act);
#else	/* THREAD_SWAPPER */
			break;
#endif	/* THREAD_SWAPPER */
		}
                if (new_act == THR_ACT_NULL) {		/* port destroyed */
			ip_release(port);
			ip_unlock(port);
			ipc_port_release_send(exc_port);
			ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
                        thread_exception_return();
			/* NOTREACHED*/
                }

		rpc_lock(self);
		old_act = self->top_act;

		(void) switch_act(new_act);

		new_sp = (int *) new_act->user_stack;

        	/*
         	 * Save rpc state needed for the return path.
         	 */
        	act_lock(a_self);

        	a_self->r_port = port;
        	a_self->r_exc_port = exc_port;
		for (i = 0; i < codeCnt; i++)
		    a_self->r_code[i] = code[i];
        	a_self->r_exc_flavor = flavor;
        	a_self->r_ostate_cnt = state_cnt;
#if     	ETAP_EVENT_MONITOR
        	a_self->r_exception = exception;
#endif
        	act_unlock(a_self);

		ip_unlock(port);
		rpc_unlock(self);
		act_unlock(new_act);
		if (new_act->user_stop_count || new_act->suspend_count) {
			install_special_handler(new_act);
			act_execute_returnhandlers();
		}

		server_func = subsystem->routine[ 2 ].impl_routine;
		assert(server_func);
		assert(new_sp);

		/*
		 * Call the server -- the switch to the server activation's
		 * stack is encapsulated here.  Also, to ensure the atomicity
		 * of swapping operations, call_exc_serv() takes care of
		 * switching back to the client activation after the RPC.
		 */
		kr = call_exc_serv(exc_port->ip_receiver_name, exception,
				   a_self->r_code, codeCnt, new_sp,
				   server_func, &a_self->r_exc_flavor,
				   statep, &a_self->r_ostate_cnt);

		/* NOTREACHED */

		} /* end in-line expansion of E_R_S() and machine_rpc_simple */
	    } else {
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state(exc_port, exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}
	    }

	    if (kr == KERN_SUCCESS ||
		kr == MACH_RCV_PORT_DIED) {
		    ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
                    thread_exception_return();
		    /* NOTREACHED*/
		    return;
	    }
	    } break;

	case EXCEPTION_DEFAULT:
		c_thr_exc_raise++;
		kr = exception_raise(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
                        thread_exception_return();
			/* NOTREACHED*/
			return;
		}
		break;

	case EXCEPTION_STATE_IDENTITY: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_thr_exc_raise_state_id++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state_identity(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
                        ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
                        thread_exception_return();
			/* NOTREACHED*/
			return;
		}
		} break;
	default:
		panic ("bad behavior!");
	}/* switch */

	/*
	 * When a task is being terminated, it's no longer ripped
	 * directly out of the rcv from its "kill me" message, and
	 * so returns here.  The following causes it to return out
	 * to the glue code and clean itself up.
	 */
	if (self->top_act && !self->top_act->active) {
                ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
		thread_exception_return();
	}

	exception_try_task(exception, code, codeCnt);
	/* NOTREACHED */
}

/*
 * We only use the machine-independent exception() routine
 * if a faster MD version isn't available.
 */
#else	/* MACHINE_FAST_EXCEPTION */
/*
 *	If continuations are not used/supported, the NOTREACHED comments
 *	below are incorrect.  The exception function is expected to return.
 *	So the return statements along the slow paths are important.
 */

/*
 *	Routine:	exception
 *	Purpose:
 *		The current thread caught an exception.
 *		We make an up-call to the thread's exception server.
 *	Conditions:
 *		Nothing locked and no resources held.
 *		Called from an exception context, so
 *		thread_exception_return and thread_kdb_return
 *		are possible.
 *	Returns:
 *		Doesn't return.
 */

void
exception(
	exception_type_t	exception,
	exception_data_t	code,
	mach_msg_type_number_t  codeCnt)
{
	ipc_thread_t		self = current_thread();
	thread_act_t		a_self = self->top_act;
	ipc_port_t		exc_port;
	int			i;
	struct exception_action *excp = &a_self->exc_actions[exception];
	int			flavor;
	kern_return_t		kr;

	assert(exception != EXC_RPC_ALERT);

	if (exception == KERN_SUCCESS)
		panic("exception");

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	/*
	 *	Optimized version of retrieve_thread_exception.
	 */

	act_lock(a_self);
	assert(a_self->ith_self != IP_NULL);
	exc_port = excp->port;
	if (!IP_VALID(exc_port)) {
		act_unlock(a_self);
		exception_try_task(exception, code, codeCnt);
		/*NOTREACHED*/
		return;
	}
	flavor = excp->flavor;

	ip_lock(exc_port);
	act_unlock(a_self);
	if (!ip_active(exc_port)) {
		ip_unlock(exc_port);
		exception_try_task(exception, code, codeCnt);
		/*NOTREACHED*/
		return;
	}

	/*
	 * Hold a reference to the port over the exception_raise_* calls
	 * so it can't be destroyed.  This seems like overkill, but keeps
	 * the port from disappearing between now and when
	 * ipc_object_copyin_from_kernel is finally called.
	 */
	ip_reference(exc_port);	
	exc_port->ip_srights++;
	ip_unlock(exc_port);

	switch (excp->behavior) {
	case EXCEPTION_STATE: {
		mach_msg_type_number_t state_cnt;
		rpc_subsystem_t subsystem = ((ipc_port_t)exc_port)->ip_subsystem;

		c_thr_exc_raise_state++;
		if (flavor == MACHINE_THREAD_STATE &&
				subsystem &&
			    is_fast_space(exc_port->ip_receiver)) {
			natural_t *statep;
			/* Requested flavor is the same format in which
			 * we save state on this machine, so no copy is
			 * necessary.  Obtain direct pointer to saved state:
			 */
			statep = (natural_t *)act_machine_state_ptr(self->top_act);
			state_cnt = MACHINE_THREAD_STATE_COUNT;
			kr = exception_raise_state(exc_port, exception,
				    code, codeCnt,
				    &flavor,
				    statep, state_cnt,
				    statep, &state_cnt);
			/* Server is required to return same flavor: */
			assert(flavor == MACHINE_THREAD_STATE);
		} else {
			natural_t state[ THREAD_MACHINE_STATE_MAX ];

			state_cnt = state_count[flavor];
			kr = thread_getstatus(a_self, flavor, 
					      (thread_state_t)state,
					      &state_cnt);
			if (kr == KERN_SUCCESS) {
			    kr = exception_raise_state(exc_port, exception,
					code, codeCnt,
					&flavor,
					state, state_cnt,
					state, &state_cnt);
			    if (kr == MACH_MSG_SUCCESS)
				kr = thread_setstatus(a_self, flavor, 
						      (thread_state_t)state,
						      state_cnt);
			}
		}

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			thread_exception_return();
			/*NOTREACHED*/
			return;
		}
		} break;

	case EXCEPTION_DEFAULT:
		c_thr_exc_raise++;
		kr = exception_raise(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			thread_exception_return();
			/*NOTREACHED*/
			return;
		}
		break;

	case EXCEPTION_STATE_IDENTITY: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_thr_exc_raise_state_id++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor,
				      (thread_state_t)state,
				      &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state_identity(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			kr = thread_setstatus(a_self, flavor,
					      (thread_state_t)state,
					      state_cnt);
		}

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			thread_exception_return();
			/*NOTREACHED*/
			return;
		}
		} break;
	default:
		panic ("bad behavior!");
	}/* switch */

	/*
	 * When a task is being terminated, it's no longer ripped
	 * directly out of the rcv from its "kill me" message, and
	 * so returns here.  The following causes it to return out
	 * to the glue code and clean itself up.
	 */
	if (thread_should_halt(self)) {
		thread_exception_return();
		/*NOTREACHED*/
	}

	exception_try_task(exception, code, codeCnt);
	/* NOTREACHED */
	return;
}
#endif /* defined MACHINE_FAST_EXCEPTION */

/*
 *	Routine:	exception_try_task
 *	Purpose:
 *		The current thread caught an exception.
 *		We make an up-call to the task's exception server.
 *	Conditions:
 *		Nothing locked and no resources held.
 *		Called from an exception context, so
 *		thread_exception_return and thread_kdb_return
 *		are possible.
 *	Returns:
 *		Doesn't return.
 */

void
exception_try_task(
	exception_type_t	exception,
	exception_data_t	code,
	mach_msg_type_number_t  codeCnt)
{
	thread_act_t	a_self = current_act();
	ipc_thread_t 	self = a_self->thread;
	register task_t task = a_self->task;
	register 	ipc_port_t exc_port;
	int 		flavor, i;
	kern_return_t 	kr;

	assert(exception != EXC_RPC_ALERT);

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	/*
	 *	Optimized version of retrieve_task_exception.
	 */

	itk_lock(task);
	assert(task->itk_self != IP_NULL);
	exc_port = task->exc_actions[exception].port;
	if (exception == EXC_MACH_SYSCALL && exc_port == realhost.host_self) {
		itk_unlock(task);
		restart_mach_syscall();		/* magic ! */
		/* NOTREACHED */
	}
	if (!IP_VALID(exc_port)) {
		itk_unlock(task);
		exception_no_server();
		/*NOTREACHED*/
		return;
	}
	flavor = task->exc_actions[exception].flavor;

	ip_lock(exc_port);
	itk_unlock(task);
	if (!ip_active(exc_port)) {
		ip_unlock(exc_port);
		exception_no_server();
		/*NOTREACHED*/
		return;
	}

	/*
	 * Hold a reference to the port over the exception_raise_* calls
	 * (see longer comment in exception())
	 */
	ip_reference(exc_port);
	exc_port->ip_srights++;
	ip_unlock(exc_port);

	switch (task->exc_actions[exception].behavior) {
	case EXCEPTION_STATE: {
	    mach_msg_type_number_t state_cnt;
	    rpc_subsystem_t subsystem = ((ipc_port_t)exc_port)->ip_subsystem;

#ifdef	MACHINE_FAST_EXCEPTION

	    if (flavor == MACHINE_THREAD_STATE
			&& subsystem /* && subsystem->start == 2400 */
			&& is_fast_space(exc_port->ip_receiver)) {

		thread_state_t statep = act_machine_state_ptr(self->top_act);

		state_cnt = MACHINE_THREAD_STATE_COUNT;
		/*
		 * In-lined expansion of
		 * kr = exception_raise_state(exc_port, exception,
		 *		code, codeCnt,
		 *		&flavor, statep, state_cnt,
		 *		statep, &state_cnt);
		 * and machine_rpc_simple(port, 2402, 9, (9 E_R_S args))
		 * which comprises the bulk of E_R_S.
		 */
		{
		thread_act_t		old_act, new_act = THR_ACT_NULL;
		ipc_port_t		port = exc_port;
		mig_impl_routine_t	server_func;
		int *			new_sp;

		/*
		 * To save several compares in the course of this routine, we
		 * depend on the fact that the opening members of an ipc_pset
		 * and an ipc_port (ip{,s}_object, ip{,s}_act) are laid out
		 * the same way.  Hence we operate on a portset as if it were
		 * a port.
		 */
		if (port->ip_pset != IPS_NULL)
			port = (ipc_port_t)(port->ip_pset);
		ip_lock(port);
		ip_reference(port);

		/*
		 * If the port's target isn't the kernel, then
		 * obtain a thread into which to migrate the
		 * shuttle, and migrate it.
		 *
		 * In this in-lined routine, we're called only on the
		 * syscall exception path, we're running on a kernel
		 * stack but in the client thread.  So, we switch threads
		 * and switch stacks (to the new thread's stack).
		 */
		for (;;) {
#if 1
			/* InLine expansion of:
			 * new_act = thread_pool_get_act(port);
			 */
			{   thread_pool_t thread_pool = &port->ip_thread_pool;
			    while ((new_act = thread_pool->thr_acts)
				   == THR_ACT_NULL) {
				    if (!ip_active(port)) {
					    break;
				    }
				    thread_pool->waiting = 1;
				    assert_wait((event_t)thread_pool, FALSE);
				    ip_unlock(port);
				    thread_block((void (*)(void)) 0);
				    ip_lock(port);
			    }
			    if (new_act != THR_ACT_NULL) {
				    assert(new_act->thread == THREAD_NULL);
				    assert(new_act->suspend_count >= 0);
				    thread_pool->thr_acts =
					    new_act->thread_pool_next;
				    act_lock(new_act);
				    new_act->thread_pool_next = 0;
			    }
		    } /* end InLine expansion of thread_pool_get_act */
#else
			new_act = thread_pool_get_act(port);
#endif
#if	THREAD_SWAPPER
			if (new_act == THR_ACT_NULL ||
			    new_act->swap_state == TH_SW_IN ||
			    new_act->swap_state == TH_SW_UNSWAPPABLE) {
				/* got one ! */
				break;
			}
			act_locked_act_reference(new_act);
			thread_pool_put_act(new_act);
			switch_act_swapins++;	/* stats */
			(void)thread_swapin_blocking(new_act);
			act_locked_act_deallocate(new_act);
			act_unlock(new_act);
#else	/* THREAD_SWAPPER */
			break;
#endif	/* THREAD_SWAPPER */
		}
                if (new_act == THR_ACT_NULL) {		/* port destroyed */
			ip_release(port);
			ip_unlock(port);
			ipc_port_release_send(exc_port);
			ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
                        thread_exception_return();
			/* NOTREACHED*/
                }

		rpc_lock(self);
		old_act = self->top_act;

		(void) switch_act(new_act);

		new_sp = (int *) new_act->user_stack;

        	/*
         	 * Save rpc state needed for the return path.
         	 */
        	act_lock(a_self);

        	a_self->r_port = port;
        	a_self->r_exc_port = exc_port;
		for (i = 0; i < codeCnt; i++)
		    a_self->r_code[i] = code[i];
        	a_self->r_exc_flavor = flavor;
        	a_self->r_ostate_cnt = state_cnt;
#if     	ETAP_EVENT_MONITOR
        	a_self->r_exception = exception;
#endif
        	act_unlock(a_self);

		ip_unlock(port);
		rpc_unlock(self);
		act_unlock(new_act);
		if (new_act->user_stop_count || new_act->suspend_count) {
			install_special_handler(new_act);
			act_execute_returnhandlers();
		}

		server_func = subsystem->routine[ 2 ].impl_routine;
		assert(server_func);
		assert(new_sp);

		/*
		 * Call the server -- the switch to the server activation's
		 * stack is encapsulated here.  Also, to ensure the atomicity
		 * of swapping operations, call_exc_serv() takes care of
		 * switching back to the client activation after the RPC.
		 */
		kr = call_exc_serv(exc_port->ip_receiver_name, exception,
				   a_self->r_code, codeCnt, new_sp,
				   server_func, &a_self->r_exc_flavor,
				   statep, &a_self->r_ostate_cnt);


		/* NOTREACHED */

		} /* end in-line expansion of E_R_S() and machine_rpc_simple */
	    } else {
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_tsk_exc_raise_state++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state(exc_port, exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}
	    }

	    if (kr == KERN_SUCCESS ||
		kr == MACH_RCV_PORT_DIED) {
		    ETAP_EXCEPTION_PROBE(EVENT_END, self, exception, code);
                    thread_exception_return();
		    /* NOTREACHED*/
		    return;
	    }

#else	/* MACHINE_FAST_EXCEPTION */

	c_tsk_exc_raise_state++;
	if (flavor == MACHINE_THREAD_STATE &&
	    subsystem &&
	    is_fast_space(exc_port->ip_receiver)) {
		natural_t *statep;
		/* Requested flavor is the same format in which
		 * we save state on this machine, so no copy is
		 * necessary.  Obtain direct pointer to saved state:
		 */
		statep = (natural_t *)act_machine_state_ptr(self->top_act);
		state_cnt = MACHINE_THREAD_STATE_COUNT;
		kr = exception_raise_state(exc_port, exception,
					   code, codeCnt,
					   &flavor,
					   statep, state_cnt,
					   statep, &state_cnt);
		/* Server is required to return same flavor: */
		assert(flavor == MACHINE_THREAD_STATE);
	} else {
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state(exc_port, exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == KERN_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}

		if (kr == MACH_MSG_SUCCESS || kr == MACH_RCV_PORT_DIED) {
		    thread_exception_return();
		    /*NOTREACHED*/
		    return;
		}
	}

#endif	/* MACHINE_FAST_EXCEPTION */

	} break;

	case EXCEPTION_DEFAULT:
		c_tsk_exc_raise++;
		kr = exception_raise(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception, code, codeCnt);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			thread_exception_return();
			/*NOTREACHED*/
			return;
		}
		break;

	case EXCEPTION_STATE_IDENTITY: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_tsk_exc_raise_state_id++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state_identity(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == KERN_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}

		if (kr == MACH_MSG_SUCCESS || kr == MACH_RCV_PORT_DIED) {
		    thread_exception_return();
		    /*NOTREACHED*/
		    return;
		}
		} break;

	default:
		panic ("bad behavior!");
	}/* switch */

	exception_no_server();
	/*NOTREACHED*/
}

/*
 *	Routine:	exception_no_server
 *	Purpose:
 *		The current thread took an exception,
 *		and no exception server took responsibility
 *		for the exception.  So good bye, charlie.
 *	Conditions:
 *		Nothing locked and no resources held.
 *		Called from an exception context, so
 *		thread_kdb_return is possible.
 *	Returns:
 *		Doesn't return.
 */

void
exception_no_server(void)
{
	register ipc_thread_t self = current_thread();

	/*
	 *	If this thread is being terminated, cooperate.
	 *
	 * When a task is dying, it's no longer ripped
	 * directly out of the rcv from its "kill me" message, and
	 * so returns here.  The following causes it to return out
	 * to the glue code and clean itself up.
	 */
	if (thread_should_halt(self)) {
		thread_exception_return();
		panic("exception_no_server - 1");
	}


	if (self->top_act->task == kernel_task)
		panic("kernel task terminating\n");

#if	MACH_KDB
	if (debug_user_with_kdb) {
		/*
		 *	Debug the exception with kdb.
		 *	If kdb handles the exception,
		 *	then thread_kdb_return won't return.
		 */
		db_printf("No exception server, calling kdb...\n");
#if	iPSC860
		db_printf("Dropping into ddb, avoiding thread_kdb_return\n");
		gimmeabreak();
#endif
		thread_kdb_return();
	}
#endif	/* MACH_KDB */

	/*
	 *	All else failed; terminate task.
	 */

	(void) task_terminate(self->top_act->task);
	thread_terminate_self();
	/*NOTREACHED*/
	panic("exception_no_server: returning!");
}

#ifdef MACHINE_FAST_EXCEPTION	/* from <machine/thread.h> if at all */
/*
 *	Routine:	alert_exception
 *	Purpose:
 *		The current thread caught an exception.
 *		We make an up-call to the thread's exception server.
 *	Conditions:
 *		Nothing locked and no resources held.
 *	Returns:
 *		KERN_RPC_TERMINATE_ORPHAN - if orphan should be terminated
 *		KERN_RPC_CONTINUE_ORPHAN - if orphan should be allowed to
 *			continue execution
 */

kern_return_t
alert_exception(
	exception_type_t	exception,
	exception_data_t	code,
	int			codeCnt	)
{
	ipc_thread_t		self = current_thread();
	thread_act_t		a_self = self->top_act;
	thread_act_t		cln_act;
	ipc_port_t		exc_port;
	int			i;
	struct exception_action *excp = &a_self->exc_actions[exception];
	int			flavor;
	kern_return_t		kr;

	assert(exception == EXC_RPC_ALERT);

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	/*
	 *	Optimized version of retrieve_thread_exception.
	 */

	act_lock(a_self);
	assert(a_self->ith_self != IP_NULL);
	exc_port = excp->port;
	if (!IP_VALID(exc_port)) {
		act_unlock(a_self);
		return(alert_exception_try_task(exception, code, codeCnt));
	}
	flavor = excp->flavor;

#ifdef i386
	/* For this flavor, we must copy in the first few procedure call
	 * args from the user's stack, since that is part of the important
	 * state in a syscall exception (this is for performance -- we
	 * can do the copyin much faster than the server, even if it is
	 * kernel-loaded):
	 */
	if (flavor == i386_SAVED_STATE) {
		struct i386_saved_state *statep = (struct i386_saved_state *)
					act_machine_state_ptr(self->top_act);
		statep->argv_status = FALSE;
		if (syscall_exc_copyin && copyin((char *)statep->uesp,
			   (char *)statep->argv,
			   i386_SAVED_ARGV_COUNT * sizeof(int)) == 0) {
			/* Indicate success for the server: */
			statep->argv_status = TRUE;
		}
	}
#endif

	ip_lock(exc_port);
	act_unlock(a_self);
	if (!ip_active(exc_port)) {
		ip_unlock(exc_port);
		return(alert_exception_try_task(exception, code, codeCnt));
	}

	/*
	 * Hold a reference to the port over the exception_raise_* calls
	 * so it can't be destroyed.  This seems like overkill, but keeps
	 * the port from disappearing between now and when
	 * ipc_object_copyin_from_kernel is finally called.
	 */
	ip_reference(exc_port);	
	/* exc_port->ip_srights++; ipc_object_copy_from_kernel does this */
	ip_unlock(exc_port);

	switch (excp->behavior) {
	case EXCEPTION_STATE: {
	    mach_msg_type_number_t state_cnt;
	    rpc_subsystem_t subsystem = ((ipc_port_t)exc_port)->ip_subsystem;

	    if (flavor == MACHINE_THREAD_STATE
			&& subsystem /* && subsystem->start == 2400 */
			&& is_fast_space(exc_port->ip_receiver)) {

		thread_state_t statep = act_machine_state_ptr(self->top_act);

		state_cnt = MACHINE_THREAD_STATE_COUNT;
		/*
		 * In-lined expansion of
		 * kr = exception_raise_state(exc_port, exception,
		 *		code, codeCnt,
		 *		&flavor, statep, state_cnt,
		 *		statep, &state_cnt);
		 * and machine_rpc_simple(port, 2402, 9, (9 E_R_S args))
		 * which comprises the bulk of E_R_S.
		 */
		{
		thread_act_t		old_act, new_act = THR_ACT_NULL;
		ipc_port_t		port = exc_port;
		mig_impl_routine_t	server_func;
		int *			new_sp;


		/*
		 * To save several compares in the course of this routine, we
		 * depend on the fact that the opening members of an ipc_pset
		 * and an ipc_port (ip{,s}_object, ip{,s}_act) are laid out
		 * the same way.  Hence we operate on a portset as if it were
		 * a port.
		 */
		if (port->ip_pset != IPS_NULL)
			port = (ipc_port_t)(port->ip_pset);
		ip_lock(port);
		ip_reference(port);

		/*
		 * If the port's target isn't the kernel, then
		 * obtain a thread into which to migrate the
		 * shuttle, and migrate it.
		 *
		 * In this in-lined routine, we're called only on the
		 * syscall exception path, we're running on a kernel
		 * stack but in the client thread.  So, we switch threads
		 * and switch stacks (to the new thread's stack).
		 */
		for (;;) {
#if 1
			/* InLine expansion of:
			 * new_act = thread_pool_get_act(port);
			 */
			{   thread_pool_t thread_pool = &port->ip_thread_pool;
			    while ((new_act = thread_pool->thr_acts)
				   == THR_ACT_NULL) {
				    if (!ip_active(port)) {
					    break;
				    }
				    thread_pool->waiting = 1;
				    assert_wait((event_t)thread_pool, FALSE);
				    ip_unlock(port);
				    thread_block((void (*)(void)) 0);
				    ip_lock(port);
			    }
			    if (new_act != THR_ACT_NULL) {
				    assert(new_act->thread == THREAD_NULL);
				    assert(new_act->suspend_count >= 0);
				    thread_pool->thr_acts =
					new_act->thread_pool_next;
				    act_lock(new_act);
				    new_act->thread_pool_next = 0;
			    }
		    } /* end InLine expansion of thread_pool_get_act */
#else
			new_act = thread_pool_get_act(port);
#endif
#if	THREAD_SWAPPER
			if (new_act->swap_state == TH_SW_IN ||
			    new_act->swap_state == TH_SW_UNSWAPPABLE) {
				/* got one ! */
				break;
			}
			act_locked_act_reference(new_act);
			thread_pool_put_act(new_act);
			switch_act_swapins++;	/* stats */
			(void)thread_swapin_blocking(new_act);
			act_locked_act_deallocate(new_act);
			act_unlock(new_act);
#else	/* THREAD_SWAPPER */
			break;
#endif	/* THREAD_SWAPPER */
		}

                if (new_act == THR_ACT_NULL) {		/* port destroyed */
                        ip_release(port);               
                        ip_release(port);               
                        ip_unlock(port);
			return(KERN_RPC_CONTINUE_ORPHAN);
                }

		rpc_lock(self);
		old_act = self->top_act;

		(void) switch_act(new_act);

		new_sp = (int *) new_act->user_stack;

                /*
                 * Save rpc state needed for the return path.
                 */
                act_lock(a_self);

                a_self->r_port = port;
                a_self->r_exc_port = exc_port;
		for (i = 0; i < codeCnt; i++)
		    a_self->r_code[i] = code[i];
        	a_self->r_exc_flavor = flavor;
        	a_self->r_ostate_cnt = state_cnt;
#if             ETAP_EVENT_MONITOR
                a_self->r_exception = exception;
#endif
                act_unlock(a_self);

		ip_unlock(port);
		rpc_unlock(self);
		act_unlock(new_act);

		if (new_act->user_stop_count || new_act->suspend_count) {
			install_special_handler(new_act);
			act_execute_returnhandlers();
		}

		server_func = subsystem->routine[ 2 ].impl_routine;
		assert(server_func != 0);

		/* 
		 * XXX This side call to the server function should end by
	 	 *     doing a "return(KERN_RPC_TERMINATE_ORPHAN)", instead
		 *     of the "thread_exception_return()" which the wrapper
		 *     exception_return_wrapper() will call. So a separate
		 *     version of exception_return_wrapper() is needed for
		 *     the alert case.
		 */
		kr = call_exc_serv(exc_port->ip_receiver_name, exception,
				   a_self->r_code, codeCnt, new_sp,
				   server_func, &a_self->r_exc_flavor,
				   statep, &a_self->r_ostate_cnt);

		/* NOTREACHED */

		} /* end in-line expansion of E_R_S() and machine_rpc_simple */
	    } else {
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state(exc_port, exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}
	    }
	    ip_lock(exc_port);
	    ip_release(exc_port);	
	    ip_unlock(exc_port);

	    if (kr == KERN_SUCCESS ||
		kr == MACH_RCV_PORT_DIED) {
		    return(KERN_RPC_TERMINATE_ORPHAN);
	    }
	    } break;

	case EXCEPTION_DEFAULT:
		c_thr_exc_raise++;
		kr = exception_raise(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt);
		ip_lock(exc_port);
		ip_release(exc_port);	
		ip_unlock(exc_port);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			return(KERN_RPC_TERMINATE_ORPHAN);
		}
		break;

	case EXCEPTION_STATE_IDENTITY: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_thr_exc_raise_state_id++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state_identity(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}
		ip_lock(exc_port);
		ip_release(exc_port);	
		ip_unlock(exc_port);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			return(KERN_RPC_TERMINATE_ORPHAN);
		}
		} break;
	default:
		panic ("bad behavior!");
	}/* switch */

	/*
	 * When a task is being terminated, it's no longer ripped
	 * directly out of the rcv from its "kill me" message, and
	 * so returns here.  The following causes it to return out
	 * to the glue code and clean itself up.
	 */
	if (self->top_act && !self->top_act->active)
		return(KERN_RPC_TERMINATE_ORPHAN);

	return(alert_exception_try_task(exception, code, codeCnt));
}
#else	/* MACHINE_FAST_EXCEPTION */

/*
 *	Routine:	alert_exception
 *	Purpose:
 *		The current thread caught an exception.
 *		We make an up-call to the thread's exception server.
 *	Conditions:
 *		Nothing locked and no resources held.
 *	Returns:
 *		KERN_RPC_TERMINATE_ORPHAN - if orphan should be terminated
 *		KERN_RPC_CONTINUE_ORPHAN - if orphan should be allowed to
 *			continue execution
 */

kern_return_t
alert_exception(
	exception_type_t	exception,
	exception_data_t	code,
	int			codeCnt	)
{
	ipc_thread_t		self = current_thread();
	thread_act_t		a_self = self->top_act;
	ipc_port_t		exc_port;
	int			i;
	struct exception_action *excp = &a_self->exc_actions[exception];
	int			flavor;
	kern_return_t		kr;

	assert(exception == EXC_RPC_ALERT);

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	/*
	 *	Optimized version of retrieve_thread_exception.
	 */

	act_lock(a_self);
	assert(a_self->ith_self != IP_NULL);
	exc_port = excp->port;
	if (!IP_VALID(exc_port)) {
		act_unlock(a_self);
		return(alert_exception_try_task(exception, code, codeCnt));
	}
	flavor = excp->flavor;

	ip_lock(exc_port);
	act_unlock(a_self);
	if (!ip_active(exc_port)) {
		ip_unlock(exc_port);
		return(alert_exception_try_task(exception, code, codeCnt));
	}

	/*
	 * Hold a reference to the port over the exception_raise_* calls
	 * so it can't be destroyed.  This seems like overkill, but keeps
	 * the port from disappearing between now and when
	 * ipc_object_copyin_from_kernel is finally called.
	 */
	ip_reference(exc_port);	
	/* exc_port->ip_srights++; ipc_object_copy_from_kernel does this */
	ip_unlock(exc_port);

	switch (excp->behavior) {
	case EXCEPTION_STATE: {
		mach_msg_type_number_t state_cnt;
		rpc_subsystem_t subsystem = ((ipc_port_t)exc_port)->ip_subsystem;

		c_thr_exc_raise_state++;
		if (flavor == MACHINE_THREAD_STATE &&
				subsystem &&
			    is_fast_space(exc_port->ip_receiver)) {
			natural_t *statep;
			/* Requested flavor is the same format in which
			 * we save state on this machine, so no copy is
			 * necessary.  Obtain direct pointer to saved state:
			 */
			statep = act_machine_state_ptr(self->top_act);
			state_cnt = MACHINE_THREAD_STATE_COUNT;
			kr = exception_raise_state(exc_port, exception,
				    code, codeCnt,
				    &flavor,
				    statep, state_cnt,
				    statep, &state_cnt);
			/* Server is required to return same flavor: */
			assert(flavor == MACHINE_THREAD_STATE);
		} else {
			natural_t state[ THREAD_MACHINE_STATE_MAX ];

			state_cnt = state_count[flavor];
			kr = thread_getstatus(a_self, flavor,
					      (thread_state_t)state,
					      &state_cnt);
			if (kr == KERN_SUCCESS) {
			    kr = exception_raise_state(exc_port, exception,
					code, codeCnt,
					&flavor,
					state, state_cnt,
					state, &state_cnt);
			    if (kr == MACH_MSG_SUCCESS)
				kr = thread_setstatus(a_self, flavor,
						      (thread_state_t)state,
						      state_cnt);
			}
		}
		ip_lock(exc_port);
		ip_release(exc_port);	
		ip_unlock(exc_port);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			return(KERN_RPC_TERMINATE_ORPHAN);
		}
		} break;

	case EXCEPTION_DEFAULT:
		c_thr_exc_raise++;
		kr = exception_raise(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt);
		ip_lock(exc_port);
		ip_release(exc_port);	
		ip_unlock(exc_port);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			return(KERN_RPC_TERMINATE_ORPHAN);
		}
		break;

	case EXCEPTION_STATE_IDENTITY: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_thr_exc_raise_state_id++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor,
				      (thread_state_t)state,
				      &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state_identity(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == MACH_MSG_SUCCESS)
			    kr = thread_setstatus(a_self, flavor,
						  (thread_state_t)state,
						  state_cnt);
		}
		ip_lock(exc_port);
		ip_release(exc_port);	
		ip_unlock(exc_port);

		if (kr == KERN_SUCCESS ||
		    kr == MACH_RCV_PORT_DIED) {
			return(KERN_RPC_TERMINATE_ORPHAN);
		}
		} break;
	default:
		panic ("bad behavior!");
	}/* switch */

	/*
	 * When a task is being terminated, it's no longer ripped
	 * directly out of the rcv from its "kill me" message, and
	 * so returns here.  The following causes it to return out
	 * to the glue code and clean itself up.
	 */
	if (thread_should_halt(self)) {
		return(KERN_RPC_TERMINATE_ORPHAN);
	}

	return(alert_exception_try_task(exception, code, codeCnt));
}
#endif /* defined MACHINE_FAST_EXCEPTION */

/*
 *	Routine:	alert_exception_try_task
 *	Purpose:
 *		The current thread caught an exception.
 *		We make an up-call to the task's exception server.
 *	Conditions:
 *		Nothing locked and no resources held.
 *	Returns:
 *		KERN_RPC_TERMINATE_ORPHAN - if orphan should be terminated
 *		KERN_RPC_CONTINUE_ORPHAN - if orphan should be allowed to
 *			continue execution
 */

kern_return_t
alert_exception_try_task(
	exception_type_t	exception,
	exception_data_t	code,
	int			codeCnt	)
{
	thread_act_t	a_self = current_act();
	ipc_thread_t self = a_self->thread;
	register task_t task = a_self->task;
	register ipc_port_t exc_port;
	int flavor;
	kern_return_t kr;

	assert(exception == EXC_RPC_ALERT);

	self->ith_scatter_list = MACH_MSG_BODY_NULL;

	/*
	 *	Optimized version of retrieve_task_exception.
	 */

	itk_lock(task);
	assert(task->itk_self != IP_NULL);
	exc_port = task->exc_actions[exception].port;
	if (!IP_VALID(exc_port)) {
		itk_unlock(task);
		return(KERN_RPC_CONTINUE_ORPHAN);
	}
	flavor = task->exc_actions[exception].flavor;

	ip_lock(exc_port);
	itk_unlock(task);
	if (!ip_active(exc_port)) {
		ip_unlock(exc_port);
		return(KERN_RPC_CONTINUE_ORPHAN);
	}

	/*
	 * Hold a reference to the port over the exception_raise_* calls
	 * (see longer comment in exception())
	 */
	ip_reference(exc_port);
	/* exc_port->ip_srights++; */
	ip_unlock(exc_port);

	switch (task->exc_actions[exception].behavior) {
	case EXCEPTION_STATE: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_tsk_exc_raise_state++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state(exc_port, exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == KERN_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}
		ip_lock(exc_port);
		ip_release(exc_port);
		ip_unlock(exc_port);

		if (kr == MACH_MSG_SUCCESS || kr == MACH_RCV_PORT_DIED) {
		    return(KERN_RPC_TERMINATE_ORPHAN);
		}
		} break;

	case EXCEPTION_DEFAULT:
		c_tsk_exc_raise++;
		kr = exception_raise(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception, code, codeCnt);
		ip_lock(exc_port);
		ip_release(exc_port);
		ip_unlock(exc_port);

		if (kr == KERN_SUCCESS || kr == MACH_RCV_PORT_DIED) {
		    return(KERN_RPC_TERMINATE_ORPHAN);
		}
		break;

	case EXCEPTION_STATE_IDENTITY: {
		mach_msg_type_number_t state_cnt;
		natural_t state[ THREAD_MACHINE_STATE_MAX ];

		c_tsk_exc_raise_state_id++;
		state_cnt = state_count[flavor];
		kr = thread_getstatus(a_self, flavor, state, &state_cnt);
		if (kr == KERN_SUCCESS) {
		    kr = exception_raise_state_identity(exc_port,
				retrieve_act_self_fast(a_self),
				retrieve_task_self_fast(a_self->task),
				exception,
				code, codeCnt,
				&flavor,
				state, state_cnt,
				state, &state_cnt);
		    if (kr == KERN_SUCCESS)
			kr = thread_setstatus(a_self, flavor, state, state_cnt);
		}
		ip_lock(exc_port);
		ip_release(exc_port);
		ip_unlock(exc_port);

		if (kr == MACH_MSG_SUCCESS || kr == MACH_RCV_PORT_DIED) {
		    return(KERN_RPC_TERMINATE_ORPHAN);
		}
		} break;

	default:
		panic ("bad behavior!");
	}/* switch */

	return(KERN_RPC_CONTINUE_ORPHAN);
}
