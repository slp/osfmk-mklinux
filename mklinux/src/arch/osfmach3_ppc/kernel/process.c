/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  linux/arch/ppc/kernel/process.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Adapted for PowerPC by Gary Thomas
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

#include <linux/autoconf.h>

#ifdef	CONFIG_OSFMACH3
#include <mach/mach_interface.h>
#include <mach/mach_port.h>
#include <mach/mach_host.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/user_memory.h>
#endif	/* CONFIG_OSFMACH3 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/elf.h>
#include <linux/ptrace.h>

#include <asm/pgtable.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/io.h>

#include "ppc_machine.h"

#ifdef	CONFIG_OSFMACH3
#include <mach/mach_interface.h>
#include <mach/mach_port.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/uniproc.h>
#endif	/* CONFIG_OSFMACH3 */

#define SHOW_THREAD_COPIES
#undef  SHOW_THREAD_COPIES

#define SHOW_TASK_SWITCHES
#undef  SHOW_TASK_SWITCHES

int dump_fpu (struct pt_regs *regs, elf_fpregset_t *fpregs)
#ifdef	CONFIG_OSFMACH3
{
	kern_return_t		kr;
	mach_msg_type_number_t	count;
	mach_port_t		child_thread;

	/* assumption - layout of ppc_float_state
	 * and floating state in linux server are
	 * the same
	 */


	count = PPC_FLOAT_STATE_COUNT;
	child_thread = current->osfmach3.thread->mach_thread_port;

	server_thread_blocking(FALSE);
	kr = thread_get_state(child_thread,
			      PPC_FLOAT_STATE,
			      (thread_state_t)fpregs,
			      &count);
	server_thread_unblocking(FALSE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("dump_fpu: "
			     "thread_get_state(0x%x)",
			     child_thread));
	}

	return 1;
}
	
#else	/* CONFIG_OSFMACH3 */
dump_fpu()
{
	return (0);
}
#endif	/* CONFIG_OSFMACH3 */

#ifndef	CONFIG_OSFMACH3
void
switch_to(struct task_struct *prev, struct task_struct *new)
{
	struct pt_regs *regs;
	struct thread_struct *new_tss, *old_tss;
	int s = _disable_interrupts();
	regs = new->tss.ksp;
#ifdef SHOW_TASK_SWITCHES
	printk("Task %x(%d) -> %x(%d)", current, current->pid, new, new->pid);
	printk(" - IP: %x, SR: %x, SP: %x\n", regs->nip, regs->msr, regs);
	cnpause();
#endif
	new_tss = &new->tss;
	old_tss = &current->tss;
	current_set[0] = new;	/* FIX ME! */
	_switch(old_tss, new_tss);
#ifdef SHOW_TASK_SWITCHES
	printk("Back in task %x(%d)\n", current, current->pid);
#endif
	_enable_interrupts(s);
}

asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;

	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		schedule();
	}
}

void hard_reset_now(void)
{
	halt();
}
#endif	/* CONFIG_OSFMACH3 */

void show_regs(struct pt_regs * regs)
{
#ifdef	CONFIG_OSFMACH3
	struct pt_regs	*extra_regs;

	extra_regs = (struct pt_regs *) regs->pt_extra_regs;

	printk(" R0: 0x%08lx     R1: 0x%08lx     R2: 0x%08lx     R3: 0x%08lx\n",
	       regs->gpr[0], regs->gpr[1], regs->gpr[2], regs->gpr[3]);
	printk(" R4: 0x%08lx     R5: 0x%08lx     R6: 0x%08lx     R7: 0x%08lx\n",
	       regs->gpr[4], regs->gpr[5], regs->gpr[6], regs->gpr[7]);
	printk(" R8: 0x%08lx     R9: 0x%08lx    R10: 0x%08lx    R11: 0x%08lx\n",
	       regs->gpr[8], regs->gpr[9], regs->gpr[10], regs->gpr[11]);
	printk("R12: 0x%08lx    R13: 0x%08lx    R14: 0x%08lx    R15: 0x%08lx\n",
	       regs->gpr[12], regs->gpr[13], regs->gpr[14], regs->gpr[15]);
	printk("R16: 0x%08lx    R17: 0x%08lx    R18: 0x%08lx    R19: 0x%08lx\n",
	       regs->gpr[16], regs->gpr[17], regs->gpr[18], regs->gpr[19]);
	printk("R20: 0x%08lx    R21: 0x%08lx    R22: 0x%08lx    R23: 0x%08lx\n",
	       regs->gpr[20], regs->gpr[21], regs->gpr[22], regs->gpr[23]);
	printk("R24: 0x%08lx    R25: 0x%08lx    R26: 0x%08lx    R27: 0x%08lx\n",
	       regs->gpr[24], regs->gpr[25], regs->gpr[26], regs->gpr[27]);
	printk("R28: 0x%08lx    R29: 0x%08lx    R30: 0x%08lx    R31: 0x%08lx\n",
	       regs->gpr[28], regs->gpr[29], regs->gpr[30], regs->gpr[31]);
	printk("NIP: 0x%08lx    MSR: 0x%08lx    CTR: 0x%08lx   LINK: 0x%08lx\n",
	       regs->nip, regs->msr, regs->ctr, regs->link);
	printk("CCR: 0x%08lx    XER: 0x%08lx   TRAP: 0x%08lx    OR3: 0x%08lx\n",
	       regs->ccr, regs->xer,
	       extra_regs->trap, extra_regs->orig_gpr3);
#else	/* CONFIG_OSFMACH3 */
	_panic("show_regs");
#endif	/* CONFIG_OSFMACH3 */
}

