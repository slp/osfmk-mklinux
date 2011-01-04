/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/exception.h>
#include <mach/thread_status.h>
#include <mach/mach_port.h>
#include <mach/mach_host.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#include <mach/message.h>
#include <mach/exc_server.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/parent_server.h>
#include <osfmach3/assert.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/serv_port.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/sys.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/interrupt.h>

#include <asm/ptrace.h>
#include <asm/segment.h>

#define SO_MASK	0x10000000

typedef int sysfun(int arg1, int arg2, int arg3, int arg4,
		   int arg5, int arg6, struct pt_regs *regs);
typedef sysfun *sysfun_p;
extern sysfun_p sys_call_table[];
extern void syscall_trace(void);
extern char *sys_call_name_table[];	/* forward */

extern int do_signal(unsigned long oldmask, struct pt_regs *regs);
extern void do_page_fault(struct pt_regs *regs,
			  unsigned long address,
			  unsigned long error_code);

extern void MachineCheckException(struct pt_regs *regs);
extern void ProgramCheckException(struct pt_regs *regs);
extern void SingleStepException(struct pt_regs *regs);
extern void FloatingPointCheckException(struct pt_regs *regs);
extern void AlignmentException(struct pt_regs *regs);

extern boolean_t in_kernel;
extern boolean_t use_activations;
extern mach_port_t exc_subsystem_port;
extern mach_port_t server_exception_port;
extern boolean_t server_exception_in_progress;

#if	CONFIG_OSFMACH3_DEBUG
#define SYSCALLTRACE 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#if	SYSCALLTRACE
int syscalltrace = -2;
int syscalltrace_regs = 0;
#endif	/* SYSCALLTRACE */

void
mach_trap_init(void)
{
	osfmach3_trap_init(EXCEPTION_STATE, PPC_THREAD_STATE);
}

int
emulate_instruction(
	struct pt_regs	*regs,
	long		error_code)
{
	return -1;
}

#define TASK_BY_PID_TRAPNO	-33

