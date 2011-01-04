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
#include <mach/machine.h>

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

#include <asm/vm86.h>
#include <asm/ptrace.h>
#include <asm/segment.h>

#define CF_MASK	0x00000001

typedef int (*sysfun_p)(struct pt_regs regs);
extern sysfun_p sys_call_table[];
extern void syscall_trace(void);
extern char *sys_call_name_table[];	/* forward */

/* syscall_wrapper is in entry.S */
extern void syscall_wrapper(sysfun_p func,
			    struct i386_saved_state *in_state,
			    struct pt_regs **regs_ptr,
			    unsigned long *orig_eax_ptr);

extern int do_signal(unsigned long oldmask, struct pt_regs *regs);

extern void do_page_fault(struct pt_regs *regs, unsigned long error_code);

extern void do_invalid_op(struct pt_regs *regs, long error_code);
extern void do_divide_error(struct pt_regs *regs, long error_code);
extern void do_debug(struct pt_regs *regs, long error_code);

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

int debug_invalid_op = 0;

extern cpu_type_t cpu_type;
extern cpu_subtype_t cpu_subtype;
extern char x86;
void
mach_trap_init(void)
{
	switch (cpu_type) {
	    case CPU_TYPE_I386:
		x86 = 3;
		break;
	    case CPU_TYPE_I486:
		x86 = 4;
		break;
	    case CPU_TYPE_PENTIUM:
		x86 = 5;
		break;
	    case CPU_TYPE_PENTIUMPRO:
		x86 = 6;
		break;
	}
	osfmach3_trap_init(EXCEPTION_STATE, i386_SAVED_STATE);
}

int
emulate_instruction(
	struct pt_regs	*regs,
	long		error_code)
{
	unsigned char	instr;
	boolean_t	data_16;

	data_16 = FALSE;

	if (debug_invalid_op) {
		Debugger("emulate_instruction");
	}

	for (;;) {
		if (verify_area(VERIFY_READ, (void *) regs->eip, 1))
			return -1;
		instr = get_fs_byte((const char *) regs->eip);

		switch ((unsigned int) instr) {
		    case 0x66:	/* data_16 prefix */
			data_16 = TRUE;
#if	CONFIG_OSFMACH3_DEBUG
			printk("emulate_instruction [P%d %s]: "
			       "skipped %d-byte \"data_16\" prefix at 0x%lx\n",
			       current->pid, current->comm,
			       1, regs->eip);
#endif	/* CONFIG_OSFMACH3_DEBUG */
			regs->eip++;
			break;

		    case 0xEE:	/* outw dx,al */
#if	CONFIG_OSFMACH3_DEBUG
			printk("emulate_instruction [P%d %s]: "
			       "skipped %d-byte \"outw dx(=0x%lx),al(=0x%lx)\""
			       "at 0x%lx\n",
			       current->pid, current->comm,
			       1, regs->edx, regs->eax, regs->eip);
#endif	/* CONFIG_OSFMACH3_DEBUG */
			regs->eip++;
			return 0;

		    case 0xEF:	/* outw dx,ax */
#if	CONFIG_OSFMACH3_DEBUG
			printk("emulate_instruction [P%d %s]: "
			       "skipped %d-byte \"outw dx(=0x%lx),ax(=0x%lx)\""
			       "at 0x%lx\n",
			       current->pid, current->comm,
			       1, regs->edx, regs->eax, regs->eip);
#endif	/* CONFIG_OSFMACH3_DEBUG */
			regs->eip++;
			return 0;

		    default:	/* others */
#if	CONFIG_OSFMACH3_DEBUG
			printk("emulate_instruction [P%d %s]: "
			       "invalid opcode 0x%x at 0x%lx\n",
			       current->pid, current->comm,
			       instr, regs->eip);
#endif	/* CONFIG_OSFMACH3_DEBUG */
			return -1;
		}
	}
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
	struct i386_saved_state	*in_state;
	struct i386_saved_state	*out_state;
	unsigned long		trapno;
	sysfun_p		func;
	struct pt_regs		*regs;
	struct pt_regs		private_regs;
	unsigned long		orig_eax;
#if	SYSCALLTRACE
	char			thread_name[100];
#endif	/* SYSCALLTRACE */
	long			error_code;
	int			i;
	mach_port_t		trap_port_name;
	boolean_t		dont_mess_with_state = FALSE;
	struct server_thread_priv_data *priv_datap;
	osfmach3_jmp_buf	jmp_buf;

	priv_datap = server_thread_get_priv_data(cthread_self());
	while (!priv_datap->activation_ready) {
		/*
		 * The cthread hasn't been wired to the activation yet.
		 * Wait until its' done (see new_thread_activation()).
		 */
		server_thread_yield(10);
	}

	ASSERT(*flavor == i386_SAVED_STATE);
	in_state = (struct i386_saved_state *) old_state;
	out_state = (struct i386_saved_state *) new_state;

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
		osfmach3_state_to_ptregs(in_state,
					 &private_regs,
					 (unsigned long) -1);
		die_if_kernel("LINUX SERVER EXCEPTION",
			      &private_regs,
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

	if (exc_type == EXC_SYSCALL) {
		orig_eax = in_state->eax;
		goto syscall;
	}

	/*
	 * That's a real user exception, not a syscall exception.
	 */
	trapno = ~0UL;
	orig_eax = (unsigned long) -1;

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
			goto ret_from_sys_call;
		}
		/*
		 * The debugger has sent the exception back to us.
		 * Go on with the exception processing.
		 */
	}

	osfmach3_state_to_ptregs(in_state, regs, orig_eax);

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
		break;
	    case EXC_BAD_ACCESS:
		current->osfmach3.thread->fault_address = 
			code_count > 1 ? code[1] : 0;
		do_page_fault(regs, error_code);
		break;
	    case EXC_BAD_INSTRUCTION:
		if (emulate_instruction(regs, error_code)) {
			/* emulation failed */
			do_invalid_op(regs, error_code);
		}
		break;
	    case EXC_ARITHMETIC:
		do_divide_error(regs, error_code);
		break;
	    case EXC_EMULATION:
		do_invalid_op(regs, error_code);	/* XXX */
		break;
	    case EXC_SOFTWARE:
		do_invalid_op(regs, error_code);
		break;
	    case EXC_BREAKPOINT:
		do_debug(regs, error_code);
		break;
	    case EXC_SYSCALL:
		if (regs->orig_eax == TASK_BY_PID_TRAPNO) {
			/* CMU's task_by_pid() syscall */
			goto syscall;
		}
		do_invalid_op(regs, error_code);	/* XXX */
		break;
	    case EXC_MACH_SYSCALL:
		do_invalid_op(regs, error_code);	/* XXX */
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
	osfmach3_ptregs_to_state(regs, in_state, &orig_eax);
	goto ret_from_sys_call;

    syscall:
	trapno = orig_eax;
	in_state->eax = -ENOSYS;