#ifndef	CONFIG_OSFMACH3
/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
}

void flush_thread(void)
{
}

void
release_thread(struct task_struct *t)
{
}
#endif	/* CONFIG_OSFMACH3 */

/*
 * Copy a thread..
 */
void copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
	struct task_struct * p, struct pt_regs * regs)
{
#ifdef	CONFIG_OSFMACH3
	kern_return_t kr;
	mach_port_t child_task, child_thread;
	struct ppc_thread_state child_state;
	boolean_t inherit;
	boolean_t do_setup_thread;
	struct task_struct *parent;
	struct pt_regs *p_regs, *p_extra_regs;
	struct pt_regs *current_regs, *current_extra_regs;

	parent = p->p_opptr;

	if (clone_flags & CLONE_VM) {
		/*
		 * Use the same Mach task as the parent:
		 * just create a Mach thread.
		 */
		p->osfmach3.task->mach_task_count++;
		child_task = p->osfmach3.task->mach_task_port;
	} else {
		/*
		 * Create a new Mach task.
		 */
		if (clone_flags & CLONING_INIT) {
			/* creating an empty task: don't inherit memory */
			inherit = FALSE;
		} else {
			inherit = TRUE;
		}

		kr = task_create(parent->osfmach3.task->mach_task_port,
				 (ledger_port_array_t) 0, 0,
				 inherit, &child_task);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr, ("copy_thread: task_create"));
			panic("can't create task.");
		}

		p->osfmach3.task = (struct osfmach3_mach_task_struct *)
			kmalloc(sizeof (*p->osfmach3.task), GFP_KERNEL);
		if (!p->osfmach3.task) {
			panic("copy_thread: "
			      "can't allocate osfmach3_mach_task_struct.\n");
		}
		p->osfmach3.task->mach_task_count = 1;
		p->osfmach3.task->mach_task_port = child_task;
		p->osfmach3.task->mach_aware =
			parent->osfmach3.task->mach_aware;
		user_memory_task_init(p);
	}

	/*
	 * Update the "mm_mach_task" pointer.
	 */
	p->mm->mm_mach_task = p->osfmach3.task;

	if (clone_flags & CLONING_KERNEL) {
		child_thread = MACH_PORT_NULL;
		do_setup_thread = TRUE;
	} else if (parent != current) {
		/*
		 * Asynchronous fork: creating a Linux task for an
		 * existing Mach thread.
		 * See catch_exception_raise_state_identity for this
		 * special use of "usp".
		 */
		child_thread = (mach_port_t) usp;
		do_setup_thread = FALSE;	/* the thread already exists */
	} else {
		kr = thread_create(child_task, &child_thread);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr, ("copy_thread: thread_create"));
			panic("can't create thread");
		}
		do_setup_thread = TRUE;
	}

	p->osfmach3.thread = (struct osfmach3_mach_thread_struct *)
		kmalloc(sizeof (*p->osfmach3.thread), GFP_KERNEL);

	if (!p->osfmach3.thread) {
		panic("copy_thread: "
		      "can't allocate osfmach3_mach_thread_struct.\n");
	}
	p->osfmach3.thread->mach_thread_count = 1;
	p->osfmach3.thread->mach_thread_port = child_thread;
	p->osfmach3.thread->active_on_cthread = CTHREAD_NULL;
	p->osfmach3.thread->reg_fs = USER_DS;

	if (do_setup_thread) {
		osfmach3_set_priority(p);
	}

	/* setup notification port */
	osfmach3_notify_register(p);

	/* setup exception ports */
	osfmach3_trap_setup_task(p, EXCEPTION_STATE, PPC_THREAD_STATE);

	condition_init(&p->osfmach3.thread->mach_wait_channel);

	/* setup saved regs for ptrace */
	p_regs = &p->osfmach3.thread->regs;
	p->osfmach3.thread->regs_ptr = p_regs;
	current_regs = parent->osfmach3.thread->regs_ptr;
	/*
	 * Copy the regular registers (common to Mach and Linux).
	 */
	memcpy(p_regs,
	       current_regs,
	       (((int)(&current_regs->pt_extra_regs)) -
		((int)(current_regs))));
	/*
	 * Set up the pointer to the "extra regs".
	 */
	p_extra_regs = p_regs;
	current_extra_regs = (struct pt_regs *) current_regs->pt_extra_regs;
	p_regs->pt_extra_regs = (unsigned long) p_extra_regs;
	if (! current_extra_regs) {
		/*
		 * Kernel threads don't have extra registers
		 * and they don't share their Linux state with
		 * Mach's state, so use the same structure as
		 * the regular registers.
		 */
		current_extra_regs = current_regs;
	}
	/*
	 * Copy the "extra registers" (which exist only in Linux).
	 */
	memcpy(&p_extra_regs->pt_extra_regs + 1,
	       &current_extra_regs->pt_extra_regs + 1,
	       (sizeof (*regs) - (((int)(&p_extra_regs->pt_extra_regs+1)) -
				  ((int)(p_extra_regs)))));

	/* setup fake interrupt fields */
	p->osfmach3.thread->under_server_control = FALSE;
	p->osfmach3.thread->in_interrupt_list = FALSE;
	p->osfmach3.thread->fake_interrupt_count = 0;
	p->osfmach3.thread->in_fake_interrupt = FALSE;

	if (clone_flags & CLONING_KERNEL) {
		void **args;

		/*
		 * Creating a kernel "thread" via kernel_thread().
		 * Start a server cthread...
		 */
		ASSERT(usp == 0);
		args = (void **) regs;	/* see kernel_thread(). */
		args[2] = (void *) p;
		server_thread_start(server_thread_bootstrap, (void *) args);
	} else {
		launch_new_ux_server_loop(p);
	}

	if (clone_flags & CLONING_INIT) {
		/*
		 * Creating an empty task: no state yet, so don't initialize
		 * the registers nor resume the thread.
		 */
	} else if (clone_flags & CLONING_KERNEL) {
		/*
		 * Creating a kernel thread: its state will be setup by
		 * the cthread_fork() in server_thread_start().
		 */
	} else if (do_setup_thread) {
		osfmach3_ptregs_to_state(p_regs, &child_state, NULL);
		child_state.r3 = 0;		/* result from fork */
		child_state.r1 = usp;

		kr = thread_set_state(child_thread, PPC_THREAD_STATE,
				      (thread_state_t) &child_state,
				      PPC_THREAD_STATE_COUNT);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr, ("copy_thread: thread_set_state"));
			panic("can't set thread state");
		}
	}