kern_return_t
catch_exception_raise_state(
	mach_port_t			port,
	exception_type_t		exc_type,
	exception_data_t		code,
	mach_msg_type_number_t		code_count,
	int				*flavor,
	thread_state_t			old_state,
	mach_msg_type_number_t		icnt,
	thread_state_t			new_state,
	mach_msg_type_number_t		*ocnt)
{
	struct ppc_thread_state	*in_state;
	struct ppc_thread_state	*out_state;
	unsigned long 		trapno;
	sysfun_p 		func;
	struct pt_regs 		*regs, *extra_regs;
	struct pt_regs 		private_regs;
#if	SYSCALLTRACE
	char 			thread_name[100];
#endif	/* SYSCALLTRACE */
	long 			error_code;
	int 			i;
	mach_port_t		trap_port_name;
	boolean_t		dont_mess_with_state = FALSE;
	struct server_thread_priv_data *priv_datap;
	osfmach3_jmp_buf	jmp_buf;
	unsigned long		orig_gpr3;

	priv_datap = server_thread_get_priv_data(cthread_self());
	while (!priv_datap->activation_ready) {
		/*
		 * The cthread hasn't been wired to the activation yet.
		 * Wait until its' done (see new_thread_activation()).
		 */
		server_thread_yield(10);
	}

	ASSERT(*flavor == PPC_THREAD_STATE);
	in_state = (struct ppc_thread_state *) old_state;
	out_state = (struct ppc_thread_state *) new_state;

	if (port == server_exception_port) {
		extern void die_if_kernel(char *str,
					  struct pt_regs *regs,
					  long err);
		extern void console_verbose(void);

		/*
		 * Oops: a server exception...
		 */
		console_verbose();
		server_exception_in_progress = TRUE;
		printk("*** LINUX SERVER EXCEPTION ***\n");
		switch (exc_type) {
		    case 0:
			printk("*** EXCEPTION 0 (fake interrupt)\n");
			break;
		    case EXC_BAD_ACCESS:
			printk("*** EXC_BAD_ACCESS\n");
			break;
		    case EXC_BAD_INSTRUCTION:
			printk("*** EXC_BAD_INSTRUCTION\n");
			break;
		    case EXC_ARITHMETIC:
			printk("*** EXC_ARITHMETIC\n");
			break;
		    case EXC_EMULATION:
			printk("*** EXC_EMULATION\n");
			break;
		    case EXC_SOFTWARE:
			printk("*** EXC_SOFTWARE\n");
			break;
		    case EXC_BREAKPOINT:
			printk("*** EXC_BREAKPOINT\n");
			break;
		    case EXC_SYSCALL:
			printk("*** EXC_SYSCALL\n");
			break;
		    case EXC_MACH_SYSCALL:
			printk("*** EXC_MACH_SYSCALL\n");
			break;
		    case EXC_RPC_ALERT:
			printk("*** EXC_RPC_ALERT\n");
			break;
		    default:
			printk("*** UNKNOWN EXCEPTION %d\n", exc_type);
			break;
		}
		for (i = 0; i < code_count; i++) {
			printk("*** CODE[%d] = %d (0x%x)\n",
			       i, code[i], code[i]);
		}
		regs = &private_regs;
		extra_regs = regs;
		osfmach3_state_to_ptregs(in_state,
					 regs,
					 extra_regs,
					 (unsigned long) -1);
		die_if_kernel("LINUX SERVER EXCEPTION",
			      regs,
			      0);
		panic("LINUX SERVER EXCEPTION");
		server_exception_in_progress = FALSE;
		return KERN_SUCCESS;	/* just in case... */
	}

	if (use_activations) {
		/*
		 * We come here directly as the result of a thread
		 * migration and short-circuited exception RPC.
		 */

		mutex_lock(&uniproc_antechamber_mutex);
		uniproc_enter();
		mutex_unlock(&uniproc_antechamber_mutex);

		/*
		 * As an optimization, we only do the setjmp
		 * if it hasn't been left ready-to-use by a previous
		 * use of this routine from this c-thread.
		 *
		 * If we're not called from the same context as the
		 * last call (jmp_buf not at the same place in the 
		 * stack), we need to do a setjmp again to update
		 * the jump buffer.
		 */
		if (priv_datap->jmp_buf != &jmp_buf) {
			priv_datap->jmp_buf = &jmp_buf;
			if (osfmach3_setjmp(priv_datap->jmp_buf)) {
				/*
				 * The user task is being terminated.
				 */
				uniproc_exit();
				return MIG_NO_REPLY;
			}
		}
	}
		
	ASSERT(current == FIRST_TASK);
	trap_port_name = (mach_port_t) serv_port_name(port);
	uniproc_switch_to(current, (struct task_struct *) (trap_port_name - 1));

	current->osfmach3.thread->under_server_control = TRUE;
	if (current == FIRST_TASK) {
		/*
		 * The server is processing one of its own syscalls.
		 * Don't mess with the task's registers.
		 */
		regs = &private_regs;
	} else {
		/* ptr to task regs storage */
		regs = current->osfmach3.thread->regs_ptr;
	}

	if (in_state == out_state) {
		/*
		 * Use the Mach thread state directly. Still use the private
		 * registers as space holder for the extra regs.
		 */
		extra_regs = regs;
		regs = (struct pt_regs *) in_state;
		current->osfmach3.thread->regs_ptr = regs;
	} else {
		extra_regs = regs;
	}

	if (exc_type == EXC_SYSCALL) {
		orig_gpr3 = in_state->r3;
		goto syscall;
	}

	/*
	 * That's a real user exception, not a syscall exception.
	 */
	trapno = ~0UL;
	orig_gpr3 = (unsigned long) -1;

	/*
	 * Forward the exception if a debugger requested it.
	 * Don't do that for "fake interrupt" exceptions of course !
	 */
	if (exc_type != 0) {
		osfmach3_trap_forward(exc_type, code, code_count, flavor,
				      old_state, &icnt,
				      new_state, ocnt);
		if (current->osfmach3.thread->exception_completed) {
			/*
			 * The exception has been fully processed
			 * by the debugger, so just return to user mode
			 * without processing it again.
			 */
			dont_mess_with_state = TRUE;
			goto ret_from_exception;
		}
		/*
		 * The debugger has sent the exception back to us.
		 * Go on with the exception processing.
		 */
	}

	osfmach3_state_to_ptregs(in_state, regs, extra_regs, orig_gpr3);

#if	SYSCALLTRACE
	sprintf(thread_name, "[P%d %s] EXC %d",
		current->pid,
		current->comm,
		exc_type);
	cthread_set_name(cthread_self(), thread_name);
	if (syscalltrace == -1 || syscalltrace == current->pid) {
		printk("  [P%d %s] EXC %d(%d, %d)\n",
		       current->pid, current->comm, 
		       exc_type,
		       code_count > 0 ? code[0] : 0,
		       code_count > 1 ? code[1] : 0);
		if (syscalltrace_regs)
			show_regs(regs);
	}
#endif	/* SYSCALLTRACE */
	if (code_count > 0) {
		error_code = code[0];
	}  else {
		error_code = 0;
	}
	switch (exc_type) {
	    case 0:
		/* fake interrupt: nothing to do */
		current->osfmach3.thread->in_fake_interrupt = TRUE;
		extra_regs->trap = 0;
		break;
	    case EXC_BAD_ACCESS:
		extra_regs->trap = 0x0300;
		current->osfmach3.thread->fault_address = 
			code_count > 1 ? code[1] : 0;
		do_page_fault(regs,
			      current->osfmach3.thread->fault_address,
			      error_code);
		break;
	    case EXC_BAD_INSTRUCTION:
		extra_regs->trap = 0x0700;
		ProgramCheckException(regs);
		break;
	    case EXC_ARITHMETIC:
		extra_regs->trap = 0x0800;
		FloatingPointCheckException(regs);
		break;
	    case EXC_EMULATION:
		extra_regs->trap = 0x0700;
		ProgramCheckException(regs);
		break;
	    case EXC_SOFTWARE:
		extra_regs->trap = 0x0700;
		ProgramCheckException(regs);
		break;
	    case EXC_BREAKPOINT:
		if (code[0] == EXC_PPC_TRACE) {
			extra_regs->trap = 0x0d00;
			SingleStepException(regs);
		} else {
			extra_regs->trap = 0x0700;
			ProgramCheckException(regs);
		}
		break;
	    case EXC_MACH_SYSCALL:
		extra_regs->trap = 0x0700;
		ProgramCheckException(regs);
		break;
	    case EXC_RPC_ALERT:
		/* this shouldn't reach the server */
		panic("catch_exception_raise_state: "
		      "EXC_RPC_ALERT");
	    default:
		printk("Unknown exception %d  code_count %d\n",
		       exc_type, code_count);
		for (i = 0; i < code_count; i++) {
			printk("code[%d] = %d (0x%x)\n",
			       i, code[i], code[i]);
		}
		panic("catch_exception_raise_state: "
		      "unknown exception");
	}
	osfmach3_ptregs_to_state(regs, in_state, &orig_gpr3);
	goto ret_from_exception;

    syscall:

	osfmach3_state_to_ptregs(in_state, regs, extra_regs, orig_gpr3);
	extra_regs->trap = 0x0c00;
	trapno = regs->gpr[0];

#if	SYSCALLTRACE
	if (trapno >= NR_syscalls || sys_call_name_table[trapno] == NULL) {
		sprintf(thread_name, "[P%d %s] 0x%lx",
			current->pid,
			current->comm,
			trapno);
	} else {
		sprintf(thread_name, "[P%d %s] %s",
			current->pid,
			current->comm,
			sys_call_name_table[trapno]);
	}
	cthread_set_name(cthread_self(), thread_name);

	if (syscalltrace == -1 || syscalltrace == current->pid) {
		if (trapno >= NR_syscalls ||
		    sys_call_name_table[trapno] == NULL) {
			printk("  [P%d %s] 0x%lx(%x, %x, %x, %x, %x)\n",
			       current->pid, current->comm, 
			       trapno,
			       (int) regs->gpr[3], (int) regs->gpr[4],
			       (int) regs->gpr[5], (int) regs->gpr[6],
			       (int) regs->gpr[7]);
		} else {
			printk("  [P%d %s] %s(%x, %x, %x, %x, %x)\n",
			       current->pid, current->comm, 
			       sys_call_name_table[trapno],
			       (int) regs->gpr[3], (int) regs->gpr[4],
			       (int) regs->gpr[5], (int) regs->gpr[6],
			       (int) regs->gpr[7]);
		}
		if (syscalltrace_regs)
			show_regs(regs);
	}
#endif	/* SYSCALLTRACE */

	if (trapno >= NR_syscalls) {
		if (trapno == (unsigned long) TASK_BY_PID_TRAPNO) {
			extern sysfun sys_task_by_pid;
			func = sys_task_by_pid;
		} else if (trapno == 0x7777) {
			/* sigreturn: see below */
			func = NULL;
		} else {
			extra_regs->result = -ENOSYS;
			goto ret_from_sys_call;
		}
	} else {
		func = sys_call_table[trapno];
		if (func == NULL) {
			extra_regs->result = -ENOSYS;
			goto ret_from_sys_call;
		}
	}

	regs->ccr &= ~SO_MASK;	/* clear SO - assume no errors */

	/* Special case for 'sys_sigreturn' */
	if (trapno == 0x7777) {
		extern int sys_sigreturn(struct pt_regs *regs);
		extra_regs->result = sys_sigreturn(regs);
		/* Check for restarted system call */
		if ((long) extra_regs->result >= 0) {
			goto ret_from_sigreturn_restart;
		} else {
			goto ret_from_sigreturn;
		}
	}

	if (current->flags & PF_TRACESYS) {
		if (trapno != (unsigned long) TASK_BY_PID_TRAPNO)
			syscall_trace();
	}

	extra_regs->result = (*func)(regs->gpr[3], regs->gpr[4],
				     regs->gpr[5], regs->gpr[6],
				     regs->gpr[7], regs->gpr[8],
				     regs);

    ret_from_sigreturn:
    ret_from_sys_call:

	/* note: extra_regs->result is an "unsigned long", so cast it */
	if ((long) extra_regs->result < 0 &&
	    (- (long) extra_regs->result) <= _LAST_ERRNO) {
		regs->ccr |= SO_MASK; /* set SO to indicate error */
		regs->gpr[3] = - (long) extra_regs->result;
	} else {
		regs->gpr[3] = extra_regs->result;
	}

	if (current->flags & PF_TRACESYS) {
		if (trapno != (unsigned long) TASK_BY_PID_TRAPNO)
			syscall_trace();
	}

    ret_from_sigreturn_restart:
    ret_from_exception:

	if (intr_count == 0 && (bh_active & bh_mask)) {
		intr_count++;
		do_bottom_half();
		intr_count--;
	}

	current->osfmach3.thread->under_server_control = FALSE;

	if (current != FIRST_TASK) {
		if (current->signal & ~current->blocked) {
			do_signal(current->blocked, regs);
		}
	}

#if	SYSCALLTRACE
	if (syscalltrace == -1 || syscalltrace == current->pid) {
		if (trapno >= NR_syscalls) {
			printk("  [P%d %s] 0x%lx ==> %d (0x%x) %s\n",
			       current->pid, current->comm, 
			       trapno,
			       (int) regs->gpr[3], (int) regs->gpr[3],
			       (regs->ccr & SO_MASK) ? "**ERROR**" : "");
		} else {
			printk("  [P%d %s] %s ==> %d (0x%x) %s\n",
			       current->pid, current->comm, 
			       sys_call_name_table[trapno],
			       (int) regs->gpr[3], (int) regs->gpr[3], 
			       (regs->ccr & SO_MASK) ? "**ERROR**" : "");
		}
		if (syscalltrace_regs)
			show_regs(regs);
	}

	cthread_set_name(cthread_self(), "ux_server_loop");
#endif	/* SYSCALLTRACE */

	if (! dont_mess_with_state) {
		osfmach3_ptregs_to_state(regs, out_state, NULL);
		/*
		 * The following parts of the user state shouldn't be
		 * modified by the Linux server.
		 */
		out_state->mq = in_state->mq;
		out_state->ctr = in_state->ctr;

		*ocnt = icnt;
	}

	current->osfmach3.thread->regs_ptr = &current->osfmach3.thread->regs;

	uniproc_switch_to(current, FIRST_TASK);
	ASSERT(current == FIRST_TASK);

	if (use_activations) {
		uniproc_exit();
	}

	return KERN_SUCCESS;
}