#if	SYSCALLTRACE
	if (trapno >= NR_syscalls || sys_call_name_table[trapno] == NULL) {
		sprintf(thread_name, "[P%d %s] %ld",
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
			printk("  [P%d %s] %ld(%x, %x, %x, %x, %x)\n",
			       current->pid, current->comm, 
			       trapno,
			       (int) in_state->ebx, (int) in_state->ecx,
			       (int) in_state->edx, (int) in_state->esi,
			       (int) in_state->edi);
		} else {
			printk("  [P%d %s] %s(%x, %x, %x, %x, %x)\n",
			       current->pid, current->comm, 
			       sys_call_name_table[trapno],
			       (int) in_state->ebx, (int) in_state->ecx,
			       (int) in_state->edx, (int) in_state->esi,
			       (int) in_state->edi);
		}
		if (syscalltrace_regs) {
			osfmach3_state_to_ptregs(in_state, regs, orig_eax);
			show_regs(regs);
		}
	}
#endif	/* SYSCALLTRACE */

	if (trapno >= NR_syscalls) {
		if (trapno == (unsigned long) TASK_BY_PID_TRAPNO) {
			extern int sys_task_by_pid(struct pt_regs regs);
			func = sys_task_by_pid;
		} else {
			goto ret_from_sys_call;
		}
	} else {
		func = sys_call_table[trapno];
		if (func == NULL) {
			goto ret_from_sys_call;
		}
	}

	in_state->efl &= ~CF_MASK;	/* clear carry - assume no errors */
#if 0
	current->errno = 0;
#endif

	if (current->flags & PF_TRACESYS) {
		if (trapno != (unsigned long) TASK_BY_PID_TRAPNO) {
			osfmach3_state_to_ptregs(in_state, regs, orig_eax);
			syscall_trace();
			osfmach3_ptregs_to_state(regs, in_state, &orig_eax);
		}
	}

	if (current == FIRST_TASK) {
		syscall_wrapper(func, in_state, &regs, &orig_eax);
	} else {
		syscall_wrapper(func, in_state,
				&current->osfmach3.thread->regs_ptr, &orig_eax);
	}

#if 0
	if (current->errno != 0) {
		in_state->eax = - current->errno;
		in_state->efl |= CF_MASK; /* set carry to indicate error */
	}
#endif


	if (current->flags & PF_TRACESYS) {
		if (trapno != (unsigned long) TASK_BY_PID_TRAPNO) {
			osfmach3_state_to_ptregs(in_state, regs, orig_eax);
			syscall_trace();
			osfmach3_ptregs_to_state(regs, in_state, &orig_eax);
		}
	}

    ret_from_sys_call:

	if (intr_count == 0 && (bh_active & bh_mask)) {
		intr_count++;
		do_bottom_half();
		intr_count--;
	}

	if (! dont_mess_with_state) {
		in_state->efl |= IF_MASK;
		in_state->efl &= ~NT_MASK;
	}

	current->osfmach3.thread->under_server_control = FALSE;

	if (current != FIRST_TASK) {
		if (current->signal & ~current->blocked) {
			osfmach3_state_to_ptregs(in_state, regs, orig_eax);
			do_signal(current->blocked, regs);
			osfmach3_ptregs_to_state(regs, in_state, &orig_eax);
		}
	}