#else	/* CONFIG_OSFMACH3 */
	int i;
	SEGREG *segs, *init_segs;
	struct pt_regs * childregs;
#ifdef SHOW_THREAD_COPIES
printk("copy thread - NR: %d, Flags: %x, USP: %x, Task: %x, Regs: %x\n", nr, clone_flags, usp, p, regs);
#endif
	/* Construct segment registers */
	segs = p->tss.segs;
	for (i = 0;  i < 8;  i++)
	{
		segs[i].ks = 0;
		segs[i].kp = 1;
		segs[i].vsid = i | (nr << 4);
	}
	if ((p->mm->context == 0) || (p->mm->count == 1))
	{
		p->mm->context = (nr<<4);
#ifdef SHOW_THREAD_COPIES
printk("Setting MM[%x] Context = %x Task = %x Current = %x/%x\n", p->mm, p->mm->context, p, current, current->mm);
#endif
	}
	/* Last 8 are shared with kernel & everybody else... */
	init_segs = init_task.tss.segs;
	for (i = 8;  i < 16;  i++)
	{
	  segs[i] = init_segs[i];
	}
#ifdef SHOW_THREAD_COPIES
	for (i = 0;  i < 16;  i++)
	  {
	    printk("SR[%02d] = %08X", i, *(long *)&segs[i]);
	    if ((i % 4) == 3) printk("\n"); else printk(", ");
	  }
cnpause();
#endif
	/* Copy registers */
