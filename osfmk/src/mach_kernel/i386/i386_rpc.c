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
#include <mach_prof.h>
#include <mach_ldebug.h>
#include <mach_rt.h>

#include <mach/kern_return.h>
#include <mach/rpc.h>
#include <mach/mach_syscalls.h>
#include <mach/i386/vm_param.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_space.h>
#include <kern/etap_macros.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <kern/ipc_sched.h>
#include <i386/thread.h>
#include <i386/eflags.h>
#include <i386/proc_reg.h>
#include <i386/seg.h>
#include <i386/tss.h>
#include <i386/user_ldt.h>
#include <i386/fpu.h>
#include <i386/asm.h>

#if 1
#include <kern/rpc_inline.c> 
#endif

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

extern int switch_act_swapins;

/*
 * Do a direct RPC with only simple arguments.  Used by the
 * MiG client when short-circuiting within the same address
 * space, to effect stack/thread switching and control transfer.
 * The client code processes complex args.
 *
 * We assume port/portset and current shuttle are not locked
 * by caller, and leave them unlocked on exit.  Internally,
 * must lock shuttle before port/portset.
 *
 * NB:  Cleanliness has been sacrificed to efficiency in handling
 * the variable argument list of which portp indicates the
 * start.  On principle, we should use stdarg to access the
 * added arguments, but instead we depend on their being above
 * the port on the stack.
 */
kern_return_t
machine_rpc_simple(
	int		routine_num,
	int		argc,
	void		*portp)
{
	int			*new_argv;
	ipc_port_t		port = *(ipc_port_t *)portp;
	ipc_space_t		receiver = port->ip_receiver;
	kern_return_t		kr;
	mig_impl_routine_t	server_func;
	rpc_subsystem_t		subsystem = port->ip_subsystem;
	thread_act_t		old_act, new_act;
	thread_t		cur_thread = current_thread();
	vm_offset_t		new_sp;

	new_act = THR_ACT_NULL;


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
	 * We assume we're called in two cases:  a) On
	 * the syscall exception path, we're running on
	 * a kernel stack but in the client thread.
	 * b) When a server requests Mach services,
	 * we're running on the server shuttle's stack
	 * and in its thread.
	 *
	 * In case a), we switch threads and switch
	 * stacks (to the new thread's stack).  In case
	 * b), we don't switch threads, but switch
	 * stacks to the kernel stack for the current
	 * thread.
	 */
	if (receiver != ipc_space_kernel) {
		for (;;) {
			new_act = thread_pool_get_act(port);	/* may block */
#if	THREAD_SWAPPER
			if (new_act == THR_ACT_NULL ||
			    new_act->swap_state == TH_SW_IN ||
			    new_act->swap_state == TH_SW_UNSWAPPABLE) {
				/* got one ! */
				break;
			}
			/* ask for the activation to be swapped in */
			act_locked_act_reference(new_act);
			thread_pool_put_act(new_act);
			switch_act_swapins++;           /* stats */
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
			return(MACH_RCV_PORT_DIED);
		}
		rpc_lock(cur_thread);
		old_act = cur_thread->top_act;
		(void) switch_act(new_act);
		new_sp = new_act->user_stack;
		act_unlock(new_act);
		mp_disable_preemption();
	} else {
		rpc_lock(cur_thread);
		old_act = cur_thread->top_act;
		mp_disable_preemption();
		new_sp = kernel_stack[cpu_number()];
	}
	/*
	 * Now transfer the caller's arguments to the new stack.
	 * Note: gcc steadfastly considers new_sp and new_argv
	 * to be synonyms.  To keep new_sp untouched while
	 * (effectively) incrementing new_argv, the asm fragment
	 * below _must_ appear as it currently does -- i.e., must
	 * be new_argv into a register itself.  (Since new_argv
	 * isn't used following the asm, on principle it could be
	 * deleted and new_sp used instead.)
	 */
	new_argv = (int *)new_sp - argc;
	new_sp = (vm_offset_t)new_argv;
	/*
	 */
	__asm__ volatile(
		"movl %0,%%ecx; jecxz 1f; \
			movl %2, %%edx; 0: movl (%1),%%eax; addl $4, %1; \
			movl %%eax,(%%edx); addl $4, %%edx; loop 0b; 1:"
		: /* no outputs */
		: "g" (argc), "r" (portp), "g" (new_argv)
		: "%ecx", "%eax", "%edx");

	rpc_unlock(cur_thread);
	ip_unlock(port);
	act_unlock(new_act);

	/*
	 * Set the server function here.
	 */
	assert(subsystem != 0);
	server_func = subsystem->routine[routine_num -
					 subsystem->start].impl_routine;
	assert(server_func != 0);
	/*
	 * Switch stacks, call the function, and switch back.  Since
	 * we're switching stacks, the asm fragment can't use auto's
	 * for temporary storage; use a callee-saves register instead.
	 */
	__asm__ volatile(
		"movl %0, %%ebx; xchgl %%ebx,%%esp; \
			call %1; movl %%ebx,%%esp; movl %%eax,%2"
		: /* no outputs */
		: "g" (new_sp), "r" (server_func), "g" (kr)
		: "%ebx", "%eax", "%ecx", "%edx", "cc", "memory");
	mp_enable_preemption();

	/*
	 * Release our reference to the port/portset.
	 */
	ip_lock(port);
	ip_release(port);
	rpc_lock(cur_thread);

	/*
	 * If we switched threads coming in, switch again going out.
	 */
	if (receiver != ipc_space_kernel) {
		act_switch_swapcheck(cur_thread, port);
		assert(cur_thread->top_act == new_act);
		(void) switch_act(THR_ACT_NULL);
		assert(cur_thread->top_act == old_act);
	}

	rpc_unlock(cur_thread);
	ip_unlock(port);
	/*
	 * Check here for MIG_NO_REPLY return from server, and honor it.
	 * If thread should already halt, it will terminate at return to
	 * user mode; otherwise, block it here indefinitely.
	 */
	if (kr == MIG_NO_REPLY) {
		while (!thread_should_halt(cur_thread)) {
			/*
			 * Turn off TH_RUN to synch with thread_wait().
			 */
			thread_will_wait(cur_thread);
			thread_block((void (*)(void))0);
		}
	}
	return (kr);
}