kern_return_t
serv_callback_fake_interrupt(
	mach_port_t	trap_port)
{
	struct task_struct 	*task;
	struct ppc_thread_state	state;
	mach_msg_type_number_t	state_count;
	kern_return_t		kr;
	thread_state_flavor_t	flavor;
	mach_port_t		trap_port_name;

	trap_port_name = (mach_port_t) serv_port_name(trap_port);
	task = (struct task_struct *) (trap_port_name - 1);
	while (task->osfmach3.thread->fake_interrupt_count > 1) {
		if (task->state == TASK_ZOMBIE) {
			task->osfmach3.thread->fake_interrupt_count--;
			return KERN_SUCCESS;
		}
		osfmach3_yield();
	}
	if (task->state == TASK_ZOMBIE) {
		task->osfmach3.thread->fake_interrupt_count--;
		return KERN_SUCCESS;
	}

	state_count = PPC_THREAD_STATE_COUNT;
	server_thread_blocking(FALSE);
	kr = thread_get_state(task->osfmach3.thread->mach_thread_port,
			      PPC_THREAD_STATE,
			      (thread_state_t) &state,
			      &state_count);
	server_thread_unblocking(FALSE);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr,
				    ("serv_callback_fake_interrupt(0x%p): "
				     "thread_get_state(0%x)",
				     task,
				     task->osfmach3.thread->mach_thread_port));
			task->osfmach3.thread->fake_interrupt_count--;
			return kr;
		}
	}

	flavor = PPC_THREAD_STATE;
	if (use_activations)
		uniproc_exit();
	kr = catch_exception_raise_state(trap_port,
					 0,	/* exception type */
					 0,	/* codes[] */
					 0,	/* code_count */
					 &flavor,
					 (thread_state_t) &state,
					 PPC_THREAD_STATE_COUNT,
					 (thread_state_t) &state,
					 &state_count);
	if (use_activations)
		uniproc_enter();
	if (kr != KERN_SUCCESS) {
		if (kr == MIG_NO_REPLY) {
			/*
			 * Task has been terminated.
			 * Just return.
			 */
			return KERN_SUCCESS;
		}
		MACH3_DEBUG(1, kr, ("serv_callback_fake_interrupt(0x%p): "
				    "catch_exception_raise_state",
				    task));
		ASSERT(task->osfmach3.thread->in_fake_interrupt);
		ASSERT(task->osfmach3.thread->fake_interrupt_count > 0);
		task->osfmach3.thread->in_fake_interrupt = FALSE;
		task->osfmach3.thread->fake_interrupt_count--;
		return kr;
	}
	ASSERT(flavor == PPC_THREAD_STATE);

	kr = thread_set_state(task->osfmach3.thread->mach_thread_port,
			      PPC_THREAD_STATE,
			      (thread_state_t) &state,
			      state_count);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr,
				    ("serv_callback_fake_interrupt(0x%p): "
				     "thread_set_state(0x%x)",
				     task,
				     task->osfmach3.thread->mach_thread_port));
			ASSERT(task->osfmach3.thread->in_fake_interrupt);
			ASSERT(task->osfmach3.thread->fake_interrupt_count > 0);
			task->osfmach3.thread->in_fake_interrupt = FALSE;
			task->osfmach3.thread->fake_interrupt_count--;
			return kr;
		}
	}

	ASSERT(task->osfmach3.thread->in_fake_interrupt);
	ASSERT(task->osfmach3.thread->fake_interrupt_count > 0);
	task->osfmach3.thread->in_fake_interrupt = FALSE;
	task->osfmach3.thread->fake_interrupt_count--;

	kr = thread_resume(task->osfmach3.thread->mach_thread_port);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr,
				    ("serv_callback_fake_interrupt(0x%p): "
				     "thread_resume(0x%x)",
				     task,
				     task->osfmach3.thread->mach_thread_port));
		}
	}

	return KERN_SUCCESS;
}