#ifdef STACK_HAS_TWO_PAGES
	childregs = ((struct pt_regs *) (p->kernel_stack_page + 2*PAGE_SIZE)) - 2;
#else	
	childregs = ((struct pt_regs *) (p->kernel_stack_page + 1*PAGE_SIZE)) - 2;
#endif	
	*childregs = *regs;	/* STRUCT COPY */
	childregs->gpr[3] = 0;  /* Result from fork() */
	p->tss.ksp = childregs;
	if (usp >= (unsigned long)regs)
	{ /* Stack is in kernel space - must adjust */
		childregs->gpr[1] = childregs+1;
	} else
	{ /* Provided stack is in user space */
		childregs->gpr[1] = usp;
	}
#endif	/* CONFIG_OSFMACH3 */
}

/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
}

#if 0
/*
 * Do necessary setup to start up a newly executed thread.
 */
void start_thread(struct pt_regs * regs, unsigned long eip, unsigned long esp)
{
	regs->nip = eip;
	regs->gpr[1] = esp;
	regs->msr = MSR_USER;
#if 0
{
	int len;
	len = (unsigned long)0x80000000 - esp;
	if (len > 128) len = 128;
	printk("Start thread [%x] at PC: %x, SR: %x, SP: %x\n", regs, eip, regs->msr, esp);
	dump_buf(esp, len);
	dump_buf(eip, 0x80);
	cnpause();
}
#endif	
}
#endif

asmlinkage int sys_fork(int p1, int p2, int p3, int p4, int p5, int p6, struct pt_regs *regs)
{
	return do_fork(SIGCHLD, regs->gpr[1], regs);
}

/*
 * sys_execve() executes a new program.
 *
 * This works due to the PowerPC calling sequence: the first 6 args
 * are gotten from registers, while the rest is on the stack, so
 * we get a0-a5 for free, and then magically find "struct pt_regs"
 * on the stack for us..
 *
 * Don't do this at home.
 */
asmlinkage int sys_execve(unsigned long a0, unsigned long a1, unsigned long a2,
	unsigned long a3, unsigned long a4, unsigned long a5,
	struct pt_regs *regs)
{
	int error;
	char * filename;

	error = getname((char *) a0, &filename);
	if (error)
	{
#ifndef	CONFIG_OSFMACH3
printk("Error getting EXEC name: %d\n", error);		
#endif	/* CONFIG_OSFMACH3 */
		return error;
	}
#ifndef	CONFIG_OSFMACH3
	flush_instruction_cache();
#endif	/* CONFIG_OSFMACH3 */
	error = do_execve(filename, (char **) a1, (char **) a2, regs);
#if 0
if (error)
{	
printk("EXECVE - file = '%s', error = %d\n", filename, error);
}
#endif
	putname(filename);
	return error;
}

/*
 * This doesn't actually work correctly like this: we need to do the
 * same stack setups that fork() does first.
 */
asmlinkage int sys_clone(int p1, int p2, int p3, int p4, int p5, int p6, struct pt_regs *regs)
{
	unsigned long clone_flags = p1;
	int res;
	res = do_fork(clone_flags, regs->gpr[1], regs);
	return res;
}

void
print_backtrace(unsigned long *sp)
{
	int cnt = 0;
	printk("... Call backtrace:\n");
	while (*sp)
	{
#ifdef	CONFIG_OSFMACH3
		printk("%08lX ", sp[1]);
		sp = (unsigned long *) *sp;
#else	/* CONFIG_OSFMACH3 */
		printk("%08X ", sp[1]);
		sp = *sp;
#endif	/* CONFIG_OSFMACH3 */
		if (++cnt == 8)
		{
			printk("\n");
		}
		if (cnt > 16) break;
	}
	printk("\n");
}

#ifndef	CONFIG_OSFMACH3
void
print_user_backtrace(unsigned long *sp)
{
	int cnt = 0;
	printk("... [User] Call backtrace:\n");
	while (valid_addr(sp) && *sp)
	{
		printk("%08X ", sp[1]);
		sp = *sp;
		if (++cnt == 8)
		{
			printk("\n");
		}
		if (cnt > 16) break;
	}
	printk("\n");
}
#endif	/* CONFIG_OSFMACH3 */

void
print_kernel_backtrace(void)
{
	unsigned long *_get_SP(void);
	print_backtrace(_get_SP());
}

#ifdef	CONFIG_OSFMACH3
void start_thread(struct pt_regs * regs, unsigned long eip, unsigned long esp)
{
	regs->nip = eip;
	regs->gpr[1] = esp;
}
#endif	/* CONFIG_OSFMACH3 */
