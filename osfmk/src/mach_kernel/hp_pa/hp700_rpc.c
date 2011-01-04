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
#include <cpus.h>
#include <mach_ldebug.h>

#include <mach/kern_return.h>
#include <mach/rpc.h>
#include <mach/mach_syscalls.h>
#include <mach/hp_pa/vm_param.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_space.h>
#include <kern/etap_macros.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <kern/ipc_sched.h>
#include <kern/spl.h>
#include <hp_pa/machine_rpc.h>

#include <string.h>

#if	ETAP_EVENT_MONITOR

#define	ETAP_EXCEPTION_PROBE(_f, _th, _ex, _sysnum)		\
	if (_ex == EXC_SYSCALL) {				\
		ETAP_PROBE_DATA(ETAP_P_SYSCALL_UNIX,		\
				_f,				\
				_th,				\
				_sysnum,			\
				sizeof(int));			\
	}
#else	/* ETAP_EVENT_MONITOR */
#define ETAP_EXCEPTION_PROBE(_f, _th, _ex, _sysnum)
#endif	/* ETAP_EVENT_MONITOR */

kern_return_t
machine_rpc_simple(
	int		routine_num,
	int		argc,
	void		*portp)
{
	panic("machine_rpc_simple");
	return 0;
}

void rpc_upcall(vm_offset_t stack, vm_offset_t new_stack, vm_offset_t server_func, int return_code )
{
	panic("rpc_upcall not implemented\n");
}
	
void mach_rpc_return_wrapper(void)
{
	panic("mach_rpc_return_wrapper not implemented\n");
}

unsigned server_dp = 0;

/*
 * 	exception_return_wrapper
 *
 *	The reply side of a short-circuited rpc to a collocated
 * 	server returns here. We switch back to the kernel stack 
 *	and complete the exception processing. When we enter here,
 *	we are on the collocated server's "user" stack.
 *
 *	XXX To avoid having to size local variable space on the stack,
 *	    we should have the server return to exception_return_wrapper()
 *	    in locore, where we switch back to the kernel_stack. Then just
 *	    call exception_return() here, to complete the exception.
 */

void
exception_return_wrapper(kern_return_t kr)
{

	thread_t		self;
        thread_act_t            act;
	ipc_port_t		port;

	/*
	 *	Now we are running on the kernel stack and the return
	 *	code has been saved away. Initialize any variables we
	 *	need to complete the return path from exception.
	 */

	self = current_thread();

	/*
	 *	Complete the return path from exception.
	 */

	assert(self->top_act != THR_ACT_NULL);
	assert(self->top_act->lower != THR_ACT_NULL);

	port = self->top_act->lower->r_port;
	ip_lock(port);
	ip_release(port);

	rpc_lock(self);
        act_switch_swapcheck(self, port);
	(void) switch_act(THR_ACT_NULL);
       	rpc_unlock(self);
        ip_unlock(port);

	act = current_act();
	
	/*
	 * 	Release port references.
	 */

	ip_lock(act->r_exc_port);
	ip_release(act->r_exc_port);	
	ip_check_unlock(act->r_exc_port);

	/*
	 * Check for terminated bit and eventually alerts.
	 */

	if ( act->alerts & SERVER_TERMINATED ) {
		act->alerts &= ~SERVER_TERMINATED;
           	/* MACH_RPC_RET(act) = KERN_RPC_SERVER_TERMINATED; */
       	}

	/*
	 * Check here for MIG_NO_REPLY return from server, and honor it.
	 * If thread should already halt, it will terminate at return to
	 * user mode; otherwise, block it here indefinitely.
	 */

	if (kr == MIG_NO_REPLY) {
		while (!thread_should_halt(self)) {
			/*
			 * Turn off TH_RUN to synch with thread_wait().
			 */
			thread_will_wait(self);
			thread_block((void (*)(void))0);
		}
	}

	ETAP_EXCEPTION_PROBE(EVENT_END, self, act->r_exception, 
			     act->r_code);

        thread_exception_return();
	/* NOTREACHED */

	return;
}