#if	SYSCALLTRACE
	if (syscalltrace == -1 || syscalltrace == current->pid) {
		if (trapno >= NR_syscalls) {
			printk("  [P%d %s] %ld ==> %d (0x%x) %s\n",
			       current->pid, current->comm, 
			       trapno,
			       (int) in_state->eax, (int) in_state->eax,
			       ((long) in_state->eax < 0) ? "**ERROR**" : "");
		} else {
			printk("  [P%d %s] %s ==> %d (0x%x) %s\n",
			       current->pid, current->comm, 
			       sys_call_name_table[trapno],
			       (int) in_state->eax, (int) in_state->eax, 
			       ((long) in_state->eax < 0) ? "**ERROR**" : "");
		}
		if (syscalltrace_regs) {
			osfmach3_state_to_ptregs(in_state, regs, orig_eax);
			show_regs(regs);
		}
	}

	cthread_set_name(cthread_self(), "ux_server_loop");
#endif	/* SYSCALLTRACE */

	/*
	 * The following parts of the user state shouldn't be
	 * modified by the Linux server.
	 */
	if (out_state != in_state && !dont_mess_with_state) {
		out_state->gs = in_state->gs;
		out_state->fs = in_state->fs;
		out_state->es = in_state->es;
		out_state->ds = in_state->ds;
		out_state->cs = in_state->cs;
		out_state->esp = in_state->esp;	/* kernel stack pointer */
		out_state->trapno = in_state->trapno;
		out_state->err = in_state->err;
		out_state->ss = in_state->ss;
		out_state->v86_segs.v86_es = in_state->v86_segs.v86_es;
		out_state->v86_segs.v86_ds = in_state->v86_segs.v86_ds;
		out_state->v86_segs.v86_fs = in_state->v86_segs.v86_fs;
		out_state->v86_segs.v86_gs = in_state->v86_segs.v86_gs;
		out_state->argv_status = 0;

		/*
		 * The following registers might have been modified by
		 * the Linux server.
		 */
		out_state->edi = in_state->edi;
		out_state->esi = in_state->esi;
		out_state->ebp = in_state->ebp;
		out_state->ebx = in_state->ebx;
		out_state->edx = in_state->edx;
		out_state->ecx = in_state->ecx;
		out_state->eax = in_state->eax;
		out_state->eip = in_state->eip;
		out_state->efl = in_state->efl;
		out_state->uesp = in_state->uesp;
	}

	*ocnt = icnt;

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
	struct i386_saved_state	state;
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

	state_count = i386_SAVED_STATE_COUNT;
	server_thread_blocking(FALSE);
	kr = thread_get_state(task->osfmach3.thread->mach_thread_port,
			      i386_SAVED_STATE,
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

	flavor = i386_SAVED_STATE;
	if (use_activations)
		uniproc_exit();
	kr = catch_exception_raise_state(trap_port,
					 0,	/* exception type */
					 0,	/* codes[] */
					 0,	/* code_count */
					 &flavor,
					 (thread_state_t) &state,
					 i386_SAVED_STATE_COUNT,
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
	ASSERT(flavor == i386_SAVED_STATE);

	kr = thread_set_state(task->osfmach3.thread->mach_thread_port,
			      i386_SAVED_STATE,
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
	struct i386_saved_state	state;
	mach_msg_type_number_t	state_count;

	state_count = i386_SAVED_STATE_COUNT;
	server_thread_blocking(FALSE);
	kr = thread_get_state(thread_port,
			      i386_SAVED_STATE,
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

	state.argv_status = 0;

	/*
	 * The following registers might have been modified by
	 * the Linux server.
	 */
	state.edi = regs->edi;
	state.esi = regs->esi;
	state.ebp = regs->ebp;
	state.ebx = regs->ebx;
	state.edx = regs->edx;
	state.ecx = regs->ecx;
	state.eax = regs->eax;
	state.eip = regs->eip;
	state.cs = regs->cs;
	state.efl = regs->eflags;
	state.uesp = regs->esp;
	kr = thread_set_state(thread_port,
			      i386_SAVED_STATE,
			      (thread_state_t) &state,
			      i386_SAVED_STATE_COUNT);
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
	struct i386_saved_state	*in_state;
	struct i386_saved_state	*out_state;
	
	ASSERT(*flavor == i386_SAVED_STATE);
	in_state = (struct i386_saved_state *) old_state;
	out_state = (struct i386_saved_state *) new_state;

	switch (exc_type) {
	    case EXC_SYSCALL:
		/* "int $0x80" instruction */
		in_state->eip -= 2;
		break;
	    case EXC_MACH_SYSCALL:
		/* "lcall $0x7,0" instruction */
		in_state->eip -= 7;
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
	"old_mmap",		/* 90 */
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
	"select",
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
	"sched_get_priority_min",	/* 160 */
	"sched_rr_get_interval",
	"sys_nanosleep",
	"sys_mremap"
};

long abs(long i)
{
	return (i > 0) ? i : -i;
}
