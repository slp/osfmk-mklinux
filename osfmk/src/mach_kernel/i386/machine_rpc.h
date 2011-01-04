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

#ifndef _MACHINE_RPC_H_
#define _MACHINE_RPC_H_

#if     ETAP_EVENT_MONITOR
#define ETAP_EXCEPTION_PROBE(_f, _th, _ex, _sysnum)             \
        if (_ex == EXC_SYSCALL) {                               \
                ETAP_PROBE_DATA(ETAP_P_SYSCALL_UNIX,            \
                                _f,                             \
                                _th,                            \
                                _sysnum,                        \
                                sizeof(int));                   \
        }
#else   /* ETAP_EVENT_MONITOR */
#define ETAP_EXCEPTION_PROBE(_f, _th, _ex, _sysnum)
#endif  /* ETAP_EVENT_MONITOR */

extern void exception_return_wrapper( void );

#ifdef MACHINE_FAST_EXCEPTION
/*
 * Switch stacks, call the function, and switch back.  Since
 * we're switching stacks, the asm fragment can't use auto's
 * for temporary storage; use a callee-saves register instead.
 *
 * Don't save anything in the user stack, since caller ensures that
 * all data queried from server work function resides in stable,
 * act-private storage, not on (shared) kernel stack.
 */
static kern_return_t __inline__
call_exc_serv(
	      mach_port_t exc_port, exception_type_t exception,
	      exception_data_t code, mach_msg_type_number_t codeCnt,
	      int *new_sp, mig_impl_routine_t func, int *flavor,
	      thread_state_t statep, mach_msg_type_number_t *state_cnt)
{	
	/*
	 * Now transfer the caller's arguments to the new stack.
	 */
	new_sp -= 9; /* argc */
	new_sp[0] = (int) exc_port;
	new_sp[1] = (int) exception;
	new_sp[2] = (int) code;
	new_sp[3] = (int) codeCnt;
	new_sp[4] = (int) flavor;
	new_sp[5] = (int) statep;
	new_sp[7] = (int) statep;
	new_sp[6] = (int) *state_cnt;
	new_sp[8] = (int) state_cnt;

	/*
       	 * Execute a side-call to the collocated server. The
         * call is implemented using a jmp instruction because
         * we setup the return address manually to arrange a
         * return to exception_return_wrapper();
         */
        new_sp -= 1;
        new_sp[0] = (int)exception_return_wrapper;

        __asm__ volatile(                               \
        	"movl  %0, %%ebx;                       \
                 xchgl %%ebx, %%esp;                    \
                 movl  $0, %%ebp;                       \
                 jmp   %1"
                :
                : "g" (new_sp), "S" (func)
                : "%ebx", "%eax", "%edx", "cc", "memory");

	/* NOTREACHED */
	return KERN_FAILURE;
}


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
exception_return_wrapper( void )
{

	thread_t		self;
        thread_act_t            act;
	ipc_port_t		port;
	kern_return_t		kr;


	/*
	 *	The return code is in register %eax at this point.
	 *
	 *	XXX We copy result code into ecx but wait until we
	 *	    switch stacks before storing into the local variable.
	 *	    What should happen: switch stacks in locore stub and
	 *	    call this routine with the return code as an argument.
	 *	
	 */

	__asm__ volatile(				\
		"movl %%eax, %%esi"			
		: /* no outputs */
		: /* no inputs */
		: "cc", "memory");

	/*
	 *	Next, return to the per-shuttle kernel stack.
	 */

	mp_disable_preemption();
	__asm__ volatile(				\
        	"movl  %1, %%ebx;                       \
               	 xchgl %%ebx, %%esp;			\
		 movl  %%esp, %%ebp;			\
		 subl  $0x32, %%esp;			\
        	 movl  %%esi, %0"
		: "=g" (kr)
                : "g" (kernel_stack[cpu_number()])
                : "%ebx", "%eax", "cc", "memory");
	mp_enable_preemption();


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


#endif /* MACHINE_FAST_EXCEPTION */
#endif /* _MACHINE_RPC_H_ */