#if MACH_PROF
/*
 * Put a "user" pc value into the current activation's PCB,
 * for later retrieval by profiling code.
 */
#define RECORD_USER_PC(act) 		\
MACRO_BEGIN 				\
	unsigned long _pc__; 		\
					\
	__asm__ volatile( 		\
		"movl 4(%%ebp),%0" 	\
		: "=r" (_pc__)); 	\
	user_pc(act) = _pc__; 		\
MACRO_END
#else
#define RECORD_USER_PC(act)
#endif

/*
 * XXX - need better way to access this
 */
vm_map_t port_name_to_map(mach_port_t);

#define switch_to_new_map(target, act, old, new)	\
	MACRO_BEGIN					\
	if ((new = port_name_to_map(target)) != NULL) {	\
	    old = act->map;				\
	    mp_disable_preemption();			\
	    PMAP_SWITCH_USER(act, new, cpu_number());	\
	    mp_enable_preemption();			\
	}						\
	MACRO_END

#define switch_to_old_map(act, old, new)		\
	MACRO_BEGIN					\
	mp_disable_preemption();			\
	PMAP_SWITCH_USER(act, old, cpu_number());	\
	mp_enable_preemption();				\
	vm_map_dealloc_fast(new);			\
	MACRO_END

/*
 * Encapsulate here the call to (and return from) kernel code:
 * switch stacks, check for AST's, and switch back again.
 */
#if NCPUS == 1

#define klkernel_call(entry, ksp, rv)                                   \
        __asm__ volatile(                                               \
                "movl %0,%%ebx;                                         \
                 xchgl %%ebx,%%esp;                                     \
                 call " CC_SYM_PREFIX entry ";                          \
                 movl %%eax,%1;                                         \
              8: cmpl $0," CC_SYM_PREFIX "need_ast;                     \
                 je 9f;                                                 \
                 pushl $0;                                              \
                 call " CC_SYM_PREFIX "i386_astintr;                    \
                 addl $4,%%esp;                                         \
                 jmp 8b;                                                \
              9: movl %%ebx,%%esp"                                      \
                : /* no outputs */                                      \
                : "g" (ksp), "g" (rv)                                   \
                : "%ebx", "%eax", "%ecx", "%edx", "cc", "memory")

#else	/* NCPUS > 1 */

#if	MACH_RT
#define CALL_MP_ENABLE_PREEMPTION \
	"call " CC_SYM_PREFIX "_mp_enable_preemption;"
#define CALL_MP_DISABLE_PREEMPTION \
	"call " CC_SYM_PREFIX "_mp_disable_preemption;"
#else	/* MACH_RT */
#define CALL_MP_ENABLE_PREEMPTION
#define CALL_MP_DISABLE_PREEMPTION
#endif	/* MACH_RT */