kern_return_t
serv_callback_fake_interrupt_rpc(
	mach_port_t	trap_port)
{
	return serv_callback_fake_interrupt(trap_port);
}

kern_return_t
osfmach3_thread_set_state(
	mach_port_t	thread_port,
	struct pt_regs	*regs)
{
	kern_return_t		kr;
	struct ppc_thread_state	state;
	mach_msg_type_number_t	state_count;

	state_count = PPC_THREAD_STATE_COUNT;
	server_thread_blocking(FALSE);
	kr = thread_get_state(thread_port,
			      PPC_THREAD_STATE,
			      (thread_state_t) &state,
			      &state_count);
	server_thread_unblocking(FALSE);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr, ("osfmach3_thread_set_state: "
					    "thread_get_state(0%x)",
					    thread_port));
			return kr;
		}
	}

#if 0
	/*
	 * The following parts of the user state shouldn't be
	 * modified by the Linux server.
	 */
	state.mq = regs->mq;
	state.ctr = regs->ctr;
#endif

	/*
	 * The following registers might have been modified by
	 * the Linux server.
	 */
	state.srr0 = regs->nip;
	state.srr1 = regs->msr;
	state.r0 = regs->gpr[0];
	state.r1 = regs->gpr[1];
	state.r2 = regs->gpr[2];
	state.r3 = regs->gpr[3];
	state.r4 = regs->gpr[4];
	state.r5 = regs->gpr[5];
	state.r6 = regs->gpr[6];
	state.r7 = regs->gpr[7];
	state.r8 = regs->gpr[8];
	state.r9 = regs->gpr[9];
	state.r10 = regs->gpr[10];
	state.r11 = regs->gpr[11];
	state.r12 = regs->gpr[12];
	state.r13 = regs->gpr[13];
	state.r14 = regs->gpr[14];
	state.r15 = regs->gpr[15];
	state.r16 = regs->gpr[16];
	state.r17 = regs->gpr[17];
	state.r18 = regs->gpr[18];
	state.r19 = regs->gpr[19];
	state.r20 = regs->gpr[20];
	state.r21 = regs->gpr[21];
	state.r22 = regs->gpr[22];
	state.r23 = regs->gpr[23];
	state.r24 = regs->gpr[24];
	state.r25 = regs->gpr[25];
	state.r26 = regs->gpr[26];
	state.r27 = regs->gpr[27];
	state.r28 = regs->gpr[28];
	state.r29 = regs->gpr[29];
	state.r30 = regs->gpr[30];
	state.r31 = regs->gpr[31];
	state.cr = regs->ccr;
	state.xer = regs->xer;
	state.lr = regs->link;

	kr = thread_set_state(thread_port,
			      PPC_THREAD_STATE,
			      (thread_state_t) &state,
			      PPC_THREAD_STATE_COUNT);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr, ("osfmach3_thread_set_state: "
					    "thread_set_state(0x%x)",
					    thread_port));
			return kr;
		}
	}

	return KERN_SUCCESS;
}