#define klkernel_call(entry, ksp, rv) 					\
MACRO_BEGIN                                                             \
	/* On MP, preemption is disabled at this point */		\
        __asm__ volatile(                                               \
                "movl %0,%%ebx;                                         \
                 xchgl %%ebx,%%esp;                                     \
		 " CALL_MP_ENABLE_PREEMPTION "				\
                 call " CC_SYM_PREFIX entry ";                          \
                 movl %%eax,%1;                                         \
              8: " CALL_MP_DISABLE_PREEMPTION "				\
		 call " CC_SYM_PREFIX "_cpu_number;			\
		 cmpl $0," CC_SYM_PREFIX "need_ast(,%%eax,4);           \
                 je 9f;                                                 \
		 " CALL_MP_ENABLE_PREEMPTION "				\
                 pushl $0;                                              \
                 call " CC_SYM_PREFIX "i386_astintr;                    \
                 addl $4,%%esp;                                         \
                 jmp 8b;                                                \
              9: movl %%ebx,%%esp"                                      \
                : /* no outputs */                                      \
                : "g" (ksp), "g" (rv)		                        \
                : "%ebx", "%eax", "%ecx", "%edx", "cc", "memory");      \
	/* On MP, preemption is still disabled */			\
MACRO_END
	
#endif /* NCPUS > 1 */

/*
 * Wrap kernel's copyin function for use from a kernel-loaded server.
 * We assume that we're running in a server thread on its user stack,
 * so just switch to the thread's kernel stack "around" the call.
 */
boolean_t
klcopyin(
	mach_port_t	target,
	void 		*argp)
{
	thread_act_t		act;
	boolean_t	rv;
	int		*new_argv;
	vm_map_t	oldmap, map;
	vm_offset_t	new_sp;

	assert(current_thread());
	act = current_act_fast();
	RECORD_USER_PC(act);
	switch_to_new_map(target, act, oldmap, map);
	if (map == NULL)
		return(TRUE);	/* Failure */
	mp_disable_preemption();
	new_sp = kernel_stack[cpu_number()];
	/*
	 * Now transfer the caller's arguments to the new stack.
	 * gcc doesn't seem to take the asm fragments below into
	 * account in doing def/ref calculations, so alter them
	 * at your own risk...
	 */
	new_argv = (int *)new_sp - 3;	/* XXX - count in "int's" */
	new_sp = (vm_offset_t)new_argv;
	__asm__ volatile(
		"movl %1, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; movl %%eax,(%%edx)"
		: /* no outputs */
		: "r" (argp), "g" (new_argv)
		: "%eax", "%edx");
	/*
	 * Switch stacks, call the function, and switch back.  Since
	 * we're switching stacks, the asm fragment can't use auto's
	 * for temporary storage; use a callee-saves register instead.
	 */
	klkernel_call("copyin", new_sp, rv);
	mp_enable_preemption();
	switch_to_old_map(act, oldmap, map);
	return (rv);
}

/*
 * Wrap kernel's copyout function for use from a kernel-loaded server.
 * We assume that we're running in a server thread on its user stack,
 * so just switch to the thread's kernel stack "around" the call.
 */
boolean_t
klcopyout(
	mach_port_t	target,
	void 		*argp)
{
	thread_act_t		act;
	boolean_t	rv;
	int		*new_argv;
	vm_map_t	oldmap, map;
	vm_offset_t	new_sp;

        assert(current_thread());
	act = current_act_fast();
	RECORD_USER_PC(act);
	switch_to_new_map(target, act, oldmap, map);
	if (map == NULL)
		return(TRUE);	/* Failure */
	mp_disable_preemption();
	new_sp = kernel_stack[cpu_number()];
	/*
	 * Now transfer the caller's arguments to the new stack.
	 * gcc doesn't seem to take the asm fragments below into
	 * account in doing def/ref calculations, so alter them
	 * at your own risk...
	 */
	new_argv = (int *)new_sp - 3;	/* XXX - count in "int's" */
	new_sp = (vm_offset_t)new_argv;
	__asm__ volatile(
		"movl %1, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; movl %%eax,(%%edx)"
		: /* no outputs */
		: "r" (argp), "g" (new_argv)
		: "%eax", "%edx");
	/*
	 * Switch stacks, call the function, and switch back.  Since
	 * we're switching stacks, the asm fragment can't use auto's
	 * for temporary storage; use a callee-saves register instead.
	 */
	klkernel_call("copyout", new_sp, rv);
	mp_enable_preemption();
	switch_to_old_map(act, oldmap, map);
	return (rv);
}