kern_return_t
osfmach3_trap_unwind(
	mach_port_t			trap_port,
	exception_type_t		exc_type,
	exception_data_t		code,
	mach_msg_type_number_t		code_count,
	int				*flavor,
	thread_state_t			old_state,
	mach_msg_type_number_t		icnt,
	thread_state_t			new_state,
	mach_msg_type_number_t 		*ocnt)
{
	struct ppc_thread_state	*in_state;
	struct ppc_thread_state	*out_state;
	
	ASSERT(*flavor == PPC_THREAD_STATE);
	in_state = (struct ppc_thread_state *) old_state;
	out_state = (struct ppc_thread_state *) new_state;

	switch (exc_type) {
	    case EXC_SYSCALL:
	    case EXC_MACH_SYSCALL:
		/* "sc" instruction */
		in_state->srr0 -= 4;
		break;
	    default:
		/* regular exception: PC still on faulting instruction */
		break;
	}
	if (out_state != in_state) {
		memcpy(out_state, in_state, icnt * sizeof (int));
	}
	*ocnt = icnt;

	return KERN_SUCCESS;
}

char * sys_call_name_table[] = {
	"setup",		/* 0 */
	"exit",
	"fork",
	"read",
	"write",
	"open",			/* 5 */
	"close",
	"waitpid",
	"creat",
	"link",
	"unlink",		/* 10 */
	"execve",
	"chdir",
	"time",
	"mknod",
	"chmod",		/* 15 */
	"chown",
	"break",
	"stat",
	"lseek",
	"getpid",		/* 20 */
	"mount",
	"umount",
	"setuid",
	"getuid",
	"stime",		/* 25 */
	"ptrace",
	"alarm",
	"fstat",
	"pause",
	"utime",		/* 30 */
	"stty",
	"gtty",
	"access",
	"nice",
	"ftime",		/* 35 */
	"sync",
	"kill",
	"rename",
	"mkdir",
	"rmdir",		/* 40 */
	"dup",
	"pipe",
	"times",
	"prof",
	"brk",			/* 45 */
	"setgid",
	"getgid",
	"signal",
	"geteuid",
	"getegid",		/* 50 */
	"acct",
	"phys",
	"lock",
	"ioctl",
	"fcntl",		/* 55 */
	"mpx",
	"setpgid",
	"ulimit",
	"olduname",
	"umask",		/* 60 */
	"chroot",
	"ustat",
	"dup2",
	"getppid",
	"getpgrp",		/* 65 */
	"setsid",
	"sigaction",
	"sgetmask",
	"ssetmask",
	"setreuid",		/* 70 */
	"setregid",
	"sigsuspend",
	"sigpending",
	"sethostname",
	"setrlimit",		/* 75 */
	"getrlimit",
	"getrusage",
	"gettimeofday",
	"settimeofday",
	"getgroups",		/* 80 */
	"setgroups",
	"old_select",
	"symlink",
	"lstat",
	"readlink",		/* 85 */
	"uselib",
	"swapon",
	"reboot",
	"old_readdir",
	"old_mmap",			/* 90 */
	"munmap",
	"truncate",
	"ftruncate",
	"fchmod",
	"fchown",		/* 95 */
	"getpriority",
	"setpriority",
	"profil",
	"statfs",
	"fstatfs",		/* 100 */
	"ioperm",
	"socketcall",
	"syslog",
	"setitimer",
	"getitimer",		/* 105 */
	"newstat",
	"newlstat",
	"newfstat",
	"uname",
	"iopl",			/* 110 */
	"vhangup",
	"idle",
	"vm86",
	"wait4",
	"swapoff",		/* 115 */
	"sysinfo",
	"ipc",
	"fsync",
	"sigreturn",
	"clone",		/* 120 */
	"setdomainname",
	"newuname",
	"modify_ldt",
	"adjtimex",
	"mprotect",		/* 125 */
	"sigprocmask",
	"create_module",
	"init_module",
	"delete_module",
	"get_kernel_syms",	/* 130 */
	"quotactl",
	"getpgid",
	"fchdir",
	"bdflush",
	"sysfs",		/* 135 */
	"personality",
	"(afs_syscall)",
	"setfsuid",
	"setfsgid",
	"llseek",		/* 140 */
	"getdents",
	"newselect",
	"flock",
	"msync",
	"readv",		/* 145 */
	"writev",
	"getsid",
	"fdatasync",
	"sysctl",
	"mlock",		/* 150 */
	"munlock",
	"mlockall",
	"munlockall",
	"sched_setparam",
	"sched_getparam",	/* 155 */
	"sched_setscheduler",
	"sched_getscheduler",
	"sched_yield",
	"sched_get_priority_max",
	"sched_get_priority_min", /* 160 */
	"sched_rr_get_interval",
	"nanosleep",
	"mremap"
};