/*
 * Call the "kernel's" "copyin string" function from a kernel-loaded
 * server.  We assume that we're running in a server thread on its
 * user stack, so just switch to the thread's kernel stack "around"
 * the call.  The copyinstr function was created just for use from
 * this wrapper -- the kernel doesn't employ it.
 */
boolean_t
klcopyinstr(
	mach_port_t	target,
	void 		*argp)
{
	thread_act_t		act;
	boolean_t	rv;
	int		*new_argv;
	vm_map_t	oldmap, map;
	vm_offset_t	new_sp;

        assert(current_thread());
	act = current_act_fast();
	RECORD_USER_PC(act);
	switch_to_new_map(target, act, oldmap, map);
	if (map == NULL)
		return(TRUE);	/* Failure */
	mp_disable_preemption();
	new_sp = kernel_stack[cpu_number()];
	/*
	 * Now transfer the caller's arguments to the new stack.
	 * gcc doesn't seem to take the asm fragments below into
	 * account in doing def/ref calculations, so alter them
	 * at your own risk...
	 */
	new_argv = (int *)new_sp - 4;	/* XXX - count in "int's" */
	new_sp = (vm_offset_t)new_argv;
	__asm__ volatile(
		"movl %1, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; movl %%eax,(%%edx)"
		: /* no outputs */
		: "r" (argp), "g" (new_argv)
		: "%eax", "%edx");
	/*
	 * Switch stacks, call the function, and switch back.  Since
	 * we're switching stacks, the asm fragment can't use auto's
	 * for temporary storage; use a callee-saves register instead.
	 */
	klkernel_call("copyinstr", new_sp, rv);
	mp_enable_preemption();
	switch_to_old_map(act, oldmap, map);
	return (rv);
}

/*
 * Call the kernel's thread_switch function from a kernel-loaded
 * server.  We assume that we're running in a server thread on its
 * user stack, so just switch to the thread's kernel stack "around"
 * the call.
 */
kern_return_t
klthread_switch(
	void *argp)
{
	boolean_t	rv;
	int		*new_argv;
	vm_offset_t	new_sp;

	RECORD_USER_PC(current_act_fast());
	mp_disable_preemption();
	new_sp = kernel_stack[cpu_number()];
	/*
	 * Now transfer the caller's arguments to the new stack.
	 * gcc doesn't seem to take the asm fragments below into
	 * account in doing def/ref calculations, so alter them
	 * at your own risk...
	 */
	new_argv = (int *)new_sp - 3;	/* XXX - count in "int's" */
	new_sp = (vm_offset_t)new_argv;
	__asm__ volatile(
		"movl %1, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; addl $4, %0; \
			 movl %%eax,(%%edx); addl $4, %%edx; \
			movl (%0),%%eax; movl %%eax,(%%edx)"
		: /* no outputs */
		: "r" (argp), "g" (new_argv)
		: "%eax", "%edx");
	/*
	 * Switch stacks, call the function, and switch back.  Since
	 * we're switching stacks, the asm fragment can't use auto's
	 * for temporary storage; use a callee-saves register instead.
	 */
	klkernel_call("syscall_thread_switch", new_sp, rv);
	mp_enable_preemption();
	return (rv);
}

/*
 * Call the kernel's thread_depress_abort function from a kernel-loaded
 * server.  We assume that we're running in a server thread on its
 * user stack, so just switch to the thread's kernel stack "around"
 * the call.
 */
kern_return_t
klthread_depress_abort(
	mach_port_t		name)
{
	boolean_t	rv;
	int		*new_argv;
	vm_offset_t	new_sp;

	RECORD_USER_PC(current_act_fast());
	mp_disable_preemption();
	new_sp = kernel_stack[cpu_number()];
	/*
	 * Now transfer the caller's arguments to the new stack.
	 * gcc doesn't seem to take the asm fragments below into
	 * account in doing def/ref calculations, so alter them
	 * at your own risk...
	 */
	new_argv = (int *)new_sp - 1;	/* XXX - count in "int's" */
	new_sp = (vm_offset_t)new_argv;
	__asm__ volatile(
		"movl %0, %%edx; movl %%edx, %1"
		: /* no outputs */
		: "g" (name), "g" (*new_argv)
		: "%edx");
	/*
	 * Switch stacks, call the function, and switch back.  Since
	 * we're switching stacks, the asm fragment can't use auto's
	 * for temporary storage; use a callee-saves register instead.
	 */
	klkernel_call("syscall_thread_depress_abort", new_sp, rv);
	mp_enable_preemption();
	return (rv);
}
